#pragma once

#include <QHash>
#include <QKeySequence>
#include <QObject>
#include <QShortcut>
#include <QWidget>
#include "../controller/tool.hpp"
#include "../logging/SimpleLogging.h"

/**
 * @brief Centralized keyboard shortcut management system
 *
 * @details This manager provides comprehensive keyboard shortcut handling
 * including:
 * - Global application shortcuts that work across all widgets
 * - Context-sensitive shortcuts that only work in specific widgets
 * - Navigation shortcuts (Page Up/Down, Home/End, arrow keys)
 * - Zoom shortcuts with proper validation
 * - File operation shortcuts with error handling
 * - Accessibility support for keyboard-only navigation
 * - Shortcut conflict detection and resolution
 * - Dynamic shortcut registration and modification
 *
 * **Shortcut Categories:**
 * - File Operations: Ctrl+O, Ctrl+S, Ctrl+Shift+S, etc.
 * - Navigation: Page Up/Down, Home/End, Ctrl+Home/End, arrow keys
 * - Zoom: Ctrl++, Ctrl+-, Ctrl+0, Ctrl+Shift++, etc.
 * - View: F9, F11, Ctrl+1, Ctrl+2, etc.
 * - Tab Management: Ctrl+T, Ctrl+W, Ctrl+Tab, etc.
 * - Search: Ctrl+F, F3, Shift+F3, Escape
 * - Accessibility: Tab, Shift+Tab, Enter, Space, etc.
 *
 * **Accessibility Features:**
 * - Full keyboard navigation support
 * - Focus management for keyboard-only users
 * - Screen reader compatibility
 * - High contrast mode support
 * - Customizable shortcut keys
 */
class KeyboardShortcutManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Shortcut context defines where shortcuts are active
     */
    enum class ShortcutContext {
        Global,        // Active everywhere in the application
        DocumentView,  // Active only when document viewer has focus
        MenuBar,       // Active only when menu bar has focus
        ToolBar,       // Active only when toolbar has focus
        SideBar,       // Active only when sidebar has focus
        SearchWidget,  // Active only when search widget has focus
        Dialog         // Active only in dialog windows
    };

    /**
     * @brief Shortcut priority for conflict resolution
     */
    enum class ShortcutPriority { Low = 0, Normal = 1, High = 2, Critical = 3 };

    /**
     * @brief Shortcut information structure
     */
    struct ShortcutInfo {
        QKeySequence keySequence;
        ActionMap action;
        ShortcutContext context;
        ShortcutPriority priority;
        QString description;
        bool enabled;
        QWidget* contextWidget;

        ShortcutInfo() = default;
        ShortcutInfo(const QKeySequence& seq, ActionMap act,
                     ShortcutContext ctx,
                     ShortcutPriority prio = ShortcutPriority::Normal,
                     const QString& desc = QString(), QWidget* widget = nullptr)
            : keySequence(seq),
              action(act),
              context(ctx),
              priority(prio),
              description(desc),
              enabled(true),
              contextWidget(widget) {}
    };

    /**
     * @brief Get singleton instance
     */
    static KeyboardShortcutManager& instance();

    /**
     * @brief Initialize the shortcut manager with main window
     * @param mainWindow Main application window
     */
    void initialize(QWidget* mainWindow);

    /**
     * @brief Register a keyboard shortcut
     * @param shortcutInfo Shortcut information
     * @return true if registered successfully, false if conflict exists
     */
    bool registerShortcut(const ShortcutInfo& shortcutInfo);

    /**
     * @brief Unregister a keyboard shortcut
     * @param keySequence Key sequence to unregister
     * @param context Shortcut context
     * @return true if unregistered successfully
     */
    bool unregisterShortcut(const QKeySequence& keySequence,
                            ShortcutContext context);

    /**
     * @brief Enable or disable a shortcut
     * @param keySequence Key sequence
     * @param context Shortcut context
     * @param enabled Enable state
     */
    void setShortcutEnabled(const QKeySequence& keySequence,
                            ShortcutContext context, bool enabled);

    /**
     * @brief Get all registered shortcuts for a context
     * @param context Shortcut context
     * @return List of shortcut information
     */
    QList<ShortcutInfo> getShortcuts(ShortcutContext context) const;

    /**
     * @brief Get shortcut for an action
     * @param action Action to find shortcut for
     * @param context Shortcut context
     * @return Key sequence or empty if not found
     */
    QKeySequence getShortcutForAction(ActionMap action,
                                      ShortcutContext context) const;

    /**
     * @brief Check if a key sequence conflicts with existing shortcuts
     * @param keySequence Key sequence to check
     * @param context Shortcut context
     * @return true if conflict exists
     */
    bool hasConflict(const QKeySequence& keySequence,
                     ShortcutContext context) const;

    /**
     * @brief Set up default application shortcuts
     */
    void setupDefaultShortcuts();

    /**
     * @brief Set up navigation shortcuts (Page Up/Down, Home/End, arrows)
     */
    void setupNavigationShortcuts();

    /**
     * @brief Set up zoom shortcuts with validation
     */
    void setupZoomShortcuts();

    /**
     * @brief Set up file operation shortcuts
     */
    void setupFileOperationShortcuts();

    /**
     * @brief Set up accessibility shortcuts
     */
    void setupAccessibilityShortcuts();

    /**
     * @brief Enable accessibility mode
     * @param enabled Enable accessibility features
     */
    void setAccessibilityMode(bool enabled);

    /**
     * @brief Get accessibility mode state
     */
    bool isAccessibilityMode() const { return m_accessibilityMode; }

    /**
     * @brief Set context widget for focus-based shortcuts
     * @param context Shortcut context
     * @param widget Context widget
     */
    void setContextWidget(ShortcutContext context, QWidget* widget);

    /**
     * @brief Get context widget
     * @param context Shortcut context
     * @return Context widget or nullptr
     */
    QWidget* getContextWidget(ShortcutContext context) const;

signals:
    /**
     * @brief Emitted when a shortcut is activated
     * @param action Action to execute
     * @param context Context where shortcut was activated
     */
    void shortcutActivated(ActionMap action,
                           KeyboardShortcutManager::ShortcutContext context);

    /**
     * @brief Emitted when shortcut configuration changes
     */
    void shortcutsChanged();

    /**
     * @brief Emitted when accessibility mode changes
     * @param enabled Accessibility mode state
     */
    void accessibilityModeChanged(bool enabled);

private slots:
    /**
     * @brief Handle shortcut activation
     */
    void onShortcutActivated();

    /**
     * @brief Handle focus changes for context-sensitive shortcuts
     * @param old Previous focus widget
     * @param now Current focus widget
     */
    void onFocusChanged(QWidget* old, QWidget* now);

private:
    /**
     * @brief Private constructor for singleton
     */
    explicit KeyboardShortcutManager(QObject* parent = nullptr);

    /**
     * @brief Create QShortcut object for shortcut info
     * @param info Shortcut information
     * @return Created QShortcut object
     */
    QShortcut* createShortcut(const ShortcutInfo& info);

    /**
     * @brief Update shortcut enabled state based on context
     * @param context Current context
     */
    void updateShortcutsForContext(ShortcutContext context);

    /**
     * @brief Get current context based on focus
     * @return Current shortcut context
     */
    ShortcutContext getCurrentContext() const;

    /**
     * @brief Validate shortcut key sequence
     * @param keySequence Key sequence to validate
     * @return true if valid
     */
    bool isValidKeySequence(const QKeySequence& keySequence) const;

    // Singleton instance
    static KeyboardShortcutManager* s_instance;

    // Main application window
    QWidget* m_mainWindow;

    // Shortcut storage
    QHash<QString, ShortcutInfo> m_shortcuts;  // key: "context:keysequence"
    QHash<QString, QShortcut*> m_qshortcuts;   // key: "context:keysequence"

    // Context widgets
    QHash<ShortcutContext, QWidget*> m_contextWidgets;

    // Current context
    ShortcutContext m_currentContext;

    // Accessibility mode
    bool m_accessibilityMode;

    // Logging
    SastLogging::CategoryLogger m_logger;

    // Helper methods for shortcut key generation
    QString getShortcutKey(const QKeySequence& keySequence,
                           ShortcutContext context) const;
    QString contextToString(ShortcutContext context) const;
};
