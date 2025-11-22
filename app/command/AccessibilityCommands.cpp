#include "AccessibilityCommands.h"
#include "../controller/AccessibilityController.h"

// AccessibilityCommand base class
AccessibilityCommand::AccessibilityCommand(AccessibilityController* controller,
                                           const QString& text,
                                           QUndoCommand* parent)
    : QUndoCommand(text, parent),
      m_controller(controller),
      m_logger("AccessibilityCommand") {}

// ToggleScreenReaderCommand
ToggleScreenReaderCommand::ToggleScreenReaderCommand(
    AccessibilityController* controller, bool enable, QUndoCommand* parent)
    : AccessibilityCommand(controller,
                           enable ? QObject::tr("Enable Screen Reader")
                                  : QObject::tr("Disable Screen Reader"),
                           parent),
      m_enable(enable),
      m_previousState(false) {
    if (m_controller) {
        m_previousState = m_controller->isScreenReaderEnabled();
    }
}

void ToggleScreenReaderCommand::undo() {
    if (m_controller) {
        m_controller->enableScreenReader(m_previousState);
        m_logger.info("Undo screen reader toggle");
    }
}

void ToggleScreenReaderCommand::redo() {
    if (m_controller) {
        m_controller->enableScreenReader(m_enable);
        m_logger.info("Redo screen reader toggle");
    }
}

// ToggleHighContrastCommand
ToggleHighContrastCommand::ToggleHighContrastCommand(
    AccessibilityController* controller, bool enable, QUndoCommand* parent)
    : AccessibilityCommand(controller,
                           enable ? QObject::tr("Enable High Contrast")
                                  : QObject::tr("Disable High Contrast"),
                           parent),
      m_enable(enable),
      m_previousState(false) {
    if (m_controller) {
        m_previousState = m_controller->isHighContrastMode();
    }
}

void ToggleHighContrastCommand::undo() {
    if (m_controller) {
        m_controller->setHighContrastMode(m_previousState);
        m_logger.info("Undo high contrast toggle");
    }
}

void ToggleHighContrastCommand::redo() {
    if (m_controller) {
        m_controller->setHighContrastMode(m_enable);
        m_logger.info("Redo high contrast toggle");
    }
}

// ToggleTextToSpeechCommand
ToggleTextToSpeechCommand::ToggleTextToSpeechCommand(
    AccessibilityController* controller, bool enable, QUndoCommand* parent)
    : AccessibilityCommand(controller,
                           enable ? QObject::tr("Enable Text-to-Speech")
                                  : QObject::tr("Disable Text-to-Speech"),
                           parent),
      m_enable(enable),
      m_previousState(false) {
    if (m_controller) {
        m_previousState = m_controller->isTextToSpeechEnabled();
    }
}

void ToggleTextToSpeechCommand::undo() {
    if (m_controller) {
        m_controller->enableTextToSpeech(m_previousState);
        m_logger.info("Undo text-to-speech toggle");
    }
}

void ToggleTextToSpeechCommand::redo() {
    if (m_controller) {
        m_controller->enableTextToSpeech(m_enable);
        m_logger.info("Redo text-to-speech toggle");
    }
}

// SetSpeechRateCommand
SetSpeechRateCommand::SetSpeechRateCommand(AccessibilityController* controller,
                                           qreal rate, QUndoCommand* parent)
    : AccessibilityCommand(controller, QObject::tr("Change Speech Rate"),
                           parent),
      m_rate(rate),
      m_previousRate(0.0) {
    if (m_controller) {
        m_previousRate = m_controller->speechRate();
    }
}

void SetSpeechRateCommand::undo() {
    if (m_controller) {
        m_controller->setSpeechRate(m_previousRate);
        m_logger.debug("Undo speech rate change");
    }
}

void SetSpeechRateCommand::redo() {
    if (m_controller) {
        m_controller->setSpeechRate(m_rate);
        m_logger.debug("Redo speech rate change");
    }
}

bool SetSpeechRateCommand::mergeWith(const QUndoCommand* other) {
    if (other->id() != id()) {
        return false;
    }

    const auto* cmd = static_cast<const SetSpeechRateCommand*>(other);
    m_rate = cmd->m_rate;
    return true;
}

// SetSpeechPitchCommand
SetSpeechPitchCommand::SetSpeechPitchCommand(
    AccessibilityController* controller, qreal pitch, QUndoCommand* parent)
    : AccessibilityCommand(controller, QObject::tr("Change Speech Pitch"),
                           parent),
      m_pitch(pitch),
      m_previousPitch(0.0) {
    if (m_controller) {
        m_previousPitch = m_controller->speechPitch();
    }
}

void SetSpeechPitchCommand::undo() {
    if (m_controller) {
        m_controller->setSpeechPitch(m_previousPitch);
        m_logger.debug("Undo speech pitch change");
    }
}

void SetSpeechPitchCommand::redo() {
    if (m_controller) {
        m_controller->setSpeechPitch(m_pitch);
        m_logger.debug("Redo speech pitch change");
    }
}

bool SetSpeechPitchCommand::mergeWith(const QUndoCommand* other) {
    if (other->id() != id()) {
        return false;
    }

    const auto* cmd = static_cast<const SetSpeechPitchCommand*>(other);
    m_pitch = cmd->m_pitch;
    return true;
}

// SetSpeechVolumeCommand
SetSpeechVolumeCommand::SetSpeechVolumeCommand(
    AccessibilityController* controller, qreal volume, QUndoCommand* parent)
    : AccessibilityCommand(controller, QObject::tr("Change Speech Volume"),
                           parent),
      m_volume(volume),
      m_previousVolume(1.0) {
    if (m_controller) {
        m_previousVolume = m_controller->speechVolume();
    }
}

void SetSpeechVolumeCommand::undo() {
    if (m_controller) {
        m_controller->setSpeechVolume(m_previousVolume);
        m_logger.debug("Undo speech volume change");
    }
}

void SetSpeechVolumeCommand::redo() {
    if (m_controller) {
        m_controller->setSpeechVolume(m_volume);
        m_logger.debug("Redo speech volume change");
    }
}

bool SetSpeechVolumeCommand::mergeWith(const QUndoCommand* other) {
    if (other->id() != id()) {
        return false;
    }

    const auto* cmd = static_cast<const SetSpeechVolumeCommand*>(other);
    m_volume = cmd->m_volume;
    return true;
}

// SetTtsVoiceCommand
SetTtsVoiceCommand::SetTtsVoiceCommand(AccessibilityController* controller,
                                       const QVoice& voice,
                                       QUndoCommand* parent)
    : AccessibilityCommand(controller, QObject::tr("Change TTS Voice"), parent),
      m_voice(voice),
      m_previousVoice() {
    if (m_controller) {
        m_previousVoice = m_controller->currentVoice();
    }
}

void SetTtsVoiceCommand::undo() {
    if (m_controller) {
        m_controller->setVoice(m_previousVoice);
        m_logger.info("Undo TTS voice change");
    }
}

void SetTtsVoiceCommand::redo() {
    if (m_controller) {
        m_controller->setVoice(m_voice);
        m_logger.info("Redo TTS voice change");
    }
}

// SetTtsLocaleCommand
SetTtsLocaleCommand::SetTtsLocaleCommand(AccessibilityController* controller,
                                         const QLocale& locale,
                                         QUndoCommand* parent)
    : AccessibilityCommand(controller, QObject::tr("Change TTS Language"),
                           parent),
      m_locale(locale),
      m_previousLocale() {
    if (m_controller) {
        m_previousLocale = m_controller->currentLocale();
    }
}

void SetTtsLocaleCommand::undo() {
    if (m_controller) {
        m_controller->setLocale(m_previousLocale);
        m_logger.info("Undo TTS locale change");
    }
}

void SetTtsLocaleCommand::redo() {
    if (m_controller) {
        m_controller->setLocale(m_locale);
        m_logger.info("Redo TTS locale change");
    }
}

// SetHighContrastColorsCommand
SetHighContrastColorsCommand::SetHighContrastColorsCommand(
    AccessibilityController* controller, const QColor& background,
    const QColor& foreground, const QColor& highlight, QUndoCommand* parent)
    : AccessibilityCommand(controller,
                           QObject::tr("Change High Contrast Colors"), parent),
      m_backgroundColor(background),
      m_foregroundColor(foreground),
      m_highlightColor(highlight),
      m_previousBackgroundColor(),
      m_previousForegroundColor(),
      m_previousHighlightColor() {
    if (m_controller && m_controller->model()) {
        m_previousBackgroundColor = m_controller->model()->backgroundColor();
        m_previousForegroundColor = m_controller->model()->foregroundColor();
        m_previousHighlightColor = m_controller->model()->highlightColor();
    }
}

void SetHighContrastColorsCommand::undo() {
    if (m_controller && m_controller->model()) {
        auto* model = m_controller->model();
        model->setBackgroundColor(m_previousBackgroundColor);
        model->setForegroundColor(m_previousForegroundColor);
        model->setHighlightColor(m_previousHighlightColor);
        m_logger.info("Undo high contrast colors change");
    }
}

void SetHighContrastColorsCommand::redo() {
    if (m_controller && m_controller->model()) {
        auto* model = m_controller->model();
        model->setBackgroundColor(m_backgroundColor);
        model->setForegroundColor(m_foregroundColor);
        model->setHighlightColor(m_highlightColor);
        m_logger.info("Redo high contrast colors change");
    }
}

// ToggleTextEnlargementCommand
ToggleTextEnlargementCommand::ToggleTextEnlargementCommand(
    AccessibilityController* controller, bool enable, QUndoCommand* parent)
    : AccessibilityCommand(controller,
                           enable ? QObject::tr("Enable Text Enlargement")
                                  : QObject::tr("Disable Text Enlargement"),
                           parent),
      m_enable(enable),
      m_previousState(false) {
    if (m_controller) {
        m_previousState = m_controller->isTextEnlargementEnabled();
    }
}

void ToggleTextEnlargementCommand::undo() {
    if (m_controller) {
        m_controller->setTextEnlargement(m_previousState);
        m_logger.info("Undo text enlargement toggle");
    }
}

void ToggleTextEnlargementCommand::redo() {
    if (m_controller) {
        m_controller->setTextEnlargement(m_enable);
        m_logger.info("Redo text enlargement toggle");
    }
}

// SetTextScaleFactorCommand
SetTextScaleFactorCommand::SetTextScaleFactorCommand(
    AccessibilityController* controller, qreal factor, QUndoCommand* parent)
    : AccessibilityCommand(controller, QObject::tr("Change Text Scale"),
                           parent),
      m_factor(factor),
      m_previousFactor(1.0) {
    if (m_controller) {
        m_previousFactor = m_controller->textScaleFactor();
    }
}

void SetTextScaleFactorCommand::undo() {
    if (m_controller) {
        m_controller->setTextScaleFactor(m_previousFactor);
        m_logger.debug("Undo text scale factor change");
    }
}

void SetTextScaleFactorCommand::redo() {
    if (m_controller) {
        m_controller->setTextScaleFactor(m_factor);
        m_logger.debug("Redo text scale factor change");
    }
}

bool SetTextScaleFactorCommand::mergeWith(const QUndoCommand* other) {
    if (other->id() != id()) {
        return false;
    }

    const auto* cmd = static_cast<const SetTextScaleFactorCommand*>(other);
    m_factor = cmd->m_factor;
    return true;
}

// ToggleReduceMotionCommand
ToggleReduceMotionCommand::ToggleReduceMotionCommand(
    AccessibilityController* controller, bool enable, QUndoCommand* parent)
    : AccessibilityCommand(controller,
                           enable ? QObject::tr("Enable Reduce Motion")
                                  : QObject::tr("Disable Reduce Motion"),
                           parent),
      m_enable(enable),
      m_previousState(false) {
    if (m_controller) {
        m_previousState = m_controller->shouldReduceMotion();
    }
}

void ToggleReduceMotionCommand::undo() {
    if (m_controller) {
        m_controller->setReduceMotion(m_previousState);
        m_logger.info("Undo reduce motion toggle");
    }
}

void ToggleReduceMotionCommand::redo() {
    if (m_controller) {
        m_controller->setReduceMotion(m_enable);
        m_logger.info("Redo reduce motion toggle");
    }
}

// ToggleReduceTransparencyCommand
ToggleReduceTransparencyCommand::ToggleReduceTransparencyCommand(
    AccessibilityController* controller, bool enable, QUndoCommand* parent)
    : AccessibilityCommand(controller,
                           enable ? QObject::tr("Enable Reduce Transparency")
                                  : QObject::tr("Disable Reduce Transparency"),
                           parent),
      m_enable(enable),
      m_previousState(false) {
    if (m_controller) {
        m_previousState = m_controller->shouldReduceTransparency();
    }
}

void ToggleReduceTransparencyCommand::undo() {
    if (m_controller) {
        m_controller->setReduceTransparency(m_previousState);
        m_logger.info("Undo reduce transparency toggle");
    }
}

void ToggleReduceTransparencyCommand::redo() {
    if (m_controller) {
        m_controller->setReduceTransparency(m_enable);
        m_logger.info("Redo reduce transparency toggle");
    }
}

// ResetAccessibilitySettingsCommand
ResetAccessibilitySettingsCommand::ResetAccessibilitySettingsCommand(
    AccessibilityController* controller, QUndoCommand* parent)
    : AccessibilityCommand(controller,
                           QObject::tr("Reset Accessibility Settings"), parent),
      m_previousSettings() {
    if (m_controller && m_controller->model()) {
        m_previousSettings = m_controller->model()->settings();
    }
}

void ResetAccessibilitySettingsCommand::undo() {
    if (m_controller && m_controller->model()) {
        m_controller->model()->setSettings(m_previousSettings);
        m_logger.info("Undo accessibility settings reset");
    }
}

void ResetAccessibilitySettingsCommand::redo() {
    if (m_controller && m_controller->model()) {
        m_controller->model()->resetToDefaults();
        m_logger.info("Redo accessibility settings reset");
    }
}

// ImportAccessibilitySettingsCommand
ImportAccessibilitySettingsCommand::ImportAccessibilitySettingsCommand(
    AccessibilityController* controller, const QString& filePath,
    QUndoCommand* parent)
    : AccessibilityCommand(
          controller, QObject::tr("Import Accessibility Settings"), parent),
      m_filePath(filePath),
      m_previousSettings(),
      m_importedSettings(),
      m_firstRun(true) {
    if (m_controller && m_controller->model()) {
        m_previousSettings = m_controller->model()->settings();
    }
}

void ImportAccessibilitySettingsCommand::undo() {
    if (m_controller && m_controller->model()) {
        m_controller->model()->setSettings(m_previousSettings);
        m_logger.info("Undo accessibility settings import");
    }
}

void ImportAccessibilitySettingsCommand::redo() {
    if (m_controller && m_controller->model()) {
        if (m_firstRun) {
            m_controller->model()->importSettings(m_filePath);
            m_importedSettings = m_controller->model()->settings();
            m_firstRun = false;
        } else {
            m_controller->model()->setSettings(m_importedSettings);
        }
        m_logger.info("Redo accessibility settings import");
    }
}

// BatchAccessibilitySettingsCommand
BatchAccessibilitySettingsCommand::BatchAccessibilitySettingsCommand(
    AccessibilityController* controller, const AccessibilitySettings& settings,
    QUndoCommand* parent)
    : AccessibilityCommand(
          controller, QObject::tr("Update Accessibility Settings"), parent),
      m_newSettings(settings),
      m_previousSettings() {
    if (m_controller && m_controller->model()) {
        m_previousSettings = m_controller->model()->settings();
    }
}

void BatchAccessibilitySettingsCommand::undo() {
    if (m_controller && m_controller->model()) {
        m_controller->model()->setSettings(m_previousSettings);
        m_logger.info("Undo batch accessibility settings change");
    }
}

void BatchAccessibilitySettingsCommand::redo() {
    if (m_controller && m_controller->model()) {
        m_controller->model()->setSettings(m_newSettings);
        m_logger.info("Redo batch accessibility settings change");
    }
}

// AccessibilityCommandFactory
std::unique_ptr<AccessibilityCommand>
AccessibilityCommandFactory::createToggleScreenReaderCommand(
    AccessibilityController* controller, bool enable) {
    return std::make_unique<ToggleScreenReaderCommand>(controller, enable);
}

std::unique_ptr<AccessibilityCommand>
AccessibilityCommandFactory::createToggleHighContrastCommand(
    AccessibilityController* controller, bool enable) {
    return std::make_unique<ToggleHighContrastCommand>(controller, enable);
}

std::unique_ptr<AccessibilityCommand>
AccessibilityCommandFactory::createToggleTtsCommand(
    AccessibilityController* controller, bool enable) {
    return std::make_unique<ToggleTextToSpeechCommand>(controller, enable);
}

std::unique_ptr<AccessibilityCommand>
AccessibilityCommandFactory::createSetSpeechRateCommand(
    AccessibilityController* controller, qreal rate) {
    return std::make_unique<SetSpeechRateCommand>(controller, rate);
}

std::unique_ptr<AccessibilityCommand>
AccessibilityCommandFactory::createSetSpeechPitchCommand(
    AccessibilityController* controller, qreal pitch) {
    return std::make_unique<SetSpeechPitchCommand>(controller, pitch);
}

std::unique_ptr<AccessibilityCommand>
AccessibilityCommandFactory::createSetSpeechVolumeCommand(
    AccessibilityController* controller, qreal volume) {
    return std::make_unique<SetSpeechVolumeCommand>(controller, volume);
}

std::unique_ptr<AccessibilityCommand>
AccessibilityCommandFactory::createSetTextScaleFactorCommand(
    AccessibilityController* controller, qreal factor) {
    return std::make_unique<SetTextScaleFactorCommand>(controller, factor);
}

std::unique_ptr<AccessibilityCommand>
AccessibilityCommandFactory::createResetSettingsCommand(
    AccessibilityController* controller) {
    return std::make_unique<ResetAccessibilitySettingsCommand>(controller);
}

std::unique_ptr<AccessibilityCommand>
AccessibilityCommandFactory::createBatchSettingsCommand(
    AccessibilityController* controller,
    const AccessibilitySettings& settings) {
    return std::make_unique<BatchAccessibilitySettingsCommand>(controller,
                                                               settings);
}
