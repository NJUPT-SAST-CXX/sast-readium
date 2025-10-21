#!/usr/bin/env pwsh
# Configure vcpkg build for Windows using MSVC (Visual Studio 2022)

$ErrorActionPreference = "Stop"

Write-Host "=== Configuring vcpkg Build for Windows (MSVC) ===" -ForegroundColor Cyan

# Set up paths
$VCPKG_ROOT = "D:\vcpkg"

# Verify paths exist
if (-not (Test-Path $VCPKG_ROOT)) {
    Write-Error "vcpkg not found at: $VCPKG_ROOT"
    exit 1
}

# Set environment variable
$env:VCPKG_ROOT = $VCPKG_ROOT

# CRITICAL: Remove MSYS2 from PATH to prevent vcpkg from using MSYS2 cmake
# vcpkg MUST use Visual Studio's cmake, not MSYS2's cmake
$originalPath = $env:PATH
$pathEntries = $env:PATH -split ';'
$cleanedPath = $pathEntries | Where-Object {
    $_ -notmatch 'msys64' -and $_ -notmatch 'MSYS2'
}
$env:PATH = $cleanedPath -join ';'

# Add Visual Studio's cmake to PATH (it's bundled with VS 2022)
$vsCmakePath = "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
if (Test-Path $vsCmakePath) {
    $env:PATH = "$vsCmakePath;$env:PATH"
    Write-Host "Added Visual Studio CMake to PATH: $vsCmakePath" -ForegroundColor Green
} else {
    Write-Warning "Visual Studio CMake not found at: $vsCmakePath"
    Write-Warning "Attempting to use system cmake..."
}

# Clear MSYSTEM to ensure we're not in MSYS2 mode
$env:MSYSTEM = $null

Write-Host "Environment configured:" -ForegroundColor Green
Write-Host "  VCPKG_ROOT: $VCPKG_ROOT"
Write-Host "  MSYS2 removed from PATH: Yes"

# Clean build directory if it exists
$BUILD_DIR = "build\Debug-Windows"
if (Test-Path $BUILD_DIR) {
    Write-Host "Cleaning existing build directory: $BUILD_DIR" -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BUILD_DIR
}

# Run CMake configure
Write-Host "`nConfiguring CMake with Visual Studio 2022..." -ForegroundColor Cyan
cmake --preset=Debug-Windows

if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed with exit code: $LASTEXITCODE"

    # Check for vcpkg log
    if (Test-Path "$BUILD_DIR\vcpkg-manifest-install.log") {
        Write-Host "`nvcpkg installation log:" -ForegroundColor Yellow
        Get-Content "$BUILD_DIR\vcpkg-manifest-install.log" -Tail 100
    }

    exit $LASTEXITCODE
}

Write-Host "`n=== Configuration Complete ===" -ForegroundColor Green
Write-Host "To build, run: cmake --build build/Debug-Windows --config Debug"
