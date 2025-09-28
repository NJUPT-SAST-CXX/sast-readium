# Clang toolchain for Windows x64
# This toolchain file enables building with Clang compiler on Windows
# Supports both standard Clang and Clang-cl (MSVC compatible mode)

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Detect Clang variant and set appropriate compilers
if(DEFINED ENV{CLANG_CL_MODE} OR CLANG_VARIANT STREQUAL "MSVC")
    # Clang-cl (MSVC compatible mode)
    set(CMAKE_C_COMPILER clang-cl)
    set(CMAKE_CXX_COMPILER clang-cl)
    set(CMAKE_ASM_COMPILER clang-cl)
    set(CMAKE_RC_COMPILER rc)
    set(CLANG_WINDOWS_MODE "MSVC" CACHE STRING "Clang Windows mode")
    message(STATUS "Using Clang-cl (MSVC compatible mode)")
else()
    # Standard Clang
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)
    set(CMAKE_ASM_COMPILER clang)
    set(CMAKE_RC_COMPILER windres)
    set(CLANG_WINDOWS_MODE "Standard" CACHE STRING "Clang Windows mode")
    message(STATUS "Using standard Clang")
endif()

# Cross-compilation root paths
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Windows-specific definitions
add_definitions(-DWIN32 -D_WINDOWS -DWIN32_LEAN_AND_MEAN -DNOMINMAX)

# Compiler flags based on Clang mode
if(CLANG_WINDOWS_MODE STREQUAL "MSVC")
    # Clang-cl flags (MSVC compatible)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /EHsc /utf-8")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /utf-8")
    
    # MSVC-style optimization flags
    set(CMAKE_C_FLAGS_DEBUG "/Od /Zi /RTC1")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /RTC1")
    set(CMAKE_C_FLAGS_RELEASE "/O2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "/O2 /Zi /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /Zi /DNDEBUG")
    set(CMAKE_C_FLAGS_MINSIZEREL "/Os /DNDEBUG")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "/Os /DNDEBUG")
else()
    # Standard Clang flags
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fms-compatibility")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fms-compatibility")
    
    # Standard optimization flags
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
    set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
    
    # Linker flags for standard Clang
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--subsystem,windows")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
endif()

# Qt6 specific settings for Windows Clang
if(DEFINED ENV{QT_HOST_PATH})
    set(QT_HOST_PATH "$ENV{QT_HOST_PATH}" CACHE PATH "Path to host Qt installation")
else()
    # Common Qt6 installation paths on Windows
    set(QT_HOST_PATH "C:/Qt/6.5.0/msvc2022_64" CACHE PATH "Path to host Qt installation")
endif()

# vcpkg integration
if(CLANG_WINDOWS_MODE STREQUAL "MSVC")
    # Use MSVC-compatible triplet for Clang-cl
    set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "vcpkg target triplet")
else()
    # Use MinGW-style triplet for standard Clang
    set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "vcpkg target triplet")
endif()

# vcpkg chainloading support
if(DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake")
    # vcpkg is already being used, ensure proper chainloading
    set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_FILE} CACHE STRING "Chainload this toolchain")
    message(STATUS "vcpkg chainloading enabled for Clang Windows toolchain")
endif()

# Set vcpkg environment variables for Clang compatibility
if(CLANG_WINDOWS_MODE STREQUAL "MSVC")
    set(VCPKG_ENV_PASSTHROUGH_UNTRACKED "CLANG_CL_MODE" CACHE STRING "Pass Clang-cl mode to vcpkg")
endif()

# Windows SDK configuration
if(DEFINED ENV{WINDOWS_SDK_PATH})
    set(CMAKE_WINDOWS_KITS_10_DIR "$ENV{WINDOWS_SDK_PATH}")
endif()

# Clang-specific optimizations for Windows
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CLANG_WINDOWS_MODE STREQUAL "Standard")
        # Enable LTO for standard Clang
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto=thin")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto=thin")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto=thin")
    endif()
endif()

# Color diagnostics support
if(NOT DEFINED ENV{NO_COLOR})
    if(CLANG_WINDOWS_MODE STREQUAL "Standard")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
    endif()
endif()

# Set target architecture
set(CMAKE_GENERATOR_PLATFORM x64)

message(STATUS "Clang Windows toolchain configured")
message(STATUS "  Mode: ${CLANG_WINDOWS_MODE}")
message(STATUS "  vcpkg triplet: ${VCPKG_TARGET_TRIPLET}")
message(STATUS "  Qt6 path: ${QT_HOST_PATH}")
