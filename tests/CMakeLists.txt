# Tests CMakeLists.txt for QGraphics PDF Support

cmake_minimum_required(VERSION 3.16)

# Find required packages for testing
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test Network)
find_package(spdlog REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER_QT6 REQUIRED poppler-qt6)

# Enable testing
enable_testing()

# Common test settings
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Common include directories
set(COMMON_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../app
    ${Qt6Core_INCLUDE_DIRS}
    ${Qt6Widgets_INCLUDE_DIRS}
    ${Qt6Test_INCLUDE_DIRS}
    ${POPPLER_QT6_INCLUDE_DIRS}
)

# Common libraries
set(COMMON_LIBRARIES
    Qt6::Core
    Qt6::Widgets
    Qt6::Test
    Qt6::Network
    ${POPPLER_QT6_LIBRARIES}
    spdlog::spdlog
)

# Common compile definitions
set(COMMON_DEFINITIONS
    ${Qt6Core_DEFINITIONS}
    ${Qt6Widgets_DEFINITIONS}
    ${Qt6Test_DEFINITIONS}
)

# Add QGraphics support definition if enabled
if(ENABLE_QGRAPHICS_PDF_SUPPORT)
    list(APPEND COMMON_DEFINITIONS ENABLE_QGRAPHICS_PDF_SUPPORT)
endif()

# Function to create a test executable
function(create_test_executable TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    target_include_directories(${TEST_NAME} PRIVATE ${COMMON_INCLUDE_DIRS})
    target_link_libraries(${TEST_NAME} PRIVATE ${COMMON_LIBRARIES})
    target_compile_definitions(${TEST_NAME} PRIVATE ${COMMON_DEFINITIONS})
    
    # Link against main application sources (needed for testing)
    target_sources(${TEST_NAME} PRIVATE
        # Core viewer sources
        ../app/ui/viewer/PDFViewer.cpp
        ../app/ui/viewer/PDFOutlineWidget.cpp
        ../app/ui/viewer/PDFViewerEnhancements.cpp
        ../app/ui/viewer/PDFAnimations.cpp
        ../app/ui/viewer/PDFPrerenderer.cpp

        # Model sources
        ../app/model/DocumentModel.cpp
        ../app/model/SearchModel.cpp
        ../app/model/PDFOutlineModel.cpp
        ../app/model/AsyncDocumentLoader.cpp

        # Search engine sources
        ../app/search/OptimizedSearchEngine.cpp

        # Cache sources
        ../app/cache/SearchResultCache.cpp
        ../app/cache/PDFCacheManager.cpp

        # Manager sources
        ../app/managers/StyleManager.cpp

        # Widget sources
        ../app/ui/widgets/SearchWidget.cpp

        # Utility sources
        ../app/utils/Logger.cpp
        ../app/utils/QtSpdlogBridge.cpp
        ../app/utils/LoggingManager.cpp
        ../app/utils/LoggingMacros.cpp
        ../app/utils/LoggingConfig.cpp
        ../app/utils/ErrorHandling.cpp
        ../app/utils/ErrorRecovery.cpp
        ../app/utils/PDFUtilities.cpp

        # QGraphics sources (conditionally compiled)
        ../app/ui/viewer/QGraphicsPDFViewer.cpp
    )
    
    # Set output directory
    set_target_properties(${TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/${ARGV2}
    )
    
    # Add to CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    
    # Set test properties
    set_tests_properties(${TEST_NAME} PROPERTIES
        TIMEOUT 300  # 5 minutes timeout
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"  # Run tests headless
    )
endfunction()

# Unit Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_qgraphics_components.cpp)
    create_test_executable(test_qgraphics_components
        unit/test_qgraphics_components.cpp
        unit)
endif()

# PDF Optimizations Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_pdf_optimizations.cpp)
    create_test_executable(test_pdf_optimizations
        unit/test_pdf_optimizations.cpp
        unit)
endif()

# Error Handling Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_error_handling.cpp)
    create_test_executable(test_error_handling
        unit/test_error_handling.cpp
        unit)
endif()

# Error Recovery Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_error_recovery.cpp)
    create_test_executable(test_error_recovery
        unit/test_error_recovery.cpp
        unit)
endif()

# Thread Safety Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_thread_safety.cpp)
    # Find Google Test for thread safety tests
    find_package(GTest QUIET)
    if(GTest_FOUND)
        create_test_executable(test_thread_safety
            unit/test_thread_safety.cpp
            unit)
        target_link_libraries(test_thread_safety PRIVATE GTest::gtest GTest::gtest_main)

        # Add thumbnail generator sources
        target_sources(test_thread_safety PRIVATE
            ../app/ui/thumbnail/ThumbnailGenerator.cpp
        )

        message(STATUS "Thread safety tests enabled with Google Test")
    else()
        message(WARNING "Google Test not found, thread safety tests disabled")
    endif()
endif()

# Integration Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration/test_rendering_mode_switch.cpp)
    create_test_executable(test_rendering_mode_switch 
        integration/test_rendering_mode_switch.cpp 
        integration)
endif()

# Performance Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/performance/test_rendering_performance.cpp)
    create_test_executable(test_rendering_performance
        performance/test_rendering_performance.cpp
        performance)
endif()

# Search Optimization Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/performance/test_search_optimizations.cpp)
    create_test_executable(test_search_optimizations
        performance/test_search_optimizations.cpp
        performance)
endif()

# Enhanced Search Engine Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_search_engine_core.cpp)
    create_test_executable(test_search_engine_core
        unit/test_search_engine_core.cpp
        unit)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_search_advanced_features.cpp)
    create_test_executable(test_search_advanced_features
        unit/test_search_advanced_features.cpp
        unit)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_search_performance_caching.cpp)
    create_test_executable(test_search_performance_caching
        unit/test_search_performance_caching.cpp
        unit)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_search_edge_cases.cpp)
    create_test_executable(test_search_edge_cases
        unit/test_search_edge_cases.cpp
        unit)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration/test_search_integration.cpp)
    create_test_executable(test_search_integration
        integration/test_search_integration.cpp
        integration)
endif()

# Real World Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/real_world/test_real_pdf_documents.cpp)
    create_test_executable(test_real_pdf_documents 
        real_world/test_real_pdf_documents.cpp 
        real_world)
endif()

# Create a simple smoke test that always exists
add_executable(smoke_test smoke_test.cpp)
target_include_directories(smoke_test PRIVATE ${COMMON_INCLUDE_DIRS})
target_link_libraries(smoke_test PRIVATE ${COMMON_LIBRARIES})
target_compile_definitions(smoke_test PRIVATE ${COMMON_DEFINITIONS})

set_target_properties(smoke_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

add_test(NAME smoke_test COMMAND smoke_test)
set_tests_properties(smoke_test PROPERTIES
    TIMEOUT 30
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

# Print configuration information
message(STATUS "QGraphics PDF Support Tests Configuration:")
message(STATUS "  ENABLE_QGRAPHICS_PDF_SUPPORT: ${ENABLE_QGRAPHICS_PDF_SUPPORT}")
message(STATUS "  Qt6 version: ${Qt6_VERSION}")
message(STATUS "  Poppler-Qt6 found: ${POPPLER_QT6_FOUND}")
message(STATUS "  Test timeout: 300 seconds")

# Custom target to run all tests
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS smoke_test
    COMMENT "Running all QGraphics PDF support tests"
)

# Custom target to run tests with different configurations
add_custom_target(test_configurations
    COMMAND ${CMAKE_COMMAND} -E echo "Testing QGraphics PDF Support Configurations"
    COMMAND ${CMAKE_COMMAND} -E echo "Current configuration: ENABLE_QGRAPHICS_PDF_SUPPORT=${ENABLE_QGRAPHICS_PDF_SUPPORT}"
    DEPENDS run_all_tests
    COMMENT "Testing different QGraphics configurations"
)
