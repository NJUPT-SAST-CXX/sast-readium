# SAST Readium Architecture

This document provides a comprehensive overview of the SAST Readium architecture, design patterns, and component organization.

## Table of Contents

- [Overview](#overview)
- [Architecture Patterns](#architecture-patterns)
- [Core Components](#core-components)
- [Key Subsystems](#key-subsystems)
- [Data Flow](#data-flow)
- [Extension Points](#extension-points)

## Overview

SAST Readium follows a modular Qt6 architecture with clear separation of concerns. The application is built using modern C++20 features and implements several well-established design patterns to ensure maintainability, testability, and extensibility.

### Design Principles

1. **Separation of Concerns**: Clear boundaries between UI, business logic, and data layers
2. **Dependency Injection**: Loose coupling through Service Locator pattern
3. **Command Pattern**: All user actions are encapsulated as commands for undo/redo support
4. **Event-Driven**: Decoupled communication through Event Bus
5. **Factory Pattern**: Standardized object creation and configuration
6. **Plugin Architecture**: Extensibility through well-defined interfaces

## Architecture Patterns

### MVC (Model-View-Controller)

The application follows the MVC pattern with Qt's Model/View framework:

- **Models** (`app/model/`): Data and business logic
  - `DocumentModel` - PDF document state
  - `RenderModel` - Rendering coordination
  - `PageModel` - Page-level operations
  - `ThumbnailModel` - Thumbnail data and caching
  - `SearchModel` - Search state and results
  - `BookmarkModel` - Bookmark management
  - `AnnotationModel` - Annotation data

- **Views** (`app/ui/`): User interface components
  - `ViewWidget` - Main PDF viewing area
  - `ThumbnailListView` - Thumbnail sidebar
  - `SearchWidget` - Search interface
  - `MenuBar`, `ToolBar`, `StatusBar` - UI chrome

- **Controllers** (`app/controller/`): Application logic coordination
  - `ApplicationController` - Application-level coordination
  - `DocumentController` - Document lifecycle
  - `PageController` - Page navigation
  - `ConfigurationManager` - Settings management

### Command Pattern

All user actions are implemented as commands for undo/redo support:

```cpp
// Command execution
CommandManager& cmdMgr = GlobalCommandManager::instance();
cmdMgr.executeCommand("open_document");

// Undo/Redo
cmdMgr.undo();
cmdMgr.redo();
```

**Key Components:**

- **CommandManager** (`app/command/CommandManager.h`): Command execution and history
  - Centralized command execution
  - Undo/redo stack management
  - Keyboard shortcut registration
  - Command lifecycle management

- **Command Types**:
  - `DocumentCommands` - Document operations (open, close, save)
  - `NavigationCommands` - Page navigation (next, previous, goto)
  - `InitializationCommand` - Application startup sequence

**Benefits:**

- Full undo/redo support
- Macro recording capability
- Command history and replay
- Testable user actions

### Service Locator Pattern

Dependency injection and service management:

```cpp
// Register a service
ServiceLocator::instance().registerService<DocumentController>(
    []() { return new DocumentController(); }
);

// Retrieve a service
auto* docController = ServiceLocator::instance().getService<DocumentController>();
```

**Key Features:**

- Lazy service initialization
- Automatic lifecycle management
- Type-safe service retrieval
- Signal-based lifecycle notifications

**Benefits:**

- Loose coupling between components
- Easy testing with mock services
- Centralized dependency management
- Runtime service replacement

### Event Bus Pattern

Decoupled event-driven communication:

```cpp
// Subscribe to events
EventBus::instance().subscribe("document_opened", this, 
    [](Event* event) {
        QString filename = event->data().toString();
        // Handle event
    }
);

// Publish events
EventBus::instance().publish("document_opened", filename);
```

**Key Features:**

- Synchronous and asynchronous delivery
- Event filtering and priority
- Thread-safe event processing
- Event history and replay

**Benefits:**

- Decoupled component communication
- Easy to add new event handlers
- Testable event flows
- Flexible event routing

### Factory Pattern

Standardized object creation:

```cpp
// Create models
ModelFactory factory;
auto* renderModel = factory.createRenderModel(dpiX, dpiY);
auto* documentModel = factory.createDocumentModel(renderModel);

// Create widgets
WidgetFactory widgetFactory(pageController);
auto* nextButton = widgetFactory.createButton(actionID::next, "Next");
```

**Key Components:**

- **ModelFactory** (`app/factory/ModelFactory.h`): Creates and configures models
  - Automatic dependency injection
  - Consistent initialization
  - Signal connection setup
  - Model set creation for common scenarios

- **WidgetFactory** (`app/factory/WidgetFactory.h`): Creates UI widgets
  - Automatic signal/slot connections
  - Consistent styling
  - Command integration

**Benefits:**

- Consistent object creation
- Centralized configuration
- Easy to modify creation logic
- Testable with mock factories

### Plugin System

Extensibility through plugins:

```cpp
// Plugin interface
class PluginInterface {
public:
    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
};

// Plugin manager
PluginManager::instance().loadPlugin("path/to/plugin.so");
```

**Key Features:**

- Dynamic plugin loading
- Plugin lifecycle management
- Dependency resolution
- Plugin metadata and versioning

## Core Components

### Application Entry

- **main.cpp**: Application entry point
  - Qt application initialization
  - Logging system setup
  - Command-line argument processing
  - Main window creation

- **MainWindow**: Main application window
  - UI layout management
  - Component coordination
  - Event handling
  - Menu and toolbar setup

### Delegates

Delegates handle specialized rendering and interaction:

- **ThumbnailDelegate** (`app/delegate/ThumbnailDelegate.h`): Thumbnail rendering
  - Chrome-style visual design
  - Hover and selection animations
  - Theme support (light/dark)
  - GPU-accelerated rendering

- **PageNavigationDelegate** (`app/delegate/PageNavigationDelegate.h`): Page navigation UI
- **ViewDelegate** (`app/delegate/ViewDelegate.h`): Main view rendering

## Key Subsystems

### Search Engine

High-performance PDF text search:

**Components:**

- `SearchEngine` - Core search functionality
- `IncrementalSearchManager` - Real-time incremental search
- `BackgroundProcessor` - Asynchronous search operations
- `TextExtractor` - PDF text extraction
- `SearchValidator` - Input validation and error recovery
- `MemoryManager` - Memory-aware result caching

**Features:**

- Multi-threaded search
- Incremental search with live results
- Memory-aware caching
- Error recovery and validation
- Search history and suggestions

### PDF Rendering

Efficient PDF rendering pipeline:

**Components:**

- `DocumentModel` - Document state management
- `RenderModel` - Rendering coordination
- `ThumbnailModel` - Thumbnail generation and caching
- `AsyncDocumentLoader` - Background document loading

**Features:**

- Asynchronous rendering
- Multi-resolution caching
- GPU fallback rendering
- Virtual scrolling for large documents
- Prerendering and smart caching

### Caching System

Intelligent multi-level caching:

**Components:**

- `CacheManager` - Unified cache interface
- `PDFCacheManager` - PDF-specific caching
- `SearchResultCache` - Search result persistence
- `PageTextCache` - Extracted text caching

**Features:**

- Multiple eviction strategies (LRU, LFU, size-based)
- Memory pressure handling
- Persistent cache support
- Cache statistics and monitoring

### Logging System

High-performance logging based on spdlog:

**Components:**

- `Logger` - Main logging interface
- `LoggingManager` - Configuration and management
- `QtSpdlogBridge` - Qt integration
- `SimpleLogging` - Simplified API

**Features:**

- Multiple sinks (console, file, rotating file, Qt widget)
- Runtime configuration
- Qt message redirection
- Performance monitoring
- Thread-safe logging

See [Logging System Documentation](logging-system.md) for detailed information.

### Managers

Application-level services:

- `I18nManager` - Internationalization (English/Chinese)
- `StyleManager` - Theme management (light/dark)
- `RecentFilesManager` - Recent files tracking
- `SystemTrayManager` - System tray integration
- `FileTypeIconManager` - File type associations
- `OnboardingManager` - First-time user experience

## Data Flow

### Document Loading Flow

```
User Action → OpenDocumentCommand → DocumentController
    ↓
AsyncDocumentLoader (background thread)
    ↓
DocumentModel.setDocument()
    ↓
Event: "document_opened" published
    ↓
Subscribers notified:
    - ThumbnailModel (generate thumbnails)
    - SearchModel (prepare for search)
    - UI Components (update display)
```

### Search Flow

```
User Input → SearchWidget → SearchValidator
    ↓
SearchEngine.search() (background thread)
    ↓
TextExtractor.extractText()
    ↓
Search algorithm execution
    ↓
Results → SearchResultCache
    ↓
SearchModel.setResults()
    ↓
UI update with highlighted results
```

### Rendering Flow

```
Page Request → RenderModel
    ↓
Check PageCache
    ↓ (cache miss)
Poppler rendering (background thread)
    ↓
Image processing and optimization
    ↓
Cache storage
    ↓
ViewWidget display update
```

## Extension Points

### Adding New Commands

```cpp
// 1. Define command class
class MyCommand : public DocumentCommand {
public:
    bool execute() override {
        // Implementation
        return true;
    }
    
    bool undo() override {
        // Undo logic
        return true;
    }
};

// 2. Register with CommandManager
GlobalCommandManager::registerCommand("my_command", 
    []() { return new MyCommand(); }
);

// 3. Optionally add keyboard shortcut
GlobalCommandManager::registerShortcut("my_command", "Ctrl+M");
```

### Adding New Services

```cpp
// 1. Define service interface
class MyService : public QObject {
    Q_OBJECT
public:
    virtual void doSomething() = 0;
};

// 2. Implement service
class MyServiceImpl : public MyService {
public:
    void doSomething() override {
        // Implementation
    }
};

// 3. Register with ServiceLocator
ServiceLocator::instance().registerService<MyService>(
    []() { return new MyServiceImpl(); }
);
```

### Adding New Event Types

```cpp
// 1. Define event type constant
const QString EVENT_MY_EVENT = "my_event";

// 2. Subscribe to event
EventBus::instance().subscribe(EVENT_MY_EVENT, this,
    [](Event* event) {
        // Handle event
    }
);

// 3. Publish event
EventBus::instance().publish(EVENT_MY_EVENT, eventData);
```

### Creating Plugins

```cpp
// 1. Implement PluginInterface
class MyPlugin : public QObject, public PluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.sast.readium.PluginInterface")
    Q_INTERFACES(PluginInterface)
    
public:
    QString name() const override { return "MyPlugin"; }
    QString version() const override { return "1.0.0"; }
    
    bool initialize() override {
        // Plugin initialization
        return true;
    }
    
    void shutdown() override {
        // Cleanup
    }
};

// 2. Build as shared library
// 3. Place in plugins directory
// 4. Plugin will be auto-discovered and loaded
```

## Best Practices

### Component Communication

1. **Use EventBus for loose coupling**: Prefer event-based communication over direct method calls
2. **Use Commands for user actions**: All user-initiated actions should be commands
3. **Use ServiceLocator for dependencies**: Avoid tight coupling through direct instantiation
4. **Use Factories for object creation**: Centralize object creation logic

### Threading

1. **Keep UI thread responsive**: Use background threads for heavy operations
2. **Use Qt's signal/slot mechanism**: For thread-safe communication
3. **Protect shared data**: Use mutexes or atomic operations
4. **Avoid blocking operations**: In the UI thread

See [Thread Safety Guidelines](thread-safety-guidelines.md) for detailed information.

### Memory Management

1. **Use smart pointers**: Prefer `std::unique_ptr` and `std::shared_ptr`
2. **Leverage Qt's parent-child**: For automatic memory management
3. **Implement proper cleanup**: In destructors and shutdown methods
4. **Monitor memory usage**: Use MemoryManager for tracking

### Testing

1. **Write unit tests**: For all business logic
2. **Use mock objects**: For testing in isolation
3. **Test command execution**: Verify undo/redo behavior
4. **Test event flows**: Verify event publication and handling

## Related Documentation

- [API Reference](api-reference.md) - Detailed API documentation
- [Logging System](logging-system.md) - Logging system guide
- [Thread Safety Guidelines](thread-safety-guidelines.md) - Thread safety best practices
- [Testing Guide](../tests/README.md) - Testing infrastructure and guidelines
