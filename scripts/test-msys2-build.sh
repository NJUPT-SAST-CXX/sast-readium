#!/bin/bash
# MSYS2 Build Test Script for SAST Readium
# This script tests the MSYS2 build configuration and verifies the output

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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

# Function to check if file exists and is executable
check_executable() {
    local file="$1"
    local description="$2"

    if [[ -f "$file" ]]; then
        print_success "$description found: $file"

        # Check if it's executable
        if [[ -x "$file" ]]; then
            print_success "$description is executable"
        else
            print_warning "$description is not executable"
        fi

        # Show file size
        local size=$(du -h "$file" | cut -f1)
        print_status "File size: $size"

        return 0
    else
        print_error "$description not found: $file"
        return 1
    fi
}

# Function to test dependencies of an executable
test_dependencies() {
    local exe="$1"
    local description="$2"

    print_status "Testing dependencies for $description..."

    if command -v ldd &>/dev/null; then
        print_status "Checking shared library dependencies:"
        ldd "$exe" | head -20  # Show first 20 dependencies

        # Check for missing dependencies
        if ldd "$exe" | grep -q "not found"; then
            print_error "Missing dependencies found!"
            ldd "$exe" | grep "not found"
            return 1
        else
            print_success "All dependencies resolved"
        fi
    else
        print_warning "ldd not available, skipping dependency check"
    fi

    return 0
}

# Function to test basic executable functionality
test_executable() {
    local exe="$1"
    local description="$2"

    print_status "Testing $description functionality..."

    # Test --help flag (if supported)
    if "$exe" --help &>/dev/null; then
        print_success "$description responds to --help"
    elif "$exe" -h &>/dev/null; then
        print_success "$description responds to -h"
    else
        print_warning "$description doesn't respond to help flags (this may be normal for GUI apps)"
    fi

    # Test --version flag (if supported)
    if "$exe" --version &>/dev/null; then
        print_success "$description responds to --version"
    elif "$exe" -v &>/dev/null; then
        print_success "$description responds to -v"
    else
        print_warning "$description doesn't respond to version flags (this may be normal for GUI apps)"
    fi

    return 0
}

# Function to test a specific build configuration
test_build_config() {
    local build_type="$1"
    local use_vcpkg="$2"

    local build_dir="build/${build_type}-MSYS2"
    local config_name="${build_type} with system packages"

    if [[ "$use_vcpkg" == "true" ]]; then
        build_dir="build/${build_type}-MSYS2-vcpkg"
        config_name="${build_type} with vcpkg"
    fi

    print_status "Testing build configuration: $config_name"
    print_status "Build directory: $build_dir"

    # Check if build directory exists
    if [[ ! -d "$build_dir" ]]; then
        print_error "Build directory not found: $build_dir"
        print_status "Run the build first: ./scripts/build-msys2.sh -t $build_type $([ "$use_vcpkg" == "true" ] && echo "-v")"
        return 1
    fi

    local success=true

    # Test main executable
    local main_exe="$build_dir/app/app.exe"
    if ! check_executable "$main_exe" "Main application"; then
        success=false
    else
        if ! test_dependencies "$main_exe" "Main application"; then
            success=false
        fi
        test_executable "$main_exe" "Main application"
    fi


    # Check for Qt deployment
    local qt_libs_dir="$build_dir/app"
    if [[ -d "$qt_libs_dir" ]]; then
        local qt_dll_count=$(find "$qt_libs_dir" -name "*.dll" | wc -l)
        if [[ $qt_dll_count -gt 0 ]]; then
            print_success "Qt libraries deployed: $qt_dll_count DLLs found"
        else
            print_warning "No Qt DLLs found in app directory"
        fi
    fi

    # Check for assets
    local styles_dir="$build_dir/app/styles"
    if [[ -d "$styles_dir" ]]; then
        print_success "Styles directory found: $styles_dir"
        local style_count=$(find "$styles_dir" -type f | wc -l)
        print_status "Style files: $style_count"
    else
        print_warning "Styles directory not found: $styles_dir"
    fi

    if [[ "$success" == "true" ]]; then
        print_success "Build configuration test passed: $config_name"
    else
        print_error "Build configuration test failed: $config_name"
    fi

    return $([[ "$success" == "true" ]] && echo 0 || echo 1)
}

# Function to show usage
show_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Test MSYS2 build configurations for SAST Readium

OPTIONS:
    -t, --type TYPE         Test specific build type: Debug, Release, or all (default: all)
    -v, --vcpkg             Test vcpkg builds only
    -s, --system            Test system package builds only
    -h, --help              Show this help message

EXAMPLES:
    $0                      # Test all build configurations
    $0 -t Release           # Test only Release builds
    $0 -v                   # Test only vcpkg builds
    $0 -s -t Debug          # Test only Debug build with system packages

EOF
}

# Main function
main() {
    local test_type="all"
    local test_vcpkg="both"

    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -t|--type)
                test_type="$2"
                if [[ "$test_type" != "Debug" && "$test_type" != "Release" && "$test_type" != "all" ]]; then
                    print_error "Invalid build type: $test_type. Must be Debug, Release, or all."
                    exit 1
                fi
                shift 2
                ;;
            -v|--vcpkg)
                test_vcpkg="vcpkg"
                shift
                ;;
            -s|--system)
                test_vcpkg="system"
                shift
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

    print_status "MSYS2 Build Test for SAST Readium"
    echo "=================================="

    # Check MSYS2 environment
    if [[ -z "$MSYSTEM" ]]; then
        print_error "Not running in MSYS2 environment"
        print_error "Please start MSYS2 and run this script from there"
        exit 1
    fi

    print_status "MSYS2 environment: $MSYSTEM"
    print_status "Test type: $test_type"
    print_status "Test vcpkg: $test_vcpkg"
    echo

    local overall_success=true
    local tests_run=0
    local tests_passed=0

    # Determine which build types to test
    local build_types=()
    if [[ "$test_type" == "all" ]]; then
        build_types=("Debug" "Release")
    else
        build_types=("$test_type")
    fi

    # Determine which vcpkg modes to test
    local vcpkg_modes=()
    case "$test_vcpkg" in
        "both")
            vcpkg_modes=("false" "true")
            ;;
        "system")
            vcpkg_modes=("false")
            ;;
        "vcpkg")
            vcpkg_modes=("true")
            ;;
    esac

    # Run tests
    for build_type in "${build_types[@]}"; do
        for use_vcpkg in "${vcpkg_modes[@]}"; do
            echo
            echo "----------------------------------------"
            tests_run=$((tests_run + 1))

            if test_build_config "$build_type" "$use_vcpkg"; then
                tests_passed=$((tests_passed + 1))
            else
                overall_success=false
            fi
        done
    done

    echo
    echo "=================================="
    print_status "Test Summary:"
    echo "Tests run: $tests_run"
    echo "Tests passed: $tests_passed"
    echo "Tests failed: $((tests_run - tests_passed))"

    if [[ "$overall_success" == "true" ]]; then
        print_success "All MSYS2 build tests passed!"
        exit 0
    else
        print_error "Some MSYS2 build tests failed!"
        exit 1
    fi
}

# Run main function
main "$@"
