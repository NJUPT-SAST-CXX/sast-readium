#pragma once

#include <QObject>
#include <QSettings>
#include <QVariant>
#include <QHash>
#include <QString>
#include <QMap>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include <memory>
#include <functional>
#include "../logging/SimpleLogging.h"

/**
 * @brief ConfigurationManager - Centralized configuration management
 * 
 * This class provides a centralized, type-safe way to manage application
 * configuration. It follows the Singleton pattern and provides both
 * persistent (saved to disk) and transient (runtime only) configuration.
 */
class ConfigurationManager : public QObject {
    Q_OBJECT

public:
    ~ConfigurationManager();

    // Singleton access
    static ConfigurationManager& instance();
    
    // Configuration groups
    enum ConfigGroup {
        General,
        UI,
        Document,
        View,
        Navigation,
        Performance,
        Network,
        Advanced
    };
    
    // General configuration access
    QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setValue(const QString& key, const QVariant& value);
    
    // Group-based configuration access
    QVariant getValue(ConfigGroup group, const QString& key, 
                     const QVariant& defaultValue = QVariant()) const;
    void setValue(ConfigGroup group, const QString& key, const QVariant& value);
    
    // Type-safe accessors
    bool getBool(const QString& key, bool defaultValue = false) const;
    int getInt(const QString& key, int defaultValue = 0) const;
    double getDouble(const QString& key, double defaultValue = 0.0) const;
    QString getString(const QString& key, const QString& defaultValue = QString()) const;
    QStringList getStringList(const QString& key, const QStringList& defaultValue = QStringList()) const;
    
    // Configuration management
    void saveConfiguration();
    void loadConfiguration();
    void resetToDefaults();
    void resetGroup(ConfigGroup group);
    
    // Import/Export
    bool exportConfiguration(const QString& filePath);
    bool importConfiguration(const QString& filePath);
    
    // Configuration validation
    bool validateConfiguration();
    QStringList validationErrors() const { return m_validationErrors; }
    
    // Runtime configuration (not persisted)
    void setRuntimeValue(const QString& key, const QVariant& value);
    QVariant getRuntimeValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void clearRuntimeValues();
    
    // Configuration monitoring
    void watchKey(const QString& key);
    void unwatchKey(const QString& key);
    bool isWatching(const QString& key) const;

    // Configuration introspection
    QStringList allKeys() const;

signals:
    void configurationChanged(const QString& key, const QVariant& value);
    void configurationChanged(ConfigGroup group, const QString& key, const QVariant& value);
    void configurationSaved();
    void configurationLoaded();
    void configurationReset();
    void validationFailed(const QStringList& errors);

private:
    ConfigurationManager(QObject* parent = nullptr);
    ConfigurationManager(const ConfigurationManager&) = delete;
    ConfigurationManager& operator=(const ConfigurationManager&) = delete;
    
    // Helper methods
    QString groupToString(ConfigGroup group) const;
    void initializeDefaults();
    void notifyChange(const QString& key, const QVariant& value);
    void notifyChange(ConfigGroup group, const QString& key, const QVariant& value);
    
    // Storage
    std::unique_ptr<QSettings> m_settings;
    QHash<QString, QVariant> m_runtimeValues;
    QHash<QString, QVariant> m_defaults;
    QHash<QString, QVariant> m_savedConfiguration;  // Backup for save/load functionality
    QStringList m_watchedKeys;
    QStringList m_validationErrors;
    
    // Logging
    SastLogging::CategoryLogger m_logger{"ConfigurationManager"};
};

/**
 * @brief ConfigurationValidator - Validates configuration values
 */
class ConfigurationValidator {
public:
    explicit ConfigurationValidator(ConfigurationManager* manager);
    
    // Validation rules
    struct ValidationRule {
        QString key;
        std::function<bool(const QVariant&)> validator;
        QString errorMessage;
    };
    
    // Add validation rules
    void addRule(const ValidationRule& rule);
    void addRangeRule(const QString& key, int min, int max);
    void addRangeRule(const QString& key, double min, double max);
    void addRegexRule(const QString& key, const QString& pattern);
    void addEnumRule(const QString& key, const QStringList& validValues);
    
    // Validate configuration
    bool validate();
    QStringList errors() const { return m_errors; }
    
private:
    ConfigurationManager* m_manager;
    QList<ValidationRule> m_rules;
    QStringList m_errors;
};

/**
 * @brief ConfigurationProfile - Manages configuration profiles
 */
class ConfigurationProfile {
public:
    explicit ConfigurationProfile(const QString& name);
    
    // Profile management
    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }
    
    // Configuration storage
    void setValue(const QString& key, const QVariant& value);
    QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    QHash<QString, QVariant> values() const { return m_values; }
    
    // Profile operations
    void applyTo(ConfigurationManager* manager);
    void loadFrom(ConfigurationManager* manager);
    
    // Serialization
    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
    
private:
    QString m_name;
    QHash<QString, QVariant> m_values;
};

/**
 * @brief ConfigurationProfileManager - Manages multiple configuration profiles
 */
class ConfigurationProfileManager : public QObject {
    Q_OBJECT

public:
    explicit ConfigurationProfileManager(ConfigurationManager* manager,
                                       QObject* parent = nullptr);
    ~ConfigurationProfileManager();
    
    // Profile management
    void addProfile(std::unique_ptr<ConfigurationProfile> profile);
    void removeProfile(const QString& name);
    ConfigurationProfile* getProfile(const QString& name);
    QStringList profileNames() const;
    
    // Active profile
    void setActiveProfile(const QString& name);
    QString activeProfile() const { return m_activeProfileName; }
    void applyActiveProfile();
    
    // Profile operations
    void saveProfiles();
    void loadProfiles();
    bool exportProfile(const QString& name, const QString& filePath);
    bool importProfile(const QString& filePath);
    
signals:
    void profileAdded(const QString& name);
    void profileRemoved(const QString& name);
    void activeProfileChanged(const QString& name);
    
private:
    ConfigurationManager* m_manager;
    QHash<QString, ConfigurationProfile*> m_profiles;
    QString m_activeProfileName;
};

// Convenience macros for configuration access
#define CONFIG ConfigurationManager::instance()
#define CONFIG_GET(key, defaultValue) CONFIG.getValue(key, defaultValue)
#define CONFIG_SET(key, value) CONFIG.setValue(key, value)
#define CONFIG_BOOL(key, defaultValue) CONFIG.getBool(key, defaultValue)
#define CONFIG_INT(key, defaultValue) CONFIG.getInt(key, defaultValue)
#define CONFIG_STRING(key, defaultValue) CONFIG.getString(key, defaultValue)
