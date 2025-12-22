#!/usr/bin/env pwsh
# UI Testing Helper Script for sast-readium
# This script helps automate parts of the UI testing process

param(
    [switch]$SetupScreenshots,
    [switch]$LaunchApp,
    [switch]$GenerateReport,
    [switch]$Help
)

$ErrorActionPreference = "Stop"

# Configuration
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$BuildDir = Join-Path $ProjectRoot "build"
$AppExe = Join-Path $BuildDir "app.exe"
$ScreenshotDir = Join-Path $ProjectRoot "docs\screenshots\ui_testing"
$ReportFile = Join-Path $ProjectRoot "docs\UI_TEST_REPORT.md"

function Show-Help {
    Write-Host @"
UI Testing Helper Script for sast-readium

USAGE:
    .\ui_test_helper.ps1 [OPTIONS]

OPTIONS:
    -SetupScreenshots    Create screenshot directory structure
    -LaunchApp          Launch the application for testing
    -GenerateReport     Generate test report template
    -Help               Show this help message

EXAMPLES:
    # Setup screenshot directory
    .\ui_test_helper.ps1 -SetupScreenshots

    # Launch application for testing
    .\ui_test_helper.ps1 -LaunchApp

    # Generate test report template
    .\ui_test_helper.ps1 -GenerateReport

TESTING WORKFLOW:
    1. Run with -SetupScreenshots to create directory structure
    2. Run with -LaunchApp to start the application
    3. Follow the testing guide in docs/UI_TESTING_GUIDE.md
    4. Capture screenshots manually using your preferred tool
    5. Run with -GenerateReport to create report template
    6. Fill in the report with your findings

"@
}

function Setup-Screenshots {
    Write-Host "Setting up screenshot directory structure..." -ForegroundColor Cyan

    if (-not (Test-Path $ScreenshotDir)) {
        New-Item -ItemType Directory -Path $ScreenshotDir -Force | Out-Null
        Write-Host "✓ Created directory: $ScreenshotDir" -ForegroundColor Green
    } else {
        Write-Host "✓ Directory already exists: $ScreenshotDir" -ForegroundColor Yellow
    }

    # Create subdirectories for organization
    $subdirs = @(
        "01_launch_and_initial",
        "02_theme_switching",
        "03_splitter_interaction",
        "04_window_resizing",
        "05_sidebar_visibility",
        "06_language_switching",
        "07_document_loading"
    )

    foreach ($subdir in $subdirs) {
        $path = Join-Path $ScreenshotDir $subdir
        if (-not (Test-Path $path)) {
            New-Item -ItemType Directory -Path $path -Force | Out-Null
            Write-Host "  ✓ Created: $subdir" -ForegroundColor Green
        }
    }

    Write-Host "`nScreenshot directory structure ready!" -ForegroundColor Green
    Write-Host "Location: $ScreenshotDir" -ForegroundColor Cyan
    Write-Host "`nYou can now capture screenshots during testing." -ForegroundColor White
}

function Launch-Application {
    Write-Host "Launching sast-readium application..." -ForegroundColor Cyan

    # Check if application exists
    if (-not (Test-Path $AppExe)) {
        Write-Host "✗ Application not found: $AppExe" -ForegroundColor Red
        Write-Host "  Please build the application first:" -ForegroundColor Yellow
        Write-Host "    cd build" -ForegroundColor White
        Write-Host "    cmake --build . --config Release" -ForegroundColor White
        exit 1
    }

    Write-Host "✓ Found application: $AppExe" -ForegroundColor Green
    Write-Host "`nStarting application..." -ForegroundColor Cyan
    Write-Host "Follow the testing guide in docs/UI_TESTING_GUIDE.md" -ForegroundColor Yellow
    Write-Host "`nPress Ctrl+C in this window to stop monitoring (app will continue running)" -ForegroundColor Gray
    Write-Host "─────────────────────────────────────────────────────────────" -ForegroundColor Gray

    # Launch application
    $process = Start-Process -FilePath $AppExe -WorkingDirectory $BuildDir -PassThru

    Write-Host "`n✓ Application launched (PID: $($process.Id))" -ForegroundColor Green
    Write-Host "`nTesting Checklist:" -ForegroundColor Cyan
    Write-Host "  [ ] Test 1: Application Launch & Initial State" -ForegroundColor White
    Write-Host "  [ ] Test 2: Theme Switching" -ForegroundColor White
    Write-Host "  [ ] Test 3: Splitter Interaction" -ForegroundColor White
    Write-Host "  [ ] Test 4: Window Resizing" -ForegroundColor White
    Write-Host "  [ ] Test 5: Sidebar Visibility" -ForegroundColor White
    Write-Host "  [ ] Test 6: Language Switching" -ForegroundColor White
    Write-Host "  [ ] Test 7: Document Loading" -ForegroundColor White

    Write-Host "`nApplication is running. Close the application window when testing is complete." -ForegroundColor Yellow
}

function Generate-Report {
    Write-Host "Generating test report template..." -ForegroundColor Cyan

    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $reportContent = @"
# MainWindow UI Optimization - Test Report

**Test Date:** $timestamp
**Tester:** [Your Name]
**Application Version:** 0.1.0.0
**Build:** Release

---

## Executive Summary

[Provide a brief overview of the testing results]

**Overall Result:** ☐ PASS / ☐ FAIL

**Tests Passed:** __ / 7
**Issues Found:** __
**Critical Issues:** __

---

## Test Results

### Test 1: Application Launch & Initial State
**Status:** ☐ PASS / ☐ FAIL

**Verification Results:**
- [ ] Application launches without errors
- [ ] Light theme is active by default
- [ ] Splitter handles are visible (6px wide, gradient styling)
- [ ] Content area has visible spacing (4px vertical margins)
- [ ] Sidebar backgrounds are correct (#f4f5f7)
- [ ] Sidebar borders are visible (#d6d8dc)

**Screenshots:**
- 01_initial_light_theme.png

**Notes:**
[Add any observations or issues]

---

### Test 2: Theme Switching
**Status:** ☐ PASS / ☐ FAIL

**Verification Results - Dark Theme:**
- [ ] Background changes to dark (#1f1f22)
- [ ] Text changes to light (#e5e5e7)
- [ ] Splitter handles use dark gradient
- [ ] Sidebar backgrounds are dark (#27282c)
- [ ] Sidebar borders are dark (#3c3e44)
- [ ] All UI elements are readable

**Verification Results - Light Theme Return:**
- [ ] All colors revert correctly
- [ ] No visual artifacts

**Screenshots:**
- 02_dark_theme_full.png
- 03_dark_theme_splitters.png
- 04_light_theme_return.png

**Notes:**
[Add any observations or issues]

---

### Test 3: Splitter Interaction
**Status:** ☐ PASS / ☐ FAIL

**Verification Results - Light Theme:**
- [ ] Hover state: Blue gradient (#0078d4, #0066b8)
- [ ] Pressed state: Darker blue
- [ ] Cursor changes to resize cursor
- [ ] Dragging smoothly resizes panels
- [ ] Visual feedback is immediate

**Verification Results - Dark Theme:**
- [ ] Hover state: Cyan gradient (#60cdff, #4eb8e6)
- [ ] Pressed state: Darker cyan
- [ ] Same smooth interaction

**Screenshots:**
- 05_splitter_normal_light.png
- 06_splitter_hover_light.png
- 07_splitter_drag_light.png
- 08_splitter_hover_dark.png
- 09_splitter_drag_dark.png

**Notes:**
[Add any observations or issues]

---

### Test 4: Window Resizing
**Status:** ☐ PASS / ☐ FAIL

**Verification Results:**
- [ ] Sidebars can collapse when window is narrow
- [ ] Main content area always remains visible
- [ ] Splitter handles remain functional at all sizes
- [ ] Content margins remain consistent
- [ ] No layout overflow or clipping
- [ ] Stretch factors work correctly
- [ ] UI remains usable at minimum size

**Screenshots:**
- 10_window_minimum.png
- 11_window_maximum.png
- 12_window_narrow.png
- 13_window_wide.png

**Notes:**
[Add any observations or issues]

---

### Test 5: Sidebar Visibility
**Status:** ☐ PASS / ☐ FAIL

**Verification Results:**
- [ ] Sidebars hide/show smoothly
- [ ] Main content expands to fill space
- [ ] Splitter handles disappear when sidebar hidden
- [ ] Sidebar borders clearly visible
- [ ] Background color difference is subtle but clear
- [ ] No visual glitches during transitions

**Screenshots:**
- 14_both_sidebars_visible.png
- 15_left_sidebar_hidden.png
- 16_right_sidebar_hidden.png
- 17_both_sidebars_hidden.png

**Notes:**
[Add any observations or issues]

---

### Test 6: Language Switching
**Status:** ☐ PASS / ☐ FAIL

**Verification Results:**
- [ ] All menu items display in Chinese
- [ ] All UI labels display in Chinese
- [ ] Chinese text renders clearly
- [ ] Splitter styling remains unchanged
- [ ] Content spacing remains unchanged
- [ ] Visual hierarchy remains unchanged
- [ ] No layout issues with Chinese text
- [ ] Switching back to English works

**Screenshots:**
- 18_chinese_ui_full.png
- 19_chinese_menu.png
- 20_english_return.png

**Notes:**
[Add any observations or issues]

---

### Test 7: Document Loading
**Status:** ☐ PASS / ☐ FAIL

**Verification Results:**
- [ ] Document loads successfully
- [ ] 4px top margin provides separation from toolbar
- [ ] 4px bottom margin provides separation from status bar
- [ ] Document content doesn't touch edges
- [ ] Splitter interaction works with document
- [ ] Resizing doesn't cause rendering issues
- [ ] Visual hierarchy is clear

**Screenshots:**
- 21_document_loaded_light.png
- 22_document_loaded_dark.png
- 23_document_spacing_detail.png
- 24_document_splitter_interaction.png

**Notes:**
[Add any observations or issues]

---

## Issues Found

### Issue 1: [Title]
**Severity:** ☐ Critical / ☐ High / ☐ Medium / ☐ Low
**Test:** [Test number and name]
**Theme:** ☐ Light / ☐ Dark / ☐ Both

**Description:**
[Detailed description]

**Steps to Reproduce:**
1.
2.
3.

**Expected Behavior:**
[What should happen]

**Actual Behavior:**
[What actually happens]

**Screenshot:** [Filename]

---

## Recommendations

[Provide recommendations based on test results]

- [ ] Ready for release
- [ ] Needs minor fixes
- [ ] Requires significant changes
- [ ] Requires further testing

---

## Additional Notes

[Any other observations, suggestions, or comments]

---

## Appendix: Test Environment

**Operating System:** Windows 11
**Qt Version:** 6.9.2
**Compiler:** GCC 15.2.0
**Build Configuration:** Release
**Screen Resolution:** [Your resolution]
**DPI Scaling:** [Your DPI setting]

"@

    $reportContent | Out-File -FilePath $ReportFile -Encoding UTF8
    Write-Host "✓ Test report template generated: $ReportFile" -ForegroundColor Green
    Write-Host "`nFill in the report after completing your tests." -ForegroundColor Yellow
}

# Main script logic
if ($Help -or $args.Count -eq 0) {
    Show-Help
    exit 0
}

if ($SetupScreenshots) {
    Setup-Screenshots
}

if ($LaunchApp) {
    Launch-Application
}

if ($GenerateReport) {
    Generate-Report
}

Write-Host "`nDone!" -ForegroundColor Green
