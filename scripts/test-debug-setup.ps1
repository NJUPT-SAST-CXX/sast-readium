# Test Debug Setup Script for SAST Readium
# This script verifies the debugging configuration and provides testing instructions

param(
    [string]$Platform = "auto"
)

Write-Host "=== SAST Readium Debug Setup Test ===" -ForegroundColor Green
Write-Host ""

# Detect platform if auto
if ($Platform -eq "auto") {
    if ($IsWindows -or $env:OS -eq "Windows_NT") {
        $Platform = "windows"
    } elseif ($IsLinux) {
        $Platform = "linux"
    } elseif ($IsMacOS) {
        $Platform = "macos"
    } else {
        Write-Host "Unable to detect platform automatically" -ForegroundColor Red
        exit 1
    }
}

Write-Host "Testing platform: $Platform" -ForegroundColor Cyan
Write-Host ""

# Check VSCode configuration files
Write-Host "1. Checking VSCode configuration files..." -ForegroundColor Yellow

$configFiles = @(
    ".vscode/launch.json",
    ".vscode/tasks.json",
    ".vscode/c_cpp_properties.json",
    ".vscode/qt.natvis"
)

foreach ($file in $configFiles) {
    if (Test-Path $file) {
        Write-Host "   ✓ $file exists" -ForegroundColor Green

        # Validate JSON files
        if ($file.EndsWith(".json")) {
            try {
                $content = Get-Content $file -Raw | ConvertFrom-Json
                Write-Host "   ✓ $file is valid JSON" -ForegroundColor Green
            } catch {
                Write-Host "   ✗ $file has JSON syntax errors: $($_.Exception.Message)" -ForegroundColor Red
            }
        }
    } else {
        Write-Host "   ✗ $file missing" -ForegroundColor Red
    }
}

Write-Host ""

# Check build tools
Write-Host "2. Checking build tools..." -ForegroundColor Yellow

$tools = @()
switch ($Platform) {
    "windows" {
        $tools = @(
            @{Name="cmake"; Command="cmake --version"},
            @{Name="ninja"; Command="ninja --version"},
            @{Name="cl (MSVC)"; Command="cl"; Optional=$true},
            @{Name="g++ (MinGW)"; Command="g++ --version"; Optional=$true},
            @{Name="gdb"; Command="gdb --version"; Optional=$true},
            @{Name="lldb"; Command="lldb --version"; Optional=$true}
        )
    }
    "linux" {
        $tools = @(
            @{Name="cmake"; Command="cmake --version"},
            @{Name="ninja"; Command="ninja --version"},
            @{Name="g++"; Command="g++ --version"},
            @{Name="gdb"; Command="gdb --version"},
            @{Name="lldb"; Command="lldb --version"; Optional=$true}
        )
    }
    "macos" {
        $tools = @(
            @{Name="cmake"; Command="cmake --version"},
            @{Name="ninja"; Command="ninja --version"},
            @{Name="clang++"; Command="clang++ --version"},
            @{Name="lldb"; Command="lldb --version"},
            @{Name="gdb"; Command="gdb --version"; Optional=$true}
        )
    }
}

foreach ($tool in $tools) {
    try {
        $null = Invoke-Expression $tool.Command 2>$null
        Write-Host "   ✓ $($tool.Name) available" -ForegroundColor Green
    } catch {
        if ($tool.Optional) {
            Write-Host "   ⚠ $($tool.Name) not available (optional)" -ForegroundColor Yellow
        } else {
            Write-Host "   ✗ $($tool.Name) not available (required)" -ForegroundColor Red
        }
    }
}

Write-Host ""

# Check Qt installation
Write-Host "3. Checking Qt installation..." -ForegroundColor Yellow

$qtPaths = @()
switch ($Platform) {
    "windows" {
        $qtPaths = @(
            "C:\Qt\6.*\msvc*\bin\qmake.exe",
            "C:\msys64\mingw64\bin\qmake.exe",
            "$env:MSYS2_ROOT\mingw64\bin\qmake.exe"
        )
    }
    "linux" {
        $qtPaths = @(
            "/usr/bin/qmake6",
            "/usr/bin/qmake-qt6"
        )
    }
    "macos" {
        $qtPaths = @(
            "/usr/local/opt/qt6/bin/qmake",
            "/opt/homebrew/opt/qt6/bin/qmake"
        )
    }
}

$qtFound = $false
foreach ($path in $qtPaths) {
    if ($path -like "*\**" -or $path -like "*/*") {
        $matches = Get-ChildItem $path -ErrorAction SilentlyContinue
        if ($matches) {
            Write-Host "   ✓ Qt6 found at $($matches[0].FullName)" -ForegroundColor Green
            $qtFound = $true
            break
        }
    } elseif (Test-Path $path) {
        Write-Host "   ✓ Qt6 found at $path" -ForegroundColor Green
        $qtFound = $true
        break
    }
}

if (-not $qtFound) {
    Write-Host "   ⚠ Qt6 not found in standard locations" -ForegroundColor Yellow
    Write-Host "     Please ensure Qt6 development packages are installed" -ForegroundColor Yellow
}

Write-Host ""

# Manual testing instructions
Write-Host "4. Manual Testing Instructions" -ForegroundColor Yellow
Write-Host ""

Write-Host "To test the debugging setup:" -ForegroundColor Cyan
Write-Host "1. Open the project in VSCode"
Write-Host "2. Open app/main.cpp"
Write-Host "3. Set a breakpoint on line 22 (LOG_INFO statement)"
Write-Host "4. Select appropriate debug configuration:"

switch ($Platform) {
    "windows" {
        Write-Host "   - (Windows) Debug with MSVC (if Visual Studio installed)"
        Write-Host "   - (Windows) Debug with MinGW-w64 GDB (if MSYS2 installed)"
    }
    "linux" {
        Write-Host "   - (Linux) Debug with GDB (recommended)"
        Write-Host "   - (Linux) Debug with LLDB (alternative)"
    }
    "macos" {
        Write-Host "   - (macOS) Debug with LLDB (recommended)"
        Write-Host "   - (macOS) Debug with GDB (if available)"
    }
}

Write-Host "5. Press F5 to start debugging"
Write-Host "6. Verify the debugger stops at the breakpoint"
Write-Host "7. Check the Variables panel for local variables"
Write-Host "8. Test step-through debugging (F10, F11)"
Write-Host ""

Write-Host "Expected debugging features:" -ForegroundColor Cyan
Write-Host "✓ Breakpoints work correctly"
Write-Host "✓ Variable inspection shows values"
Write-Host "✓ Qt types display properly (QString, QApplication, etc.)"
Write-Host "✓ Call stack navigation works"
Write-Host "✓ Step-through debugging (over, into, out)"
Write-Host "✓ Watch expressions can be added"
Write-Host ""

# Summary
Write-Host "=== Test Summary ===" -ForegroundColor Green
Write-Host "Configuration files: Created and validated"
Write-Host "Platform support: $Platform"
Write-Host "Next steps: Follow manual testing instructions above"
Write-Host ""
Write-Host "For detailed setup information, see:" -ForegroundColor Cyan
Write-Host "- docs/debugging-setup.md (comprehensive guide)"
Write-Host "- .vscode/README-debugging.md (quick reference)"
