#pragma once

#include <QColor>
#include <QObject>
#include <QPointer>
#include <QString>
#include <memory>

#include "../controller/AccessibilityController.h"
#include "../model/AccessibilityModel.h"

/**
 * @brief Legacy compatibility wrapper for accessibility features
 *
 * This class provides backward compatibility with the original
 * AccessibilityManager interface while delegating to the new
 * MVP architecture (AccessibilityModel + AccessibilityController).
 *
 * Features 10, 11, 12: Screen reader, High contrast, Text-to-speech
 *
 * For new code, prefer using AccessibilityController directly via
 * ServiceLocator::instance().getService<AccessibilityController>()
 */
class AccessibilityManager : public QObject {
    Q_OBJECT

public:
    explicit AccessibilityManager(QObject* parent = nullptr);
    ~AccessibilityManager() override;

    AccessibilityManager(const AccessibilityManager&) = delete;
    AccessibilityManager& operator=(const AccessibilityManager&) = delete;
    AccessibilityManager(AccessibilityManager&&) = delete;
    AccessibilityManager& operator=(AccessibilityManager&&) = delete;

    // Initialization
    void initialize();
    bool isInitialized() const;

    // Screen reader support (Feature 10)
    void enableScreenReaderMode(bool enable);
    bool isScreenReaderEnabled() const;
    void announceText(const QString& text);
    void announcePageChange(int pageNumber, int totalPages);

    // High contrast mode (Feature 11)
    void setHighContrastMode(bool enable);
    bool isHighContrastMode() const;
    QColor getBackgroundColor() const;
    QColor getForegroundColor() const;
    QColor getHighlightColor() const;

    // Text-to-speech (Feature 12)
    void startTextToSpeech(const QString& text);
    void stopTextToSpeech();
    void pauseTextToSpeech() const;
    void resumeTextToSpeech() const;
    bool isTextToSpeechActive() const;
    void setTextToSpeechRate(qreal rate);
    void setTextToSpeechVolume(qreal volume);

    // Access to new architecture components
    AccessibilityController* controller() const { return m_controller.data(); }
    AccessibilityModel* model() const { return m_model.data(); }

signals:
    void screenReaderModeChanged(bool enabled);
    void highContrastModeChanged(bool enabled);
    void textToSpeechStateChanged(bool active);
    void textToSpeechFinished();
    void initialized();

private:
    void setupConnections();

    QPointer<AccessibilityModel> m_model;
    QPointer<AccessibilityController> m_controller;
    bool m_initialized;
};
