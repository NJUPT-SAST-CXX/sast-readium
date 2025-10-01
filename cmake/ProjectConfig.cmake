# ProjectConfig.cmake - Project configuration and compiler settings for SAST Readium
# This module consolidates project setup, compiler configuration, and platform utilities

cmake_minimum_required(VERSION 3.28)

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
setup_compiler_settings
-----------------------

Set up common compiler and language settings for the project.

.. code-block:: cmake

  setup_compiler_settings()

This function configures:
- C++20 standard with required compliance
- Compile commands export for IDE integration
- Platform-specific compiler flags
- Build type specific optimizations

#]=======================================================================]
function(setup_compiler_settings)
    message(STATUS "Setting up compiler settings...")
    
    # C++ Standard Configuration
    set(CMAKE_CXX_STANDARD 20 PARENT_SCOPE)
    set(CMAKE_CXX_STANDARD_REQUIRED ON PARENT_SCOPE)
    set(CMAKE_CXX_EXTENSIONS OFF PARENT_SCOPE)

    # Export compile commands for IDE integration
    # This generates compile_commands.json in the build directory as a build artifact
    # The file contains compilation information for language servers like clangd
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON PARENT_SCOPE)
    
    # Platform-specific compiler settings
    _setup_platform_compiler_flags()
    
    message(STATUS "Compiler settings configured: C++${CMAKE_CXX_STANDARD}")
endfunction()

#[=======================================================================[.rst:
setup_qt_automation
-------------------

Configure Qt's automatic code generation tools.

.. code-block:: cmake

  setup_qt_automation()

Enables:
- CMAKE_AUTOMOC for Meta-Object Compiler
- CMAKE_AUTOUIC for User Interface Compiler  
- CMAKE_AUTORCC for Resource Compiler

#]=======================================================================]
function(setup_qt_automation)
    message(STATUS "Setting up Qt automation...")
    
    set(CMAKE_AUTOMOC ON PARENT_SCOPE)
    set(CMAKE_AUTOUIC ON PARENT_SCOPE)
    set(CMAKE_AUTORCC ON PARENT_SCOPE)
    
    message(STATUS "Qt automation enabled (MOC, UIC, RCC)")
endfunction()

#[=======================================================================[.rst:
detect_platform_environment
---------------------------

Detect the current platform and build environment.

.. code-block:: cmake

  detect_platform_environment()

Sets the following variables in parent scope:
- PLATFORM_WINDOWS, PLATFORM_LINUX, PLATFORM_MACOS
- MSYS2_DETECTED (Windows only)
- COMPILER_MSVC, COMPILER_GCC, COMPILER_CLANG

#]=======================================================================]
function(detect_platform_environment)
    message(STATUS "Detecting platform and build environment...")
    
    # Platform detection
    if(WIN32)
        set(PLATFORM_WINDOWS TRUE PARENT_SCOPE)
        message(STATUS "Platform: Windows")
        
        # MSYS2 detection
        if(DEFINED ENV{MSYSTEM})
            set(MSYS2_DETECTED TRUE PARENT_SCOPE)
            message(STATUS "MSYS2 environment detected: $ENV{MSYSTEM}")
        else()
            set(MSYS2_DETECTED FALSE PARENT_SCOPE)
        endif()
    elseif(APPLE)
        set(PLATFORM_MACOS TRUE PARENT_SCOPE)
        message(STATUS "Platform: macOS")
    elseif(UNIX)
        set(PLATFORM_LINUX TRUE PARENT_SCOPE)
        message(STATUS "Platform: Linux")
    endif()
    
    # Compiler detection
    if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        set(COMPILER_MSVC TRUE PARENT_SCOPE)
        message(STATUS "Compiler: MSVC ${CMAKE_CXX_COMPILER_VERSION}")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set(COMPILER_GCC TRUE PARENT_SCOPE)
        message(STATUS "Compiler: GCC ${CMAKE_CXX_COMPILER_VERSION}")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(COMPILER_CLANG TRUE PARENT_SCOPE)
        message(STATUS "Compiler: Clang ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
    
    message(STATUS "Platform detection completed")
endfunction()

#[=======================================================================[.rst:
configure_platform_specific_settings
------------------------------------

Configure platform-specific build settings.

.. code-block:: cmake

  configure_platform_specific_settings()

Applies platform-specific configurations for:
- Windows: MSVC flags, Unicode support
- Linux: Threading, dynamic linking
- macOS: Framework settings

#]=======================================================================]
function(configure_platform_specific_settings)
    message(STATUS "Configuring platform-specific settings...")
    
    if(PLATFORM_WINDOWS)
        # Windows-specific settings
        if(COMPILER_MSVC)
            add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
            message(STATUS "Applied Windows/MSVC-specific settings")
        endif()
    elseif(PLATFORM_LINUX)
        # Linux-specific settings
        find_package(Threads REQUIRED)
        message(STATUS "Applied Linux-specific settings")
    elseif(PLATFORM_MACOS)
        # macOS-specific settings
        set(CMAKE_MACOSX_RPATH ON PARENT_SCOPE)
        message(STATUS "Applied macOS-specific settings")
    endif()
    
    message(STATUS "Platform-specific configuration completed")
endfunction()

#[=======================================================================[.rst:
validate_build_environment
--------------------------

Validate that the build environment is properly configured.

.. code-block:: cmake

  validate_build_environment()

Checks:
- Compiler version compatibility
- Required tools availability
- Platform-specific requirements

#]=======================================================================]
function(validate_build_environment)
    message(STATUS "Validating build environment...")
    
    # Check C++20 support
    if(CMAKE_CXX_STANDARD LESS 20)
        message(FATAL_ERROR "C++20 support is required")
    endif()
    
    # Check compiler version
    if(COMPILER_GCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "10.0")
        message(FATAL_ERROR "GCC 10.0 or later is required for C++20 support")
    elseif(COMPILER_CLANG AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12.0")
        message(FATAL_ERROR "Clang 12.0 or later is required for C++20 support")
    elseif(COMPILER_MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19.29")
        message(FATAL_ERROR "MSVC 19.29 (Visual Studio 2019 16.10) or later is required")
    endif()
    
    message(STATUS "Build environment validation passed")
endfunction()

#[=======================================================================[.rst:
print_build_summary
-------------------

Print a summary of the build configuration.

.. code-block:: cmake

  print_build_summary()

Displays:
- Platform and compiler information
- Build type and key options
- Dependency management strategy

#]=======================================================================]
#[=======================================================================[.rst:
setup_clangd_integration
------------------------

Automatically configure clangd by generating .clangd configuration file
that points to the current build directory's compile_commands.json.

This function implements optimal build artifact management by:
1. Ensuring compile_commands.json remains in build directory (as it should)
2. Configuring clangd to find the compilation database in the correct location
3. Preventing manual file management and associated override issues
4. Maintaining consistency between build configuration and language server setup

.. code-block:: cmake

  setup_clangd_integration()

Features:
- Generates .clangd file with correct CompilationDatabase path
- Respects ENABLE_CLANGD_CONFIG option
- Cross-platform path handling
- Comprehensive clangd configuration with compiler flags and settings
- Automatic synchronization with active build directory
- Protection against manual configuration drift

#]=======================================================================]
function(setup_clangd_integration)
    # Check if clangd configuration is enabled
    if(NOT ENABLE_CLANGD_CONFIG)
        message(STATUS "clangd configuration disabled (ENABLE_CLANGD_CONFIG=OFF)")
        return()
    endif()

    message(STATUS "Setting up clangd integration...")

    # Calculate relative path from source to binary directory
    # This ensures .clangd points to the correct build directory where
    # compile_commands.json is automatically generated by CMAKE_EXPORT_COMPILE_COMMANDS
    file(RELATIVE_PATH build_dir_relative "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")

    # Normalize path separators for clangd (always use forward slashes)
    # clangd expects Unix-style paths regardless of platform
    string(REPLACE "\\" "/" build_dir_normalized "${build_dir_relative}")

    # Validate that build directory exists and has been configured
    if(NOT EXISTS "${CMAKE_BINARY_DIR}")
        message(WARNING "Build directory does not exist: ${CMAKE_BINARY_DIR}")
        return()
    endif()

    # Set the .clangd file path in project root
    # This file will point clangd to the build directory for compilation database
    set(clangd_file "${CMAKE_SOURCE_DIR}/.clangd")

    # Get current timestamp for metadata
    string(TIMESTAMP current_time "%Y-%m-%d %H:%M:%S UTC" UTC)

    # Generate .clangd configuration content
    set(clangd_content "# clangd configuration for SAST Readium
# Auto-generated by CMake build system
# Build directory: ${build_dir_normalized}
# Generated: ${current_time}

CompileFlags:
  Add:
    - -std=c++20
    - -Wall
    - -Wextra
  Remove:
    - -W*
    - -fcoroutines-ts
    # Remove C++20 modules flags that clangd doesn't recognize
    # These are automatically added by CMake 3.28+ but cause clangd warnings
    - -fmodules-ts
    - -fmodule-mapper=*
    - -fdeps-format=*

CompilationDatabase: ${build_dir_normalized}

Index:
  Background: Build
  StandardLibrary: Yes

InlayHints:
  Enabled: Yes
  ParameterNames: Yes
  DeducedTypes: Yes

Hover:
  ShowAKA: Yes

Diagnostics:
  ClangTidy:
    Add:
      - readability-*
      - modernize-*
      - performance-*
    Remove:
      - readability-magic-numbers
      - modernize-use-trailing-return-type

# IMPORTANT: This file is automatically maintained by the CMake build system
#
# The 'CompilationDatabase' above points to the build directory where
# compile_commands.json is automatically generated as a build artifact.
# This ensures clangd always has access to current compilation information.
#
# Manual changes to this file will be overwritten during CMake configuration
# to maintain consistency with the active build configuration.
#
# To modify clangd settings, edit the setup_clangd_integration() function
# in cmake/ProjectConfig.cmake instead.
")

    # Write the .clangd file
    file(WRITE "${clangd_file}" "${clangd_content}")

    message(STATUS "Generated .clangd configuration pointing to: ${build_dir_normalized}")
    message(STATUS "Restart your language server to apply clangd changes")
endfunction()

function(print_build_summary)
    message(STATUS "")
    message(STATUS "=== Build Configuration Summary ===")
    message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
    message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
    message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")

    if(USE_VCPKG)
        message(STATUS "Package Manager: vcpkg")
    else()
        message(STATUS "Package Manager: system packages")
    endif()

    if(BUILD_TESTING)
        message(STATUS "Testing: Enabled")
    else()
        message(STATUS "Testing: Disabled")
    endif()

    if(ENABLE_CLANGD_CONFIG)
        message(STATUS "clangd Integration: Enabled")
    else()
        message(STATUS "clangd Integration: Disabled")
    endif()

    message(STATUS "===================================")
    message(STATUS "")
endfunction()

# Internal helper functions
function(_setup_platform_compiler_flags)
    # Get current flags to avoid overwriting
    set(current_cxx_flags ${CMAKE_CXX_FLAGS})
    
    # Windows/MSVC specific settings
    if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        # Enhanced C++ standard compliance
        string(APPEND current_cxx_flags " /Zc:__cplusplus /EHsc /utf-8")
        message(STATUS "Applied MSVC C++ compliance flags")
    endif()
    
    # GCC specific settings
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        # Enable useful warnings for GCC
        string(APPEND current_cxx_flags " -Wall -Wextra")
        message(STATUS "Applied GCC warning flags")
    endif()

    # Clang specific settings
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # Enable useful warnings for Clang
        string(APPEND current_cxx_flags " -Wall -Wextra")
        message(STATUS "Applied Clang warning flags")
    endif()
    
    # Update parent scope
    set(CMAKE_CXX_FLAGS ${current_cxx_flags} PARENT_SCOPE)
endfunction()
