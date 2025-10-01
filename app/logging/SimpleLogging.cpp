/**
 * @file SimpleLogging.cpp
 * @brief Implementation of simplified logging interface
 * @author SAST Readium Project
 * @version 2.0
 * @date 2025-09-12
 */

#include "SimpleLogging.h"
#include "Logger.h"
#include "LoggingManager.h"
#include "LoggingConfig.h"
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <chrono>
#include <spdlog/fmt/fmt.h>

namespace SastLogging {

// Implementation classes for PIMPL pattern
// Note: CategoryLogger::Implementation is now defined in the header for inline template access

class ScopedLevel::Implementation
{
public:
    explicit Implementation(Level originalLevel) : originalLevel(originalLevel) {}
    ~Implementation() = default;

    Level originalLevel;
};

class ScopedSilence::Implementation
{
public:
    explicit Implementation(Level originalLevel) : originalLevel(originalLevel) {}
    ~Implementation() = default;

    Level originalLevel;
};

// ============================================================================
// Internal State Management
// ============================================================================

namespace {
    QString g_lastError;
    
    Logger::LogLevel convertToLoggerLevel(Level level) {
        switch (level) {
            case Level::Trace: return Logger::LogLevel::Trace;
            case Level::Debug: return Logger::LogLevel::Debug;
            case Level::Info: return Logger::LogLevel::Info;
            case Level::Warning: return Logger::LogLevel::Warning;
            case Level::Error: return Logger::LogLevel::Error;
            case Level::Critical: return Logger::LogLevel::Critical;
            case Level::Off: return Logger::LogLevel::Off;
            default: return Logger::LogLevel::Info;
        }
    }
    
    Level convertFromLoggerLevel(Logger::LogLevel level) {
        switch (level) {
            case Logger::LogLevel::Trace: return Level::Trace;
            case Logger::LogLevel::Debug: return Level::Debug;
            case Logger::LogLevel::Info: return Level::Info;
            case Logger::LogLevel::Warning: return Level::Warning;
            case Logger::LogLevel::Error: return Level::Error;
            case Logger::LogLevel::Critical: return Level::Critical;
            case Logger::LogLevel::Off: return Level::Off;
            default: return Level::Info;
        }
    }
}

// ============================================================================
// Core Logging Functions Implementation
// ============================================================================

bool init() {
    try {
        LoggingManager::instance().initialize();
        g_lastError.clear();
        return true;
    } catch (const std::exception& e) {
        g_lastError = QString::fromStdString(e.what());
        return false;
    }
}

bool init(const QString& logFile, bool consoleEnabled, Level level) {
    try {
        LoggingManager::LoggingConfiguration config;
        config.globalLogLevel = convertToLoggerLevel(level);
        config.enableConsoleLogging = consoleEnabled;
        config.enableFileLogging = !logFile.isEmpty();
        config.logFileName = logFile.isEmpty() ? "sast-readium.log" : logFile;
        
        LoggingManager::instance().initialize(config);
        g_lastError.clear();
        return true;
    } catch (const std::exception& e) {
        g_lastError = QString::fromStdString(e.what());
        return false;
    }
}

bool init(const Config& config) {
    try {
        LoggingManager::LoggingConfiguration mgmtConfig;
        mgmtConfig.globalLogLevel = convertToLoggerLevel(config.level);
        mgmtConfig.logPattern = config.pattern;
        mgmtConfig.enableConsoleLogging = config.console;
        mgmtConfig.enableFileLogging = config.file;
        mgmtConfig.logFileName = config.logFile.isEmpty() ? "sast-readium.log" : config.logFile;
        mgmtConfig.logDirectory = config.logDir;
        mgmtConfig.maxFileSize = config.maxFileSize;
        mgmtConfig.maxFiles = config.maxFiles;
        mgmtConfig.enableAsyncLogging = config.async;
        
        LoggingManager::instance().initialize(mgmtConfig);
        g_lastError.clear();
        return true;
    } catch (const std::exception& e) {
        g_lastError = QString::fromStdString(e.what());
        return false;
    }
}

void shutdown() {
    LoggingManager::instance().shutdown();
}

void setLevel(Level level) {
    LoggingManager::instance().setGlobalLogLevel(convertToLoggerLevel(level));
}

Level getLevel() {
    return convertFromLoggerLevel(LoggingManager::instance().getConfiguration().globalLogLevel);
}

void flush() {
    LoggingManager::instance().flushLogs();
}

// ============================================================================
// Simple Logging Functions Implementation
// ============================================================================

void trace(const QString& message) {
    Logger::instance().trace(message);
}

void debug(const QString& message) {
    Logger::instance().debug(message);
}

void info(const QString& message) {
    Logger::instance().info(message);
}

void warning(const QString& message) {
    Logger::instance().warning(message);
}

void error(const QString& message) {
    Logger::instance().error(message);
}

void critical(const QString& message) {
    Logger::instance().critical(message);
}

// ============================================================================
// Detail Namespace Implementation
// ============================================================================

namespace detail {

void logFormatted(Level level, const std::string& formatted) {
    QString message = QString::fromStdString(formatted);
    switch (level) {
        case Level::Trace: trace(message); break;
        case Level::Debug: debug(message); break;
        case Level::Info: info(message); break;
        case Level::Warning: warning(message); break;
        case Level::Error: error(message); break;
        case Level::Critical: critical(message); break;
        default: break;
    }
}

std::string formatString(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Get required buffer size
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(nullptr, 0, format, args_copy) + 1;
    va_end(args_copy);
    
    // Format the string
    std::vector<char> buffer(size);
    vsnprintf(buffer.data(), size, format, args);
    va_end(args);
    
    return std::string(buffer.data());
}

} // namespace detail

// ============================================================================
// CategoryLogger Implementation
// ============================================================================

CategoryLogger::CategoryLogger(const QString& category)
    : d(std::make_unique<Implementation>(category)) {
    LoggingManager::instance().addLoggingCategory(category);
}

void CategoryLogger::trace(const QString& message) {
    if (d->level <= Level::Trace) {
        Logger::instance().trace("[{}] {}", d->category.toStdString(), message.toStdString());
    }
}

void CategoryLogger::debug(const QString& message) {
    if (d->level <= Level::Debug) {
        Logger::instance().debug("[{}] {}", d->category.toStdString(), message.toStdString());
    }
}

void CategoryLogger::info(const QString& message) {
    if (d->level <= Level::Info) {
        Logger::instance().info("[{}] {}", d->category.toStdString(), message.toStdString());
    }
}

void CategoryLogger::warning(const QString& message) {
    if (d->level <= Level::Warning) {
        Logger::instance().warning("[{}] {}", d->category.toStdString(), message.toStdString());
    }
}

void CategoryLogger::error(const QString& message) {
    if (d->level <= Level::Error) {
        Logger::instance().error("[{}] {}", d->category.toStdString(), message.toStdString());
    }
}

void CategoryLogger::critical(const QString& message) {
    if (d->level <= Level::Critical) {
        Logger::instance().critical("[{}] {}", d->category.toStdString(), message.toStdString());
    }
}

void CategoryLogger::setLevel(Level level) {
    d->level = level;
    LoggingManager::instance().setLoggingCategoryLevel(
        d->category, convertToLoggerLevel(level));
}

Level CategoryLogger::getLevel() const {
    return d->level;
}

// ============================================================================
// Timer Implementation
// ============================================================================

class Timer::Impl {
public:
    Impl(const QString& name) 
        : m_name(name),
          m_startTime(std::chrono::high_resolution_clock::now()) {
        SastLogging::debug("Timer [%s] started", name.toStdString().c_str());
    }
    
    ~Impl() {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - m_startTime).count();
        SastLogging::debug("Timer [%s] finished: %lld ms", 
                          m_name.toStdString().c_str(), duration);
    }
    
    void checkpoint(const QString& name) {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_lastCheckpoint).count();
        QString checkpointName = name.isEmpty() ? 
            QString("Checkpoint %1").arg(++m_checkpointCount) : name;
        SastLogging::debug("Timer [%s] %s: %lld ms", 
                          m_name.toStdString().c_str(),
                          checkpointName.toStdString().c_str(),
                          duration);
        m_lastCheckpoint = now;
    }
    
private:
    QString m_name;
    std::chrono::high_resolution_clock::time_point m_startTime;
    std::chrono::high_resolution_clock::time_point m_lastCheckpoint;
    int m_checkpointCount = 0;
};

Timer::Timer(const QString& name) 
    : m_impl(std::make_unique<Impl>(name)) {
}

Timer::~Timer() = default;

void Timer::checkpoint(const QString& name) {
    m_impl->checkpoint(name);
}

// ============================================================================
// ScopedLevel Implementation
// ============================================================================

ScopedLevel::ScopedLevel(Level tempLevel)
    : d(std::make_unique<Implementation>(getLevel())) {
    setLevel(tempLevel);
}

ScopedLevel::~ScopedLevel() {
    setLevel(d->originalLevel);
}

// ============================================================================
// ScopedSilence Implementation
// ============================================================================

ScopedSilence::ScopedSilence()
    : d(std::make_unique<Implementation>(getLevel())) {
    setLevel(Level::Off);
}

ScopedSilence::~ScopedSilence() {
    setLevel(d->originalLevel);
}

// ============================================================================
// File Operations Implementation
// ============================================================================

QString getCurrentLogFile() {
    return LoggingManager::instance().getCurrentLogFilePath();
}

QStringList getLogFiles() {
    return LoggingManager::instance().getLogFileList();
}

void rotateLogFiles() {
    LoggingManager::instance().rotateLogFiles();
}

qint64 getTotalLogSize() {
    return LoggingManager::instance().getTotalLogFileSize();
}

// ============================================================================
// Utility Functions Implementation
// ============================================================================

bool isInitialized() {
    return LoggingManager::instance().isInitialized();
}

QString getLastError() {
    return g_lastError;
}

void clearLastError() {
    g_lastError.clear();
}

} // namespace SastLogging
