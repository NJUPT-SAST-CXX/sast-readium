# ProjectUtils.cmake - Project-wide utility functions for SAST Readium
# This module provides functions for project configuration and management

cmake_minimum_required(VERSION 3.28)

#[=======================================================================[.rst:
setup_clangd_integration
------------------------

Set up automatic clangd configuration updates.

.. code-block:: cmake

  setup_clangd_integration()

This function:
- Sets up automatic .clangd configuration updates
- Configures compile_commands.json export
- Creates custom target for clangd config updates
- Handles platform-specific script selection

Requires:
- CMAKE_EXPORT_COMPILE_COMMANDS to be ON
- ENABLE_CLANGD_CONFIG option to be ON

#]=======================================================================]
function(setup_clangd_integration)
    if(NOT CMAKE_EXPORT_COMPILE_COMMANDS)
        message(STATUS "clangd integration disabled: CMAKE_EXPORT_COMPILE_COMMANDS is OFF")
        return()
    endif()
    
    if(NOT ENABLE_CLANGD_CONFIG)
        message(STATUS "clangd integration disabled: ENABLE_CLANGD_CONFIG is OFF")
        return()
    endif()
    
    message(STATUS "Setting up clangd integration...")
    
    # Calculate relative path from source directory to build directory
    file(RELATIVE_PATH BUILD_DIR_RELATIVE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
    
    # Convert Windows backslashes to forward slashes for consistency
    string(REPLACE "\\" "/" BUILD_DIR_RELATIVE ${BUILD_DIR_RELATIVE})
    
    # Find appropriate script for platform
    _find_clangd_script(script_command)
    
    if(script_command)
        # Add custom target to update .clangd configuration
        add_custom_target(update_clangd_config ALL
            COMMAND ${script_command}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Updating .clangd configuration for build directory: ${BUILD_DIR_RELATIVE}"
            DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json
            VERBATIM
        )
        
        message(STATUS "clangd integration configured")
    else()
        message(WARNING "No suitable script interpreter found for updating .clangd configuration")
    endif()
endfunction()

#[=======================================================================[.rst:
print_build_summary
------------------

Print a comprehensive build configuration summary.

.. code-block:: cmake

  print_build_summary()

Displays information about:
- Build type and compiler
- Dependency management strategy
- Platform and environment
- Key build options
- Target information

#]=======================================================================]
function(print_build_summary)
    message(STATUS "")
    message(STATUS "=== Build Configuration Summary ===")
    
    # Basic build information
    message(STATUS "Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
    message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
    message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    
    # Dependency management
    if(USE_VCPKG_INTERNAL)
        message(STATUS "Dependency management: vcpkg")
    else()
        message(STATUS "Dependency management: system packages")
    endif()
    
    # Platform information
    message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
    if(MSYS2_DETECTED)
        message(STATUS "MSYS2 environment: $ENV{MSYSTEM}")
    endif()
    
    # Build options
    message(STATUS "Export compile commands: ${CMAKE_EXPORT_COMPILE_COMMANDS}")
    message(STATUS "clangd auto-config: ${ENABLE_CLANGD_CONFIG}")
    message(STATUS "QGraphics PDF support: ${ENABLE_QGRAPHICS_PDF_SUPPORT}")
    message(STATUS "Build testing: ${BUILD_TESTING}")
    
    # Additional platform-specific info
    if(MSYS2_DETECTED)
        message(STATUS "")
        message(STATUS "=== MSYS2 Build Configuration ===")
        message(STATUS "MSYSTEM: $ENV{MSYSTEM}")
        message(STATUS "MSYSTEM_PREFIX: $ENV{MSYSTEM_PREFIX}")
        message(STATUS "Using vcpkg: ${USE_VCPKG_INTERNAL}")
        message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
        message(STATUS "================================")
    endif()
    
    message(STATUS "=====================================")
    message(STATUS "")
endfunction()

#[=======================================================================[.rst:
validate_build_environment
--------------------------

Validate the build environment and dependencies.

.. code-block:: cmake

  validate_build_environment()

Performs validation checks for:
- Required CMake version
- Compiler compatibility
- Required dependencies
- Platform-specific requirements

Fails the build if critical requirements are not met.

#]=======================================================================]
function(validate_build_environment)
    message(STATUS "Validating build environment...")
    
    # Check CMake version
    if(CMAKE_VERSION VERSION_LESS "3.28")
        message(FATAL_ERROR "CMake 3.28 or higher is required. Current version: ${CMAKE_VERSION}")
    endif()
    
    # Check C++ compiler
    if(NOT CMAKE_CXX_COMPILER)
        message(FATAL_ERROR "No C++ compiler found")
    endif()
    
    # Check C++ standard support
    if(CMAKE_CXX_STANDARD LESS 20)
        message(FATAL_ERROR "C++20 or higher is required")
    endif()
    
    # Platform-specific validation
    _validate_platform_requirements()
    
    # Dependency validation
    _validate_dependencies()
    
    message(STATUS "Build environment validation passed")
endfunction()

#[=======================================================================[.rst:
setup_project_options
---------------------

Set up project-wide options and cache variables.

.. code-block:: cmake

  setup_project_options()

Defines standard project options:
- USE_VCPKG: Use vcpkg for dependency management
- FORCE_VCPKG: Force vcpkg usage even in MSYS2
- ENABLE_CLANGD_CONFIG: Enable automatic clangd configuration
- ENABLE_QGRAPHICS_PDF_SUPPORT: Enable QGraphics PDF support
- BUILD_TESTING: Build tests

#]=======================================================================]
function(setup_project_options)
    message(STATUS "Setting up project options...")
    
    # MSYS2 and vcpkg configuration
    option(USE_VCPKG "Use vcpkg for dependency management" OFF)
    option(FORCE_VCPKG "Force vcpkg usage even in MSYS2" OFF)
    
    # clangd configuration
    option(ENABLE_CLANGD_CONFIG "Enable automatic clangd configuration updates" ON)
    
    # QGraphics PDF support
    option(ENABLE_QGRAPHICS_PDF_SUPPORT "Enable QGraphics-based PDF rendering support" OFF)
    
    # Testing support
    option(BUILD_TESTING "Build tests" ON)
    
    message(STATUS "Project options configured")
endfunction()

#[=======================================================================[.rst:
_find_clangd_script
------------------

Internal function to find the appropriate clangd update script.
#]=======================================================================]
function(_find_clangd_script output_var)
    # Calculate relative path for script
    file(RELATIVE_PATH BUILD_DIR_RELATIVE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
    string(REPLACE "\\" "/" BUILD_DIR_RELATIVE ${BUILD_DIR_RELATIVE})
    
    set(script_command)
    
    if(WIN32)
        # Windows: Try PowerShell first, then batch script
        find_program(POWERSHELL_PATH powershell)
        
        if(POWERSHELL_PATH)
            set(script_command ${POWERSHELL_PATH} -ExecutionPolicy Bypass -File
                "${CMAKE_SOURCE_DIR}/scripts/update-clangd-config.ps1"
                -BuildDir "${BUILD_DIR_RELATIVE}" -Verbose)
        else()
            # Fallback to batch script
            set(script_command "${CMAKE_SOURCE_DIR}/scripts/update-clangd-config.bat" "${BUILD_DIR_RELATIVE}")
        endif()
    else()
        # Unix-like systems: Use shell script
        find_program(BASH_PATH bash)
        if(BASH_PATH)
            set(script_command ${BASH_PATH} "${CMAKE_SOURCE_DIR}/scripts/update-clangd-config.sh"
                --build-dir "${BUILD_DIR_RELATIVE}" --verbose)
        else()
            # Fallback to sh
            set(script_command sh "${CMAKE_SOURCE_DIR}/scripts/update-clangd-config.sh"
                --build-dir "${BUILD_DIR_RELATIVE}" --verbose)
        endif()
    endif()
    
    set(${output_var} "${script_command}" PARENT_SCOPE)
endfunction()

#[=======================================================================[.rst:
_validate_platform_requirements
-------------------------------

Internal function to validate platform-specific requirements.
#]=======================================================================]
function(_validate_platform_requirements)
    if(WIN32)
        # Windows-specific validation
        if(MSYS2_DETECTED AND NOT DEFINED ENV{MSYSTEM_PREFIX})
            message(WARNING "MSYSTEM_PREFIX not defined in MSYS2 environment")
        endif()
    elseif(UNIX AND NOT APPLE)
        # Linux-specific validation
        # Add any Linux-specific checks here
    elseif(APPLE)
        # macOS-specific validation
        if(CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS "10.15")
            message(WARNING "macOS deployment target is older than recommended (10.15)")
        endif()
    endif()
endfunction()

#[=======================================================================[.rst:
_validate_dependencies
----------------------

Internal function to validate that required dependencies are available.
#]=======================================================================]
function(_validate_dependencies)
    # Check if dependencies have been found
    if(NOT SAST_READIUM_DEPENDENCIES_FOUND)
        message(WARNING "Dependencies have not been found yet - call find_project_dependencies() first")
        return()
    endif()
    
    # Validate Qt6
    if(NOT TARGET Qt6::Core)
        message(FATAL_ERROR "Qt6::Core target not found - Qt6 dependency resolution failed")
    endif()
    
    # Validate spdlog
    if(NOT TARGET spdlog::spdlog)
        message(FATAL_ERROR "spdlog::spdlog target not found - spdlog dependency resolution failed")
    endif()
    
    # Validate poppler-qt6
    if(NOT TARGET PkgConfig::POPPLER_QT6)
        message(FATAL_ERROR "PkgConfig::POPPLER_QT6 target not found - poppler-qt6 dependency resolution failed")
    endif()
    
    message(STATUS "All required dependencies validated")
endfunction()
