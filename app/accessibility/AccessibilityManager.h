#pragma once

#include <QObject>
#include <QString>

/**
 * @brief Manages accessibility features for the PDF reader
 * Features 10, 11, 12: Screen reader, High contrast, Text-to-speech
 */
class AccessibilityManager : public QObject {
    Q_OBJECT

public:
    explicit AccessibilityManager(QObject* parent = nullptr);
    ~AccessibilityManager() = default;

    // Screen reader support (Feature 10)
    void enableScreenReaderMode(bool enable);
    bool isScreenReaderEnabled() const { return m_screenReaderEnabled; }
    void announceText(const QString& text);
    void announcePageChange(int pageNumber, int totalPages);

    // High contrast mode (Feature 11)
    void setHighContrastMode(bool enable);
    bool isHighContrastMode() const { return m_highContrastMode; }
    QColor getBackgroundColor() const;
    QColor getForegroundColor() const;
    QColor getHighlightColor() const;

    // Text-to-speech (Feature 12)
    void startTextToSpeech(const QString& text);
    void stopTextToSpeech();
    void pauseTextToSpeech() const;
    void resumeTextToSpeech() const;
    bool isTextToSpeechActive() const { return m_ttsActive; }
    void setTextToSpeechRate(qreal rate);
    void setTextToSpeechVolume(qreal volume);

signals:
    void screenReaderModeChanged(bool enabled);
    void highContrastModeChanged(bool enabled);
    void textToSpeechStateChanged(bool active);
    void textToSpeechFinished();

private:
    bool m_screenReaderEnabled;
    bool m_highContrastMode;
    bool m_ttsActive;
    qreal m_ttsRate;
    qreal m_ttsVolume;
};
