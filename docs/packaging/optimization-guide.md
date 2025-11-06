# Package Size Optimization Guide

This guide explains the package size optimization features in SAST Readium's packaging system and how to configure them for different deployment scenarios.

## Overview

The packaging system includes comprehensive filtering and cleanup mechanisms to create lean, production-ready packages by excluding unnecessary development files, debug symbols, and build artifacts.

## Optimization Features

### 1. Minimal Qt Deployment

**Option:** `PACKAGING_MINIMAL` (Default: `ON`)

Excludes unnecessary Qt plugins and modules that are not used by the application:

- **Excluded Plugin Types:**
  - `qmltooling` - QML debugging tools
  - `generic` - Generic plugins
  - `networkinformation` - Network information plugins
  - `position` - Positioning plugins
  - `sensors` - Sensor plugins
  - `webview` - WebView plugins

- **Excluded Specific Plugins:**
  - Virtual keyboard plugins
  - Touch UI plugins

- **Excluded Qt Modules:**
  - QML/Quick runtime
  - WebKit
  - ANGLE (uses native OpenGL instead)
  - Virtual keyboard

**Impact:** Reduces package size by 30-50% compared to full Qt deployment.

### 2. Debug Symbol Stripping

**Option:** `PACKAGING_STRIP_DEBUG` (Default: `ON`)

Automatically strips debug symbols from all executables and DLLs in release builds.

- Uses `strip --strip-debug` command
- Only applies to Release and RelWithDebInfo builds
- Preserves functionality while reducing binary sizes

**Impact:** Reduces binary sizes by 20-40% depending on the build configuration.

### 3. Aggressive Dependency Cleanup

**Option:** `PACKAGING_AGGRESSIVE_CLEANUP` (Default: `ON`)

Removes all development and build artifacts from the installed package:

#### Files Removed

**Header Files:**

- `*.h`, `*.hpp`, `*.hxx`, `*.h++`, `*.hh`

**Static Libraries:**

- `*.a` (Unix static libraries)
- `*.lib` (Windows static libraries)
- `*.dll.a` (MinGW import libraries)

**Debug and Build Artifacts:**

- `*.pdb` (Debug symbols)
- `*.ilk` (Incremental linker files)
- `*.exp` (Export files)
- `*.map` (Map files)

**Configuration Files:**

- `*.cmake`, `CMakeLists.txt`
- `*Config.cmake`, `*ConfigVersion.cmake`, `*Targets.cmake`
- `*.pc` (pkg-config files)
- `*.pri`, `*.prl` (Qt project files)

**Temporary and Backup Files:**

- `*~`, `*.bak`, `*.tmp`
- `*.pyc`, `*.pyo` (Python bytecode)

#### Directories Removed

- `include/` - All header files
- `lib/cmake/` - CMake configuration files
- `lib/pkgconfig/` - pkg-config files
- `share/cmake/`, `share/pkgconfig/`
- `share/doc/`, `share/man/`, `share/info/` - Documentation
- `share/examples/` - Example files
- `doc/`, `docs/` - Documentation
- `examples/` - Example code
- `tests/` - Test files
- `mkspecs/` - Qt mkspecs
- `qml/` - QML files (not used in this application)

**Impact:** Reduces package size by 40-60% by removing all non-runtime files.

### 4. Runtime Dependency Filtering

Enhanced `RUNTIME_DEPENDENCIES` configuration that excludes:

- Windows system DLLs
- Debug DLLs (with 'd' suffix or '_debug' suffix)
- Development files in dependency directories
- Build artifacts

**Impact:** Ensures only essential runtime DLLs are included.

## Configuration Options

### CMake Configuration

Configure packaging options when running CMake:

```bash
# Default configuration (all optimizations enabled)
cmake --preset=Release-Windows

# Disable aggressive cleanup (keep some development files)
cmake --preset=Release-Windows -DPACKAGING_AGGRESSIVE_CLEANUP=OFF

# Disable debug stripping (keep debug symbols)
cmake --preset=Release-Windows -DPACKAGING_STRIP_DEBUG=OFF

# Full deployment (include all Qt plugins)
cmake --preset=Release-Windows -DPACKAGING_MINIMAL=OFF
```

### Option Summary

| Option | Default | Description | Size Impact |
|--------|---------|-------------|-------------|
| `ENABLE_PACKAGING` | `ON` | Enable packaging support | N/A |
| `PACKAGE_PORTABLE` | `ON` | Create portable ZIP packages | N/A |
| `PACKAGE_INSTALLER` | `ON` | Create installer packages | N/A |
| `DEPLOY_QT_PLUGINS` | `ON` | Deploy Qt plugins automatically | N/A |
| `PACKAGING_MINIMAL` | `ON` | Exclude unnecessary Qt plugins | 30-50% reduction |
| `PACKAGING_STRIP_DEBUG` | `ON` | Strip debug symbols from binaries | 20-40% reduction |
| `PACKAGING_AGGRESSIVE_CLEANUP` | `ON` | Remove all development files | 40-60% reduction |

## Deployment Scenarios

### Production Deployment (Recommended)

**Goal:** Smallest possible package size with all necessary runtime files.

```bash
cmake --preset=Release-Windows \
    -DPACKAGING_MINIMAL=ON \
    -DPACKAGING_STRIP_DEBUG=ON \
    -DPACKAGING_AGGRESSIVE_CLEANUP=ON

cmake --build --preset=Release-Windows
cpack -G WIX -C Release
```

**Expected Size Reduction:** 60-80% compared to unoptimized package.

### Development Distribution

**Goal:** Include some debugging capabilities while keeping size reasonable.

```bash
cmake --preset=Release-Windows \
    -DPACKAGING_MINIMAL=ON \
    -DPACKAGING_STRIP_DEBUG=OFF \
    -DPACKAGING_AGGRESSIVE_CLEANUP=ON

cmake --build --preset=Release-Windows
cpack -G ZIP -C Release
```

**Expected Size Reduction:** 40-60% compared to unoptimized package.

### Full Deployment (Testing)

**Goal:** Include everything for comprehensive testing.

```bash
cmake --preset=Release-Windows \
    -DPACKAGING_MINIMAL=OFF \
    -DPACKAGING_STRIP_DEBUG=OFF \
    -DPACKAGING_AGGRESSIVE_CLEANUP=OFF

cmake --build --preset=Release-Windows
cpack -G ZIP -C Release
```

**Expected Size Reduction:** Minimal (baseline for comparison).

## Verification

### Check Package Contents

After creating a package, verify the contents:

```powershell
# Extract ZIP package
Expand-Archive package.zip -DestinationPath test-package

# List all files
Get-ChildItem -Recurse test-package | Select-Object FullName, Length

# Check for unwanted files
Get-ChildItem -Recurse test-package -Include *.h,*.lib,*.a,*.pdb
```

### Measure Size Reduction

```powershell
# Compare package sizes
$baseline = (Get-Item baseline-package.zip).Length
$optimized = (Get-Item optimized-package.zip).Length
$reduction = [math]::Round((1 - $optimized/$baseline) * 100, 2)
Write-Host "Size reduction: $reduction%"
```

## Troubleshooting

### Missing DLLs at Runtime

If the application fails to start due to missing DLLs:

1. **Check if aggressive cleanup removed necessary files:**

   ```bash
   cmake -DPACKAGING_AGGRESSIVE_CLEANUP=OFF
   ```

2. **Verify Qt deployment:**

   ```bash
   # Manually run windeployqt
   windeployqt --verbose path/to/app.exe
   ```

3. **Use dependency analysis:**

   ```powershell
   .\scripts\analyze-dependencies.ps1 -ExePath "path\to\app.exe"
   ```

### Package Too Large

If the package is still too large:

1. **Enable all optimizations:**

   ```bash
   cmake -DPACKAGING_MINIMAL=ON \
         -DPACKAGING_STRIP_DEBUG=ON \
         -DPACKAGING_AGGRESSIVE_CLEANUP=ON
   ```

2. **Check for unexpected files:**

   ```powershell
   # Find large files in package
   Get-ChildItem -Recurse package |
       Sort-Object Length -Descending |
       Select-Object -First 20 FullName, @{N='Size(MB)';E={[math]::Round($_.Length/1MB,2)}}
   ```

3. **Review Qt plugins:**
   - Check `bin/plugins/` directory
   - Verify only necessary plugins are included

## Best Practices

1. **Always use Release build type for production packages:**

   ```bash
   cmake --preset=Release-Windows
   ```

2. **Test optimized packages thoroughly:**
   - Verify all features work correctly
   - Test on clean systems without development tools

3. **Keep baseline packages for comparison:**
   - Build one package with all optimizations OFF
   - Compare sizes and functionality

4. **Document custom configurations:**
   - If you disable optimizations, document why
   - Include configuration in build scripts

5. **Use verification scripts:**

   ```powershell
   .\scripts\verify-package.ps1 -InstallPath "path\to\package"
   ```

## Size Benchmarks

Typical package sizes for SAST Readium (Windows x64):

| Configuration | Approximate Size | Use Case |
|--------------|------------------|----------|
| Unoptimized | 150-200 MB | Development/Testing |
| Minimal Qt Only | 100-120 MB | Basic optimization |
| All Optimizations | 40-60 MB | Production (Recommended) |

*Note: Actual sizes depend on Qt version, dependencies, and application features.*

## See Also

- [Windows Packaging Guide](windows-packaging.md)
- [Dependency Analysis](../troubleshooting/dependency-analysis.md)
- [Build Configuration](../getting-started/build-configuration.md)
