# Build System

SAST Readium uses CMake as the unified build system with simplified configuration for different development environments.

## CMake Build System

The project uses a streamlined CMake configuration that has been significantly simplified from the original complex setup.

**Key Features:**

- Simplified preset system (6 essential presets)
- Consolidated CMake modules (3 focused modules)
- Multiple dependency management strategies
- Comprehensive testing support
- Full CI/CD integration

## Build Presets

The project provides 6 essential build presets covering all major platform and configuration combinations:

### Unix Platforms (Linux/macOS)

- **Debug-Unix**: Debug build using system packages
- **Release-Unix**: Release build using system packages

### Windows with vcpkg

- **Debug-Windows**: Debug build using vcpkg for dependencies
- **Release-Windows**: Release build using vcpkg for dependencies

### Windows with MSYS2

- **Debug-MSYS2**: Debug build using MSYS2 system packages
- **Release-MSYS2**: Release build using MSYS2 system packages

## Simplification Summary

The build system has been streamlined from the original complex configuration:

- **CMake Presets**: Reduced from 22+ to 6 essential presets (82% reduction)
- **CMake Modules**: Consolidated from 8 separate files to 3 focused modules
- **Build Systems**: Unified to CMake only (removed alternative build systems)
- **Scripts**: Simplified and consolidated redundant build scripts

## Documentation

- [CMake Build Guide](../setup/cmake-build.md)
- [Dependency Management](../getting-started/dependency-management.md)

Complete documentation for the Xmake build system:

- [Build Guide](xmake/xmake-build.md) - Installation and usage instructions
- [Status Report](xmake/xmake-status.md) - Current implementation status
- [Final Report](xmake/xmake-final-report.md) - Implementation summary and conclusions

## Choosing a Build System

### Use CMake if:

- You need maximum stability and platform support
- You're working in a production environment
- You prefer the established CMake ecosystem
- You need the most comprehensive configuration options

### Use Xmake if:

- You prefer simpler, more readable configuration files
- You want built-in package management
- You're experimenting or prototyping
- You value faster build times

## Next Steps

1. Review the [Build System Comparison](build-system-comparison.md) to understand the differences
2. Choose your preferred build system
3. Follow the appropriate setup guides in [Setup & Configuration](../setup/)
