# CreateAppImage.cmake - AppImage creation script for Linux

message(STATUS "Creating AppImage package...")

# Check for linuxdeploy
find_program(LINUXDEPLOY_TOOL linuxdeploy)
if(NOT LINUXDEPLOY_TOOL)
    message(WARNING "linuxdeploy not found - skipping AppImage creation")
    message(WARNING "Install from: https://github.com/linuxdeploy/linuxdeploy/releases")
    return()
endif()

# Check for linuxdeploy-plugin-qt
find_program(LINUXDEPLOY_QT_PLUGIN linuxdeploy-plugin-qt)
if(NOT LINUXDEPLOY_QT_PLUGIN)
    message(WARNING "linuxdeploy-plugin-qt not found - Qt deployment may be incomplete")
endif()

set(APPDIR "${CPACK_TOPLEVEL_DIRECTORY}/AppDir")
set(OUTPUT_DIR "${CPACK_PACKAGE_DIRECTORY}")

# Clean AppDir if exists
if(EXISTS "${APPDIR}")
    file(REMOVE_RECURSE "${APPDIR}")
endif()

# Create AppDir structure
file(MAKE_DIRECTORY "${APPDIR}/usr/bin")
file(MAKE_DIRECTORY "${APPDIR}/usr/lib")
file(MAKE_DIRECTORY "${APPDIR}/usr/share/applications")
file(MAKE_DIRECTORY "${APPDIR}/usr/share/icons/hicolor/256x256/apps")
file(MAKE_DIRECTORY "${APPDIR}/usr/share/metainfo")

# Copy executable
file(INSTALL "${CPACK_TOPLEVEL_DIRECTORY}/_CPack_Packages/${CPACK_TOPLEVEL_TAG}/TGZ/${CPACK_PACKAGE_FILE_NAME}/bin/sast-readium"
     DESTINATION "${APPDIR}/usr/bin"
     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

# Create desktop file
file(WRITE "${APPDIR}/usr/share/applications/sast-readium.desktop"
"[Desktop Entry]
Type=Application
Name=SAST Readium
GenericName=PDF Reader
Comment=Qt6-based PDF reader with advanced search capabilities
Exec=sast-readium %F
Icon=sast-readium
Categories=Office;Viewer;Qt;
MimeType=application/pdf;
Terminal=false
StartupNotify=true
Keywords=PDF;Reader;Viewer;Document;
")

# Create AppStream metadata
file(WRITE "${APPDIR}/usr/share/metainfo/sast-readium.appdata.xml"
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<component type=\"desktop-application\">
  <id>io.github.njupt_sast.readium</id>
  <name>SAST Readium</name>
  <summary>Advanced PDF reader</summary>
  <metadata_license>MIT</metadata_license>
  <project_license>MIT</project_license>
  <description>
    <p>SAST Readium is a Qt6-based PDF reader with advanced search capabilities.</p>
  </description>
  <launchable type=\"desktop-id\">sast-readium.desktop</launchable>
  <provides>
    <binary>sast-readium</binary>
  </provides>
  <releases>
    <release version=\"${CPACK_PACKAGE_VERSION}\" date=\"${CPACK_PACKAGE_VERSION_MAJOR}-${CPACK_PACKAGE_VERSION_MINOR}-${CPACK_PACKAGE_VERSION_PATCH}\"/>
  </releases>
</component>
")

# Copy icon if exists
if(EXISTS "${CPACK_SOURCE_DIR}/assets/images/icon.png")
    file(COPY "${CPACK_SOURCE_DIR}/assets/images/icon.png"
         DESTINATION "${APPDIR}/usr/share/icons/hicolor/256x256/apps/")
    file(RENAME "${APPDIR}/usr/share/icons/hicolor/256x256/apps/icon.png"
                "${APPDIR}/usr/share/icons/hicolor/256x256/apps/sast-readium.png")
endif()

# Set environment for linuxdeploy
set(ENV{OUTPUT} "${OUTPUT_DIR}/SAST-Readium-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_PROCESSOR}.AppImage")
set(ENV{ARCH} "${CMAKE_SYSTEM_PROCESSOR}")

# Run linuxdeploy
message(STATUS "Running linuxdeploy...")
execute_process(
    COMMAND "${LINUXDEPLOY_TOOL}"
            --appdir "${APPDIR}"
            --output appimage
            --desktop-file "${APPDIR}/usr/share/applications/sast-readium.desktop"
            --icon-file "${APPDIR}/usr/share/icons/hicolor/256x256/apps/sast-readium.png"
            ${LINUXDEPLOY_QT_PLUGIN_ARG}
    WORKING_DIRECTORY "${OUTPUT_DIR}"
    RESULT_VARIABLE LINUXDEPLOY_RESULT
    OUTPUT_VARIABLE LINUXDEPLOY_OUTPUT
    ERROR_VARIABLE LINUXDEPLOY_ERROR
)

if(LINUXDEPLOY_RESULT EQUAL 0)
    message(STATUS "AppImage created successfully")
else()
    message(WARNING "AppImage creation failed")
    message(STATUS "Output: ${LINUXDEPLOY_OUTPUT}")
    message(STATUS "Error: ${LINUXDEPLOY_ERROR}")
endif()
