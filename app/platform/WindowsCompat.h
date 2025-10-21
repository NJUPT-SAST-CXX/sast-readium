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

// Define WIN32_LEAN_AND_MEAN to reduce Windows header size and avoid conflicts
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Include Windows headers
#include <psapi.h>
#include <windows.h>

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
