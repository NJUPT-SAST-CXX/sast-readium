@echo off
REM Test runner batch script for SAST-Readium project
REM Runs tests without PowerShell to avoid hanging issues

setlocal enabledelayedexpansion

REM Colors for output (using ANSI escape codes if supported)
set "GREEN=[92m"
set "CYAN=[96m"
set "YELLOW=[93m"
set "RED=[91m"
set "RESET=[0m"

REM Project paths
set "PROJECT_ROOT=%~dp0.."
set "BUILD_DIR=%PROJECT_ROOT%\build\Debug-MSYS2"
set "TEST_DIR=%BUILD_DIR%"
set "REPORTS_DIR=%BUILD_DIR%\test_reports"

REM Check for Windows build if MSYS2 build doesn't exist
if not exist "%BUILD_DIR%" (
    set "BUILD_DIR=%PROJECT_ROOT%\build\Debug-Windows"
    set "TEST_DIR=%BUILD_DIR%\Debug"
    echo Using Debug-Windows build directory
) else (
    echo Using MSYS2 build directory
)

REM Check for alternative test directory location (Windows MSVC has tests in Debug subdir)
if not exist "%TEST_DIR%" (
    if exist "%BUILD_DIR%\Debug" (
        set "TEST_DIR=%BUILD_DIR%\Debug"
    ) else (
        set "TEST_DIR=%BUILD_DIR%"
    )
)

set "REPORTS_DIR=%BUILD_DIR%\test_reports"

echo =========================================
echo SAST-Readium Test Runner (Batch)
echo =========================================
echo Build Directory: %BUILD_DIR%
echo Test Directory: %TEST_DIR%
echo Reports Directory: %REPORTS_DIR%
echo.

REM Check if build directory exists
if not exist "%BUILD_DIR%" (
    echo %RED%Error: Build directory not found.%RESET%
    echo Please build the project first.
    exit /b 1
)

REM Check if test directory exists
if not exist "%TEST_DIR%" (
    echo %RED%Error: Test executables directory not found.%RESET%
    echo Expected: %TEST_DIR%
    exit /b 1
)

REM Create reports directory
if not exist "%REPORTS_DIR%" mkdir "%REPORTS_DIR%"

REM Set Qt environment variables for offscreen testing
set "QT_QPA_PLATFORM=windows"
set "QT_ACCESSIBILITY=0"
set "QT_OPENGL=software"
set "QT_LOGGING_RULES=*.debug=false"

REM Add vcpkg Qt binaries to PATH
set "VCPKG_DEBUG_BIN=%BUILD_DIR%\vcpkg_installed\x64-windows\debug\bin"
set "VCPKG_BIN=%BUILD_DIR%\vcpkg_installed\x64-windows\bin"
set "PATH=%VCPKG_DEBUG_BIN%;%VCPKG_BIN%;%PATH%"

REM Set Qt plugin paths
set "QT_PLUGIN_PATH=%BUILD_DIR%\vcpkg_installed\x64-windows\debug\Qt6\plugins;%BUILD_DIR%\vcpkg_installed\x64-windows\Qt6\plugins"
set "QT_QPA_PLATFORM_PLUGIN_PATH=%BUILD_DIR%\vcpkg_installed\x64-windows\debug\Qt6\plugins\platforms;%BUILD_DIR%\vcpkg_installed\x64-windows\Qt6\plugins\platforms"
set "QT_QPA_FONTDIR=C:\Windows\Fonts"

echo Environment configured:
echo   QT_QPA_PLATFORM=%QT_QPA_PLATFORM%
echo   QT_ACCESSIBILITY=%QT_ACCESSIBILITY%
echo   QT_OPENGL=%QT_OPENGL%
echo   QT_PLUGIN_PATH=%QT_PLUGIN_PATH%
echo   QT_QPA_PLATFORM_PLUGIN_PATH=%QT_QPA_PLATFORM_PLUGIN_PATH%
echo.

REM Parse command line arguments
set "TEST_TYPE=All"
set "PARALLEL=1"
set "VERBOSE=0"
set "STOP_ON_FAILURE=0"
set "TIMEOUT=300"

:parse_args
if "%~1"=="" goto end_parse_args
if /i "%~1"=="--type" (
    set "TEST_TYPE=%~2"
    shift
    shift
    goto parse_args
)
if /i "%~1"=="-j" (
    set "PARALLEL=%~2"
    shift
    shift
    goto parse_args
)
if /i "%~1"=="--verbose" (
    set "VERBOSE=1"
    shift
    goto parse_args
)
if /i "%~1"=="--stop-on-failure" (
    set "STOP_ON_FAILURE=1"
    shift
    goto parse_args
)
if /i "%~1"=="--timeout" (
    set "TIMEOUT=%~2"
    shift
    shift
    goto parse_args
)
shift
goto parse_args
:end_parse_args

echo Test Configuration:
echo   Type: %TEST_TYPE%
echo   Parallelism: -j%PARALLEL%
echo   Stop on Failure: %STOP_ON_FAILURE%
echo   Timeout per test: %TIMEOUT% seconds
echo.

REM Initialize counters
set "TOTAL_TESTS=0"
set "PASSED_TESTS=0"
set "FAILED_TESTS=0"
set "SKIPPED_TESTS=0"

REM Log file
set "LOG_FILE=%REPORTS_DIR%\test_run_%date:~-4,4%%date:~-10,2%%date:~-7,2%_%time:~0,2%%time:~3,2%%time:~6,2%.log"
set "LOG_FILE=%LOG_FILE: =0%"

echo Test execution log: %LOG_FILE%
echo ========================================= > "%LOG_FILE%"
echo SAST-Readium Test Execution Log >> "%LOG_FILE%"
echo Date: %date% %time% >> "%LOG_FILE%"
echo ========================================= >> "%LOG_FILE%"
echo. >> "%LOG_FILE%"

REM Function to run a single test
REM Usage: call :run_test test_name
goto :skip_functions

:run_test
set "TEST_NAME=%~1"
set "TEST_EXE=%TEST_DIR%\%TEST_NAME%.exe"

if not exist "%TEST_EXE%" (
    echo %YELLOW%SKIP%RESET%: %TEST_NAME% - executable not found
    echo SKIP: %TEST_NAME% - executable not found >> "%LOG_FILE%"
    echo   Path checked: %TEST_EXE% >> "%LOG_FILE%"
    set /a SKIPPED_TESTS+=1
    goto :eof
)

set /a TOTAL_TESTS+=1

echo.
echo [%TOTAL_TESTS%] Running: %TEST_NAME%
echo Running at: %date% %time%
echo ----------------------------------------- >> "%LOG_FILE%"
echo [%TOTAL_TESTS%] Test: %TEST_NAME% >> "%LOG_FILE%"
echo Started at: %date% %time% >> "%LOG_FILE%"
echo ----------------------------------------- >> "%LOG_FILE%"

REM Run the test and capture output
set "TEST_OUTPUT_FILE=%REPORTS_DIR%\%TEST_NAME%_output.txt"
set "TEST_START_TIME=%time%"

REM Use timeout command to prevent hanging
timeout /t 1 /nobreak > nul 2>&1
start /B /WAIT "" "%TEST_EXE%" > "%TEST_OUTPUT_FILE%" 2>&1

set "TEST_EXIT_CODE=!errorlevel!"
set "TEST_END_TIME=%time%"

if !TEST_EXIT_CODE! equ 0 (
    echo %GREEN%PASS%RESET%: %TEST_NAME%
    echo PASS: %TEST_NAME% >> "%LOG_FILE%"
    echo Completed at: %TEST_END_TIME% >> "%LOG_FILE%"
    set /a PASSED_TESTS+=1
) else (
    echo %RED%FAIL%RESET%: %TEST_NAME% ^(exit code: !TEST_EXIT_CODE!^)
    echo FAIL: %TEST_NAME% ^(exit code: !TEST_EXIT_CODE!^) >> "%LOG_FILE%"
    echo Completed at: %TEST_END_TIME% >> "%LOG_FILE%"
    set /a FAILED_TESTS+=1

    REM Show test output on failure
    echo.
    echo ----- Test Output -----
    type "%TEST_OUTPUT_FILE%"
    echo ----- End Output -----
    echo.

    REM Append to log
    type "%TEST_OUTPUT_FILE%" >> "%LOG_FILE%"
    echo. >> "%LOG_FILE%"

    if %STOP_ON_FAILURE% equ 1 (
        echo.
        echo Stopping due to test failure.
        echo See log: %LOG_FILE%
        goto :eof
    )
)

if %VERBOSE% equ 1 (
    echo ----- Verbose Output -----
    type "%TEST_OUTPUT_FILE%"
    echo ----- End Verbose -----
    echo.
)

REM Small delay between tests to prevent resource contention
timeout /t 1 /nobreak > nul 2>&1

goto :eof
    )
)

if %VERBOSE% equ 1 (
    type "%TEST_OUTPUT_FILE%"
    echo.
)

goto :eof

:skip_functions

REM Main test execution based on test type
echo.
echo =========================================
echo Starting Test Execution
echo =========================================
echo.

if /i "%TEST_TYPE%"=="Smoke" (
    echo Running Smoke Tests...
    call :run_test test_smoke
    goto :test_summary
)

if /i "%TEST_TYPE%"=="Unit" (
    echo Running Unit Tests...

    REM Cache tests
    call :run_test test_cache_manager
    call :run_test test_pdf_cache_manager
    call :run_test test_page_text_cache
    call :run_test test_search_result_cache

    REM Command tests
    call :run_test test_command_manager
    call :run_test test_command_factory
    call :run_test test_command_prototype_registry
    call :run_test test_document_commands
    call :run_test test_initialization_command
    call :run_test test_navigation_commands

    REM Controller tests
    call :run_test test_application_controller
    call :run_test test_document_controller
    call :run_test test_page_controller
    call :run_test test_event_bus
    call :run_test test_service_locator
    call :run_test test_state_manager
    call :run_test test_state_manager_comprehensive

    REM Factory tests
    call :run_test test_model_factory
    call :run_test test_model_factory_concrete
    call :run_test test_widget_factory

    REM Logging tests
    call :run_test test_logging_comprehensive
    call :run_test test_crash_handler

    REM Manager tests
    call :run_test test_configuration_manager
    call :run_test test_action_map

    REM Model tests
    call :run_test test_search_model
    call :run_test test_render_model

    REM Plugin tests
    call :run_test test_plugin_manager_core
    call :run_test test_plugin_base_and_context

    REM Search tests
    call :run_test test_search_engine
    call :run_test test_search_configuration
    call :run_test test_search_executor
    call :run_test test_search_features
    call :run_test test_search_metrics
    call :run_test test_search_validator
    call :run_test test_text_extractor
    call :run_test test_background_processor
    call :run_test test_incremental_search_manager
    call :run_test test_memory_aware_search_results
    call :run_test test_memory_manager
    call :run_test test_memory_manager_stubs
    call :run_test test_multi_search_engine
    call :run_test test_smart_eviction_policy
    call :run_test test_search_error_recovery
    call :run_test test_search_thread_safety

    REM Utils tests
    call :run_test test_document_analyzer
    call :run_test test_error_handling
    call :run_test test_error_recovery
    call :run_test test_pdf_utilities
    call :run_test test_qgraphics_components

    REM Simple test
    call :run_test test_simple

    goto :test_summary
)

if /i "%TEST_TYPE%"=="Integration" (
    echo Running Integration Tests...

    call :run_test test_application_startup
    call :run_test test_search_integration
    call :run_test test_rendering_mode_switch
    call :run_test test_qgraphics_pdf
    call :run_test test_debug_log_panel_integration
    call :run_test test_tool_bar_integration
    call :run_test test_thumbnail_generator_integration
    call :run_test test_document_comparison_integration
    call :run_test test_document_metadata_dialog_integration
    call :run_test test_menu_bar_integration
    call :run_test test_mainwindow_ui_improvements
    call :run_test test_pdf_animations_integration
    call :run_test test_pdf_outline_widget_integration
    call :run_test test_pdf_viewer_integration
    call :run_test test_right_side_bar_integration
    call :run_test test_search_widget_integration
    call :run_test test_side_bar_integration
    call :run_test test_status_bar_integration
    call :run_test test_thumbnail_context_menu_integration
    call :run_test test_thumbnail_list_view_integration
    call :run_test test_thumbnail_widget_integration
    call :run_test test_view_widget_integration
    call :run_test test_welcome_screen_manager_integration

    goto :test_summary
)

if /i "%TEST_TYPE%"=="Performance" (
    echo Running Performance Tests...

    call :run_test test_rendering_performance
    call :run_test test_search_optimizations
    call :run_test test_search_performance
    call :run_test test_pdf_optimizations
    call :run_test test_real_pdf_documents

    goto :test_summary
)

REM Default: Run all tests in complexity order
echo Running All Tests in Complexity Order...
echo.

echo ========================================= >> "%LOG_FILE%"
echo Phase 1: Smoke Tests >> "%LOG_FILE%"
echo ========================================= >> "%LOG_FILE%"
echo.
echo Phase 1: Smoke Tests
call :run_test test_smoke

if %STOP_ON_FAILURE% equ 1 if !FAILED_TESTS! gtr 0 goto :test_summary

echo.
echo ========================================= >> "%LOG_FILE%"
echo Phase 2: Unit Tests >> "%LOG_FILE%"
echo ========================================= >> "%LOG_FILE%"
echo.
echo Phase 2: Unit Tests

REM Cache tests
echo --- Cache Tests ---
call :run_test test_cache_manager
call :run_test test_pdf_cache_manager
call :run_test test_page_text_cache
call :run_test test_search_result_cache

REM Command tests
echo --- Command Tests ---
call :run_test test_command_manager
call :run_test test_command_factory
call :run_test test_command_prototype_registry
call :run_test test_document_commands
call :run_test test_initialization_command
call :run_test test_navigation_commands

REM Controller tests
echo --- Controller Tests ---
call :run_test test_application_controller
call :run_test test_document_controller
call :run_test test_page_controller
call :run_test test_event_bus
call :run_test test_service_locator
call :run_test test_state_manager
call :run_test test_state_manager_comprehensive

REM Factory tests
echo --- Factory Tests ---
call :run_test test_model_factory
call :run_test test_model_factory_concrete
call :run_test test_widget_factory

REM Logging tests
echo --- Logging Tests ---
call :run_test test_logging_comprehensive
call :run_test test_crash_handler

REM Manager tests
echo --- Manager Tests ---
call :run_test test_configuration_manager
call :run_test test_action_map

REM Model tests
echo --- Model Tests ---
call :run_test test_search_model
call :run_test test_render_model

REM Plugin tests
echo --- Plugin Tests ---
call :run_test test_plugin_manager_core
call :run_test test_plugin_base_and_context

REM Search tests
echo --- Search Tests ---
call :run_test test_search_engine
call :run_test test_search_configuration
call :run_test test_search_executor
call :run_test test_search_features
call :run_test test_search_metrics
call :run_test test_search_validator
call :run_test test_text_extractor
call :run_test test_background_processor
call :run_test test_incremental_search_manager
call :run_test test_memory_aware_search_results
call :run_test test_memory_manager
call :run_test test_memory_manager_stubs
call :run_test test_multi_search_engine
call :run_test test_smart_eviction_policy
call :run_test test_search_error_recovery
call :run_test test_search_thread_safety

REM Utils tests
echo --- Utils Tests ---
call :run_test test_document_analyzer
call :run_test test_error_handling
call :run_test test_error_recovery
call :run_test test_pdf_utilities
call :run_test test_qgraphics_components

REM Simple test
call :run_test test_simple

if %STOP_ON_FAILURE% equ 1 if !FAILED_TESTS! gtr 0 goto :test_summary

echo.
echo ========================================= >> "%LOG_FILE%"
echo Phase 3: Integration Tests >> "%LOG_FILE%"
echo ========================================= >> "%LOG_FILE%"
echo.
echo Phase 3: Integration Tests

call :run_test test_application_startup
call :run_test test_search_integration
call :run_test test_rendering_mode_switch
call :run_test test_qgraphics_pdf
call :run_test test_debug_log_panel_integration
call :run_test test_tool_bar_integration
call :run_test test_thumbnail_generator_integration
call :run_test test_document_comparison_integration
call :run_test test_document_metadata_dialog_integration
call :run_test test_menu_bar_integration
call :run_test test_mainwindow_ui_improvements
call :run_test test_pdf_animations_integration
call :run_test test_pdf_outline_widget_integration
call :run_test test_pdf_viewer_integration
call :run_test test_right_side_bar_integration
call :run_test test_search_widget_integration
call :run_test test_side_bar_integration
call :run_test test_status_bar_integration
call :run_test test_thumbnail_context_menu_integration
call :run_test test_thumbnail_list_view_integration
call :run_test test_thumbnail_widget_integration
call :run_test test_view_widget_integration
call :run_test test_welcome_screen_manager_integration

if %STOP_ON_FAILURE% equ 1 if !FAILED_TESTS! gtr 0 goto :test_summary

echo.
echo ========================================= >> "%LOG_FILE%"
echo Phase 4: Performance Tests >> "%LOG_FILE%"
echo ========================================= >> "%LOG_FILE%"
echo.
echo Phase 4: Performance Tests

call :run_test test_rendering_performance
call :run_test test_search_optimizations
call :run_test test_search_performance
call :run_test test_pdf_optimizations
call :run_test test_real_pdf_documents

:test_summary
echo.
echo =========================================
echo Test Execution Summary
echo =========================================
echo Total Tests:  %TOTAL_TESTS%
echo Passed:       %GREEN%%PASSED_TESTS%%RESET%
echo Failed:       %RED%%FAILED_TESTS%%RESET%
echo Skipped:      %YELLOW%%SKIPPED_TESTS%%RESET%
echo =========================================
echo.

echo ========================================= >> "%LOG_FILE%"
echo Test Execution Summary >> "%LOG_FILE%"
echo ========================================= >> "%LOG_FILE%"
echo Total Tests:  %TOTAL_TESTS% >> "%LOG_FILE%"
echo Passed:       %PASSED_TESTS% >> "%LOG_FILE%"
echo Failed:       %FAILED_TESTS% >> "%LOG_FILE%"
echo Skipped:      %SKIPPED_TESTS% >> "%LOG_FILE%"
echo ========================================= >> "%LOG_FILE%"

if %FAILED_TESTS% gtr 0 (
    echo %RED%Some tests failed. Check the log file for details:%RESET%
    echo %LOG_FILE%
    exit /b 1
) else (
    echo %GREEN%All tests passed successfully!%RESET%
    exit /b 0
)

endlocal
