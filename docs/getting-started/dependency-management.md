# Dependency Management Strategy

This document outlines the dependency management strategy for the SAST Readium project, prioritizing system packages over vcpkg for better build performance and reliability.

## Overview

The project uses a **tiered dependency management approach**:

1. **System Packages (Preferred)** - Faster, more reliable, better integration
2. **vcpkg (Fallback)** - When system packages are unavailable or insufficient
3. **MSYS2 (Windows Alternative)** - MinGW-based builds with system packages

## Dependency Priority Matrix

| Platform      | Primary Method           | Fallback Method | Notes                                   |
| ------------- | ------------------------ | --------------- | --------------------------------------- |
| Ubuntu/Debian | `apt` system packages    | vcpkg           | System packages preferred for CI speed  |
| macOS         | `brew` system packages   | vcpkg           | Homebrew provides excellent Qt6 support |
| Windows MSVC  | vcpkg                    | N/A             | Limited system package options          |
| Windows MSYS2 | `pacman` system packages | vcpkg           | Best Windows alternative to vcpkg       |

## Required Dependencies

### Core Dependencies

- **Qt6** (>= 6.2)

  - Components: Core, Gui, Widgets, Svg, LinguistTools
  - System package names:
    - Ubuntu: `qt6-base-dev qt6-svg-dev qt6-tools-dev qt6-speech-dev qt6-l10n-tools`
    - macOS: `qt@6`
    - MSYS2: `mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-svg mingw-w64-x86_64-qt6-tools mingw-w64-x86_64-qt6-speech`

- **Poppler-Qt6**
  - PDF rendering library
  - System package names:
    - Ubuntu: `libpoppler-qt6-dev`
    - macOS: Built from source with Qt6 support (automated in CI)
    - MSYS2: `mingw-w64-x86_64-poppler-qt6`

### Build Tools

- **CMake** (>= 3.28)
- **Ninja** (preferred) or Make
- **pkg-config**
- **C++20 compatible compiler**

## Build Configuration Selection

### Automatic Selection Logic

The CMake configuration automatically selects the appropriate dependency method:

```cmake
# Detect MSYS2 environment
if(WIN32 AND DEFINED ENV{MSYSTEM})
    set(MSYS2_DETECTED TRUE)
    message(STATUS "MSYS2 environment detected: $ENV{MSYSTEM}")
else()
    set(MSYS2_DETECTED FALSE)
endif()

# Determine whether to use vcpkg
if(FORCE_VCPKG)
    set(USE_VCPKG_INTERNAL TRUE)
    message(STATUS "vcpkg usage forced via FORCE_VCPKG option")
elseif(MSYS2_DETECTED AND NOT USE_VCPKG)
    set(USE_VCPKG_INTERNAL FALSE)
    message(STATUS "MSYS2 detected - using system packages instead of vcpkg")
else()
    set(USE_VCPKG_INTERNAL ${USE_VCPKG})
endif()
```

### Manual Override Options

You can override the automatic selection:

- **Force vcpkg**: `cmake -DFORCE_VCPKG=ON`
- **Force system packages**: `cmake -DUSE_VCPKG=OFF`
- **Default behavior**: Let CMake auto-detect

## CMake Presets

### System Package Presets (Preferred)

- `Debug-Unix` - Debug build with system packages
- `Release-Unix` - Release build with system packages
- `Debug-MSYS2` - Debug build with MSYS2 system packages
- `Release-MSYS2` - Release build with MSYS2 system packages

### vcpkg Presets (Fallback)

- `Debug-Unix-vcpkg` - Debug build with vcpkg
- `Release-Unix-vcpkg` - Release build with vcpkg
- `Debug-Windows` - Debug build with vcpkg (Windows MSVC)
- `Release-Windows` - Release build with vcpkg (Windows MSVC)
- `Debug-MSYS2-vcpkg` - Debug build with vcpkg in MSYS2
- `Release-MSYS2-vcpkg` - Release build with vcpkg in MSYS2

## CI/CD Strategy

### GitHub Actions Workflows

1. **System Package Builds (Primary)**

   - Ubuntu: Uses `apt` packages
   - macOS: Uses `brew` packages
   - Faster execution, better caching

2. **vcpkg Builds (Fallback)**

   - All platforms including Windows MSVC
   - Runs if system builds fail or for comprehensive testing
   - Slower but more consistent across platforms

3. **MSYS2 Builds (Windows Alternative)**
   - Dedicated workflow for MSYS2 environment
   - Prioritizes system packages with vcpkg fallback

## When to Use vcpkg

### Required Scenarios

- **Windows MSVC builds** - Limited system package ecosystem
- **Specific version requirements** - When system packages are too old/new
- **Cross-compilation** - When targeting different architectures
- **Hermetic builds** - When reproducibility is critical

### Optional Scenarios

- **Development consistency** - All developers using same versions
- **CI/CD fallback** - When system packages fail or are unavailable

## Performance Comparison

| Method          | Build Time | Cache Efficiency | Reliability | Maintenance |
| --------------- | ---------- | ---------------- | ----------- | ----------- |
| System Packages | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐       | ⭐⭐⭐⭐    | ⭐⭐⭐⭐⭐  |
| vcpkg           | ⭐⭐       | ⭐⭐⭐           | ⭐⭐⭐⭐⭐  | ⭐⭐⭐      |
| MSYS2           | ⭐⭐⭐⭐   | ⭐⭐⭐⭐         | ⭐⭐⭐⭐    | ⭐⭐⭐⭐    |

## Troubleshooting

### System Package Issues

```bash
# Ubuntu: Update package lists
sudo apt update

# macOS: Update Homebrew
brew update

# MSYS2: Update package database
pacman -Sy
```

### vcpkg Issues

```bash
# Update vcpkg
git -C $VCPKG_ROOT pull
$VCPKG_ROOT/bootstrap-vcpkg.sh  # or .bat on Windows

# Clear vcpkg cache
$VCPKG_ROOT/vcpkg remove --outdated
```

### CMake Configuration Issues

```bash
# Clear CMake cache
rm -rf build/
cmake --preset <your-preset>

# Force dependency method
cmake --preset <your-preset> -DUSE_VCPKG=OFF  # or ON
```

## Migration Guide

### From vcpkg-only to System Packages

1. **Install system dependencies**:

   ```bash
   # Ubuntu
   sudo apt install qt6-base-dev qt6-svg-dev qt6-tools-dev qt6-speech-dev libpoppler-qt6-dev

   # macOS
   brew install qt@6
   # Note: poppler-qt6 is built from source automatically in CI
   # For local development, see manual build instructions below

   # MSYS2
   pacman -S mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-svg mingw-w64-x86_64-qt6-speech mingw-w64-x86_64-poppler-qt6
   ```

2. **Use system package presets**:

   ```bash
   cmake --preset Release-Unix      # Instead of Release-Unix-vcpkg
   cmake --preset Release-MSYS2     # Instead of Release-MSYS2-vcpkg
   ```

3. **Update CI workflows** to prioritize system builds

### Fallback to vcpkg

If system packages fail, you can always fallback:

```bash
cmake --preset Release-Unix-vcpkg -DFORCE_VCPKG=ON
```

### Building poppler-qt6 manually on macOS

For local development on macOS, you'll need to build poppler with Qt6 support manually:

```bash
# Install dependencies
brew install cmake ninja qt@6 cairo fontconfig freetype gettext glib gpgme jpeg-turbo libpng libtiff little-cms2 nspr nss openjpeg

# Download and build poppler
POPPLER_VERSION="25.08.0"
curl -L "https://poppler.freedesktop.org/poppler-${POPPLER_VERSION}.tar.xz" -o poppler.tar.xz
tar -xf poppler.tar.xz
cd "poppler-${POPPLER_VERSION}"

# Configure with Qt6 support
export PKG_CONFIG_PATH="$(brew --prefix qt@6)/lib/pkgconfig:$PKG_CONFIG_PATH"
mkdir build && cd build
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$(brew --prefix)" \
  -DENABLE_QT6=ON \
  -DENABLE_QT5=OFF \
  -DENABLE_GLIB=ON \
  -DENABLE_CPP=ON \
  -DBUILD_GTK_TESTS=OFF \
  -DENABLE_BOOST=OFF

# Build and install
make -j$(sysctl -n hw.ncpu)
sudo make install

# Verify installation
export PKG_CONFIG_PATH="$(brew --prefix)/lib/pkgconfig:$PKG_CONFIG_PATH"
pkg-config --exists poppler-qt6 && echo "✓ poppler-qt6 installed successfully"
```
