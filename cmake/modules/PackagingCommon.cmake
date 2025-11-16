# PackagingCommon.cmake - Common packaging utilities for cross-platform deployment

# Generate checksum files for packages
function(generate_package_checksums package_dir)
    if(NOT EXISTS "${package_dir}")
        return()
    endif()

    message(VERBOSE "Generating package checksums...")

    file(GLOB packages "${package_dir}/*")
    foreach(package ${packages})
        if(IS_DIRECTORY "${package}")
            continue()
        endif()

        get_filename_component(package_name "${package}" NAME)

        # Generate SHA256 checksum
        file(SHA256 "${package}" checksum)
        file(WRITE "${package}.sha256" "${checksum}  ${package_name}\n")

        message(VERBOSE "  Generated SHA256 for ${package_name}")
    endforeach()
endfunction()

# Create portable ZIP package with proper structure
function(create_portable_package target_name)
    set(portable_dir "${CMAKE_BINARY_DIR}/portable")
    set(portable_root "${portable_dir}/SAST-Readium-${PROJECT_VERSION}")

    # Create portable directory structure
    file(MAKE_DIRECTORY "${portable_root}")
    file(MAKE_DIRECTORY "${portable_root}/bin")
    file(MAKE_DIRECTORY "${portable_root}/styles")
    file(MAKE_DIRECTORY "${portable_root}/translations")
    file(MAKE_DIRECTORY "${portable_root}/docs")

    # Copy executable
    install(TARGETS ${target_name}
        RUNTIME DESTINATION "${portable_root}/bin"
        COMPONENT Portable
    )

    # Copy resources
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/styles/"
        DESTINATION "${portable_root}/styles"
        COMPONENT Portable
    )

    # Copy documentation
    if(EXISTS "${CMAKE_SOURCE_DIR}/README.md")
        install(FILES "${CMAKE_SOURCE_DIR}/README.md"
            DESTINATION "${portable_root}/docs"
            COMPONENT Portable
        )
    endif()
    if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
        install(FILES "${CMAKE_SOURCE_DIR}/LICENSE"
            DESTINATION "${portable_root}/docs"
            COMPONENT Portable
        )
    endif()

    # Create portable marker file
    file(WRITE "${portable_root}/PORTABLE_MODE.txt"
        "This is a portable installation of SAST Readium.\nNo installation required - run from this directory.\n")

    message(VERBOSE "Portable package structure created")
endfunction()

# Platform-specific package metadata
function(set_platform_package_metadata)
    if(WIN32)
        set(CPACK_SYSTEM_NAME "Windows" PARENT_SCOPE)
    elseif(APPLE)
        set(CPACK_SYSTEM_NAME "macOS" PARENT_SCOPE)
    elseif(UNIX)
        # Detect Linux distribution
        if(EXISTS "/etc/os-release")
            file(STRINGS "/etc/os-release" OS_RELEASE)
            foreach(line ${OS_RELEASE})
                if(line MATCHES "^ID=(.+)")
                    set(LINUX_DISTRO "${CMAKE_MATCH_1}" PARENT_SCOPE)
                    break()
                endif()
            endforeach()
        endif()
        set(CPACK_SYSTEM_NAME "Linux" PARENT_SCOPE)
    endif()
endfunction()

# AppImage support for Linux
function(setup_appimage_support target_name)
    if(NOT UNIX OR APPLE)
        return()
    endif()

    find_program(LINUXDEPLOY_TOOL linuxdeploy)
    if(NOT LINUXDEPLOY_TOOL)
        message(VERBOSE "linuxdeploy not found - AppImage generation disabled")
        return()
    endif()

    message(VERBOSE "AppImage support enabled")

    # Create AppDir structure
    set(APPDIR "${CMAKE_BINARY_DIR}/AppDir")

    install(CODE "
        set(APPDIR \"${APPDIR}\")
        file(MAKE_DIRECTORY \"\${APPDIR}/usr/bin\")
        file(MAKE_DIRECTORY \"\${APPDIR}/usr/share/applications\")
        file(MAKE_DIRECTORY \"\${APPDIR}/usr/share/icons/hicolor/256x256/apps\")

        # Copy executable
        file(INSTALL \"${CMAKE_INSTALL_PREFIX}/bin/${target_name}\"
             DESTINATION \"\${APPDIR}/usr/bin\")

        # Create desktop file
        file(WRITE \"\${APPDIR}/usr/share/applications/sast-readium.desktop\"
\"[Desktop Entry]
Type=Application
Name=SAST Readium
Comment=Qt6-based PDF reader
Exec=${target_name}
Icon=sast-readium
Categories=Office;Viewer;
Terminal=false
\")

        # Copy icon if exists
        if(EXISTS \"${CMAKE_SOURCE_DIR}/assets/images/icon.png\")
            file(COPY \"${CMAKE_SOURCE_DIR}/assets/images/icon.png\"
                 DESTINATION \"\${APPDIR}/usr/share/icons/hicolor/256x256/apps/sast-readium.png\")
        endif()

        message(STATUS \"AppDir structure created at \${APPDIR}\")
    ")
endfunction()

# Code signing support
function(setup_code_signing target_name)
    if(WIN32 AND DEFINED ENV{SIGNTOOL_PATH})
        # Windows code signing with signtool
        set(SIGNTOOL "$ENV{SIGNTOOL_PATH}")
        if(EXISTS "${SIGNTOOL}")
            install(CODE "
                execute_process(
                    COMMAND \"${SIGNTOOL}\" sign /tr http://timestamp.digicert.com /td sha256 /fd sha256
                            \"\${CMAKE_INSTALL_PREFIX}/bin/${target_name}.exe\"
                    RESULT_VARIABLE sign_result
                )
                if(sign_result EQUAL 0)
                    message(STATUS \"Executable signed successfully\")
                else()
                    message(WARNING \"Code signing failed\")
                endif()
            ")
        endif()
    elseif(APPLE AND DEFINED ENV{CODESIGN_IDENTITY})
        # macOS code signing
        set(CODESIGN_ID "$ENV{CODESIGN_IDENTITY}")
        install(CODE "
            execute_process(
                COMMAND codesign --force --deep --sign \"${CODESIGN_ID}\"
                        \"\${CMAKE_INSTALL_PREFIX}/${target_name}.app\"
                RESULT_VARIABLE sign_result
            )
            if(sign_result EQUAL 0)
                message(STATUS \"Application signed successfully\")
            else()
                message(WARNING \"Code signing failed\")
            endif()
        ")
    endif()
endfunction()
