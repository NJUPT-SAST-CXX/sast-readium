#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QJsonObject>
#include <memory>
#include "../logging/SimpleLogging.h"

// Forward declarations
class ServiceLocator;
class EventBus;
class CommandManager;
class ConfigurationManager;

/**
 * @brief IPluginInterface - Base interface for all plugins
 *
 * This interface defines the contract that all plugins must implement
 * to be loaded and managed by the plugin system.
 *
 * Note: This is a pure C++ interface. For Qt plugin interfaces, see
 * IPlugin in PluginManager.h which inherits from QObject.
 */
class IPluginInterface {
public:
    virtual ~IPluginInterface() = default;

    // Plugin lifecycle
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;

    // Plugin metadata
    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual QString description() const = 0;
    virtual QString author() const = 0;
    virtual QStringList dependencies() const = 0;

    // Plugin capabilities
    virtual QStringList provides() const = 0;
    virtual QStringList requiredPlugins() const = 0;

    // Configuration
    virtual void configure(const QJsonObject& config) = 0;
    virtual QJsonObject configuration() const = 0;

    // Plugin API version
    virtual int apiVersion() const = 0;
};

/**
 * @brief PluginBase - Base implementation for plugins
 *
 * Provides common functionality for plugin implementations.
 */
class PluginBase : public QObject, public IPluginInterface {
    Q_OBJECT

public:
    explicit PluginBase(QObject* parent = nullptr);
    ~PluginBase() override;

    // IPluginInterface implementation
    bool initialize() override;
    void shutdown() override;
    bool isInitialized() const override { return m_initialized; }
    
    QString name() const override { return m_metadata.name; }
    QString version() const override { return m_metadata.version; }
    QString description() const override { return m_metadata.description; }
    QString author() const override { return m_metadata.author; }
    QStringList dependencies() const override { return m_metadata.dependencies; }
    
    QStringList provides() const override { return m_capabilities.provides; }
    QStringList requiredPlugins() const override { return m_capabilities.requiredPlugins; }
    
    void configure(const QJsonObject& config) override;
    QJsonObject configuration() const override { return m_configuration; }
    
    int apiVersion() const override { return 1; }
    
signals:
    void initialized();
    void shutdownCompleted();
    void error(const QString& message);
    void statusChanged(const QString& status);

protected:
    // Override these in derived classes
    virtual bool onInitialize() = 0;
    virtual void onShutdown() = 0;
    
    // Helper methods for plugin implementations
    ServiceLocator* serviceLocator();
    EventBus* eventBus();
    CommandManager* commandManager();
    ConfigurationManager* configurationManager();
    
    // Plugin metadata
    struct Metadata {
        QString name;
        QString version;
        QString description;
        QString author;
        QStringList dependencies;
    } m_metadata;
    
    // Plugin capabilities
    struct Capabilities {
        QStringList provides;
        QStringList requiredPlugins;
    } m_capabilities;
    
    // Configuration
    QJsonObject m_configuration;
    
    // State
    bool m_initialized = false;
    
    // Logging
    SastLogging::CategoryLogger m_logger{"Plugin"};
};

/**
 * @brief IPluginFactory - Factory interface for creating plugins
 */
class IPluginFactory {
public:
    virtual ~IPluginFactory() = default;

    virtual std::unique_ptr<IPluginInterface> createPlugin() = 0;
    virtual QString pluginName() const = 0;
    virtual bool canCreate() const = 0;
};

/**
 * @brief PluginContext - Context provided to plugins
 * 
 * Provides access to application services and APIs that plugins can use.
 */
class PluginContext : public QObject {
    Q_OBJECT

public:
    explicit PluginContext(QObject* parent = nullptr);
    
    // Service access
    void setServiceLocator(ServiceLocator* locator) { m_serviceLocator = locator; }
    ServiceLocator* serviceLocator() const { return m_serviceLocator; }
    
    void setEventBus(EventBus* bus) { m_eventBus = bus; }
    EventBus* eventBus() const { return m_eventBus; }
    
    void setCommandManager(CommandManager* manager) { m_commandManager = manager; }
    CommandManager* commandManager() const { return m_commandManager; }
    
    void setConfigurationManager(ConfigurationManager* manager) { m_configManager = manager; }
    ConfigurationManager* configurationManager() const { return m_configManager; }
    
    // Plugin communication
    bool sendMessage(const QString& targetPlugin, const QVariant& message);
    void broadcastMessage(const QVariant& message);
    
    // Resource access
    QString pluginDataPath(const QString& pluginName) const;
    QString pluginConfigPath(const QString& pluginName) const;
    
signals:
    void messageReceived(const QString& sourcePlugin, const QVariant& message);
    
private:
    ServiceLocator* m_serviceLocator = nullptr;
    EventBus* m_eventBus = nullptr;
    CommandManager* m_commandManager = nullptr;
    ConfigurationManager* m_configManager = nullptr;
};

/**
 * @brief PluginHost - Interface for the plugin host
 */
class IPluginHost {
public:
    virtual ~IPluginHost() = default;

    // Plugin management
    virtual bool loadPlugin(const QString& path) = 0;
    virtual bool unloadPlugin(const QString& name) = 0;
    virtual IPluginInterface* getPlugin(const QString& name) = 0;
    virtual QList<IPluginInterface*> getPlugins() const = 0;

    // Plugin discovery
    virtual void scanPluginDirectory(const QString& directory) = 0;
    virtual QStringList availablePlugins() const = 0;

    // Plugin lifecycle
    virtual bool initializePlugin(const QString& name) = 0;
    virtual void shutdownPlugin(const QString& name) = 0;

    // Plugin communication
    virtual bool sendPluginMessage(const QString& from, const QString& to, const QVariant& message) = 0;
    virtual void broadcastPluginMessage(const QString& from, const QVariant& message) = 0;
};

/**
 * @brief Extension point interface for plugins to extend functionality
 */
class IExtensionPoint {
public:
    virtual ~IExtensionPoint() = default;

    virtual QString id() const = 0;
    virtual QString description() const = 0;
    virtual bool accepts(IPluginInterface* plugin) const = 0;
    virtual void extend(IPluginInterface* plugin) = 0;
};

/**
 * @brief Menu extension point for adding menu items
 */
class MenuExtensionPoint : public IExtensionPoint {
public:
    QString id() const override { return "org.sast.readium.menu"; }
    QString description() const override { return "Extends application menus"; }
    bool accepts(IPluginInterface* plugin) const override;
    void extend(IPluginInterface* plugin) override;
};

/**
 * @brief Toolbar extension point for adding toolbar buttons
 */
class ToolbarExtensionPoint : public IExtensionPoint {
public:
    QString id() const override { return "org.sast.readium.toolbar"; }
    QString description() const override { return "Extends application toolbar"; }
    bool accepts(IPluginInterface* plugin) const override;
    void extend(IPluginInterface* plugin) override;
};

/**
 * @brief Document handler extension point for custom document types
 */
class DocumentHandlerExtensionPoint : public IExtensionPoint {
public:
    QString id() const override { return "org.sast.readium.document_handler"; }
    QString description() const override { return "Adds support for new document types"; }
    bool accepts(IPluginInterface* plugin) const override;
    void extend(IPluginInterface* plugin) override;
};
