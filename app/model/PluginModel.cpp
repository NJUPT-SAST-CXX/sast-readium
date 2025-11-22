#include "PluginModel.h"

#include <QIcon>

PluginModel::PluginModel(PluginManager* manager, QObject* parent)
    : QAbstractListModel(parent),
      m_pluginManager(manager),
      m_showOnlyLoaded(false),
      m_showOnlyEnabled(false),
      m_logger("PluginModel") {
    if (!m_pluginManager) {
        m_logger.error("PluginModel created with null PluginManager");
        return;
    }

    connectToPluginManager();
    buildPluginList();
}

PluginModel::~PluginModel() = default;

void PluginModel::connectToPluginManager() {
    if (!m_pluginManager) {
        return;
    }

    connect(m_pluginManager, &PluginManager::pluginLoaded, this,
            &PluginModel::onPluginLoaded);
    connect(m_pluginManager, &PluginManager::pluginUnloaded, this,
            &PluginModel::onPluginUnloaded);
    connect(m_pluginManager, &PluginManager::pluginEnabled, this,
            &PluginModel::onPluginEnabled);
    connect(m_pluginManager, &PluginManager::pluginDisabled, this,
            &PluginModel::onPluginDisabled);
    connect(m_pluginManager, &PluginManager::pluginError, this,
            &PluginModel::onPluginError);
    connect(m_pluginManager, &PluginManager::pluginsScanned, this,
            &PluginModel::onPluginsScanned);
}

void PluginModel::buildPluginList() {
    if (!m_pluginManager) {
        return;
    }

    m_logger.info("Building plugin list");

    // Get all plugin metadata
    m_metadataCache = m_pluginManager->getAllPluginMetadata();
    m_allPluginNames = m_metadataCache.keys();
    m_allPluginNames.sort();

    // Apply filters
    applyFilters();
}

void PluginModel::applyFilters() {
    beginResetModel();

    m_pluginNames.clear();

    for (const QString& name : m_allPluginNames) {
        const PluginMetadata& metadata = m_metadataCache[name];

        if (matchesFilter(metadata)) {
            m_pluginNames.append(name);
        }
    }

    endResetModel();

    m_logger.info("Applied filters: {} plugins visible out of {}",
                  m_pluginNames.size(), m_allPluginNames.size());

    emit filterChanged();
}

bool PluginModel::matchesFilter(const PluginMetadata& metadata) const {
    // Filter by loaded state
    if (m_showOnlyLoaded && !metadata.isLoaded) {
        return false;
    }

    // Filter by enabled state
    if (m_showOnlyEnabled && !metadata.isEnabled) {
        return false;
    }

    // Filter by text (name, description, author)
    if (!m_filterText.isEmpty()) {
        QString searchText = m_filterText.toLower();

        bool matchesName = metadata.name.toLower().contains(searchText);
        bool matchesDescription =
            metadata.description.toLower().contains(searchText);
        bool matchesAuthor = metadata.author.toLower().contains(searchText);

        if (!matchesName && !matchesDescription && !matchesAuthor) {
            return false;
        }
    }

    return true;
}

int PluginModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return m_pluginNames.size();
}

QVariant PluginModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_pluginNames.size()) {
        return QVariant();
    }

    const QString& pluginName = m_pluginNames[index.row()];
    const PluginMetadata& metadata = m_metadataCache[pluginName];

    switch (role) {
        case Qt::DisplayRole:
            return metadata.name;

        case Qt::ToolTipRole:
            return QString("%1 v%2\n%3\nBy %4")
                .arg(metadata.name, metadata.version, metadata.description,
                     metadata.author);

        case NameRole:
            return metadata.name;

        case VersionRole:
            return metadata.version;

        case DescriptionRole:
            return metadata.description;

        case AuthorRole:
            return metadata.author;

        case FilePathRole:
            return metadata.filePath;

        case DependenciesRole:
            return metadata.dependencies;

        case SupportedTypesRole:
            return metadata.supportedTypes;

        case FeaturesRole:
            return metadata.features;

        case IsLoadedRole:
            return metadata.isLoaded;

        case IsEnabledRole:
            return metadata.isEnabled;

        case LoadTimeRole:
            return metadata.loadTime;

        case ErrorsRole:
            if (m_pluginManager) {
                return m_pluginManager->getPluginErrors(pluginName);
            }
            return QStringList();

        case ConfigurationRole:
            return metadata.configuration;

        case PluginTypeRole:
            return getPluginType(metadata);

        case StatusTextRole:
            return getStatusText(metadata);

        case IconRole:
            // Return default icon based on plugin state
            // TODO: Allow plugins to provide custom icons
            return QVariant();

        default:
            return QVariant();
    }
}

QHash<int, QByteArray> PluginModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[VersionRole] = "version";
    roles[DescriptionRole] = "description";
    roles[AuthorRole] = "author";
    roles[FilePathRole] = "filePath";
    roles[DependenciesRole] = "dependencies";
    roles[SupportedTypesRole] = "supportedTypes";
    roles[FeaturesRole] = "features";
    roles[IsLoadedRole] = "isLoaded";
    roles[IsEnabledRole] = "isEnabled";
    roles[LoadTimeRole] = "loadTime";
    roles[ErrorsRole] = "errors";
    roles[ConfigurationRole] = "configuration";
    roles[PluginTypeRole] = "pluginType";
    roles[StatusTextRole] = "statusText";
    roles[IconRole] = "icon";
    return roles;
}

QString PluginModel::getStatusText(const PluginMetadata& metadata) const {
    if (!metadata.isEnabled) {
        return tr("Disabled");
    }
    if (!metadata.isLoaded) {
        return tr("Not Loaded");
    }
    if (m_pluginManager &&
        !m_pluginManager->getPluginErrors(metadata.name).isEmpty()) {
        return tr("Error");
    }
    return tr("Active");
}

QString PluginModel::getPluginType(const PluginMetadata& metadata) const {
    if (!m_pluginManager) {
        return tr("Unknown");
    }

    // Check if it's a specialized plugin
    if (m_pluginManager->getDocumentProcessorPlugins().size() > 0) {
        auto* plugin =
            m_pluginManager->getPlugin<IDocumentProcessorPlugin>(metadata.name);
        if (plugin) {
            return tr("Document Processor");
        }
    }

    if (m_pluginManager->getRenderPlugins().size() > 0) {
        auto* plugin = m_pluginManager->getPlugin<IRenderPlugin>(metadata.name);
        if (plugin) {
            return tr("Render");
        }
    }

    if (m_pluginManager->getSearchPlugins().size() > 0) {
        auto* plugin = m_pluginManager->getPlugin<ISearchPlugin>(metadata.name);
        if (plugin) {
            return tr("Search");
        }
    }

    if (m_pluginManager->getCacheStrategyPlugins().size() > 0) {
        auto* plugin =
            m_pluginManager->getPlugin<ICacheStrategyPlugin>(metadata.name);
        if (plugin) {
            return tr("Cache Strategy");
        }
    }

    if (m_pluginManager->getAnnotationPlugins().size() > 0) {
        auto* plugin =
            m_pluginManager->getPlugin<IAnnotationPlugin>(metadata.name);
        if (plugin) {
            return tr("Annotation");
        }
    }

    // Check if it's a UI plugin
    auto* uiPlugin = m_pluginManager->getPlugin<IUIPlugin>(metadata.name);
    if (uiPlugin) {
        return tr("UI");
    }

    // Check if it's a document plugin
    auto* docPlugin =
        m_pluginManager->getPlugin<IDocumentPlugin>(metadata.name);
    if (docPlugin) {
        return tr("Document");
    }

    return tr("General");
}

bool PluginModel::loadPlugin(int row) {
    if (row < 0 || row >= m_pluginNames.size() || !m_pluginManager) {
        return false;
    }

    const QString& pluginName = m_pluginNames[row];
    m_logger.info("Loading plugin: {}", pluginName.toStdString());

    bool success = m_pluginManager->loadPlugin(pluginName);

    if (!success) {
        QString error = m_pluginManager->getPluginErrors(pluginName).join("; ");
        m_logger.error("Failed to load plugin {}: {}", pluginName.toStdString(),
                       error.toStdString());
        emit pluginErrorOccurred(pluginName, error);
    }

    return success;
}

bool PluginModel::unloadPlugin(int row) {
    if (row < 0 || row >= m_pluginNames.size() || !m_pluginManager) {
        return false;
    }

    const QString& pluginName = m_pluginNames[row];
    m_logger.info("Unloading plugin: {}", pluginName.toStdString());

    return m_pluginManager->unloadPlugin(pluginName);
}

bool PluginModel::enablePlugin(int row) {
    if (row < 0 || row >= m_pluginNames.size() || !m_pluginManager) {
        return false;
    }

    const QString& pluginName = m_pluginNames[row];
    m_logger.info("Enabling plugin: {}", pluginName.toStdString());

    m_pluginManager->setPluginEnabled(pluginName, true);
    return true;
}

bool PluginModel::disablePlugin(int row) {
    if (row < 0 || row >= m_pluginNames.size() || !m_pluginManager) {
        return false;
    }

    const QString& pluginName = m_pluginNames[row];
    m_logger.info("Disabling plugin: {}", pluginName.toStdString());

    m_pluginManager->setPluginEnabled(pluginName, false);
    return true;
}

bool PluginModel::reloadPlugin(int row) {
    if (row < 0 || row >= m_pluginNames.size() || !m_pluginManager) {
        return false;
    }

    const QString& pluginName = m_pluginNames[row];
    m_logger.info("Reloading plugin: {}", pluginName.toStdString());

    m_pluginManager->reloadPlugin(pluginName);
    return m_pluginManager->isPluginLoaded(pluginName);
}

QString PluginModel::getPluginName(int row) const {
    if (row < 0 || row >= m_pluginNames.size()) {
        return QString();
    }
    return m_pluginNames[row];
}

bool PluginModel::isPluginLoaded(int row) const {
    if (row < 0 || row >= m_pluginNames.size() || !m_pluginManager) {
        return false;
    }
    return m_pluginManager->isPluginLoaded(m_pluginNames[row]);
}

bool PluginModel::isPluginEnabled(int row) const {
    if (row < 0 || row >= m_pluginNames.size() || !m_pluginManager) {
        return false;
    }
    return m_pluginManager->isPluginEnabled(m_pluginNames[row]);
}

PluginMetadata PluginModel::getPluginMetadata(int row) const {
    if (row < 0 || row >= m_pluginNames.size()) {
        return PluginMetadata();
    }
    return m_metadataCache[m_pluginNames[row]];
}

void PluginModel::setFilterText(const QString& filter) {
    if (m_filterText == filter) {
        return;
    }

    m_filterText = filter;
    applyFilters();
}

void PluginModel::setShowOnlyLoaded(bool onlyLoaded) {
    if (m_showOnlyLoaded == onlyLoaded) {
        return;
    }

    m_showOnlyLoaded = onlyLoaded;
    applyFilters();
}

void PluginModel::setShowOnlyEnabled(bool onlyEnabled) {
    if (m_showOnlyEnabled == onlyEnabled) {
        return;
    }

    m_showOnlyEnabled = onlyEnabled;
    applyFilters();
}

void PluginModel::clearFilters() {
    m_filterText.clear();
    m_showOnlyLoaded = false;
    m_showOnlyEnabled = false;
    applyFilters();
}

void PluginModel::refresh() {
    m_logger.info("Refreshing plugin model");
    buildPluginList();
    emit modelRefreshed();
}

void PluginModel::rescanPlugins() {
    if (!m_pluginManager) {
        return;
    }

    m_logger.info("Rescanning plugins");
    m_pluginManager->scanForPlugins();
}

int PluginModel::findPluginRow(const QString& pluginName) const {
    return m_pluginNames.indexOf(pluginName);
}

QStringList PluginModel::getAllPluginNames() const { return m_allPluginNames; }

int PluginModel::loadedPluginCount() const {
    int count = 0;
    for (const QString& name : m_pluginNames) {
        if (m_metadataCache[name].isLoaded) {
            ++count;
        }
    }
    return count;
}

int PluginModel::enabledPluginCount() const {
    int count = 0;
    for (const QString& name : m_pluginNames) {
        if (m_metadataCache[name].isEnabled) {
            ++count;
        }
    }
    return count;
}

void PluginModel::onPluginLoaded(const QString& pluginName) {
    m_logger.info("Plugin loaded: {}", pluginName.toStdString());

    // Update metadata cache
    if (m_pluginManager) {
        m_metadataCache[pluginName] =
            m_pluginManager->getPluginMetadata(pluginName);
    }

    // Notify views
    int row = findPluginRow(pluginName);
    if (row >= 0) {
        QModelIndex idx = index(row);
        emit dataChanged(idx, idx);
    }

    emit pluginLoadStateChanged(pluginName, true);
}

void PluginModel::onPluginUnloaded(const QString& pluginName) {
    m_logger.info("Plugin unloaded: {}", pluginName.toStdString());

    // Update metadata cache
    if (m_pluginManager) {
        m_metadataCache[pluginName] =
            m_pluginManager->getPluginMetadata(pluginName);
    }

    // Notify views
    int row = findPluginRow(pluginName);
    if (row >= 0) {
        QModelIndex idx = index(row);
        emit dataChanged(idx, idx);
    }

    emit pluginLoadStateChanged(pluginName, false);
}

void PluginModel::onPluginEnabled(const QString& pluginName) {
    m_logger.info("Plugin enabled: {}", pluginName.toStdString());

    // Update metadata cache
    if (m_pluginManager) {
        m_metadataCache[pluginName] =
            m_pluginManager->getPluginMetadata(pluginName);
    }

    // Notify views
    int row = findPluginRow(pluginName);
    if (row >= 0) {
        QModelIndex idx = index(row);
        emit dataChanged(idx, idx);
    }

    emit pluginEnableStateChanged(pluginName, true);
}

void PluginModel::onPluginDisabled(const QString& pluginName) {
    m_logger.info("Plugin disabled: {}", pluginName.toStdString());

    // Update metadata cache
    if (m_pluginManager) {
        m_metadataCache[pluginName] =
            m_pluginManager->getPluginMetadata(pluginName);
    }

    // Notify views
    int row = findPluginRow(pluginName);
    if (row >= 0) {
        QModelIndex idx = index(row);
        emit dataChanged(idx, idx);
    }

    emit pluginEnableStateChanged(pluginName, false);
}

void PluginModel::onPluginError(const QString& pluginName,
                                const QString& error) {
    m_logger.error("Plugin error in {}: {}", pluginName.toStdString(),
                   error.toStdString());

    // Notify views
    int row = findPluginRow(pluginName);
    if (row >= 0) {
        QModelIndex idx = index(row);
        emit dataChanged(idx, idx);
    }

    emit pluginErrorOccurred(pluginName, error);
}

void PluginModel::onPluginsScanned(int count) {
    m_logger.info("Plugins scanned: {} plugins found", count);
    refresh();
}
