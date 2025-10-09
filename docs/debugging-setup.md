# Cross-Platform Debugging Setup for SAST Readium

This document describes the comprehensive cross-platform debugging setup for the SAST Readium C++ Qt application using Visual Studio Code.

## Overview

The debugging configuration supports multiple platforms and debuggers:

- **Windows**: MSVC debugger, MinGW-w64 GDB, LLDB
- **Linux**: GDB, LLDB
- **macOS**: LLDB (primary), GDB (if available)

## Prerequisites

### Windows

#### Option 1: Visual Studio (MSVC)
- Visual Studio 2019/2022 with C++ development tools
- Qt6 development libraries (MSVC build)
- CMake 3.16+

#### Option 2: MinGW-w64 (MSYS2)
- MSYS2 with MinGW-w64 toolchain
- Qt6 development libraries (MinGW build)
- GDB debugger: `pacman -S mingw-w64-x86_64-gdb`
- CMake and Ninja: `pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja`

#### Option 3: LLDB (Optional)
- LLVM/Clang toolchain with LLDB
- Can be installed via MSYS2: `pacman -S mingw-w64-x86_64-lldb`

### Linux (Ubuntu/Debian)

```bash
# Essential build tools
sudo apt update
sudo apt install build-essential cmake ninja-build

# Qt6 development
sudo apt install qt6-base-dev qt6-svg-dev qt6-tools-dev

# Debuggers
sudo apt install gdb lldb

# Additional Qt6 modules (if needed)
sudo apt install qt6-multimedia-dev qt6-concurrent-dev
```

### Linux (Fedora/RHEL)

```bash
# Essential build tools
sudo dnf install gcc-c++ cmake ninja-build

# Qt6 development
sudo dnf install qt6-qtbase-devel qt6-qtsvg-devel qt6-qttools-devel

# Debuggers
sudo dnf install gdb lldb
```

### macOS

```bash
# Install Xcode command line tools
xcode-select --install

# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake ninja qt6

# LLDB is included with Xcode
# Optional: Install GDB (may require code signing)
brew install gdb
```

## Environment Variables

Set these environment variables for better cross-platform support:

### Windows (MSYS2)
```bash
export MSYS2_ROOT="C:/msys64"  # or your MSYS2 installation path
```

### All Platforms
```bash
export Qt6_DIR="/path/to/qt6/lib/cmake/Qt6"  # Adjust to your Qt6 installation
```

## VSCode Extensions

Install these essential extensions:

1. **C/C++** (ms-vscode.cpptools) - Core C++ support
2. **C/C++ Extension Pack** (ms-vscode.cpptools-extension-pack) - Complete C++ tooling
3. **CMake Tools** (ms-vscode.cmake-tools) - CMake integration
4. **CodeLLDB** (vadimcn.vscode-lldb) - Enhanced LLDB support

## Configuration Files

The setup includes three main configuration files:

### 1. `.vscode/launch.json`
Contains debugging configurations for all platforms:

- **(Windows) Debug with MSVC** - Uses Visual Studio debugger
- **(Windows) Debug with MinGW-w64 GDB** - Uses GDB with MinGW
- **(Windows) Debug with LLDB** - Uses LLDB on Windows
- **(Linux) Debug with GDB** - Standard Linux GDB debugging
- **(Linux) Debug with LLDB** - LLDB debugging on Linux
- **(macOS) Debug with LLDB** - Primary macOS debugger
- **(macOS) Debug with GDB** - Alternative macOS debugger
- **Cross-platform utility configurations** - Flexible debugging options

### 2. `.vscode/tasks.json`
Build tasks for all platforms with debug symbol generation:

- **Configure/Build Debug (Windows MSVC)** - MSVC with `/Zi` debug symbols
- **Configure/Build Debug (Windows MinGW)** - MinGW with `-g` debug symbols
- **Configure/Build Debug (Linux)** - GCC/Clang with `-g` debug symbols
- **Configure/Build Debug (macOS)** - Clang with `-g` debug symbols

### 3. `.vscode/c_cpp_properties.json`
IntelliSense configurations for each platform with proper include paths and compiler settings.

## Usage Instructions

### Quick Start

1. **Select Configuration**: Choose the appropriate configuration from the debug dropdown
2. **Set Breakpoints**: Click in the gutter next to line numbers
3. **Start Debugging**: Press F5 or click the debug button
4. **Debug Controls**: Use F10 (step over), F11 (step into), Shift+F11 (step out)

### Platform-Specific Setup

#### Windows with MSVC
1. Open project in VSCode
2. Select "Windows-MSVC" configuration in C/C++ properties
3. Use "(Windows) Debug with MSVC" launch configuration
4. Ensure Visual Studio is installed and cl.exe is in PATH

#### Windows with MinGW
1. Set MSYS2_ROOT environment variable
2. Update MinGW GDB path when prompted
3. Select "Windows-MinGW" configuration in C/C++ properties
4. Use "(Windows) Debug with MinGW-w64 GDB" launch configuration

#### Linux
1. Install required packages (see Prerequisites)
2. Select "Linux" configuration in C/C++ properties
3. Use "(Linux) Debug with GDB" or "(Linux) Debug with LLDB"
4. Build directory will be `build/Debug-Linux`

#### macOS
1. Install Xcode and Homebrew dependencies
2. Select "macOS" configuration in C/C++ properties
3. Use "(macOS) Debug with LLDB" (recommended) or "(macOS) Debug with GDB"
4. Build directory will be `build/Debug-macOS`

## Debugging Features

### Breakpoints
- **Line breakpoints**: Click in gutter or press F9
- **Conditional breakpoints**: Right-click breakpoint, add condition
- **Function breakpoints**: Debug console: `break function_name`

### Variable Inspection
- **Variables panel**: Shows local variables and their values
- **Watch expressions**: Add custom expressions to monitor
- **Hover inspection**: Hover over variables in code
- **Qt visualizers**: Custom display for Qt types (QString, QList, etc.)

### Advanced Features
- **Call stack navigation**: See function call hierarchy
- **Memory inspection**: View raw memory contents
- **Disassembly view**: See generated assembly code
- **Multi-threaded debugging**: Debug Qt application threads

## Troubleshooting

### Common Issues

#### "Debugger not found"
- Ensure debugger is installed and in PATH
- Update debugger path in launch configuration
- For MinGW: Set correct path in input prompt

#### "Debug symbols not found"
- Verify debug build configuration
- Check that `-g` (GCC/Clang) or `/Zi` (MSVC) flags are used
- Rebuild with debug configuration

#### "Qt types not displaying properly"
- Ensure qt.natvis file is present in .vscode folder
- For GDB: Install Qt pretty printers
- Check visualizerFile setting in launch configuration

#### "Build task failed"
- Verify CMake and build tools are installed
- Check Qt6 installation and paths
- Review CMakeLists.txt for missing dependencies

### Platform-Specific Issues

#### Windows
- **MSVC not found**: Install Visual Studio with C++ tools
- **MinGW path issues**: Use forward slashes in paths
- **Permission errors**: Run VSCode as administrator if needed

#### Linux
- **GDB permission denied**: May need to adjust ptrace_scope
- **Qt not found**: Install qt6-base-dev package
- **Missing libraries**: Check ldd output for missing dependencies

#### macOS
- **Code signing required**: GDB may need code signing on macOS
- **Homebrew paths**: Ensure /opt/homebrew/bin is in PATH
- **Xcode license**: Accept Xcode license agreement

## Best Practices

1. **Use appropriate debugger**: LLDB for macOS, GDB for Linux, either for Windows
2. **Enable all warnings**: Use `-Wall -Wextra` for better code quality
3. **Debug builds only**: Always use Debug configuration for debugging
4. **Clean builds**: Use clean tasks when switching configurations
5. **Version control**: Don't commit build directories or user-specific paths

## Additional Resources

- [VSCode C++ Debugging](https://code.visualstudio.com/docs/cpp/cpp-debug)
- [CMake Tools Extension](https://github.com/microsoft/vscode-cmake-tools)
- [Qt Creator vs VSCode](https://doc.qt.io/qt-6/debug.html)
- [GDB Documentation](https://www.gnu.org/software/gdb/documentation/)
- [LLDB Tutorial](https://lldb.llvm.org/use/tutorial.html)
