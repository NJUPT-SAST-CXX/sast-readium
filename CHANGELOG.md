# Changelog

All notable changes to SAST Readium will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

#### Build System & Tooling
- **Cross-Compilation Toolchains**: Comprehensive toolchain support for macOS (Intel & Apple Silicon), Linux (x86_64 & ARM64), and Windows (MinGW & MSYS2)
  - `arm64-apple-darwin.cmake` - macOS Apple Silicon targeting
  - `x86_64-apple-darwin.cmake` - macOS Intel targeting
  - `x86_64-linux-gnu.cmake` - Linux x86_64 targeting
  - `x86_64-w64-mingw32.cmake` - Windows MinGW cross-compilation
  - `x86_64-w64-mingw64-msys2.cmake` - MSYS2 MinGW-w64 with automatic detection
- **Clang Compiler Support**: Full Clang compiler support across all desktop platforms
  - `clang-windows.cmake` - Windows Clang (standard and Clang-cl modes)
  - `clang-linux.cmake` - Linux Clang with security hardening
  - `clang-macos.cmake` - macOS Clang with universal binary support
- **CMake Utility Modules**: Modular build system with reusable components
  - `ProjectConfig.cmake` - Consolidated project setup, compiler settings, and platform detection
  - `TargetUtils.cmake` - Source discovery and test creation utilities
  - `Dependencies.cmake` - Intelligent dependency management (system packages vs vcpkg)
- **clangd Integration**: Automatic clangd configuration with unified scripts
  - `scripts/clangd-config.sh` - Unix/Linux/macOS configuration script
  - `scripts/clangd-config.ps1` - Windows PowerShell configuration script
- **Build System Documentation**: Comprehensive documentation in `cmake/README.md` and `docs/MIGRATION-GUIDE.md`

#### Architecture & Design Patterns
- **Command Pattern**: Full implementation with undo/redo support
  - `CommandManager` - Centralized command execution and history management
  - `DocumentCommands` - Document-related commands (open, close, save)
  - `NavigationCommands` - Page navigation commands (next, previous, goto)
  - `InitializationCommand` - Application initialization sequence
- **Service Locator Pattern**: Dependency injection and service management
  - `ServiceLocator` - Centralized service registry with lazy loading
  - Factory-based service creation with lifecycle management
  - Signal-based service lifecycle notifications
- **Event Bus Pattern**: Decoupled event-driven communication
  - `EventBus` - Publish-subscribe event system
  - Synchronous and asynchronous event delivery
  - Event filtering and priority handling
  - Thread-safe event processing
- **Factory Pattern**: Standardized object creation
  - `ModelFactory` - Creates and configures all model objects
  - `WidgetFactory` - Creates UI widgets with proper connections
  - `CommandFactory` - Creates command objects with dependencies
- **Plugin System**: Extensibility infrastructure
  - `PluginInterface` - Standard plugin interface
  - `PluginManager` - Plugin discovery, loading, and lifecycle management

#### Core Features
- **Centralized Logging System**: High-performance logging based on spdlog
  - Multiple sinks (console, file, rotating file, Qt widget)
  - Qt integration with automatic message redirection
  - Runtime configuration and log level management
  - Performance monitoring and memory tracking
  - Comprehensive macro system for easy logging
- **Enhanced Thumbnail System**: Chrome-style PDF thumbnails
  - `ThumbnailModel` - Virtual scrolling with intelligent caching
  - `ThumbnailDelegate` - Modern rendering with animations
  - GPU fallback rendering for better performance
  - Lazy loading and memory optimization
- **Multi-Document Support**: Open and manage multiple PDF documents
  - `AsyncDocumentLoader` - Background document loading
  - Recent files management with persistence
  - Document metadata tracking
- **Welcome Screen**: User-friendly onboarding experience
  - `WelcomeWidget` - Welcome screen with recent files
  - `OnboardingWidget` - First-time user tutorial
  - `TutorialCard` - Interactive tutorial components
- **System Tray Integration**: Background operation support
  - `SystemTrayManager` - System tray icon and menu
  - Minimize to tray functionality
  - Quick access to recent documents

#### UI Enhancements
- **File Type Icon Management**: Visual file type identification
  - `FileTypeIconManager` - SVG-based file type icons
  - Support for PDF, EPUB, DOC, TXT, and default icons
- **Advanced Search Features**: Enhanced search capabilities
  - `SearchValidator` - Input validation and error recovery
  - `SearchErrorRecovery` - Automatic error handling
  - `MemoryManager` - Memory-aware search result caching
  - `IncrementalSearchManager` - Real-time incremental search
- **Document Metadata Dialog**: Comprehensive PDF metadata viewing
  - Styled dialog with modern appearance
  - Translation support for metadata fields
  - Copy-to-clipboard functionality

#### Testing Infrastructure
- **Comprehensive Test Suite**: 100+ unit and integration tests
  - Command pattern tests (CommandManager, all command types)
  - Controller tests (ApplicationController, DocumentController, PageController)
  - Factory tests (ModelFactory, WidgetFactory)
  - Service Locator and EventBus tests
  - Search engine tests (core, performance, thread safety, error recovery)
  - PDF utilities and error handling tests
  - UI component integration tests
- **Test Utilities Library**: Shared testing infrastructure
  - `TestUtilities` - Common test helpers and mocks
  - Automatic test discovery and registration
  - Test categorization (unit, integration, performance, smoke)

### Changed

#### Build System
- **Simplified CMake Configuration**: Reduced complexity while maintaining functionality
  - Consolidated 8 CMake modules into 3 focused modules
  - Reduced CMake presets from 22+ to 6 essential configurations
  - Unified build system (CMake only, removed xmake and Makefile alternatives)
- **Improved Dependency Management**: Tiered approach for better flexibility
  - System packages prioritized for better performance
  - vcpkg as cross-platform alternative
  - MSYS2 support for Windows Unix-like development
  - Automatic platform and environment detection
- **Enhanced clangd Support**: Automatic configuration with manual override options
  - Auto-generated `.clangd` configuration
  - Unified configuration scripts across platforms
  - Build directory detection and validation

#### Architecture
- **Logging System Reorganization**: Moved from `app/utils/` to `app/logging/`
  - Better organization and discoverability
  - Simplified API with `SimpleLogging.h`
  - Enhanced Qt integration with `QtSpdlogBridge`
- **Controller Refactoring**: Improved separation of concerns
  - `ApplicationController` - Application-level coordination
  - `DocumentController` - Document lifecycle management
  - `PageController` - Page navigation and state
  - `ConfigurationManager` - Centralized configuration
- **Model Improvements**: Enhanced model architecture
  - `RenderModel` - Centralized rendering coordination
  - `DocumentModel` - Document state management
  - `ThumbnailModel` - Virtual scrolling and caching
  - `SearchModel` - Search state and results

#### Performance
- **Memory Optimization**: Improved memory management across the application
  - Smart caching with multiple eviction strategies
  - Memory-aware search result management
  - Thumbnail cache optimization
  - Automatic memory pressure handling
- **Rendering Performance**: Enhanced PDF rendering efficiency
  - Asynchronous thumbnail generation
  - GPU fallback for better performance
  - Prerendering and smart caching
  - Virtual scrolling for large documents

### Fixed
- **Build System Issues**:
  - Fixed vcpkg dependency detection in MSYS2 environment
  - Resolved spdlog and Qt6TextToSpeech missing dependencies
  - Fixed clangd configuration drift issues
- **UI Issues**:
  - Fixed incorrect page number updates when opening PDF files
  - Fixed scale factor exceeding limits
  - Resolved enum naming conflict with unistd.h (`close` → `closeFile`)
  - Fixed horizontal scrollbar and view widget styles
- **Performance Issues**:
  - Optimized search performance with better caching
  - Reduced memory usage in thumbnail generation
  - Improved rendering performance with QGraphicsView

### Deprecated
- **Old Build Scripts**: Replaced with unified scripts
  - `update-clangd-config.bat` → `clangd-config.ps1`
  - `update-clangd-config.sh` → `clangd-config.sh`
  - Multiple test scripts consolidated into `run_tests.ps1` and `run_tests.py`

### Removed
- **Obsolete Build System Files**:
  - Removed xmake build system (CMake is now the only build system)
  - Removed Makefile-based build (use CMake presets instead)
  - Removed redundant CMake modules (consolidated into 3 modules)
- **Deprecated Scripts**:
  - `build-msys2.bat` (use `build-msys2.sh` instead)
  - `test-msys2-build.sh`, `test-clangd-config.ps1`, `test-debug-setup.*`
  - `migrate_tests.py`, `reorganize_tests.*`, `restart-clangd.*`

### Security
- **Clang Security Hardening**: Enhanced security features for Linux builds
  - Stack protector enabled
  - RELRO (Relocation Read-Only) protection
  - Position Independent Executables (PIE)

## [0.1.0] - Initial Release

### Added
- Basic PDF viewing functionality
- Page navigation
- Zoom and rotation controls
- Search functionality
- Bookmark support
- Annotation support
- Multi-tab interface
- Internationalization (English/Chinese)
- Theme support (light/dark)
- Cross-platform support (Windows, Linux, macOS)

---

## Migration Notes

### For Developers

If you're upgrading from an earlier version:

1. **Update Build Configuration**: Use new simplified CMake presets
   ```bash
   cmake --preset Release-Unix      # Linux/macOS
   cmake --preset Release-Windows   # Windows with vcpkg
   cmake --preset Release-MSYS2     # Windows MSYS2
   ```

2. **Update clangd Configuration**: Use new unified scripts
   ```bash
   ./scripts/clangd-config.sh --auto        # Unix/Linux/macOS
   .\scripts\clangd-config.ps1 -Auto        # Windows
   ```

3. **Update Logging Includes**: Logging moved to dedicated directory
   ```cpp
   // Old
   #include "utils/LoggingMacros.h"
   
   // New
   #include "logging/SimpleLogging.h"
   ```

4. **Adopt New Patterns**: Consider using new architecture patterns
   - Use `CommandManager` for undoable operations
   - Use `ServiceLocator` for dependency injection
   - Use `EventBus` for decoupled communication
   - Use `ModelFactory` and `WidgetFactory` for object creation

See [MIGRATION-GUIDE.md](docs/MIGRATION-GUIDE.md) for detailed migration instructions.

---

[Unreleased]: https://github.com/NJUPT-SAST-CXX/sast-readium/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/NJUPT-SAST-CXX/sast-readium/releases/tag/v0.1.0

