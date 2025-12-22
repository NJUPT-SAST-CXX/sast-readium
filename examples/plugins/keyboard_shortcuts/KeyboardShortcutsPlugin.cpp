#include "KeyboardShortcutsPlugin.h"
#include <QAction>
#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include "controller/EventBus.h"
#include "plugin/PluginHookRegistry.h"

KeyboardShortcutsPlugin::KeyboardShortcutsPlugin(QObject* parent)
    : PluginBase(parent),
      m_shortcutEditorAction(nullptr),
      m_commandPaletteAction(nullptr),
      m_commandsExecuted(0) {
    m_metadata.name = "Keyboard Shortcuts";
    m_metadata.version = "1.0.0";
    m_metadata.description =
        "Custom command registration and keyboard shortcut management";
    m_metadata.author = "SAST Readium Team";
    m_metadata.dependencies = QStringList();
    m_capabilities.provides = QStringList()
                              << "command.register" << "shortcut.custom"
                              << "command.palette" << "ui.extension";
}

KeyboardShortcutsPlugin::~KeyboardShortcutsPlugin() {
    qDeleteAll(m_menuActions);
    destroyShortcuts();
}

bool KeyboardShortcutsPlugin::onInitialize() {
    m_logger.info("KeyboardShortcutsPlugin: Initializing...");

    // Register built-in commands
    registerBuiltinCommands();

    // Load saved shortcuts
    loadShortcuts();

    // Create shortcut objects
    createShortcuts();

    // Create UI actions
    m_commandPaletteAction = new QAction("Command Palette...", this);
    m_commandPaletteAction->setShortcut(QKeySequence("Ctrl+Shift+P"));
    connect(m_commandPaletteAction, &QAction::triggered, this,
            &KeyboardShortcutsPlugin::onOpenCommandPalette);
    m_menuActions.append(m_commandPaletteAction);

    m_shortcutEditorAction = new QAction("Keyboard Shortcuts...", this);
    m_shortcutEditorAction->setShortcut(QKeySequence("Ctrl+K Ctrl+S"));
    connect(m_shortcutEditorAction, &QAction::triggered, this,
            &KeyboardShortcutsPlugin::onOpenShortcutEditor);
    m_menuActions.append(m_shortcutEditorAction);

    // Register hooks
    registerHooks();

    m_logger.info(QString("KeyboardShortcutsPlugin: Registered %1 commands")
                      .arg(m_commands.size()));
    return true;
}

void KeyboardShortcutsPlugin::onShutdown() {
    m_logger.info("KeyboardShortcutsPlugin: Shutting down...");

    // Save shortcuts
    saveShortcuts();

    // Destroy shortcuts
    destroyShortcuts();

    // Unregister
    PluginHookRegistry::instance().unregisterAllCallbacks(name());

    m_logger.info(QString("KeyboardShortcutsPlugin: Commands executed: %1")
                      .arg(m_commandsExecuted));
}

void KeyboardShortcutsPlugin::handleMessage(const QString& from,
                                            const QVariant& message) {
    QVariantMap msgMap = message.toMap();
    QString action = msgMap.value("action").toString();

    if (action == "register_command") {
        CommandDefinition cmd;
        cmd.id = msgMap.value("id").toString();
        cmd.displayName = msgMap.value("displayName").toString();
        cmd.description = msgMap.value("description").toString();
        cmd.category = msgMap.value("category").toString();
        cmd.defaultShortcut = QKeySequence(msgMap.value("shortcut").toString());
        cmd.currentShortcut = cmd.defaultShortcut;
        cmd.enabled = true;

        bool success = registerCommand(cmd);

        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["success"] = success;
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);

    } else if (action == "execute_command") {
        QString commandId = msgMap.value("commandId").toString();
        executeCommand(commandId);

    } else if (action == "set_shortcut") {
        QString commandId = msgMap.value("commandId").toString();
        QKeySequence shortcut(msgMap.value("shortcut").toString());
        bool success = setShortcut(commandId, shortcut);

        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["success"] = success;
        data["conflicts"] = findConflicts(shortcut);
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);

    } else if (action == "get_commands") {
        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;

        QJsonArray commandsArray;
        for (auto it = m_commands.begin(); it != m_commands.end(); ++it) {
            QJsonObject obj;
            obj["id"] = it.value().id;
            obj["displayName"] = it.value().displayName;
            obj["category"] = it.value().category;
            obj["shortcut"] = it.value().currentShortcut.toString();
            commandsArray.append(obj);
        }
        data["commands"] = commandsArray.toVariantList();
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);
    }
}

// ============================================================================
// IUIExtension
// ============================================================================

QList<QAction*> KeyboardShortcutsPlugin::menuActions() const {
    return m_menuActions;
}

QList<QAction*> KeyboardShortcutsPlugin::toolbarActions() const {
    return QList<QAction*>();
}

// ============================================================================
// Command Management
// ============================================================================

bool KeyboardShortcutsPlugin::registerCommand(const CommandDefinition& cmd) {
    if (cmd.id.isEmpty() || m_commands.contains(cmd.id)) {
        return false;
    }

    m_commands[cmd.id] = cmd;
    m_logger.info(QString("KeyboardShortcutsPlugin: Registered command '%1'")
                      .arg(cmd.id));

    // Create shortcut if has one
    if (!cmd.currentShortcut.isEmpty()) {
        QShortcut* shortcut =
            new QShortcut(cmd.currentShortcut, qApp->activeWindow());
        connect(shortcut, &QShortcut::activated, this,
                [this, cmdId = cmd.id]() { executeCommand(cmdId); });
        m_shortcuts[cmd.id] = shortcut;
    }

    return true;
}

bool KeyboardShortcutsPlugin::unregisterCommand(const QString& commandId) {
    if (!m_commands.contains(commandId)) {
        return false;
    }

    m_commands.remove(commandId);

    if (m_shortcuts.contains(commandId)) {
        delete m_shortcuts.take(commandId);
    }

    return true;
}

void KeyboardShortcutsPlugin::executeCommand(const QString& commandId) {
    if (!m_commands.contains(commandId)) {
        m_logger.warning(
            QString("KeyboardShortcutsPlugin: Command '%1' not found")
                .arg(commandId));
        return;
    }

    const CommandDefinition& cmd = m_commands[commandId];
    if (!cmd.enabled) {
        return;
    }

    // Execute action if defined
    if (cmd.action) {
        cmd.action();
    }

    // Publish command executed event
    Event* event = new Event("command.executed");
    QVariantMap data;
    data["commandId"] = commandId;
    data["displayName"] = cmd.displayName;
    event->setData(QVariant::fromValue(data));
    eventBus()->publish(event);

    m_commandsExecuted++;
    m_logger.debug(
        QString("KeyboardShortcutsPlugin: Executed '%1'").arg(commandId));
}

bool KeyboardShortcutsPlugin::setShortcut(const QString& commandId,
                                          const QKeySequence& shortcut) {
    if (!m_commands.contains(commandId)) {
        return false;
    }

    // Check for conflicts
    QStringList conflicts = findConflicts(shortcut);
    conflicts.removeAll(commandId);
    if (!conflicts.isEmpty()) {
        m_logger.warning(
            QString("KeyboardShortcutsPlugin: Shortcut conflict with %1")
                .arg(conflicts.join(", ")));
    }

    // Update command
    m_commands[commandId].currentShortcut = shortcut;

    // Update shortcut object
    if (m_shortcuts.contains(commandId)) {
        m_shortcuts[commandId]->setKey(shortcut);
    } else if (!shortcut.isEmpty()) {
        QShortcut* sc = new QShortcut(shortcut, qApp->activeWindow());
        connect(sc, &QShortcut::activated, this,
                [this, commandId]() { executeCommand(commandId); });
        m_shortcuts[commandId] = sc;
    }

    return true;
}

QKeySequence KeyboardShortcutsPlugin::getShortcut(
    const QString& commandId) const {
    if (m_commands.contains(commandId)) {
        return m_commands[commandId].currentShortcut;
    }
    return QKeySequence();
}

QStringList KeyboardShortcutsPlugin::getCommandIds() const {
    return m_commands.keys();
}

QStringList KeyboardShortcutsPlugin::findConflicts(
    const QKeySequence& shortcut) const {
    QStringList conflicts;
    if (shortcut.isEmpty()) {
        return conflicts;
    }

    for (auto it = m_commands.begin(); it != m_commands.end(); ++it) {
        if (it.value().currentShortcut == shortcut) {
            conflicts.append(it.key());
        }
    }
    return conflicts;
}

// ============================================================================
// Built-in Commands
// ============================================================================

void KeyboardShortcutsPlugin::registerBuiltinCommands() {
    // Navigation commands
    CommandDefinition nextPage;
    nextPage.id = "navigation.nextPage";
    nextPage.displayName = "Next Page";
    nextPage.description = "Go to next page";
    nextPage.category = "Navigation";
    nextPage.defaultShortcut = QKeySequence("Right");
    nextPage.currentShortcut = nextPage.defaultShortcut;
    nextPage.enabled = true;
    nextPage.action = [this]() {
        eventBus()->publish(new Event("navigation.next"));
    };
    registerCommand(nextPage);

    CommandDefinition prevPage;
    prevPage.id = "navigation.previousPage";
    prevPage.displayName = "Previous Page";
    prevPage.description = "Go to previous page";
    prevPage.category = "Navigation";
    prevPage.defaultShortcut = QKeySequence("Left");
    prevPage.currentShortcut = prevPage.defaultShortcut;
    prevPage.enabled = true;
    prevPage.action = [this]() {
        eventBus()->publish(new Event("navigation.previous"));
    };
    registerCommand(prevPage);

    // View commands
    CommandDefinition zoomIn;
    zoomIn.id = "view.zoomIn";
    zoomIn.displayName = "Zoom In";
    zoomIn.description = "Increase zoom level";
    zoomIn.category = "View";
    zoomIn.defaultShortcut = QKeySequence("Ctrl++");
    zoomIn.currentShortcut = zoomIn.defaultShortcut;
    zoomIn.enabled = true;
    zoomIn.action = [this]() { eventBus()->publish(new Event("view.zoomIn")); };
    registerCommand(zoomIn);

    CommandDefinition zoomOut;
    zoomOut.id = "view.zoomOut";
    zoomOut.displayName = "Zoom Out";
    zoomOut.description = "Decrease zoom level";
    zoomOut.category = "View";
    zoomOut.defaultShortcut = QKeySequence("Ctrl+-");
    zoomOut.currentShortcut = zoomOut.defaultShortcut;
    zoomOut.enabled = true;
    zoomOut.action = [this]() {
        eventBus()->publish(new Event("view.zoomOut"));
    };
    registerCommand(zoomOut);

    CommandDefinition fitWidth;
    fitWidth.id = "view.fitWidth";
    fitWidth.displayName = "Fit Width";
    fitWidth.description = "Fit page to window width";
    fitWidth.category = "View";
    fitWidth.defaultShortcut = QKeySequence("Ctrl+W");
    fitWidth.currentShortcut = fitWidth.defaultShortcut;
    fitWidth.enabled = true;
    fitWidth.action = [this]() {
        eventBus()->publish(new Event("view.fitWidth"));
    };
    registerCommand(fitWidth);

    // Search
    CommandDefinition find;
    find.id = "edit.find";
    find.displayName = "Find";
    find.description = "Open search dialog";
    find.category = "Edit";
    find.defaultShortcut = QKeySequence("Ctrl+F");
    find.currentShortcut = find.defaultShortcut;
    find.enabled = true;
    find.action = [this]() { eventBus()->publish(new Event("edit.find")); };
    registerCommand(find);

    // Toggle sidebar
    CommandDefinition toggleSidebar;
    toggleSidebar.id = "view.toggleSidebar";
    toggleSidebar.displayName = "Toggle Sidebar";
    toggleSidebar.description = "Show/hide sidebar";
    toggleSidebar.category = "View";
    toggleSidebar.defaultShortcut = QKeySequence("Ctrl+B");
    toggleSidebar.currentShortcut = toggleSidebar.defaultShortcut;
    toggleSidebar.enabled = true;
    toggleSidebar.action = [this]() {
        eventBus()->publish(new Event("view.toggleSidebar"));
    };
    registerCommand(toggleSidebar);
}

// ============================================================================
// Persistence
// ============================================================================

void KeyboardShortcutsPlugin::loadShortcuts() {
    QString path =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
        "/shortcuts.json";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonObject shortcuts = doc.object()["shortcuts"].toObject();
    for (auto it = shortcuts.begin(); it != shortcuts.end(); ++it) {
        QString commandId = it.key();
        QString shortcut = it.value().toString();
        if (m_commands.contains(commandId)) {
            m_commands[commandId].currentShortcut = QKeySequence(shortcut);
        }
    }
}

void KeyboardShortcutsPlugin::saveShortcuts() {
    QString path =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
        "/shortcuts.json";

    QJsonObject shortcuts;
    for (auto it = m_commands.begin(); it != m_commands.end(); ++it) {
        if (it.value().currentShortcut != it.value().defaultShortcut) {
            shortcuts[it.key()] = it.value().currentShortcut.toString();
        }
    }

    if (shortcuts.isEmpty()) {
        return;
    }

    QJsonObject root;
    root["shortcuts"] = shortcuts;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

void KeyboardShortcutsPlugin::createShortcuts() {
    // Shortcuts created in registerCommand
}

void KeyboardShortcutsPlugin::destroyShortcuts() {
    qDeleteAll(m_shortcuts);
    m_shortcuts.clear();
}

// ============================================================================
// Slots
// ============================================================================

void KeyboardShortcutsPlugin::onOpenShortcutEditor() {
    m_logger.info("KeyboardShortcutsPlugin: Shortcut editor requested");
    eventBus()->publish(new Event("ui.openShortcutEditor"));
}

void KeyboardShortcutsPlugin::onOpenCommandPalette() {
    m_logger.info("KeyboardShortcutsPlugin: Command palette requested");
    eventBus()->publish(new Event("ui.openCommandPalette"));
}

// ============================================================================
// Hooks
// ============================================================================

void KeyboardShortcutsPlugin::registerHooks() {
    auto& registry = PluginHookRegistry::instance();
    registry.registerCallback(
        "command.execute", name(),
        [this](const QVariantMap& ctx) { return onCommandExecute(ctx); });
}

QVariant KeyboardShortcutsPlugin::onCommandExecute(const QVariantMap& context) {
    QString commandId = context.value("commandId").toString();
    executeCommand(commandId);

    QVariantMap result;
    result["handled"] = m_commands.contains(commandId);
    return result;
}
