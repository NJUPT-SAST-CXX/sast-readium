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

# Create executable with platform-specific configuration
if(WIN32)
    setup_executable(app
        WIN32
        SOURCES ${APP_SOURCES}
    )
else()
    setup_executable(app
        SOURCES ${APP_SOURCES}
    )
endif()

# Print source summary for verification
print_source_summary("${APP_SOURCES}" TITLE "Application Sources")

# Add translations
qt_add_translations(
    app
    SOURCES ${UIs}
    TS_FILE_DIR ${CMAKE_SOURCE_DIR}/app/i18n
    TS_FILE_BASE app
)

# Add asset copying using utility function
add_asset_copying(app ${CMAKE_SOURCE_DIR}/assets/styles)


