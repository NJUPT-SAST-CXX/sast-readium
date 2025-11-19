#!/usr/bin/env pwsh
# Local packaging script for SAST Readium (Windows)
# Supports creating .msi packages and portable distributions
# Now integrated with CMake CPack system

param(
    [string]$PackageType = "auto",
    [string]$Version = "0.1.0",
    [string]$BuildType = "Release",
    [switch]$Clean,
    [switch]$UseCMake = $true,
    [switch]$Verify = $false,
    [switch]$Help
)

# Script directory and project root
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = (Get-Item $ScriptDir).Parent.FullName
$BuildDir = Join-Path $ProjectRoot "build"
$PackageDir = Join-Path $ProjectRoot "package"

# Application info
$AppName = "sast-readium"
$AppDisplayName = "SAST Readium"

# Logging functions
function Write-Status {
    param([string]$Message)
    Write-Host "[INFO] $Message" -ForegroundColor Blue
}

function Write-Success {
    param([string]$Message)
    Write-Host "[SUCCESS] $Message" -ForegroundColor Green
}

function Write-Warning {
    param([string]$Message)
    Write-Host "[WARNING] $Message" -ForegroundColor Yellow
}

function Write-Error {
    param([string]$Message)
    Write-Host "[ERROR] $Message" -ForegroundColor Red
}

# Show usage information
function Show-Usage {
    @"
Usage: .\scripts\package.ps1 [OPTIONS]

Create Windows packages for SAST Readium using CMake CPack

OPTIONS:
    -PackageType TYPE       Package type: msi, nsis, zip, all (default: auto)
    -Version VERSION        Version string (default: $Version)
    -BuildType TYPE         Build type: Debug or Release (default: $BuildType)
    -Clean                  Clean packaging directory before building
    -UseCMake               Use CMake CPack for packaging (default: true)
    -Verify                 Run verification after packaging
    -Help                   Show this help message

PACKAGE TYPES:
    msi                     Windows Installer package (.msi) via WiX
    nsis                    NSIS installer (.exe) for MSYS2 builds
    zip                     Portable ZIP distribution
    all                     Build all supported packages
    auto                    Auto-detect and create appropriate package

EXAMPLES:
    .\scripts\package.ps1                           # Create package using CMake
    .\scripts\package.ps1 -PackageType zip          # Create portable ZIP
    .\scripts\package.ps1 -Verify                   # Create and verify package
    .\scripts\package.ps1 -PackageType all -Version 1.0.0  # Create all packages
    .\scripts\package.ps1 -Clean                    # Clean and rebuild packages

REQUIREMENTS:
    CMake 3.28+ with CPack
    MSI: WiX Toolset 3.x (https://wixtoolset.org/)
    NSIS: NSIS 3.x (https://nsis.sourceforge.io/)
    ZIP: Built-in PowerShell Compress-Archive

"@
}

# Detect build type (MSYS2 or Windows)
function Detect-BuildType {
    # Check for MSYS2 build first
    $MSYS2Path = Join-Path $BuildDir "$BuildType-MSYS2"
    $WindowsPath = Join-Path $BuildDir "$BuildType-Windows"

    if (Test-Path (Join-Path $MSYS2Path "app\sast-readium.exe")) {
        Write-Status "Detected MSYS2 build"
        return @{
            Type = "MSYS2"
            Path = $MSYS2Path
        }
    } elseif (Test-Path (Join-Path $WindowsPath ("app\" + $BuildType + "\sast-readium.exe"))) {
        Write-Status "Detected Windows (MSVC) build"
        return @{
            Type = "Windows"
            Path = $WindowsPath
        }
    } else {
        Write-Error "No build found. Please build the project first:"
        Write-Error "  MSYS2: ./scripts/build-msys2.sh -t $BuildType"
        Write-Error "  Windows: cmake --preset=$BuildType-Windows && cmake --build --preset=$BuildType-Windows"
        exit 1
    }
}

# Check if build exists
function Test-Build {
    $BuildInfo = Detect-BuildType

    if ($BuildInfo.Type -eq "Windows") {
        $exeRelative = "app\" + $BuildType + "\sast-readium.exe"
    } else {
        $exeRelative = "app\sast-readium.exe"
    }

    $AppPath = Join-Path $BuildInfo.Path $exeRelative

    if (-not (Test-Path $AppPath)) {
        Write-Error "Build not found at $AppPath"
        exit 1
    }

    Write-Success "Found build at $AppPath"
    return $BuildInfo
}

# Clean packaging directory
function Clear-Packaging {
    if ($Clean) {
        Write-Status "Cleaning packaging directory..."
        if (Test-Path $PackageDir) {
            Remove-Item $PackageDir -Recurse -Force
        }
    }
    New-Item -ItemType Directory -Path $PackageDir -Force | Out-Null
}

# Create package using CMake CPack
function New-CMakePackage {
    param(
        [object]$BuildInfo,
        [string]$Generator
    )

    Write-Status "Creating package using CMake CPack..."
    Write-Status "  Build path: $($BuildInfo.Path)"
    Write-Status "  Generator: $Generator"

    # Check if CMake is available
    $cmake = Get-Command cmake -ErrorAction SilentlyContinue
    if (-not $cmake) {
        Write-Error "CMake not found in PATH"
        return $false
    }

    # Check if cpack is available
    $cpack = Get-Command cpack -ErrorAction SilentlyContinue
    if (-not $cpack) {
        Write-Error "CPack not found in PATH"
        return $false
    }

    # Run CPack
    Push-Location $BuildInfo.Path
    try {
        Write-Status "Running CPack with generator: $Generator"

        $cpackArgs = @(
            "-G", $Generator,
            "-C", $BuildType,
            "-B", $PackageDir
        )

        Write-Status "Command: cpack $($cpackArgs -join ' ')"

        & cpack @cpackArgs

        if ($LASTEXITCODE -ne 0) {
            Write-Error "CPack failed with exit code $LASTEXITCODE"
            return $false
        }

        # Find generated package
        $packages = Get-ChildItem -Path $PackageDir -File | Where-Object {
            $_.Extension -in @('.msi', '.exe', '.zip')
        }

        if ($packages.Count -eq 0) {
            Write-Error "No package files found in $PackageDir"
            return $false
        }

        foreach ($pkg in $packages) {
            Write-Success "Created package: $($pkg.FullName)"
            Write-Status "  Size: $([math]::Round($pkg.Length / 1MB, 2)) MB"
        }

        return $true
    }
    catch {
        Write-Error "CPack execution failed: $_"
        return $false
    }
    finally {
        Pop-Location
    }
}

# Create MSI package using WiX (Windows/MSVC builds only)
function New-MsiPackage {
    param([object]$BuildInfo)

    # Skip MSI for MSYS2 builds (use NSIS instead)
    if ($BuildInfo.Type -eq "MSYS2") {
        Write-Warning "Skipping MSI creation for MSYS2 build (use portable package instead)"
        return $true
    }

    Write-Status "Creating MSI package..."

    # Check for WiX toolset
    $WixPath = Get-Command "candle.exe" -ErrorAction SilentlyContinue
    if (-not $WixPath) {
        Write-Error "WiX Toolset not found. Please install from https://wixtoolset.org/"
        return $false
    }

    $MsiDir = Join-Path $PackageDir "msi"
    New-Item -ItemType Directory -Path $MsiDir -Force | Out-Null

    # Create WiX source file
    $WixSource = @"
<?xml version="1.0" encoding="UTF-8"?>
<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">
  <Product Id="*" Name="$AppDisplayName" Language="1033" Version="$Version"
           Manufacturer="SAST Team" UpgradeCode="12345678-1234-1234-1234-123456789012">
    <Package InstallerVersion="200" Compressed="yes" InstallScope="perMachine" />
    <MajorUpgrade DowngradeErrorMessage="A newer version is already installed." />
    <MediaTemplate EmbedCab="yes" />

    <Feature Id="ProductFeature" Title="$AppDisplayName" Level="1">
      <ComponentGroupRef Id="ProductComponents" />
    </Feature>

    <Property Id="WIXUI_INSTALLDIR" Value="INSTALLFOLDER" />
    <UIRef Id="WixUI_InstallDir" />
  </Product>

  <Fragment>
    <Directory Id="TARGETDIR" Name="SourceDir">
      <Directory Id="ProgramFilesFolder">
        <Directory Id="INSTALLFOLDER" Name="$AppDisplayName" />
      </Directory>
      <Directory Id="ProgramMenuFolder">
        <Directory Id="ApplicationProgramsFolder" Name="$AppDisplayName"/>
      </Directory>
    </Directory>
  </Fragment>

  <Fragment>
    <ComponentGroup Id="ProductComponents" Directory="INSTALLFOLDER">
      <Component Id="MainExecutable" Guid="*">
        <File Id="AppExe" Source="$($BuildInfo.Path)\app\app.exe" KeyPath="yes" />
      </Component>
      <Component Id="QtLibraries" Guid="*">
        <File Id="Qt6Core" Source="$($BuildInfo.Path)\app\Qt6Core.dll" />
        <File Id="Qt6Gui" Source="$($BuildInfo.Path)\app\Qt6Gui.dll" />
        <File Id="Qt6Widgets" Source="$($BuildInfo.Path)\app\Qt6Widgets.dll" />
        <File Id="Qt6Svg" Source="$($BuildInfo.Path)\app\Qt6Svg.dll" />
      </Component>
      <Component Id="StartMenuShortcut" Guid="*">
        <Shortcut Id="ApplicationStartMenuShortcut"
                  Name="$AppDisplayName"
                  Description="Qt6-based PDF reader"
                  Target="[#AppExe]"
                  WorkingDirectory="INSTALLFOLDER"
                  Directory="ApplicationProgramsFolder" />
        <RemoveFolder Id="ApplicationProgramsFolder" On="uninstall"/>
        <RegistryValue Root="HKCU" Key="Software\SAST\$AppName"
                       Name="installed" Type="integer" Value="1" KeyPath="yes"/>
      </Component>
    </ComponentGroup>
  </Fragment>
</Wix>
"@

    $WixFile = Join-Path $MsiDir "installer.wxs"
    $WixSource | Out-File -FilePath $WixFile -Encoding UTF8

    # Compile and link
    Push-Location $MsiDir
    try {
        & candle.exe "installer.wxs"
        if ($LASTEXITCODE -ne 0) {
            Write-Error "WiX compilation failed"
            return $false
        }

        $MsiFile = Join-Path $ProjectRoot "$AppName-$Version-x64.msi"
        & light.exe "installer.wixobj" -o $MsiFile -ext WixUIExtension
        if ($LASTEXITCODE -ne 0) {
            Write-Error "WiX linking failed"
            return $false
        }

        Write-Success "Created $MsiFile"
        return $true
    }
    finally {
        Pop-Location
    }
}

# Create portable ZIP package
function New-PortablePackage {
    param([object]$BuildInfo)

    Write-Status "Creating portable package..."

    $PortableDir = Join-Path $PackageDir "portable"
    $AppDir = Join-Path $PortableDir $AppDisplayName
    New-Item -ItemType Directory -Path $AppDir -Force | Out-Null

    # Copy application files
    $AppFiles = Join-Path $BuildInfo.Path "app\*"
    Copy-Item $AppFiles -Destination $AppDir -Recurse -Force

    # Create launcher script
    $LauncherScript = @"
@echo off
cd /d "%~dp0"
start "" "$AppDisplayName\app.exe" %*
"@

    $LauncherFile = Join-Path $PortableDir "$AppName.bat"
    $LauncherScript | Out-File -FilePath $LauncherFile -Encoding ASCII

    # Create README
    $ReadmeContent = @"
$AppDisplayName $Version - Portable Edition
==========================================

This is a portable version of $AppDisplayName that doesn't require installation.

To run the application:
1. Extract this ZIP file to any folder
2. Double-click $AppName.bat or run app.exe directly from the $AppDisplayName folder

System Requirements:
- Windows 10 or later
- Visual C++ Redistributable (usually pre-installed)

For more information, visit: https://github.com/SAST-Readium/sast-readium
"@

    $ReadmeFile = Join-Path $PortableDir "README.txt"
    $ReadmeContent | Out-File -FilePath $ReadmeFile -Encoding UTF8

    # Create ZIP archive
    $ZipFile = Join-Path $ProjectRoot "$AppName-$Version-portable.zip"

    if (Get-Command "7z" -ErrorAction SilentlyContinue) {
        # Use 7-Zip if available
        & 7z a -tzip $ZipFile "$PortableDir\*"
    } else {
        # Use PowerShell built-in compression
        Compress-Archive -Path "$PortableDir\*" -DestinationPath $ZipFile -Force
    }

    Write-Success "Created $ZipFile"
    return $true
}

# Main function
function Main {
    if ($Help) {
        Show-Usage
        exit 0
    }

    Write-Host "`n╔════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
    Write-Host "║     $AppDisplayName Packaging Script                      ║" -ForegroundColor Cyan
    Write-Host "╚════════════════════════════════════════════════════════════╝`n" -ForegroundColor Cyan

    # Detect build and auto-select package type
    $BuildInfo = Test-Build

    if ($PackageType -eq "auto") {
        if ($BuildInfo.Type -eq "MSYS2") {
            $PackageType = "nsis"
            Write-Status "Auto-selected NSIS installer for MSYS2 build"
        } else {
            $PackageType = "msi"
            Write-Status "Auto-selected MSI installer for MSVC build"
        }
    }

    Clear-Packaging

    $Success = $true

    # Use CMake CPack if enabled
    if ($UseCMake) {
        Write-Status "Using CMake CPack for packaging"

        # Map package types to CPack generators
        $generator = switch ($PackageType) {
            "msi"  { "WIX" }
            "nsis" { "NSIS" }
            "zip"  { "ZIP" }
            "all"  {
                if ($BuildInfo.Type -eq "MSYS2") {
                    "NSIS;ZIP"
                } else {
                    "WIX;ZIP"
                }
            }
            default {
                Write-Error "Unknown package type: $PackageType"
                Show-Usage
                exit 1
            }
        }

        $Success = New-CMakePackage -BuildInfo $BuildInfo -Generator $generator
    }
    else {
        # Legacy packaging (fallback)
        Write-Warning "Using legacy packaging method (not recommended)"

        switch ($PackageType) {
            "msi" {
                $Success = New-MsiPackage -BuildInfo $BuildInfo
            }
            "zip" {
                $Success = New-PortablePackage -BuildInfo $BuildInfo
            }
            "all" {
                $Success = (New-MsiPackage -BuildInfo $BuildInfo) -and (New-PortablePackage -BuildInfo $BuildInfo)
            }
            default {
                Write-Error "Unknown package type: $PackageType"
                Show-Usage
                exit 1
            }
        }
    }

    if ($Success) {
        Write-Success "`nPackaging completed successfully!"
        Write-Status "Generated packages:"

        $packages = Get-ChildItem $PackageDir -File -ErrorAction SilentlyContinue | Where-Object {
            $_.Extension -in @('.msi', '.zip', '.exe')
        }

        foreach ($pkg in $packages) {
            Write-Host "  ✓ $($pkg.Name)" -ForegroundColor Green
            Write-Host "    Size: $([math]::Round($pkg.Length / 1MB, 2)) MB" -ForegroundColor Gray
            Write-Host "    Path: $($pkg.FullName)" -ForegroundColor Gray
        }

        # Run verification if requested
        if ($Verify) {
            Write-Status "`nRunning package verification..."

            $verifyScript = Join-Path $ScriptDir "verify-package.ps1"
            if (Test-Path $verifyScript) {
                # Extract ZIP or use MSI install location for verification
                $testPath = $PackageDir

                & $verifyScript -InstallPath $testPath -SkipFunctionalTest

                if ($LASTEXITCODE -ne 0) {
                    Write-Warning "Package verification found issues"
                }
            } else {
                Write-Warning "Verification script not found: $verifyScript"
            }
        }

        Write-Host "`n✓ All done!" -ForegroundColor Green
    } else {
        Write-Error "`nPackaging failed!"
        exit 1
    }
}

# Run main function
Main
