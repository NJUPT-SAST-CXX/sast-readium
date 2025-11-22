#include "AccessibilityModel.h"
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include "../logging/SimpleLogging.h"

// AccessibilitySettings serialization
QJsonObject AccessibilitySettings::toJson() const {
    QJsonObject json;

    // Screen reader
    json["screenReaderEnabled"] = screenReaderEnabled;
    json["announcePageChanges"] = announcePageChanges;
    json["announceZoomChanges"] = announceZoomChanges;
    json["announceSelectionChanges"] = announceSelectionChanges;

    // High contrast
    json["highContrastMode"] = highContrastMode;
    json["backgroundColor"] = backgroundColor.name(QColor::HexArgb);
    json["foregroundColor"] = foregroundColor.name(QColor::HexArgb);
    json["highlightColor"] = highlightColor.name(QColor::HexArgb);
    json["selectionColor"] = selectionColor.name(QColor::HexArgb);

    // Text-to-speech
    json["ttsEnabled"] = ttsEnabled;
    json["ttsEngine"] = ttsEngine;
    json["ttsLocale"] = ttsLocale.name();
    json["ttsVoiceName"] = ttsVoice.name();
    json["ttsRate"] = ttsRate;
    json["ttsPitch"] = ttsPitch;
    json["ttsVolume"] = ttsVolume;

    // Keyboard navigation
    json["keyboardNavigationEnhanced"] = keyboardNavigationEnhanced;
    json["focusIndicatorVisible"] = focusIndicatorVisible;
    json["focusIndicatorWidth"] = focusIndicatorWidth;

    // Text rendering
    json["enlargeText"] = enlargeText;
    json["textScaleFactor"] = textScaleFactor;
    json["boldText"] = boldText;

    // Animation
    json["reduceMotion"] = reduceMotion;
    json["reduceTransparency"] = reduceTransparency;

    // Metadata
    json["lastModified"] = lastModified.toString(Qt::ISODate);
    json["version"] = version;

    return json;
}

AccessibilitySettings AccessibilitySettings::fromJson(const QJsonObject& json) {
    AccessibilitySettings settings;

    // Screen reader
    settings.screenReaderEnabled = json["screenReaderEnabled"].toBool(false);
    settings.announcePageChanges = json["announcePageChanges"].toBool(true);
    settings.announceZoomChanges = json["announceZoomChanges"].toBool(true);
    settings.announceSelectionChanges =
        json["announceSelectionChanges"].toBool(true);

    // High contrast
    settings.highContrastMode = json["highContrastMode"].toBool(false);
    settings.backgroundColor =
        QColor(json["backgroundColor"].toString("#FFFFFF"));
    settings.foregroundColor =
        QColor(json["foregroundColor"].toString("#000000"));
    settings.highlightColor =
        QColor(json["highlightColor"].toString("#80FFFF00"));
    settings.selectionColor =
        QColor(json["selectionColor"].toString("#0078D7"));

    // Text-to-speech
    settings.ttsEnabled = json["ttsEnabled"].toBool(false);
    settings.ttsEngine = json["ttsEngine"].toString();
    settings.ttsLocale =
        QLocale(json["ttsLocale"].toString(QLocale::system().name()));
    settings.ttsRate = json["ttsRate"].toDouble(0.0);
    settings.ttsPitch = json["ttsPitch"].toDouble(0.0);
    settings.ttsVolume = json["ttsVolume"].toDouble(1.0);

    // Keyboard navigation
    settings.keyboardNavigationEnhanced =
        json["keyboardNavigationEnhanced"].toBool(false);
    settings.focusIndicatorVisible = json["focusIndicatorVisible"].toBool(true);
    settings.focusIndicatorWidth = json["focusIndicatorWidth"].toInt(2);

    // Text rendering
    settings.enlargeText = json["enlargeText"].toBool(false);
    settings.textScaleFactor = json["textScaleFactor"].toDouble(1.0);
    settings.boldText = json["boldText"].toBool(false);

    // Animation
    settings.reduceMotion = json["reduceMotion"].toBool(false);
    settings.reduceTransparency = json["reduceTransparency"].toBool(false);

    // Metadata
    if (json.contains("lastModified")) {
        settings.lastModified =
            QDateTime::fromString(json["lastModified"].toString(), Qt::ISODate);
    }
    settings.version = json["version"].toInt(1);

    return settings;
}

// AccessibilityModel implementation
AccessibilityModel::AccessibilityModel(QObject* parent)
    : QObject(parent), m_autoSave(true) {
    m_qsettings = new QSettings(QSettings::IniFormat, QSettings::UserScope,
                                "SAST", "Readium-Accessibility", this);

    loadSettings();
}

AccessibilityModel::~AccessibilityModel() {
    if (m_autoSave) {
        saveSettings();
    }
}

void AccessibilityModel::setSettings(const AccessibilitySettings& settings) {
    AccessibilitySettings oldSettings = m_settings;
    m_settings = settings;
    m_settings.lastModified = QDateTime::currentDateTime();

    validateSettings();
    notifyChanges(oldSettings);

    emit settingsChanged(m_settings);

    if (m_autoSave) {
        saveSettings();
    }
}

void AccessibilityModel::resetToDefaults() {
    AccessibilitySettings defaultSettings;
    setSettings(defaultSettings);
    emit settingsReset();
    SLOG_INFO("Accessibility settings reset to defaults");
}

void AccessibilityModel::setScreenReaderEnabled(bool enabled) {
    if (m_settings.screenReaderEnabled != enabled) {
        m_settings.screenReaderEnabled = enabled;
        onSettingsModified();
        emit screenReaderEnabledChanged(enabled);
        SLOG_INFO_F("Screen reader mode: {}", enabled ? "enabled" : "disabled");
    }
}

void AccessibilityModel::setShouldAnnouncePageChanges(bool announce) {
    if (m_settings.announcePageChanges != announce) {
        m_settings.announcePageChanges = announce;
        onSettingsModified();
    }
}

void AccessibilityModel::setShouldAnnounceZoomChanges(bool announce) {
    if (m_settings.announceZoomChanges != announce) {
        m_settings.announceZoomChanges = announce;
        onSettingsModified();
    }
}

void AccessibilityModel::setHighContrastMode(bool enabled) {
    if (m_settings.highContrastMode != enabled) {
        m_settings.highContrastMode = enabled;
        onSettingsModified();
        emit highContrastModeChanged(enabled);
        emit colorsChanged();
        SLOG_INFO_F("High contrast mode: {}", enabled ? "enabled" : "disabled");
    }
}

void AccessibilityModel::setBackgroundColor(const QColor& color) {
    if (m_settings.backgroundColor != color) {
        m_settings.backgroundColor = color;
        onSettingsModified();
        emit backgroundColorChanged(color);
        emit colorsChanged();
    }
}

void AccessibilityModel::setForegroundColor(const QColor& color) {
    if (m_settings.foregroundColor != color) {
        m_settings.foregroundColor = color;
        onSettingsModified();
        emit foregroundColorChanged(color);
        emit colorsChanged();
    }
}

void AccessibilityModel::setHighlightColor(const QColor& color) {
    if (m_settings.highlightColor != color) {
        m_settings.highlightColor = color;
        onSettingsModified();
        emit highlightColorChanged(color);
        emit colorsChanged();
    }
}

void AccessibilityModel::setSelectionColor(const QColor& color) {
    if (m_settings.selectionColor != color) {
        m_settings.selectionColor = color;
        onSettingsModified();
        emit colorsChanged();
    }
}

void AccessibilityModel::setTtsEnabled(bool enabled) {
    if (m_settings.ttsEnabled != enabled) {
        m_settings.ttsEnabled = enabled;
        onSettingsModified();
        emit ttsEnabledChanged(enabled);
        SLOG_INFO_F("Text-to-speech: {}", enabled ? "enabled" : "disabled");
    }
}

void AccessibilityModel::setTtsEngine(const QString& engine) {
    if (m_settings.ttsEngine != engine) {
        m_settings.ttsEngine = engine;
        onSettingsModified();
        emit ttsEngineChanged(engine);
    }
}

void AccessibilityModel::setTtsLocale(const QLocale& locale) {
    if (m_settings.ttsLocale != locale) {
        m_settings.ttsLocale = locale;
        onSettingsModified();
        emit ttsLocaleChanged(locale);
    }
}

void AccessibilityModel::setTtsVoice(const QVoice& voice) {
    if (m_settings.ttsVoice != voice) {
        m_settings.ttsVoice = voice;
        onSettingsModified();
        emit ttsVoiceChanged(voice);
    }
}

void AccessibilityModel::setTtsRate(qreal rate) {
    qreal clampedRate = qBound(-1.0, rate, 1.0);
    if (m_settings.ttsRate != clampedRate) {
        m_settings.ttsRate = clampedRate;
        onSettingsModified();
        emit ttsRateChanged(clampedRate);
    }
}

void AccessibilityModel::setTtsPitch(qreal pitch) {
    qreal clampedPitch = qBound(-1.0, pitch, 1.0);
    if (m_settings.ttsPitch != clampedPitch) {
        m_settings.ttsPitch = clampedPitch;
        onSettingsModified();
        emit ttsPitchChanged(clampedPitch);
    }
}

void AccessibilityModel::setTtsVolume(qreal volume) {
    qreal clampedVolume = qBound(0.0, volume, 1.0);
    if (m_settings.ttsVolume != clampedVolume) {
        m_settings.ttsVolume = clampedVolume;
        onSettingsModified();
        emit ttsVolumeChanged(clampedVolume);
    }
}

void AccessibilityModel::setTextEnlargementEnabled(bool enabled) {
    if (m_settings.enlargeText != enabled) {
        m_settings.enlargeText = enabled;
        onSettingsModified();
    }
}

void AccessibilityModel::setTextScaleFactor(qreal factor) {
    qreal clampedFactor = qBound(0.5, factor, 3.0);
    if (m_settings.textScaleFactor != clampedFactor) {
        m_settings.textScaleFactor = clampedFactor;
        onSettingsModified();
        emit textScaleFactorChanged(clampedFactor);
    }
}

void AccessibilityModel::setBoldTextEnabled(bool enabled) {
    if (m_settings.boldText != enabled) {
        m_settings.boldText = enabled;
        onSettingsModified();
    }
}

void AccessibilityModel::setReduceMotion(bool reduce) {
    if (m_settings.reduceMotion != reduce) {
        m_settings.reduceMotion = reduce;
        onSettingsModified();
        emit reduceMotionChanged(reduce);
    }
}

void AccessibilityModel::setReduceTransparency(bool reduce) {
    if (m_settings.reduceTransparency != reduce) {
        m_settings.reduceTransparency = reduce;
        onSettingsModified();
    }
}

void AccessibilityModel::setEnhancedKeyboardNavigationEnabled(bool enabled) {
    if (m_settings.keyboardNavigationEnhanced != enabled) {
        m_settings.keyboardNavigationEnhanced = enabled;
        onSettingsModified();
    }
}

void AccessibilityModel::setFocusIndicatorVisible(bool visible) {
    if (m_settings.focusIndicatorVisible != visible) {
        m_settings.focusIndicatorVisible = visible;
        onSettingsModified();
    }
}

void AccessibilityModel::setFocusIndicatorWidth(int width) {
    int clampedWidth = qBound(1, width, 10);
    if (m_settings.focusIndicatorWidth != clampedWidth) {
        m_settings.focusIndicatorWidth = clampedWidth;
        onSettingsModified();
    }
}

bool AccessibilityModel::isFeatureEnabled(AccessibilityFeature feature) const {
    switch (feature) {
        case ScreenReader:
            return m_settings.screenReaderEnabled;
        case HighContrast:
            return m_settings.highContrastMode;
        case TextToSpeech:
            return m_settings.ttsEnabled;
        case EnhancedKeyboard:
            return m_settings.keyboardNavigationEnhanced;
        case TextEnlargement:
            return m_settings.enlargeText;
        case ReduceMotion:
            return m_settings.reduceMotion;
        default:
            return false;
    }
}

AccessibilityModel::AccessibilityFeatures AccessibilityModel::enabledFeatures()
    const {
    AccessibilityFeatures features;

    if (m_settings.screenReaderEnabled)
        features |= ScreenReader;
    if (m_settings.highContrastMode)
        features |= HighContrast;
    if (m_settings.ttsEnabled)
        features |= TextToSpeech;
    if (m_settings.keyboardNavigationEnhanced)
        features |= EnhancedKeyboard;
    if (m_settings.enlargeText)
        features |= TextEnlargement;
    if (m_settings.reduceMotion)
        features |= ReduceMotion;

    return features;
}

bool AccessibilityModel::saveSettings() {
    try {
        // Save to QSettings (INI format)
        QJsonObject json = m_settings.toJson();

        m_qsettings->beginGroup("Accessibility");
        for (auto it = json.constBegin(); it != json.constEnd(); ++it) {
            m_qsettings->setValue(it.key(), it.value().toVariant());
        }
        m_qsettings->endGroup();
        m_qsettings->sync();

        // Also save as JSON file for portability
        QString jsonFilePath = getSettingsFilePath();
        QFile file(jsonFilePath);
        if (!file.open(QIODevice::WriteOnly)) {
            SLOG_ERROR_F("Failed to open settings file for writing: {}",
                         jsonFilePath);
            return false;
        }

        QJsonDocument doc(json);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();

        emit settingsSaved();
        SLOG_DEBUG_F("Accessibility settings saved to {}", jsonFilePath);
        return true;

    } catch (const std::exception& e) {
        QString error =
            QString("Failed to save accessibility settings: %1").arg(e.what());
        SLOG_ERROR(error);
        emit errorOccurred(error);
        return false;
    }
}

bool AccessibilityModel::loadSettings() {
    try {
        // Try to load from JSON file first
        QString jsonFilePath = getSettingsFilePath();
        QFile file(jsonFilePath);

        if (file.exists() && file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();

            if (!doc.isNull() && doc.isObject()) {
                m_settings = AccessibilitySettings::fromJson(doc.object());
                validateSettings();
                emit settingsLoaded();
                SLOG_DEBUG_F("Accessibility settings loaded from {}",
                             jsonFilePath);
                return true;
            }
        }

        // Fallback to QSettings
        m_qsettings->beginGroup("Accessibility");
        QStringList keys = m_qsettings->childKeys();

        if (!keys.isEmpty()) {
            QJsonObject json;
            for (const QString& key : keys) {
                QVariant value = m_qsettings->value(key);
                json[key] = QJsonValue::fromVariant(value);
            }
            m_qsettings->endGroup();

            m_settings = AccessibilitySettings::fromJson(json);
            validateSettings();
            emit settingsLoaded();
            SLOG_DEBUG("Accessibility settings loaded from QSettings");
            return true;
        }

        m_qsettings->endGroup();
        SLOG_DEBUG("No saved accessibility settings found, using defaults");
        return false;

    } catch (const std::exception& e) {
        QString error =
            QString("Failed to load accessibility settings: %1").arg(e.what());
        SLOG_ERROR(error);
        emit errorOccurred(error);
        return false;
    }
}

bool AccessibilityModel::exportSettings(const QString& filePath) {
    try {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            QString error =
                QString("Failed to open file for export: %1").arg(filePath);
            emit errorOccurred(error);
            return false;
        }

        QJsonDocument doc(m_settings.toJson());
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();

        emit settingsExported(filePath);
        SLOG_INFO_F("Accessibility settings exported to {}", filePath);
        return true;

    } catch (const std::exception& e) {
        QString error = QString("Failed to export settings: %1").arg(e.what());
        emit errorOccurred(error);
        return false;
    }
}

bool AccessibilityModel::importSettings(const QString& filePath) {
    try {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            QString error =
                QString("Failed to open file for import: %1").arg(filePath);
            emit errorOccurred(error);
            return false;
        }

        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        if (doc.isNull() || !doc.isObject()) {
            QString error = "Invalid JSON format in settings file";
            emit errorOccurred(error);
            return false;
        }

        AccessibilitySettings oldSettings = m_settings;
        m_settings = AccessibilitySettings::fromJson(doc.object());
        m_settings.lastModified = QDateTime::currentDateTime();

        validateSettings();
        notifyChanges(oldSettings);

        emit settingsChanged(m_settings);
        emit settingsImported(filePath);

        if (m_autoSave) {
            saveSettings();
        }

        SLOG_INFO_F("Accessibility settings imported from {}", filePath);
        return true;

    } catch (const std::exception& e) {
        QString error = QString("Failed to import settings: %1").arg(e.what());
        emit errorOccurred(error);
        return false;
    }
}

void AccessibilityModel::onSettingsModified() {
    m_settings.lastModified = QDateTime::currentDateTime();
    emit settingsChanged(m_settings);

    if (m_autoSave) {
        saveSettings();
    }
}

QString AccessibilityModel::getSettingsFilePath() const {
    QString dataPath =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);
    return dataPath + "/accessibility_settings.json";
}

void AccessibilityModel::validateSettings() {
    // Clamp values to valid ranges
    m_settings.ttsRate = qBound(-1.0, m_settings.ttsRate, 1.0);
    m_settings.ttsPitch = qBound(-1.0, m_settings.ttsPitch, 1.0);
    m_settings.ttsVolume = qBound(0.0, m_settings.ttsVolume, 1.0);
    m_settings.textScaleFactor = qBound(0.5, m_settings.textScaleFactor, 3.0);
    m_settings.focusIndicatorWidth =
        qBound(1, m_settings.focusIndicatorWidth, 10);

    // Ensure colors are valid
    if (!m_settings.backgroundColor.isValid()) {
        m_settings.backgroundColor = Qt::white;
    }
    if (!m_settings.foregroundColor.isValid()) {
        m_settings.foregroundColor = Qt::black;
    }
    if (!m_settings.highlightColor.isValid()) {
        m_settings.highlightColor = QColor(255, 255, 0, 128);
    }
    if (!m_settings.selectionColor.isValid()) {
        m_settings.selectionColor = QColor(0, 120, 215);
    }
}

void AccessibilityModel::notifyChanges(
    const AccessibilitySettings& oldSettings) {
    if (oldSettings.screenReaderEnabled != m_settings.screenReaderEnabled) {
        emit screenReaderEnabledChanged(m_settings.screenReaderEnabled);
    }

    if (oldSettings.highContrastMode != m_settings.highContrastMode) {
        emit highContrastModeChanged(m_settings.highContrastMode);
    }

    if (oldSettings.ttsEnabled != m_settings.ttsEnabled) {
        emit ttsEnabledChanged(m_settings.ttsEnabled);
    }

    if (oldSettings.ttsRate != m_settings.ttsRate) {
        emit ttsRateChanged(m_settings.ttsRate);
    }

    if (oldSettings.ttsPitch != m_settings.ttsPitch) {
        emit ttsPitchChanged(m_settings.ttsPitch);
    }

    if (oldSettings.ttsVolume != m_settings.ttsVolume) {
        emit ttsVolumeChanged(m_settings.ttsVolume);
    }

    if (oldSettings.ttsVoice != m_settings.ttsVoice) {
        emit ttsVoiceChanged(m_settings.ttsVoice);
    }

    if (oldSettings.ttsLocale != m_settings.ttsLocale) {
        emit ttsLocaleChanged(m_settings.ttsLocale);
    }

    if (oldSettings.ttsEngine != m_settings.ttsEngine) {
        emit ttsEngineChanged(m_settings.ttsEngine);
    }

    if (oldSettings.textScaleFactor != m_settings.textScaleFactor) {
        emit textScaleFactorChanged(m_settings.textScaleFactor);
    }

    if (oldSettings.reduceMotion != m_settings.reduceMotion) {
        emit reduceMotionChanged(m_settings.reduceMotion);
    }

    bool colorsChanged =
        (oldSettings.backgroundColor != m_settings.backgroundColor ||
         oldSettings.foregroundColor != m_settings.foregroundColor ||
         oldSettings.highlightColor != m_settings.highlightColor ||
         oldSettings.selectionColor != m_settings.selectionColor);

    if (colorsChanged) {
        emit this->colorsChanged();

        if (oldSettings.backgroundColor != m_settings.backgroundColor) {
            emit backgroundColorChanged(m_settings.backgroundColor);
        }
        if (oldSettings.foregroundColor != m_settings.foregroundColor) {
            emit foregroundColorChanged(m_settings.foregroundColor);
        }
        if (oldSettings.highlightColor != m_settings.highlightColor) {
            emit highlightColorChanged(m_settings.highlightColor);
        }
    }
}
