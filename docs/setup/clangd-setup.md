# clangd Configuration for Simplified Build System

This document describes the unified clangd configuration system for the simplified SAST Readium build system.

## Overview

The project uses a streamlined clangd configuration system that works with the 6 essential CMake presets. The system now includes **automatic compile database copying** - `compile_commands.json` is automatically copied to the project root after each build, ensuring clangd always has access to the latest compilation information without manual intervention.

For advanced use cases, the unified scripts can still be used to manually configure clangd for specific build directories, providing enhanced IDE features across all platforms.

## Simplified Build Presets

The clangd configuration system works with 6 essential build presets:

- **Debug-Unix** / **Release-Unix**: Linux/macOS with system packages
- **Debug-Windows** / **Release-Windows**: Windows with vcpkg
- **Debug-MSYS2** / **Release-MSYS2**: Windows MSYS2 with system packages

## Automatic Compile Database Copying

**New Feature**: The build system now automatically copies `compile_commands.json` to the project root directory after each successful build. This means:

- **No manual script execution required** for basic clangd functionality
- **Always up-to-date** compile database in the project root
- **Seamless IDE integration** - clangd finds the database automatically
- **Cross-platform compatibility** - works with all build presets

The `.clangd` configuration file is pre-configured to use the root directory compile database (`CompilationDatabase: .`), so clangd will work out-of-the-box after your first build.

## Unified Scripts

### clangd-config.sh (Unix/Linux/macOS)

Cross-platform shell script for Unix-like systems:

- **Path**: `scripts/clangd-config.sh`
- **Platforms**: Linux, macOS, WSL, MSYS2
- **Features**: Auto-detection, manual configuration, preset listing

### clangd-config.ps1 (Windows PowerShell)

PowerShell script for Windows environments:

- **Path**: `scripts/clangd-config.ps1`
- **Platforms**: Windows PowerShell, PowerShell Core
- **Features**: Auto-detection, manual configuration, preset listing

## Features

### Unified Interface

Both scripts provide identical functionality:

- **Auto-detection**: Finds the most recently built configuration
- **Manual selection**: Specify exact build directory
- **Preset listing**: Shows all available build directories
- **Verbose output**: Detailed logging for troubleshooting

### Simplified Configuration

The scripts work with the streamlined preset system:

- **6 essential presets** instead of 20+ complex configurations
- **Clear naming**: Debug/Release × Platform combinations
- **Consistent paths**: Predictable build directory structure

### Cross-Platform Compatibility

- **Unix script**: Works on Linux, macOS, WSL, MSYS2
- **PowerShell script**: Works on Windows PowerShell and PowerShell Core
- **Consistent behavior**: Same functionality across all platforms

## Usage Examples

### Quick Start

The most common usage is auto-detection:

```bash
# Unix/Linux/macOS
./scripts/clangd-config.sh --auto

# Windows PowerShell
.\scripts\clangd-config.ps1 -Auto
```

### Windows PowerShell

```powershell
# Auto-detect best configuration
.\scripts\clangd-config.ps1 -Auto

# Use specific build directory
.\scripts\clangd-config.ps1 -BuildDir "build/Debug-Windows"

# List available configurations
.\scripts\clangd-config.ps1 -List

# Enable verbose output
.\scripts\clangd-config.ps1 -Auto -Verbose

# Show help
.\scripts\clangd-config.ps1 -Help
```

### Unix/Linux/macOS Shell

```bash
# Auto-detect best configuration
./scripts/clangd-config.sh --auto

# Use specific build directory
./scripts/clangd-config.sh build/Debug-Unix

# List available configurations
./scripts/clangd-config.sh --list

# Enable verbose output
./scripts/clangd-config.sh --auto --verbose

# Show help
./scripts/clangd-config.sh --help
```

### Available Build Directories

The scripts work with the simplified preset system:

- `build/Debug-Unix` / `build/Release-Unix`
- `build/Debug-Windows` / `build/Release-Windows`
- `build/Debug-MSYS2` / `build/Release-MSYS2`

## IDE Integration

### VS Code

1. Install the clangd extension
2. Configure your project with a preset:

   ```bash
   cmake --preset Debug-Unix  # or your platform preset
   ```

3. Update clangd configuration:

   ```bash
   ./scripts/clangd-config.sh --auto
   ```

4. Restart VS Code or reload the window

### CLion

1. Open the project in CLion
2. Configure CMake with your preferred preset
3. Run the clangd configuration script:

   ```bash
   ./scripts/clangd-config.sh --auto
   ```

4. CLion will automatically detect the compilation database

### Other IDEs

Any IDE that supports clangd can use the generated `.clangd` configuration:

1. Configure the project: `cmake --preset Debug-Unix`
2. Update clangd: `./scripts/clangd-config.sh --auto`
3. Point your IDE to the project root directory

## Configuration Options

### Force Override

The scripts include a `--force` option to update configuration even when `compile_commands.json` doesn't exist:

**Windows PowerShell:**

```powershell
# Force update even without compile_commands.json
.\scripts\clangd-config.ps1 -Auto -Force
.\scripts\clangd-config.ps1 -BuildDir "build/Debug-Windows" -Force
```

**Unix/Linux/macOS:**

```bash
# Force update even without compile_commands.json
./scripts/clangd-config.sh --auto --force
./scripts/clangd-config.sh build/Debug-Unix --force
```

### Generated Configuration

The scripts generate a comprehensive `.clangd` configuration:

```yaml
# clangd configuration for SAST Readium
CompileFlags:
  Add:
    - -std=c++20
    - -Wall
    - -Wextra
  Remove:
    - -W*
    - -fcoroutines-ts

CompilationDatabase: build/Debug-Unix

Index:
  Background: Build
  StandardLibrary: Yes

InlayHints:
  Enabled: Yes
  ParameterNames: Yes
  DeducedTypes: Yes

Diagnostics:
  ClangTidy:
    Add:
      - readability-*
      - modernize-*
      - performance-*
```

## Troubleshooting

### Common Issues

**clangd not working:**

```bash
# List available configurations
./scripts/clangd-config.sh --list
.\scripts\clangd-config.ps1 -List
```

**No build directories found:**

```bash
# Configure project first
cmake --preset Debug-Unix  # or your platform preset
```

**Wrong build directory:**

```bash
# Auto-detect the correct one
./scripts/clangd-config.sh --auto
.\scripts\clangd-config.ps1 -Auto
```

**Script execution errors on Windows:**

- Ensure PowerShell execution policy allows script execution
- Try running PowerShell as administrator

### C++20 Compatibility

The generated `.clangd` configuration automatically handles C++20 compatibility issues by filtering out unsupported compiler flags. If you encounter warnings about unknown arguments, the configuration should resolve them automatically.

## Benefits of Simplified System

1. **Unified Scripts**: Single script per platform instead of multiple variants
2. **Clear Presets**: 6 essential configurations instead of 20+ complex options
3. **Consistent Interface**: Same functionality across all platforms
4. **Better Maintenance**: Easier to update and maintain
5. **Improved Documentation**: Clear examples and usage patterns

## Migration from Old System

If you're migrating from the previous clangd configuration system:

1. **Remove old scripts**: The new unified scripts replace multiple old scripts
2. **Update commands**: Use `clangd-config.sh` instead of `update-clangd-config.sh`
3. **Use new presets**: Reference the 6 simplified presets instead of old complex names
4. **Update documentation**: Any local documentation should reference the new script names

For detailed migration information, see the [Migration Guide](../MIGRATION-GUIDE.md).
