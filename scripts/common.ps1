#!/usr/bin/env pwsh
# =============================================================================
# SAST Readium - Common PowerShell Utilities
# =============================================================================
# Shared functions for PowerShell scripts in the scripts directory.
# Dot-source this file at the beginning of your script:
#   . "$PSScriptRoot\common.ps1"
# =============================================================================

# =============================================================================
# Logging Functions
# =============================================================================

function Write-Success {
    param([string]$Message)
    Write-Host "✓ $Message" -ForegroundColor Green
}

function Write-Info {
    param([string]$Message)
    Write-Host "ℹ $Message" -ForegroundColor Cyan
}

function Write-Warning-Custom {
    param([string]$Message)
    Write-Host "⚠ $Message" -ForegroundColor Yellow
}

function Write-Error-Custom {
    param([string]$Message)
    Write-Host "✗ $Message" -ForegroundColor Red
}

function Write-Debug-Custom {
    param([string]$Message)
    if ($env:VERBOSE -eq "1" -or $Script:Verbose) {
        Write-Host "[DEBUG] $Message" -ForegroundColor Magenta
    }
}

function Write-Header {
    param([string]$Message)
    Write-Host ""
    Write-Host "=== $Message ===" -ForegroundColor Cyan
}

function Write-Section {
    param([string]$Message)
    Write-Host ""
    Write-Host "--- $Message ---" -ForegroundColor Blue
}

# =============================================================================
# Path Detection Functions
# =============================================================================

function Get-ProjectRoot {
    param([string]$StartPath = $PSScriptRoot)

    $currentDir = $StartPath
    while ($currentDir -and $currentDir -ne [System.IO.Path]::GetPathRoot($currentDir)) {
        if ((Test-Path (Join-Path $currentDir "CMakeLists.txt")) -and
            (Test-Path (Join-Path $currentDir "app"))) {
            return $currentDir
        }
        $currentDir = Split-Path $currentDir -Parent
    }

    # Fallback: assume scripts is directly under project root
    return Split-Path $StartPath -Parent
}

function Find-MSYS2Root {
    # Check environment variable first
    if ($env:MSYS2_ROOT -and (Test-Path "$env:MSYS2_ROOT\mingw64")) {
        return $env:MSYS2_ROOT
    }

    # Check common paths
    $searchPaths = @(
        "D:\msys64",
        "C:\msys64",
        "C:\msys2",
        "$env:USERPROFILE\msys64"
    )

    foreach ($path in $searchPaths) {
        if ($path -and (Test-Path "$path\mingw64")) {
            return $path
        }
    }

    return $null
}

function Find-VcpkgRoot {
    # Check environment variable first
    if ($env:VCPKG_ROOT -and (Test-Path "$env:VCPKG_ROOT\vcpkg.exe")) {
        return $env:VCPKG_ROOT
    }

    # Check common paths
    $searchPaths = @(
        "D:\vcpkg",
        "C:\vcpkg",
        "C:\src\vcpkg",
        "$env:USERPROFILE\vcpkg"
    )

    foreach ($path in $searchPaths) {
        if ($path -and (Test-Path "$path\vcpkg.exe")) {
            return $path
        }
    }

    return $null
}

function Find-VisualStudioPath {
    $vsPaths = @(
        "D:\Program Files\Microsoft Visual Studio\2022\Community",
        "C:\Program Files\Microsoft Visual Studio\2022\Community",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional"
    )

    foreach ($vsPath in $vsPaths) {
        if (Test-Path $vsPath) {
            return $vsPath
        }
    }

    return $null
}

function Find-VSCMakePath {
    $vsPath = Find-VisualStudioPath
    if ($vsPath) {
        $cmakePath = Join-Path $vsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
        if (Test-Path $cmakePath) {
            return $cmakePath
        }
    }
    return $null
}

# =============================================================================
# Build Directory Functions
# =============================================================================

$Script:PossibleBuildDirs = @(
    "build\Debug-Unix",
    "build\Release-Unix",
    "build\Debug-Windows",
    "build\Release-Windows",
    "build\Debug-MSYS2",
    "build\Release-MSYS2"
)

function Find-LatestBuildDir {
    param([string]$ProjectRoot)

    $bestDir = ""
    $bestTime = [DateTime]::MinValue

    foreach ($dir in $Script:PossibleBuildDirs) {
        $fullPath = Join-Path $ProjectRoot $dir
        $compileCommands = Join-Path $fullPath "compile_commands.json"

        if (Test-Path $compileCommands) {
            $modTime = (Get-Item $compileCommands).LastWriteTime
            if ($modTime -gt $bestTime) {
                $bestTime = $modTime
                $bestDir = $dir
            }
        }
    }

    return $bestDir
}

function Get-BuildDirs {
    param([string]$ProjectRoot)

    $dirs = @()
    foreach ($dir in $Script:PossibleBuildDirs) {
        $fullPath = Join-Path $ProjectRoot $dir
        if (Test-Path $fullPath) {
            $hasCompileCommands = Test-Path (Join-Path $fullPath "compile_commands.json")
            $dirs += @{
                Path = $dir
                FullPath = $fullPath
                HasCompileCommands = $hasCompileCommands
            }
        }
    }
    return $dirs
}

# =============================================================================
# Environment Checks
# =============================================================================

function Test-MSYS2Environment {
    return $null -ne $env:MSYSTEM
}

function Test-CommandExists {
    param([string]$Command)
    $null -ne (Get-Command $Command -ErrorAction SilentlyContinue)
}

# =============================================================================
# Utility Functions
# =============================================================================

function Confirm-Action {
    param(
        [string]$Prompt = "Continue?",
        [switch]$DefaultYes
    )

    $suffix = if ($DefaultYes) { "[Y/n]" } else { "[y/N]" }
    $response = Read-Host "$Prompt $suffix"

    if ($DefaultYes) {
        return $response -notmatch '^[Nn]$'
    } else {
        return $response -match '^[Yy]$'
    }
}

function Remove-MSYS2FromPath {
    $pathEntries = $env:PATH -split ';'
    $cleanedPath = $pathEntries | Where-Object {
        $_ -notmatch 'msys64' -and $_ -notmatch 'MSYS2'
    }
    return $cleanedPath -join ';'
}

# =============================================================================
# Initialize Project Root
# =============================================================================

if (-not $Script:ProjectRoot) {
    $Script:ProjectRoot = Get-ProjectRoot
}
