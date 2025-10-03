#include "PluginInterface.h"
#include <QDir>
#include <QStandardPaths>
#include "../command/CommandManager.h"
#include "../controller/ConfigurationManager.h"
#include "../controller/EventBus.h"
#include "../controller/ServiceLocator.h"

// ============================================================================
// PluginBase Implementation
// ============================================================================

PluginBase::PluginBase(QObject* parent) : QObject(parent), m_logger("Plugin") {
    m_logger.debug("PluginBase created");
}

PluginBase::~PluginBase() {
    if (m_initialized) {
        shutdown();
    }
}

bool PluginBase::initialize() {
    if (m_initialized) {
        m_logger.warning("Plugin already initialized");
        return true;
    }

    m_logger.info(QString("Initializing plugin: %1").arg(m_metadata.name));

    try {
        if (!onInitialize()) {
            m_logger.error("Plugin initialization failed");
            emit error("Initialization failed");
            return false;
        }

        m_initialized = true;
        emit initialized();
        emit statusChanged("Initialized");

        m_logger.info(QString("Plugin initialized successfully: %1")
                          .arg(m_metadata.name));
        return true;

    } catch (const std::exception& e) {
        m_logger.error(QString("Exception during plugin initialization: %1")
                           .arg(e.what()));
        emit error(QString("Initialization exception: %1").arg(e.what()));
        return false;
    }
}

void PluginBase::shutdown() {
    if (!m_initialized) {
        return;
    }

    m_logger.info(QString("Shutting down plugin: %1").arg(m_metadata.name));

    try {
        onShutdown();
        m_initialized = false;
        emit shutdownCompleted();
        emit statusChanged("Shutdown");

        m_logger.info(
            QString("Plugin shutdown successfully: %1").arg(m_metadata.name));

    } catch (const std::exception& e) {
        m_logger.error(
            QString("Exception during plugin shutdown: %1").arg(e.what()));
        emit error(QString("Shutdown exception: %1").arg(e.what()));
    }
}

void PluginBase::configure(const QJsonObject& config) {
    m_configuration = config;
    m_logger.debug(QString("Plugin configured: %1").arg(m_metadata.name));
}

ServiceLocator* PluginBase::serviceLocator() {
    return &ServiceLocator::instance();
}

EventBus* PluginBase::eventBus() { return &EventBus::instance(); }

CommandManager* PluginBase::commandManager() {
    return &GlobalCommandManager::instance();
}

ConfigurationManager* PluginBase::configurationManager() {
    return &ConfigurationManager::instance();
}

// ============================================================================
// PluginContext Implementation
// ============================================================================

PluginContext::PluginContext(QObject* parent) : QObject(parent) {}

bool PluginContext::sendMessage(const QString& targetPlugin,
                                const QVariant& message) {
    // This would require integration with PluginManager
    // For now, emit a signal that PluginManager can connect to
    emit messageReceived(targetPlugin, message);
    return true;
}

void PluginContext::broadcastMessage(const QVariant& message) {
    // Broadcast to all plugins
    emit messageReceived("*", message);
}

QString PluginContext::pluginDataPath(const QString& pluginName) const {
    QString dataPath =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString pluginPath = dataPath + "/plugins/" + pluginName + "/data";

    QDir dir;
    dir.mkpath(pluginPath);

    return pluginPath;
}

QString PluginContext::pluginConfigPath(const QString& pluginName) const {
    QString configPath =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString pluginPath = configPath + "/plugins/" + pluginName;

    QDir dir;
    dir.mkpath(pluginPath);

    return pluginPath;
}

// ============================================================================
// MenuExtensionPoint Implementation
// ============================================================================

bool MenuExtensionPoint::accepts(IPluginInterface* plugin) const {
    if (!plugin) {
        return false;
    }

    // Check if plugin provides menu extension capability
    QStringList provides = plugin->provides();
    return provides.contains("menu") || provides.contains("ui.menu");
}

void MenuExtensionPoint::extend(IPluginInterface* plugin) {
    if (!plugin) {
        return;
    }

    // This would integrate with the application's menu system
    // For now, this is a placeholder that would need to be implemented
    // based on the specific menu framework being used
    qDebug() << "MenuExtensionPoint::extend called for plugin:"
             << plugin->name();
}

// ============================================================================
// ToolbarExtensionPoint Implementation
// ============================================================================

bool ToolbarExtensionPoint::accepts(IPluginInterface* plugin) const {
    if (!plugin) {
        return false;
    }

    // Check if plugin provides toolbar extension capability
    QStringList provides = plugin->provides();
    return provides.contains("toolbar") || provides.contains("ui.toolbar");
}

void ToolbarExtensionPoint::extend(IPluginInterface* plugin) {
    if (!plugin) {
        return;
    }

    // This would integrate with the application's toolbar system
    // For now, this is a placeholder that would need to be implemented
    // based on the specific toolbar framework being used
    qDebug() << "ToolbarExtensionPoint::extend called for plugin:"
             << plugin->name();
}

// ============================================================================
// DocumentHandlerExtensionPoint Implementation
// ============================================================================

bool DocumentHandlerExtensionPoint::accepts(IPluginInterface* plugin) const {
    if (!plugin) {
        return false;
    }

    // Check if plugin provides document handler capability
    QStringList provides = plugin->provides();
    return provides.contains("document.handler") ||
           provides.contains("document.type") ||
           provides.contains("file.handler");
}

void DocumentHandlerExtensionPoint::extend(IPluginInterface* plugin) {
    if (!plugin) {
        return;
    }

    // This would integrate with the application's document handling system
    // For now, this is a placeholder that would need to be implemented
    // based on the specific document framework being used
    qDebug() << "DocumentHandlerExtensionPoint::extend called for plugin:"
             << plugin->name();

    // In a full implementation, this would:
    // 1. Register file type associations
    // 2. Register document loaders
    // 3. Register document renderers
    // 4. Register document exporters
}
