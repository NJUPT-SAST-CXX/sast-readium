# Hello Plugin - Example Plugin for SAST Readium

This is a comprehensive example plugin that demonstrates how to create feature-rich plugins for SAST Readium.

## Features

This example plugin demonstrates:

- **Plugin Lifecycle Management**: Proper initialization and shutdown
- **Event Subscription**: Subscribing to application events (document open/close/page view)
- **Service Access**: Accessing application services via ServiceLocator
- **Configuration Management**: Loading and using plugin configuration
- **Metadata Declaration**: Declaring plugin metadata in JSON format
- **Logging**: Using the plugin logging system
- **UI Extensions (IUIExtension)**:
  - Menu items under "Tools → Hello Plugin"
  - Toolbar button with statistics shortcut
  - Context menu actions for documents
  - Status bar integration with live statistics
- **Hook Registration**: Using PluginHookRegistry for document workflow hooks
- **Inter-plugin Communication**: Handling messages from other plugins

## Building the Plugin

### Prerequisites

- CMake 3.28 or higher
- Qt6 Core
- SAST Readium development headers

### Build Instructions

```bash
# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build .

# Install (optional)
cmake --install . --prefix /path/to/readium/plugins
```

## Installing the Plugin

1. Build the plugin following the instructions above
2. Copy the compiled plugin file (`hello_plugin.dll`, `hello_plugin.so`, or `hello_plugin.dylib`) to the SAST Readium plugins directory:
   - Windows: `%APPDATA%/SAST/Readium/plugins/`
   - Linux: `~/.local/share/SAST/Readium/plugins/`
   - macOS: `~/Library/Application Support/SAST/Readium/plugins/`
3. Copy `hello_plugin.json` to the same directory
4. Launch SAST Readium
5. Open the Plugin Manager (Tools → Plugin Manager)
6. The Hello Plugin should appear in the list
7. Enable the plugin if it's not already enabled

## Plugin Structure

```
hello_plugin/
├── HelloPlugin.h           # Plugin header file
├── HelloPlugin.cpp         # Plugin implementation
├── hello_plugin.json       # Plugin metadata
├── CMakeLists.txt          # Build configuration
└── README.md              # This file
```

## Plugin Metadata

The `hello_plugin.json` file contains:

```json
{
    "name": "Hello Plugin",
    "version": "1.0.0",
    "description": "Example plugin demonstrating SAST Readium plugin system",
    "author": "SAST Readium Team",
    "dependencies": [],
    "features": ["example", "demo", "event-handling", "configuration"],
    "configuration": {
        "greeting": "Hello from SAST Readium Plugin System!",
        "enableLogging": true,
        "maxDocuments": 100
    }
}
```

## Plugin Functionality

### Initialization

When the plugin is loaded, it:

1. Creates UI actions (menu items, toolbar buttons, context menu actions)
2. Sets up metadata (name, version, description, author)
3. Declares capabilities (provided features, dependencies)
4. Subscribes to application events
5. Registers hook callbacks with PluginHookRegistry
6. Loads configuration and applies UI visibility settings
7. Logs initialization status

### Event Handling

The plugin subscribes to:

- `document.opened`: Triggered when a document is opened
- `document.closed`: Triggered when a document is closed
- `page.viewed`: Triggered when a page is viewed (for statistics)

### Hook Registration

The plugin registers callbacks for:

- `document.pre_load`: Called before document loading (can approve/reject)
- `document.post_load`: Called after document is loaded

### UI Extensions

The plugin provides:

- **Menu Items** (Tools → Hello Plugin):
  - Show Statistics: Display document and page statistics
  - Reset Counters: Reset all plugin counters
  - About Hello Plugin: Show plugin information
- **Toolbar Button**: Quick access to statistics (📊 icon)
- **Context Menu**: Copy Document Path action
- **Status Bar**: Live display of documents opened and pages viewed

### Inter-plugin Communication

The plugin handles messages from other plugins:

- `get_stats`: Returns current statistics
- `reset`: Resets all counters

### Configuration

The plugin supports configuration through the `hello_plugin.json` file:

- `greeting`: Custom greeting message
- `enableLogging`: Enable/disable logging
- `enableMenu`: Show/hide menu items
- `enableToolbar`: Show/hide toolbar button
- `enableContextMenu`: Show/hide context menu actions
- `enableStatusBar`: Show/hide status bar messages
- `maxDocuments`: Maximum number of documents to track

### Shutdown

When the plugin is unloaded, it:

1. Unregisters all hook callbacks
2. Unsubscribes from all events
3. Destroys UI actions
4. Logs shutdown statistics
5. Cleans up resources

## Creating Your Own Plugin

Use this plugin as a template for creating your own plugins:

1. **Copy the plugin directory**: Start with this example as a template
2. **Rename the files**: Replace `HelloPlugin` with your plugin name
3. **Update metadata**: Edit `hello_plugin.json` with your plugin information
4. **Implement functionality**: Override `onInitialize()` and `onShutdown()`
5. **Add features**: Implement your plugin's specific features
6. **Build and test**: Compile your plugin and test it in SAST Readium

## API Reference

### PluginBase Class

Inherit from `PluginBase` to create a plugin:

```cpp
class MyPlugin : public PluginBase {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE "my_plugin.json")
    Q_INTERFACES(IPluginInterface)

protected:
    bool onInitialize() override;
    void onShutdown() override;
};
```

### Available Services

Access services via `PluginBase` helper methods:

```cpp
// Get ServiceLocator
ServiceLocator* locator = serviceLocator();

// Get EventBus
EventBus* bus = eventBus();

// Get CommandManager
CommandManager* cmdManager = commandManager();

// Get ConfigurationManager
ConfigurationManager* configManager = configurationManager();
```

### Event Subscription

Subscribe to events using EventBus:

```cpp
eventBus()->subscribe("event.type", this, [this](Event* event) {
    // Handle event
    QVariant data = event->data();
});
```

### Logging

Use the plugin logger:

```cpp
m_logger.info("Information message");
m_logger.warning("Warning message");
m_logger.error("Error message");
m_logger.debug("Debug message");
```

## Troubleshooting

### Plugin doesn't appear in Plugin Manager

- Check that the plugin file is in the correct directory
- Verify that the `hello_plugin.json` file is present
- Check the application logs for plugin loading errors

### Plugin fails to load

- Ensure all dependencies are met
- Check that the plugin was compiled with the same Qt version as SAST Readium
- Review the error messages in the Plugin Manager

### Events not being received

- Verify that the plugin is initialized
- Check that event subscriptions are set up correctly
- Ensure the event names match the application's event types

## License

This example plugin is part of the SAST Readium project and follows the same license.

## Support

For questions or issues:

- Check the SAST Readium documentation
- Visit the project repository
- Contact the SAST Readium development team
