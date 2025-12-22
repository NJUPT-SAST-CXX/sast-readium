#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Analyze DLL dependencies for SAST Readium executable.

.DESCRIPTION
    This script analyzes the DLL dependencies of the SAST Readium executable
    and identifies missing or unnecessary dependencies. It helps optimize
    the packaging process by ensuring only required DLLs are included.

.PARAMETER ExePath
    Path to the app.exe executable to analyze.

.PARAMETER OutputFormat
    Output format: 'text' (default), 'json', or 'csv'.

.PARAMETER CheckMissing
    Check for missing dependencies in the same directory.

.PARAMETER ShowSystemDlls
    Include system DLLs in the output.

.EXAMPLE
    .\analyze-dependencies.ps1 -ExePath ".\build\app.exe"

.EXAMPLE
    .\analyze-dependencies.ps1 -ExePath ".\build\app.exe" -CheckMissing -OutputFormat json
#>

param(
    [Parameter(Mandatory=$true)]
    [string]$ExePath,

    [Parameter(Mandatory=$false)]
    [ValidateSet('text', 'json', 'csv')]
    [string]$OutputFormat = 'text',

    [switch]$CheckMissing = $false,

    [switch]$ShowSystemDlls = $false
)

# Verify executable exists
if (-not (Test-Path $ExePath)) {
    Write-Error "Executable not found: $ExePath"
    exit 1
}

$ExePath = Resolve-Path $ExePath
$ExeDir = Split-Path $ExePath -Parent

Write-Host "Analyzing dependencies for: $ExePath" -ForegroundColor Cyan
Write-Host "Executable directory: $ExeDir`n" -ForegroundColor Cyan

# System DLL patterns (Windows system libraries)
$SystemDllPatterns = @(
    '^kernel32\.dll$',
    '^user32\.dll$',
    '^advapi32\.dll$',
    '^shell32\.dll$',
    '^ole32\.dll$',
    '^oleaut32\.dll$',
    '^gdi32\.dll$',
    '^comdlg32\.dll$',
    '^ws2_32\.dll$',
    '^msvcrt\.dll$',
    '^ntdll\.dll$',
    '^comctl32\.dll$',
    '^shlwapi\.dll$',
    '^version\.dll$',
    '^winmm\.dll$',
    '^imm32\.dll$',
    '^setupapi\.dll$',
    '^dwmapi\.dll$',
    '^uxtheme\.dll$',
    '^netapi32\.dll$',
    '^userenv\.dll$',
    '^winspool\.drv$',
    '^bcrypt\.dll$',
    '^crypt32\.dll$',
    '^secur32\.dll$',
    '^api-ms-.*\.dll$'
)

function Test-SystemDll {
    param([string]$DllName)

    foreach ($pattern in $SystemDllPatterns) {
        if ($DllName -match $pattern) {
            return $true
        }
    }
    return $false
}

function Get-DllDependencies {
    param([string]$FilePath)

    $dependencies = @()

    # Try dumpbin (Visual Studio)
    $dumpbin = Get-Command dumpbin -ErrorAction SilentlyContinue
    if ($dumpbin) {
        Write-Host "Using dumpbin for analysis..." -ForegroundColor Green

        $output = & dumpbin /dependents $FilePath 2>&1
        $inDependencies = $false

        foreach ($line in $output) {
            if ($line -match 'dependencies:') {
                $inDependencies = $true
                continue
            }

            if ($inDependencies -and $line -match '^\s+(\S+\.dll)\s*$') {
                $dependencies += $Matches[1].ToLower()
            }

            if ($line -match 'Summary') {
                break
            }
        }

        return $dependencies
    }

    # Try objdump (MinGW/MSYS2)
    $objdump = Get-Command objdump -ErrorAction SilentlyContinue
    if ($objdump) {
        Write-Host "Using objdump for analysis..." -ForegroundColor Green

        $output = & objdump -p $FilePath 2>&1

        foreach ($line in $output) {
            if ($line -match 'DLL Name:\s+(\S+\.dll)') {
                $dependencies += $Matches[1].ToLower()
            }
        }

        return $dependencies
    }

    Write-Error "No suitable tool found for dependency analysis."
    Write-Error "Please install Visual Studio (dumpbin) or MSYS2 (objdump)."
    exit 1
}

# Analyze dependencies
$allDependencies = Get-DllDependencies -FilePath $ExePath

if ($allDependencies.Count -eq 0) {
    Write-Warning "No dependencies found. This might indicate an analysis error."
    exit 1
}

Write-Host "Found $($allDependencies.Count) total dependencies`n" -ForegroundColor Green

# Categorize dependencies
$systemDlls = @()
$qt6Dlls = @()
$thirdPartyDlls = @()
$runtimeDlls = @()
$missingDlls = @()
$presentDlls = @()

foreach ($dll in $allDependencies) {
    $isSystem = Test-SystemDll -DllName $dll
    $dllPath = Join-Path $ExeDir $dll
    $exists = Test-Path $dllPath

    if ($exists) {
        $presentDlls += $dll
    } else {
        $missingDlls += $dll
    }

    if ($isSystem) {
        $systemDlls += $dll
    } elseif ($dll -match '^qt6') {
        $qt6Dlls += $dll
    } elseif ($dll -match '^(vcruntime|msvcp|ucrtbase)') {
        $runtimeDlls += $dll
    } elseif ($dll -match '^(libgcc|libstdc\+\+|libwinpthread|libgomp|libssp)') {
        $runtimeDlls += $dll
    } else {
        $thirdPartyDlls += $dll
    }
}

# Output results based on format
if ($OutputFormat -eq 'json') {
    $result = @{
        executable = $ExePath
        totalDependencies = $allDependencies.Count
        categories = @{
            system = $systemDlls
            qt6 = $qt6Dlls
            runtime = $runtimeDlls
            thirdParty = $thirdPartyDlls
        }
        status = @{
            present = $presentDlls
            missing = $missingDlls
        }
    }

    $result | ConvertTo-Json -Depth 10

} elseif ($OutputFormat -eq 'csv') {
    Write-Output "DLL,Category,Status"

    foreach ($dll in $allDependencies) {
        $category = "Unknown"
        if ($dll -in $systemDlls) { $category = "System" }
        elseif ($dll -in $qt6Dlls) { $category = "Qt6" }
        elseif ($dll -in $runtimeDlls) { $category = "Runtime" }
        elseif ($dll -in $thirdPartyDlls) { $category = "ThirdParty" }

        $status = if ($dll -in $presentDlls) { "Present" } else { "Missing" }

        Write-Output "$dll,$category,$status"
    }

} else {
    # Text format (default)

    if (-not $ShowSystemDlls) {
        Write-Host "=== Qt6 Dependencies ($($qt6Dlls.Count)) ===" -ForegroundColor Magenta
        foreach ($dll in $qt6Dlls | Sort-Object) {
            $status = if ($dll -in $presentDlls) { "✓" } else { "✗" }
            $color = if ($dll -in $presentDlls) { "Green" } else { "Red" }
            Write-Host "  $status $dll" -ForegroundColor $color
        }
        Write-Host ""

        Write-Host "=== Runtime Dependencies ($($runtimeDlls.Count)) ===" -ForegroundColor Magenta
        foreach ($dll in $runtimeDlls | Sort-Object) {
            $status = if ($dll -in $presentDlls) { "✓" } else { "✗" }
            $color = if ($dll -in $presentDlls) { "Green" } else { "Red" }
            Write-Host "  $status $dll" -ForegroundColor $color
        }
        Write-Host ""

        Write-Host "=== Third-Party Dependencies ($($thirdPartyDlls.Count)) ===" -ForegroundColor Magenta
        foreach ($dll in $thirdPartyDlls | Sort-Object) {
            $status = if ($dll -in $presentDlls) { "✓" } else { "✗" }
            $color = if ($dll -in $presentDlls) { "Green" } else { "Red" }
            Write-Host "  $status $dll" -ForegroundColor $color
        }
        Write-Host ""

        Write-Host "=== System Dependencies ($($systemDlls.Count)) ===" -ForegroundColor Magenta
        Write-Host "  (System DLLs are provided by Windows - not shown)" -ForegroundColor Gray
        Write-Host "  Use -ShowSystemDlls to display them" -ForegroundColor Gray
        Write-Host ""
    } else {
        # Show all categories including system DLLs
        Write-Host "=== All Dependencies ===" -ForegroundColor Magenta

        $categories = @{
            "Qt6" = $qt6Dlls
            "Runtime" = $runtimeDlls
            "Third-Party" = $thirdPartyDlls
            "System" = $systemDlls
        }

        foreach ($category in $categories.Keys) {
            Write-Host "`n--- $category ($($categories[$category].Count)) ---" -ForegroundColor Yellow
            foreach ($dll in $categories[$category] | Sort-Object) {
                $status = if ($dll -in $presentDlls) { "✓" } else { "✗" }
                $color = if ($dll -in $presentDlls) { "Green" } else { "Red" }
                Write-Host "  $status $dll" -ForegroundColor $color
            }
        }
        Write-Host ""
    }

    # Summary
    Write-Host "=== Summary ===" -ForegroundColor Cyan
    Write-Host "  Total dependencies: $($allDependencies.Count)" -ForegroundColor White
    Write-Host "  Qt6 libraries: $($qt6Dlls.Count)" -ForegroundColor White
    Write-Host "  Runtime libraries: $($runtimeDlls.Count)" -ForegroundColor White
    Write-Host "  Third-party libraries: $($thirdPartyDlls.Count)" -ForegroundColor White
    Write-Host "  System libraries: $($systemDlls.Count)" -ForegroundColor White
    Write-Host ""

    if ($CheckMissing) {
        $nonSystemMissing = $missingDlls | Where-Object { -not (Test-SystemDll -DllName $_) }

        if ($nonSystemMissing.Count -gt 0) {
            Write-Host "=== Missing Non-System Dependencies ===" -ForegroundColor Red
            foreach ($dll in $nonSystemMissing | Sort-Object) {
                Write-Host "  ✗ $dll" -ForegroundColor Red
            }
            Write-Host ""
            Write-Warning "Found $($nonSystemMissing.Count) missing non-system DLL(s)"
            Write-Warning "These DLLs must be included in the package!"
            exit 1
        } else {
            Write-Host "✓ All non-system dependencies are present" -ForegroundColor Green
            Write-Host ""
        }
    }
}

# Recommendations
if ($OutputFormat -eq 'text') {
    Write-Host "=== Recommendations ===" -ForegroundColor Cyan

    $totalNonSystemDlls = $qt6Dlls.Count + $runtimeDlls.Count + $thirdPartyDlls.Count

    if ($totalNonSystemDlls -lt 20) {
        Write-Host "  ✓ Dependency count is minimal" -ForegroundColor Green
    } elseif ($totalNonSystemDlls -lt 40) {
        Write-Host "  ⚠ Dependency count is moderate - consider optimization" -ForegroundColor Yellow
    } else {
        Write-Host "  ⚠ High dependency count - review for unnecessary libraries" -ForegroundColor Yellow
    }

    Write-Host ""
}
