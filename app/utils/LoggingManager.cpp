#include "LoggingManager.h"
#include <spdlog/async.h>
#include <spdlog/spdlog.h>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QThread>
#include "LoggingMacros.h"
#include "LoggingConfig.h"

LoggingManager& LoggingManager::instance() {
    static LoggingManager instance;
    return instance;
}

LoggingManager::~LoggingManager() { shutdown(); }

LoggingManager::LoggingConfiguration LoggingManager::createDevelopmentConfiguration(
    Logger::LogLevel globalLevel,
    const QString& pattern,
    Logger::LogLevel consoleLevel,
    const QString& fileName,
    size_t maxFileSize,
    size_t maxFiles,
    Logger::LogLevel fileLevel) {
    
    LoggingConfigBuilder builder;
    auto config = builder
        .useDevelopmentPreset()
        .setGlobalLevel(globalLevel)
        .setGlobalPattern(pattern)
        .addConsoleSink("console", consoleLevel)
        .addRotatingFileSink("file", fileName, maxFileSize, maxFiles, fileLevel)
        .addCategory("main", Logger::LogLevel::Debug)
        .addCategory("ui", Logger::LogLevel::Info)
        .buildUnique();

    // Convert LoggingConfig to LoggingManager::LoggingConfiguration
    LoggingConfiguration managerConfig;
    
    const auto& globalConfig = config->getGlobalConfig();
    managerConfig.globalLogLevel = globalConfig.globalLevel;
    managerConfig.logPattern = globalConfig.globalPattern;
    
    // Extract console and file settings from sinks
    const auto& sinks = config->getSinkConfigurations();
    for (const auto& sink : sinks) {
        if (sink.type == "console") {
            managerConfig.enableConsoleLogging = sink.enabled;
            managerConfig.consoleLogLevel = sink.level;
        } else if (sink.type == "rotating_file" || sink.type == "file") {
            managerConfig.enableFileLogging = sink.enabled;
            managerConfig.fileLogLevel = sink.level;
            managerConfig.logFileName = sink.filename.isEmpty() ? "sast-readium.log" : sink.filename;
            managerConfig.maxFileSize = sink.maxFileSize;
            managerConfig.maxFiles = sink.maxFiles;
            managerConfig.rotateOnStartup = sink.rotateOnStartup;
        }
    }
    
    // Set other default values to match original configuration
    managerConfig.logDirectory = "";  // empty = default
    managerConfig.enableQtWidgetLogging = false;
    managerConfig.qtWidgetLogLevel = Logger::LogLevel::Debug;
    managerConfig.enableQtMessageHandlerRedirection = true;
    managerConfig.enableQtCategoryFiltering = true;
    
    return managerConfig;
}

LoggingManager::LoggingConfiguration LoggingManager::fromLoggingConfig(const LoggingConfig& config) {
    LoggingConfiguration managerConfig;
    
    const auto& globalConfig = config.getGlobalConfig();
    managerConfig.globalLogLevel = globalConfig.globalLevel;
    managerConfig.logPattern = globalConfig.globalPattern;
    
    // Extract console and file settings from sinks
    const auto& sinks = config.getSinkConfigurations();
    for (const auto& sink : sinks) {
        if (sink.type == "console") {
            managerConfig.enableConsoleLogging = sink.enabled;
            managerConfig.consoleLogLevel = sink.level;
        } else if (sink.type == "rotating_file" || sink.type == "file") {
            managerConfig.enableFileLogging = sink.enabled;
            managerConfig.fileLogLevel = sink.level;
            managerConfig.logFileName = sink.filename.isEmpty() ? "sast-readium.log" : sink.filename;
            managerConfig.maxFileSize = sink.maxFileSize;
            managerConfig.maxFiles = sink.maxFiles;
            managerConfig.rotateOnStartup = sink.rotateOnStartup;
        }
    }
    
    // Set other default values to match original configuration
    managerConfig.logDirectory = "";  // empty = default
    managerConfig.enableQtWidgetLogging = false;
    managerConfig.qtWidgetLogLevel = Logger::LogLevel::Debug;
    managerConfig.enableQtMessageHandlerRedirection = true;
    managerConfig.enableQtCategoryFiltering = true;
    
    return managerConfig;
}

void LoggingManager::initialize(const LoggingConfiguration& config) {
    QMutexLocker locker(&m_mutex);

    if (m_initialized) {
        return;  // Already initialized
    }

    m_config = config;
    m_statistics.initializationTime = QDateTime::currentDateTime();

    try {
        // Create log directory if needed
        createLogDirectory();

        // Initialize the core logger
        initializeLogger();

        // Initialize Qt bridge if enabled
        if (m_config.enableQtMessageHandlerRedirection) {
            initializeQtBridge();
        }

        // Setup periodic operations
        setupPeriodicFlush();

        // Connect internal signals
        connectSignals();

        m_initialized = true;

        // Log successful initialization
        LOG_INFO("LoggingManager initialized successfully");
        LOG_INFO("Log level: {}", static_cast<int>(m_config.globalLogLevel));
        LOG_INFO("Console logging: {}",
                 m_config.enableConsoleLogging ? "enabled" : "disabled");
        LOG_INFO("File logging: {}",
                 m_config.enableFileLogging ? "enabled" : "disabled");
        LOG_INFO("Qt widget logging: {}",
                 m_config.enableQtWidgetLogging ? "enabled" : "disabled");

        emit loggingInitialized();

    } catch (const std::exception& e) {
        // Fallback initialization with minimal configuration
        LoggingConfiguration fallbackConfig;
        fallbackConfig.enableFileLogging = false;
        fallbackConfig.enableQtWidgetLogging = false;
        fallbackConfig.enableQtMessageHandlerRedirection = false;

        m_config = fallbackConfig;
        initializeLogger();
        m_initialized = true;

        LOG_ERROR(
            "LoggingManager initialization failed: {}. Using fallback "
            "configuration.",
            e.what());
    }
}

void LoggingManager::shutdown() {
    QMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        return;
    }

    LOG_INFO("Shutting down LoggingManager");

    // Disconnect signals
    disconnectSignals();

    // Stop timers
    if (m_flushTimer) {
        m_flushTimer->stop();
        delete m_flushTimer;
        m_flushTimer = nullptr;
    }

    if (m_statisticsTimer) {
        m_statisticsTimer->stop();
        delete m_statisticsTimer;
        m_statisticsTimer = nullptr;
    }

    // Flush all logs before shutdown
    flushLogs();

    // Restore Qt message handler
    if (m_config.enableQtMessageHandlerRedirection) {
        QtSpdlogBridge::instance().restoreDefaultMessageHandler();
    }

    // Shutdown spdlog
    spdlog::shutdown();

    m_initialized = false;
    emit loggingShutdown();
}

void LoggingManager::initializeLogger() {
    Logger::LoggerConfig loggerConfig;
    loggerConfig.level = m_config.globalLogLevel;
    loggerConfig.pattern = m_config.logPattern;
    loggerConfig.enableConsole = m_config.enableConsoleLogging;
    loggerConfig.enableFile = m_config.enableFileLogging;
    loggerConfig.enableQtWidget = m_config.enableQtWidgetLogging;
    loggerConfig.qtWidget = m_qtLogWidget;
    loggerConfig.logFileName = getLogFilePath();
    loggerConfig.maxFileSize = m_config.maxFileSize;
    loggerConfig.maxFiles = m_config.maxFiles;

    Logger::instance().initialize(loggerConfig);
}

void LoggingManager::initializeQtBridge() {
    QtSpdlogBridge& bridge = QtSpdlogBridge::instance();
    bridge.initialize();
    bridge.setQtCategoryFilteringEnabled(m_config.enableQtCategoryFiltering);
}

void LoggingManager::setupPeriodicFlush() {
    if (m_config.flushIntervalSeconds > 0) {
        m_flushTimer = new QTimer(this);
        connect(m_flushTimer, &QTimer::timeout, this,
                &LoggingManager::onPeriodicFlush);
        m_flushTimer->start(m_config.flushIntervalSeconds * 1000);
    }

    // Statistics update timer
    m_statisticsTimer = new QTimer(this);
    connect(m_statisticsTimer, &QTimer::timeout, this,
            &LoggingManager::updateStatistics);
    m_statisticsTimer->start(10000);  // Update every 10 seconds
}

void LoggingManager::createLogDirectory() {
    QString logDir = m_config.logDirectory.isEmpty() ? getDefaultLogDirectory()
                                                     : m_config.logDirectory;
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
    QString logDir = m_config.logDirectory.isEmpty() ? getDefaultLogDirectory()
                                                     : m_config.logDirectory;
    return logDir + "/" + m_config.logFileName;
}

void LoggingManager::setConfiguration(const LoggingConfiguration& config) {
    QMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        m_config = config;
        return;
    }

    // Store old config for comparison
    LoggingConfiguration oldConfig = m_config;
    m_config = config;

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
        updateLoggerConfiguration();
    }

    emit configurationChanged();
}

void LoggingManager::updateLoggerConfiguration() {
    Logger& logger = Logger::instance();
    logger.setLogLevel(m_config.globalLogLevel);
    logger.setPattern(m_config.logPattern);
}

void LoggingManager::setGlobalLogLevel(Logger::LogLevel level) {
    QMutexLocker locker(&m_mutex);
    m_config.globalLogLevel = level;
    if (m_initialized) {
        Logger::instance().setLogLevel(level);
    }
}

void LoggingManager::setQtLogWidget(QTextEdit* widget) {
    QMutexLocker locker(&m_mutex);
    m_qtLogWidget = widget;

    if (m_initialized) {
        Logger::instance().setQtWidget(widget);
    }
}

QTextEdit* LoggingManager::getQtLogWidget() const {
    QMutexLocker locker(&m_mutex);
    return m_qtLogWidget;
}

void LoggingManager::flushLogs() {
    if (m_initialized) {
        spdlog::apply_all(
            [](std::shared_ptr<spdlog::logger> logger) { logger->flush(); });
    }
}

void LoggingManager::rotateLogFiles() {
    if (m_initialized && m_config.enableFileLogging) {
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

LoggingManager::LoggingStatistics LoggingManager::getStatistics() const {
    QMutexLocker locker(&m_mutex);

    // Update file size information
    LoggingStatistics stats = m_statistics;

    if (m_config.enableFileLogging) {
        QFileInfo fileInfo(getCurrentLogFilePath());
        if (fileInfo.exists()) {
            stats.currentLogFileSize = fileInfo.size();
        }

        // Calculate total log files size
        QString logDir = m_config.logDirectory.isEmpty()
                             ? getDefaultLogDirectory()
                             : m_config.logDirectory;
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
        QMutexLocker locker(&m_mutex);

        m_statistics.totalMessagesLogged++;
        m_statistics.lastLogTime = timestamp;

        switch (static_cast<Logger::LogLevel>(level)) {
            case Logger::LogLevel::Debug:
                m_statistics.debugMessages++;
                break;
            case Logger::LogLevel::Info:
                m_statistics.infoMessages++;
                break;
            case Logger::LogLevel::Warning:
                m_statistics.warningMessages++;
                break;
            case Logger::LogLevel::Error:
                m_statistics.errorMessages++;
                break;
            case Logger::LogLevel::Critical:
                m_statistics.criticalMessages++;
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

// ScopedLoggingConfig implementation
ScopedLoggingConfig::ScopedLoggingConfig(Logger::LogLevel tempLevel)
    : m_originalConfig(LoggingManager::instance().getConfiguration()),
      m_levelOnly(true) {
    LoggingManager::instance().setGlobalLogLevel(tempLevel);
}

ScopedLoggingConfig::ScopedLoggingConfig(
    const LoggingManager::LoggingConfiguration& tempConfig)
    : m_originalConfig(LoggingManager::instance().getConfiguration()),
      m_levelOnly(false) {
    LoggingManager::instance().setConfiguration(tempConfig);
}

ScopedLoggingConfig::~ScopedLoggingConfig() {
    if (m_levelOnly) {
        LoggingManager::instance().setGlobalLogLevel(
            m_originalConfig.globalLogLevel);
    } else {
        LoggingManager::instance().setConfiguration(m_originalConfig);
    }
}
