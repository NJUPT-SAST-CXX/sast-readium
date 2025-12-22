# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

The project uses CMake 3.28+ with automatic source discovery and multiple build presets. The build system is structured with utility modules in `cmake/`:

- `ProjectConfig.cmake` - Project setup, compiler settings, platform detection, and clangd integration
- `Dependencies.cmake` - Dependency management (system packages vs vcpkg)
- `TargetUtils.cmake` - Source discovery and test creation utilities

### Common Build Commands

**Quick Setup (Recommended):**

```bash
# Linux/macOS with system packages
cmake --preset=Debug-Unix        # or Release-Unix
cmake --build --preset=Debug-Unix

# Windows with MSYS2 (Recommended for Windows, run in MSYS2 MINGW64 terminal)
./scripts/build-msys2.sh -d      # Install deps and build
# Or use presets directly:
cmake --preset=Debug-MSYS2
cmake --build --preset=Debug-MSYS2

# Cross-platform with vcpkg (Alternative)
cmake --preset=Debug-Windows     # Windows (also: Debug-Linux-vcpkg, Debug-macOS-vcpkg)
cmake --build --preset=Debug-Windows
```

**Development Commands:**

```bash
# List all available presets
cmake --list-presets=configure

# Run all tests
ctest --test-dir build -V        # Verbose output with test details
ctest --test-dir build --output-on-failure  # Show output only for failures

# Run specific test categories (Windows PowerShell)
.\scripts\run_tests.ps1 -TestType Unit
.\scripts\run_tests.ps1 -TestType Integration
.\scripts\run_tests.ps1 -TestType Smoke
.\scripts\run_tests.ps1 -TestType All -Parallel -Report  # Parallel with HTML report

# Run single test executable directly
./build/Debug-MSYS2/tests/test_cache_manager.exe  # Windows MSYS2
./build/Debug-Unix/tests/test_cache_manager        # Linux/macOS

# clangd configuration (auto-updated during cmake configuration)
./scripts/update-clangd-config.sh --auto    # Linux/macOS/MSYS2
.\scripts\update-clangd-config.ps1 -Auto    # Windows PowerShell
./scripts/update-clangd-config.sh --list    # List available configs
```

**Makefile Support (Linux/macOS):**

```bash
make help        # Show available targets
make configure   # Configure Debug build
make build       # Build project
make test        # Run tests
make clangd-auto # Update clangd config
make dev         # Setup development environment (installs pre-commit hooks)
```

**Packaging:**

```powershell
# Windows packaging
.\scripts\package.ps1                      # Create MSI installer (MSVC)
.\scripts\package.ps1 -PackageType nsis    # Create NSIS installer (MSYS2)
.\scripts\package.ps1 -PackageType zip     # Create portable ZIP
.\scripts\package.ps1 -Verify              # Create and verify package
```

## Architecture

The application follows a modular Qt6 architecture with clear separation of concerns:

### Core Components

**Application Entry:**

- `app/main.cpp` - Application entry point with console output and initialization
- `MainWindow` - Main application window (UI layer)

**Architecture Patterns:**

- **MVC Pattern:** Models in `app/model/`, Controllers in `app/controller/`, Delegates in `app/delegate/`
- **Command Pattern:** Commands in `app/command/` with `CommandManager` for undo/redo
- **Factory Pattern:** `ModelFactory` and `WidgetFactory` for object creation
- **Service Locator:** `ServiceLocator` for dependency injection
- **Plugin System:** `PluginManager` with `PluginInterface` for extensibility

**Key Subsystems:**

1. **Search Engine (`app/search/`):**
   - `SearchEngine` - Core search functionality with async execution
   - `IncrementalSearchManager` - Real-time search
   - `BackgroundProcessor` - Asynchronous search operations
   - `TextExtractor` - PDF text extraction
   - `SearchValidator` - Input validation and error recovery
   - `MemoryManager` - Search result memory optimization

2. **PDF Rendering (`app/model/`, `app/delegate/`):**
   - `DocumentModel` - PDF document state management
   - `RenderModel` - Page rendering coordination
   - `ThumbnailModel` + `ThumbnailDelegate` - Thumbnail generation with GPU fallback
   - `ThumbnailGenerator` - GPU/CPU rendering with `SafePDFRenderer` integration
   - `AsyncDocumentLoader` - Background document loading
   - `TextSelectionManager` - Spatial indexing with QMultiHash grid for O(1) character lookup

3. **Caching System (`app/cache/`):**
   - `CacheManager` - Unified cache interface with MVP architecture
   - `PDFCacheManager` - PDF-specific caching with thread-safe document access
   - `SearchResultCache` - Search result persistence
   - `PageTextCache` - Extracted text caching
   - Specialized models: `CacheEntryModel`, `CacheDataModel`, `CacheConfigModel`, `CacheStatsModel`

4. **Logging System (`app/logging/`):**
   - `Logger` - Main logging interface
   - `LoggingManager` - Log configuration and management
   - `QtSpdlogBridge` - Qt/spdlog integration
   - Comprehensive macro system in `LoggingMacros.h`

5. **Accessibility System (`app/accessibility/`):**
   - `AccessibilityManager` - Main interface and compatibility wrapper
   - `AccessibilityController` - Logic coordination and service access
   - `AccessibilityModel` - State management (TTS, screen reader, high contrast)
   - Text-to-Speech with rate/volume control and auto-advance

6. **Plugin System (`app/plugin/`):**
   - `PluginInterface` - Standard plugin interface
   - `PluginManager` - Plugin discovery, loading, lifecycle management
   - `PluginHookRegistry` - 20+ predefined workflow hooks
   - Specialized interfaces: Document processing, rendering, search, cache, annotations

7. **Annotation System (`app/annotations/`):**
   - `AnnotationModel` - Data management with Poppler integration
   - `AnnotationController` - Business logic coordination
   - `AnnotationCommands` - Undo/redo support
   - `AnnotationSelectionManager` - Interactive selection and resizing

8. **Managers (`app/managers/`):**
   - `I18nManager` - Internationalization (English/Chinese)
   - `StyleManager` - Theme management (light/dark/sepia)
   - `RecentFilesManager` - Recent files tracking
   - `SystemTrayManager` - System tray integration
   - `FileTypeIconManager` - File type associations
   - `KeyboardShortcutManager` - Centralized shortcut management with conflict detection

### Testing

Tests are located in `tests/` with a dedicated `TestUtilities` library. The build system automatically creates test targets using the `create_test_target()` function from `TargetUtils.cmake`. All test files follow the standardized `test_*.cpp` naming pattern.

### Dependency Management

The project uses a tiered approach:

1. **System packages** (recommended) for better performance
2. **vcpkg** as cross-platform alternative for consistency
3. **MSYS2** for Windows Unix-like development

Dependencies are configured in `cmake/Dependencies.cmake` with automatic platform detection.

### Language Server Integration

Automatic clangd configuration is handled by CMake. The `.clangd` file is auto-generated to point to the correct build directory's `compile_commands.json`. This ensures consistent IDE support without manual configuration drift.

## Development Patterns

### Command Pattern (Undo/Redo)

All user actions must be implemented as commands in `app/command/`:

```cpp
// Example: Adding a new command
class MyCommand : public QUndoCommand {
public:
    void undo() override { /* revert action */ }
    void redo() override { /* perform action */ }
};

// Register with CommandManager
CommandManager::instance().executeCommand(new MyCommand());
```

See existing commands: `DocumentCommands.cpp`, `NavigationCommands.cpp`, `InitializationCommand.cpp`

### Service Locator (Dependency Injection)

Central access to services via `ServiceLocator` in `app/controller/ServiceLocator.h`:

```cpp
// Register service during initialization
ServiceLocator::instance().registerService<MyService>([] { return new MyService(); });

// Retrieve service
auto* service = ServiceLocator::instance().getService<DocumentController>();
```

### Event Bus (Decoupled Communication)

Publish-subscribe pattern via `EventBus` in `app/controller/EventBus.h`:

```cpp
// Subscribe to events
EventBus::instance().subscribe("document_opened", this, [](const QVariant& data) {
    QString filename = data.toString();
    // Handle event
});

// Publish events
EventBus::instance().publish("document_opened", filename);
```

### Logging System

Use `app/logging/SimpleLogging.h` for logging:

```cpp
// Initialize logging (in main.cpp)
SastLogging::init();

// Use logging macros
SLOG_INFO("Document opened: %1").arg(path);
SLOG_ERROR("Failed to load: %1").arg(error);

// Or direct API
SastLogging::info("Operation completed");
```

Runtime configuration via JSON files in `config/logging*.json`. See `docs/logging-system.md` for details.

### Adding New Components

**New Command:**

1. Create command class in `app/command/`
2. Implement `undo()` and `redo()` methods
3. Register with `CommandManager`
4. Bind to UI/shortcuts

**New Service:**

1. Define interface/implementation
2. Register with `ServiceLocator` during initialization
3. Retrieve via `getService<T>()` where needed

**New Test:**

1. Create `test_*.cpp` in appropriate `tests/` subdirectory
2. CMake automatically discovers and creates test target
3. Use `TestUtilities` library for common test infrastructure
4. Run with `ctest` or `run_tests.ps1`

## Key Files to Reference

When working on the codebase, these files provide essential context:

**Entry Points:**

- `app/main.cpp` - Application entry point with initialization sequence
- `app/MainWindow.h/.cpp` - Main window and UI coordination

**Core Patterns:**

- `app/command/CommandManager.h` - Command pattern for undo/redo
- `app/controller/ServiceLocator.h` - Dependency injection
- `app/controller/EventBus.h` - Event-driven communication
- `app/factory/ModelFactory.h` - Model creation and configuration
- `app/factory/WidgetFactory.h` - Widget creation with auto-wiring

**Key Subsystems:**

- `app/cache/CacheManager.h` - Caching infrastructure
- `app/search/SearchEngine.h` - PDF text search
- `app/logging/SimpleLogging.h` - Logging API
- `app/model/DocumentModel.h` - PDF document state

**Documentation:**

- `docs/architecture.md` - Detailed architecture documentation
- `docs/logging-system.md` - Logging system guide
- `tests/README.md` - Testing framework documentation
- `cmake/README.md` - CMake modules documentation
- `.cursor/rules/` - Cursor AI rules for coding standards

## Thread Safety & Concurrency

The project has undergone extensive concurrency audits with critical fixes applied:

**Thread-Safe Patterns:**

- `PDFCacheManager` - Uses `documentMutex` to serialize background `PreloadTasks` accessing `Poppler::Document`
- `PDFPrerenderer` - Uses `QMutex` to serialize `Poppler::Document` access across worker threads
- `ThumbnailGenerator` - Integrates `SafePDFRenderer` for all rendering operations preventing hangs on corrupted PDFs
- `SearchEngine` - Executes searches directly in `BackgroundProcessor` worker thread, posts results via `Qt::QueuedConnection`
- `EventBus` - Lock minimization to prevent deadlocks in event processing

**Fixed Issues:**

- `CacheManager::optimizeCacheDistribution` - Deadlock fix applied
- `SearchEngine::performSearch` - Removed `Qt::BlockingQueuedConnection` to prevent UI freezes
- `AsyncDocumentLoader` - Async thread cleanup with timeout-bounded waits
- Timer callback overlap prevention using atomic flags in `CacheManager`

**Best Practices:**

- Use `QWaitCondition` with mutexes in producer-consumer patterns
- Avoid `Qt::BlockingQueuedConnection` from background threads to UI thread
- Use `std::unique_ptr<Poppler::Page>` for proper memory management (fixes leaks in `TextSelectionManager`)

## Performance Optimizations

- **Spatial Indexing:** `TextSelectionManager` uses `QMultiHash` grid system for O(1) character lookup (vs O(N) linear search)
- **Night Mode Caching:** `PageWidget::paintEvent` caches inverted images to reduce CPU usage
- **Outline Lookup:** `PDFOutlineWidget` uses `QMultiMap` for O(1) page-to-outline-item lookup
- **Virtual Scrolling:** `ThumbnailModel` implements lazy loading for large documents
- **Prerendering:** Smart page preloading based on scroll direction and view history

## Development Notes

- Source discovery is automatic via `discover_app_sources()` in `TargetUtils.cmake`
- The build system validates critical sources during configuration
- Library separation: `app_lib` contains all application code except `main.cpp` for testability
- Platform-specific configuration is handled in `ProjectConfig.cmake`
- C++20 standard is required with full compiler compliance checks
- Pre-commit hooks enforce code formatting (clang-format), static analysis (clang-tidy), and documentation (markdownlint)
  - Install with: `pip install pre-commit && pre-commit install`
  - Run manually: `pre-commit run --all-files`
- Project documentation system maintained in `llmdoc/` with master index at `llmdoc/index.md`
- Feature documentation in `llmdoc/feature/` covers accessibility, annotations, plugins, cache architecture, and freeze fixes
