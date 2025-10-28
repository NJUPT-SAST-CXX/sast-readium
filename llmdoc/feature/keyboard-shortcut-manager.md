# Keyboard Shortcut Manager

## 1. Purpose

The KeyboardShortcutManager provides centralized management of keyboard shortcuts throughout the application. It enables context-sensitive shortcuts (different behavior in different UI contexts), conflict detection, accessibility support, and dynamic shortcut registration without hardcoding shortcuts into individual widgets.

## 2. How It Works

### Architecture

The manager uses a singleton pattern for global access. Core responsibilities include:

- **Registration System**: Shortcuts are registered as `ShortcutInfo` objects containing key sequence, action, context, priority, and description
- **Context Management**: Shortcuts are filtered based on current focus context (Global, DocumentView, MenuBar, ToolBar, SideBar, SearchWidget, Dialog)
- **Conflict Detection**: Before registering, checks for existing shortcuts with same key sequence in same context
- **Focus Tracking**: Monitors QApplication focus changes via `onFocusChanged` signal handler to update active context

### Data Flow

1. Application calls `KeyboardShortcutManager::initialize(mainWindow)` at startup
2. Manager connects to QApplication::focusChanged signal using `qobject_cast<QApplication*>(QApplication::instance())`
3. Shortcuts registered via `registerShortcut()` are stored in `m_shortcuts` hash (key: "context:keysequence")
4. QShortcut objects created via `createShortcut()` are stored in `m_qshortcuts` hash
5. When shortcut activated, `onShortcutActivated()` emits `shortcutActivated(ActionMap action, ShortcutContext context)` signal
6. Connected components handle the action and update UI accordingly

### Key Components

- **ShortcutInfo Structure**: Encapsulates key sequence, action, context, priority, description, enabled state, and context widget
- **ShortcutContext Enum**: Defines where shortcuts are active (Global, DocumentView, MenuBar, ToolBar, SideBar, SearchWidget, Dialog)
- **ShortcutPriority Enum**: Used for conflict resolution (Low=0, Normal=1, High=2, Critical=3)
- **ActionMap Enum**: Maps shortcuts to 60+ predefined actions (openFile, toggleTheme, zoomIn, etc.)

### Critical Build Fix Applied

**Type Cast Issue (Line 35)**: Changed from direct `QApplication::instance()` to `qobject_cast<QApplication*>(QApplication::instance())` to resolve invalid conversion from `QCoreApplication*` to `const QApplication*`. The `focusChanged` signal is only available on QApplication, not QCoreApplication.

## 3. Relevant Code Modules

- `/app/managers/KeyboardShortcutManager.h` - Header with class definition and API
- `/app/managers/KeyboardShortcutManager.cpp` - Implementation with singleton instance and signal handling
- `/app/controller/tool.hpp` - ActionMap enum definition with 60+ action types
- `/app/logging/SimpleLogging.h` - Logging integration

## 4. Attention

- Shortcuts are stored with composite key "context:keysequence" to allow same key in different contexts
- Focus change tracking relies on QApplication instance availability at initialization time
- Accessibility mode can enable/disable shortcuts dynamically via `setAccessibilityMode()`
- Context widgets must be explicitly registered via `setContextWidget()` for context-sensitive filtering to work
