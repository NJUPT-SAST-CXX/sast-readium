# Theme Provider Plugin

This plugin demonstrates custom theming with `IUIExtension` and settings persistence.

## Features

- **6 Built-in Themes**: Light, Dark, Sepia, High Contrast, Nord, Solarized Light
- **Custom Theme Support**: Register and save custom themes
- **Dynamic Switching**: Real-time theme application via EventBus
- **StyleSheet Generation**: Automatic Qt stylesheet generation
- **Settings Persistence**: Save/load theme preferences
- **Menu Integration**: Theme selector in View menu

## Built-in Themes

| Theme | Type | Description |
|-------|------|-------------|
| Light | Light | Clean white background |
| Dark | Dark | Dark gray background |
| Sepia | Light | Warm paper-like tones |
| High Contrast | Dark | Maximum contrast for accessibility |
| Nord | Dark | Arctic, bluish color palette |
| Solarized Light | Light | Precision colors for readability |

## ThemeDefinition Structure

```cpp
struct ThemeDefinition {
    QString name;           // Internal identifier
    QString displayName;    // UI display name
    QColor backgroundColor;
    QColor textColor;
    QColor accentColor;
    QColor highlightColor;
    QColor borderColor;
    QString customStyleSheet;
    bool isDark;
};
```

## Inter-plugin Communication

### Get Available Themes

```cpp
QVariantMap msg;
msg["action"] = "get_themes";
pluginManager->sendMessage("Theme Provider", msg);
// Response: { "themes": [...], "activeTheme": "dark" }
```

### Set Theme

```cpp
QVariantMap msg;
msg["action"] = "set_theme";
msg["theme"] = "nord";
pluginManager->sendMessage("Theme Provider", msg);
```

### Register Custom Theme

```cpp
QVariantMap msg;
msg["action"] = "register_theme";
msg["name"] = "custom_ocean";
msg["displayName"] = "Ocean Blue";
msg["backgroundColor"] = "#0D1B2A";
msg["textColor"] = "#E0E1DD";
msg["accentColor"] = "#3D5A80";
msg["isDark"] = true;
pluginManager->sendMessage("Theme Provider", msg);
```

## Events Published

### theme.changed

Published when theme changes:

```cpp
{
    "themeName": "dark",
    "displayName": "Dark",
    "isDark": true,
    "backgroundColor": "#1E1E1E",
    "textColor": "#E0E0E0",
    "accentColor": "#64B5F6",
    "styleSheet": "..." // Generated Qt stylesheet
}
```

## Configuration

```json
{
    "activeTheme": "light",
    "enableCustomThemes": true,
    "autoSaveTheme": true
}
```

## Building

```bash
mkdir build && cd build
cmake .. && cmake --build .
```
