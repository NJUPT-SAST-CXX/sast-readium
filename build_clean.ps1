# Clean build script for SAST Readium
# This script performs a clean build using MSYS2 toolchain

$ErrorActionPreference = "Continue"

Write-Host "=== SAST Readium Clean Build Script ===" -ForegroundColor Cyan
Write-Host ""

# Set up MSYS2 environment paths
$MSYS2_ROOT = "D:\msys64"
$MINGW64_BIN = "$MSYS2_ROOT\mingw64\bin"
$MSYS2_BIN = "$MSYS2_ROOT\usr\bin"

# Add MSYS2 to PATH
$env:PATH = "$MINGW64_BIN;$MSYS2_BIN;$env:PATH"
$env:MSYSTEM = "MINGW64"
$env:MSYSTEM_PREFIX = "$MSYS2_ROOT\mingw64"

Write-Host "Environment Setup:" -ForegroundColor Yellow
Write-Host "  MSYS2_ROOT: $MSYS2_ROOT"
Write-Host "  MSYSTEM: $env:MSYSTEM"
Write-Host "  MSYSTEM_PREFIX: $env:MSYSTEM_PREFIX"
Write-Host ""

# Verify tools
Write-Host "Verifying build tools..." -ForegroundColor Yellow
& "$MINGW64_BIN\cmake.exe" --version | Select-Object -First 1
& "$MINGW64_BIN\ninja.exe" --version
& "$MINGW64_BIN\g++.exe" --version | Select-Object -First 1
Write-Host ""

# Clean build directory
$BUILD_DIR = "build\Debug-MSYS2"
if (Test-Path $BUILD_DIR) {
    Write-Host "Removing existing build directory: $BUILD_DIR" -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BUILD_DIR
    Write-Host "Build directory cleaned" -ForegroundColor Green
}
Write-Host ""

# Configure with CMake
Write-Host "Configuring project with CMake..." -ForegroundColor Yellow
& "$MINGW64_BIN\cmake.exe" --preset=Debug-MSYS2
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed with exit code: $LASTEXITCODE" -ForegroundColor Red
    exit $LASTEXITCODE
}
Write-Host "Configuration completed successfully" -ForegroundColor Green
Write-Host ""

# Build with verbose output
Write-Host "Building project..." -ForegroundColor Yellow
& "$MINGW64_BIN\cmake.exe" --build --preset=Debug-MSYS2 --verbose
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed with exit code: $LASTEXITCODE" -ForegroundColor Red
    exit $LASTEXITCODE
}
Write-Host ""
Write-Host "Build completed successfully!" -ForegroundColor Green
