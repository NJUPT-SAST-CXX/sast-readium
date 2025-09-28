# Setup & Configuration

This section provides comprehensive guides for setting up your development environment with the simplified CMake build system.

## Overview

The SAST Readium project now uses a streamlined CMake build system with 6 essential presets covering all major development scenarios. This section covers platform-specific setup and IDE configuration for the simplified build system.

## Documentation in this Section

### [MSYS2 Build Setup](msys2-build.md)

Comprehensive guide for setting up SAST Readium development on Windows using MSYS2 with the simplified build system:

- MSYS2 installation and configuration
- Dependency installation via pacman
- Simplified CMake preset usage (Debug-MSYS2/Release-MSYS2)
- Build system configuration
- Troubleshooting common issues
- Performance optimization tips

### [clangd IDE Setup](clangd-setup.md)

Cross-platform clangd configuration for the simplified build system:

- Unified clangd configuration scripts
- Automatic build directory detection for 6 essential presets
- Cross-platform script usage (clangd-config.sh/clangd-config.ps1)
- IDE integration (VS Code, CLion, etc.)
- Troubleshooting clangd issues

## Simplified Build System

The project now uses 6 essential CMake presets that cover all major development scenarios:

### Available Presets

- **Debug-Unix** / **Release-Unix**: Linux/macOS with system packages
- **Debug-Windows** / **Release-Windows**: Windows with vcpkg dependencies
- **Debug-MSYS2** / **Release-MSYS2**: Windows MSYS2 with system packages

### Quick Start

1. **List available presets:**
   ```bash
   cmake --list-presets=configure
   ```

2. **Configure for your platform:**
   ```bash
   cmake --preset Debug-Unix        # Linux/macOS
   cmake --preset Debug-Windows     # Windows with vcpkg
   cmake --preset Debug-MSYS2       # Windows MSYS2
   ```

3. **Build:**
   ```bash
   cmake --build --preset Debug-Unix  # Use your chosen preset
   ```

4. **Configure clangd:**
   ```bash
   ./scripts/clangd-config.sh --auto              # Unix/Linux/macOS
   .\scripts\clangd-config.ps1 -Auto              # Windows PowerShell
   ```

## Platform-Specific Recommendations

### Windows

- **Recommended**: MSYS2 with `Debug-MSYS2`/`Release-MSYS2` presets
- **Alternative**: Native Windows with `Debug-Windows`/`Release-Windows` presets
- **IDE**: VS Code with clangd extension
- **Script**: Use `clangd-config.ps1` for configuration

### Linux

- **Package Manager**: System packages (apt, dnf, etc.)
- **Presets**: `Debug-Unix`/`Release-Unix`
- **IDE**: Any editor with clangd support
- **Script**: Use `clangd-config.sh` for configuration

### macOS

- **Package Manager**: Homebrew for dependencies
- **Presets**: `Debug-Unix`/`Release-Unix`
- **IDE**: Xcode or VS Code with clangd
- **Script**: Use `clangd-config.sh` for configuration

## Quick Setup Guide

1. **Choose your platform preset**:

   - **Windows MSYS2**: Follow [MSYS2 Build Setup](msys2-build.md) for `Debug-MSYS2`/`Release-MSYS2`
   - **Windows vcpkg**: Use `Debug-Windows`/`Release-Windows` presets
   - **Linux/macOS**: Use `Debug-Unix`/`Release-Unix` presets

2. **Configure IDE integration**:

   - Follow [clangd IDE Setup](clangd-setup.md) for the simplified configuration system

3. **Verify setup**:
   - Configure: `cmake --preset Debug-Unix` (or your platform preset)
   - Build: `cmake --build --preset Debug-Unix`
   - Configure clangd: `./scripts/clangd-config.sh --auto`

## Migration from Old System

If you're migrating from the previous complex build system, see the [Migration Guide](../MIGRATION-GUIDE.md) for:

- Preset mapping from old to new system
- Updated script usage
- Troubleshooting common migration issues

## Next Steps

After completing the setup:

- Explore the [Features](../features/) documentation to understand the codebase
- Review the [Build System](../build-systems/) documentation for the simplified configuration
- Start developing with the streamlined build system and enhanced IDE support
