# TestUtils.cmake - Test creation and management utilities for SAST Readium
# This module provides functions for standardized test setup and execution

cmake_minimum_required(VERSION 3.28)

include(Dependencies)
include(TargetUtils)

#[=======================================================================[.rst:
setup_testing_environment
-------------------------

Set up the testing environment with common configuration.

.. code-block:: cmake

  setup_testing_environment()

This function:
- Enables testing with enable_testing()
- Sets up common test settings (AUTOMOC, etc.)
- Configures test-specific compile definitions
- Sets up test library dependencies

#]=======================================================================]
function(setup_testing_environment)
    message(STATUS "Setting up testing environment...")
    
    # Enable testing
    enable_testing()
    
    # Common test settings
    set(CMAKE_AUTOMOC ON PARENT_SCOPE)
    set(CMAKE_AUTORCC ON PARENT_SCOPE)
    set(CMAKE_AUTOUIC ON PARENT_SCOPE)
    
    # Test-specific compile definitions
    _setup_test_definitions()
    
    message(STATUS "Testing environment configured")
endfunction()

#[=======================================================================[.rst:
create_test_executable
----------------------

Create a test executable with standardized configuration.

.. code-block:: cmake

  create_test_executable(test_name
    SOURCES source1 source2 ...
    [CATEGORY unit|integration|performance]
    [TIMEOUT seconds]
    [WORKING_DIRECTORY dir]
    [ENVIRONMENT var1=value1 var2=value2 ...]
    [LINK_LIBRARIES lib1 lib2 ...]
  )

Arguments:
  test_name         Name of the test executable
  SOURCES          List of test source files
  CATEGORY         Test category for organization
  TIMEOUT          Test timeout in seconds (default: 300)
  WORKING_DIRECTORY Working directory for test execution
  ENVIRONMENT      Environment variables for test
  LINK_LIBRARIES   Additional libraries to link

#]=======================================================================]
function(create_test_executable test_name)
    cmake_parse_arguments(TEST 
        "" 
        "CATEGORY;TIMEOUT;WORKING_DIRECTORY" 
        "SOURCES;ENVIRONMENT;LINK_LIBRARIES" 
        ${ARGN}
    )
    
    if(NOT TEST_SOURCES)
        message(FATAL_ERROR "SOURCES must be specified for test ${test_name}")
    endif()
    
    message(STATUS "Creating test executable: ${test_name}")
    
    # Create the test executable
    add_executable(${test_name} ${TEST_SOURCES})
    
    # Get test libraries
    get_test_libraries(test_libs)
    if(TEST_LINK_LIBRARIES)
        list(APPEND test_libs ${TEST_LINK_LIBRARIES})
    endif()
    
    # Configure the test target
    setup_target(${test_name}
        TYPE EXECUTABLE
        LINK_LIBRARIES ${test_libs}
    )

    # Add app directory to include path for tests to find application headers
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/app
    )
    
    # Set output directory
    set(category ${TEST_CATEGORY})
    if(NOT category)
        set(category "general")
    endif()
    
    set_target_properties(${test_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/${category}
    )
    
    # Add to CTest
    _register_test_with_ctest(${test_name} "${TEST_TIMEOUT}" "${TEST_WORKING_DIRECTORY}" "${TEST_ENVIRONMENT}")
    
    message(STATUS "Test executable ${test_name} created successfully")
endfunction()

#[=======================================================================[.rst:
create_test_library
-------------------

Create a library specifically for testing purposes.

.. code-block:: cmake

  create_test_library(library_name
    SOURCES source1 source2 ...
    [TYPE STATIC|SHARED]
    [LINK_LIBRARIES lib1 lib2 ...]
  )

Arguments:
  library_name     Name of the test library
  SOURCES         List of source files
  TYPE            Library type (default: STATIC)
  LINK_LIBRARIES  Libraries to link against

#]=======================================================================]
function(create_test_library library_name)
    cmake_parse_arguments(LIB "" "TYPE;DESCRIPTION" "SOURCES;LINK_LIBRARIES" ${ARGN})
    
    if(NOT LIB_SOURCES)
        message(FATAL_ERROR "SOURCES must be specified for test library ${library_name}")
    endif()
    
    message(STATUS "Creating test library: ${library_name}")
    
    # Default to STATIC library
    set(lib_type ${LIB_TYPE})
    if(NOT lib_type)
        set(lib_type STATIC)
    endif()
    
    # Create the library
    add_library(${library_name} ${lib_type} ${LIB_SOURCES})
    
    # Get test libraries for linking
    get_test_libraries(test_libs)
    if(LIB_LINK_LIBRARIES)
        list(APPEND test_libs ${LIB_LINK_LIBRARIES})
    endif()
    
    # Configure the library
    setup_target(${library_name}
        TYPE LIBRARY
        LINK_LIBRARIES ${test_libs}
    )
    
    message(STATUS "Test library ${library_name} created successfully")
endfunction()

#[=======================================================================[.rst:
add_test_categories
------------------

Add custom targets for running different categories of tests.

.. code-block:: cmake

  add_test_categories()

Creates the following custom targets:
- run_all_tests: Run all tests
- run_unit_tests: Run unit tests only
- run_integration_tests: Run integration tests only
- run_performance_tests: Run performance tests only

#]=======================================================================]
function(add_test_categories)
    message(STATUS "Adding test category targets...")
    
    # Run all tests
    add_custom_target(run_all_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
        COMMENT "Running all tests..."
        VERBATIM
    )
    
    # Run unit tests only
    add_custom_target(run_unit_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "unit"
        COMMENT "Running unit tests..."
        VERBATIM
    )
    
    # Run integration tests only
    add_custom_target(run_integration_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "integration"
        COMMENT "Running integration tests..."
        VERBATIM
    )
    
    # Run performance tests only
    add_custom_target(run_performance_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "performance"
        COMMENT "Running performance tests..."
        VERBATIM
    )
    
    # Generate test report
    add_custom_target(test_report
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose > test_report.txt 2>&1
        COMMAND ${CMAKE_COMMAND} -E echo "Test report generated in test_report.txt"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests
        COMMENT "Generating test report..."
        VERBATIM
    )
    
    message(STATUS "Test category targets added")
endfunction()

#[=======================================================================[.rst:
setup_test_coverage
-------------------

Set up test coverage reporting (if available).

.. code-block:: cmake

  setup_test_coverage()

Configures coverage reporting using:
- gcov/lcov for GCC
- llvm-cov for Clang
- Requires Debug build type

#]=======================================================================]
function(setup_test_coverage)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Coverage requires Debug build - skipping coverage setup")
        return()
    endif()
    
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Setting up test coverage...")
        
        # Find coverage tools
        find_program(LCOV_EXECUTABLE lcov)
        find_program(GENHTML_EXECUTABLE genhtml)
        
        if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
            # Add coverage compile flags
            add_compile_options(--coverage)
            add_link_options(--coverage)
            
            # Add coverage target
            add_custom_target(coverage
                COMMAND ${LCOV_EXECUTABLE} --capture --directory . --output-file coverage.info
                COMMAND ${LCOV_EXECUTABLE} --remove coverage.info '/usr/*' '*/test/*' --output-file coverage.info.cleaned
                COMMAND ${GENHTML_EXECUTABLE} -o coverage coverage.info.cleaned
                COMMAND ${CMAKE_COMMAND} -E echo "Coverage report generated in coverage/index.html"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating coverage report..."
                VERBATIM
            )
            
            message(STATUS "Coverage reporting configured")
        else()
            message(WARNING "lcov/genhtml not found - coverage reporting disabled")
        endif()
    else()
        message(STATUS "Coverage reporting not supported for ${CMAKE_CXX_COMPILER_ID}")
    endif()
endfunction()

#[=======================================================================[.rst:
_setup_test_definitions
-----------------------

Internal function to set up test-specific compile definitions.
#]=======================================================================]
function(_setup_test_definitions)
    # Add QGraphics support definition if enabled
    if(ENABLE_QGRAPHICS_PDF_SUPPORT)
        add_compile_definitions(ENABLE_QGRAPHICS_PDF_SUPPORT)
    endif()
    
    # Test-specific definitions
    add_compile_definitions(TESTING_BUILD)
endfunction()

#[=======================================================================[.rst:
_register_test_with_ctest
------------------------

Internal function to register a test with CTest.
#]=======================================================================]
function(_register_test_with_ctest test_name timeout working_dir environment)
    # Add to CTest
    add_test(NAME ${test_name} COMMAND ${test_name})
    
    # Set test properties
    set(test_properties)
    
    # Timeout
    set(test_timeout ${timeout})
    if(NOT test_timeout)
        set(test_timeout 300)  # 5 minutes default
    endif()
    list(APPEND test_properties TIMEOUT ${test_timeout})
    
    # Working directory
    if(working_dir)
        list(APPEND test_properties WORKING_DIRECTORY ${working_dir})
    endif()
    
    # Environment variables
    set(test_env "QT_QPA_PLATFORM=offscreen")  # Default for headless testing
    if(environment)
        list(APPEND test_env ${environment})
    endif()
    list(APPEND test_properties ENVIRONMENT "${test_env}")
    
    # Apply properties
    set_tests_properties(${test_name} PROPERTIES ${test_properties})
    
    message(STATUS "  Registered with CTest (timeout: ${test_timeout}s)")
endfunction()
