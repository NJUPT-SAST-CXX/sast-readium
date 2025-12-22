#include "PluginManager.h"
#include <QApplication>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QWidget>
#include <algorithm>
#include <functional>
#include "../logging/LoggingMacros.h"
#include "IAnnotationPlugin.h"
#include "ICacheStrategyPlugin.h"
#include "IDocumentProcessorPlugin.h"
#include "IRenderPlugin.h"
#include "ISearchPlugin.h"
#include "PluginHookPoint.h"
#include "PluginHookRegistry.h"
#include "PluginInterface.h"

// Static instance
PluginManager* PluginManager::s_instance = nullptr;

// PluginDependencyResolver Implementation
QStringList PluginDependencyResolver::resolveDependencies(
    const QHash<QString, PluginMetadata>& plugins) {
    QStringList result;
    QHash<QString, int> visited;  // 0 = not visited, 1 = visiting, 2 = visited

    for (auto it = plugins.begin(); it != plugins.end(); ++it) {
        if (visited[it.key()] == 0) {
            visitPlugin(it.key(), plugins, visited, result);
        }
    }

    return result;
}

bool PluginDependencyResolver::hasCyclicDependencies(
    const QHash<QString, PluginMetadata>& plugins) {
    QHash<QString, int> visited;  // 0 = not visited, 1 = visiting, 2 = visited

    // Depth-first search with explicit cycle detection
    std::function<bool(const QString&)> dfs = [&](const QString& name) -> bool {
        const int state = visited.value(name, 0);
        if (state == 1) {
            // Found a back edge => cycle
            return true;
        }
        if (state == 2) {
            return false;
        }
        visited[name] = 1;  // mark visiting
        if (plugins.contains(name)) {
            const PluginMetadata& md = plugins[name];
            for (const QString& dep : md.dependencies) {
                // Ignore missing dependencies for cycle detection
                if (!plugins.contains(dep)) {
                    continue;
                }
                if (dfs(dep)) {
                    return true;
                }
            }
        }
        visited[name] = 2;  // mark visited
        return false;
    };

    for (auto it = plugins.begin(); it != plugins.end(); ++it) {
        if (visited.value(it.key(), 0) == 0) {
            if (dfs(it.key())) {
                return true;
            }
        }
    }
    return false;
}

QStringList PluginDependencyResolver::getLoadOrder(
    const QHash<QString, PluginMetadata>& plugins) {
    if (hasCyclicDependencies(plugins)) {
        LOG_WARNING("Cyclic dependencies detected in plugins");
        return plugins.keys();  // Return original order if cycles exist
    }

    return resolveDependencies(plugins);
}

void PluginDependencyResolver::visitPlugin(
    const QString& pluginName, const QHash<QString, PluginMetadata>& plugins,
    QHash<QString, int>& visited, QStringList& result) {
    if (visited[pluginName] == 2) {
        return;  // Already processed
    }

    if (visited[pluginName] == 1) {
        LOG_WARNING("Cyclic dependency detected involving plugin '{}'",
                    pluginName.toStdString());
        return;
    }

    visited[pluginName] = 1;  // Mark as visiting

    if (plugins.contains(pluginName)) {
        const PluginMetadata& metadata = plugins[pluginName];

        // Visit dependencies first
        for (const QString& dependency : metadata.dependencies) {
            if (plugins.contains(dependency)) {
                visitPlugin(dependency, plugins, visited, result);
            }
        }
    }

    visited[pluginName] = 2;  // Mark as visited
    result.append(pluginName);
}

// PluginManager Implementation
PluginManager& PluginManager::instance() {
    if (s_instance == nullptr) {
        s_instance = new PluginManager(qApp);
    }
    return *s_instance;
}

PluginManager::PluginManager(QObject* parent)
    : QObject(parent),
      m_settings(nullptr),
      m_hotReloadingEnabled(false),
      m_hotReloadTimer(nullptr) {
    // Initialize settings
    m_settings = new QSettings("SAST", "Readium-Plugins", this);

    // Setup default plugin directories
    QStringList defaultDirs;
    defaultDirs << QApplication::applicationDirPath() + "/plugins";
    defaultDirs << QStandardPaths::writableLocation(
                       QStandardPaths::AppDataLocation) +
                       "/plugins";
    setPluginDirectories(defaultDirs);

    // Setup hot reloading timer
    m_hotReloadTimer = new QTimer(this);
    m_hotReloadTimer->setInterval(5000);  // Check every 5 seconds
    connect(m_hotReloadTimer, &QTimer::timeout, this,
            &PluginManager::checkForPluginChanges);

    loadSettings();
}

void PluginManager::setPluginDirectories(const QStringList& directories) {
    m_pluginDirectories = directories;

    // Create directories if they don't exist
    for (const QString& dir : directories) {
        QDir().mkpath(dir);
    }
}

void PluginManager::scanForPlugins() {
    LOG_DEBUG("Scanning for plugins in directories: [{}]",
              m_pluginDirectories.join(", ").toStdString());

    m_pluginMetadata.clear();
    int pluginCount = 0;

    for (const QString& directory : m_pluginDirectories) {
        QDir pluginDir(directory);
        if (!pluginDir.exists()) {
            LOG_WARNING("PluginManager: directory '{}' does not exist",
                        directory.toStdString());
            continue;
        }

        QDirIterator directoryIterator(
            directory, QStringList() << "*.dll" << "*.so" << "*.dylib",
            QDir::Files, QDirIterator::Subdirectories);

        while (directoryIterator.hasNext()) {
            const QString filePath = directoryIterator.next();

            if (validatePlugin(filePath)) {
                QPluginLoader loader(filePath);
                PluginMetadata metadata = extractMetadata(&loader);

                if (!metadata.name.isEmpty()) {
                    metadata.filePath = filePath;
                    m_pluginMetadata[metadata.name] = metadata;
                    pluginCount++;

                    LOG_INFO("PluginManager: found plugin '{}' at '{}'",
                             metadata.name.toStdString(),
                             filePath.toStdString());
                }
            }
        }
    }

    LOG_INFO("PluginManager: discovered {} plugins", pluginCount);
    emit pluginsScanned(pluginCount);
}

bool PluginManager::loadPlugin(const QString& pluginName) {
    if (isPluginLoaded(pluginName)) {
        LOG_DEBUG("PluginManager: plugin '{}' already loaded",
                  pluginName.toStdString());
        return true;
    }

    if (!m_pluginMetadata.contains(pluginName)) {
        LOG_WARNING("PluginManager: plugin '{}' not found",
                    pluginName.toStdString());
        return false;
    }

    const PluginMetadata& metadata = m_pluginMetadata[pluginName];

    if (!metadata.isEnabled) {
        LOG_INFO("PluginManager: plugin '{}' is disabled",
                 pluginName.toStdString());
        return false;
    }

    // Check dependencies
    if (!checkDependencies(pluginName)) {
        LOG_WARNING("PluginManager: dependencies not satisfied for plugin '{}'",
                    pluginName.toStdString());
        return false;
    }

    return loadPluginFromFile(metadata.filePath);
}

bool PluginManager::loadPluginFromFile(const QString& filePath) {
    QElapsedTimer timer;
    timer.start();

    QPluginLoader* loader = new QPluginLoader(filePath, this);

    if (!loader->load()) {
        LOG_ERROR("PluginManager: failed to load plugin '{}' ({})",
                  filePath.toStdString(), loader->errorString().toStdString());
        m_pluginErrors[QFileInfo(filePath).baseName()].append(
            loader->errorString());
        delete loader;
        return false;
    }

    QObject* pluginObject = loader->instance();
    if (pluginObject == nullptr) {
        LOG_ERROR("PluginManager: failed to get plugin instance '{}'",
                  filePath.toStdString());
        loader->unload();
        delete loader;
        return false;
    }

    IPluginInterface* plugin = qobject_cast<IPluginInterface*>(pluginObject);
    if (plugin == nullptr) {
        LOG_ERROR(
            "PluginManager: plugin '{}' does not implement IPluginInterface "
            "interface",
            filePath.toStdString());
        loader->unload();
        delete loader;
        return false;
    }

    // Set host before initialization
    plugin->setPluginHost(this);

    // Initialize plugin
    if (!plugin->initialize()) {
        LOG_ERROR("PluginManager: initialization failed for plugin '{}'",
                  plugin->name().toStdString());
        loader->unload();
        delete loader;
        return false;
    }

    const QString pluginName = plugin->name();
    m_pluginLoaders[pluginName] = loader;
    m_loadedPlugins[pluginName] = plugin;

    // Apply UI extension points if the plugin implements IPluginInterface
    auto* pluginInterface = dynamic_cast<IPluginInterface*>(plugin);
    if (pluginInterface) {
        applyExtensionPoints(pluginInterface);
    }

    // Update metadata
    if (m_pluginMetadata.contains(pluginName)) {
        m_pluginMetadata[pluginName].isLoaded = true;
        m_pluginMetadata[pluginName].loadTime = timer.elapsed();
    }

    LOG_INFO("Successfully loaded plugin '{}' in {} ms",
             pluginName.toStdString(), timer.elapsed());
    emit pluginLoaded(pluginName);

    return true;
}

bool PluginManager::unloadPlugin(const QString& pluginName) {
    if (!isPluginLoaded(pluginName)) {
        return true;
    }

    unloadPluginInternal(pluginName);
    return true;
}

void PluginManager::unloadPluginInternal(const QString& pluginName) {
    // Cleanup UI elements first (before shutting down the plugin)
    cleanupPluginUIElements(pluginName);

    // Unregister all hook callbacks
    unregisterAllHooks(pluginName);

    if (m_loadedPlugins.contains(pluginName)) {
        IPluginInterface* plugin = m_loadedPlugins[pluginName];
        plugin->shutdown();
        m_loadedPlugins.remove(pluginName);
    }

    if (m_pluginLoaders.contains(pluginName)) {
        QPluginLoader* loader = m_pluginLoaders[pluginName];
        loader->unload();
        delete loader;
        m_pluginLoaders.remove(pluginName);
    }

    // Update metadata
    if (m_pluginMetadata.contains(pluginName)) {
        m_pluginMetadata[pluginName].isLoaded = false;
    }

    LOG_INFO("Unloaded plugin '{}'", pluginName.toStdString());
    emit pluginUnloaded(pluginName);
}

void PluginManager::loadAllPlugins() {
    QStringList loadOrder =
        PluginDependencyResolver::getLoadOrder(m_pluginMetadata);

    for (const QString& pluginName : loadOrder) {
        if (m_pluginMetadata[pluginName].isEnabled) {
            loadPlugin(pluginName);
        }
    }
}

void PluginManager::unloadAllPlugins() {
    QStringList loadedPlugins = getLoadedPlugins();

    // Unload in reverse order
    for (int i = loadedPlugins.size() - 1; i >= 0; --i) {
        unloadPlugin(loadedPlugins[i]);
    }
}

QStringList PluginManager::getAvailablePlugins() const {
    return m_pluginMetadata.keys();
}

QStringList PluginManager::getLoadedPlugins() const {
    return m_loadedPlugins.keys();
}

QStringList PluginManager::getEnabledPlugins() const {
    QStringList enabled;
    for (auto it = m_pluginMetadata.begin(); it != m_pluginMetadata.end();
         ++it) {
        if (it.value().isEnabled) {
            enabled.append(it.key());
        }
    }
    return enabled;
}

bool PluginManager::isPluginLoaded(const QString& pluginName) const {
    return m_loadedPlugins.contains(pluginName);
}

bool PluginManager::isPluginEnabled(const QString& pluginName) const {
    if (m_pluginMetadata.contains(pluginName)) {
        return m_pluginMetadata[pluginName].isEnabled;
    }
    return false;
}

void PluginManager::setPluginEnabled(const QString& pluginName, bool enabled) {
    if (m_pluginMetadata.contains(pluginName)) {
        m_pluginMetadata[pluginName].isEnabled = enabled;

        if (enabled) {
            emit pluginEnabled(pluginName);
        } else {
            emit pluginDisabled(pluginName);
            if (isPluginLoaded(pluginName)) {
                unloadPlugin(pluginName);
            }
        }
    }
}

QList<IPlugin*> PluginManager::getPluginsByType(
    const QString& interfaceId) const {
    QList<IPlugin*> result;

    for (IPluginInterface* plugin : m_loadedPlugins.values()) {
        IPlugin* iplugin = dynamic_cast<IPlugin*>(plugin);
        if (iplugin != nullptr) {
            QObject* obj = dynamic_cast<QObject*>(plugin);
            if (obj != nullptr &&
                obj->inherits(interfaceId.toUtf8().constData())) {
                result.append(iplugin);
            }
        }
    }

    return result;
}

IPlugin* PluginManager::getPluginByName(const QString& pluginName) const {
    auto it = m_loadedPlugins.find(pluginName);
    if (it != m_loadedPlugins.end()) {
        return dynamic_cast<IPlugin*>(it.value());
    }
    return nullptr;
}

QList<IDocumentProcessorPlugin*> PluginManager::getDocumentProcessorPlugins()
    const {
    QList<IDocumentProcessorPlugin*> result;

    for (IPluginInterface* plugin : m_loadedPlugins.values()) {
        IDocumentProcessorPlugin* docProcessor =
            dynamic_cast<IDocumentProcessorPlugin*>(plugin);
        if (docProcessor != nullptr) {
            result.append(docProcessor);
        }
    }

    return result;
}

QList<IRenderPlugin*> PluginManager::getRenderPlugins() const {
    QList<IRenderPlugin*> result;

    for (IPluginInterface* plugin : m_loadedPlugins.values()) {
        IRenderPlugin* renderPlugin = dynamic_cast<IRenderPlugin*>(plugin);
        if (renderPlugin != nullptr) {
            result.append(renderPlugin);
        }
    }

    return result;
}

QList<ISearchPlugin*> PluginManager::getSearchPlugins() const {
    QList<ISearchPlugin*> result;

    for (IPluginInterface* plugin : m_loadedPlugins.values()) {
        ISearchPlugin* searchPlugin = dynamic_cast<ISearchPlugin*>(plugin);
        if (searchPlugin != nullptr) {
            result.append(searchPlugin);
        }
    }

    return result;
}

QList<ICacheStrategyPlugin*> PluginManager::getCacheStrategyPlugins() const {
    QList<ICacheStrategyPlugin*> result;

    for (IPluginInterface* plugin : m_loadedPlugins.values()) {
        ICacheStrategyPlugin* cachePlugin =
            dynamic_cast<ICacheStrategyPlugin*>(plugin);
        if (cachePlugin != nullptr) {
            result.append(cachePlugin);
        }
    }

    return result;
}

QList<IAnnotationPlugin*> PluginManager::getAnnotationPlugins() const {
    QList<IAnnotationPlugin*> result;

    for (IPluginInterface* plugin : m_loadedPlugins.values()) {
        IAnnotationPlugin* annotationPlugin =
            dynamic_cast<IAnnotationPlugin*>(plugin);
        if (annotationPlugin != nullptr) {
            result.append(annotationPlugin);
        }
    }

    return result;
}

PluginMetadata PluginManager::getPluginMetadata(
    const QString& pluginName) const {
    return m_pluginMetadata.value(pluginName, PluginMetadata());
}

QHash<QString, PluginMetadata> PluginManager::getAllPluginMetadata() const {
    return m_pluginMetadata;
}

PluginMetadata PluginManager::extractMetadata(QPluginLoader* loader) {
    PluginMetadata metadata;

    QJsonObject metaData = loader->metaData().value("MetaData").toObject();

    metadata.name = metaData.value("name").toString();
    metadata.version = metaData.value("version").toString();
    metadata.description = metaData.value("description").toString();
    metadata.author = metaData.value("author").toString();

    QJsonArray deps = metaData.value("dependencies").toArray();
    for (const auto& dep : deps) {
        metadata.dependencies.append(dep.toString());
    }

    QJsonArray types = metaData.value("supportedTypes").toArray();
    for (const auto& type : types) {
        metadata.supportedTypes.append(type.toString());
    }

    QJsonArray features = metaData.value("features").toArray();
    for (const auto& feature : features) {
        metadata.features.append(feature.toString());
    }

    metadata.configuration = metaData.value("configuration").toObject();

    return metadata;
}

bool PluginManager::checkDependencies(const QString& pluginName) const {
    if (!m_pluginMetadata.contains(pluginName)) {
        return false;
    }

    const PluginMetadata& metadata = m_pluginMetadata[pluginName];

    return std::ranges::all_of(metadata.dependencies,
                               [this](const QString& dependency) {
                                   return isPluginLoaded(dependency);
                               });
}

bool PluginManager::validatePlugin(const QString& filePath) {
    QPluginLoader loader(filePath);
    QJsonObject metaData = loader.metaData();

    if (metaData.isEmpty()) {
        return false;
    }

    // Check if it has required metadata
    QJsonObject pluginMetaData = metaData.value("MetaData").toObject();
    return !pluginMetaData.value("name").toString().isEmpty();
}

QStringList PluginManager::getPluginErrors(const QString& pluginName) const {
    return m_pluginErrors.value(pluginName, QStringList());
}

void PluginManager::loadSettings() {
    if (m_settings == nullptr) {
        return;
    }

    m_settings->beginGroup("plugins");

    // Load enabled/disabled state
    for (auto it = m_pluginMetadata.begin(); it != m_pluginMetadata.end();
         ++it) {
        bool enabled = m_settings->value(it.key() + "/enabled", true).toBool();
        it.value().isEnabled = enabled;
    }

    m_settings->endGroup();
}

void PluginManager::saveSettings() {
    if (m_settings == nullptr) {
        return;
    }

    m_settings->beginGroup("plugins");

    // Save enabled/disabled state
    for (auto it = m_pluginMetadata.begin(); it != m_pluginMetadata.end();
         ++it) {
        m_settings->setValue(it.key() + "/enabled", it.value().isEnabled);
    }

    m_settings->endGroup();
    m_settings->sync();
}

void PluginManager::enableHotReloading(bool enabled) {
    m_hotReloadingEnabled = enabled;

    if (enabled) {
        // Record current modification times
        for (auto it = m_pluginMetadata.begin(); it != m_pluginMetadata.end();
             ++it) {
            QFileInfo fileInfo(it.value().filePath);
            m_pluginModificationTimes[it.key()] =
                fileInfo.lastModified().toMSecsSinceEpoch();
        }

        m_hotReloadTimer->start();
    } else {
        m_hotReloadTimer->stop();
    }
}

void PluginManager::checkForPluginChanges() {
    if (!m_hotReloadingEnabled) {
        return;
    }

    for (auto it = m_pluginMetadata.begin(); it != m_pluginMetadata.end();
         ++it) {
        QFileInfo fileInfo(it.value().filePath);
        qint64 currentModTime = fileInfo.lastModified().toMSecsSinceEpoch();
        qint64 recordedModTime = m_pluginModificationTimes.value(it.key(), 0);

        if (currentModTime > recordedModTime) {
            LOG_INFO("PluginManager: plugin file '{}' changed, reloading",
                     it.key().toStdString());

            // Unload and reload the plugin
            if (isPluginLoaded(it.key())) {
                unloadPlugin(it.key());
                loadPlugin(it.key());
            }

            m_pluginModificationTimes[it.key()] = currentModTime;
        }
    }
}

QJsonObject PluginManager::getPluginConfiguration(
    const QString& pluginName) const {
    if (m_pluginMetadata.contains(pluginName)) {
        return m_pluginMetadata[pluginName].configuration;
    }
    return QJsonObject();
}

void PluginManager::setPluginConfiguration(const QString& pluginName,
                                           const QJsonObject& config) {
    if (m_pluginMetadata.contains(pluginName)) {
        m_pluginMetadata[pluginName].configuration = config;

        // Apply configuration to loaded plugin
        IPluginInterface* plugin = getPlugin(pluginName);
        if (plugin != nullptr) {
            plugin->configure(config);
        }
    }
}

QStringList PluginManager::getPluginsWithFeature(const QString& feature) const {
    QStringList result;

    for (auto it = m_pluginMetadata.begin(); it != m_pluginMetadata.end();
         ++it) {
        if (it.value().features.contains(feature)) {
            result.append(it.key());
        }
    }

    return result;
}

QStringList PluginManager::getPluginsForFileType(
    const QString& fileType) const {
    QStringList result;

    for (auto it = m_pluginMetadata.begin(); it != m_pluginMetadata.end();
         ++it) {
        if (it.value().supportedTypes.contains(fileType)) {
            result.append(it.key());
        }
    }

    return result;
}

bool PluginManager::isFeatureAvailable(const QString& feature) const {
    return !getPluginsWithFeature(feature).isEmpty();
}

// Additional plugin management functions
bool PluginManager::installPlugin(const QString& pluginPath) {
    QFileInfo fileInfo(pluginPath);
    if (!fileInfo.exists() || !validatePlugin(pluginPath)) {
        LOG_WARNING("PluginManager: invalid plugin file '{}'",
                    pluginPath.toStdString());
        return false;
    }

    // Copy plugin to plugin directory
    QString targetDir = m_pluginDirectories.first();
    QString targetPath = targetDir + "/" + fileInfo.fileName();

    if (QFile::exists(targetPath)) {
        LOG_WARNING("PluginManager: plugin already exists at '{}'",
                    targetPath.toStdString());
        return false;
    }

    if (!QFile::copy(pluginPath, targetPath)) {
        LOG_ERROR("PluginManager: failed to copy plugin to '{}'",
                  targetPath.toStdString());
        return false;
    }

    // Rescan for plugins to pick up the new one
    scanForPlugins();

    QString pluginName = QFileInfo(targetPath).baseName();
    emit pluginInstalled(pluginName, targetPath);

    return true;
}

bool PluginManager::uninstallPlugin(const QString& pluginName) {
    if (!m_pluginMetadata.contains(pluginName)) {
        return false;
    }

    // Unload plugin if it's loaded
    if (isPluginLoaded(pluginName)) {
        unloadPlugin(pluginName);
    }

    // Remove plugin file
    QString filePath = m_pluginMetadata[pluginName].filePath;
    if (QFile::exists(filePath)) {
        if (!QFile::remove(filePath)) {
            LOG_ERROR("PluginManager: failed to remove plugin file '{}'",
                      filePath.toStdString());
            return false;
        }
    }

    // Remove from metadata
    m_pluginMetadata.remove(pluginName);

    emit pluginUninstalled(pluginName);

    return true;
}

bool PluginManager::updatePlugin(const QString& pluginName,
                                 const QString& newPluginPath) {
    if (!m_pluginMetadata.contains(pluginName)) {
        return false;
    }

    // Validate new plugin
    if (!validatePlugin(newPluginPath)) {
        return false;
    }

    // Unload current plugin
    bool wasLoaded = isPluginLoaded(pluginName);
    if (wasLoaded) {
        unloadPlugin(pluginName);
    }

    // Replace plugin file
    QString oldPath = m_pluginMetadata[pluginName].filePath;
    if (QFile::exists(oldPath)) {
        QFile::remove(oldPath);
    }

    if (!QFile::copy(newPluginPath, oldPath)) {
        LOG_ERROR("PluginManager: failed to update plugin file '{}'",
                  oldPath.toStdString());
        return false;
    }

    // Update metadata
    QPluginLoader loader(oldPath);
    PluginMetadata newMetadata = extractMetadata(&loader);
    newMetadata.filePath = oldPath;
    m_pluginMetadata[pluginName] = newMetadata;

    // Reload if it was loaded before
    if (wasLoaded) {
        loadPlugin(pluginName);
    }

    emit pluginUpdated(pluginName);

    return true;
}

QStringList PluginManager::getPluginDependencies(
    const QString& pluginName) const {
    if (m_pluginMetadata.contains(pluginName)) {
        return m_pluginMetadata[pluginName].dependencies;
    }
    return QStringList();
}

QStringList PluginManager::getPluginsDependingOn(
    const QString& pluginName) const {
    QStringList dependents;

    for (auto it = m_pluginMetadata.begin(); it != m_pluginMetadata.end();
         ++it) {
        if (it.value().dependencies.contains(pluginName)) {
            dependents.append(it.key());
        }
    }

    return dependents;
}

bool PluginManager::canUnloadPlugin(const QString& pluginName) const {
    // Check if other loaded plugins depend on this one
    QStringList dependents = getPluginsDependingOn(pluginName);

    return std::ranges::all_of(dependents, [this](const QString& dependent) {
        return !isPluginLoaded(dependent);
    });
}

void PluginManager::reloadPlugin(const QString& pluginName) {
    if (isPluginLoaded(pluginName)) {
        unloadPlugin(pluginName);
    }
    loadPlugin(pluginName);
}

void PluginManager::reloadAllPlugins() {
    QStringList loadedPlugins = getLoadedPlugins();

    // Unload all plugins
    unloadAllPlugins();

    // Rescan for plugins (in case files changed)
    scanForPlugins();

    // Reload previously loaded plugins
    for (const QString& pluginName : loadedPlugins) {
        if (m_pluginMetadata.contains(pluginName) &&
            m_pluginMetadata[pluginName].isEnabled) {
            loadPlugin(pluginName);
        }
    }
}

QJsonObject PluginManager::getPluginInfo(const QString& pluginName) const {
    QJsonObject info;

    if (!m_pluginMetadata.contains(pluginName)) {
        return info;
    }

    const PluginMetadata& metadata = m_pluginMetadata[pluginName];

    info["name"] = metadata.name;
    info["version"] = metadata.version;
    info["description"] = metadata.description;
    info["author"] = metadata.author;
    info["filePath"] = metadata.filePath;
    info["isEnabled"] = metadata.isEnabled;
    info["isLoaded"] = metadata.isLoaded;
    info["loadTime"] = metadata.loadTime;

    QJsonArray depsArray;
    for (const QString& dep : metadata.dependencies) {
        depsArray.append(dep);
    }
    info["dependencies"] = depsArray;

    QJsonArray typesArray;
    for (const QString& type : metadata.supportedTypes) {
        typesArray.append(type);
    }
    info["supportedTypes"] = typesArray;

    QJsonArray featuresArray;
    for (const QString& feature : metadata.features) {
        featuresArray.append(feature);
    }
    info["features"] = featuresArray;

    info["configuration"] = metadata.configuration;

    return info;
}

void PluginManager::exportPluginList(const QString& filePath) const {
    QJsonObject root;
    QJsonArray pluginsArray;

    for (auto it = m_pluginMetadata.begin(); it != m_pluginMetadata.end();
         ++it) {
        QJsonObject pluginObj = getPluginInfo(it.key());
        pluginsArray.append(pluginObj);
    }

    root["plugins"] = pluginsArray;
    root["totalPlugins"] = m_pluginMetadata.size();
    root["loadedPlugins"] = getLoadedPlugins().size();
    root["enabledPlugins"] = getEnabledPlugins().size();
    root["exportTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonDocument doc(root);

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        emit pluginListExported(filePath);
    }
}

void PluginManager::createPluginReport() const {
    QString report;
    QTextStream stream(&report);

    stream << "Plugin Manager Report\n";
    stream << "====================\n\n";

    stream << "Summary:\n";
    stream << "  Total plugins: " << m_pluginMetadata.size() << "\n";
    stream << "  Loaded plugins: " << getLoadedPlugins().size() << "\n";
    stream << "  Enabled plugins: " << getEnabledPlugins().size() << "\n\n";

    stream << "Plugin Details:\n";
    for (auto it = m_pluginMetadata.begin(); it != m_pluginMetadata.end();
         ++it) {
        const PluginMetadata& metadata = it.value();

        stream << "  " << metadata.name << " (" << metadata.version << ")\n";
        stream << "    Author: " << metadata.author << "\n";
        stream << "    Description: " << metadata.description << "\n";
        stream << "    Status: "
               << (metadata.isLoaded ? "Loaded" : "Not Loaded");
        stream << " / " << (metadata.isEnabled ? "Enabled" : "Disabled")
               << "\n";
        stream << "    File: " << metadata.filePath << "\n";

        if (!metadata.dependencies.isEmpty()) {
            stream << "    Dependencies: " << metadata.dependencies.join(", ")
                   << "\n";
        }

        if (!metadata.features.isEmpty()) {
            stream << "    Features: " << metadata.features.join(", ") << "\n";
        }

        stream << "\n";
    }

    // Save report
    QString fileName =
        QString("plugin_report_%1.txt")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(report.toUtf8());
        emit pluginReportCreated(fileName);
    }
}

bool PluginManager::backupPluginConfiguration(const QString& filePath) const {
    QJsonObject backup;
    QJsonArray pluginsArray;

    for (auto it = m_pluginMetadata.begin(); it != m_pluginMetadata.end();
         ++it) {
        QJsonObject pluginObj;
        pluginObj["name"] = it.key();
        pluginObj["enabled"] = it.value().isEnabled;
        pluginObj["configuration"] = it.value().configuration;
        pluginsArray.append(pluginObj);
    }

    backup["plugins"] = pluginsArray;
    backup["backupTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    backup["version"] = "1.0";

    QJsonDocument doc(backup);

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        emit pluginConfigurationBackedUp(filePath);
        return true;
    }

    return false;
}

bool PluginManager::restorePluginConfiguration(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject backup = doc.object();

    QJsonArray pluginsArray = backup["plugins"].toArray();

    for (const auto& value : pluginsArray) {
        QJsonObject pluginObj = value.toObject();
        QString pluginName = pluginObj["name"].toString();

        if (m_pluginMetadata.contains(pluginName)) {
            bool enabled = pluginObj["enabled"].toBool();
            QJsonObject config = pluginObj["configuration"].toObject();

            setPluginEnabled(pluginName, enabled);
            setPluginConfiguration(pluginName, config);
        }
    }

    saveSettings();
    emit pluginConfigurationRestored(filePath);

    return true;
}

// ============================================================================
// UI Extension Management
// ============================================================================

void PluginManager::registerExtensionPoint(IExtensionPoint* extensionPoint) {
    if (!extensionPoint) {
        LOG_WARNING("PluginManager: attempt to register null extension point");
        return;
    }

    // Check if already registered
    for (IExtensionPoint* existing : m_extensionPoints) {
        if (existing->id() == extensionPoint->id()) {
            LOG_WARNING(
                "PluginManager: extension point '{}' already registered",
                extensionPoint->id().toStdString());
            return;
        }
    }

    m_extensionPoints.append(extensionPoint);
    LOG_INFO("PluginManager: registered extension point '{}'",
             extensionPoint->id().toStdString());

    // Apply to all currently loaded plugins
    for (IPluginInterface* plugin : m_loadedPlugins.values()) {
        auto* pluginInterface = plugin;  // Already IPluginInterface
        if (pluginInterface && extensionPoint->accepts(pluginInterface)) {
            extensionPoint->extend(pluginInterface);
        }
    }
}

void PluginManager::unregisterExtensionPoint(const QString& extensionId) {
    for (int i = 0; i < m_extensionPoints.size(); ++i) {
        if (m_extensionPoints[i]->id() == extensionId) {
            LOG_INFO("PluginManager: unregistered extension point '{}'",
                     extensionId.toStdString());
            m_extensionPoints.removeAt(i);
            return;
        }
    }

    LOG_WARNING("PluginManager: extension point '{}' not found for unregister",
                extensionId.toStdString());
}

QList<IExtensionPoint*> PluginManager::getExtensionPoints() const {
    return m_extensionPoints;
}

void PluginManager::applyExtensionPoints(IPluginInterface* plugin) {
    if (!plugin) {
        return;
    }

    LOG_DEBUG("PluginManager: applying extension points to plugin '{}'",
              plugin->name().toStdString());

    for (IExtensionPoint* extensionPoint : m_extensionPoints) {
        if (extensionPoint->accepts(plugin)) {
            LOG_DEBUG("PluginManager: applying extension point '{}' to '{}'",
                      extensionPoint->id().toStdString(),
                      plugin->name().toStdString());
            extensionPoint->extend(plugin);
        }
    }
}

void PluginManager::registerPluginUIElement(const QString& pluginName,
                                            QObject* uiElement) {
    if (!uiElement) {
        return;
    }

    if (!m_pluginUIElements.contains(pluginName)) {
        m_pluginUIElements[pluginName] = QList<QObject*>();
    }

    m_pluginUIElements[pluginName].append(uiElement);
    LOG_DEBUG("PluginManager: registered UI element for plugin '{}'",
              pluginName.toStdString());
}

void PluginManager::cleanupPluginUIElements(const QString& pluginName) {
    if (!m_pluginUIElements.contains(pluginName)) {
        return;
    }

    LOG_DEBUG("PluginManager: cleaning up UI elements for plugin '{}'",
              pluginName.toStdString());

    QList<QObject*>& elements = m_pluginUIElements[pluginName];

    for (QObject* element : elements) {
        if (element) {
            // Check if it's a widget and remove it from parent
            if (auto* widget = qobject_cast<QWidget*>(element)) {
                widget->setParent(nullptr);
            }

            // Delete the object
            element->deleteLater();
        }
    }

    elements.clear();
    m_pluginUIElements.remove(pluginName);

    LOG_INFO("PluginManager: cleaned up UI elements for plugin '{}'",
             pluginName.toStdString());
}

void PluginManager::registerStandardHooks() {
    LOG_DEBUG("PluginManager: Registering standard hooks");

    auto& hookRegistry = PluginHookRegistry::instance();

    // Document workflow hooks
    hookRegistry.registerHook(StandardHooks::DOCUMENT_PRE_LOAD,
                              "Before document is loaded");
    hookRegistry.registerHook(StandardHooks::DOCUMENT_POST_LOAD,
                              "After document is loaded");
    hookRegistry.registerHook(StandardHooks::DOCUMENT_PRE_CLOSE,
                              "Before document is closed");
    hookRegistry.registerHook(StandardHooks::DOCUMENT_POST_CLOSE,
                              "After document is closed");

    // Rendering workflow hooks
    hookRegistry.registerHook(StandardHooks::RENDER_PRE_PAGE,
                              "Before page is rendered");
    hookRegistry.registerHook(StandardHooks::RENDER_POST_PAGE,
                              "After page is rendered");

    // Search workflow hooks
    hookRegistry.registerHook(StandardHooks::SEARCH_PRE_EXECUTE,
                              "Before search is executed");
    hookRegistry.registerHook(StandardHooks::SEARCH_POST_EXECUTE,
                              "After search is executed");
    hookRegistry.registerHook(StandardHooks::SEARCH_INDEX_BUILD,
                              "When search index is built");
    hookRegistry.registerHook(StandardHooks::SEARCH_RESULTS_RANK,
                              "When search results are ranked");

    // Cache workflow hooks
    hookRegistry.registerHook(StandardHooks::CACHE_PRE_ADD,
                              "Before item is added to cache");
    hookRegistry.registerHook(StandardHooks::CACHE_POST_ADD,
                              "After item is added to cache");
    hookRegistry.registerHook(StandardHooks::CACHE_PRE_EVICT,
                              "Before cache eviction");
    hookRegistry.registerHook(StandardHooks::CACHE_POST_EVICT,
                              "After cache eviction");

    // Export workflow hooks
    hookRegistry.registerHook(StandardHooks::EXPORT_PRE_EXECUTE,
                              "Before document export");
    hookRegistry.registerHook(StandardHooks::EXPORT_POST_EXECUTE,
                              "After document export");

    // Annotation workflow hooks
    hookRegistry.registerHook(StandardHooks::ANNOTATION_CREATED,
                              "After annotation is created");
    hookRegistry.registerHook(StandardHooks::ANNOTATION_UPDATED,
                              "After annotation is updated");
    hookRegistry.registerHook(StandardHooks::ANNOTATION_DELETED,
                              "After annotation is deleted");

    LOG_INFO("PluginManager: Registered {} standard hooks",
             hookRegistry.getHookNames().size());
}

void PluginManager::unregisterAllHooks(const QString& pluginName) {
    LOG_DEBUG("PluginManager: Unregistering all hooks for plugin '{}'",
              pluginName.toStdString());

    auto& hookRegistry = PluginHookRegistry::instance();
    hookRegistry.unregisterAllCallbacks(pluginName);

    LOG_INFO("PluginManager: Unregistered all hooks for plugin '{}'",
             pluginName.toStdString());
}

// ============================================================================
// Configuration Schema Management
// ============================================================================

QJsonObject PluginManager::getPluginConfigSchema(
    const QString& pluginName) const {
    if (!m_pluginMetadata.contains(pluginName)) {
        return QJsonObject();
    }

    QJsonObject config = m_pluginMetadata[pluginName].configuration;

    // Check for explicit configSchema
    if (config.contains("configSchema")) {
        return config["configSchema"].toObject();
    }

    // Fallback: return the configuration itself as simple schema
    return config;
}

bool PluginManager::hasConfigSchema(const QString& pluginName) const {
    if (!m_pluginMetadata.contains(pluginName)) {
        return false;
    }

    QJsonObject config = m_pluginMetadata[pluginName].configuration;
    return !config.isEmpty();
}

bool PluginManager::validatePluginConfiguration(const QString& pluginName,
                                                QStringList* errors) const {
    if (!m_pluginMetadata.contains(pluginName)) {
        if (errors) {
            errors->append(tr("Plugin '%1' not found").arg(pluginName));
        }
        return false;
    }

    QJsonObject schema = getPluginConfigSchema(pluginName);
    if (schema.isEmpty()) {
        return true;  // No schema = no validation needed
    }

    QJsonObject currentConfig = getPluginConfiguration(pluginName);
    bool valid = true;

    // Check required fields
    QJsonObject properties = schema.value("properties").toObject();
    if (properties.isEmpty()) {
        properties = schema;  // Fallback
    }

    for (auto it = properties.begin(); it != properties.end(); ++it) {
        if (it.key() == "groups")
            continue;

        QJsonObject propSchema = it.value().toObject();
        bool isRequired = propSchema.value("required").toBool(false);

        if (isRequired) {
            if (!currentConfig.contains(it.key())) {
                valid = false;
                if (errors) {
                    QString displayName =
                        propSchema.value("displayName").toString(it.key());
                    errors->append(tr("Required field '%1' is not configured")
                                       .arg(displayName));
                }
            } else {
                QJsonValue val = currentConfig.value(it.key());
                if (val.isNull() ||
                    (val.isString() && val.toString().isEmpty())) {
                    valid = false;
                    if (errors) {
                        QString displayName =
                            propSchema.value("displayName").toString(it.key());
                        errors->append(tr("Required field '%1' is empty")
                                           .arg(displayName));
                    }
                }
            }
        }
    }

    return valid;
}

// ============================================================================
// First-run and Setup Wizard Support
// ============================================================================

bool PluginManager::isPluginConfigured(const QString& pluginName) const {
    if (!m_settings) {
        return true;  // Assume configured if no settings
    }

    return m_settings->value(pluginName + "/configured", false).toBool();
}

void PluginManager::markPluginConfigured(const QString& pluginName,
                                         bool configured) {
    if (!m_settings) {
        return;
    }

    m_settings->setValue(pluginName + "/configured", configured);
    m_settings->sync();

    LOG_INFO("PluginManager: Marked plugin '{}' as {}",
             pluginName.toStdString(),
             configured ? "configured" : "not configured");
}

bool PluginManager::needsSetupWizard(const QString& pluginName) const {
    // Check if already configured
    if (isPluginConfigured(pluginName)) {
        return false;
    }

    // Check if has required config that needs to be set
    QStringList requiredKeys = getRequiredConfigKeys(pluginName);
    if (requiredKeys.isEmpty()) {
        // No required config, mark as configured and return false
        // (const_cast needed since this is a const method but we want to
        // auto-mark)
        return false;
    }

    // Check if required config is set
    QJsonObject currentConfig = getPluginConfiguration(pluginName);
    for (const QString& key : requiredKeys) {
        if (!currentConfig.contains(key)) {
            return true;
        }
        QJsonValue val = currentConfig.value(key);
        if (val.isNull() || (val.isString() && val.toString().isEmpty())) {
            return true;
        }
    }

    return false;
}

QStringList PluginManager::getRequiredConfigKeys(
    const QString& pluginName) const {
    QStringList requiredKeys;

    QJsonObject schema = getPluginConfigSchema(pluginName);
    if (schema.isEmpty()) {
        return requiredKeys;
    }

    QJsonObject properties = schema.value("properties").toObject();
    if (properties.isEmpty()) {
        properties = schema;  // Fallback
    }

    for (auto it = properties.begin(); it != properties.end(); ++it) {
        if (it.key() == "groups")
            continue;

        QJsonObject propSchema = it.value().toObject();
        if (propSchema.value("required").toBool(false)) {
            requiredKeys.append(it.key());
        }
    }

    return requiredKeys;
}

// ============================================================================
// IPluginHost Implementation
// ============================================================================

IPluginInterface* PluginManager::getPlugin(const QString& name) {
    return m_loadedPlugins.value(name, nullptr);
}

QList<IPluginInterface*> PluginManager::getPlugins() const {
    return m_loadedPlugins.values();
}

void PluginManager::scanPluginDirectory(const QString& directory) {
    QStringList dirs = m_pluginDirectories;
    if (!dirs.contains(directory)) {
        dirs.append(directory);
        setPluginDirectories(dirs);
    }
    scanForPlugins();
}

QStringList PluginManager::availablePlugins() const {
    return getAvailablePlugins();
}

bool PluginManager::initializePlugin(const QString& name) {
    return loadPlugin(name);
}

void PluginManager::shutdownPlugin(const QString& name) { unloadPlugin(name); }

bool PluginManager::sendPluginMessage(const QString& from,
                                      const QString& target,
                                      const QVariant& message) {
    IPluginInterface* targetPlugin = m_loadedPlugins.value(target, nullptr);
    if (targetPlugin != nullptr) {
        targetPlugin->handleMessage(from, message);
        return true;
    }
    return false;
}

void PluginManager::broadcastPluginMessage(const QString& from,
                                           const QVariant& message) {
    for (IPluginInterface* plugin : m_loadedPlugins.values()) {
        plugin->handleMessage(from, message);
    }
}
