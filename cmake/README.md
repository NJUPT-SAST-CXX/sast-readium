# CMake Modules for SAST Readium

This directory contains modular CMake utilities that provide reusable functions for building the SAST Readium project. The modules are designed to eliminate redundancy, improve maintainability, and follow modern CMake best practices.

## Module Overview

### Dependencies.cmake
Centralized dependency management for the project.

**Key Functions:**
- `find_project_dependencies()` - Find all required dependencies with intelligent caching
- `get_common_libraries()` - Get standard libraries for linking
- `get_test_libraries()` - Get libraries needed for testing

**Features:**
- Automatic vcpkg vs system package detection
- MSYS2 environment handling
- Dependency caching to avoid redundant find_package calls

### CompilerSettings.cmake
Common compiler and language configuration.

**Key Functions:**
- `setup_compiler_settings()` - Configure C++20 standard and compiler flags
- `setup_qt_automation()` - Enable Qt's MOC, UIC, RCC automation
- `configure_build_optimizations()` - Apply build-type specific optimizations
- `print_compiler_info()` - Display compiler configuration summary

**Features:**
- Platform-specific compiler flag handling
- Build type optimizations (Debug/Release)
- Consistent C++ standard enforcement

### PlatformUtils.cmake
Platform detection and configuration utilities.

**Key Functions:**
- `detect_platform_environment()` - Detect OS, compiler, and environment
- `configure_platform_specific_settings()` - Apply platform-specific configurations
- `setup_deployment_tools()` - Configure platform deployment (windeployqt, etc.)
- `print_platform_info()` - Display platform information

**Features:**
- Windows, Linux, macOS support
- MSYS2 environment detection
- Automatic deployment tool setup

### TargetUtils.cmake
Standardized target creation and configuration.

**Key Functions:**
- `setup_target()` - Common target configuration with flexible options
- `setup_executable()` - Executable-specific setup with platform handling
- `setup_library()` - Library creation with type specification
- `add_asset_copying()` - Automated asset management

**Features:**
- Consistent target configuration across the project
- Automatic include directory and library linkage
- Platform-specific executable configuration
- Asset copying with proper dependency tracking

### TestUtils.cmake
Test creation and management utilities.

**Key Functions:**
- `setup_testing_environment()` - Configure testing environment
- `create_test_executable()` - Standardized test creation
- `create_test_library()` - Test-specific library creation
- `add_test_categories()` - Custom targets for test categories
- `setup_test_coverage()` - Coverage reporting configuration

**Features:**
- Unified test configuration combining best practices
- Test categorization (unit, integration, performance)
- Automatic CTest registration
- Coverage reporting support

### ProjectUtils.cmake
Project-wide utilities and configuration.

**Key Functions:**
- `setup_clangd_integration()` - Automatic clangd configuration
- `print_build_summary()` - Comprehensive build information display
- `validate_build_environment()` - Environment and dependency validation
- `setup_project_options()` - Standard project option definitions

**Features:**
- Automatic clangd configuration updates
- Build environment validation
- Comprehensive build summaries
- Standardized project options

## Usage Examples

### Basic Project Setup
```cmake
# In root CMakeLists.txt
cmake_minimum_required(VERSION 3.28)
project(sast-readium LANGUAGES CXX VERSION 0.1.0.0)

# Add cmake modules to path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Include required modules
include(ProjectUtils)
include(CompilerSettings)
include(Dependencies)
include(PlatformUtils)

# Setup project
setup_project_options()
setup_compiler_settings()
setup_qt_automation()
detect_platform_environment()
find_project_dependencies()
validate_build_environment()

# Print summary
print_build_summary()
```

### Creating an Executable
```cmake
# In app/CMakeLists.txt
include(TargetUtils)

# Create and configure executable
setup_executable(app
    SOURCES ${APP_SOURCES}
    WIN32  # Windows GUI app
)

# Add asset copying
add_asset_copying(app ${CMAKE_SOURCE_DIR}/assets/styles)
```

### Creating Tests
```cmake
# In tests/CMakeLists.txt
include(TestUtils)

# Setup testing environment
setup_testing_environment()

# Create test executable
create_test_executable(test_search_engine
    SOURCES test_search_engine.cpp
    CATEGORY unit
    TIMEOUT 60
)

# Add test categories
add_test_categories()
```

## Migration Guide

When migrating from the old CMake structure:

1. **Update root CMakeLists.txt:**
   - Add cmake directory to CMAKE_MODULE_PATH
   - Replace inline configurations with module function calls
   - Remove redundant dependency finding

2. **Update app/CMakeLists.txt:**
   - Use `setup_executable()` instead of manual configuration
   - Replace asset copying with `add_asset_copying()`

3. **Update test CMakeLists.txt:**
   - Use `create_test_executable()` for consistent test creation
   - Remove redundant dependency finding
   - Use test category functions

4. **Clean up:**
   - Remove obsolete CMakeLists_new.txt files
   - Remove redundant code and configurations

## Best Practices

1. **Always include required modules** at the top of CMakeLists.txt files
2. **Use function parameters** instead of global variables when possible
3. **Call validation functions** to catch configuration errors early
4. **Use consistent naming** for targets and variables
5. **Document custom functions** using the RST format shown in modules

## Troubleshooting

### Common Issues

**"Function not found" errors:**
- Ensure the module is included with `include(ModuleName)`
- Check that CMAKE_MODULE_PATH includes the cmake directory

**Dependency not found:**
- Call `find_project_dependencies()` before using dependency targets
- Check that required packages are installed

**Platform-specific issues:**
- Call `detect_platform_environment()` early in configuration
- Use platform-specific functions when needed

### Debug Information

Use these functions to get detailed information:
- `print_build_summary()` - Overall build configuration
- `print_compiler_info()` - Compiler details
- `print_platform_info()` - Platform information

### SourceUtils.cmake
Automatic source file discovery and validation utilities.

**Key Functions:**
- `discover_app_sources()` - Automatically discover application source files
- `discover_component_sources()` - Discover sources for specific components
- `validate_discovered_sources()` - Validate that required sources were found
- `print_source_summary()` - Display discovered source file summary

**Features:**
- Automatic source file discovery using GLOB_RECURSE
- Component-based source organization
- Test file exclusion with regex patterns
- Source validation with required files and patterns
- Duplicate source detection and removal

### ComponentUtils.cmake
Component-based library creation and management utilities.

**Key Functions:**
- `create_component_library()` - Create a library from specific components
- `create_standard_app_components()` - Create standard component libraries
- `get_component_libraries()` - Get list of available component libraries
- `print_component_summary()` - Display component library summary

**Features:**
- Automatic component library creation from source discovery
- Shared libraries between application and tests
- Eliminates source duplication
- Component-based dependency management
- Standardized library naming and organization

## Contributing

When adding new functionality:

1. **Follow the existing pattern** of documented functions with RST comments
2. **Add validation** and error checking to functions
3. **Use cmake_parse_arguments()** for complex function parameters
4. **Test on multiple platforms** when possible
5. **Update this README** with new functions and usage examples
