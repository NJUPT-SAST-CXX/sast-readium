@echo off
REM Windows batch-based test runner for SAST Readium (Debug-Windows toolchain)
REM Executes tests in deterministic order without relying on PowerShell

setlocal enabledelayedexpansion

REM ---------------------------------------------------------------------------
REM Resolve project paths
REM ---------------------------------------------------------------------------
set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
set "PROJECT_ROOT=%SCRIPT_DIR%\.."
for %%I in ("%PROJECT_ROOT%") do set "PROJECT_ROOT=%%~fI"

set "BUILD_DIR=%PROJECT_ROOT%\build\Debug-Windows"
set "TEST_DIR=%BUILD_DIR%\Debug"
set "REPORTS_DIR=%BUILD_DIR%\test_reports"

if not exist "%BUILD_DIR%" (
    echo [ERROR] Debug-Windows build directory not found at:
    echo         %BUILD_DIR%
    echo Please configure and build the Debug-Windows preset before running tests.
    exit /b 1
)

if not exist "%TEST_DIR%" (
    echo [ERROR] Expected test binaries under:
    echo         %TEST_DIR%
    echo Verify the Debug configuration was built successfully.
    exit /b 1
)

if not exist "%REPORTS_DIR%" mkdir "%REPORTS_DIR%"

REM ---------------------------------------------------------------------------
REM Configure runtime environment for headless Qt execution
REM ---------------------------------------------------------------------------
set "QT_QPA_PLATFORM=offscreen"
set "QT_ACCESSIBILITY=0"
set "QT_OPENGL=software"
set "QT_LOGGING_RULES=*.debug=false"
set "QT_QPA_FONTDIR=C:\Windows\Fonts"

set "VCPKG_ROOT=%BUILD_DIR%\vcpkg_installed\x64-windows"
set "QT_PLUGIN_PATH=%VCPKG_ROOT%\debug\Qt6\plugins;%VCPKG_ROOT%\Qt6\plugins"
set "QT_QPA_PLATFORM_PLUGIN_PATH=%VCPKG_ROOT%\debug\Qt6\plugins\platforms;%VCPKG_ROOT%\Qt6\plugins\platforms"
set "PATH=%TEST_DIR%;%BUILD_DIR%;%VCPKG_ROOT%\debug\bin;%VCPKG_ROOT%\bin;%PATH%"

REM ---------------------------------------------------------------------------
REM Runtime options (defaults)
REM ---------------------------------------------------------------------------
set "STOP_ON_FAILURE=0"
set "PARALLELISM=1"
set "SELECTED_PHASE=all"

:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="--stop-on-failure" (
    set "STOP_ON_FAILURE=1"
    shift
    goto :parse_args
)
if /i "%~1"=="-j" (
    set "PARALLELISM=%~2"
    shift
    shift
    goto :parse_args
)
if /i "%~1"=="--phase" (
    set "SELECTED_PHASE=%~2"
    shift
    shift
    goto :parse_args
)
shift
goto :parse_args
:args_done

if "%PARALLELISM%"=="" set "PARALLELISM=1"

echo ==========================================================
echo SAST Readium Test Runner ^(Debug-Windows, serial^-friendly^)
echo ==========================================================
echo Build root : %BUILD_DIR%
echo Test bin   : %TEST_DIR%
echo Reports    : %REPORTS_DIR%
echo Parallelism: %PARALLELISM% ^(executed sequentially; equivalent to -j1^)
echo Phase      : %SELECTED_PHASE%
echo Qt vars    : QT_QPA_PLATFORM=%QT_QPA_PLATFORM%, QT_OPENGL=%QT_OPENGL%
echo ==========================================================
echo.

REM Timestamp used for report naming (remove forbidden characters)
set "RUN_ID=%date:~-4,4%%date:~-10,2%%date:~-7,2%_%time:~0,2%%time:~3,2%%time:~6,2%"
set "RUN_ID=%RUN_ID: =0%"
set "RUN_ID=%RUN_ID::=%"
set "RUN_ID=%RUN_ID:/=%"

set "MASTER_LOG=%REPORTS_DIR%\test_run_%RUN_ID%.log"
echo Test run started at %date% %time% > "%MASTER_LOG%"
echo STOP_ON_FAILURE=%STOP_ON_FAILURE% >> "%MASTER_LOG%"
echo.

REM ---------------------------------------------------------------------------
REM Define test phases (lists mirror scripts/run_tests.bat)
REM ---------------------------------------------------------------------------
set "SMOKE_TESTS=test_smoke"

set "UNIT_TESTS=test_cache_manager test_pdf_cache_manager test_page_text_cache test_search_result_cache"
set "UNIT_TESTS=%UNIT_TESTS% test_command_manager test_command_factory test_command_prototype_registry test_document_commands"
set "UNIT_TESTS=%UNIT_TESTS% test_initialization_command test_navigation_commands"
set "UNIT_TESTS=%UNIT_TESTS% test_application_controller test_document_controller test_page_controller"
set "UNIT_TESTS=%UNIT_TESTS% test_event_bus test_service_locator test_state_manager test_state_manager_comprehensive"
set "UNIT_TESTS=%UNIT_TESTS% test_model_factory test_model_factory_concrete test_widget_factory"
set "UNIT_TESTS=%UNIT_TESTS% test_logging_comprehensive test_crash_handler"
set "UNIT_TESTS=%UNIT_TESTS% test_configuration_manager test_action_map"
set "UNIT_TESTS=%UNIT_TESTS% test_search_model test_render_model"
set "UNIT_TESTS=%UNIT_TESTS% test_plugin_manager_core test_plugin_base_and_context"
set "UNIT_TESTS=%UNIT_TESTS% test_search_engine test_search_configuration test_search_executor test_search_features"
set "UNIT_TESTS=%UNIT_TESTS% test_search_metrics test_search_validator test_text_extractor"
set "UNIT_TESTS=%UNIT_TESTS% test_background_processor test_incremental_search_manager"
set "UNIT_TESTS=%UNIT_TESTS% test_memory_aware_search_results test_memory_manager test_memory_manager_stubs"
set "UNIT_TESTS=%UNIT_TESTS% test_multi_search_engine test_smart_eviction_policy"
set "UNIT_TESTS=%UNIT_TESTS% test_search_error_recovery test_search_thread_safety"
set "UNIT_TESTS=%UNIT_TESTS% test_document_analyzer test_error_handling test_error_recovery"
set "UNIT_TESTS=%UNIT_TESTS% test_pdf_utilities test_qgraphics_components test_simple"

set "INTEGRATION_TESTS=test_application_startup test_search_integration test_rendering_mode_switch test_qgraphics_pdf"
set "INTEGRATION_TESTS=%INTEGRATION_TESTS% test_debug_log_panel_integration test_tool_bar_integration"
set "INTEGRATION_TESTS=%INTEGRATION_TESTS% test_thumbnail_generator_integration test_document_comparison_integration"
set "INTEGRATION_TESTS=%INTEGRATION_TESTS% test_document_metadata_dialog_integration test_menu_bar_integration"
set "INTEGRATION_TESTS=%INTEGRATION_TESTS% test_mainwindow_ui_improvements test_pdf_animations_integration"
set "INTEGRATION_TESTS=%INTEGRATION_TESTS% test_pdf_outline_widget_integration test_pdf_viewer_integration"
set "INTEGRATION_TESTS=%INTEGRATION_TESTS% test_right_side_bar_integration test_search_widget_integration"
set "INTEGRATION_TESTS=%INTEGRATION_TESTS% test_side_bar_integration test_status_bar_integration"
set "INTEGRATION_TESTS=%INTEGRATION_TESTS% test_thumbnail_context_menu_integration"
set "INTEGRATION_TESTS=%INTEGRATION_TESTS% test_thumbnail_list_view_integration"
set "INTEGRATION_TESTS=%INTEGRATION_TESTS% test_thumbnail_widget_integration"
set "INTEGRATION_TESTS=%INTEGRATION_TESTS% test_view_widget_integration"
set "INTEGRATION_TESTS=%INTEGRATION_TESTS% test_welcome_screen_manager_integration"

set "PERFORMANCE_TESTS=test_rendering_performance test_search_optimizations"
set "PERFORMANCE_TESTS=%PERFORMANCE_TESTS% test_search_performance test_pdf_optimizations test_real_pdf_documents"

REM Counters
set "TOTAL_TESTS=0"
set "PASSED_TESTS=0"
set "FAILED_TESTS=0"
set "SKIPPED_TESTS=0"

REM ---------------------------------------------------------------------------
REM Execute selected phases
REM ---------------------------------------------------------------------------
call :run_phase smoke "%SMOKE_TESTS%"
if %FAILED_TESTS% gtr 0 (
    if %STOP_ON_FAILURE% equ 1 goto :summary
)
if /i "%SELECTED_PHASE%"=="smoke" goto :summary

if /i "%SELECTED_PHASE%"=="unit" goto :ph_unit_only
if /i "%SELECTED_PHASE%"=="integration" goto :ph_integration_only
if /i "%SELECTED_PHASE%"=="performance" goto :ph_performance_only
if /i "%SELECTED_PHASE%"=="all" goto :ph_all

echo [WARN] Unknown phase "%SELECTED_PHASE%" specified; executing all phases.

:ph_all
call :run_phase unit "%UNIT_TESTS%"
if %FAILED_TESTS% gtr 0 (
    if %STOP_ON_FAILURE% equ 1 goto :summary
)
call :run_phase integration "%INTEGRATION_TESTS%"
if %FAILED_TESTS% gtr 0 (
    if %STOP_ON_FAILURE% equ 1 goto :summary
)
call :run_phase performance "%PERFORMANCE_TESTS%"
goto :summary

:ph_unit_only
call :run_phase unit "%UNIT_TESTS%"
goto :summary

:ph_integration_only
call :run_phase integration "%INTEGRATION_TESTS%"
goto :summary

:ph_performance_only
call :run_phase performance "%PERFORMANCE_TESTS%"

:summary
echo.
echo ==========================================================
echo Test Summary
echo ==========================================================
echo Total   : %TOTAL_TESTS%
echo Passed  : %PASSED_TESTS%
echo Failed  : %FAILED_TESTS%
echo Skipped : %SKIPPED_TESTS%
echo Reports : %MASTER_LOG%
echo ==========================================================

echo. >> "%MASTER_LOG%"
echo Summary >> "%MASTER_LOG%"
echo Total=%TOTAL_TESTS% Passed=%PASSED_TESTS% Failed=%FAILED_TESTS% Skipped=%SKIPPED_TESTS% >> "%MASTER_LOG%"

if %FAILED_TESTS% gtr 0 (
    echo Some tests failed. Review individual logs under %REPORTS_DIR%.
    exit /b 1
)

echo All requested tests passed.
exit /b 0

REM ---------------------------------------------------------------------------
REM Helper routines
REM ---------------------------------------------------------------------------
:run_phase
set "PHASE=%~1"
set "TEST_LIST=%~2"

if /i "%SELECTED_PHASE%" NEQ "all" if /i "%SELECTED_PHASE%" NEQ "%PHASE%" (
    if /i "%PHASE%" NEQ "smoke" goto :eof
)

echo.
echo --- %PHASE:~0,1%%PHASE:~1% tests ---
echo.

for %%T in (%TEST_LIST%) do (
    call :run_test "%%~T"
    if %FAILED_TESTS% gtr 0 (
        if %STOP_ON_FAILURE% equ 1 goto :after_loop
    )
)

:after_loop
exit /b 0

:run_test
set "TEST_NAME=%~1"
set "TEST_NAME=%TEST_NAME:"=%"
set "TEST_EXE=%TEST_DIR%\%TEST_NAME%.exe"
set "TEST_LOG=%REPORTS_DIR%\%TEST_NAME%_%RUN_ID%.log"

echo [TRACE] candidate executable: [%TEST_EXE%]
echo [RUN ] %TEST_NAME%
echo [%date% %time%] START %TEST_NAME% >> "%MASTER_LOG%"
echo Command: "%TEST_EXE%" >> "%MASTER_LOG%"

"%TEST_EXE%" > "%TEST_LOG%" 2>&1
set "EXIT_CODE=!errorlevel!"

if !EXIT_CODE! EQU 9009 (
    goto :test_missing
)

set /a TOTAL_TESTS+=1

if !EXIT_CODE! EQU 0 (
    goto :test_pass
)

goto :test_fail

:test_pass
echo [PASS] %TEST_NAME%
echo [%date% %time%] PASS  %TEST_NAME% >> "%MASTER_LOG%"
set /a PASSED_TESTS+=1
type "%TEST_LOG%" >> "%MASTER_LOG%"
echo. >> "%MASTER_LOG%"
goto :eof

:test_fail
echo [FAIL] %TEST_NAME% (exit code !EXIT_CODE!)
echo [%date% %time%] FAIL  %TEST_NAME% (exit !EXIT_CODE!) >> "%MASTER_LOG%"
set /a FAILED_TESTS+=1
type "%TEST_LOG%" >> "%MASTER_LOG%"
echo. >> "%MASTER_LOG%"
goto :eof

:test_missing
echo [SKIP] %TEST_NAME% (executable missing)
echo %TEST_NAME%: SKIP - executable missing (exit 9009) >> "%MASTER_LOG%"
set /a SKIPPED_TESTS+=1
echo --- Missing executable diagnostics --- >> "%MASTER_LOG%"
type "%TEST_LOG%" >> "%MASTER_LOG%"
echo --- End diagnostics --- >> "%MASTER_LOG%"
goto :eof

echo ----- Output: %TEST_NAME% -----
type "%TEST_LOG%"
echo ----- End Output -----

goto :eof

endlocal
