# Cross-compilation toolchain for Linux x64 (x86_64)
# This toolchain file enables explicit targeting of x86_64 Linux systems

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Compiler configuration
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_ASM_COMPILER gcc)

# Cross-compilation tool prefixes (for explicit x64 targeting)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Standard library paths for x86_64
set(CMAKE_FIND_ROOT_PATH
    "/usr/lib/x86_64-linux-gnu"
    "/usr/include/x86_64-linux-gnu"
    "/lib/x86_64-linux-gnu"
)

# Set the architecture for pkg-config
set(ENV{PKG_CONFIG_PATH} "/usr/lib/x86_64-linux-gnu/pkgconfig")
set(ENV{PKG_CONFIG_LIBDIR} "/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig")

# Qt6 specific settings for Linux x64
if(DEFINED ENV{QT_HOST_PATH})
    set(QT_HOST_PATH "$ENV{QT_HOST_PATH}" CACHE PATH "Path to host Qt installation")
else()
    set(QT_HOST_PATH "/usr" CACHE PATH "Path to host Qt installation")
endif()

set(CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu/cmake" CACHE PATH "Path to x64 CMake modules")

# Compiler flags for x86_64
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath-link,/usr/lib/x86_64-linux-gnu")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-rpath-link,/usr/lib/x86_64-linux-gnu")

# Set target architecture for vcpkg if used
set(VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "")

# Linux specific definitions
add_definitions(-DLINUX -DTARGET_OS_LINUX=1)
