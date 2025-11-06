/**
 * @file StackTrace.h
 * @brief Cross-platform stack trace capture utility
 * @author SAST Readium Project
 * @version 1.0
 * @date 2025-10-31
 *
 * This file provides cross-platform stack trace capture functionality
 * for crash reporting and debugging purposes.
 */

#pragma once

#include <QString>
#include <QStringList>
#include <QThread>
#include <memory>
#include <vector>

namespace StackTraceUtils {

/**
 * @brief Stack frame information
 */
struct StackFrame {
    QString functionName;     ///< Demangled function name
    QString moduleName;       ///< Module/library name
    QString fileName;         ///< Source file name (if available)
    int lineNumber = -1;      ///< Line number (if available)
    void* address = nullptr;  ///< Instruction pointer address
    QString addressString;    ///< Formatted address string

    QString toString() const;
};

/**
 * @brief Capture current stack trace
 * @param maxFrames Maximum number of frames to capture (default: 64)
 * @param skipFrames Number of frames to skip from the top (default: 0)
 * @return Vector of stack frames
 */
std::vector<StackFrame> captureStackTrace(int maxFrames = 64,
                                          int skipFrames = 0);

/**
 * @brief Format stack trace as a string
 * @param frames Stack frames to format
 * @param includeAddresses Include memory addresses in output
 * @return Formatted stack trace string
 */
QString formatStackTrace(const std::vector<StackFrame>& frames,
                         bool includeAddresses = true);

/**
 * @brief Capture and format stack trace in one call
 * @param maxFrames Maximum number of frames to capture
 * @param skipFrames Number of frames to skip from the top
 * @param includeAddresses Include memory addresses in output
 * @return Formatted stack trace string
 */
QString captureAndFormatStackTrace(int maxFrames = 64, int skipFrames = 1,
                                   bool includeAddresses = true);

/**
 * @brief Get thread information
 * @return String containing thread ID and name
 */
QString getThreadInfo();

/**
 * @brief Get current thread ID as string
 * @return Thread ID string
 */
QString getCurrentThreadId();

/**
 * @brief Get current thread name (if available)
 * @return Thread name or empty string
 */
QString getCurrentThreadName();

/**
 * @brief Initialize stack trace system (platform-specific setup)
 * @return true if successful, false otherwise
 */
bool initialize();

/**
 * @brief Cleanup stack trace system
 */
void cleanup();

/**
 * @brief Check if stack trace capture is available on this platform
 * @return true if available, false otherwise
 */
bool isAvailable();

/**
 * @brief Demangle C++ symbol name
 * @param mangledName Mangled symbol name
 * @return Demangled name or original if demangling fails
 */
QString demangleSymbol(const QString& mangledName);

}  // namespace StackTraceUtils
