# Accessibility System

## 1. Purpose

The Accessibility System provides comprehensive support for users with diverse accessibility needs. It implements an MVP (Model-View-Presenter) architecture pattern enabling screen reader support, high contrast themes, text-to-speech, text enlargement, motion reduction, and enhanced keyboard navigation. The system follows the project's standard architectural patterns with full undo/redo support and complete settings persistence.

## 2. How it Works

### Architecture Overview

The accessibility system is built on three core MVP components:

**AccessibilityModel** - State management layer that maintains all accessibility settings and feature toggles. Emits signals when settings change to trigger reactive updates across the application.

**AccessibilityController** - Business logic coordination layer managing text-to-speech engine lifecycle, event bus integration, and feature coordination with other managers (StyleManager, I18nManager).

**AccessibilityCommands** - Undo/redo support through 15+ command classes (ToggleScreenReaderCommand, SetSpeechRateCommand, SetHighContrastColorsCommand, etc.) with command merging for slider-based parameters.

**AccessibilityManager** - Legacy compatibility wrapper in `app/accessibility/` that preserves the original interface while delegating to the MVP architecture for backward compatibility.

### Feature Architecture

#### Screen Reader Mode

Maintains announcement queue for non-blocking text announcements. Triggered by page changes, zoom operations, selection changes, and user-initiated reads. The announceText() method queues announcements for sequential delivery to accessibility APIs.

#### High Contrast Mode

Integrates with StyleManager to apply custom color schemes. Maintains four color settings: background (default white), foreground (default black), highlight (yellow with transparency), and selection (blue). Colors stored in AccessibilitySettings struct with JSON serialization support.

#### Text-to-Speech (TTS)

Full QTextToSpeech engine integration supporting:

- Engine selection from system-available TTS engines
- Locale/voice configuration with QVoice objects
- Rate/pitch/volume control with range -1.0 to 1.0 (or 0.0 to 1.0 for volume)
- Thread-safe speak/pause/resume/stop operations
- State tracking with textToSpeechStateChanged signals

#### Text Enlargement

Scale factor control ranging 0.5 to 3.0 applied through StyleManager. Independently configurable with bold text option for improved readability.

#### Motion Reduction

Two flags: reduceMotion disables animations, reduceTransparency maintains solid backgrounds. Settings integrate with animation/rendering systems via EventBus.

### Data Flow

1. User toggles feature via UI → AccessibilityCommand created
2. Command executes → AccessibilityModel updates settings
3. Model emits signal → AccessibilityController handles business logic
4. Controller updates system integrations (StyleManager, EventBus, TTS engine)
5. Settings persisted to QSettings and optional JSON export

### Settings Persistence

AccessibilitySettings struct serializes to/from JSON with QJsonObject. Supports:

- Auto-save to QSettings on model changes (configurable via setAutoSave())
- Explicit save/load from JSON files
- Export/import for sharing settings across devices
- Version tracking for forward compatibility

## 3. Relevant Code Modules

- `app/model/AccessibilityModel.h/.cpp` - State management and settings storage
- `app/controller/AccessibilityController.h/.cpp` - Business logic and TTS integration
- `app/command/AccessibilityCommands.h/.cpp` - 15+ command classes with undo/redo
- `app/accessibility/AccessibilityManager.h/.cpp` - Backward compatibility wrapper
- `app/i18n/accessibility_en.ts` - English localization strings
- `app/i18n/accessibility_zh.ts` - Chinese localization strings
- `app/managers/StyleManager.h` - Color scheme and theme integration
- `app/managers/I18nManager.h` - Internationalization support
- `app/controller/EventBus.h` - Feature change notifications
- `app/logging/SimpleLogging.h` - Diagnostic logging

## 4. Attention

- QTextToSpeech engine lifecycle must be managed in initialize()/shutdown() to avoid resource leaks
- Settings changes emit signals; subscribe via model()->settingsChanged() for reactive updates
- High contrast colors override application theme; restore with restoreNormalColors() or disable mode
- TTS operations are asynchronous; use textToSpeechStateChanged signals for state tracking
- Announcement queue prevents blocking; use announceText() for non-critical messages
