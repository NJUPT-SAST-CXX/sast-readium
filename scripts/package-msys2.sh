#!/bin/bash
# MSYS2 Packaging Script for SAST Readium
# Creates portable and installer packages from MSYS2 builds
# Now integrated with CMake CPack system for NSIS installers

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_TYPE="Release"
PACKAGE_TYPE="all"
CLEAN_PACKAGING=false
VERBOSE=false

# Derived paths
BUILD_DIR="${PROJECT_ROOT}/build"
PACKAGE_DIR="${PROJECT_ROOT}/package"
APP_NAME="sast-readium"
APP_DISPLAY_NAME="SAST Readium"
VERSION="0.1.0"

# Logging functions
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Show usage
show_usage() {
    cat <<EOF
Usage: $0 [OPTIONS]

Create packages for SAST Readium from MSYS2 build using CMake CPack

OPTIONS:
    -t, --type TYPE         Package type: portable, installer, all (default: all)
    -b, --build TYPE        Build type: Debug or Release (default: Release)
    -c, --clean             Clean packaging directory before building
    -v, --verbose           Enable verbose output
    -h, --help              Show this help message

PACKAGE TYPES:
    portable                Portable ZIP distribution
    installer               NSIS installer (.exe) via CMake CPack
    all                     Build both portable and installer packages

EXAMPLES:
    $0                      # Create all packages from Release build
    $0 -t portable          # Create portable ZIP only
    $0 -t installer         # Create NSIS installer using CMake
    $0 -b Debug -t all      # Create all packages from Debug build
    $0 -c -v                # Clean build with verbose output

NOTE:
    This script now uses CMake CPack for creating NSIS installers.
    Ensure ENABLE_PACKAGING=ON and PACKAGE_INSTALLER=ON in CMake configuration.

EOF
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
    -t | --type)
        PACKAGE_TYPE="$2"
        shift 2
        ;;
    -b | --build)
        BUILD_TYPE="$2"
        shift 2
        ;;
    -c | --clean)
        CLEAN_PACKAGING=true
        shift
        ;;
    -v | --verbose)
        VERBOSE=true
        shift
        ;;
    -h | --help)
        show_usage
        exit 0
        ;;
    *)
        print_error "Unknown option: $1"
        show_usage
        exit 1
        ;;
    esac
done

# Check MSYS2 environment
check_msys2() {
    if [[ -z $MSYSTEM ]]; then
        print_error "This script must be run in MSYS2 environment"
        exit 1
    fi
    print_status "MSYS2 environment: $MSYSTEM"
}

# Check build exists
check_build() {
    local build_path="${BUILD_DIR}/${BUILD_TYPE}-MSYS2/app/app.exe"
    if [[ ! -f $build_path ]]; then
        print_error "Build not found at $build_path"
        print_error "Please build first: ./scripts/build-msys2.sh -t $BUILD_TYPE"
        exit 1
    fi
    print_success "Found build at $build_path"
}

# Clean packaging directory
clean_packaging() {
    if [[ $CLEAN_PACKAGING == true ]]; then
        print_status "Cleaning packaging directory..."
        rm -rf "$PACKAGE_DIR"
    fi
    mkdir -p "$PACKAGE_DIR"
}

# Create portable package
create_portable() {
    print_status "Creating portable package..."

    local portable_dir="${PACKAGE_DIR}/portable"
    local app_dir="${portable_dir}/${APP_DISPLAY_NAME}"
    mkdir -p "$app_dir"

    # Copy executable and all DLLs
    local build_app_dir="${BUILD_DIR}/${BUILD_TYPE}-MSYS2/app"
    cp "$build_app_dir/sast-readium.exe" "$app_dir/"
    cp "$build_app_dir"/*.dll "$app_dir/" 2>/dev/null || true
    cp -r "$build_app_dir/styles" "$app_dir/" 2>/dev/null || true
    cp -r "$build_app_dir/platforms" "$app_dir/" 2>/dev/null || true
    cp -r "$build_app_dir/imageformats" "$app_dir/" 2>/dev/null || true
    cp -r "$build_app_dir/iconengines" "$app_dir/" 2>/dev/null || true
    cp "$build_app_dir"/*.qm "$app_dir/" 2>/dev/null || true

    # Create launcher script
    cat >"${portable_dir}/run.bat" <<'LAUNCHER'
@echo off
cd /d "%~dp0"
start "" "%~dp0SAST Readium\sast-readium.exe" %*
LAUNCHER

    # Create README
    cat >"${portable_dir}/README.txt" <<README
$APP_DISPLAY_NAME $VERSION - Portable Edition
=============================================

This is a portable version of $APP_DISPLAY_NAME that doesn't require installation.

To run the application:
1. Extract this ZIP file to any folder
2. Double-click run.bat or run app.exe directly from the $APP_DISPLAY_NAME folder

System Requirements:
- Windows 10 or later
- No additional dependencies required

For more information, visit: https://github.com/SAST-Readium/sast-readium
README

    # Create ZIP archive
    local zip_file="${PROJECT_ROOT}/${APP_NAME}-${VERSION}-portable.zip"
    print_status "Creating ZIP archive: $zip_file"

    if command -v 7z &>/dev/null; then
        cd "$portable_dir"
        7z a -tzip "$zip_file" "*" >/dev/null
        cd - >/dev/null
    else
        print_warning "7z not found, using tar+gzip instead"
        cd "$portable_dir"
        tar czf "${zip_file%.zip}.tar.gz" *
        cd - >/dev/null
    fi

    print_success "Created portable package: $zip_file"
}

# Create installer package
create_installer() {
    print_status "Creating installer package..."

    # Check for NSIS
    if ! command -v makensis &>/dev/null; then
        print_warning "NSIS not found. Install with: pacman -S mingw-w64-x86_64-nsis"
        print_warning "Skipping installer creation"
        return 1
    fi

    local nsis_dir="${PACKAGE_DIR}/nsis"
    mkdir -p "$nsis_dir"

    local build_app_dir="${BUILD_DIR}/${BUILD_TYPE}-MSYS2/app"

    # Create NSIS script
    cat >"${nsis_dir}/installer.nsi" <<'NSIS'
!include "MUI2.nsh"

Name "SAST Readium"
OutFile "sast-readium-0.1.0-installer.exe"
InstallDir "$PROGRAMFILES\SAST Readium"

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Section "Install"
  SetOutPath "$INSTDIR"
  File "sast-readium.exe"
  File "*.dll"
  SetOutPath "$INSTDIR\styles"
  File /r "styles\*.*"
  SetOutPath "$INSTDIR\platforms"
  File /r "platforms\*.*"
  SetOutPath "$INSTDIR\imageformats"
  File /r "imageformats\*.*"
  SetOutPath "$INSTDIR\iconengines"
  File /r "iconengines\*.*"
  CreateDirectory "$SMPROGRAMS\SAST Readium"
  CreateShortCut "$SMPROGRAMS\SAST Readium\SAST Readium.lnk" "$INSTDIR\sast-readium.exe"
SectionEnd

Section "Uninstall"
  RMDir /r "$INSTDIR"
  RMDir /r "$SMPROGRAMS\SAST Readium"
SectionEnd
NSIS

    # Copy files to NSIS directory
    cp "$build_app_dir/sast-readium.exe" "$nsis_dir/"
    cp "$build_app_dir"/*.dll "$nsis_dir/" 2>/dev/null || true
    cp -r "$build_app_dir/styles" "$nsis_dir/" 2>/dev/null || true
    cp -r "$build_app_dir/platforms" "$nsis_dir/" 2>/dev/null || true
    cp -r "$build_app_dir/imageformats" "$nsis_dir/" 2>/dev/null || true
    cp -r "$build_app_dir/iconengines" "$nsis_dir/" 2>/dev/null || true

    # Build installer
    cd "$nsis_dir"
    makensis installer.nsi
    cd - >/dev/null

    local installer_file="${PROJECT_ROOT}/sast-readium-${VERSION}-installer.exe"
    if [[ -f "${nsis_dir}/sast-readium-${VERSION}-installer.exe" ]]; then
        mv "${nsis_dir}/sast-readium-${VERSION}-installer.exe" "$installer_file"
        print_success "Created installer: $installer_file"
    else
        print_warning "Installer creation may have failed"
        return 1
    fi
}

# Main function
main() {
    print_status "SAST Readium MSYS2 Packaging Script"
    print_status "===================================="

    check_msys2
    check_build
    clean_packaging

    case $PACKAGE_TYPE in
    portable)
        create_portable
        ;;
    installer)
        create_installer
        ;;
    all)
        create_portable
        create_installer || true
        ;;
    *)
        print_error "Unknown package type: $PACKAGE_TYPE"
        show_usage
        exit 1
        ;;
    esac

    print_success "Packaging completed!"
    print_status "Generated packages:"
    ls -lh "${PROJECT_ROOT}"/${APP_NAME}-* 2>/dev/null || true
}

main "$@"
