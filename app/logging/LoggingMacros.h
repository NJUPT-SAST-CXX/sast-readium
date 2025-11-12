/**
 * @file LoggingMacros.h
 * @brief Comprehensive logging macros for seamless migration from Qt logging to
 * spdlog
 * @author SAST Readium Project
 * @version 1.0
 * @date 2025-09-12
 *
 * This file provides drop-in replacement macros for Qt's logging functions
 * (qDebug, qWarning, qCritical, etc.) while using spdlog as the backend.
 * It supports both immediate migration and gradual transition approaches.
 */

#pragma once

// ============================================================================
// Include Dependencies
// ============================================================================
// This file provides the main logging macros and utilities for the application.
// Include hierarchy:
//   LoggingMacros.h (this file)
//   ├── Logger.h (core logging functionality)
//   └── LoggingManager.h (configuration management)
//       └── LoggingConfig.h (modern configuration system)
//
// Note: LoggingManager.h is included here to provide access to configuration
// management for scoped logging classes and category-based logging.

#include <fmt/format.h>
#include <QString>
#include <chrono>
#include <memory>
#include "Logger.h"
#include "LoggingManager.h"
#include "QtSpdlogBridge.h"

// Forward declarations to reduce header dependencies
class QDebug;
class QThread;
template <typename Key, typename T>
class QHash;

// ============================================================================
// Core Logging Macros (spdlog-style with format strings)
// ============================================================================

/**
 * @defgroup CoreLogging Core Logging Macros
 * @brief Basic logging macros with format string support
 * @{
 */

/** @brief Log trace level message with format string */
#define LOG_T(...) Logger::instance().trace(__VA_ARGS__)
/** @brief Log debug level message with format string */
#define LOG_D(...) Logger::instance().debug(__VA_ARGS__)
/** @brief Log info level message with format string */
#define LOG_I(...) Logger::instance().info(__VA_ARGS__)
/** @brief Log warning level message with format string */
#define LOG_W(...) Logger::instance().warning(__VA_ARGS__)
/** @brief Log error level message with format string */
#define LOG_E(...) Logger::instance().error(__VA_ARGS__)
/** @brief Log critical level message with format string */
#define LOG_C(...) Logger::instance().critical(__VA_ARGS__)

/** @brief Log trace level message with format string (full name) */
#define LOG_TRACE(...) Logger::instance().trace(__VA_ARGS__)
/** @brief Log debug level message with format string (full name) */
#define LOG_DEBUG(...) Logger::instance().debug(__VA_ARGS__)
/** @brief Log info level message with format string (full name) */
#define LOG_INFO(...) Logger::instance().info(__VA_ARGS__)
/** @brief Log warning level message with format string (full name) */
#define LOG_WARNING(...) Logger::instance().warning(__VA_ARGS__)
/** @brief Log error level message with format string (full name) */
#define LOG_ERROR(...) Logger::instance().error(__VA_ARGS__)
/** @brief Log critical level message with format string (full name) */
#define LOG_CRITICAL(...) Logger::instance().critical(__VA_ARGS__)

/** @} */

// ============================================================================
// Qt-Style Streaming Macros (for gradual migration)
// ============================================================================

/**
 * @defgroup QtStreaming Qt-Style Streaming Macros
 * @brief Qt-style streaming macros for easier migration
 * @{
 */

/** @brief Debug level streaming macro */
#define LOG_DEBUG_STREAM() spdlogDebug()
/** @brief Info level streaming macro */
#define LOG_INFO_STREAM() spdlogInfo()
/** @brief Warning level streaming macro */
#define LOG_WARNING_STREAM() spdlogWarning()
/** @brief Critical level streaming macro */
#define LOG_CRITICAL_STREAM() spdlogCritical()

/** @} */

// ============================================================================
// Conditional Logging Macros
// ============================================================================

/**
 * @defgroup ConditionalLogging Conditional Logging Macros
 * @brief Macros that log only when certain conditions are met
 * @{
 */

/**
 * @brief Log message only if condition is true
 * @param condition Boolean condition to check
 * @param level Log level (DEBUG, INFO, WARNING, ERROR, CRITICAL)
 * @param ... Format string and arguments
 */
#define LOG_IF(condition, level, ...) \
    do {                              \
        if (condition) {              \
            LOG_##level(__VA_ARGS__); \
        }                             \
    } while (0)

/** @brief Log debug message only if condition is true */
#define LOG_DEBUG_IF(condition, ...) LOG_IF(condition, DEBUG, __VA_ARGS__)
/** @brief Log info message only if condition is true */
#define LOG_INFO_IF(condition, ...) LOG_IF(condition, INFO, __VA_ARGS__)
/** @brief Log warning message only if condition is true */
#define LOG_WARNING_IF(condition, ...) LOG_IF(condition, WARNING, __VA_ARGS__)
/** @brief Log error message only if condition is true */
#define LOG_ERROR_IF(condition, ...) LOG_IF(condition, ERROR, __VA_ARGS__)
/** @brief Log critical message only if condition is true */
#define LOG_CRITICAL_IF(condition, ...) LOG_IF(condition, CRITICAL, __VA_ARGS__)

/** @} */

// ============================================================================
// Category-Based Logging (QLoggingCategory replacement)
// ============================================================================

/**
 * @defgroup CategoryLogging Category-Based Logging
 * @brief Macros for category-based logging (replacement for QLoggingCategory)
 * @{
 */

/**
 * @brief Declare a logging category
 * @param name Category name identifier
 */
#define DECLARE_LOG_CATEGORY(name) extern const char* const LOG_CAT_##name

/**
 * @brief Define a logging category
 * @param name Category name identifier
 * @param string_name String name of the category
 */
#define DEFINE_LOG_CATEGORY(name, string_name) \
    const char* const LOG_CAT_##name = string_name

/** @brief Log debug message for specific category */
#define LOG_CATEGORY_DEBUG(category, ...)                                   \
    do {                                                                    \
        if (LoggingManager::instance().getLoggingCategoryLevel(category) <= \
            Logger::LogLevel::Debug) {                                      \
            Logger::instance().debug("[{}] {}", category,                   \
                                     fmt::format(__VA_ARGS__));             \
        }                                                                   \
    } while (0)

/** @brief Log info message for specific category */
#define LOG_CATEGORY_INFO(category, ...)                                    \
    do {                                                                    \
        if (LoggingManager::instance().getLoggingCategoryLevel(category) <= \
            Logger::LogLevel::Info) {                                       \
            Logger::instance().info("[{}] {}", category,                    \
                                    fmt::format(__VA_ARGS__));              \
        }                                                                   \
    } while (0)

/** @brief Log warning message for specific category */
#define LOG_CATEGORY_WARNING(category, ...)                                 \
    do {                                                                    \
        if (LoggingManager::instance().getLoggingCategoryLevel(category) <= \
            Logger::LogLevel::Warning) {                                    \
            Logger::instance().warning("[{}] {}", category,                 \
                                       fmt::format(__VA_ARGS__));           \
        }                                                                   \
    } while (0)

/** @brief Log error message for specific category */
#define LOG_CATEGORY_ERROR(category, ...)                                   \
    do {                                                                    \
        if (LoggingManager::instance().getLoggingCategoryLevel(category) <= \
            Logger::LogLevel::Error) {                                      \
            Logger::instance().error("[{}] {}", category,                   \
                                     fmt::format(__VA_ARGS__));             \
        }                                                                   \
    } while (0)

/** @} */

// ============================================================================
// Performance Logging Macros
// ============================================================================

/**
 * @defgroup PerformanceLogging Performance Logging Macros
 * @brief Macros for performance measurement and logging
 * @{
 */

/**
 * @brief Start performance measurement
 * @param name Unique identifier for the measurement
 */
#define LOG_PERFORMANCE_START(name) \
    auto _perf_start_##name = std::chrono::high_resolution_clock::now()

/**
 * @brief End performance measurement and log result
 * @param name Unique identifier for the measurement (must match
 * LOG_PERFORMANCE_START)
 * @param ... Additional format string and arguments
 */
#define LOG_PERFORMANCE_END(name, ...)                                  \
    do {                                                                \
        auto _perf_end = std::chrono::high_resolution_clock::now();     \
        auto _perf_duration =                                           \
            std::chrono::duration_cast<std::chrono::milliseconds>(      \
                _perf_end - _perf_start_##name)                         \
                .count();                                               \
        LOG_DEBUG("Performance [{}]: {}ms - {}", #name, _perf_duration, \
                  fmt::format(__VA_ARGS__));                            \
    } while (0)

/** @brief Log function entry for tracing */
#define LOG_FUNCTION_ENTRY() LOG_TRACE("Entering function: {}", __FUNCTION__)
/** @brief Log function exit for tracing */
#define LOG_FUNCTION_EXIT() LOG_TRACE("Exiting function: {}", __FUNCTION__)

/**
 * @brief RAII performance logger for scope measurement
 * @param name Name identifier for the scope
 */
#define LOG_PERFORMANCE_SCOPE(name) \
    PerformanceLogger _perf_logger(name, __FILE__, __LINE__)

/** @} */

// ============================================================================
// Scoped Logging Configuration Macros
// ============================================================================

/**
 * @defgroup ScopedLogging Scoped Logging Configuration
 * @brief Macros for temporary logging configuration changes
 * @{
 */

/** @brief Temporarily change log level for current scope */
#define SCOPED_LOG_LEVEL(level) ScopedLogLevel _scoped_log_level(level)

/** @brief Temporarily change entire logging configuration for current scope */
#define SCOPED_LOG_CONFIG(config) ScopedLogLevel _scoped_log_config(config)

/** @brief Temporarily enable debug logging for current scope */
#define SCOPED_DEBUG_LOGGING() SCOPED_LOG_LEVEL(Logger::LogLevel::Debug)

/** @brief Temporarily enable trace logging for current scope */
#define SCOPED_TRACE_LOGGING() SCOPED_LOG_LEVEL(Logger::LogLevel::Trace)

/** @brief Temporarily disable logging (critical only) for current scope */
#define SCOPED_QUIET_LOGGING() SCOPED_LOG_LEVEL(Logger::LogLevel::Critical)

/** @} */

// ============================================================================
// Debug-Only Logging Macros
// ============================================================================

/**
 * @defgroup DebugOnlyLogging Debug-Only Logging Macros
 * @brief Macros that only log in debug builds
 * @{
 */

#ifdef QT_DEBUG
/** @brief Log debug message only in debug builds */
#define LOG_DEBUG_ONLY(...) LOG_DEBUG(__VA_ARGS__)
/** @brief Log trace message only in debug builds */
#define LOG_TRACE_ONLY(...) LOG_TRACE(__VA_ARGS__)
#else
#define LOG_DEBUG_ONLY(...) \
    do {                    \
    } while (0)
#define LOG_TRACE_ONLY(...) \
    do {                    \
    } while (0)
#endif

/** @} */

// ============================================================================
// Qt Compatibility Macros
// ============================================================================

/**
 * @defgroup QtCompatibility Qt Compatibility Macros
 * @brief Macros for compatibility with Qt logging system
 * @{
 */

// NOTE: We do NOT undef or redefine Qt's qDebug/qWarning/qCritical macros
// because it breaks Qt's internal usage in template headers like
// qrangemodel_impl.h The original Qt logging macros remain available for
// backward compatibility. For spdlog integration, use the spd_q* alternatives
// or LOG_* macros below.

/** @brief Alternative debug macro with spdlog prefix */
#define spd_qDebug() spdlogDebug()
/** @brief Alternative info macro with spdlog prefix */
#define spd_qInfo() spdlogInfo()
/** @brief Alternative warning macro with spdlog prefix */
#define spd_qWarning() spdlogWarning()
/** @brief Alternative critical macro with spdlog prefix */
#define spd_qCritical() spdlogCritical()

/** @} */

// ============================================================================
// Utility Classes for Advanced Logging
// ============================================================================

/**
 * @class PerformanceLogger
 * @brief RAII performance logger for measuring function/scope execution time
 *
 * This class automatically measures execution time of a scope and logs
 * the result when the object is destroyed. It supports checkpoints and
 * threshold-based logging.
 */
class PerformanceLogger {
public:
    /**
     * @brief Construct a new Performance Logger object
     * @param name Name identifier for the performance measurement
     * @param file Source file name (optional)
     * @param line Source line number (optional)
     */
    explicit PerformanceLogger(const QString& name, const char* file = nullptr,
                               int line = 0);

    /**
     * @brief Destroy the Performance Logger object and log execution time
     */
    ~PerformanceLogger();

    // Non-copyable and non-movable for safety
    PerformanceLogger(const PerformanceLogger&) = delete;
    PerformanceLogger& operator=(const PerformanceLogger&) = delete;
    PerformanceLogger(PerformanceLogger&&) = delete;
    PerformanceLogger& operator=(PerformanceLogger&&) = delete;

    /**
     * @brief Add a checkpoint with optional description
     * @param description Description of the checkpoint
     */
    void checkpoint(const QString& description = "");

    /**
     * @brief Set minimum threshold for logging (only log if time exceeds
     * threshold)
     * @param milliseconds Threshold in milliseconds
     */
    void setThreshold(int milliseconds);

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};

// Forward declaration for LoggingManager
class LoggingManager;

/**
 * @class ScopedLogLevel
 * @brief Enhanced scoped logging configuration changer that can handle both
 * level-only and full configuration changes
 *
 * This class provides RAII-style temporary changes to logging configuration.
 * It can either change just the log level or the entire logging configuration,
 * and automatically restores the original settings when destroyed.
 */
class ScopedLogLevel {
public:
    /**
     * @brief Construct a scoped log level changer (level-only mode)
     * @param tempLevel Temporary log level to set
     */
    explicit ScopedLogLevel(Logger::LogLevel tempLevel);

    /**
     * @brief Construct a scoped configuration changer (full configuration mode)
     * @param tempConfig Temporary logging configuration to set
     */
    explicit ScopedLogLevel(
        const LoggingManager::LoggingConfiguration& tempConfig);

    /**
     * @brief Destroy the scoped changer and restore original configuration
     */
    ~ScopedLogLevel();

    // Non-copyable and non-movable for safety
    ScopedLogLevel(const ScopedLogLevel&) = delete;
    ScopedLogLevel& operator=(const ScopedLogLevel&) = delete;
    ScopedLogLevel(ScopedLogLevel&&) = delete;
    ScopedLogLevel& operator=(ScopedLogLevel&&) = delete;

private:
    LoggingManager::LoggingConfiguration
        m_originalConfig;  ///< Original configuration to restore
    bool m_levelOnly;  ///< True if only changing level, false if changing full
                       ///< config
};

/**
 * @class MemoryLogger
 * @brief Utility class for logging memory usage information
 */
class MemoryLogger {
public:
    /**
     * @brief Log current memory usage
     * @param context Context description for the log message
     */
    static void logCurrentUsage(const QString& context = "");

    /**
     * @brief Log memory usage delta since last call
     * @param context Context description for the log message
     */
    static void logMemoryDelta(const QString& context = "");

    /**
     * @brief Start memory tracking for a specific context
     * @param context Context identifier for tracking
     */
    static void startMemoryTracking(const QString& context = "");

    /**
     * @brief End memory tracking and log the difference
     * @param context Context identifier for tracking (must match
     * startMemoryTracking)
     */
    static void endMemoryTracking(const QString& context = "");

private:
    /**
     * @brief Get current memory usage in bytes
     * @return qint64 Current memory usage
     */
    static qint64 getCurrentMemoryUsage();

    static QHash<QString, qint64>
        s_memoryBaselines;  ///< Memory baselines for tracking
};

// ============================================================================
// Convenience Macros for Common Patterns
// ============================================================================

/**
 * @defgroup ConvenienceMacros Convenience Macros
 * @brief Macros for common logging patterns and error handling
 * @{
 */

/**
 * @brief Check for null pointer and log error if null, then return
 * @param ptr Pointer to check
 * @param message Error message to log
 */
#define LOG_NULL_CHECK(ptr, message)                                        \
    do {                                                                    \
        if (!(ptr)) {                                                       \
            LOG_ERROR("Null pointer check failed: {} - {}", #ptr, message); \
            return;                                                         \
        }                                                                   \
    } while (0)

/**
 * @brief Check for null pointer and log error if null, then return specified
 * value
 * @param ptr Pointer to check
 * @param message Error message to log
 * @param retval Value to return if pointer is null
 */
#define LOG_NULL_CHECK_RET(ptr, message, retval)                            \
    do {                                                                    \
        if (!(ptr)) {                                                       \
            LOG_ERROR("Null pointer check failed: {} - {}", #ptr, message); \
            return retval;                                                  \
        }                                                                   \
    } while (0)

/**
 * @brief Log error and return if condition is true
 * @param condition Condition to check
 * @param message Error message to log
 * @param retval Value to return if condition is true
 */
#define LOG_ERROR_AND_RETURN(condition, message, retval)                \
    do {                                                                \
        if (condition) {                                                \
            LOG_ERROR("Error condition: {} - {}", #condition, message); \
            return retval;                                              \
        }                                                               \
    } while (0)

/**
 * @brief Log operation result (success or failure)
 * @param operation Operation expression to evaluate
 * @param success_msg Message to log on success
 * @param error_msg Message to log on failure
 */
#define LOG_OPERATION_RESULT(operation, success_msg, error_msg)                \
    do {                                                                       \
        if (operation) {                                                       \
            LOG_INFO("Operation succeeded: {} - {}", #operation, success_msg); \
        } else {                                                               \
            LOG_ERROR("Operation failed: {} - {}", #operation, error_msg);     \
        }                                                                      \
    } while (0)

/** @} */

// ============================================================================
// Thread-Safe Logging Helpers
// ============================================================================

/**
 * @defgroup ThreadLogging Thread-Safe Logging Helpers
 * @brief Macros for thread-aware logging
 * @{
 */

/** @brief Log current thread ID */
#define LOG_THREAD_ID()        \
    LOG_DEBUG("Thread ID: {}", \
              reinterpret_cast<quintptr>(QThread::currentThreadId()))

/**
 * @brief Log message with thread ID prefix
 * @param level Log level (DEBUG, INFO, WARNING, ERROR, CRITICAL)
 * @param ... Format string and arguments
 */
#define LOG_WITH_THREAD(level, ...)                                     \
    LOG_##level("[Thread:{}] {}",                                       \
                reinterpret_cast<quintptr>(QThread::currentThreadId()), \
                fmt::format(__VA_ARGS__))

/** @} */

// ============================================================================
// File and Line Information Macros
// ============================================================================

/**
 * @defgroup LocationMacros Location Information Macros
 * @brief Macros that include file and line information in logs
 * @{
 */

/** @brief Log current execution point (file and line) */
#define LOG_HERE() LOG_DEBUG("Execution point: {}:{}", __FILE__, __LINE__)

/**
 * @brief Log debug message with file and line information
 * @param ... Format string and arguments
 */
#define LOG_DEBUG_HERE(...) \
    LOG_DEBUG("{}:{} - {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))

/**
 * @brief Log error message with file and line information
 * @param ... Format string and arguments
 */
#define LOG_ERROR_HERE(...) \
    LOG_ERROR("{}:{} - {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__))

/** @} */

// ============================================================================
// Migration Helper Macros
// ============================================================================

/**
 * @defgroup MigrationHelpers Migration Helper Macros
 * @brief Macros to assist with migration from Qt logging to spdlog
 * @{
 */

/** @brief Disable Qt logging temporarily during migration */
#define DISABLE_QT_LOGGING_TEMPORARILY()                                  \
    static bool _qt_logging_disabled = []() {                             \
        qInstallMessageHandler(                                           \
            [](QtMsgType, const QMessageLogContext&, const QString&) {}); \
        return true;                                                      \
    }()

/** @} */
