#!/usr/bin/env bash
# =============================================================================
# SAST Readium - Unix/Linux/macOS Test Runner
# =============================================================================
# Run tests for Unix-like systems with coverage support.
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
    print_header() { echo -e "\n\033[0;36m=== $1 ===\033[0m"; }
fi

# Default values
BUILD_TYPE="Debug"
TEST_TYPE="All"
COVERAGE=false
VERBOSE=false
STOP_ON_FAIL=false
HTML_REPORT=false
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# =============================================================================
# Help
# =============================================================================

show_help() {
    cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Run SAST Readium tests on Unix/Linux/macOS.

OPTIONS:
    -t, --type TYPE         Build type: Debug, Release (default: Debug)
    -T, --test-type TYPE    Test type: All, Unit, Integration, Smoke (default: All)
    -c, --coverage          Generate code coverage report
    -j, --jobs N            Number of parallel test jobs (default: auto)
    -s, --stop-on-fail      Stop on first test failure
    --html                  Generate HTML test report
    -v, --verbose           Verbose output
    -h, --help              Show this help

EXAMPLES:
    $(basename "$0")
    $(basename "$0") -T Unit -c --html
    $(basename "$0") -t Release -s
EOF
}

# =============================================================================
# Argument Parsing
# =============================================================================

while [[ $# -gt 0 ]]; do
    case $1 in
    -t | --type)
        BUILD_TYPE="$2"
        shift 2
        ;;
    -T | --test-type)
        TEST_TYPE="$2"
        shift 2
        ;;
    -c | --coverage)
        COVERAGE=true
        shift
        ;;
    -j | --jobs)
        JOBS="$2"
        shift 2
        ;;
    -s | --stop-on-fail)
        STOP_ON_FAIL=true
        shift
        ;;
    --html)
        HTML_REPORT=true
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
# Setup
# =============================================================================

PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build/${BUILD_TYPE}-Unix"
REPORTS_DIR="$PROJECT_ROOT/test-reports"

print_header "Test Configuration"
echo "  Build Type:   $BUILD_TYPE"
echo "  Test Type:    $TEST_TYPE"
echo "  Build Dir:    $BUILD_DIR"
echo "  Coverage:     $COVERAGE"
echo "  Jobs:         $JOBS"

# Check build exists
if [[ ! -d $BUILD_DIR ]]; then
    log_error "Build directory not found: $BUILD_DIR"
    log_info "Please build first: ./scripts/build/build-unix.sh -t $BUILD_TYPE"
    exit 1
fi

# Create reports directory
mkdir -p "$REPORTS_DIR"

# =============================================================================
# Run Tests
# =============================================================================

print_header "Running Tests"

cd "$BUILD_DIR"

CTEST_ARGS=("--output-on-failure" "-j" "$JOBS")

if [[ $VERBOSE == true ]]; then
    CTEST_ARGS+=("-V")
fi

if [[ $STOP_ON_FAIL == true ]]; then
    CTEST_ARGS+=("--stop-on-failure")
fi

# Filter by test type
case "$TEST_TYPE" in
Unit)
    CTEST_ARGS+=("-R" "test_")
    ;;
Integration)
    CTEST_ARGS+=("-R" "integration_")
    ;;
Smoke)
    CTEST_ARGS+=("-R" "smoke_")
    ;;
All)
    # Run all tests
    ;;
*)
    log_error "Unknown test type: $TEST_TYPE"
    exit 1
    ;;
esac

# Run ctest
log_info "Running ctest..."
ctest "${CTEST_ARGS[@]}" 2>&1 | tee "$REPORTS_DIR/test-output.txt"

TEST_RESULT=${PIPESTATUS[0]}

# =============================================================================
# Coverage Report
# =============================================================================

if [[ $COVERAGE == true ]]; then
    print_header "Generating Coverage Report"

    COVERAGE_DIR="$REPORTS_DIR/coverage"
    mkdir -p "$COVERAGE_DIR"

    if command -v llvm-profdata &>/dev/null && command -v llvm-cov &>/dev/null; then
        # LLVM coverage
        log_info "Using LLVM coverage tools..."

        if [[ -f default.profraw ]]; then
            llvm-profdata merge -sparse default.profraw -o default.profdata
            llvm-cov report ./tests/* -instr-profile=default.profdata >"$COVERAGE_DIR/coverage.txt"

            if [[ $HTML_REPORT == true ]]; then
                llvm-cov show ./tests/* -instr-profile=default.profdata -format=html -output-dir="$COVERAGE_DIR/html"
            fi
        else
            log_warning "No .profraw files found. Was coverage enabled during build?"
        fi

    elif command -v gcov &>/dev/null && command -v lcov &>/dev/null; then
        # GCC coverage
        log_info "Using gcov/lcov..."

        lcov --capture --directory . --output-file "$COVERAGE_DIR/coverage.info" --ignore-errors source
        lcov --remove "$COVERAGE_DIR/coverage.info" '/usr/*' '*/tests/*' --output-file "$COVERAGE_DIR/coverage.info"

        if [[ $HTML_REPORT == true ]] && command -v genhtml &>/dev/null; then
            genhtml "$COVERAGE_DIR/coverage.info" --output-directory "$COVERAGE_DIR/html"
            log_success "HTML coverage report: $COVERAGE_DIR/html/index.html"
        fi

    else
        log_warning "No coverage tools found (llvm-cov or lcov)"
    fi
fi

# =============================================================================
# JUnit Report
# =============================================================================

if [[ $HTML_REPORT == true ]]; then
    # CTest can generate JUnit XML
    log_info "Generating JUnit XML report..."

    ctest --output-junit "$REPORTS_DIR/junit.xml" "${CTEST_ARGS[@]}" 2>/dev/null || true

    if [[ -f "$REPORTS_DIR/junit.xml" ]]; then
        log_success "JUnit report: $REPORTS_DIR/junit.xml"
    fi
fi

# =============================================================================
# Summary
# =============================================================================

print_header "Test Summary"

# Count results from ctest
TOTAL=$(grep -c "Test #" "$REPORTS_DIR/test-output.txt" 2>/dev/null || echo 0)
PASSED=$(grep -c "Passed" "$REPORTS_DIR/test-output.txt" 2>/dev/null || echo 0)
FAILED=$(grep -c "Failed" "$REPORTS_DIR/test-output.txt" 2>/dev/null || echo 0)

echo "  Total:  $TOTAL"
echo "  Passed: $PASSED"
echo "  Failed: $FAILED"
echo ""
echo "  Reports: $REPORTS_DIR"

if [[ $TEST_RESULT -eq 0 ]]; then
    log_success "All tests passed!"
    exit 0
else
    log_error "Some tests failed"
    exit 1
fi
