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
    if(TARGET app_lib)
        target_link_libraries(TestUtilities PUBLIC app_lib)
    endif()
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

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/test_service_locator.cpp)
    create_test_target(test_service_locator
        SOURCES controller/test_service_locator.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/test_state_manager.cpp)
    create_test_target(test_state_manager
        SOURCES controller/test_state_manager.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

# State Manager Comprehensive Tests - ENABLED - Updated with correct filename
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/test_state_manager_comprehensive.cpp)
    create_test_target(test_state_manager_comprehensive
        SOURCES controller/test_state_manager_comprehensive.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/test_event_bus.cpp)
    create_test_target(test_event_bus
        SOURCES controller/test_event_bus.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/test_application_controller.cpp)
    create_test_target(test_application_controller
        SOURCES controller/test_application_controller.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

# Note: document_controller_test and page_controller_test have their own mock definitions
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/test_document_controller.cpp)
    create_test_target(test_document_controller
        SOURCES controller/test_document_controller.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/test_page_controller.cpp)
    create_test_target(test_page_controller
        SOURCES controller/test_page_controller.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/test_configuration_manager.cpp)
    create_test_target(test_configuration_manager
        SOURCES controller/test_configuration_manager.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/test_action_map.cpp)
    create_test_target(test_action_map
        SOURCES controller/test_action_map.cpp controller/ControllerTestMocks.cpp
        TIMEOUT 300
    )
endif()

# ============================================================================
# Factory Tests - Factory pattern implementation tests
# ============================================================================

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/factory/test_model_factory.cpp)
    create_test_target(test_model_factory
        SOURCES factory/test_model_factory.cpp
        TIMEOUT 300
    )
endif()

# Command Prototype Registry Test - Updated with correct filename
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/factory/test_command_prototype_registry.cpp)
    create_test_target(test_command_prototype_registry
        SOURCES factory/test_command_prototype_registry.cpp
        TIMEOUT 300
    )
endif()

# Additional Factory Tests - New
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/factory/test_command_factory.cpp)
    create_test_target(test_command_factory
        SOURCES factory/test_command_factory.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/factory/test_widget_factory.cpp)
    create_test_target(test_widget_factory
        SOURCES factory/test_widget_factory.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/factory/test_model_factory_concrete.cpp)
    create_test_target(test_model_factory_concrete
        SOURCES factory/test_model_factory_concrete.cpp
        TIMEOUT 300
    )
endif()

# ============================================================================
# Plugin Tests - Plugin system core and base/context
# ============================================================================
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/plugin/test_plugin_manager_core.cpp)
    create_test_target(test_plugin_manager_core
        SOURCES plugin/test_plugin_manager_core.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/plugin/test_plugin_base_and_context.cpp)
    create_test_target(test_plugin_base_and_context
        SOURCES plugin/test_plugin_base_and_context.cpp
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

# Logging Comprehensive Test - Updated with correct filename
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/logging/test_logging_comprehensive.cpp)
    create_test_target(test_logging_comprehensive
        SOURCES logging/test_logging_comprehensive.cpp
        TIMEOUT 300
    )
endif()

# Crash Handler Test
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/logging/test_crash_handler.cpp)
    create_test_target(test_crash_handler
        SOURCES logging/test_crash_handler.cpp
        TIMEOUT 300
    )
endif()

# ============================================================================
# Managers Tests - Manager component tests
# ============================================================================

# Windows Path Test - Uses GTest, requires special handling - Updated with correct filename
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/managers/test_windows_path.cpp)
    # Prefer GTest from vcpkg when using MSVC to avoid pulling MSYS2 headers into MSVC builds
    if(MSVC)
        if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
            set(_vcpkg_gtest_dir "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/gtest")
            if(EXISTS "${_vcpkg_gtest_dir}/GTestConfig.cmake")
                set(GTest_DIR "${_vcpkg_gtest_dir}" CACHE PATH "Path to GTest (vcpkg)" FORCE)
            endif()
        endif()
        find_package(GTest CONFIG QUIET)
        # If GTest was found but not from vcpkg, disable these tests to avoid mixing toolchains
        if(GTest_FOUND)
            set(_expected_prefix "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
            file(TO_CMAKE_PATH "${_expected_prefix}" _expected_prefix_norm)
            if(DEFINED GTest_DIR)
                file(TO_CMAKE_PATH "${GTest_DIR}" _gtest_dir_norm)
                string(FIND "${_gtest_dir_norm}" "${_expected_prefix_norm}" _vpos)
                if(_vpos EQUAL -1)
                    message(WARNING "MSVC build detected non-vcpkg GTest at ${GTest_DIR}; disabling GTest-based managers tests")
                    set(GTest_FOUND FALSE)
                endif()
            else()
                set(GTest_FOUND FALSE)
            endif()
        endif()
    else()
        find_package(GTest QUIET)
    endif()
    if(GTest_FOUND)
        create_test_target(test_windows_path
            SOURCES managers/test_windows_path.cpp
            LINK_LIBRARIES GTest::gtest GTest::gtest_main
            TIMEOUT 300
        )
        message(STATUS "Windows path tests enabled with Google Test")
    else()
        message(WARNING "Google Test not found, test_windows_path disabled")
    endif()
endif()

# ============================================================================
# Model Tests - Model component tests
# ============================================================================

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/model/test_search_model.cpp)
    create_test_target(test_search_model
        SOURCES model/test_search_model.cpp
        TIMEOUT 300
    )
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/model/test_render_model.cpp)
    create_test_target(test_render_model
        SOURCES model/test_render_model.cpp
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

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_search_engine.cpp)
    create_test_target(test_search_engine
        SOURCES search/test_search_engine.cpp
        TIMEOUT 300
    )
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_simple.cpp)
    create_test_target(test_simple
        SOURCES search/test_simple.cpp
        TIMEOUT 120
    )
endif()


if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_search_configuration.cpp)
    create_test_target(test_search_configuration
        SOURCES search/test_search_configuration.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_search_executor.cpp)
    create_test_target(test_search_executor
        SOURCES search/test_search_executor.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_search_features.cpp)
    create_test_target(test_search_features
        SOURCES search/test_search_features.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_search_metrics.cpp)
    create_test_target(test_search_metrics
        SOURCES search/test_search_metrics.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_search_performance.cpp)
    create_test_target(test_search_performance
        SOURCES search/test_search_performance.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_search_validator.cpp)
    create_test_target(test_search_validator
        SOURCES search/test_search_validator.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_text_extractor.cpp)
    create_test_target(test_text_extractor
        SOURCES search/test_text_extractor.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_background_processor.cpp)
    create_test_target(test_background_processor
        SOURCES search/test_background_processor.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_incremental_search_manager.cpp)
    create_test_target(test_incremental_search_manager
        SOURCES search/test_incremental_search_manager.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_memory_aware_search_results.cpp)
    create_test_target(test_memory_aware_search_results
        SOURCES search/test_memory_aware_search_results.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_memory_manager.cpp)
    create_test_target(test_memory_manager
        SOURCES search/test_memory_manager.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_memory_manager_stubs.cpp)
    create_test_target(test_memory_manager_stubs
        SOURCES search/test_memory_manager_stubs.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_multi_search_engine.cpp)
    create_test_target(test_multi_search_engine
        SOURCES search/test_multi_search_engine.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_smart_eviction_policy.cpp)
    create_test_target(test_smart_eviction_policy
        SOURCES search/test_smart_eviction_policy.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_search_error_recovery.cpp)
    create_test_target(test_search_error_recovery
        SOURCES search/test_search_error_recovery.cpp
        TIMEOUT 300
    )
    # Known unstable on Windows due to QTimer lifecycle within test harness; test file already
    # indicates temporary skip. Make the skip effective by disabling the test on WIN32 to avoid
    # intermittent crashes in CI while broader fix is investigated.
    if(WIN32)
        set_tests_properties(test_search_error_recovery PROPERTIES DISABLED TRUE)
        message(WARNING "Disabling test_search_error_recovery on Windows (temporarily skipped in source)")
    endif()
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/test_search_thread_safety.cpp)
    create_test_target(test_search_thread_safety
        SOURCES search/test_search_thread_safety.cpp
        TIMEOUT 600
    )
endif()

# ============================================================================
# UI Tests - User interface integration tests
# ============================================================================

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_debug_log_panel_integration.cpp)
    create_test_target(test_debug_log_panel_integration
        SOURCES ui/test_debug_log_panel_integration.cpp
        TIMEOUT 600
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_tool_bar_integration.cpp)
    create_test_target(test_tool_bar_integration
        SOURCES ui/test_tool_bar_integration.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_thumbnail_generator_integration.cpp)
    create_test_target(test_thumbnail_generator_integration
        SOURCES ui/test_thumbnail_generator_integration.cpp
        TIMEOUT 600
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_document_comparison_integration.cpp)
    create_test_target(test_document_comparison_integration
        SOURCES ui/test_document_comparison_integration.cpp
        TIMEOUT 600
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_document_metadata_dialog_integration.cpp)
    create_test_target(test_document_metadata_dialog_integration
        SOURCES ui/test_document_metadata_dialog_integration.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_menu_bar_integration.cpp)
    create_test_target(test_menu_bar_integration
        SOURCES ui/test_menu_bar_integration.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_mainwindow_ui_improvements.cpp)
    create_test_target(test_mainwindow_ui_improvements
        SOURCES ui/test_mainwindow_ui_improvements.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_pdf_animations_integration.cpp)
    create_test_target(test_pdf_animations_integration
        SOURCES ui/test_pdf_animations_integration.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_pdf_outline_widget_integration.cpp)
    create_test_target(test_pdf_outline_widget_integration
        SOURCES ui/test_pdf_outline_widget_integration.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_pdf_viewer_integration.cpp)
    create_test_target(test_pdf_viewer_integration
        SOURCES ui/test_pdf_viewer_integration.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_right_side_bar_integration.cpp)
    create_test_target(test_right_side_bar_integration
        SOURCES ui/test_right_side_bar_integration.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_search_widget_integration.cpp)
    create_test_target(test_search_widget_integration
        SOURCES ui/test_search_widget_integration.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_side_bar_integration.cpp)
    create_test_target(test_side_bar_integration
        SOURCES ui/test_side_bar_integration.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_status_bar_integration.cpp)
    create_test_target(test_status_bar_integration
        SOURCES ui/test_status_bar_integration.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_thumbnail_context_menu_integration.cpp)
    create_test_target(test_thumbnail_context_menu_integration
        SOURCES ui/test_thumbnail_context_menu_integration.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_thumbnail_list_view_integration.cpp)
    create_test_target(test_thumbnail_list_view_integration
        SOURCES ui/test_thumbnail_list_view_integration.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_thumbnail_widget_integration.cpp)
    create_test_target(test_thumbnail_widget_integration
        SOURCES ui/test_thumbnail_widget_integration.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_view_widget_integration.cpp)
    create_test_target(test_view_widget_integration
        SOURCES ui/test_view_widget_integration.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/test_welcome_screen_manager_integration.cpp)
    create_test_target(test_welcome_screen_manager_integration
        SOURCES ui/test_welcome_screen_manager_integration.cpp
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

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/utils/test_document_analyzer.cpp)
    create_test_target(test_document_analyzer
        SOURCES utils/test_document_analyzer.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/utils/test_error_handling.cpp)
    create_test_target(test_error_handling
        SOURCES utils/test_error_handling.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/utils/test_error_recovery.cpp)
    create_test_target(test_error_recovery
        SOURCES utils/test_error_recovery.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/utils/test_pdf_utilities.cpp)
    create_test_target(test_pdf_utilities
        SOURCES utils/test_pdf_utilities.cpp
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
    # Prefer GTest from vcpkg when using MSVC to avoid mixing toolchains
    if(MSVC)
        if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
            set(_vcpkg_gtest_dir "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/gtest")
            if(EXISTS "${_vcpkg_gtest_dir}/GTestConfig.cmake")
                set(GTest_DIR "${_vcpkg_gtest_dir}" CACHE PATH "Path to GTest (vcpkg)" FORCE)
            endif()
        endif()
        find_package(GTest CONFIG QUIET)
        # If GTest was found but not from vcpkg, disable these tests to avoid mixing toolchains
        if(GTest_FOUND)
            set(_expected_prefix "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
            file(TO_CMAKE_PATH "${_expected_prefix}" _expected_prefix_norm)
            if(DEFINED GTest_DIR)
                file(TO_CMAKE_PATH "${GTest_DIR}" _gtest_dir_norm)
                string(FIND "${_gtest_dir_norm}" "${_expected_prefix_norm}" _vpos)
                if(_vpos EQUAL -1)
                    message(WARNING "MSVC build detected non-vcpkg GTest at ${GTest_DIR}; disabling GTest-based utils tests")
                    set(GTest_FOUND FALSE)
                endif()
            else()
                set(GTest_FOUND FALSE)
            endif()
        endif()
    else()
        find_package(GTest QUIET)
    endif()
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
