#!/usr/bin/env bash
# Test runner script for SAST-Readium project (MSYS2 environment)
# Runs tests without PowerShell to avoid hanging issues

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Helper functions
log_success() { echo -e "${GREEN}$1${NC}"; }
log_info() { echo -e "${CYAN}$1${NC}"; }
log_warning() { echo -e "${YELLOW}$1${NC}"; }
log_error() { echo -e "${RED}$1${NC}"; }

# Project paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/build/Debug-MSYS2"
TEST_DIR="${BUILD_DIR}"
REPORTS_DIR="${BUILD_DIR}/test_reports"

# Parse command line arguments
TEST_TYPE="All"
PARALLEL=1
VERBOSE=0
STOP_ON_FAILURE=0
TIMEOUT=300

while [[ $# -gt 0 ]]; do
    case $1 in
    --type)
        TEST_TYPE="$2"
        shift 2
        ;;
    -j)
        PARALLEL="$2"
        shift 2
        ;;
    --verbose)
        VERBOSE=1
        shift
        ;;
    --stop-on-failure)
        STOP_ON_FAILURE=1
        shift
        ;;
    --timeout)
        TIMEOUT="$2"
        shift 2
        ;;
    *)
        log_warning "Unknown option: $1"
        shift
        ;;
    esac
done

echo "========================================="
echo "SAST-Readium Test Runner (MSYS2)"
echo "========================================="
echo "Build Directory: $BUILD_DIR"
echo "Test Directory: $TEST_DIR"
echo "Reports Directory: $REPORTS_DIR"
echo ""

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    log_error "Error: Build directory not found: $BUILD_DIR"
    log_info "Please build the project first using: ./scripts/build/build-msys2.sh -d"
    exit 1
fi

# Check if test directory has executables
if [ ! -d "$TEST_DIR" ]; then
    log_error "Error: Test directory not found: $TEST_DIR"
    exit 1
fi

# Create reports directory
mkdir -p "$REPORTS_DIR"

# Set Qt environment variables for offscreen testing
export QT_QPA_PLATFORM=windows
export QT_ACCESSIBILITY=0
export QT_OPENGL=software
export QT_LOGGING_RULES="*.debug=false"

log_info "Environment configured:"
log_info "  QT_QPA_PLATFORM=$QT_QPA_PLATFORM"
log_info "  QT_ACCESSIBILITY=$QT_ACCESSIBILITY"
log_info "  QT_OPENGL=$QT_OPENGL"
echo ""

log_info "Test Configuration:"
log_info "  Type: $TEST_TYPE"
log_info "  Parallelism: -j${PARALLEL}"
log_info "  Stop on Failure: $STOP_ON_FAILURE"
log_info "  Timeout per test: ${TIMEOUT} seconds"
echo ""

# Initialize counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Log file
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_FILE="${REPORTS_DIR}/test_run_${TIMESTAMP}.log"

echo "Test execution log: $LOG_FILE"
{
    echo "========================================="
    echo "SAST-Readium Test Execution Log"
    echo "Date: $(date)"
    echo "========================================="
    echo ""
} >"$LOG_FILE"

# Function to run a single test
run_test() {
    local TEST_NAME="$1"
    local TEST_EXE="${TEST_DIR}/${TEST_NAME}.exe"

    if [ ! -f "$TEST_EXE" ]; then
        log_warning "SKIP: $TEST_NAME - executable not found"
        echo "SKIP: $TEST_NAME - executable not found" >>"$LOG_FILE"
        echo "  Path checked: $TEST_EXE" >>"$LOG_FILE"
        ((SKIPPED_TESTS++))
        return 0
    fi

    ((TOTAL_TESTS++))

    echo ""
    echo "[$TOTAL_TESTS] Running: $TEST_NAME"
    echo "Started at: $(date)"
    {
        echo "-----------------------------------------"
        echo "[$TOTAL_TESTS] Test: $TEST_NAME"
        echo "Started at: $(date)"
        echo "-----------------------------------------"
    } >>"$LOG_FILE"

    # Run the test with timeout
    local TEST_OUTPUT_FILE="${REPORTS_DIR}/${TEST_NAME}_output.txt"
    local TEST_EXIT_CODE=0

    if timeout ${TIMEOUT}s "$TEST_EXE" >"$TEST_OUTPUT_FILE" 2>&1; then
        TEST_EXIT_CODE=0
    else
        TEST_EXIT_CODE=$?
    fi

    if [ $TEST_EXIT_CODE -eq 0 ]; then
        log_success "PASS: $TEST_NAME"
        echo "PASS: $TEST_NAME" >>"$LOG_FILE"
        echo "Completed at: $(date)" >>"$LOG_FILE"
        ((PASSED_TESTS++))
    elif [ $TEST_EXIT_CODE -eq 124 ]; then
        log_error "TIMEOUT: $TEST_NAME (exceeded ${TIMEOUT}s)"
        echo "TIMEOUT: $TEST_NAME (exceeded ${TIMEOUT}s)" >>"$LOG_FILE"
        echo "Completed at: $(date)" >>"$LOG_FILE"
        ((FAILED_TESTS++))

        # Show test output on timeout
        echo ""
        echo "----- Test Output (Timeout) -----"
        cat "$TEST_OUTPUT_FILE"
        echo "----- End Output -----"
        echo ""

        cat "$TEST_OUTPUT_FILE" >>"$LOG_FILE"
        echo "" >>"$LOG_FILE"

        if [ $STOP_ON_FAILURE -eq 1 ]; then
            log_error "Stopping due to test timeout."
            log_info "See log: $LOG_FILE"
            return 1
        fi
    else
        log_error "FAIL: $TEST_NAME (exit code: $TEST_EXIT_CODE)"
        echo "FAIL: $TEST_NAME (exit code: $TEST_EXIT_CODE)" >>"$LOG_FILE"
        echo "Completed at: $(date)" >>"$LOG_FILE"
        ((FAILED_TESTS++))

        # Show test output on failure
        echo ""
        echo "----- Test Output -----"
        cat "$TEST_OUTPUT_FILE"
        echo "----- End Output -----"
        echo ""

        cat "$TEST_OUTPUT_FILE" >>"$LOG_FILE"
        echo "" >>"$LOG_FILE"

        if [ $STOP_ON_FAILURE -eq 1 ]; then
            log_error "Stopping due to test failure."
            log_info "See log: $LOG_FILE"
            return 1
        fi
    fi

    if [ $VERBOSE -eq 1 ]; then
        echo "----- Verbose Output -----"
        cat "$TEST_OUTPUT_FILE"
        echo "----- End Verbose -----"
        echo ""
    fi

    # Small delay between tests
    sleep 1

    return 0
}

# Main test execution based on test type
echo ""
echo "========================================="
echo "Starting Test Execution"
echo "========================================="
echo ""

if [ "$TEST_TYPE" = "Smoke" ]; then
    log_info "Running Smoke Tests..."
    run_test "test_smoke"
elif [ "$TEST_TYPE" = "Unit" ]; then
    log_info "Running Unit Tests..."

    # Cache tests
    run_test "test_cache_manager"
    run_test "test_pdf_cache_manager"
    run_test "test_page_text_cache"
    run_test "test_search_result_cache"

    # Command tests
    run_test "test_command_manager"
    run_test "test_command_factory"
    run_test "test_command_prototype_registry"
    run_test "test_document_commands"
    run_test "test_initialization_command"
    run_test "test_navigation_commands"

    # Controller tests
    run_test "test_application_controller"
    run_test "test_document_controller"
    run_test "test_page_controller"
    run_test "test_event_bus"
    run_test "test_service_locator"
    run_test "test_state_manager"
    run_test "test_state_manager_comprehensive"

    # Factory tests
    run_test "test_model_factory"
    run_test "test_model_factory_concrete"
    run_test "test_widget_factory"

    # Logging tests
    run_test "test_logging_comprehensive"
    run_test "test_crash_handler"

    # Manager tests
    run_test "test_configuration_manager"
    run_test "test_action_map"

    # Model tests
    run_test "test_search_model"
    run_test "test_render_model"

    # Plugin tests
    run_test "test_plugin_manager_core"
    run_test "test_plugin_base_and_context"

    # Search tests
    run_test "test_search_engine"
    run_test "test_search_configuration"
    run_test "test_search_executor"
    run_test "test_search_features"
    run_test "test_search_metrics"
    run_test "test_search_validator"
    run_test "test_text_extractor"
    run_test "test_background_processor"
    run_test "test_incremental_search_manager"
    run_test "test_memory_aware_search_results"
    run_test "test_memory_manager"
    run_test "test_memory_manager_stubs"
    run_test "test_multi_search_engine"
    run_test "test_smart_eviction_policy"
    run_test "test_search_error_recovery"
    run_test "test_search_thread_safety"

    # Utils tests
    run_test "test_document_analyzer"
    run_test "test_error_handling"
    run_test "test_error_recovery"
    run_test "test_pdf_utilities"
    run_test "test_qgraphics_components"

    # Simple test
    run_test "test_simple"

elif [ "$TEST_TYPE" = "Integration" ]; then
    log_info "Running Integration Tests..."

    run_test "test_application_startup"
    run_test "test_search_integration"
    run_test "test_rendering_mode_switch"
    run_test "test_qgraphics_pdf"
    run_test "test_debug_log_panel_integration"
    run_test "test_tool_bar_integration"
    run_test "test_thumbnail_generator_integration"
    run_test "test_document_comparison_integration"
    run_test "test_document_metadata_dialog_integration"
    run_test "test_menu_bar_integration"
    run_test "test_mainwindow_ui_improvements"
    run_test "test_pdf_animations_integration"
    run_test "test_pdf_outline_widget_integration"
    run_test "test_pdf_viewer_integration"
    run_test "test_right_side_bar_integration"
    run_test "test_search_widget_integration"
    run_test "test_side_bar_integration"
    run_test "test_status_bar_integration"
    run_test "test_thumbnail_context_menu_integration"
    run_test "test_thumbnail_list_view_integration"
    run_test "test_thumbnail_widget_integration"
    run_test "test_view_widget_integration"
    run_test "test_welcome_screen_manager_integration"

elif [ "$TEST_TYPE" = "Performance" ]; then
    log_info "Running Performance Tests..."

    run_test "test_rendering_performance"
    run_test "test_search_optimizations"
    run_test "test_search_performance"
    run_test "test_pdf_optimizations"
    run_test "test_real_pdf_documents"

else
    # Default: Run all tests in complexity order
    log_info "Running All Tests in Complexity Order..."
    echo ""

    {
        echo "========================================="
        echo "Phase 1: Smoke Tests"
        echo "========================================="
    } >>"$LOG_FILE"
    echo "Phase 1: Smoke Tests"
    run_test "test_smoke" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1

    if [ $STOP_ON_FAILURE -eq 1 ] && [ $FAILED_TESTS -gt 0 ]; then
        exit 1
    fi

    echo ""
    {
        echo "========================================="
        echo "Phase 2: Unit Tests"
        echo "========================================="
    } >>"$LOG_FILE"
    echo "Phase 2: Unit Tests"

    # Cache tests
    echo "--- Cache Tests ---"
    run_test "test_cache_manager" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_pdf_cache_manager" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_page_text_cache" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_search_result_cache" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1

    # Command tests
    echo "--- Command Tests ---"
    run_test "test_command_manager" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_command_factory" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_command_prototype_registry" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_document_commands" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_initialization_command" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_navigation_commands" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1

    # Controller tests
    echo "--- Controller Tests ---"
    run_test "test_application_controller" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_document_controller" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_page_controller" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_event_bus" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_service_locator" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_state_manager" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_state_manager_comprehensive" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1

    # Factory tests
    echo "--- Factory Tests ---"
    run_test "test_model_factory" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_model_factory_concrete" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_widget_factory" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1

    # Logging tests
    echo "--- Logging Tests ---"
    run_test "test_logging_comprehensive" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_crash_handler" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1

    # Manager tests
    echo "--- Manager Tests ---"
    run_test "test_configuration_manager" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_action_map" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1

    # Model tests
    echo "--- Model Tests ---"
    run_test "test_search_model" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_render_model" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1

    # Plugin tests
    echo "--- Plugin Tests ---"
    run_test "test_plugin_manager_core" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_plugin_base_and_context" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1

    # Search tests
    echo "--- Search Tests ---"
    run_test "test_search_engine" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_search_configuration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_search_executor" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_search_features" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_search_metrics" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_search_validator" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_text_extractor" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_background_processor" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_incremental_search_manager" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_memory_aware_search_results" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_memory_manager" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_memory_manager_stubs" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_multi_search_engine" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_smart_eviction_policy" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_search_error_recovery" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_search_thread_safety" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1

    # Utils tests
    echo "--- Utils Tests ---"
    run_test "test_document_analyzer" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_error_handling" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_error_recovery" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_pdf_utilities" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_qgraphics_components" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1

    # Simple test
    run_test "test_simple" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1

    if [ $STOP_ON_FAILURE -eq 1 ] && [ $FAILED_TESTS -gt 0 ]; then
        exit 1
    fi

    echo ""
    {
        echo "========================================="
        echo "Phase 3: Integration Tests"
        echo "========================================="
    } >>"$LOG_FILE"
    echo "Phase 3: Integration Tests"

    run_test "test_application_startup" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_search_integration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_rendering_mode_switch" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_qgraphics_pdf" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_debug_log_panel_integration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_tool_bar_integration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_thumbnail_generator_integration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_document_comparison_integration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_document_metadata_dialog_integration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_menu_bar_integration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_mainwindow_ui_improvements" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_pdf_animations_integration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_pdf_outline_widget_integration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_pdf_viewer_integration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_right_side_bar_integration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_search_widget_integration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_side_bar_integration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_status_bar_integration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_thumbnail_context_menu_integration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_thumbnail_list_view_integration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_thumbnail_widget_integration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_view_widget_integration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_welcome_screen_manager_integration" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1

    if [ $STOP_ON_FAILURE -eq 1 ] && [ $FAILED_TESTS -gt 0 ]; then
        exit 1
    fi

    echo ""
    {
        echo "========================================="
        echo "Phase 4: Performance Tests"
        echo "========================================="
    } >>"$LOG_FILE"
    echo "Phase 4: Performance Tests"

    run_test "test_rendering_performance" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_search_optimizations" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_search_performance" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_pdf_optimizations" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
    run_test "test_real_pdf_documents" || [ $STOP_ON_FAILURE -eq 0 ] || exit 1
fi

# Test Summary
echo ""
echo "========================================="
echo "Test Execution Summary"
echo "========================================="
echo "Total Tests:  $TOTAL_TESTS"
log_success "Passed:       $PASSED_TESTS"
log_error "Failed:       $FAILED_TESTS"
log_warning "Skipped:      $SKIPPED_TESTS"
echo "========================================="
echo ""

{
    echo "========================================="
    echo "Test Execution Summary"
    echo "========================================="
    echo "Total Tests:  $TOTAL_TESTS"
    echo "Passed:       $PASSED_TESTS"
    echo "Failed:       $FAILED_TESTS"
    echo "Skipped:      $SKIPPED_TESTS"
    echo "========================================="
} >>"$LOG_FILE"

if [ $FAILED_TESTS -gt 0 ]; then
    log_error "Some tests failed. Check the log file for details:"
    echo "$LOG_FILE"
    exit 1
else
    log_success "All tests passed successfully!"
    exit 0
fi
