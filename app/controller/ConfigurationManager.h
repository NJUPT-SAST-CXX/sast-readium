#pragma once

#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <cstdint>
#include <functional>
#include <memory>
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
    ~ConfigurationManager() override;

    // Singleton access
    static ConfigurationManager& instance();

    // Configuration groups
    enum class ConfigGroup : std::uint8_t {
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
    [[nodiscard]] QVariant getValue(
        const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setValue(const QString& key, const QVariant& value);

    // Group-based configuration access
    [[nodiscard]] QVariant getValue(
        ConfigGroup group, const QString& key,
        const QVariant& defaultValue = QVariant()) const;
    void setValue(ConfigGroup group, const QString& key, const QVariant& value);

    // Type-safe accessors
    [[nodiscard]] bool getBool(const QString& key,
                               bool defaultValue = false) const;
    [[nodiscard]] int getInt(const QString& key, int defaultValue = 0) const;
    [[nodiscard]] double getDouble(const QString& key,
                                   double defaultValue = 0.0) const;
    [[nodiscard]] QString getString(
        const QString& key, const QString& defaultValue = QString()) const;
    [[nodiscard]] QStringList getStringList(
        const QString& key,
        const QStringList& defaultValue = QStringList()) const;

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
    [[nodiscard]] QStringList validationErrors() const {
        return m_validationErrors;
    }

    // Runtime configuration (not persisted)
    void setRuntimeValue(const QString& key, const QVariant& value);
    [[nodiscard]] QVariant getRuntimeValue(
        const QString& key, const QVariant& defaultValue = QVariant()) const;
    void clearRuntimeValues();

    // Configuration monitoring
    void watchKey(const QString& key);
    void unwatchKey(const QString& key);
    [[nodiscard]] bool isWatching(const QString& key) const;

    // Configuration introspection
    [[nodiscard]] QStringList allKeys() const;

signals:
    void configurationChanged(const QString& key, const QVariant& value);
    void configurationChanged(ConfigGroup group, const QString& key,
                              const QVariant& value);
    void configurationSaved();
    void configurationLoaded();
    void configurationReset();
    void validationFailed(const QStringList& errors);

private:
    explicit ConfigurationManager(QObject* parent = nullptr);

public:
    ConfigurationManager(const ConfigurationManager&) = delete;
    ConfigurationManager& operator=(const ConfigurationManager&) = delete;
    ConfigurationManager(ConfigurationManager&&) = delete;
    ConfigurationManager& operator=(ConfigurationManager&&) = delete;

private:
    // Helper methods
    [[nodiscard]] QString groupToString(ConfigGroup group) const;
    void initializeDefaults();
    void notifyChange(const QString& key, const QVariant& value);
    void notifyChange(ConfigGroup group, const QString& key,
                      const QVariant& value);

    // Storage
    std::unique_ptr<QSettings> m_settings;
    QHash<QString, QVariant> m_runtimeValues;
    QHash<QString, QVariant> m_defaults;
    QHash<QString, QVariant>
        m_savedConfiguration;  // Backup for save/load functionality
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
    [[nodiscard]] QStringList errors() const { return m_errors; }

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
    [[nodiscard]] QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    // Configuration storage
    void setValue(const QString& key, const QVariant& value);
    [[nodiscard]] QVariant getValue(
        const QString& key, const QVariant& defaultValue = QVariant()) const;
    [[nodiscard]] QHash<QString, QVariant> values() const { return m_values; }

    // Profile operations
    void applyTo(ConfigurationManager* manager);
    void loadFrom(ConfigurationManager* manager);

    // Serialization
    [[nodiscard]] QByteArray serialize() const;
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
    ~ConfigurationProfileManager() override;

    // Special member functions
    ConfigurationProfileManager(const ConfigurationProfileManager&) = delete;
    ConfigurationProfileManager& operator=(const ConfigurationProfileManager&) =
        delete;
    ConfigurationProfileManager(ConfigurationProfileManager&&) = delete;
    ConfigurationProfileManager& operator=(ConfigurationProfileManager&&) =
        delete;

    // Profile management
    void addProfile(std::unique_ptr<ConfigurationProfile> profile);
    void removeProfile(const QString& name);
    ConfigurationProfile* getProfile(const QString& name);
    [[nodiscard]] QStringList profileNames() const;

    // Active profile
    void setActiveProfile(const QString& name);
    [[nodiscard]] QString activeProfile() const { return m_activeProfileName; }
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
    SastLogging::CategoryLogger m_logger{"ConfigurationProfileManager"};
};

// Convenience macros for configuration access
#define CONFIG ConfigurationManager::instance()
#define CONFIG_GET(key, defaultValue) CONFIG.getValue(key, defaultValue)
#define CONFIG_SET(key, value) CONFIG.setValue(key, value)
#define CONFIG_BOOL(key, defaultValue) CONFIG.getBool(key, defaultValue)
#define CONFIG_INT(key, defaultValue) CONFIG.getInt(key, defaultValue)
#define CONFIG_STRING(key, defaultValue) CONFIG.getString(key, defaultValue)
