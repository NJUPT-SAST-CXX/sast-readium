# MSYS2 toolchain for Windows x64 using MinGW-w64
# This toolchain file enables building with MSYS2's MinGW-w64 toolchain
# with automatic MSYS2 installation detection

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Function to detect MSYS2 installation
function(_detect_msys2_installation)
    set(MSYS2_CANDIDATES "")
    
    # Priority 1: Explicit MSYS2_ROOT environment variable
    if(DEFINED ENV{MSYS2_ROOT})
        list(APPEND MSYS2_CANDIDATES "$ENV{MSYS2_ROOT}")
        message(STATUS "MSYS2: Checking explicit MSYS2_ROOT: $ENV{MSYS2_ROOT}")
    endif()
    
    # Priority 2: Active MSYS2 environment (MSYSTEM_PREFIX)
    if(DEFINED ENV{MSYSTEM_PREFIX})
        # Extract MSYS2 root from MSYSTEM_PREFIX (e.g., /c/msys64/mingw64 -> /c/msys64)
        string(REGEX REPLACE "/mingw[0-9]*$" "" MSYS2_FROM_PREFIX "$ENV{MSYSTEM_PREFIX}")
        # Convert MSYS2 path to Windows path
        string(REGEX REPLACE "^/([a-zA-Z])/" "\\1:/" MSYS2_FROM_PREFIX "${MSYS2_FROM_PREFIX}")
        list(APPEND MSYS2_CANDIDATES "${MSYS2_FROM_PREFIX}")
        message(STATUS "MSYS2: Checking active environment: ${MSYS2_FROM_PREFIX}")
    endif()
    
    # Priority 3: Common default installation paths
    list(APPEND MSYS2_CANDIDATES 
        "C:/msys64"
        "C:/msys2"
        "D:/msys64"
        "D:/msys2"
    )
    
    # Test each candidate path
    foreach(candidate ${MSYS2_CANDIDATES})
        message(STATUS "MSYS2: Testing candidate path: ${candidate}")
        
        # Check if msys2_shell.cmd exists (primary indicator)
        if(EXISTS "${candidate}/msys2_shell.cmd")
            # Check if mingw64 directory exists
            if(EXISTS "${candidate}/mingw64")
                # Check if compiler exists
                if(EXISTS "${candidate}/mingw64/bin/gcc.exe")
                    set(MSYS2_ROOT "${candidate}" PARENT_SCOPE)
                    message(STATUS "MSYS2: Found valid installation at: ${candidate}")
                    return()
                else()
                    message(STATUS "MSYS2: Missing compiler at: ${candidate}/mingw64/bin/gcc.exe")
                endif()
            else()
                message(STATUS "MSYS2: Missing mingw64 directory at: ${candidate}/mingw64")
            endif()
        else()
            message(STATUS "MSYS2: Missing msys2_shell.cmd at: ${candidate}/msys2_shell.cmd")
        endif()
    endforeach()
    
    # If we reach here, no valid MSYS2 installation was found
    message(FATAL_ERROR 
        "MSYS2 installation not found!\n"
        "Searched paths:\n"
        "  - MSYS2_ROOT environment variable\n"
        "  - MSYSTEM_PREFIX (active MSYS2 environment)\n"
        "  - C:/msys64, C:/msys2, D:/msys64, D:/msys2\n\n"
        "To fix this:\n"
        "  1. Install MSYS2 from https://www.msys2.org/\n"
        "  2. Install MinGW-w64 toolchain: pacman -S mingw-w64-x86_64-toolchain\n"
        "  3. Set MSYS2_ROOT environment variable to your MSYS2 installation path\n"
        "     Example: set MSYS2_ROOT=C:\\msys64"
    )
endfunction()

# Detect MSYS2 installation
_detect_msys2_installation()

# Validate that we found a working MSYS2 installation
if(NOT DEFINED MSYS2_ROOT)
    message(FATAL_ERROR "MSYS2_ROOT not set after detection - this should not happen")
endif()

message(STATUS "MSYS2: Using installation at: ${MSYS2_ROOT}")

# Set up compiler paths
set(MSYS2_MINGW_PREFIX "${MSYS2_ROOT}/mingw64")
set(CMAKE_C_COMPILER "${MSYS2_MINGW_PREFIX}/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "${MSYS2_MINGW_PREFIX}/bin/g++.exe")
set(CMAKE_ASM_COMPILER "${MSYS2_MINGW_PREFIX}/bin/gcc.exe")
set(CMAKE_RC_COMPILER "${MSYS2_MINGW_PREFIX}/bin/windres.exe")

# Verify compilers exist
foreach(compiler CMAKE_C_COMPILER CMAKE_CXX_COMPILER CMAKE_RC_COMPILER)
    if(NOT EXISTS "${${compiler}}")
        message(FATAL_ERROR 
            "MSYS2 compiler not found: ${${compiler}}\n"
            "Please install the MinGW-w64 toolchain:\n"
            "  pacman -S mingw-w64-x86_64-toolchain"
        )
    endif()
endforeach()

# Cross-compilation configuration
set(CMAKE_FIND_ROOT_PATH "${MSYS2_MINGW_PREFIX}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# MSYS2-specific paths
set(CMAKE_PREFIX_PATH
    "${MSYS2_MINGW_PREFIX}"
    "${MSYS2_MINGW_PREFIX}/lib/cmake"
    "${MSYS2_MINGW_PREFIX}/share/cmake"
    CACHE PATH "MSYS2 MinGW prefix paths"
)

# Additional system library paths
set(CMAKE_LIBRARY_PATH
    "${MSYS2_MINGW_PREFIX}/lib"
    "${MSYS2_MINGW_PREFIX}/bin"
    CACHE PATH "MSYS2 library search paths"
)

set(CMAKE_INCLUDE_PATH
    "${MSYS2_MINGW_PREFIX}/include"
    CACHE PATH "MSYS2 include search paths"
)

# Qt6 specific settings for MSYS2
set(QT_HOST_PATH "${MSYS2_MINGW_PREFIX}" CACHE PATH "Path to MSYS2 Qt installation")

# pkg-config configuration for MSYS2
set(ENV{PKG_CONFIG_PATH} "${MSYS2_MINGW_PREFIX}/lib/pkgconfig:${MSYS2_MINGW_PREFIX}/share/pkgconfig")
set(ENV{PKG_CONFIG_LIBDIR} "${MSYS2_MINGW_PREFIX}/lib/pkgconfig:${MSYS2_MINGW_PREFIX}/share/pkgconfig")

# Set pkg-config executable if available
find_program(PKG_CONFIG_EXECUTABLE
    NAMES pkg-config
    PATHS "${MSYS2_MINGW_PREFIX}/bin"
    NO_DEFAULT_PATH
)

if(PKG_CONFIG_EXECUTABLE)
    message(STATUS "MSYS2: Found pkg-config: ${PKG_CONFIG_EXECUTABLE}")
else()
    message(WARNING "MSYS2: pkg-config not found. Install with: pacman -S mingw-w64-x86_64-pkg-config")
endif()

# MSYS2 package discovery helpers
set(CMAKE_PROGRAM_PATH
    "${MSYS2_MINGW_PREFIX}/bin"
    CACHE PATH "MSYS2 program search paths"
)

# Compiler flags for MSYS2 MinGW-w64
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")

# Windows-specific compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWIN32 -D_WINDOWS")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32 -D_WINDOWS")

# Linker flags for MSYS2
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")

# Windows subsystem for GUI applications
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--subsystem,windows")

# Set target architecture for vcpkg if used
set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "")

# Windows and MSYS2 specific definitions
add_definitions(-DWIN32 -D_WINDOWS -DUNICODE -D_UNICODE -DTARGET_OS_WINDOWS=1 -DMSYS2=1)

# MSYS2 specific settings
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
set(CMAKE_IMPORT_LIBRARY_PREFIX "")
set(CMAKE_IMPORT_LIBRARY_SUFFIX ".dll.a")
set(CMAKE_EXECUTABLE_SUFFIX ".exe")

# Export MSYS2_ROOT for use by other parts of the build system
set(MSYS2_ROOT "${MSYS2_ROOT}" CACHE PATH "MSYS2 installation root directory")

message(STATUS "MSYS2: Toolchain configuration complete")
message(STATUS "MSYS2: C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "MSYS2: CXX Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "MSYS2: RC Compiler: ${CMAKE_RC_COMPILER}")
message(STATUS "MSYS2: Find Root Path: ${CMAKE_FIND_ROOT_PATH}")
message(STATUS "MSYS2: vcpkg Triplet: ${VCPKG_TARGET_TRIPLET}")
