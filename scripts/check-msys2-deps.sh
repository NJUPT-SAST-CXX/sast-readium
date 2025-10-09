#!/bin/bash
# MSYS2 Dependency Checker for SAST Readium
# This script checks for required dependencies and can install missing ones

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

# Function to check if a package is installed
check_package() {
    local package="$1"
    if pacman -Qi "$package" &>/dev/null; then
        return 0  # Package is installed
    else
        return 1  # Package is not installed
    fi
}

# Function to check if a command exists
check_command() {
    local cmd="$1"
    if command -v "$cmd" &>/dev/null; then
        return 0  # Command exists
    else
        return 1  # Command does not exist
    fi
}

# Function to get package version
get_package_version() {
    local package="$1"
    pacman -Qi "$package" 2>/dev/null | grep "Version" | awk '{print $3}' || echo "Not installed"
}

# Function to check MSYS2 environment
check_msys2_env() {
    print_status "Checking MSYS2 environment..."

    if [[ -z "$MSYSTEM" ]]; then
        print_error "Not running in MSYS2 environment"
        print_error "Please start MSYS2 and run this script from there"
        return 1
    fi

    print_success "MSYS2 environment: $MSYSTEM"
    print_status "MSYSTEM_PREFIX: $MSYSTEM_PREFIX"
    print_status "MSYSTEM_CARCH: $MSYSTEM_CARCH"

    return 0
}

# Function to check core build tools
check_build_tools() {
    print_status "Checking core build tools..."

    local tools=(
        "cmake:mingw-w64-$MSYSTEM_CARCH-cmake"
        "ninja:mingw-w64-$MSYSTEM_CARCH-ninja"
        "gcc:mingw-w64-$MSYSTEM_CARCH-gcc"
        "g++:mingw-w64-$MSYSTEM_CARCH-gcc"
        "pkg-config:mingw-w64-$MSYSTEM_CARCH-pkg-config"
        "git:git"
    )

    local missing_tools=()

    for tool_info in "${tools[@]}"; do
        IFS=':' read -r cmd package <<< "$tool_info"

        if check_command "$cmd"; then
            local version
            case "$cmd" in
                "cmake")
                    version=$(cmake --version | head -n1 | awk '{print $3}')
                    ;;
                "ninja")
                    version=$(ninja --version)
                    ;;
                "gcc"|"g++")
                    version=$(gcc --version | head -n1 | awk '{print $4}')
                    ;;
                "pkg-config")
                    version=$(pkg-config --version)
                    ;;
                "git")
                    version=$(git --version | awk '{print $3}')
                    ;;
                *)
                    version="unknown"
                    ;;
            esac
            print_success "$cmd: $version"
        else
            print_warning "$cmd: Not found"
            missing_tools+=("$package")
        fi
    done

    if [[ ${#missing_tools[@]} -gt 0 ]]; then
        print_warning "Missing build tools. Install with:"
        echo "pacman -S ${missing_tools[*]}"
        return 1
    fi

    return 0
}

# Function to check Qt6 dependencies
check_qt6_deps() {
    print_status "Checking Qt6 dependencies..."

    local qt_packages=(
        "mingw-w64-$MSYSTEM_CARCH-qt6-base"
        "mingw-w64-$MSYSTEM_CARCH-qt6-svg"
        "mingw-w64-$MSYSTEM_CARCH-qt6-tools"
    )

    local missing_qt=()

    for package in "${qt_packages[@]}"; do
        if check_package "$package"; then
            local version=$(get_package_version "$package")
            print_success "$package: $version"
        else
            print_warning "$package: Not installed"
            missing_qt+=("$package")
        fi
    done

    # Check Qt6 commands
    local qt_commands=(
        "qmake6"
        "moc"
        "uic"
        "rcc"
    )

    for cmd in "${qt_commands[@]}"; do
        if check_command "$cmd"; then
            print_success "$cmd: Available"
        else
            print_warning "$cmd: Not found"
        fi
    done

    if [[ ${#missing_qt[@]} -gt 0 ]]; then
        print_warning "Missing Qt6 packages. Install with:"
        echo "pacman -S ${missing_qt[*]}"
        return 1
    fi

    return 0
}

# Function to check Poppler dependencies
check_poppler_deps() {
    print_status "Checking Poppler dependencies..."

    local poppler_package="mingw-w64-$MSYSTEM_CARCH-poppler-qt6"

    if check_package "$poppler_package"; then
        local version=$(get_package_version "$poppler_package")
        print_success "$poppler_package: $version"

        # Check pkg-config for poppler-qt6
        if pkg-config --exists poppler-qt6; then
            local pc_version=$(pkg-config --modversion poppler-qt6)
            print_success "poppler-qt6 pkg-config: $pc_version"
        else
            print_warning "poppler-qt6 pkg-config not found"
        fi

        return 0
    else
        print_warning "$poppler_package: Not installed"
        print_warning "Install with: pacman -S $poppler_package"
        return 1
    fi
}

# Function to check vcpkg (optional)
check_vcpkg() {
    print_status "Checking vcpkg (optional)..."

    if [[ -n "$VCPKG_ROOT" ]]; then
        if [[ -d "$VCPKG_ROOT" ]]; then
            print_success "VCPKG_ROOT: $VCPKG_ROOT"

            if [[ -f "$VCPKG_ROOT/vcpkg.exe" ]]; then
                print_success "vcpkg executable found"
            else
                print_warning "vcpkg executable not found in VCPKG_ROOT"
            fi
        else
            print_warning "VCPKG_ROOT directory does not exist: $VCPKG_ROOT"
        fi
    else
        print_status "VCPKG_ROOT not set (using system packages)"
    fi
}

# Function to install all missing dependencies
install_all_deps() {
    print_status "Installing all required dependencies..."

    local all_packages=(
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

    print_status "Installing packages: ${all_packages[*]}"
    pacman -S --needed --noconfirm "${all_packages[@]}"

    print_success "All dependencies installed successfully"
}

# Function to show usage
show_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Check and install MSYS2 dependencies for SAST Readium

OPTIONS:
    -i, --install           Install all missing dependencies
    -h, --help              Show this help message

EXAMPLES:
    $0                      # Check dependencies only
    $0 -i                   # Check and install missing dependencies

EOF
}

# Main function
main() {
    local install_deps=false

    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -i|--install)
                install_deps=true
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

    print_status "MSYS2 Dependency Checker for SAST Readium"
    echo "=============================================="

    # Check environment
    if ! check_msys2_env; then
        exit 1
    fi

    echo

    # Install dependencies if requested
    if [[ "$install_deps" == "true" ]]; then
        install_all_deps
        echo
    fi

    # Check all dependencies
    local all_good=true

    if ! check_build_tools; then
        all_good=false
    fi

    echo

    if ! check_qt6_deps; then
        all_good=false
    fi

    echo

    if ! check_poppler_deps; then
        all_good=false
    fi

    echo

    check_vcpkg

    echo
    echo "=============================================="

    if [[ "$all_good" == "true" ]]; then
        print_success "All required dependencies are available!"
        print_status "You can now build the project with:"
        echo "  ./scripts/build-msys2.sh"
    else
        print_warning "Some dependencies are missing."
        print_status "Install missing dependencies with:"
        echo "  ./scripts/check-msys2-deps.sh -i"
        echo "Or install manually using the pacman commands shown above."
    fi
}

# Run main function
main "$@"
