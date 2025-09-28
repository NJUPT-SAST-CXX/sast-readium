# Clang toolchain for macOS (Intel and Apple Silicon)
# This toolchain file enables building with Clang compiler on macOS
# Supports both Intel x86_64 and Apple Silicon ARM64 architectures

set(CMAKE_SYSTEM_NAME Darwin)

# Detect target architecture (can be overridden)
if(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
    # Auto-detect based on host system
    execute_process(
        COMMAND uname -m
        OUTPUT_VARIABLE HOST_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    if(HOST_ARCH STREQUAL "arm64")
        set(CMAKE_SYSTEM_PROCESSOR arm64)
        set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Target architecture")
        message(STATUS "Auto-detected Apple Silicon (ARM64)")
    else()
        set(CMAKE_SYSTEM_PROCESSOR x86_64)
        set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Target architecture")
        message(STATUS "Auto-detected Intel (x86_64)")
    endif()
else()
    set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_OSX_ARCHITECTURES})
endif()

# macOS deployment target (minimum supported version)
if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version for Apple Silicon")
else()
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version for Intel")
endif()

# Clang compiler configuration (prefer system Clang on macOS)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_ASM_COMPILER clang)

# Cross-compilation root paths
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# macOS SDK configuration
if(DEFINED ENV{MACOS_SDK_PATH})
    set(CMAKE_OSX_SYSROOT "$ENV{MACOS_SDK_PATH}")
    message(STATUS "Using explicit macOS SDK: $ENV{MACOS_SDK_PATH}")
else()
    # Try to find macOS SDK automatically
    execute_process(
        COMMAND xcrun --sdk macosx --show-sdk-path
        OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    if(CMAKE_OSX_SYSROOT)
        message(STATUS "Auto-detected macOS SDK: ${CMAKE_OSX_SYSROOT}")
    else()
        message(WARNING "Could not auto-detect macOS SDK path")
    endif()
endif()

# Architecture-specific compiler flags
if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch arm64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch arm64")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -arch arm64")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -arch arm64")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch x86_64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch x86_64")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -arch x86_64")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -arch x86_64")
endif()

# macOS-specific definitions
add_definitions(-DMACOS -DTARGET_OS_MAC=1)

# Standard C++ library (libc++ is standard on macOS)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

# Build type specific flags
set(CMAKE_C_FLAGS_DEBUG "-O0 -g -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -flto=thin")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -flto=thin")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG -flto=thin")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG -flto=thin")

# LTO linker flags for release builds
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto=thin")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -flto=thin")
set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -flto=thin")
set(CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL "${CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL} -flto=thin")

# RPATH configuration for macOS
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "@executable_path/../lib;@executable_path/../Frameworks")

# Framework search paths
set(CMAKE_FRAMEWORK_PATH 
    "/System/Library/Frameworks"
    "/Library/Frameworks"
    CACHE STRING "Framework search paths"
)

# Qt6 specific settings for macOS
if(DEFINED ENV{QT_HOST_PATH})
    set(QT_HOST_PATH "$ENV{QT_HOST_PATH}" CACHE PATH "Path to host Qt installation")
else()
    # Common Qt6 installation paths on macOS
    if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        set(QT_HOST_PATH "/opt/homebrew/opt/qt6" CACHE PATH "Path to host Qt installation")
    else()
        set(QT_HOST_PATH "/usr/local/opt/qt6" CACHE PATH "Path to host Qt installation")
    endif()
endif()

# vcpkg integration
if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
    set(VCPKG_TARGET_TRIPLET "arm64-osx" CACHE STRING "vcpkg target triplet")
else()
    set(VCPKG_TARGET_TRIPLET "x64-osx" CACHE STRING "vcpkg target triplet")
endif()

# vcpkg chainloading support
if(DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake")
    # vcpkg is already being used, ensure proper chainloading
    set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_FILE} CACHE STRING "Chainload this toolchain")
    message(STATUS "vcpkg chainloading enabled for Clang macOS toolchain")
endif()

# Set vcpkg environment variables for Clang compatibility
set(VCPKG_ENV_PASSTHROUGH_UNTRACKED "CC;CXX;MACOSX_DEPLOYMENT_TARGET" CACHE STRING "Pass Clang compilers and deployment target to vcpkg")
set(ENV{CC} ${CMAKE_C_COMPILER})
set(ENV{CXX} ${CMAKE_CXX_COMPILER})
set(ENV{MACOSX_DEPLOYMENT_TARGET} ${CMAKE_OSX_DEPLOYMENT_TARGET})

# Color diagnostics support
if(NOT DEFINED ENV{NO_COLOR})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
endif()

# Apple-specific optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Enable native CPU optimizations for the target architecture
    if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -mcpu=apple-a14")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mcpu=apple-a14")
    else()
        # For Intel, use native optimization
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -march=native")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
    endif()
    
    # Function and data sections for better dead code elimination
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -ffunction-sections -fdata-sections")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ffunction-sections -fdata-sections")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -Wl,-dead_strip")
endif()

# Debug information format
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -gdwarf-4")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-4")
endif()

# Objective-C++ support (useful for Qt on macOS)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fobjc-arc")

# Security hardening
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-strong")

# Universal binary support (if building for multiple architectures)
if(CMAKE_OSX_ARCHITECTURES MATCHES "x86_64;arm64" OR CMAKE_OSX_ARCHITECTURES MATCHES "arm64;x86_64")
    message(STATUS "Configuring for Universal Binary (x86_64 + arm64)")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch x86_64 -arch arm64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch x86_64 -arch arm64")
endif()

message(STATUS "Clang macOS toolchain configured")
message(STATUS "  Target: ${CMAKE_SYSTEM_NAME} ${CMAKE_OSX_ARCHITECTURES}")
message(STATUS "  Deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
message(STATUS "  SDK: ${CMAKE_OSX_SYSROOT}")
message(STATUS "  vcpkg triplet: ${VCPKG_TARGET_TRIPLET}")
message(STATUS "  Qt6 path: ${QT_HOST_PATH}")
message(STATUS "  Standard library: libc++")
