# TargetUtils.cmake - Target setup, source discovery, and testing utilities for SAST Readium
# This module consolidates target creation, source discovery, component libraries, and testing

cmake_minimum_required(VERSION 3.28)

include(Dependencies)

#[=======================================================================[.rst:
discover_app_sources
--------------------

Automatically discover application source files organized by component.

.. code-block:: cmake

  discover_app_sources(output_var [BASE_DIR directory])

Arguments:
  output_var    Variable to store discovered source files
  BASE_DIR      Base directory for source discovery (default: CMAKE_CURRENT_SOURCE_DIR)

This function discovers sources in standard application components:
- ui/core, ui/viewer, ui/widgets, ui/dialogs, ui/thumbnail, ui/managers
- managers, model, controller, delegate, view, cache, utils
- plugin, factory, command, search, logging

#]=======================================================================]
function(discover_app_sources output_var)
    cmake_parse_arguments(APP "" "BASE_DIR" "" ${ARGN})

    set(base_dir ${APP_BASE_DIR})
    if(NOT base_dir)
        set(base_dir ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    message(VERBOSE "Discovering application sources in: ${base_dir}")

    # Define standard application components
    set(app_components
        ui/core ui/viewer ui/widgets ui/dialogs ui/thumbnail ui/managers ui/pages
        managers model controller delegate view cache utils adapters
        plugin factory command search logging
    )

    set(discovered_sources)

    # Discover sources in each component
    foreach(component ${app_components})
        set(component_dir "${base_dir}/${component}")
        if(EXISTS "${component_dir}")
            file(GLOB_RECURSE component_sources
                "${component_dir}/*.cpp"
                "${component_dir}/*.cc"
                "${component_dir}/*.cxx"
            )

            # Exclude test files
            list(FILTER component_sources EXCLUDE REGEX ".*_test\\.(cpp|cc|cxx)$")
            list(FILTER component_sources EXCLUDE REGEX ".*Test\\.(cpp|cc|cxx)$")
            list(FILTER component_sources EXCLUDE REGEX "^test_.*\\.(cpp|cc|cxx)$")

            if(component_sources)
                list(APPEND discovered_sources ${component_sources})
                list(LENGTH component_sources source_count)
                message(VERBOSE "  Found ${source_count} sources in ${component}")
            endif()
        endif()
    endforeach()

    # Also include main source files in base directory
    file(GLOB base_sources
        "${base_dir}/*.cpp"
        "${base_dir}/*.cc"
        "${base_dir}/*.cxx"
    )
    list(FILTER base_sources EXCLUDE REGEX ".*_test\\.(cpp|cc|cxx)$")
    list(FILTER base_sources EXCLUDE REGEX ".*Test\\.(cpp|cc|cxx)$")
    list(FILTER base_sources EXCLUDE REGEX "^test_.*\\.(cpp|cc|cxx)$")

    if(base_sources)
        list(APPEND discovered_sources ${base_sources})
        list(LENGTH base_sources source_count)
        message(VERBOSE "  Found ${source_count} sources in base directory")
    endif()

    list(LENGTH discovered_sources total_count)
    message(VERBOSE "Total application sources discovered: ${total_count}")

    set(${output_var} ${discovered_sources} PARENT_SCOPE)
endfunction()

#[=======================================================================[.rst:
validate_discovered_sources
---------------------------

Validate that critical source files were discovered.

.. code-block:: cmake

  validate_discovered_sources(source_list
    REQUIRED_FILES file1 file2 ...
    [REQUIRED_PATTERNS pattern1 pattern2 ...]
  )

Arguments:
  source_list        List of discovered source files
  REQUIRED_FILES     List of required filenames (basename only)
  REQUIRED_PATTERNS  List of regex patterns that must match at least one file

#]=======================================================================]
function(validate_discovered_sources source_list)
    cmake_parse_arguments(VALIDATE "" "" "REQUIRED_FILES;REQUIRED_PATTERNS" ${ARGN})

    # Check required files
    if(VALIDATE_REQUIRED_FILES)
        foreach(required_file ${VALIDATE_REQUIRED_FILES})
            set(found FALSE)
            foreach(source ${source_list})
                get_filename_component(basename ${source} NAME)
                if(basename STREQUAL required_file)
                    set(found TRUE)
                    break()
                endif()
            endforeach()

            if(NOT found)
                message(FATAL_ERROR "Required source file not found: ${required_file}")
            endif()
        endforeach()
    endif()

    # Check required patterns
    if(VALIDATE_REQUIRED_PATTERNS)
        foreach(pattern ${VALIDATE_REQUIRED_PATTERNS})
            set(found FALSE)
            foreach(source ${source_list})
                if(source MATCHES ${pattern})
                    set(found TRUE)
                    break()
                endif()
            endforeach()

            if(NOT found)
                message(FATAL_ERROR "No source file matches required pattern: ${pattern}")
            endif()
        endforeach()
    endif()

    message(VERBOSE "Source validation passed")
endfunction()

#[=======================================================================[.rst:
setup_target
-----------

Set up common configuration for a target.

.. code-block:: cmake

  setup_target(target_name
    [TYPE EXECUTABLE|LIBRARY]
    [SOURCES source1 source2 ...]
    [INCLUDE_DIRS dir1 dir2 ...]
    [LINK_LIBRARIES lib1 lib2 ...]
    [COMPILE_DEFINITIONS def1 def2 ...]
    [CXX_STANDARD standard]
  )

Arguments:
  target_name         Name of the target
  TYPE               Target type (default: EXECUTABLE)
  SOURCES            Source files for the target
  INCLUDE_DIRS       Include directories
  LINK_LIBRARIES     Libraries to link
  COMPILE_DEFINITIONS Compile definitions
  CXX_STANDARD       C++ standard (default: 20)

#]=======================================================================]
function(setup_target target_name)
    cmake_parse_arguments(TARGET "" "TYPE;CXX_STANDARD" "SOURCES;INCLUDE_DIRS;LINK_LIBRARIES;COMPILE_DEFINITIONS" ${ARGN})

    # Set defaults
    if(NOT TARGET_TYPE)
        set(TARGET_TYPE EXECUTABLE)
    endif()

    if(NOT TARGET_CXX_STANDARD)
        set(TARGET_CXX_STANDARD 20)
    endif()

    message(VERBOSE "Setting up target: ${target_name} (${TARGET_TYPE})")

    # Create target
    if(TARGET_TYPE STREQUAL "EXECUTABLE")
        add_executable(${target_name} ${TARGET_SOURCES})
    elseif(TARGET_TYPE STREQUAL "LIBRARY")
        add_library(${target_name} ${TARGET_SOURCES})
    else()
        message(FATAL_ERROR "Unknown target type: ${TARGET_TYPE}")
    endif()

    # Set C++ standard
    set_target_properties(${target_name} PROPERTIES
        CXX_STANDARD ${TARGET_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    # Add include directories
    if(TARGET_INCLUDE_DIRS)
        target_include_directories(${target_name} PRIVATE ${TARGET_INCLUDE_DIRS})
    endif()

    # Link libraries
    if(TARGET_LINK_LIBRARIES)
        target_link_libraries(${target_name} PRIVATE ${TARGET_LINK_LIBRARIES})
    endif()

    # Add compile definitions
    if(TARGET_COMPILE_DEFINITIONS)
        target_compile_definitions(${target_name} PRIVATE ${TARGET_COMPILE_DEFINITIONS})
    endif()

    # Get common libraries and link them
    get_common_libraries(common_libs)
    target_link_libraries(${target_name} PRIVATE ${common_libs})

    message(VERBOSE "Target ${target_name} configured successfully")
endfunction()

#[=======================================================================[.rst:
setup_testing_environment
-------------------------

Set up the testing environment for the project.

.. code-block:: cmake

  setup_testing_environment()

This function:
- Enables testing with enable_testing()
- Sets up common test settings (AUTOMOC, etc.)
- Configures test-specific compile definitions

#]=======================================================================]
function(setup_testing_environment)
    message(VERBOSE "Setting up testing environment...")

    # Note: enable_testing() must be called at top level, not in function
    # This is handled in the root CMakeLists.txt

    # Common test settings
    set(CMAKE_AUTOMOC ON PARENT_SCOPE)
    set(CMAKE_AUTORCC ON PARENT_SCOPE)
    set(CMAKE_AUTOUIC ON PARENT_SCOPE)

    message(VERBOSE "Testing environment configured")
endfunction()

#[=======================================================================[.rst:
create_test_target
------------------

Create a test target with standard configuration.

.. code-block:: cmake

  create_test_target(test_name
    SOURCES source1 source2 ...
    [LINK_LIBRARIES lib1 lib2 ...]
    [WORKING_DIRECTORY directory]
    [TIMEOUT seconds]
  )

Arguments:
  test_name         Name of the test
  SOURCES          Test source files
  LINK_LIBRARIES   Additional libraries to link
  WORKING_DIRECTORY Working directory for test execution
  TIMEOUT          Test timeout in seconds (default: 30)

#]=======================================================================]
function(create_test_target test_name)
    cmake_parse_arguments(TEST "" "WORKING_DIRECTORY;TIMEOUT" "SOURCES;LINK_LIBRARIES" ${ARGN})

    if(NOT BUILD_TESTING)
        return()
    endif()

    message(VERBOSE "Creating test target: ${test_name}")

    # Create test executable
    add_executable(${test_name} ${TEST_SOURCES})

    # Set C++ standard
    set_target_properties(${test_name} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    # Reduce GNU ld memory footprint when linking large test binaries on MinGW
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_options(${test_name} PRIVATE "-Wl,--no-keep-memory" "-Wl,--reduce-memory-overheads")
    endif()

    # Add include directories for tests to access app sources
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/app
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # Get test libraries
    get_test_libraries(test_libs)
    target_link_libraries(${test_name} PRIVATE ${test_libs})

    # Link to app_lib if it exists (for tests that need app functionality)
    if(TARGET app_lib)
        target_link_libraries(${test_name} PRIVATE app_lib)
    endif()

    # Resources are linked via app_lib -> app_resources (PUBLIC), so tests
    # inherit linkage automatically and can register them at runtime via
    # SastResources::ensureInitialized().

    # Link to TestUtilities if it exists (for tests that use test utilities)
    if(TARGET TestUtilities)
        target_link_libraries(${test_name} PRIVATE TestUtilities)
    endif()

    # Link additional libraries
    if(TEST_LINK_LIBRARIES)
        target_link_libraries(${test_name} PRIVATE ${TEST_LINK_LIBRARIES})
    endif()

    # Add test to CTest
    add_test(NAME ${test_name} COMMAND ${test_name})

    # Timeout
    set(timeout ${TEST_TIMEOUT})
    if(NOT timeout)
        set(timeout 30)
    endif()

    # Collect test environment variables (single ENVIRONMENT property)
    set(_test_env)
    # Headless/offscreen Qt platform for CI/testing
    list(APPEND _test_env "QT_QPA_PLATFORM=offscreen")
    # Disable Qt Accessibility in headless runs to avoid QAccessible crashes with dock widgets
    list(APPEND _test_env "QT_ACCESSIBILITY=0")

    # On Windows with vcpkg Qt, ensure tests can find Qt DLLs and plugins
    if(WIN32)
        # Compute Qt bin and plugin directories from vcpkg layout if available
        if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
            # vcpkg layout (multi-config): prefer Debug first, then Release
            set(_qt_bin_dir_dbg "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/debug/bin")
            set(_qt_bin_dir_rel "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin")
            set(_qt_plugins_dir_dbg "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/debug/Qt6/plugins")
            set(_qt_plugins_dir_rel "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/Qt6/plugins")
            set(_qt_platforms_dir_dbg "${_qt_plugins_dir_dbg}/platforms")
            set(_qt_platforms_dir_rel "${_qt_plugins_dir_rel}/platforms")

            # Normalize to forward slashes to avoid CTest escaping issues
            file(TO_CMAKE_PATH "${_qt_bin_dir_dbg}" _qt_bin_dir_dbg_norm)
            file(TO_CMAKE_PATH "${_qt_bin_dir_rel}" _qt_bin_dir_rel_norm)
            file(TO_CMAKE_PATH "${_qt_plugins_dir_dbg}" _qt_plugins_dir_dbg_norm)
            file(TO_CMAKE_PATH "${_qt_plugins_dir_rel}" _qt_plugins_dir_rel_norm)
            file(TO_CMAKE_PATH "${_qt_platforms_dir_dbg}" _qt_platforms_dir_dbg_norm)
            file(TO_CMAKE_PATH "${_qt_platforms_dir_rel}" _qt_platforms_dir_rel_norm)

            # Build PATH override: prepend Qt bin, then original PATH; escape semicolons
            set(_combined_path "${_qt_bin_dir_dbg_norm};${_qt_bin_dir_rel_norm};$ENV{PATH}")
            string(REPLACE "\\" "/" _combined_path_slashed "${_combined_path}")
            string(REPLACE ";" "\\;" _combined_path_escaped "${_combined_path_slashed}")

            # Prepend Qt bin to PATH so Qt6*.dll are found; set plugin search paths explicitly
            # Compose plugin paths: Debug first, then Release
            set(_plugins_path "${_qt_plugins_dir_dbg_norm};${_qt_plugins_dir_rel_norm}")
            set(_platforms_path "${_qt_platforms_dir_dbg_norm};${_qt_platforms_dir_rel_norm}")
            list(APPEND _test_env
                "PATH=${_combined_path_escaped}"
                "QT_PLUGIN_PATH=${_plugins_path}"
                "QT_QPA_PLATFORM_PLUGIN_PATH=${_platforms_path}"
                # Prefer system fonts on Windows to avoid QFontDatabase warnings
                "QT_QPA_FONTDIR=C:/Windows/Fonts"
                # Force software rendering to avoid GPU/ANGLE issues in headless CI
                "QT_OPENGL=software"
            )
        endif()
    endif()

    # Join environment entries into a single string for the ENVIRONMENT property
    set(_env_string "")
    if(_test_env)
        list(JOIN _test_env ";" _env_string)
    endif()

    # Apply properties in one call (avoid ambiguous list expansions)
    if(TEST_WORKING_DIRECTORY)
        if(_env_string)
            set_tests_properties(${test_name} PROPERTIES
                WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
                TIMEOUT ${timeout}
                ENVIRONMENT "${_env_string}"
            )
        else()
            set_tests_properties(${test_name} PROPERTIES
                WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
                TIMEOUT ${timeout}
            )
        endif()
    else()
        if(_env_string)
            set_tests_properties(${test_name} PROPERTIES
                TIMEOUT ${timeout}
                ENVIRONMENT "${_env_string}"
            )
        else()
            set_tests_properties(${test_name} PROPERTIES
                TIMEOUT ${timeout}
            )
        endif()
    endif()

    message(VERBOSE "Test ${test_name} registered with CTest")
endfunction()
