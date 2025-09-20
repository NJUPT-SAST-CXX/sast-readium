@echo off
REM MSYS2 Build Script for SAST Readium (Windows Batch Version)
REM This script launches the MSYS2 build process from Windows

setlocal enabledelayedexpansion

REM Default values
set "BUILD_TYPE=Release"
set "USE_VCPKG=false"
set "CLEAN_BUILD=false"
set "INSTALL_DEPS=false"
set "MSYS2_PATH=C:\msys64"
set "SCRIPT_ARGS="

REM Function to show usage
:show_usage
echo Usage: %~nx0 [OPTIONS]
echo.
echo Build SAST Readium in MSYS2 environment from Windows
echo.
echo OPTIONS:
echo     -t, --type TYPE         Build type: Debug or Release (default: Release)
echo     -v, --vcpkg             Use vcpkg for dependencies instead of system packages
echo     -c, --clean             Clean build directory before building
echo     -d, --install-deps      Install required MSYS2 dependencies
echo     -p, --msys2-path PATH   Path to MSYS2 installation (default: C:\msys64)
echo     -h, --help              Show this help message
echo.
echo EXAMPLES:
echo     %~nx0                   # Build release with system packages
echo     %~nx0 -t Debug          # Build debug with system packages
echo     %~nx0 -v                # Build release with vcpkg
echo     %~nx0 -c -d             # Clean build and install dependencies
echo.
goto :eof

REM Parse command line arguments
:parse_args
if "%~1"=="" goto :check_msys2
if "%~1"=="-t" (
    set "BUILD_TYPE=%~2"
    set "SCRIPT_ARGS=!SCRIPT_ARGS! -t %~2"
    shift
    shift
    goto :parse_args
)
if "%~1"=="--type" (
    set "BUILD_TYPE=%~2"
    set "SCRIPT_ARGS=!SCRIPT_ARGS! -t %~2"
    shift
    shift
    goto :parse_args
)
if "%~1"=="-v" (
    set "USE_VCPKG=true"
    set "SCRIPT_ARGS=!SCRIPT_ARGS! -v"
    shift
    goto :parse_args
)
if "%~1"=="--vcpkg" (
    set "USE_VCPKG=true"
    set "SCRIPT_ARGS=!SCRIPT_ARGS! -v"
    shift
    goto :parse_args
)
if "%~1"=="-c" (
    set "CLEAN_BUILD=true"
    set "SCRIPT_ARGS=!SCRIPT_ARGS! -c"
    shift
    goto :parse_args
)
if "%~1"=="--clean" (
    set "CLEAN_BUILD=true"
    set "SCRIPT_ARGS=!SCRIPT_ARGS! -c"
    shift
    goto :parse_args
)
if "%~1"=="-d" (
    set "INSTALL_DEPS=true"
    set "SCRIPT_ARGS=!SCRIPT_ARGS! -d"
    shift
    goto :parse_args
)
if "%~1"=="--install-deps" (
    set "INSTALL_DEPS=true"
    set "SCRIPT_ARGS=!SCRIPT_ARGS! -d"
    shift
    goto :parse_args
)
if "%~1"=="-p" (
    set "MSYS2_PATH=%~2"
    shift
    shift
    goto :parse_args
)
if "%~1"=="--msys2-path" (
    set "MSYS2_PATH=%~2"
    shift
    shift
    goto :parse_args
)
if "%~1"=="-h" (
    call :show_usage
    exit /b 0
)
if "%~1"=="--help" (
    call :show_usage
    exit /b 0
)
echo Error: Unknown option: %~1
call :show_usage
exit /b 1

:check_msys2
REM Check if MSYS2 is installed
if not exist "%MSYS2_PATH%\msys2_shell.cmd" (
    echo Error: MSYS2 not found at %MSYS2_PATH%
    echo Please install MSYS2 or specify the correct path with -p option
    echo Download MSYS2 from: https://www.msys2.org/
    exit /b 1
)

echo MSYS2 found at: %MSYS2_PATH%

REM Get the directory of this script
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."

REM Convert Windows path to MSYS2 path
set "MSYS2_PROJECT_ROOT=%PROJECT_ROOT:\=/%"
set "MSYS2_PROJECT_ROOT=%MSYS2_PROJECT_ROOT:C:=/c%"
set "MSYS2_PROJECT_ROOT=%MSYS2_PROJECT_ROOT:D:=/d%"
set "MSYS2_PROJECT_ROOT=%MSYS2_PROJECT_ROOT:E:=/e%"
set "MSYS2_PROJECT_ROOT=%MSYS2_PROJECT_ROOT:F:=/f%"

echo Starting MSYS2 build process...
echo Build Type: %BUILD_TYPE%
echo Use vcpkg: %USE_VCPKG%
echo Clean Build: %CLEAN_BUILD%
echo Install Dependencies: %INSTALL_DEPS%
echo Project Root: %PROJECT_ROOT%
echo.

REM Launch MSYS2 with the build script
"%MSYS2_PATH%\msys2_shell.cmd" -mingw64 -defterm -no-start -here -c "cd '%MSYS2_PROJECT_ROOT%' && chmod +x scripts/build-msys2.sh && ./scripts/build-msys2.sh %SCRIPT_ARGS%"

if %ERRORLEVEL% equ 0 (
    echo.
    echo Build completed successfully!
    echo.
    echo You can find the built application in:
    if "%USE_VCPKG%"=="true" (
        echo   %PROJECT_ROOT%\build\%BUILD_TYPE%-MSYS2-vcpkg\app\app.exe
    ) else (
        echo   %PROJECT_ROOT%\build\%BUILD_TYPE%-MSYS2\app\app.exe
    )
) else (
    echo.
    echo Build failed with error code: %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

goto :eof

REM Main execution starts here
call :parse_args %*
exit /b %ERRORLEVEL%
