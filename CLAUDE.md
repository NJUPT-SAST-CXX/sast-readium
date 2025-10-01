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
cmake --preset=Release-Unix
cmake --build --preset=Release-Unix

# Windows with MSYS2 (Recommended for Windows)
./scripts/build-msys2.sh -d    # Install deps and build

# Cross-platform with vcpkg (Alternative)
cmake --preset=Release-Windows     # Windows
cmake --preset=Release-Linux-vcpkg # Linux
cmake --build --preset=Release-Windows
```

**Development Commands:**
```bash
# List all available presets
cmake --list-presets=configure

# Debug builds
cmake --preset=Debug-Unix
cmake --build --preset=Debug-Unix

# Run tests
ctest --test-dir build -V
./scripts/run_tests.ps1    # Windows
./scripts/run_tests.py     # Python test runner

# clangd configuration (automatic with cmake, manual available)
./scripts/update-clangd-config.sh --auto
```

**Makefile Support (Linux/macOS):**
```bash
make help        # Show available targets
make configure   # Configure Debug build
make build       # Build project
make test        # Run tests
make clangd-auto # Update clangd config
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
   - `SearchEngine` - Core search functionality
   - `IncrementalSearchManager` - Real-time search
   - `BackgroundProcessor` - Asynchronous search operations
   - `TextExtractor` - PDF text extraction
   - `SearchValidator` - Input validation and error recovery
   - `MemoryManager` - Search result memory optimization

2. **PDF Rendering (`app/model/`, `app/delegate/`):**
   - `DocumentModel` - PDF document state management
   - `RenderModel` - Page rendering coordination
   - `ThumbnailModel` + `ThumbnailDelegate` - Thumbnail generation with GPU fallback
   - `AsyncDocumentLoader` - Background document loading

3. **Caching System (`app/cache/`):**
   - `CacheManager` - Unified cache interface
   - `PDFCacheManager` - PDF-specific caching
   - `SearchResultCache` - Search result persistence
   - `PageTextCache` - Extracted text caching

4. **Logging System (`app/logging/`):**
   - `Logger` - Main logging interface
   - `LoggingManager` - Log configuration and management
   - `QtSpdlogBridge` - Qt/spdlog integration
   - Comprehensive macro system in `LoggingMacros.h`

5. **Managers (`app/managers/`):**
   - `I18nManager` - Internationalization (English/Chinese)
   - `StyleManager` - Theme management (light/dark)
   - `RecentFilesManager` - Recent files tracking
   - `SystemTrayManager` - System tray integration
   - `FileTypeIconManager` - File type associations

### Testing

Tests are located in `tests/` with a dedicated `TestUtilities` library. The build system automatically creates test targets using the `create_test_target()` function from `TargetUtils.cmake`. Test discovery follows the pattern `test_*.cpp` for unit tests and `*_test.cpp` for integration tests.

### Dependency Management

The project uses a tiered approach:
1. **System packages** (recommended) for better performance
2. **vcpkg** as cross-platform alternative for consistency
3. **MSYS2** for Windows Unix-like development

Dependencies are configured in `cmake/Dependencies.cmake` with automatic platform detection.

### Language Server Integration

Automatic clangd configuration is handled by CMake. The `.clangd` file is auto-generated to point to the correct build directory's `compile_commands.json`. This ensures consistent IDE support without manual configuration drift.

## Development Notes

- Source discovery is automatic via `discover_app_sources()` in `TargetUtils.cmake`
- The build system validates critical sources during configuration
- Library separation: `app_lib` contains all application code except `main.cpp` for testability
- Platform-specific configuration is handled in `ProjectConfig.cmake`
- C++20 standard is required with full compiler compliance checks