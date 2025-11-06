# SAST Readium

A Qt6-based PDF reader application with comprehensive build support for multiple environments.

## Features

### Core Functionality

- **PDF Viewing**: High-quality PDF rendering with zoom, rotation, and navigation
- **Search Functionality**: Advanced text search with highlighting, incremental search, and error recovery
- **Bookmarks**: Create, manage, and navigate bookmarks
- **Annotations**: Add and manage PDF annotations
- **Thumbnails**: Chrome-style thumbnails with GPU fallback rendering and virtual scrolling
- **Multi-Document Support**: Open and manage multiple PDF documents with recent files tracking
- **Internationalization**: Support for multiple languages (English/Chinese)
- **Theme Support**: Light and dark theme options with modern UI design

### Architecture & Design Patterns

- **Command Pattern**: Undo/redo support with `CommandManager` for all operations
- **Service Locator**: Dependency injection with `ServiceLocator` for loose coupling
- **Event Bus**: Decoupled communication with publish-subscribe pattern
- **Factory Pattern**: Standardized object creation with `ModelFactory` and `WidgetFactory`
- **Plugin System**: Extensibility through `PluginInterface` and `PluginManager`

### Performance & Quality

- **Advanced Logging**: High-performance spdlog-based logging with Qt integration
- **Memory Management**: Smart caching with multiple eviction strategies and memory pressure handling
- **Performance Optimizations**: Asynchronous rendering, preloading, and virtual scrolling
- **Comprehensive Testing**: 100+ unit and integration tests with dedicated test utilities

### Developer Experience

- **Cross-platform Support**: Windows, Linux, macOS with multiple build environments
- **Multiple Build Environments**:
  - **System packages** for native Linux/macOS builds (recommended)
  - **vcpkg** for cross-platform dependency management
  - **MSYS2** for Windows Unix-like development
- **Cross-Compilation**: Toolchains for macOS (Intel & ARM64), Linux (x86_64 & ARM64), Windows (MinGW)
- **Clang Support**: Full Clang compiler support across all desktop platforms
- **IDE Integration**: Automatic clangd configuration for enhanced development experience

## Build System

The project uses **CMake** as the unified build system with simplified configuration presets for different platforms.

The project uses a **tiered dependency management approach** that prioritizes system packages for better performance and reliability, with vcpkg as a cross-platform alternative for consistent dependency versions. See the [Dependency Management Guide](docs/getting-started/dependency-management.md) for detailed information.

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

**Note**: vcpkg builds are slower but provide consistent dependency versions across platforms. Use when system packages are unavailable or when you need identical dependency versions across different platforms.

## Available Build Presets

The simplified CMake configuration provides multiple build presets for different platforms and dependency approaches:

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

### Pre-commit Hooks

The project uses pre-commit hooks to ensure code quality and consistency:

```bash
# Install pre-commit
pip install pre-commit

# Install the hooks
pre-commit install

# Run hooks manually on all files
pre-commit run --all-files
```

The hooks include:

- **Code formatting**: clang-format for C++, black for Python, cmake-format for CMake files
- **Static analysis**: clang-tidy for C++ code quality
- **File hygiene**: trailing whitespace removal, proper line endings, large file detection
- **Documentation**: markdownlint for documentation files

See [Pre-commit Setup Guide](docs/pre-commit-setup.md) for detailed configuration and usage.

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

## Packaging and Distribution

SAST Readium includes a comprehensive packaging system for creating production-ready installers:

### Windows Packaging

Create Windows installers using the integrated CMake CPack system:

```powershell
# Quick start - create MSI installer (MSVC build)
.\scripts\package.ps1

# Create NSIS installer (MSYS2 build)
.\scripts\package.ps1 -PackageType nsis

# Create portable ZIP
.\scripts\package.ps1 -PackageType zip

# Create and verify package
.\scripts\package.ps1 -Verify
```

**Supported Package Types:**

- **MSI Installer** (MSVC builds) - Windows Installer package using WiX Toolset
- **NSIS Installer** (MSYS2 builds) - Executable installer using NSIS
- **Portable ZIP** (both builds) - No-installation-required archive

**Features:**

- Optimized dependency deployment with minimal package size
- Automatic Qt plugin deployment with windeployqt
- Start Menu and Desktop shortcuts
- Uninstaller registration
- Comprehensive verification tools

See the [Windows Packaging Guide](docs/packaging/windows-packaging.md) for detailed instructions.

### Package Verification

```powershell
# Verify package integrity
.\scripts\verify-package.ps1 -InstallPath "C:\Program Files\SAST Readium"

# Analyze DLL dependencies
.\scripts\analyze-dependencies.ps1 -ExePath ".\build\Release-Windows\app\Release\app.exe" -CheckMissing
```

See the [Feature Testing Checklist](docs/packaging/feature-testing-checklist.md) for comprehensive testing procedures.

## Documentation

### Getting Started

- [Documentation Index](docs/index.md) - Main documentation hub
- [MSYS2 Build Guide](docs/setup/msys2-build.md) - Comprehensive MSYS2 setup and build instructions (Recommended)
- [Dependency Management Guide](docs/getting-started/dependency-management.md) - Detailed dependency management
- [Platform Support](docs/getting-started/platform-support.md) - Supported platforms and architectures

### Build System Documentation

- [Build System Comparison](docs/build-systems/build-system-comparison.md) - CMake vs xmake feature comparison
- [CMake Modules Documentation](cmake/README.md) - Detailed CMake module documentation
- [Migration Guide](docs/MIGRATION-GUIDE.md) - Guide for migrating to the new build system
- [clangd Setup Guide](docs/setup/clangd-setup.md) - IDE integration and clangd configuration

### Packaging Documentation

- [Windows Packaging Guide](docs/packaging/windows-packaging.md) - Comprehensive Windows packaging instructions
- [Feature Testing Checklist](docs/packaging/feature-testing-checklist.md) - Post-packaging verification checklist

### Architecture & Features

- [Architecture Guide](docs/architecture.md) - Comprehensive architecture documentation
- [Logging System](docs/logging-system.md) - Logging system documentation and usage
- [Thumbnail System](docs/features/thumbnail-system.md) - Chrome-style thumbnail system
- [Thread Safety Guidelines](docs/thread-safety-guidelines.md) - Thread safety best practices

### Advanced Topics

- [API Reference](docs/api-reference.md) - API documentation for core components
- [PDF Performance Optimizations](docs/PDF_Performance_Optimizations.md) - Performance optimization techniques
- [QGraphics PDF Support](docs/QGraphics_PDF_Support.md) - QGraphics-based PDF rendering

## Dependencies

### Required

- **Qt6** (Core, Gui, Widgets, Svg, LinguistTools, TextToSpeech)
- **Poppler-Qt6** for PDF rendering
- **spdlog** for high-performance logging
- **CMake** 3.28+ and **Ninja** for building

### Optional

- **vcpkg** for cross-platform dependency management
- **MSYS2** for Windows Unix-like development environment
- **Clang** for alternative compiler support

See the [Dependency Management Guide](docs/getting-started/dependency-management.md) for detailed installation instructions.

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for a detailed list of changes, new features, and migration notes.

## Contributing

We welcome contributions! Please see our contributing guidelines and code of conduct in the repository.

## License

MIT License - see [LICENSE](LICENSE) for details.
