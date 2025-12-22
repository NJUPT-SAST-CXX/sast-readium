<!-- OPENSPEC:START -->
# OpenSpec Instructions

These instructions are for AI assistants working in this project.

Always open `@/openspec/AGENTS.md` when the request:

- Mentions planning or proposals (words like proposal, spec, change, plan)
- Introduces new capabilities, breaking changes, architecture shifts, or big performance/security work
- Sounds ambiguous and you need the authoritative spec before coding

Use `@/openspec/AGENTS.md` to learn:

- How to create and apply change proposals
- Spec format and conventions
- Project structure and guidelines

Keep this managed block so 'openspec update' can refresh the instructions.

<!-- OPENSPEC:END -->

# AGENTS.md

This file provides guidance to AI assistants (GitHub Copilot, Cursor, etc.) when working with code in this repository.

## Project Overview

SAST Readium is a modern Qt6-based PDF reader application with C++20, featuring:

- **MVC Architecture** with Command, Factory, Service Locator, and Event Bus patterns
- **Cross-platform Support** for Windows (MSYS2/vcpkg), Linux, macOS
- **Advanced Features**: Text-to-Speech, annotations, spatial indexing, plugin system
- **Thread-safe Design** with comprehensive concurrency patterns

## Quick Reference

### Build Commands

```bash
# Windows MSYS2 (Recommended)
./scripts/build-msys2.sh -d              # Install deps and build

# Linux/macOS
cmake --preset=Release-Unix && cmake --build --preset=Release-Unix

# Windows vcpkg
cmake --preset=Release-Windows && cmake --build --preset=Release-Windows

# Tests
ctest --test-dir build --output-on-failure
.\scripts\run_tests.ps1 -TestType All    # Windows PowerShell
```

### Key Directories

| Directory | Purpose |
|-----------|---------|
| `app/` | Main application source code |
| `app/command/` | Command pattern implementations |
| `app/controller/` | Controllers (Application, Document, Page) |
| `app/model/` | Data models (Document, Render, Thumbnail, Search) |
| `app/cache/` | Caching system with MVP architecture |
| `app/search/` | Search engine with async processing |
| `app/plugin/` | Plugin system with hook registry |
| `app/accessibility/` | TTS, screen reader, high contrast support |
| `tests/` | Unit and integration tests |
| `llmdoc/` | AI-generated feature documentation |
| `docs/` | User and developer documentation |

## Architecture Patterns

### 1. Command Pattern (`app/command/`)

All user actions must be commands for undo/redo support:

```cpp
CommandManager& cmdMgr = GlobalCommandManager::instance();
cmdMgr.executeCommand("open_document");
cmdMgr.undo();
cmdMgr.redo();
```

### 2. Service Locator (`app/controller/ServiceLocator.h`)

Dependency injection for loose coupling:

```cpp
ServiceLocator::instance().registerService<MyService>([] { return new MyService(); });
auto* service = ServiceLocator::instance().getService<DocumentController>();
```

### 3. Event Bus (`app/controller/EventBus.h`)

Decoupled publish-subscribe communication:

```cpp
EventBus::instance().subscribe("document_opened", this, [](Event* e) { /* handle */ });
EventBus::instance().publish("document_opened", filename);
```

### 4. Factory Pattern (`app/factory/`)

Use `ModelFactory` and `WidgetFactory` for object creation instead of direct instantiation.

## Thread Safety Guidelines

**CRITICAL**: Follow these patterns to avoid deadlocks and freezes:

- Use `QWaitCondition` with mutexes in producer-consumer patterns
- **NEVER** use `Qt::BlockingQueuedConnection` from background threads to UI thread
- Use `std::unique_ptr<Poppler::Page>` for proper memory management
- Serialize `Poppler::Document` access with mutex in background tasks
- Use atomic flags to prevent timer callback overlap

**Known Safe Implementations:**

- `PDFCacheManager` - Uses `documentMutex` for thread-safe preloading
- `SearchEngine` - Runs searches in `BackgroundProcessor`, posts results via `Qt::QueuedConnection`
- `ThumbnailGenerator` - Uses `SafePDFRenderer` for corruption-safe rendering

## Performance Considerations

- **Spatial Indexing**: `TextSelectionManager` uses `QMultiHash` grid for O(1) character lookup
- **Virtual Scrolling**: `ThumbnailModel` lazy-loads for large documents
- **Cache Architecture**: MVP-based with specialized models (`CacheEntryModel`, `CacheDataModel`, etc.)
- **Prerendering**: Adaptive preloading based on scroll direction

## Adding New Features

### New Command

1. Create class in `app/command/` inheriting from `QUndoCommand`
2. Implement `undo()` and `redo()` methods
3. Register with `CommandManager`
4. Bind to UI/shortcuts

### New Test

1. Create `test_*.cpp` in appropriate `tests/` subdirectory
2. CMake automatically discovers and creates test target
3. Use `TestUtilities` library for common infrastructure

### New Plugin

1. Implement `PluginInterface` in `app/plugin/`
2. Use `PluginHookRegistry` for workflow integration
3. See `examples/plugins/` for reference implementations

## Documentation References

- **Architecture**: `docs/architecture.md`
- **Feature Documentation**: `llmdoc/feature/` (accessibility, annotations, plugins, cache, freeze fixes)
- **Thread Safety**: `docs/thread-safety-guidelines.md`
- **Testing**: `tests/README.md`

## Code Style

- C++20 standard required
- Follow existing naming conventions (camelCase for functions, PascalCase for classes)
- Use logging macros from `app/logging/SimpleLogging.h`
- Pre-commit hooks enforce clang-format and clang-tidy
