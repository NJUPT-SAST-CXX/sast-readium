# SAST Readium

A Qt6-based PDF reader application with comprehensive build support for multiple environments.

## Features

- **PDF Viewing**: High-quality PDF rendering with zoom, rotation, and navigation
- **Search Functionality**: Advanced text search with highlighting and navigation
- **Bookmarks**: Create, manage, and navigate bookmarks
- **Annotations**: Add and manage PDF annotations
- **Thumbnails**: Generate and display page thumbnails with GPU fallback rendering
- **Multi-tab Interface**: Open multiple documents simultaneously
- **Internationalization**: Support for multiple languages (English/Chinese)
- **Theme Support**: Light and dark theme options
- **Performance Optimizations**: Efficient rendering and memory management
- **Advanced Logging**: Comprehensive logging system with configurable sinks and categories
- **Memory Management**: Smart memory optimization with multiple eviction strategies
- **Debug Tools**: Advanced debug log panel with search and filtering capabilities
- **Cross-platform Support**: Windows, Linux, macOS
- **Multiple Build Environments**:
  - **vcpkg** for dependency management
  - **MSYS2** for Windows Unix-like development
  - **System packages** for native Linux/macOS builds

## Build Environments

The project supports **multiple build systems** for maximum flexibility:
- **CMake** (primary) - Comprehensive build system with extensive configuration
- **Xmake** (alternative) - Modern Lua-based build system with simpler syntax

The project uses a **tiered dependency management approach** that prioritizes system packages for better performance and reliability. See [Dependency Management Guide](docs/getting-started/dependency-management.md) for detailed information.

### Linux/macOS (Recommended - System Packages)

```bash
# Install system dependencies
# Ubuntu/Debian:
sudo apt install cmake ninja-build qt6-base-dev qt6-svg-dev qt6-tools-dev libpoppler-qt6-dev

# macOS:
brew install cmake ninja qt@6
# Note: poppler-qt6 needs to be built from source (see docs/getting-started/dependency-management.md)

# Configure and build
cmake --preset=Release-Unix
cmake --build --preset=Release-Unix
```

### Windows with MSYS2 (Recommended for Windows)

```bash
# Quick start - install dependencies and build
./scripts/build-msys2.sh -d

# Or step by step
./scripts/check-msys2-deps.sh -i  # Install dependencies
./scripts/build-msys2.sh          # Build with system packages
```

For detailed MSYS2 setup and build instructions, see [MSYS2 Build Guide](docs/setup/msys2-build.md).

### Windows with vcpkg (Fallback)

```bash
# Configure and build with vcpkg
cmake --preset=Release-Windows
cmake --build --preset=Release-Windows
```

**Note**: vcpkg builds are slower but provide consistent dependency versions across platforms. Use when system packages are unavailable or insufficient.

### Xmake Build System (Experimental)

An alternative xmake build system is available but currently has compatibility issues:

```bash
# Install xmake first (see docs/build-systems/xmake/xmake-build.md for installation)

# Note: Currently has Qt detection issues in MSYS2 environment
# Use CMake for production builds
```

**Status**: 🟡 Partially implemented - Qt integration pending

**Benefits when complete**:
- Simpler configuration syntax
- Built-in package management
- Faster builds with automatic caching
- Cross-platform consistency

See [Xmake Status](docs/build-systems/xmake/xmake-status.md) for current implementation status and [Xmake Build Guide](docs/build-systems/xmake/xmake-build.md) for detailed instructions.

## Quick Start

### Option 1: Xmake (Recommended for new users)

1. Install [xmake](https://xmake.io/)
2. Build and run:

   ```bash
   git clone <repository-url>
   cd sast-readium
   xmake f -m release  # Configure release build
   xmake               # Build
   xmake run           # Run application
   ```

### Option 2: MSYS2 (Windows, Unix-like experience)

1. Install [MSYS2](https://www.msys2.org/)
2. Open MSYS2 MINGW64 terminal
3. Clone and build:

   ```bash
   git clone <repository-url>
   cd sast-readium
   ./scripts/build-msys2.sh -d  # Install deps and build
   ```

### Option 2: vcpkg (Windows, traditional)

1. Install [vcpkg](https://vcpkg.io/)
2. Set `VCPKG_ROOT` environment variable
3. Build:

   ```cmd
   cmake --preset=Release-Windows
   cmake --build --preset=Release-Windows
   ```

### Option 3: System packages (Linux/macOS)

1. Install system dependencies
2. Build:

   ```bash
   cmake --preset=Release-Unix
   cmake --build --preset=Release-Unix
   ```

## Build Options

The build system supports flexible dependency management and IDE integration:

- **USE_VCPKG**: Enable/disable vcpkg (auto-detected)
- **FORCE_VCPKG**: Force vcpkg even in MSYS2
- **CMAKE_BUILD_TYPE**: Debug/Release
- **ENABLE_CLANGD_CONFIG**: Enable/disable automatic clangd configuration (default: ON)

### Disabling clangd Auto-Configuration

```bash
# Disable clangd auto-configuration
cmake --preset Debug-MSYS2 -DENABLE_CLANGD_CONFIG=OFF

# Force update clangd config even when disabled
.\scripts\update-clangd-config.ps1 -Auto -Force    # Windows
./scripts/update-clangd-config.sh --auto --force   # Linux/macOS
```

## Development Environment

### clangd Integration

The project includes automatic clangd configuration for enhanced IDE support:

```bash
# Configuration is automatically updated when running cmake
cmake --preset Debug          # Linux/macOS
cmake --preset Debug-MSYS2    # Windows MSYS2

# Manual configuration update
./scripts/update-clangd-config.sh --auto    # Linux/macOS
.\scripts\update-clangd-config.ps1 -Auto    # Windows

# List available configurations
./scripts/update-clangd-config.sh --list    # Linux/macOS
.\scripts\update-clangd-config.ps1 -List    # Windows
```

### Makefile Support (Linux/macOS)

```bash
make help           # Show available targets
make configure      # Configure Debug build
make build          # Build project
make clangd-auto    # Update clangd configuration
make dev            # Setup development environment
```

## Documentation

- [MSYS2 Build Guide](docs/setup/msys2-build.md) - Comprehensive MSYS2 setup and build instructions (Recommended)
- [Dependency Management Guide](docs/getting-started/dependency-management.md) - Detailed dependency management information
- [Xmake Status](docs/build-systems/xmake/xmake-status.md) - Current xmake implementation status and issues
- [Xmake Build Guide](docs/build-systems/xmake/xmake-build.md) - Modern Lua-based build system instructions (Experimental)
- [Build System Comparison](docs/build-systems/build-system-comparison.md) - CMake vs xmake feature comparison
- [clangd Setup Guide](docs/setup/clangd-setup.md) - IDE integration and clangd configuration
- [clangd Troubleshooting](docs/setup/clangd-troubleshooting.md) - Solutions for common clangd issues
- [clangd Configuration Options](docs/setup/clangd-config-options.md) - Advanced configuration control
- [Build Troubleshooting](docs/setup/msys2-build.md#troubleshooting) - Common build issues and solutions

## Dependencies

- **Qt6** (Core, Gui, Widgets, Svg, LinguistTools)
- **Poppler-Qt6** for PDF rendering
- **CMake** 3.28+ and **Ninja** for building

## License

MIT License - see [LICENSE](LICENSE) for details.
