#!/usr/bin/env pwsh
# Test script to verify clangd configuration is working correctly
# This script checks for common issues and validates the configuration

param(
    [switch]$Verbose
)

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ClangdFile = Join-Path $ProjectRoot ".clangd"

function Write-TestLog {
    param([string]$Message, [string]$Level = "INFO")
    $timestamp = Get-Date -Format "HH:mm:ss"
    $color = switch ($Level) {
        "PASS" { "Green" }
        "FAIL" { "Red" }
        "WARN" { "Yellow" }
        default { "White" }
    }
    Write-Host "[$timestamp] [$Level] $Message" -ForegroundColor $color
}

function Test-ClangdFile {
    Write-TestLog "Testing .clangd file..." "INFO"

    if (-not (Test-Path $ClangdFile)) {
        Write-TestLog ".clangd file not found" "FAIL"
        return $false
    }

    $content = Get-Content $ClangdFile -Raw

    # Check for CompilationDatabase
    if ($content -notmatch "CompilationDatabase:") {
        Write-TestLog "CompilationDatabase not found in .clangd" "FAIL"
        return $false
    }

    # Check for Remove section
    if ($content -notmatch "Remove:") {
        Write-TestLog "Remove section not found in .clangd" "FAIL"
        return $false
    }

    # Check for C++20 modules flags removal
    $requiredRemovals = @("-fmodules-ts", "-fmodule-mapper=", "-fdeps-format=")
    foreach ($removal in $requiredRemovals) {
        if ($content -notmatch [regex]::Escape($removal)) {
            Write-TestLog "Missing removal for $removal" "FAIL"
            return $false
        }
    }

    Write-TestLog ".clangd file is correctly configured" "PASS"
    return $true
}

function Test-CompileCommands {
    Write-TestLog "Testing compile_commands.json..." "INFO"

    # Extract build directory from .clangd
    $content = Get-Content $ClangdFile -Raw
    if ($content -match "CompilationDatabase:\s*(.+)") {
        $buildDir = $matches[1].Trim()
        $compileCommandsPath = Join-Path $ProjectRoot $buildDir "compile_commands.json"

        if (-not (Test-Path $compileCommandsPath)) {
            Write-TestLog "compile_commands.json not found at $compileCommandsPath" "FAIL"
            return $false
        }

        # Check if file is valid JSON
        try {
            $json = Get-Content $compileCommandsPath -Raw | ConvertFrom-Json
            if ($json.Count -eq 0) {
                Write-TestLog "compile_commands.json is empty" "FAIL"
                return $false
            }

            # Check for problematic flags
            $jsonString = Get-Content $compileCommandsPath -Raw
            $problematicFlags = @("-fmodules-ts", "-fmodule-mapper=", "-fdeps-format=")
            $foundProblematic = $false

            foreach ($flag in $problematicFlags) {
                if ($jsonString -match [regex]::Escape($flag)) {
                    Write-TestLog "Found problematic flag $flag in compile_commands.json (this is expected)" "INFO"
                    $foundProblematic = $true
                }
            }

            if ($foundProblematic) {
                Write-TestLog "Problematic flags found in compile_commands.json - .clangd Remove section should filter them" "INFO"
            }

            Write-TestLog "compile_commands.json is valid with $($json.Count) entries" "PASS"
            return $true
        }
        catch {
            Write-TestLog "compile_commands.json is not valid JSON: $_" "FAIL"
            return $false
        }
    }
    else {
        Write-TestLog "Could not extract build directory from .clangd" "FAIL"
        return $false
    }
}

function Test-Scripts {
    Write-TestLog "Testing configuration scripts..." "INFO"

    $scripts = @(
        "scripts/update-clangd-config.ps1",
        "scripts/update-clangd-config.bat",
        "scripts/update-clangd-config.sh"
    )

    $allExist = $true
    foreach ($script in $scripts) {
        $scriptPath = Join-Path $ProjectRoot $script
        if (-not (Test-Path $scriptPath)) {
            Write-TestLog "Script not found: $script" "FAIL"
            $allExist = $false
        }
    }

    if ($allExist) {
        Write-TestLog "All configuration scripts are present" "PASS"
        return $true
    }

    return $false
}

function Test-CMakeOption {
    Write-TestLog "Testing CMake ENABLE_CLANGD_CONFIG option..." "INFO"

    # Look for CMakeCache.txt in common build directories
    $buildDirs = @("build/Debug-MSYS2", "build/Release-MSYS2", "build/Debug", "build/Release")

    foreach ($buildDir in $buildDirs) {
        $cacheFile = Join-Path $ProjectRoot $buildDir "CMakeCache.txt"
        if (Test-Path $cacheFile) {
            $cacheContent = Get-Content $cacheFile
            $clangdOption = $cacheContent | Where-Object { $_ -match "^ENABLE_CLANGD_CONFIG:BOOL=(.+)$" }

            if ($clangdOption) {
                $enabled = $matches[1]
                Write-TestLog "Found ENABLE_CLANGD_CONFIG=$enabled in $buildDir" "INFO"

                if ($enabled -eq "ON") {
                    Write-TestLog "clangd auto-configuration is enabled" "PASS"
                } else {
                    Write-TestLog "clangd auto-configuration is disabled" "WARN"
                }
                return $true
            }
        }
    }

    Write-TestLog "No CMake cache found - run cmake configuration first" "WARN"
    return $false
}

# Run all tests
Write-TestLog "Starting clangd configuration tests..." "INFO"
Write-TestLog "Project root: $ProjectRoot" "INFO"

$tests = @(
    { Test-ClangdFile },
    { Test-CompileCommands },
    { Test-Scripts },
    { Test-CMakeOption }
)

$passed = 0
$total = $tests.Count

foreach ($test in $tests) {
    if (& $test) {
        $passed++
    }
}

Write-TestLog "Test Results: $passed/$total tests passed" "INFO"

if ($passed -eq $total) {
    Write-TestLog "All tests passed! clangd configuration is working correctly." "PASS"
    exit 0
} else {
    Write-TestLog "Some tests failed. Check the output above for details." "FAIL"
    exit 1
}
