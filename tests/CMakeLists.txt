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

# Rendering Mode Switch Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration/test_rendering_mode_switch.cpp)
    create_test_target(test_rendering_mode_switch
        SOURCES integration/test_rendering_mode_switch.cpp
        TIMEOUT 600
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
