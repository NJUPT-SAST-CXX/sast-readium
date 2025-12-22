#pragma once

#include <QHash>
#include <QKeySequence>
#include <QObject>
#include <QShortcut>
#include "plugin/PluginInterface.h"

/**
 * @brief Custom command definition
 */
struct CommandDefinition {
    QString id;
    QString displayName;
    QString description;
    QString category;
    QKeySequence defaultShortcut;
    QKeySequence currentShortcut;
    std::function<void()> action;
    bool enabled;
};

/**
 * @brief KeyboardShortcutsPlugin - Example command and shortcut plugin
 *
 * This plugin demonstrates:
 * - **Command Registration**: Register custom commands with actions
 * - **Shortcut Customization**: Rebindable keyboard shortcuts
 * - **Conflict Detection**: Detect and resolve shortcut conflicts
 * - **Settings Persistence**: Save/load shortcut bindings
 * - **Command Palette**: Quick command access via search
 */
class KeyboardShortcutsPlugin : public PluginBase, public IUIExtension {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE
                          "keyboard_shortcuts.json")
    Q_INTERFACES(IPluginInterface IUIExtension)

public:
    explicit KeyboardShortcutsPlugin(QObject* parent = nullptr);
    ~KeyboardShortcutsPlugin() override;

    void handleMessage(const QString& from, const QVariant& message) override;

    // IUIExtension interface
    QList<QAction*> menuActions() const override;
    QList<QAction*> toolbarActions() const override;
    QList<QAction*> contextMenuActions() const override { return {}; }
    QString statusBarMessage() const override { return QString(); }
    QWidget* createDockWidget() override { return nullptr; }
    QString menuPath() const override { return "Tools"; }
    QString toolbarId() const override { return "tools_toolbar"; }

    // Command management
    bool registerCommand(const CommandDefinition& cmd);
    bool unregisterCommand(const QString& commandId);
    void executeCommand(const QString& commandId);
    bool setShortcut(const QString& commandId, const QKeySequence& shortcut);
    QKeySequence getShortcut(const QString& commandId) const;
    QStringList getCommandIds() const;
    QStringList findConflicts(const QKeySequence& shortcut) const;

protected:
    bool onInitialize() override;
    void onShutdown() override;

private slots:
    void onOpenShortcutEditor();
    void onOpenCommandPalette();

private:
    void registerHooks();
    void registerBuiltinCommands();
    void loadShortcuts();
    void saveShortcuts();
    void createShortcuts();
    void destroyShortcuts();

    QVariant onCommandExecute(const QVariantMap& context);

    // Commands storage
    QHash<QString, CommandDefinition> m_commands;
    QHash<QString, QShortcut*> m_shortcuts;

    // UI
    QList<QAction*> m_menuActions;
    QAction* m_shortcutEditorAction;
    QAction* m_commandPaletteAction;

    // Statistics
    int m_commandsExecuted;
};
