#include "Logger.h"
#include <fmt/format.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/qt_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <QApplication>
#include <QDir>
#include <QMutex>
#include <QMutexLocker>
#include <QRecursiveMutex>
#include <QStandardPaths>
#include <QTextEdit>
#include <iostream>
#include <string>
#include <vector>
#include "LoggingConfig.h"

// Logger Implementation class
class Logger::Implementation {
public:
    Implementation() = default;
    ~Implementation() = default;

    // Private data members
    std::shared_ptr<spdlog::logger> logger;
    std::vector<std::shared_ptr<spdlog::sinks::sink>> sinks;
    Logger::LoggerConfig config;
    QTextEdit* qtWidget = nullptr;
    // CRITICAL FIX: Use QRecursiveMutex instead of QMutex to allow same thread to lock multiple times
    // This prevents deadlock when initialize() calls methods that also try to lock
    mutable QRecursiveMutex mutex;
    bool initialized = false;

    // Private methods
    void createLogger();
    static spdlog::level::level_enum toSpdlogLevel(Logger::LogLevel level);
    static Logger::LogLevel fromSpdlogLevel(spdlog::level::level_enum level);
    static Logger::LoggerConfig convertFromLoggingConfig(
        const LoggingConfig& modernConfig);
};

Logger::Logger() : d(std::make_unique<Implementation>()) {}

Logger::~Logger() {
    // Ensure proper cleanup of spdlog resources
    if (d && d->initialized) {
        try {
            // Flush all pending log messages
            if (d->logger) {
                d->logger->flush();
            }
            // Note: spdlog::shutdown() should be called by LoggingManager
            // to avoid double-shutdown issues
        } catch (...) {
            // Suppress exceptions in destructor
        }
    }
}

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::initialize(const LoggerConfig& config) {
    QMutexLocker locker(&d->mutex);

    if (d->initialized) {
        return;  // Already initialized
    }

    d->config = config;
    d->sinks.clear();

    try {
        // Add console sink if enabled
        if (d->config.enableConsole) {
            addConsoleSink();
        }

        // Add file sink if enabled
        if (d->config.enableFile) {
            // Create logs directory if it doesn't exist
            QString logDir = QStandardPaths::writableLocation(
                                 QStandardPaths::AppDataLocation) +
                             "/logs";
            QDir().mkpath(logDir);
            QString logPath = logDir + "/" + d->config.logFileName;
            addRotatingFileSink(logPath, d->config.maxFileSize,
                                d->config.maxFiles);
        }

        // Add Qt widget sink if enabled and widget is provided
        if (d->config.enableQtWidget && d->config.qtWidget) {
            addQtWidgetSink(d->config.qtWidget);
        }

        d->createLogger();
        setLogLevel(d->config.level);
        setPattern(d->config.pattern);

        d->initialized = true;

        // Log initialization success
        info("Logger initialized successfully with {} sinks", d->sinks.size());

    } catch (const std::exception& e) {
        // Fallback to console-only logging if initialization fails
        d->sinks.clear();
        addConsoleSink();
        d->createLogger();
        error(
            "Logger initialization failed: {}. Falling back to console-only "
            "logging.",
            e.what());
        d->initialized = true;
    }
}

void Logger::initialize(const LoggingConfig& config) {
    QMutexLocker locker(&d->mutex);

    if (d->initialized) {
        return;  // Already initialized
    }

    // Convert LoggingConfig to LoggerConfig and initialize
    LoggerConfig loggerConfig =
        Implementation::convertFromLoggingConfig(config);
    initialize(loggerConfig);
}

// Implementation class methods
void Logger::Implementation::createLogger() {
    if (sinks.empty()) {
        // This should not happen as we ensure at least console sink exists
        return;
    }

    // Check if logger already exists and drop it to avoid registration
    // conflicts
    auto existing = spdlog::get("sast-readium");
    if (existing) {
        spdlog::drop("sast-readium");
    }

    logger = std::make_shared<spdlog::logger>("sast-readium", sinks.begin(),
                                              sinks.end());
    logger->set_level(spdlog::level::trace);  // Set to lowest level, actual
                                              // filtering done per sink
    logger->flush_on(spdlog::level::warn);  // Auto-flush on warnings and errors

    // Register with spdlog
    try {
        spdlog::register_logger(logger);
    } catch (const spdlog::spdlog_ex& ex) {
        // Logger might already be registered, which is fine
        // Just use the existing logger instance
        Q_UNUSED(ex);
    }
}

void Logger::setLogLevel(LogLevel level) {
    // FIXED: Now using QRecursiveMutex, so this is safe even when called from initialize()
    QMutexLocker locker(&d->mutex);
    d->config.level = level;

    if (d->logger) {
        d->logger->set_level(Implementation::toSpdlogLevel(level));
    }
}

void Logger::setPattern(const QString& pattern) {
    // FIXED: Now using QRecursiveMutex, so this is safe even when called from initialize()
    QMutexLocker locker(&d->mutex);
    d->config.pattern = pattern;

    if (d->logger) {
        d->logger->set_pattern(pattern.toStdString());
    }
}

void Logger::addConsoleSink() {
    // FIXED: Now using QRecursiveMutex, so this is safe even when called from initialize()
    QMutexLocker locker(&d->mutex);
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(Implementation::toSpdlogLevel(d->config.level));
    d->sinks.push_back(console_sink);
}

void Logger::addFileSink(const QString& filename) {
    // FIXED: Now using QRecursiveMutex, so this is safe even when called from initialize()
    QMutexLocker locker(&d->mutex);
    try {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            filename.toStdString(), true);
        file_sink->set_level(Implementation::toSpdlogLevel(d->config.level));
        d->sinks.push_back(file_sink);
    } catch (const std::exception& e) {
        // If we can't create file sink, log error to console
        if (d->logger) {
            error("Failed to create file sink '{}': {}", filename.toStdString(),
                  e.what());
        }
    }
}

void Logger::addRotatingFileSink(const QString& filename, size_t maxSize,
                                 size_t maxFiles) {
    // FIXED: Now using QRecursiveMutex, so this is safe even when called from initialize()
    QMutexLocker locker(&d->mutex);
    try {
        auto rotating_sink =
            std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                filename.toStdString(), maxSize, maxFiles);
        rotating_sink->set_level(
            Implementation::toSpdlogLevel(d->config.level));
        d->sinks.push_back(rotating_sink);
    } catch (const std::exception& e) {
        // If we can't create rotating file sink, log error to console
        if (d->logger) {
            error("Failed to create rotating file sink '{}': {}",
                  filename.toStdString(), e.what());
        }
    }
}

void Logger::addQtWidgetSink(QTextEdit* widget) {
    if (widget == nullptr) {
        return;
    }

    // FIXED: Now using QRecursiveMutex, so this is safe even when called from initialize()
    QMutexLocker locker(&d->mutex);
    d->qtWidget = widget;

    try {
        // qt_sink requires QObject* and meta method name (signal name)
        // The second parameter should be the name of a Qt signal that accepts a
        // QString
        auto qt_sink =
            std::make_shared<spdlog::sinks::qt_sink_mt>(widget, "append");
        qt_sink->set_level(Implementation::toSpdlogLevel(d->config.level));
        d->sinks.push_back(qt_sink);

        // Connect Qt signal for additional processing if needed
        connect(this, &Logger::logMessage, this,
                [](const QString& message, int level) {
                    // Additional Qt-specific processing can be added here
                    Q_UNUSED(message)
                    Q_UNUSED(level)
                });

    } catch (const std::exception& e) {
        if (d->logger) {
            error("Failed to create Qt widget sink: {}", e.what());
        }
    }
}

void Logger::setQtWidget(QTextEdit* widget) {
    QMutexLocker locker(&d->mutex);

    if (d->qtWidget == widget) {
        return;  // Same widget, nothing to do
    }

    // Remove existing Qt widget sink if any
    removeSink(SinkType::QtWidget);

    if (widget != nullptr) {
        addQtWidgetSink(widget);

        // Recreate logger with new sinks
        if (d->initialized) {
            d->createLogger();
            setLogLevel(d->config.level);
            setPattern(d->config.pattern);
        }
    }
}

QTextEdit* Logger::getQtWidget() const { return d->qtWidget; }

void Logger::removeSink(SinkType type) {
    // Implementation for removing specific sink types
    // This is a simplified version - in practice, you'd need to track sink
    // types
    switch (type) {
        case SinkType::QtWidget:
            d->qtWidget = nullptr;
            break;
        default:
            break;
    }
}

spdlog::level::level_enum Logger::Implementation::toSpdlogLevel(
    Logger::LogLevel level) {
    switch (level) {
        case Logger::LogLevel::Trace:
            return spdlog::level::trace;
        case Logger::LogLevel::Debug:
            return spdlog::level::debug;
        case Logger::LogLevel::Info:
            return spdlog::level::info;
        case Logger::LogLevel::Warning:
            return spdlog::level::warn;
        case Logger::LogLevel::Error:
            return spdlog::level::err;
        case Logger::LogLevel::Critical:
            return spdlog::level::critical;
        case Logger::LogLevel::Off:
            return spdlog::level::off;
        default:
            return spdlog::level::info;
    }
}

Logger::LogLevel Logger::Implementation::fromSpdlogLevel(
    spdlog::level::level_enum level) {
    switch (level) {
        case spdlog::level::trace:
            return Logger::LogLevel::Trace;
        case spdlog::level::debug:
            return Logger::LogLevel::Debug;
        case spdlog::level::info:
            return Logger::LogLevel::Info;
        case spdlog::level::warn:
            return Logger::LogLevel::Warning;
        case spdlog::level::err:
            return Logger::LogLevel::Error;
        case spdlog::level::critical:
            return Logger::LogLevel::Critical;
        case spdlog::level::off:
            return Logger::LogLevel::Off;
        default:
            return Logger::LogLevel::Info;
    }
}

// Simple string logging methods for Qt-style compatibility
void Logger::trace(const QString& message) {
    QMutexLocker locker(&d->mutex);
    if (d->logger) {
        d->logger->trace(message.toStdString());
    }
}

void Logger::debug(const QString& message) {
    QMutexLocker locker(&d->mutex);
    if (d->logger) {
        d->logger->debug(message.toStdString());
    }
}

void Logger::info(const QString& message) {
    QMutexLocker locker(&d->mutex);
    if (d->logger) {
        d->logger->info(message.toStdString());
    }
}

void Logger::warning(const QString& message) {
    QMutexLocker locker(&d->mutex);
    if (d->logger) {
        d->logger->warn(message.toStdString());
    }
}

void Logger::error(const QString& message) {
    QMutexLocker locker(&d->mutex);
    if (d->logger) {
        d->logger->error(message.toStdString());
    }
}

void Logger::critical(const QString& message) {
    QMutexLocker locker(&d->mutex);
    if (d->logger) {
        d->logger->critical(message.toStdString());
    }
}

std::shared_ptr<spdlog::logger> Logger::getSpdlogLogger() const {
    QMutexLocker locker(&d->mutex);
    return d->logger;
}

Logger::LoggerConfig Logger::Implementation::convertFromLoggingConfig(
    const LoggingConfig& modernConfig) {
    LoggerConfig loggerConfig;

    // Convert global configuration
    const auto& globalConfig = modernConfig.getGlobalConfig();
    loggerConfig.level = globalConfig.globalLevel;
    loggerConfig.pattern = globalConfig.globalPattern;

    // Convert sink configurations
    const auto& sinkConfigs = modernConfig.getSinkConfigurations();

    // Initialize all sink types as disabled
    loggerConfig.enableConsole = false;
    loggerConfig.enableFile = false;
    loggerConfig.enableQtWidget = false;
    loggerConfig.qtWidget = nullptr;

    for (const auto& sink : sinkConfigs) {
        if (!sink.enabled) {
            continue;
        }

        if (sink.type == "console") {
            loggerConfig.enableConsole = true;
        } else if (sink.type == "rotating_file" || sink.type == "file") {
            loggerConfig.enableFile = true;
            loggerConfig.logFileName = sink.filename;
            loggerConfig.maxFileSize = sink.maxFileSize;
            loggerConfig.maxFiles = sink.maxFiles;
        } else if (sink.type == "qt_widget") {
            loggerConfig.enableQtWidget = true;
            // Note: qtWidget pointer would need to be set separately
            // as LoggingConfig doesn't store widget pointers directly
        }
    }

    return loggerConfig;
}
