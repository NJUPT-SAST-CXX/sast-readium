#pragma once

/**
 * @file WindowsCompat.h
 * @brief Windows platform compatibility header
 *
 * This header must be included BEFORE any Qt headers to avoid macro conflicts.
 * It includes Windows headers and undefines problematic macros that conflict
 * with our code.
 *
 * NOTE: We use _WIN32 instead of Q_OS_WIN because this header is included
 * in precompiled headers BEFORE Qt headers are processed.
 */

#ifdef _WIN32

// Ensure we have the necessary Windows version definitions
#ifndef WINVER
#define WINVER 0x0A00  // Windows 10
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00  // Windows 10
#endif

// Don't define WIN32_LEAN_AND_MEAN as it excludes necessary APIs
// that psapi.h needs

// Include Windows headers (windows.h must come before psapi.h)
// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on

// Undefine problematic Windows macros that conflict with our code
#ifdef NEAR
#undef NEAR
#endif

#ifdef FAR
#undef FAR
#endif

#ifdef ERROR
#undef ERROR
#endif

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#endif  // _WIN32
