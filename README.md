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
  - **System packages** for native Linux/macOS builds (recommended)
  - **vcpkg** for cross-platform dependency management
  - **MSYS2** for Windows Unix-like development

## Build System

The project uses **CMake** as the unified build system with simplified configuration presets for different platforms and environments.

The project uses a **tiered dependency management approach** that prioritizes system packages for better performance and reliability, with vcpkg as a cross-platform alternative for consistent dependency versions. See [Dependency Management Guide](docs/getting-started/dependency-management.md) for detailed information.

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

### Linux/macOS with vcpkg (Alternative)

```bash
# Install vcpkg first
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
export VCPKG_ROOT=$(pwd)

# Configure and build with vcpkg
cd /path/to/sast-readium
cmake --preset=Release-Linux-vcpkg    # For Linux
cmake --preset=Release-macOS-vcpkg    # For macOS Intel
cmake --preset=Release-macOS-vcpkg-arm64  # For macOS Apple Silicon

cmake --build --preset=Release-Linux-vcpkg    # For Linux
cmake --build --preset=Release-macOS-vcpkg    # For macOS Intel
cmake --build --preset=Release-macOS-vcpkg-arm64  # For macOS Apple Silicon
```

### Windows with vcpkg (Alternative)

```bash
# Configure and build with vcpkg
cmake --preset=Release-Windows
cmake --build --preset=Release-Windows
```

**Note**: vcpkg builds are slower but provide consistent dependency versions across platforms. Use when system packages are unavailable or insufficient, or when you need identical dependency versions across different platforms.

## Available Build Presets

The simplified CMake configuration provides multiple build presets for different platforms and dependency management approaches:

**System Package Builds (Recommended):**

- **Debug-Unix** / **Release-Unix**: For Linux/macOS using system packages
- **Debug-MSYS2** / **Release-MSYS2**: For Windows MSYS2 using system packages

**vcpkg Builds (Alternative):**

- **Debug-Windows** / **Release-Windows**: For Windows using vcpkg
- **Debug-Linux-vcpkg** / **Release-Linux-vcpkg**: For Linux using vcpkg
- **Debug-macOS-vcpkg** / **Release-macOS-vcpkg**: For macOS Intel using vcpkg
- **Debug-macOS-vcpkg-arm64** / **Release-macOS-vcpkg-arm64**: For macOS Apple Silicon using vcpkg

List all available presets:

```bash
cmake --list-presets=configure
```

## Quick Start

### Option 1: CMake with Presets (Recommended)

1. Choose your platform preset and build:

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

### Option 2: vcpkg (Cross-platform alternative)

**For Linux/macOS:**

1. Install [vcpkg](https://vcpkg.io/):

   ```bash
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   ./bootstrap-vcpkg.sh
   export VCPKG_ROOT=$(pwd)
   ```

2. Build:

   ```bash
   git clone <repository-url>
   cd sast-readium
   cmake --preset=Release-Linux-vcpkg    # Linux
   cmake --preset=Release-macOS-vcpkg    # macOS Intel
   cmake --preset=Release-macOS-vcpkg-arm64  # macOS Apple Silicon
   cmake --build --preset=Release-Linux-vcpkg    # Linux
   cmake --build --preset=Release-macOS-vcpkg    # macOS Intel
   cmake --build --preset=Release-macOS-vcpkg-arm64  # macOS Apple Silicon
   ```

**For Windows:**

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
- **VCPKG_TARGET_TRIPLET**: Override vcpkg triplet (auto-detected from preset)

## vcpkg Troubleshooting

**Common Issues and Solutions:**

1. **VCPKG_ROOT not set:**

   ```bash
   export VCPKG_ROOT=/path/to/vcpkg  # Linux/macOS
   set VCPKG_ROOT=C:\path\to\vcpkg   # Windows
   ```

2. **vcpkg dependencies not found:**

   ```bash
   # Install dependencies manually if needed
   $VCPKG_ROOT/vcpkg install qtbase qtsvg qtspeech qttools[linguist] poppler[qt] spdlog
   ```

3. **Slow vcpkg builds:**
   - vcpkg builds from source and can be very slow (30+ minutes)
   - Consider using system packages for faster builds
   - Use `--preset=Release-*` for faster builds than Debug

4. **Platform-specific issues:**
   - **Linux**: Ensure build-essential, cmake, ninja-build are installed
   - **macOS**: Ensure Xcode command line tools are installed
   - **Apple Silicon**: Use `arm64-osx` triplet presets for native builds

5. **Disk space issues:**
   - vcpkg requires significant disk space (2-5GB+)
   - Clean vcpkg buildtrees: `$VCPKG_ROOT/vcpkg remove --outdated`

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
