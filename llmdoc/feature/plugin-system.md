# Plugin System Implementation

## Overview

The SAST Readium plugin system provides a comprehensive, extensible architecture for adding functionality to the application without modifying the core codebase. The system supports dynamic plugin loading, dependency management, event-driven communication, and service integration.

## Architecture

### Component Hierarchy

```
PluginManager (Singleton)
├── Plugin Discovery & Scanning
├── Dependency Resolution
├── Plugin Loading/Unloading
├── Lifecycle Management
└── Settings Persistence

IPluginInterface (Abstract)
├── PluginBase (Base Implementation)
│   ├── Metadata Management
│   ├── Service Access
│   ├── Event Subscription
│   └── Configuration
└── Derived Plugin Types
    ├── IPlugin (Qt Plugin Interface)
    ├── IDocumentPlugin (Document Processing)
    └── IUIPlugin (UI Extensions)
```

### Key Components

#### 0. Specialized Plugin Interfaces

Five domain-specific plugin interfaces extend core functionality:

- **IDocumentProcessorPlugin** - Document processing, metadata extraction, export
- **IRenderPlugin** - Page rendering filters, overlays, transformations
- **ISearchPlugin** - Custom search algorithms, result ranking, indexing
- **ICacheStrategyPlugin** - Cache eviction policies, persistence strategies
- **IAnnotationPlugin** - Annotation types, import/export, rendering

These interfaces hook into **20+ workflow stages** via the centralized **PluginHookRegistry** system. See [Specialized Plugin Interfaces](specialized-plugin-interfaces.md) for full documentation.

#### 1. PluginManager (`app/plugin/PluginManager.h`)

**Responsibilities**:

- Plugin discovery and scanning
- Loading/unloading plugins
- Dependency resolution
- State persistence
- Plugin queries and metadata access

**Key Features**:

- Singleton pattern for global access
- Qt Plugin Loader integration
- Automatic dependency resolution
- Cyclic dependency detection
- Hot reloading support
- Settings persistence via QSettings

**Core API**:

```cpp
// Plugin management
bool loadPlugin(const QString& pluginName);
bool unloadPlugin(const QString& pluginName);
void loadAllPlugins();
void unloadAllPlugins();

// Plugin discovery
void scanForPlugins();
QStringList getAvailablePlugins() const;
QStringList getLoadedPlugins() const;

// Plugin state
bool isPluginLoaded(const QString& pluginName) const;
bool isPluginEnabled(const QString& pluginName) const;
void setPluginEnabled(const QString& pluginName, bool enabled);

// Metadata access
PluginMetadata getPluginMetadata(const QString& pluginName) const;
QHash<QString, PluginMetadata> getAllPluginMetadata() const;

// Specialized plugin access
QList<IDocumentProcessorPlugin*> getDocumentProcessorPlugins() const;
QList<IRenderPlugin*> getRenderPlugins() const;
QList<ISearchPlugin*> getSearchPlugins() const;
QList<ICacheStrategyPlugin*> getCacheStrategyPlugins() const;
QList<IAnnotationPlugin*> getAnnotationPlugins() const;

// Hook registry management
void registerStandardHooks();
void unregisterAllHooks(const QString& pluginName);
```

#### 2. PluginInterface (`app/plugin/PluginInterface.h`)

**Interface Hierarchy**:

- `IPluginInterface`: Pure C++ interface
- `PluginBase`: Base implementation with common functionality
- `IPlugin`: Qt plugin interface (Q_PLUGIN_METADATA)
- `IDocumentPlugin`: Document processing extensions
- `IUIPlugin`: UI component extensions

**PluginBase Features**:

- Metadata management
- Service locator access
- Event bus integration
- Command manager access
- Configuration management
- Logging support

**Lifecycle Methods**:

```cpp
// Override in derived classes
virtual bool onInitialize() = 0;
virtual void onShutdown() = 0;

// Helper methods
ServiceLocator* serviceLocator();
EventBus* eventBus();
CommandManager* commandManager();
ConfigurationManager* configurationManager();
```

#### 3. Extension Points

**Types**:

- `MenuExtensionPoint`: Adds menu items
- `ToolbarExtensionPoint`: Adds toolbar buttons
- `DocumentHandlerExtensionPoint`: Registers document handlers

**Usage**:

```cpp
class MenuExtensionPoint : public IExtensionPoint {
    QString id() const override;
    QString description() const override;
    bool accepts(IPluginInterface* plugin) const override;
    void extend(IPluginInterface* plugin) override;
};
```

#### 4. Plugin Commands (`app/command/PluginCommands.h`)

Command pattern implementation for plugin operations:

**Available Commands**:

- `LoadPluginCommand`: Load a plugin
- `UnloadPluginCommand`: Unload a plugin
- `EnablePluginCommand`: Enable a plugin
- `DisablePluginCommand`: Disable a plugin
- `InstallPluginCommand`: Install a new plugin
- `UninstallPluginCommand`: Remove a plugin
- `ReloadPluginCommand`: Reload a plugin
- `ScanPluginsCommand`: Scan for available plugins

**Factory Pattern**:

```cpp
auto cmd = PluginCommandFactory::createLoadCommand(manager, "MyPlugin");
bool success = cmd->execute();
```

## Integration

### Application Initialization

Plugin system is initialized in `main.cpp`:

```cpp
void initializePluginSystem() {
    PluginManager& manager = PluginManager::instance();

    // Register with ServiceLocator
    ServiceLocator::instance().registerService<PluginManager>(&manager);

    // Set plugin directories
    QStringList dirs;
    dirs << QApplication::applicationDirPath() + "/plugins";
    dirs << QStandardPaths::writableLocation(
                  QStandardPaths::AppDataLocation) + "/plugins";
    manager.setPluginDirectories(dirs);

    // Scan and load plugins
    manager.scanForPlugins();
    manager.loadSettings();
    manager.loadAllPlugins();
}
```

### ServiceLocator Integration

PluginManager is registered with ServiceLocator for global access:

```cpp
// In plugins or application code
auto* manager = ServiceLocator::instance().getService<PluginManager>();
```

### EventBus Integration

Plugins can subscribe to and publish events:

```cpp
// In plugin initialization
eventBus()->subscribe("document.opened", this, [this](Event* event) {
    QString filePath = event->data().toString();
    // Handle document opened
});
```

### UI Integration

### Extension Point Architecture

The plugin system uses an **Extension Point Pattern** to allow plugins to extend the UI without modifying core code. Extension points act as bridges between plugins and UI components.

**Architecture**:

```
Plugin (IUIExtension) → Extension Point (IExtensionPoint) → UI Component
```

**Available Extension Points**:

- `MenuExtensionPoint` - Adds menu items to application menus
- `ToolbarExtensionPoint` - Adds buttons to toolbars
- `DockWidgetExtensionPoint` - Adds dockable widgets (requires QMainWindow)
- `ContextMenuExtensionPoint` - Extends context menus throughout the app
- `StatusBarExtensionPoint` - Displays status bar messages
- `DocumentHandlerExtensionPoint` - Registers document handlers

**Initialization Flow**:

1. Application starts in `main.cpp`
2. `initializePluginSystem()` called
   - Registers `DocumentHandlerExtensionPoint`
   - Scans and loads plugins
3. `MainWindow` created and calls `initPluginUIExtensions()`
   - Registers `MenuExtensionPoint`, `ToolbarExtensionPoint`
   - Registers `ContextMenuExtensionPoint`, `StatusBarExtensionPoint`
   - Note: `DockWidgetExtensionPoint` commented out (requires QMainWindow, ElaWindow doesn't inherit from it)
4. Extension points applied to all loaded plugins via `applyExtensionPoints()`

### UI Extension Interface (IUIExtension)

Plugins implement `IUIExtension` to provide UI elements:

```cpp
class IUIExtension {
    // Menu extensions
    virtual QList<QAction*> menuActions() const { return QList<QAction*>(); }
    virtual QString menuPath() const { return QString(); }

    // Toolbar extensions
    virtual QList<QAction*> toolbarActions() const { return QList<QAction*>(); }
    virtual QString toolbarName() const { return QString(); }

    // Dock widget extensions
    virtual QWidget* createDockWidget(QWidget* parent = nullptr) { return nullptr; }
    virtual QString dockWidgetTitle() const { return QString(); }
    virtual Qt::DockWidgetArea dockWidgetArea() const { return Qt::RightDockWidgetArea; }

    // Context menu extensions
    virtual QList<QAction*> contextMenuActions(const QString& contextId) const;

    // Status bar integration
    virtual QString statusBarMessage() const { return QString(); }
    virtual int statusBarTimeout() const { return 0; }  // 0 = permanent
};
```

### UI Element Lifecycle and Cleanup

PluginManager tracks all UI elements created by plugins:

- **Registration**: UI elements registered via `registerPluginUIElement(pluginName, uiElement)`
- **Automatic Cleanup**: When plugin unloads, `cleanupPluginUIElements(pluginName)` is called
  - Removes widgets from parent
  - Calls `deleteLater()` on tracked objects
  - Clears tracking list

**Key Member Variables**:

- `m_extensionPoints` - List of registered extension points
- `m_pluginUIElements` - Hash map tracking plugin UI objects (`QHash<QString, QList<QObject*>>`)

### Plugin Manager Page in MainWindow

Plugin Manager page in MainWindow:

```cpp
// Create plugin manager page
m_pluginManagerPage = new PluginManagerPage(this);
addPageNode(tr("Plugin Manager"), m_pluginManagerPage, m_toolsKey,
            ElaIconType::Puzzle);
```

## Plugin Management UI

### PluginManagerPage (`app/ui/pages/PluginManagerPage.h`)

**Features**:

- Plugin list view with metadata
- Search and filtering
- Enable/disable plugins
- Install/uninstall plugins
- Plugin details panel
- Real-time status updates

**UI Components**:

- `ElaTableView`: Plugin list
- `QStandardItemModel`: Data model
- Filter controls (search, category)
- Action buttons (enable, install, configure)
- Details panel with metadata display

**Signal Integration**:

```cpp
// Connect to PluginManager signals
connect(m_pluginManager, &PluginManager::pluginLoaded,
        this, &PluginManagerPage::onPluginLoaded);
connect(m_pluginManager, &PluginManager::pluginError,
        this, &PluginManagerPage::onPluginError);
```

## Dependency Management

### Dependency Resolution

The `PluginDependencyResolver` class handles:

- Topological sorting of plugins
- Cyclic dependency detection
- Load order determination

**Algorithm**:

```cpp
QStringList PluginDependencyResolver::resolveDependencies(
    const QHash<QString, PluginMetadata>& plugins) {

    QStringList result;
    QHash<QString, int> visited;  // 0=not visited, 1=visiting, 2=visited

    // Depth-first traversal
    for (auto it = plugins.begin(); it != plugins.end(); ++it) {
        if (visited[it.key()] == 0) {
            visitPlugin(it.key(), plugins, visited, result);
        }
    }

    return result;
}
```

### Cyclic Dependency Detection

```cpp
bool hasCyclicDependencies(const QHash<QString, PluginMetadata>& plugins) {
    // DFS with state tracking
    // state: 0=unvisited, 1=visiting (in stack), 2=visited
    // Cycle detected if we encounter state==1
}
```

## Plugin Metadata

### Structure

```cpp
struct PluginMetadata {
    QString name;
    QString version;
    QString description;
    QString author;
    QString filePath;
    QStringList dependencies;
    QStringList supportedTypes;
    QStringList features;
    QJsonObject configuration;
    bool isLoaded;
    bool isEnabled;
    qint64 loadTime;
};
```

### JSON Format

```json
{
    "name": "Plugin Name",
    "version": "1.0.0",
    "description": "Plugin description",
    "author": "Author Name",
    "dependencies": ["OtherPlugin"],
    "supportedTypes": [".pdf", ".epub"],
    "features": ["feature1", "feature2"],
    "configuration": {
        "option1": "value1",
        "option2": true
    }
}
```

## Settings Persistence

Plugin state is persisted using QSettings:

```cpp
void PluginManager::saveSettings() {
    m_settings->beginGroup("plugins");

    for (auto it = m_pluginMetadata.begin(); it != m_pluginMetadata.end(); ++it) {
        m_settings->setValue(it.key() + "/enabled", it.value().isEnabled);
    }

    m_settings->endGroup();
    m_settings->sync();
}

void PluginManager::loadSettings() {
    m_settings->beginGroup("plugins");

    for (auto it = m_pluginMetadata.begin(); it != m_pluginMetadata.end(); ++it) {
        bool enabled = m_settings->value(it.key() + "/enabled", true).toBool();
        it.value().isEnabled = enabled;
    }

    m_settings->endGroup();
}
```

## Testing

### Unit Tests (`tests/test_plugin_manager.cpp`)

**Test Coverage**:

- Singleton pattern
- Plugin directory management
- Plugin scanning
- Metadata retrieval
- Enable/disable functionality
- Settings persistence
- Dependency resolution
- Cyclic dependency detection

### Command Tests (`tests/test_plugin_commands.cpp`)

**Test Coverage**:

- Command creation
- Command execution
- Error handling
- Signal emission
- Factory pattern
- canExecute() validation

## Example Plugin

Location: `examples/plugins/hello_plugin/`

**Features Demonstrated**:

- Plugin structure
- Metadata declaration
- Event subscription
- Service access
- Configuration usage
- Logging

**Implementation**:

```cpp
class HelloPlugin : public PluginBase {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE "hello_plugin.json")
    Q_INTERFACES(IPluginInterface)

protected:
    bool onInitialize() override {
        // Subscribe to events
        eventBus()->subscribe("document.opened", this, [this](Event* event) {
            QString filePath = event->data().toString();
            m_logger.info("Document opened: {}", filePath.toStdString());
        });
        return true;
    }

    void onShutdown() override {
        eventBus()->unsubscribeAll(this);
    }
};
```

## Design Patterns

### 1. Singleton Pattern

- `PluginManager::instance()`
- Ensures single instance of plugin manager

### 2. Factory Pattern

- `PluginCommandFactory`
- Creates plugin commands

### 3. Command Pattern

- `PluginCommand` hierarchy
- Encapsulates plugin operations

### 4. Observer Pattern

- EventBus integration
- Plugin event subscriptions

### 5. Service Locator Pattern

- ServiceLocator integration
- Plugin service access

## Performance Considerations

### Plugin Loading

- **Lazy Loading**: Plugins are only loaded when enabled
- **Parallel Scanning**: Directory scanning could be parallelized
- **Caching**: Metadata is cached to avoid repeated file reads

### Memory Management

- **Smart Pointers**: Used for plugin ownership
- **QPointer**: Used for service references
- **RAII**: Resources cleaned up in destructors

### Hot Reloading

Optional hot reloading support:

```cpp
manager.enableHotReloading(true);
// Monitors plugin files for changes
// Automatically reloads modified plugins
```

## Security

### Plugin Validation

```cpp
bool PluginManager::validatePlugin(const QString& filePath) {
    QPluginLoader loader(filePath);
    QJsonObject metaData = loader.metaData();

    if (metaData.isEmpty()) {
        return false;
    }

    QJsonObject pluginMetaData = metaData.value("MetaData").toObject();
    return !pluginMetaData.value("name").toString().isEmpty();
}
```

### Isolation

- Plugins run in separate libraries
- Error handling prevents plugin crashes from affecting main app
- Resource cleanup ensures no memory leaks

## UI Extension Management API

### PluginManager Extension Point Methods

The PluginManager class provides these methods for managing UI extension points:

**Extension Point Registration**:

```cpp
// Register a new extension point
void registerExtensionPoint(IExtensionPoint* extensionPoint);

// Unregister an extension point by ID
void unregisterExtensionPoint(const QString& extensionId);

// Get all registered extension points
QList<IExtensionPoint*> getExtensionPoints() const;

// Apply extension points to a plugin
void applyExtensionPoints(IPluginInterface* plugin);
```

**UI Element Tracking**:

```cpp
// Register a UI element created by a plugin (for cleanup tracking)
void registerPluginUIElement(const QString& pluginName, QObject* uiElement);

// Clean up all UI elements when plugin unloads
void cleanupPluginUIElements(const QString& pluginName);
```

### Extension Point Implementation

Each extension point implements `IExtensionPoint`:

```cpp
class IExtensionPoint {
public:
    virtual ~IExtensionPoint() = default;

    virtual QString id() const = 0;
    virtual QString description() const = 0;
    virtual bool accepts(IPluginInterface* plugin) const = 0;
    virtual void extend(IPluginInterface* plugin) = 0;
};
```

**Key Methods**:

- `id()` - Unique identifier for the extension point (e.g., `org.sast.readium.menu`)
- `description()` - Human-readable description
- `accepts()` - Determines if a plugin should be extended (based on capabilities)
- `extend()` - Performs the actual extension (adds menus, toolbar buttons, etc.)

### Capability Declaration

Plugins declare UI capabilities in their metadata to be accepted by extension points. For example:

```json
{
    "name": "My UI Plugin",
    "version": "1.0.0",
    "features": ["menu", "toolbar", "status_bar"]
}
```

Extension points check plugin capabilities to determine acceptance:

```cpp
// Example: MenuExtensionPoint checks for "menu" or "ui.menu" capability
bool MenuExtensionPoint::accepts(IPluginInterface* plugin) const {
    QStringList provides = plugin->provides();
    return provides.contains("menu") || provides.contains("ui.menu");
}
```

### Code Locations

- Extension point definitions: `app/plugin/PluginInterface.h` (lines 236-341)
- PluginManager UI methods: `app/plugin/PluginManager.h` (lines 212-220)
- MainWindow initialization: `app/MainWindow.cpp` (lines 605-647)
- Application initialization: `app/main.cpp` (lines 258-307)

## Hook Registry System

### Purpose

The **PluginHookRegistry** provides centralized callback management for workflow events. Plugins register callbacks at predefined hook points to intercept document processing, rendering, search, caching, and annotation operations.

### Hook Points

**20+ standard hook points** across five workflow domains:

- **Document**: `pre_load`, `post_load`, `pre_close`, `post_close`, `metadata_extracted`
- **Render**: `pre_page`, `post_page`, `apply_filter`, `overlay`
- **Search**: `pre_execute`, `post_execute`, `index_build`, `results_rank`
- **Cache**: `pre_add`, `post_add`, `pre_evict`, `post_evict`, `optimize`
- **Annotation**: `created`, `updated`, `deleted`, `render`
- **Export**: `pre_execute`, `post_execute`

### Initialization

Hooks are registered during plugin system initialization:

```cpp
// In PluginManager::registerStandardHooks() called from main.cpp line 286
PluginHookRegistry::instance().registerHook(StandardHooks::DOCUMENT_PRE_LOAD);
PluginHookRegistry::instance().registerHook(StandardHooks::RENDER_POST_PAGE);
// ... register all standard hooks ...
```

### Plugin Callback Registration

Plugins register callbacks during initialization:

```cpp
// In IDocumentProcessorPlugin::onInitialize()
auto callback = [this](const QVariantMap& context) -> QVariant {
    QString filePath = context["filePath"].toString();
    return processResult;
};

PluginHookRegistry::instance().registerCallback(
    StandardHooks::DOCUMENT_POST_LOAD,
    pluginName(),
    callback);
```

### Cleanup

When plugins unload, all registered callbacks are removed:

```cpp
// In PluginManager::unloadPluginInternal()
unregisterAllHooks(pluginName);  // Calls PluginHookRegistry::unregisterAllCallbacks()
```

### Code Location

- Registry definition: `app/plugin/PluginHookRegistry.h`
- Standard hook names: `app/plugin/PluginHookRegistry.h` (lines 165-201)
- Hook execution: `PluginHookPoint::execute()` in `SpecializedPlugins.h` (lines 564-573)
- Initialization: `app/main.cpp` (line 286)

## Future Enhancements

1. **Plugin Marketplace**: Online plugin repository
2. **Digital Signatures**: Verify plugin authenticity
3. **Sandboxing**: Restrict plugin capabilities
4. **Versioning**: Better version compatibility checking
5. **Hot Reload**: Improve hot reload stability
6. **Plugin Dependencies**: Better dependency version management
7. **Plugin API Documentation**: Auto-generated API docs
8. **Hook Composition**: Chain multiple plugins at single hook point
9. **Async Hooks**: Support asynchronous plugin callbacks
10. **Hook Priorities**: Execute hooks in specified order

## Enhanced UI and Data Components

The plugin system has been extended with MVC-compliant components for rich plugin management UI and configuration handling.

### PluginModel (`app/model/PluginModel.h`)

**Purpose**: Qt Model/View architecture integration for plugin management with automatic synchronization with PluginManager.

**Features**:

- Custom data roles (Name, Version, Description, Author, Dependencies, Status, etc.)
- Real-time updates via EventBus integration
- Search and filtering capabilities (by text, load state, enable state)
- Plugin type detection (Document, UI, Specialized)
- Thread-safe metadata caching

**Data Roles**:

```cpp
enum PluginDataRole {
    NameRole, VersionRole, DescriptionRole, AuthorRole,
    FilePathRole, DependenciesRole, SupportedTypesRole,
    FeaturesRole, IsLoadedRole, IsEnabledRole,
    LoadTimeRole, ErrorsRole, ConfigurationRole,
    PluginTypeRole, StatusTextRole, IconRole
};
```

**Core API**:

```cpp
// Plugin operations via model
bool loadPlugin(int row);
bool unloadPlugin(int row);
bool enablePlugin(int row);
bool disablePlugin(int row);

// Filtering
void setFilterText(const QString& filter);
void setShowOnlyLoaded(bool onlyLoaded);
void setShowOnlyEnabled(bool onlyEnabled);
void clearFilters();

// Model management
void refresh();
void rescanPlugins();
```

### PluginConfigModel (`app/model/PluginConfigModel.h`)

**Purpose**: Table-based configuration management for plugins with type-aware value editing and validation.

**Features**:

- JSON configuration support with automatic type detection
- Type-aware editing (bool, int, double, string, object, array)
- Configuration validation and read-only protection
- Undo/redo support via ConfigurePluginCommand
- Real-time configuration updates with signal notifications

**Configuration Entry Structure**:

```cpp
struct ConfigEntry {
    QString key;
    QVariant value;
    QString type;           // Type information for editing
    QString description;    // Optional user-facing description
    bool isReadOnly;        // Prevent editing
};
```

**Core API**:

```cpp
// Plugin configuration management
void setPluginName(const QString& pluginName);
void loadConfiguration();
bool saveConfiguration();
void resetToDefaults();

// Configuration access
QJsonObject getConfiguration() const;
void setConfiguration(const QJsonObject& config);

// Entry management
bool setValue(const QString& key, const QVariant& value);
QVariant getValue(const QString& key) const;
bool isValidValue(const QString& type, const QVariant& value) const;
```

### PluginListDelegate (`app/delegate/PluginListDelegate.h`)

**Purpose**: Custom item delegate for rich visual rendering of plugin items in lists and tables.

**Display Modes**:

- **CompactMode**: Single-line with minimal information
- **NormalMode**: Two-line display with name, version, and status
- **DetailedMode**: Multi-line with full plugin details

**Visual Features**:

- Plugin icons with fallback support
- Load/enable status indicators (colored circles)
- Error highlighting with custom colors
- Hover effects for interactivity
- Automatic text color adjustment based on state

**Customization**:

```cpp
// Display customization
void setDisplayMode(DisplayMode mode);
void setShowIcons(bool show);
void setShowStatus(bool show);
void setHighlightErrors(bool highlight);

// Color customization
void setLoadedColor(const QColor& color);
void setDisabledColor(const QColor& color);
void setErrorColor(const QColor& color);
```

**Sizing Constants**:

- Icon size: 32px (16px in compact mode)
- Status indicator: 8px
- Standard margins and spacing: 4-6px

### ConfigurePluginCommand (`app/command/PluginCommands.h`)

**Purpose**: Plugin configuration command with full undo/redo support through CommandManager.

**Features**:

- State preservation for undo operations
- Configuration validation before execution
- Error handling with descriptive messages
- Signal-based progress reporting
- Factory pattern integration

**Implementation**:

```cpp
class ConfigurePluginCommand : public PluginCommand {
    bool execute() override;      // Apply new configuration
    bool undo() override;         // Restore previous configuration
    bool canExecute() const override;

    QJsonObject oldConfiguration() const;
    QJsonObject newConfiguration() const;
};
```

**Usage**:

```cpp
auto cmd = PluginCommandFactory::createConfigureCommand(
    manager, "PluginName", newConfig);
bool success = cmd->execute();
if (success) {
    commandManager->executeCommand(cmd);  // For undo/redo
}
```

### Internationalization Support

Full translation support for plugin UI components via Qt Linguist framework.

**Translation Files**:

- `app/i18n/plugin_system_en.ts` - English translations
- `app/i18n/plugin_system_zh.ts` - Chinese translations

**Supported Strings**:

- Plugin status messages (loaded, unloaded, enabled, disabled, error)
- Configuration UI labels and descriptions
- Delegate display text
- Error messages and warnings
- Dialog and button labels

**Integration with I18nManager**:

```cpp
// I18nManager automatically loads plugin translations
i18nManager->setLanguage("zh");  // Plugin strings update immediately
```

### Integration Points

**EventBus Events**:

- `plugin.loaded` - Plugin successfully loaded
- `plugin.unloaded` - Plugin unloaded
- `plugin.enabled` - Plugin enabled
- `plugin.disabled` - Plugin disabled
- `plugin.error` - Plugin error occurred
- `plugin.config_changed` - Configuration changed

**ServiceLocator Registration**:

```cpp
// Register models in initialization
ServiceLocator::instance().registerService<PluginModel>(
    [manager] { return new PluginModel(manager); });
ServiceLocator::instance().registerService<PluginConfigModel>(
    [manager] { return new PluginConfigModel(manager); });
```

**CommandManager Integration**:

```cpp
// ConfigurePluginCommand integrates with undo/redo
auto cmd = PluginCommandFactory::createConfigureCommand(
    manager, pluginName, newConfig);
CommandManager::instance().executeCommand(cmd);
```

## References

**Core Plugin System**:

- Plugin Manager: `app/plugin/PluginManager.h`
- Plugin Interface: `app/plugin/PluginInterface.h`
- Plugin Commands: `app/command/PluginCommands.h`
- Plugin UI: `app/ui/pages/PluginManagerPage.h`

**Enhanced Components**:

- Plugin Model: `app/model/PluginModel.h` and `.cpp`
- Plugin Config Model: `app/model/PluginConfigModel.h` and `.cpp`
- Plugin List Delegate: `app/delegate/PluginListDelegate.h` and `.cpp`
- Internationalization: `app/i18n/plugin_system_en.ts`, `plugin_system_zh.ts`

**Specialized Plugin Interfaces**:

- Specialized Plugins: `app/plugin/SpecializedPlugins.h`
- Hook Registry: `app/plugin/PluginHookRegistry.h` and `.cpp`
- Example Plugin: `examples/plugins/metadata_extractor/`
- Documentation: [Specialized Plugin Interfaces](specialized-plugin-interfaces.md)

**Additional Resources**:

- Example Plugin: `examples/plugins/hello_plugin/`
- Development Guide: `docs/plugin-development.md`
- Tests: `tests/test_plugin_manager.cpp`, `tests/test_plugin_commands.cpp`
