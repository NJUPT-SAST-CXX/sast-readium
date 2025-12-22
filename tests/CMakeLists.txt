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
    message(VERBOSE "Created TestUtilities library")
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

# Define cache tests with common helper sources
set(CACHE_TESTS
    test_cache_manager
    test_pdf_cache_manager
    test_page_text_cache
    test_search_result_cache
)

foreach(test_name IN LISTS CACHE_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cache/${test_name}.cpp")
        create_test_target(${test_name}
            SOURCES cache/${test_name}.cpp cache/CacheTestHelpers.cpp
            TIMEOUT 300
        )
    endif()
endforeach()

# ============================================================================
# Command Tests - Command pattern implementation tests
# ============================================================================

# Define command tests
set(COMMAND_TESTS
    test_command_manager
    test_document_commands
    test_initialization_command
    test_navigation_commands
    test_accessibility_commands
)

foreach(test_name IN LISTS COMMAND_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/command/${test_name}.cpp")
        create_test_target(${test_name}
            SOURCES command/${test_name}.cpp
            TIMEOUT 300
        )
    endif()
endforeach()

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
# Plugin Tests - Plugin system core, base/context, hooks, and extensions
# ============================================================================

# Define plugin tests
set(PLUGIN_TESTS
    test_plugin_manager_core
    test_plugin_base_and_context
    test_plugin_hook_registry
    test_plugin_extension_points
    test_plugin_specialized_interfaces
)

foreach(test_name IN LISTS PLUGIN_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugin/${test_name}.cpp")
        create_test_target(${test_name}
            SOURCES plugin/${test_name}.cpp
            TIMEOUT 300
        )
    endif()
endforeach()

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

# New managers tests
set(MANAGERS_TESTS
    test_highlight_manager
    test_keyboard_shortcut_manager
    test_onboarding_manager
    test_system_tray_manager
    test_file_type_icon_manager
)

foreach(test_name IN LISTS MANAGERS_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/managers/${test_name}.cpp")
        create_test_target(${test_name}
            SOURCES managers/${test_name}.cpp
            TIMEOUT 300
        )
    endif()
endforeach()

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
        message(VERBOSE "Windows path tests enabled with Google Test")
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

# New model tests
set(MODEL_TESTS
    test_accessibility_model
    test_annotation_model
    test_bookmark_model
    test_page_model
    test_thumbnail_model
)

foreach(test_name IN LISTS MODEL_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/model/${test_name}.cpp")
        create_test_target(${test_name}
            SOURCES model/${test_name}.cpp
            TIMEOUT 300
        )
    endif()
endforeach()

# ============================================================================
# Delegate Tests - Delegate pattern implementation tests
# ============================================================================

set(DELEGATE_TESTS
    test_annotation_render_delegate
    test_page_navigation_delegate
    test_plugin_list_delegate
    test_thumbnail_delegate
    test_view_delegate
)

foreach(test_name IN LISTS DELEGATE_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/delegate/${test_name}.cpp")
        create_test_target(${test_name}
            SOURCES delegate/${test_name}.cpp
            TIMEOUT 300
        )
    endif()
endforeach()

# ============================================================================
# Interaction Tests - User interaction handling tests
# ============================================================================

set(INTERACTION_TESTS
    test_annotation_interaction_handler
    test_multi_page_text_selection
    test_text_selection_manager
)

foreach(test_name IN LISTS INTERACTION_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/interaction/${test_name}.cpp")
        create_test_target(${test_name}
            SOURCES interaction/${test_name}.cpp
            TIMEOUT 300
        )
    endif()
endforeach()

# ============================================================================
# Adapters Tests - Adapter pattern implementation tests
# ============================================================================

set(ADAPTERS_TESTS
    test_document_adapter
    test_page_adapter
    test_search_adapter
    test_view_adapter
)

foreach(test_name IN LISTS ADAPTERS_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/adapters/${test_name}.cpp")
        create_test_target(${test_name}
            SOURCES adapters/${test_name}.cpp
            TIMEOUT 300
        )
    endif()
endforeach()

# ============================================================================
# Security Tests - Security feature tests
# ============================================================================

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/security/test_signature_verifier.cpp)
    create_test_target(test_signature_verifier
        SOURCES security/test_signature_verifier.cpp
        TIMEOUT 300
    )
endif()

# ============================================================================
# Accessibility Tests - Accessibility feature tests
# ============================================================================

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/accessibility/test_accessibility_manager.cpp)
    create_test_target(test_accessibility_manager
        SOURCES accessibility/test_accessibility_manager.cpp
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
# Organized by subdirectory to mirror app/ui/ structure
# ============================================================================

# ----------------------------------------------------------------------------
# UI Core Tests - Core UI components (MenuBar, ToolBar, StatusBar, SideBar, etc.)
# ----------------------------------------------------------------------------

set(UI_CORE_TESTS
    test_menu_bar_integration
    test_side_bar_integration
    test_right_side_bar_integration
    test_status_bar_integration
    test_status_bar_functionality_comprehensive
    test_tool_bar_integration
    test_toolbar_functionality_comprehensive
    test_view_widget_integration
    test_view_widget_functionality_comprehensive
    test_keyboard_shortcut_comprehensive
    test_menu_functionality_comprehensive
    test_ui_performance_comprehensive
    test_context_menu_manager
    test_ui_consistency_manager
    test_ui_error_handler
    test_ui_recovery_manager
    test_ui_resource_manager
    test_ui_state_manager
)

foreach(test_name IN LISTS UI_CORE_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ui/core/${test_name}.cpp")
        create_test_target(${test_name}
            SOURCES ui/core/${test_name}.cpp
            TIMEOUT 300
        )
    endif()
endforeach()

# ----------------------------------------------------------------------------
# UI Dialogs Tests - Dialog components
# ----------------------------------------------------------------------------

set(UI_DIALOGS_TESTS
    test_document_comparison
    test_document_comparison_integration
    test_document_metadata_dialog_integration
    test_settings_dialog
    test_settings_dialog_integration
)

foreach(test_name IN LISTS UI_DIALOGS_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ui/dialogs/${test_name}.cpp")
        set(_timeout 300)
        if("${test_name}" STREQUAL "test_document_comparison_integration")
            set(_timeout 600)
        endif()
        create_test_target(${test_name}
            SOURCES ui/dialogs/${test_name}.cpp
            TIMEOUT ${_timeout}
        )
    endif()
endforeach()

# ----------------------------------------------------------------------------
# UI Integration Tests - Cross-component integration tests
# ----------------------------------------------------------------------------

set(UI_INTEGRATION_TESTS
    test_ui_workflow_integration_comprehensive
    test_final_integration_comprehensive
    test_annotation_integration_helper
)

foreach(test_name IN LISTS UI_INTEGRATION_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ui/integration/${test_name}.cpp")
        create_test_target(${test_name}
            SOURCES ui/integration/${test_name}.cpp
            TIMEOUT 600
        )
    endif()
endforeach()

# ----------------------------------------------------------------------------
# UI Managers Tests - UI manager components
# ----------------------------------------------------------------------------

set(UI_MANAGERS_TESTS
    test_welcome_screen_manager_integration
    test_annotation_selection_manager
)

foreach(test_name IN LISTS UI_MANAGERS_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ui/managers/${test_name}.cpp")
        create_test_target(${test_name}
            SOURCES ui/managers/${test_name}.cpp
            TIMEOUT 300
        )
    endif()
endforeach()

# ----------------------------------------------------------------------------
# UI Pages Tests - Page components
# ----------------------------------------------------------------------------

set(UI_PAGES_TESTS
    test_mainwindow_ui_improvements
    test_about_page
    test_home_page
    test_pdf_viewer_page
    test_plugin_manager_page
    test_settings_page
)

foreach(test_name IN LISTS UI_PAGES_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ui/pages/${test_name}.cpp")
        create_test_target(${test_name}
            SOURCES ui/pages/${test_name}.cpp
            TIMEOUT 300
        )
    endif()
endforeach()

# ----------------------------------------------------------------------------
# UI Theme Tests - Theme and reading mode components
# ----------------------------------------------------------------------------

set(UI_THEME_TESTS
    test_reading_mode_manager
)

foreach(test_name IN LISTS UI_THEME_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ui/theme/${test_name}.cpp")
        create_test_target(${test_name}
            SOURCES ui/theme/${test_name}.cpp
            TIMEOUT 300
        )
    endif()
endforeach()

# ----------------------------------------------------------------------------
# UI Thumbnail Tests - Thumbnail system components
# ----------------------------------------------------------------------------

set(UI_THUMBNAIL_TESTS
    test_thumbnail_generator_integration
    test_thumbnail_context_menu_integration
    test_thumbnail_list_view_integration
    test_thumbnail_widget_integration
)

foreach(test_name IN LISTS UI_THUMBNAIL_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ui/thumbnail/${test_name}.cpp")
        set(_timeout 300)
        if("${test_name}" STREQUAL "test_thumbnail_generator_integration")
            set(_timeout 600)
        endif()
        create_test_target(${test_name}
            SOURCES ui/thumbnail/${test_name}.cpp
            TIMEOUT ${_timeout}
        )
    endif()
endforeach()

# ----------------------------------------------------------------------------
# UI Utils Tests - UI utility components
# ----------------------------------------------------------------------------

set(UI_UTILS_TESTS
    test_validation_utils
)

foreach(test_name IN LISTS UI_UTILS_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ui/utils/${test_name}.cpp")
        create_test_target(${test_name}
            SOURCES ui/utils/${test_name}.cpp
            TIMEOUT 300
        )
    endif()
endforeach()

# ----------------------------------------------------------------------------
# UI Viewer Tests - PDF viewer components
# ----------------------------------------------------------------------------

set(UI_VIEWER_TESTS
    test_pdf_viewer_integration
    test_pdf_animations_integration
    test_pdf_outline_widget_integration
    test_qgraphics_components
    test_pdf_prerenderer
    test_pdf_viewer_components
    test_split_view_manager
)

foreach(test_name IN LISTS UI_VIEWER_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ui/viewer/${test_name}.cpp")
        create_test_target(${test_name}
            SOURCES ui/viewer/${test_name}.cpp
            TIMEOUT 300
        )
    endif()
endforeach()

# ----------------------------------------------------------------------------
# UI Widgets Tests - Widget components
# ----------------------------------------------------------------------------

set(UI_WIDGETS_TESTS
    test_debug_log_panel_integration
    test_search_widget_integration
    test_search_widget_functionality_comprehensive
    test_welcome_widget_functionality
    test_accessibility_settings_widget
    test_annotation_settings_widget
    test_annotation_toolbar
    test_annotations_panel
    test_bookmark_panel
    test_bookmark_widget
    test_cache_settings_widget
    test_document_properties_panel
    test_document_settings_widget
    test_document_tab_widget
    test_enhanced_focus_indicator
    test_layers_panel
    test_logging_settings_widget
    test_notification_helper
    test_onboarding_widget
    test_outline_panel
    test_plugin_settings_widget
    test_properties_panel
    test_recent_file_list_widget
    test_search_panel
    test_search_settings_widget
    test_shortcut_settings_widget
    test_skeleton_widget
    test_system_tray_settings_widget
    test_thumbnail_panel
    test_toast_notification
    test_tutorial_card
)

foreach(test_name IN LISTS UI_WIDGETS_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ui/widgets/${test_name}.cpp")
        set(_timeout 300)
        if("${test_name}" STREQUAL "test_debug_log_panel_integration")
            set(_timeout 600)
        endif()
        create_test_target(${test_name}
            SOURCES ui/widgets/${test_name}.cpp
            TIMEOUT ${_timeout}
        )
    endif()
endforeach()

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

# New utils tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/utils/test_safe_pdf_renderer.cpp)
    create_test_target(test_safe_pdf_renderer
        SOURCES utils/test_safe_pdf_renderer.cpp
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/utils/test_resources_init.cpp)
    create_test_target(test_resources_init
        SOURCES utils/test_resources_init.cpp
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
        message(VERBOSE "Thread safety tests enabled with Google Test")
    else()
        message(WARNING "Google Test not found, thread safety tests disabled")
    endif()
endif()

# ============================================================================
# Test Configuration Summary
# ============================================================================

message(NOTICE "")
message(NOTICE "Test Configuration Summary:")
message(NOTICE "  ENABLE_QGRAPHICS_PDF_SUPPORT: ${ENABLE_QGRAPHICS_PDF_SUPPORT}")
message(NOTICE "  Qt6 version: ${Qt6_VERSION}")
message(NOTICE "  Default test timeout: 300 seconds")
message(NOTICE "  Test organization: Mirrors app/ directory structure")
message(NOTICE "")
message(NOTICE "Test directories:")
message(NOTICE "  - accessibility/: Accessibility feature tests")
message(NOTICE "  - adapters/     : Adapter pattern tests")
message(NOTICE "  - cache/        : Cache subsystem tests")
message(NOTICE "  - command/      : Command pattern tests")
message(NOTICE "  - controller/   : Controller and service tests")
message(NOTICE "  - delegate/     : Delegate pattern tests")
message(NOTICE "  - factory/      : Factory pattern tests")
message(NOTICE "  - integration/  : Multi-component integration tests")
message(NOTICE "  - interaction/  : User interaction tests")
message(NOTICE "  - logging/      : Logging system tests")
message(NOTICE "  - managers/     : Manager component tests")
message(NOTICE "  - model/        : Model component tests")
message(NOTICE "  - performance/  : Performance benchmarks")
message(NOTICE "  - real_world/   : Real-world scenario tests")
message(NOTICE "  - search/       : Search subsystem tests")
message(NOTICE "  - security/     : Security feature tests")
message(NOTICE "  - ui/           : UI integration tests")
message(NOTICE "  - utils/        : Utility component tests")
message(NOTICE "")
