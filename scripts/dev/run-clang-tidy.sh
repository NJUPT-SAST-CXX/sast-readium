#!/usr/bin/env bash
# =============================================================================
# SAST Readium - Comprehensive clang-tidy Runner (Bash)
# =============================================================================
# Applies fixes across all project and test sources, aggregates results.
# =============================================================================

set -e

# Source common utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -f "$SCRIPT_DIR/../common.sh" ]]; then
    source "$SCRIPT_DIR/../common.sh"
else
    log_success() { echo -e "\033[0;32m✓ $1\033[0m"; }
    log_info() { echo -e "\033[0;36mℹ $1\033[0m"; }
    log_warning() { echo -e "\033[1;33m⚠ $1\033[0m"; }
    log_error() { echo -e "\033[0;31m✗ $1\033[0m"; }
fi

# Default values
CLANG_TIDY=""
BUILD_DIR="build/Debug-MSYS2"
OUTPUT_FILE="analysis/clang-tidy-results.txt"
FIX=false
FIX_ERRORS=false
VERBOSE=false

# =============================================================================
# Help
# =============================================================================

show_help() {
    cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Run clang-tidy on project source files.

OPTIONS:
    -c, --clang-tidy PATH   Path to clang-tidy (auto-detect if not specified)
    -b, --build-dir DIR     Build directory with compile_commands.json
    -o, --output FILE       Output file for results
    -f, --fix               Apply suggested fixes
    --fix-errors            Apply fixes for errors only
    -v, --verbose           Verbose output
    -h, --help              Show this help

EXAMPLES:
    $(basename "$0")
    $(basename "$0") -b build/Release-Unix --fix
    $(basename "$0") -c /usr/bin/clang-tidy-18 -v
EOF
}

# =============================================================================
# Argument Parsing
# =============================================================================

while [[ $# -gt 0 ]]; do
    case $1 in
    -c | --clang-tidy)
        CLANG_TIDY="$2"
        shift 2
        ;;
    -b | --build-dir)
        BUILD_DIR="$2"
        shift 2
        ;;
    -o | --output)
        OUTPUT_FILE="$2"
        shift 2
        ;;
    -f | --fix)
        FIX=true
        shift
        ;;
    --fix-errors)
        FIX_ERRORS=true
        shift
        ;;
    -v | --verbose)
        VERBOSE=true
        shift
        ;;
    -h | --help)
        show_help
        exit 0
        ;;
    *)
        log_error "Unknown option: $1"
        show_help
        exit 1
        ;;
    esac
done

# =============================================================================
# Auto-detect clang-tidy
# =============================================================================

find_clang_tidy() {
    # Check if in PATH
    if command -v clang-tidy &>/dev/null; then
        command -v clang-tidy
        return 0
    fi

    # Check versioned binaries
    for version in 18 17 16 15 14; do
        if command -v "clang-tidy-$version" &>/dev/null; then
            command -v "clang-tidy-$version"
            return 0
        fi
    done

    # Check common paths
    local search_paths=(
        "/usr/bin/clang-tidy"
        "/usr/local/bin/clang-tidy"
        "/opt/homebrew/bin/clang-tidy"
        "$MSYS2_ROOT/mingw64/bin/clang-tidy.exe"
        "/mingw64/bin/clang-tidy.exe"
    )

    for path in "${search_paths[@]}"; do
        if [[ -x $path ]]; then
            echo "$path"
            return 0
        fi
    done

    return 1
}

if [[ -z $CLANG_TIDY ]]; then
    CLANG_TIDY=$(find_clang_tidy) || {
        log_error "clang-tidy not found. Install it or specify with -c option."
        exit 1
    }
fi

if [[ ! -x $CLANG_TIDY ]]; then
    log_error "clang-tidy not executable: $CLANG_TIDY"
    exit 1
fi

log_info "Using clang-tidy: $CLANG_TIDY"
log_info "Version: $($CLANG_TIDY --version | head -1)"

# =============================================================================
# Verify build directory
# =============================================================================

PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
COMPILE_COMMANDS="$PROJECT_ROOT/$BUILD_DIR/compile_commands.json"

if [[ ! -f $COMPILE_COMMANDS ]]; then
    log_error "compile_commands.json not found at: $COMPILE_COMMANDS"
    log_info "Available build directories:"
    for dir in "$PROJECT_ROOT"/build/*/; do
        if [[ -f "$dir/compile_commands.json" ]]; then
            log_info "  - ${dir#$PROJECT_ROOT/}"
        fi
    done
    exit 1
fi

log_info "Using compile_commands.json from: $BUILD_DIR"

# =============================================================================
# Find source files
# =============================================================================

find_source_files() {
    local dir="$1"
    find "$dir" -type f \( -name "*.cpp" -o -name "*.h" \) \
        ! -path "*/build/*" \
        ! -path "*/.git/*" \
        ! -path "*/libs/*" \
        2>/dev/null
}

APP_FILES=$(find_source_files "$PROJECT_ROOT/app")
TEST_FILES=$(find_source_files "$PROJECT_ROOT/tests")

APP_COUNT=$(echo "$APP_FILES" | grep -c . || echo 0)
TEST_COUNT=$(echo "$TEST_FILES" | grep -c . || echo 0)

log_info "Found $APP_COUNT app files and $TEST_COUNT test files"

# =============================================================================
# Run clang-tidy
# =============================================================================

mkdir -p "$(dirname "$OUTPUT_FILE")"

# Build arguments
TIDY_ARGS=("-p" "$PROJECT_ROOT/$BUILD_DIR")

if [[ $FIX == true ]]; then
    TIDY_ARGS+=("--fix")
fi

if [[ $FIX_ERRORS == true ]]; then
    TIDY_ARGS+=("--fix-errors")
fi

run_clang_tidy() {
    local file="$1"
    local result

    if [[ $VERBOSE == true ]]; then
        log_info "Checking: ${file#$PROJECT_ROOT/}"
    fi

    result=$("$CLANG_TIDY" "${TIDY_ARGS[@]}" "$file" 2>&1) || true
    echo "$result"
}

# Process files
log_info "Running clang-tidy analysis..."

{
    echo "=== SAST Readium clang-tidy Analysis ==="
    echo "Date: $(date)"
    echo "Build Directory: $BUILD_DIR"
    echo "clang-tidy: $CLANG_TIDY"
    echo ""
    echo "=== App Sources ==="

    for file in $APP_FILES; do
        run_clang_tidy "$file"
    done

    echo ""
    echo "=== Test Sources ==="

    for file in $TEST_FILES; do
        run_clang_tidy "$file"
    done
} >"$OUTPUT_FILE" 2>&1

# =============================================================================
# Summary
# =============================================================================

WARNINGS=$(grep -c "warning:" "$OUTPUT_FILE" 2>/dev/null || echo 0)
ERRORS=$(grep -c "error:" "$OUTPUT_FILE" 2>/dev/null || echo 0)

echo ""
echo "=== Analysis Summary ==="
echo "  Warnings: $WARNINGS"
echo "  Errors:   $ERRORS"
echo "  Output:   $OUTPUT_FILE"
echo ""

if [[ $ERRORS -gt 0 ]]; then
    log_error "Found $ERRORS errors"
    exit 1
elif [[ $WARNINGS -gt 0 ]]; then
    log_warning "Found $WARNINGS warnings"
else
    log_success "No issues found"
fi
