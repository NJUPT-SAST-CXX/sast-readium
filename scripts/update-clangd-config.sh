#!/bin/bash
# Script to update .clangd configuration file with the correct CompilationDatabase path
# This ensures clangd can find the compilation database for different build configurations
# Supports Linux, macOS, and other Unix-like systems

set -e

# Default values
BUILD_DIR=""
AUTO_MODE=false
LIST_MODE=false
VERBOSE=false
FORCE=false

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CLANGD_FILE="$PROJECT_ROOT/.clangd"

# List of possible build directories from CMakePresets.json
POSSIBLE_BUILD_DIRS=(
    "build/Debug-MSYS2"
    "build/Release-MSYS2"
    "build/Debug-MSYS2-vcpkg"
    "build/Release-MSYS2-vcpkg"
    "build/Debug"
    "build/Release"
    "build/Debug-Windows"
    "build/Release-Windows"
)

# Logging function
log_message() {
    local level="$1"
    local message="$2"
    if [[ "$VERBOSE" == true || "$level" == "ERROR" ]]; then
        local timestamp=$(date '+%H:%M:%S')
        echo "[$timestamp] [$level] $message"
    fi
}

# Function to check if clangd configuration is enabled
check_clangd_config_enabled() {
    local build_path="$1"
    local cmake_cache_path="$build_path/CMakeCache.txt"

    if [[ -f "$cmake_cache_path" ]]; then
        if grep -q "^ENABLE_CLANGD_CONFIG:BOOL=OFF" "$cmake_cache_path"; then
            return 1  # Disabled
        fi
    fi
    return 0  # Enabled or not found (default enabled)
}

# Function to find valid build directories
find_valid_build_dirs() {
    local valid_dirs=()
    for dir in "${POSSIBLE_BUILD_DIRS[@]}"; do
        local full_path="$PROJECT_ROOT/$dir"
        local compile_commands_path="$full_path/compile_commands.json"
        if [[ -f "$compile_commands_path" ]]; then
            valid_dirs+=("$dir")
            log_message "INFO" "Found valid build directory: $dir"
        fi
    done
    echo "${valid_dirs[@]}"
}

# Function to update .clangd configuration
update_clangd_config() {
    local target_build_dir="$1"

    log_message "INFO" "Updating .clangd configuration..."
    log_message "INFO" "Target build directory: $target_build_dir"

    # Check if .clangd file exists
    if [[ ! -f "$CLANGD_FILE" ]]; then
        log_message "INFO" ".clangd file not found, creating new one..."
        cat > "$CLANGD_FILE" << EOF
CompileFlags:
  CompilationDatabase: $target_build_dir
  Remove:
    # Remove C++20 modules flags that clangd doesn't support yet
    - "-fmodules-ts"
    - "-fmodule-mapper=*"
    - "-fdeps-format=*"
    # Remove other potentially problematic flags
    - "-MD"
    - "-x"

# Additional clangd configuration
Index:
  Background: Build

Diagnostics:
  UnusedIncludes: Strict
  MissingIncludes: Strict

InlayHints:
  Enabled: true
  ParameterNames: true
  DeducedTypes: true

Hover:
  ShowAKA: true
EOF
        log_message "INFO" "Created new .clangd file with build directory: $target_build_dir"
        return
    fi

    # Update existing .clangd file
    # Use sed to replace the CompilationDatabase line
    if command -v sed >/dev/null 2>&1; then
        # Create a backup
        cp "$CLANGD_FILE" "$CLANGD_FILE.bak"

        # Replace the CompilationDatabase line
        sed -i.tmp "s|^  CompilationDatabase:.*|  CompilationDatabase: $target_build_dir|" "$CLANGD_FILE"
        rm -f "$CLANGD_FILE.tmp"

        # Check if Remove section exists, if not add it
        if ! grep -q "Remove:" "$CLANGD_FILE"; then
            # Create a temporary file with the Remove section added
            awk '
            /^  CompilationDatabase:/ {
                print $0
                print "  Remove:"
                print "    # Remove C++20 modules flags that clangd doesn'\''t support yet"
                print "    - \"-fmodules-ts\""
                print "    - \"-fmodule-mapper=*\""
                print "    - \"-fdeps-format=*\""
                print "    # Remove other potentially problematic flags"
                print "    - \"-MD\""
                print "    - \"-x\""
                next
            }
            { print }
            ' "$CLANGD_FILE" > "$CLANGD_FILE.tmp"
            mv "$CLANGD_FILE.tmp" "$CLANGD_FILE"
        fi

        log_message "INFO" "Successfully updated .clangd configuration"
    else
        log_message "ERROR" "sed command not found. Cannot update .clangd file."
        exit 1
    fi
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -d, --build-dir <path>    Set specific build directory (e.g., 'build/Debug')"
    echo "  -a, --auto                Auto-detect and use the best available build directory"
    echo "  -l, --list                List all available build directories"
    echo "  -v, --verbose             Enable verbose logging"
    echo "  -f, --force               Force update even if ENABLE_CLANGD_CONFIG is OFF"
    echo "  -h, --help                Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 --auto"
    echo "  $0 --build-dir build/Release"
    echo "  $0 --list"
    echo "  $0 --auto --force"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -a|--auto)
            AUTO_MODE=true
            shift
            ;;
        -l|--list)
            LIST_MODE=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -f|--force)
            FORCE=true
            shift
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Handle list mode
if [[ "$LIST_MODE" == true ]]; then
    echo "Available build directories with compile_commands.json:"
    valid_dirs=($(find_valid_build_dirs))
    if [[ ${#valid_dirs[@]} -eq 0 ]]; then
        echo "  No valid build directories found."
        echo "  Run cmake configuration first to generate compile_commands.json"
    else
        for dir in "${valid_dirs[@]}"; do
            echo "  - $dir"
        done
    fi
    exit 0
fi

# Handle auto mode
if [[ "$AUTO_MODE" == true ]]; then
    log_message "INFO" "Auto-detecting build directory..."
    valid_dirs=($(find_valid_build_dirs))

    if [[ ${#valid_dirs[@]} -eq 0 ]]; then
        log_message "ERROR" "No valid build directories found with compile_commands.json"
        echo "Please run cmake configuration first, for example:"
        echo "  cmake --preset Debug"
        exit 1
    fi

    # Use the first valid directory (or prefer Debug if available)
    preferred_order=("build/Debug" "build/Release" "build/Debug-MSYS2" "build/Release-MSYS2")
    selected_dir="${valid_dirs[0]}"

    for preferred in "${preferred_order[@]}"; do
        for valid in "${valid_dirs[@]}"; do
            if [[ "$valid" == "$preferred" ]]; then
                selected_dir="$preferred"
                break 2
            fi
        done
    done

    # Check if clangd configuration is enabled (unless forced)
    full_build_path="$PROJECT_ROOT/$selected_dir"
    if [[ "$FORCE" != true ]] && ! check_clangd_config_enabled "$full_build_path"; then
        log_message "ERROR" "clangd configuration is disabled in CMake (ENABLE_CLANGD_CONFIG=OFF)"
        echo "To override this, use the --force parameter:"
        echo "  $0 --auto --force"
        exit 1
    fi

    log_message "INFO" "Auto-selected build directory: $selected_dir"
    update_clangd_config "$selected_dir"
    echo "clangd configuration updated to use: $selected_dir"
    exit 0
fi

# Handle explicit build directory
if [[ -n "$BUILD_DIR" ]]; then
    full_build_path="$PROJECT_ROOT/$BUILD_DIR"
    compile_commands_path="$full_build_path/compile_commands.json"

    if [[ ! -f "$compile_commands_path" ]]; then
        log_message "ERROR" "compile_commands.json not found in: $BUILD_DIR"
        echo "Available directories:"
        valid_dirs=($(find_valid_build_dirs))
        for dir in "${valid_dirs[@]}"; do
            echo "  - $dir"
        done
        exit 1
    fi

    # Check if clangd configuration is enabled (unless forced)
    if [[ "$FORCE" != true ]] && ! check_clangd_config_enabled "$full_build_path"; then
        log_message "ERROR" "clangd configuration is disabled in CMake (ENABLE_CLANGD_CONFIG=OFF)"
        echo "To override this, use the --force parameter:"
        echo "  $0 --build-dir \"$BUILD_DIR\" --force"
        exit 1
    fi

    update_clangd_config "$BUILD_DIR"
    echo "clangd configuration updated to use: $BUILD_DIR"
    exit 0
fi

# No arguments provided, show usage
show_usage
