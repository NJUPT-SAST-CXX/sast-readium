# Plugin UI Integration Guide

This guide explains how plugins can integrate with the SAST Readium user interface through various extension points.

## Table of Contents

1. [Overview](#overview)
2. [UI Extension Architecture](#ui-extension-architecture)
3. [Extension Points](#extension-points)
4. [Implementing UI Extensions](#implementing-ui-extensions)
5. [Menu Extensions](#menu-extensions)
6. [Toolbar Extensions](#toolbar-extensions)
7. [Dock Widget Extensions](#dock-widget-extensions)
8. [Context Menu Extensions](#context-menu-extensions)
9. [Status Bar Extensions](#status-bar-extensions)
10. [Lifecycle and Cleanup](#lifecycle-and-cleanup)
11. [Best Practices](#best-practices)
12. [Examples](#examples)

## Overview

The SAST Readium plugin system provides a comprehensive UI integration framework that allows plugins to extend the application's user interface without modifying core code. Plugins can add:

- **Menu items** - Extend application menus with plugin-specific actions
- **Toolbar buttons** - Add custom toolbar actions
- **Dock widgets** - Register dockable panels (requires QMainWindow compatibility)
- **Context menu items** - Extend context menus throughout the application
- **Status bar messages** - Display plugin status information

## UI Extension Architecture

### Extension Point Pattern

The plugin system uses the **Extension Point** pattern to decouple plugins from the UI:

```
Plugin → IUIExtension interface → Extension Point → UI Component
```

**Key Components:**

1. **IUIExtension** - Interface that plugins implement to provide UI elements
2. **Extension Points** - Bridge between plugins and UI (MenuExtensionPoint, ToolbarExtensionPoint, etc.)
3. **PluginManager** - Registers extension points and applies them to plugins
4. **MainWindow** - Registers extension points during initialization

### Initialization Flow

```
1. Application starts → main.cpp
2. initializePluginSystem() called
   - Registers DocumentHandlerExtensionPoint
   - Scans and loads plugins
3. MainWindow created
4. MainWindow::initPluginUIExtensions() called
   - Registers MenuExtensionPoint
   - Registers ToolbarExtensionPoint
   - Registers ContextMenuExtensionPoint
   - Registers StatusBarExtensionPoint
5. Extension points applied to loaded plugins
6. Plugin UI elements appear in application
```

## Extension Points

### Available Extension Points

| Extension Point | ID | Description | Requirements |
|----------------|----|--------------| -------------|
| MenuExtensionPoint | `org.sast.readium.menu` | Adds menu items | Plugin provides "menu" or "ui.menu" capability |
| ToolbarExtensionPoint | `org.sast.readium.toolbar` | Adds toolbar buttons | Plugin provides "toolbar" or "ui.toolbar" capability |
| DockWidgetExtensionPoint | `org.sast.readium.dock_widget` | Adds dockable widgets | Plugin provides "dock_widget" capability, requires QMainWindow |
| ContextMenuExtensionPoint | `org.sast.readium.context_menu` | Extends context menus | Plugin provides "context_menu" capability |
| StatusBarExtensionPoint | `org.sast.readium.status_bar` | Displays status messages | Plugin provides "status_bar" capability |
| DocumentHandlerExtensionPoint | `org.sast.readium.document_handler` | Registers document handlers | Plugin provides "document.handler" capability |

### How Extension Points Work

1. **Registration**: Extension points are registered with PluginManager
2. **Plugin Loading**: When a plugin loads, PluginManager applies all registered extension points
3. **Acceptance Check**: Each extension point checks if it accepts the plugin (via plugin capabilities)
4. **Extension**: If accepted, the extension point extends the plugin into the UI

## Implementing UI Extensions

### Step 1: Declare Capabilities in Plugin Metadata

In your `my_plugin.json`:

```json
{
    "name": "My UI Plugin",
    "version": "1.0.0",
    "features": ["menu", "toolbar", "status_bar"],
    "configuration": {}
}
```

### Step 2: Inherit from IUIExtension

```cpp
#include "plugin/PluginInterface.h"

class MyPlugin : public PluginBase, public IUIExtension {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE "my_plugin.json")
    Q_INTERFACES(IPluginInterface)

public:
    explicit MyPlugin(QObject* parent = nullptr);

protected:
    // PluginBase overrides
    bool onInitialize() override;
    void onShutdown() override;

    // IUIExtension overrides
    QList<QAction*> menuActions() const override;
    QString menuPath() const override;

    QList<QAction*> toolbarActions() const override;

    QString statusBarMessage() const override;
    int statusBarTimeout() const override;

private:
    QList<QAction*> m_menuActions;
    QList<QAction*> m_toolbarActions;
};
```

### Step 3: Declare Capabilities in Constructor

```cpp
MyPlugin::MyPlugin(QObject* parent) : PluginBase(parent) {
    m_metadata.name = "My UI Plugin";
    m_metadata.version = "1.0.0";
    m_metadata.description = "Demonstrates UI integration";
    m_metadata.author = "Your Name";

    // Declare UI capabilities
    m_capabilities.provides = QStringList()
        << "menu"
        << "toolbar"
        << "status_bar";
}
```

### Step 4: Implement UI Methods

```cpp
bool MyPlugin::onInitialize() {
    m_logger.info("MyPlugin initializing...");

    // Create menu actions
    QAction* action1 = new QAction("My Action", this);
    connect(action1, &QAction::triggered, this, &MyPlugin::onAction1Triggered);
    m_menuActions.append(action1);

    QAction* action2 = new QAction("My Settings", this);
    connect(action2, &QAction::triggered, this, &MyPlugin::onAction2Triggered);
    m_menuActions.append(action2);

    // Create toolbar actions
    QAction* toolbarAction = new QAction(QIcon(":/icons/my_icon.png"), "Quick Action", this);
    connect(toolbarAction, &QAction::triggered, this, &MyPlugin::onQuickAction);
    m_toolbarActions.append(toolbarAction);

    return true;
}

void MyPlugin::onShutdown() {
    // Cleanup is handled automatically by PluginManager
    m_logger.info("MyPlugin shutting down...");
}

QList<QAction*> MyPlugin::menuActions() const {
    return m_menuActions;
}

QString MyPlugin::menuPath() const {
    return "Tools/My Plugin";  // Plugins menu will be created automatically
}

QList<QAction*> MyPlugin::toolbarActions() const {
    return m_toolbarActions;
}

QString MyPlugin::statusBarMessage() const {
    return "My Plugin is active and ready";
}

int MyPlugin::statusBarTimeout() const {
    return 3000;  // Show for 3 seconds, 0 = permanent
}
```

## Menu Extensions

### How It Works

1. Plugin declares "menu" or "ui.menu" in capabilities
2. Plugin implements `menuActions()` and optionally `menuPath()`
3. MenuExtensionPoint creates a "Plugins" menu if it doesn't exist
4. Plugin submenu is added with plugin actions

### Current Implementation

The current implementation creates a simple menu structure:

```
Main Menu Bar
└── Plugins (auto-created)
    └── [Plugin Name]
        └── About [Plugin Name]
```

### Future Enhancement

Plugins will be able to specify custom menu paths like:

```cpp
QString MyPlugin::menuPath() const {
    return "Tools/My Plugin/Submenu";  // Hierarchical menu structure
}
```

## Toolbar Extensions

### How It Works

1. Plugin declares "toolbar" or "ui.toolbar" in capabilities
2. Plugin implements `toolbarActions()` and optionally `toolbarName()`
3. ToolbarExtensionPoint retrieves the main toolbar from ServiceLocator
4. Plugin actions are added to the toolbar with a separator

### Example

```cpp
QList<QAction*> MyPlugin::toolbarActions() const {
    QList<QAction*> actions;

    QAction* action = new QAction(QIcon(":/my_icon.png"), "My Tool", nullptr);
    action->setToolTip("My plugin tool");
    action->setStatusTip("Execute my plugin tool");
    connect(action, &QAction::triggered, this, &MyPlugin::onToolTriggered);

    actions.append(action);
    return actions;
}

QString MyPlugin::toolbarName() const {
    return "My Plugin Toolbar";  // Optional: Create separate toolbar
}
```

## Dock Widget Extensions

### How It Works

1. Plugin declares "dock_widget", "ui.dock", or "ui.dockwidget" in capabilities
2. Plugin implements `createDockWidget()`, `dockWidgetTitle()`, and `dockWidgetArea()`
3. DockWidgetExtensionPoint creates a QDockWidget and adds it to MainWindow

### Requirements

**Note**: Dock widgets require `QMainWindow`. Since SAST Readium uses `ElaWindow` (which doesn't inherit from QMainWindow), dock widget support is currently limited. This will be addressed in future updates.

### Example (for future compatibility)

```cpp
QWidget* MyPlugin::createDockWidget(QWidget* parent) {
    QWidget* widget = new QWidget(parent);
    QVBoxLayout* layout = new QVBoxLayout(widget);

    QLabel* label = new QLabel("My Plugin Panel", widget);
    QPushButton* button = new QPushButton("Action", widget);

    layout->addWidget(label);
    layout->addWidget(button);

    return widget;
}

QString MyPlugin::dockWidgetTitle() const {
    return "My Plugin Panel";
}

Qt::DockWidgetArea MyPlugin::dockWidgetArea() const {
    return Qt::RightDockWidgetArea;  // or Left, Top, Bottom
}
```

## Context Menu Extensions

### How It Works

1. Plugin declares "context_menu" or "ui.context_menu" in capabilities
2. Plugin implements `contextMenuActions(const QString& contextId)`
3. ContextMenuExtensionPoint registers the plugin as a context menu provider
4. Application context menus query registered providers for actions

### Example

```cpp
QList<QAction*> MyPlugin::contextMenuActions(const QString& contextId) const {
    QList<QAction*> actions;

    if (contextId == "pdf.document") {
        QAction* analyzeAction = new QAction("Analyze with My Plugin", nullptr);
        connect(analyzeAction, &QAction::triggered, this, &MyPlugin::onAnalyze);
        actions.append(analyzeAction);
    }

    if (contextId == "pdf.selection") {
        QAction* processAction = new QAction("Process Selection", nullptr);
        connect(processAction, &QAction::triggered, this, &MyPlugin::onProcess);
        actions.append(processAction);
    }

    return actions;
}
```

### Context IDs

Common context IDs (application-specific):

- `pdf.document` - Document-level context menu
- `pdf.page` - Page-level context menu
- `pdf.selection` - Selection context menu
- `annotation` - Annotation context menu

## Status Bar Extensions

### How It Works

1. Plugin declares "status_bar" or "ui.status_bar" in capabilities
2. Plugin implements `statusBarMessage()` and `statusBarTimeout()`
3. StatusBarExtensionPoint displays the message in the status bar

### Example

```cpp
QString MyPlugin::statusBarMessage() const {
    return QString("My Plugin: %1 items processed").arg(m_processedCount);
}

int MyPlugin::statusBarTimeout() const {
    return 0;  // 0 = permanent until replaced
    // return 5000;  // 5 seconds
}
```

### Dynamic Status Updates

To update status dynamically:

```cpp
void MyPlugin::updateStatus() {
    // Get status bar from ServiceLocator
    auto& serviceLocator = ServiceLocator::instance();
    QMainWindow* mainWindow = serviceLocator.getService<QMainWindow>();

    if (mainWindow && mainWindow->statusBar()) {
        QString message = QString("Processing: %1%").arg(m_progress);
        mainWindow->statusBar()->showMessage(message);
    }
}
```

## Lifecycle and Cleanup

### Automatic Cleanup

The PluginManager automatically tracks and cleans up all UI elements created by plugins:

1. **On Plugin Load**: UI elements are registered with `PluginManager::registerPluginUIElement()`
2. **On Plugin Unload**: `PluginManager::cleanupPluginUIElements()` is called automatically
3. **Cleanup Process**:
   - Removes widgets from parent
   - Calls `deleteLater()` on all tracked UI objects
   - Clears tracking list

### Manual Cleanup (Optional)

If your plugin creates UI elements outside the extension point system:

```cpp
void MyPlugin::onShutdown() {
    // Extension point UI is cleaned up automatically
    // Only clean up additional UI elements here

    if (m_customDialog) {
        m_customDialog->close();
        m_customDialog->deleteLater();
        m_customDialog = nullptr;
    }
}
```

### UI Element Tracking

The PluginManager maintains a hash map of plugin UI elements:

```cpp
// Internal PluginManager structure
QHash<QString, QList<QObject*>> m_pluginUIElements;

// When plugin loads and creates UI
void PluginManager::applyExtensionPoints(IPluginInterface* plugin) {
    for (IExtensionPoint* ep : m_extensionPoints) {
        if (ep->accepts(plugin)) {
            ep->extend(plugin);
            // UI elements created during extend() are tracked
        }
    }
}

// When plugin unloads
void PluginManager::unloadPluginInternal(const QString& pluginName) {
    cleanupPluginUIElements(pluginName);  // Automatic cleanup
    // ... rest of unload process
}
```

## Best Practices

### 1. Memory Management

- **Use parent-child relationships**: Always pass proper parent to QWidgets
- **Don't delete tracked UI**: Extension point UI is managed by PluginManager
- **Use QPointer for references**: Store weak references to UI elements

```cpp
class MyPlugin : public PluginBase {
private:
    QPointer<QAction> m_menuAction;  // Weak reference
    QPointer<QWidget> m_dockWidget;  // Weak reference
};
```

### 2. Thread Safety

- **UI operations on main thread**: All UI operations must be on the main thread
- **Use signals/slots for cross-thread**: Never manipulate UI from worker threads

```cpp
// Worker thread
void MyPlugin::processData() {
    // Do processing...

    // Update UI via signal (thread-safe)
    emit statusUpdated("Processing complete");
}

// Main thread
void MyPlugin::onStatusUpdated(const QString& status) {
    // Safe to update UI here
    if (m_statusLabel) {
        m_statusLabel->setText(status);
    }
}
```

### 3. Resource Cleanup

- **Disconnect signals**: Disconnect all signal/slot connections in shutdown
- **Stop timers**: Stop and delete any QTimers
- **Close dialogs**: Close any open dialogs or windows

```cpp
void MyPlugin::onShutdown() {
    // Disconnect signals
    eventBus()->unsubscribeAll(this);

    // Stop timers
    if (m_updateTimer) {
        m_updateTimer->stop();
        delete m_updateTimer;
        m_updateTimer = nullptr;
    }

    // Close dialogs
    if (m_settingsDialog) {
        m_settingsDialog->close();
    }
}
```

### 4. Error Handling

- **Check for null pointers**: Always validate UI element pointers
- **Handle missing services**: ServiceLocator might not have all services
- **Graceful degradation**: Plugin should work even if some UI features fail

```cpp
QList<QAction*> MyPlugin::menuActions() const {
    QList<QAction*> actions;

    try {
        QAction* action = new QAction("My Action", nullptr);
        if (action) {
            connect(action, &QAction::triggered, this, &MyPlugin::onActionTriggered);
            actions.append(action);
        }
    } catch (const std::exception& e) {
        m_logger.error("Failed to create menu action: {}", e.what());
    }

    return actions;
}
```

### 5. Icon and Resource Management

- **Use Qt Resource System**: Store icons in .qrc files
- **Provide fallback icons**: Handle missing icons gracefully
- **Scale-aware icons**: Provide different icon sizes

```cpp
QAction* MyPlugin::createAction() {
    QAction* action = new QAction("My Action", this);

    // Try to load icon, provide fallback
    QIcon icon(":/my_plugin/icons/action.png");
    if (icon.isNull()) {
        m_logger.warning("Failed to load plugin icon");
        icon = QIcon::fromTheme("document-new");  // Fallback
    }

    action->setIcon(icon);
    return action;
}
```

## Examples

### Complete Example: Simple UI Plugin

```cpp
// SimpleUIPlugin.h
#pragma once
#include "plugin/PluginInterface.h"
#include <QAction>

class SimpleUIPlugin : public PluginBase, public IUIExtension {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE "simple_ui_plugin.json")
    Q_INTERFACES(IPluginInterface)

public:
    explicit SimpleUIPlugin(QObject* parent = nullptr);

protected:
    bool onInitialize() override;
    void onShutdown() override;

    // IUIExtension interface
    QList<QAction*> menuActions() const override;
    QString menuPath() const override;
    QList<QAction*> toolbarActions() const override;
    QString statusBarMessage() const override;
    int statusBarTimeout() const override;

private slots:
    void onShowInfo();
    void onQuickAction();

private:
    QList<QAction*> m_menuActions;
    QList<QAction*> m_toolbarActions;
    int m_clickCount = 0;
};

// SimpleUIPlugin.cpp
#include "SimpleUIPlugin.h"
#include <QMessageBox>

SimpleUIPlugin::SimpleUIPlugin(QObject* parent) : PluginBase(parent) {
    m_metadata.name = "Simple UI Plugin";
    m_metadata.version = "1.0.0";
    m_metadata.description = "Demonstrates UI integration";
    m_metadata.author = "Plugin Developer";

    m_capabilities.provides = QStringList()
        << "menu"
        << "toolbar"
        << "status_bar";
}

bool SimpleUIPlugin::onInitialize() {
    m_logger.info("SimpleUIPlugin initializing...");

    // Create menu actions
    QAction* infoAction = new QAction("Show Info", this);
    infoAction->setStatusTip("Show plugin information");
    connect(infoAction, &QAction::triggered, this, &SimpleUIPlugin::onShowInfo);
    m_menuActions.append(infoAction);

    // Create toolbar actions
    QAction* quickAction = new QAction(QIcon(":/icons/quick.png"), "Quick", this);
    quickAction->setToolTip("Quick action");
    quickAction->setStatusTip("Execute quick action");
    connect(quickAction, &QAction::triggered, this, &SimpleUIPlugin::onQuickAction);
    m_toolbarActions.append(quickAction);

    m_logger.info("SimpleUIPlugin initialized successfully");
    return true;
}

void SimpleUIPlugin::onShutdown() {
    m_logger.info("SimpleUIPlugin shutting down...");
    // Extension point UI is cleaned up automatically
}

QList<QAction*> SimpleUIPlugin::menuActions() const {
    return m_menuActions;
}

QString SimpleUIPlugin::menuPath() const {
    return "Plugins/Simple UI Plugin";
}

QList<QAction*> SimpleUIPlugin::toolbarActions() const {
    return m_toolbarActions;
}

QString SimpleUIPlugin::statusBarMessage() const {
    return QString("Simple UI Plugin ready (clicks: %1)").arg(m_clickCount);
}

int SimpleUIPlugin::statusBarTimeout() const {
    return 0;  // Permanent
}

void SimpleUIPlugin::onShowInfo() {
    QString info = QString(
        "Plugin: %1\n"
        "Version: %2\n"
        "Author: %3\n\n"
        "%4\n\n"
        "Total clicks: %5"
    ).arg(m_metadata.name)
     .arg(m_metadata.version)
     .arg(m_metadata.author)
     .arg(m_metadata.description)
     .arg(m_clickCount);

    QMessageBox::information(nullptr, "Plugin Information", info);
}

void SimpleUIPlugin::onQuickAction() {
    m_clickCount++;
    m_logger.info("Quick action executed (total clicks: {})", m_clickCount);

    // Update status bar (if needed)
    // Status bar will automatically show our statusBarMessage()
}
```

### simple_ui_plugin.json

```json
{
    "name": "Simple UI Plugin",
    "version": "1.0.0",
    "description": "Demonstrates UI integration with menu, toolbar, and status bar",
    "author": "Plugin Developer",
    "dependencies": [],
    "features": ["menu", "toolbar", "status_bar"],
    "configuration": {
        "showWelcomeMessage": true,
        "enableQuickAction": true
    }
}
```

## See Also

- [Plugin Development Guide](plugin-development.md) - General plugin development
- [Plugin System Architecture](../llmdoc/feature/plugin-system.md) - Technical architecture details
- [Example Plugins](../examples/plugins/) - More plugin examples
- [API Reference](../docs/api-reference.md) - Complete API documentation

## Support

For questions or issues:

- Check existing plugins in `examples/plugins/`
- Review the plugin system source code in `app/plugin/`
- Consult the technical documentation in `llmdoc/feature/`
- Submit issues on the project repository
