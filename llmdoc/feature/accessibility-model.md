# AccessibilityModel

## 1. Purpose

AccessibilityModel serves as the state management layer for the accessibility system. It maintains the complete state of all accessibility features (screen reader, high contrast, text-to-speech, text enlargement, motion reduction, enhanced keyboard navigation) and provides signal-based reactivity for UI updates. The model persists settings via QSettings with optional JSON import/export.

## 2. How it Works

### State Container: AccessibilitySettings Struct

The AccessibilitySettings structure contains all configurable accessibility state:

**Screen Reader Settings**

- screenReaderEnabled: Toggle for screen reader mode
- announcePageChanges: Announce page navigation events
- announceZoomChanges: Announce zoom level changes
- announceSelectionChanges: Announce text selection changes

**High Contrast Settings**

- highContrastMode: Enable/disable high contrast
- backgroundColor, foregroundColor, highlightColor, selectionColor: Custom color palette

**Text-to-Speech Settings**

- ttsEnabled: TTS feature toggle
- ttsEngine: Selected TTS engine identifier
- ttsLocale: Locale for voice selection
- ttsVoice: Selected QVoice object
- ttsRate, ttsPitch, ttsVolume: Parameter ranges (-1.0 to 1.0 for rate/pitch, 0.0 to 1.0 for volume)

**Text Rendering Settings**

- enlargeText: Toggle text enlargement
- textScaleFactor: Scale multiplier (0.5 to 3.0)
- boldText: Companion bold text option

**Keyboard Navigation Settings**

- keyboardNavigationEnhanced: Enhanced keyboard focus
- focusIndicatorVisible: Visual focus indicator
- focusIndicatorWidth: Focus border thickness in pixels

**Motion Settings**

- reduceMotion: Disable animations
- reduceTransparency: Maintain solid backgrounds

**Metadata**

- lastModified: QDateTime timestamp
- version: Settings schema version (currently 1)

### Model Interface

**State Access Methods**

- settings(): Returns current AccessibilitySettings object
- isScreenReaderEnabled(), isHighContrastMode(), isTtsEnabled(): Individual feature checks
- currentLocale(), currentVoice(), speechRate(), speechVolume(): Parameter accessors

**State Modification Methods**

- setSettings(const AccessibilitySettings&): Bulk settings replacement
- setScreenReaderEnabled(bool), setHighContrastMode(bool), etc.: Individual setters
- setTtsRate(qreal), setTtsVolume(qreal), setTextScaleFactor(qreal): Parameter setters

**Settings Persistence**

- saveSettings(): Write to QSettings
- loadSettings(): Read from QSettings
- exportSettings(const QString& path): Write JSON to file
- importSettings(const QString& path): Read JSON from file
- resetToDefaults(): Clear all customization

### Signal-Based Reactivity

Core signals emitted by the model:

- settingsChanged(const AccessibilitySettings&): Emitted on any setting change
- settingsReset(): Emitted when reset to defaults
- settingsSaved(): Emitted after QSettings write completes
- settingsLoaded(): Emitted after QSettings read completes
- settingsImported(const QString& filePath): Emitted after JSON import succeeds

Subscribers connect to these signals to update UI, apply theme changes, reinitialize TTS engine, etc.

### JSON Serialization

AccessibilitySettings implements toJson() and fromJson() methods for persistence:

- Converts all settings to QJsonObject representation
- Preserves version and metadata
- Enables settings sharing across systems
- Used by importSettings() and exportSettings() workflows

## 3. Relevant Code Modules

- `app/model/AccessibilityModel.h/.cpp` - Model implementation
- `app/controller/AccessibilityController.h/.cpp` - Controller observing model signals
- `app/command/AccessibilityCommands.h/.cpp` - Commands modifying model state
- `app/accessibility/AccessibilityManager.h/.cpp` - Legacy wrapper accessing model
- `app/factory/ModelFactory.h` - Model instantiation

## 4. Attention

- All setters validate input ranges (e.g., textScaleFactor: 0.5-3.0) before state change
- settingsChanged signal emits on every single setter call; receivers should implement debouncing if needed
- QSettings keys prefixed with "accessibility/" to namespace settings
- JSON serialization supports future schema evolution via version field
- Model does NOT apply settings to UI/theme directly; controller handles integration
