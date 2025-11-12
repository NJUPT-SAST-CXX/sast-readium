#include "PluginManager.h"
#include <QApplication>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <algorithm>
#include <functional>
#include "../logging/LoggingMacros.h"

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

    IPlugin* plugin = qobject_cast<IPlugin*>(pluginObject);
    if (plugin == nullptr) {
        LOG_ERROR(
            "PluginManager: plugin '{}' does not implement IPlugin interface",
            filePath.toStdString());
        loader->unload();
        delete loader;
        return false;
    }

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
    if (m_loadedPlugins.contains(pluginName)) {
        IPlugin* plugin = m_loadedPlugins[pluginName];
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

IPlugin* PluginManager::getPlugin(const QString& pluginName) const {
    return m_loadedPlugins.value(pluginName, nullptr);
}

QList<IPlugin*> PluginManager::getPluginsByType(
    const QString& interfaceId) const {
    QList<IPlugin*> result;

    for (IPlugin* plugin : m_loadedPlugins.values()) {
        QObject* obj = qobject_cast<QObject*>(plugin);
        if (obj != nullptr && obj->inherits(interfaceId.toUtf8().constData())) {
            result.append(plugin);
        }
    }

    return result;
}

QList<IDocumentPlugin*> PluginManager::getDocumentPlugins() const {
    QList<IDocumentPlugin*> result;

    for (IPlugin* plugin : m_loadedPlugins.values()) {
        IDocumentPlugin* docPlugin = qobject_cast<IDocumentPlugin*>(plugin);
        if (docPlugin != nullptr) {
            result.append(docPlugin);
        }
    }

    return result;
}

QList<IUIPlugin*> PluginManager::getUIPlugins() const {
    QList<IUIPlugin*> result;

    for (IPlugin* plugin : m_loadedPlugins.values()) {
        IUIPlugin* uiPlugin = qobject_cast<IUIPlugin*>(plugin);
        if (uiPlugin != nullptr) {
            result.append(uiPlugin);
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
        IPlugin* plugin = getPlugin(pluginName);
        if (plugin != nullptr) {
            plugin->setConfiguration(config);
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
