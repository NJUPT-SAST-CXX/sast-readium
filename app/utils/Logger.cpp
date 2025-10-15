#include "Logger.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/qt_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <QApplication>
#include <QDir>
#include <QMutexLocker>
#include <QStandardPaths>

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::initialize(const LoggerConfig& config) {
    QMutexLocker locker(&m_mutex);

    if (m_initialized) {
        return;  // Already initialized
    }

    m_config = config;
    m_sinks.clear();

    try {
        // Add console sink if enabled
        if (m_config.enableConsole) {
            addConsoleSink();
        }

        // Add file sink if enabled
        if (m_config.enableFile) {
            // Create logs directory if it doesn't exist
            QString logDir = QStandardPaths::writableLocation(
                                 QStandardPaths::AppDataLocation) +
                             "/logs";
            QDir().mkpath(logDir);
            QString logPath = logDir + "/" + m_config.logFileName;
            addRotatingFileSink(logPath, m_config.maxFileSize,
                                m_config.maxFiles);
        }

        // Add Qt widget sink if enabled and widget is provided
        if (m_config.enableQtWidget && m_config.qtWidget) {
            addQtWidgetSink(m_config.qtWidget);
        }

        createLogger();
        setLogLevelInternal(m_config.level);
        setPatternInternal(m_config.pattern);

        m_initialized = true;

        // Log initialization success
        info("Logger initialized successfully with {} sinks", m_sinks.size());

    } catch (const std::exception& e) {
        // Fallback to console-only logging if initialization fails
        m_sinks.clear();
        addConsoleSink();
        createLogger();
        error(
            "Logger initialization failed: {}. Falling back to console-only "
            "logging.",
            e.what());
        m_initialized = true;
    }
}

void Logger::createLogger() {
    if (m_sinks.empty()) {
        // Ensure we have at least a console sink
        addConsoleSink();
    }

    m_logger = std::make_shared<spdlog::logger>("sast-readium", m_sinks.begin(),
                                                m_sinks.end());
    m_logger->set_level(spdlog::level::trace);  // Set to lowest level, actual
                                                // filtering done per sink
    m_logger->flush_on(
        spdlog::level::warn);  // Auto-flush on warnings and errors

    // Register with spdlog
    spdlog::register_logger(m_logger);
}

void Logger::setLogLevel(LogLevel level) {
    QMutexLocker locker(&m_mutex);
    setLogLevelInternal(level);
}

void Logger::setLogLevelInternal(LogLevel level) {
    m_config.level = level;

    if (m_logger) {
        m_logger->set_level(toSpdlogLevel(level));
    }
}

void Logger::setPattern(const QString& pattern) {
    QMutexLocker locker(&m_mutex);
    setPatternInternal(pattern);
}

void Logger::setPatternInternal(const QString& pattern) {
    m_config.pattern = pattern;

    if (m_logger) {
        m_logger->set_pattern(pattern.toStdString());
    }
}

void Logger::addConsoleSink() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(toSpdlogLevel(m_config.level));
    m_sinks.push_back(console_sink);
}

void Logger::addFileSink(const QString& filename) {
    try {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            filename.toStdString(), true);
        file_sink->set_level(toSpdlogLevel(m_config.level));
        m_sinks.push_back(file_sink);
    } catch (const std::exception& e) {
        // If we can't create file sink, log error to console
        if (m_logger) {
            error("Failed to create file sink '{}': {}", filename.toStdString(),
                  e.what());
        }
    }
}

void Logger::addRotatingFileSink(const QString& filename, size_t maxSize,
                                 size_t maxFiles) {
    try {
        auto rotating_sink =
            std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                filename.toStdString(), maxSize, maxFiles);
        rotating_sink->set_level(toSpdlogLevel(m_config.level));
        m_sinks.push_back(rotating_sink);
    } catch (const std::exception& e) {
        // If we can't create rotating file sink, log error to console
        if (m_logger) {
            error("Failed to create rotating file sink '{}': {}",
                  filename.toStdString(), e.what());
        }
    }
}

void Logger::addQtWidgetSink(QTextEdit* widget) {
    if (!widget) {
        return;
    }

    m_qtWidget = widget;

    try {
        // qt_sink requires QObject* and meta method name (signal name)
        // The second parameter should be the name of a Qt signal that accepts a
        // QString
        auto qt_sink =
            std::make_shared<spdlog::sinks::qt_sink_mt>(widget, "append");
        qt_sink->set_level(toSpdlogLevel(m_config.level));
        m_sinks.push_back(qt_sink);

        // Connect Qt signal for additional processing if needed
        connect(this, &Logger::logMessage, this,
                [this](const QString& message, int level) {
                    // Additional Qt-specific processing can be added here
                    Q_UNUSED(message)
                    Q_UNUSED(level)
                });

    } catch (const std::exception& e) {
        if (m_logger) {
            error("Failed to create Qt widget sink: {}", e.what());
        }
    }
}

void Logger::setQtWidget(QTextEdit* widget) {
    QMutexLocker locker(&m_mutex);

    if (m_qtWidget == widget) {
        return;  // Same widget, nothing to do
    }

    // Remove existing Qt widget sink if any
    removeSink(SinkType::QtWidget);

    if (widget) {
        addQtWidgetSink(widget);

        // Recreate logger with new sinks
        if (m_initialized) {
            createLogger();
            setLogLevelInternal(m_config.level);
            setPatternInternal(m_config.pattern);
        }
    }
}

void Logger::removeSink(SinkType type) {
    // Implementation for removing specific sink types
    // This is a simplified version - in practice, you'd need to track sink
    // types
    switch (type) {
        case SinkType::QtWidget:
            m_qtWidget = nullptr;
            break;
        default:
            break;
    }
}

spdlog::level::level_enum Logger::toSpdlogLevel(LogLevel level) const {
    switch (level) {
        case LogLevel::Trace:
            return spdlog::level::trace;
        case LogLevel::Debug:
            return spdlog::level::debug;
        case LogLevel::Info:
            return spdlog::level::info;
        case LogLevel::Warning:
            return spdlog::level::warn;
        case LogLevel::Error:
            return spdlog::level::err;
        case LogLevel::Critical:
            return spdlog::level::critical;
        case LogLevel::Off:
            return spdlog::level::off;
        default:
            return spdlog::level::info;
    }
}

Logger::LogLevel Logger::fromSpdlogLevel(
    spdlog::level::level_enum level) const {
    switch (level) {
        case spdlog::level::trace:
            return LogLevel::Trace;
        case spdlog::level::debug:
            return LogLevel::Debug;
        case spdlog::level::info:
            return LogLevel::Info;
        case spdlog::level::warn:
            return LogLevel::Warning;
        case spdlog::level::err:
            return LogLevel::Error;
        case spdlog::level::critical:
            return LogLevel::Critical;
        case spdlog::level::off:
            return LogLevel::Off;
        default:
            return LogLevel::Info;
    }
}

// Simple string logging methods for Qt-style compatibility
void Logger::trace(const QString& message) {
    if (m_logger) {
        m_logger->trace(message.toStdString());
    }
}

void Logger::debug(const QString& message) {
    if (m_logger) {
        m_logger->debug(message.toStdString());
    }
}

void Logger::info(const QString& message) {
    if (m_logger) {
        m_logger->info(message.toStdString());
    }
}

void Logger::warning(const QString& message) {
    if (m_logger) {
        m_logger->warn(message.toStdString());
    }
}

void Logger::error(const QString& message) {
    if (m_logger) {
        m_logger->error(message.toStdString());
    }
}

void Logger::critical(const QString& message) {
    if (m_logger) {
        m_logger->critical(message.toStdString());
    }
}
