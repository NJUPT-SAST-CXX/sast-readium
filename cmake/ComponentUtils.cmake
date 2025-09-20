# ComponentUtils.cmake - Component library creation and management utilities for SAST Readium
# This module provides functions for creating reusable component libraries

cmake_minimum_required(VERSION 3.28)

include(SourceUtils)
include(TargetUtils)

#[=======================================================================[.rst:
create_component_library
------------------------

Create a reusable component library from discovered sources.

.. code-block:: cmake

  create_component_library(library_name
    COMPONENTS component1 component2 ...
    [BASE_DIR directory]
    [TYPE STATIC|SHARED|INTERFACE]
    [DESCRIPTION "Library description"]
    [EXCLUDE_PATTERNS pattern1 pattern2 ...]
    [ADDITIONAL_SOURCES source1 source2 ...]
  )

Arguments:
  library_name        Name of the component library to create
  COMPONENTS         List of component directories to include
  BASE_DIR           Base directory for source discovery (default: ../app)
  TYPE               Library type (default: STATIC)
  DESCRIPTION        Description for the library
  EXCLUDE_PATTERNS   Patterns to exclude from source discovery
  ADDITIONAL_SOURCES Additional source files to include

This function creates a reusable component library that can be shared
between the main application and tests, eliminating source duplication.

#]=======================================================================]
function(create_component_library library_name)
    cmake_parse_arguments(COMP 
        "" 
        "BASE_DIR;TYPE;DESCRIPTION" 
        "COMPONENTS;EXCLUDE_PATTERNS;ADDITIONAL_SOURCES" 
        ${ARGN}
    )
    
    if(NOT COMP_COMPONENTS AND NOT COMP_ADDITIONAL_SOURCES)
        message(FATAL_ERROR "Either COMPONENTS or ADDITIONAL_SOURCES must be specified for ${library_name}")
    endif()
    
    # Set defaults
    set(base_dir ${COMP_BASE_DIR})
    if(NOT base_dir)
        set(base_dir "${CMAKE_CURRENT_SOURCE_DIR}/../app")
    endif()
    
    set(lib_type ${COMP_TYPE})
    if(NOT lib_type)
        set(lib_type STATIC)
    endif()
    
    message(STATUS "Creating component library: ${library_name}")
    if(COMP_DESCRIPTION)
        message(STATUS "  Description: ${COMP_DESCRIPTION}")
    endif()
    
    # Discover sources for the specified components (if any)
    set(component_sources)
    if(COMP_COMPONENTS)
        discover_component_sources(component_sources
            BASE_DIR ${base_dir}
            COMPONENTS ${COMP_COMPONENTS}
            EXCLUDE_PATTERNS ${COMP_EXCLUDE_PATTERNS}
        )
    endif()
    
    # Add additional sources if specified
    set(all_sources ${component_sources})
    if(COMP_ADDITIONAL_SOURCES)
        list(APPEND all_sources ${COMP_ADDITIONAL_SOURCES})
    endif()
    
    if(NOT all_sources)
        message(WARNING "No sources found for component library ${library_name}")
        return()
    endif()
    
    # Create the library
    add_library(${library_name} ${lib_type})
    
    # Add sources with proper path resolution
    set(resolved_sources)
    foreach(source ${all_sources})
        if(IS_ABSOLUTE ${source})
            list(APPEND resolved_sources ${source})
        else()
            list(APPEND resolved_sources "${base_dir}/${source}")
        endif()
    endforeach()
    
    target_sources(${library_name} PRIVATE ${resolved_sources})
    
    # Apply common target setup
    setup_target(${library_name} TYPE LIBRARY)

    # Add the base directory as an include directory for proper header resolution
    target_include_directories(${library_name} PUBLIC ${base_dir})



    # Set library-specific properties
    _configure_component_library_properties(${library_name} "${COMP_COMPONENTS}")
    
    list(LENGTH all_sources source_count)
    message(STATUS "Component library ${library_name} created with ${source_count} sources")
endfunction()

#[=======================================================================[.rst:
create_standard_app_components
------------------------------

Create standard component libraries for the application.

.. code-block:: cmake

  create_standard_app_components([BASE_DIR directory])

Arguments:
  BASE_DIR    Base directory for source discovery (default: ../app)

This function creates a set of standard component libraries that cover
the main application components: Core, UI, Models, Controllers, etc.

#]=======================================================================]
function(create_standard_app_components)
    cmake_parse_arguments(STD "" "BASE_DIR" "" ${ARGN})
    
    set(base_dir ${STD_BASE_DIR})
    if(NOT base_dir)
        set(base_dir "${CMAKE_CURRENT_SOURCE_DIR}/../app")
    endif()
    
    message(STATUS "Creating standard application component libraries...")
    
    # Core application components (excluding logging to avoid MOC conflicts)
    create_component_library(CoreAppLib
        BASE_DIR ${base_dir}
        COMPONENTS
            ui/viewer
            model
            cache
            utils
        DESCRIPTION "Core application components"
        EXCLUDE_PATTERNS ".*_test\\.cpp$" ".*Test\\.cpp$" "^test_.*\\.cpp$"
    )

    # Search engine components
    create_component_library(SearchLib
        BASE_DIR ${base_dir}
        COMPONENTS search
        DESCRIPTION "Search engine components"
        EXCLUDE_PATTERNS ".*_test\\.cpp$" ".*Test\\.cpp$" "^test_.*\\.cpp$"
    )

    # UI components
    create_component_library(UILib
        BASE_DIR ${base_dir}
        COMPONENTS
            ui/core
            ui/widgets
            ui/dialogs
            ui/thumbnail
            ui/managers
            plugin
            delegate
            view
        DESCRIPTION "UI components"
        EXCLUDE_PATTERNS ".*_test\\.cpp$" ".*Test\\.cpp$" "^test_.*\\.cpp$"
    )

    # Controller components
    create_component_library(ControllerLib
        BASE_DIR ${base_dir}
        COMPONENTS controller
        DESCRIPTION "Controller components"
        EXCLUDE_PATTERNS ".*_test\\.cpp$" ".*Test\\.cpp$" "^test_.*\\.cpp$"
    )

    # Factory components
    create_component_library(FactoryLib
        BASE_DIR ${base_dir}
        COMPONENTS factory
        DESCRIPTION "Factory components"
        EXCLUDE_PATTERNS ".*_test\\.cpp$" ".*Test\\.cpp$" "^test_.*\\.cpp$"
    )

    # Command system components
    create_component_library(CommandLib
        BASE_DIR ${base_dir}
        COMPONENTS command
        DESCRIPTION "Command system components"
        EXCLUDE_PATTERNS ".*_test\\.cpp$" ".*Test\\.cpp$" "^test_.*\\.cpp$"
    )

    # Manager components
    create_component_library(ManagerLib
        BASE_DIR ${base_dir}
        COMPONENTS managers
        DESCRIPTION "Manager components"
        EXCLUDE_PATTERNS ".*_test\\.cpp$" ".*Test\\.cpp$" "^test_.*\\.cpp$"
    )

    # Logging components
    create_component_library(LoggingLib
        BASE_DIR ${base_dir}
        COMPONENTS logging
        DESCRIPTION "Logging components"
        EXCLUDE_PATTERNS ".*_test\\.cpp$" ".*Test\\.cpp$" "^test_.*\\.cpp$"
    )

    # Main application components (root level files)
    create_component_library(MainLib
        BASE_DIR ${base_dir}
        ADDITIONAL_SOURCES MainWindow.cpp MainWindow.h
        DESCRIPTION "Main application components"
    )

    # Set up library dependencies
    message(STATUS "Setting up component library dependencies...")

    # CoreAppLib depends on SearchLib, ManagerLib, and LoggingLib (NOT UILib to avoid circular dependency)
    if(TARGET CoreAppLib AND TARGET SearchLib AND TARGET ManagerLib AND TARGET LoggingLib)
        target_link_libraries(CoreAppLib PUBLIC SearchLib ManagerLib LoggingLib)
        message(STATUS "  CoreAppLib -> SearchLib, ManagerLib, LoggingLib")
    endif()

    # UILib depends on ControllerLib, FactoryLib, and ManagerLib
    if(TARGET UILib AND TARGET ControllerLib AND TARGET FactoryLib AND TARGET ManagerLib)
        target_link_libraries(UILib PUBLIC ControllerLib FactoryLib ManagerLib)
        message(STATUS "  UILib -> ControllerLib, FactoryLib, ManagerLib")
    endif()

    # SearchLib depends on LoggingLib
    if(TARGET SearchLib AND TARGET LoggingLib)
        target_link_libraries(SearchLib PUBLIC LoggingLib)
        message(STATUS "  SearchLib -> LoggingLib")
    endif()

    # FactoryLib depends on ControllerLib and CommandLib
    if(TARGET FactoryLib AND TARGET ControllerLib AND TARGET CommandLib)
        target_link_libraries(FactoryLib PUBLIC ControllerLib CommandLib)
        message(STATUS "  FactoryLib -> ControllerLib, CommandLib")
    endif()

    # ControllerLib depends on CoreAppLib, MainLib, ManagerLib and LoggingLib
    if(TARGET ControllerLib AND TARGET CoreAppLib AND TARGET MainLib AND TARGET ManagerLib AND TARGET LoggingLib)
        target_link_libraries(ControllerLib PUBLIC CoreAppLib MainLib ManagerLib LoggingLib)
        message(STATUS "  ControllerLib -> CoreAppLib, MainLib, ManagerLib, LoggingLib")
    endif()

    # CommandLib depends on ControllerLib and LoggingLib
    if(TARGET CommandLib AND TARGET ControllerLib AND TARGET LoggingLib)
        target_link_libraries(CommandLib PUBLIC ControllerLib LoggingLib)
        message(STATUS "  CommandLib -> ControllerLib, LoggingLib")
    endif()

    message(STATUS "Standard component libraries created successfully")
endfunction()

#[=======================================================================[.rst:
get_component_libraries
-----------------------

Get a list of standard component libraries for linking.

.. code-block:: cmake

  get_component_libraries(output_var [CATEGORIES category1 category2 ...])

Arguments:
  output_var    Variable to store the library list
  CATEGORIES   Specific categories to include (default: all)

Available categories: Core, Search, UI, Controller, Factory, Command, Manager

#]=======================================================================]
function(get_component_libraries output_var)
    cmake_parse_arguments(GET "" "" "CATEGORIES" ${ARGN})
    
    # Define all available component libraries
    set(all_libraries
        CoreAppLib
        SearchLib
        UILib
        ControllerLib
        FactoryLib
        CommandLib
        ManagerLib
        LoggingLib
    )
    
    # If specific categories requested, filter the list
    if(GET_CATEGORIES)
        set(filtered_libraries)
        foreach(category ${GET_CATEGORIES})
            if(category STREQUAL "Core")
                list(APPEND filtered_libraries CoreAppLib)
            elseif(category STREQUAL "Search")
                list(APPEND filtered_libraries SearchLib)
            elseif(category STREQUAL "UI")
                list(APPEND filtered_libraries UILib)
            elseif(category STREQUAL "Controller")
                list(APPEND filtered_libraries ControllerLib)
            elseif(category STREQUAL "Factory")
                list(APPEND filtered_libraries FactoryLib)
            elseif(category STREQUAL "Command")
                list(APPEND filtered_libraries CommandLib)
            elseif(category STREQUAL "Manager")
                list(APPEND filtered_libraries ManagerLib)
            else()
                message(WARNING "Unknown component category: ${category}")
            endif()
        endforeach()
        set(${output_var} ${filtered_libraries} PARENT_SCOPE)
    else()
        set(${output_var} ${all_libraries} PARENT_SCOPE)
    endif()
endfunction()

#[=======================================================================[.rst:
_configure_component_library_properties
---------------------------------------

Internal function to configure component library properties.
#]=======================================================================]
function(_configure_component_library_properties library_name components)
    # Set include directories based on components
    set(include_dirs)
    foreach(component ${components})
        get_filename_component(parent_dir ${component} DIRECTORY)
        if(parent_dir)
            list(APPEND include_dirs ${parent_dir})
        endif()
        list(APPEND include_dirs ${component})
    endforeach()
    
    if(include_dirs)
        list(REMOVE_DUPLICATES include_dirs)
        foreach(include_dir ${include_dirs})
            target_include_directories(${library_name} PUBLIC 
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../app/${include_dir}>
            )
        endforeach()
    endif()
    
    # Apply QGraphics PDF support if enabled
    if(ENABLE_QGRAPHICS_PDF_SUPPORT)
        target_compile_definitions(${library_name} PUBLIC ENABLE_QGRAPHICS_PDF_SUPPORT)
    endif()
endfunction()

#[=======================================================================[.rst:
validate_component_libraries
----------------------------

Validate that component libraries were created successfully.

.. code-block:: cmake

  validate_component_libraries(library_list)

Arguments:
  library_list    List of library names to validate

#]=======================================================================]
function(validate_component_libraries library_list)
    set(validation_errors)
    
    foreach(library ${library_list})
        if(NOT TARGET ${library})
            list(APPEND validation_errors "Component library not found: ${library}")
        endif()
    endforeach()
    
    if(validation_errors)
        message(WARNING "Component library validation warnings:")
        foreach(error ${validation_errors})
            message(WARNING "  ${error}")
        endforeach()
    else()
        message(STATUS "Component library validation passed")
    endif()
endfunction()

#[=======================================================================[.rst:
print_component_summary
-----------------------

Print a summary of created component libraries.

.. code-block:: cmake

  print_component_summary()

#]=======================================================================]
function(print_component_summary)
    message(STATUS "")
    message(STATUS "=== Component Libraries Summary ===")
    
    get_component_libraries(all_libs)
    
    foreach(lib ${all_libs})
        if(TARGET ${lib})
            get_target_property(sources ${lib} SOURCES)
            if(sources)
                list(LENGTH sources source_count)
                message(STATUS "${lib}: ${source_count} sources")
            else()
                message(STATUS "${lib}: no sources")
            endif()
        else()
            message(STATUS "${lib}: not created")
        endif()
    endforeach()
    
    message(STATUS "===================================")
    message(STATUS "")
endfunction()

#[=======================================================================[.rst:
print_component_summary
-----------------------

Print a summary of available component libraries.

.. code-block:: cmake

  print_component_summary()

#]=======================================================================]
function(print_component_summary)
    message(STATUS "")
    message(STATUS "=== Component Libraries Summary ===")
    message(STATUS "CoreAppLib: Core application components (model, cache, utils)")
    message(STATUS "SearchLib: Search engine components")
    message(STATUS "UILib: User interface components")
    message(STATUS "ControllerLib: Controller components")
    message(STATUS "FactoryLib: Factory components")
    message(STATUS "CommandLib: Command system components")
    message(STATUS "ManagerLib: Manager components")
    message(STATUS "===================================")
    message(STATUS "")
endfunction()
