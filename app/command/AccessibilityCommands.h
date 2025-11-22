#pragma once

#include <QColor>
#include <QLocale>
#include <QObject>
#include <QString>
#include <QUndoCommand>
#include <QVoice>
#include <memory>
#include "../logging/SimpleLogging.h"
#include "../model/AccessibilityModel.h"

// Forward declarations
class AccessibilityController;

/**
 * @brief Base class for accessibility-related commands
 *
 * Provides common functionality for all accessibility commands
 * with undo/redo support.
 */
class AccessibilityCommand : public QUndoCommand {
public:
    explicit AccessibilityCommand(AccessibilityController* controller,
                                  const QString& text,
                                  QUndoCommand* parent = nullptr);
    ~AccessibilityCommand() override = default;

protected:
    AccessibilityController* m_controller;
    SastLogging::CategoryLogger m_logger;
};

/**
 * @brief Command to toggle screen reader mode
 */
class ToggleScreenReaderCommand : public AccessibilityCommand {
public:
    explicit ToggleScreenReaderCommand(AccessibilityController* controller,
                                       bool enable,
                                       QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1001; }

private:
    bool m_enable;
    bool m_previousState;
};

/**
 * @brief Command to toggle high contrast mode
 */
class ToggleHighContrastCommand : public AccessibilityCommand {
public:
    explicit ToggleHighContrastCommand(AccessibilityController* controller,
                                       bool enable,
                                       QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1002; }

private:
    bool m_enable;
    bool m_previousState;
};

/**
 * @brief Command to toggle text-to-speech
 */
class ToggleTextToSpeechCommand : public AccessibilityCommand {
public:
    explicit ToggleTextToSpeechCommand(AccessibilityController* controller,
                                       bool enable,
                                       QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1003; }

private:
    bool m_enable;
    bool m_previousState;
};

/**
 * @brief Command to change TTS speech rate
 */
class SetSpeechRateCommand : public AccessibilityCommand {
public:
    explicit SetSpeechRateCommand(AccessibilityController* controller,
                                  qreal rate, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1004; }
    bool mergeWith(const QUndoCommand* other) override;

private:
    qreal m_rate;
    qreal m_previousRate;
};

/**
 * @brief Command to change TTS speech pitch
 */
class SetSpeechPitchCommand : public AccessibilityCommand {
public:
    explicit SetSpeechPitchCommand(AccessibilityController* controller,
                                   qreal pitch, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1005; }
    bool mergeWith(const QUndoCommand* other) override;

private:
    qreal m_pitch;
    qreal m_previousPitch;
};

/**
 * @brief Command to change TTS speech volume
 */
class SetSpeechVolumeCommand : public AccessibilityCommand {
public:
    explicit SetSpeechVolumeCommand(AccessibilityController* controller,
                                    qreal volume,
                                    QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1006; }
    bool mergeWith(const QUndoCommand* other) override;

private:
    qreal m_volume;
    qreal m_previousVolume;
};

/**
 * @brief Command to change TTS voice
 */
class SetTtsVoiceCommand : public AccessibilityCommand {
public:
    explicit SetTtsVoiceCommand(AccessibilityController* controller,
                                const QVoice& voice,
                                QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1007; }

private:
    QVoice m_voice;
    QVoice m_previousVoice;
};

/**
 * @brief Command to change TTS locale
 */
class SetTtsLocaleCommand : public AccessibilityCommand {
public:
    explicit SetTtsLocaleCommand(AccessibilityController* controller,
                                 const QLocale& locale,
                                 QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1008; }

private:
    QLocale m_locale;
    QLocale m_previousLocale;
};

/**
 * @brief Command to set high contrast colors
 */
class SetHighContrastColorsCommand : public AccessibilityCommand {
public:
    explicit SetHighContrastColorsCommand(AccessibilityController* controller,
                                          const QColor& background,
                                          const QColor& foreground,
                                          const QColor& highlight,
                                          QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1009; }

private:
    QColor m_backgroundColor;
    QColor m_foregroundColor;
    QColor m_highlightColor;
    QColor m_previousBackgroundColor;
    QColor m_previousForegroundColor;
    QColor m_previousHighlightColor;
};

/**
 * @brief Command to toggle text enlargement
 */
class ToggleTextEnlargementCommand : public AccessibilityCommand {
public:
    explicit ToggleTextEnlargementCommand(AccessibilityController* controller,
                                          bool enable,
                                          QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1010; }

private:
    bool m_enable;
    bool m_previousState;
};

/**
 * @brief Command to set text scale factor
 */
class SetTextScaleFactorCommand : public AccessibilityCommand {
public:
    explicit SetTextScaleFactorCommand(AccessibilityController* controller,
                                       qreal factor,
                                       QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1011; }
    bool mergeWith(const QUndoCommand* other) override;

private:
    qreal m_factor;
    qreal m_previousFactor;
};

/**
 * @brief Command to toggle reduce motion
 */
class ToggleReduceMotionCommand : public AccessibilityCommand {
public:
    explicit ToggleReduceMotionCommand(AccessibilityController* controller,
                                       bool enable,
                                       QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1012; }

private:
    bool m_enable;
    bool m_previousState;
};

/**
 * @brief Command to toggle reduce transparency
 */
class ToggleReduceTransparencyCommand : public AccessibilityCommand {
public:
    explicit ToggleReduceTransparencyCommand(
        AccessibilityController* controller, bool enable,
        QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1013; }

private:
    bool m_enable;
    bool m_previousState;
};

/**
 * @brief Command to reset accessibility settings to defaults
 */
class ResetAccessibilitySettingsCommand : public AccessibilityCommand {
public:
    explicit ResetAccessibilitySettingsCommand(
        AccessibilityController* controller, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1014; }

private:
    AccessibilitySettings m_previousSettings;
};

/**
 * @brief Command to import accessibility settings
 */
class ImportAccessibilitySettingsCommand : public AccessibilityCommand {
public:
    explicit ImportAccessibilitySettingsCommand(
        AccessibilityController* controller, const QString& filePath,
        QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1015; }

private:
    QString m_filePath;
    AccessibilitySettings m_previousSettings;
    AccessibilitySettings m_importedSettings;
    bool m_firstRun;
};

/**
 * @brief Composite command for batch accessibility settings changes
 */
class BatchAccessibilitySettingsCommand : public AccessibilityCommand {
public:
    explicit BatchAccessibilitySettingsCommand(
        AccessibilityController* controller,
        const AccessibilitySettings& settings, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1016; }

private:
    AccessibilitySettings m_newSettings;
    AccessibilitySettings m_previousSettings;
};

/**
 * @brief Factory for creating accessibility commands
 */
class AccessibilityCommandFactory {
public:
    static std::unique_ptr<AccessibilityCommand>
    createToggleScreenReaderCommand(AccessibilityController* controller,
                                    bool enable);

    static std::unique_ptr<AccessibilityCommand>
    createToggleHighContrastCommand(AccessibilityController* controller,
                                    bool enable);

    static std::unique_ptr<AccessibilityCommand> createToggleTtsCommand(
        AccessibilityController* controller, bool enable);

    static std::unique_ptr<AccessibilityCommand> createSetSpeechRateCommand(
        AccessibilityController* controller, qreal rate);

    static std::unique_ptr<AccessibilityCommand> createSetSpeechPitchCommand(
        AccessibilityController* controller, qreal pitch);

    static std::unique_ptr<AccessibilityCommand> createSetSpeechVolumeCommand(
        AccessibilityController* controller, qreal volume);

    static std::unique_ptr<AccessibilityCommand>
    createSetTextScaleFactorCommand(AccessibilityController* controller,
                                    qreal factor);

    static std::unique_ptr<AccessibilityCommand> createResetSettingsCommand(
        AccessibilityController* controller);

    static std::unique_ptr<AccessibilityCommand> createBatchSettingsCommand(
        AccessibilityController* controller,
        const AccessibilitySettings& settings);
};
