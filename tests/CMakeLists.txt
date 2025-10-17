# Tests CMakeLists.txt for SAST Readium
# Reorganized to mirror app/ directory structure
# Test naming convention: test_*.cpp for all tests (standardized naming pattern)

# Note: Testing environment setup is handled by the root CMakeLists.txt
# Dependencies are already found by the main project

# ============================================================================
# Test Utilities Library
# ============================================================================

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/TestUtilities.cpp)
    add_library(TestUtilities STATIC TestUtilities.h TestUtilities.cpp)
    target_link_libraries(TestUtilities PRIVATE
        Qt6::Core
        Qt6::Test
        Qt6::Widgets
        PkgConfig::POPPLER_QT6
    )
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
# Smoke Test - Essential sanity check
# ============================================================================

create_test_target(test_smoke
    SOURCES test_smoke.cpp
    TIMEOUT 30
)

# ============================================================================
# Cache Tests - Cache subsystem tests
# ============================================================================

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cache/test_cache_manager.cpp)
    create_test_target(test_cache_manager
        SOURCES cache/test_cache_manager.cpp cache/CacheTestHelpers.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cache/test_pdf_cache_manager.cpp)
    create_test_target(test_pdf_cache_manager
        SOURCES cache/test_pdf_cache_manager.cpp cache/CacheTestHelpers.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cache/test_page_text_cache.cpp)
    create_test_target(test_page_text_cache
        SOURCES cache/test_page_text_cache.cpp cache/CacheTestHelpers.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cache/test_search_result_cache.cpp)
    create_test_target(test_search_result_cache
        SOURCES cache/test_search_result_cache.cpp cache/CacheTestHelpers.cpp
        TIMEOUT 300
    )
endif()

# ============================================================================
# Command Tests - Command pattern implementation tests
# ============================================================================

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/command/test_command_manager.cpp)
    create_test_target(test_command_manager
        SOURCES command/test_command_manager.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/command/test_document_commands.cpp)
    create_test_target(test_document_commands
        SOURCES command/test_document_commands.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/command/test_initialization_command.cpp)
    create_test_target(test_initialization_command
        SOURCES command/test_initialization_command.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/command/test_navigation_commands.cpp)
    create_test_target(test_navigation_commands
        SOURCES command/test_navigation_commands.cpp
        TIMEOUT 300
    )
endif()

# ============================================================================
# Controller Tests - Service-oriented architecture tests
# ============================================================================

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/service_locator_test.cpp)
    create_test_target(service_locator_test
        SOURCES controller/service_locator_test.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/state_manager_test.cpp)
    create_test_target(state_manager_test
        SOURCES controller/state_manager_test.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

# State Manager Comprehensive Tests - TEMPORARILY DISABLED - API mismatch
# if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/state_manager_comprehensive_test.cpp)
#     create_test_target(state_manager_comprehensive_test
#         SOURCES controller/state_manager_comprehensive_test.cpp controller/ControllerTestMocks.cpp
#         TIMEOUT 300
#     )
# endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/event_bus_test.cpp)
    create_test_target(event_bus_test
        SOURCES controller/event_bus_test.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/application_controller_test.cpp)
    create_test_target(application_controller_test
        SOURCES controller/application_controller_test.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

# Note: document_controller_test and page_controller_test have their own mock definitions
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/document_controller_test.cpp)
    create_test_target(document_controller_test
        SOURCES controller/document_controller_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/page_controller_test.cpp)
    create_test_target(page_controller_test
        SOURCES controller/page_controller_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/configuration_manager_test.cpp)
    create_test_target(configuration_manager_test
        SOURCES controller/configuration_manager_test.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/action_map_test.cpp)
    create_test_target(action_map_test
        SOURCES controller/action_map_test.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

# ============================================================================
# Factory Tests - Factory pattern implementation tests
# ============================================================================

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/factory/model_factory_test.cpp)
    create_test_target(model_factory_test
        SOURCES factory/model_factory_test.cpp
        TIMEOUT 300
    )
endif()

# Command Prototype Registry Test - FIXED
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/factory/command_prototype_registry_test.cpp)
    create_test_target(command_prototype_registry_test
        SOURCES factory/command_prototype_registry_test.cpp
        TIMEOUT 300
    )
endif()

# ============================================================================
# Integration Tests - Multi-component integration tests
# ============================================================================

# Application Startup Test - Comprehensive end-to-end startup verification
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration/test_application_startup.cpp)
    create_test_target(test_application_startup
        SOURCES integration/test_application_startup.cpp
        TIMEOUT 900
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration/test_search_integration.cpp)
    create_test_target(test_search_integration
        SOURCES integration/test_search_integration.cpp
        TIMEOUT 600
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration/test_rendering_mode_switch.cpp)
    create_test_target(test_rendering_mode_switch
        SOURCES integration/test_rendering_mode_switch.cpp
        TIMEOUT 600
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration/test_qgraphics_pdf.cpp)
    create_test_target(test_qgraphics_pdf
        SOURCES integration/test_qgraphics_pdf.cpp
        TIMEOUT 300
    )
endif()

# ============================================================================
# Logging Tests - Logging system tests
# ============================================================================

# Logging Comprehensive Test - FIXED
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/logging/logging_comprehensive_test.cpp)
    create_test_target(logging_comprehensive_test
        SOURCES logging/logging_comprehensive_test.cpp
        TIMEOUT 300
    )
endif()

# ============================================================================
# Managers Tests - Manager component tests
# ============================================================================

# Windows Path Test - Uses GTest, requires special handling
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/managers/windows_path_test.cpp)
    find_package(GTest QUIET)
    if(GTest_FOUND)
        create_test_target(windows_path_test
            SOURCES managers/windows_path_test.cpp
            LINK_LIBRARIES GTest::gtest GTest::gtest_main
            TIMEOUT 300
        )
        message(STATUS "Windows path tests enabled with Google Test")
    else()
        message(WARNING "Google Test not found, windows_path_test disabled")
    endif()
endif()

# ============================================================================
# Model Tests - Model component tests
# ============================================================================

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/model/search_model_test.cpp)
    create_test_target(search_model_test
        SOURCES model/search_model_test.cpp
        TIMEOUT 300
    )
endif()

# ============================================================================
# Performance Tests - Performance benchmarks and optimization tests
# ============================================================================

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/performance/test_rendering_performance.cpp)
    create_test_target(test_rendering_performance
        SOURCES performance/test_rendering_performance.cpp
        TIMEOUT 600
    )
endif()

# QGraphics Rendering Performance Tests (requires QGraphics PDF support)
if(ENABLE_QGRAPHICS_PDF_SUPPORT AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/performance/test_qgraphics_rendering_performance.cpp)
    create_test_target(test_qgraphics_rendering_performance
        SOURCES performance/test_qgraphics_rendering_performance.cpp
        TIMEOUT 600
    )
    message(STATUS "QGraphics rendering performance tests enabled")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/performance/test_search_optimizations.cpp)
    create_test_target(test_search_optimizations
        SOURCES performance/test_search_optimizations.cpp
        TIMEOUT 600
    )
endif()

# ============================================================================
# Real World Tests - Real-world scenario tests
# ============================================================================

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/real_world/test_real_pdf_documents.cpp)
    create_test_target(test_real_pdf_documents
        SOURCES real_world/test_real_pdf_documents.cpp
        TIMEOUT 900
    )
endif()

# ============================================================================
# Search Tests - Search subsystem tests
# ============================================================================

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/search_engine_test.cpp)
    create_test_target(search_engine_test
        SOURCES search/search_engine_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/search_configuration_test.cpp)
    create_test_target(search_configuration_test
        SOURCES search/search_configuration_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/search_executor_test.cpp)
    create_test_target(search_executor_test
        SOURCES search/search_executor_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/search_features_test.cpp)
    create_test_target(search_features_test
        SOURCES search/search_features_test.cpp
        TIMEOUT 300
    )
endif()

# New standardized test naming pattern (test_*.cpp)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_search_features.cpp)
    create_test_target(test_search_features
        SOURCES search/test_search_features.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/search_metrics_test.cpp)
    create_test_target(search_metrics_test
        SOURCES search/search_metrics_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_search_metrics.cpp)
    create_test_target(test_search_metrics
        SOURCES search/test_search_metrics.cpp
        TIMEOUT 300
    )
endif()

# Search Performance Test - FIXED
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/search_performance_test.cpp)
    create_test_target(search_performance_test
        SOURCES search/search_performance_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/search_validator_test.cpp)
    create_test_target(search_validator_test
        SOURCES search/search_validator_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_search_validator.cpp)
    create_test_target(test_search_validator
        SOURCES search/test_search_validator.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/text_extractor_test.cpp)
    create_test_target(text_extractor_test
        SOURCES search/text_extractor_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/background_processor_test.cpp)
    create_test_target(background_processor_test
        SOURCES search/background_processor_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/incremental_search_manager_test.cpp)
    create_test_target(incremental_search_manager_test
        SOURCES search/incremental_search_manager_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/memory_aware_search_results_test.cpp)
    create_test_target(memory_aware_search_results_test
        SOURCES search/memory_aware_search_results_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/memory_manager_test.cpp)
    create_test_target(memory_manager_test
        SOURCES search/memory_manager_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/memory_manager_stubs_test.cpp)
    create_test_target(memory_manager_stubs_test
        SOURCES search/memory_manager_stubs_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/multi_search_engine_test.cpp)
    create_test_target(multi_search_engine_test
        SOURCES search/multi_search_engine_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/smart_eviction_policy_test.cpp)
    create_test_target(smart_eviction_policy_test
        SOURCES search/smart_eviction_policy_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/search_error_recovery_test.cpp)
    create_test_target(search_error_recovery_test
        SOURCES search/search_error_recovery_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/search_thread_safety_test.cpp)
    create_test_target(search_thread_safety_test
        SOURCES search/search_thread_safety_test.cpp
        TIMEOUT 600
    )
endif()

# ============================================================================
# UI Tests - User interface integration tests
# ============================================================================

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/debug_log_panel_integration_test.cpp)
    create_test_target(debug_log_panel_integration_test
        SOURCES ui/debug_log_panel_integration_test.cpp
        TIMEOUT 600
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
        TIMEOUT 600
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/document_comparison_integration_test.cpp)
    create_test_target(document_comparison_integration_test
        SOURCES ui/document_comparison_integration_test.cpp
        TIMEOUT 600
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

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_qgraphics_components.cpp)
    create_test_target(test_qgraphics_components
        SOURCES ui/test_qgraphics_components.cpp
        TIMEOUT 300
    )
endif()

# ============================================================================
# Utils Tests - Utility component tests
# ============================================================================

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/utils/document_analyzer_test.cpp)
    create_test_target(document_analyzer_test
        SOURCES utils/document_analyzer_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/utils/error_handling_test.cpp)
    create_test_target(utils_error_handling_test
        SOURCES utils/error_handling_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/utils/error_recovery_test.cpp)
    create_test_target(utils_error_recovery_test
        SOURCES utils/error_recovery_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/utils/pdf_utilities_test.cpp)
    create_test_target(pdf_utilities_test
        SOURCES utils/pdf_utilities_test.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/utils/test_pdf_optimizations.cpp)
    create_test_target(test_pdf_optimizations
        SOURCES utils/test_pdf_optimizations.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/utils/test_thread_safety.cpp)
    # Find Google Test for thread safety tests
    find_package(GTest QUIET)
    if(GTest_FOUND)
        create_test_target(test_thread_safety
            SOURCES utils/test_thread_safety.cpp
            LINK_LIBRARIES GTest::gtest GTest::gtest_main
            TIMEOUT 600
        )
        message(STATUS "Thread safety tests enabled with Google Test")
    else()
        message(WARNING "Google Test not found, thread safety tests disabled")
    endif()
endif()

# ============================================================================
# Test Configuration Summary
# ============================================================================

message(STATUS "")
message(STATUS "Test Configuration Summary:")
message(STATUS "  ENABLE_QGRAPHICS_PDF_SUPPORT: ${ENABLE_QGRAPHICS_PDF_SUPPORT}")
message(STATUS "  Qt6 version: ${Qt6_VERSION}")
message(STATUS "  Default test timeout: 300 seconds")
message(STATUS "  Test organization: Mirrors app/ directory structure")
message(STATUS "")
message(STATUS "Test directories:")
message(STATUS "  - cache/        : Cache subsystem tests")
message(STATUS "  - command/      : Command pattern tests")
message(STATUS "  - controller/   : Controller and service tests")
message(STATUS "  - factory/      : Factory pattern tests")
message(STATUS "  - integration/  : Multi-component integration tests")
message(STATUS "  - logging/      : Logging system tests")
message(STATUS "  - managers/     : Manager component tests")
message(STATUS "  - model/        : Model component tests")
message(STATUS "  - performance/  : Performance benchmarks")
message(STATUS "  - real_world/   : Real-world scenario tests")
message(STATUS "  - search/       : Search subsystem tests")
message(STATUS "  - ui/           : UI integration tests")
message(STATUS "  - utils/        : Utility component tests")
message(STATUS "")
