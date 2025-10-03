# Dependencies.cmake - Centralized dependency management for SAST Readium
# This module provides functions for finding and configuring project dependencies

cmake_minimum_required(VERSION 3.28)

# Cache variables to avoid redundant package finding
set(SAST_READIUM_DEPENDENCIES_FOUND FALSE CACHE INTERNAL "Dependencies already found")

#[=======================================================================[.rst:
find_project_dependencies
--------------------------

Find all required project dependencies with intelligent caching and environment detection.

.. code-block:: cmake

  find_project_dependencies([FORCE])

Options:
  FORCE    Force re-finding dependencies even if already cached

This function handles:
- vcpkg vs system package detection
- MSYS2 environment configuration  
- Qt6 component finding
- spdlog and poppler-qt6 setup
- Caching to avoid redundant find_package calls

#]=======================================================================]
macro(find_project_dependencies)
    cmake_parse_arguments(DEPS "FORCE" "" "" ${ARGN})

    # Skip if already found (unless forced)
    if(SAST_READIUM_DEPENDENCIES_FOUND AND NOT DEPS_FORCE)
        message(STATUS "Dependencies already found, skipping...")
        return()
    endif()

    message(STATUS "Finding project dependencies...")

    # Detect build environment first
    _detect_build_environment()

    # Configure dependency finding strategy
    _configure_dependency_strategy()

    # Find core dependencies
    _find_qt_dependencies()
    _find_logging_dependencies()
    _find_pdf_dependencies()

    # Mark dependencies as found
    set(SAST_READIUM_DEPENDENCIES_FOUND TRUE CACHE INTERNAL "Dependencies found")

    message(STATUS "All project dependencies found successfully")
endmacro()

#[=======================================================================[.rst:
_detect_build_environment
-------------------------

Internal function to detect MSYS2 and other build environments.
Sets global variables for environment detection.
#]=======================================================================]
function(_detect_build_environment)
    # Detect MSYS2 environment
    if(WIN32 AND DEFINED ENV{MSYSTEM})
        set(MSYS2_DETECTED TRUE PARENT_SCOPE)
        message(STATUS "MSYS2 environment detected: $ENV{MSYSTEM}")

        # Set Qt6 installation path for MSYS2
        if(DEFINED ENV{MSYSTEM_PREFIX})
            list(APPEND CMAKE_PREFIX_PATH "$ENV{MSYSTEM_PREFIX}")
            set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} PARENT_SCOPE)
            message(STATUS "Added MSYSTEM_PREFIX to CMAKE_PREFIX_PATH: $ENV{MSYSTEM_PREFIX}")
        endif()
    else()
        set(MSYS2_DETECTED FALSE PARENT_SCOPE)
    endif()

    # Detect vcpkg environment
    if(DEFINED ENV{VCPKG_ROOT} OR DEFINED CMAKE_TOOLCHAIN_FILE)
        if(CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
            set(VCPKG_DETECTED TRUE PARENT_SCOPE)
            message(STATUS "vcpkg toolchain detected: ${CMAKE_TOOLCHAIN_FILE}")
        elseif(DEFINED ENV{VCPKG_ROOT})
            set(VCPKG_DETECTED TRUE PARENT_SCOPE)
            message(STATUS "vcpkg environment detected: $ENV{VCPKG_ROOT}")
        endif()
    else()
        set(VCPKG_DETECTED FALSE PARENT_SCOPE)
    endif()

    # Detect platform for vcpkg builds
    if(UNIX AND NOT APPLE)
        set(PLATFORM_LINUX TRUE PARENT_SCOPE)
        message(STATUS "Linux platform detected")
    elseif(APPLE)
        set(PLATFORM_MACOS TRUE PARENT_SCOPE)
        message(STATUS "macOS platform detected")
    elseif(WIN32)
        set(PLATFORM_WINDOWS TRUE PARENT_SCOPE)
        message(STATUS "Windows platform detected")
    endif()
endfunction()

#[=======================================================================[.rst:
_configure_dependency_strategy
------------------------------

Internal function to determine whether to use vcpkg or system packages.
Sets USE_VCPKG_INTERNAL variable based on options and environment.
#]=======================================================================]
function(_configure_dependency_strategy)
    # Determine whether to use vcpkg
    if(FORCE_VCPKG)
        set(USE_VCPKG_INTERNAL TRUE PARENT_SCOPE)
        message(STATUS "vcpkg usage forced via FORCE_VCPKG option")
    elseif(MSYS2_DETECTED AND NOT USE_VCPKG)
        set(USE_VCPKG_INTERNAL FALSE PARENT_SCOPE)
        message(STATUS "MSYS2 detected - using system packages instead of vcpkg")
    elseif(VCPKG_DETECTED AND USE_VCPKG)
        set(USE_VCPKG_INTERNAL TRUE PARENT_SCOPE)
        if(PLATFORM_LINUX)
            message(STATUS "Linux vcpkg build detected - using vcpkg for dependency management")
        elseif(PLATFORM_MACOS)
            message(STATUS "macOS vcpkg build detected - using vcpkg for dependency management")
        elseif(PLATFORM_WINDOWS)
            message(STATUS "Windows vcpkg build detected - using vcpkg for dependency management")
        else()
            message(STATUS "vcpkg build detected - using vcpkg for dependency management")
        endif()
    else()
        set(USE_VCPKG_INTERNAL ${USE_VCPKG} PARENT_SCOPE)
        if(USE_VCPKG)
            message(STATUS "Using vcpkg for dependency management")
        else()
            message(STATUS "Using system packages for dependency management")
        endif()
    endif()
endfunction()

#[=======================================================================[.rst:
_find_qt_dependencies
---------------------

Internal macro to find Qt6 dependencies with appropriate mode.
Note: Using macro instead of function to ensure Qt variables are in global scope.
#]=======================================================================]
macro(_find_qt_dependencies)
    set(QT_COMPONENTS Core Gui Widgets OpenGLWidgets Svg LinguistTools Concurrent TextToSpeech Core5Compat PrintSupport)

    # Add Test component if building tests
    if(BUILD_TESTING)
        list(APPEND QT_COMPONENTS Test Network)
    endif()

    if(USE_VCPKG_INTERNAL)
        # vcpkg mode - use CONFIG mode for all packages
        find_package(Qt6 COMPONENTS ${QT_COMPONENTS} CONFIG REQUIRED)
    else()
        # System packages mode
        find_package(Qt6 REQUIRED COMPONENTS ${QT_COMPONENTS})
    endif()

    message(STATUS "Qt6 found: ${Qt6_VERSION}")
endmacro()

#[=======================================================================[.rst:
_find_logging_dependencies
--------------------------

Internal function to find spdlog logging library.
#]=======================================================================]
function(_find_logging_dependencies)
    if(USE_VCPKG_INTERNAL)
        find_package(spdlog CONFIG REQUIRED)
    else()
        find_package(spdlog REQUIRED)
    endif()
    
    message(STATUS "spdlog found")
endfunction()

#[=======================================================================[.rst:
_find_pdf_dependencies
----------------------

Internal function to find PDF-related dependencies (poppler-qt6).
#]=======================================================================]
function(_find_pdf_dependencies)
    if(USE_VCPKG_INTERNAL)
        # vcpkg mode - use CONFIG mode for poppler
        find_package(poppler CONFIG REQUIRED)
        message(STATUS "poppler found via vcpkg")

        # Create an alias for consistency with pkg-config version
        if(TARGET poppler::poppler-qt6 AND NOT TARGET PkgConfig::POPPLER_QT6)
            add_library(PkgConfig::POPPLER_QT6 ALIAS poppler::poppler-qt6)
        endif()
    else()
        # System packages mode - use pkg-config
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(POPPLER_QT6 REQUIRED IMPORTED_TARGET poppler-qt6)
        message(STATUS "poppler-qt6 found via pkg-config")
    endif()
endfunction()

#[=======================================================================[.rst:
get_common_libraries
-------------------

Get the list of common libraries that most targets should link against.

.. code-block:: cmake

  get_common_libraries(output_var)

Returns a list of common library targets in output_var.
#]=======================================================================]
function(get_common_libraries output_var)
    set(common_libs
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGLWidgets
        Qt6::Svg
        Qt6::Concurrent
        Qt6::Core5Compat
        Qt6::PrintSupport
        PkgConfig::POPPLER_QT6
        spdlog::spdlog
    )

    set(${output_var} ${common_libs} PARENT_SCOPE)
endfunction()

#[=======================================================================[.rst:
get_test_libraries
-----------------

Get the list of libraries needed for testing.

.. code-block:: cmake

  get_test_libraries(output_var)

Returns a list of test library targets in output_var.
#]=======================================================================]
function(get_test_libraries output_var)
    get_common_libraries(common_libs)
    
    set(test_libs
        ${common_libs}
        Qt6::Test
        Qt6::Network
    )
    
    set(${output_var} ${test_libs} PARENT_SCOPE)
endfunction()
