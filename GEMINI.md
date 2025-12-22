# GEMINI.md

This file provides guidance to Google Gemini when working with code in this repository.

## Project Overview

SAST Readium is a modern Qt6-based PDF reader application with C++20, featuring comprehensive multi-platform support (Windows, Linux, macOS), advanced architectural patterns, and a sophisticated PDF rendering system.

## Build System

The project uses CMake 3.28+ with automatic source discovery. Build system modules in `cmake/`:

- `ProjectConfig.cmake` - Project setup, compiler settings, platform detection
- `Dependencies.cmake` - Dependency management (system packages vs vcpkg)
- `TargetUtils.cmake` - Source discovery and test creation utilities

### Quick Build Commands

```bash
# Windows MSYS2 (Recommended)
./scripts/build-msys2.sh -d              # Install deps and build

# Linux/macOS with system packages
cmake --preset=Release-Unix
cmake --build --preset=Release-Unix

# Windows with vcpkg
cmake --preset=Release-Windows
cmake --build --preset=Release-Windows

# Run tests
ctest --test-dir build --output-on-failure
.\scripts\run_tests.ps1 -TestType All    # Windows PowerShell
```

## Architecture

### Core Patterns

The application follows a modular Qt6 architecture with:

- **MVC Pattern**: Models in `app/model/`, Controllers in `app/controller/`, Delegates in `app/delegate/`
- **Command Pattern**: Commands in `app/command/` with `CommandManager` for undo/redo support
- **Factory Pattern**: `ModelFactory` and `WidgetFactory` for standardized object creation
- **Service Locator**: `ServiceLocator` for dependency injection
- **Event Bus**: `EventBus` for decoupled publish-subscribe communication
- **Plugin System**: `PluginManager` with `PluginInterface` for extensibility

### Key Subsystems

1. **Search Engine (`app/search/`)**:
   - `SearchEngine` - Core search with async execution via `BackgroundProcessor`
   - `IncrementalSearchManager` - Real-time search
   - `SearchValidator` - Input validation and error recovery

2. **PDF Rendering (`app/model/`, `app/delegate/`)**:
   - `DocumentModel` - Document state management
   - `RenderModel` - Rendering coordination
   - `ThumbnailModel` + `ThumbnailGenerator` - GPU/CPU thumbnail generation
   - `TextSelectionManager` - Spatial indexing with O(1) character lookup

3. **Caching System (`app/cache/`)**:
   - MVP architecture with `CacheManager`, `PDFCacheManager`
   - Specialized models: `CacheEntryModel`, `CacheDataModel`, `CacheConfigModel`, `CacheStatsModel`

4. **Accessibility (`app/accessibility/`)**:
   - `AccessibilityManager` - Main interface
   - Text-to-Speech with rate/volume control
   - Screen reader and high contrast support

5. **Plugin System (`app/plugin/`)**:
   - `PluginInterface` and `PluginManager`
   - `PluginHookRegistry` - 20+ predefined workflow hooks
   - Specialized interfaces for document processing, rendering, search, cache, annotations

6. **Annotation System (`app/annotations/`)**:
   - `AnnotationModel` - Data management with Poppler integration
   - `AnnotationController` - Business logic
   - `AnnotationCommands` - Undo/redo support

### Managers (`app/managers/`)

- `I18nManager` - Internationalization (English/Chinese)
- `StyleManager` - Theme management (light/dark/sepia)
- `RecentFilesManager` - Recent files tracking
- `KeyboardShortcutManager` - Centralized shortcut management

## Thread Safety Guidelines

**CRITICAL** - Follow these patterns to avoid deadlocks:

- Use `QWaitCondition` with mutexes in producer-consumer patterns
- **NEVER** use `Qt::BlockingQueuedConnection` from background threads to UI thread
- Use `std::unique_ptr<Poppler::Page>` for proper memory management
- Serialize `Poppler::Document` access with mutex in background tasks

**Thread-Safe Components:**

- `PDFCacheManager` - Uses `documentMutex` for thread-safe preloading
- `SearchEngine` - Runs searches in `BackgroundProcessor`, posts results via `Qt::QueuedConnection`
- `ThumbnailGenerator` - Uses `SafePDFRenderer` for corruption-safe rendering

## Performance Optimizations

- **Spatial Indexing**: `TextSelectionManager` uses `QMultiHash` grid for O(1) character lookup
- **Night Mode Caching**: `PageWidget::paintEvent` caches inverted images
- **Virtual Scrolling**: `ThumbnailModel` lazy-loads for large documents
- **Prerendering**: Adaptive page preloading based on scroll direction

## Development Patterns

### Adding a New Command

```cpp
class MyCommand : public QUndoCommand {
public:
    void undo() override { /* revert action */ }
    void redo() override { /* perform action */ }
};

CommandManager::instance().executeCommand(new MyCommand());
```

### Using Service Locator

```cpp
// Register service
ServiceLocator::instance().registerService<MyService>([] { return new MyService(); });

// Retrieve service
auto* service = ServiceLocator::instance().getService<DocumentController>();
```

### Using Event Bus

```cpp
// Subscribe
EventBus::instance().subscribe("document_opened", this, [](Event* e) { /* handle */ });

// Publish
EventBus::instance().publish("document_opened", filename);
```

### Logging

```cpp
#include "logging/SimpleLogging.h"

SLOG_INFO("Document opened: %1").arg(path);
SLOG_ERROR("Failed to load: %1").arg(error);
```

## Key Files Reference

**Entry Points:**

- `app/main.cpp` - Application entry point
- `app/MainWindow.h/.cpp` - Main window and UI coordination

**Core Patterns:**

- `app/command/CommandManager.h` - Command pattern
- `app/controller/ServiceLocator.h` - Dependency injection
- `app/controller/EventBus.h` - Event-driven communication
- `app/factory/ModelFactory.h` - Model creation

**Key Subsystems:**

- `app/cache/CacheManager.h` - Caching infrastructure
- `app/search/SearchEngine.h` - PDF text search
- `app/logging/SimpleLogging.h` - Logging API

**Documentation:**

- `docs/architecture.md` - Architecture documentation
- `docs/thread-safety-guidelines.md` - Concurrency guidelines
- `llmdoc/feature/` - Feature documentation (accessibility, annotations, plugins, cache, freeze fixes)
- `tests/README.md` - Testing framework

## Development Notes

- C++20 standard required
- Source discovery automatic via `discover_app_sources()` in `TargetUtils.cmake`
- Library separation: `app_lib` contains all code except `main.cpp` for testability
- Pre-commit hooks enforce clang-format and clang-tidy
- Project documentation in `llmdoc/` with master index at `llmdoc/index.md`
