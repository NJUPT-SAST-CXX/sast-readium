# Plugin Configuration System

This document describes the enhanced plugin configuration system that provides user-friendly configuration management for plugins.

## Overview

The plugin configuration system allows plugins to define their configuration options using a JSON schema, which is then used to automatically generate appropriate UI controls for editing settings.

## Key Components

### 1. PluginConfigModel (`app/model/PluginConfigModel.h`)

Enhanced Qt Model that manages plugin configuration with:

- **ConfigEntry**: Extended structure with schema metadata
  - `key`, `value`, `type` - Basic configuration data
  - `description`, `displayName` - Human-readable text
  - `group` - Configuration grouping
  - `isRequired` - Required field marker
  - `minValue`, `maxValue` - Numeric constraints
  - `enumValues` - Valid values for enum type
  - `defaultValue` - Default value

- **ConfigGroup**: Group metadata
  - `id`, `displayName`, `description`
  - `order` - Display order
  - `isAdvanced` - Hide by default

### 2. PluginConfigWidget (`app/ui/widgets/PluginConfigWidget.h`)

Dynamic configuration editor widget that generates appropriate controls based on config type:

| Type | Control |
|------|---------|
| `bool` | ElaToggleSwitch |
| `int` | ElaSpinBox or ElaSlider (if range defined) |
| `double` | ElaDoubleSpinBox |
| `string` | ElaLineEdit |
| `text`/`textarea` | QTextEdit (multi-line) |
| `enum` | ElaComboBox |
| `path` | ElaLineEdit + Browse (directory) |
| `file` | ElaLineEdit + Browse (file) |
| `color` | Color picker button |

**Features:**

- Automatic UI generation from schema
- Real-time validation with inline error display
- Group-based organization
- Required-only mode for setup wizard
- Search/filter functionality for large configurations
- Validation error highlighting on labels

### 3. PluginConfigDialog (`app/ui/dialogs/PluginConfigDialog.h`)

Complete configuration dialog with:

- Plugin info header (name, version, description)
- **Search bar** for filtering configuration entries
- Configuration editor with grouped settings
- Import/Export functionality (JSON format)
- Reset to defaults
- Real-time validation display
- Advanced settings toggle

### 4. PluginSetupWizard (`app/ui/dialogs/PluginSetupWizard.h`)

First-run configuration wizard for plugins with required settings:

**Pages:**

1. **Welcome** - Plugin introduction
2. **Required Settings** - Only required configuration items
3. **Optional Settings** - Other configuration items
4. **Completion** - Configuration summary

### 5. PluginManager Extensions

New methods in `PluginManager`:

```cpp
// Schema Management
QJsonObject getPluginConfigSchema(const QString& pluginName) const;
bool hasConfigSchema(const QString& pluginName) const;
bool validatePluginConfiguration(const QString& pluginName, QStringList* errors = nullptr) const;

// First-run Support
bool isPluginConfigured(const QString& pluginName) const;
void markPluginConfigured(const QString& pluginName, bool configured = true);
bool needsSetupWizard(const QString& pluginName) const;
QStringList getRequiredConfigKeys(const QString& pluginName) const;
```

## Configuration Schema Format

Plugins define their configuration schema in the JSON metadata file:

```json
{
    "configuration": {
        "configSchema": {
            "groups": {
                "general": {
                    "displayName": "General Settings",
                    "description": "Basic plugin configuration",
                    "order": 0
                },
                "advanced": {
                    "displayName": "Advanced Settings",
                    "order": 1,
                    "advanced": true
                }
            },
            "properties": {
                "greeting": {
                    "type": "string",
                    "displayName": "Greeting Message",
                    "description": "The message to display",
                    "default": "Hello!",
                    "placeholder": "Enter message...",
                    "group": "general",
                    "required": false,
                    "order": 0
                },
                "maxItems": {
                    "type": "int",
                    "displayName": "Maximum Items",
                    "description": "Maximum number of items",
                    "default": 100,
                    "minimum": 1,
                    "maximum": 1000,
                    "group": "advanced",
                    "order": 0
                },
                "displayMode": {
                    "type": "string",
                    "displayName": "Display Mode",
                    "default": "normal",
                    "enum": ["normal", "compact", "expanded"],
                    "group": "general",
                    "order": 1
                }
            }
        }
    }
}
```

### Schema Property Fields

| Field | Type | Description |
|-------|------|-------------|
| `type` | string | `bool`, `int`, `double`, `string`, `text`, `enum`, `path`, `file`, `color` |
| `displayName` | string | Human-readable name |
| `description` | string | Tooltip/help text |
| `default` | any | Default value |
| `group` | string | Group ID (default: "general") |
| `required` | bool | Whether required for plugin to work |
| `readOnly` | bool | Cannot be edited |
| `order` | int | Display order within group |
| `placeholder` | string | Placeholder text for inputs |
| `minimum` | number | Minimum value (numeric types) |
| `maximum` | number | Maximum value (numeric types) |
| `enum` | array | Valid values for enum type |

### Group Fields

| Field | Type | Description |
|-------|------|-------------|
| `displayName` | string | Human-readable group name |
| `description` | string | Group description |
| `order` | int | Display order |
| `advanced` | bool | Hide in normal view |
| `collapsible` | bool | Can be collapsed |

## User Workflow

### Enabling a Plugin with Required Settings

1. User clicks "Enable" on a plugin in Plugin Manager
2. System checks if plugin has required configuration (`needsSetupWizard()`)
3. If yes, `PluginSetupWizard` is shown
4. User completes required settings
5. Plugin is enabled and marked as configured

### Configuring an Enabled Plugin

1. User selects plugin in Plugin Manager
2. User clicks "Configure" button
3. `PluginConfigDialog` opens with all settings
4. User can:
   - Edit configuration values
   - Reset to defaults
   - Import/Export configuration
5. Changes are saved and applied to running plugin

## Integration Example

```cpp
// In PluginManagerPage::onConfigureClicked()
void PluginManagerPage::onConfigureClicked() {
    if (!m_pluginManager->hasConfigSchema(m_selectedPluginName)) {
        // No configuration available
        return;
    }

    PluginConfigDialog dialog(m_selectedPluginName, this);
    if (dialog.exec() == QDialog::Accepted) {
        m_pluginManager->markPluginConfigured(m_selectedPluginName, true);
    }
}

// In PluginManagerPage::onEnableDisableClicked()
if (m_pluginManager->needsSetupWizard(pluginName)) {
    PluginSetupWizard wizard(pluginName, this);
    if (wizard.exec() != QDialog::Accepted) {
        return; // Don't enable if wizard not completed
    }
}
```

## Files

| File | Description |
|------|-------------|
| `app/model/PluginConfigModel.h/cpp` | Configuration model with schema support |
| `app/ui/widgets/PluginConfigWidget.h/cpp` | Dynamic configuration editor |
| `app/ui/dialogs/PluginConfigDialog.h/cpp` | Configuration dialog |
| `app/ui/dialogs/PluginSetupWizard.h/cpp` | First-run wizard |
| `app/plugin/PluginManager.h/cpp` | Schema and first-run detection methods |
| `app/ui/pages/PluginManagerPage.cpp` | Integration point |

## See Also

- `examples/plugins/hello_plugin/hello_plugin.json` - Basic schema example
- `examples/plugins/reading_progress/reading_progress.json` - Schema with required fields
