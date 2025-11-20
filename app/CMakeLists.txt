project(app LANGUAGES CXX VERSION 0.1.0.0 DESCRIPTION "SAST Readium")

if(APPLE)
    set(SAST_READIUM_MACOS_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")
    configure_file("${CMAKE_SOURCE_DIR}/distrib/Info.plist.in" "${SAST_READIUM_MACOS_PLIST}" @ONLY)
endif()

# Include utility modules
include(TargetUtils)

# Configure header generation
configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# Collect UI files for translations
file(GLOB UIs "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")

# Discover application sources automatically
message(VERBOSE "Discovering application sources...")
discover_app_sources(APP_SOURCES)

# Build resources as a dedicated STATIC library to avoid MSVC OBJECT linking
# issues. Dead-stripping is prevented by explicitly calling the generated
# qInitResources_* symbols via SastResources::ensureInitialized() in tests/app.
add_library(app_resources STATIC app.qrc ela_ui.qrc)
set_target_properties(app_resources PROPERTIES
    AUTORCC ON
)
target_link_libraries(app_resources PUBLIC Qt6::Core)

# Add platform-specific sources
if(WIN32)
    list(APPEND APP_SOURCES app.rc)
endif()

# Validate that critical sources were found
validate_discovered_sources("${APP_SOURCES}"
    REQUIRED_FILES "main.cpp" "MainWindow.cpp"
    REQUIRED_PATTERNS ".*Logger\\.cpp$" ".*SearchEngine\\.cpp$"
)

# Verify source discovery worked correctly
list(LENGTH APP_SOURCES source_count)
message(VERBOSE "Discovered ${source_count} application sources")

# Separate main.cpp from other sources for library creation
list(REMOVE_ITEM APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

# Include all UI components in app_lib to satisfy cross-module references
# If you want to exclude legacy executables, use BUILD_LEGACY_UI to control only the extra exe, not sources.

# Create application library (without main.cpp) for testing
add_library(app_lib STATIC ${APP_SOURCES})
target_include_directories(app_lib PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(SAST_SPDLOG_INCLUDE_DIR AND EXISTS "${SAST_SPDLOG_INCLUDE_DIR}/spdlog/spdlog.h")
    target_include_directories(app_lib PUBLIC "${SAST_SPDLOG_INCLUDE_DIR}")
endif()

# Get common libraries and link them to app_lib with proper visibility
get_common_libraries(common_libs)
target_link_libraries(app_lib
    PUBLIC
        # Public dependencies that consumers of app_lib need
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        ElaWidgetTools  # ElaWidgetTools library for modern UI components
        app_resources   # Ensure qrc object code is linked for dependents
    PRIVATE
        # Private dependencies that only app_lib needs internally
        Qt6::OpenGLWidgets
        Qt6::Svg
        Qt6::Concurrent
        Qt6::Core5Compat
        Qt6::PrintSupport
        Qt6::TextToSpeech  # Required for AccessibilityModel TTS support
        PkgConfig::POPPLER_QT6
        $<$<PLATFORM_ID:Windows>:dbghelp>
        $<$<PLATFORM_ID:Windows>:psapi>
        $<$<NOT:$<PLATFORM_ID:Windows>>:${CMAKE_DL_LIBS}>
)

# Ensure spdlog usage requirements (e.g., SPDLOG_COMPILED_LIB) propagate to
# consumers like the main executable so that all translation units use the
# compiled-library mode consistently instead of mixing header-only and
# compiled modes, which can cause multiple-definition link errors.
target_link_libraries(app_lib PUBLIC spdlog::spdlog)

# Set C++ standard for library
set_target_properties(app_lib PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Configure precompiled headers for faster builds
# IMPORTANT: Windows compatibility header must come BEFORE Qt headers to avoid type conflicts
# Include frequently used Qt and STL headers
target_precompile_headers(app_lib PRIVATE
    # Platform-specific headers (must be first - has internal guards)
    "platform/WindowsCompat.h"
    # Qt headers
    <QWidget>
    <QString>
    <QList>
    <QVector>
    <QMap>
    <QHash>
    <QDebug>
    # STL headers
    <memory>
    <vector>
    <string>
    <algorithm>
    # Project headers
    "logging/Logger.h"
)

# ==============================================================================
# Main Executable (ElaWidgetTools UI)
# ==============================================================================

message(VERBOSE "Building main executable: sast-readium")

# Create executable with modern target properties
add_executable(sast-readium "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

# Set target properties using modern CMake approach
set_target_properties(sast-readium PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    # Use generator expressions for platform-specific settings
    WIN32_EXECUTABLE $<$<PLATFORM_ID:Windows>:FALSE>  # Temporarily disable for debugging
    MACOSX_BUNDLE $<$<PLATFORM_ID:Darwin>:TRUE>
    # Output name and version
    OUTPUT_NAME "sast-readium"
    VERSION ${PROJECT_VERSION}
)

if(APPLE)
    # Use a custom Info.plist so that the macOS bundle has proper metadata and
    # declares PDF document support.
    set_target_properties(sast-readium PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${SAST_READIUM_MACOS_PLIST}"
    )
endif()

# Link the executable to the library
target_link_libraries(sast-readium PRIVATE
    app_lib
    ElaWidgetTools  # ElaWidgetTools library
)

# Add generated config header and app source directory to include path
target_include_directories(sast-readium PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add translations
qt_add_translations(
    sast-readium
    SOURCES ${UIs}
    TS_FILE_DIR ${CMAKE_SOURCE_DIR}/app/i18n
    TS_FILE_BASE app
)

# Copy assets to output directory
add_custom_command(TARGET sast-readium POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets/styles
    $<TARGET_FILE_DIR:sast-readium>/styles
    COMMENT "Copying application assets"
)

# Add Qt deployment command for build directory (for development/testing)
add_deploy_qt_command(sast-readium)

# Setup install targets for packaging
setup_install_targets(sast-readium)

# Setup Qt deployment for installation (generates deployment script)
setup_qt_deployment(sast-readium)

if(APPLE AND ENABLE_PACKAGING)
    setup_code_signing(sast-readium)
endif()

message(NOTICE "Main executable configured: sast-readium")

# ==============================================================================
# Legacy Executable (Optional)
# ==============================================================================

# Create legacy executable only if BUILD_LEGACY_UI is enabled
if(BUILD_LEGACY_UI)
    message(VERBOSE "Building legacy UI executable: app-legacy")

    # Create legacy executable with modern target properties
    add_executable(app-legacy "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

    # Set target properties using modern CMake approach
    set_target_properties(app-legacy PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        # Use generator expressions for platform-specific settings
        WIN32_EXECUTABLE $<$<PLATFORM_ID:Windows>:FALSE>  # Temporarily disable for debugging
        MACOSX_BUNDLE $<$<PLATFORM_ID:Darwin>:TRUE>
        # Output name and version
        OUTPUT_NAME "app-legacy"
        VERSION ${PROJECT_VERSION}
    )

    # Link the executable to the library
    target_link_libraries(app-legacy PRIVATE app_lib)

    # Add generated config header and app source directory to include path
    target_include_directories(app-legacy PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # Add translations
    qt_add_translations(
        app-legacy
        SOURCES ${UIs}
        TS_FILE_DIR ${CMAKE_SOURCE_DIR}/app/i18n
        TS_FILE_BASE app
    )

    # Copy assets to output directory
    add_custom_command(TARGET app-legacy POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/styles
        $<TARGET_FILE_DIR:app-legacy>/styles
        COMMENT "Copying application assets for legacy UI"
    )

    # Add Qt deployment command for build directory (for development/testing)
    add_deploy_qt_command(app-legacy)

    # Setup install targets for packaging
    setup_install_targets(app-legacy)

    # Setup Qt deployment for installation (generates deployment script)
    setup_qt_deployment(app-legacy)

    message(VERBOSE "Legacy UI executable configured: app-legacy")
else()
    message(VERBOSE "Legacy UI executable disabled (BUILD_LEGACY_UI=OFF)")
endif()
