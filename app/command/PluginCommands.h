#pragma once

#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QString>
#include <memory>

#include "../logging/SimpleLogging.h"
#include "../plugin/PluginManager.h"

/**
 * @brief Base class for plugin-related commands
 *
 * Provides common functionality for all plugin operations
 * following the Command pattern.
 */
class PluginCommand : public QObject {
    Q_OBJECT

public:
    explicit PluginCommand(PluginManager* manager, const QString& name,
                           QObject* parent = nullptr);
    virtual ~PluginCommand() = default;

    Q_DISABLE_COPY_MOVE(PluginCommand)

    // Command interface
    virtual bool execute() = 0;
    [[nodiscard]] virtual bool canExecute() const;
    virtual bool undo() {
        return false;
    }  // Most plugin commands are not undoable

    // Command metadata
    [[nodiscard]] QString name() const { return m_name; }
    [[nodiscard]] QString description() const { return m_description; }

    // Error handling
    [[nodiscard]] bool hasError() const { return !m_errorMessage.isEmpty(); }
    [[nodiscard]] QString errorMessage() const { return m_errorMessage; }

signals:
    void executed(bool success);
    void progress(int value, int maximum);
    void statusMessage(const QString& message);

protected:
    void setDescription(const QString& desc) { m_description = desc; }
    void setErrorMessage(const QString& error) { m_errorMessage = error; }
    void clearError() { m_errorMessage.clear(); }

    [[nodiscard]] PluginManager* pluginManager() const { return m_manager; }

private:
    QPointer<PluginManager> m_manager;
    QString m_name;
    QString m_description;
    QString m_errorMessage;

protected:
    SastLogging::CategoryLogger m_logger{"PluginCommand"};
};

/**
 * @brief Command to load a plugin
 */
class LoadPluginCommand : public PluginCommand {
    Q_OBJECT

public:
    explicit LoadPluginCommand(PluginManager* manager,
                               const QString& pluginName,
                               QObject* parent = nullptr);

    Q_DISABLE_COPY_MOVE(LoadPluginCommand)

    void setPluginName(const QString& name) { m_pluginName = name; }
    [[nodiscard]] QString pluginName() const { return m_pluginName; }

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;

private:
    QString m_pluginName;
};

/**
 * @brief Command to unload a plugin
 */
class UnloadPluginCommand : public PluginCommand {
    Q_OBJECT

public:
    explicit UnloadPluginCommand(PluginManager* manager,
                                 const QString& pluginName,
                                 QObject* parent = nullptr);

    Q_DISABLE_COPY_MOVE(UnloadPluginCommand)

    void setPluginName(const QString& name) { m_pluginName = name; }
    [[nodiscard]] QString pluginName() const { return m_pluginName; }

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;

private:
    QString m_pluginName;
};

/**
 * @brief Command to enable a plugin
 */
class EnablePluginCommand : public PluginCommand {
    Q_OBJECT

public:
    explicit EnablePluginCommand(PluginManager* manager,
                                 const QString& pluginName,
                                 QObject* parent = nullptr);

    Q_DISABLE_COPY_MOVE(EnablePluginCommand)

    void setPluginName(const QString& name) { m_pluginName = name; }
    [[nodiscard]] QString pluginName() const { return m_pluginName; }

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;

private:
    QString m_pluginName;
};

/**
 * @brief Command to disable a plugin
 */
class DisablePluginCommand : public PluginCommand {
    Q_OBJECT

public:
    explicit DisablePluginCommand(PluginManager* manager,
                                  const QString& pluginName,
                                  QObject* parent = nullptr);

    Q_DISABLE_COPY_MOVE(DisablePluginCommand)

    void setPluginName(const QString& name) { m_pluginName = name; }
    [[nodiscard]] QString pluginName() const { return m_pluginName; }

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;

private:
    QString m_pluginName;
};

/**
 * @brief Command to install a plugin
 */
class InstallPluginCommand : public PluginCommand {
    Q_OBJECT

public:
    explicit InstallPluginCommand(PluginManager* manager,
                                  const QString& pluginPath = QString(),
                                  QObject* parent = nullptr);

    Q_DISABLE_COPY_MOVE(InstallPluginCommand)

    void setPluginPath(const QString& path) { m_pluginPath = path; }
    [[nodiscard]] QString pluginPath() const { return m_pluginPath; }

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;

private:
    QString m_pluginPath;
};

/**
 * @brief Command to uninstall a plugin
 */
class UninstallPluginCommand : public PluginCommand {
    Q_OBJECT

public:
    explicit UninstallPluginCommand(PluginManager* manager,
                                    const QString& pluginName,
                                    QObject* parent = nullptr);

    Q_DISABLE_COPY_MOVE(UninstallPluginCommand)

    void setPluginName(const QString& name) { m_pluginName = name; }
    [[nodiscard]] QString pluginName() const { return m_pluginName; }

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;

private:
    QString m_pluginName;
};

/**
 * @brief Command to reload a plugin
 */
class ReloadPluginCommand : public PluginCommand {
    Q_OBJECT

public:
    explicit ReloadPluginCommand(PluginManager* manager,
                                 const QString& pluginName,
                                 QObject* parent = nullptr);

    Q_DISABLE_COPY_MOVE(ReloadPluginCommand)

    void setPluginName(const QString& name) { m_pluginName = name; }
    [[nodiscard]] QString pluginName() const { return m_pluginName; }

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;

private:
    QString m_pluginName;
};

/**
 * @brief Command to scan for plugins
 */
class ScanPluginsCommand : public PluginCommand {
    Q_OBJECT

public:
    explicit ScanPluginsCommand(PluginManager* manager,
                                QObject* parent = nullptr);

    Q_DISABLE_COPY_MOVE(ScanPluginsCommand)

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;
};

/**
 * @brief Command to configure a plugin with undo/redo support
 *
 * This command provides undo/redo capabilities for plugin configuration
 * changes by storing the previous configuration state and allowing it
 * to be restored.
 */
class ConfigurePluginCommand : public PluginCommand {
    Q_OBJECT

public:
    explicit ConfigurePluginCommand(PluginManager* manager,
                                    const QString& pluginName,
                                    const QJsonObject& newConfig,
                                    QObject* parent = nullptr);

    Q_DISABLE_COPY_MOVE(ConfigurePluginCommand)

    void setPluginName(const QString& name) { m_pluginName = name; }
    [[nodiscard]] QString pluginName() const { return m_pluginName; }

    void setNewConfiguration(const QJsonObject& config) {
        m_newConfig = config;
    }
    [[nodiscard]] QJsonObject newConfiguration() const { return m_newConfig; }

    [[nodiscard]] QJsonObject oldConfiguration() const { return m_oldConfig; }

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;
    bool undo() override;  // This command IS undoable

private:
    QString m_pluginName;
    QJsonObject m_newConfig;
    QJsonObject m_oldConfig;  // Stored when execute() is called
};

/**
 * @brief Factory class for creating plugin commands
 */
class PluginCommandFactory {
public:
    static std::unique_ptr<PluginCommand> createLoadCommand(
        PluginManager* manager, const QString& pluginName);

    static std::unique_ptr<PluginCommand> createUnloadCommand(
        PluginManager* manager, const QString& pluginName);

    static std::unique_ptr<PluginCommand> createEnableCommand(
        PluginManager* manager, const QString& pluginName);

    static std::unique_ptr<PluginCommand> createDisableCommand(
        PluginManager* manager, const QString& pluginName);

    static std::unique_ptr<PluginCommand> createInstallCommand(
        PluginManager* manager, const QString& pluginPath = QString());

    static std::unique_ptr<PluginCommand> createUninstallCommand(
        PluginManager* manager, const QString& pluginName);

    static std::unique_ptr<PluginCommand> createReloadCommand(
        PluginManager* manager, const QString& pluginName);

    static std::unique_ptr<PluginCommand> createScanCommand(
        PluginManager* manager);

    static std::unique_ptr<PluginCommand> createConfigureCommand(
        PluginManager* manager, const QString& pluginName,
        const QJsonObject& newConfig);

    static std::unique_ptr<PluginCommand> createCommandFromType(
        const QString& type, PluginManager* manager);
};
