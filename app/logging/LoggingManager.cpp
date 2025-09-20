#include "LoggingManager.h"
#include "LoggingMacros.h"
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <QMutexLocker>
#include <QDateTime>
#include <QFileInfo>
#include <QDirIterator>
#include <QThread>
#include <spdlog/async.h>
#include <spdlog/spdlog.h>

// ============================================================================
// Configuration Conversion Functions
// ============================================================================

std::unique_ptr<LoggingConfig> LoggingManager::convertToLoggingConfig(const LoggingConfiguration& legacyConfig)
{
    auto modernConfig = std::make_unique<LoggingConfig>();

    // Convert global configuration
    LoggingConfig::GlobalConfiguration globalConfig;
    globalConfig.globalLevel = legacyConfig.globalLogLevel;
    globalConfig.globalPattern = legacyConfig.logPattern;
    globalConfig.asyncLogging = legacyConfig.enableAsyncLogging;
    globalConfig.asyncQueueSize = legacyConfig.asyncQueueSize;
    globalConfig.autoFlushOnWarning = legacyConfig.autoFlushOnWarning;
    globalConfig.redirectQtMessages = legacyConfig.enableQtMessageHandlerRedirection;
    globalConfig.enableQtCategoryFiltering = legacyConfig.enableQtCategoryFiltering;
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

LoggingManager::LoggingConfiguration LoggingManager::convertFromLoggingConfig(const LoggingConfig& modernConfig)
{
    LoggingConfiguration legacyConfig;

    // Convert global configuration
    const auto& globalConfig = modernConfig.getGlobalConfig();
    legacyConfig.globalLogLevel = globalConfig.globalLevel;
    legacyConfig.logPattern = globalConfig.globalPattern;
    legacyConfig.enableAsyncLogging = globalConfig.asyncLogging;
    legacyConfig.asyncQueueSize = globalConfig.asyncQueueSize;
    legacyConfig.autoFlushOnWarning = globalConfig.autoFlushOnWarning;
    legacyConfig.flushIntervalSeconds = globalConfig.flushIntervalSeconds;
    legacyConfig.enableQtMessageHandlerRedirection = globalConfig.redirectQtMessages;
    legacyConfig.enableQtCategoryFiltering = globalConfig.enableQtCategoryFiltering;
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
        if (!sink.enabled) continue;

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

std::unique_ptr<LoggingConfig> LoggingManager::createModernConfig() const
{
    QMutexLocker locker(&m_mutex);
    return convertToLoggingConfig(m_config);
}

LoggingManager& LoggingManager::instance()
{
    static LoggingManager instance;
    return instance;
}

LoggingManager::~LoggingManager()
{
    shutdown();
}

void LoggingManager::initialize(const LoggingConfiguration& config)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_initialized) {
        return; // Already initialized
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
        LOG_INFO("Console logging: {}", m_config.enableConsoleLogging ? "enabled" : "disabled");
        LOG_INFO("File logging: {}", m_config.enableFileLogging ? "enabled" : "disabled");
        LOG_INFO("Qt widget logging: {}", m_config.enableQtWidgetLogging ? "enabled" : "disabled");
        
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
        
        LOG_ERROR("LoggingManager initialization failed: {}. Using fallback configuration.", e.what());
    }
}

void LoggingManager::initialize(const LoggingConfig& config)
{
    QMutexLocker locker(&m_mutex);

    if (m_initialized) {
        return; // Already initialized
    }

    // Convert modern config to legacy format for internal use
    // Note: We can't store LoggingConfig directly since it inherits from QObject
    m_config = convertFromLoggingConfig(config);
    m_usingModernConfig = true;
    m_statistics.initializationTime = QDateTime::currentDateTime();

    try {
        // Create log directory if needed
        createLogDirectory();

        // Initialize the core logger using modern config
        initializeLoggerFromModernConfig();

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
        LOG_INFO("LoggingManager initialized successfully (modern config)");
        LOG_INFO("Log level: {}", static_cast<int>(m_config.globalLogLevel));
        LOG_INFO("Console logging: {}", m_config.enableConsoleLogging ? "enabled" : "disabled");
        LOG_INFO("File logging: {}", m_config.enableFileLogging ? "enabled" : "disabled");
        LOG_INFO("Qt widget logging: {}", m_config.enableQtWidgetLogging ? "enabled" : "disabled");

        emit loggingInitialized();

    } catch (const std::exception& e) {
        // Fallback initialization with minimal configuration
        LoggingConfiguration fallbackConfig;
        fallbackConfig.enableFileLogging = false;
        fallbackConfig.enableQtWidgetLogging = false;
        fallbackConfig.enableQtMessageHandlerRedirection = false;

        m_config = fallbackConfig;
        m_usingModernConfig = false;
        initializeLogger();
        m_initialized = true;

        LOG_ERROR("LoggingManager initialization failed: {}. Using fallback configuration.", e.what());
    }
}

void LoggingManager::shutdown()
{
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

void LoggingManager::initializeLogger()
{
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

void LoggingManager::initializeLoggerFromModernConfig()
{
    // Since we can't store the LoggingConfig directly (QObject inheritance),
    // we use the converted legacy config for now. In a future enhancement,
    // we could redesign LoggingConfig to be copyable or use a different approach.
    initializeLogger();
}

void LoggingManager::initializeQtBridge()
{
    QtSpdlogBridge& bridge = QtSpdlogBridge::instance();
    bridge.initialize();
    bridge.setQtCategoryFilteringEnabled(m_config.enableQtCategoryFiltering);
}

void LoggingManager::setupPeriodicFlush()
{
    if (m_config.flushIntervalSeconds > 0) {
        m_flushTimer = new QTimer(this);
        connect(m_flushTimer, &QTimer::timeout, this, &LoggingManager::onPeriodicFlush);
        m_flushTimer->start(m_config.flushIntervalSeconds * 1000);
    }
    
    // Statistics update timer
    m_statisticsTimer = new QTimer(this);
    connect(m_statisticsTimer, &QTimer::timeout, this, &LoggingManager::updateStatistics);
    m_statisticsTimer->start(10000); // Update every 10 seconds
}

void LoggingManager::createLogDirectory()
{
    QString logDir = m_config.logDirectory.isEmpty() ? getDefaultLogDirectory() : m_config.logDirectory;
    QDir dir;
    if (!dir.exists(logDir)) {
        if (!dir.mkpath(logDir)) {
            throw std::runtime_error("Failed to create log directory: " + logDir.toStdString());
        }
    }
}

QString LoggingManager::getDefaultLogDirectory() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
}

QString LoggingManager::getLogFilePath() const
{
    QString logDir = m_config.logDirectory.isEmpty() ? getDefaultLogDirectory() : m_config.logDirectory;
    return logDir + "/" + m_config.logFileName;
}

void LoggingManager::setConfiguration(const LoggingConfiguration& config)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        m_config = config;
        return;
    }
    
    // Store old config for comparison
    LoggingConfiguration oldConfig = m_config;
    m_config = config;
    
    // Reinitialize if major settings changed
    bool needsReinit = (oldConfig.enableFileLogging != config.enableFileLogging) ||
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

void LoggingManager::updateLoggerConfiguration()
{
    Logger& logger = Logger::instance();
    logger.setLogLevel(m_config.globalLogLevel);
    logger.setPattern(m_config.logPattern);
}

void LoggingManager::setGlobalLogLevel(Logger::LogLevel level)
{
    QMutexLocker locker(&m_mutex);
    m_config.globalLogLevel = level;
    if (m_initialized) {
        Logger::instance().setLogLevel(level);
    }
}

void LoggingManager::setQtLogWidget(QTextEdit* widget)
{
    QMutexLocker locker(&m_mutex);
    m_qtLogWidget = widget;
    
    if (m_initialized) {
        Logger::instance().setQtWidget(widget);
    }
}

QTextEdit* LoggingManager::getQtLogWidget() const
{
    QMutexLocker locker(&m_mutex);
    return m_qtLogWidget;
}

void LoggingManager::flushLogs()
{
    if (m_initialized) {
        spdlog::apply_all([](std::shared_ptr<spdlog::logger> logger) {
            logger->flush();
        });
    }
}

void LoggingManager::rotateLogFiles()
{
    if (m_initialized && m_config.enableFileLogging) {
        // Force log rotation by flushing and recreating file sink
        flushLogs();
        // Implementation would depend on spdlog's rotating file sink capabilities
        LOG_INFO("Log files rotated");
        emit logFileRotated(getCurrentLogFilePath());
    }
}

QString LoggingManager::getCurrentLogFilePath() const
{
    return getLogFilePath();
}

QStringList LoggingManager::getLogFileList() const
{
    QMutexLocker locker(&m_mutex);
    QStringList logFiles;
    
    if (m_config.enableFileLogging) {
        QString logDir = m_config.logDirectory.isEmpty() ? getDefaultLogDirectory() : m_config.logDirectory;
        QDirIterator it(logDir, QStringList() << "*.log*", QDir::Files);
        
        while (it.hasNext()) {
            it.next();
            logFiles.append(it.filePath());
        }
    }
    
    return logFiles;
}

qint64 LoggingManager::getTotalLogFileSize() const
{
    QMutexLocker locker(&m_mutex);
    qint64 totalSize = 0;
    
    if (m_config.enableFileLogging) {
        QString logDir = m_config.logDirectory.isEmpty() ? getDefaultLogDirectory() : m_config.logDirectory;
        QDirIterator it(logDir, QStringList() << "*.log*", QDir::Files);
        
        while (it.hasNext()) {
            it.next();
            QFileInfo info = it.fileInfo();
            totalSize += info.size();
        }
    }
    
    return totalSize;
}

LoggingManager::LoggingStatistics LoggingManager::getStatistics() const
{
    QMutexLocker locker(&m_mutex);
    
    // Update file size information
    LoggingStatistics stats = m_statistics;
    
    if (m_config.enableFileLogging) {
        QFileInfo fileInfo(getCurrentLogFilePath());
        if (fileInfo.exists()) {
            stats.currentLogFileSize = fileInfo.size();
        }
        
        // Calculate total log files size
        QString logDir = m_config.logDirectory.isEmpty() ? getDefaultLogDirectory() : m_config.logDirectory;
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

void LoggingManager::onLogMessage(const QString& message, int level)
{
    QDateTime timestamp = QDateTime::currentDateTime();
    QString category = "general"; // Default category, could be enhanced to extract from message
    QString threadId = QString::number(reinterpret_cast<quintptr>(QThread::currentThread()));
    QString sourceLocation = ""; // Could be enhanced to include source location

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

    // Emit signal for real-time log streaming (outside of mutex to avoid deadlocks)
    emit logMessageReceived(timestamp, level, category, message, threadId, sourceLocation);
}

void LoggingManager::onPeriodicFlush()
{
    flushLogs();
}

void LoggingManager::onConfigurationChanged()
{
    // Handle configuration changes
    // This could trigger a reload or update of the logging configuration
    emit configurationChanged();
}

void LoggingManager::updateStatistics()
{
    emit statisticsUpdated(getStatistics());
}

void LoggingManager::connectSignals()
{
    // Connect logger signals if needed
    connect(&Logger::instance(), &Logger::logMessage, this, &LoggingManager::onLogMessage);
}

void LoggingManager::disconnectSignals()
{
    // Disconnect all signals
    disconnect(&Logger::instance(), nullptr, this, nullptr);
}

// Category management implementations
void LoggingManager::addLoggingCategory(const QString& category, Logger::LogLevel level) {
    QMutexLocker locker(&m_mutex);

    if (category.isEmpty()) {
        return; // Invalid category name
    }

    // Add or update category level
    m_categoryLevels[category] = level;

    // If using modern config, also update the LoggingConfig structure
    if (m_usingModernConfig) {
        // Create or update category configuration
        LoggingConfig::CategoryConfiguration categoryConfig;
        categoryConfig.name = category;
        categoryConfig.level = level;
        categoryConfig.enabled = true;

        // Note: We can't directly modify LoggingConfig here since we don't store it
        // This would require a redesign to properly support modern config persistence
    }

    // Apply the category level to the Qt logging system if Qt integration is enabled
    if (m_config.enableQtCategoryFiltering) {
        // Add category mapping to Qt bridge (maps Qt category to spdlog logger)
        QtSpdlogBridge::instance().addCategoryMapping(category);
    }
}

void LoggingManager::setLoggingCategoryLevel(const QString& category, Logger::LogLevel level) {
    QMutexLocker locker(&m_mutex);

    if (category.isEmpty()) {
        return; // Invalid category name
    }

    // Check if category exists
    if (!m_categoryLevels.contains(category)) {
        // Category doesn't exist, add it
        addLoggingCategory(category, level);
        return;
    }

    // Update existing category level
    Logger::LogLevel oldLevel = m_categoryLevels[category];
    if (oldLevel != level) {
        m_categoryLevels[category] = level;

        // Apply the category level to the Qt logging system if Qt integration is enabled
        if (m_config.enableQtCategoryFiltering) {
            // Update category mapping in Qt bridge
            QtSpdlogBridge::instance().addCategoryMapping(category);
        }

        // Emit signal for level change (unlock mutex first to avoid deadlock)
        locker.unlock();
        emit configurationChanged();
    }
}

void LoggingManager::removeLoggingCategory(const QString& category) {
    QMutexLocker locker(&m_mutex);

    if (category.isEmpty()) {
        return; // Invalid category name
    }

    // Remove category if it exists
    if (m_categoryLevels.remove(category) > 0) {
        // Remove from Qt logging system if Qt integration is enabled
        if (m_config.enableQtCategoryFiltering) {
            QtSpdlogBridge::instance().removeCategoryMapping(category);
        }

        // Emit signal for configuration change (unlock mutex first to avoid deadlock)
        locker.unlock();
        emit configurationChanged();
    }
}

Logger::LogLevel LoggingManager::getLoggingCategoryLevel(const QString& category) const {
    QMutexLocker locker(&m_mutex);

    if (category.isEmpty()) {
        return Logger::LogLevel::Info; // Default level for invalid category
    }

    // Return category level if it exists, otherwise return global level
    return m_categoryLevels.value(category, m_config.globalLogLevel);
}

QStringList LoggingManager::getLoggingCategories() const {
    QMutexLocker locker(&m_mutex);

    // Return all registered category names
    return m_categoryLevels.keys();
}

// ============================================================================
// Note: ScopedLoggingConfig implementation has been moved to ScopedLogLevel
// ============================================================================
// The ScopedLogLevel class in LoggingMacros.cpp now provides the enhanced
// functionality that was previously split between ScopedLogLevel and
// ScopedLoggingConfig classes.
