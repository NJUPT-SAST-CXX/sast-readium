@echo off
REM Script to update .clangd configuration file with the correct CompilationDatabase path
REM This ensures clangd can find the compilation database for different build configurations

setlocal enabledelayedexpansion

set PROJECT_ROOT=%~dp0..
set CLANGD_FILE=%PROJECT_ROOT%\.clangd
set BUILD_DIR=%~1
set PARAM2=%~2
set FORCE_MODE=false

REM Check for force parameter
if "%PARAM2%"=="--force" set FORCE_MODE=true
if "%BUILD_DIR%"=="--force" (
    set BUILD_DIR=%PARAM2%
    set FORCE_MODE=true
)

REM List of possible build directories
set BUILD_DIRS=build/Debug-MSYS2 build/Release-MSYS2 build/Debug-MSYS2-vcpkg build/Release-MSYS2-vcpkg build/Debug build/Release build/Debug-Windows build/Release-Windows

if "%BUILD_DIR%"=="--list" goto :list_dirs
if "%BUILD_DIR%"=="--auto" goto :auto_detect
if "%BUILD_DIR%"=="" goto :show_usage

REM Validate specified build directory
set COMPILE_COMMANDS_PATH=%PROJECT_ROOT%\%BUILD_DIR%\compile_commands.json
if not exist "%COMPILE_COMMANDS_PATH%" (
    echo Error: compile_commands.json not found in %BUILD_DIR%
    echo Available directories:
    call :list_valid_dirs
    exit /b 1
)

REM Check if clangd configuration is enabled (unless forced)
if "%FORCE_MODE%"=="false" (
    call :check_clangd_enabled "%BUILD_DIR%"
    if !errorlevel! neq 0 (
        echo Error: clangd configuration is disabled in CMake (ENABLE_CLANGD_CONFIG=OFF)
        echo To override this, add --force parameter:
        echo   %~nx0 %BUILD_DIR% --force
        exit /b 1
    )
)

call :update_clangd_config "%BUILD_DIR%"
echo clangd configuration updated to use: %BUILD_DIR%
exit /b 0

:auto_detect
echo Auto-detecting build directory...
set SELECTED_DIR=
for %%d in (%BUILD_DIRS%) do (
    if exist "%PROJECT_ROOT%\%%d\compile_commands.json" (
        set SELECTED_DIR=%%d
        goto :auto_found
    )
)

echo Error: No valid build directories found with compile_commands.json
echo Please run cmake configuration first, for example:
echo   cmake --preset Debug-MSYS2
exit /b 1

:auto_found
echo Auto-selected build directory: !SELECTED_DIR!
call :update_clangd_config "!SELECTED_DIR!"
echo clangd configuration updated to use: !SELECTED_DIR!
exit /b 0

:list_dirs
echo Available build directories with compile_commands.json:
call :list_valid_dirs
exit /b 0

:list_valid_dirs
set FOUND_ANY=0
for %%d in (%BUILD_DIRS%) do (
    if exist "%PROJECT_ROOT%\%%d\compile_commands.json" (
        echo   - %%d
        set FOUND_ANY=1
    )
)
if !FOUND_ANY!==0 (
    echo   No valid build directories found.
    echo   Run cmake configuration first to generate compile_commands.json
)
exit /b 0

:check_clangd_enabled
set CHECK_BUILD_DIR=%~1
set CMAKE_CACHE_PATH=%PROJECT_ROOT%\%CHECK_BUILD_DIR%\CMakeCache.txt
if exist "%CMAKE_CACHE_PATH%" (
    findstr /C:"ENABLE_CLANGD_CONFIG:BOOL=OFF" "%CMAKE_CACHE_PATH%" >nul
    if !errorlevel! equ 0 (
        exit /b 1
    )
)
exit /b 0

:update_clangd_config
set TARGET_BUILD_DIR=%~1
echo Updating .clangd configuration...
echo Target build directory: %TARGET_BUILD_DIR%

REM Check if .clangd file exists
if not exist "%CLANGD_FILE%" (
    echo .clangd file not found, creating new one...
    (
        echo CompileFlags:
        echo   CompilationDatabase: %TARGET_BUILD_DIR%
        echo   Remove:
        echo     # Remove C++20 modules flags that clangd doesn't support yet
        echo     - "-fmodules-ts"
        echo     - "-fmodule-mapper=*"
        echo     - "-fdeps-format=*"
        echo     # Remove other potentially problematic flags
        echo     - "-MD"
        echo     - "-x"
        echo.
        echo # Additional clangd configuration
        echo Index:
        echo   Background: Build
        echo.
        echo Diagnostics:
        echo   UnusedIncludes: Strict
        echo   MissingIncludes: Strict
        echo.
        echo InlayHints:
        echo   Enabled: true
        echo   ParameterNames: true
        echo   DeducedTypes: true
        echo.
        echo Hover:
        echo   ShowAKA: true
    ) > "%CLANGD_FILE%"
    echo Created new .clangd file with build directory: %TARGET_BUILD_DIR%
    exit /b 0
)

REM For simplicity, recreate the entire .clangd file with updated content
(
    echo CompileFlags:
    echo   CompilationDatabase: %TARGET_BUILD_DIR%
    echo   Remove:
    echo     # Remove C++20 modules flags that clangd doesn't support yet
    echo     - "-fmodules-ts"
    echo     - "-fmodule-mapper=*"
    echo     - "-fdeps-format=*"
    echo     # Remove other potentially problematic flags
    echo     - "-MD"
    echo     - "-x"
    echo.
    echo # Additional clangd configuration
    echo Index:
    echo   Background: Build
    echo.
    echo Diagnostics:
    echo   UnusedIncludes: Strict
    echo   MissingIncludes: Strict
    echo.
    echo InlayHints:
    echo   Enabled: true
    echo   ParameterNames: true
    echo   DeducedTypes: true
    echo.
    echo Hover:
    echo   ShowAKA: true
) > "%CLANGD_FILE%"

echo Successfully updated .clangd configuration
exit /b 0

:show_usage
echo Usage: update-clangd-config.bat [BUILD_DIR^|--auto^|--list] [--force]
echo.
echo Arguments:
echo   BUILD_DIR    Set specific build directory (e.g., 'build/Debug-MSYS2'^)
echo   --auto       Auto-detect and use the best available build directory
echo   --list       List all available build directories
echo   --force      Force update even if ENABLE_CLANGD_CONFIG is OFF
echo.
echo Examples:
echo   %~nx0 --auto
echo   %~nx0 build/Release-MSYS2
echo   %~nx0 --list
echo   %~nx0 --auto --force
exit /b 0

:VCEnd
