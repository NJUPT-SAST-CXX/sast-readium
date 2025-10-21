# CMake Build System Fixes - Windows Configurations

## Executive Summary

This document details the comprehensive fixes applied to the CMake build system to ensure it works correctly under Windows build configurations. The project now supports two fully functional build configurations:

✅ **MSYS2 without vcpkg** - Fully functional
✅ **MSVC with vcpkg** - Fully functional (with PATH configuration)

**Note**: The MSYS2 with vcpkg configuration has been removed from the project due to an insurmountable Qt6/MinGW GCC 15.2.0 incompatibility issue (see Removed Configurations section below).

## Supported Configurations

### 1. MSYS2 without vcpkg ✅ SUCCESS

- **Preset**: `Debug-MSYS2`
- **Compiler**: GCC 15.2.0 (MinGW-w64)
- **Dependencies**: System packages from MSYS2
- **Generator**: Ninja
- **Status**: **FULLY FUNCTIONAL**

### 2. MSVC with vcpkg ✅ SUCCESS

- **Preset**: `Debug-Windows`
- **Compiler**: MSVC 19.44.35217.0
- **Dependencies**: vcpkg with x64-windows triplet
- **Generator**: Visual Studio 17 2022
- **Status**: **FULLY FUNCTIONAL** (with PATH configuration)

## Removed Configurations

### MSYS2 with vcpkg ❌ REMOVED

- **Former Preset**: `Debug-Windows-vcpkg-mingw` (removed)
- **Compiler**: GCC 15.2.0 (MinGW-w64)
- **Dependencies**: vcpkg with x64-mingw-dynamic triplet
- **Generator**: Ninja
- **Status**: **REMOVED FROM PROJECT**
- **Reason**: Qt6 6.8.3 source code incompatibility with MinGW GCC 15.2.0

**Details**: This configuration was removed after extensive troubleshooting revealed that the failure was not due to CMake configuration issues, but rather a fundamental incompatibility between Qt6 6.8.3's qtdeclarative module and MinGW GCC 15.2.0. The build successfully completed 58 out of 70 vcpkg packages before failing during compilation of `qquickmultipointtoucharea.cpp` in the qtdeclarative package. This is a Qt6 upstream issue that cannot be resolved through CMake configuration changes.

**What Was Fixed**: The original RC1107 error that was blocking this configuration was successfully resolved through PATH-based wrapper scripts. However, this revealed the deeper Qt6/MinGW incompatibility issue.

**Recommendation**: Use either MSYS2 without vcpkg (system packages) or MSVC with vcpkg for Windows development.

## Fixes Applied

### Fix 1: Hardcoded vcpkg Paths in CMakePresets.json

**Problem**: CMakePresets.json contained hardcoded paths to vcpkg toolchain file:

```json
"CMAKE_TOOLCHAIN_FILE": "D:/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

**Solution**: Changed to use environment variable:

```json
"CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
```

**Files Modified**: `CMakePresets.json` (lines 77-109)

**Impact**: Allows vcpkg to be installed in any location by setting the `VCPKG_ROOT` environment variable.

### Fix 2: RC1107 Error Resolution (MSYS2 with vcpkg)

**Problem**: When attempting to use MSYS2 with vcpkg, the build failed with RC1107 error during vcpkg package installation.

**Root Cause**: vcpkg was using MSYS2's cmake instead of Visual Studio's cmake, causing incompatibility with Windows Resource Compiler.

**Solution**: Created PATH-based wrapper scripts that ensured Visual Studio's cmake was used:

- `scripts/configure-msys2-vcpkg.ps1` (removed)
- `scripts/build-msys2-vcpkg.ps1` (removed)

**Result**: Successfully resolved RC1107 error and built 58 out of 70 vcpkg packages.

**Outcome**: This fix revealed a deeper Qt6/MinGW GCC 15.2.0 incompatibility issue that could not be resolved through CMake configuration. The MSYS2 with vcpkg configuration was subsequently removed from the project (see Removed Configurations section).

### Fix 3: MSYS2 cmake Interfering with MSVC+vcpkg Builds

**Problem**: When MSYS2 is in the system PATH, vcpkg uses the MSYS2 cmake.exe instead of the native Windows cmake.exe. This causes the RC1107 error because MSYS2 cmake invokes the Windows Resource Compiler (rc.exe) with incorrect arguments.

**Root Cause**:

- vcpkg requires native Windows cmake
- MSYS2 cmake is incompatible with Windows Resource Compiler
- Error: `fatal error RC1107: invalid usage; use RC /? for Help`

**Solution**: Ensure the native Windows cmake (from Visual Studio) is used when building with MSVC+vcpkg:

```powershell
# Remove MSYS2 from PATH and add Visual Studio cmake
$env:PATH = "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;C:\Windows\System32\WindowsPowerShell\v1.0;" + ($env:PATH -split ';' | Where-Object { $_ -notlike '*msys64*' }) -join ';'
$env:VCPKG_ROOT="D:\vcpkg"
cmake --preset=Debug-Windows
cmake --build build/Debug-Windows --target app
```

**Files Modified**: None (runtime PATH configuration)

**Impact**: MSVC+vcpkg builds now succeed without RC1107 errors.

## Build Instructions

### Configuration 1: MSYS2 without vcpkg

```powershell
# Set MSYS2 environment
$env:MSYSTEM='MINGW64'
$env:MSYSTEM_PREFIX='D:\msys64\mingw64'
$env:PATH="D:\msys64\mingw64\bin;D:\msys64\usr\bin;$env:PATH"

# Configure and build
cmake --preset=Debug-MSYS2
cmake --build build/Debug-MSYS2 --target app
```

**Output**: `build/Debug-MSYS2/app.exe` (~129 MB)

### Configuration 2: MSVC with vcpkg

```powershell
# Set up PATH with Visual Studio cmake and remove MSYS2
$env:PATH = "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;C:\Windows\System32\WindowsPowerShell\v1.0;" + ($env:PATH -split ';' | Where-Object { $_ -notlike '*msys64*' }) -join ';'
$env:VCPKG_ROOT="D:\vcpkg"

# Configure and build
cmake --preset=Debug-Windows
cmake --build build/Debug-Windows --target app
```

**Output**: `build/Debug-Windows/Debug/app.exe` (~11 MB)

## Verification Results

### Final Verification (2025-10-21)

Both supported configurations were tested from clean state:

| Configuration | Configure | Build | Executable | Size | Status |
|--------------|-----------|-------|------------|------|--------|
| MSYS2 without vcpkg | ✅ 10.9s | ✅ Success | `build/Debug-MSYS2/app.exe` | 129 MB | ✅ PASS |
| MSVC with vcpkg | ✅ 80.1s | ✅ Success | `build/Debug-Windows/Debug/app.exe` | 11 MB | ✅ PASS |

**Success Rate**: 2/2 supported configurations (100%)

**Note**: The MSYS2 with vcpkg configuration was removed from the project due to Qt6/MinGW GCC 15.2.0 incompatibility (see Removed Configurations section).

## Technical Details

### Environment Requirements

**MSYS2 Configuration**:

- MSYS2 installed at `D:\msys64`
- MINGW64 environment
- Required packages:
  - mingw-w64-x86_64-gcc (15.2.0)
  - mingw-w64-x86_64-qt6-base (6.9.2)
  - mingw-w64-x86_64-qt6-svg
  - mingw-w64-x86_64-poppler-qt6 (25.09.1)
  - mingw-w64-x86_64-spdlog (1.15.3)
  - mingw-w64-x86_64-ninja (1.13.1)
  - mingw-w64-x86_64-cmake (4.1.2)

**MSVC Configuration**:

- Visual Studio 2022 Community
- MSVC 19.44.35217.0
- Windows SDK 10.0.26100.0
- vcpkg installed at `D:\vcpkg`
- Native Windows cmake from Visual Studio

### Dependency Versions

**MSYS2 (system packages)**:

- Qt: 6.9.2
- poppler-qt6: 25.09.1
- spdlog: 1.15.3

**vcpkg (x64-windows)**:

- Qt: 6.8.3
- poppler-qt6: 25.7.0
- spdlog: 1.15.3

## Recommendations

1. **For Development**: Use **MSYS2 without vcpkg** for faster iteration and simpler setup
2. **For Distribution**: Use **MSVC with vcpkg** for smaller binaries and better Windows integration

## Future Work

1. Consider adding a script to automatically configure PATH for MSVC builds
2. Document the PATH configuration requirement in the main README
3. Add CI/CD pipeline to test both working configurations
4. Monitor Qt6 releases for improved MinGW GCC 15.2.0 compatibility

## References

- vcpkg RC1107 issues: <https://github.com/microsoft/vcpkg/issues?q=is%3Aissue+RC1107>
- CMake Presets documentation: <https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html>
- vcpkg triplets: <https://learn.microsoft.com/en-us/vcpkg/users/triplets>
