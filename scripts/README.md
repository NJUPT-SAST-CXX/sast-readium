# Scripts Directory

This directory contains essential utility scripts for the SAST Readium project with simplified build system.

## Overview

The scripts have been streamlined to support the simplified CMake build system with 6 essential presets:

- **Debug-Unix** / **Release-Unix**: Linux/macOS with system packages
- **Debug-Windows** / **Release-Windows**: Windows with vcpkg
- **Debug-MSYS2** / **Release-MSYS2**: Windows MSYS2 with system packages

## clangd Configuration Scripts

Cross-platform scripts to automatically update the `.clangd` configuration file for the simplified preset system.

### clangd-config.sh (Unix/Linux/macOS)

**Usage:**

```bash
# Auto-detect and use the most recent build directory
./scripts/clangd-config.sh --auto

# Use a specific build directory
./scripts/clangd-config.sh build/Debug-Unix

# List all available build directories
./scripts/clangd-config.sh --list

# Enable verbose output
./scripts/clangd-config.sh --auto --verbose
```

### clangd-config.ps1 (Windows PowerShell)

**Usage:**

```powershell
# Auto-detect and use the most recent build directory
.\scripts\clangd-config.ps1 -Auto

# Use a specific build directory
.\scripts\clangd-config.ps1 -BuildDir "build/Debug-Windows"

# List all available build directories
.\scripts\clangd-config.ps1 -List

# Enable verbose output
.\scripts\clangd-config.ps1 -Auto -Verbose
```

## Build Configuration Scripts

### setup-build-environment.ps1 (Universal vcpkg Configuration) ⭐ RECOMMENDED

**NEW**: Universal, intelligent build environment configuration script that automatically detects available compilers and configures the optimal build environment for vcpkg-based builds on Windows.

**Features:**

- Automatic compiler detection (MSVC, MinGW-w64, Clang)
- Intelligent PATH configuration
- Automatic vcpkg triplet selection
- Support for both Debug and Release builds
- Clean build option
- Verbose output for debugging

**Usage:**

```powershell
# Auto-detect compiler and configure (recommended)
.\scripts\setup-build-environment.ps1

# Force specific compiler
.\scripts\setup-build-environment.ps1 -Compiler msvc
.\scripts\setup-build-environment.ps1 -Compiler mingw

# Release build with clean
.\scripts\setup-build-environment.ps1 -BuildType Release -CleanBuild

# Override vcpkg location
.\scripts\setup-build-environment.ps1 -VcpkgRoot "C:\vcpkg"

# Verbose output for debugging
.\scripts\setup-build-environment.ps1 -Verbose

# Use specific preset
.\scripts\setup-build-environment.ps1 -Preset Debug-Windows
```

**Compiler Priority:**

1. MSVC (Visual Studio) - Recommended for Windows
2. MinGW-w64 (MSYS2) - Alternative for GCC-based builds
3. Clang - Alternative for LLVM-based builds

### configure-vcpkg-msvc.ps1 (Legacy - MSVC Only)

Legacy script for configuring MSVC builds with vcpkg. **Recommended to use `setup-build-environment.ps1` instead.**

**Usage:**

```powershell
.\scripts\configure-vcpkg-msvc.ps1
```

### configure-vcpkg-mingw.ps1 (Legacy - MinGW Only)

Legacy script for configuring MinGW builds with vcpkg. **Recommended to use `setup-build-environment.ps1` instead.**

**Note:** MinGW with vcpkg may have compatibility issues with Qt6 6.8.3. Consider using MSYS2 without vcpkg instead.

**Usage:**

```powershell
.\scripts\configure-vcpkg-mingw.ps1
```

### build-msys2.sh

Comprehensive build script for MSYS2 environment with support for:

- Debug and Release builds
- Dependency installation
- Clean builds
- Parallel compilation

**Usage:**

```bash
# Build release version
./scripts/build-msys2.sh

# Build debug version with dependency installation
./scripts/build-msys2.sh --type Debug --install-deps

# Clean build with verbose output
./scripts/build-msys2.sh --clean --verbose
```

### check-msys2-deps.sh

Utility script to verify MSYS2 dependencies are properly installed.

**Usage:**

```bash
./scripts/check-msys2-deps.sh
```

## Testing Scripts

### run_qgraphics_tests.py

Comprehensive Python test runner for QGraphics PDF support testing.

**Usage:**

```bash
# Run all tests
python scripts/run_qgraphics_tests.py

# Run with specific configuration
python scripts/run_qgraphics_tests.py --config Debug-Unix
```

### run_tests.ps1

PowerShell test runner for Windows environments.

**Usage:**

```powershell
# Run all tests
.\scripts\run_tests.ps1

# Run specific test suite
.\scripts\run_tests.ps1 -TestSuite Unit
```

## Packaging Scripts

### package.sh / package.ps1

Cross-platform packaging scripts for creating distribution packages.

**Usage:**

```bash
# Unix/Linux/macOS
./scripts/package.sh

# Windows PowerShell
.\scripts\package.ps1
```

## Quick Start

### Windows with vcpkg (Recommended)

1. **Configure build environment (automatic compiler detection):**

   ```powershell
   .\scripts\setup-build-environment.ps1
   ```

2. **Build the project:**

   ```powershell
   cmake --build build\Debug-Windows --config Debug
   ```

3. **Update clangd configuration:**

   ```powershell
   .\scripts\clangd-config.ps1 -Auto
   ```

### Windows MSYS2 (System Packages)

1. **Configure your project:**

   ```bash
   cmake --preset Debug-MSYS2
   ```

2. **Build the project:**

   ```bash
   cmake --build build/Debug-MSYS2
   ```

3. **Update clangd configuration:**

   ```bash
   ./scripts/clangd-config.sh --auto
   ```

### Linux/macOS (System Packages)

1. **Configure your project:**

   ```bash
   cmake --preset Debug-Unix
   ```

2. **Build the project:**

   ```bash
   cmake --build build/Debug
   ```

3. **Update clangd configuration:**

   ```bash
   ./scripts/clangd-config.sh --auto
   ```

### Running Tests

```bash
# Comprehensive testing (all platforms)
python scripts/run_qgraphics_tests.py

# Windows-specific tests
.\scripts\run_tests.ps1
```

## Simplified Architecture

The scripts have been streamlined as part of the build system simplification:

- **Reduced complexity**: Removed redundant platform-specific variants
- **Unified approach**: Single scripts with cross-platform support where possible
- **Essential functionality**: Kept only the most useful and frequently used scripts
- **Clear documentation**: Updated examples for the simplified preset system

For more information, see the [Build System Documentation](../docs/build-systems/).
