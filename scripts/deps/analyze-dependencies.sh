#!/usr/bin/env bash
# =============================================================================
# SAST Readium - DLL/SO Dependency Analyzer (Bash)
# =============================================================================
# Cross-platform script to analyze dependencies for executables.
# Uses ldd (Linux/MSYS2), otool (macOS), or objdump (fallback).
# =============================================================================

set -e

# Source common utilities if available
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -f "$SCRIPT_DIR/../common.sh" ]]; then
    source "$SCRIPT_DIR/../common.sh"
else
    # Fallback logging functions
    log_success() { echo -e "\033[0;32m✓ $1\033[0m"; }
    log_info() { echo -e "\033[0;36mℹ $1\033[0m"; }
    log_warning() { echo -e "\033[1;33m⚠ $1\033[0m"; }
    log_error() { echo -e "\033[0;31m✗ $1\033[0m"; }
fi

# Default values
EXECUTABLE=""
OUTPUT_FORMAT="text"
CHECK_MISSING=false
VERBOSE=false

# =============================================================================
# Help
# =============================================================================

show_help() {
    cat <<EOF
Usage: $(basename "$0") [OPTIONS] [EXECUTABLE]

Analyze dependencies for an executable.

OPTIONS:
    -e, --executable PATH   Path to executable (default: auto-detect)
    -f, --format FORMAT     Output format: text, json, csv (default: text)
    -m, --check-missing     Check for missing dependencies
    -v, --verbose           Verbose output
    -h, --help              Show this help

EXAMPLES:
    $(basename "$0")
    $(basename "$0") -e build/Debug-MSYS2/app/sast-readium.exe
    $(basename "$0") --format json --check-missing
EOF
}

# =============================================================================
# Argument Parsing
# =============================================================================

while [[ $# -gt 0 ]]; do
    case $1 in
    -e | --executable)
        EXECUTABLE="$2"
        shift 2
        ;;
    -f | --format)
        OUTPUT_FORMAT="$2"
        shift 2
        ;;
    -m | --check-missing)
        CHECK_MISSING=true
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
        if [[ -z $EXECUTABLE ]]; then
            EXECUTABLE="$1"
        fi
        shift
        ;;
    esac
done

# =============================================================================
# Auto-detect executable
# =============================================================================

detect_executable() {
    local project_root
    project_root="$(cd "$SCRIPT_DIR/../.." && pwd)"

    local search_paths=(
        "$project_root/build/Debug-MSYS2/app/sast-readium.exe"
        "$project_root/build/Release-MSYS2/app/sast-readium.exe"
        "$project_root/build/Debug-Unix/app/sast-readium"
        "$project_root/build/Release-Unix/app/sast-readium"
    )

    for path in "${search_paths[@]}"; do
        if [[ -f $path ]]; then
            echo "$path"
            return 0
        fi
    done

    return 1
}

if [[ -z $EXECUTABLE ]]; then
    EXECUTABLE=$(detect_executable) || {
        log_error "No executable found. Please specify with -e option."
        exit 1
    }
    log_info "Auto-detected executable: $EXECUTABLE"
fi

if [[ ! -f $EXECUTABLE ]]; then
    log_error "Executable not found: $EXECUTABLE"
    exit 1
fi

# =============================================================================
# Dependency Analysis
# =============================================================================

declare -a ALL_DEPS
declare -a MISSING_DEPS
declare -a SYSTEM_DEPS
declare -a QT_DEPS
declare -a THIRD_PARTY_DEPS

analyze_with_ldd() {
    local exe="$1"

    if ! command -v ldd &>/dev/null; then
        return 1
    fi

    while IFS= read -r line; do
        # Parse ldd output: libname.so => /path/to/lib (address)
        if [[ $line =~ ([^[:space:]]+)[[:space:]]+'=>'[[:space:]]+([^[:space:]]+) ]]; then
            local libname="${BASH_REMATCH[1]}"
            local libpath="${BASH_REMATCH[2]}"

            ALL_DEPS+=("$libname")

            if [[ $libpath == "not found" ]]; then
                MISSING_DEPS+=("$libname")
            elif [[ $libname =~ ^libQt6 || $libname =~ ^Qt6 ]]; then
                QT_DEPS+=("$libname")
            elif [[ $libpath =~ ^/usr/lib || $libpath =~ ^/lib || $libpath =~ ^/c/Windows ]]; then
                SYSTEM_DEPS+=("$libname")
            else
                THIRD_PARTY_DEPS+=("$libname")
            fi
        fi
    done < <(ldd "$exe" 2>/dev/null)

    return 0
}

analyze_with_otool() {
    local exe="$1"

    if ! command -v otool &>/dev/null; then
        return 1
    fi

    while IFS= read -r line; do
        # Skip header lines
        [[ $line =~ ^\t ]] || continue

        local libpath
        libpath=$(echo "$line" | awk '{print $1}')
        local libname
        libname=$(basename "$libpath")

        ALL_DEPS+=("$libname")

        if [[ $libname =~ ^libQt || $libname =~ ^Qt ]]; then
            QT_DEPS+=("$libname")
        elif [[ $libpath =~ ^/usr/lib || $libpath =~ ^/System ]]; then
            SYSTEM_DEPS+=("$libname")
        else
            THIRD_PARTY_DEPS+=("$libname")
        fi
    done < <(otool -L "$exe" 2>/dev/null)

    return 0
}

analyze_with_objdump() {
    local exe="$1"

    if ! command -v objdump &>/dev/null; then
        return 1
    fi

    while IFS= read -r line; do
        if [[ $line =~ DLL\ Name:\ ([^[:space:]]+) ]]; then
            local libname="${BASH_REMATCH[1]}"
            ALL_DEPS+=("$libname")

            if [[ $libname =~ ^Qt6 ]]; then
                QT_DEPS+=("$libname")
            elif [[ $libname =~ ^(KERNEL32|USER32|ADVAPI32|SHELL32|msvcrt|ntdll) ]]; then
                SYSTEM_DEPS+=("$libname")
            else
                THIRD_PARTY_DEPS+=("$libname")
            fi
        fi
    done < <(objdump -p "$exe" 2>/dev/null | grep "DLL Name")

    return 0
}

# Run analysis
log_info "Analyzing dependencies for: $EXECUTABLE"

if analyze_with_ldd "$EXECUTABLE"; then
    [[ $VERBOSE == true ]] && log_info "Used ldd for analysis"
elif analyze_with_otool "$EXECUTABLE"; then
    [[ $VERBOSE == true ]] && log_info "Used otool for analysis"
elif analyze_with_objdump "$EXECUTABLE"; then
    [[ $VERBOSE == true ]] && log_info "Used objdump for analysis"
else
    log_error "No suitable analysis tool found (ldd, otool, or objdump)"
    exit 1
fi

# =============================================================================
# Output
# =============================================================================

output_text() {
    echo ""
    echo "=== Dependency Analysis Results ==="
    echo "Executable: $EXECUTABLE"
    echo ""

    echo "Summary:"
    echo "  Total dependencies: ${#ALL_DEPS[@]}"
    echo "  System libraries:   ${#SYSTEM_DEPS[@]}"
    echo "  Qt6 libraries:      ${#QT_DEPS[@]}"
    echo "  Third-party:        ${#THIRD_PARTY_DEPS[@]}"
    echo "  Missing:            ${#MISSING_DEPS[@]}"
    echo ""

    if [[ ${#QT_DEPS[@]} -gt 0 ]]; then
        echo "Qt6 Dependencies:"
        printf '  - %s\n' "${QT_DEPS[@]}"
        echo ""
    fi

    if [[ ${#THIRD_PARTY_DEPS[@]} -gt 0 ]]; then
        echo "Third-Party Dependencies:"
        printf '  - %s\n' "${THIRD_PARTY_DEPS[@]}"
        echo ""
    fi

    if [[ ${#MISSING_DEPS[@]} -gt 0 ]]; then
        echo "Missing Dependencies:"
        printf '  - %s\n' "${MISSING_DEPS[@]}"
        echo ""
    fi
}

output_json() {
    echo "{"
    echo "  \"executable\": \"$EXECUTABLE\","
    echo "  \"total\": ${#ALL_DEPS[@]},"
    echo '  "system": ['
    local first=true
    for dep in "${SYSTEM_DEPS[@]}"; do
        [[ $first == true ]] || echo ","
        echo -n "    \"$dep\""
        first=false
    done
    echo ""
    echo "  ],"
    echo '  "qt6": ['
    first=true
    for dep in "${QT_DEPS[@]}"; do
        [[ $first == true ]] || echo ","
        echo -n "    \"$dep\""
        first=false
    done
    echo ""
    echo "  ],"
    echo '  "thirdParty": ['
    first=true
    for dep in "${THIRD_PARTY_DEPS[@]}"; do
        [[ $first == true ]] || echo ","
        echo -n "    \"$dep\""
        first=false
    done
    echo ""
    echo "  ],"
    echo '  "missing": ['
    first=true
    for dep in "${MISSING_DEPS[@]}"; do
        [[ $first == true ]] || echo ","
        echo -n "    \"$dep\""
        first=false
    done
    echo ""
    echo "  ]"
    echo "}"
}

output_csv() {
    echo "category,library"
    for dep in "${SYSTEM_DEPS[@]}"; do
        echo "system,$dep"
    done
    for dep in "${QT_DEPS[@]}"; do
        echo "qt6,$dep"
    done
    for dep in "${THIRD_PARTY_DEPS[@]}"; do
        echo "thirdParty,$dep"
    done
    for dep in "${MISSING_DEPS[@]}"; do
        echo "missing,$dep"
    done
}

case "$OUTPUT_FORMAT" in
text)
    output_text
    ;;
json)
    output_json
    ;;
csv)
    output_csv
    ;;
*)
    log_error "Unknown output format: $OUTPUT_FORMAT"
    exit 1
    ;;
esac

# Check for missing dependencies
if [[ $CHECK_MISSING == true && ${#MISSING_DEPS[@]} -gt 0 ]]; then
    log_error "Found ${#MISSING_DEPS[@]} missing dependencies"
    exit 1
fi

log_success "Analysis complete"
