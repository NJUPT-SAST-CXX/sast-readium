# Plugin System Enhancements

## Overview

This document describes the enhanced plugin system components that integrate with the existing MVC architecture, ServiceLocator, EventBus, and I18nManager systems in the SAST Readium application.

## Architecture Components

### 1. PluginModel (`app/model/PluginModel.h`)

**Purpose**: Qt Model/View compatible data model for plugin management.

**Key Features**:

- Automatic synchronization with PluginManager
- Filtering and search capabilities (by name, loaded status, enabled status)
- Custom data roles for extended plugin information
- Event-driven updates via signals

**Integration with Existing Systems**:

```cpp
// In MainWindow or initialization code
PluginManager* pluginManager = &PluginManager::instance();

// Register PluginManager with ServiceLocator (if not already done)
ServiceLocator::instance().registerService<PluginManager>(
    []() { return &PluginManager::instance(); });

// Create PluginModel
auto* pluginModel = new PluginModel(pluginManager, this);

// Connect to EventBus for plugin lifecycle events
EventBus::instance().subscribe("plugin.loaded", this, [pluginModel](const QVariant& data) {
    QString pluginName = data.toString();
    pluginModel->refresh();
});

// Use with any Qt view
QTableView* tableView = new QTableView();
tableView->setModel(pluginModel);
```

**Custom Data Roles**:

```cpp
enum PluginDataRole {
    NameRole = Qt::UserRole + 1,
    VersionRole,
    DescriptionRole,
    AuthorRole,
    FilePathRole,
    DependenciesRole,
    SupportedTypesRole,
    FeaturesRole,
    IsLoadedRole,
    IsEnabledRole,
    LoadTimeRole,
    ErrorsRole,
    ConfigurationRole,
    PluginTypeRole,
    StatusTextRole,
    IconRole
};
```

**Filtering Example**:

```cpp
// Filter by text
pluginModel->setFilterText("search term");

// Show only loaded plugins
pluginModel->setShowOnlyLoaded(true);

// Show only enabled plugins
pluginModel->setShowOnlyEnabled(true);

// Clear all filters
pluginModel->clearFilters();
```

### 2. PluginConfigModel (`app/model/PluginConfigModel.h`)

**Purpose**: Table-based model for editing plugin configuration settings.

**Key Features**:

- JSON configuration management
- Type-aware value editing (bool, int, string, etc.)
- Configuration validation
- Real-time configuration updates
- Support for ConfigurePluginCommand integration

**Integration Example**:

```cpp
// Create config model for a specific plugin
auto* configModel = new PluginConfigModel(pluginManager, "MyPlugin", this);

// Load current configuration
configModel->loadConfiguration();

// Use with table view
QTableView* configTable = new QTableView();
configTable->setModel(configModel);

// Programmatically add/modify configuration
configModel->addEntry("newSetting", true, "bool", "Enable feature X");
configModel->setValue("existingSetting", 42);

// Save configuration back to plugin
if (configModel->saveConfiguration()) {
    qDebug() << "Configuration saved successfully";
}

// Connect to signals for change tracking
connect(configModel, &PluginConfigModel::configurationChanged, this, []() {
    qDebug() << "Configuration modified (unsaved)";
});

connect(configModel, &PluginConfigModel::configurationSaved, this, []() {
    qDebug() << "Configuration saved";
});
```

**Using with ConfigurePluginCommand** (for undo/redo support):

```cpp
// Get current configuration from model
QJsonObject newConfig = configModel->getConfiguration();

// Create command
auto cmd = PluginCommandFactory::createConfigureCommand(
    pluginManager, "MyPlugin", newConfig);

// Execute command
if (cmd->execute()) {
    // Configuration applied successfully

    // Command can be added to undo stack
    // undoStack->push(cmd.release());
}
```

### 3. PluginListDelegate (`app/delegate/PluginListDelegate.h`)

**Purpose**: Custom visual rendering for plugin items in lists and tables.

**Key Features**:

- Three display modes: Compact, Normal, Detailed
- Visual status indicators (loaded, disabled, error states)
- Custom colors based on state
- Icon support
- Hover effects

**Display Modes**:

1. **CompactMode**: Single line with minimal info
2. **NormalMode**: Two lines with name, version, and status (default)
3. **DetailedMode**: Multi-line with full details including description, author, etc.

**Integration Example**:

```cpp
// Create delegate
auto* delegate = new PluginListDelegate(this);

// Configure display mode
delegate->setDisplayMode(PluginListDelegate::DetailedMode);

// Customize appearance
delegate->setShowIcons(true);
delegate->setShowStatus(true);
delegate->setHighlightErrors(true);

// Set custom colors
delegate->setLoadedColor(QColor(34, 139, 34));  // Forest green
delegate->setDisabledColor(QColor(128, 128, 128));  // Gray
delegate->setErrorColor(QColor(220, 20, 60));  // Crimson

// Apply to view
QListView* listView = new QListView();
listView->setItemDelegate(delegate);
listView->setModel(pluginModel);
```

### 4. ConfigurePluginCommand (`app/command/PluginCommands.h`)

**Purpose**: Command with undo/redo support for plugin configuration changes.

**Key Features**:

- Stores previous configuration for undo
- Integrates with Qt's QUndoStack
- Proper error handling and validation

**Integration with CommandManager**:

```cpp
#include "../controller/CommandManager.h"

// Get CommandManager from ServiceLocator
auto* commandManager = ServiceLocator::instance().getService<CommandManager>();

// Create configuration change command
QJsonObject newConfig;
newConfig["setting1"] = "value1";
newConfig["setting2"] = 42;

auto* cmd = new ConfigurePluginCommand(pluginManager, "MyPlugin", newConfig);

// Execute through CommandManager for undo/redo support
commandManager->executeCommand(cmd);

// User can now undo/redo this configuration change
// commandManager->undo();
// commandManager->redo();
```

## Internationalization (i18n) Integration

All plugin system components support full internationalization via `I18nManager`.

**Translation Files**:

- `app/i18n/plugin_system_en.ts` - English translations
- `app/i18n/plugin_system_zh.ts` - Chinese translations

**Using Translations in Code**:

```cpp
// All tr() calls automatically use I18nManager
QString status = tr("Active");  // Will show "活动" in Chinese

// Loading specific translation file
I18nManager& i18n = I18nManager::instance();
i18n.loadLanguage(I18nManager::Language::Chinese);

// Models and delegates automatically update when language changes
```

**Translation Contexts**:

- `PluginModel` - Plugin status and type strings
- `PluginConfigModel` - Configuration table headers and error messages
- `PluginListDelegate` - Display strings and labels
- `PluginManagerPage` - UI labels and buttons
- `PluginCommands` - Command status messages
- `PluginManager` - Plugin lifecycle notifications

## ServiceLocator Integration

The plugin system components should be registered with `ServiceLocator` for dependency injection:

```cpp
// In main.cpp or initialization code

void initializePluginSystem() {
    // Register PluginManager
    ServiceLocator::instance().registerService<PluginManager>(
        []() { return &PluginManager::instance(); });

    // Create and register shared PluginModel instance (optional)
    auto* pluginModel = new PluginModel(&PluginManager::instance());
    ServiceLocator::instance().registerService<PluginModel>([pluginModel]() {
        return pluginModel;
    });

    // Register I18nManager (if not already registered)
    ServiceLocator::instance().registerService<I18nManager>(
        []() { return &I18nManager::instance(); });
}

// Usage in application code
auto* pluginManager = ServiceLocator::instance().getService<PluginManager>();
auto* pluginModel = ServiceLocator::instance().getService<PluginModel>();
```

## EventBus Integration

The plugin system publishes and subscribes to events for loose coupling:

**Published Events**:

```cpp
// When plugin is loaded
EventBus::instance().publish("plugin.loaded", pluginName);

// When plugin is unloaded
EventBus::instance().publish("plugin.unloaded", pluginName);

// When plugin is enabled
EventBus::instance().publish("plugin.enabled", pluginName);

// When plugin is disabled
EventBus::instance().publish("plugin.disabled", pluginName);

// When plugin error occurs
QVariantMap errorData;
errorData["pluginName"] = pluginName;
errorData["error"] = errorMessage;
EventBus::instance().publish("plugin.error", errorData);

// When plugin configuration changes
QVariantMap configData;
configData["pluginName"] = pluginName;
configData["configuration"] = config;
EventBus::instance().publish("plugin.config_changed", configData);
```

**Subscribing to Plugin Events**:

```cpp
// In any component that needs to react to plugin changes

// Subscribe to plugin loaded event
EventBus::instance().subscribe("plugin.loaded", this, [this](const QVariant& data) {
    QString pluginName = data.toString();
    qDebug() << "Plugin loaded:" << pluginName;
    updateUI();
});

// Subscribe to plugin errors
EventBus::instance().subscribe("plugin.error", this, [](const QVariant& data) {
    QVariantMap errorData = data.toMap();
    QString pluginName = errorData["pluginName"].toString();
    QString error = errorData["error"].toString();

    // Show error to user
    QMessageBox::warning(nullptr, tr("Plugin Error"),
        tr("Plugin %1 error: %2").arg(pluginName, error));
});

// Subscribe to configuration changes
EventBus::instance().subscribe("plugin.config_changed", this, [](const QVariant& data) {
    QVariantMap configData = data.toMap();
    QString pluginName = configData["pluginName"].toString();

    // Reload plugin if needed
    PluginManager::instance().reloadPlugin(pluginName);
});

// Cleanup: Unsubscribe when component is destroyed
EventBus::instance().unsubscribeAll(this);
```

## Complete Integration Example

Here's a complete example showing how all components work together:

```cpp
// In PluginManagerPage or similar UI component

class PluginManagerPageEnhanced : public QWidget {
    Q_OBJECT

public:
    explicit PluginManagerPageEnhanced(QWidget* parent = nullptr)
        : QWidget(parent) {

        // Get services from ServiceLocator
        m_pluginManager = ServiceLocator::instance().getService<PluginManager>();
        m_commandManager = ServiceLocator::instance().getService<CommandManager>();

        // Create models
        m_pluginModel = new PluginModel(m_pluginManager, this);
        m_configModel = new PluginConfigModel(m_pluginManager, QString(), this);

        // Create delegate
        m_delegate = new PluginListDelegate(this);
        m_delegate->setDisplayMode(PluginListDelegate::NormalMode);

        // Setup UI
        setupUI();

        // Connect to EventBus
        connectToEventBus();

        // Load translations
        retranslateUI();
    }

private:
    void setupUI() {
        auto* layout = new QVBoxLayout(this);

        // Plugin list
        m_listView = new QListView(this);
        m_listView->setModel(m_pluginModel);
        m_listView->setItemDelegate(m_delegate);
        layout->addWidget(m_listView);

        // Configuration table
        m_configTable = new QTableView(this);
        m_configTable->setModel(m_configModel);
        layout->addWidget(m_configTable);

        // Connect selection changes
        connect(m_listView->selectionModel(), &QItemSelectionModel::currentChanged,
                this, &PluginManagerPageEnhanced::onPluginSelectionChanged);
    }

    void connectToEventBus() {
        auto& eventBus = EventBus::instance();

        // Subscribe to plugin events
        eventBus.subscribe("plugin.loaded", this, [this](const QVariant& data) {
            m_pluginModel->refresh();
        });

        eventBus.subscribe("plugin.error", this, [this](const QVariant& data) {
            QVariantMap errorData = data.toMap();
            QString error = errorData["error"].toString();
            QMessageBox::warning(this, tr("Plugin Error"), error);
        });

        // Subscribe to language changes
        connect(&I18nManager::instance(), &I18nManager::languageChanged,
                this, &PluginManagerPageEnhanced::retranslateUI);
    }

    void retranslateUI() {
        // Update all translatable strings
        setWindowTitle(tr("Plugin Manager"));
        // ... update other UI elements
    }

private slots:
    void onPluginSelectionChanged(const QModelIndex& current, const QModelIndex&) {
        if (!current.isValid()) {
            return;
        }

        // Get selected plugin name
        QString pluginName = m_pluginModel->getPluginName(current.row());

        // Load configuration for selected plugin
        m_configModel->setPluginName(pluginName);
        m_configModel->loadConfiguration();
    }

    void onSaveConfiguration() {
        // Get current configuration from model
        QJsonObject newConfig = m_configModel->getConfiguration();
        QString pluginName = m_configModel->pluginName();

        // Create undoable command
        auto* cmd = new ConfigurePluginCommand(
            m_pluginManager, pluginName, newConfig);

        // Execute through CommandManager for undo/redo
        m_commandManager->executeCommand(cmd);

        // Publish event
        QVariantMap configData;
        configData["pluginName"] = pluginName;
        configData["configuration"] = newConfig;
        EventBus::instance().publish("plugin.config_changed", configData);
    }

private:
    PluginManager* m_pluginManager;
    CommandManager* m_commandManager;
    PluginModel* m_pluginModel;
    PluginConfigModel* m_configModel;
    PluginListDelegate* m_delegate;
    QListView* m_listView;
    QTableView* m_configTable;
};
```

## Testing

Comprehensive unit tests are provided in `tests/plugin/test_plugin_enhancements.cpp`:

```bash
# Build and run tests (MSYS2)
cmake --build build/Debug-MSYS2 --target test_plugin_enhancements
./build/Debug-MSYS2/tests/test_plugin_enhancements

# Run specific test
./build/Debug-MSYS2/tests/test_plugin_enhancements --gtest_filter=PluginEnhancementsTest.PluginModelConstruction
```

**Test Coverage**:

- PluginModel construction and data roles
- PluginModel filtering and signals
- PluginConfigModel configuration management
- PluginConfigModel entry add/remove/update
- PluginListDelegate display modes
- ConfigurePluginCommand execution and undo
- Integration between components

## Best Practices

1. **Always use ServiceLocator** for accessing PluginManager and other services
2. **Subscribe to EventBus** for reactive updates instead of polling
3. **Use Commands** for all plugin operations that should support undo/redo
4. **Leverage tr()** for all user-facing strings to support i18n
5. **Filter models** instead of recreating them for better performance
6. **Connect to signals** for reactive UI updates

## Migration Guide

If you have existing code using PluginManager directly:

**Before**:

```cpp
PluginManager* pm = &PluginManager::instance();
QStringList plugins = pm->getAvailablePlugins();
// Manually create list widgets, handle updates, etc.
```

**After**:

```cpp
// Get from ServiceLocator
auto* pm = ServiceLocator::instance().getService<PluginManager>();

// Use PluginModel for automatic updates
auto* model = new PluginModel(pm, this);
listView->setModel(model);

// Subscribe to events for reactive updates
EventBus::instance().subscribe("plugin.loaded", this, [model]() {
    model->refresh();
});
```

## References

- **Core Plugin System**: See [Plugin Development Guide](development-guide.md)
- **Specialized Plugins**: See [Specialized Plugins Guide](specialized-plugins.md)
- **MVC Architecture**: See `docs/architecture.md`
- **Command Pattern**: See `app/command/README.md` (if exists)
- **ServiceLocator**: See `app/controller/ServiceLocator.h`
- **EventBus**: See `app/controller/EventBus.h`
- **I18n System**: See `docs/logging-system.md` (or equivalent i18n docs)
