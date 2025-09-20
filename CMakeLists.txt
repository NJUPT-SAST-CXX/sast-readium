cmake_minimum_required(VERSION 3.28)
project(sast-readium LANGUAGES CXX VERSION 0.1.0.0)

# Add cmake modules to path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Include required utility modules
include(ProjectUtils)
include(CompilerSettings)
include(Dependencies)
include(PlatformUtils)

# Setup project options and configuration
setup_project_options()
setup_compiler_settings()
setup_qt_automation()

# Detect platform and environment
detect_platform_environment()
configure_platform_specific_settings()

# Find and configure all project dependencies
find_project_dependencies()

# Qt standard project setup (must be immediately after Qt finding)
qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES zh)

# Validate build environment
validate_build_environment()

# Setup clangd integration
setup_clangd_integration()

# Add subdirectories
add_subdirectory(app)

# Add tests if enabled
if(BUILD_TESTING)
    include(TestUtils)
    setup_testing_environment()
    add_subdirectory(tests)
endif()

# Print build configuration summary
print_build_summary()


