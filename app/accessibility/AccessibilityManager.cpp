#include "AccessibilityManager.h"
#include "logging/SimpleLogging.h"

AccessibilityManager::AccessibilityManager(QObject* parent)
    : QObject(parent),
      m_screenReaderEnabled(false),
      m_highContrastMode(false),
      m_ttsActive(false),
      m_ttsRate(0.0),
      m_ttsVolume(1.0) {}

void AccessibilityManager::enableScreenReaderMode(bool enable) {
    if (m_screenReaderEnabled != enable) {
        m_screenReaderEnabled = enable;
        SLOG_DEBUG_F("Screen reader mode: {}", enable ? "enabled" : "disabled");
        emit screenReaderModeChanged(enable);
    }
}

void AccessibilityManager::announceText(const QString& text) {
    if (m_screenReaderEnabled) {
        SLOG_DEBUG_F("Announcing: {}", text);
        // Integration with platform screen reader APIs
        startTextToSpeech(text);
    }
}

void AccessibilityManager::announcePageChange(int pageNumber, int totalPages) {
    if (m_screenReaderEnabled) {
        QString announcement =
            QString("Page %1 of %2").arg(pageNumber).arg(totalPages);
        announceText(announcement);
    }
}

void AccessibilityManager::setHighContrastMode(bool enable) {
    if (m_highContrastMode != enable) {
        m_highContrastMode = enable;
        emit highContrastModeChanged(enable);
    }
}

QColor AccessibilityManager::getBackgroundColor() const {
    return m_highContrastMode ? QColor(Qt::black) : QColor(Qt::white);
}

QColor AccessibilityManager::getForegroundColor() const {
    return m_highContrastMode ? QColor(Qt::white) : QColor(Qt::black);
}

QColor AccessibilityManager::getHighlightColor() const {
    return m_highContrastMode ? QColor(Qt::yellow) : QColor(255, 255, 0, 128);
}

void AccessibilityManager::startTextToSpeech(const QString& text) {
    // Qt TextToSpeech integration
    SLOG_DEBUG_F("TTS starting: {}", text.left(50));
    m_ttsActive = true;
    emit textToSpeechStateChanged(true);
    // Actual QTextToSpeech implementation would go here
}

void AccessibilityManager::stopTextToSpeech() {
    m_ttsActive = false;
    emit textToSpeechStateChanged(false);
}

void AccessibilityManager::pauseTextToSpeech() const {
    if (m_ttsActive) {
        SLOG_DEBUG("TTS paused");
    }
}

void AccessibilityManager::resumeTextToSpeech() const {
    if (m_ttsActive) {
        SLOG_DEBUG("TTS resumed");
    }
}

void AccessibilityManager::setTextToSpeechRate(qreal rate) {
    m_ttsRate = qBound(-1.0, rate, 1.0);
}

void AccessibilityManager::setTextToSpeechVolume(qreal volume) {
    m_ttsVolume = qBound(0.0, volume, 1.0);
}
