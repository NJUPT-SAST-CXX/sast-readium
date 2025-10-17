project(app LANGUAGES CXX VERSION 0.1.0.0 DESCRIPTION "SAST Readium")

# Include utility modules
include(TargetUtils)

# Configure header generation
configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# Collect UI files for translations
file(GLOB UIs "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")

# Discover application sources automatically
message(STATUS "Discovering application sources...")
discover_app_sources(APP_SOURCES)

# Add Qt resource file for automatic compilation
list(APPEND APP_SOURCES app.qrc)

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
message(STATUS "Discovered ${source_count} application sources")

# Separate main.cpp from other sources for library creation
list(REMOVE_ITEM APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

# Create application library (without main.cpp) for testing
add_library(app_lib STATIC ${APP_SOURCES})
target_include_directories(app_lib PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Get common libraries and link them to app_lib
get_common_libraries(common_libs)
target_link_libraries(app_lib PUBLIC ${common_libs})

# Set C++ standard for library
set_target_properties(app_lib PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Configure precompiled headers for faster builds
# Include frequently used Qt and STL headers
target_precompile_headers(app_lib PRIVATE
    <QWidget>
    <QString>
    <QList>
    <QVector>
    <QMap>
    <QHash>
    <QDebug>
    <memory>
    <vector>
    <string>
    <algorithm>
    "logging/Logger.h"
)

# Create executable with platform-specific configuration
if(WIN32)
    add_executable(app "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
    # Temporarily disable WIN32_EXECUTABLE for debugging
    set_target_properties(app PROPERTIES WIN32_EXECUTABLE FALSE)
else()
    add_executable(app "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
endif()

# Link the executable to the library
target_link_libraries(app PRIVATE app_lib)

# Add generated config header and app source directory to include path
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add translations
qt_add_translations(
    app
    SOURCES ${UIs}
    TS_FILE_DIR ${CMAKE_SOURCE_DIR}/app/i18n
    TS_FILE_BASE app
)

# Copy assets to output directory
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets/styles
    $<TARGET_FILE_DIR:app>/styles
    COMMENT "Copying application assets"
)
