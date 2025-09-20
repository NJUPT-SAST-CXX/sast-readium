# SourceUtils.cmake - Source file discovery and organization utilities for SAST Readium
# This module provides functions for automatic source file discovery and component organization

cmake_minimum_required(VERSION 3.28)

#[=======================================================================[.rst:
discover_component_sources
--------------------------

Automatically discover source files organized by component.

.. code-block:: cmake

  discover_component_sources(output_var
    BASE_DIR directory
    COMPONENTS component1 component2 ...
    [EXTENSIONS .cpp .cc .cxx]
    [EXCLUDE_PATTERNS pattern1 pattern2 ...]
    [INCLUDE_PATTERNS pattern1 pattern2 ...]
  )

Arguments:
  output_var         Variable to store the discovered source files
  BASE_DIR          Base directory to search for sources
  COMPONENTS        List of component directories to search
  EXTENSIONS        File extensions to include (default: .cpp .cc .cxx)
  EXCLUDE_PATTERNS  Patterns to exclude from discovery
  INCLUDE_PATTERNS  Additional patterns to include

This function automatically discovers source files organized by component
directories, eliminating the need for manual source file lists.

#]=======================================================================]
function(discover_component_sources output_var)
    cmake_parse_arguments(DISC 
        "" 
        "BASE_DIR" 
        "COMPONENTS;EXTENSIONS;EXCLUDE_PATTERNS;INCLUDE_PATTERNS" 
        ${ARGN}
    )
    
    if(NOT DISC_BASE_DIR)
        message(FATAL_ERROR "BASE_DIR must be specified")
    endif()
    
    if(NOT DISC_COMPONENTS)
        message(FATAL_ERROR "COMPONENTS must be specified")
    endif()
    
    # Default extensions
    set(extensions ${DISC_EXTENSIONS})
    if(NOT extensions)
        set(extensions .cpp .cc .cxx)
    endif()
    
    set(discovered_sources)
    
    foreach(component ${DISC_COMPONENTS})
        set(component_dir "${DISC_BASE_DIR}/${component}")
        
        if(EXISTS ${component_dir})
            _discover_sources_in_directory(component_sources 
                ${component_dir} 
                "${extensions}"
                "${DISC_EXCLUDE_PATTERNS}"
                "${DISC_INCLUDE_PATTERNS}"
            )
            
            if(component_sources)
                list(APPEND discovered_sources ${component_sources})
                list(LENGTH component_sources source_count)
                message(STATUS "  ${component}: found ${source_count} source files")
            endif()
        else()
            message(STATUS "  ${component}: directory not found, skipping")
        endif()
    endforeach()
    
    list(LENGTH discovered_sources total_count)
    message(STATUS "Total discovered sources: ${total_count}")
    
    set(${output_var} ${discovered_sources} PARENT_SCOPE)
endfunction()

#[=======================================================================[.rst:
discover_app_sources
--------------------

Discover source files for the main application using predefined components.

.. code-block:: cmake

  discover_app_sources(output_var [BASE_DIR directory])

Arguments:
  output_var    Variable to store the discovered source files
  BASE_DIR     Base directory (default: CMAKE_CURRENT_SOURCE_DIR)

This is a convenience function that discovers sources for the standard
application components: ui, managers, model, controller, delegate, view,
cache, utils, plugin, factory, command, search, logging.

#]=======================================================================]
function(discover_app_sources output_var)
    cmake_parse_arguments(APP "" "BASE_DIR" "" ${ARGN})
    
    set(base_dir ${APP_BASE_DIR})
    if(NOT base_dir)
        set(base_dir ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
    
    message(STATUS "Discovering application sources in: ${base_dir}")
    
    # Define standard application components
    set(app_components
        ui/core
        ui/viewer
        ui/widgets
        ui/dialogs
        ui/thumbnail
        ui/managers
        managers
        model
        controller
        delegate
        view
        cache
        utils
        plugin
        factory
        command
        search
        logging
    )
    
    discover_component_sources(app_sources
        BASE_DIR ${base_dir}
        COMPONENTS ${app_components}
        EXCLUDE_PATTERNS ".*_test\\.cpp$" ".*Test\\.cpp$" "^test_.*\\.cpp$"
    )
    
    # Add main application files
    set(main_files)
    foreach(main_file main.cpp MainWindow.cpp)
        if(EXISTS "${base_dir}/${main_file}")
            list(APPEND main_files "${main_file}")
        endif()
    endforeach()
    
    if(main_files)
        list(APPEND app_sources ${main_files})
        list(LENGTH main_files main_count)
        message(STATUS "  main: found ${main_count} main files")
    endif()
    
    set(${output_var} ${app_sources} PARENT_SCOPE)
endfunction()

#[=======================================================================[.rst:
validate_discovered_sources
---------------------------

Validate that discovered sources include expected files.

.. code-block:: cmake

  validate_discovered_sources(source_list
    [REQUIRED_FILES file1 file2 ...]
    [REQUIRED_PATTERNS pattern1 pattern2 ...]
  )

Arguments:
  source_list        List of discovered source files
  REQUIRED_FILES     Specific files that must be present
  REQUIRED_PATTERNS  Patterns that must match at least one file

This function validates that source discovery found expected files,
helping to catch missing sources early in the build process.

#]=======================================================================]
function(validate_discovered_sources source_list)
    cmake_parse_arguments(VAL "" "" "REQUIRED_FILES;REQUIRED_PATTERNS" ${ARGN})
    
    set(validation_errors)
    
    # Check required files
    if(VAL_REQUIRED_FILES)
        foreach(required_file ${VAL_REQUIRED_FILES})
            set(found FALSE)
            foreach(source ${source_list})
                if(source MATCHES ".*${required_file}$")
                    set(found TRUE)
                    break()
                endif()
            endforeach()
            
            if(NOT found)
                list(APPEND validation_errors "Required file not found: ${required_file}")
            endif()
        endforeach()
    endif()
    
    # Check required patterns
    if(VAL_REQUIRED_PATTERNS)
        foreach(pattern ${VAL_REQUIRED_PATTERNS})
            set(found FALSE)
            foreach(source ${source_list})
                if(source MATCHES ${pattern})
                    set(found TRUE)
                    break()
                endif()
            endforeach()
            
            if(NOT found)
                list(APPEND validation_errors "No files found matching pattern: ${pattern}")
            endif()
        endforeach()
    endif()
    
    # Report validation results
    if(validation_errors)
        message(WARNING "Source validation warnings:")
        foreach(error ${validation_errors})
            message(WARNING "  ${error}")
        endforeach()
    else()
        message(STATUS "Source validation passed")
    endif()
endfunction()

#[=======================================================================[.rst:
_discover_sources_in_directory
------------------------------

Internal function to discover sources in a specific directory.
#]=======================================================================]
function(_discover_sources_in_directory output_var directory extensions exclude_patterns include_patterns)
    set(sources)

    # Use GLOB_RECURSE directly for each extension to avoid duplicates
    foreach(ext ${extensions})
        file(GLOB_RECURSE ext_files "${directory}/*${ext}")
        if(ext_files)
            list(APPEND sources ${ext_files})
        endif()
    endforeach()

    # Remove duplicates (important for cases where files might match multiple patterns)
    if(sources)
        list(REMOVE_DUPLICATES sources)
    endif()
    
    # Apply exclude patterns
    if(exclude_patterns AND sources)
        foreach(exclude_pattern ${exclude_patterns})
            list(FILTER sources EXCLUDE REGEX ${exclude_pattern})
        endforeach()
    endif()
    
    # Apply include patterns (if specified, only keep matching files)
    if(include_patterns AND sources)
        set(filtered_sources)
        foreach(include_pattern ${include_patterns})
            foreach(source ${sources})
                if(source MATCHES ${include_pattern})
                    list(APPEND filtered_sources ${source})
                endif()
            endforeach()
        endforeach()
        set(sources ${filtered_sources})
    endif()
    
    # Convert to relative paths and ensure no duplicates
    set(relative_sources)
    foreach(source ${sources})
        file(RELATIVE_PATH rel_source ${CMAKE_CURRENT_SOURCE_DIR} ${source})
        list(APPEND relative_sources ${rel_source})
    endforeach()

    # Final duplicate removal on relative paths
    if(relative_sources)
        list(REMOVE_DUPLICATES relative_sources)
    endif()

    set(${output_var} ${relative_sources} PARENT_SCOPE)
endfunction()

#[=======================================================================[.rst:
print_source_summary
--------------------

Print a summary of discovered sources organized by component.

.. code-block:: cmake

  print_source_summary(source_list [TITLE "Custom Title"])

Arguments:
  source_list    List of source files to summarize
  TITLE         Custom title for the summary

#]=======================================================================]
function(print_source_summary source_list)
    cmake_parse_arguments(PRINT "" "TITLE" "" ${ARGN})
    
    set(title ${PRINT_TITLE})
    if(NOT title)
        set(title "Source File Summary")
    endif()
    
    message(STATUS "")
    message(STATUS "=== ${title} ===")
    
    # Organize sources by component
    set(components)
    foreach(source ${source_list})
        get_filename_component(dir ${source} DIRECTORY)
        if(dir)
            string(REGEX REPLACE "/.*" "" component ${dir})
            list(APPEND components ${component})
        else()
            list(APPEND components "root")
        endif()
    endforeach()
    
    if(components)
        list(REMOVE_DUPLICATES components)
        list(SORT components)
        
        foreach(component ${components})
            set(component_sources)
            foreach(source ${source_list})
                if(source MATCHES "^${component}/.*" OR (component STREQUAL "root" AND NOT source MATCHES ".*/.*"))
                    list(APPEND component_sources ${source})
                endif()
            endforeach()
            
            list(LENGTH component_sources count)
            message(STATUS "${component}: ${count} files")
        endforeach()
    endif()
    
    list(LENGTH source_list total)
    message(STATUS "Total: ${total} source files")
    message(STATUS "========================")
    message(STATUS "")
endfunction()
