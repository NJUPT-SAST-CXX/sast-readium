#include "CommandManager.h"
#include <QAction>
#include <QApplication>
#include <QShortcut>
#include <QTimer>
#include <QVariant>
#include <QWidget>
#include "DocumentCommands.h"
#include "NavigationCommands.h"

// CommandManager implementation
CommandManager::CommandManager(QObject* parent)
    : QObject(parent), m_logger("CommandManager") {
    m_logger.debug("CommandManager initialized");

    // Create undo/redo actions
    m_undoAction = new QAction("Undo", this);
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setEnabled(false);
    connect(m_undoAction, &QAction::triggered, this, &CommandManager::undo);

    m_redoAction = new QAction("Redo", this);
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setEnabled(false);
    connect(m_redoAction, &QAction::triggered, this, &CommandManager::redo);
}

CommandManager::~CommandManager() {
    clearHistory();
    m_logger.debug("CommandManager destroyed");
}

bool CommandManager::executeCommand(const QString& commandId) {
    if (!m_enabled) {
        m_logger.warning(
            QString("Command execution disabled, ignoring: %1").arg(commandId));
        return false;
    }

    if (m_isExecuting) {
        m_logger.warning(
            QString("Already executing command, ignoring: %1").arg(commandId));
        return false;
    }

    QObject* command = createCommand(commandId);
    if (command == nullptr) {
        m_logger.error(QString("Failed to create command: %1").arg(commandId));
        return false;
    }

    bool result = executeCommandObject(command);

    // Clean up if command is not added to history
    if (!result) {
        delete command;
    }

    return result;
}

bool CommandManager::executeCommand(QObject* command) {
    if (command == nullptr) {
        m_logger.error("Cannot execute null command");
        return false;
    }

    if (!m_enabled) {
        m_logger.warning("Command execution disabled");
        return false;
    }

    if (m_isExecuting) {
        m_logger.warning("Already executing command");
        return false;
    }

    return executeCommandObject(command);
}

void CommandManager::registerCommand(const QString& commandId,
                                     const CommandFactory& factory) {
    if (commandId.isEmpty()) {
        m_logger.error("Cannot register command with empty ID");
        return;
    }

    if (!factory) {
        m_logger.error(QString("Cannot register command with null factory: %1")
                           .arg(commandId));
        return;
    }

    m_commandFactories[commandId] = factory;
    m_logger.debug(QString("Registered command: %1").arg(commandId));
}

void CommandManager::registerCommand(const QString& commandId,
                                     const CommandFactory& factory,
                                     const QString& shortcut) {
    registerCommand(commandId, factory);

    if (!shortcut.isEmpty()) {
        registerShortcut(commandId, shortcut);
    }
}

bool CommandManager::canUndo() const { return !m_undoStack.empty(); }

bool CommandManager::canRedo() const { return !m_redoStack.empty(); }

QString CommandManager::undoCommandName() const {
    if (m_undoStack.empty()) {
        return {};
    }

    QObject* command = m_undoStack.back().get();

    // Try to get command name from different command types
    if (auto* navCommand = qobject_cast<NavigationCommand*>(command)) {
        return navCommand->name();
    }
    if (auto* docCommand = qobject_cast<DocumentCommand*>(command)) {
        return docCommand->name();
    }

    return "Unknown Command";
}

QString CommandManager::redoCommandName() const {
    if (m_redoStack.empty()) {
        return {};
    }

    QObject* command = m_redoStack.back().get();

    // Try to get command name from different command types
    if (auto* navCommand = qobject_cast<NavigationCommand*>(command)) {
        return navCommand->name();
    }
    if (auto* docCommand = qobject_cast<DocumentCommand*>(command)) {
        return docCommand->name();
    }

    return "Unknown Command";
}

void CommandManager::setHistorySize(int size) {
    if (size < 0) {
        m_logger.warning("Invalid history size, using default");
        size = 100;
    }

    m_historySize = size;

    // Trim history if necessary
    while (m_undoStack.size() > static_cast<size_t>(m_historySize)) {
        m_undoStack.erase(m_undoStack.begin());
    }

    updateUndoRedoActions();
    emit historyChanged();
    m_logger.debug(QString("History size set to: %1").arg(size));
}

QObject* CommandManager::createCommand(const QString& commandId) const {
    auto iterator = m_commandFactories.find(commandId);
    if (iterator != m_commandFactories.end()) {
        try {
            return iterator.value()();
        } catch (const std::exception& e) {
            m_logger.error(QString("Exception creating command %1: %2")
                               .arg(commandId)
                               .arg(e.what()));
            return nullptr;
        }
    }

    m_logger.warning(QString("Unknown command ID: %1").arg(commandId));
    return nullptr;
}

bool CommandManager::hasCommand(const QString& commandId) const {
    return m_commandFactories.contains(commandId);
}

QStringList CommandManager::availableCommands() const {
    return m_commandFactories.keys();
}

void CommandManager::registerShortcut(const QString& commandId,
                                      const QString& shortcut) {
    if (commandId.isEmpty() || shortcut.isEmpty()) {
        m_logger.warning(
            "Cannot register shortcut with empty command ID or shortcut");
        return;
    }

    m_shortcuts[commandId] = shortcut;
    m_logger.debug(QString("Registered shortcut %1 for command: %2")
                       .arg(shortcut)
                       .arg(commandId));
}

void CommandManager::registerShortcuts(QWidget* widget) {
    if (widget == nullptr) {
        m_logger.warning("Cannot register shortcuts on null widget");
        return;
    }

    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        const QString& commandId = it.key();
        const QString& shortcut = it.value();

        auto* shortcutObj = new QShortcut(QKeySequence(shortcut), widget);
        connect(shortcutObj, &QShortcut::activated, this,
                [this, commandId]() { executeCommand(commandId); });

        m_logger.debug(
            QString("Registered shortcut %1 for command %2 on widget")
                .arg(shortcut)
                .arg(commandId));
    }
}

QString CommandManager::shortcutForCommand(const QString& commandId) const {
    return m_shortcuts.value(commandId, {});
}

QStringList CommandManager::commandsWithShortcuts() const {
    return m_shortcuts.keys();
}

QHash<QString, QString> CommandManager::allShortcuts() const {
    return m_shortcuts;
}

QString CommandManager::findCommandByShortcut(const QString& shortcut) const {
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        if (it.value() == shortcut) {
            return it.key();
        }
    }
    return {};
}

bool CommandManager::isShortcutRegistered(const QString& shortcut) const {
    return m_shortcuts.values().contains(shortcut);
}

void CommandManager::unregisterShortcut(const QString& commandId) {
    if (m_shortcuts.contains(commandId)) {
        QString shortcut = m_shortcuts.take(commandId);
        m_logger.debug(QString("Unregistered shortcut %1 for command: %2")
                           .arg(shortcut)
                           .arg(commandId));
    }
}

void CommandManager::clearShortcuts() {
    m_shortcuts.clear();
    m_logger.debug("Cleared all shortcuts");
}

void CommandManager::clearHistory() {
    // Clear all commands in undo stack (smart pointers handle deletion)
    m_undoStack.clear();

    // Clear all commands in redo stack (smart pointers handle deletion)
    m_redoStack.clear();

    updateUndoRedoActions();
    emit historyChanged();
    m_logger.debug("Command history cleared");
}

QStringList CommandManager::commandHistory() const {
    QStringList history;

    for (const auto& commandPtr : m_undoStack) {
        QObject* command = commandPtr.get();
        if (auto* navCommand = qobject_cast<NavigationCommand*>(command)) {
            history.append(navCommand->name());
        } else if (auto* docCommand = qobject_cast<DocumentCommand*>(command)) {
            history.append(docCommand->name());
        } else {
            history.append("Unknown Command");
        }
    }

    return history;
}

void CommandManager::undo() {
    if (!canUndo()) {
        m_logger.warning("No commands to undo");
        return;
    }

    if (m_isExecuting) {
        m_logger.warning("Cannot undo while executing command");
        return;
    }

    auto commandPtr = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    QObject* command = commandPtr.get();

    try {
        bool success = false;

        // Try to undo the command based on its type
        if (auto* navCommand = qobject_cast<NavigationCommand*>(command)) {
            success = navCommand->undo();
        } else if (auto* docCommand = qobject_cast<DocumentCommand*>(command)) {
            success = docCommand->undo();
        }

        if (success) {
            m_redoStack.push_back(std::move(commandPtr));
            m_logger.debug("Successfully undid command");
            emit commandUndone(undoCommandName());
        } else {
            // If undo failed, put command back on undo stack
            m_undoStack.push_back(std::move(commandPtr));
            m_logger.warning("Failed to undo command");
        }

    } catch (const std::exception& e) {
        m_logger.error(QString("Exception during undo: %1").arg(e.what()));
        // Put command back on undo stack
        m_undoStack.push_back(std::move(commandPtr));
    }

    updateUndoRedoActions();
}

void CommandManager::redo() {
    if (!canRedo()) {
        m_logger.warning("No commands to redo");
        return;
    }

    if (m_isExecuting) {
        m_logger.warning("Cannot redo while executing command");
        return;
    }

    auto commandPtr = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    QObject* command = commandPtr.get();

    try {
        bool success = executeCommandObject(command);

        if (!success) {
            // If redo failed, put command back on redo stack
            m_redoStack.push_back(std::move(commandPtr));
            m_logger.warning("Failed to redo command");
        } else {
            m_logger.debug("Successfully redid command");
            emit commandRedone(redoCommandName());
        }

    } catch (const std::exception& e) {
        m_logger.error(QString("Exception during redo: %1").arg(e.what()));
        // Put command back on redo stack
        m_redoStack.push_back(std::move(commandPtr));
    }

    updateUndoRedoActions();
}

// Private methods implementation
bool CommandManager::executeCommandObject(QObject* command) {
    if (command == nullptr) {
        return false;
    }

    m_isExecuting = true;
    bool success = false;

    try {
        connectCommandSignals(command);

        // Execute the command based on its type
        if (auto* navCommand = qobject_cast<NavigationCommand*>(command)) {
            success = navCommand->execute();
        } else if (auto* docCommand = qobject_cast<DocumentCommand*>(command)) {
            success = docCommand->execute();
        } else {
            m_logger.error("Unknown command type");
            success = false;
        }

        if (success) {
            addToHistory(command);
        }

        // Emit signal with command name
        QString commandName = "Unknown Command";
        if (auto* navCommand = qobject_cast<NavigationCommand*>(command)) {
            commandName = navCommand->name();
        } else if (auto* docCommand = qobject_cast<DocumentCommand*>(command)) {
            commandName = docCommand->name();
        }
        emit commandExecuted(commandName, success);

    } catch (const std::exception& e) {
        m_logger.error(
            QString("Exception executing command: %1").arg(e.what()));
        success = false;

        QString commandName = "Unknown Command";
        if (auto* navCommand = qobject_cast<NavigationCommand*>(command)) {
            commandName = navCommand->name();
        } else if (auto* docCommand = qobject_cast<DocumentCommand*>(command)) {
            commandName = docCommand->name();
        }
        emit commandExecuted(commandName, false);
    }

    m_isExecuting = false;
    return success;
}

void CommandManager::addToHistory(QObject* command) {
    if (command == nullptr) {
        return;
    }

    // Clear redo stack when new command is executed
    clearRedoStack();

    // Add to undo stack (transfer ownership to smart pointer)
    m_undoStack.push_back(std::unique_ptr<QObject>(command));

    // Trim history if necessary
    while (m_undoStack.size() > static_cast<size_t>(m_historySize)) {
        m_undoStack.erase(m_undoStack.begin());
    }

    updateUndoRedoActions();
    emit historyChanged();
}

void CommandManager::clearRedoStack() {
    // Clear redo stack (smart pointers handle deletion automatically)
    m_redoStack.clear();
}

void CommandManager::updateUndoRedoActions() {
    bool canUndoNow = canUndo();
    bool canRedoNow = canRedo();

    if (m_undoAction != nullptr) {
        m_undoAction->setEnabled(canUndoNow);
        QString undoText =
            canUndoNow ? QString("Undo %1").arg(undoCommandName()) : "Undo";
        m_undoAction->setText(undoText);
    }

    if (m_redoAction != nullptr) {
        m_redoAction->setEnabled(canRedoNow);
        QString redoText =
            canRedoNow ? QString("Redo %1").arg(redoCommandName()) : "Redo";
        m_redoAction->setText(redoText);
    }

    // Emit state change signals
    emit canUndoChanged(canUndoNow);
    emit canRedoChanged(canRedoNow);
}

void CommandManager::connectCommandSignals(QObject* command) {
    if (command == nullptr) {
        return;
    }

    // Connect signals based on command type
    if (auto* navCommand = qobject_cast<NavigationCommand*>(command)) {
        connect(navCommand, &NavigationCommand::executed, this,
                [this, navCommand](bool success) {
                    emit commandExecuted(navCommand->name(), success);
                });
    } else if (auto* docCommand = qobject_cast<DocumentCommand*>(command)) {
        connect(docCommand, &DocumentCommand::executed, this,
                [this, docCommand](bool success) {
                    emit commandExecuted(docCommand->name(), success);
                });
    }
}

// GlobalCommandManager implementation
CommandManager& GlobalCommandManager::instance() {
    static CommandManager instance;
    return instance;
}

bool GlobalCommandManager::execute(const QString& commandId) {
    return instance().executeCommand(commandId);
}

void GlobalCommandManager::registerCommand(
    const QString& commandId, const CommandManager::CommandFactory& factory) {
    instance().registerCommand(commandId, factory);
}

void GlobalCommandManager::registerShortcut(const QString& commandId,
                                            const QString& shortcut) {
    instance().registerShortcut(commandId, shortcut);
}

bool GlobalCommandManager::canUndo() { return instance().canUndo(); }

bool GlobalCommandManager::canRedo() { return instance().canRedo(); }

void GlobalCommandManager::undo() { instance().undo(); }

void GlobalCommandManager::redo() { instance().redo(); }

void GlobalCommandManager::clearHistory() { instance().clearHistory(); }

QStringList GlobalCommandManager::availableCommands() {
    return instance().availableCommands();
}

QString GlobalCommandManager::shortcutForCommand(const QString& commandId) {
    return instance().shortcutForCommand(commandId);
}

void GlobalCommandManager::setHistorySize(int size) {
    instance().setHistorySize(size);
}

void GlobalCommandManager::setEnabled(bool enabled) {
    instance().setEnabled(enabled);
}

bool GlobalCommandManager::isEnabled() { return instance().isEnabled(); }

// CommandInvoker implementation
CommandInvoker::CommandInvoker(CommandManager* manager, QObject* parent)
    : QObject(parent), m_manager(manager), m_sequenceTimer(new QTimer(this)) {
    if (m_manager == nullptr) {
        m_manager = &GlobalCommandManager::instance();
    }

    if (m_sequenceTimer) {
        m_sequenceTimer->setSingleShot(true);
        connect(m_sequenceTimer, &QTimer::timeout, this,
                &CommandInvoker::executeNextInSequence);
    }
}

void CommandInvoker::invoke(const QString& commandId) {
    if (m_manager == nullptr) {
        emit invocationCompleted(commandId, false);
        return;
    }

    bool success = m_manager->executeCommand(commandId);
    emit invocationCompleted(commandId, success);
}

void CommandInvoker::invoke(const QString& commandId, const QVariant& param) {
    Q_UNUSED(param)
    // For now, just invoke without parameters
    // Parameter support would require extending the command system
    invoke(commandId);
}

void CommandInvoker::invoke(const QString& commandId,
                            const QVariantList& params) {
    Q_UNUSED(params)
    // For now, just invoke without parameters
    // Parameter support would require extending the command system
    invoke(commandId);
}

void CommandInvoker::invoke(const QString& commandId,
                            const QVariantMap& params) {
    Q_UNUSED(params)
    // For now, just invoke without parameters
    // Parameter support would require extending the command system
    invoke(commandId);
}

void CommandInvoker::invokeBatch(const QStringList& commandIds) {
    int successCount = 0;
    int failureCount = 0;

    for (const QString& commandId : commandIds) {
        bool success = m_manager->executeCommand(commandId);
        if (success) {
            successCount++;
        } else {
            failureCount++;
        }
    }

    emit batchCompleted(successCount, failureCount);
}

void CommandInvoker::invokeSequence(const QStringList& commandIds,
                                    int delayMs) {
    m_sequenceQueue = commandIds;
    m_sequenceDelay = delayMs;

    if (!m_sequenceQueue.isEmpty()) {
        executeNextInSequence();
    }
}

void CommandInvoker::executeNextInSequence() {
    if (m_sequenceQueue.isEmpty()) {
        // Sequence complete
        return;
    }

    QString commandId = m_sequenceQueue.takeFirst();
    bool success = m_manager->executeCommand(commandId);
    emit invocationCompleted(commandId, success);

    if (!m_sequenceQueue.isEmpty()) {
        if (m_sequenceDelay > 0 && m_sequenceTimer) {
            m_sequenceTimer->start(m_sequenceDelay);
        } else {
            executeNextInSequence();
        }
    }
}

// CommandRecorder implementation
CommandRecorder::CommandRecorder(CommandManager* manager, QObject* parent)
    : QObject(parent), m_manager(manager) {
    if (m_manager == nullptr) {
        m_manager = &GlobalCommandManager::instance();
    }

    // Connect to command manager to record executed commands
    connect(m_manager, &CommandManager::commandExecuted, this,
            &CommandRecorder::onCommandExecuted);
}

void CommandRecorder::startRecording() {
    if (m_isRecording) {
        return;
    }

    m_isRecording = true;
    m_recordedCommands.clear();
    emit recordingStarted();
}

void CommandRecorder::stopRecording() {
    if (!m_isRecording) {
        return;
    }

    m_isRecording = false;
    emit recordingStopped();
}

void CommandRecorder::playback() { playbackWithDelay(0); }

void CommandRecorder::playbackWithDelay(int delayMs) {
    if (m_manager == nullptr || m_recordedCommands.isEmpty()) {
        emit playbackCompleted();
        return;
    }

    // Use CommandInvoker for playback
    CommandInvoker invoker(m_manager);
    connect(&invoker, &CommandInvoker::batchCompleted, this,
            &CommandRecorder::playbackCompleted);

    if (delayMs > 0) {
        invoker.invokeSequence(m_recordedCommands, delayMs);
    } else {
        invoker.invokeBatch(m_recordedCommands);
    }
}

void CommandRecorder::onCommandExecuted(const QString& commandId,
                                        bool success) {
    if (m_isRecording && success) {
        m_recordedCommands.append(commandId);
        emit commandRecorded(commandId);
    }
}
