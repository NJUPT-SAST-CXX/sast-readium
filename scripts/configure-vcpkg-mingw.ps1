#!/usr/bin/env pwsh
# Configure vcpkg build for Windows using MinGW

$ErrorActionPreference = "Stop"

Write-Host "=== Configuring vcpkg Build for Windows (MinGW) ===" -ForegroundColor Cyan

# Set up paths
$MSYS2_ROOT = "D:\msys64"
$MINGW64_BIN = "$MSYS2_ROOT\mingw64\bin"
$MSYS2_BIN = "$MSYS2_ROOT\usr\bin"
$VCPKG_ROOT = "D:\vcpkg"

# Verify paths exist
if (-not (Test-Path $MINGW64_BIN)) {
    Write-Error "MSYS2 MinGW64 not found at: $MINGW64_BIN"
    exit 1
}

if (-not (Test-Path $VCPKG_ROOT)) {
    Write-Error "vcpkg not found at: $VCPKG_ROOT"
    exit 1
}

# Set environment variables
$env:PATH = "$MINGW64_BIN;$MSYS2_BIN;$env:PATH"
$env:MSYSTEM = ""  # Clear MSYSTEM to avoid MSYS2 detection

Write-Host "Environment configured:" -ForegroundColor Green
Write-Host "  MINGW64_BIN: $MINGW64_BIN"
Write-Host "  VCPKG_ROOT: $VCPKG_ROOT"
Write-Host "  PATH includes MinGW64"

# Clean build directory if it exists
$BUILD_DIR = "build\Debug-Windows-vcpkg"
if (Test-Path $BUILD_DIR) {
    Write-Host "Cleaning existing build directory: $BUILD_DIR" -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BUILD_DIR
}

# Run CMake configure
Write-Host "`nConfiguring CMake..." -ForegroundColor Cyan
& "$MINGW64_BIN\cmake.exe" --preset=Debug-Windows-vcpkg-mingw

if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed with exit code: $LASTEXITCODE"
    exit $LASTEXITCODE
}

Write-Host "`n=== Configuration Complete ===" -ForegroundColor Green
Write-Host "To build, run: cmake --build build/Debug-Windows-vcpkg -j 8"
