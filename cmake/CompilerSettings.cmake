# CompilerSettings.cmake - Common compiler and language settings for SAST Readium
# This module provides functions for setting up compiler configurations

cmake_minimum_required(VERSION 3.28)

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
configure_build_optimizations
-----------------------------

Configure build-type specific optimizations.

.. code-block:: cmake

  configure_build_optimizations()

Applies optimizations based on:
- Build type (Debug, Release, etc.)
- Platform (Windows, Linux, macOS)
- Compiler (GCC, Clang, MSVC)

#]=======================================================================]
function(configure_build_optimizations)
    message(STATUS "Configuring build optimizations...")
    
    # MSYS2 specific optimizations
    if(MSYS2_DETECTED AND CMAKE_BUILD_TYPE STREQUAL "Release")
        # Strip debug symbols in release builds for smaller binaries
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s" PARENT_SCOPE)
        message(STATUS "MSYS2 Release: Enabled symbol stripping")
    endif()
    
    # Windows-specific optimizations
    if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        # MSVC-specific flags for better C++ compliance
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus /EHsc /utf-8" PARENT_SCOPE)
        message(STATUS "MSVC: Enhanced C++ compliance flags enabled")
    endif()
    
    message(STATUS "Build optimizations configured for ${CMAKE_BUILD_TYPE}")
endfunction()

#[=======================================================================[.rst:
_setup_platform_compiler_flags
------------------------------

Internal function to set up platform-specific compiler flags.
#]=======================================================================]
function(_setup_platform_compiler_flags)
    # Get current flags to avoid overwriting
    set(current_cxx_flags ${CMAKE_CXX_FLAGS})
    set(current_exe_flags ${CMAKE_EXE_LINKER_FLAGS})
    
    # Windows/MSVC specific settings
    if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        # Enhanced C++ standard compliance
        string(APPEND current_cxx_flags " /Zc:__cplusplus /EHsc /utf-8")
        message(STATUS "Applied MSVC C++ compliance flags")
    endif()
    
    # GCC/Clang specific settings
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Enable useful warnings
        string(APPEND current_cxx_flags " -Wall -Wextra")
        message(STATUS "Applied GCC/Clang warning flags")
    endif()
    
    # Update parent scope
    set(CMAKE_CXX_FLAGS ${current_cxx_flags} PARENT_SCOPE)
    set(CMAKE_EXE_LINKER_FLAGS ${current_exe_flags} PARENT_SCOPE)
endfunction()

#[=======================================================================[.rst:
setup_compile_definitions
-------------------------

Set up common compile definitions for the project.

.. code-block:: cmake

  setup_compile_definitions()

Configures:
- QGraphics PDF support (if enabled)
- Platform-specific definitions
- Debug/Release specific definitions

#]=======================================================================]
function(setup_compile_definitions)
    message(STATUS "Setting up compile definitions...")
    
    # QGraphics PDF support
    if(ENABLE_QGRAPHICS_PDF_SUPPORT)
        add_compile_definitions(ENABLE_QGRAPHICS_PDF_SUPPORT)
        message(STATUS "QGraphics PDF support enabled")
    endif()
    
    # Platform-specific definitions
    if(WIN32)
        add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
        message(STATUS "Windows-specific definitions added")
    endif()
    
    # Debug-specific definitions
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_definitions(DEBUG_BUILD)
        message(STATUS "Debug build definitions added")
    endif()
    
    message(STATUS "Compile definitions configured")
endfunction()

#[=======================================================================[.rst:
get_common_compile_options
--------------------------

Get common compile options that should be applied to targets.

.. code-block:: cmake

  get_common_compile_options(output_var)

Returns a list of compile options in output_var.
#]=======================================================================]
function(get_common_compile_options output_var)
    set(compile_options)
    
    # Warning levels
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        list(APPEND compile_options -Wall -Wextra -Wpedantic)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        list(APPEND compile_options /W4)
    endif()
    
    # Debug information
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            list(APPEND compile_options -g -O0)
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
            list(APPEND compile_options /Zi /Od)
        endif()
    endif()
    
    set(${output_var} ${compile_options} PARENT_SCOPE)
endfunction()

#[=======================================================================[.rst:
print_compiler_info
-------------------

Print information about the current compiler configuration.

.. code-block:: cmake

  print_compiler_info()

Displays:
- Compiler ID and version
- C++ standard setting
- Build type
- Key compiler flags

#]=======================================================================]
function(print_compiler_info)
    message(STATUS "=== Compiler Configuration ===")
    message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
    message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
    message(STATUS "CXX Flags: ${CMAKE_CXX_FLAGS}")
    if(CMAKE_BUILD_TYPE)
        string(TOUPPER ${CMAKE_BUILD_TYPE} build_type_upper)
        message(STATUS "${CMAKE_BUILD_TYPE} Flags: ${CMAKE_CXX_FLAGS_${build_type_upper}}")
    endif()
    message(STATUS "==============================")
endfunction()
