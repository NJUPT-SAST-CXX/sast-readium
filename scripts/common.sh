#!/usr/bin/env bash
# =============================================================================
# SAST Readium - Common Bash Utilities
# =============================================================================
# Shared functions for bash scripts in the scripts directory.
# Source this file at the beginning of your script:
#   source "$(dirname "${BASH_SOURCE[0]}")/common.sh"
# =============================================================================

# Colors for output
export COLOR_RED='\033[0;31m'
export COLOR_GREEN='\033[0;32m'
export COLOR_YELLOW='\033[1;33m'
export COLOR_BLUE='\033[0;34m'
export COLOR_CYAN='\033[0;36m'
export COLOR_MAGENTA='\033[0;35m'
export COLOR_NC='\033[0m' # No Color

# =============================================================================
# Logging Functions
# =============================================================================

log_success() { echo -e "${COLOR_GREEN}✓ $1${COLOR_NC}"; }
log_info() { echo -e "${COLOR_CYAN}ℹ $1${COLOR_NC}"; }
log_warning() { echo -e "${COLOR_YELLOW}⚠ $1${COLOR_NC}"; }
log_error() { echo -e "${COLOR_RED}✗ $1${COLOR_NC}"; }
log_debug() { [[ ${VERBOSE:-0} -eq 1 ]] && echo -e "${COLOR_MAGENTA}[DEBUG] $1${COLOR_NC}"; }

print_header() {
    echo ""
    echo -e "${COLOR_CYAN}=== $1 ===${COLOR_NC}"
}

print_section() {
    echo ""
    echo -e "${COLOR_BLUE}--- $1 ---${COLOR_NC}"
}

# =============================================================================
# Path Detection Functions
# =============================================================================

# Detect project root directory
detect_project_root() {
    local script_dir="$1"
    if [[ -z $script_dir ]]; then
        script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    fi

    # Navigate up from scripts directory to find project root
    local current_dir="$script_dir"
    while [[ $current_dir != "/" ]]; do
        if [[ -f "$current_dir/CMakeLists.txt" && -d "$current_dir/app" ]]; then
            echo "$current_dir"
            return 0
        fi
        current_dir="$(dirname "$current_dir")"
    done

    # Fallback: assume scripts is directly under project root
    echo "$(dirname "$script_dir")"
}

# Detect MSYS2 root directory
detect_msys2_root() {
    # Check environment variable first
    if [[ -n $MSYS2_ROOT && -d "$MSYS2_ROOT/mingw64" ]]; then
        echo "$MSYS2_ROOT"
        return 0
    fi

    # Check common paths
    local search_paths=(
        "/d/msys64"
        "/c/msys64"
        "/c/msys2"
        "$HOME/msys64"
    )

    for path in "${search_paths[@]}"; do
        if [[ -d "$path/mingw64" ]]; then
            echo "$path"
            return 0
        fi
    done

    # Check MSYSTEM_PREFIX for MSYS2 environment
    if [[ -n $MSYSTEM_PREFIX ]]; then
        echo "$(dirname "$MSYSTEM_PREFIX")"
        return 0
    fi

    return 1
}

# Detect vcpkg root directory
detect_vcpkg_root() {
    # Check environment variable first
    if [[ -n $VCPKG_ROOT && -f "$VCPKG_ROOT/vcpkg" ]]; then
        echo "$VCPKG_ROOT"
        return 0
    fi

    # Check common paths
    local search_paths=(
        "/d/vcpkg"
        "/c/vcpkg"
        "/c/src/vcpkg"
        "$HOME/vcpkg"
    )

    for path in "${search_paths[@]}"; do
        if [[ -f "$path/vcpkg" || -f "$path/vcpkg.exe" ]]; then
            echo "$path"
            return 0
        fi
    done

    return 1
}

# =============================================================================
# Build Directory Functions
# =============================================================================

# List of possible build directories
POSSIBLE_BUILD_DIRS=(
    "build/Debug-Unix"
    "build/Release-Unix"
    "build/Debug-Windows"
    "build/Release-Windows"
    "build/Debug-MSYS2"
    "build/Release-MSYS2"
)

# Find most recent build directory with compile_commands.json
find_latest_build_dir() {
    local project_root="$1"
    local best_dir=""
    local best_time=0

    for dir in "${POSSIBLE_BUILD_DIRS[@]}"; do
        local full_path="$project_root/$dir"
        local compile_commands="$full_path/compile_commands.json"

        if [[ -f $compile_commands ]]; then
            local mod_time
            if [[ $OSTYPE == "darwin"* ]]; then
                mod_time=$(stat -f %m "$compile_commands" 2>/dev/null || echo 0)
            else
                mod_time=$(stat -c %Y "$compile_commands" 2>/dev/null || echo 0)
            fi

            if [[ $mod_time -gt $best_time ]]; then
                best_time=$mod_time
                best_dir="$dir"
            fi
        fi
    done

    echo "$best_dir"
}

# =============================================================================
# Environment Checks
# =============================================================================

# Check if running in MSYS2 environment
is_msys2() {
    [[ -n $MSYSTEM ]]
}

# Check if running on Windows (any environment)
is_windows() {
    [[ $OSTYPE == "msys" || $OSTYPE == "cygwin" || $OSTYPE == "win32" ]]
}

# Check if running on macOS
is_macos() {
    [[ $OSTYPE == "darwin"* ]]
}

# Check if running on Linux
is_linux() {
    [[ $OSTYPE == "linux"* ]]
}

# Check if a command exists
command_exists() {
    command -v "$1" &>/dev/null
}

# =============================================================================
# Utility Functions
# =============================================================================

# Ensure directory exists
ensure_dir() {
    local dir="$1"
    if [[ ! -d $dir ]]; then
        mkdir -p "$dir"
        log_debug "Created directory: $dir"
    fi
}

# Clean exit with message
die() {
    log_error "$1"
    exit "${2:-1}"
}

# Confirmation prompt
confirm() {
    local prompt="${1:-Continue?}"
    read -r -p "$prompt [y/N] " response
    [[ $response =~ ^[Yy]$ ]]
}

# Export PROJECT_ROOT if we can detect it
if [[ -z $PROJECT_ROOT ]]; then
    PROJECT_ROOT="$(detect_project_root)"
    export PROJECT_ROOT
fi
