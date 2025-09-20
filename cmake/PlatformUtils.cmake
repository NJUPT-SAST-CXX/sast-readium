# PlatformUtils.cmake - Platform-specific utilities for SAST Readium
# This module provides functions for platform detection and configuration

cmake_minimum_required(VERSION 3.28)

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
    message(STATUS "Detecting platform environment...")
    
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
    elseif(UNIX AND NOT APPLE)
        set(PLATFORM_LINUX TRUE PARENT_SCOPE)
        message(STATUS "Platform: Linux")
    elseif(APPLE)
        set(PLATFORM_MACOS TRUE PARENT_SCOPE)
        message(STATUS "Platform: macOS")
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
endfunction()

#[=======================================================================[.rst:
configure_platform_specific_settings
------------------------------------

Configure platform-specific build settings.

.. code-block:: cmake

  configure_platform_specific_settings()

Applies platform-specific configurations for:
- Runtime library paths (RPATH)
- Deployment settings
- Platform-specific compiler flags

#]=======================================================================]
function(configure_platform_specific_settings)
    message(STATUS "Configuring platform-specific settings...")
    
    if(PLATFORM_WINDOWS)
        _configure_windows_settings()
    elseif(PLATFORM_LINUX)
        _configure_linux_settings()
    elseif(PLATFORM_MACOS)
        _configure_macos_settings()
    endif()
    
    # MSYS2 specific configurations
    if(MSYS2_DETECTED)
        _configure_msys2_settings()
    endif()
    
    message(STATUS "Platform-specific settings configured")
endfunction()

#[=======================================================================[.rst:
setup_deployment_tools
----------------------

Set up platform-specific deployment tools.

.. code-block:: cmake

  setup_deployment_tools(target_name)

Arguments:
  target_name    The target to set up deployment for

Configures:
- Windows: windeployqt for Qt deployment
- macOS: macdeployqt for Qt deployment  
- Linux: Custom deployment scripts

#]=======================================================================]
function(setup_deployment_tools target_name)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Target ${target_name} does not exist")
    endif()
    
    message(STATUS "Setting up deployment tools for ${target_name}...")
    
    if(PLATFORM_WINDOWS)
        _setup_windows_deployment(${target_name})
    elseif(PLATFORM_MACOS)
        _setup_macos_deployment(${target_name})
    elseif(PLATFORM_LINUX)
        _setup_linux_deployment(${target_name})
    endif()
endfunction()

#[=======================================================================[.rst:
_configure_windows_settings
---------------------------

Internal function for Windows-specific settings.
#]=======================================================================]
function(_configure_windows_settings)
    message(STATUS "Applying Windows-specific settings...")
    
    # Windows-specific compile definitions
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
    
    # MSVC-specific settings
    if(COMPILER_MSVC)
        # Enhanced C++ compliance
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus /EHsc /utf-8" PARENT_SCOPE)
    endif()
endfunction()

#[=======================================================================[.rst:
_configure_linux_settings
-------------------------

Internal function for Linux-specific settings.
#]=======================================================================]
function(_configure_linux_settings)
    message(STATUS "Applying Linux-specific settings...")
    
    # Set RPATH for shared libraries
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE PARENT_SCOPE)
    
    # Enable position independent code
    set(CMAKE_POSITION_INDEPENDENT_CODE ON PARENT_SCOPE)
endfunction()

#[=======================================================================[.rst:
_configure_macos_settings
------------------------

Internal function for macOS-specific settings.
#]=======================================================================]
function(_configure_macos_settings)
    message(STATUS "Applying macOS-specific settings...")
    
    # Set RPATH for shared libraries
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE PARENT_SCOPE)
    
    # macOS deployment target
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" PARENT_SCOPE)
        message(STATUS "Set macOS deployment target to 10.15")
    endif()
endfunction()

#[=======================================================================[.rst:
_configure_msys2_settings
------------------------

Internal function for MSYS2-specific settings.
#]=======================================================================]
function(_configure_msys2_settings)
    message(STATUS "Applying MSYS2-specific settings...")
    
    # Add MSYSTEM_PREFIX to CMAKE_PREFIX_PATH
    if(DEFINED ENV{MSYSTEM_PREFIX})
        list(APPEND CMAKE_PREFIX_PATH "$ENV{MSYSTEM_PREFIX}")
        set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} PARENT_SCOPE)
        message(STATUS "Added MSYSTEM_PREFIX to CMAKE_PREFIX_PATH: $ENV{MSYSTEM_PREFIX}")
    endif()
    
    # Set RPATH for MSYS2 libraries
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE PARENT_SCOPE)
    endif()
endfunction()

#[=======================================================================[.rst:
_setup_windows_deployment
-------------------------

Internal function for Windows deployment setup.
#]=======================================================================]
function(_setup_windows_deployment target_name)
    # Find windeployqt
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${Qt6_DIR}/../../../bin)
    
    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS "Found windeployqt: ${WINDEPLOYQT_EXECUTABLE}")
        
        # Add post-build step to deploy Qt
        add_custom_command(
            TARGET ${target_name}
            POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE} $<TARGET_FILE:${target_name}>
            COMMENT "Deploying Qt libraries for ${target_name}"
            VERBATIM
        )
    else()
        message(WARNING "windeployqt not found - Qt deployment will not be automatic")
    endif()
endfunction()

#[=======================================================================[.rst:
_setup_macos_deployment
-----------------------

Internal function for macOS deployment setup.
#]=======================================================================]
function(_setup_macos_deployment target_name)
    # Find macdeployqt
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS ${Qt6_DIR}/../../../bin)
    
    if(MACDEPLOYQT_EXECUTABLE)
        message(STATUS "Found macdeployqt: ${MACDEPLOYQT_EXECUTABLE}")
        
        # Add post-build step to deploy Qt
        add_custom_command(
            TARGET ${target_name}
            POST_BUILD
            COMMAND ${MACDEPLOYQT_EXECUTABLE} $<TARGET_FILE:${target_name}>
            COMMENT "Deploying Qt libraries for ${target_name}"
            VERBATIM
        )
    else()
        message(WARNING "macdeployqt not found - Qt deployment will not be automatic")
    endif()
endfunction()

#[=======================================================================[.rst:
_setup_linux_deployment
-----------------------

Internal function for Linux deployment setup.
#]=======================================================================]
function(_setup_linux_deployment target_name)
    message(STATUS "Linux deployment setup for ${target_name}")
    # Linux deployment is typically handled by package managers
    # or custom scripts - no automatic deployment tool like Windows/macOS
endfunction()

#[=======================================================================[.rst:
print_platform_info
-------------------

Print information about the detected platform and environment.

.. code-block:: cmake

  print_platform_info()

#]=======================================================================]
function(print_platform_info)
    message(STATUS "=== Platform Information ===")
    message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
    message(STATUS "Processor: ${CMAKE_SYSTEM_PROCESSOR}")
    message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    
    if(MSYS2_DETECTED)
        message(STATUS "MSYS2: $ENV{MSYSTEM}")
        message(STATUS "MSYSTEM_PREFIX: $ENV{MSYSTEM_PREFIX}")
    endif()
    
    message(STATUS "============================")
endfunction()
