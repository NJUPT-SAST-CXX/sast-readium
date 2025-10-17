#pragma once

#include <QList>
#include <QObject>
#include <memory>
#include <vector>

#include "../logging/SimpleLogging.h"

// Forward declarations
class ApplicationController;

/**
 * @brief Base class for initialization commands
 *
 * Follows the Command pattern to encapsulate initialization steps
 * and provide undo capability for error recovery.
 */
class InitializationCommand : public QObject {
    Q_OBJECT

public:
    explicit InitializationCommand(QString name, QObject* parent = nullptr);
    ~InitializationCommand() override = default;

    // Delete copy operations (QObject and command pattern are not copyable)
    InitializationCommand(const InitializationCommand&) = delete;
    InitializationCommand& operator=(const InitializationCommand&) = delete;
    InitializationCommand(InitializationCommand&&) = delete;
    InitializationCommand& operator=(InitializationCommand&&) = delete;

    // Command pattern interface
    virtual bool execute() = 0;
    virtual bool undo() { return true; }  // Optional undo for rollback
    [[nodiscard]] virtual bool canExecute() const { return !m_executed; }

    // Command metadata
    [[nodiscard]] QString name() const { return m_name; }
    [[nodiscard]] bool isExecuted() const { return m_executed; }
    [[nodiscard]] bool isSuccessful() const { return m_successful; }
    [[nodiscard]] QString errorMessage() const { return m_errorMessage; }

signals:
    void executionStarted(const QString& name);
    void executionCompleted(const QString& name, bool success);
    void executionProgress(const QString& name, int progress);

protected:
    void setExecuted(bool executed) { m_executed = executed; }
    void setSuccessful(bool successful) { m_successful = successful; }
    void setErrorMessage(const QString& error) { m_errorMessage = error; }

private:
    QString m_name;
    bool m_executed = false;
    bool m_successful = false;
    QString m_errorMessage;
    SastLogging::CategoryLogger m_logger{"InitializationCommand"};
};

/**
 * @brief Command for initializing models
 */
class InitializeModelsCommand : public InitializationCommand {
    Q_OBJECT

public:
    explicit InitializeModelsCommand(ApplicationController* controller,
                                     QObject* parent = nullptr);

    bool execute() override;
    bool undo() override;

private:
    ApplicationController* m_controller;
};

/**
 * @brief Command for initializing controllers
 */
class InitializeControllersCommand : public InitializationCommand {
    Q_OBJECT

public:
    explicit InitializeControllersCommand(ApplicationController* controller,
                                          QObject* parent = nullptr);

    bool execute() override;
    bool undo() override;

private:
    ApplicationController* m_controller;
};

/**
 * @brief Command for initializing views
 */
class InitializeViewsCommand : public InitializationCommand {
    Q_OBJECT

public:
    explicit InitializeViewsCommand(ApplicationController* controller,
                                    QObject* parent = nullptr);

    bool execute() override;
    bool undo() override;

private:
    ApplicationController* m_controller;
};

/**
 * @brief Command for initializing connections
 */
class InitializeConnectionsCommand : public InitializationCommand {
    Q_OBJECT

public:
    explicit InitializeConnectionsCommand(ApplicationController* controller,
                                          QObject* parent = nullptr);

    bool execute() override;
    // No undo for connections - they are cleaned up automatically

private:
    ApplicationController* m_controller;
};

/**
 * @brief Command for applying theme
 */
class ApplyThemeCommand : public InitializationCommand {
    Q_OBJECT

public:
    explicit ApplyThemeCommand(ApplicationController* controller, QString theme,
                               QObject* parent = nullptr);

    bool execute() override;
    bool undo() override;

private:
    ApplicationController* m_controller;
    QString m_theme;
    QString m_previousTheme;
};

/**
 * @brief Composite command for executing multiple initialization commands
 *
 * This implements the Composite pattern to allow grouping of commands
 * and ensures proper rollback on failure.
 */
class CompositeInitializationCommand : public InitializationCommand {
    Q_OBJECT

public:
    explicit CompositeInitializationCommand(const QString& name,
                                            QObject* parent = nullptr);
    ~CompositeInitializationCommand() override;

    // Delete copy operations
    CompositeInitializationCommand(const CompositeInitializationCommand&) =
        delete;
    CompositeInitializationCommand& operator=(
        const CompositeInitializationCommand&) = delete;
    CompositeInitializationCommand(CompositeInitializationCommand&&) = delete;
    CompositeInitializationCommand& operator=(
        CompositeInitializationCommand&&) = delete;

    // Add commands to the composite
    void addCommand(std::unique_ptr<InitializationCommand> command);
    void clearCommands();

    // Execute all commands in sequence
    bool execute() override;

    // Undo all executed commands in reverse order
    bool undo() override;

    // Get command count
    [[nodiscard]] qsizetype commandCount() const {
        return static_cast<qsizetype>(m_commands.size());
    }

private:
    std::vector<std::unique_ptr<InitializationCommand>> m_commands;
    QList<InitializationCommand*> m_executedCommands;
};

/**
 * @brief Factory for creating initialization command sequences
 */
class InitializationCommandFactory {
public:
    /**
     * Create the complete initialization sequence
     */
    static std::unique_ptr<CompositeInitializationCommand>
    createFullInitializationSequence(ApplicationController* controller);

    /**
     * Create a minimal initialization sequence (for testing)
     */
    static std::unique_ptr<CompositeInitializationCommand>
    createMinimalInitializationSequence(ApplicationController* controller);

    /**
     * Create custom initialization sequence with specified steps
     */
    static std::unique_ptr<CompositeInitializationCommand>
    createCustomInitializationSequence(ApplicationController* controller,
                                       const QList<QString>& steps);
};
