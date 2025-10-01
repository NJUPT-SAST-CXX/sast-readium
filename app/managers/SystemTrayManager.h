#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <memory>
#include "../logging/SimpleLogging.h"

// Forward declarations
class QMainWindow;
class RecentFilesManager;
class ConfigurationManager;
class SystemTrayManagerImpl;

/**
 * @brief SystemTrayManager - Manages system tray functionality
 * 
 * This class provides comprehensive system tray integration following the
 * established manager pattern used throughout SAST Readium. It handles
 * system tray icon display, context menu management, window minimize/restore
 * functionality, and user notifications.
 * 
 * Features:
 * - Cross-platform system tray support with graceful fallback
 * - Context menu with restore and exit actions
 * - Window minimize-to-tray functionality
 * - First-time user notifications and guidance
 * - Integration with existing settings system
 * - Proper lifecycle management and error handling
 */
class SystemTrayManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Destructor
     */
    ~SystemTrayManager();

    /**
     * @brief Get singleton instance
     * @return Reference to the singleton instance
     */
    static SystemTrayManager& instance();

    /**
     * @brief Initialize the system tray manager
     * @param mainWindow Pointer to the main application window
     * @return true if initialization was successful, false otherwise
     */
    bool initialize(QMainWindow* mainWindow);

    /**
     * @brief Shutdown the system tray manager
     * Performs cleanup and saves settings
     */
    void shutdown();

    /**
     * @brief Check if system tray is available on this platform
     * @return true if system tray is available, false otherwise
     */
    static bool isSystemTrayAvailable();

    /**
     * @brief Check if system tray is currently enabled
     * @return true if enabled, false otherwise
     */
    bool isEnabled() const;

    /**
     * @brief Enable or disable system tray functionality
     * @param enabled true to enable, false to disable
     */
    void setEnabled(bool enabled);

    /**
     * @brief Check if minimize-to-tray is enabled
     * @return true if minimize-to-tray is enabled, false otherwise
     */
    bool isMinimizeToTrayEnabled() const;

    /**
     * @brief Enable or disable minimize-to-tray functionality
     * @param enabled true to enable, false to disable
     */
    void setMinimizeToTrayEnabled(bool enabled);

    /**
     * @brief Show the main window and bring it to front
     */
    void showMainWindow();

    /**
     * @brief Hide the main window to system tray
     * @param showNotification true to show first-time notification if applicable
     */
    void hideMainWindow(bool showNotification = true);

    /**
     * @brief Check if the main window is currently hidden to tray
     * @return true if hidden to tray, false otherwise
     */
    bool isMainWindowHidden() const;

    /**
     * @brief Request application exit through proper channels
     */
    void requestApplicationExit();

    /**
     * @brief Apply settings changes at runtime
     * @param settingsGroup The settings group that changed
     * @param key The specific setting key that changed
     * @param value The new value
     */
    void applySettingsChange(const QString& settingsGroup, const QString& key, const QVariant& value);

    /**
     * @brief Check and handle runtime system tray availability changes
     * This method can be called periodically to detect if system tray becomes available/unavailable
     */
    void checkSystemTrayAvailability();

    // Enhanced functionality methods

    /**
     * @brief Set the current application status for tray icon display
     * @param status The status to display (idle, processing, error, etc.)
     * @param message Optional status message for tooltip
     */
    void setApplicationStatus(const QString& status, const QString& message = QString());

    /**
     * @brief Show a notification through the system tray
     * @param title Notification title
     * @param message Notification message
     * @param type Notification type (info, warning, error)
     * @param timeout Display timeout in milliseconds
     */
    void showNotification(const QString& title, const QString& message,
                         const QString& type = "info", int timeout = 5000);

    /**
     * @brief Update the dynamic tooltip with current application state
     * @param tooltip New tooltip text
     */
    void updateDynamicTooltip(const QString& tooltip = QString());

    /**
     * @brief Get current application status
     * @return Current status string
     */
    QString currentApplicationStatus() const;

    /**
     * @brief Check if enhanced features are enabled
     * @return true if enhanced features are enabled
     */
    bool areEnhancedFeaturesEnabled() const;

    /**
     * @brief Set notification types that should be shown
     * @param types Comma-separated list of notification types (document,status,error,all)
     */
    void setNotificationTypes(const QString& types);

    /**
     * @brief Get current notification types setting
     * @return Comma-separated list of enabled notification types
     */
    QString getNotificationTypes() const;

    /**
     * @brief Connect to RecentFilesManager signals
     * @param recentFilesManager Pointer to RecentFilesManager instance
     */
    void connectToRecentFilesManager(RecentFilesManager* recentFilesManager);

public slots:
    /**
     * @brief Handle main window close event
     * Called by MainWindow::closeEvent() to determine behavior
     * @return true if the close event should be ignored (minimize to tray), false to allow normal close
     */
    bool handleMainWindowCloseEvent();

    /**
     * @brief Load settings from configuration
     */
    void loadSettings();

    /**
     * @brief Save current settings to configuration
     */
    void saveSettings();

signals:
    /**
     * @brief Emitted when system tray enabled state changes
     * @param enabled true if enabled, false if disabled
     */
    void enabledChanged(bool enabled);

    /**
     * @brief Emitted when minimize-to-tray enabled state changes
     * @param enabled true if enabled, false if disabled
     */
    void minimizeToTrayEnabledChanged(bool enabled);

    /**
     * @brief Emitted when main window visibility changes due to tray operations
     * @param visible true if window is visible, false if hidden to tray
     */
    void mainWindowVisibilityChanged(bool visible);

    /**
     * @brief Emitted when show notifications setting changes
     * @param enabled true if notifications are enabled, false if disabled
     */
    void showNotificationsChanged(bool enabled);

    /**
     * @brief Emitted when application exit is requested from tray
     */
    void applicationExitRequested();

    // Enhanced functionality signals

    /**
     * @brief Emitted when application status changes
     * @param status New status string
     * @param message Optional status message
     */
    void applicationStatusChanged(const QString& status, const QString& message);

    /**
     * @brief Emitted when a notification is shown
     * @param title Notification title
     * @param message Notification message
     * @param type Notification type
     */
    void notificationShown(const QString& title, const QString& message, const QString& type);

    /**
     * @brief Emitted when recent file is requested to be opened
     * @param filePath Path to the file to open
     */
    void recentFileRequested(const QString& filePath);

    /**
     * @brief Emitted when quick action is triggered
     * @param actionId Identifier of the triggered action
     */
    void quickActionTriggered(const QString& actionId);

    /**
     * @brief Emitted when settings dialog is requested
     */
    void settingsDialogRequested();

    /**
     * @brief Emitted when about dialog is requested
     */
    void aboutDialogRequested();

    /**
     * @brief Emitted when enhanced features enabled state changes
     * @param enabled true if enhanced features are enabled
     */
    void enhancedFeaturesChanged(bool enabled);

private slots:
    /**
     * @brief Handle system tray icon activation
     * @param reason The activation reason (click, double-click, etc.)
     */
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

    /**
     * @brief Handle restore window action
     */
    void onRestoreAction();

    /**
     * @brief Handle exit application action
     */
    void onExitAction();

private:
    /**
     * @brief Private constructor for singleton pattern
     * @param parent Parent object
     */
    explicit SystemTrayManager(QObject* parent = nullptr);

    /**
     * @brief Create and setup the system tray icon
     */
    void createTrayIcon();

    /**
     * @brief Create and setup the context menu
     */
    void createContextMenu();

    /**
     * @brief Update context menu state based on window visibility
     */
    void updateContextMenuState();

    /**
     * @brief Show first-time user notification
     */
    void showFirstTimeNotification();

    /**
     * @brief Update tray icon visibility based on settings
     */
    void updateTrayIconVisibility();

    /**
     * @brief Initialize default settings
     */
    void initializeSettings();

    // Enhanced functionality private methods

    /**
     * @brief Create enhanced context menu with additional features
     */
    void createEnhancedContextMenu();

    /**
     * @brief Create recent files submenu
     */
    void createRecentFilesMenu();

    /**
     * @brief Create quick actions submenu
     */
    void createQuickActionsMenu();

    /**
     * @brief Create settings submenu
     */
    void createSettingsMenu();

    /**
     * @brief Update tray icon based on current status
     * @param status Current application status
     */
    void updateTrayIconForStatus(const QString& status);

    /**
     * @brief Generate status icon with overlay
     * @param baseIcon Base application icon
     * @param status Status to overlay
     * @return Generated icon with status overlay
     */
    QIcon generateStatusIcon(const QIcon& baseIcon, const QString& status);

    /**
     * @brief Handle notification type filtering
     * @param type Notification type to check
     * @return true if notification type is enabled
     */
    bool isNotificationTypeEnabled(const QString& type) const;

    /**
     * @brief Connect to application events for status updates
     */
    void connectToApplicationEvents();

    /**
     * @brief Update status information in context menu
     */
    void updateStatusInContextMenu();

    /**
     * @brief Update recent files menu with current files
     */
    void updateRecentFilesMenu();



    /**
     * @brief Create a default tray icon when application icon is not available
     * @param icon Reference to QIcon to set the default icon
     */
    void createDefaultTrayIcon(QIcon& icon);

    std::unique_ptr<SystemTrayManagerImpl> pImpl;

public:
    // Settings keys - made public for implementation class access
    static const QString SETTINGS_GROUP;
    static const QString SETTINGS_ENABLED_KEY;
    static const QString SETTINGS_MINIMIZE_TO_TRAY_KEY;
    static const QString SETTINGS_SHOW_NOTIFICATIONS_KEY;
    static const QString SETTINGS_FIRST_TIME_NOTIFICATION_SHOWN_KEY;

    // Enhanced feature settings keys
    static const QString SETTINGS_SHOW_STATUS_INDICATORS_KEY;
    static const QString SETTINGS_SHOW_RECENT_FILES_KEY;
    static const QString SETTINGS_RECENT_FILES_COUNT_KEY;
    static const QString SETTINGS_SHOW_QUICK_ACTIONS_KEY;
    static const QString SETTINGS_ENHANCED_NOTIFICATIONS_KEY;
    static const QString SETTINGS_NOTIFICATION_TYPES_KEY;
    static const QString SETTINGS_DYNAMIC_TOOLTIP_KEY;

    // Default values - made public for implementation class access
    static const bool DEFAULT_ENABLED;
    static const bool DEFAULT_MINIMIZE_TO_TRAY;
    static const bool DEFAULT_SHOW_NOTIFICATIONS;
    static const bool DEFAULT_SHOW_STATUS_INDICATORS;
    static const bool DEFAULT_SHOW_RECENT_FILES;
    static const int DEFAULT_RECENT_FILES_COUNT;
    static const bool DEFAULT_SHOW_QUICK_ACTIONS;
    static const bool DEFAULT_ENHANCED_NOTIFICATIONS;
    static const bool DEFAULT_DYNAMIC_TOOLTIP;
};
