#!/usr/bin/env pwsh
# Local packaging script for SAST Readium (Windows)
# Supports creating .msi packages and portable distributions

param(
    [string]$PackageType = "auto",
    [string]$Version = "0.1.0",
    [string]$BuildType = "Release",
    [switch]$Clean,
    [switch]$Help
)

# Script directory and project root
$ScriptDir = Split-Path -Parent $PSScriptRoot
$ProjectRoot = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $ProjectRoot "build"
$PackageDir = Join-Path $ProjectRoot "packaging"

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

Create Windows packages for SAST Readium

OPTIONS:
    -PackageType TYPE       Package type: msi, portable, all (default: auto)
    -Version VERSION        Version string (default: $Version)
    -BuildType TYPE         Build type: Debug or Release (default: $BuildType)
    -Clean                  Clean packaging directory before building
    -Help                   Show this help message

PACKAGE TYPES:
    msi                     Windows Installer package (.msi)
    portable                Portable ZIP distribution
    all                     Build all supported packages
    auto                    Auto-detect and create MSI

EXAMPLES:
    .\scripts\package.ps1                           # Create MSI package
    .\scripts\package.ps1 -PackageType portable     # Create portable ZIP
    .\scripts\package.ps1 -PackageType all -Version 1.0.0  # Create all packages
    .\scripts\package.ps1 -Clean                    # Clean build packages

REQUIREMENTS:
    MSI: WiX Toolset (https://wixtoolset.org/)
    Portable: 7-Zip or PowerShell Compress-Archive

"@
}

# Check if build exists
function Test-Build {
    $BuildPath = Join-Path $BuildDir "$BuildType-Windows"
    $AppPath = Join-Path $BuildPath "app\app.exe"

    if (-not (Test-Path $AppPath)) {
        Write-Error "Build not found at $AppPath"
        Write-Error "Please build the project first:"
        Write-Error "  cmake --preset=$BuildType-Windows"
        Write-Error "  cmake --build --preset=$BuildType-Windows"
        exit 1
    }

    Write-Success "Found build at $AppPath"
    return $BuildPath
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

# Create MSI package using WiX
function New-MsiPackage {
    param([string]$BuildPath)

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
        <File Id="AppExe" Source="$BuildPath\app\app.exe" KeyPath="yes" />
      </Component>
      <Component Id="QtLibraries" Guid="*">
        <File Id="Qt6Core" Source="$BuildPath\app\Qt6Core.dll" />
        <File Id="Qt6Gui" Source="$BuildPath\app\Qt6Gui.dll" />
        <File Id="Qt6Widgets" Source="$BuildPath\app\Qt6Widgets.dll" />
        <File Id="Qt6Svg" Source="$BuildPath\app\Qt6Svg.dll" />
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
    param([string]$BuildPath)

    Write-Status "Creating portable package..."

    $PortableDir = Join-Path $PackageDir "portable"
    $AppDir = Join-Path $PortableDir $AppDisplayName
    New-Item -ItemType Directory -Path $AppDir -Force | Out-Null

    # Copy application files
    $AppFiles = Join-Path $BuildPath "app\*"
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

    Write-Status "$AppDisplayName Packaging Script"
    Write-Status "=============================="

    # Auto-detect package type
    if ($PackageType -eq "auto") {
        $PackageType = "msi"
    }

    $BuildPath = Test-Build
    Clear-Packaging

    $Success = $true

    # Create packages based on requested types
    switch ($PackageType) {
        "msi" {
            $Success = New-MsiPackage -BuildPath $BuildPath
        }
        "portable" {
            $Success = New-PortablePackage -BuildPath $BuildPath
        }
        "all" {
            $Success = (New-MsiPackage -BuildPath $BuildPath) -and (New-PortablePackage -BuildPath $BuildPath)
        }
        default {
            Write-Error "Unknown package type: $PackageType"
            Show-Usage
            exit 1
        }
    }

    if ($Success) {
        Write-Success "Packaging completed successfully!"
        Write-Status "Generated packages:"
        Get-ChildItem $ProjectRoot -Filter "*$AppName*" | Where-Object { $_.Extension -in @('.msi', '.zip') } | ForEach-Object {
            Write-Host "  $($_.Name)" -ForegroundColor Cyan
        }
    } else {
        Write-Error "Packaging failed!"
        exit 1
    }
}

# Run main function
Main
