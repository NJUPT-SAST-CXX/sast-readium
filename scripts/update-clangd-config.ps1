#!/usr/bin/env pwsh
# Script to update .clangd configuration file with the correct CompilationDatabase path
# This ensures clangd can find the compilation database for different build configurations

param(
    [string]$BuildDir = "",
    [switch]$Auto,
    [switch]$List,
    [switch]$Verbose,
    [switch]$Force
)

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ClangdFile = Join-Path $ProjectRoot ".clangd"

# List of possible build directories from CMakePresets.json
$PossibleBuildDirs = @(
    "build/Debug-MSYS2",
    "build/Release-MSYS2",
    "build/Debug-MSYS2-vcpkg",
    "build/Release-MSYS2-vcpkg",
    "build/Debug",
    "build/Release",
    "build/Debug-Windows",
    "build/Release-Windows"
)

function Write-Log {
    param([string]$Message, [string]$Level = "INFO")
    if ($Verbose -or $Level -eq "ERROR") {
        $timestamp = Get-Date -Format "HH:mm:ss"
        Write-Host "[$timestamp] [$Level] $Message"
    }
}

function Test-ClangdConfigEnabled {
    param([string]$BuildPath)

    # Check if ENABLE_CLANGD_CONFIG is disabled in CMake cache
    $cmakeCachePath = Join-Path $BuildPath "CMakeCache.txt"
    if (Test-Path $cmakeCachePath) {
        $cacheContent = Get-Content $cmakeCachePath
        $clangdConfigLine = $cacheContent | Where-Object { $_ -match "^ENABLE_CLANGD_CONFIG:BOOL=(.+)$" }
        if ($clangdConfigLine) {
            $enabled = $matches[1]
            if ($enabled -eq "OFF") {
                return $false
            }
        }
    }
    return $true
}

function Find-ValidBuildDirs {
    $validDirs = @()
    foreach ($dir in $PossibleBuildDirs) {
        $fullPath = Join-Path $ProjectRoot $dir
        $compileCommandsPath = Join-Path $fullPath "compile_commands.json"
        if (Test-Path $compileCommandsPath) {
            $validDirs += $dir
            Write-Log "Found valid build directory: $dir" "INFO"
        }
    }
    return $validDirs
}

function Update-ClangdConfig {
    param([string]$TargetBuildDir)

    Write-Log "Updating .clangd configuration..." "INFO"
    Write-Log "Target build directory: $TargetBuildDir" "INFO"

    # Check if .clangd file exists
    if (-not (Test-Path $ClangdFile)) {
        Write-Log ".clangd file not found, creating new one..." "INFO"
        $defaultContent = @"
CompileFlags:
  CompilationDatabase: $TargetBuildDir
  Remove:
    # Remove C++20 modules flags that clangd doesn't support yet
    - "-fmodules-ts"
    - "-fmodule-mapper=*"
    - "-fdeps-format=*"
    # Remove other potentially problematic flags
    - "-MD"
    - "-x"

# Additional clangd configuration
Index:
  Background: Build

Diagnostics:
  UnusedIncludes: Strict
  MissingIncludes: Strict

InlayHints:
  Enabled: true
  ParameterNames: true
  DeducedTypes: true

Hover:
  ShowAKA: true
"@
        Set-Content $ClangdFile $defaultContent -Encoding UTF8
        Write-Log "Created new .clangd file with build directory: $TargetBuildDir" "INFO"
        return
    }

    # Read existing .clangd file
    $content = Get-Content $ClangdFile -Raw -Encoding UTF8

    # Update CompilationDatabase line
    $updatedContent = $content -replace "CompilationDatabase:.*", "CompilationDatabase: $TargetBuildDir"

    # Ensure Remove section exists after CompilationDatabase
    if ($updatedContent -notmatch "Remove:") {
        $removeSection = @"
  Remove:
    # Remove C++20 modules flags that clangd doesn't support yet
    - "-fmodules-ts"
    - "-fmodule-mapper=*"
    - "-fdeps-format=*"
    # Remove other potentially problematic flags
    - "-MD"
    - "-x"
"@
        $updatedContent = $updatedContent -replace "(CompilationDatabase: $TargetBuildDir)", "`$1`n$removeSection"
    }

    # Write back to file
    Set-Content $ClangdFile $updatedContent -Encoding UTF8
    Write-Log "Successfully updated .clangd configuration" "INFO"
}

# Handle -List parameter
if ($List) {
    Write-Host "Available build directories with compile_commands.json:"
    $validDirs = Find-ValidBuildDirs
    if ($validDirs.Count -eq 0) {
        Write-Host "  No valid build directories found."
        Write-Host "  Run cmake configuration first to generate compile_commands.json"
    } else {
        foreach ($dir in $validDirs) {
            Write-Host "  - $dir"
        }
    }
    exit 0
}

# Handle -Auto parameter
if ($Auto) {
    Write-Log "Auto-detecting build directory..." "INFO"
    $validDirs = Find-ValidBuildDirs

    if ($validDirs.Count -eq 0) {
        Write-Log "No valid build directories found with compile_commands.json" "ERROR"
        Write-Host "Please run cmake configuration first, for example:"
        Write-Host "  cmake --preset Debug-MSYS2"
        exit 1
    }

    # Use the first valid directory (or prefer MSYS2 debug if available)
    $preferredOrder = @("build/Debug-MSYS2", "build/Release-MSYS2", "build/Debug", "build/Release")
    $selectedDir = $validDirs[0]

    foreach ($preferred in $preferredOrder) {
        if ($validDirs -contains $preferred) {
            $selectedDir = $preferred
            break
        }
    }

    # Check if clangd configuration is enabled (unless forced)
    $fullBuildPath = Join-Path $ProjectRoot $selectedDir
    if (-not $Force -and -not (Test-ClangdConfigEnabled $fullBuildPath)) {
        Write-Log "clangd configuration is disabled in CMake (ENABLE_CLANGD_CONFIG=OFF)" "ERROR"
        Write-Host "To override this, use the -Force parameter:"
        Write-Host "  .\scripts\update-clangd-config.ps1 -Auto -Force"
        exit 1
    }

    Write-Log "Auto-selected build directory: $selectedDir" "INFO"
    Update-ClangdConfig $selectedDir
    Write-Host "clangd configuration updated to use: $selectedDir"
    exit 0
}

# Handle explicit BuildDir parameter
if ($BuildDir -ne "") {
    $fullBuildPath = Join-Path $ProjectRoot $BuildDir
    $compileCommandsPath = Join-Path $fullBuildPath "compile_commands.json"

    if (-not (Test-Path $compileCommandsPath)) {
        Write-Log "compile_commands.json not found in: $BuildDir" "ERROR"
        Write-Host "Available directories:"
        $validDirs = Find-ValidBuildDirs
        foreach ($dir in $validDirs) {
            Write-Host "  - $dir"
        }
        exit 1
    }

    # Check if clangd configuration is enabled (unless forced)
    if (-not $Force -and -not (Test-ClangdConfigEnabled $fullBuildPath)) {
        Write-Log "clangd configuration is disabled in CMake (ENABLE_CLANGD_CONFIG=OFF)" "ERROR"
        Write-Host "To override this, use the -Force parameter:"
        Write-Host "  .\scripts\update-clangd-config.ps1 -BuildDir `"$BuildDir`" -Force"
        exit 1
    }

    Update-ClangdConfig $BuildDir
    Write-Host "clangd configuration updated to use: $BuildDir"
    exit 0
}

# No parameters provided, show usage
Write-Host "Usage: update-clangd-config.ps1 [OPTIONS]"
Write-Host ""
Write-Host "Options:"
Write-Host "  -BuildDir <path>    Set specific build directory (e.g., 'build/Debug-MSYS2')"
Write-Host "  -Auto               Auto-detect and use the best available build directory"
Write-Host "  -List               List all available build directories"
Write-Host "  -Verbose            Enable verbose logging"
Write-Host "  -Force              Force update even if ENABLE_CLANGD_CONFIG is OFF"
Write-Host ""
Write-Host "Examples:"
Write-Host "  .\scripts\update-clangd-config.ps1 -Auto"
Write-Host "  .\scripts\update-clangd-config.ps1 -BuildDir build/Release-MSYS2"
Write-Host "  .\scripts\update-clangd-config.ps1 -List"
Write-Host "  .\scripts\update-clangd-config.ps1 -Auto -Force"
