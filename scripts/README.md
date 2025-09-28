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

## Build Scripts

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

1. **Configure your project with simplified presets:**

   ```bash
   # List available presets
   cmake --list-presets=configure

   # Configure for your platform
   cmake --preset Debug-Unix        # Linux/macOS
   cmake --preset Debug-Windows     # Windows with vcpkg
   cmake --preset Debug-MSYS2       # Windows MSYS2
   ```

2. **Update clangd configuration:**

   ```bash
   # Auto-detect best configuration
   ./scripts/clangd-config.sh --auto              # Unix/Linux/macOS
   .\scripts\clangd-config.ps1 -Auto              # Windows PowerShell
   ```

3. **Build the project:**

   ```bash
   cmake --build --preset Debug-Unix              # Or your chosen preset
   ```

4. **Run tests:**

   ```bash
   python scripts/run_qgraphics_tests.py          # Comprehensive testing
   .\scripts\run_tests.ps1                        # Windows-specific tests
   ```

## Simplified Architecture

The scripts have been streamlined as part of the build system simplification:

- **Reduced complexity**: Removed redundant platform-specific variants
- **Unified approach**: Single scripts with cross-platform support where possible
- **Essential functionality**: Kept only the most useful and frequently used scripts
- **Clear documentation**: Updated examples for the simplified preset system

For more information, see the [Build System Documentation](../docs/build-systems/).
