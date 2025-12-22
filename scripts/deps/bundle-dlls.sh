#!/bin/bash
# DLL Bundling Helper for MSYS2 Builds
# Automatically detects and bundles all required DLLs for distribution

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

# Configuration
APP_EXE="${1:-.}"
OUTPUT_DIR="${2:-.}"
VERBOSE="${3:-false}"

if [[ ! -f $APP_EXE ]]; then
    print_error "Usage: $0 <app.exe> [output_dir] [verbose]"
    print_error "Example: $0 build/Release-MSYS2/app/app.exe build/Release-MSYS2/app"
    exit 1
fi

# Get directory containing executable
if [[ $OUTPUT_DIR == "." ]]; then
    OUTPUT_DIR="$(dirname "$APP_EXE")"
fi

print_status "Bundling DLLs for: $APP_EXE"
print_status "Output directory: $OUTPUT_DIR"

# Find all DLL dependencies using ldd
find_dll_dependencies() {
    local exe="$1"
    local dlls=()

    # Use ldd to find dependencies
    if command -v ldd &>/dev/null; then
        while IFS= read -r line; do
            # Extract DLL path from ldd output
            if [[ $line =~ ^[[:space:]]*([^[:space:]]+)[[:space:]]*=\>[[:space:]]*([^[:space:]]+) ]]; then
                local dll_path="${BASH_REMATCH[2]}"
                if [[ -f $dll_path && $dll_path == *.dll ]]; then
                    dlls+=("$dll_path")
                fi
            fi
        done < <(ldd "$exe" 2>/dev/null || true)
    fi

    printf '%s\n' "${dlls[@]}"
}

# Find Qt6 plugin DLLs
find_qt_plugins() {
    local plugins_dir="${MSYSTEM_PREFIX}/lib/qt6/plugins"

    if [[ ! -d $plugins_dir ]]; then
        print_warning "Qt6 plugins directory not found"
        return 1
    fi

    # Essential plugin directories
    local plugin_dirs=(
        "platforms"
        "imageformats"
        "iconengines"
        "styles"
    )

    for plugin_dir in "${plugin_dirs[@]}"; do
        if [[ -d "$plugins_dir/$plugin_dir" ]]; then
            find "$plugins_dir/$plugin_dir" -name "*.dll" -type f
        fi
    done
}

# Copy DLL with deduplication
copy_dll() {
    local src="$1"
    local dst="$2"
    local basename=$(basename "$src")

    if [[ -f "$dst/$basename" ]]; then
        if [[ $VERBOSE == "true" ]]; then
            print_status "Already copied: $basename"
        fi
        return 0
    fi

    if cp "$src" "$dst/$basename" 2>/dev/null; then
        print_success "Copied: $basename"
        return 0
    else
        print_warning "Failed to copy: $basename"
        return 1
    fi
}

# Main bundling logic
main() {
    local copied_count=0
    local failed_count=0
    local skipped_count=0

    # Create output directory
    mkdir -p "$OUTPUT_DIR"

    print_status "Finding DLL dependencies..."

    # Get direct dependencies
    local deps=$(find_dll_dependencies "$APP_EXE")

    # Copy each dependency
    while IFS= read -r dll; do
        if [[ -n $dll && -f $dll ]]; then
            if copy_dll "$dll" "$OUTPUT_DIR"; then
                ((copied_count++))
            else
                ((failed_count++))
            fi
        fi
    done <<<"$deps"

    # Copy Qt plugins
    print_status "Finding Qt6 plugins..."
    local plugins=$(find_qt_plugins)

    while IFS= read -r plugin; do
        if [[ -n $plugin && -f $plugin ]]; then
            local plugin_dir=$(dirname "$plugin")
            local plugin_name=$(basename "$plugin_dir")
            local output_plugin_dir="$OUTPUT_DIR/$plugin_name"

            mkdir -p "$output_plugin_dir"

            if cp "$plugin" "$output_plugin_dir/" 2>/dev/null; then
                print_success "Copied plugin: $plugin_name/$(basename "$plugin")"
                ((copied_count++))
            else
                ((failed_count++))
            fi
        fi
    done <<<"$plugins"

    # Copy runtime libraries
    print_status "Copying runtime libraries..."

    local runtime_libs=(
        "libgcc_s_seh-1.dll"
        "libstdc++-6.dll"
        "libwinpthread-1.dll"
    )

    for lib in "${runtime_libs[@]}"; do
        local lib_path="${MSYSTEM_PREFIX}/bin/$lib"
        if [[ -f $lib_path ]]; then
            if copy_dll "$lib_path" "$OUTPUT_DIR"; then
                ((copied_count++))
            else
                ((failed_count++))
            fi
        else
            print_warning "Runtime library not found: $lib"
            ((skipped_count++))
        fi
    done

    # Summary
    print_status "DLL Bundling Summary"
    print_status "===================="
    print_success "Copied: $copied_count DLLs"
    if [[ $failed_count -gt 0 ]]; then
        print_warning "Failed: $failed_count DLLs"
    fi
    if [[ $skipped_count -gt 0 ]]; then
        print_warning "Skipped: $skipped_count DLLs"
    fi

    if [[ $failed_count -eq 0 ]]; then
        print_success "DLL bundling completed successfully!"
        return 0
    else
        print_warning "Some DLLs failed to copy"
        return 1
    fi
}

main "$@"
