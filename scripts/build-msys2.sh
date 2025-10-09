#!/bin/bash
# MSYS2 Build Script for SAST Readium
# This script automates the build process in MSYS2 environment

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
BUILD_TYPE="Release"
USE_VCPKG="OFF"
CLEAN_BUILD="false"
INSTALL_DEPS="false"
JOBS=$(nproc)

# Function to print colored output
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

# Function to show usage
show_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Build SAST Readium in MSYS2 environment

OPTIONS:
    -t, --type TYPE         Build type: Debug or Release (default: Release)
    -v, --vcpkg             Use vcpkg for dependencies instead of system packages
    -c, --clean             Clean build directory before building
    -d, --install-deps      Install required MSYS2 dependencies
    -j, --jobs JOBS         Number of parallel jobs (default: $(nproc))
    -h, --help              Show this help message

EXAMPLES:
    $0                      # Build release with system packages
    $0 -t Debug             # Build debug with system packages
    $0 -v                   # Build release with vcpkg
    $0 -c -d                # Clean build and install dependencies
    $0 -t Debug -v -c       # Clean debug build with vcpkg

EOF
}

# Function to check MSYS2 environment
check_msys2_env() {
    if [[ -z "$MSYSTEM" ]]; then
        print_error "This script must be run in MSYS2 environment"
        print_error "Please start MSYS2 and run this script from there"
        exit 1
    fi

    print_status "MSYS2 environment detected: $MSYSTEM"
    print_status "MSYSTEM_PREFIX: $MSYSTEM_PREFIX"
}

# Function to install MSYS2 dependencies
install_dependencies() {
    print_status "Installing MSYS2 dependencies..."

    local packages=(
        "mingw-w64-$MSYSTEM_CARCH-cmake"
        "mingw-w64-$MSYSTEM_CARCH-ninja"
        "mingw-w64-$MSYSTEM_CARCH-gcc"
        "mingw-w64-$MSYSTEM_CARCH-qt6-base"
        "mingw-w64-$MSYSTEM_CARCH-qt6-svg"
        "mingw-w64-$MSYSTEM_CARCH-qt6-tools"
        "mingw-w64-$MSYSTEM_CARCH-poppler-qt6"
        "mingw-w64-$MSYSTEM_CARCH-pkg-config"
        "git"
    )

    print_status "Updating package database..."
    pacman -Sy

    print_status "Installing packages: ${packages[*]}"
    pacman -S --needed --noconfirm "${packages[@]}"

    print_success "Dependencies installed successfully"
}

# Function to clean build directory
clean_build() {
    local build_dir="build/${BUILD_TYPE}-MSYS2"
    if [[ "$USE_VCPKG" == "ON" ]]; then
        build_dir="build/${BUILD_TYPE}-MSYS2-vcpkg"
    fi

    if [[ -d "$build_dir" ]]; then
        print_status "Cleaning build directory: $build_dir"
        rm -rf "$build_dir"
        print_success "Build directory cleaned"
    fi
}

# Function to configure CMake
configure_cmake() {
    print_status "Configuring CMake..."

    local preset="${BUILD_TYPE}-MSYS2"
    if [[ "$USE_VCPKG" == "ON" ]]; then
        preset="${BUILD_TYPE}-MSYS2-vcpkg"

        # Check if VCPKG_ROOT is set when using vcpkg
        if [[ -z "$VCPKG_ROOT" ]]; then
            print_error "VCPKG_ROOT environment variable is not set"
            print_error "Please set VCPKG_ROOT to your vcpkg installation directory"
            exit 1
        fi
        print_status "Using vcpkg from: $VCPKG_ROOT"
    fi

    print_status "Using CMake preset: $preset"
    cmake --preset="$preset"

    print_success "CMake configuration completed"
}

# Function to build the project
build_project() {
    print_status "Building project..."

    local preset="${BUILD_TYPE}-MSYS2"
    if [[ "$USE_VCPKG" == "ON" ]]; then
        preset="${BUILD_TYPE}-MSYS2-vcpkg"
    fi

    cmake --build --preset="$preset" --parallel "$JOBS"

    print_success "Build completed successfully"
}

# Function to show build information
show_build_info() {
    local build_dir="build/${BUILD_TYPE}-MSYS2"
    if [[ "$USE_VCPKG" == "ON" ]]; then
        build_dir="build/${BUILD_TYPE}-MSYS2-vcpkg"
    fi

    print_success "=== Build Information ==="
    echo "Build Type: $BUILD_TYPE"
    echo "Using vcpkg: $USE_VCPKG"
    echo "Build Directory: $build_dir"
    echo "Parallel Jobs: $JOBS"
    echo "MSYSTEM: $MSYSTEM"

    if [[ -f "$build_dir/app/app.exe" ]]; then
        echo "Executable: $build_dir/app/app.exe"
        echo "Executable size: $(du -h "$build_dir/app/app.exe" | cut -f1)"
    fi
    print_success "========================="
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--type)
            BUILD_TYPE="$2"
            if [[ "$BUILD_TYPE" != "Debug" && "$BUILD_TYPE" != "Release" ]]; then
                print_error "Invalid build type: $BUILD_TYPE. Must be Debug or Release."
                exit 1
            fi
            shift 2
            ;;
        -v|--vcpkg)
            USE_VCPKG="ON"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD="true"
            shift
            ;;
        -d|--install-deps)
            INSTALL_DEPS="true"
            shift
            ;;
        -j|--jobs)
            JOBS="$2"
            if ! [[ "$JOBS" =~ ^[0-9]+$ ]]; then
                print_error "Invalid number of jobs: $JOBS"
                exit 1
            fi
            shift 2
            ;;
        -h|--help)
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

# Main execution
main() {
    print_status "Starting MSYS2 build process..."

    # Check environment
    check_msys2_env

    # Install dependencies if requested
    if [[ "$INSTALL_DEPS" == "true" ]]; then
        install_dependencies
    fi

    # Clean build if requested
    if [[ "$CLEAN_BUILD" == "true" ]]; then
        clean_build
    fi

    # Configure and build
    configure_cmake
    build_project

    # Show build information
    show_build_info

    print_success "MSYS2 build process completed successfully!"
}

# Run main function
main "$@"
