/**
 * @file SimpleLogging.h
 * @brief Simplified logging interface for easy external usage
 * @author SAST Readium Project
 * @version 2.0
 * @date 2025-09-12
 *
 * This file provides a simplified, clean interface for logging functionality,
 * making it easy for external modules to use the logging system without
 * dealing with complex configurations.
 */

#pragma once

#include <QString>
#include <memory>
#include <string>

// Forward declarations to minimize dependencies
class Logger;
class LoggingManager;

namespace SastLogging {

// ============================================================================
// Log Levels
// ============================================================================

/**
 * @brief Simplified log level enumeration
 */
enum class Level { Trace, Debug, Info, Warning, Error, Critical, Off };

// ============================================================================
// Core Logging Functions - Simple and Direct
// ============================================================================

/**
 * @brief Initialize logging with default configuration
 * @return true if successful, false otherwise
 */
bool init();

/**
 * @brief Initialize logging with custom configuration
 * @param logFile Path to log file (empty for console only)
 * @param consoleEnabled Enable console output
 * @param level Default log level
 * @return true if successful, false otherwise
 */
bool init(const QString& logFile, bool consoleEnabled = true,
          Level level = Level::Info);

/**
 * @brief Initialize logging with detailed configuration
 */
struct Config {
    Level level = Level::Info;
    QString logFile = "";
    QString logDir = "";  // Empty means default app data location
    bool console = true;
    bool file = true;
    size_t maxFileSize = 10 * 1024 * 1024;  // 10MB
    size_t maxFiles = 5;
    bool async = false;
    QString pattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v";
};

bool init(const Config& config);

/**
 * @brief Shutdown logging system
 */
void shutdown();

/**
 * @brief Set global log level
 */
void setLevel(Level level);

/**
 * @brief Get current log level
 */
Level getLevel();

/**
 * @brief Flush all log messages
 */
void flush();

// ============================================================================
// Simple Logging Functions - Most Common Use Cases
// ============================================================================

// String-based logging (simplest interface)
void trace(const QString& message);
void debug(const QString& message);
void info(const QString& message);
void warning(const QString& message);
void error(const QString& message);
void critical(const QString& message);

// Format string logging (for convenience)
template <typename... Args>
void trace(const char* format, Args&&... args);

template <typename... Args>
void debug(const char* format, Args&&... args);

template <typename... Args>
void info(const char* format, Args&&... args);

template <typename... Args>
void warning(const char* format, Args&&... args);

template <typename... Args>
void error(const char* format, Args&&... args);

template <typename... Args>
void critical(const char* format, Args&&... args);

// ============================================================================
// Conditional Logging - Only Log When Needed
// ============================================================================

/**
 * @brief Log only if condition is true
 */
template <typename... Args>
void logIf(bool condition, Level level, const char* format, Args&&... args);

/**
 * @brief Log only in debug builds
 */
#ifdef QT_DEBUG
template <typename... Args>
void debugOnly(const char* format, Args&&... args);
#else
template <typename... Args>
inline void debugOnly(const char*, Args&&...) {}
#endif

// ============================================================================
// Category-Based Logging
// ============================================================================

/**
 * @brief Simple category logger for module-specific logging
 */
class CategoryLogger {
public:
    explicit CategoryLogger(const QString& category);
    ~CategoryLogger() = default;

    void trace(const QString& message);
    void debug(const QString& message);
    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);
    void critical(const QString& message);

    template <typename... Args>
    void log(Level level, const char* format, Args&&... args);

    void setLevel(Level level);
    Level getLevel() const;

    // Implementation class definition for inline template methods
    class Implementation {
    public:
        explicit Implementation(const QString& category)
            : category(category), level(Level::Info) {}
        ~Implementation() = default;

        QString category;
        Level level;
    };

private:
    std::unique_ptr<Implementation> d;
};

// ============================================================================
// Performance Logging - Simple Timer
// ============================================================================

/**
 * @brief Simple RAII performance timer
 */
class Timer {
public:
    explicit Timer(const QString& name);
    ~Timer();

    void checkpoint(const QString& name = "");

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// ============================================================================
// Scoped Configuration - Temporary Changes
// ============================================================================

/**
 * @brief RAII scoped log level changer
 */
class ScopedLevel {
public:
    explicit ScopedLevel(Level tempLevel);
    ~ScopedLevel();

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};

/**
 * @brief RAII scoped log suppression
 */
class ScopedSilence {
public:
    ScopedSilence();
    ~ScopedSilence();

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};

// ============================================================================
// File Operations
// ============================================================================

/**
 * @brief Get current log file path
 */
QString getCurrentLogFile();

/**
 * @brief Get all log files
 */
QStringList getLogFiles();

/**
 * @brief Rotate log files manually
 */
void rotateLogFiles();

/**
 * @brief Get total size of all log files
 */
qint64 getTotalLogSize();

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Check if logging is initialized
 */
bool isInitialized();

/**
 * @brief Get last error message if any operation failed
 */
QString getLastError();

/**
 * @brief Clear last error
 */
void clearLastError();

}  // namespace SastLogging

// ============================================================================
// Convenience Macros - Optional, for even simpler usage
// ============================================================================

// Basic logging macros
#define SLOG_TRACE(msg) SastLogging::trace(msg)
#define SLOG_DEBUG(msg) SastLogging::debug(msg)
#define SLOG_INFO(msg) SastLogging::info(msg)
#define SLOG_WARNING(msg) SastLogging::warning(msg)
#define SLOG_ERROR(msg) SastLogging::error(msg)
#define SLOG_CRITICAL(msg) SastLogging::critical(msg)

// Format string macros
#define SLOG_TRACE_F(...) SastLogging::trace(__VA_ARGS__)
#define SLOG_DEBUG_F(...) SastLogging::debug(__VA_ARGS__)
#define SLOG_INFO_F(...) SastLogging::info(__VA_ARGS__)
#define SLOG_WARNING_F(...) SastLogging::warning(__VA_ARGS__)
#define SLOG_ERROR_F(...) SastLogging::error(__VA_ARGS__)
#define SLOG_CRITICAL_F(...) SastLogging::critical(__VA_ARGS__)

// Conditional logging
#define SLOG_IF(cond, level, ...) \
    if (cond)                     \
    SastLogging::level(__VA_ARGS__)

// Performance timing
#define SLOG_TIMER(name) SastLogging::Timer _timer(name)
#define SLOG_CHECKPOINT(name) _timer.checkpoint(name)

// Scoped configuration
#define SLOG_SCOPED_LEVEL(level) SastLogging::ScopedLevel _scoped(level)
#define SLOG_SCOPED_SILENCE() SastLogging::ScopedSilence _silence

// ============================================================================
// Template Implementations
// ============================================================================

namespace SastLogging {

// Forward declaration of internal implementation
namespace detail {
void logFormatted(Level level, const std::string& formatted);
std::string formatString(const char* format, ...);
}  // namespace detail

template <typename... Args>
void trace(const char* format, Args&&... args) {
    detail::logFormatted(
        Level::Trace,
        detail::formatString(format, std::forward<Args>(args)...));
}

template <typename... Args>
void debug(const char* format, Args&&... args) {
    detail::logFormatted(
        Level::Debug,
        detail::formatString(format, std::forward<Args>(args)...));
}

template <typename... Args>
void info(const char* format, Args&&... args) {
    detail::logFormatted(
        Level::Info, detail::formatString(format, std::forward<Args>(args)...));
}

template <typename... Args>
void warning(const char* format, Args&&... args) {
    detail::logFormatted(
        Level::Warning,
        detail::formatString(format, std::forward<Args>(args)...));
}

template <typename... Args>
void error(const char* format, Args&&... args) {
    detail::logFormatted(
        Level::Error,
        detail::formatString(format, std::forward<Args>(args)...));
}

template <typename... Args>
void critical(const char* format, Args&&... args) {
    detail::logFormatted(
        Level::Critical,
        detail::formatString(format, std::forward<Args>(args)...));
}

template <typename... Args>
void logIf(bool condition, Level level, const char* format, Args&&... args) {
    if (condition) {
        detail::logFormatted(
            level, detail::formatString(format, std::forward<Args>(args)...));
    }
}

#ifdef QT_DEBUG
template <typename... Args>
void debugOnly(const char* format, Args&&... args) {
    debug(format, std::forward<Args>(args)...);
}
#endif

template <typename... Args>
void CategoryLogger::log(Level level, const char* format, Args&&... args) {
    if (level >= d->level) {
        QString msg = QString("[%1] %2")
                          .arg(d->category)
                          .arg(QString::fromStdString(detail::formatString(
                              format, std::forward<Args>(args)...)));
        detail::logFormatted(level, msg.toStdString());
    }
}

}  // namespace SastLogging
