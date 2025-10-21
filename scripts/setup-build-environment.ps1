#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Universal vcpkg build environment configuration script for Windows.

.DESCRIPTION
    This script automatically detects available compilers (MSVC, MinGW, Clang) and configures
    the build environment for vcpkg-based builds. It intelligently selects the appropriate
    compiler, configures PATH variables, and runs CMake configuration.

.PARAMETER Compiler
    Override automatic compiler detection. Valid values: 'msvc', 'mingw', 'clang'

.PARAMETER Preset
    CMake preset to use. If not specified, will be auto-selected based on compiler.

.PARAMETER VcpkgRoot
    Path to vcpkg installation. Defaults to D:\vcpkg or $env:VCPKG_ROOT

.PARAMETER MSYS2Root
    Path to MSYS2 installation. Defaults to D:\msys64 or $env:MSYS2_ROOT

.PARAMETER BuildType
    Build type: Debug or Release. Defaults to Debug.

.PARAMETER CleanBuild
    Remove existing build directory before configuration.

.PARAMETER ShowDetails
    Enable detailed output for debugging.

.EXAMPLE
    .\setup-build-environment.ps1
    Auto-detect compiler and configure build environment

.EXAMPLE
    .\setup-build-environment.ps1 -Compiler msvc -BuildType Release
    Force MSVC compiler with Release build

.EXAMPLE
    .\setup-build-environment.ps1 -Compiler mingw -CleanBuild
    Force MinGW compiler and clean build directory
#>

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet('msvc', 'mingw', 'clang', 'auto')]
    [string]$Compiler = 'auto',

    [Parameter(Mandatory=$false)]
    [string]$Preset = '',

    [Parameter(Mandatory=$false)]
    [string]$VcpkgRoot = '',

    [Parameter(Mandatory=$false)]
    [string]$MSYS2Root = '',

    [Parameter(Mandatory=$false)]
    [ValidateSet('Debug', 'Release')]
    [string]$BuildType = 'Debug',

    [Parameter(Mandatory=$false)]
    [switch]$CleanBuild,

    [Parameter(Mandatory=$false)]
    [switch]$ShowDetails
)

$ErrorActionPreference = "Stop"

# ============================================================================
# Helper Functions
# ============================================================================

function Write-Header {
    param([string]$Message)
    Write-Host "`n=== $Message ===" -ForegroundColor Cyan
}

function Write-Success {
    param([string]$Message)
    Write-Host "✓ $Message" -ForegroundColor Green
}

function Write-Info {
    param([string]$Message)
    Write-Host "  $Message" -ForegroundColor Gray
}

function Write-Warning-Custom {
    param([string]$Message)
    Write-Host "⚠ $Message" -ForegroundColor Yellow
}

function Write-Error-Custom {
    param([string]$Message)
    Write-Host "✗ $Message" -ForegroundColor Red
}

function Test-CommandExists {
    param([string]$Command)
    $null -ne (Get-Command $Command -ErrorAction SilentlyContinue)
}

# ============================================================================
# Compiler Detection Functions
# ============================================================================

function Find-MSVC {
    Write-Info "Searching for MSVC (Visual Studio)..."

    # Common Visual Studio installation paths
    $vsPaths = @(
        "D:\Program Files\Microsoft Visual Studio\2022\Community",
        "C:\Program Files\Microsoft Visual Studio\2022\Community",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise",
        "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community",
        "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional",
        "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise"
    )

    foreach ($vsPath in $vsPaths) {
        if (Test-Path $vsPath) {
            $cmakePath = Join-Path $vsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
            $vcvarsPath = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"

            if (Test-Path $cmakePath) {
                $version = Split-Path (Split-Path $vsPath -Parent) -Leaf
                Write-Success "Found Visual Studio $version at: $vsPath"
                return @{
                    Found = $true
                    Path = $vsPath
                    CMakePath = $cmakePath
                    VCVarsPath = $vcvarsPath
                    Version = $version
                }
            }
        }
    }

    Write-Warning-Custom "MSVC not found"
    return @{ Found = $false }
}

function Find-MinGW {
    param([string]$MSYS2Root)

    Write-Info "Searching for MinGW-w64..."

    # Try provided MSYS2Root first
    if ($MSYS2Root -and (Test-Path $MSYS2Root)) {
        $mingwPath = Join-Path $MSYS2Root "mingw64\bin"
        if (Test-Path $mingwPath) {
            $gccPath = Join-Path $mingwPath "gcc.exe"
            if (Test-Path $gccPath) {
                $version = & $gccPath --version 2>&1 | Select-Object -First 1
                Write-Success "Found MinGW-w64 at: $mingwPath"
                Write-Info "Version: $version"
                return @{
                    Found = $true
                    Path = $mingwPath
                    MSYS2Root = $MSYS2Root
                    GCCPath = $gccPath
                    Version = $version
                }
            }
        }
    }

    # Try common MSYS2 installation paths
    $msys2Paths = @(
        "D:\msys64",
        "C:\msys64",
        "C:\msys2",
        "$env:USERPROFILE\msys64"
    )

    foreach ($msys2Path in $msys2Paths) {
        if (Test-Path $msys2Path) {
            $mingwPath = Join-Path $msys2Path "mingw64\bin"
            if (Test-Path $mingwPath) {
                $gccPath = Join-Path $mingwPath "gcc.exe"
                if (Test-Path $gccPath) {
                    $version = & $gccPath --version 2>&1 | Select-Object -First 1
                    Write-Success "Found MinGW-w64 at: $mingwPath"
                    Write-Info "Version: $version"
                    return @{
                        Found = $true
                        Path = $mingwPath
                        MSYS2Root = $msys2Path
                        GCCPath = $gccPath
                        Version = $version
                    }
                }
            }
        }
    }

    Write-Warning-Custom "MinGW-w64 not found"
    return @{ Found = $false }
}

function Find-Clang {
    Write-Info "Searching for Clang..."

    # Check if clang is in PATH
    if (Test-CommandExists "clang") {
        $clangPath = (Get-Command clang).Source
        $version = & clang --version 2>&1 | Select-Object -First 1
        Write-Success "Found Clang in PATH: $clangPath"
        Write-Info "Version: $version"
        return @{
            Found = $true
            Path = (Split-Path $clangPath -Parent)
            ClangPath = $clangPath
            Version = $version
        }
    }

    # Check common installation paths
    $clangPaths = @(
        "C:\Program Files\LLVM\bin",
        "C:\Program Files (x86)\LLVM\bin"
    )

    foreach ($path in $clangPaths) {
        $clangExe = Join-Path $path "clang.exe"
        if (Test-Path $clangExe) {
            $version = & $clangExe --version 2>&1 | Select-Object -First 1
            Write-Success "Found Clang at: $path"
            Write-Info "Version: $version"
            return @{
                Found = $true
                Path = $path
                ClangPath = $clangExe
                Version = $version
            }
        }
    }

    Write-Warning-Custom "Clang not found"
    return @{ Found = $false }
}

function Find-Vcpkg {
    param([string]$VcpkgRoot)

    Write-Info "Searching for vcpkg..."

    # Try provided VcpkgRoot first
    if ($VcpkgRoot -and (Test-Path $VcpkgRoot)) {
        $vcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
        if (Test-Path $vcpkgExe) {
            Write-Success "Found vcpkg at: $VcpkgRoot"
            return @{
                Found = $true
                Path = $VcpkgRoot
                Executable = $vcpkgExe
            }
        }
    }

    # Try environment variable
    if ($env:VCPKG_ROOT -and (Test-Path $env:VCPKG_ROOT)) {
        $vcpkgExe = Join-Path $env:VCPKG_ROOT "vcpkg.exe"
        if (Test-Path $vcpkgExe) {
            Write-Success "Found vcpkg at: $env:VCPKG_ROOT"
            return @{
                Found = $true
                Path = $env:VCPKG_ROOT
                Executable = $vcpkgExe
            }
        }
    }

    # Try common installation paths
    $vcpkgPaths = @(
        "D:\vcpkg",
        "C:\vcpkg",
        "C:\src\vcpkg",
        "$env:USERPROFILE\vcpkg"
    )

    foreach ($path in $vcpkgPaths) {
        $vcpkgExe = Join-Path $path "vcpkg.exe"
        if (Test-Path $vcpkgExe) {
            Write-Success "Found vcpkg at: $path"
            return @{
                Found = $true
                Path = $path
                Executable = $vcpkgExe
            }
        }
    }

    Write-Error-Custom "vcpkg not found. Please install vcpkg or specify -VcpkgRoot"
    return @{ Found = $false }
}

# ============================================================================
# Environment Configuration Functions
# ============================================================================

function Configure-MSVC-Environment {
    param($msvcInfo, $vcpkgInfo)

    Write-Header "Configuring MSVC Build Environment"

    # Remove MSYS2 from PATH to prevent vcpkg from using MSYS2 cmake
    $pathEntries = $env:PATH -split ';'
    $cleanedPath = $pathEntries | Where-Object {
        $_ -notmatch 'msys64' -and $_ -notmatch 'MSYS2'
    }
    $env:PATH = $cleanedPath -join ';'
    Write-Success "Removed MSYS2 from PATH"

    # Add Visual Studio's cmake to PATH
    if (Test-Path $msvcInfo.CMakePath) {
        $env:PATH = "$($msvcInfo.CMakePath);$env:PATH"
        Write-Success "Added Visual Studio CMake to PATH"
    }

    # Set vcpkg root
    $env:VCPKG_ROOT = $vcpkgInfo.Path
    Write-Success "Set VCPKG_ROOT: $($vcpkgInfo.Path)"

    # Clear MSYSTEM to ensure we're not in MSYS2 mode
    $env:MSYSTEM = $null

    Write-Info "Compiler: MSVC $($msvcInfo.Version)"
    Write-Info "Generator: Visual Studio 17 2022"
    Write-Info "Triplet: x64-windows"

    return @{
        Preset = "Debug-Windows"
        BuildDir = "build\Debug-Windows"
        Generator = "Visual Studio 17 2022"
        Triplet = "x64-windows"
    }
}

function Configure-MinGW-Environment {
    param($mingwInfo, $vcpkgInfo)

    Write-Header "Configuring MinGW Build Environment"

    # Add MinGW to PATH
    $env:PATH = "$($mingwInfo.Path);$(Join-Path $mingwInfo.MSYS2Root 'usr\bin');$env:PATH"
    Write-Success "Added MinGW to PATH"

    # Set vcpkg root
    $env:VCPKG_ROOT = $vcpkgInfo.Path
    Write-Success "Set VCPKG_ROOT: $($vcpkgInfo.Path)"

    # Clear MSYSTEM to avoid MSYS2 detection issues
    $env:MSYSTEM = ""

    Write-Info "Compiler: MinGW-w64 GCC"
    Write-Info "Generator: Ninja"
    Write-Info "Triplet: x64-mingw-dynamic"
    Write-Warning-Custom "Note: MinGW with vcpkg may have compatibility issues with Qt6 6.8.3"

    return @{
        Preset = "Debug-MSYS2"
        BuildDir = "build\Debug-MSYS2"
        Generator = "Ninja"
        Triplet = "x64-mingw-dynamic"
    }
}

function Configure-Clang-Environment {
    param($clangInfo, $vcpkgInfo)

    Write-Header "Configuring Clang Build Environment"

    # Add Clang to PATH
    $env:PATH = "$($clangInfo.Path);$env:PATH"
    Write-Success "Added Clang to PATH"

    # Set vcpkg root
    $env:VCPKG_ROOT = $vcpkgInfo.Path
    Write-Success "Set VCPKG_ROOT: $($vcpkgInfo.Path)"

    Write-Info "Compiler: Clang"
    Write-Info "Generator: Ninja"
    Write-Info "Triplet: x64-windows"

    return @{
        Preset = "Debug-Windows"
        BuildDir = "build\Debug-Clang"
        Generator = "Ninja"
        Triplet = "x64-windows"
    }
}

# ============================================================================
# Main Script Logic
# ============================================================================

Write-Header "Universal vcpkg Build Environment Setup"

# Detect vcpkg
$vcpkgInfo = Find-Vcpkg -VcpkgRoot $VcpkgRoot
if (-not $vcpkgInfo.Found) {
    Write-Error "vcpkg is required but not found. Exiting."
    exit 1
}

# Detect compilers
$compilerInfo = @{}

if ($Compiler -eq 'auto' -or $Compiler -eq 'msvc') {
    $msvcInfo = Find-MSVC
    if ($msvcInfo.Found) {
        $compilerInfo['msvc'] = $msvcInfo
    }
}

if ($Compiler -eq 'auto' -or $Compiler -eq 'mingw') {
    $mingwInfo = Find-MinGW -MSYS2Root $MSYS2Root
    if ($mingwInfo.Found) {
        $compilerInfo['mingw'] = $mingwInfo
    }
}

if ($Compiler -eq 'auto' -or $Compiler -eq 'clang') {
    $clangInfo = Find-Clang
    if ($clangInfo.Found) {
        $compilerInfo['clang'] = $clangInfo
    }
}

# Select compiler
$selectedCompiler = $null
$config = $null

if ($Compiler -ne 'auto') {
    # User specified a compiler
    if ($compilerInfo.ContainsKey($Compiler)) {
        $selectedCompiler = $Compiler
        Write-Success "Using user-specified compiler: $Compiler"
    } else {
        Write-Error "Requested compiler '$Compiler' not found on system"
        exit 1
    }
} else {
    # Auto-select compiler (prefer MSVC > MinGW > Clang)
    if ($compilerInfo.ContainsKey('msvc')) {
        $selectedCompiler = 'msvc'
        Write-Success "Auto-selected compiler: MSVC (recommended for Windows)"
    } elseif ($compilerInfo.ContainsKey('mingw')) {
        $selectedCompiler = 'mingw'
        Write-Success "Auto-selected compiler: MinGW"
    } elseif ($compilerInfo.ContainsKey('clang')) {
        $selectedCompiler = 'clang'
        Write-Success "Auto-selected compiler: Clang"
    } else {
        Write-Error "No compatible compiler found on system"
        Write-Info "Please install one of: MSVC (Visual Studio), MinGW-w64 (MSYS2), or Clang"
        exit 1
    }
}

# Configure environment based on selected compiler
switch ($selectedCompiler) {
    'msvc' {
        $config = Configure-MSVC-Environment -msvcInfo $compilerInfo['msvc'] -vcpkgInfo $vcpkgInfo
    }
    'mingw' {
        $config = Configure-MinGW-Environment -mingwInfo $compilerInfo['mingw'] -vcpkgInfo $vcpkgInfo
    }
    'clang' {
        $config = Configure-Clang-Environment -clangInfo $compilerInfo['clang'] -vcpkgInfo $vcpkgInfo
    }
}

# Override preset if user specified one
if ($Preset) {
    $config.Preset = $Preset
    Write-Info "Using user-specified preset: $Preset"
}

# Adjust preset for build type
if ($BuildType -eq 'Release') {
    $config.Preset = $config.Preset -replace 'Debug', 'Release'
    $config.BuildDir = $config.BuildDir -replace 'Debug', 'Release'
}

# Clean build directory if requested
if ($CleanBuild -and (Test-Path $config.BuildDir)) {
    Write-Header "Cleaning Build Directory"
    Remove-Item -Recurse -Force $config.BuildDir
    Write-Success "Removed: $($config.BuildDir)"
}

# Run CMake configuration
Write-Header "Running CMake Configuration"
Write-Info "Preset: $($config.Preset)"
Write-Info "Build Directory: $($config.BuildDir)"

$cmakeArgs = @("--preset=$($config.Preset)")
if ($Verbose) {
    $cmakeArgs += "--verbose"
}

Write-Host "`nExecuting: cmake $($cmakeArgs -join ' ')" -ForegroundColor Cyan
& cmake @cmakeArgs

if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "CMake configuration failed with exit code: $LASTEXITCODE"

    # Check for vcpkg log
    $vcpkgLog = Join-Path $config.BuildDir "vcpkg-manifest-install.log"
    if (Test-Path $vcpkgLog) {
        Write-Host "`nvcpkg installation log (last 100 lines):" -ForegroundColor Yellow
        Get-Content $vcpkgLog -Tail 100
    }

    exit $LASTEXITCODE
}

# Success!
Write-Header "Configuration Complete"
Write-Success "Build environment configured successfully"
Write-Info "Compiler: $selectedCompiler"
Write-Info "Preset: $($config.Preset)"
Write-Info "Build Directory: $($config.BuildDir)"

# Provide build instructions
Write-Host "`nNext steps:" -ForegroundColor Cyan
if ($selectedCompiler -eq 'msvc') {
    Write-Host "  cmake --build $($config.BuildDir) --config $BuildType" -ForegroundColor White
} else {
    Write-Host "  cmake --build $($config.BuildDir)" -ForegroundColor White
}

Write-Host "`nEnvironment variables set:" -ForegroundColor Cyan
Write-Host "  VCPKG_ROOT=$env:VCPKG_ROOT" -ForegroundColor Gray
if ($null -ne $env:MSYSTEM) {
    Write-Host "  MSYSTEM=$env:MSYSTEM" -ForegroundColor Gray
}
