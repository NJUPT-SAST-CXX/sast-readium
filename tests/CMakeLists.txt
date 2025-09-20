# Tests CMakeLists.txt for SAST Readium - Optimized with Component Libraries

# Include utility modules
include(TestUtils)
include(ComponentUtils)

# Note: Testing environment setup is handled by the root CMakeLists.txt
# Dependencies are already found by the main project

# Create standard component libraries for testing
message(STATUS "Creating component libraries for testing...")
create_standard_app_components(BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../app)

# Create test utilities library
create_test_library(TestUtilities
    SOURCES TestUtilities.h TestUtilities.cpp
    DESCRIPTION "Common test utilities and helpers"
)

# Component libraries are now created by create_standard_app_components() above
# This replaces the old manual library creation with automatic component-based libraries

# Print component library summary
print_component_summary()

# ============================================================================
# Unit Tests
# ============================================================================

# QGraphics Components Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_qgraphics_components.cpp)
    create_test_executable(test_qgraphics_components
        SOURCES unit/test_qgraphics_components.cpp
        CATEGORY unit
        LINK_LIBRARIES CoreAppLib UILib ManagerLib LoggingLib SearchLib ControllerLib FactoryLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# PDF Optimizations Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_pdf_optimizations.cpp)
    create_test_executable(test_pdf_optimizations
        SOURCES unit/test_pdf_optimizations.cpp
        CATEGORY unit
        LINK_LIBRARIES CoreAppLib UILib ManagerLib LoggingLib SearchLib ControllerLib FactoryLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# Error Handling Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_error_handling.cpp)
    create_test_executable(test_error_handling
        SOURCES unit/test_error_handling.cpp
        CATEGORY unit
        LINK_LIBRARIES CoreAppLib UILib ManagerLib LoggingLib SearchLib ControllerLib FactoryLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# Error Recovery Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_error_recovery.cpp)
    create_test_executable(test_error_recovery
        SOURCES unit/test_error_recovery.cpp
        CATEGORY unit
        LINK_LIBRARIES CoreAppLib UILib ManagerLib LoggingLib SearchLib ControllerLib FactoryLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# Thread Safety Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_thread_safety.cpp)
    # Find Google Test for thread safety tests
    find_package(GTest QUIET)
    if(GTest_FOUND)
        create_test_executable(test_thread_safety
            SOURCES unit/test_thread_safety.cpp
            CATEGORY unit
            LINK_LIBRARIES CoreAppLib UILib ManagerLib LoggingLib TestUtilities GTest::gtest GTest::gtest_main
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
    create_test_executable(test_search_engine_core
        SOURCES unit/test_search_engine_core.cpp
        CATEGORY unit
        LINK_LIBRARIES SearchLib CoreAppLib UILib ManagerLib LoggingLib
        TIMEOUT 300
    )
endif()

# Advanced Search Features Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_search_advanced_features.cpp)
    create_test_executable(test_search_advanced_features
        SOURCES unit/test_search_advanced_features.cpp
        CATEGORY unit
        LINK_LIBRARIES SearchLib CoreAppLib UILib ManagerLib LoggingLib
        TIMEOUT 300
    )
endif()

# Search Performance and Caching Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_search_performance_caching.cpp)
    create_test_executable(test_search_performance_caching
        SOURCES unit/test_search_performance_caching.cpp
        CATEGORY unit
        LINK_LIBRARIES SearchLib CoreAppLib UILib ManagerLib LoggingLib
        TIMEOUT 300
    )
endif()

# Search Edge Cases Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_search_edge_cases.cpp)
    create_test_executable(test_search_edge_cases
        SOURCES unit/test_search_edge_cases.cpp
        CATEGORY unit
        LINK_LIBRARIES SearchLib CoreAppLib UILib ManagerLib LoggingLib
        TIMEOUT 300
    )
endif()

# Search Validation Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_search_validation.cpp)
    create_test_executable(test_search_validation
        SOURCES unit/test_search_validation.cpp
        CATEGORY unit
        LINK_LIBRARIES SearchLib CoreAppLib UILib ManagerLib LoggingLib
        TIMEOUT 300
    )
endif()

# Search Error Recovery Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_search_error_recovery.cpp)
    create_test_executable(test_search_error_recovery
        SOURCES unit/test_search_error_recovery.cpp
        CATEGORY unit
        LINK_LIBRARIES SearchLib CoreAppLib UILib ManagerLib LoggingLib
        TIMEOUT 300
    )
endif()

# ============================================================================
# Integration Tests
# ============================================================================

# Search Integration Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration/test_search_integration.cpp)
    create_test_executable(test_search_integration
        SOURCES integration/test_search_integration.cpp
        CATEGORY integration
        LINK_LIBRARIES SearchLib CoreAppLib UILib ManagerLib LoggingLib ControllerLib FactoryLib
        TIMEOUT 600
    )
endif()

# Rendering Mode Switch Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration/test_rendering_mode_switch.cpp)
    create_test_executable(test_rendering_mode_switch
        SOURCES integration/test_rendering_mode_switch.cpp
        CATEGORY integration
        LINK_LIBRARIES CoreAppLib UILib ManagerLib LoggingLib TestUtilities
        TIMEOUT 600
    )
endif()

# UI Integration Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/debug_log_panel_integration_test.cpp)
    create_test_executable(debug_log_panel_integration_test
        SOURCES ui/debug_log_panel_integration_test.cpp
        CATEGORY integration
        LINK_LIBRARIES UILib LoggingLib CoreAppLib ManagerLib SearchLib ControllerLib FactoryLib
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/tool_bar_integration_test.cpp)
    create_test_executable(tool_bar_integration_test
        SOURCES ui/tool_bar_integration_test.cpp
        CATEGORY integration
        LINK_LIBRARIES UILib CoreAppLib ManagerLib SearchLib ControllerLib FactoryLib
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/thumbnail_generator_integration_test.cpp)
    create_test_executable(thumbnail_generator_integration_test
        SOURCES ui/thumbnail_generator_integration_test.cpp
        CATEGORY integration
        LINK_LIBRARIES UILib CoreAppLib ManagerLib ControllerLib FactoryLib SearchLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# ============================================================================
# New UI Integration Tests - Core Components
# ============================================================================

# MenuBar Integration Test
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/MenuBarIntegrationTest.cpp)
    create_test_executable(test_menubar_integration
        SOURCES ui/MenuBarIntegrationTest.cpp
        CATEGORY integration
        LINK_LIBRARIES UILib CoreAppLib ManagerLib ControllerLib FactoryLib SearchLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# SideBar Integration Test
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/SideBarIntegrationTest.cpp)
    create_test_executable(test_sidebar_integration
        SOURCES ui/SideBarIntegrationTest.cpp
        CATEGORY integration
        LINK_LIBRARIES UILib CoreAppLib ManagerLib ControllerLib FactoryLib SearchLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# StatusBar Integration Test
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/StatusBarIntegrationTest.cpp)
    create_test_executable(test_statusbar_integration
        SOURCES ui/StatusBarIntegrationTest.cpp
        CATEGORY integration
        LINK_LIBRARIES UILib CoreAppLib ManagerLib ControllerLib FactoryLib SearchLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# RightSideBar Integration Test
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/RightSideBarIntegrationTest.cpp)
    create_test_executable(test_rightsidebar_integration
        SOURCES ui/RightSideBarIntegrationTest.cpp
        CATEGORY integration
        LINK_LIBRARIES UILib CoreAppLib ManagerLib ControllerLib FactoryLib SearchLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# ViewWidget Integration Test
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/ViewWidgetIntegrationTest.cpp)
    create_test_executable(test_viewwidget_integration
        SOURCES ui/ViewWidgetIntegrationTest.cpp
        CATEGORY integration
        LINK_LIBRARIES UILib CoreAppLib ManagerLib ControllerLib FactoryLib SearchLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# ============================================================================
# New UI Integration Tests - Dialog Components
# ============================================================================

# DocumentComparison Integration Test
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/DocumentComparisonIntegrationTest.cpp)
    create_test_executable(test_documentcomparison_integration
        SOURCES ui/DocumentComparisonIntegrationTest.cpp
        CATEGORY integration
        LINK_LIBRARIES UILib CoreAppLib ManagerLib ControllerLib FactoryLib SearchLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# DocumentMetadataDialog Integration Test
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/DocumentMetadataDialogIntegrationTest.cpp)
    create_test_executable(test_documentmetadatadialog_integration
        SOURCES ui/DocumentMetadataDialogIntegrationTest.cpp
        CATEGORY integration
        LINK_LIBRARIES UILib CoreAppLib ManagerLib ControllerLib FactoryLib SearchLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# ============================================================================
# New UI Integration Tests - Manager Components
# ============================================================================

# WelcomeScreenManager Integration Test
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/WelcomeScreenManagerIntegrationTest.cpp)
    create_test_executable(test_welcomescreenmanager_integration
        SOURCES ui/WelcomeScreenManagerIntegrationTest.cpp
        CATEGORY integration
        LINK_LIBRARIES UILib CoreAppLib ManagerLib ControllerLib FactoryLib SearchLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# ============================================================================
# New UI Integration Tests - Thumbnail Components
# ============================================================================

# ThumbnailContextMenu Integration Test
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/ThumbnailContextMenuIntegrationTest.cpp)
    create_test_executable(test_thumbnailcontextmenu_integration
        SOURCES ui/ThumbnailContextMenuIntegrationTest.cpp
        CATEGORY integration
        LINK_LIBRARIES UILib CoreAppLib ManagerLib ControllerLib FactoryLib SearchLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# ThumbnailListView Integration Test
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/ThumbnailListViewIntegrationTest.cpp)
    create_test_executable(test_thumbnaillistview_integration
        SOURCES ui/ThumbnailListViewIntegrationTest.cpp
        CATEGORY integration
        LINK_LIBRARIES UILib CoreAppLib ManagerLib ControllerLib FactoryLib SearchLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# ThumbnailWidget Integration Test
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/ThumbnailWidgetIntegrationTest.cpp)
    create_test_executable(test_thumbnailwidget_integration
        SOURCES ui/ThumbnailWidgetIntegrationTest.cpp
        CATEGORY integration
        LINK_LIBRARIES UILib CoreAppLib ManagerLib ControllerLib FactoryLib SearchLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# ============================================================================
# New UI Integration Tests - Viewer Components
# ============================================================================

# PDFOutlineWidget Integration Test
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/PDFOutlineWidgetIntegrationTest.cpp)
    create_test_executable(test_pdfoutlinewidget_integration
        SOURCES ui/PDFOutlineWidgetIntegrationTest.cpp
        CATEGORY integration
        LINK_LIBRARIES UILib CoreAppLib ManagerLib ControllerLib FactoryLib SearchLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# PDFViewer Integration Test
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/PDFViewerIntegrationTest.cpp)
    create_test_executable(test_pdfviewer_integration
        SOURCES ui/PDFViewerIntegrationTest.cpp
        CATEGORY integration
        LINK_LIBRARIES UILib CoreAppLib ManagerLib ControllerLib FactoryLib SearchLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# PDFAnimations Integration Test
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/PDFAnimationsIntegrationTest.cpp)
    create_test_executable(test_pdfanimations_integration
        SOURCES ui/PDFAnimationsIntegrationTest.cpp
        CATEGORY integration
        LINK_LIBRARIES UILib CoreAppLib ManagerLib ControllerLib FactoryLib SearchLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# SearchWidget Integration Test
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/SearchWidgetIntegrationTest.cpp)
    create_test_executable(test_searchwidget_integration
        SOURCES ui/SearchWidgetIntegrationTest.cpp
        CATEGORY integration
        LINK_LIBRARIES UILib CoreAppLib ManagerLib SearchLib ControllerLib FactoryLib
        TIMEOUT 300
    )
endif()

# ============================================================================
# Performance Tests
# ============================================================================

# Rendering Performance Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/performance/test_rendering_performance.cpp)
    create_test_executable(test_rendering_performance
        SOURCES performance/test_rendering_performance.cpp
        CATEGORY performance
        LINK_LIBRARIES CoreAppLib UILib ManagerLib LoggingLib
        TIMEOUT 900
    )
endif()

# Search Optimizations Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/performance/test_search_optimizations.cpp)
    create_test_executable(test_search_optimizations
        SOURCES performance/test_search_optimizations.cpp
        CATEGORY performance
        LINK_LIBRARIES SearchLib CoreAppLib UILib ManagerLib LoggingLib
        TIMEOUT 900
    )
endif()

# ============================================================================
# Real World Tests
# ============================================================================

# Real PDF Documents Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/real_world/test_real_pdf_documents.cpp)
    create_test_executable(test_real_pdf_documents
        SOURCES real_world/test_real_pdf_documents.cpp
        CATEGORY real_world
        LINK_LIBRARIES CoreAppLib UILib SearchLib ManagerLib LoggingLib
        TIMEOUT 1200
    )
endif()

# ============================================================================
# Logging Tests
# ============================================================================

# Logging Stubs Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/logging/logging_stubs_test.cpp)
    create_test_executable(logging_stubs_test
        SOURCES logging/logging_stubs_test.cpp
        CATEGORY unit
        LINK_LIBRARIES LoggingLib TestUtilities CoreAppLib UILib ManagerLib ControllerLib FactoryLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# Minimal LoggingStubs test for debugging
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/logging/logging_stubs_test_minimal.cpp)
    create_test_executable(logging_stubs_test_minimal
        SOURCES logging/logging_stubs_test_minimal.cpp
        CATEGORY unit
        LINK_LIBRARIES LoggingLib TestUtilities CoreAppLib UILib ManagerLib ControllerLib FactoryLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# Memory Management Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/memory_aware_search_results_test.cpp)
    create_test_executable(memory_aware_search_results_test
        SOURCES search/memory_aware_search_results_test.cpp
        CATEGORY unit
        LINK_LIBRARIES SearchLib TestUtilities CoreAppLib LoggingLib UILib ManagerLib ControllerLib FactoryLib
        TIMEOUT 300
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/smart_eviction_policy_test.cpp)
    create_test_executable(smart_eviction_policy_test
        SOURCES search/smart_eviction_policy_test.cpp
        CATEGORY unit
        LINK_LIBRARIES SearchLib TestUtilities CoreAppLib LoggingLib UILib ManagerLib ControllerLib FactoryLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# Minimal SmartEvictionPolicy test for debugging
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/search/smart_eviction_policy_test_minimal.cpp)
    create_test_executable(smart_eviction_policy_test_minimal
        SOURCES search/smart_eviction_policy_test_minimal.cpp
        CATEGORY unit
        LINK_LIBRARIES SearchLib TestUtilities CoreAppLib LoggingLib UILib ManagerLib ControllerLib FactoryLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# ============================================================================
# Component Tests
# ============================================================================

# Controller Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/service_locator_test.cpp)
    create_test_executable(service_locator_test
        SOURCES controller/service_locator_test.cpp
        CATEGORY unit
        LINK_LIBRARIES CoreAppLib ControllerLib UILib ManagerLib LoggingLib MainLib TestUtilities
        TIMEOUT 300
    )
endif()

# ApplicationController Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/application_controller_test.cpp)
    create_test_executable(application_controller_test
        SOURCES controller/application_controller_test.cpp
        CATEGORY unit
        LINK_LIBRARIES ControllerTestMocks TestUtilities
        TIMEOUT 300
    )
endif()

# DocumentController Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/document_controller_test.cpp)
    create_test_executable(document_controller_test
        SOURCES controller/document_controller_test.cpp
        CATEGORY unit
        LINK_LIBRARIES CoreAppLib ControllerLib UILib ManagerLib LoggingLib MainLib TestUtilities
        TIMEOUT 300
    )
endif()

# PageController Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/page_controller_test.cpp)
    create_test_executable(page_controller_test
        SOURCES controller/page_controller_test.cpp
        CATEGORY unit
        LINK_LIBRARIES CoreAppLib ControllerLib UILib ManagerLib LoggingLib MainLib TestUtilities
        TIMEOUT 300
    )
endif()

# ActionMap Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/action_map_test.cpp)
    create_test_executable(action_map_test
        SOURCES controller/action_map_test.cpp
        CATEGORY unit
        LINK_LIBRARIES TestUtilities
        TIMEOUT 300
    )
endif()

# ConfigurationManager Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/configuration_manager_test.cpp)
    create_test_executable(configuration_manager_test
        SOURCES controller/configuration_manager_test.cpp
        CATEGORY unit
        LINK_LIBRARIES CoreAppLib ControllerLib ManagerLib LoggingLib TestUtilities UILib FactoryLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# EventBus Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/event_bus_test.cpp)
    create_test_executable(event_bus_test
        SOURCES controller/event_bus_test.cpp
        CATEGORY unit
        LINK_LIBRARIES CoreAppLib ControllerLib ManagerLib LoggingLib TestUtilities UILib FactoryLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# Controller Test Mocks Library
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/controller/ControllerTestMocks.cpp)
    create_test_library(ControllerTestMocks
        SOURCES controller/ControllerTestMocks.h controller/ControllerTestMocks.cpp
        DESCRIPTION "Mock objects for controller testing"
    )
endif()

# Factory Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/factory/model_factory_test.cpp)
    create_test_executable(model_factory_test
        SOURCES factory/model_factory_test.cpp
        CATEGORY unit
        LINK_LIBRARIES CoreAppLib FactoryLib ManagerLib LoggingLib TestUtilities UILib ControllerLib CommandLib MainLib
        TIMEOUT 300
    )
endif()

# Command Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/command/initialization_command_test.cpp)
    create_test_executable(initialization_command_test
        SOURCES command/initialization_command_test.cpp
        CATEGORY unit
        LINK_LIBRARIES CoreAppLib CommandLib ManagerLib LoggingLib TestUtilities UILib ControllerLib FactoryLib MainLib
        TIMEOUT 300
    )
endif()

# CommandManager Tests (basic Qt test)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/command/command_manager_test.cpp)
    create_test_executable(command_manager_test
        SOURCES command/command_manager_test.cpp
        CATEGORY unit
        LINK_LIBRARIES TestUtilities
        TIMEOUT 300
    )
endif()

# DocumentCommands Tests (basic Qt test)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/command/document_commands_test.cpp)
    create_test_executable(document_commands_test
        SOURCES command/document_commands_test.cpp
        CATEGORY unit
        LINK_LIBRARIES TestUtilities
        TIMEOUT 300
    )
endif()

# NavigationCommands Tests (basic Qt test)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/command/navigation_commands_test.cpp)
    create_test_executable(navigation_commands_test
        SOURCES command/navigation_commands_test.cpp
        CATEGORY unit
        LINK_LIBRARIES TestUtilities
        TIMEOUT 300
    )
endif()



# ============================================================================
# Smoke Test
# ============================================================================

# Create a simple smoke test that always exists
create_test_executable(smoke_test
    SOURCES smoke_test.cpp
    CATEGORY smoke
    LINK_LIBRARIES CoreAppLib UILib ManagerLib LoggingLib
    TIMEOUT 30
)

# ============================================================================
# Test Categories and Custom Targets
# ============================================================================

# Add test categories for organized execution
add_test_categories()

# Add custom target for UI integration tests
add_custom_target(run_ui_integration_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "test_.*_integration" -E "test_(debug_log_panel|toolbar|thumbnail_generator)_integration"
    COMMENT "Running UI integration tests..."
    VERBATIM
)

# Print configuration information
message(STATUS "")
message(STATUS "QGraphics PDF Support Tests Configuration:")
message(STATUS "  ENABLE_QGRAPHICS_PDF_SUPPORT: ${ENABLE_QGRAPHICS_PDF_SUPPORT}")
message(STATUS "  Qt6 version: ${Qt6_VERSION}")
message(STATUS "  Poppler-Qt6 found: 1")
message(STATUS "  Test timeout: 300 seconds")
message(STATUS "")
