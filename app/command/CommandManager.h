#pragma once

#include <QObject>
#include <QStack>
#include <QHash>
#include <QAction>
#include <memory>
#include <functional>
#include "../logging/SimpleLogging.h"

// Forward declarations
class QWidget;
class DocumentCommand;
class NavigationCommand;

/**
 * @brief CommandManager - Manages command execution, history, and undo/redo
 * 
 * This class implements the Command pattern's invoker role, providing:
 * - Centralized command execution
 * - Command history management
 * - Undo/redo functionality
 * - Command registration and lookup
 * - Keyboard shortcut management
 */
class CommandManager : public QObject {
    Q_OBJECT

public:
    explicit CommandManager(QObject* parent = nullptr);
    ~CommandManager();

    // Command execution
    bool executeCommand(const QString& commandId);
    bool executeCommand(QObject* command);
    
    template<typename CommandType, typename... Args>
    bool executeCommand(Args&&... args) {
        auto command = std::make_unique<CommandType>(std::forward<Args>(args)...);
        return executeCommandObject(command.release());
    }
    
    // Command registration
    using CommandFactory = std::function<QObject*()>;
    void registerCommand(const QString& id, CommandFactory factory);
    void registerCommand(const QString& id, CommandFactory factory, const QString& shortcut);
    
    // Undo/Redo support
    bool canUndo() const;
    bool canRedo() const;
    QString undoCommandName() const;
    QString redoCommandName() const;
    int historySize() const { return m_historySize; }
    void setHistorySize(int size);
    
    // Command lookup
    QObject* createCommand(const QString& id) const;
    bool hasCommand(const QString& id) const;
    QStringList availableCommands() const;
    
    // Shortcut management
    void registerShortcut(const QString& commandId, const QString& shortcut);
    void registerShortcuts(QWidget* widget);
    QString shortcutForCommand(const QString& commandId) const;
    QStringList commandsWithShortcuts() const;
    QHash<QString, QString> allShortcuts() const;
    QString findCommandByShortcut(const QString& shortcut) const;
    bool isShortcutRegistered(const QString& shortcut) const;
    void unregisterShortcut(const QString& commandId);
    void clearShortcuts();
    
    // Command state
    bool isExecuting() const { return m_isExecuting; }
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    // History management
    void clearHistory();
    QStringList commandHistory() const;
    
public slots:
    void undo();
    void redo();
    
signals:
    void commandExecuted(const QString& commandId, bool success);
    void commandUndone(const QString& commandId);
    void commandRedone(const QString& commandId);
    void historyChanged();
    void canUndoChanged(bool canUndo);
    void canRedoChanged(bool canRedo);
    void executionStarted(const QString& commandId);
    void executionFinished(const QString& commandId, bool success);

private:
    bool executeCommandObject(QObject* command);
    void addToHistory(QObject* command);
    void clearRedoStack();
    void updateUndoRedoActions();
    void connectCommandSignals(QObject* command);
    
    // Command registry
    QHash<QString, CommandFactory> m_commandFactories;
    QHash<QString, QString> m_shortcuts;
    
    // Command history
    QStack<QObject*> m_undoStack;
    QStack<QObject*> m_redoStack;
    int m_historySize = 100;
    
    // State
    bool m_isExecuting = false;
    bool m_enabled = true;
    
    // Actions
    QAction* m_undoAction = nullptr;
    QAction* m_redoAction = nullptr;
    
    // Logging
    mutable SastLogging::CategoryLogger m_logger;
};

/**
 * @brief GlobalCommandManager - Singleton command manager for application-wide access
 */
class GlobalCommandManager {
public:
    static CommandManager& instance();
    
    // Convenience methods
    static bool execute(const QString& commandId);
    static void registerCommand(const QString& id, CommandManager::CommandFactory factory);
    static void registerShortcut(const QString& commandId, const QString& shortcut);
    static bool canUndo();
    static bool canRedo();
    static void undo();
    static void redo();
    static void clearHistory();
    static QStringList availableCommands();
    static QString shortcutForCommand(const QString& commandId);
    static void setHistorySize(int size);
    static void setEnabled(bool enabled);
    static bool isEnabled();
    
private:
    GlobalCommandManager() = default;
    ~GlobalCommandManager() = default;
    GlobalCommandManager(const GlobalCommandManager&) = delete;
    GlobalCommandManager& operator=(const GlobalCommandManager&) = delete;
};

/**
 * @brief CommandInvoker - Helper class for invoking commands with parameters
 */
class CommandInvoker : public QObject {
    Q_OBJECT

public:
    explicit CommandInvoker(CommandManager* manager, QObject* parent = nullptr);
    
    // Invoke commands with various parameter types
    void invoke(const QString& commandId);
    void invoke(const QString& commandId, const QVariant& param);
    void invoke(const QString& commandId, const QVariantList& params);
    void invoke(const QString& commandId, const QVariantMap& params);
    
    // Batch execution
    void invokeBatch(const QStringList& commandIds);
    void invokeSequence(const QStringList& commandIds, int delayMs = 0);
    
signals:
    void invocationCompleted(const QString& commandId, bool success);
    void batchCompleted(int successCount, int failureCount);
    
private slots:
    void executeNextInSequence();
    
private:
    CommandManager* m_manager;
    QStringList m_sequenceQueue;
    int m_sequenceDelay = 0;
    QTimer* m_sequenceTimer = nullptr;
};

/**
 * @brief CommandRecorder - Records command execution for macro creation
 */
class CommandRecorder : public QObject {
    Q_OBJECT

public:
    explicit CommandRecorder(CommandManager* manager, QObject* parent = nullptr);
    
    // Recording control
    void startRecording();
    void stopRecording();
    bool isRecording() const { return m_isRecording; }
    
    // Recorded commands
    QStringList recordedCommands() const { return m_recordedCommands; }
    void clearRecording() { m_recordedCommands.clear(); }
    
    // Playback
    void playback();
    void playbackWithDelay(int delayMs);
    
signals:
    void recordingStarted();
    void recordingStopped();
    void commandRecorded(const QString& commandId);
    void playbackCompleted();
    
private slots:
    void onCommandExecuted(const QString& commandId, bool success);
    
private:
    CommandManager* m_manager;
    bool m_isRecording = false;
    QStringList m_recordedCommands;
};
