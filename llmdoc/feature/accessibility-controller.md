# AccessibilityController

## 1. Purpose

AccessibilityController implements the business logic layer for the accessibility system. It coordinates feature implementation across the application by managing text-to-speech engine lifecycle, integrating with the event bus for decoupled communication, coordinating with StyleManager for high contrast theming, and handling screen reader announcements. The controller acts as the primary interface between the accessibility model and application subsystems.

## 2. How it Works

### Initialization and Lifecycle

The controller requires initialize() before use to set up the QTextToSpeech engine and event bus subscriptions. Shutdown() must be called during application teardown to release the TTS engine and disconnect signals. The isInitialized() flag prevents double initialization.

### Screen Reader System

Screen reader support operates through:

- enableScreenReaderEnabled(bool): Toggles feature and publishes event via EventBus
- Announcement methods: announceText(), announcePageChange(), announceZoomChange(), announceSelectionChange(), announceError(), announceWarning(), announceSuccess()
- All announcements queue non-blocking via the announcement queue system
- readCurrentPage(), readSelectedText(), readDocumentTitle(): Specialized read operations
- readStatusMessage(const QString&): Generic message announcement

Announcements support priority ordering; urgent announcements (errors, warnings) may be expedited. The controller manages the queue to prevent announcement overlap and maintains state for accessibility APIs.

### Text-to-Speech Engine Management

Full QTextToSpeech integration with lifecycle management:

**Engine Selection**

- availableEngines(): Returns list of system TTS engines
- currentEngine(): Get selected engine
- setEngine(const QString&): Switch TTS engine and reinitialize

**Locale and Voice Selection**

- availableLocales(): Returns list of supported locales
- currentLocale(): Get selected locale
- setLocale(const QLocale&): Update locale (triggers voice list refresh)
- availableVoices(): Returns voices for current locale
- currentVoice(): Get selected voice
- setVoice(const QVoice&): Update voice

**TTS Parameter Control**

- speechRate()/setSpeechRate(qreal): Rate parameter (-1.0 to 1.0)
- speechPitch()/setSpeechPitch(qreal): Pitch parameter (-1.0 to 1.0)
- speechVolume()/setSpeechVolume(qreal): Volume parameter (0.0 to 1.0)

**TTS Operations**

- speak(const QString&): Queue text for speech
- pause(): Pause active speech
- resume(): Resume paused speech
- stop(): Cancel speech and clear queue
- textToSpeechState(): Query QTextToSpeech::State (Ready, Speaking, Paused, Error)
- isTextToSpeechAvailable(): Check engine availability

### High Contrast Mode

Integrates with StyleManager to apply accessibility color schemes:

- setHighContrastMode(bool): Enable/disable high contrast
- isHighContrastMode(): Check current state
- applyHighContrastColors(): Apply stored color palette via StyleManager
- restoreNormalColors(): Revert to application default theme

High contrast colors are stored in AccessibilityModel and applied through the color adaptation pipeline in StyleManager.

### Text Rendering

- setTextEnlargement(bool): Toggle feature
- isTextEnlargementEnabled(): Check state
- setTextScaleFactor(qreal): Set scale multiplier (0.5 to 3.0)
- textScaleFactor(): Query current scale

The controller publishes scale factor changes via EventBus for UI components to consume.

### Motion and Effects Control

- setReduceMotion(bool)/shouldReduceMotion(): Control animation disabling
- setReduceTransparency(bool)/shouldReduceTransparency(): Control transparency effects
- Both settings published via EventBus to rendering/animation subsystems

### Keyboard Navigation Enhancement

- setEnhancedKeyboardNavigation(bool): Enable focus tracking enhancements
- isEnhancedKeyboardNavigationEnabled(): Check state
- Coordinates with keyboard shortcut system for enhanced focus indicators

### Event Bus Integration

The controller publishes accessibility changes to EventBus for decoupled subsystem updates:

- "accessibility/screen_reader_enabled" → screen reader state
- "accessibility/high_contrast_enabled" → contrast mode state
- "accessibility/text_scale_factor_changed" → enlargement factor
- "accessibility/reduce_motion" → motion reduction state
- Subscribers can react to changes without direct controller dependency

### Utility Methods

- testTextToSpeech(): Verify TTS engine functionality
- getAccessibilityStatus(): Return JSON string describing enabled features
- getEnabledFeatures(): Return QStringList of active feature names

## 3. Relevant Code Modules

- `app/controller/AccessibilityController.h/.cpp` - Controller implementation
- `app/model/AccessibilityModel.h/.cpp` - Underlying state model
- `app/command/AccessibilityCommands.h/.cpp` - Commands triggering controller methods
- `app/controller/EventBus.h` - Event publication interface
- `app/managers/StyleManager.h` - High contrast color application
- `app/managers/I18nManager.h` - Localization for announcements
- `app/logging/SimpleLogging.h` - Diagnostic logging

## 4. Attention

- initialize()/shutdown() must be paired; double initialization is guarded but wastes resources
- QTextToSpeech is asynchronous; monitor textToSpeechStateChanged signals for completion
- setEngine() requires full reinitialization; collect multiple changes before applying to reduce overhead
- Announcement queue is non-blocking; subscribers implement their own delivery mechanism
- EventBus subscriptions in initialize() must be disconnected in shutdown() to prevent dangling subscriptions
