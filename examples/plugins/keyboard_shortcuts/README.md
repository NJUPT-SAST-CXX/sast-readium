# Keyboard Shortcuts Plugin

This plugin demonstrates command registration and keyboard shortcut customization.

## Features

- **Command Registration**: Register custom commands with actions
- **Shortcut Customization**: Rebindable keyboard shortcuts
- **Conflict Detection**: Detect and warn about shortcut conflicts
- **Settings Persistence**: Save/load custom shortcut bindings
- **Command Palette**: Quick command access (Ctrl+Shift+P)
- **Built-in Commands**: Navigation, view, and edit commands

## Built-in Commands

| Command ID | Display Name | Default Shortcut | Category |
|------------|--------------|------------------|----------|
| `navigation.nextPage` | Next Page | Right | Navigation |
| `navigation.previousPage` | Previous Page | Left | Navigation |
| `view.zoomIn` | Zoom In | Ctrl++ | View |
| `view.zoomOut` | Zoom Out | Ctrl+- | View |
| `view.fitWidth` | Fit Width | Ctrl+W | View |
| `view.toggleSidebar` | Toggle Sidebar | Ctrl+B | View |
| `edit.find` | Find | Ctrl+F | Edit |

## CommandDefinition Structure

```cpp
struct CommandDefinition {
    QString id;                    // Unique identifier
    QString displayName;           // UI display name
    QString description;           // Tooltip/help text
    QString category;              // Grouping category
    QKeySequence defaultShortcut;  // Factory default
    QKeySequence currentShortcut;  // User-customized
    std::function<void()> action;  // Callback function
    bool enabled;
};
```

## Inter-plugin Communication

### Register Command

```cpp
QVariantMap msg;
msg["action"] = "register_command";
msg["id"] = "myPlugin.doSomething";
msg["displayName"] = "Do Something";
msg["category"] = "My Plugin";
msg["shortcut"] = "Ctrl+Alt+D";
pluginManager->sendMessage("Keyboard Shortcuts", msg);
```

### Execute Command

```cpp
QVariantMap msg;
msg["action"] = "execute_command";
msg["commandId"] = "view.zoomIn";
pluginManager->sendMessage("Keyboard Shortcuts", msg);
```

### Set Shortcut

```cpp
QVariantMap msg;
msg["action"] = "set_shortcut";
msg["commandId"] = "navigation.nextPage";
msg["shortcut"] = "PageDown";
pluginManager->sendMessage("Keyboard Shortcuts", msg);
// Response includes conflicts if any
```

### Get All Commands

```cpp
QVariantMap msg;
msg["action"] = "get_commands";
pluginManager->sendMessage("Keyboard Shortcuts", msg);
// Response: { "commands": [...] }
```

## Events Published

### command.executed

```cpp
{
    "commandId": "view.zoomIn",
    "displayName": "Zoom In"
}
```

## Conflict Detection

When setting a shortcut that's already in use:

```cpp
QStringList conflicts = findConflicts(QKeySequence("Ctrl+F"));
// Returns: ["edit.find"] if Ctrl+F is already bound
```

## Configuration

```json
{
    "enableCommandPalette": true,
    "showShortcutsInMenu": true,
    "autoSave": true
}
```

## Building

```bash
mkdir build && cd build
cmake .. && cmake --build .
```
