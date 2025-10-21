#include "LoggingManager.h"
#include <spdlog/async.h>
#include <spdlog/spdlog.h>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QSettings>
#include <QStandardPaths>
#include <QTextEdit>
#include <QThread>
#include <QTimer>
#include <cstddef>
#include <iostream>
#include "Logger.h"
#include "LoggingConfig.h"
#include "LoggingMacros.h"
#include "QtSpdlogBridge.h"

// LoggingManager Implementation class
class LoggingManager::Implementation {
public:
    explicit Implementation(LoggingManager* q) : q_ptr(q) {}
    ~Implementation() = default;

    // Private data members
    LoggingManager::LoggingConfiguration config;
    bool usingModernConfig = false;  // Track which config system is active
    bool initialized = false;
    bool asyncInitialized = false;  // Track if async thread pool is initialized
    mutable QMutex mutex;

    // Timers (using raw pointers with parent ownership - Qt will delete them)
    QTimer* flushTimer = nullptr;
    QTimer* statisticsTimer = nullptr;

    // Statistics
    mutable LoggingManager::LoggingStatistics statistics;

    // Category management
    QHash<QString, Logger::LogLevel> categoryLevels;

    // Qt widget reference
    QTextEdit* qtLogWidget = nullptr;

    // Private methods
    void initializeAsyncLogging();
    void initializeLogger();
    void initializeLoggerFromModernConfig();  // New method for modern config
    void initializeQtBridge();
    void setupPeriodicFlush();
    void createLogDirectory();
    QString getDefaultLogDirectory() const;
    QString getLogFilePath() const;
    void updateLoggerConfiguration();
    void connectSignals();
    void disconnectSignals();

private:
    LoggingManager* q_ptr;
};

// ============================================================================
// Configuration Conversion Functions
// ============================================================================

std::unique_ptr<LoggingConfig> LoggingManager::convertToLoggingConfig(
    const LoggingConfiguration& legacyConfig) {
    auto modernConfig = std::make_unique<LoggingConfig>();

    // Convert global configuration
    LoggingConfig::GlobalConfiguration globalConfig;
    globalConfig.globalLevel = legacyConfig.globalLogLevel;
    globalConfig.globalPattern = legacyConfig.logPattern;
    globalConfig.asyncLogging = legacyConfig.enableAsyncLogging;
    globalConfig.asyncQueueSize = legacyConfig.asyncQueueSize;
    globalConfig.autoFlushOnWarning = legacyConfig.autoFlushOnWarning;
    globalConfig.redirectQtMessages =
        legacyConfig.enableQtMessageHandlerRedirection;
    globalConfig.enableQtCategoryFiltering =
        legacyConfig.enableQtCategoryFiltering;
    globalConfig.enableSourceLocation = legacyConfig.enableSourceLocation;
    globalConfig.enableThreadId = legacyConfig.enableThreadId;
    globalConfig.enableProcessId = legacyConfig.enableProcessId;

    modernConfig->setGlobalConfig(globalConfig);

    // Convert sink configurations
    QList<LoggingConfig::SinkConfiguration> sinkConfigs;

    // Console sink
    if (legacyConfig.enableConsoleLogging) {
        LoggingConfig::SinkConfiguration consoleSink;
        consoleSink.name = "console";
        consoleSink.type = "console";
        consoleSink.level = legacyConfig.consoleLogLevel;
        consoleSink.pattern = legacyConfig.logPattern;
        consoleSink.enabled = true;
        consoleSink.colorEnabled = true;
        sinkConfigs.append(consoleSink);
    }

    // File sink
    if (legacyConfig.enableFileLogging) {
        LoggingConfig::SinkConfiguration fileSink;
        fileSink.name = "file";
        fileSink.type = "rotating_file";
        fileSink.level = legacyConfig.fileLogLevel;
        fileSink.pattern = legacyConfig.logPattern;
        fileSink.enabled = true;
        fileSink.filename = legacyConfig.logFileName;
        fileSink.maxFileSize = legacyConfig.maxFileSize;
        fileSink.maxFiles = legacyConfig.maxFiles;
        fileSink.rotateOnStartup = legacyConfig.rotateOnStartup;
        sinkConfigs.append(fileSink);
    }

    // Qt widget sink
    if (legacyConfig.enableQtWidgetLogging) {
        LoggingConfig::SinkConfiguration qtSink;
        qtSink.name = "qt_widget";
        qtSink.type = "qt_widget";
        qtSink.level = legacyConfig.qtWidgetLogLevel;
        qtSink.pattern = legacyConfig.logPattern;
        qtSink.enabled = true;
        sinkConfigs.append(qtSink);
    }

    modernConfig->setSinkConfigurations(sinkConfigs);

    return modernConfig;
}

LoggingManager::LoggingConfiguration LoggingManager::convertFromLoggingConfig(
    const LoggingConfig& modernConfig) {
    LoggingConfiguration legacyConfig;

    // Convert global configuration
    const auto& globalConfig = modernConfig.getGlobalConfig();
    legacyConfig.globalLogLevel = globalConfig.globalLevel;
    legacyConfig.logPattern = globalConfig.globalPattern;
    legacyConfig.enableAsyncLogging = globalConfig.asyncLogging;
    legacyConfig.asyncQueueSize = globalConfig.asyncQueueSize;
    legacyConfig.autoFlushOnWarning = globalConfig.autoFlushOnWarning;
    legacyConfig.flushIntervalSeconds = globalConfig.flushIntervalSeconds;
    legacyConfig.enableQtMessageHandlerRedirection =
        globalConfig.redirectQtMessages;
    legacyConfig.enableQtCategoryFiltering =
        globalConfig.enableQtCategoryFiltering;
    legacyConfig.enableSourceLocation = globalConfig.enableSourceLocation;
    legacyConfig.enableThreadId = globalConfig.enableThreadId;
    legacyConfig.enableProcessId = globalConfig.enableProcessId;

    // Convert sink configurations
    const auto& sinkConfigs = modernConfig.getSinkConfigurations();

    // Initialize all sink types as disabled
    legacyConfig.enableConsoleLogging = false;
    legacyConfig.enableFileLogging = false;
    legacyConfig.enableQtWidgetLogging = false;

    for (const auto& sink : sinkConfigs) {
        if (!sink.enabled) {
            continue;
        }

        if (sink.type == "console") {
            legacyConfig.enableConsoleLogging = true;
            legacyConfig.consoleLogLevel = sink.level;
        } else if (sink.type == "rotating_file" || sink.type == "file") {
            legacyConfig.enableFileLogging = true;
            legacyConfig.fileLogLevel = sink.level;
            legacyConfig.logFileName = sink.filename;
            legacyConfig.maxFileSize = sink.maxFileSize;
            legacyConfig.maxFiles = sink.maxFiles;
            legacyConfig.rotateOnStartup = sink.rotateOnStartup;
        } else if (sink.type == "qt_widget") {
            legacyConfig.enableQtWidgetLogging = true;
            legacyConfig.qtWidgetLogLevel = sink.level;
        }
    }

    return legacyConfig;
}

std::unique_ptr<LoggingConfig> LoggingManager::createModernConfig() const {
    QMutexLocker locker(&d->mutex);
    return convertToLoggingConfig(d->config);
}

LoggingManager& LoggingManager::instance() {
    static LoggingManager instance;
    return instance;
}

// Constructor implementation for PIMPL
LoggingManager::LoggingManager() : d(std::make_unique<Implementation>(this)) {}

LoggingManager::~LoggingManager() { shutdown(); }

void LoggingManager::initialize(const LoggingConfiguration& config) {
    std::cout << "[DEBUG] LoggingManager::initialize() - Entry" << std::endl;
    std::cout.flush();
    QMutexLocker locker(&d->mutex);
    std::cout << "[DEBUG] LoggingManager::initialize() - Mutex locked"
              << std::endl;
    std::cout.flush();

    if (d->initialized) {
        std::cout << "[DEBUG] LoggingManager::initialize() - Already "
                     "initialized, returning"
                  << std::endl;
        std::cout.flush();
        return;  // Already initialized
    }

    std::cout << "[DEBUG] LoggingManager::initialize() - Setting config"
              << std::endl;
    std::cout.flush();
    d->config = config;
    d->statistics.initializationTime = QDateTime::currentDateTime();

    try {
        // Initialize async logging if enabled (must be done before creating
        // loggers)
        if (d->config.enableAsyncLogging) {
            std::cout << "[DEBUG] LoggingManager::initialize() - Initializing "
                         "async logging"
                      << std::endl;
            std::cout.flush();
            d->initializeAsyncLogging();
        }

        // Create log directory if needed
        std::cout
            << "[DEBUG] LoggingManager::initialize() - Creating log directory"
            << std::endl;
        std::cout.flush();
        d->createLogDirectory();

        // Initialize the core logger
        std::cout
            << "[DEBUG] LoggingManager::initialize() - Initializing logger"
            << std::endl;
        std::cout.flush();
        d->initializeLogger();
        std::cout << "[DEBUG] LoggingManager::initialize() - Logger initialized"
                  << std::endl;
        std::cout.flush();

        // Initialize Qt bridge if enabled
        if (d->config.enableQtMessageHandlerRedirection) {
            std::cout << "[DEBUG] LoggingManager::initialize() - Initializing "
                         "Qt bridge"
                      << std::endl;
            std::cout.flush();
            d->initializeQtBridge();
        }

        // Setup periodic operations
        std::cout << "[DEBUG] LoggingManager::initialize() - Setting up "
                     "periodic flush"
                  << std::endl;
        std::cout.flush();
        d->setupPeriodicFlush();

        // Connect internal signals
        // CRITICAL FIX: Skip signal connections before event loop starts
        // d->connectSignals();

        std::cout
            << "[DEBUG] LoggingManager::initialize() - Marking as initialized"
            << std::endl;
        std::cout.flush();
        d->initialized = true;

        // Log successful initialization
        LOG_INFO("LoggingManager initialized successfully");
        LOG_INFO("Log level: {}", static_cast<int>(d->config.globalLogLevel));
        LOG_INFO("Console logging: {}",
                 d->config.enableConsoleLogging ? "enabled" : "disabled");
        LOG_INFO("File logging: {}",
                 d->config.enableFileLogging ? "enabled" : "disabled");
        LOG_INFO("Qt widget logging: {}",
                 d->config.enableQtWidgetLogging ? "enabled" : "disabled");

        emit loggingInitialized();

    } catch (const std::exception& e) {
        // Fallback initialization with minimal configuration
        LoggingConfiguration fallbackConfig;
        fallbackConfig.enableFileLogging = false;
        fallbackConfig.enableQtWidgetLogging = false;
        fallbackConfig.enableQtMessageHandlerRedirection = false;

        d->config = fallbackConfig;
        d->initializeLogger();
        d->initialized = true;

        LOG_ERROR(
            "LoggingManager initialization failed: {}. Using fallback "
            "configuration.",
            e.what());
    }
}

void LoggingManager::initialize(const LoggingConfig& config) {
    QMutexLocker locker(&d->mutex);

    if (d->initialized) {
        return;  // Already initialized
    }

    // Convert modern config to legacy format for internal use
    // Note: We can't store LoggingConfig directly since it inherits from
    // QObject
    d->config = convertFromLoggingConfig(config);
    d->usingModernConfig = true;
    d->statistics.initializationTime = QDateTime::currentDateTime();

    try {
        // Initialize async logging if enabled (must be done before creating
        // loggers)
        if (d->config.enableAsyncLogging) {
            d->initializeAsyncLogging();
        }

        // Create log directory if needed
        d->createLogDirectory();

        // Initialize the core logger using modern config
        d->initializeLoggerFromModernConfig();

        // Initialize Qt bridge if enabled
        if (d->config.enableQtMessageHandlerRedirection) {
            d->initializeQtBridge();
        }

        // Setup periodic operations
        d->setupPeriodicFlush();

        // Connect internal signals
        d->connectSignals();

        d->initialized = true;

        // Log successful initialization
        LOG_INFO("LoggingManager initialized successfully (modern config)");
        LOG_INFO("Log level: {}", static_cast<int>(d->config.globalLogLevel));
        LOG_INFO("Console logging: {}",
                 d->config.enableConsoleLogging ? "enabled" : "disabled");
        LOG_INFO("File logging: {}",
                 d->config.enableFileLogging ? "enabled" : "disabled");
        LOG_INFO("Qt widget logging: {}",
                 d->config.enableQtWidgetLogging ? "enabled" : "disabled");

        emit loggingInitialized();

    } catch (const std::exception& e) {
        // Fallback initialization with minimal configuration
        LoggingConfiguration fallbackConfig;
        fallbackConfig.enableFileLogging = false;
        fallbackConfig.enableQtWidgetLogging = false;
        fallbackConfig.enableQtMessageHandlerRedirection = false;

        d->config = fallbackConfig;
        d->usingModernConfig = false;
        d->initializeLogger();
        d->initialized = true;

        LOG_ERROR(
            "LoggingManager initialization failed: {}. Using fallback "
            "configuration.",
            e.what());
    }
}

void LoggingManager::shutdown() {
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return;
    }

    LOG_INFO("Shutting down LoggingManager");

    // Disconnect signals
    d->disconnectSignals();

    // Stop timers (Qt will delete them as they have 'this' as parent)
    if (d->flushTimer != nullptr) {
        d->flushTimer->stop();
        d->flushTimer->deleteLater();
        d->flushTimer = nullptr;
    }

    if (d->statisticsTimer != nullptr) {
        d->statisticsTimer->stop();
        d->statisticsTimer->deleteLater();
        d->statisticsTimer = nullptr;
    }

    // Flush all logs before shutdown
    flushLogs();

    // Restore Qt message handler
    if (d->config.enableQtMessageHandlerRedirection) {
        QtSpdlogBridge::instance().restoreDefaultMessageHandler();
    }

    // Shutdown spdlog (this also shuts down async thread pool)
    // Note: spdlog::shutdown() can be called multiple times safely
    try {
        spdlog::shutdown();
    } catch (...) {
        // Suppress any exceptions during shutdown
    }

    d->initialized = false;
    d->asyncInitialized = false;
    emit loggingShutdown();
}

bool LoggingManager::isInitialized() const {
    QMutexLocker locker(&d->mutex);
    return d->initialized;
}

const LoggingManager::LoggingConfiguration& LoggingManager::getConfiguration()
    const {
    QMutexLocker locker(&d->mutex);
    return d->config;
}

bool LoggingManager::isUsingModernConfig() const {
    QMutexLocker locker(&d->mutex);
    return d->usingModernConfig;
}

QtSpdlogBridge& LoggingManager::getQtBridge() {
    return QtSpdlogBridge::instance();
}

const QtSpdlogBridge& LoggingManager::getQtBridge() const {
    return QtSpdlogBridge::instance();
}

void LoggingManager::Implementation::initializeAsyncLogging() {
    if (asyncInitialized) {
        return;  // Already initialized
    }

    try {
        // Initialize spdlog's thread pool for async logging
        // Default: queue size from config (or 8192), 1 worker thread for
        // message ordering
        size_t queueSize =
            config.asyncQueueSize > 0 ? config.asyncQueueSize : 8192;
        size_t threadCount = 1;  // Single thread to preserve message ordering

        spdlog::init_thread_pool(queueSize, threadCount);
        asyncInitialized = true;

        // Note: Async loggers will be created using spdlog::async_factory
        // This is handled by the Logger class when creating sinks
    } catch (const spdlog::spdlog_ex& ex) {
        // If async initialization fails, log error and continue with sync
        // logging The error will be logged once the logger is initialized
        config.enableAsyncLogging = false;
        asyncInitialized = false;
        Q_UNUSED(ex);
    }
}

void LoggingManager::initializeLogger() {
    Logger::LoggerConfig loggerConfig;
    loggerConfig.level = d->config.globalLogLevel;
    loggerConfig.pattern = d->config.logPattern;
    loggerConfig.enableConsole = d->config.enableConsoleLogging;
    loggerConfig.enableFile = d->config.enableFileLogging;
    loggerConfig.enableQtWidget = d->config.enableQtWidgetLogging;
    loggerConfig.qtWidget = d->qtLogWidget;
    loggerConfig.logFileName = getLogFilePath();
    loggerConfig.maxFileSize = d->config.maxFileSize;
    loggerConfig.maxFiles = d->config.maxFiles;

    Logger::instance().initialize(loggerConfig);
}

void LoggingManager::initializeLoggerFromModernConfig() {
    // Since we can't store the LoggingConfig directly (QObject inheritance),
    // we use the converted legacy config for now. In a future enhancement,
    // we could redesign LoggingConfig to be copyable or use a different
    // approach.
    initializeLogger();
}

void LoggingManager::initializeQtBridge() {
    QtSpdlogBridge& bridge = QtSpdlogBridge::instance();
    bridge.initialize();
    bridge.setQtCategoryFilteringEnabled(d->config.enableQtCategoryFiltering);
}

void LoggingManager::setupPeriodicFlush() {
    // CRITICAL FIX: Skip timer setup entirely before event loop starts
    // QTimers require the event loop to be running
    // These timers will be set up later when needed
    // if (d->config.flushIntervalSeconds > 0) {
    //     d->flushTimer = new QTimer(this);
    //     connect(d->flushTimer, &QTimer::timeout, this,
    //             &LoggingManager::onPeriodicFlush);
    // }
    //
    // d->statisticsTimer = new QTimer(this);
    // connect(d->statisticsTimer, &QTimer::timeout, this,
    //         &LoggingManager::updateStatistics);
}

void LoggingManager::createLogDirectory() {
    QString logDir = d->config.logDirectory.isEmpty()
                         ? d->getDefaultLogDirectory()
                         : d->config.logDirectory;
    QDir dir;
    if (!dir.exists(logDir)) {
        if (!dir.mkpath(logDir)) {
            throw std::runtime_error("Failed to create log directory: " +
                                     logDir.toStdString());
        }
    }
}

QString LoggingManager::getDefaultLogDirectory() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
           "/logs";
}

QString LoggingManager::getLogFilePath() const {
    QString logDir = d->config.logDirectory.isEmpty()
                         ? d->getDefaultLogDirectory()
                         : d->config.logDirectory;
    return logDir + "/" + d->config.logFileName;
}

void LoggingManager::setConfiguration(const LoggingConfiguration& config) {
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        d->config = config;
        return;
    }

    // Store old config for comparison
    LoggingConfiguration oldConfig = d->config;
    d->config = config;

    // Reinitialize if major settings changed
    bool needsReinit =
        (oldConfig.enableFileLogging != config.enableFileLogging) ||
        (oldConfig.enableConsoleLogging != config.enableConsoleLogging) ||
        (oldConfig.enableQtWidgetLogging != config.enableQtWidgetLogging) ||
        (oldConfig.logFileName != config.logFileName) ||
        (oldConfig.logDirectory != config.logDirectory);

    if (needsReinit) {
        shutdown();
        initialize(config);
    } else {
        // Update runtime settings
        d->updateLoggerConfiguration();
    }

    emit configurationChanged();
}

void LoggingManager::updateLoggerConfiguration() {
    Logger& logger = Logger::instance();
    logger.setLogLevel(d->config.globalLogLevel);
    logger.setPattern(d->config.logPattern);
}

void LoggingManager::setGlobalLogLevel(Logger::LogLevel level) {
    QMutexLocker locker(&d->mutex);
    d->config.globalLogLevel = level;
    if (d->initialized) {
        Logger::instance().setLogLevel(level);
    }
}

void LoggingManager::setQtLogWidget(QTextEdit* widget) {
    QMutexLocker locker(&d->mutex);
    d->qtLogWidget = widget;

    if (d->initialized) {
        Logger::instance().setQtWidget(widget);
    }
}

QTextEdit* LoggingManager::getQtLogWidget() const {
    QMutexLocker locker(&d->mutex);
    return d->qtLogWidget;
}

void LoggingManager::flushLogs() {
    if (d->initialized) {
        spdlog::apply_all(
            [](std::shared_ptr<spdlog::logger> logger) { logger->flush(); });
    }
}

void LoggingManager::rotateLogFiles() {
    if (d->initialized && d->config.enableFileLogging) {
        // Force log rotation by flushing and recreating file sink
        flushLogs();
        // Implementation would depend on spdlog's rotating file sink
        // capabilities
        LOG_INFO("Log files rotated");
        emit logFileRotated(getCurrentLogFilePath());
    }
}

QString LoggingManager::getCurrentLogFilePath() const {
    return getLogFilePath();
}

QStringList LoggingManager::getLogFileList() const {
    QMutexLocker locker(&d->mutex);
    QStringList logFiles;

    if (d->config.enableFileLogging) {
        QString logDir = d->config.logDirectory.isEmpty()
                             ? d->getDefaultLogDirectory()
                             : d->config.logDirectory;
        QDirIterator it(logDir, QStringList() << "*.log*", QDir::Files);

        while (it.hasNext()) {
            it.next();
            logFiles.append(it.filePath());
        }
    }

    return logFiles;
}

qint64 LoggingManager::getTotalLogFileSize() const {
    QMutexLocker locker(&d->mutex);
    qint64 totalSize = 0;

    if (d->config.enableFileLogging) {
        QString logDir = d->config.logDirectory.isEmpty()
                             ? d->getDefaultLogDirectory()
                             : d->config.logDirectory;
        QDirIterator it(logDir, QStringList() << "*.log*", QDir::Files);

        while (it.hasNext()) {
            it.next();
            QFileInfo info = it.fileInfo();
            totalSize += info.size();
        }
    }

    return totalSize;
}

LoggingManager::LoggingStatistics LoggingManager::getStatistics() const {
    QMutexLocker locker(&d->mutex);

    // Update file size information
    LoggingStatistics stats = d->statistics;

    if (d->config.enableFileLogging) {
        QFileInfo fileInfo(getCurrentLogFilePath());
        if (fileInfo.exists()) {
            stats.currentLogFileSize = fileInfo.size();
        }

        // Calculate total log files size
        QString logDir = d->config.logDirectory.isEmpty()
                             ? d->getDefaultLogDirectory()
                             : d->config.logDirectory;
        QDirIterator it(logDir, QStringList() << "*.log*", QDir::Files);
        stats.totalLogFilesSize = 0;
        stats.activeLogFiles = 0;

        while (it.hasNext()) {
            it.next();
            QFileInfo info = it.fileInfo();
            stats.totalLogFilesSize += info.size();
            stats.activeLogFiles++;
        }
    }

    return stats;
}

void LoggingManager::onLogMessage(const QString& message, int level) {
    QDateTime timestamp = QDateTime::currentDateTime();
    QString category = "general";  // Default category, could be enhanced to
                                   // extract from message
    QString threadId =
        QString::number(reinterpret_cast<quintptr>(QThread::currentThread()));
    QString sourceLocation =
        "";  // Could be enhanced to include source location

    {
        QMutexLocker locker(&d->mutex);

        d->statistics.totalMessagesLogged++;
        d->statistics.lastLogTime = timestamp;

        switch (static_cast<Logger::LogLevel>(level)) {
            case Logger::LogLevel::Debug:
                d->statistics.debugMessages++;
                break;
            case Logger::LogLevel::Info:
                d->statistics.infoMessages++;
                break;
            case Logger::LogLevel::Warning:
                d->statistics.warningMessages++;
                break;
            case Logger::LogLevel::Error:
                d->statistics.errorMessages++;
                break;
            case Logger::LogLevel::Critical:
                d->statistics.criticalMessages++;
                break;
            default:
                break;
        }
    }

    // Emit signal for real-time log streaming (outside of mutex to avoid
    // deadlocks)
    emit logMessageReceived(timestamp, level, category, message, threadId,
                            sourceLocation);
}

void LoggingManager::onPeriodicFlush() { flushLogs(); }

void LoggingManager::onConfigurationChanged() {
    // Handle configuration changes
    // This could trigger a reload or update of the logging configuration
    emit configurationChanged();
}

void LoggingManager::updateStatistics() {
    emit statisticsUpdated(getStatistics());
}

void LoggingManager::connectSignals() {
    // Connect logger signals if needed
    connect(&Logger::instance(), &Logger::logMessage, this,
            &LoggingManager::onLogMessage);
}

void LoggingManager::disconnectSignals() {
    // Disconnect all signals
    disconnect(&Logger::instance(), nullptr, this, nullptr);
}

// Category management implementations
void LoggingManager::addLoggingCategory(const QString& category,
                                        Logger::LogLevel level) {
    QMutexLocker locker(&d->mutex);

    if (category.isEmpty()) {
        return;  // Invalid category name
    }

    // Add or update category level
    d->categoryLevels[category] = level;

    // If using modern config, also update the LoggingConfig structure
    if (d->usingModernConfig) {
        // Create or update category configuration
        LoggingConfig::CategoryConfiguration categoryConfig;
        categoryConfig.name = category;
        categoryConfig.level = level;
        categoryConfig.enabled = true;

        // Note: We can't directly modify LoggingConfig here since we don't
        // store it This would require a redesign to properly support modern
        // config persistence
    }

    // Apply the category level to the Qt logging system if Qt integration is
    // enabled
    if (d->config.enableQtCategoryFiltering) {
        // Add category mapping to Qt bridge (maps Qt category to spdlog logger)
        QtSpdlogBridge::instance().addCategoryMapping(category);
    }
}

void LoggingManager::setLoggingCategoryLevel(const QString& category,
                                             Logger::LogLevel level) {
    QMutexLocker locker(&d->mutex);

    if (category.isEmpty()) {
        return;  // Invalid category name
    }

    // Check if category exists
    if (!d->categoryLevels.contains(category)) {
        // Category doesn't exist, add it
        addLoggingCategory(category, level);
        return;
    }

    // Update existing category level
    Logger::LogLevel oldLevel = d->categoryLevels[category];
    if (oldLevel != level) {
        d->categoryLevels[category] = level;

        // Apply the category level to the Qt logging system if Qt integration
        // is enabled
        if (d->config.enableQtCategoryFiltering) {
            // Update category mapping in Qt bridge
            QtSpdlogBridge::instance().addCategoryMapping(category);
        }

        // Emit signal for level change (unlock mutex first to avoid deadlock)
        locker.unlock();
        emit configurationChanged();
    }
}

void LoggingManager::removeLoggingCategory(const QString& category) {
    QMutexLocker locker(&d->mutex);

    if (category.isEmpty()) {
        return;  // Invalid category name
    }

    // Remove category if it exists
    if (d->categoryLevels.remove(category) > 0) {
        // Remove from Qt logging system if Qt integration is enabled
        if (d->config.enableQtCategoryFiltering) {
            QtSpdlogBridge::instance().removeCategoryMapping(category);
        }

        // Emit signal for configuration change (unlock mutex first to avoid
        // deadlock)
        locker.unlock();
        emit configurationChanged();
    }
}

Logger::LogLevel LoggingManager::getLoggingCategoryLevel(
    const QString& category) const {
    QMutexLocker locker(&d->mutex);

    if (category.isEmpty()) {
        return Logger::LogLevel::Info;  // Default level for invalid category
    }

    // Return category level if it exists, otherwise return global level
    return d->categoryLevels.value(category, d->config.globalLogLevel);
}

QStringList LoggingManager::getLoggingCategories() const {
    QMutexLocker locker(&d->mutex);

    // Return all registered category names
    return d->categoryLevels.keys();
}

// ============================================================================
// LoggingManager::Implementation method implementations
// ============================================================================

void LoggingManager::Implementation::createLogDirectory() {
    QString logDir = getDefaultLogDirectory();
    QDir dir;
    if (!dir.exists(logDir)) {
        dir.mkpath(logDir);
    }
}

QString LoggingManager::Implementation::getDefaultLogDirectory() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
           "/logs";
}

void LoggingManager::Implementation::initializeLogger() {
    // Initialize the main logger with current configuration
    Logger::LoggerConfig loggerConfig;
    loggerConfig.level = config.globalLogLevel;
    loggerConfig.pattern = config.logPattern;
    Logger::instance().initialize(loggerConfig);
}

void LoggingManager::Implementation::initializeLoggerFromModernConfig() {
    // This method would initialize from LoggingConfig instead of
    // LoggingConfiguration For now, delegate to the standard initialization
    initializeLogger();
}

void LoggingManager::Implementation::initializeQtBridge() {
    // Initialize Qt message handler bridge
    QtSpdlogBridge::instance().initialize();
}

void LoggingManager::Implementation::setupPeriodicFlush() {
    if (flushTimer == nullptr) {
        flushTimer = new QTimer();
        flushTimer->setInterval(5000);  // 5 seconds
        QObject::connect(flushTimer, &QTimer::timeout, []() {
            // Periodic flush functionality would go here
            // Currently Logger doesn't expose a flush method
        });
    }
    // CRITICAL FIX: Do NOT start timer here - it will be started when the event
    // loop begins Starting QTimer before QApplication::exec() causes hanging
    // The timer will be started automatically when the event loop starts if
    // needed flushTimer->start();  // COMMENTED OUT - causes hang before event
    // loop
}

void LoggingManager::Implementation::connectSignals() {
    // Connect internal signals for log configuration changes
    // This allows components to react to logging configuration updates

    // Future enhancements could include:
    // - Connecting to configuration change signals
    // - Setting up log rotation triggers
    // - Connecting to performance monitoring signals
    // - Setting up log level change notifications

    // For now, this is a no-op as the LoggingManager uses direct method calls
    // rather than signal/slot communication for configuration updates
}

void LoggingManager::Implementation::disconnectSignals() {
    // Disconnect internal signals during shutdown
    // This ensures clean shutdown without dangling signal connections

    // Future enhancements would disconnect signals connected in
    // connectSignals()

    // For now, this is a no-op as there are no signals to disconnect
}

void LoggingManager::Implementation::updateLoggerConfiguration() {
    // Update logger configuration with current settings
    Logger::instance().setLogLevel(config.globalLogLevel);
    Logger::instance().setPattern(config.logPattern);
}

// ============================================================================
// Note: ScopedLoggingConfig implementation has been moved to ScopedLogLevel
// ============================================================================
// The ScopedLogLevel class in LoggingMacros.cpp now provides the enhanced
// functionality that was previously split between ScopedLogLevel and
// ScopedLoggingConfig classes.
