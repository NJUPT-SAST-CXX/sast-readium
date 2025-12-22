# Build Size Optimization Guide

This document describes the build size optimization features available in both CMake and xmake build systems for SAST Readium.

## Overview

Build size optimization is critical for:

- **Distribution**: Smaller downloads for end users
- **CI/CD**: Faster artifact transfers and storage savings
- **Development**: Reduced disk usage during development

Both build systems now provide unified options for controlling binary size.

## Quick Reference

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `SAST_SPLIT_DEBUG_INFO` | ON | Split debug info to separate files |
| `SAST_STRIP_BINARIES` | OFF | Strip debug symbols from executables |
| `SAST_STRIP_TESTS` | OFF | Strip debug symbols from test executables |
| `SAST_MINIMAL_TEST_DEBUG` | ON | Use minimal debug info for tests |
| `SAST_ENABLE_LTO` | OFF | Enable Link Time Optimization |
| `SAST_UPX_COMPRESS` | OFF | Compress executables with UPX |
| `SAST_ENABLE_SANITIZERS` | OFF | Enable ASan/UBSan (increases size) |
| `PACKAGING_MINIMAL` | ON | Exclude unnecessary Qt plugins |
| `PACKAGING_STRIP_DEBUG` | ON | Strip debug symbols during packaging |
| `PACKAGING_AGGRESSIVE_CLEANUP` | ON | Remove development files from packages |

### xmake Options

| Option | Default | Description |
|--------|---------|-------------|
| `split_debug_info` | true | Split debug info to separate files |
| `strip_binaries` | false | Strip debug symbols from executables |
| `enable_lto` | false | Enable Link Time Optimization |
| `upx_compress` | false | Compress executables with UPX |
| `enable_sanitizers` | false | Enable ASan/UBSan (increases size) |
| `minimal_deployment` | true | Exclude unnecessary Qt plugins |

## Recommended Configurations

### Development Build (Fast iteration)

**CMake:**

```bash
cmake --preset=Debug-MSYS2
# or
cmake --preset=Debug-Unix
```

**xmake:**

```bash
xmake f -m debug
xmake
```

**Expected size:** ~200-500 MB (with split debug info)

### Release Build (Balanced)

**CMake:**

```bash
cmake --preset=Release-MSYS2
cmake --build --preset=Release-MSYS2
```

**xmake:**

```bash
xmake f -m release
xmake
```

**Expected size:** ~50-100 MB

### Minimum Size Build (Distribution)

**CMake:**

```bash
cmake --preset=MinSizeRel-MSYS2
cmake --build --preset=MinSizeRel-MSYS2
```

**xmake:**

```bash
xmake f -m release --strip_binaries=y --enable_lto=y
xmake
```

**Expected size:** ~30-50 MB

### Ultra-Compact Build (With UPX)

**CMake:**

```bash
cmake -B build -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DSAST_STRIP_BINARIES=ON \
    -DSAST_ENABLE_LTO=ON \
    -DSAST_UPX_COMPRESS=ON
cmake --build build
```

**xmake:**

```bash
xmake f -m release --strip_binaries=y --enable_lto=y --upx_compress=y
xmake
```

**Expected size:** ~15-25 MB

## Size Optimization Techniques

### 1. Debug Info Management

**Split Debug Info** separates debugging symbols from executables:

- On Linux/macOS: Uses `-gsplit-dwarf` to create `.dwo` files
- On Windows/MinGW: Uses `-g1` for minimal debug info

This reduces executable size by 50-70% while keeping debug capability.

### 2. Symbol Stripping

Removes debug symbols entirely:

```bash
# CMake
cmake -DSAST_STRIP_BINARIES=ON ...

# xmake
xmake f --strip_binaries=y
```

**Reduction:** 30-50% size decrease

### 3. Link Time Optimization (LTO)

Enables whole-program optimization:

```bash
# CMake
cmake -DSAST_ENABLE_LTO=ON ...

# xmake
xmake f --enable_lto=y
```

**Reduction:** 10-20% size decrease, may increase link time

### 4. Dead Code Elimination

Both build systems enable dead code elimination by default:

- Compiler flags: `-ffunction-sections -fdata-sections`
- Linker flags: `-Wl,--gc-sections`

This removes unused functions and data.

### 5. Qt Plugin Filtering

Minimal deployment excludes unnecessary Qt plugins:

- QML tooling plugins
- Multimedia plugins
- Database driver plugins
- Sensor plugins
- WebView plugins

**Reduction:** 20-40 MB from Qt deployment

### 6. UPX Compression

UPX compresses executables with automatic decompression at runtime:

```bash
# Install UPX first
# Windows (MSYS2): pacman -S upx
# Linux: apt install upx-ucl
# macOS: brew install upx

# Then enable in build
cmake -DSAST_UPX_COMPRESS=ON ...
```

**Reduction:** 50-70% additional size decrease

**Trade-offs:**

- Slight startup time increase (~100ms)
- Cannot be used with code signing (must sign after decompression)
- May trigger antivirus false positives

### 7. Sanitizers (Debug Only)

**Warning:** AddressSanitizer and UBSan increase binary size by 2-3x!

They are now **disabled by default** and must be explicitly enabled:

```bash
# CMake
cmake -DSAST_ENABLE_SANITIZERS=ON ...

# xmake
xmake f --enable_sanitizers=y
```

Only enable for specific debugging sessions.

## Size Comparison Table

| Configuration | Debug | Release | MinSizeRel | +UPX |
|---------------|-------|---------|------------|------|
| Executable | ~300 MB | ~80 MB | ~40 MB | ~15 MB |
| With Qt DLLs | ~500 MB | ~150 MB | ~80 MB | ~35 MB |
| Full Package | ~600 MB | ~180 MB | ~100 MB | ~50 MB |

*Approximate sizes for Windows MSYS2 build*

## Packaging for Distribution

### CMake Package Presets

```bash
# Windows MSVC (MSI installer)
cmake --preset=Package-Windows-MSVC
cmake --build --preset=Package-Windows-MSVC
cpack --config build/Package-Windows-MSVC/CPackConfig.cmake

# Windows MinGW (NSIS installer + ZIP)
cmake --preset=Package-Windows-MinGW
cmake --build --preset=Package-Windows-MinGW
cpack --config build/Package-Windows-MinGW/CPackConfig.cmake
```

### xmake Packaging

```bash
xmake f -m release --enable_packaging=y --minimal_deployment=y
xmake
# Packages will be created in ./package/
```

## Troubleshooting

### Binary size unexpectedly large

1. Check if sanitizers are enabled: `SAST_ENABLE_SANITIZERS` / `enable_sanitizers`
2. Verify build type is Release or MinSizeRel
3. Check if debug info is being included

### UPX compression fails

1. Ensure UPX is installed and in PATH
2. Check if binary is already compressed
3. Verify sufficient disk space for compression

### Qt deployment too large

1. Enable `PACKAGING_MINIMAL` / `minimal_deployment`
2. Verify `--skip-plugin-types` is being applied
3. Check for debug Qt DLLs (`*d.dll`)

## Best Practices

1. **Development**: Use Debug with split debug info for fast iteration
2. **Testing**: Use Release with minimal deployment for CI
3. **Distribution**: Use MinSizeRel + LTO + Strip for official releases
4. **Size-critical**: Add UPX compression for download-sensitive scenarios

## vcpkg Dependency Optimization

The `vcpkg.json` manifest is configured with minimal Qt features:

```json
{
    "name": "qtbase",
    "default-features": false,
    "features": ["concurrent", "gui", "opengl", "thread", "widgets"]
}
```

**Excluded features** (not needed for PDF reader):

- `network` - No network functionality required
- `sql-sqlite` - No database storage needed
- `testlib` - Only needed for testing, not runtime

This reduces the vcpkg build time and installed size significantly.

## Build Scripts

### MSYS2 Build Script

The `scripts/build-msys2.sh` script supports all optimization options:

```bash
# Minimum size release build
./scripts/build-msys2.sh -t MinSizeRel -m

# Release with specific optimizations
./scripts/build-msys2.sh -t Release -l -s  # LTO + Strip

# Full optimization with packaging
./scripts/build-msys2.sh -t MinSizeRel -m -p portable
```

### Available Options

| Option | Description |
|--------|-------------|
| `-t MinSizeRel` | Minimum size release build type |
| `-l, --lto` | Enable Link Time Optimization |
| `-s, --strip` | Strip debug symbols |
| `-u, --upx` | Compress with UPX |
| `-m, --minimal` | Enable all optimizations (LTO + strip + UPX) |

## See Also

- [CMake Presets Reference](../CMakePresets.json)
- [Build System Comparison](build-system-comparison.md)
- [Packaging Guide](../PACKAGING.md)
