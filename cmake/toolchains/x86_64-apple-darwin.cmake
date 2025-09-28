# Cross-compilation toolchain for macOS x64 (Intel)
# This toolchain file enables cross-compilation to Intel-based macOS systems

set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# macOS deployment target (minimum supported version)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Target architecture")

# Compiler configuration
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
else()
    # Try to find macOS SDK automatically
    execute_process(
        COMMAND xcrun --sdk macosx --show-sdk-path
        OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

# Compiler flags for x86_64
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch x86_64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch x86_64")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -arch x86_64")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -arch x86_64")

# Qt6 specific settings for macOS
if(DEFINED ENV{QT_HOST_PATH})
    set(QT_HOST_PATH "$ENV{QT_HOST_PATH}" CACHE PATH "Path to host Qt installation")
else()
    set(QT_HOST_PATH "/usr/local/Qt" CACHE PATH "Path to host Qt installation")
endif()

# Framework search paths
set(CMAKE_FRAMEWORK_PATH 
    "/System/Library/Frameworks"
    "/Library/Frameworks"
    CACHE STRING "Framework search paths"
)

# Set target architecture for vcpkg if used
set(VCPKG_TARGET_TRIPLET "x64-osx" CACHE STRING "")

# macOS specific definitions
add_definitions(-DMACOS -DTARGET_OS_MAC=1)

# Ensure proper C++ standard library
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
