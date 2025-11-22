#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTextToSpeech>
#include <QTimer>
#include <QVoice>
#include <memory>
#include "../logging/SimpleLogging.h"
#include "../model/AccessibilityModel.h"

// Forward declarations
class EventBus;
class StyleManager;
class I18nManager;

/**
 * @brief Controller for managing accessibility features
 *
 * This controller coordinates all accessibility-related functionality including
 * screen reader mode, text-to-speech, high contrast themes, and keyboard
 * navigation. It follows the project's MVP architecture pattern.
 *
 * The controller manages:
 * - Text-to-speech engine lifecycle and state
 * - Integration with screen reader announcements
 * - High contrast mode coordination with StyleManager
 * - Event bus integration for accessibility events
 * - Keyboard shortcut accessibility support
 */
class AccessibilityController : public QObject {
    Q_OBJECT

public:
    explicit AccessibilityController(AccessibilityModel* model,
                                     QObject* parent = nullptr);
    ~AccessibilityController() override;

    AccessibilityController(const AccessibilityController&) = delete;
    AccessibilityController& operator=(const AccessibilityController&) = delete;
    AccessibilityController(AccessibilityController&&) = delete;
    AccessibilityController& operator=(AccessibilityController&&) = delete;

    // Initialization
    void initialize();
    void shutdown();
    bool isInitialized() const { return m_initialized; }

    // Model access
    AccessibilityModel* model() const { return m_model; }

    // Screen reader operations
    void enableScreenReader(bool enable);
    bool isScreenReaderEnabled() const;
    void announceText(const QString& text);
    void announcePageChange(int pageNumber, int totalPages);
    void announceZoomChange(double zoomLevel);
    void announceSelectionChange(const QString& selectedText);
    void announceError(const QString& error);
    void announceWarning(const QString& warning);
    void announceSuccess(const QString& message);

    // High contrast operations
    void setHighContrastMode(bool enable);
    bool isHighContrastMode() const;
    void applyHighContrastColors();
    void restoreNormalColors();

    // Text-to-speech operations
    void enableTextToSpeech(bool enable);
    bool isTextToSpeechEnabled() const;
    void speak(const QString& text);
    void pause();
    void resume();
    void stop();
    QTextToSpeech::State textToSpeechState() const;
    bool isTextToSpeechAvailable() const;

    // TTS engine management
    QStringList availableEngines() const;
    QString currentEngine() const;
    void setEngine(const QString& engine);
    QList<QLocale> availableLocales() const;
    QLocale currentLocale() const;
    void setLocale(const QLocale& locale);
    QList<QVoice> availableVoices() const;
    QVoice currentVoice() const;
    void setVoice(const QVoice& voice);

    // TTS parameters
    qreal speechRate() const;
    void setSpeechRate(qreal rate);
    qreal speechPitch() const;
    void setSpeechPitch(qreal pitch);
    qreal speechVolume() const;
    void setSpeechVolume(qreal volume);

    // Text rendering
    void setTextEnlargement(bool enable);
    bool isTextEnlargementEnabled() const;
    void setTextScaleFactor(qreal factor);
    qreal textScaleFactor() const;

    // Motion and effects
    void setReduceMotion(bool reduce);
    bool shouldReduceMotion() const;
    void setReduceTransparency(bool reduce);
    bool shouldReduceTransparency() const;

    // Keyboard navigation
    void setEnhancedKeyboardNavigation(bool enable);
    bool isEnhancedKeyboardNavigationEnabled() const;

    // Utility methods
    void readCurrentPage();
    void readSelectedText();
    void readDocumentTitle();
    void readStatusMessage(const QString& message);

    // Testing and diagnostics
    bool testTextToSpeech();
    QString getAccessibilityStatus() const;
    QStringList getEnabledFeatures() const;

signals:
    // Controller state signals
    void initialized();
    void shutdownComplete();

    // Screen reader signals
    void screenReaderStateChanged(bool enabled);
    void textAnnounced(const QString& text);

    // TTS signals
    void textToSpeechStateChanged(QTextToSpeech::State state);
    void textToSpeechError(const QString& error);
    void speechStarted(const QString& text);
    void speechFinished();
    void speechPaused();
    void speechResumed();

    // High contrast signals
    void highContrastStateChanged(bool enabled);

    // Feature signals
    void featureEnabled(const QString& featureName);
    void featureDisabled(const QString& featureName);

    // Error signals
    void errorOccurred(const QString& error);
    void warningOccurred(const QString& warning);

public slots:
    // Event handlers
    void onPageChanged(int pageNumber, int totalPages);
    void onZoomChanged(double zoomLevel);
    void onDocumentOpened(const QString& filePath);
    void onDocumentClosed();
    void onSelectionChanged(const QString& selectedText);
    void onThemeChanged();
    void onLanguageChanged(const QString& languageCode);

private slots:
    // Model signal handlers
    void onModelSettingsChanged(const AccessibilitySettings& settings);
    void onScreenReaderEnabledChanged(bool enabled);
    void onHighContrastModeChanged(bool enabled);
    void onTtsEnabledChanged(bool enabled);
    void onTtsVoiceChanged(const QVoice& voice);
    void onTtsLocaleChanged(const QLocale& locale);
    void onTtsEngineChanged(const QString& engine);

    // TTS engine signal handlers
    void onTtsStateChanged(QTextToSpeech::State state);
    void onTtsErrorOccurred(QTextToSpeech::ErrorReason reason,
                            const QString& errorString);

    // Queue processing
    void processAnnouncementQueue();

private:
    void initializeTextToSpeech();
    void shutdownTextToSpeech();
    void recreateTextToSpeech();
    void applyTtsSettings();
    void connectModelSignals();
    void connectEventBusSignals();
    void publishAccessibilityEvent(const QString& eventType,
                                   const QVariant& data = QVariant());

    QString formatPageAnnouncement(int pageNumber, int totalPages) const;
    QString formatZoomAnnouncement(double zoomLevel) const;
    QString localizeMessage(const QString& key,
                            const QStringList& args = QStringList()) const;

    void queueAnnouncement(const QString& text, int priority = 0);
    void clearAnnouncementQueue();

    // Member variables
    QPointer<AccessibilityModel> m_model;
    std::unique_ptr<QTextToSpeech> m_tts;
    bool m_initialized;

    // Announcement queue for screen reader
    struct Announcement {
        QString text;
        int priority;
        qint64 timestamp;

        bool operator<(const Announcement& other) const {
            if (priority != other.priority) {
                return priority < other.priority;
            }
            return timestamp > other.timestamp;
        }
    };

    QList<Announcement> m_announcementQueue;
    QTimer* m_announcementTimer;
    bool m_isAnnouncing;
    int m_maxQueueSize;

    // Current document context
    QString m_currentDocument;
    int m_currentPage;
    int m_totalPages;
    double m_currentZoom;

    // TTS state
    QTextToSpeech::State m_lastTtsState;
    QString m_currentSpeechText;

    // Logging
    SastLogging::CategoryLogger m_logger{"AccessibilityController"};
};
