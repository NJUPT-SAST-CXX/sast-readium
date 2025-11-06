# Windows Packaging Guide for SAST Readium

This guide explains how to create production-ready Windows installers for SAST Readium using the integrated CMake CPack packaging system.

## Table of Contents

- [Overview](#overview)
- [Prerequisites](#prerequisites)
- [Build Environments](#build-environments)
- [Quick Start](#quick-start)
- [Package Types](#package-types)
- [Package Size Optimization](#package-size-optimization)
- [Detailed Instructions](#detailed-instructions)
- [Customization](#customization)
- [Verification](#verification)
- [Troubleshooting](#troubleshooting)
- [Best Practices](#best-practices)

## Overview

SAST Readium supports three types of Windows packages:

1. **MSI Installer** (MSVC builds) - Windows Installer package using WiX Toolset
2. **NSIS Installer** (MSYS2 builds) - Executable installer using NSIS
3. **Portable ZIP** (both builds) - No-installation-required archive

The packaging system is integrated with CMake and uses CPack for automated package generation.

## Prerequisites

### Common Requirements

- **CMake 3.28+** - Build system generator
- **Git** - Version control (for cloning the repository)
- **Windows 10/11** - Target platform

### For MSVC Builds (MSI Packages)

- **Visual Studio 2022** - C++ compiler and build tools
- **vcpkg** - Dependency manager (automatically configured)
- **WiX Toolset 3.x** - MSI installer creation
  - Download from: <https://wixtoolset.org/>
  - Add to PATH: `C:\Program Files (x86)\WiX Toolset v3.x\bin`

### For MSYS2 Builds (NSIS Packages)

- **MSYS2** - Unix-like environment for Windows
  - Download from: <https://www.msys2.org/>
  - Install MINGW64 toolchain and dependencies
- **NSIS 3.x** - Executable installer creation
  - Download from: <https://nsis.sourceforge.io/>
  - Add to PATH: `C:\Program Files (x86)\NSIS`

## Build Environments

### MSVC Environment (Recommended for MSI)

```powershell
# Configure build
cmake --preset=Release-Windows

# Build application
cmake --build --preset=Release-Windows

# Create MSI package
cd build\Release-Windows
cpack -G WIX -C Release
```

### MSYS2 Environment (Recommended for NSIS)

```bash
# In MSYS2 MINGW64 shell
./scripts/build-msys2.sh -t Release

# Create NSIS installer
cd build/Release-MSYS2
cpack -G NSIS -C Release
```

## Quick Start

### Using PowerShell Script (Easiest)

```powershell
# Create MSI installer (MSVC build)
.\scripts\package.ps1

# Create NSIS installer (MSYS2 build)
.\scripts\package.ps1 -PackageType nsis

# Create portable ZIP
.\scripts\package.ps1 -PackageType zip

# Create all packages
.\scripts\package.ps1 -PackageType all

# Create and verify package
.\scripts\package.ps1 -Verify
```

### Using CMake CPack Directly

```powershell
# Navigate to build directory
cd build\Release-Windows  # or build\Release-MSYS2

# Create MSI (MSVC only)
cpack -G WIX -C Release -B ../../packaging

# Create NSIS (MSYS2 only)
cpack -G NSIS -C Release -B ../../packaging

# Create ZIP (both)
cpack -G ZIP -C Release -B ../../packaging
```

## Package Types

### MSI Installer (Windows Installer)

**Advantages:**

- Native Windows installation experience
- Integrates with Add/Remove Programs
- Supports upgrades and patches
- Can be deployed via Group Policy
- Digital signature support

**Requirements:**

- MSVC build (Release-Windows preset)
- WiX Toolset 3.x installed

**Features:**

- Start Menu shortcuts
- Desktop shortcut (optional)
- Uninstaller registration
- Upgrade GUID for version management
- Custom installation directory

**File:** `sast-readium-<version>-win64.msi`

### NSIS Installer (Nullsoft Scriptable Install System)

**Advantages:**

- Smaller installer size
- Highly customizable
- Fast installation
- Modern UI

**Requirements:**

- MSYS2 build (Release-MSYS2 preset)
- NSIS 3.x installed

**Features:**

- Start Menu shortcuts
- Desktop shortcut
- Uninstaller
- PDF file association (optional)
- Finish page with "Run application" option

**File:** `sast-readium-<version>-win64.exe`

### Portable ZIP

**Advantages:**

- No installation required
- Can run from USB drive
- No administrator rights needed
- Easy to distribute

**Requirements:**

- Any build (MSVC or MSYS2)

**Features:**

- Self-contained directory
- All dependencies included
- Launcher script included
- README with instructions

**File:** `sast-readium-<version>-win64.zip`

## Package Size Optimization

SAST Readium includes comprehensive package size optimization features that can reduce the final package size by 60-80% compared to unoptimized builds.

### Optimization Options

The packaging system provides several CMake options to control package size:

| Option | Default | Description | Size Impact |
|--------|---------|-------------|-------------|
| `PACKAGING_MINIMAL` | `ON` | Exclude unnecessary Qt plugins | 30-50% reduction |
| `PACKAGING_STRIP_DEBUG` | `ON` | Strip debug symbols from binaries | 20-40% reduction |
| `PACKAGING_AGGRESSIVE_CLEANUP` | `ON` | Remove all development files | 40-60% reduction |

### Quick Configuration

**Production (Recommended):**

```powershell
# All optimizations enabled (default)
cmake --preset=Release-Windows
cmake --build --preset=Release-Windows
cpack -G WIX -C Release
```

**Development Distribution:**

```powershell
# Keep debug symbols for troubleshooting
cmake --preset=Release-Windows -DPACKAGING_STRIP_DEBUG=OFF
cmake --build --preset=Release-Windows
cpack -G ZIP -C Release
```

**Full Deployment (Testing):**

```powershell
# Disable all optimizations
cmake --preset=Release-Windows \
    -DPACKAGING_MINIMAL=OFF \
    -DPACKAGING_STRIP_DEBUG=OFF \
    -DPACKAGING_AGGRESSIVE_CLEANUP=OFF
cmake --build --preset=Release-Windows
cpack -G ZIP -C Release
```

### What Gets Removed

When optimizations are enabled, the following files are automatically excluded:

**Development Files:**

- Header files (`.h`, `.hpp`, `.hxx`)
- Static libraries (`.a`, `.lib`)
- CMake configuration files
- pkg-config files (`.pc`)

**Debug Artifacts:**

- Debug symbols (`.pdb`)
- Incremental linker files (`.ilk`)
- Export files (`.exp`)
- Map files (`.map`)

**Documentation:**

- `doc/`, `docs/`, `examples/` directories
- Man pages and info files
- Qt mkspecs

**Unnecessary Qt Components:**

- QML/Quick runtime (not used)
- WebKit/WebView (not used)
- Virtual keyboard (not used)
- Unused plugin types

### Size Benchmarks

Typical package sizes for Windows x64:

| Configuration | Approximate Size | Use Case |
|--------------|------------------|----------|
| Unoptimized | 150-200 MB | Development/Testing |
| Minimal Qt Only | 100-120 MB | Basic optimization |
| All Optimizations | 40-60 MB | Production (Recommended) |

### Detailed Guide

For comprehensive information about package optimization, including troubleshooting and advanced configuration, see the [Package Size Optimization Guide](optimization-guide.md).

## Detailed Instructions

### Step 1: Build the Application

#### MSVC Build

```powershell
# Clone repository
git clone https://github.com/NJUPT-SAST/sast-readium.git
cd sast-readium

# Configure with vcpkg
cmake --preset=Release-Windows

# Build
cmake --build --preset=Release-Windows --config Release

# Verify build
.\build\Release-Windows\app\Release\app.exe --version
```

#### MSYS2 Build

```bash
# In MSYS2 MINGW64 shell
git clone https://github.com/NJUPT-SAST/sast-readium.git
cd sast-readium

# Install dependencies and build
./scripts/build-msys2.sh -d -t Release

# Verify build
./build/Release-MSYS2/app/app.exe --version
```

### Step 2: Configure Packaging Options

Packaging options are configured in CMake. You can customize them:

```cmake
# In CMakeLists.txt or via command line
option(ENABLE_PACKAGING "Enable packaging support" ON)
option(PACKAGE_PORTABLE "Create portable ZIP packages" ON)
option(PACKAGE_INSTALLER "Create installer packages" ON)
option(DEPLOY_QT_PLUGINS "Automatically deploy Qt plugins" ON)
option(PACKAGING_MINIMAL "Create minimal deployment" ON)
```

To change options:

```powershell
# Via CMake command line
cmake --preset=Release-Windows -DPACKAGING_MINIMAL=OFF

# Or edit CMakePresets.json
```

### Step 3: Create Packages

#### Using PowerShell Script (Recommended)

```powershell
# Simple: Create default package for your build
.\scripts\package.ps1

# Advanced: Specify package type
.\scripts\package.ps1 -PackageType msi -Version 1.0.0

# With verification
.\scripts\package.ps1 -Verify

# Clean build
.\scripts\package.ps1 -Clean
```

#### Using CPack Directly

```powershell
# Navigate to build directory
cd build\Release-Windows

# Create MSI
cpack -G WIX -C Release -B ..\..\packaging

# Create ZIP
cpack -G ZIP -C Release -B ..\..\packaging

# Create multiple formats
cpack -G "WIX;ZIP" -C Release -B ..\..\packaging
```

### Step 4: Verify Package

```powershell
# Run verification script
.\scripts\verify-package.ps1 -InstallPath ".\packaging\<extracted-or-installed-path>"

# With DLL dependency check
.\scripts\verify-package.ps1 -InstallPath ".\packaging\..." -CheckMissing

# Analyze dependencies
.\scripts\analyze-dependencies.ps1 -ExePath ".\build\Release-Windows\app\Release\app.exe" -CheckMissing
```

### Step 5: Test Installation

#### MSI Testing

```powershell
# Install MSI (requires admin)
msiexec /i sast-readium-1.0.0-win64.msi /l*v install.log

# Silent install
msiexec /i sast-readium-1.0.0-win64.msi /qn /l*v install.log

# Uninstall
msiexec /x sast-readium-1.0.0-win64.msi /qn
```

#### NSIS Testing

```powershell
# Run installer
.\sast-readium-1.0.0-win64.exe

# Silent install
.\sast-readium-1.0.0-win64.exe /S

# Silent uninstall
"C:\Program Files\SAST Readium\uninstall.exe" /S
```

#### Portable Testing

```powershell
# Extract ZIP
Expand-Archive sast-readium-1.0.0-win64.zip -DestinationPath test-portable

# Run application
cd test-portable\SAST Readium
.\app.exe
```

## Customization

### Minimal vs Full Deployment

The `PACKAGING_MINIMAL` option controls dependency optimization:

#### Minimal Deployment (Default: ON)

- Excludes unnecessary Qt plugins (qmltooling, generic, networkinformation)
- Smaller package size (~30-50% reduction)
- Faster installation
- All required features still work

#### Full Deployment (PACKAGING_MINIMAL=OFF)

- Includes all Qt plugins
- Larger package size
- May include unused dependencies

To disable minimal deployment:

```powershell
cmake --preset=Release-Windows -DPACKAGING_MINIMAL=OFF
cmake --build --preset=Release-Windows
```

### Customizing windeployqt Flags

Edit `cmake/Packaging.cmake` to modify deployment flags:

```cmake
# In add_deploy_qt_command() function
list(APPEND DEPLOY_COMMAND
    --no-translations
    --no-compiler-runtime
    --no-system-d3d-compiler
    --no-opengl-sw
    # Add custom flags here
    --exclude-plugins qpdf,qwebp  # Exclude specific plugins
)
```

### Customizing Installer Metadata

Edit `cmake/Packaging.cmake` to change installer properties:

```cmake
# WiX MSI configuration
set(CPACK_WIX_PRODUCT_NAME "SAST Readium" PARENT_SCOPE)
set(CPACK_WIX_MANUFACTURER "SAST Team" PARENT_SCOPE)
set(CPACK_WIX_PROPERTY_ARPCONTACT "your-email@example.com" PARENT_SCOPE)

# NSIS configuration
set(CPACK_NSIS_DISPLAY_NAME "SAST Readium" PARENT_SCOPE)
set(CPACK_NSIS_CONTACT "your-email@example.com" PARENT_SCOPE)
```

## Verification

### Automated Verification

```powershell
# Full verification
.\scripts\verify-package.ps1 -InstallPath "C:\Program Files\SAST Readium"

# Quick file check only
.\scripts\verify-package.ps1 -InstallPath "..." -SkipDllCheck -SkipFunctionalTest
```

### Manual Verification Checklist

Use the comprehensive checklist:

```powershell
# Open checklist
notepad docs\packaging\feature-testing-checklist.md
```

Key items to verify:

- [ ] Application launches without DLL errors
- [ ] PDF files open and render correctly
- [ ] Search functionality works
- [ ] Thumbnails generate properly
- [ ] Themes switch correctly
- [ ] Translations load
- [ ] All shortcuts work
- [ ] Uninstaller removes all files

### Dependency Analysis

```powershell
# Analyze DLL dependencies
.\scripts\analyze-dependencies.ps1 -ExePath ".\build\Release-Windows\app\Release\app.exe"

# Check for missing DLLs
.\scripts\analyze-dependencies.ps1 -ExePath "..." -CheckMissing

# Export to JSON
.\scripts\analyze-dependencies.ps1 -ExePath "..." -OutputFormat json > deps.json
```

## Troubleshooting

### Common Issues

#### "WiX Toolset not found"

**Solution:**

```powershell
# Install WiX Toolset 3.x
# Download from https://wixtoolset.org/
# Add to PATH
$env:PATH += ";C:\Program Files (x86)\WiX Toolset v3.14\bin"
```

#### "NSIS not found"

**Solution:**

```powershell
# Install NSIS
# Download from https://nsis.sourceforge.io/
# Add to PATH
$env:PATH += ";C:\Program Files (x86)\NSIS"
```

#### "Missing DLL: Qt6Core.dll"

**Solution:**

```powershell
# Ensure windeployqt ran successfully
# Check build log for deployment errors
# Manually run windeployqt
cd build\Release-Windows\app\Release
windeployqt app.exe
```

#### "Package size too large"

**Solution:**

```powershell
# Enable minimal deployment
cmake --preset=Release-Windows -DPACKAGING_MINIMAL=ON
cmake --build --preset=Release-Windows

# Analyze what's included
.\scripts\analyze-dependencies.ps1 -ExePath "..." -CheckMissing
```

#### "Application crashes on clean system"

**Solution:**

```powershell
# Check for missing runtime dependencies
.\scripts\verify-package.ps1 -InstallPath "..." -CheckMissing

# Ensure Visual C++ Redistributable is included (MSVC builds)
# Or MinGW runtime DLLs are bundled (MSYS2 builds)
```

### Debug Mode

Enable verbose output:

```powershell
# PowerShell script
.\scripts\package.ps1 -Verbose

# CPack
cpack -G WIX -C Release -B packaging --verbose
```

## Best Practices

### 1. Always Test on Clean Systems

- Use a VM or clean Windows installation
- Don't test on development machines with Qt/MSVC installed
- Verify all dependencies are included

### 2. Use Minimal Deployment

- Keep `PACKAGING_MINIMAL=ON` for production
- Reduces package size by 30-50%
- Faster downloads and installations

### 3. Version Management

- Use semantic versioning (MAJOR.MINOR.PATCH)
- Update version in `CMakeLists.txt`
- Keep consistent across all package types

### 4. Digital Signatures

For production releases, sign your installers:

```powershell
# Sign MSI
signtool sign /f certificate.pfx /p password /t http://timestamp.digicert.com sast-readium.msi

# Sign NSIS
signtool sign /f certificate.pfx /p password /t http://timestamp.digicert.com sast-readium.exe
```

### 5. Automated Testing

- Run verification script in CI/CD pipeline
- Test installation/uninstallation
- Verify all features work post-installation

### 6. Documentation

- Include README in portable packages
- Provide installation instructions
- Document system requirements

### 7. Package Naming

Follow consistent naming convention:

- `sast-readium-<version>-win64.msi`
- `sast-readium-<version>-win64.exe`
- `sast-readium-<version>-win64.zip`

## Additional Resources

- [CMake CPack Documentation](https://cmake.org/cmake/help/latest/module/CPack.html)
- [WiX Toolset Documentation](https://wixtoolset.org/documentation/)
- [NSIS Documentation](https://nsis.sourceforge.io/Docs/)
- [Qt Deployment Documentation](https://doc.qt.io/qt-6/windows-deployment.html)
- [Feature Testing Checklist](feature-testing-checklist.md)

## Support

For issues or questions:

- GitHub Issues: <https://github.com/NJUPT-SAST/sast-readium/issues>
- Email: <sast@njupt.edu.cn>

---

**Last Updated:** 2025-10-28
**Version:** 1.0.0
