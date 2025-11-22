#pragma once

#include <QAbstractItemModel>
#include <QColor>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QLocale>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QTextToSpeech>
#include <QVoice>

/**
 * @brief Accessibility settings and state configuration
 *
 * Represents the complete state of accessibility features including
 * screen reader mode, high contrast settings, and text-to-speech configuration.
 */
struct AccessibilitySettings {
    // Screen reader settings
    bool screenReaderEnabled = false;
    bool announcePageChanges = true;
    bool announceZoomChanges = true;
    bool announceSelectionChanges = true;

    // High contrast settings
    bool highContrastMode = false;
    QColor backgroundColor = Qt::white;
    QColor foregroundColor = Qt::black;
    QColor highlightColor = QColor(255, 255, 0, 128);
    QColor selectionColor = QColor(0, 120, 215);

    // Text-to-speech settings
    bool ttsEnabled = false;
    QString ttsEngine;
    QLocale ttsLocale = QLocale::system();
    QVoice ttsVoice;
    qreal ttsRate = 0.0;    // -1.0 to 1.0
    qreal ttsPitch = 0.0;   // -1.0 to 1.0
    qreal ttsVolume = 1.0;  // 0.0 to 1.0

    // Keyboard navigation settings
    bool keyboardNavigationEnhanced = false;
    bool focusIndicatorVisible = true;
    int focusIndicatorWidth = 2;

    // Text rendering settings
    bool enlargeText = false;
    qreal textScaleFactor = 1.0;  // 0.5 to 3.0
    bool boldText = false;

    // Animation settings
    bool reduceMotion = false;
    bool reduceTransparency = false;

    // Metadata
    QDateTime lastModified = QDateTime::currentDateTime();
    int version = 1;

    // Serialization
    QJsonObject toJson() const;
    static AccessibilitySettings fromJson(const QJsonObject& json);

    // Comparison
    bool operator==(const AccessibilitySettings& other) const {
        return screenReaderEnabled == other.screenReaderEnabled &&
               highContrastMode == other.highContrastMode &&
               ttsEnabled == other.ttsEnabled && ttsRate == other.ttsRate &&
               ttsVolume == other.ttsVolume;
    }
    bool operator!=(const AccessibilitySettings& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Model for managing accessibility features and settings
 *
 * This model follows the project's MVP architecture pattern and manages
 * all accessibility-related state including screen reader mode, high
 * contrast themes, text-to-speech configuration, and enhanced keyboard
 * navigation.
 */
class AccessibilityModel : public QObject {
    Q_OBJECT

public:
    enum AccessibilityFeature {
        ScreenReader = 0x01,
        HighContrast = 0x02,
        TextToSpeech = 0x04,
        EnhancedKeyboard = 0x08,
        TextEnlargement = 0x10,
        ReduceMotion = 0x20
    };
    Q_DECLARE_FLAGS(AccessibilityFeatures, AccessibilityFeature)

    explicit AccessibilityModel(QObject* parent = nullptr);
    ~AccessibilityModel() override;

    // Settings management
    AccessibilitySettings settings() const { return m_settings; }
    void setSettings(const AccessibilitySettings& settings);
    void resetToDefaults();

    // Screen reader
    bool isScreenReaderEnabled() const {
        return m_settings.screenReaderEnabled;
    }
    void setScreenReaderEnabled(bool enabled);
    bool shouldAnnouncePageChanges() const {
        return m_settings.announcePageChanges;
    }
    void setShouldAnnouncePageChanges(bool announce);
    bool shouldAnnounceZoomChanges() const {
        return m_settings.announceZoomChanges;
    }
    void setShouldAnnounceZoomChanges(bool announce);

    // High contrast
    bool isHighContrastMode() const { return m_settings.highContrastMode; }
    void setHighContrastMode(bool enabled);
    QColor backgroundColor() const { return m_settings.backgroundColor; }
    void setBackgroundColor(const QColor& color);
    QColor foregroundColor() const { return m_settings.foregroundColor; }
    void setForegroundColor(const QColor& color);
    QColor highlightColor() const { return m_settings.highlightColor; }
    void setHighlightColor(const QColor& color);
    QColor selectionColor() const { return m_settings.selectionColor; }
    void setSelectionColor(const QColor& color);

    // Text-to-speech
    bool isTtsEnabled() const { return m_settings.ttsEnabled; }
    void setTtsEnabled(bool enabled);
    QString ttsEngine() const { return m_settings.ttsEngine; }
    void setTtsEngine(const QString& engine);
    QLocale ttsLocale() const { return m_settings.ttsLocale; }
    void setTtsLocale(const QLocale& locale);
    QVoice ttsVoice() const { return m_settings.ttsVoice; }
    void setTtsVoice(const QVoice& voice);
    qreal ttsRate() const { return m_settings.ttsRate; }
    void setTtsRate(qreal rate);
    qreal ttsPitch() const { return m_settings.ttsPitch; }
    void setTtsPitch(qreal pitch);
    qreal ttsVolume() const { return m_settings.ttsVolume; }
    void setTtsVolume(qreal volume);

    // Text rendering
    bool isTextEnlargementEnabled() const { return m_settings.enlargeText; }
    void setTextEnlargementEnabled(bool enabled);
    qreal textScaleFactor() const { return m_settings.textScaleFactor; }
    void setTextScaleFactor(qreal factor);
    bool isBoldTextEnabled() const { return m_settings.boldText; }
    void setBoldTextEnabled(bool enabled);

    // Motion and effects
    bool shouldReduceMotion() const { return m_settings.reduceMotion; }
    void setReduceMotion(bool reduce);
    bool shouldReduceTransparency() const {
        return m_settings.reduceTransparency;
    }
    void setReduceTransparency(bool reduce);

    // Keyboard navigation
    bool isEnhancedKeyboardNavigationEnabled() const {
        return m_settings.keyboardNavigationEnhanced;
    }
    void setEnhancedKeyboardNavigationEnabled(bool enabled);
    bool isFocusIndicatorVisible() const {
        return m_settings.focusIndicatorVisible;
    }
    void setFocusIndicatorVisible(bool visible);
    int focusIndicatorWidth() const { return m_settings.focusIndicatorWidth; }
    void setFocusIndicatorWidth(int width);

    // Feature checking
    bool isFeatureEnabled(AccessibilityFeature feature) const;
    AccessibilityFeatures enabledFeatures() const;

    // Persistence
    bool saveSettings();
    bool loadSettings();
    void setAutoSave(bool enabled) { m_autoSave = enabled; }
    bool isAutoSaveEnabled() const { return m_autoSave; }

    // Export/Import
    bool exportSettings(const QString& filePath);
    bool importSettings(const QString& filePath);

    // Statistics
    QDateTime lastModified() const { return m_settings.lastModified; }
    int settingsVersion() const { return m_settings.version; }

signals:
    // Settings signals
    void settingsChanged(const AccessibilitySettings& settings);
    void settingsReset();
    void settingsSaved();
    void settingsLoaded();
    void settingsImported(const QString& filePath);
    void settingsExported(const QString& filePath);

    // Feature-specific signals
    void screenReaderEnabledChanged(bool enabled);
    void highContrastModeChanged(bool enabled);
    void ttsEnabledChanged(bool enabled);
    void ttsRateChanged(qreal rate);
    void ttsPitchChanged(qreal pitch);
    void ttsVolumeChanged(qreal volume);
    void ttsVoiceChanged(const QVoice& voice);
    void ttsLocaleChanged(const QLocale& locale);
    void ttsEngineChanged(const QString& engine);
    void textScaleFactorChanged(qreal factor);
    void reduceMotionChanged(bool reduce);

    // Color signals
    void colorsChanged();
    void backgroundColorChanged(const QColor& color);
    void foregroundColorChanged(const QColor& color);
    void highlightColorChanged(const QColor& color);

    // Error signals
    void errorOccurred(const QString& error);

private slots:
    void onSettingsModified();

private:
    QString getSettingsFilePath() const;
    void validateSettings();
    void notifyChanges(const AccessibilitySettings& oldSettings);

    AccessibilitySettings m_settings;
    bool m_autoSave;
    QSettings* m_qsettings;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AccessibilityModel::AccessibilityFeatures)
