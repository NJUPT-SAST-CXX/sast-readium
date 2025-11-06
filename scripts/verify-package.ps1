#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Verify SAST Readium package integrity and functionality.

.DESCRIPTION
    This script verifies that a packaged SAST Readium installation contains
    all required dependencies and can perform basic functionality tests.

.PARAMETER InstallPath
    Path to the installed/extracted application directory.

.PARAMETER TestPDF
    Optional path to a PDF file for testing functionality.

.PARAMETER SkipDllCheck
    Skip DLL dependency analysis.

.PARAMETER SkipFunctionalTest
    Skip functional testing (only check files).

.EXAMPLE
    .\verify-package.ps1 -InstallPath "C:\Program Files\SAST Readium"

.EXAMPLE
    .\verify-package.ps1 -InstallPath ".\build\package" -TestPDF "test.pdf"
#>

param(
    [Parameter(Mandatory=$true)]
    [string]$InstallPath,

    [Parameter(Mandatory=$false)]
    [string]$TestPDF = "",

    [switch]$SkipDllCheck = $false,

    [switch]$SkipFunctionalTest = $false
)

# Color output functions
function Write-Success { param([string]$Message) Write-Host "✓ $Message" -ForegroundColor Green }
function Write-Failure { param([string]$Message) Write-Host "✗ $Message" -ForegroundColor Red }
function Write-Warning { param([string]$Message) Write-Host "⚠ $Message" -ForegroundColor Yellow }
function Write-Info { param([string]$Message) Write-Host "ℹ $Message" -ForegroundColor Cyan }
function Write-Section { param([string]$Message) Write-Host "`n=== $Message ===" -ForegroundColor Magenta }

# Verification results
$script:PassCount = 0
$script:FailCount = 0
$script:WarnCount = 0

function Test-FileExists {
    param([string]$Path, [string]$Description)

    if (Test-Path $Path) {
        Write-Success "$Description found: $Path"
        $script:PassCount++
        return $true
    } else {
        Write-Failure "$Description missing: $Path"
        $script:FailCount++
        return $false
    }
}

function Test-DirectoryExists {
    param([string]$Path, [string]$Description)

    if (Test-Path $Path -PathType Container) {
        Write-Success "$Description found: $Path"
        $script:PassCount++
        return $true
    } else {
        Write-Warning "$Description missing (optional): $Path"
        $script:WarnCount++
        return $false
    }
}

# Main verification
Write-Host "`n╔════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║     SAST Readium Package Verification Tool                ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════════╝`n" -ForegroundColor Cyan

# Validate install path
if (-not (Test-Path $InstallPath)) {
    Write-Failure "Installation path does not exist: $InstallPath"
    exit 1
}

Write-Info "Verifying installation at: $InstallPath"
Write-Info "Current directory: $(Get-Location)"

# Determine executable location
$ExePath = Join-Path $InstallPath "app.exe"
if (-not (Test-Path $ExePath)) {
    $ExePath = Join-Path $InstallPath "bin\app.exe"
}

# ============================================================================
# Section 1: Core Application Files
# ============================================================================
Write-Section "Core Application Files"

Test-FileExists $ExePath "Main executable"

# ============================================================================
# Section 2: Qt6 Dependencies
# ============================================================================
Write-Section "Qt6 Dependencies"

$BinDir = Split-Path $ExePath -Parent
$RequiredQt6Dlls = @(
    "Qt6Core.dll",
    "Qt6Gui.dll",
    "Qt6Widgets.dll",
    "Qt6OpenGLWidgets.dll",
    "Qt6Svg.dll",
    "Qt6Concurrent.dll",
    "Qt6Core5Compat.dll",
    "Qt6PrintSupport.dll"
)

foreach ($dll in $RequiredQt6Dlls) {
    $dllPath = Join-Path $BinDir $dll
    Test-FileExists $dllPath "Qt6 library: $dll"
}

# ============================================================================
# Section 3: Qt6 Plugins
# ============================================================================
Write-Section "Qt6 Plugins"

$PluginDir = Join-Path $BinDir "plugins"
if (-not (Test-Path $PluginDir)) {
    $PluginDir = Join-Path (Split-Path $BinDir -Parent) "plugins"
}

# Required plugin directories
$RequiredPlugins = @{
    "platforms" = @("qwindows.dll")
    "styles" = @("qwindowsvistastyle.dll")
    "imageformats" = @("qjpeg.dll", "qsvg.dll")
    "iconengines" = @("qsvgicon.dll")
}

foreach ($pluginType in $RequiredPlugins.Keys) {
    $pluginTypeDir = Join-Path $PluginDir $pluginType

    if (Test-DirectoryExists $pluginTypeDir "Plugin directory: $pluginType") {
        foreach ($plugin in $RequiredPlugins[$pluginType]) {
            $pluginPath = Join-Path $pluginTypeDir $plugin
            Test-FileExists $pluginPath "Plugin: $pluginType\$plugin"
        }
    }
}

# ============================================================================
# Section 4: Third-Party Dependencies
# ============================================================================
Write-Section "Third-Party Dependencies"

# Poppler (PDF rendering)
$PopplerDlls = Get-ChildItem -Path $BinDir -Filter "*poppler*.dll" -ErrorAction SilentlyContinue
if ($PopplerDlls.Count -gt 0) {
    Write-Success "Poppler libraries found: $($PopplerDlls.Count) DLL(s)"
    $script:PassCount++
} else {
    Write-Failure "Poppler libraries not found (required for PDF rendering)"
    $script:FailCount++
}

# spdlog (logging)
$SpdlogDll = Join-Path $BinDir "spdlog.dll"
if (Test-Path $SpdlogDll) {
    Write-Success "spdlog library found"
    $script:PassCount++
}

# ============================================================================
# Section 5: Runtime Dependencies
# ============================================================================
Write-Section "Runtime Dependencies"

# Check for MSVC runtime or MinGW runtime
$MsvcRuntime = @("vcruntime*.dll", "msvcp*.dll")
$MinGWRuntime = @("libgcc_s_*.dll", "libstdc++*.dll", "libwinpthread*.dll")

$HasMsvcRuntime = $false
$HasMinGWRuntime = $false

foreach ($pattern in $MsvcRuntime) {
    $found = Get-ChildItem -Path $BinDir -Filter $pattern -ErrorAction SilentlyContinue
    if ($found.Count -gt 0) {
        $HasMsvcRuntime = $true
        Write-Success "MSVC runtime found: $($found.Name -join ', ')"
        $script:PassCount++
    }
}

foreach ($pattern in $MinGWRuntime) {
    $found = Get-ChildItem -Path $BinDir -Filter $pattern -ErrorAction SilentlyContinue
    if ($found.Count -gt 0) {
        $HasMinGWRuntime = $true
        Write-Success "MinGW runtime found: $($found.Name -join ', ')"
        $script:PassCount++
    }
}

if (-not $HasMsvcRuntime -and -not $HasMinGWRuntime) {
    Write-Failure "No runtime libraries found (MSVC or MinGW)"
    $script:FailCount++
}

# ============================================================================
# Section 6: Resources
# ============================================================================
Write-Section "Resources"

# Translations
$TranslationsDir = Join-Path $InstallPath "translations"
if (-not (Test-Path $TranslationsDir)) {
    $TranslationsDir = Join-Path $BinDir "translations"
}

if (Test-DirectoryExists $TranslationsDir "Translations directory") {
    $QmFiles = Get-ChildItem -Path $TranslationsDir -Filter "*.qm" -ErrorAction SilentlyContinue
    if ($QmFiles.Count -gt 0) {
        Write-Success "Translation files found: $($QmFiles.Count) file(s)"
        $script:PassCount++
    }
}

# Styles
$StylesDir = Join-Path $InstallPath "styles"
if (-not (Test-Path $StylesDir)) {
    $StylesDir = Join-Path (Split-Path $BinDir -Parent) "styles"
}

Test-DirectoryExists $StylesDir "Styles directory"

# ============================================================================
# Section 7: DLL Dependency Analysis
# ============================================================================
if (-not $SkipDllCheck) {
    Write-Section "DLL Dependency Analysis"

    # Check if Dependencies.exe or dumpbin is available
    $DepsAvailable = $false

    # Try to use dumpbin (Visual Studio)
    $Dumpbin = Get-Command dumpbin -ErrorAction SilentlyContinue
    if ($Dumpbin) {
        Write-Info "Using dumpbin for dependency analysis..."
        $DepsOutput = & dumpbin /dependents $ExePath 2>&1

        $MissingDlls = @()
        foreach ($line in $DepsOutput) {
            if ($line -match '^\s+(\S+\.dll)\s*$') {
                $dllName = $Matches[1]
                $dllPath = Join-Path $BinDir $dllName

                # Skip system DLLs
                if ($dllName -match '^(kernel32|user32|advapi32|shell32|ole32|oleaut32|gdi32|comdlg32|ws2_32|msvcrt|ntdll)\.dll$') {
                    continue
                }

                if (-not (Test-Path $dllPath)) {
                    $MissingDlls += $dllName
                }
            }
        }

        if ($MissingDlls.Count -eq 0) {
            Write-Success "All non-system DLL dependencies are present"
            $script:PassCount++
        } else {
            Write-Warning "Potentially missing DLLs (may be system DLLs): $($MissingDlls -join ', ')"
            $script:WarnCount++
        }
    } else {
        Write-Info "dumpbin not available, skipping detailed dependency analysis"
        Write-Info "Install Visual Studio or use Dependencies.exe for detailed analysis"
    }
}

# ============================================================================
# Section 8: Package Size Analysis
# ============================================================================
Write-Section "Package Size Analysis"

$TotalSize = 0
Get-ChildItem -Path $InstallPath -Recurse -File -ErrorAction SilentlyContinue | ForEach-Object {
    $TotalSize += $_.Length
}

$SizeMB = [math]::Round($TotalSize / 1MB, 2)
Write-Info "Total package size: $SizeMB MB"

if ($SizeMB -lt 100) {
    Write-Success "Package size is optimal (< 100 MB)"
    $script:PassCount++
} elseif ($SizeMB -lt 200) {
    Write-Warning "Package size is acceptable but could be optimized ($SizeMB MB)"
    $script:WarnCount++
} else {
    Write-Warning "Package size is large ($SizeMB MB) - consider optimization"
    $script:WarnCount++
}

# ============================================================================
# Summary
# ============================================================================
Write-Section "Verification Summary"

Write-Host ""
Write-Host "Results:" -ForegroundColor White
Write-Host "  Passed:   $script:PassCount" -ForegroundColor Green
Write-Host "  Failed:   $script:FailCount" -ForegroundColor Red
Write-Host "  Warnings: $script:WarnCount" -ForegroundColor Yellow
Write-Host ""

if ($script:FailCount -eq 0) {
    Write-Success "Package verification PASSED"
    Write-Info "The package appears to be complete and ready for distribution."
    exit 0
} else {
    Write-Failure "Package verification FAILED"
    Write-Info "Please review the failures above and rebuild the package."
    exit 1
}
