# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Project Overview

SAST Readium is a modern Qt6-based PDF reader application with comprehensive multi-platform support. The project uses C++20 and features a sophisticated architecture with MVC pattern, extensive build system support, and advanced PDF rendering capabilities.

## Common Development Commands

### Build Commands

**Windows (MSYS2 - Recommended):**

```bash
# Quick build with dependency installation
./scripts/build-msys2.sh -d

# Development build
./scripts/build-msys2.sh -t Debug -d

# Clean release build
./scripts/build-msys2.sh -t Release -c

# Build with vcpkg (if system packages unavailable)
./scripts/build-msys2.sh -v -t Debug
```

**Windows (vcpkg fallback):**

```bash
cmake --preset=Release-Windows
cmake --build --preset=Release-Windows
```

**Linux/macOS (System packages):**

```bash
# Configure and build
cmake --preset=Release-Unix
cmake --build --preset=Release-Unix

# Or using Makefile shortcuts
make configure      # Configure Debug build
make build          # Build project
make dev           # Setup development environment
```

### CMake Presets Available

- `Debug-MSYS2` / `Release-MSYS2` - MSYS2 with system packages (Windows recommended)
- `Debug-Unix` / `Release-Unix` - System packages (Linux/macOS)
- `Debug-Windows` / `Release-Windows` - vcpkg with Visual Studio (Windows fallback)
- `Debug-MSYS2-vcpkg` / `Release-MSYS2-vcpkg` - MSYS2 with vcpkg (advanced)

### Testing

```bash
# Run search engine tests
cd tests
python run_search_tests.py --build-dir ../build/Debug-MSYS2

# Run specific test categories
python run_search_tests.py --benchmarks-only

# Using CTest
cd build
ctest -R "test_search" --output-on-failure
```

### Development Environment Setup

**clangd Configuration (automatic):**

```bash
# Configuration updates automatically during CMake runs
cmake --preset Debug-MSYS2    # Windows MSYS2
cmake --preset Debug-Unix     # Linux/macOS

# Manual updates
./scripts/update-clangd-config.sh --auto    # Linux/macOS
.\scripts\update-clangd-config.ps1 -Auto    # Windows
```

**Pre-commit hooks:**

```bash
# Install pre-commit
pip install pre-commit

# Setup hooks (includes clang-format, cmake-format, black)
pre-commit install

# Run on all files
pre-commit run --all-files
```

## Architecture Overview

### Core Architecture Pattern

The application follows a **Model-View-Controller (MVC)** architecture with additional manager and factory patterns:

**Models:**

- `RenderModel` - Core PDF rendering and DPI management
- `DocumentModel` - Document state and operations
- `PageModel` - Individual page management
- `SearchModel`, `BookmarkModel`, `AnnotationModel` - Feature-specific models

**Controllers:**

- `DocumentController` - Primary document operations and coordination
- `PageController` - Page navigation and display logic

**Views (UI Components):**

- `MainWindow` - Central application window with QStackedWidget
- `ViewWidget` - PDF display area
- `SideBar`/`RightSideBar` - Navigation and tool panels
- `WelcomeWidget` - Initial application screen

### Key Architectural Components

**Manager Classes:**

- `StyleManager` - Theme management and UI consistency
- `RecentFilesManager` - File history with LRU caching
- `FileTypeIconManager` - File type icon handling
- `WelcomeScreenManager` - Welcome screen logic

**Factory Pattern:**

- `WidgetFactory` - Centralized UI component creation

**Advanced Features:**

- `OptimizedSearchEngine` - Enhanced PDF search with fuzzy matching
- `ThumbnailGenerator` - Chrome-style thumbnail system
- `PDFCacheManager` - Intelligent caching system
- `AsyncDocumentLoader` - Non-blocking document loading

### Dependency Management Strategy

The project uses a **tiered approach** prioritizing system packages:

1. **Primary**: MSYS2 system packages (Windows) / Native system packages (Linux/macOS)
2. **Fallback**: vcpkg for consistent cross-platform dependencies
3. **Detection**: Automatic environment detection and dependency selection

### Logging System

Comprehensive logging using spdlog with categories:

- Configurable log levels and outputs
- Development vs production presets
- Thread-safe operation
- Multiple sink support (console, file)

## Development Guidelines

### Build Environment Detection

The build system automatically detects:

- MSYS2 environment (`$MSYSTEM` variable)
- vcpkg availability (`$VCPKG_ROOT`)
- Platform-specific toolchains

### Code Quality Tools

- **clang-format**: C++ code formatting (automatic via pre-commit)
- **cmake-format**: CMake file formatting
- **pre-commit hooks**: Comprehensive quality checks
- **clangd**: Language server support with automatic configuration

### Cross-Platform Considerations

- C++20 standard across all platforms
- Qt6 minimum version requirements
- Platform-specific optimizations in CMake configuration
- Automatic deployment scripts (windeployqt for Windows)

### Performance Optimizations

- Virtual scrolling for large documents
- Intelligent caching strategies
- Background/async operations for UI responsiveness
- Memory management with cleanup verification

### Testing Strategy

- Unit tests for core functionality
- Integration tests for component interaction
- Performance benchmarks with regression detection
- Edge case and error handling validation

## Key Dependencies

**Required:**

- Qt6 (Core, Gui, Widgets, Svg, LinguistTools, Concurrent, TextToSpeech)
- Poppler-Qt6 (PDF rendering)
- spdlog (logging)
- CMake 3.28+, Ninja

**Development:**

- clang-format, cmake-format
- Python 3.x (for build scripts and testing)
- pre-commit (code quality)

## Project Structure Notes

- `app/` - Main application source code with organized subdirectories
- `scripts/` - Build automation and development tools
- `tests/` - Comprehensive test suite with performance benchmarks
- `docs/` - Extensive documentation with setup guides
- `config/` - Logging and configuration files
- `assets/styles/` - UI theme and style resources

The project emphasizes developer experience with automatic IDE integration, comprehensive documentation, and flexible build options suitable for various development environments.
