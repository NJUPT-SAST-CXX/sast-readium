#pragma once

#include "Logger.h"

#include <QFileSystemWatcher>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>

/**
 * @brief Runtime configuration system for logging
 *
 * This class provides a comprehensive configuration system that allows
 * runtime control of logging levels, output destinations, formatting,
 * and other logging parameters. It supports both programmatic configuration
 * and configuration file-based setup.
 */
class LoggingConfig : public QObject {
    Q_OBJECT

public:
    enum class ConfigSource {
        Default,
        SettingsFile,
        JsonFile,
        Environment,
        Programmatic
    };

    struct SinkConfiguration {
        QString name;
        QString type;  // "console", "file", "rotating_file", "qt_widget"
        Logger::LogLevel level = Logger::LogLevel::Info;
        QString pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v";
        bool enabled = true;

        // File-specific settings
        QString filename;
        size_t maxFileSize = 10 * 1024 * 1024;  // 10MB
        size_t maxFiles = 5;
        bool rotateOnStartup = false;

        // Console-specific settings
        bool colorEnabled = true;

        // Qt widget-specific settings
        QString widgetObjectName;

        // Custom properties
        QHash<QString, QVariant> customProperties;
    };

    struct CategoryConfiguration {
        QString name;
        Logger::LogLevel level = Logger::LogLevel::Info;
        bool enabled = true;
        QString pattern;           // Optional category-specific pattern
        QStringList enabledSinks;  // Which sinks this category should use
    };

    struct GlobalConfiguration {
        Logger::LogLevel globalLevel = Logger::LogLevel::Info;
        QString globalPattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v";
        bool asyncLogging = false;
        size_t asyncQueueSize = 8192;
        int flushIntervalSeconds = 5;
        bool autoFlushOnWarning = true;
        bool enableSourceLocation = false;
        bool enableThreadId = false;
        bool enableProcessId = false;

        // Qt integration settings
        bool redirectQtMessages = true;
        bool enableQtCategoryFiltering = true;

        // Performance settings
        bool enablePerformanceLogging = false;
        int performanceThresholdMs = 100;

        // Debug settings
        bool enableMemoryLogging = false;
        int memoryLoggingIntervalSeconds = 60;
    };

    explicit LoggingConfig(QObject* parent = nullptr);
    ~LoggingConfig() = default;

    // Configuration loading and saving
    bool loadFromSettings(QSettings& settings);
    bool saveToSettings(QSettings& settings);
    bool loadFromJsonFile(const QString& filename);
    bool saveToJsonFile(const QString& filename);
    bool loadFromJsonObject(const QJsonObject& json);
    QJsonObject saveToJsonObject() const;
    bool loadFromEnvironment();

    // Configuration management
    void setConfigurationSource(ConfigSource source) {
        m_configSource = source;
    }
    ConfigSource getConfigurationSource() const { return m_configSource; }
    void resetToDefaults();
    bool isValid() const;
    QStringList validate() const;

    // Global configuration
    const GlobalConfiguration& getGlobalConfig() const {
        return m_globalConfig;
    }
    void setGlobalConfig(const GlobalConfiguration& config);
    void setGlobalLogLevel(Logger::LogLevel level);
    void setGlobalPattern(const QString& pattern);
    void setAsyncLogging(bool enabled, size_t queueSize = 8192);
    void setFlushInterval(int seconds);
    void setAutoFlushOnWarning(bool enabled);

    // Sink configuration
    QList<SinkConfiguration> getSinkConfigurations() const {
        return m_sinkConfigs;
    }
    void setSinkConfigurations(const QList<SinkConfiguration>& configs);
    void addSinkConfiguration(const SinkConfiguration& config);
    void removeSinkConfiguration(const QString& name);
    void updateSinkConfiguration(const QString& name,
                                 const SinkConfiguration& config);
    SinkConfiguration getSinkConfiguration(const QString& name) const;
    bool hasSinkConfiguration(const QString& name) const;

    // Category configuration
    QList<CategoryConfiguration> getCategoryConfigurations() const {
        return m_categoryConfigs;
    }
    void setCategoryConfigurations(const QList<CategoryConfiguration>& configs);
    void addCategoryConfiguration(const CategoryConfiguration& config);
    void removeCategoryConfiguration(const QString& name);
    void updateCategoryConfiguration(const QString& name,
                                     const CategoryConfiguration& config);
    CategoryConfiguration getCategoryConfiguration(const QString& name) const;
    bool hasCategoryConfiguration(const QString& name) const;
    void setCategoryLevel(const QString& category, Logger::LogLevel level);
    Logger::LogLevel getCategoryLevel(const QString& category) const;

    // Convenience methods for common configurations
    void enableConsoleLogging(Logger::LogLevel level = Logger::LogLevel::Debug,
                              bool colored = true);
    void enableFileLogging(const QString& filename,
                           Logger::LogLevel level = Logger::LogLevel::Info);
    void enableRotatingFileLogging(
        const QString& filename, size_t maxSize, size_t maxFiles,
        Logger::LogLevel level = Logger::LogLevel::Info);
    void enableQtWidgetLogging(
        const QString& widgetName,
        Logger::LogLevel level = Logger::LogLevel::Debug);
    void disableAllSinks();
    void enableDefaultConfiguration();

    // Runtime configuration changes
    void applyConfiguration();
    void reloadConfiguration();
    bool isAutoReloadEnabled() const { return m_autoReload; }
    void setAutoReload(bool enabled);

    // Configuration file watching
    void watchConfigurationFile(const QString& filename);
    void stopWatchingConfigurationFile();

    // Preset configurations
    void loadDevelopmentPreset();
    void loadProductionPreset();
    void loadDebugPreset();
    void loadPerformancePreset();
    void loadMinimalPreset();

    // Configuration export/import
    QString exportToString() const;

    // Utility methods
    Logger::LogLevel stringToLogLevel(const QString& levelStr) const;
    bool importFromString(const QString& configString);
    QByteArray exportToBinary() const;
    bool importFromBinary(const QByteArray& data);

    // Configuration comparison and merging
    bool isEquivalentTo(const LoggingConfig& other) const;
    void mergeWith(const LoggingConfig& other, bool overwriteExisting = true);
    LoggingConfig createDiff(const LoggingConfig& other) const;

    // Environment variable support
    static QStringList getSupportedEnvironmentVariables();
    void applyEnvironmentOverrides();

public slots:
    void onConfigurationFileChanged(const QString& path);
    void onGlobalLevelChanged(int level);
    void onSinkLevelChanged(const QString& sinkName, int level);
    void onCategoryLevelChanged(const QString& categoryName, int level);

signals:
    void configurationChanged();
    void configurationLoaded(ConfigSource source);
    void configurationSaved(ConfigSource source);
    void configurationError(const QString& error);
    void sinkConfigurationChanged(const QString& sinkName);
    void categoryConfigurationChanged(const QString& categoryName);
    void globalConfigurationChanged();

private slots:
    void handleFileSystemChange(const QString& path);

private:
    void initializeDefaults();
    void connectSignals();
    void disconnectSignals();

    // JSON serialization helpers
    QJsonObject sinkConfigToJson(const SinkConfiguration& config) const;
    SinkConfiguration sinkConfigFromJson(const QJsonObject& json) const;
    QJsonObject categoryConfigToJson(const CategoryConfiguration& config) const;
    CategoryConfiguration categoryConfigFromJson(const QJsonObject& json) const;
    QJsonObject globalConfigToJson(const GlobalConfiguration& config) const;
    GlobalConfiguration globalConfigFromJson(const QJsonObject& json) const;

    // Settings serialization helpers
    void sinkConfigToSettings(QSettings& settings,
                              const SinkConfiguration& config) const;
    SinkConfiguration sinkConfigFromSettings(QSettings& settings) const;
    void categoryConfigToSettings(QSettings& settings,
                                  const CategoryConfiguration& config) const;
    CategoryConfiguration categoryConfigFromSettings(QSettings& settings) const;
    void globalConfigToSettings(QSettings& settings,
                                const GlobalConfiguration& config) const;
    GlobalConfiguration globalConfigFromSettings(QSettings& settings) const;

    // Environment variable helpers
    QString getEnvironmentVariable(
        const QString& name, const QString& defaultValue = QString()) const;
    Logger::LogLevel parseLogLevelFromString(const QString& levelStr) const;
    QString logLevelToString(Logger::LogLevel level) const;

    // Validation helpers
    bool validateSinkConfiguration(const SinkConfiguration& config,
                                   QStringList& errors) const;
    bool validateCategoryConfiguration(const CategoryConfiguration& config,
                                       QStringList& errors) const;
    bool validateGlobalConfiguration(const GlobalConfiguration& config,
                                     QStringList& errors) const;

    GlobalConfiguration m_globalConfig;
    QList<SinkConfiguration> m_sinkConfigs;
    QList<CategoryConfiguration> m_categoryConfigs;

    ConfigSource m_configSource = ConfigSource::Default;
    bool m_autoReload = false;
    QString m_watchedConfigFile;
    QFileSystemWatcher* m_fileWatcher = nullptr;

    mutable QMutex m_mutex;

    // Default configurations
    static const GlobalConfiguration s_defaultGlobalConfig;
    static const QList<SinkConfiguration> s_defaultSinkConfigs;
    static const QHash<QString, QString> s_environmentVariableMap;
};

/**
 * @brief Configuration builder for fluent API
 */
class LoggingConfigBuilder {
public:
    LoggingConfigBuilder();

    // Global settings
    LoggingConfigBuilder& setGlobalLevel(Logger::LogLevel level);
    LoggingConfigBuilder& setGlobalPattern(const QString& pattern);
    LoggingConfigBuilder& enableAsyncLogging(size_t queueSize = 8192);
    LoggingConfigBuilder& setFlushInterval(int seconds);
    LoggingConfigBuilder& enableAutoFlush(bool enabled = true);

    // Sink configuration
    LoggingConfigBuilder& addConsoleSink(
        const QString& name = "console",
        Logger::LogLevel level = Logger::LogLevel::Debug);
    LoggingConfigBuilder& addFileSink(
        const QString& name, const QString& filename,
        Logger::LogLevel level = Logger::LogLevel::Info);
    LoggingConfigBuilder& addRotatingFileSink(
        const QString& name, const QString& filename, size_t maxSize,
        size_t maxFiles, Logger::LogLevel level = Logger::LogLevel::Info);
    LoggingConfigBuilder& addQtWidgetSink(
        const QString& name, const QString& widgetName,
        Logger::LogLevel level = Logger::LogLevel::Debug);

    // Category configuration
    LoggingConfigBuilder& addCategory(
        const QString& name, Logger::LogLevel level = Logger::LogLevel::Info);
    LoggingConfigBuilder& setCategoryLevel(const QString& name,
                                           Logger::LogLevel level);

    // Preset configurations
    LoggingConfigBuilder& useDevelopmentPreset();
    LoggingConfigBuilder& useProductionPreset();
    LoggingConfigBuilder& useDebugPreset();

    // Build the configuration
    LoggingConfig& build();
    std::unique_ptr<LoggingConfig> buildUnique();

private:
    std::unique_ptr<LoggingConfig> m_config;
};
