#include "LoggingConfig.h"
#include <QJsonArray>
#include <QJsonValue>
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <QMutexLocker>
#include <QDebug>

// Static member definitions
const LoggingConfig::GlobalConfiguration LoggingConfig::s_defaultGlobalConfig = {};

const QList<LoggingConfig::SinkConfiguration> LoggingConfig::s_defaultSinkConfigs = {
    {"console", "console", Logger::LogLevel::Debug, "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v", true},
    {"file", "rotating_file", Logger::LogLevel::Info, "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v", true}
};

const QHash<QString, QString> LoggingConfig::s_environmentVariableMap = {
    {"SAST_READIUM_LOG_LEVEL", "globalLevel"},
    {"SAST_READIUM_LOG_PATTERN", "globalPattern"},
    {"SAST_READIUM_LOG_ASYNC", "asyncLogging"},
    {"SAST_READIUM_LOG_CONSOLE", "consoleEnabled"},
    {"SAST_READIUM_LOG_FILE", "fileEnabled"},
    {"SAST_READIUM_LOG_FILE_PATH", "logFilePath"}
};

LoggingConfig::LoggingConfig(QObject* parent)
    : QObject(parent), m_fileWatcher(nullptr)
{
    initializeDefaults();
    connectSignals();
}

void LoggingConfig::initializeDefaults()
{
    m_globalConfig = s_defaultGlobalConfig;
    m_sinkConfigs = s_defaultSinkConfigs;
    
    // Set default file path for file sink
    for (auto& sink : m_sinkConfigs) {
        if (sink.type == "rotating_file" && sink.filename.isEmpty()) {
            QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
            sink.filename = logDir + "/sast-readium.log";
        }
    }
}

bool LoggingConfig::loadFromSettings(QSettings& settings)
{
    QMutexLocker locker(&m_mutex);
    
    try {
        settings.beginGroup("Logging");
        
        // Load global configuration
        settings.beginGroup("Global");
        // m_globalConfig = globalConfigFromSettings(settings);
        settings.endGroup();
        
        // Load sink configurations
        settings.beginGroup("Sinks");
        m_sinkConfigs.clear();
        QStringList sinkNames = settings.childGroups();
        for (const QString& sinkName : sinkNames) {
            settings.beginGroup(sinkName);
            // SinkConfiguration config = sinkConfigFromSettings(settings);
            // config.name = sinkName;
            // m_sinkConfigs.append(config);
            settings.endGroup();
        }
        settings.endGroup();
        
        // Load category configurations
        settings.beginGroup("Categories");
        m_categoryConfigs.clear();
        QStringList categoryNames = settings.childGroups();
        for (const QString& categoryName : categoryNames) {
            settings.beginGroup(categoryName);
            // CategoryConfiguration config = categoryConfigFromSettings(settings);
            // config.name = categoryName;
            // m_categoryConfigs.append(config);
            settings.endGroup();
        }
        settings.endGroup();
        
        settings.endGroup(); // Logging
        
        m_configSource = ConfigSource::SettingsFile;
        emit configurationLoaded(m_configSource);
        return true;
        
    } catch (const std::exception& e) {
        emit configurationError(QString("Failed to load from settings: %1").arg(e.what()));
        return false;
    }
}

bool LoggingConfig::saveToSettings(QSettings& settings) const
{
    QMutexLocker locker(&m_mutex);
    
    try {
        settings.beginGroup("Logging");
        settings.remove(""); // Clear existing logging settings
        
        // Save global configuration
        settings.beginGroup("Global");
        // globalConfigToSettings(settings, m_globalConfig);
        settings.endGroup();
        
        // Save sink configurations
        settings.beginGroup("Sinks");
        for (const auto& sink : m_sinkConfigs) {
            settings.beginGroup(sink.name);
            // sinkConfigToSettings(settings, sink);
            settings.endGroup();
        }
        settings.endGroup();
        
        // Save category configurations
        settings.beginGroup("Categories");
        for (const auto& category : m_categoryConfigs) {
            settings.beginGroup(category.name);
            // categoryConfigToSettings(settings, category);
            settings.endGroup();
        }
        settings.endGroup();
        
        settings.endGroup(); // Logging
        
        emit configurationSaved(ConfigSource::SettingsFile);
        return true;
        
    } catch (const std::exception& e) {
        emit configurationError(QString("Failed to save to settings: %1").arg(e.what()));
        return false;
    }
}

bool LoggingConfig::loadFromJsonFile(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        emit configurationError(QString("Cannot open config file: %1").arg(filename));
        return false;
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        emit configurationError(QString("JSON parse error: %1").arg(error.errorString()));
        return false;
    }
    
    bool result = loadFromJsonObject(doc.object());
    if (result) {
        m_configSource = ConfigSource::JsonFile;
        m_watchedConfigFile = filename;
    }
    return result;
}

bool LoggingConfig::saveToJsonFile(const QString& filename) const
{
    QJsonDocument doc(saveToJsonObject());
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        emit configurationError(QString("Cannot write config file: %1").arg(filename));
        return false;
    }
    
    file.write(doc.toJson());
    emit configurationSaved(ConfigSource::JsonFile);
    return true;
}

bool LoggingConfig::loadFromJsonObject(const QJsonObject& json)
{
    QMutexLocker locker(&m_mutex);
    
    try {
        // Load global configuration
        if (json.contains("global")) {
            // m_globalConfig = globalConfigFromJson(json["global"].toObject());
        }
        
        // Load sink configurations
        if (json.contains("sinks")) {
            m_sinkConfigs.clear();
            QJsonArray sinksArray = json["sinks"].toArray();
            for (const auto& value : sinksArray) {
                // SinkConfiguration config = sinkConfigFromJson(value.toObject());
                // m_sinkConfigs.append(config);
            }
        }
        
        // Load category configurations
        if (json.contains("categories")) {
            m_categoryConfigs.clear();
            QJsonArray categoriesArray = json["categories"].toArray();
            for (const auto& value : categoriesArray) {
                // CategoryConfiguration config = categoryConfigFromJson(value.toObject());
                // m_categoryConfigs.append(config);
            }
        }
        
        emit configurationLoaded(m_configSource);
        return true;
        
    } catch (const std::exception& e) {
        emit configurationError(QString("Failed to load from JSON: %1").arg(e.what()));
        return false;
    }
}

QJsonObject LoggingConfig::saveToJsonObject() const
{
    QMutexLocker locker(&m_mutex);
    
    QJsonObject json;
    
    // Save global configuration
    // json["global"] = globalConfigToJson(m_globalConfig);
    
    // Save sink configurations
    QJsonArray sinksArray;
    for (const auto& sink : m_sinkConfigs) {
        // sinksArray.append(sinkConfigToJson(sink));
    }
    json["sinks"] = sinksArray;
    
    // Save category configurations
    QJsonArray categoriesArray;
    for (const auto& category : m_categoryConfigs) {
        // categoriesArray.append(categoryConfigToJson(category));
    }
    json["categories"] = categoriesArray;
    
    return json;
}

void LoggingConfig::setGlobalConfig(const GlobalConfiguration& config)
{
    QMutexLocker locker(&m_mutex);
    m_globalConfig = config;
    emit globalConfigurationChanged();
    emit configurationChanged();
}

void LoggingConfig::setGlobalLogLevel(Logger::LogLevel level)
{
    QMutexLocker locker(&m_mutex);
    m_globalConfig.globalLevel = level;
    emit globalConfigurationChanged();
    emit configurationChanged();
}

void LoggingConfig::addSinkConfiguration(const SinkConfiguration& config)
{
    QMutexLocker locker(&m_mutex);
    
    // Remove existing sink with same name
    m_sinkConfigs.removeIf([&config](const SinkConfiguration& existing) {
        return existing.name == config.name;
    });
    
    m_sinkConfigs.append(config);
    emit sinkConfigurationChanged(config.name);
    emit configurationChanged();
}

void LoggingConfig::addCategoryConfiguration(const CategoryConfiguration& config)
{
    QMutexLocker locker(&m_mutex);
    
    // Remove existing category with same name
    m_categoryConfigs.removeIf([&config](const CategoryConfiguration& existing) {
        return existing.name == config.name;
    });
    
    m_categoryConfigs.append(config);
    emit categoryConfigurationChanged(config.name);
    emit configurationChanged();
}

void LoggingConfig::enableConsoleLogging(Logger::LogLevel level, bool colored)
{
    SinkConfiguration config;
    config.name = "console";
    config.type = "console";
    config.level = level;
    config.enabled = true;
    config.colorEnabled = colored;
    config.pattern = colored ? "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v" : "[%Y-%m-%d %H:%M:%S.%e] [%l] %v";
    
    addSinkConfiguration(config);
}

void LoggingConfig::enableFileLogging(const QString& filename, Logger::LogLevel level)
{
    SinkConfiguration config;
    config.name = "file";
    config.type = "rotating_file";
    config.level = level;
    config.enabled = true;
    config.filename = filename;
    config.maxFileSize = 10 * 1024 * 1024; // 10MB
    config.maxFiles = 5;
    config.pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v";
    
    addSinkConfiguration(config);
}

void LoggingConfig::loadDevelopmentPreset()
{
    // resetToDefaults();
    
    // Enable console with debug level and colors
    enableConsoleLogging(Logger::LogLevel::Debug, true);
    
    // Enable file logging with info level
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    QDir().mkpath(logDir);
    enableFileLogging(logDir + "/sast-readium-dev.log", Logger::LogLevel::Info);
    
    // Enable performance and memory logging
    m_globalConfig.enablePerformanceLogging = true;
    m_globalConfig.enableMemoryLogging = true;
    m_globalConfig.enableSourceLocation = true;
    m_globalConfig.enableThreadId = true;
    
    emit configurationChanged();
}

void LoggingConfig::loadProductionPreset()
{
    // resetToDefaults();
    
    // Disable console logging in production
    // Enable file logging with warning level
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    QDir().mkpath(logDir);
    enableFileLogging(logDir + "/sast-readium.log", Logger::LogLevel::Warning);
    
    // Production settings
    m_globalConfig.globalLevel = Logger::LogLevel::Warning;
    m_globalConfig.asyncLogging = true;
    m_globalConfig.enablePerformanceLogging = false;
    m_globalConfig.enableMemoryLogging = false;
    m_globalConfig.enableSourceLocation = false;
    m_globalConfig.enableThreadId = false;
    
    emit configurationChanged();
}

Logger::LogLevel LoggingConfig::parseLogLevelFromString(const QString& levelStr) const
{
    QString lower = levelStr.toLower();
    if (lower == "trace") return Logger::LogLevel::Trace;
    if (lower == "debug") return Logger::LogLevel::Debug;
    if (lower == "info") return Logger::LogLevel::Info;
    if (lower == "warning" || lower == "warn") return Logger::LogLevel::Warning;
    if (lower == "error") return Logger::LogLevel::Error;
    if (lower == "critical") return Logger::LogLevel::Critical;
    if (lower == "off") return Logger::LogLevel::Off;
    return Logger::LogLevel::Info; // Default
}

QString LoggingConfig::logLevelToString(Logger::LogLevel level) const
{
    switch (level) {
        case Logger::LogLevel::Trace: return "trace";
        case Logger::LogLevel::Debug: return "debug";
        case Logger::LogLevel::Info: return "info";
        case Logger::LogLevel::Warning: return "warning";
        case Logger::LogLevel::Error: return "error";
        case Logger::LogLevel::Critical: return "critical";
        case Logger::LogLevel::Off: return "off";
        default: return "info";
    }
}

void LoggingConfig::connectSignals()
{
    // Connect internal signals if needed
}

void LoggingConfig::disconnectSignals()
{
    // Disconnect internal signals if needed
}

// LoggingConfigBuilder implementation
LoggingConfigBuilder::LoggingConfigBuilder()
    : m_config(std::make_unique<LoggingConfig>())
{
}

LoggingConfigBuilder& LoggingConfigBuilder::setGlobalLevel(Logger::LogLevel level)
{
    m_config->setGlobalLogLevel(level);
    return *this;
}

LoggingConfigBuilder& LoggingConfigBuilder::addConsoleSink(const QString& name, Logger::LogLevel level)
{
    m_config->enableConsoleLogging(level, true);
    return *this;
}

LoggingConfigBuilder& LoggingConfigBuilder::useDevelopmentPreset()
{
    m_config->loadDevelopmentPreset();
    return *this;
}

LoggingConfig& LoggingConfigBuilder::build() const
{
    return *m_config;
}

// std::unique_ptr<LoggingConfig> LoggingConfigBuilder::buildUnique() const
// {
//     return std::make_unique<LoggingConfig>(*m_config);
// }
