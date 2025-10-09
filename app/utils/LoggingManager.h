#pragma once

#include <QtCore/qglobal.h>
#include <QDateTime>
#include <QMutex>
#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QTextEdit>
#include <QTimer>
#include <cstddef>
#include <memory>
#include "Logger.h"
#include "QtSpdlogBridge.h"

/**
 * @brief Centralized logging manager for the entire application
 *
 * This singleton class manages all logging configuration, initialization,
 * and coordination between different logging components. It provides
 * thread-safe access to logging functionality and handles configuration
 * persistence.
 */
class LoggingManager : public QObject {
    Q_OBJECT

public:
    struct LoggingConfiguration {
        // General settings
        Logger::LogLevel globalLogLevel;
        QString logPattern;

        // Console logging
        bool enableConsoleLogging;
        Logger::LogLevel consoleLogLevel;

        // File logging
        bool enableFileLogging;
        Logger::LogLevel fileLogLevel;
        QString logFileName;
        QString logDirectory;  // Empty means use default app data location
        size_t maxFileSize;    // 10MB
        size_t maxFiles;
        bool rotateOnStartup;

        // Qt widget logging
        bool enableQtWidgetLogging;
        Logger::LogLevel qtWidgetLogLevel;

        // Qt integration
        bool enableQtMessageHandlerRedirection;
        bool enableQtCategoryFiltering;

        // Performance settings
        bool enableAsyncLogging;
        size_t asyncQueueSize;
        bool autoFlushOnWarning;
        int flushIntervalSeconds;

        // Debug settings
        bool enableSourceLocation;  // Only in debug builds
        bool enableThreadId;
        bool enableProcessId;

        LoggingConfiguration(
            Logger::LogLevel globalLogLevel = Logger::LogLevel::Info,
            const QString& logPattern =
                "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v",
            bool enableConsoleLogging = true,
            Logger::LogLevel consoleLogLevel = Logger::LogLevel::Debug,
            bool enableFileLogging = true,
            Logger::LogLevel fileLogLevel = Logger::LogLevel::Info,
            const QString& logFileName = "sast-readium.log",
            const QString& logDirectory = "",
            size_t maxFileSize = 10 * 1024 * 1024, size_t maxFiles = 5,
            bool rotateOnStartup = false, bool enableQtWidgetLogging = false,
            Logger::LogLevel qtWidgetLogLevel = Logger::LogLevel::Debug,
            bool enableQtMessageHandlerRedirection = true,
            bool enableQtCategoryFiltering = true,
            bool enableAsyncLogging = false, size_t asyncQueueSize = 8192,
            bool autoFlushOnWarning = true, int flushIntervalSeconds = 5,
            bool enableSourceLocation = false, bool enableThreadId = false,
            bool enableProcessId = false)
            : globalLogLevel(globalLogLevel),
              logPattern(logPattern),
              enableConsoleLogging(enableConsoleLogging),
              consoleLogLevel(consoleLogLevel),
              enableFileLogging(enableFileLogging),
              fileLogLevel(fileLogLevel),
              logFileName(logFileName),
              logDirectory(logDirectory),
              maxFileSize(maxFileSize),
              maxFiles(maxFiles),
              rotateOnStartup(rotateOnStartup),
              enableQtWidgetLogging(enableQtWidgetLogging),
              qtWidgetLogLevel(qtWidgetLogLevel),
              enableQtMessageHandlerRedirection(
                  enableQtMessageHandlerRedirection),
              enableQtCategoryFiltering(enableQtCategoryFiltering),
              enableAsyncLogging(enableAsyncLogging),
              asyncQueueSize(asyncQueueSize),
              autoFlushOnWarning(autoFlushOnWarning),
              flushIntervalSeconds(flushIntervalSeconds),
              enableSourceLocation(enableSourceLocation),
              enableThreadId(enableThreadId),
              enableProcessId(enableProcessId) {}
    };

    static LoggingManager& instance();
    ~LoggingManager();

    // Initialization and configuration
    void initialize(
        const LoggingConfiguration& config = LoggingConfiguration());
    void shutdown();
    bool isInitialized() const { return m_initialized; }

    // Configuration management
    void setConfiguration(const LoggingConfiguration& config);
    const LoggingConfiguration& getConfiguration() const { return m_config; }
    void loadConfigurationFromSettings(QSettings& settings);
    void saveConfigurationToSettings(QSettings& settings) const;
    void resetToDefaultConfiguration();

    // Runtime configuration changes
    void setGlobalLogLevel(Logger::LogLevel level);
    void setConsoleLogLevel(Logger::LogLevel level);
    void setFileLogLevel(Logger::LogLevel level);
    void setQtWidgetLogLevel(Logger::LogLevel level);
    void setLogPattern(const QString& pattern);

    // Qt widget integration
    void setQtLogWidget(QTextEdit* widget);
    QTextEdit* getQtLogWidget() const;
    void enableQtWidgetLogging(bool enable);

    // File logging management
    void rotateLogFiles();
    void flushLogs();
    QString getCurrentLogFilePath() const;
    QStringList getLogFileList() const;
    qint64 getTotalLogFileSize() const;

    // Logger access
    Logger& getLogger() { return Logger::instance(); }
    const Logger& getLogger() const { return Logger::instance(); }

    // Qt bridge access
    QtSpdlogBridge& getQtBridge() { return QtSpdlogBridge::instance(); }
    const QtSpdlogBridge& getQtBridge() const {
        return QtSpdlogBridge::instance();
    }

    // Statistics and monitoring
    struct LoggingStatistics {
        qint64 totalMessagesLogged = 0;
        qint64 debugMessages = 0;
        qint64 infoMessages = 0;
        qint64 warningMessages = 0;
        qint64 errorMessages = 0;
        qint64 criticalMessages = 0;
        qint64 currentLogFileSize = 0;
        qint64 totalLogFilesSize = 0;
        int activeLogFiles = 0;
        QDateTime lastLogTime;
        QDateTime initializationTime;
    };

    LoggingStatistics getStatistics() const;
    void resetStatistics();

    // Category management for Qt integration
    void addLoggingCategory(const QString& category,
                            Logger::LogLevel level = Logger::LogLevel::Debug);
    void removeLoggingCategory(const QString& category);
    void setLoggingCategoryLevel(const QString& category,
                                 Logger::LogLevel level);
    Logger::LogLevel getLoggingCategoryLevel(const QString& category) const;
    QStringList getLoggingCategories() const;

public slots:
    void onLogMessage(const QString& message, int level);
    void onPeriodicFlush();
    void onConfigurationChanged();

signals:
    void loggingInitialized();
    void loggingShutdown();
    void configurationChanged();
    void logFileRotated(const QString& newFilePath);
    void statisticsUpdated(const LoggingStatistics& stats);
    void logMessageReceived(const QDateTime& timestamp, int level,
                            const QString& category, const QString& message,
                            const QString& threadId = "",
                            const QString& sourceLocation = "");

private slots:
    void updateStatistics();

private:
    LoggingManager() = default;
    LoggingManager(const LoggingManager&) = delete;
    LoggingManager& operator=(const LoggingManager&) = delete;

    void initializeLogger();
    void initializeQtBridge();
    void setupPeriodicFlush();
    void createLogDirectory();
    QString getDefaultLogDirectory() const;
    QString getLogFilePath() const;
    void updateLoggerConfiguration();
    void connectSignals();
    void disconnectSignals();

    LoggingConfiguration m_config;
    bool m_initialized = false;
    mutable QMutex m_mutex;

    // Timers
    QTimer* m_flushTimer = nullptr;
    QTimer* m_statisticsTimer = nullptr;

    // Statistics
    mutable LoggingStatistics m_statistics;

    // Category management
    QHash<QString, Logger::LogLevel> m_categoryLevels;

    // Qt widget reference
    QTextEdit* m_qtLogWidget = nullptr;
};

/**
 * @brief RAII helper class for scoped logging configuration
 */
class ScopedLoggingConfig {
public:
    explicit ScopedLoggingConfig(Logger::LogLevel tempLevel);
    explicit ScopedLoggingConfig(
        const LoggingManager::LoggingConfiguration& tempConfig);
    ~ScopedLoggingConfig();

private:
    LoggingManager::LoggingConfiguration m_originalConfig;
    bool m_levelOnly;
};

/**
 * @brief Convenience macros for common logging operations
 */
#define LOGGING_MANAGER LoggingManager::instance()
#define INIT_LOGGING(config) LoggingManager::instance().initialize(config)
#define SHUTDOWN_LOGGING() LoggingManager::instance().shutdown()

// Scoped logging level changes
#define SCOPED_LOG_LEVEL(level) ScopedLoggingConfig _scoped_log_config(level)

// Category-based logging (for migration from QLoggingCategory)
// Note: These macros are renamed to avoid conflicts with LoggingMacros.h
#define LOGGING_CATEGORY_DEBUG(category, message)            \
    if (LOGGING_MANAGER.getLoggingCategoryLevel(category) <= \
        Logger::LogLevel::Debug)                             \
    LOG_DEBUG("[{}] {}", category.toStdString(), message)

#define LOGGING_CATEGORY_INFO(category, message)             \
    if (LOGGING_MANAGER.getLoggingCategoryLevel(category) <= \
        Logger::LogLevel::Info)                              \
    LOG_INFO("[{}] {}", category.toStdString(), message)

#define LOGGING_CATEGORY_WARNING(category, message)          \
    if (LOGGING_MANAGER.getLoggingCategoryLevel(category) <= \
        Logger::LogLevel::Warning)                           \
    LOG_WARNING("[{}] {}", category.toStdString(), message)

#define LOGGING_CATEGORY_ERROR(category, message)            \
    if (LOGGING_MANAGER.getLoggingCategoryLevel(category) <= \
        Logger::LogLevel::Error)                             \
    LOG_ERROR("[{}] {}", category.toStdString(), message)
