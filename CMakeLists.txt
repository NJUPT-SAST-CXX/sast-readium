cmake_minimum_required(VERSION 3.28)
project(sast-readium LANGUAGES CXX VERSION 0.1.0.0)

# Add cmake modules to path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Include consolidated utility modules
include(ProjectConfig)
include(Dependencies)
include(TargetUtils)
include(Packaging)

add_subdirectory(libs)

# Setup project options and configuration
setup_project_options()
setup_packaging_options()
setup_compiler_settings()
setup_qt_automation()

# Detect platform and environment
detect_platform_environment()
configure_platform_specific_settings()

# Find and configure all project dependencies
find_project_dependencies()

# Setup packaging infrastructure
find_qt_deployment_tools()
setup_cpack_configuration()

# Include CPack after configuration
include(CPack)

# Qt standard project setup (must be immediately after Qt finding)
qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES zh)

# Validate build environment
validate_build_environment()

# Legacy UI option (old implementation) - disabled by default
# Must be defined before add_subdirectory(app) so it's available in app/CMakeLists.txt
option(BUILD_LEGACY_UI "Build the legacy UI executable" OFF)
if(BUILD_LEGACY_UI)
    message(STATUS "Building legacy UI executable (app-legacy)")
endif()

# Add subdirectories
add_subdirectory(app)

# Add tests if enabled
if(BUILD_TESTING)
    # Enable testing at top level (required for CTest)
    enable_testing()
    setup_testing_environment()
    add_subdirectory(tests)
endif()

# Setup clangd integration
setup_clangd_integration()

# Print build configuration summary
print_build_summary()
