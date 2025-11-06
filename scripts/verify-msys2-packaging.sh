#!/bin/bash
# Verify MSYS2 environment for packaging
# Checks Qt6 installation, DLL locations, and packaging tools

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Logging
print_status() { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Check MSYS2 environment
check_msys2_env() {
    print_status "Checking MSYS2 environment..."

    if [[ -z $MSYSTEM ]]; then
        print_error "Not in MSYS2 environment"
        return 1
    fi

    print_success "MSYSTEM: $MSYSTEM"
    print_success "MSYSTEM_PREFIX: $MSYSTEM_PREFIX"
    print_success "MSYSTEM_CARCH: $MSYSTEM_CARCH"
    return 0
}

# Check Qt6 installation
check_qt6_installation() {
    print_status "Checking Qt6 installation..."

    local qt_bin="${MSYSTEM_PREFIX}/bin"
    local qt_lib="${MSYSTEM_PREFIX}/lib"
    local qt_cmake="${MSYSTEM_PREFIX}/lib/cmake/Qt6"

    if [[ ! -d $qt_cmake ]]; then
        print_error "Qt6 CMake files not found at $qt_cmake"
        print_warning "Install with: pacman -S mingw-w64-$MSYSTEM_CARCH-qt6-base"
        return 1
    fi

    print_success "Qt6 CMake files found at $qt_cmake"

    # Check for key Qt6 DLLs
    local qt_dlls=(
        "Qt6Core.dll"
        "Qt6Gui.dll"
        "Qt6Widgets.dll"
        "Qt6Svg.dll"
    )

    for dll in "${qt_dlls[@]}"; do
        if [[ -f "$qt_bin/$dll" ]]; then
            print_success "Found $dll"
        else
            print_warning "Missing $dll"
        fi
    done

    return 0
}

# Check Qt6 plugins
check_qt6_plugins() {
    print_status "Checking Qt6 plugins..."

    local plugins_dir="${MSYSTEM_PREFIX}/lib/qt6/plugins"

    if [[ ! -d $plugins_dir ]]; then
        print_error "Qt6 plugins directory not found at $plugins_dir"
        return 1
    fi

    print_success "Qt6 plugins directory found"

    # Check for essential plugins
    local plugin_dirs=(
        "platforms"
        "imageformats"
        "iconengines"
        "styles"
    )

    for plugin_dir in "${plugin_dirs[@]}"; do
        if [[ -d "$plugins_dir/$plugin_dir" ]]; then
            local count=$(ls -1 "$plugins_dir/$plugin_dir"/*.dll 2>/dev/null | wc -l)
            print_success "Found $plugin_dir: $count plugins"
        else
            print_warning "Missing plugin directory: $plugin_dir"
        fi
    done

    return 0
}

# Check windeployqt
check_windeployqt() {
    print_status "Checking windeployqt..."

    if command -v windeployqt &>/dev/null; then
        local version=$(windeployqt --version 2>&1 || echo "unknown")
        print_success "windeployqt found: $version"
        return 0
    else
        print_warning "windeployqt not found"
        print_warning "Install with: pacman -S mingw-w64-$MSYSTEM_CARCH-qt6-tools"
        return 1
    fi
}

# Check runtime dependencies
check_runtime_deps() {
    print_status "Checking runtime dependencies..."

    local runtime_libs=(
        "libgcc_s_seh-1.dll"
        "libstdc++-6.dll"
        "libwinpthread-1.dll"
    )

    for lib in "${runtime_libs[@]}"; do
        if [[ -f "${MSYSTEM_PREFIX}/bin/$lib" ]]; then
            print_success "Found $lib"
        else
            print_warning "Missing $lib"
        fi
    done

    return 0
}

# Check packaging tools
check_packaging_tools() {
    print_status "Checking packaging tools..."

    # Check for 7z
    if command -v 7z &>/dev/null; then
        print_success "7z found (for ZIP creation)"
    else
        print_warning "7z not found (will use tar instead)"
    fi

    # Check for NSIS
    if command -v makensis &>/dev/null; then
        print_success "NSIS found (for installer creation)"
    else
        print_warning "NSIS not found (installer creation will be skipped)"
        print_warning "Install with: pacman -S mingw-w64-$MSYSTEM_CARCH-nsis"
    fi

    return 0
}

# Check CMake
check_cmake() {
    print_status "Checking CMake..."

    if command -v cmake &>/dev/null; then
        local version=$(cmake --version | head -1)
        print_success "$version"
    else
        print_error "CMake not found"
        print_warning "Install with: pacman -S mingw-w64-$MSYSTEM_CARCH-cmake"
        return 1
    fi

    return 0
}

# Check Ninja
check_ninja() {
    print_status "Checking Ninja..."

    if command -v ninja &>/dev/null; then
        local version=$(ninja --version)
        print_success "Ninja found: $version"
    else
        print_error "Ninja not found"
        print_warning "Install with: pacman -S mingw-w64-$MSYSTEM_CARCH-ninja"
        return 1
    fi

    return 0
}

# Generate summary
generate_summary() {
    print_status "Generating environment summary..."

    cat >"${PROJECT_ROOT}/MSYS2_PACKAGING_ENV.txt" <<EOF
MSYS2 Packaging Environment Summary
===================================

Generated: $(date)

Environment:
  MSYSTEM: $MSYSTEM
  MSYSTEM_PREFIX: $MSYSTEM_PREFIX
  MSYSTEM_CARCH: $MSYSTEM_CARCH

Qt6 Installation:
  CMake: $MSYSTEM_PREFIX/lib/cmake/Qt6
  Plugins: $MSYSTEM_PREFIX/lib/qt6/plugins
  Binaries: $MSYSTEM_PREFIX/bin

Build Tools:
  CMake: $(cmake --version | head -1)
  Ninja: $(ninja --version 2>/dev/null || echo "NOT FOUND")
  GCC: $(gcc --version | head -1)

Packaging Tools:
  7z: $(7z --version 2>/dev/null | head -1 || echo "NOT FOUND")
  NSIS: $(makensis -VERSION 2>/dev/null || echo "NOT FOUND")
  windeployqt: $(windeployqt --version 2>/dev/null || echo "NOT FOUND")

For more information, see: docs/setup/msys2-build.md
EOF

    print_success "Summary saved to MSYS2_PACKAGING_ENV.txt"
}

# Main
main() {
    print_status "MSYS2 Packaging Environment Verification"
    print_status "========================================="

    local all_ok=true

    check_msys2_env || all_ok=false
    check_cmake || all_ok=false
    check_ninja || all_ok=false
    check_qt6_installation || all_ok=false
    check_qt6_plugins || all_ok=false
    check_windeployqt || all_ok=false
    check_runtime_deps || all_ok=false
    check_packaging_tools || all_ok=false

    generate_summary

    if [[ $all_ok == true ]]; then
        print_success "All checks passed! Ready for packaging."
        return 0
    else
        print_warning "Some checks failed. See above for details."
        return 1
    fi
}

main "$@"
