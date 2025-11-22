# AccessibilityCommands

## 1. Purpose

AccessibilityCommands implements the command pattern for accessibility feature modifications, enabling full undo/redo support for all user accessibility customizations. The command system consists of 15+ command classes following the project's standard QUndoCommand pattern, with support for command merging on slider-based parameters and a factory for command instantiation.

## 2. How it Works

### Command Architecture

All accessibility commands inherit from AccessibilityCommand base class which derives from QUndoCommand. Each command maintains a reference to AccessibilityModel and implements undo()/redo() methods to modify and restore state.

### Command Categories and Classes

**Feature Toggles**

- ToggleScreenReaderCommand: Enable/disable screen reader mode
- ToggleHighContrastCommand: Enable/disable high contrast
- ToggleTextToSpeechCommand: Enable/disable TTS feature
- ToggleTextEnlargementCommand: Enable/disable text enlargement
- ToggleReduceMotionCommand: Enable/disable motion reduction
- ToggleReduceTransparencyCommand: Enable/disable transparency reduction

**TTS Parameter Control (with command merging support)**

- SetSpeechRateCommand: Adjust speech rate (-1.0 to 1.0), merges consecutive rate changes
- SetSpeechPitchCommand: Adjust pitch (-1.0 to 1.0), merges consecutive pitch changes
- SetSpeechVolumeCommand: Adjust volume (0.0 to 1.0), merges consecutive volume changes

**TTS Configuration**

- SetTtsVoiceCommand: Change voice selection with QVoice object
- SetTtsLocaleCommand: Change locale for voice selection

**High Contrast Customization**

- SetHighContrastColorsCommand: Update color palette (background, foreground, highlight, selection)

**Text Rendering**

- SetTextScaleFactorCommand: Set text enlargement scale (0.5 to 3.0)

**Batch Operations**

- ResetAccessibilitySettingsCommand: Clear all customization to defaults
- ImportAccessibilitySettingsCommand: Load settings from JSON file
- BatchAccessibilitySettingsCommand: Apply multiple settings atomically

### Command Merging

Commands for slider-based parameters (rate, pitch, volume, scale factor) implement the mergeWith() method. When consecutive commands of the same type are issued (e.g., dragging a slider), they merge into a single undo action. Merging threshold: changes within short time window merge; after timeout, new command created.

Example: User drags volume slider from 0.5 to 0.8 (10 intermediate values) → single command in undo stack.

### BaseCommand: AccessibilityCommand

The base class template provides:

- Constructor accepting AccessibilityModel* and parameters
- setText() for localized UI labels via i18n keys
- Undo/redo implementation for derived classes
- Parameter validation before state change

### Factory Pattern: AccessibilityCommandFactory

The factory simplifies command creation with static methods:

- createFeatureToggleCommand(const QString& feature, bool enabled): Create appropriate toggle command
- createTtsParameterCommand(const QString& parameter, qreal value): Create TTS adjustment command
- createColorCommand(const QColor& bg, const QColor& fg, etc.): Create high contrast command
- createImportCommand(const QString& filePath): Create settings import command

Factory methods validate parameters and return base QUndoCommand* pointers for direct execution via CommandManager.

### Integration with CommandManager

Commands are instantiated and executed through the application's central CommandManager:

```cpp
auto* cmd = AccessibilityCommandFactory::createFeatureToggleCommand("screen_reader", true);
CommandManager::instance().executeCommand(cmd);
```

The CommandManager maintains undo/redo stacks and handles stack state updates automatically.

### Undo/Redo Workflow

1. User toggles feature → AccessibilityCommand created
2. Command::redo() executes → model->setFeature() called → model emits signal
3. Model change triggers UI update
4. User hits Ctrl+Z → CommandManager calls undo() → restores previous state
5. User hits Ctrl+Y → CommandManager calls redo() → reapplies change

### Text Localization

Command text (displayed in undo/redo menus) is localized via i18n keys:

- "accessibility.cmd.toggle_screen_reader"
- "accessibility.cmd.set_speech_rate"
- "accessibility.cmd.import_settings"

Localization strings stored in `app/i18n/accessibility_en.ts` and `accessibility_zh.ts`.

## 3. Relevant Code Modules

- `app/command/AccessibilityCommands.h/.cpp` - Command class implementations
- `app/model/AccessibilityModel.h/.cpp` - State modified by commands
- `app/command/CommandManager.h/.cpp` - Execution and undo/redo stack management
- `app/factory/CommandFactory.h` - Factory integration point
- `app/i18n/accessibility_en.ts` - English command text
- `app/i18n/accessibility_zh.ts` - Chinese command text

## 4. Attention

- Parameter validation occurs in command::redo(), not constructor; invalid parameters rejected silently
- Merging is time-based; rapid consecutive changes merge, pauses create new stack entries
- BatchAccessibilitySettingsCommand applies all changes atomically; single undo reverts all
- ImportAccessibilitySettingsCommand validates JSON schema before import; malformed files rejected
- ResetAccessibilitySettingsCommand is irreversible via import; exported settings enable recovery
