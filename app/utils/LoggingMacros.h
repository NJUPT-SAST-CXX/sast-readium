#pragma once

#include <spdlog/fmt/fmt.h>
#include <QDebug>
#include <QString>
#include "Logger.h"
#include "LoggingManager.h"

/**
 * @file LoggingMacros.h
 * @brief Comprehensive logging macros for seamless migration from Qt logging to
 * spdlog
 *
 * This file provides drop-in replacement macros for Qt's logging functions
 * (qDebug, qWarning, qCritical, etc.) while using spdlog as the backend.
 * It supports both immediate migration and gradual transition approaches.
 */

// ============================================================================
// Core Logging Macros (spdlog-style with format strings)
// ============================================================================
// Note: These macros are commented out to avoid conflicts with spdlog's own
// macros Use the LOG_* macros below instead, or call Logger methods directly

/*
#define SPDLOG_TRACE(...)    Logger::instance().trace(__VA_ARGS__)
#define SPDLOG_DEBUG(...)    Logger::instance().debug(__VA_ARGS__)
#define SPDLOG_INFO(...)     Logger::instance().info(__VA_ARGS__)
#define SPDLOG_WARNING(...)  Logger::instance().warning(__VA_ARGS__)
#define SPDLOG_ERROR(...)    Logger::instance().error(__VA_ARGS__)
#define SPDLOG_CRITICAL(...) Logger::instance().critical(__VA_ARGS__)
*/

// Short aliases
#define LOG_T(...) Logger::instance().trace(__VA_ARGS__)
#define LOG_D(...) Logger::instance().debug(__VA_ARGS__)
#define LOG_I(...) Logger::instance().info(__VA_ARGS__)
#define LOG_W(...) Logger::instance().warning(__VA_ARGS__)
#define LOG_E(...) Logger::instance().error(__VA_ARGS__)
#define LOG_C(...) Logger::instance().critical(__VA_ARGS__)

// Full name aliases for easier migration
#define LOG_TRACE(...) Logger::instance().trace(__VA_ARGS__)
#define LOG_DEBUG(...) Logger::instance().debug(__VA_ARGS__)
#define LOG_INFO(...) Logger::instance().info(__VA_ARGS__)
#define LOG_WARNING(...) Logger::instance().warning(__VA_ARGS__)
#define LOG_ERROR(...) Logger::instance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) Logger::instance().critical(__VA_ARGS__)

// ============================================================================
// Qt-Style Streaming Macros (for gradual migration)
// ============================================================================

#define LOG_DEBUG_STREAM() spdlogDebug()
#define LOG_INFO_STREAM() spdlogInfo()
#define LOG_WARNING_STREAM() spdlogWarning()
#define LOG_CRITICAL_STREAM() spdlogCritical()

// ============================================================================
// Conditional Logging Macros
// ============================================================================

#define LOG_IF(condition, level, ...) \
    do {                              \
        if (condition) {              \
            LOG_##level(__VA_ARGS__); \
        }                             \
    } while (0)

#define LOG_DEBUG_IF(condition, ...) LOG_IF(condition, DEBUG, __VA_ARGS__)
#define LOG_INFO_IF(condition, ...) LOG_IF(condition, INFO, __VA_ARGS__)
#define LOG_WARNING_IF(condition, ...) LOG_IF(condition, WARNING, __VA_ARGS__)
#define LOG_ERROR_IF(condition, ...) LOG_IF(condition, ERROR, __VA_ARGS__)
#define LOG_CRITICAL_IF(condition, ...) LOG_IF(condition, CRITICAL, __VA_ARGS__)

// ============================================================================
// Category-Based Logging (QLoggingCategory replacement)
// ============================================================================

#define DECLARE_LOG_CATEGORY(name) extern const char* const LOG_CAT_##name

#define DEFINE_LOG_CATEGORY(name, string_name) \
    const char* const LOG_CAT_##name = string_name

#define LOG_CATEGORY_DEBUG(category, ...)                                   \
    do {                                                                    \
        if (LoggingManager::instance().getLoggingCategoryLevel(category) <= \
            Logger::LogLevel::Debug) {                                      \
            Logger::instance().debug("[{}] " fmt::format(__VA_ARGS__),      \
                                     category);                             \
        }                                                                   \
    } while (0)

#define LOG_CATEGORY_INFO(category, ...)                                    \
    do {                                                                    \
        if (LoggingManager::instance().getLoggingCategoryLevel(category) <= \
            Logger::LogLevel::Info) {                                       \
            Logger::instance().info("[{}] " fmt::format(__VA_ARGS__),       \
                                    category);                              \
        }                                                                   \
    } while (0)

#define LOG_CATEGORY_WARNING(category, ...)                                 \
    do {                                                                    \
        if (LoggingManager::instance().getLoggingCategoryLevel(category) <= \
            Logger::LogLevel::Warning) {                                    \
            Logger::instance().warning("[{}] " fmt::format(__VA_ARGS__),    \
                                       category);                           \
        }                                                                   \
    } while (0)

#define LOG_CATEGORY_ERROR(category, ...)                                   \
    do {                                                                    \
        if (LoggingManager::instance().getLoggingCategoryLevel(category) <= \
            Logger::LogLevel::Error) {                                      \
            Logger::instance().error("[{}] " fmt::format(__VA_ARGS__),      \
                                     category);                             \
        }                                                                   \
    } while (0)

// ============================================================================
// Performance Logging Macros
// ============================================================================

#define LOG_PERFORMANCE_START(name) \
    auto _perf_start_##name = std::chrono::high_resolution_clock::now()

#define LOG_PERFORMANCE_END(name, ...)                              \
    do {                                                            \
        auto _perf_end = std::chrono::high_resolution_clock::now(); \
        auto _perf_duration =                                       \
            std::chrono::duration_cast<std::chrono::milliseconds>(  \
                _perf_end - _perf_start_##name)                     \
                .count();                                           \
        LOG_DEBUG("Performance [{}]: {}ms - " __VA_ARGS__, #name,   \
                  _perf_duration);                                  \
    } while (0)

#define LOG_FUNCTION_ENTRY() LOG_TRACE("Entering function: {}", __FUNCTION__)
#define LOG_FUNCTION_EXIT() LOG_TRACE("Exiting function: {}", __FUNCTION__)

// RAII performance logger
#define LOG_PERFORMANCE_SCOPE(name) \
    PerformanceLogger _perf_logger(name, __FILE__, __LINE__)

// ============================================================================
// Debug-Only Logging Macros
// ============================================================================

#ifdef QT_DEBUG
#define LOG_DEBUG_ONLY(...) LOG_DEBUG(__VA_ARGS__)
#define LOG_TRACE_ONLY(...) LOG_TRACE(__VA_ARGS__)
#else
#define LOG_DEBUG_ONLY(...) \
    do {                    \
    } while (0)
#define LOG_TRACE_ONLY(...) \
    do {                    \
    } while (0)
#endif

// ============================================================================
// Qt Compatibility Macros (Gradual Migration Option)
// ============================================================================

// Option 1: Immediate replacement (uncomment to use)
/*
#ifdef qDebug
#undef qDebug
#endif
#ifdef qInfo
#undef qInfo
#endif
#ifdef qWarning
#undef qWarning
#endif
#ifdef qCritical
#undef qCritical
#endif

#define qDebug()    spdlogDebug()
#define qInfo()     spdlogInfo()
#define qWarning()  spdlogWarning()
#define qCritical() spdlogCritical()
*/

// Option 2: Side-by-side macros (default for gradual migration)
#define spd_qDebug() spdlogDebug()
#define spd_qInfo() spdlogInfo()
#define spd_qWarning() spdlogWarning()
#define spd_qCritical() spdlogCritical()

// ============================================================================
// Utility Classes for Advanced Logging
// ============================================================================

/**
 * @brief RAII performance logger for measuring function/scope execution time
 */
class PerformanceLogger {
public:
    PerformanceLogger(const QString& name, const char* file = nullptr,
                      int line = 0);
    ~PerformanceLogger();

    void checkpoint(const QString& description = "");
    void setThreshold(int milliseconds) { m_thresholdMs = milliseconds; }

private:
    QString m_name;
    QString m_location;
    std::chrono::high_resolution_clock::time_point m_startTime;
    int m_thresholdMs = 0;  // Only log if execution time exceeds threshold
};

/**
 * @brief Scoped log level changer
 */
class ScopedLogLevel {
public:
    explicit ScopedLogLevel(Logger::LogLevel tempLevel);
    ~ScopedLogLevel();

private:
    Logger::LogLevel m_originalLevel;
};

/**
 * @brief Memory usage logger
 */
class MemoryLogger {
public:
    static void logCurrentUsage(const QString& context = "");
    static void logMemoryDelta(const QString& context = "");
    static void startMemoryTracking(const QString& context = "");
    static void endMemoryTracking(const QString& context = "");

private:
    static qint64 getCurrentMemoryUsage();
    static QHash<QString, qint64> s_memoryBaselines;
};

// ============================================================================
// Convenience Macros for Common Patterns
// ============================================================================

// Null pointer checks with logging
#define LOG_NULL_CHECK(ptr, message)                                        \
    do {                                                                    \
        if (!(ptr)) {                                                       \
            LOG_ERROR("Null pointer check failed: {} - {}", #ptr, message); \
            return;                                                         \
        }                                                                   \
    } while (0)

#define LOG_NULL_CHECK_RET(ptr, message, retval)                            \
    do {                                                                    \
        if (!(ptr)) {                                                       \
            LOG_ERROR("Null pointer check failed: {} - {}", #ptr, message); \
            return retval;                                                  \
        }                                                                   \
    } while (0)

// Error condition logging
#define LOG_ERROR_AND_RETURN(condition, message, retval)                \
    do {                                                                \
        if (condition) {                                                \
            LOG_ERROR("Error condition: {} - {}", #condition, message); \
            return retval;                                              \
        }                                                               \
    } while (0)

// Success/failure logging for operations
#define LOG_OPERATION_RESULT(operation, success_msg, error_msg)                \
    do {                                                                       \
        if (operation) {                                                       \
            LOG_INFO("Operation succeeded: {} - {}", #operation, success_msg); \
        } else {                                                               \
            LOG_ERROR("Operation failed: {} - {}", #operation, error_msg);     \
        }                                                                      \
    } while (0)

// ============================================================================
// Thread-Safe Logging Helpers
// ============================================================================

#define LOG_THREAD_ID() LOG_DEBUG("Thread ID: {}", QThread::currentThreadId())

#define LOG_WITH_THREAD(level, ...) \
    LOG_##level("[Thread:{}] " __VA_ARGS__, QThread::currentThreadId())

// ============================================================================
// File and Line Information Macros
// ============================================================================

#define LOG_HERE() LOG_DEBUG("Execution point: {}:{}", __FILE__, __LINE__)

#define LOG_DEBUG_HERE(...) \
    LOG_DEBUG("{}:{} - " __VA_ARGS__, __FILE__, __LINE__)

#define LOG_ERROR_HERE(...) \
    LOG_ERROR("{}:{} - " __VA_ARGS__, __FILE__, __LINE__)

// ============================================================================
// Migration Helper Macros
// ============================================================================

// For marking code that needs migration
#define TODO_MIGRATE_LOGGING() \
    LOG_WARNING("TODO: Migrate logging in {}:{}", __FILE__, __LINE__)

// For temporarily disabling Qt logging during migration
#define DISABLE_QT_LOGGING_TEMPORARILY()                                  \
    static bool _qt_logging_disabled = []() {                             \
        qInstallMessageHandler(                                           \
            [](QtMsgType, const QMessageLogContext&, const QString&) {}); \
        return true;                                                      \
    }()
