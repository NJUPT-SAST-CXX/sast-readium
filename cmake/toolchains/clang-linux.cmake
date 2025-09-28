# Clang toolchain for Linux x64
# This toolchain file enables building with Clang compiler on Linux
# Supports both native and cross-compilation scenarios

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Clang compiler configuration
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_ASM_COMPILER clang)

# Cross-compilation root paths
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Standard library paths for x86_64 Linux
set(CMAKE_FIND_ROOT_PATH 
    "/usr/lib/x86_64-linux-gnu"
    "/usr/include/x86_64-linux-gnu"
    "/lib/x86_64-linux-gnu"
)

# Set the architecture for pkg-config
set(ENV{PKG_CONFIG_PATH} "/usr/lib/x86_64-linux-gnu/pkgconfig")
set(ENV{PKG_CONFIG_LIBDIR} "/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig")

# Qt6 specific settings for Linux Clang
if(DEFINED ENV{QT_HOST_PATH})
    set(QT_HOST_PATH "$ENV{QT_HOST_PATH}" CACHE PATH "Path to host Qt installation")
else()
    set(QT_HOST_PATH "/usr" CACHE PATH "Path to host Qt installation")
endif()

set(CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu/cmake" CACHE PATH "Path to x64 CMake modules")

# Clang-specific compiler flags for Linux
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")

# Linux-specific definitions
add_definitions(-DLINUX -DTARGET_OS_LINUX=1)

# Position Independent Code (required for shared libraries)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Clang-specific optimizations
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# Security hardening flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-strong")

# Build type specific flags
set(CMAKE_C_FLAGS_DEBUG "-O0 -g -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -flto=thin")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -flto=thin")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG -flto=thin")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG -flto=thin")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath-link,/usr/lib/x86_64-linux-gnu")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-rpath-link,/usr/lib/x86_64-linux-gnu")

# Security hardening linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,relro,-z,now")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,relro,-z,now")

# Optimization linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--as-needed")

# LTO linker flags for release builds
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto=thin")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -flto=thin")
set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -flto=thin")
set(CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL "${CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL} -flto=thin")

# Color diagnostics support
if(NOT DEFINED ENV{NO_COLOR})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
endif()

# Set target architecture for vcpkg if used
set(VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "vcpkg target triplet")

# vcpkg chainloading support
if(DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake")
    # vcpkg is already being used, ensure proper chainloading
    set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_FILE} CACHE STRING "Chainload this toolchain")
    message(STATUS "vcpkg chainloading enabled for Clang Linux toolchain")
endif()

# Set vcpkg environment variables for Clang compatibility
set(VCPKG_ENV_PASSTHROUGH_UNTRACKED "CC;CXX" CACHE STRING "Pass Clang compilers to vcpkg")
set(ENV{CC} ${CMAKE_C_COMPILER})
set(ENV{CXX} ${CMAKE_CXX_COMPILER})

# Standard C++ library
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")

# Thread support
find_package(Threads REQUIRED)

# Common Linux libraries
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread -ldl")

# RPATH configuration for better library loading
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# Debug information format
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -gdwarf-4")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-4")
endif()

# Architecture-specific optimizations for release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Enable native CPU optimizations (can be overridden)
    if(NOT DEFINED CLANG_NO_NATIVE_ARCH)
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -march=native")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
    endif()
    
    # Function and data sections for better dead code elimination
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -ffunction-sections -fdata-sections")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ffunction-sections -fdata-sections")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -Wl,--gc-sections")
endif()

message(STATUS "Clang Linux toolchain configured")
message(STATUS "  Target: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  vcpkg triplet: ${VCPKG_TARGET_TRIPLET}")
message(STATUS "  Qt6 path: ${QT_HOST_PATH}")
message(STATUS "  Standard library: libstdc++")
