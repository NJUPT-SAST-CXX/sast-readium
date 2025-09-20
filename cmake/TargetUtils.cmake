# TargetUtils.cmake - Target setup and configuration utilities for SAST Readium
# This module provides functions for standardized target creation and configuration

cmake_minimum_required(VERSION 3.28)

include(Dependencies)
include(PlatformUtils)
include(CompilerSettings)
include(SourceUtils)

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
  target_name           Name of the target to configure
  TYPE                  Type of target (EXECUTABLE or LIBRARY)
  SOURCES              List of source files
  INCLUDE_DIRS         List of include directories
  LINK_LIBRARIES       List of libraries to link
  COMPILE_DEFINITIONS  List of compile definitions
  CXX_STANDARD         C++ standard (default: 20)

This function applies common settings to targets including:
- C++ standard and compile features
- Common include directories
- Standard library linkage
- Platform-specific configurations

#]=======================================================================]
function(setup_target target_name)
    cmake_parse_arguments(TARGET 
        "" 
        "TYPE;CXX_STANDARD" 
        "SOURCES;INCLUDE_DIRS;LINK_LIBRARIES;COMPILE_DEFINITIONS" 
        ${ARGN}
    )
    
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Target ${target_name} does not exist")
    endif()
    
    message(STATUS "Setting up target: ${target_name}")
    
    # Set C++ standard
    set(cxx_standard ${TARGET_CXX_STANDARD})
    if(NOT cxx_standard)
        set(cxx_standard 20)
    endif()
    
    target_compile_features(${target_name} PUBLIC cxx_std_${cxx_standard})
    message(STATUS "  C++ standard: ${cxx_standard}")
    
    # Add sources if provided
    if(TARGET_SOURCES)
        target_sources(${target_name} PRIVATE ${TARGET_SOURCES})
        list(LENGTH TARGET_SOURCES source_count)
        message(STATUS "  Added ${source_count} source files")
    endif()
    
    # Set up include directories
    _setup_target_includes(${target_name} "${TARGET_INCLUDE_DIRS}")
    
    # Set up library linkage
    _setup_target_libraries(${target_name} "${TARGET_LINK_LIBRARIES}")
    
    # Apply compile definitions
    if(TARGET_COMPILE_DEFINITIONS)
        target_compile_definitions(${target_name} PRIVATE ${TARGET_COMPILE_DEFINITIONS})
        message(STATUS "  Applied compile definitions")
    endif()
    
    # Apply common target settings
    _apply_common_target_settings(${target_name})
    
    message(STATUS "Target ${target_name} configured successfully")
endfunction()

#[=======================================================================[.rst:
setup_executable
---------------

Set up an executable target with common configuration.

.. code-block:: cmake

  setup_executable(target_name
    [SOURCES source1 source2 ...]
    [WIN32]
    [MACOSX_BUNDLE]
  )

Arguments:
  target_name    Name of the executable target
  SOURCES       List of source files
  WIN32         Create a Windows GUI application
  MACOSX_BUNDLE Create a macOS bundle

#]=======================================================================]
function(setup_executable target_name)
    cmake_parse_arguments(EXE "WIN32;MACOSX_BUNDLE" "" "SOURCES" ${ARGN})
    
    # Create executable with appropriate options
    set(exe_options)
    if(EXE_WIN32)
        list(APPEND exe_options WIN32)
    endif()
    if(EXE_MACOSX_BUNDLE)
        list(APPEND exe_options MACOSX_BUNDLE)
    endif()
    
    add_executable(${target_name} ${exe_options})
    
    # Apply common setup
    setup_target(${target_name} 
        TYPE EXECUTABLE
        SOURCES ${EXE_SOURCES}
    )
    
    # Set runtime output directory
    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    # Platform-specific executable configuration
    _configure_executable_platform_specific(${target_name})
    
    message(STATUS "Executable ${target_name} set up successfully")
endfunction()

#[=======================================================================[.rst:
setup_library
------------

Set up a library target with common configuration.

.. code-block:: cmake

  setup_library(target_name
    [TYPE STATIC|SHARED|INTERFACE]
    [SOURCES source1 source2 ...]
    [PUBLIC_HEADERS header1 header2 ...]
  )

Arguments:
  target_name     Name of the library target
  TYPE           Type of library (STATIC, SHARED, or INTERFACE)
  SOURCES        List of source files
  PUBLIC_HEADERS List of public header files

#]=======================================================================]
function(setup_library target_name)
    cmake_parse_arguments(LIB "" "TYPE" "SOURCES;PUBLIC_HEADERS" ${ARGN})
    
    # Default to STATIC if no type specified
    set(lib_type ${LIB_TYPE})
    if(NOT lib_type)
        set(lib_type STATIC)
    endif()
    
    add_library(${target_name} ${lib_type})
    
    # Apply common setup
    setup_target(${target_name} 
        TYPE LIBRARY
        SOURCES ${LIB_SOURCES}
    )
    
    # Set up public headers
    if(LIB_PUBLIC_HEADERS)
        set_target_properties(${target_name} PROPERTIES
            PUBLIC_HEADER "${LIB_PUBLIC_HEADERS}"
        )
        message(STATUS "  Set public headers for ${target_name}")
    endif()
    
    message(STATUS "Library ${target_name} (${lib_type}) set up successfully")
endfunction()

#[=======================================================================[.rst:
add_asset_copying
----------------

Add asset copying functionality to a target.

.. code-block:: cmake

  add_asset_copying(target_name source_dir [destination_dir])

Arguments:
  target_name      Target that depends on the assets
  source_dir       Source directory containing assets
  destination_dir  Destination directory (default: target output dir)

#]=======================================================================]
function(add_asset_copying target_name source_dir)
    cmake_parse_arguments(ASSETS "" "DESTINATION" "" ${ARGN})
    
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Target ${target_name} does not exist")
    endif()
    
    # Default destination to target output directory
    set(dest_dir ${ASSETS_DESTINATION})
    if(NOT dest_dir)
        set(dest_dir $<TARGET_FILE_DIR:${target_name}>)
    endif()
    
    # Create asset copying target
    set(asset_target "${target_name}_assets")
    set(dummy_file "${CMAKE_BINARY_DIR}/${asset_target}_dummy")
    
    add_custom_target(${asset_target} DEPENDS ${dummy_file})
    add_custom_command(
        OUTPUT ${dummy_file}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${source_dir} ${dest_dir}
        COMMAND ${CMAKE_COMMAND} -E touch ${dummy_file}
        DEPENDS ${source_dir}
        COMMENT "Copying assets from ${source_dir} to ${dest_dir}"
        VERBATIM
    )
    
    # Make target depend on asset copying
    add_dependencies(${target_name} ${asset_target})
    
    message(STATUS "Asset copying configured for ${target_name}: ${source_dir} -> ${dest_dir}")
endfunction()

#[=======================================================================[.rst:
_setup_target_includes
---------------------

Internal function to set up target include directories.
#]=======================================================================]
function(_setup_target_includes target_name include_dirs)
    # Common include directories
    set(common_includes
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    # Add custom include directories
    if(include_dirs)
        list(APPEND common_includes ${include_dirs})
    endif()
    
    target_include_directories(${target_name} PRIVATE ${common_includes})
    
    list(LENGTH common_includes include_count)
    message(STATUS "  Added ${include_count} include directories")
endfunction()

#[=======================================================================[.rst:
_setup_target_libraries
----------------------

Internal function to set up target library linkage.
#]=======================================================================]
function(_setup_target_libraries target_name link_libraries)
    # Get common libraries
    get_common_libraries(common_libs)
    
    # Add custom libraries
    set(all_libs ${common_libs})
    if(link_libraries)
        list(APPEND all_libs ${link_libraries})
    endif()
    
    target_link_libraries(${target_name} PRIVATE ${all_libs})
    
    list(LENGTH all_libs lib_count)
    message(STATUS "  Linked ${lib_count} libraries")
endfunction()

#[=======================================================================[.rst:
_apply_common_target_settings
----------------------------

Internal function to apply common settings to all targets.
#]=======================================================================]
function(_apply_common_target_settings target_name)
    # Apply QGraphics PDF support if enabled
    if(ENABLE_QGRAPHICS_PDF_SUPPORT)
        target_compile_definitions(${target_name} PRIVATE ENABLE_QGRAPHICS_PDF_SUPPORT)
    endif()
    
    # Apply common compile options
    get_common_compile_options(compile_options)
    if(compile_options)
        target_compile_options(${target_name} PRIVATE ${compile_options})
    endif()
endfunction()

#[=======================================================================[.rst:
_configure_executable_platform_specific
--------------------------------------

Internal function for platform-specific executable configuration.
#]=======================================================================]
function(_configure_executable_platform_specific target_name)
    if(WIN32)
        # Windows-specific executable properties
        set_target_properties(${target_name} PROPERTIES
            WIN32_EXECUTABLE $<IF:$<CONFIG:Release>,ON,OFF>
        )

        # Set up Windows deployment
        setup_deployment_tools(${target_name})
    endif()
endfunction()

#[=======================================================================[.rst:
setup_executable_with_components
--------------------------------

Set up an executable target using component-based source discovery.

.. code-block:: cmake

  setup_executable_with_components(target_name
    [BASE_DIR directory]
    [ADDITIONAL_SOURCES source1 source2 ...]
    [WIN32]
    [MACOSX_BUNDLE]
  )

Arguments:
  target_name         Name of the executable target
  BASE_DIR           Base directory for source discovery (default: current)
  ADDITIONAL_SOURCES Additional source files beyond discovered ones
  WIN32              Create a Windows GUI application
  MACOSX_BUNDLE      Create a macOS bundle

This function automatically discovers application sources and creates
an executable with proper configuration.

#]=======================================================================]
function(setup_executable_with_components target_name)
    cmake_parse_arguments(EXE "WIN32;MACOSX_BUNDLE" "BASE_DIR" "ADDITIONAL_SOURCES" ${ARGN})

    set(base_dir ${EXE_BASE_DIR})
    if(NOT base_dir)
        set(base_dir ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    message(STATUS "Setting up executable with component discovery: ${target_name}")

    # Discover application sources
    discover_app_sources(discovered_sources BASE_DIR ${base_dir})

    # Combine with additional sources
    set(all_sources ${discovered_sources})
    if(EXE_ADDITIONAL_SOURCES)
        list(APPEND all_sources ${EXE_ADDITIONAL_SOURCES})
    endif()

    # Validate sources
    validate_discovered_sources("${all_sources}"
        REQUIRED_FILES "main.cpp"
        REQUIRED_PATTERNS ".*\\.cpp$"
    )

    # Create executable with appropriate options
    set(exe_options)
    if(EXE_WIN32)
        list(APPEND exe_options WIN32)
    endif()
    if(EXE_MACOSX_BUNDLE)
        list(APPEND exe_options MACOSX_BUNDLE)
    endif()

    add_executable(${target_name} ${exe_options})

    # Apply common setup
    setup_target(${target_name}
        TYPE EXECUTABLE
        SOURCES ${all_sources}
    )

    # Set runtime output directory
    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    # Platform-specific executable configuration
    _configure_executable_platform_specific(${target_name})

    # Print source summary
    print_source_summary("${all_sources}" TITLE "Executable Sources: ${target_name}")

    message(STATUS "Executable ${target_name} set up successfully with component discovery")
endfunction()

#[=======================================================================[.rst:
setup_library_with_components
-----------------------------

Set up a library target using component-based source discovery.

.. code-block:: cmake

  setup_library_with_components(target_name
    COMPONENTS component1 component2 ...
    [BASE_DIR directory]
    [TYPE STATIC|SHARED|INTERFACE]
    [ADDITIONAL_SOURCES source1 source2 ...]
  )

Arguments:
  target_name         Name of the library target
  COMPONENTS         List of component directories to include
  BASE_DIR           Base directory for source discovery (default: current)
  TYPE               Library type (default: STATIC)
  ADDITIONAL_SOURCES Additional source files beyond discovered ones

This function creates a library using component-based source discovery.

#]=======================================================================]
function(setup_library_with_components target_name)
    cmake_parse_arguments(LIB "" "BASE_DIR;TYPE" "COMPONENTS;ADDITIONAL_SOURCES" ${ARGN})

    if(NOT LIB_COMPONENTS)
        message(FATAL_ERROR "COMPONENTS must be specified for ${target_name}")
    endif()

    set(base_dir ${LIB_BASE_DIR})
    if(NOT base_dir)
        set(base_dir ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    set(lib_type ${LIB_TYPE})
    if(NOT lib_type)
        set(lib_type STATIC)
    endif()

    message(STATUS "Setting up library with component discovery: ${target_name}")

    # Discover component sources
    discover_component_sources(discovered_sources
        BASE_DIR ${base_dir}
        COMPONENTS ${LIB_COMPONENTS}
    )

    # Combine with additional sources
    set(all_sources ${discovered_sources})
    if(LIB_ADDITIONAL_SOURCES)
        list(APPEND all_sources ${LIB_ADDITIONAL_SOURCES})
    endif()

    if(NOT all_sources)
        message(WARNING "No sources found for library ${target_name}")
        return()
    endif()

    # Create library
    add_library(${target_name} ${lib_type})

    # Apply common setup
    setup_target(${target_name}
        TYPE LIBRARY
        SOURCES ${all_sources}
    )

    # Print source summary
    print_source_summary("${all_sources}" TITLE "Library Sources: ${target_name}")

    message(STATUS "Library ${target_name} (${lib_type}) set up successfully with component discovery")
endfunction()
