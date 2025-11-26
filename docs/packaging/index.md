# Packaging Guide

This document describes how to create installation packages for SAST Readium on different platforms.

## Supported Platforms & Formats

### Windows

- **NSIS Installer** (.exe) - Standard Windows installer
- **WiX MSI** (.msi) - Microsoft Installer format (MSVC builds)
- **ZIP Portable** (.zip) - Portable archive
- **AppX/MSIX** - Modern Windows app package (Planned)

### Linux

- **DEB Package** (.deb) - Debian/Ubuntu series
- **RPM Package** (.rpm) - RedHat/Fedora/openSUSE series
- **AppImage** - Universal executable format
- **TGZ Archive** (.tar.gz) - Source-style archive

### macOS

- **DMG Image** (.dmg) - Standard macOS installation format
- **TGZ Archive** (.tar.gz) - Universal archive

## Packaging with CMake

### Quick Start

```bash
# Configure (enable packaging)
cmake -B build -DCMAKE_BUILD_TYPE=Release \
      -DENABLE_PACKAGING=ON \
      -DSAST_ENABLE_LTO=ON \
      -DSAST_ENABLE_HARDENING=ON

# Build
cmake --build build --config Release

# Generate all packages
cd build
cpack -C Release
```

### Specifying Package Formats

```bash
# Generate only ZIP portable package
cpack -C Release -G ZIP

# Generate only NSIS installer (Windows)
cpack -C Release -G NSIS

# Generate only DEB package (Linux)
cpack -C Release -G DEB

# Generate multiple formats
cpack -C Release -G "ZIP;NSIS"
```

### Advanced Options

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release \
      -DENABLE_PACKAGING=ON \
      -DPACKAGE_PORTABLE=ON \
      -DPACKAGE_INSTALLER=ON \
      -DPACKAGING_MINIMAL=ON \          # Minimal deployment
      -DPACKAGING_STRIP_DEBUG=ON \      # Strip debug symbols
      -DSAST_ENABLE_LTO=ON              # Enable LTO optimization
```

## Unified Packaging Scripts

The project provides cross-platform unified scripts:

### Linux/macOS

```bash
# CMake
BUILD_TYPE=Release BUILD_SYSTEM=cmake ./scripts/create-packages.sh
```

### Windows (PowerShell)

```powershell
# CMake
.\scripts\package.ps1 -BuildType Release -BuildSystem cmake
```

## Platform Specifics

### Windows

#### NSIS Installer

- Requires [NSIS](https://nsis.sourceforge.io/)
- Generates .exe with installation wizard
- Supports shortcuts, start menu, uninstaller
- Optional PDF file association

#### WiX MSI (MSVC Build)

- Requires [WiX Toolset](https://wixtoolset.org/)
- Generates standard Windows Installer (.msi)
- Enterprise friendly
- Supports upgrades and repairs

### Linux

#### DEB Package

```bash
# Install dependencies
sudo apt install dpkg-dev

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_PACKAGING=ON
cmake --build build
cd build
cpack -G DEB
```

#### RPM Package

```bash
# Install dependencies
sudo dnf install rpm-build

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_PACKAGING=ON
cmake --build build
cd build
cpack -G RPM
```

#### AppImage

```bash
# Install linuxdeploy
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy-x86_64.AppImage
sudo mv linuxdeploy-x86_64.AppImage /usr/local/bin/linuxdeploy

# Install Qt plugin
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x linuxdeploy-plugin-qt-x86_64.AppImage
sudo mv linuxdeploy-plugin-qt-x86_64.AppImage /usr/local/bin/linuxdeploy-plugin-qt

# Build AppImage
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_PACKAGING=ON
cmake --build build
cd build
cpack -G External
```

### macOS

#### DMG Image

```bash
# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_PACKAGING=ON
cmake --build build
cd build
cpack -G DragNDrop
```

#### Code Signing & Notarization

```bash
# Set environment variables
export CODESIGN_IDENTITY="Developer ID Application: YourName (TEAMID)"

# Build and sign
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_PACKAGING=ON
cmake --build build
cd build
cpack -G DragNDrop

# Notarize (requires Apple Developer account)
xcrun notarytool submit SASTReadium-*.dmg --keychain-profile "notarytool-profile" --wait
xcrun stapler staple SASTReadium-*.dmg
```

## Package Verification

### Checking Package Content

#### Windows (ZIP)

```powershell
Expand-Archive -Path SASTReadium-*.zip -DestinationPath temp
tree /F temp
```

#### Linux (TGZ)

```bash
tar -tzf SASTReadium-*.tar.gz
```

#### Linux (DEB)

```bash
dpkg -c SASTReadium-*.deb
```

### Checksum Verification

All packages automatically generate SHA256 checksum files:

```bash
# Verify single package
sha256sum -c SASTReadium-0.1.0.0-Windows-x64-Release.zip.sha256

# Verify all packages
sha256sum -c SHA256SUMS
```

## Naming Convention

Format: `SASTReadium-<Version>-<Platform>-<Architecture>-<BuildType>.<Extension>`

Examples:

- `SASTReadium-0.1.0.0-Windows-x64-Release.zip`
- `SASTReadium-0.1.0.0-Linux-x86_64-Release.deb`
- `SASTReadium-0.1.0.0-macOS-arm64-Release.dmg`

## Dependency Strategy

Different platforms have slightly different strategies for handling dependencies:

- **Windows (MSVC + MSYS2)**
  - Uses CMake `RUNTIME_DEPENDENCIES` and `InstallRequiredSystemLibraries` to collect runtime DLLs.
  - MSVC builds use WiX MSI to package VC++ runtimes.
  - MSYS2 builds copy necessary DLLs (`libgcc`, `libstdc++`, `poppler`, etc.) and use `windeployqt`.
  - Minimal packaging options remove headers, static libs, and debug symbols.

- **Linux (DEB/RPM + AppImage)**
  - DEB/RPM prioritize **system packages**:
    - DEB depends on: `libc6, libqt6core6, libqt6gui6, libqt6widgets6, libqt6svg6, libpoppler-qt6-3`.
  - AppImage uses `linuxdeploy` to bundle Qt and libraries for standalone execution.

- **macOS (.app + DMG)**
  - Builds standard `MACOSX_BUNDLE` `.app`.
  - Uses `DragNDrop` generator for DMG.
  - Automatic signing if `CODESIGN_IDENTITY` is set.

## Best Practices

1. **Enable LTO for Release**: `-DSAST_ENABLE_LTO=ON`
2. **Enable Hardening**: `-DSAST_ENABLE_HARDENING=ON`
3. **Minimal Packaging**: `-DPACKAGING_MINIMAL=ON`
4. **Strip Debug Symbols**: `-DPACKAGING_STRIP_DEBUG=ON`
5. **Verify Checksums**: Always check `SHA256SUMS`.

## References

- [CPack Documentation](https://cmake.org/cmake/help/latest/module/CPack.html)
- [NSIS Documentation](https://nsis.sourceforge.io/Docs/)
- [WiX Toolset](https://wixtoolset.org/documentation/)
- [AppImage Documentation](https://docs.appimage.org/)
