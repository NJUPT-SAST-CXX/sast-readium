#include "InitializationCommand.h"

#include "../controller/ApplicationController.h"
#include "../managers/StyleManager.h"

// Base InitializationCommand
InitializationCommand::InitializationCommand(QString name, QObject* parent)
    : QObject(parent), m_name(std::move(name)),
      m_logger("InitializationCommand") {}

// InitializeModelsCommand
InitializeModelsCommand::InitializeModelsCommand(
    ApplicationController* controller, QObject* parent)
    : InitializationCommand("Initialize Models", parent),
      m_controller(controller) {}

bool InitializeModelsCommand::execute() {
    if (!canExecute()) {
        return isSuccessful();
    }

    emit executionStarted(name());
    Logger::instance().info(
        "[InitCmd] InitializeModelsCommand::execute() STARTED");

    try {
        Logger::instance().debug(
            "[InitCmd] Calling m_controller->initializeModels()...");
        m_controller->initializeModels();
        Logger::instance().info(
            "[InitCmd] InitializeModelsCommand::execute() COMPLETED successfully");
        setExecuted(true);
        setSuccessful(true);
        emit executionCompleted(name(), true);
        return true;
    } catch (const std::exception& e) {
        Logger::instance().error(
            "[InitCmd] InitializeModelsCommand::execute() FAILED: {}",
            e.what());
        setErrorMessage(QString::fromStdString(e.what()));
        setExecuted(true);
        setSuccessful(false);
        emit executionCompleted(name(), false);
        return false;
    }
}

bool InitializeModelsCommand::undo() {
    // Models cleanup is handled by ApplicationController destructor
    return true;
}

// InitializeControllersCommand
InitializeControllersCommand::InitializeControllersCommand(
    ApplicationController* controller, QObject* parent)
    : InitializationCommand("Initialize Controllers", parent),
      m_controller(controller) {}

bool InitializeControllersCommand::execute() {
    if (!canExecute()) {
        return isSuccessful();
    }

    emit executionStarted(name());
    Logger::instance().info(
        "[InitCmd] InitializeControllersCommand::execute() STARTED");

    try {
        Logger::instance().debug(
            "[InitCmd] Calling m_controller->initializeControllers()...");
        m_controller->initializeControllers();
        Logger::instance().info(
            "[InitCmd] InitializeControllersCommand::execute() COMPLETED successfully");
        setExecuted(true);
        setSuccessful(true);
        emit executionCompleted(name(), true);
        return true;
    } catch (const std::exception& e) {
        Logger::instance().error(
            "[InitCmd] InitializeControllersCommand::execute() FAILED: {}",
            e.what());
        setErrorMessage(QString::fromStdString(e.what()));
        setExecuted(true);
        setSuccessful(false);
        emit executionCompleted(name(), false);
        return false;
    }
}

bool InitializeControllersCommand::undo() {
    // Controllers cleanup is handled by ApplicationController destructor
    return true;
}

// InitializeViewsCommand
InitializeViewsCommand::InitializeViewsCommand(
    ApplicationController* controller, QObject* parent)
    : InitializationCommand("Initialize Views", parent),
      m_controller(controller) {}

bool InitializeViewsCommand::execute() {
    if (!canExecute()) {
        return isSuccessful();
    }

    emit executionStarted(name());
    Logger::instance().info(
        "[InitCmd] InitializeViewsCommand::execute() STARTED");

    try {
        Logger::instance().debug(
            "[InitCmd] Calling m_controller->initializeViews()...");
        m_controller->initializeViews();
        Logger::instance().info(
            "[InitCmd] InitializeViewsCommand::execute() COMPLETED successfully");
        setExecuted(true);
        setSuccessful(true);
        emit executionCompleted(name(), true);
        return true;
    } catch (const std::exception& e) {
        Logger::instance().error(
            "[InitCmd] InitializeViewsCommand::execute() FAILED: {}",
            e.what());
        setErrorMessage(QString::fromStdString(e.what()));
        setExecuted(true);
        setSuccessful(false);
        emit executionCompleted(name(), false);
        return false;
    }
}

bool InitializeViewsCommand::undo() {
    // Views are owned by MainWindow, no cleanup needed
    return true;
}

// InitializeConnectionsCommand
InitializeConnectionsCommand::InitializeConnectionsCommand(
    ApplicationController* controller, QObject* parent)
    : InitializationCommand("Initialize Connections", parent),
      m_controller(controller) {}

bool InitializeConnectionsCommand::execute() {
    if (!canExecute()) {
        return isSuccessful();
    }

    emit executionStarted(name());
    Logger::instance().info(
        "[InitCmd] InitializeConnectionsCommand::execute() STARTED");

    try {
        Logger::instance().debug(
            "[InitCmd] Calling m_controller->initializeConnections()...");
        m_controller->initializeConnections();
        Logger::instance().info(
            "[InitCmd] InitializeConnectionsCommand::execute() COMPLETED successfully");
        setExecuted(true);
        setSuccessful(true);
        emit executionCompleted(name(), true);
        return true;
    } catch (const std::exception& e) {
        Logger::instance().error(
            "[InitCmd] InitializeConnectionsCommand::execute() FAILED: {}",
            e.what());
        setErrorMessage(QString::fromStdString(e.what()));
        setExecuted(true);
        setSuccessful(false);
        emit executionCompleted(name(), false);
        return false;
    }
}

// ApplyThemeCommand
ApplyThemeCommand::ApplyThemeCommand(ApplicationController* controller,
                                     QString theme, QObject* parent)
    : InitializationCommand("Apply Theme", parent),
      m_controller(controller),
      m_theme(std::move(theme)) {}

bool ApplyThemeCommand::execute() {
    if (!canExecute()) {
        return isSuccessful();
    }

    emit executionStarted(name());
    Logger::instance().info(
        "[InitCmd] ApplyThemeCommand::execute() STARTED - theme: {}",
        m_theme.toStdString());

    try {
        // Save current theme for rollback
        m_previousTheme =
            (STYLE.currentTheme() == Theme::Light) ? "light" : "dark";
        Logger::instance().debug(
            "[InitCmd] Previous theme saved: {}", m_previousTheme.toStdString());

        Logger::instance().debug(
            "[InitCmd] Calling m_controller->applyTheme()...");
        m_controller->applyTheme(m_theme);
        Logger::instance().info(
            "[InitCmd] ApplyThemeCommand::execute() COMPLETED successfully");
        setExecuted(true);
        setSuccessful(true);
        emit executionCompleted(name(), true);
        return true;
    } catch (const std::exception& e) {
        Logger::instance().error(
            "[InitCmd] ApplyThemeCommand::execute() FAILED: {}", e.what());
        setErrorMessage(QString::fromStdString(e.what()));
        setExecuted(true);
        setSuccessful(false);
        emit executionCompleted(name(), false);
        return false;
    }
}

bool ApplyThemeCommand::undo() {
    if (!m_previousTheme.isEmpty()) {
        m_controller->applyTheme(m_previousTheme);
    }
    return true;
}

// CompositeInitializationCommand
CompositeInitializationCommand::CompositeInitializationCommand(
    const QString& name, QObject* parent)
    : InitializationCommand(name, parent) {}

CompositeInitializationCommand::~CompositeInitializationCommand() {
    clearCommands();
}

void CompositeInitializationCommand::addCommand(
    std::unique_ptr<InitializationCommand> command) {
    if (command) {
        // Connect progress signals
        connect(command.get(), &InitializationCommand::executionStarted, this,
                [this]() {
                    int progress = static_cast<int>(
                        (m_executedCommands.size() * 100) / m_commands.size());
                    emit executionProgress(name(), progress);
                });

        m_commands.push_back(std::move(command));
    }
}

void CompositeInitializationCommand::clearCommands() {
    m_commands.clear();
    m_executedCommands.clear();
}

bool CompositeInitializationCommand::execute() {
    if (!canExecute()) {
        return isSuccessful();
    }

    emit executionStarted(name());
    m_executedCommands.clear();

    bool allSuccessful = true;

    for (auto& command : m_commands) {
        if (command->execute()) {
            m_executedCommands.append(command.get());
            int progress = static_cast<int>(
                (m_executedCommands.size() * 100) / m_commands.size());
            emit executionProgress(name(), progress);
        } else {
            // Command failed, rollback
            setErrorMessage(QString("Failed at step: %1 - %2")
                                .arg(command->name())
                                .arg(command->errorMessage()));
            allSuccessful = false;

            // Rollback executed commands in reverse order
            undo();
            break;
        }
    }

    setExecuted(true);
    setSuccessful(allSuccessful);
    emit executionCompleted(name(), allSuccessful);

    return allSuccessful;
}

bool CompositeInitializationCommand::undo() {
    // Undo in reverse order
    for (auto* command : m_executedCommands) {
        command->undo();
    }
    m_executedCommands.clear();
    return true;
}

// InitializationCommandFactory
std::unique_ptr<CompositeInitializationCommand>
InitializationCommandFactory::createFullInitializationSequence(
    ApplicationController* controller) {
    auto composite =
        std::make_unique<CompositeInitializationCommand>("Full Initialization");

    // Determine theme
    QString theme =
        (STYLE.currentTheme() == Theme::Light) ? "light" : "dark";

    // Add commands in proper order
    composite->addCommand(
        std::make_unique<ApplyThemeCommand>(controller, theme));
    composite->addCommand(
        std::make_unique<InitializeModelsCommand>(controller));
    composite->addCommand(
        std::make_unique<InitializeControllersCommand>(controller));
    composite->addCommand(
        std::make_unique<InitializeViewsCommand>(controller));
    composite->addCommand(
        std::make_unique<InitializeConnectionsCommand>(controller));

    return composite;
}

std::unique_ptr<CompositeInitializationCommand>
InitializationCommandFactory::createMinimalInitializationSequence(
    ApplicationController* controller) {
    auto composite = std::make_unique<CompositeInitializationCommand>(
        "Minimal Initialization");

    // Only essential components
    composite->addCommand(
        std::make_unique<InitializeModelsCommand>(controller));
    composite->addCommand(
        std::make_unique<InitializeControllersCommand>(controller));

    return composite;
}

std::unique_ptr<CompositeInitializationCommand>
InitializationCommandFactory::createCustomInitializationSequence(
    ApplicationController* controller, const QList<QString>& steps) {
    auto composite = std::make_unique<CompositeInitializationCommand>(
        "Custom Initialization");

    for (const QString& step : steps) {
        if (step == "theme") {
            QString theme =
                (STYLE.currentTheme() == Theme::Light) ? "light" : "dark";
            composite->addCommand(
                std::make_unique<ApplyThemeCommand>(controller, theme));
        } else if (step == "models") {
            composite->addCommand(
                std::make_unique<InitializeModelsCommand>(controller));
        } else if (step == "controllers") {
            composite->addCommand(
                std::make_unique<InitializeControllersCommand>(controller));
        } else if (step == "views") {
            composite->addCommand(
                std::make_unique<InitializeViewsCommand>(controller));
        } else if (step == "connections") {
            composite->addCommand(
                std::make_unique<InitializeConnectionsCommand>(controller));
        }
    }

    return composite;
}
