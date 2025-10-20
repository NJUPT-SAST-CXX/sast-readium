#include "ConfigurationManager.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>

// ConfigurationManager implementation
ConfigurationManager::ConfigurationManager(QObject* parent)
    : QObject(parent), m_logger("ConfigurationManager") {
    // Initialize QSettings with organization and application name
    QString organization = QApplication::organizationName().isEmpty()
                               ? "SAST"
                               : QApplication::organizationName();
    QString application = QApplication::applicationName().isEmpty()
                              ? "Readium"
                              : QApplication::applicationName();

    m_settings = std::make_unique<QSettings>(organization, application);

    initializeDefaults();
    m_logger.debug("ConfigurationManager initialized");
}

ConfigurationManager::~ConfigurationManager() {
    m_logger.debug("ConfigurationManager destroyed");
}

ConfigurationManager& ConfigurationManager::instance() {
    static ConfigurationManager instance;
    return instance;
}

QVariant ConfigurationManager::getValue(const QString& key,
                                        const QVariant& defaultValue) const {
    // Only check persistent settings - runtime values are accessed via
    // getRuntimeValue()
    return m_settings->value(key, defaultValue);
}

void ConfigurationManager::setValue(const QString& key, const QVariant& value) {
    QVariant oldValue = getValue(key);
    m_settings->setValue(key, value);

    if (oldValue != value) {
        notifyChange(key, value);
    }
}

QVariant ConfigurationManager::getValue(ConfigGroup group, const QString& key,
                                        const QVariant& defaultValue) const {
    QString fullKey = groupToString(group) + "/" + key;
    return getValue(fullKey, defaultValue);
}

void ConfigurationManager::setValue(ConfigGroup group, const QString& key,
                                    const QVariant& value) {
    QString fullKey = groupToString(group) + "/" + key;
    setValue(fullKey, value);
    notifyChange(group, key, value);
}

bool ConfigurationManager::getBool(const QString& key,
                                   bool defaultValue) const {
    return getValue(key, defaultValue).toBool();
}

int ConfigurationManager::getInt(const QString& key, int defaultValue) const {
    return getValue(key, defaultValue).toInt();
}

double ConfigurationManager::getDouble(const QString& key,
                                       double defaultValue) const {
    return getValue(key, defaultValue).toDouble();
}

QString ConfigurationManager::getString(const QString& key,
                                        const QString& defaultValue) const {
    return getValue(key, defaultValue).toString();
}

QStringList ConfigurationManager::getStringList(
    const QString& key, const QStringList& defaultValue) const {
    return getValue(key, defaultValue).toStringList();
}

void ConfigurationManager::saveConfiguration() {
    // Save current configuration to backup
    m_savedConfiguration.clear();
    for (const QString& key : m_settings->allKeys()) {
        m_savedConfiguration[key] = m_settings->value(key);
    }

    // Also sync to persistent storage
    m_settings->sync();
    emit configurationSaved();
    m_logger.info("Configuration saved");
}

void ConfigurationManager::loadConfiguration() {
    // Restore from backup if available
    if (!m_savedConfiguration.isEmpty()) {
        for (auto it = m_savedConfiguration.begin();
             it != m_savedConfiguration.end(); ++it) {
            m_settings->setValue(it.key(), it.value());
        }
    }

    // Also sync from persistent storage
    m_settings->sync();
    emit configurationLoaded();
    m_logger.info("Configuration loaded");
}

void ConfigurationManager::resetToDefaults() {
    m_settings->clear();

    // Restore default values
    for (auto it = m_defaults.begin(); it != m_defaults.end(); ++it) {
        m_settings->setValue(it.key(), it.value());
    }

    emit configurationReset();
    m_logger.info("Configuration reset to defaults");
}

void ConfigurationManager::resetGroup(ConfigGroup group) {
    QString groupName = groupToString(group);
    m_settings->beginGroup(groupName);
    m_settings->remove("");  // Remove all keys in this group
    m_settings->endGroup();

    m_logger.info(QString("Configuration group '%1' reset").arg(groupName));
}

bool ConfigurationManager::exportConfiguration(const QString& filePath) {
    try {
        QJsonObject configObject;

        // Export all settings
        for (const QString& key : m_settings->allKeys()) {
            configObject[key] = QJsonValue::fromVariant(m_settings->value(key));
        }

        QJsonDocument doc(configObject);
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson());
            m_logger.info(
                QString("Configuration exported to: %1").arg(filePath));
            return true;
        }
    } catch (const std::exception& e) {
        m_logger.error(
            QString("Failed to export configuration: %1").arg(e.what()));
    }

    return false;
}

bool ConfigurationManager::importConfiguration(const QString& filePath) {
    try {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QJsonObject configObject = doc.object();

            // Import all settings
            for (auto it = configObject.begin(); it != configObject.end();
                 ++it) {
                m_settings->setValue(it.key(), it.value().toVariant());
            }

            m_logger.info(
                QString("Configuration imported from: %1").arg(filePath));
            return true;
        }
    } catch (const std::exception& e) {
        m_logger.error(
            QString("Failed to import configuration: %1").arg(e.what()));
    }

    return false;
}

bool ConfigurationManager::validateConfiguration() {
    m_validationErrors.clear();

    // Basic validation - can be extended
    bool isValid = true;

    // Validate that required directories exist, create them if needed
    QString logDir = getString(
        "logging/directory",
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QDir dir;
    if (!dir.exists(logDir)) {
        if (!dir.mkpath(logDir)) {
            m_validationErrors
                << QString("Failed to create log directory: %1").arg(logDir);
            isValid = false;
        } else {
            m_logger.info(QString("Created log directory: %1").arg(logDir));
        }
    }

    if (!isValid) {
        emit validationFailed(m_validationErrors);
    }

    return isValid;
}

void ConfigurationManager::setRuntimeValue(const QString& key,
                                           const QVariant& value) {
    m_runtimeValues[key] = value;
}

QVariant ConfigurationManager::getRuntimeValue(
    const QString& key, const QVariant& defaultValue) const {
    return m_runtimeValues.value(key, defaultValue);
}

void ConfigurationManager::clearRuntimeValues() { m_runtimeValues.clear(); }

void ConfigurationManager::watchKey(const QString& key) {
    if (!m_watchedKeys.contains(key)) {
        m_watchedKeys.append(key);
    }
}

void ConfigurationManager::unwatchKey(const QString& key) {
    m_watchedKeys.removeAll(key);
}

bool ConfigurationManager::isWatching(const QString& key) const {
    return m_watchedKeys.contains(key);
}

QString ConfigurationManager::groupToString(ConfigGroup group) const {
    switch (group) {
        case ConfigGroup::General:
            return "General";
        case ConfigGroup::UI:
            return "UI";
        case ConfigGroup::Document:
            return "Document";
        case ConfigGroup::View:
            return "View";
        case ConfigGroup::Navigation:
            return "Navigation";
        case ConfigGroup::Performance:
            return "Performance";
        case ConfigGroup::Network:
            return "Network";
        case ConfigGroup::Advanced:
            return "Advanced";
        default:
            return "General";
    }
}

void ConfigurationManager::initializeDefaults() {
    // Set default values
    m_defaults["General/language"] = "en";
    m_defaults["General/theme"] = "light";
    m_defaults["UI/font_size"] = 12;
    m_defaults["UI/window_width"] = 1024;
    m_defaults["UI/window_height"] = 768;

    // System tray settings
    m_defaults["UI/system_tray_enabled"] = true;
    m_defaults["UI/minimize_to_tray"] = true;
    m_defaults["UI/show_tray_notifications"] = true;
    m_defaults["UI/first_time_tray_notification_shown"] = false;

    // Enhanced system tray features
    m_defaults["UI/show_status_indicators"] = true;
    m_defaults["UI/show_recent_files"] = true;
    m_defaults["UI/recent_files_count"] = 5;
    m_defaults["UI/show_quick_actions"] = true;
    m_defaults["UI/enhanced_notifications"] = true;
    m_defaults["UI/notification_types"] = "document,status,error";
    m_defaults["UI/dynamic_tooltip"] = true;

    m_defaults["Document/auto_save"] = true;
    m_defaults["View/zoom_level"] = 1.0;
    m_defaults["Performance/cache_size"] = 100;

    // Apply defaults if not already set
    for (auto it = m_defaults.begin(); it != m_defaults.end(); ++it) {
        if (!m_settings->contains(it.key())) {
            m_settings->setValue(it.key(), it.value());
        }
    }
}

void ConfigurationManager::notifyChange(const QString& key,
                                        const QVariant& value) {
    emit configurationChanged(key, value);

    if (m_watchedKeys.contains(key)) {
        m_logger.debug(
            QString("Watched key changed: %1 = %2").arg(key, value.toString()));
    }
}

void ConfigurationManager::notifyChange(ConfigGroup group, const QString& key,
                                        const QVariant& value) {
    emit configurationChanged(group, key, value);
}

QStringList ConfigurationManager::allKeys() const {
    return m_settings->allKeys();
}

// ConfigurationValidator implementation
ConfigurationValidator::ConfigurationValidator(ConfigurationManager* manager)
    : m_manager(manager) {}

void ConfigurationValidator::addRule(const ValidationRule& rule) {
    m_rules.append(rule);
}

void ConfigurationValidator::addRangeRule(const QString& key, int min,
                                          int max) {
    ValidationRule rule;
    rule.key = key;
    rule.validator = [min, max](const QVariant& value) {
        int intValue = value.toInt();
        return intValue >= min && intValue <= max;
    };
    rule.errorMessage = QString("Value for '%1' must be between %2 and %3")
                            .arg(key)
                            .arg(min)
                            .arg(max);
    addRule(rule);
}

void ConfigurationValidator::addRangeRule(const QString& key, double min,
                                          double max) {
    ValidationRule rule;
    rule.key = key;
    rule.validator = [min, max](const QVariant& value) {
        double doubleValue = value.toDouble();
        return doubleValue >= min && doubleValue <= max;
    };
    rule.errorMessage = QString("Value for '%1' must be between %2 and %3")
                            .arg(key)
                            .arg(min)
                            .arg(max);
    addRule(rule);
}

void ConfigurationValidator::addRegexRule(const QString& key,
                                          const QString& pattern) {
    ValidationRule rule;
    rule.key = key;
    rule.validator = [pattern](const QVariant& value) {
        QRegularExpression regex(pattern);
        return regex.match(value.toString()).hasMatch();
    };
    rule.errorMessage =
        QString("Value for '%1' does not match required pattern").arg(key);
    addRule(rule);
}

void ConfigurationValidator::addEnumRule(const QString& key,
                                         const QStringList& validValues) {
    ValidationRule rule;
    rule.key = key;
    rule.validator = [validValues](const QVariant& value) {
        return validValues.contains(value.toString());
    };
    rule.errorMessage = QString("Value for '%1' must be one of: %2")
                            .arg(key, validValues.join(", "));
    addRule(rule);
}

bool ConfigurationValidator::validate() {
    m_errors.clear();

    for (const ValidationRule& rule : m_rules) {
        QVariant value = m_manager->getValue(rule.key);
        if (!rule.validator(value)) {
            m_errors.append(rule.errorMessage);
        }
    }

    return m_errors.isEmpty();
}

// ConfigurationProfile implementation
ConfigurationProfile::ConfigurationProfile(const QString& name)
    : m_name(name) {}

void ConfigurationProfile::setValue(const QString& key, const QVariant& value) {
    m_values[key] = value;
}

QVariant ConfigurationProfile::getValue(const QString& key,
                                        const QVariant& defaultValue) const {
    return m_values.value(key, defaultValue);
}

void ConfigurationProfile::applyTo(ConfigurationManager* manager) {
    for (auto it = m_values.begin(); it != m_values.end(); ++it) {
        manager->setValue(it.key(), it.value());
    }
}

void ConfigurationProfile::loadFrom(ConfigurationManager* manager) {
    m_values.clear();
    // Load all configuration keys from the manager
    QStringList keys = manager->allKeys();
    for (const QString& key : keys) {
        m_values[key] = manager->getValue(key);
    }
}

QByteArray ConfigurationProfile::serialize() const {
    QJsonObject obj;
    obj["name"] = m_name;

    QJsonObject valuesObj;
    for (auto it = m_values.begin(); it != m_values.end(); ++it) {
        valuesObj[it.key()] = QJsonValue::fromVariant(it.value());
    }
    obj["values"] = valuesObj;

    return QJsonDocument(obj).toJson();
}

bool ConfigurationProfile::deserialize(const QByteArray& data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return false;
    }

    QJsonObject obj = doc.object();
    m_name = obj["name"].toString();

    QJsonObject valuesObj = obj["values"].toObject();
    m_values.clear();
    for (auto it = valuesObj.begin(); it != valuesObj.end(); ++it) {
        m_values[it.key()] = it.value().toVariant();
    }

    return true;
}

// ConfigurationProfileManager implementation
ConfigurationProfileManager::ConfigurationProfileManager(
    ConfigurationManager* manager, QObject* parent)
    : QObject(parent), m_manager(manager) {}

ConfigurationProfileManager::~ConfigurationProfileManager() {
    // Clean up all profiles
    qDeleteAll(m_profiles);
    m_profiles.clear();
}

void ConfigurationProfileManager::addProfile(
    std::unique_ptr<ConfigurationProfile> profile) {
    QString name = profile->name();
    m_profiles[name] = profile.release();  // Transfer ownership to raw pointer
    emit profileAdded(name);
}

void ConfigurationProfileManager::removeProfile(const QString& name) {
    auto it = m_profiles.find(name);
    if (it != m_profiles.end()) {
        delete it.value();  // Clean up the profile
        m_profiles.erase(it);
        emit profileRemoved(name);
    }
}

ConfigurationProfile* ConfigurationProfileManager::getProfile(
    const QString& name) {
    auto it = m_profiles.find(name);
    return (it != m_profiles.end()) ? it.value() : nullptr;
}

QStringList ConfigurationProfileManager::profileNames() const {
    return m_profiles.keys();
}

void ConfigurationProfileManager::setActiveProfile(const QString& name) {
    if (m_profiles.contains(name)) {
        m_activeProfileName = name;
        emit activeProfileChanged(name);
    }
}

void ConfigurationProfileManager::applyActiveProfile() {
    if (!m_activeProfileName.isEmpty() &&
        m_profiles.contains(m_activeProfileName)) {
        m_profiles[m_activeProfileName]->applyTo(m_manager);
    }
}

void ConfigurationProfileManager::saveProfiles() {
    try {
        // Get profiles directory path
        QString appDataPath =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QString profilesDir = appDataPath + "/profiles";

        // Create profiles directory if it doesn't exist
        QDir dir;
        if (!dir.mkpath(profilesDir)) {
            m_logger.error(QString("Failed to create profiles directory: %1")
                               .arg(profilesDir));
            return;
        }

        // Save each profile to a separate JSON file
        for (auto it = m_profiles.begin(); it != m_profiles.end(); ++it) {
            const QString& profileName = it.key();
            ConfigurationProfile* profile = it.value();

            if (!profile) {
                continue;
            }

            // Create file path for this profile
            QString fileName = profileName;
            // Sanitize filename - remove invalid characters
            fileName.replace(QRegularExpression("[<>:\"/\\\\|?*]"), "_");
            QString filePath = profilesDir + "/" + fileName + ".json";

            // Serialize profile to JSON
            QByteArray jsonData = profile->serialize();
            QFile file(filePath);

            if (!file.open(QIODevice::WriteOnly)) {
                m_logger.warning(QString("Failed to save profile '%1' to: %2")
                                     .arg(profileName)
                                     .arg(filePath));
                continue;
            }

            file.write(jsonData);
            file.close();

            m_logger.debug(QString("Saved profile '%1' to: %2")
                               .arg(profileName)
                               .arg(filePath));
        }

        // Save active profile name to QSettings
        QSettings settings;
        settings.beginGroup("ConfigurationProfiles");
        settings.setValue("activeProfile", m_activeProfileName);
        settings.endGroup();
        settings.sync();

        m_logger.info(
            QString("Saved %1 configuration profiles").arg(m_profiles.size()));

    } catch (const std::exception& e) {
        m_logger.error(
            QString("Exception while saving profiles: %1").arg(e.what()));
    }
}

void ConfigurationProfileManager::loadProfiles() {
    try {
        // Get profiles directory path
        QString appDataPath =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QString profilesDir = appDataPath + "/profiles";

        // Check if profiles directory exists
        QDir dir(profilesDir);
        if (!dir.exists()) {
            m_logger.debug(
                "Profiles directory does not exist, no profiles to load");
            return;
        }

        // Get all JSON files in the profiles directory
        QStringList filters;
        filters << "*.json";
        QFileInfoList fileList =
            dir.entryInfoList(filters, QDir::Files | QDir::Readable);

        int loadedCount = 0;
        for (const QFileInfo& fileInfo : fileList) {
            QString filePath = fileInfo.absoluteFilePath();
            QFile file(filePath);

            if (!file.open(QIODevice::ReadOnly)) {
                m_logger.warning(
                    QString("Failed to open profile file: %1").arg(filePath));
                continue;
            }

            QByteArray jsonData = file.readAll();
            file.close();

            // Deserialize profile from JSON
            auto profile = std::make_unique<ConfigurationProfile>("");
            if (profile->deserialize(jsonData)) {
                QString profileName = profile->name();
                addProfile(std::move(profile));
                loadedCount++;

                m_logger.debug(QString("Loaded profile '%1' from: %2")
                                   .arg(profileName)
                                   .arg(filePath));
            } else {
                m_logger.warning(
                    QString("Failed to deserialize profile from: %1")
                        .arg(filePath));
            }
        }

        // Load active profile name from QSettings
        QSettings settings;
        settings.beginGroup("ConfigurationProfiles");
        QString savedActiveProfile = settings.value("activeProfile").toString();
        settings.endGroup();

        // Set active profile if it exists
        if (!savedActiveProfile.isEmpty() &&
            m_profiles.contains(savedActiveProfile)) {
            setActiveProfile(savedActiveProfile);
        }

        m_logger.info(
            QString("Loaded %1 configuration profiles").arg(loadedCount));

    } catch (const std::exception& e) {
        m_logger.error(
            QString("Exception while loading profiles: %1").arg(e.what()));
    }
}

bool ConfigurationProfileManager::exportProfile(const QString& name,
                                                const QString& filePath) {
    ConfigurationProfile* profile = getProfile(name);
    if (!profile) {
        return false;
    }

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(profile->serialize());
        return true;
    }

    return false;
}

bool ConfigurationProfileManager::importProfile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    auto profile = std::make_unique<ConfigurationProfile>("");
    if (profile->deserialize(data)) {
        addProfile(std::move(profile));
        return true;
    }

    return false;
}
