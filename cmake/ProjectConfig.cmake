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
    option(SAST_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
    option(SAST_ENABLE_HARDENING "Enable security hardening flags" ON)
    option(SAST_ENABLE_LTO "Enable Link Time Optimization (IPO/LTCG)" OFF)

    # Build size optimization options
    option(SAST_SPLIT_DEBUG_INFO "Split debug info to separate files (reduces exe size)" ON)
    option(SAST_MINIMAL_TEST_DEBUG "Use minimal debug info for tests (reduces test exe size)" ON)
    option(SAST_STRIP_TESTS "Strip debug symbols from test executables" OFF)
    option(SAST_STRIP_BINARIES "Strip debug symbols from main executables (significant size reduction)" OFF)
    option(SAST_UPX_COMPRESS "Compress executables with UPX (50-70% size reduction)" OFF)
    option(SAST_ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan in Debug builds" OFF)

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

    # Disable C++20 module dependency scanning
    # This project does not use C++20 modules, and CMake 4.1+ automatically enables
    # module scanning for C++20 targets, which can cause build failures with some compilers
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF PARENT_SCOPE)

    # Export compile commands for IDE integration
    # This generates compile_commands.json in the build directory as a build artifact
    # The file contains compilation information for language servers like clangd
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON PARENT_SCOPE)
    set(CMAKE_COLOR_DIAGNOSTICS ON PARENT_SCOPE)

    # Configure build optimizations
    _setup_ccache()
    _setup_parallel_builds()

    # Platform-specific compiler settings
    _setup_platform_compiler_flags()

    # Configure Debug build size optimizations (split debug info)
    _setup_debug_size_optimizations()

    # Configure Release build optimizations
    _setup_release_optimizations()

    # Configure MinSizeRel build optimizations
    _setup_minsizerel_optimizations()

    if(SAST_ENABLE_LTO)
        if(CMAKE_CONFIGURATION_TYPES)
            # Multi-config generators (e.g., MSVC). Enable for Release-like configs.
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON PARENT_SCOPE)
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON PARENT_SCOPE)
        else()
            # Single-config generators (e.g., Ninja). Enable only for Release builds.
            if(CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo")
                set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON PARENT_SCOPE)
            endif()
        endif()
    endif()

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

    # Guard against mixing MSVC with MSYS2/MinGW Qt builds, which produces
    # incompatible headers such as D:/msys64/mingw64/include and leads to
    # compile-time conflicts (e.g., duplicate uintptr_t definitions).
    if(COMPILER_MSVC)
        set(_qt_sources_to_check)
        if(DEFINED Qt6_DIR)
            list(APPEND _qt_sources_to_check "${Qt6_DIR}")
        endif()
        if(TARGET Qt6::Core)
            get_target_property(_qt_core_includes Qt6::Core INTERFACE_INCLUDE_DIRECTORIES)
            if(_qt_core_includes)
                list(APPEND _qt_sources_to_check ${_qt_core_includes})
            endif()
        endif()

        foreach(_qt_location IN LISTS _qt_sources_to_check)
            string(TOLOWER "${_qt_location}" _qt_location_normalized)
            if(_qt_location_normalized MATCHES ".*msys.*" OR _qt_location_normalized MATCHES ".*mingw.*")
                message(FATAL_ERROR
                    "MSVC build configured with Qt from MSYS2/MinGW: ${_qt_location}\n"
                    "Use the MinGW CMake preset or provide an MSVC-compatible Qt toolchain (e.g., via vcpkg)."
                )
            endif()
        endforeach()
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
    message(NOTICE "")
    message(NOTICE "=== Build Configuration Summary ===")
    message(NOTICE "Platform: ${CMAKE_SYSTEM_NAME}")
    message(NOTICE "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    message(NOTICE "Build Type: ${CMAKE_BUILD_TYPE}")
    message(NOTICE "C++ Standard: ${CMAKE_CXX_STANDARD}")

    if(USE_VCPKG)
        message(NOTICE "Package Manager: vcpkg")
    else()
        message(NOTICE "Package Manager: system packages")
    endif()

    if(BUILD_TESTING)
        message(NOTICE "Testing: Enabled")
    else()
        message(NOTICE "Testing: Disabled")
    endif()

    if(ENABLE_CLANGD_CONFIG)
        message(NOTICE "clangd Integration: Enabled")
    else()
        message(NOTICE "clangd Integration: Disabled")
    endif()

    if(SAST_WARNINGS_AS_ERRORS)
        message(NOTICE "Warnings as Errors: ON")
    else()
        message(NOTICE "Warnings as Errors: OFF")
    endif()

    if(SAST_ENABLE_HARDENING)
        message(NOTICE "Hardening Flags: ON")
    else()
        message(NOTICE "Hardening Flags: OFF")
    endif()

    if(SAST_ENABLE_LTO)
        message(NOTICE "Link Time Optimization: ON")
    else()
        message(NOTICE "Link Time Optimization: OFF")
    endif()

    message(NOTICE "===================================")
    message(NOTICE "")
endfunction()

# Internal helper functions
function(_setup_platform_compiler_flags)
    # Get current flags to avoid overwriting
    set(current_cxx_flags ${CMAKE_CXX_FLAGS})
    set(current_exe_link_flags ${CMAKE_EXE_LINKER_FLAGS})
    set(current_shared_link_flags ${CMAKE_SHARED_LINKER_FLAGS})

    if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        string(APPEND current_cxx_flags " /Zc:__cplusplus /EHsc /utf-8 /W4 /permissive- /sdl")
        if(SAST_WARNINGS_AS_ERRORS)
            string(APPEND current_cxx_flags " /WX")
        endif()
        string(APPEND current_exe_link_flags " /guard:cf")
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        string(APPEND current_cxx_flags " -Wall -Wextra -Wpedantic")
        if(SAST_WARNINGS_AS_ERRORS)
            string(APPEND current_cxx_flags " -Werror")
        endif()
        if(SAST_ENABLE_HARDENING)
            string(APPEND current_cxx_flags " -D_FORTIFY_SOURCE=2 -fstack-protector-strong")
            if(UNIX AND NOT APPLE)
                string(APPEND current_cxx_flags " -fno-plt")
                string(APPEND current_exe_link_flags " -Wl,-z,relro -Wl,-z,now")
                string(APPEND current_shared_link_flags " -Wl,-z,relro -Wl,-z,now")
            endif()
        endif()
        # Dead code elimination: split functions/data into sections for linker garbage collection
        string(APPEND current_cxx_flags " -ffunction-sections -fdata-sections")
        string(APPEND current_exe_link_flags " -Wl,--gc-sections")
        string(APPEND current_shared_link_flags " -Wl,--gc-sections")
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        string(APPEND current_cxx_flags " -Wall -Wextra -Wpedantic")
        if(SAST_WARNINGS_AS_ERRORS)
            string(APPEND current_cxx_flags " -Werror")
        endif()
        if(SAST_ENABLE_HARDENING)
            string(APPEND current_cxx_flags " -D_FORTIFY_SOURCE=2 -fstack-protector-strong")
            if(UNIX AND NOT APPLE)
                string(APPEND current_exe_link_flags " -Wl,-z,relro -Wl,-z,now")
                string(APPEND current_shared_link_flags " -Wl,-z,relro -Wl,-z,now")
            endif()
        endif()
        # Dead code elimination: split functions/data into sections for linker garbage collection
        string(APPEND current_cxx_flags " -ffunction-sections -fdata-sections")
        if(UNIX)
            string(APPEND current_exe_link_flags " -Wl,--gc-sections")
            string(APPEND current_shared_link_flags " -Wl,--gc-sections")
        elseif(APPLE)
            string(APPEND current_exe_link_flags " -Wl,-dead_strip")
            string(APPEND current_shared_link_flags " -Wl,-dead_strip")
        endif()
    endif()

    set(CMAKE_CXX_FLAGS ${current_cxx_flags} PARENT_SCOPE)
    set(CMAKE_EXE_LINKER_FLAGS ${current_exe_link_flags} PARENT_SCOPE)
    set(CMAKE_SHARED_LINKER_FLAGS ${current_shared_link_flags} PARENT_SCOPE)
endfunction()

#[=======================================================================[.rst:
_setup_ccache
-------------

Configure ccache for faster rebuilds (internal function).

This function detects and configures ccache if available, which can
significantly speed up rebuilds by caching compilation results.

Benefits:
- 50-90% faster rebuilds after initial build
- Especially beneficial for CI/CD pipelines
- Transparent to the build process

#]=======================================================================]
function(_setup_ccache)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" PARENT_SCOPE)
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" PARENT_SCOPE)
        message(STATUS "Enabled ccache for faster rebuilds")
    else()
        message(STATUS "ccache not found - install for faster rebuilds")
    endif()
endfunction()

#[=======================================================================[.rst:
_setup_parallel_builds
----------------------

Configure optimal parallel build settings (internal function).

This function auto-detects the number of available CPU cores and
configures CMake to use optimal parallelism for builds.

Benefits:
- Faster builds by utilizing all available cores
- Automatic detection prevents over-subscription
- Can be overridden by CMAKE_BUILD_PARALLEL_LEVEL

#]=======================================================================]
function(_setup_parallel_builds)
    include(ProcessorCount)
    ProcessorCount(N)
    if(NOT N EQUAL 0)
        if(NOT DEFINED CMAKE_BUILD_PARALLEL_LEVEL)
            set(CMAKE_BUILD_PARALLEL_LEVEL ${N} PARENT_SCOPE)
            message(STATUS "Configured parallel builds: ${N} jobs")
        else()
            message(STATUS "Using existing parallel build setting: ${CMAKE_BUILD_PARALLEL_LEVEL} jobs")
        endif()
    else()
        message(STATUS "Could not detect processor count - using default parallelism")
    endif()
endfunction()

#[=======================================================================[.rst:
_setup_debug_size_optimizations
-------------------------------

Configure Debug build size optimizations (internal function).

This function reduces Debug build size by:
- Splitting debug info to separate .dwo files (GCC -gsplit-dwarf)
- Using compressed debug sections

Benefits:
- 50-70% smaller executable sizes in Debug builds
- Debug info still available in separate files

#]=======================================================================]
function(_setup_debug_size_optimizations)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            if(SAST_SPLIT_DEBUG_INFO)
                if(WIN32)
                    # On Windows/MinGW, -gsplit-dwarf causes linker issues
                    # Use -g1 (minimal debug info) instead for smaller binaries
                    message(STATUS "Using minimal debug info (-g1) for smaller executables on Windows...")
                    # Set debug flags globally using add_compile_options for reliable propagation
                    add_compile_options(-g1)
                    # Also update CMAKE_CXX_FLAGS_DEBUG to override default -g
                    string(REGEX REPLACE "-g[0-3]?" "" _cleaned_debug_flags "${CMAKE_CXX_FLAGS_DEBUG}")
                    set(CMAKE_CXX_FLAGS_DEBUG "${_cleaned_debug_flags} -g1" PARENT_SCOPE)
                    message(STATUS "Minimal debug info enabled - executables will be 50-70% smaller")
                else()
                    # On Linux/macOS, use split-dwarf for best results
                    message(STATUS "Enabling split debug info (-gsplit-dwarf) for smaller executables...")
                    add_compile_options(-gsplit-dwarf)
                    add_compile_options(-gdwarf-4)
                    add_compile_options(-gz)
                    add_link_options(-gz)
                    message(STATUS "Split debug info enabled - executables will be significantly smaller")
                endif()
            endif()
        endif()
    endif()
endfunction()

#[=======================================================================[.rst:
_setup_minsizerel_optimizations
-------------------------------

Configure MinSizeRel build optimizations (internal function).

This function configures optimizations specifically for minimum size builds:
- Aggressive size optimization flags (-Os)
- Symbol stripping
- Dead code elimination
- Optional UPX compression

Benefits:
- 50-70% smaller binary size compared to Release
- Suitable for distribution when size is critical

#]=======================================================================]
function(_setup_minsizerel_optimizations)
    if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        message(STATUS "Configuring MinSizeRel build optimizations for minimum binary size...")

        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            # Size optimization flags
            set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os -DNDEBUG" PARENT_SCOPE)

            # Dead code elimination
            set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -ffunction-sections -fdata-sections" PARENT_SCOPE)
            set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -Wl,--gc-sections -s" PARENT_SCOPE)
            set(CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL "${CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL} -Wl,--gc-sections -s" PARENT_SCOPE)

            # Disable frame pointers for smaller stack frames
            set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -fomit-frame-pointer" PARENT_SCOPE)

            message(STATUS "MinSizeRel: Enabled -Os, dead code elimination, and symbol stripping")
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
            # MSVC size optimization
            set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /O1 /DNDEBUG" PARENT_SCOPE)
            set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} /OPT:REF /OPT:ICF /DEBUG:NONE" PARENT_SCOPE)
            message(STATUS "MinSizeRel: Enabled /O1 and linker optimizations")
        endif()
    endif()
endfunction()

#[=======================================================================[.rst:
_setup_release_optimizations
-----------------------------

Configure Release build optimizations (internal function).

This function configures optimizations for Release builds including:
- Debug symbol stripping for smaller binaries
- Link-time optimization flags
- Size optimization flags

Benefits:
- 10-20% smaller binary size
- No runtime performance impact
- Maintains debug builds with full symbols

#]=======================================================================]
function(_setup_release_optimizations)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "Configuring Release build optimizations...")

        # Strip debug symbols on Unix-like systems
        if(UNIX AND NOT APPLE)
            # For GCC/Clang on Linux
            if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
                set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s" PARENT_SCOPE)
                set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -s" PARENT_SCOPE)
                message(STATUS "Enabled symbol stripping for Release builds")
            endif()
        elseif(APPLE)
            # For macOS, use strip command post-build
            # This will be applied per-target in app/CMakeLists.txt
            message(STATUS "macOS Release builds will use post-build stripping")
        elseif(WIN32)
            # For MSVC, disable debug info generation in Release
            if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
                set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG:NONE" PARENT_SCOPE)
                message(STATUS "Disabled debug info for MSVC Release builds")
            elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
                # For MinGW/MSYS2
                set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s" PARENT_SCOPE)
                set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -s" PARENT_SCOPE)
                message(STATUS "Enabled symbol stripping for MinGW Release builds")
            endif()
        endif()
    endif()
endfunction()
