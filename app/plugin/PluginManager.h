#pragma once

#include <QAction>
#include <QDir>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QList>
#include <QMenu>
#include <QObject>
#include <QPluginLoader>
#include <QSettings>
#include <QStringList>
#include <QTimer>
#include <QToolBar>
#include <QWidget>

/**
 * Plugin interface that all plugins must implement
 */
class IPlugin : public QObject {
    Q_OBJECT
public:
    virtual ~IPlugin() = default;

    // Plugin identification
    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual QString description() const = 0;
    virtual QString author() const = 0;
    virtual QStringList dependencies() const = 0;

    // Plugin lifecycle
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;

    // Plugin capabilities
    virtual QStringList supportedFileTypes() const = 0;
    virtual QStringList providedFeatures() const = 0;
    virtual QJsonObject getConfiguration() const = 0;
    virtual void setConfiguration(const QJsonObject& config) = 0;
};

Q_DECLARE_INTERFACE(IPlugin, "com.sast.readium.IPlugin/1.0")

/**
 * Document processing plugin interface
 */
class IDocumentPlugin : public IPlugin {
public:
    virtual ~IDocumentPlugin() = default;

    // Document operations
    virtual bool canProcess(const QString& filePath) const = 0;
    virtual QVariant processDocument(const QString& filePath,
                                     const QJsonObject& options) = 0;
    virtual bool supportsFeature(const QString& feature) const = 0;
};

Q_DECLARE_INTERFACE(IDocumentPlugin, "com.sast.readium.IDocumentPlugin/1.0")

/**
 * UI enhancement plugin interface
 */
class IUIPlugin : public IPlugin {
public:
    virtual ~IUIPlugin() = default;

    // UI operations
    virtual QWidget* createWidget(QWidget* parent = nullptr) = 0;
    virtual QList<QAction*> getActions() const = 0;
    virtual QMenu* getMenu() const = 0;
    virtual QToolBar* getToolBar() const = 0;
};

Q_DECLARE_INTERFACE(IUIPlugin, "com.sast.readium.IUIPlugin/1.0")

/**
 * Plugin metadata structure
 */
struct PluginMetadata {
    QString name;
    QString version;
    QString description;
    QString author;
    QString filePath;
    QStringList dependencies;
    QStringList supportedTypes;
    QStringList features;
    QJsonObject configuration;
    bool isLoaded;
    bool isEnabled;
    qint64 loadTime;

    PluginMetadata() : isLoaded(false), isEnabled(true), loadTime(0) {}
};

/**
 * Plugin dependency resolver
 */
class PluginDependencyResolver {
public:
    static QStringList resolveDependencies(
        const QHash<QString, PluginMetadata>& plugins);
    static bool hasCyclicDependencies(
        const QHash<QString, PluginMetadata>& plugins);
    static QStringList getLoadOrder(
        const QHash<QString, PluginMetadata>& plugins);

private:
    static void visitPlugin(const QString& pluginName,
                            const QHash<QString, PluginMetadata>& plugins,
                            QHash<QString, int>& visited, QStringList& result);
};

/**
 * Manages plugin loading, unloading, and lifecycle
 */
class PluginManager : public QObject {
    Q_OBJECT

public:
    static PluginManager& instance();
    ~PluginManager() = default;

    // Plugin discovery and loading
    void setPluginDirectories(const QStringList& directories);
    QStringList getPluginDirectories() const { return m_pluginDirectories; }

    void scanForPlugins();
    bool loadPlugin(const QString& pluginName);
    bool unloadPlugin(const QString& pluginName);
    void loadAllPlugins();
    void unloadAllPlugins();

    // Plugin management
    QStringList getAvailablePlugins() const;
    QStringList getLoadedPlugins() const;
    QStringList getEnabledPlugins() const;

    bool isPluginLoaded(const QString& pluginName) const;
    bool isPluginEnabled(const QString& pluginName) const;
    void setPluginEnabled(const QString& pluginName, bool enabled);

    // Plugin access
    IPlugin* getPlugin(const QString& pluginName) const;
    template <typename T>
    T* getPlugin(const QString& pluginName) const {
        return qobject_cast<T*>(getPlugin(pluginName));
    }

    QList<IPlugin*> getPluginsByType(const QString& interfaceId) const;
    QList<IDocumentPlugin*> getDocumentPlugins() const;
    QList<IUIPlugin*> getUIPlugins() const;

    // Plugin metadata
    PluginMetadata getPluginMetadata(const QString& pluginName) const;
    QHash<QString, PluginMetadata> getAllPluginMetadata() const;

    // Plugin configuration
    QJsonObject getPluginConfiguration(const QString& pluginName) const;
    void setPluginConfiguration(const QString& pluginName,
                                const QJsonObject& config);

    // Feature queries
    QStringList getPluginsWithFeature(const QString& feature) const;
    QStringList getPluginsForFileType(const QString& fileType) const;
    bool isFeatureAvailable(const QString& feature) const;

    // Plugin validation
    bool validatePlugin(const QString& filePath) const;
    QStringList getPluginErrors(const QString& pluginName) const;

    // Settings persistence
    void loadSettings();
    void saveSettings();

    // Hot reloading
    void enableHotReloading(bool enabled);
    bool isHotReloadingEnabled() const { return m_hotReloadingEnabled; }

    // Plugin installation and management
    bool installPlugin(const QString& pluginPath);
    bool uninstallPlugin(const QString& pluginName);
    bool updatePlugin(const QString& pluginName, const QString& newPluginPath);

    // Dependency management
    QStringList getPluginDependencies(const QString& pluginName) const;
    QStringList getPluginsDependingOn(const QString& pluginName) const;
    bool canUnloadPlugin(const QString& pluginName) const;

    // Plugin reloading
    void reloadPlugin(const QString& pluginName);
    void reloadAllPlugins();

    // Plugin information and reporting
    QJsonObject getPluginInfo(const QString& pluginName) const;
    void exportPluginList(const QString& filePath) const;
    void createPluginReport() const;

    // Configuration backup and restore
    bool backupPluginConfiguration(const QString& filePath) const;
    bool restorePluginConfiguration(const QString& filePath);

signals:
    void pluginLoaded(const QString& pluginName);
    void pluginUnloaded(const QString& pluginName);
    void pluginEnabled(const QString& pluginName);
    void pluginDisabled(const QString& pluginName);
    void pluginError(const QString& pluginName, const QString& error);
    void pluginsScanned(int count);
    void pluginInstalled(const QString& pluginName, const QString& filePath);
    void pluginUninstalled(const QString& pluginName);
    void pluginUpdated(const QString& pluginName);
    void pluginListExported(const QString& filePath) const;
    void pluginReportCreated(const QString& filePath) const;
    void pluginConfigurationBackedUp(const QString& filePath) const;
    void pluginConfigurationRestored(const QString& filePath);

private slots:
    void checkForPluginChanges();

private:
    explicit PluginManager(QObject* parent = nullptr);
    Q_DISABLE_COPY(PluginManager)

    bool loadPluginFromFile(const QString& filePath);
    void unloadPluginInternal(const QString& pluginName);
    PluginMetadata extractMetadata(QPluginLoader* loader) const;
    bool checkDependencies(const QString& pluginName) const;
    void resolveAndLoadPlugins();

    // Plugin storage
    QHash<QString, QPluginLoader*> m_pluginLoaders;
    QHash<QString, IPlugin*> m_loadedPlugins;
    QHash<QString, PluginMetadata> m_pluginMetadata;
    QHash<QString, QStringList> m_pluginErrors;

    // Configuration
    QStringList m_pluginDirectories;
    QSettings* m_settings;

    // Hot reloading
    bool m_hotReloadingEnabled;
    QTimer* m_hotReloadTimer;
    QHash<QString, qint64> m_pluginModificationTimes;

    static PluginManager* s_instance;
};
