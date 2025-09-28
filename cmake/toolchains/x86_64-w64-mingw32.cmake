# Cross-compilation toolchain for Windows x64 using MinGW-w64
# This toolchain file enables cross-compilation to Windows using MinGW

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# MinGW-w64 compiler configuration
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_ASM_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Cross-compilation tool prefixes
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# MinGW-specific paths
set(CMAKE_PREFIX_PATH 
    "/usr/x86_64-w64-mingw32"
    "/usr/x86_64-w64-mingw32/sys-root/mingw"
    CACHE PATH "MinGW prefix paths"
)

# Qt6 specific settings for MinGW
if(DEFINED ENV{QT_HOST_PATH})
    set(QT_HOST_PATH "$ENV{QT_HOST_PATH}" CACHE PATH "Path to host Qt installation")
else()
    set(QT_HOST_PATH "/usr/x86_64-w64-mingw32" CACHE PATH "Path to host Qt installation")
endif()

# Compiler flags for MinGW x64
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")

# Windows-specific compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWIN32 -D_WINDOWS")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32 -D_WINDOWS")

# Linker flags for MinGW
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")

# Windows subsystem for GUI applications
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--subsystem,windows")

# Set target architecture for vcpkg if used (prefer dynamic linking for Qt)
set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "")

# Windows specific definitions
add_definitions(-DWIN32 -D_WINDOWS -DUNICODE -D_UNICODE -DTARGET_OS_WINDOWS=1)

# MinGW specific settings
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
set(CMAKE_IMPORT_LIBRARY_PREFIX "")
set(CMAKE_IMPORT_LIBRARY_SUFFIX ".dll.a")
set(CMAKE_EXECUTABLE_SUFFIX ".exe")
