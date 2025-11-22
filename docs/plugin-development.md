# Plugin Development Guide for SAST Readium

This guide explains how to develop plugins for the SAST Readium application.

## Table of Contents

1. [Introduction](#introduction)
2. [Plugin System Architecture](#plugin-system-architecture)
3. [Creating a Plugin](#creating-a-plugin)
4. [Plugin Lifecycle](#plugin-lifecycle)
5. [Plugin API](#plugin-api)
6. [Configuration](#configuration)
7. [Event System](#event-system)
8. [Service Access](#service-access)
9. [Testing Plugins](#testing-plugins)
10. [Best Practices](#best-practices)
11. [Troubleshooting](#troubleshooting)

## Introduction

SAST Readium provides a flexible plugin system that allows developers to extend the application's functionality without modifying the core codebase. Plugins can:

- Subscribe to application events
- Access core services (DocumentController, SearchEngine, etc.)
- Integrate with the UI through extension points
- Provide new features and functionality
- Process different document formats

## Plugin System Architecture

The plugin system consists of several key components:

### Core Components

1. **PluginManager**: Manages plugin lifecycle, loading, unloading, and dependency resolution
2. **PluginInterface/PluginBase**: Base interface and implementation for all plugins
3. **PluginContext**: Provides access to application services
4. **Extension Points**: Allow plugins to extend specific parts of the application
5. **PluginCommands**: Command pattern implementation for plugin operations

### Plugin Types

SAST Readium supports three main plugin types:

1. **IPlugin**: Basic plugin interface
2. **IDocumentPlugin**: Plugins that process documents
3. **IUIPlugin**: Plugins that provide UI components

## Creating a Plugin

### Step 1: Set Up Project Structure

Create a new directory for your plugin with the following structure:

```
my_plugin/
├── MyPlugin.h              # Plugin header
├── MyPlugin.cpp            # Plugin implementation
├── my_plugin.json          # Plugin metadata
├── CMakeLists.txt          # Build configuration
└── README.md               # Plugin documentation
```

### Step 2: Create Plugin Header

```cpp
#pragma once

#include "plugin/PluginInterface.h"

class MyPlugin : public PluginBase {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE "my_plugin.json")
    Q_INTERFACES(IPluginInterface)

public:
    explicit MyPlugin(QObject* parent = nullptr);
    ~MyPlugin() override;

protected:
    bool onInitialize() override;
    void onShutdown() override;

private:
    // Your plugin implementation
};
```

### Step 3: Implement Plugin Methods

```cpp
#include "MyPlugin.h"

MyPlugin::MyPlugin(QObject* parent) : PluginBase(parent) {
    // Set metadata
    m_metadata.name = "My Plugin";
    m_metadata.version = "1.0.0";
    m_metadata.description = "My awesome plugin";
    m_metadata.author = "Your Name";
    m_metadata.dependencies = QStringList(); // List dependencies

    // Set capabilities
    m_capabilities.provides = QStringList() << "my-feature";
    m_capabilities.requiredPlugins = QStringList(); // Required plugins
}

MyPlugin::~MyPlugin() {
    // Cleanup
}

bool MyPlugin::onInitialize() {
    m_logger.info("MyPlugin initializing...");

    // Subscribe to events
    eventBus()->subscribe("event.type", this, [this](Event* event) {
        // Handle event
    });

    // Access services
    auto* documentController = serviceLocator()->getService<DocumentController>();

    m_logger.info("MyPlugin initialized");
    return true;
}

void MyPlugin::onShutdown() {
    m_logger.info("MyPlugin shutting down...");

    // Unsubscribe from events
    eventBus()->unsubscribeAll(this);

    m_logger.info("MyPlugin shutdown complete");
}
```

### Step 4: Create Plugin Metadata

Create `my_plugin.json`:

```json
{
    "name": "My Plugin",
    "version": "1.0.0",
    "description": "My awesome plugin for SAST Readium",
    "author": "Your Name",
    "dependencies": [],
    "supportedTypes": [],
    "features": ["my-feature"],
    "configuration": {
        "option1": "value1",
        "option2": true
    }
}
```

### Step 5: Create CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.28)
project(MyPlugin)

find_package(Qt6 REQUIRED COMPONENTS Core)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)

add_library(my_plugin SHARED
    MyPlugin.cpp
    MyPlugin.h
)

target_link_libraries(my_plugin PRIVATE Qt6::Core)

set_target_properties(my_plugin PROPERTIES
    OUTPUT_NAME "my_plugin"
    PREFIX ""
)

install(TARGETS my_plugin
    LIBRARY DESTINATION plugins
    RUNTIME DESTINATION plugins
)

install(FILES my_plugin.json DESTINATION plugins)
```

## Plugin Lifecycle

Plugins go through the following lifecycle stages:

### 1. Discovery

The PluginManager scans plugin directories for valid plugin files (.dll, .so, .dylib) and extracts metadata.

### 2. Registration

Plugin metadata is registered in the PluginManager, making it available for loading.

### 3. Dependency Resolution

The PluginManager resolves plugin dependencies and determines the load order.

### 4. Loading

The plugin library is loaded into memory using QPluginLoader.

### 5. Initialization

The `initialize()` method is called, which internally calls your `onInitialize()` implementation.

### 6. Running

The plugin is active and can respond to events, provide services, etc.

### 7. Shutdown

The `shutdown()` method is called, which calls your `onShutdown()` implementation.

### 8. Unloading

The plugin library is unloaded from memory.

## Plugin API

### PluginBase Helper Methods

```cpp
// Access core services
ServiceLocator* serviceLocator();
EventBus* eventBus();
CommandManager* commandManager();
ConfigurationManager* configurationManager();
```

### Metadata Properties

```cpp
m_metadata.name;          // Plugin name
m_metadata.version;       // Version string
m_metadata.description;   // Description
m_metadata.author;        // Author name
m_metadata.dependencies;  // List of dependency names
```

### Capabilities

```cpp
m_capabilities.provides;         // Features provided
m_capabilities.requiredPlugins;  // Required plugin names
```

### Configuration

```cpp
// In onInitialize()
QString value = m_configuration["key"].toString();
int number = m_configuration["number"].toInt();
bool flag = m_configuration["flag"].toBool();
```

## Configuration

### Plugin Configuration

Each plugin can have its own configuration in the JSON metadata file:

```json
{
    "configuration": {
        "maxItems": 100,
        "enableFeature": true,
        "customPath": "/path/to/resource"
    }
}
```

Access in plugin code:

```cpp
bool MyPlugin::onInitialize() {
    int maxItems = m_configuration["maxItems"].toInt(100);  // Default 100
    bool enable = m_configuration["enableFeature"].toBool(false);
    QString path = m_configuration["customPath"].toString();

    // Use configuration values
    return true;
}
```

### Runtime Configuration

Plugins can be configured at runtime through the Plugin Manager UI or programmatically:

```cpp
PluginManager& manager = PluginManager::instance();

QJsonObject config;
config["option"] = "value";
manager.setPluginConfiguration("MyPlugin", config);
```

## Event System

### Subscribing to Events

```cpp
// Lambda handler
eventBus()->subscribe("document.opened", this, [this](Event* event) {
    QString filePath = event->data().toString();
    // Handle event
});

// Member function handler
eventBus()->subscribe("document.closed", this, SLOT(onDocumentClosed()));
```

### Common Application Events

- `document.opened`: Document was opened
- `document.closed`: Document was closed
- `document.saved`: Document was saved
- `document.modified`: Document was modified
- `navigation.page_changed`: Page changed
- `navigation.zoom_changed`: Zoom level changed
- `ui.theme_changed`: Theme changed
- `system.application_ready`: Application fully initialized

### Publishing Events

```cpp
// Publish simple event
eventBus()->publish("my.custom.event", QVariant("data"));

// Publish event object
Event* event = new Event("my.event");
event->setData(QVariant::fromValue(myData));
eventBus()->publish(event);
```

### Unsubscribing

```cpp
// Unsubscribe from specific event
eventBus()->unsubscribe("event.type", this);

// Unsubscribe from all events
eventBus()->unsubscribeAll(this);
```

## Service Access

### Available Services

```cpp
// Document management
auto* docController = serviceLocator()->getService<DocumentController>();

// Search functionality
auto* searchEngine = serviceLocator()->getService<SearchEngine>();

// Plugin management
auto* pluginManager = serviceLocator()->getService<PluginManager>();

// UI services
auto* styleManager = serviceLocator()->getService<StyleManager>();
auto* i18nManager = serviceLocator()->getService<I18nManager>();
```

### Service Usage Example

```cpp
bool MyPlugin::onInitialize() {
    // Get document controller
    auto* controller = serviceLocator()->getService<DocumentController>();
    if (!controller) {
        m_logger.error("DocumentController not available");
        return false;
    }

    // Use the service
    connect(controller, &DocumentController::documentOpened,
            this, &MyPlugin::onDocumentOpened);

    return true;
}
```

## Testing Plugins

### Unit Testing

Create a test class for your plugin:

```cpp
class MyPluginTest : public QObject {
    Q_OBJECT

private slots:
    void testInitialization();
    void testFeature();
    void testConfiguration();
};

void MyPluginTest::testInitialization() {
    MyPlugin plugin;
    QVERIFY(plugin.initialize());
    QCOMPARE(plugin.name(), QString("My Plugin"));
    plugin.shutdown();
}
```

### Integration Testing

Test plugin integration with the application:

```cpp
void MyPluginTest::testIntegration() {
    PluginManager& manager = PluginManager::instance();

    // Install plugin
    QVERIFY(manager.installPlugin("path/to/my_plugin.dll"));

    // Load plugin
    QVERIFY(manager.loadPlugin("My Plugin"));

    // Test functionality
    auto* plugin = manager.getPlugin("My Plugin");
    QVERIFY(plugin != nullptr);
    QVERIFY(plugin->isInitialized());

    // Cleanup
    manager.unloadPlugin("My Plugin");
}
```

## Best Practices

### 1. Resource Management

- Always clean up resources in `onShutdown()`
- Unsubscribe from all events before shutdown
- Use RAII for resource management
- Avoid memory leaks by properly managing object ownership

### 2. Error Handling

```cpp
bool MyPlugin::onInitialize() {
    try {
        // Initialization code
        return true;
    } catch (const std::exception& e) {
        m_logger.error("Initialization failed: {}", e.what());
        return false;
    }
}
```

### 3. Logging

- Use the plugin logger (`m_logger`) for all logging
- Log initialization and shutdown
- Log errors with context
- Use appropriate log levels (debug, info, warning, error)

### 4. Dependencies

- Minimize plugin dependencies
- Declare all dependencies in metadata
- Check for required services in `onInitialize()`
- Gracefully handle missing dependencies

### 5. Thread Safety

- Use Qt's signal/slot mechanism for cross-thread communication
- Protect shared data with mutexes
- Be aware of event bus thread affinity

### 6. Performance

- Avoid blocking operations in event handlers
- Use background threads for heavy processing
- Cache frequently accessed data
- Unsubscribe from unused events

## Troubleshooting

### Plugin Doesn't Load

**Problem**: Plugin doesn't appear in Plugin Manager

**Solutions**:

- Verify plugin is in correct directory
- Check that JSON metadata file exists
- Ensure plugin was compiled with same Qt version
- Review application logs for errors

### Initialization Fails

**Problem**: Plugin fails to initialize

**Solutions**:

- Check all dependencies are available
- Verify services are accessible
- Review error messages in logs
- Ensure configuration is valid

### Events Not Received

**Problem**: Plugin doesn't receive events

**Solutions**:

- Verify event names match exactly
- Check that subscription was successful
- Ensure plugin is initialized
- Verify event is actually being published

### Crashes on Unload

**Problem**: Application crashes when unloading plugin

**Solutions**:

- Ensure all event subscriptions are removed
- Clean up all resources in `onShutdown()`
- Disconnect all signal/slot connections
- Check for dangling pointers

## Additional Resources

- Example Plugin: `examples/plugins/hello_plugin/`
- Plugin Manager Source: `app/plugin/PluginManager.h`
- Plugin Interface: `app/plugin/PluginInterface.h`
- Plugin Commands: `app/command/PluginCommands.h`
- API Documentation: See code comments and Doxygen documentation

## Support

For questions or issues with plugin development:

- Check existing plugins for examples
- Review the source code documentation
- Contact the development team
- Submit issues on the project repository
