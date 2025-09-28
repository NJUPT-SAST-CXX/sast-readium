# CMake Modules for SAST Readium

This directory contains modular CMake utilities that provide reusable functions for building the SAST Readium project. The modules are designed to eliminate redundancy, improve maintainability, and follow modern CMake best practices.

## Module Overview

### Dependencies.cmake
Centralized dependency management for the project.

**Key Functions:**
- `find_project_dependencies()` - Find all required dependencies with intelligent caching
- `get_common_libraries()` - Get standard libraries for linking
- `get_test_libraries()` - Get libraries needed for testing

**Features:**
- Automatic vcpkg vs system package detection
- MSYS2 environment handling
- Dependency caching to avoid redundant find_package calls

### CompilerSettings.cmake
Common compiler and language configuration.

**Key Functions:**
- `setup_compiler_settings()` - Configure C++20 standard and compiler flags
- `setup_qt_automation()` - Enable Qt's MOC, UIC, RCC automation
- `configure_build_optimizations()` - Apply build-type specific optimizations
- `print_compiler_info()` - Display compiler configuration summary

**Features:**
- Platform-specific compiler flag handling
- Build type optimizations (Debug/Release)
- Consistent C++ standard enforcement

### PlatformUtils.cmake
Platform detection and configuration utilities.

**Key Functions:**
- `detect_platform_environment()` - Detect OS, compiler, and environment
- `configure_platform_specific_settings()` - Apply platform-specific configurations
- `setup_deployment_tools()` - Configure platform deployment (windeployqt, etc.)
- `print_platform_info()` - Display platform information

**Features:**
- Windows, Linux, macOS support
- MSYS2 environment detection
- Automatic deployment tool setup

### TargetUtils.cmake
Standardized target creation and configuration.

**Key Functions:**
- `setup_target()` - Common target configuration with flexible options
- `setup_executable()` - Executable-specific setup with platform handling
- `setup_library()` - Library creation with type specification
- `add_asset_copying()` - Automated asset management

**Features:**
- Consistent target configuration across the project
- Automatic include directory and library linkage
- Platform-specific executable configuration
- Asset copying with proper dependency tracking

### TestUtils.cmake
Test creation and management utilities.

**Key Functions:**
- `setup_testing_environment()` - Configure testing environment
- `create_test_executable()` - Standardized test creation
- `create_test_library()` - Test-specific library creation
- `add_test_categories()` - Custom targets for test categories
- `setup_test_coverage()` - Coverage reporting configuration

**Features:**
- Unified test configuration combining best practices
- Test categorization (unit, integration, performance)
- Automatic CTest registration
- Coverage reporting support

### ProjectUtils.cmake
Project-wide utilities and configuration.

**Key Functions:**
- `setup_clangd_integration()` - Automatic clangd configuration
- `print_build_summary()` - Comprehensive build information display
- `validate_build_environment()` - Environment and dependency validation
- `setup_project_options()` - Standard project option definitions

**Features:**
- Automatic clangd configuration updates
- Build environment validation
- Comprehensive build summaries
- Standardized project options

## Usage Examples

### Basic Project Setup
```cmake
# In root CMakeLists.txt
cmake_minimum_required(VERSION 3.28)
project(sast-readium LANGUAGES CXX VERSION 0.1.0.0)

# Add cmake modules to path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Include required modules
include(ProjectUtils)
include(CompilerSettings)
include(Dependencies)
include(PlatformUtils)

# Setup project
setup_project_options()
setup_compiler_settings()
setup_qt_automation()
detect_platform_environment()
find_project_dependencies()
validate_build_environment()

# Print summary
print_build_summary()
```

### Creating an Executable
```cmake
# In app/CMakeLists.txt
include(TargetUtils)

# Create and configure executable
setup_executable(app
    SOURCES ${APP_SOURCES}
    WIN32  # Windows GUI app
)

# Add asset copying
add_asset_copying(app ${CMAKE_SOURCE_DIR}/assets/styles)
```

### Creating Tests
```cmake
# In tests/CMakeLists.txt
include(TestUtils)

# Setup testing environment
setup_testing_environment()

# Create test executable
create_test_executable(test_search_engine
    SOURCES test_search_engine.cpp
    CATEGORY unit
    TIMEOUT 60
)

# Add test categories
add_test_categories()
```

## Migration Guide

When migrating from the old CMake structure:

1. **Update root CMakeLists.txt:**
   - Add cmake directory to CMAKE_MODULE_PATH
   - Replace inline configurations with module function calls
   - Remove redundant dependency finding

2. **Update app/CMakeLists.txt:**
   - Use `setup_executable()` instead of manual configuration
   - Replace asset copying with `add_asset_copying()`

3. **Update test CMakeLists.txt:**
   - Use `create_test_executable()` for consistent test creation
   - Remove redundant dependency finding
   - Use test category functions

4. **Clean up:**
   - Remove obsolete CMakeLists_new.txt files
   - Remove redundant code and configurations

## Best Practices

1. **Always include required modules** at the top of CMakeLists.txt files
2. **Use function parameters** instead of global variables when possible
3. **Call validation functions** to catch configuration errors early
4. **Use consistent naming** for targets and variables
5. **Document custom functions** using the RST format shown in modules

## Troubleshooting

### Common Issues

**"Function not found" errors:**
- Ensure the module is included with `include(ModuleName)`
- Check that CMAKE_MODULE_PATH includes the cmake directory

**Dependency not found:**
- Call `find_project_dependencies()` before using dependency targets
- Check that required packages are installed

**Platform-specific issues:**
- Call `detect_platform_environment()` early in configuration
- Use platform-specific functions when needed

### Debug Information

Use these functions to get detailed information:
- `print_build_summary()` - Overall build configuration
- `print_compiler_info()` - Compiler details
- `print_platform_info()` - Platform information

### SourceUtils.cmake
Automatic source file discovery and validation utilities.

**Key Functions:**
- `discover_app_sources()` - Automatically discover application source files
- `discover_component_sources()` - Discover sources for specific components
- `validate_discovered_sources()` - Validate that required sources were found
- `print_source_summary()` - Display discovered source file summary

**Features:**
- Automatic source file discovery using GLOB_RECURSE
- Component-based source organization
- Test file exclusion with regex patterns
- Source validation with required files and patterns
- Duplicate source detection and removal

### ComponentUtils.cmake
Component-based library creation and management utilities.

**Key Functions:**
- `create_component_library()` - Create a library from specific components
- `create_standard_app_components()` - Create standard component libraries
- `get_component_libraries()` - Get list of available component libraries
- `print_component_summary()` - Display component library summary

**Features:**
- Automatic component library creation from source discovery
- Shared libraries between application and tests
- Eliminates source duplication
- Component-based dependency management
- Standardized library naming and organization

## Cross-Platform Toolchains

The project now includes comprehensive cross-platform toolchain support for building on multiple target platforms. Each toolchain is designed to work with the existing CMake module system and dependency management.

### Available Toolchains

#### Linux Targets
- **`aarch64-linux-gnu.cmake`** - Linux ARM64 cross-compilation
- **`x86_64-linux-gnu.cmake`** - Explicit Linux x64 targeting

#### macOS Targets
- **`x86_64-apple-darwin.cmake`** - macOS Intel (x86_64)
- **`arm64-apple-darwin.cmake`** - macOS Apple Silicon (ARM64)

#### Windows Targets
- **`x86_64-w64-mingw32.cmake`** - Windows MinGW cross-compilation
- **`x86_64-w64-mingw64-msys2.cmake`** - MSYS2 MinGW-w64 with automatic detection

### Prerequisites by Platform

#### macOS Development
- Xcode Command Line Tools: `xcode-select --install`
- macOS SDK (automatically detected via `xcrun`)

#### Windows MinGW Cross-Compilation
- MinGW-w64 toolchain: `apt install gcc-mingw-w64-x86-64` (Ubuntu/Debian)
- vcpkg with MinGW triplet support

#### MSYS2 Development
- MSYS2 installation from https://www.msys2.org/
- MinGW-w64 toolchain: `pacman -S mingw-w64-x86_64-toolchain`
- Optional: pkg-config for dependency discovery: `pacman -S mingw-w64-x86_64-pkg-config`
- Optional: vcpkg for dependency management

#### Linux ARM64 Cross-Compilation
- Cross-compilation toolchain: `apt install gcc-aarch64-linux-gnu` (Ubuntu/Debian)

### CMake Presets Integration

All toolchains are integrated with CMakePresets.json for easy configuration:

```bash
# List available presets
cmake --list-presets

# Configure for specific platform
cmake --preset Release-macOS-ARM64
cmake --preset Debug-Linux-ARM64

# Build
cmake --build --preset Release-macOS-ARM64
```

### vcpkg Integration

Platforms with vcpkg support use appropriate target triplets:
- **macOS Intel**: `x64-osx`
- **macOS Apple Silicon**: `arm64-osx`
- **Windows ARM64**: `arm64-windows`
- **Windows MinGW**: `x64-mingw-dynamic`

Linux uses system packages or custom dependency management.

### Environment Variables

#### Optional for macOS
```bash
export MACOS_SDK_PATH=/path/to/macos/sdk
export QT_HOST_PATH=/path/to/qt/installation
```

#### Optional for MSYS2
```bash
# Explicit MSYS2 installation path (if not in default location)
export MSYS2_ROOT=C:\msys64

# For vcpkg integration
export VCPKG_ROOT=C:\path\to\vcpkg
```

### Cross-Compilation Examples

#### Building for macOS from Linux
```bash
# Requires macOS SDK and appropriate Qt6 installation
cmake --preset Release-macOS-ARM64
cmake --build --preset Release-macOS-ARM64
```

#### Building with MSYS2
```bash
# Using explicit MSYS2 toolchain with system packages
cmake --preset Release-MSYS2-Toolchain
cmake --build --preset Release-MSYS2-Toolchain

# Using explicit MSYS2 toolchain with vcpkg
export VCPKG_ROOT=C:\path\to\vcpkg
cmake --preset Release-MSYS2-Toolchain-vcpkg
cmake --build --preset Release-MSYS2-Toolchain-vcpkg

# Alternative: Using existing MSYS2 presets (environment-based)
cmake --preset Release-MSYS2
cmake --build --preset Release-MSYS2
```

### Troubleshooting Cross-Compilation

#### Common Issues

**Toolchain not found:**
- Ensure cross-compilation tools are installed
- Check PATH includes toolchain binaries
- Verify environment variables are set correctly

**SDK/NDK not found:**
- Set appropriate environment variables
- Ensure SDK paths are accessible
- Check SDK version compatibility

**Qt6 cross-compilation issues:**
- Set `QT_HOST_PATH` to host Qt installation
- Ensure target Qt libraries are available
- Check Qt6 cross-compilation documentation

**vcpkg triplet not found:**
- Install vcpkg with required triplets
- Set `VCPKG_ROOT` environment variable
- Run `vcpkg install` with appropriate triplet

**MSYS2 toolchain issues:**
- Ensure MSYS2 is installed from https://www.msys2.org/
- Install MinGW-w64 toolchain: `pacman -S mingw-w64-x86_64-toolchain`
- Set `MSYS2_ROOT` environment variable if not in default location
- Check that `msys2_shell.cmd` exists in MSYS2 installation directory
- Verify compilers exist: `gcc.exe`, `g++.exe`, `windres.exe` in `mingw64/bin/`

## Clang Compiler Support

The build system now includes comprehensive Clang compiler support across all desktop platforms (Windows, Linux, macOS). Clang integration provides modern C++20 features, excellent diagnostics, and cross-platform compatibility.

### Clang Toolchains

#### Windows Clang
- **`clang-windows.cmake`** - Windows Clang compiler support
  - Supports both standard Clang and Clang-cl (MSVC compatible mode)
  - Automatic mode detection and configuration
  - vcpkg integration with appropriate triplets

#### Linux Clang
- **`clang-linux.cmake`** - Linux Clang compiler support
  - Native and cross-compilation support
  - Security hardening flags enabled
  - Optimized for x86_64 architecture

#### macOS Clang
- **`clang-macos.cmake`** - macOS Clang compiler support
  - Universal binary support (Intel + Apple Silicon)
  - Automatic SDK detection
  - Framework integration

### Clang Prerequisites

#### Windows
```bash
# Install Clang via LLVM
# Download from https://releases.llvm.org/
# Or via package manager:
winget install LLVM.LLVM

# For Clang-cl mode (MSVC compatible):
# Requires Visual Studio Build Tools or Visual Studio
```

#### Linux
```bash
# Ubuntu/Debian
sudo apt install clang clang++ libc++-dev libc++abi-dev

# CentOS/RHEL/Fedora
sudo dnf install clang clang++ libcxx-devel libcxxabi-devel

# Arch Linux
sudo pacman -S clang libc++ libc++abi
```

#### macOS
```bash
# Clang is included with Xcode Command Line Tools
xcode-select --install

# Or install via Homebrew for latest version
brew install llvm
```

### Clang CMake Presets

All Clang configurations are available through CMakePresets.json:

```bash
# List all available presets (including Clang)
cmake --list-presets

# Windows Clang builds
cmake --preset Debug-Clang-Windows
cmake --preset Release-Clang-Windows

# Linux Clang builds
cmake --preset Debug-Clang-Linux
cmake --preset Release-Clang-Linux

# macOS Clang builds
cmake --preset Debug-Clang-macOS
cmake --preset Release-Clang-macOS

# Build with Clang
cmake --build --preset Release-Clang-Windows
```

### Clang Features

#### Advanced Compiler Detection
- Automatic Clang version detection and validation
- Minimum version requirements (Clang 12.0+)
- Clang variant detection (Standard, Apple, MSVC)
- Target triple detection for cross-compilation

#### C++20 Feature Support
- Concepts support (Clang 10+)
- Ranges support (Clang 13+)
- Coroutines support (Clang 14+)
- Modules support (Clang 15+, experimental)

#### Build Optimizations
- **Debug**: `-O0 -g` with frame pointers preserved
- **Release**: `-O3 -DNDEBUG` with Thin LTO enabled
- **RelWithDebInfo**: `-O2 -g` with minimal debug info
- **MinSizeRel**: `-Os` with dead code elimination

#### Platform-Specific Features
- **Windows**: MSVC compatibility mode support
- **Linux**: Security hardening (stack protector, RELRO)
- **macOS**: Objective-C++ support, framework integration

### Clang Environment Variables

#### Optional Configuration
```bash
# Force Clang-cl mode on Windows
export CLANG_CL_MODE=1

# Disable native CPU optimizations
export CLANG_NO_NATIVE_ARCH=1

# Disable color diagnostics
export NO_COLOR=1

# Qt6 host path (for cross-compilation)
export QT_HOST_PATH=/path/to/qt6

# vcpkg root (for dependency management)
export VCPKG_ROOT=/path/to/vcpkg
```

### Clang Troubleshooting

#### Common Issues

**Clang not found:**
- Ensure Clang is installed and in PATH
- Check `clang --version` and `clang++ --version`
- On Windows, verify both `clang.exe` and `clang++.exe` exist

**C++20 features not working:**
- Verify Clang version meets minimum requirements
- Check that `-std=c++20` is being used
- Some features require newer Clang versions

**vcpkg integration issues:**
- Ensure appropriate vcpkg triplet is selected
- For Windows: Use `x64-mingw-dynamic` for standard Clang
- For Windows: Use `x64-windows` for Clang-cl mode

**Qt6 compilation errors:**
- Verify Qt6 is compatible with your Clang version
- Set `QT_HOST_PATH` for cross-compilation scenarios
- Check that Qt6 was built with compatible compiler

**Link errors:**
- Ensure consistent standard library usage
- On Linux: Use `libstdc++` (default) or `libc++`
- On macOS: Use `libc++` (default)
- On Windows: Library compatibility depends on Clang mode

## Contributing

When adding new functionality:

1. **Follow the existing pattern** of documented functions with RST comments
2. **Add validation** and error checking to functions
3. **Use cmake_parse_arguments()** for complex function parameters
4. **Test on multiple platforms** when possible
5. **Update this README** with new functions and usage examples
6. **Add corresponding CMakePresets.json entries** for new toolchains
7. **Document platform-specific requirements** and setup instructions
