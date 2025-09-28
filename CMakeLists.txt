cmake_minimum_required(VERSION 3.28)
project(sast-readium LANGUAGES CXX VERSION 0.1.0.0)

# Add cmake modules to path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Include consolidated utility modules
include(ProjectConfig)
include(Dependencies)
include(TargetUtils)

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

# Add subdirectories
add_subdirectory(app)

# Add tests if enabled
if(BUILD_TESTING)
    setup_testing_environment()
    add_subdirectory(tests)
endif()

# Copy compile_commands.json to project root for clangd integration
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    # Create a custom target that runs after CMake generation
    add_custom_target(copy_compile_commands
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/compile_commands.json"
        BYPRODUCTS "${CMAKE_SOURCE_DIR}/compile_commands.json"
        COMMENT "Copying compile_commands.json to project root for clangd"
        VERBATIM
    )

    # Add a post-build step to the main project that copies the file
    # This will run whenever any target is built successfully
    add_custom_command(TARGET copy_compile_commands POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/compile_commands.json"
        COMMENT "Updating compile_commands.json in project root"
        VERBATIM
    )
endif()

# Print build configuration summary
print_build_summary()


