#include "PluginCommands.h"
#include "../plugin/PluginManager.h"

#include <QFileInfo>
#include <QJsonObject>

// ============================================================================
// PluginCommand Base Class
// ============================================================================

PluginCommand::PluginCommand(PluginManager* manager, const QString& name,
                             QObject* parent)
    : QObject(parent),
      m_manager(manager),
      m_name(name),
      m_logger("PluginCommand") {
    if (!m_manager) {
        m_logger.warning("PluginCommand created with null PluginManager");
    }
}

bool PluginCommand::canExecute() const { return m_manager != nullptr; }

// ============================================================================
// LoadPluginCommand
// ============================================================================

LoadPluginCommand::LoadPluginCommand(PluginManager* manager,
                                     const QString& pluginName, QObject* parent)
    : PluginCommand(manager, "LoadPlugin", parent), m_pluginName(pluginName) {
    setDescription(QString("Load plugin: %1").arg(pluginName));
}

bool LoadPluginCommand::execute() {
    clearError();

    if (!canExecute()) {
        setErrorMessage("PluginManager not available");
        m_logger.error(
            "Cannot execute LoadPluginCommand: PluginManager not available");
        emit executed(false);
        return false;
    }

    if (m_pluginName.isEmpty()) {
        setErrorMessage("Plugin name is empty");
        m_logger.error(
            "Cannot execute LoadPluginCommand: Plugin name is empty");
        emit executed(false);
        return false;
    }

    m_logger.info("Loading plugin: {}", m_pluginName.toStdString());
    emit statusMessage(QString("Loading plugin %1...").arg(m_pluginName));

    bool success = pluginManager()->loadPlugin(m_pluginName);

    if (success) {
        m_logger.info("Plugin loaded successfully: {}",
                      m_pluginName.toStdString());
        emit statusMessage(
            QString("Plugin %1 loaded successfully").arg(m_pluginName));
    } else {
        QString error =
            pluginManager()->getPluginErrors(m_pluginName).join("; ");
        setErrorMessage(error.isEmpty() ? "Unknown error" : error);
        m_logger.error("Failed to load plugin {}: {}",
                       m_pluginName.toStdString(), error.toStdString());
        emit statusMessage(
            QString("Failed to load plugin %1").arg(m_pluginName));
    }

    emit executed(success);
    return success;
}

bool LoadPluginCommand::canExecute() const {
    if (!PluginCommand::canExecute()) {
        return false;
    }

    if (m_pluginName.isEmpty()) {
        return false;
    }

    // Can load if plugin is not already loaded
    return !pluginManager()->isPluginLoaded(m_pluginName);
}

// ============================================================================
// UnloadPluginCommand
// ============================================================================

UnloadPluginCommand::UnloadPluginCommand(PluginManager* manager,
                                         const QString& pluginName,
                                         QObject* parent)
    : PluginCommand(manager, "UnloadPlugin", parent), m_pluginName(pluginName) {
    setDescription(QString("Unload plugin: %1").arg(pluginName));
}

bool UnloadPluginCommand::execute() {
    clearError();

    if (!canExecute()) {
        setErrorMessage("Cannot unload plugin");
        m_logger.error("Cannot execute UnloadPluginCommand");
        emit executed(false);
        return false;
    }

    m_logger.info("Unloading plugin: {}", m_pluginName.toStdString());
    emit statusMessage(QString("Unloading plugin %1...").arg(m_pluginName));

    bool success = pluginManager()->unloadPlugin(m_pluginName);

    if (success) {
        m_logger.info("Plugin unloaded successfully: {}",
                      m_pluginName.toStdString());
        emit statusMessage(
            QString("Plugin %1 unloaded successfully").arg(m_pluginName));
    } else {
        setErrorMessage("Failed to unload plugin");
        m_logger.error("Failed to unload plugin: {}",
                       m_pluginName.toStdString());
        emit statusMessage(
            QString("Failed to unload plugin %1").arg(m_pluginName));
    }

    emit executed(success);
    return success;
}

bool UnloadPluginCommand::canExecute() const {
    if (!PluginCommand::canExecute()) {
        return false;
    }

    if (m_pluginName.isEmpty()) {
        return false;
    }

    // Can unload if plugin is loaded and no other plugins depend on it
    return pluginManager()->isPluginLoaded(m_pluginName) &&
           pluginManager()->canUnloadPlugin(m_pluginName);
}

// ============================================================================
// EnablePluginCommand
// ============================================================================

EnablePluginCommand::EnablePluginCommand(PluginManager* manager,
                                         const QString& pluginName,
                                         QObject* parent)
    : PluginCommand(manager, "EnablePlugin", parent), m_pluginName(pluginName) {
    setDescription(QString("Enable plugin: %1").arg(pluginName));
}

bool EnablePluginCommand::execute() {
    clearError();

    if (!canExecute()) {
        setErrorMessage("Cannot enable plugin");
        m_logger.error("Cannot execute EnablePluginCommand");
        emit executed(false);
        return false;
    }

    m_logger.info("Enabling plugin: {}", m_pluginName.toStdString());
    emit statusMessage(QString("Enabling plugin %1...").arg(m_pluginName));

    pluginManager()->setPluginEnabled(m_pluginName, true);

    // Try to load the plugin after enabling
    bool loadSuccess = pluginManager()->loadPlugin(m_pluginName);

    if (loadSuccess) {
        m_logger.info("Plugin enabled and loaded successfully: {}",
                      m_pluginName.toStdString());
        emit statusMessage(
            QString("Plugin %1 enabled successfully").arg(m_pluginName));
    } else {
        m_logger.warning("Plugin enabled but failed to load: {}",
                         m_pluginName.toStdString());
        emit statusMessage(
            QString("Plugin %1 enabled (load failed)").arg(m_pluginName));
    }

    emit executed(true);
    return true;
}

bool EnablePluginCommand::canExecute() const {
    if (!PluginCommand::canExecute()) {
        return false;
    }

    if (m_pluginName.isEmpty()) {
        return false;
    }

    // Can enable if plugin exists and is not already enabled
    return !pluginManager()->isPluginEnabled(m_pluginName);
}

// ============================================================================
// DisablePluginCommand
// ============================================================================

DisablePluginCommand::DisablePluginCommand(PluginManager* manager,
                                           const QString& pluginName,
                                           QObject* parent)
    : PluginCommand(manager, "DisablePlugin", parent),
      m_pluginName(pluginName) {
    setDescription(QString("Disable plugin: %1").arg(pluginName));
}

bool DisablePluginCommand::execute() {
    clearError();

    if (!canExecute()) {
        setErrorMessage("Cannot disable plugin");
        m_logger.error("Cannot execute DisablePluginCommand");
        emit executed(false);
        return false;
    }

    m_logger.info("Disabling plugin: {}", m_pluginName.toStdString());
    emit statusMessage(QString("Disabling plugin %1...").arg(m_pluginName));

    // Unload plugin if it's loaded
    if (pluginManager()->isPluginLoaded(m_pluginName)) {
        pluginManager()->unloadPlugin(m_pluginName);
    }

    pluginManager()->setPluginEnabled(m_pluginName, false);

    m_logger.info("Plugin disabled successfully: {}",
                  m_pluginName.toStdString());
    emit statusMessage(
        QString("Plugin %1 disabled successfully").arg(m_pluginName));

    emit executed(true);
    return true;
}

bool DisablePluginCommand::canExecute() const {
    if (!PluginCommand::canExecute()) {
        return false;
    }

    if (m_pluginName.isEmpty()) {
        return false;
    }

    // Can disable if plugin is enabled and can be unloaded
    return pluginManager()->isPluginEnabled(m_pluginName) &&
           (!pluginManager()->isPluginLoaded(m_pluginName) ||
            pluginManager()->canUnloadPlugin(m_pluginName));
}

// ============================================================================
// InstallPluginCommand
// ============================================================================

InstallPluginCommand::InstallPluginCommand(PluginManager* manager,
                                           const QString& pluginPath,
                                           QObject* parent)
    : PluginCommand(manager, "InstallPlugin", parent),
      m_pluginPath(pluginPath) {
    setDescription(QString("Install plugin from: %1").arg(pluginPath));
}

bool InstallPluginCommand::execute() {
    clearError();

    if (!canExecute()) {
        setErrorMessage("Cannot install plugin");
        m_logger.error("Cannot execute InstallPluginCommand");
        emit executed(false);
        return false;
    }

    m_logger.info("Installing plugin from: {}", m_pluginPath.toStdString());
    emit statusMessage(
        QString("Installing plugin from %1...").arg(m_pluginPath));

    bool success = pluginManager()->installPlugin(m_pluginPath);

    if (success) {
        QString pluginName = QFileInfo(m_pluginPath).baseName();
        m_logger.info("Plugin installed successfully: {}",
                      pluginName.toStdString());
        emit statusMessage(QString("Plugin installed successfully"));
    } else {
        setErrorMessage("Failed to install plugin");
        m_logger.error("Failed to install plugin from: {}",
                       m_pluginPath.toStdString());
        emit statusMessage(QString("Failed to install plugin"));
    }

    emit executed(success);
    return success;
}

bool InstallPluginCommand::canExecute() const {
    if (!PluginCommand::canExecute()) {
        return false;
    }

    if (m_pluginPath.isEmpty()) {
        return false;
    }

    // Check if file exists
    QFileInfo fileInfo(m_pluginPath);
    return fileInfo.exists() && fileInfo.isFile();
}

// ============================================================================
// UninstallPluginCommand
// ============================================================================

UninstallPluginCommand::UninstallPluginCommand(PluginManager* manager,
                                               const QString& pluginName,
                                               QObject* parent)
    : PluginCommand(manager, "UninstallPlugin", parent),
      m_pluginName(pluginName) {
    setDescription(QString("Uninstall plugin: %1").arg(pluginName));
}

bool UninstallPluginCommand::execute() {
    clearError();

    if (!canExecute()) {
        setErrorMessage("Cannot uninstall plugin");
        m_logger.error("Cannot execute UninstallPluginCommand");
        emit executed(false);
        return false;
    }

    m_logger.info("Uninstalling plugin: {}", m_pluginName.toStdString());
    emit statusMessage(QString("Uninstalling plugin %1...").arg(m_pluginName));

    bool success = pluginManager()->uninstallPlugin(m_pluginName);

    if (success) {
        m_logger.info("Plugin uninstalled successfully: {}",
                      m_pluginName.toStdString());
        emit statusMessage(
            QString("Plugin %1 uninstalled successfully").arg(m_pluginName));
    } else {
        setErrorMessage("Failed to uninstall plugin");
        m_logger.error("Failed to uninstall plugin: {}",
                       m_pluginName.toStdString());
        emit statusMessage(
            QString("Failed to uninstall plugin %1").arg(m_pluginName));
    }

    emit executed(success);
    return success;
}

bool UninstallPluginCommand::canExecute() const {
    if (!PluginCommand::canExecute()) {
        return false;
    }

    if (m_pluginName.isEmpty()) {
        return false;
    }

    // Can uninstall if plugin exists
    QHash<QString, PluginMetadata> metadata =
        pluginManager()->getAllPluginMetadata();
    return metadata.contains(m_pluginName);
}

// ============================================================================
// ReloadPluginCommand
// ============================================================================

ReloadPluginCommand::ReloadPluginCommand(PluginManager* manager,
                                         const QString& pluginName,
                                         QObject* parent)
    : PluginCommand(manager, "ReloadPlugin", parent), m_pluginName(pluginName) {
    setDescription(QString("Reload plugin: %1").arg(pluginName));
}

bool ReloadPluginCommand::execute() {
    clearError();

    if (!canExecute()) {
        setErrorMessage("Cannot reload plugin");
        m_logger.error("Cannot execute ReloadPluginCommand");
        emit executed(false);
        return false;
    }

    m_logger.info("Reloading plugin: {}", m_pluginName.toStdString());
    emit statusMessage(QString("Reloading plugin %1...").arg(m_pluginName));

    pluginManager()->reloadPlugin(m_pluginName);

    bool isLoaded = pluginManager()->isPluginLoaded(m_pluginName);

    if (isLoaded) {
        m_logger.info("Plugin reloaded successfully: {}",
                      m_pluginName.toStdString());
        emit statusMessage(
            QString("Plugin %1 reloaded successfully").arg(m_pluginName));
    } else {
        setErrorMessage("Plugin failed to reload");
        m_logger.error("Failed to reload plugin: {}",
                       m_pluginName.toStdString());
        emit statusMessage(
            QString("Failed to reload plugin %1").arg(m_pluginName));
    }

    emit executed(isLoaded);
    return isLoaded;
}

bool ReloadPluginCommand::canExecute() const {
    if (!PluginCommand::canExecute()) {
        return false;
    }

    if (m_pluginName.isEmpty()) {
        return false;
    }

    // Can reload if plugin exists
    QHash<QString, PluginMetadata> metadata =
        pluginManager()->getAllPluginMetadata();
    return metadata.contains(m_pluginName);
}

// ============================================================================
// ScanPluginsCommand
// ============================================================================

ScanPluginsCommand::ScanPluginsCommand(PluginManager* manager, QObject* parent)
    : PluginCommand(manager, "ScanPlugins", parent) {
    setDescription("Scan for available plugins");
}

bool ScanPluginsCommand::execute() {
    clearError();

    if (!canExecute()) {
        setErrorMessage("PluginManager not available");
        m_logger.error("Cannot execute ScanPluginsCommand");
        emit executed(false);
        return false;
    }

    m_logger.info("Scanning for plugins...");
    emit statusMessage("Scanning for plugins...");

    pluginManager()->scanForPlugins();

    int count = pluginManager()->getAvailablePlugins().size();
    m_logger.info("Plugin scan complete. Found {} plugins", count);
    emit statusMessage(QString("Found %1 plugins").arg(count));

    emit executed(true);
    return true;
}

bool ScanPluginsCommand::canExecute() const {
    return PluginCommand::canExecute();
}

// ============================================================================
// ConfigurePluginCommand
// ============================================================================

ConfigurePluginCommand::ConfigurePluginCommand(PluginManager* manager,
                                               const QString& pluginName,
                                               const QJsonObject& newConfig,
                                               QObject* parent)
    : PluginCommand(manager, "ConfigurePlugin", parent),
      m_pluginName(pluginName),
      m_newConfig(newConfig) {
    setDescription(QString("Configure plugin: %1").arg(pluginName));
}

bool ConfigurePluginCommand::execute() {
    clearError();

    if (!canExecute()) {
        setErrorMessage("Cannot configure plugin");
        m_logger.error("Cannot execute ConfigurePluginCommand");
        emit executed(false);
        return false;
    }

    m_logger.info("Configuring plugin: {}", m_pluginName.toStdString());
    emit statusMessage(QString("Configuring plugin %1...").arg(m_pluginName));

    // Store old configuration for undo
    m_oldConfig = pluginManager()->getPluginConfiguration(m_pluginName);

    // Apply new configuration
    pluginManager()->setPluginConfiguration(m_pluginName, m_newConfig);

    m_logger.info("Plugin configured successfully: {}",
                  m_pluginName.toStdString());
    emit statusMessage(
        QString("Plugin %1 configured successfully").arg(m_pluginName));

    emit executed(true);
    return true;
}

bool ConfigurePluginCommand::canExecute() const {
    if (!PluginCommand::canExecute()) {
        return false;
    }

    if (m_pluginName.isEmpty()) {
        return false;
    }

    // Can configure if plugin exists
    QHash<QString, PluginMetadata> metadata =
        pluginManager()->getAllPluginMetadata();
    return metadata.contains(m_pluginName);
}

bool ConfigurePluginCommand::undo() {
    if (m_oldConfig.isEmpty()) {
        m_logger.warning("Cannot undo: no previous configuration stored");
        return false;
    }

    if (!pluginManager()) {
        m_logger.error("Cannot undo: PluginManager not available");
        return false;
    }

    m_logger.info("Undoing configuration for plugin: {}",
                  m_pluginName.toStdString());

    // Restore old configuration
    pluginManager()->setPluginConfiguration(m_pluginName, m_oldConfig);

    return true;
}

// ============================================================================
// PluginCommandFactory
// ============================================================================

std::unique_ptr<PluginCommand> PluginCommandFactory::createLoadCommand(
    PluginManager* manager, const QString& pluginName) {
    return std::make_unique<LoadPluginCommand>(manager, pluginName);
}

std::unique_ptr<PluginCommand> PluginCommandFactory::createUnloadCommand(
    PluginManager* manager, const QString& pluginName) {
    return std::make_unique<UnloadPluginCommand>(manager, pluginName);
}

std::unique_ptr<PluginCommand> PluginCommandFactory::createEnableCommand(
    PluginManager* manager, const QString& pluginName) {
    return std::make_unique<EnablePluginCommand>(manager, pluginName);
}

std::unique_ptr<PluginCommand> PluginCommandFactory::createDisableCommand(
    PluginManager* manager, const QString& pluginName) {
    return std::make_unique<DisablePluginCommand>(manager, pluginName);
}

std::unique_ptr<PluginCommand> PluginCommandFactory::createInstallCommand(
    PluginManager* manager, const QString& pluginPath) {
    return std::make_unique<InstallPluginCommand>(manager, pluginPath);
}

std::unique_ptr<PluginCommand> PluginCommandFactory::createUninstallCommand(
    PluginManager* manager, const QString& pluginName) {
    return std::make_unique<UninstallPluginCommand>(manager, pluginName);
}

std::unique_ptr<PluginCommand> PluginCommandFactory::createReloadCommand(
    PluginManager* manager, const QString& pluginName) {
    return std::make_unique<ReloadPluginCommand>(manager, pluginName);
}

std::unique_ptr<PluginCommand> PluginCommandFactory::createScanCommand(
    PluginManager* manager) {
    return std::make_unique<ScanPluginsCommand>(manager);
}

std::unique_ptr<PluginCommand> PluginCommandFactory::createConfigureCommand(
    PluginManager* manager, const QString& pluginName,
    const QJsonObject& newConfig) {
    return std::make_unique<ConfigurePluginCommand>(manager, pluginName,
                                                    newConfig);
}

std::unique_ptr<PluginCommand> PluginCommandFactory::createCommandFromType(
    const QString& type, PluginManager* manager) {
    if (type == "load") {
        return std::make_unique<LoadPluginCommand>(manager, QString());
    } else if (type == "unload") {
        return std::make_unique<UnloadPluginCommand>(manager, QString());
    } else if (type == "enable") {
        return std::make_unique<EnablePluginCommand>(manager, QString());
    } else if (type == "disable") {
        return std::make_unique<DisablePluginCommand>(manager, QString());
    } else if (type == "install") {
        return std::make_unique<InstallPluginCommand>(manager, QString());
    } else if (type == "uninstall") {
        return std::make_unique<UninstallPluginCommand>(manager, QString());
    } else if (type == "reload") {
        return std::make_unique<ReloadPluginCommand>(manager, QString());
    } else if (type == "scan") {
        return std::make_unique<ScanPluginsCommand>(manager);
    } else if (type == "configure") {
        return std::make_unique<ConfigurePluginCommand>(manager, QString(),
                                                        QJsonObject());
    }

    return nullptr;
}
