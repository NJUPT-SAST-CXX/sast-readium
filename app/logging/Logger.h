#pragma once

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <QObject>
#include <QString>
#include <memory>

// Forward declarations to reduce header dependencies
class LoggingConfig;
class QTextEdit;

/**
 * @brief Centralized logging manager that integrates spdlog with Qt
 *
 * This class provides a unified logging interface that replaces Qt's built-in
 * logging system (qDebug, qWarning, etc.) with spdlog while maintaining
 * Qt integration through qt_sinks.
 */
class Logger : public QObject {
    Q_OBJECT

public:
    enum class LogLevel {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warning = 3,
        Error = 4,
        Critical = 5,
        Off = 6
    };

    enum class SinkType { Console, File, RotatingFile, QtWidget };

    struct LoggerConfig {
        LogLevel level;
        QString pattern;
        QString logFileName;
        size_t maxFileSize;
        size_t maxFiles;
        bool enableConsole;
        bool enableFile;
        bool enableQtWidget;
        QTextEdit* qtWidget;

        LoggerConfig(
            LogLevel level = LogLevel::Info,
            const QString& pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v",
            const QString& logFileName = "sast-readium.log",
            size_t maxFileSize = 1024 * 1024 * 10,  // 10MB
            size_t maxFiles = 3, bool enableConsole = true,
            bool enableFile = true, bool enableQtWidget = false,
            QTextEdit* qtWidget = nullptr)
            : level(level),
              pattern(pattern),
              logFileName(logFileName),
              maxFileSize(maxFileSize),
              maxFiles(maxFiles),
              enableConsole(enableConsole),
              enableFile(enableFile),
              enableQtWidget(enableQtWidget),
              qtWidget(qtWidget) {}
    };

    static Logger& instance();
    ~Logger();

    // Configuration
    void initialize(const LoggerConfig& config = LoggerConfig());

    /**
     * @brief Initialize with modern LoggingConfig (preferred method)
     * @param config LoggingConfig object to use for initialization
     */
    void initialize(const LoggingConfig& config);

    void setLogLevel(LogLevel level);
    void setPattern(const QString& pattern);

    // Sink management
    void addConsoleSink();
    void addFileSink(const QString& filename);
    void addRotatingFileSink(const QString& filename, size_t maxSize,
                             size_t maxFiles);
    void addQtWidgetSink(QTextEdit* widget);
    void removeSink(SinkType type);

    // Qt widget integration
    void setQtWidget(QTextEdit* widget);
    QTextEdit* getQtWidget() const;

    // Logging methods - kept inline for template instantiation
    template <typename... Args>
    void trace(const QString& format, Args&&... args);

    // String literal overloads for runtime format strings
    template <typename... Args>
    void trace(const char* format, Args&&... args);

    template <typename... Args>
    void trace(const std::string& format, Args&&... args);

    template <typename... Args>
    void debug(const QString& format, Args&&... args);

    template <typename... Args>
    void debug(const char* format, Args&&... args);

    template <typename... Args>
    void debug(const std::string& format, Args&&... args);

    template <typename... Args>
    void info(const QString& format, Args&&... args);

    template <typename... Args>
    void info(const char* format, Args&&... args);

    template <typename... Args>
    void info(const std::string& format, Args&&... args);

    template <typename... Args>
    void warning(const QString& format, Args&&... args);

    template <typename... Args>
    void warning(const char* format, Args&&... args);

    template <typename... Args>
    void warning(const std::string& format, Args&&... args);

    template <typename... Args>
    void error(const QString& format, Args&&... args);

    template <typename... Args>
    void error(const char* format, Args&&... args);

    template <typename... Args>
    void error(const std::string& format, Args&&... args);

    template <typename... Args>
    void critical(const QString& format, Args&&... args);

    template <typename... Args>
    void critical(const char* format, Args&&... args);

    template <typename... Args>
    void critical(const std::string& format, Args&&... args);

    // Simple string logging (Qt-style compatibility)
    void trace(const QString& message);
    void debug(const QString& message);
    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);
    void critical(const QString& message);

    // Get underlying spdlog logger for advanced usage
    std::shared_ptr<spdlog::logger> getSpdlogLogger() const;

signals:
    void logMessage(const QString& message, int level);

private:
    Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    class Implementation;
    std::unique_ptr<Implementation> d;
};

// Template method implementations - must be in header for proper instantiation
template <typename... Args>
void Logger::trace(const QString& format, Args&&... args) {
    auto logger = getSpdlogLogger();
    if (logger) {
        logger->trace(format.toStdString(), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Logger::trace(const char* format, Args&&... args) {
    auto logger = getSpdlogLogger();
    if (logger) {
        logger->trace(fmt::runtime(format), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Logger::trace(const std::string& format, Args&&... args) {
    auto logger = getSpdlogLogger();
    if (logger) {
        logger->trace(fmt::runtime(format), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Logger::debug(const QString& format, Args&&... args) {
    auto logger = getSpdlogLogger();
    if (logger) {
        logger->debug(format.toStdString(), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Logger::debug(const char* format, Args&&... args) {
    auto logger = getSpdlogLogger();
    if (logger) {
        logger->debug(fmt::runtime(format), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Logger::debug(const std::string& format, Args&&... args) {
    auto logger = getSpdlogLogger();
    if (logger) {
        logger->debug(fmt::runtime(format), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Logger::info(const QString& format, Args&&... args) {
    auto logger = getSpdlogLogger();
    if (logger) {
        logger->info(format.toStdString(), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Logger::info(const char* format, Args&&... args) {
    auto logger = getSpdlogLogger();
    if (logger) {
        logger->info(fmt::runtime(format), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Logger::info(const std::string& format, Args&&... args) {
    auto logger = getSpdlogLogger();
    if (logger) {
        logger->info(fmt::runtime(format), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Logger::warning(const QString& format, Args&&... args) {
    auto logger = getSpdlogLogger();
    if (logger) {
        logger->warn(format.toStdString(), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Logger::warning(const char* format, Args&&... args) {
    auto logger = getSpdlogLogger();
    if (logger) {
        logger->warn(fmt::runtime(format), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Logger::warning(const std::string& format, Args&&... args) {
    auto logger = getSpdlogLogger();
    if (logger) {
        logger->warn(fmt::runtime(format), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Logger::error(const QString& format, Args&&... args) {
    auto logger = getSpdlogLogger();
    if (logger) {
        logger->error(format.toStdString(), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Logger::error(const char* format, Args&&... args) {
    auto logger = getSpdlogLogger();
    if (logger) {
        logger->error(fmt::runtime(format), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Logger::error(const std::string& format, Args&&... args) {
    auto logger = getSpdlogLogger();
    if (logger) {
        logger->error(fmt::runtime(format), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Logger::critical(const QString& format, Args&&... args) {
    auto logger = getSpdlogLogger();
    if (logger) {
        logger->critical(format.toStdString(), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Logger::critical(const char* format, Args&&... args) {
    auto logger = getSpdlogLogger();
    if (logger) {
        logger->critical(fmt::runtime(format), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Logger::critical(const std::string& format, Args&&... args) {
    auto logger = getSpdlogLogger();
    if (logger) {
        logger->critical(fmt::runtime(format), std::forward<Args>(args)...);
    }
}

// Convenience macros for easy migration from Qt logging
// Note: These macros are commented out to avoid conflicts with LoggingMacros.h
// Use the macros from LoggingMacros.h instead
/*
#define LOG_TRACE(...) Logger::instance().trace(__VA_ARGS__)
#define LOG_DEBUG(...) Logger::instance().debug(__VA_ARGS__)
#define LOG_INFO(...) Logger::instance().info(__VA_ARGS__)
#define LOG_WARNING(...) Logger::instance().warning(__VA_ARGS__)
#define LOG_ERROR(...) Logger::instance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) Logger::instance().critical(__VA_ARGS__)
*/

// Qt-style compatibility macros removed to avoid conflicts with Qt's own macros
