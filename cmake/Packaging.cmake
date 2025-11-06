# Packaging.cmake - Comprehensive packaging configuration for SAST Readium
# Handles Windows MSYS2, MSVC, and cross-platform deployment
#
# This module provides complete packaging support for creating standalone,
# distributable installation packages for both MSYS2/MinGW and MSVC build
# environments on Windows, as well as other platforms.
#
# Key Features:
# - Automatic environment detection (MSYS2 vs MSVC)
# - Qt6 deployment using qt_generate_deploy_app_script()
# - Runtime dependency bundling (MSYS2 DLLs, MSVC redistributables)
# - CPack configuration for multiple package formats (NSIS, WiX MSI, ZIP)
# - Component-based installation (Application, Runtime, Resources, Documentation)
#
# Usage:
#   1. Call setup_packaging_options() to configure packaging options
#   2. Call find_qt_deployment_tools() to locate Qt deployment tools
#   3. Call setup_cpack_configuration() to configure CPack
#   4. In your target CMakeLists.txt:
#      - Call setup_install_targets(target_name) to configure installation
#      - Call setup_qt_deployment(target_name) to configure Qt deployment
#
# Package Generation:
#   cmake --build build --target package
#   or
#   cpack --config build/CPackConfig.cmake
#
# Environment-Specific Behavior:
# - MSYS2/MinGW: Bundles MinGW runtime DLLs, uses NSIS installer + ZIP
# - MSVC: Includes VC++ redistributables, uses WiX MSI + ZIP
# - Both: Automatically deploy Qt libraries, plugins, and platform dependencies

cmake_minimum_required(VERSION 3.28)

#[=======================================================================[.rst:
setup_packaging_options
-----------------------

Configure packaging-related options.

.. code-block:: cmake

  setup_packaging_options()

Defines packaging options:
- ENABLE_PACKAGING: Enable packaging support
- PACKAGE_PORTABLE: Create portable ZIP packages
- PACKAGE_INSTALLER: Create installer packages (NSIS/MSI)
- DEPLOY_QT_PLUGINS: Automatically deploy Qt plugins
- PACKAGING_MINIMAL: Create minimal deployment (exclude unnecessary plugins)
- PACKAGING_STRIP_DEBUG: Remove debug symbols from binaries (reduces size)
- PACKAGING_AGGRESSIVE_CLEANUP: Aggressively remove all development files

#]=======================================================================]
function(setup_packaging_options)
    message(STATUS "Setting up packaging options...")

    option(ENABLE_PACKAGING "Enable packaging support" ON)
    option(PACKAGE_PORTABLE "Create portable ZIP packages" ON)
    option(PACKAGE_INSTALLER "Create installer packages" ON)
    option(DEPLOY_QT_PLUGINS "Automatically deploy Qt plugins" ON)
    option(PACKAGING_MINIMAL "Create minimal deployment (exclude unnecessary plugins)" ON)
    option(PACKAGING_STRIP_DEBUG "Strip debug symbols from binaries" ON)
    option(PACKAGING_AGGRESSIVE_CLEANUP "Aggressively remove all development files" ON)

    message(STATUS "Packaging options configured")
    message(STATUS "  ENABLE_PACKAGING: ${ENABLE_PACKAGING}")
    message(STATUS "  PACKAGE_PORTABLE: ${PACKAGE_PORTABLE}")
    message(STATUS "  PACKAGE_INSTALLER: ${PACKAGE_INSTALLER}")
    message(STATUS "  DEPLOY_QT_PLUGINS: ${DEPLOY_QT_PLUGINS}")
    message(STATUS "  PACKAGING_MINIMAL: ${PACKAGING_MINIMAL}")
    message(STATUS "  PACKAGING_STRIP_DEBUG: ${PACKAGING_STRIP_DEBUG}")
    message(STATUS "  PACKAGING_AGGRESSIVE_CLEANUP: ${PACKAGING_AGGRESSIVE_CLEANUP}")
endfunction()

#[=======================================================================[.rst:
find_qt_deployment_tools
------------------------

Find Qt deployment tools (windeployqt, etc).

.. code-block:: cmake

  find_qt_deployment_tools()

Sets:
- QT_DEPLOY_TOOL: Path to windeployqt or equivalent
- QT_DEPLOY_TOOL_FOUND: Whether deployment tool was found

#]=======================================================================]
function(find_qt_deployment_tools)
    message(STATUS "Searching for Qt deployment tools...")

    # Try to find windeployqt (Qt6 version is windeployqt6)
    find_program(QT_DEPLOY_TOOL
        NAMES windeployqt6 windeployqt6.exe windeployqt windeployqt.exe
        HINTS ${Qt6_DIR}/../../../bin
              ${Qt6_DIR}/../../bin
              ${MSYSTEM_PREFIX}/bin
              $ENV{MSYSTEM_PREFIX}/bin
        NO_DEFAULT_PATH
    )

    if(QT_DEPLOY_TOOL)
        message(STATUS "Found Qt deployment tool: ${QT_DEPLOY_TOOL}")
        set(QT_DEPLOY_TOOL_FOUND TRUE PARENT_SCOPE)
    else()
        message(WARNING "Qt deployment tool (windeployqt) not found. Manual DLL bundling will be used.")
        set(QT_DEPLOY_TOOL_FOUND FALSE PARENT_SCOPE)
    endif()

    set(QT_DEPLOY_TOOL "${QT_DEPLOY_TOOL}" PARENT_SCOPE)
endfunction()

#[=======================================================================[.rst:
get_msys2_runtime_dlls
----------------------

Get list of MSYS2 runtime DLLs that need to be bundled.

.. code-block:: cmake

  get_msys2_runtime_dlls(OUTPUT_VAR)

Sets OUTPUT_VAR to list of runtime DLL paths.

#]=======================================================================]
function(get_msys2_runtime_dlls OUTPUT_VAR)
    set(runtime_dlls "")

    if(MSYS2_DETECTED)
        # Common MSYS2/MinGW runtime libraries
        set(runtime_libs
            "libgcc_s_seh-1.dll"
            "libstdc++-6.dll"
            "libwinpthread-1.dll"
            "libgomp-1.dll"          # OpenMP runtime (if used)
            "libssp-0.dll"           # Stack protector (if used)
        )

        # Determine MSYSTEM_PREFIX if not already set
        if(NOT MSYSTEM_PREFIX)
            if(DEFINED ENV{MSYSTEM_PREFIX})
                set(MSYSTEM_PREFIX "$ENV{MSYSTEM_PREFIX}")
            else()
                # Try to detect from compiler path
                get_filename_component(compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
                get_filename_component(MSYSTEM_PREFIX "${compiler_dir}" DIRECTORY)
            endif()
        endif()

        message(STATUS "Searching for MSYS2 runtime DLLs in: ${MSYSTEM_PREFIX}/bin")

        foreach(lib ${runtime_libs})
            find_file(lib_path_${lib} "${lib}"
                PATHS "${MSYSTEM_PREFIX}/bin"
                NO_DEFAULT_PATH
            )
            if(lib_path_${lib})
                list(APPEND runtime_dlls "${lib_path_${lib}}")
                message(STATUS "  Found: ${lib}")
                unset(lib_path_${lib} CACHE)
            else()
                message(STATUS "  Not found (optional): ${lib}")
            endif()
        endforeach()
    endif()

    set(${OUTPUT_VAR} "${runtime_dlls}" PARENT_SCOPE)
endfunction()

#[=======================================================================[.rst:
get_qt6_plugin_dlls
--------------------

Get list of Qt6 plugin DLLs that need to be bundled.

.. code-block:: cmake

  get_qt6_plugin_dlls(OUTPUT_VAR)

Sets OUTPUT_VAR to list of Qt6 plugin DLL paths.

#]=======================================================================]
function(get_qt6_plugin_dlls OUTPUT_VAR)
    set(plugin_dlls "")

    # Qt6 plugin directories
    set(plugin_dirs
        "platforms"
        "imageformats"
        "iconengines"
        "styles"
        "networkinformation"
    )

    foreach(plugin_dir ${plugin_dirs})
        file(GLOB plugins "${Qt6_DIR}/../../plugins/${plugin_dir}/*.dll")
        list(APPEND plugin_dlls ${plugins})
    endforeach()

    set(${OUTPUT_VAR} "${plugin_dlls}" PARENT_SCOPE)
endfunction()

#[=======================================================================[.rst:
setup_msvc_redistributables
---------------------------

Setup MSVC runtime redistributables for installation.

.. code-block:: cmake

  setup_msvc_redistributables()

Configures installation of Visual C++ runtime libraries when building with MSVC.
Uses CMake's InstallRequiredSystemLibraries module.

#]=======================================================================]
function(setup_msvc_redistributables)
    if(NOT COMPILER_MSVC)
        return()
    endif()

    message(STATUS "Configuring MSVC redistributables...")

    # Include the module that handles MSVC runtime installation
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    include(InstallRequiredSystemLibraries)

    if(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
        install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
            DESTINATION bin
            COMPONENT Runtime
        )
        message(STATUS "MSVC runtime libraries will be installed")
    else()
        message(WARNING "MSVC runtime libraries not found. Users may need to install VC++ redistributables.")
    endif()
endfunction()

#[=======================================================================[.rst:
setup_qt_deployment
-------------------

Setup Qt deployment using qt_generate_deploy_app_script().

.. code-block:: cmake

  setup_qt_deployment(TARGET_NAME)

Generates and installs a deployment script that:
- Deploys Qt libraries and plugins
- Handles platform-specific Qt dependencies
- Creates a self-contained installation
- Cleans up unnecessary development files post-deployment

Requires Qt 6.3 or later.

#]=======================================================================]
function(setup_qt_deployment TARGET_NAME)
    if(NOT ENABLE_PACKAGING)
        return()
    endif()

    # Check if qt_generate_deploy_app_script is available (Qt 6.3+)
    if(NOT COMMAND qt_generate_deploy_app_script)
        message(WARNING "qt_generate_deploy_app_script not available. Qt deployment will be limited.")
        message(WARNING "Please upgrade to Qt 6.3 or later for full deployment support.")
        return()
    endif()

    message(STATUS "Setting up Qt deployment for ${TARGET_NAME}...")

    # Generate deployment script
    qt_generate_deploy_app_script(
        TARGET ${TARGET_NAME}
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )

    # Install the deployment script to run at install time
    install(SCRIPT ${deploy_script} COMPONENT Runtime)

    # Add post-deployment cleanup script to remove unnecessary files
    install(CODE "
        message(STATUS \"Cleaning up unnecessary deployment files...\")

        # Define patterns for files to remove
        set(cleanup_patterns
            \"*.pdb\"           # Debug symbols
            \"*.ilk\"           # Incremental linker files
            \"*.exp\"           # Export files
            \"*.lib\"           # Static libraries (keep only DLLs)
            \"*.a\"             # Static libraries (Unix)
            \"*d.dll\"          # Debug DLLs (Qt debug builds)
            \"*.h\"             # Header files
            \"*.hpp\"           # C++ header files
            \"*.cmake\"         # CMake files
            \"CMakeLists.txt\"  # CMake files
            \"*.pc\"            # pkg-config files
            \"*.pri\"           # Qt project include files
            \"*.prl\"           # Qt precompiled library files
        )

        # Directories to clean up completely
        set(cleanup_dirs
            \"include\"         # Header files
            \"lib/cmake\"       # CMake config files
            \"lib/pkgconfig\"   # pkg-config files
            \"share/doc\"       # Documentation
            \"share/man\"       # Man pages
            \"mkspecs\"         # Qt mkspecs
            \"doc\"             # Documentation
            \"examples\"        # Example files
        )

        # Remove files matching patterns
        foreach(pattern IN LISTS cleanup_patterns)
            file(GLOB_RECURSE files_to_remove
                \"\${CMAKE_INSTALL_PREFIX}/bin/\${pattern}\"
                \"\${CMAKE_INSTALL_PREFIX}/\${pattern}\"
            )
            foreach(file IN LISTS files_to_remove)
                if(EXISTS \"\${file}\")
                    file(REMOVE \"\${file}\")
                    message(STATUS \"  Removed: \${file}\")
                endif()
            endforeach()
        endforeach()

        # Remove directories
        foreach(dir IN LISTS cleanup_dirs)
            set(full_path \"\${CMAKE_INSTALL_PREFIX}/\${dir}\")
            if(EXISTS \"\${full_path}\" AND IS_DIRECTORY \"\${full_path}\")
                file(REMOVE_RECURSE \"\${full_path}\")
                message(STATUS \"  Removed directory: \${full_path}\")
            endif()
        endforeach()

        message(STATUS \"Deployment cleanup completed\")
    " COMPONENT Runtime)

    message(STATUS "Qt deployment script generated: ${deploy_script}")
    message(STATUS "Post-deployment cleanup configured")
endfunction()

#[=======================================================================[.rst:
install_runtime_dependencies
-----------------------------

Install runtime dependencies using CMake's RUNTIME_DEPENDENCIES feature.

.. code-block:: cmake

  install_runtime_dependencies(TARGET_NAME)

Automatically discovers and installs runtime dependencies (DLLs) for the target.
Excludes Windows system DLLs, development files, and includes only essential runtime dependencies.

#]=======================================================================]
function(install_runtime_dependencies TARGET_NAME)
    if(NOT WIN32)
        return()
    endif()

    message(STATUS "Configuring runtime dependency installation for ${TARGET_NAME}...")

    # Define patterns for system DLLs to exclude
    set(system_dll_patterns
        "api-ms-.*"
        "ext-ms-.*"
        "kernel32\\.dll"
        "user32\\.dll"
        "advapi32\\.dll"
        "shell32\\.dll"
        "ole32\\.dll"
        "oleaut32\\.dll"
        "gdi32\\.dll"
        "comdlg32\\.dll"
        "ws2_32\\.dll"
        "msvcp.*\\.dll"
        "vcruntime.*\\.dll"
        "ucrtbase\\.dll"
        ".*windows[\\\\/]system32.*"
        ".*windows[\\\\/]syswow64.*"
        "AzureAttestManager\\.dll"
        "AzureAttestNormal\\.dll"
        "wpaxholder\\.dll"
    )

    # Additional patterns for development and build artifacts to exclude
    list(APPEND system_dll_patterns
        ".*\\.pdb$"              # Debug symbols
        ".*\\.ilk$"              # Incremental linker files
        ".*\\.exp$"              # Export files
        ".*\\.lib$"              # Static libraries
        ".*\\.a$"                # Static libraries (Unix-style)
        ".*d\\.dll$"             # Debug DLLs (Qt and others with 'd' suffix)
        ".*_debug\\.dll$"        # Debug DLLs (alternative naming)
        ".*[\\\\/]include[\\\\/].*"  # Header files
        ".*[\\\\/]cmake[\\\\/].*"    # CMake files
        ".*[\\\\/]pkgconfig[\\\\/].*" # pkg-config files
        ".*[\\\\/]doc[\\\\/].*"      # Documentation
        ".*[\\\\/]examples[\\\\/].*" # Examples
        ".*[\\\\/]tests[\\\\/].*"    # Test files
    )

    # Install runtime dependencies with automatic discovery
    install(TARGETS ${TARGET_NAME}
        RUNTIME_DEPENDENCIES
            PRE_EXCLUDE_REGEXES ${system_dll_patterns}
            POST_EXCLUDE_REGEXES ${system_dll_patterns}
            POST_INCLUDE_REGEXES ".*\\.dll$"  # Only include DLL files
            DIRECTORIES
                ${CMAKE_BINARY_DIR}
                ${CMAKE_BINARY_DIR}/bin
                $<TARGET_FILE_DIR:${TARGET_NAME}>
        COMPONENT Runtime
    )

    message(STATUS "Runtime dependency installation configured with enhanced filtering")
endfunction()

#[=======================================================================[.rst:
cleanup_installed_dependencies
-------------------------------

Remove unnecessary development and build files from installed dependencies.

.. code-block:: cmake

  cleanup_installed_dependencies()

Removes:
- Header files (.h, .hpp)
- Static libraries (.a, .lib)
- CMake configuration files
- pkg-config files
- Documentation and examples
- Debug symbols and build artifacts (if PACKAGING_STRIP_DEBUG is ON)

This function is called automatically during installation to reduce package size.
The cleanup behavior is controlled by PACKAGING_AGGRESSIVE_CLEANUP option.

#]=======================================================================]
function(cleanup_installed_dependencies)
    if(NOT PACKAGING_AGGRESSIVE_CLEANUP)
        message(STATUS "Aggressive cleanup disabled - skipping dependency cleanup")
        return()
    endif()

    message(STATUS "Configuring dependency cleanup for minimal packaging...")

    # Install a cleanup script that runs after all files are installed
    install(CODE "
        message(STATUS \"Performing post-installation cleanup...\")

        # File patterns to remove (development and build artifacts)
        set(file_patterns_to_remove
            # Header files
            \"*.h\"
            \"*.hpp\"
            \"*.hxx\"
            \"*.h++\"
            \"*.hh\"

            # Static libraries
            \"*.a\"
            \"*.lib\"

            # Debug symbols and build artifacts
            \"*.pdb\"
            \"*.ilk\"
            \"*.exp\"
            \"*.map\"

            # CMake files
            \"*.cmake\"
            \"CMakeLists.txt\"
            \"*Config.cmake\"
            \"*ConfigVersion.cmake\"
            \"*Targets.cmake\"

            # pkg-config files
            \"*.pc\"

            # Qt-specific development files
            \"*.pri\"
            \"*.prl\"

            # Import libraries (keep only runtime DLLs)
            \"*.dll.a\"

            # Python bytecode (if any)
            \"*.pyc\"
            \"*.pyo\"

            # Backup and temporary files
            \"*~\"
            \"*.bak\"
            \"*.tmp\"
        )

        # Directories to remove completely
        set(dirs_to_remove
            \"include\"
            \"lib/cmake\"
            \"lib/pkgconfig\"
            \"share/cmake\"
            \"share/pkgconfig\"
            \"share/doc\"
            \"share/man\"
            \"share/info\"
            \"share/examples\"
            \"doc\"
            \"docs\"
            \"examples\"
            \"tests\"
            \"mkspecs\"
            \"qml\"              # QML files (not used in this app)
        )

        # Search and remove files matching patterns
        set(total_removed 0)
        foreach(pattern IN LISTS file_patterns_to_remove)
            file(GLOB_RECURSE files_to_remove
                \"\${CMAKE_INSTALL_PREFIX}/\${pattern}\"
                \"\${CMAKE_INSTALL_PREFIX}/bin/\${pattern}\"
                \"\${CMAKE_INSTALL_PREFIX}/lib/\${pattern}\"
            )
            foreach(file IN LISTS files_to_remove)
                if(EXISTS \"\${file}\" AND NOT IS_DIRECTORY \"\${file}\")
                    file(REMOVE \"\${file}\")
                    math(EXPR total_removed \"\${total_removed} + 1\")
                endif()
            endforeach()
        endforeach()

        # Remove directories
        set(total_dirs_removed 0)
        foreach(dir IN LISTS dirs_to_remove)
            set(full_path \"\${CMAKE_INSTALL_PREFIX}/\${dir}\")
            if(EXISTS \"\${full_path}\" AND IS_DIRECTORY \"\${full_path}\")
                file(REMOVE_RECURSE \"\${full_path}\")
                math(EXPR total_dirs_removed \"\${total_dirs_removed} + 1\")
                message(STATUS \"  Removed directory: \${dir}\")
            endif()

            # Also check in bin directory
            set(bin_path \"\${CMAKE_INSTALL_PREFIX}/bin/\${dir}\")
            if(EXISTS \"\${bin_path}\" AND IS_DIRECTORY \"\${bin_path}\")
                file(REMOVE_RECURSE \"\${bin_path}\")
                math(EXPR total_dirs_removed \"\${total_dirs_removed} + 1\")
                message(STATUS \"  Removed directory: bin/\${dir}\")
            endif()
        endforeach()

        message(STATUS \"Cleanup complete: Removed \${total_removed} files and \${total_dirs_removed} directories\")
    " COMPONENT Runtime)

    message(STATUS "Post-installation cleanup configured")
endfunction()

#[=======================================================================[.rst:
setup_install_targets
---------------------

Setup CMake install targets for application and resources.

.. code-block:: cmake

  setup_install_targets(TARGET_NAME)

Configures install rules for:
- Executable
- Qt libraries and plugins
- Application resources (styles, icons, translations)
- Runtime dependencies
- Post-installation cleanup to remove development files

#]=======================================================================]
function(setup_install_targets TARGET_NAME)
    if(NOT ENABLE_PACKAGING)
        return()
    endif()

    message(STATUS "Setting up install targets for ${TARGET_NAME}...")

    # Install executable
    # On macOS, bundles go to root; on other platforms, to bin directory
    install(TARGETS ${TARGET_NAME}
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .
        COMPONENT Application
    )

    # Install application resources
    message(STATUS "  Installing application resources...")

    # Install stylesheets
    if(EXISTS "${CMAKE_SOURCE_DIR}/assets/styles")
        install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/styles"
            DESTINATION bin
            COMPONENT Resources
            FILES_MATCHING PATTERN "*.qss"
        )
        message(STATUS "    - Stylesheets")
    endif()

    # Install images and icons
    if(EXISTS "${CMAKE_SOURCE_DIR}/assets/images")
        install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/images"
            DESTINATION bin
            COMPONENT Resources
            FILES_MATCHING
                PATTERN "*.svg"
                PATTERN "*.png"
                PATTERN "*.ico"
                PATTERN "*.jpg"
                PATTERN "*.jpeg"
        )
        message(STATUS "    - Images and icons")
    endif()

    # Install translations (compiled .qm files)
    file(GLOB translation_files "${CMAKE_SOURCE_DIR}/app/i18n/*.qm")
    if(translation_files)
        install(FILES ${translation_files}
            DESTINATION bin/translations
            COMPONENT Resources
        )
        message(STATUS "    - Translations (${CMAKE_MATCH_COUNT} files)")
    endif()

    # Install LICENSE file
    if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
        install(FILES "${CMAKE_SOURCE_DIR}/LICENSE"
            DESTINATION .
            COMPONENT Documentation
        )
        message(STATUS "    - License file")
    endif()

    # Install README if it exists
    if(EXISTS "${CMAKE_SOURCE_DIR}/README.md")
        install(FILES "${CMAKE_SOURCE_DIR}/README.md"
            DESTINATION .
            COMPONENT Documentation
        )
        message(STATUS "    - README file")
    endif()

    # Platform-specific runtime dependency installation
    if(WIN32)
        if(MSYS2_DETECTED OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            message(STATUS "  Configuring MSYS2/MinGW runtime dependencies...")

            # Get MSYS2 runtime DLLs
            get_msys2_runtime_dlls(msys2_dlls)
            if(msys2_dlls)
                install(FILES ${msys2_dlls}
                    DESTINATION bin
                    COMPONENT Runtime
                )
                list(LENGTH msys2_dlls dll_count)
                message(STATUS "    - ${dll_count} MSYS2 runtime DLLs")
            endif()

            # Note: RUNTIME_DEPENDENCIES disabled for MSYS2/MinGW due to unresolvable system DLL references
            # Qt deployment script handles Qt dependencies, and MSYS2 DLLs are manually copied above
            # install_runtime_dependencies(${TARGET_NAME})
        else()
            # MSVC build
            message(STATUS "  Configuring MSVC runtime dependencies...")
            setup_msvc_redistributables()

            # Use RUNTIME_DEPENDENCIES for automatic DLL discovery
            install_runtime_dependencies(${TARGET_NAME})
        endif()
    endif()

    # Configure post-installation cleanup to remove development files
    # This significantly reduces package size by removing unnecessary files
    cleanup_installed_dependencies()

    # Configure binary stripping for release builds
    if(PACKAGING_STRIP_DEBUG AND CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo")
        message(STATUS "  Configuring binary stripping for release build...")

        # Strip binaries on installation (removes debug symbols)
        install(CODE "
            message(STATUS \"Stripping debug symbols from binaries...\")

            # Find all executables and DLLs
            file(GLOB_RECURSE binaries
                \"\${CMAKE_INSTALL_PREFIX}/bin/*.exe\"
                \"\${CMAKE_INSTALL_PREFIX}/bin/*.dll\"
            )

            # Determine strip command based on platform
            if(WIN32)
                # Try to find strip command (MinGW/MSYS2)
                find_program(STRIP_COMMAND strip)
                if(STRIP_COMMAND)
                    foreach(binary IN LISTS binaries)
                        execute_process(
                            COMMAND \${STRIP_COMMAND} --strip-debug \"\${binary}\"
                            OUTPUT_QUIET
                            ERROR_QUIET
                        )
                    endforeach()
                    message(STATUS \"  Stripped debug symbols from \${CMAKE_INSTALL_PREFIX}/bin\")
                else()
                    message(STATUS \"  Strip command not found - skipping binary stripping\")
                endif()
            else()
                # Unix-like systems
                find_program(STRIP_COMMAND strip)
                if(STRIP_COMMAND)
                    foreach(binary IN LISTS binaries)
                        execute_process(
                            COMMAND \${STRIP_COMMAND} --strip-debug \"\${binary}\"
                            OUTPUT_QUIET
                            ERROR_QUIET
                        )
                    endforeach()
                    message(STATUS \"  Stripped debug symbols\")
                endif()
            endif()
        " COMPONENT Runtime)
    endif()

    message(STATUS "Install targets configured successfully")
endfunction()

#[=======================================================================[.rst:
normalize_architecture_name
---------------------------

Normalize architecture names for consistent package naming.

.. code-block:: cmake

  normalize_architecture_name(OUTPUT_VAR)

Converts CMAKE_SYSTEM_PROCESSOR to standardized architecture names:
- AMD64, x86_64 -> x64
- aarch64, arm64 -> arm64
- i686, i386 -> x86

Sets OUTPUT_VAR to the normalized architecture name.

#]=======================================================================]
function(normalize_architecture_name OUTPUT_VAR)
    set(arch "${CMAKE_SYSTEM_PROCESSOR}")

    # Normalize to standard names
    if(arch MATCHES "^(AMD64|x86_64)$")
        set(normalized_arch "x64")
    elseif(arch MATCHES "^(aarch64|arm64|ARM64)$")
        set(normalized_arch "arm64")
    elseif(arch MATCHES "^(i686|i386|x86)$")
        set(normalized_arch "x86")
    else()
        # Keep original if not recognized
        set(normalized_arch "${arch}")
    endif()

    set(${OUTPUT_VAR} "${normalized_arch}" PARENT_SCOPE)
    message(STATUS "Architecture: ${arch} -> ${normalized_arch}")
endfunction()

#[=======================================================================[.rst:
setup_cpack_configuration
--------------------------

Setup CPack for creating distribution packages.

.. code-block:: cmake

  setup_cpack_configuration()

Configures CPack generators based on platform:
- Windows: NSIS (for MSYS2/MinGW), WiX MSI (for MSVC), ZIP (both)
- Linux: DEB, RPM, TGZ
- macOS: DragNDrop, Bundle

Package Output:
- All packages are output to ${CMAKE_SOURCE_DIR}/package/
- Package naming: {AppName}-{Version}-{OS}-{Arch}-{BuildType}.{ext}

#]=======================================================================]
function(setup_cpack_configuration)
    if(NOT ENABLE_PACKAGING)
        return()
    endif()

    message(STATUS "Setting up CPack configuration...")

    # Basic package metadata
    set(CPACK_PACKAGE_NAME "SASTReadium" PARENT_SCOPE)
    set(CPACK_PACKAGE_VENDOR "SAST Team" PARENT_SCOPE)
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}" PARENT_SCOPE)
    set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}" PARENT_SCOPE)
    set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}" PARENT_SCOPE)
    set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}" PARENT_SCOPE)
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Qt6-based PDF reader with advanced search capabilities" PARENT_SCOPE)
    set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md" PARENT_SCOPE)
    set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/NJUPT-SAST/sast-readium" PARENT_SCOPE)
    set(CPACK_PACKAGE_CONTACT "SAST Team <sast@njupt.edu.cn>" PARENT_SCOPE)

    # Installation directory
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "SAST Readium" PARENT_SCOPE)

    # License and documentation
    if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
        set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE" PARENT_SCOPE)
    endif()
    if(EXISTS "${CMAKE_SOURCE_DIR}/README.md")
        set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md" PARENT_SCOPE)
    endif()

    # Component configuration
    set(CPACK_COMPONENTS_ALL Application Runtime Resources Documentation PARENT_SCOPE)
    set(CPACK_COMPONENT_APPLICATION_DISPLAY_NAME "SAST Readium Application" PARENT_SCOPE)
    set(CPACK_COMPONENT_APPLICATION_DESCRIPTION "Main application executable" PARENT_SCOPE)
    set(CPACK_COMPONENT_APPLICATION_REQUIRED TRUE PARENT_SCOPE)

    set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "Runtime Libraries" PARENT_SCOPE)
    set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Required runtime libraries and dependencies" PARENT_SCOPE)
    set(CPACK_COMPONENT_RUNTIME_REQUIRED TRUE PARENT_SCOPE)

    set(CPACK_COMPONENT_RESOURCES_DISPLAY_NAME "Application Resources" PARENT_SCOPE)
    set(CPACK_COMPONENT_RESOURCES_DESCRIPTION "Stylesheets, icons, and translations" PARENT_SCOPE)
    set(CPACK_COMPONENT_RESOURCES_REQUIRED TRUE PARENT_SCOPE)

    set(CPACK_COMPONENT_DOCUMENTATION_DISPLAY_NAME "Documentation" PARENT_SCOPE)
    set(CPACK_COMPONENT_DOCUMENTATION_DESCRIPTION "License and README files" PARENT_SCOPE)
    set(CPACK_COMPONENT_DOCUMENTATION_REQUIRED FALSE PARENT_SCOPE)

    # Platform-specific configuration
    if(WIN32)
        message(STATUS "  Configuring Windows packaging...")

        # Application icon
        if(EXISTS "${CMAKE_SOURCE_DIR}/assets/images/icon.ico")
            set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/assets/images/icon.ico" PARENT_SCOPE)
        endif()

        if(MSYS2_DETECTED)
            message(STATUS "    - MSYS2/MinGW build: NSIS + ZIP")

            # Generators for MSYS2 builds
            if(PACKAGE_INSTALLER)
                set(CPACK_GENERATOR "NSIS" PARENT_SCOPE)
            endif()
            if(PACKAGE_PORTABLE)
                list(APPEND CPACK_GENERATOR "ZIP")
                set(CPACK_GENERATOR ${CPACK_GENERATOR} PARENT_SCOPE)
            endif()

            # NSIS-specific configuration
            set(CPACK_NSIS_DISPLAY_NAME "SAST Readium" PARENT_SCOPE)
            set(CPACK_NSIS_PACKAGE_NAME "SAST Readium" PARENT_SCOPE)
            set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64" PARENT_SCOPE)
            set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON PARENT_SCOPE)
            set(CPACK_NSIS_MODIFY_PATH OFF PARENT_SCOPE)
            set(CPACK_NSIS_MUI_FINISHPAGE_RUN "bin\\\\app.exe" PARENT_SCOPE)

            # URLs and contact information
            set(CPACK_NSIS_HELP_LINK "https://github.com/NJUPT-SAST/sast-readium" PARENT_SCOPE)
            set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/NJUPT-SAST/sast-readium" PARENT_SCOPE)
            set(CPACK_NSIS_CONTACT "SAST Team <sast@njupt.edu.cn>" PARENT_SCOPE)

            # Compression
            set(CPACK_NSIS_COMPRESSOR "/SOLID lzma" PARENT_SCOPE)

            # Create desktop and start menu shortcuts
            set(CPACK_NSIS_CREATE_ICONS_EXTRA "
                CreateShortCut '$DESKTOP\\\\SAST Readium.lnk' '$INSTDIR\\\\bin\\\\app.exe'
                CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\SAST Readium.lnk' '$INSTDIR\\\\bin\\\\app.exe'
            " PARENT_SCOPE)
            set(CPACK_NSIS_DELETE_ICONS_EXTRA "
                Delete '$DESKTOP\\\\SAST Readium.lnk'
                Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\SAST Readium.lnk'
            " PARENT_SCOPE)

            # File associations (optional - for PDF files)
            set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
                WriteRegStr HKCR '.pdf\\\\OpenWithProgids' 'SASTReadium.pdf' ''
                WriteRegStr HKCR 'SASTReadium.pdf' '' 'PDF Document'
                WriteRegStr HKCR 'SASTReadium.pdf\\\\shell\\\\open\\\\command' '' '$INSTDIR\\\\bin\\\\app.exe \\\"%1\\\"'
            " PARENT_SCOPE)
            set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
                DeleteRegKey HKCR 'SASTReadium.pdf'
                DeleteRegValue HKCR '.pdf\\\\OpenWithProgids' 'SASTReadium.pdf'
            " PARENT_SCOPE)

            if(EXISTS "${CMAKE_SOURCE_DIR}/assets/images/icon.ico")
                set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/assets/images/icon.ico" PARENT_SCOPE)
                set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/assets/images/icon.ico" PARENT_SCOPE)
            endif()

        else()
            message(STATUS "    - MSVC build: WiX MSI + ZIP")

            # Generators for MSVC builds
            set(generators "")
            if(PACKAGE_INSTALLER)
                list(APPEND generators "WIX")
            endif()
            if(PACKAGE_PORTABLE)
                list(APPEND generators "ZIP")
            endif()
            set(CPACK_GENERATOR ${generators} PARENT_SCOPE)

            # WiX-specific configuration
            set(CPACK_WIX_UPGRADE_GUID "8B3F9C5E-7D2A-4F1B-9E3C-6A8D4B2E1F0A" PARENT_SCOPE)
            set(CPACK_WIX_PRODUCT_GUID "*" PARENT_SCOPE)  # Generate new GUID for each version

            # Product information
            set(CPACK_WIX_PRODUCT_NAME "SAST Readium" PARENT_SCOPE)
            set(CPACK_WIX_MANUFACTURER "SAST Team" PARENT_SCOPE)

            # Icons and branding
            if(EXISTS "${CMAKE_SOURCE_DIR}/assets/images/icon.ico")
                set(CPACK_WIX_PRODUCT_ICON "${CMAKE_SOURCE_DIR}/assets/images/icon.ico" PARENT_SCOPE)
            endif()

            # Add/Remove Programs properties
            set(CPACK_WIX_PROPERTY_ARPCOMMENTS "Qt6-based PDF reader with advanced search capabilities" PARENT_SCOPE)
            set(CPACK_WIX_PROPERTY_ARPHELPLINK "https://github.com/NJUPT-SAST/sast-readium" PARENT_SCOPE)
            set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "https://github.com/NJUPT-SAST/sast-readium" PARENT_SCOPE)
            set(CPACK_WIX_PROPERTY_ARPCONTACT "SAST Team <sast@njupt.edu.cn>" PARENT_SCOPE)

            # License agreement
            if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
                set(CPACK_WIX_LICENSE_RTF "${CMAKE_SOURCE_DIR}/LICENSE" PARENT_SCOPE)
            endif()

            # Program menu folder
            set(CPACK_WIX_PROGRAM_MENU_FOLDER "SAST Readium" PARENT_SCOPE)

            # Create desktop shortcut
            set(CPACK_WIX_PROPERTY_ARPSYSTEMCOMPONENT "0" PARENT_SCOPE)

            # File associations for PDF files (optional - can be enabled by user)
            # Note: This is commented out by default to avoid conflicts with existing PDF readers
            # set(CPACK_WIX_EXTENSIONS "WixUtilExtension" PARENT_SCOPE)

            message(STATUS "    - WiX MSI configuration completed")
        endif()

        # ZIP archive configuration (common for both)
        set(CPACK_ARCHIVE_COMPONENT_INSTALL ON PARENT_SCOPE)

    elseif(APPLE)
        message(STATUS "  Configuring macOS packaging...")
        set(CPACK_GENERATOR "DragNDrop" "TGZ" PARENT_SCOPE)
        set(CPACK_DMG_VOLUME_NAME "SAST Readium" PARENT_SCOPE)
        set(CPACK_DMG_FORMAT "UDZO" PARENT_SCOPE)

    else()
        message(STATUS "  Configuring Linux packaging...")
        set(CPACK_GENERATOR "DEB" "RPM" "TGZ" PARENT_SCOPE)

        # Debian-specific
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "SAST Team <sast@njupt.edu.cn>" PARENT_SCOPE)
        set(CPACK_DEBIAN_PACKAGE_SECTION "utils" PARENT_SCOPE)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libqt6core6, libqt6gui6, libqt6widgets6" PARENT_SCOPE)

        # RPM-specific
        set(CPACK_RPM_PACKAGE_LICENSE "MIT" PARENT_SCOPE)
        set(CPACK_RPM_PACKAGE_GROUP "Applications/Productivity" PARENT_SCOPE)
    endif()

    # Normalize architecture name for consistent package naming
    normalize_architecture_name(NORMALIZED_ARCH)

    # Determine build type for package naming
    if(NOT CMAKE_BUILD_TYPE)
        set(BUILD_TYPE_NAME "Unknown")
    else()
        set(BUILD_TYPE_NAME "${CMAKE_BUILD_TYPE}")
    endif()

    # Set package output directory to repository root/package/
    set(CPACK_PACKAGE_DIRECTORY "${CMAKE_SOURCE_DIR}/package" PARENT_SCOPE)

    # Output file naming: AppName-Version-OS-Arch-BuildType
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${NORMALIZED_ARCH}-${BUILD_TYPE_NAME}" PARENT_SCOPE)

    message(STATUS "CPack configuration completed")
    message(STATUS "  Package name: SAST Readium")
    message(STATUS "  Version: ${PROJECT_VERSION}")
    message(STATUS "  Package output directory: ${CMAKE_SOURCE_DIR}/package")
    message(STATUS "  Package file name pattern: ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${NORMALIZED_ARCH}-${BUILD_TYPE_NAME}")

    # Note: CPack must be included at the top level, not in a function
    # The calling CMakeLists.txt should include CPack after calling this function
endfunction()

#[=======================================================================[.rst:
add_deploy_qt_command
---------------------

Add post-build command to deploy Qt plugins and libraries.

.. code-block:: cmake

  add_deploy_qt_command(TARGET_NAME)

Creates a post-build command that:
- Runs windeployqt if available with optimized flags
- Falls back to manual DLL copying if needed
- Deploys plugins to correct directories
- Excludes unnecessary plugins when PACKAGING_MINIMAL is ON

#]=======================================================================]
function(add_deploy_qt_command TARGET_NAME)
    if(NOT DEPLOY_QT_PLUGINS OR NOT WIN32)
        return()
    endif()

    message(STATUS "Adding Qt deployment command for ${TARGET_NAME}...")

    if(QT_DEPLOY_TOOL_FOUND)
        # Build windeployqt command with optimized flags
        set(DEPLOY_COMMAND ${QT_DEPLOY_TOOL})

        # Basic flags (always applied)
        list(APPEND DEPLOY_COMMAND
            --no-translations          # We handle translations separately
            --no-compiler-runtime      # We handle runtime separately
            --no-system-d3d-compiler   # Not needed for this application
            --no-opengl-sw             # Not using software OpenGL
        )

        # Minimal deployment flags (exclude unnecessary plugins and files)
        if(PACKAGING_MINIMAL)
            message(STATUS "  Using minimal deployment configuration")
            list(APPEND DEPLOY_COMMAND
                # Skip entire plugin categories not needed by the application
                --skip-plugin-types qmltooling,generic,networkinformation,position,sensors,webview

                # Exclude specific plugins
                --exclude-plugins qtuiotouchplugin,qtvirtualkeyboardplugin

                # Don't deploy unnecessary Qt modules
                --no-quick-import          # No QML/Quick
                --no-virtualkeyboard       # No virtual keyboard
            )
            # Note: --no-webkit2 and --no-angle flags were removed as they are not supported in Qt 6.10+
        endif()

        # Target executable
        list(APPEND DEPLOY_COMMAND $<TARGET_FILE:${TARGET_NAME}>)

        # Use windeployqt for deployment
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${DEPLOY_COMMAND}
            COMMENT "Deploying Qt libraries and plugins (minimal=${PACKAGING_MINIMAL})..."
        )

        message(STATUS "  windeployqt configured with optimized flags")
    else()
        # Fallback: Manual DLL deployment
        message(WARNING "windeployqt not found - Qt DLLs will NOT be automatically deployed!")
        message(WARNING "You may need to manually copy Qt DLLs or run windeployqt manually.")
        message(WARNING "The application may not run without Qt DLLs in the same directory.")

        # Note: We intentionally do NOT copy the entire bin directory here because:
        # 1. It can contain thousands of files (especially in MSYS2)
        # 2. It may fail due to permissions or file locks
        # 3. It's inefficient and wastes disk space
        #
        # Instead, users should either:
        # - Ensure windeployqt is in PATH
        # - Run windeployqt manually after build
        # - Use a proper Qt installation with windeployqt
    endif()
endfunction()
