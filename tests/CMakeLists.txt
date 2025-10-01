# Tests CMakeLists.txt for SAST Readium - Simplified

# Note: Testing environment setup is handled by the root CMakeLists.txt
# Dependencies are already found by the main project

# Create test utilities library if sources exist
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/TestUtilities.cpp)
    add_library(TestUtilities STATIC TestUtilities.h TestUtilities.cpp)
    target_link_libraries(TestUtilities PRIVATE Qt6::Core Qt6::Test Qt6::Widgets)
    target_include_directories(TestUtilities PRIVATE
        ${CMAKE_SOURCE_DIR}/app
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    set_target_properties(TestUtilities PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    message(STATUS "Created TestUtilities library")
endif()

# ============================================================================
# Unit Tests
# ============================================================================

# QGraphics Components Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_qgraphics_components.cpp)
    create_test_target(test_qgraphics_components
        SOURCES unit/test_qgraphics_components.cpp
        TIMEOUT 300
    )
endif()

# PDF Optimizations Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_pdf_optimizations.cpp)
    create_test_target(test_pdf_optimizations
        SOURCES unit/test_pdf_optimizations.cpp
        TIMEOUT 300
    )
endif()

# Error Handling Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_error_handling.cpp)
    create_test_target(test_error_handling
        SOURCES unit/test_error_handling.cpp
        TIMEOUT 300
    )
endif()

# Error Recovery Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_error_recovery.cpp)
    create_test_target(test_error_recovery
        SOURCES unit/test_error_recovery.cpp
        TIMEOUT 300
    )
endif()

# Thread Safety Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_thread_safety.cpp)
    # Find Google Test for thread safety tests
    find_package(GTest QUIET)
    if(GTest_FOUND)
        create_test_target(test_thread_safety
            SOURCES unit/test_thread_safety.cpp
            LINK_LIBRARIES GTest::gtest GTest::gtest_main
            TIMEOUT 600
        )
        message(STATUS "Thread safety tests enabled with Google Test")
    else()
        message(WARNING "Google Test not found, thread safety tests disabled")
    endif()
endif()

# ============================================================================
# Search Engine Tests
# ============================================================================

# Core Search Engine Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_search_engine_core.cpp)
    create_test_target(test_search_engine_core
        SOURCES unit/test_search_engine_core.cpp
        TIMEOUT 300
    )
endif()

# Advanced Search Features Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_search_advanced_features.cpp)
    create_test_target(test_search_advanced_features
        SOURCES unit/test_search_advanced_features.cpp
        TIMEOUT 300
    )
endif()

# Search Performance and Caching Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_search_performance_caching.cpp)
    create_test_target(test_search_performance_caching
        SOURCES unit/test_search_performance_caching.cpp
        TIMEOUT 300
    )
endif()

# Search Edge Cases Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_search_edge_cases.cpp)
    create_test_target(test_search_edge_cases
        SOURCES unit/test_search_edge_cases.cpp
        TIMEOUT 300
    )
endif()

# Search Validation Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_search_validation.cpp)
    create_test_target(test_search_validation
        SOURCES unit/test_search_validation.cpp
        TIMEOUT 300
    )
endif()

# Search Error Recovery Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_search_error_recovery.cpp)
    create_test_target(test_search_error_recovery
        SOURCES unit/test_search_error_recovery.cpp
        TIMEOUT 300
    )
endif()

# Search Components Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_search_components.cpp)
    create_test_target(test_search_components
        SOURCES unit/test_search_components.cpp
        TIMEOUT 300
    )
endif()

# Search Engine New Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_search_engine_new.cpp)
    create_test_target(test_search_engine_new
        SOURCES unit/test_search_engine_new.cpp
        TIMEOUT 300
    )
endif()

# Search Performance Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_search_performance.cpp)
    create_test_target(test_search_performance
        SOURCES unit/test_search_performance.cpp
        TIMEOUT 300
    )
endif()

# Search Thread Safety Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_search_thread_safety.cpp)
    create_test_target(test_search_thread_safety
        SOURCES unit/test_search_thread_safety.cpp
        TIMEOUT 600
    )
endif()

# ============================================================================
# Integration Tests
# ============================================================================

# Search Integration Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration/test_search_integration.cpp)
    create_test_target(test_search_integration
        SOURCES integration/test_search_integration.cpp
        TIMEOUT 600
    )
endif()

# Search Integration New Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration/test_search_integration_new.cpp)
    create_test_target(test_search_integration_new
        SOURCES integration/test_search_integration_new.cpp
        TIMEOUT 600
    )
endif()

# Rendering Mode Switch Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration/test_rendering_mode_switch.cpp)
    create_test_target(test_rendering_mode_switch
        SOURCES integration/test_rendering_mode_switch.cpp
        TIMEOUT 600
    )
endif()

# ============================================================================
# Controller Tests
# ============================================================================

# Service Locator Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/service_locator_test.cpp)
    create_test_target(service_locator_test
        SOURCES controller/service_locator_test.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

# State Manager Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/state_manager_test.cpp)
    create_test_target(state_manager_test
        SOURCES controller/state_manager_test.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

# State Manager New Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/state_manager_test_new.cpp)
    create_test_target(state_manager_test_new
        SOURCES controller/state_manager_test_new.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

# Event Bus Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/event_bus_test.cpp)
    create_test_target(event_bus_test
        SOURCES controller/event_bus_test.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

# Application Controller Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/application_controller_test.cpp)
    create_test_target(application_controller_test
        SOURCES controller/application_controller_test.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

# Document Controller Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/document_controller_test.cpp)
    create_test_target(document_controller_test
        SOURCES controller/document_controller_test.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

# Page Controller Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/page_controller_test.cpp)
    create_test_target(page_controller_test
        SOURCES controller/page_controller_test.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

# Configuration Manager Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/configuration_manager_test.cpp)
    create_test_target(configuration_manager_test
        SOURCES controller/configuration_manager_test.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

# Action Map Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/action_map_test.cpp)
    create_test_target(action_map_test
        SOURCES controller/action_map_test.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

# ============================================================================
# Search Tests
# ============================================================================

# Search Engine Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/search_engine_test.cpp)
    create_test_target(search_engine_test
        SOURCES search/search_engine_test.cpp
        TIMEOUT 300
    )
endif()

# Search Configuration Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/search_configuration_test.cpp)
    create_test_target(search_configuration_test
        SOURCES search/search_configuration_test.cpp
        TIMEOUT 300
    )
endif()

# Search Executor Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/search_executor_test.cpp)
    create_test_target(search_executor_test
        SOURCES search/search_executor_test.cpp
        TIMEOUT 300
    )
endif()

# Search Features Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/search_features_test.cpp)
    create_test_target(search_features_test
        SOURCES search/search_features_test.cpp
        TIMEOUT 300
    )
endif()

# Search Metrics Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/search_metrics_test.cpp)
    create_test_target(search_metrics_test
        SOURCES search/search_metrics_test.cpp
        TIMEOUT 300
    )
endif()

# Search Performance Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/search_performance_test.cpp)
    create_test_target(search_performance_test
        SOURCES search/search_performance_test.cpp
        TIMEOUT 300
    )
endif()

# Search Validator Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/search_validator_test.cpp)
    create_test_target(search_validator_test
        SOURCES search/search_validator_test.cpp
        TIMEOUT 300
    )
endif()

# Text Extractor Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/text_extractor_test.cpp)
    create_test_target(text_extractor_test
        SOURCES search/text_extractor_test.cpp
        TIMEOUT 300
    )
endif()

# Background Processor Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/background_processor_test.cpp)
    create_test_target(background_processor_test
        SOURCES search/background_processor_test.cpp
        TIMEOUT 300
    )
endif()

# Incremental Search Manager Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/incremental_search_manager_test.cpp)
    create_test_target(incremental_search_manager_test
        SOURCES search/incremental_search_manager_test.cpp
        TIMEOUT 300
    )
endif()

# Memory Aware Search Results Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/memory_aware_search_results_test.cpp)
    create_test_target(memory_aware_search_results_test
        SOURCES search/memory_aware_search_results_test.cpp
        TIMEOUT 300
    )
endif()

# Memory Manager Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/memory_manager_test.cpp)
    create_test_target(memory_manager_test
        SOURCES search/memory_manager_test.cpp
        TIMEOUT 300
    )
endif()

# Memory Manager Stubs Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/memory_manager_stubs_test.cpp)
    create_test_target(memory_manager_stubs_test
        SOURCES search/memory_manager_stubs_test.cpp
        TIMEOUT 300
    )
endif()

# Multi Search Engine Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/multi_search_engine_test.cpp)
    create_test_target(multi_search_engine_test
        SOURCES search/multi_search_engine_test.cpp
        TIMEOUT 300
    )
endif()

# Smart Eviction Policy Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/smart_eviction_policy_test.cpp)
    create_test_target(smart_eviction_policy_test
        SOURCES search/smart_eviction_policy_test.cpp
        TIMEOUT 300
    )
endif()

# Smart Eviction Policy Minimal Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/smart_eviction_policy_test_minimal.cpp)
    create_test_target(smart_eviction_policy_test_minimal
        SOURCES search/smart_eviction_policy_test_minimal.cpp
        TIMEOUT 300
    )
endif()

# ============================================================================
# Command Tests
# ============================================================================

# Command Manager Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/command/command_manager_test.cpp)
    create_test_target(command_manager_test
        SOURCES command/command_manager_test.cpp
        TIMEOUT 300
    )
endif()

# Document Commands Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/command/document_commands_test.cpp)
    create_test_target(document_commands_test
        SOURCES command/document_commands_test.cpp
        TIMEOUT 300
    )
endif()

# Initialization Command Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/command/initialization_command_test.cpp)
    create_test_target(initialization_command_test
        SOURCES command/initialization_command_test.cpp
        TIMEOUT 300
    )
endif()

# Navigation Commands Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/command/navigation_commands_test.cpp)
    create_test_target(navigation_commands_test
        SOURCES command/navigation_commands_test.cpp
        TIMEOUT 300
    )
endif()

# ============================================================================
# Factory Tests
# ============================================================================

# Model Factory Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/factory/model_factory_test.cpp)
    create_test_target(model_factory_test
        SOURCES factory/model_factory_test.cpp
        TIMEOUT 300
    )
endif()

# Model Factory New Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/factory/model_factory_test_new.cpp)
    create_test_target(model_factory_test_new
        SOURCES factory/model_factory_test_new.cpp
        TIMEOUT 300
    )
endif()

# ============================================================================
# Performance Tests
# ============================================================================

# Rendering Performance Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/performance/test_rendering_performance.cpp)
    create_test_target(test_rendering_performance
        SOURCES performance/test_rendering_performance.cpp
        TIMEOUT 600
    )
endif()

# Rendering Performance New Tests (requires QGraphics PDF support)
if(ENABLE_QGRAPHICS_PDF_SUPPORT AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/performance/test_rendering_performance_new.cpp)
    create_test_target(test_rendering_performance_new
        SOURCES performance/test_rendering_performance_new.cpp
        TIMEOUT 600
    )
    message(STATUS "Rendering performance new tests enabled (requires QGraphics PDF support)")
endif()

# Search Optimizations Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/performance/test_search_optimizations.cpp)
    create_test_target(test_search_optimizations
        SOURCES performance/test_search_optimizations.cpp
        TIMEOUT 600
    )
endif()

# ============================================================================
# Real World Tests
# ============================================================================

# Real PDF Documents Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/real_world/test_real_pdf_documents.cpp)
    create_test_target(test_real_pdf_documents
        SOURCES real_world/test_real_pdf_documents.cpp
        TIMEOUT 900
    )
endif()

# ============================================================================
# Utils Tests
# ============================================================================

# Document Analyzer Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/utils/document_analyzer_test.cpp)
    create_test_target(document_analyzer_test
        SOURCES utils/document_analyzer_test.cpp
        TIMEOUT 300
    )
endif()

# Error Handling Tests (Utils)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/utils/error_handling_test.cpp)
    create_test_target(utils_error_handling_test
        SOURCES utils/error_handling_test.cpp
        TIMEOUT 300
    )
endif()

# Error Recovery Tests (Utils)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/utils/error_recovery_test.cpp)
    create_test_target(utils_error_recovery_test
        SOURCES utils/error_recovery_test.cpp
        TIMEOUT 300
    )
endif()

# PDF Utilities Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/utils/pdf_utilities_test.cpp)
    create_test_target(pdf_utilities_test
        SOURCES utils/pdf_utilities_test.cpp
        TIMEOUT 300
    )
endif()

# UI Integration Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/debug_log_panel_integration_test.cpp)
    create_test_target(debug_log_panel_integration_test
        SOURCES ui/debug_log_panel_integration_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/tool_bar_integration_test.cpp)
    create_test_target(tool_bar_integration_test
        SOURCES ui/tool_bar_integration_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/thumbnail_generator_integration_test.cpp)
    create_test_target(thumbnail_generator_integration_test
        SOURCES ui/thumbnail_generator_integration_test.cpp
        TIMEOUT 300
    )
endif()

# Additional UI Integration Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/document_comparison_integration_test.cpp)
    create_test_target(document_comparison_integration_test
        SOURCES ui/document_comparison_integration_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/document_metadata_dialog_integration_test.cpp)
    create_test_target(document_metadata_dialog_integration_test
        SOURCES ui/document_metadata_dialog_integration_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/menu_bar_integration_test.cpp)
    create_test_target(menu_bar_integration_test
        SOURCES ui/menu_bar_integration_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/pdf_animations_integration_test.cpp)
    create_test_target(pdf_animations_integration_test
        SOURCES ui/pdf_animations_integration_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/pdf_outline_widget_integration_test.cpp)
    create_test_target(pdf_outline_widget_integration_test
        SOURCES ui/pdf_outline_widget_integration_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/pdf_viewer_integration_test.cpp)
    create_test_target(pdf_viewer_integration_test
        SOURCES ui/pdf_viewer_integration_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/right_side_bar_integration_test.cpp)
    create_test_target(right_side_bar_integration_test
        SOURCES ui/right_side_bar_integration_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/search_widget_integration_test.cpp)
    create_test_target(search_widget_integration_test
        SOURCES ui/search_widget_integration_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/side_bar_integration_test.cpp)
    create_test_target(side_bar_integration_test
        SOURCES ui/side_bar_integration_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/status_bar_integration_test.cpp)
    create_test_target(status_bar_integration_test
        SOURCES ui/status_bar_integration_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/thumbnail_context_menu_integration_test.cpp)
    create_test_target(thumbnail_context_menu_integration_test
        SOURCES ui/thumbnail_context_menu_integration_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/thumbnail_list_view_integration_test.cpp)
    create_test_target(thumbnail_list_view_integration_test
        SOURCES ui/thumbnail_list_view_integration_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/thumbnail_widget_integration_test.cpp)
    create_test_target(thumbnail_widget_integration_test
        SOURCES ui/thumbnail_widget_integration_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/view_widget_integration_test.cpp)
    create_test_target(view_widget_integration_test
        SOURCES ui/view_widget_integration_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/welcome_screen_manager_integration_test.cpp)
    create_test_target(welcome_screen_manager_integration_test
        SOURCES ui/welcome_screen_manager_integration_test.cpp
        TIMEOUT 300
    )
endif()

# ============================================================================
# Logging Tests
# ============================================================================

# Logging Stubs Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/logging/logging_stubs_test.cpp)
    create_test_target(logging_stubs_test
        SOURCES logging/logging_stubs_test.cpp
        TIMEOUT 300
    )
endif()

# Logging Stubs Minimal Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/logging/logging_stubs_test_minimal.cpp)
    create_test_target(logging_stubs_test_minimal
        SOURCES logging/logging_stubs_test_minimal.cpp
        TIMEOUT 300
    )
endif()

# ============================================================================
# Additional Tests
# ============================================================================

# QGraphics PDF Test (root level)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_qgraphics_pdf_test.cpp)
    create_test_target(test_qgraphics_pdf_test
        SOURCES test_qgraphics_pdf_test.cpp
        TIMEOUT 300
    )
endif()

# ============================================================================
# Smoke Test - Essential test that always exists
# ============================================================================

# Create a simple smoke test that always exists
create_test_target(smoke_test
    SOURCES smoke_test.cpp
    TIMEOUT 30
)

# Print configuration information
message(STATUS "")
message(STATUS "Simplified Test Configuration:")
message(STATUS "  ENABLE_QGRAPHICS_PDF_SUPPORT: ${ENABLE_QGRAPHICS_PDF_SUPPORT}")
message(STATUS "  Qt6 version: ${Qt6_VERSION}")
message(STATUS "  Test timeout: 300 seconds")
message(STATUS "")
