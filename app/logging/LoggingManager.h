#pragma once

// ============================================================================
// Include Dependencies
// ============================================================================
// This file provides centralized logging management and configuration.
// Note: LoggingMacros.h includes this file to access configuration management.
// For logging macros, include LoggingMacros.h instead of this file directly.

#include <QDateTime>
#include <QObject>
#include <QStringList>
#include <memory>
#include "Logger.h"

// Forward declarations to reduce header dependencies
class LoggingConfig;
class QtSpdlogBridge;
class QSettings;
class QTextEdit;
class QTimer;

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

    // ============================================================================
    // Backward Compatibility and Configuration Conversion
    // ============================================================================

    /**
     * @brief Convert LoggingConfiguration to LoggingConfig structures
     * @param legacyConfig The old LoggingConfiguration to convert
     * @return Unique pointer to new LoggingConfig object
     */
    static std::unique_ptr<LoggingConfig> convertToLoggingConfig(
        const LoggingConfiguration& legacyConfig);

    /**
     * @brief Convert LoggingConfig to LoggingConfiguration for backward
     * compatibility
     * @param modernConfig The LoggingConfig to convert
     * @return LoggingConfiguration structure
     */
    static LoggingConfiguration convertFromLoggingConfig(
        const LoggingConfig& modernConfig);

    // ============================================================================
    // Deprecated APIs (for backward compatibility)
    // ============================================================================

    /**
     * @deprecated Use LoggingConfig instead of LoggingConfiguration
     * @brief Legacy configuration structure - use LoggingConfig for new code
     */
    using LegacyLoggingConfiguration
        [[deprecated("Use LoggingConfig instead")]] = LoggingConfiguration;

    /**
     * @deprecated Use initialize(const LoggingConfig&) instead
     * @brief Legacy initialization method - use LoggingConfig version for new
     * code
     */
    [[deprecated("Use initialize(const LoggingConfig&) instead")]]
    void initializeLegacy(const LoggingConfiguration& config) {
        initialize(config);
    }

    static LoggingManager& instance();
    ~LoggingManager();

    // Initialization and configuration
    void initialize(
        const LoggingConfiguration& config = LoggingConfiguration());

    /**
     * @brief Initialize with modern LoggingConfig (preferred method)
     * @param config LoggingConfig object to use for initialization
     */
    void initialize(const LoggingConfig& config);

    void shutdown();
    bool isInitialized() const;

    // Configuration management
    void setConfiguration(const LoggingConfiguration& config);
    const LoggingConfiguration& getConfiguration() const;
    void loadConfigurationFromSettings(QSettings& settings);
    void saveConfigurationToSettings(QSettings& settings) const;
    void resetToDefaultConfiguration();

    /**
     * @brief Create a LoggingConfig from current LoggingConfiguration (for
     * migration)
     * @return Unique pointer to LoggingConfig equivalent to current
     * configuration
     */
    std::unique_ptr<LoggingConfig> createModernConfig() const;

    /**
     * @brief Check if using modern LoggingConfig or legacy LoggingConfiguration
     * @return True if initialized with LoggingConfig, false if with
     * LoggingConfiguration
     */
    bool isUsingModernConfig() const;

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
    QtSpdlogBridge& getQtBridge();
    const QtSpdlogBridge& getQtBridge() const;

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
    LoggingManager();
    LoggingManager(const LoggingManager&) = delete;
    LoggingManager& operator=(const LoggingManager&) = delete;

    // Helper methods
    void initializeLogger();
    void initializeLoggerFromModernConfig();
    void initializeQtBridge();
    void setupPeriodicFlush();
    void createLogDirectory();
    QString getDefaultLogDirectory() const;
    QString getLogFilePath() const;
    void updateLoggerConfiguration();
    void connectSignals() const;
    void disconnectSignals() const;

    class Implementation;
    std::unique_ptr<Implementation> d;
};

// ============================================================================
// Note: ScopedLoggingConfig has been consolidated into ScopedLogLevel
// ============================================================================
// The ScopedLogLevel class in LoggingMacros.h now handles both level-only
// and full configuration changes. Use ScopedLogLevel for all scoped logging
// configuration needs.

/**
 * @brief Convenience macros for common logging operations
 */
#define LOGGING_MANAGER LoggingManager::instance()
#define INIT_LOGGING(config) LoggingManager::instance().initialize(config)
#define SHUTDOWN_LOGGING() LoggingManager::instance().shutdown()

// ============================================================================
// Note: Scoped logging macros are defined in LoggingMacros.h
// ============================================================================
// Use SCOPED_LOG_LEVEL(level) and SCOPED_LOG_CONFIG(config) from
// LoggingMacros.h for scoped logging configuration changes.

// ============================================================================
// Migration Helper Macros
// ============================================================================

/**
 * @brief Initialize logging with modern LoggingConfig (preferred)
 * @param config LoggingConfig object or LoggingConfigBuilder result
 */
#define INIT_MODERN_LOGGING(config) \
    LoggingManager::instance().initialize(config)

/**
 * @brief Create a modern config from current legacy configuration
 */
#define CREATE_MODERN_CONFIG() LoggingManager::instance().createModernConfig()

/**
 * @brief Check if using modern configuration system
 */
#define IS_USING_MODERN_CONFIG() \
    LoggingManager::instance().isUsingModernConfig()

// ============================================================================
// Note: Category logging macros have been consolidated in LoggingMacros.h
// ============================================================================
// Use LOG_CATEGORY_DEBUG, LOG_CATEGORY_INFO, LOG_CATEGORY_WARNING, and
// LOG_CATEGORY_ERROR from LoggingMacros.h for category-based logging.
// The duplicate LOGGING_CATEGORY_* macros have been removed to eliminate
// code duplication and improve maintainability.
