#include "SystemTrayManager.h"
#include <QAction>
#include <QApplication>
#include <QColor>
#include <QFileInfo>
#include <QFont>
#include <QIcon>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTimer>
#include "../controller/ConfigurationManager.h"
#include "../controller/EventBus.h"
#include "../logging/SimpleLogging.h"
#include "RecentFilesManager.h"

// Private implementation class
class SystemTrayManagerImpl {
public:
    SystemTrayManagerImpl()
        : m_mainWindow(nullptr),
          m_recentFilesManager(nullptr),
          m_restoreAction(nullptr),
          m_exitAction(nullptr),
          m_openFileAction(nullptr),
          m_settingsAction(nullptr),
          m_aboutAction(nullptr),
          m_statusSeparator(nullptr),
          m_statusAction(nullptr),
          m_isInitialized(false),
          m_isEnabled(SystemTrayManager::DEFAULT_ENABLED),
          m_minimizeToTrayEnabled(SystemTrayManager::DEFAULT_MINIMIZE_TO_TRAY),
          m_showNotifications(SystemTrayManager::DEFAULT_SHOW_NOTIFICATIONS),
          m_hasShownFirstTimeNotification(false),
          m_isMainWindowHidden(false),
          m_showStatusIndicators(
              SystemTrayManager::DEFAULT_SHOW_STATUS_INDICATORS),
          m_showRecentFiles(SystemTrayManager::DEFAULT_SHOW_RECENT_FILES),
          m_recentFilesCount(SystemTrayManager::DEFAULT_RECENT_FILES_COUNT),
          m_showQuickActions(SystemTrayManager::DEFAULT_SHOW_QUICK_ACTIONS),
          m_enhancedNotifications(
              SystemTrayManager::DEFAULT_ENHANCED_NOTIFICATIONS),
          m_dynamicTooltip(SystemTrayManager::DEFAULT_DYNAMIC_TOOLTIP),
          m_logger("SystemTrayManager") {}

    // Core components
    std::unique_ptr<QSystemTrayIcon> m_trayIcon;
    std::unique_ptr<QMenu> m_contextMenu;
    QMainWindow* m_mainWindow;

    // Manager references
    RecentFilesManager* m_recentFilesManager;

    // Menu actions
    QAction* m_restoreAction;
    QAction* m_exitAction;

    // Enhanced menu components
    std::unique_ptr<QMenu> m_recentFilesMenu;
    std::unique_ptr<QMenu> m_quickActionsMenu;
    std::unique_ptr<QMenu> m_settingsMenu;

    // Enhanced menu actions
    QAction* m_openFileAction;
    QAction* m_settingsAction;
    QAction* m_aboutAction;
    QAction* m_statusSeparator;
    QAction* m_statusAction;

    // State
    bool m_isInitialized;
    bool m_isEnabled;
    bool m_minimizeToTrayEnabled;
    bool m_showNotifications;
    bool m_hasShownFirstTimeNotification;
    bool m_isMainWindowHidden;

    // Enhanced feature state
    bool m_showStatusIndicators;
    bool m_showRecentFiles;
    int m_recentFilesCount;
    bool m_showQuickActions;
    bool m_enhancedNotifications;
    QString m_notificationTypes;
    bool m_dynamicTooltip;
    QString m_currentStatus;
    QString m_currentStatusMessage;

    // Logging
    SastLogging::CategoryLogger m_logger;
};

// Static constants
const QString SystemTrayManager::SETTINGS_GROUP = "UI";
const QString SystemTrayManager::SETTINGS_ENABLED_KEY = "system_tray_enabled";
const QString SystemTrayManager::SETTINGS_MINIMIZE_TO_TRAY_KEY =
    "minimize_to_tray";
const QString SystemTrayManager::SETTINGS_SHOW_NOTIFICATIONS_KEY =
    "show_tray_notifications";
const QString SystemTrayManager::SETTINGS_FIRST_TIME_NOTIFICATION_SHOWN_KEY =
    "first_time_tray_notification_shown";

// Enhanced feature settings keys
const QString SystemTrayManager::SETTINGS_SHOW_STATUS_INDICATORS_KEY =
    "show_status_indicators";
const QString SystemTrayManager::SETTINGS_SHOW_RECENT_FILES_KEY =
    "show_recent_files";
const QString SystemTrayManager::SETTINGS_RECENT_FILES_COUNT_KEY =
    "recent_files_count";
const QString SystemTrayManager::SETTINGS_SHOW_QUICK_ACTIONS_KEY =
    "show_quick_actions";
const QString SystemTrayManager::SETTINGS_ENHANCED_NOTIFICATIONS_KEY =
    "enhanced_notifications";
const QString SystemTrayManager::SETTINGS_NOTIFICATION_TYPES_KEY =
    "notification_types";
const QString SystemTrayManager::SETTINGS_DYNAMIC_TOOLTIP_KEY =
    "dynamic_tooltip";

const bool SystemTrayManager::DEFAULT_ENABLED = true;
const bool SystemTrayManager::DEFAULT_MINIMIZE_TO_TRAY = true;
const bool SystemTrayManager::DEFAULT_SHOW_NOTIFICATIONS = true;
const bool SystemTrayManager::DEFAULT_SHOW_STATUS_INDICATORS = true;
const bool SystemTrayManager::DEFAULT_SHOW_RECENT_FILES = true;
const int SystemTrayManager::DEFAULT_RECENT_FILES_COUNT = 5;
const bool SystemTrayManager::DEFAULT_SHOW_QUICK_ACTIONS = true;
const bool SystemTrayManager::DEFAULT_ENHANCED_NOTIFICATIONS = true;
const bool SystemTrayManager::DEFAULT_DYNAMIC_TOOLTIP = true;

SystemTrayManager::SystemTrayManager(QObject* parent)
    : QObject(parent), pImpl(std::make_unique<SystemTrayManagerImpl>()) {
    pImpl->m_logger.debug("SystemTrayManager constructor called");
}

SystemTrayManager::~SystemTrayManager() {
    pImpl->m_logger.debug("SystemTrayManager destructor called");
    shutdown();
}

SystemTrayManager& SystemTrayManager::instance() {
    static SystemTrayManager instance;
    return instance;
}

bool SystemTrayManager::isSystemTrayAvailable() {
    bool available = QSystemTrayIcon::isSystemTrayAvailable();

    // Log availability status for debugging
    static bool hasLoggedAvailability = false;
    if (!hasLoggedAvailability) {
        if (available) {
            qDebug() << "SystemTrayManager: System tray is available on this "
                        "platform";
        } else {
            qDebug() << "SystemTrayManager: System tray is NOT available on "
                        "this platform";
            qDebug() << "SystemTrayManager: This may be due to:";
            qDebug() << "  - Desktop environment without system tray support";
            qDebug() << "  - System tray disabled in desktop settings";
            qDebug() << "  - Running in a headless environment";
        }
        hasLoggedAvailability = true;
    }

    return available;
}

bool SystemTrayManager::initialize(QMainWindow* mainWindow) {
    if (pImpl->m_isInitialized) {
        pImpl->m_logger.warning("SystemTrayManager already initialized");
        return true;
    }

    if (!mainWindow) {
        pImpl->m_logger.error(
            "Cannot initialize SystemTrayManager: mainWindow is null");
        return false;
    }

    pImpl->m_logger.info("Initializing SystemTrayManager...");

    // Check system tray availability
    if (!isSystemTrayAvailable()) {
        pImpl->m_logger.warning(
            "System tray is not available on this platform");
        // Don't return false - we can still function without system tray
        pImpl->m_isEnabled = false;
    }

    pImpl->m_mainWindow = mainWindow;

    // Initialize settings
    initializeSettings();
    loadSettings();

    // Create tray components if enabled and available
    if (pImpl->m_isEnabled && isSystemTrayAvailable()) {
        createTrayIcon();
        if (areEnhancedFeaturesEnabled()) {
            createEnhancedContextMenu();
        } else {
            createContextMenu();
        }
        updateTrayIconVisibility();
    }

    // Connect to application events for status updates
    connectToApplicationEvents();

    pImpl->m_isInitialized = true;
    pImpl->m_logger.info("SystemTrayManager initialized successfully");
    return true;
}

void SystemTrayManager::shutdown() {
    if (!pImpl->m_isInitialized) {
        return;
    }

    pImpl->m_logger.info("Shutting down SystemTrayManager...");

    // Save current settings
    saveSettings();

    // Hide tray icon
    if (pImpl->m_trayIcon) {
        pImpl->m_trayIcon->hide();
    }

    // Clean up resources
    pImpl->m_contextMenu.reset();
    pImpl->m_trayIcon.reset();

    pImpl->m_restoreAction = nullptr;
    pImpl->m_exitAction = nullptr;
    pImpl->m_mainWindow = nullptr;

    pImpl->m_isInitialized = false;
    pImpl->m_logger.info("SystemTrayManager shutdown complete");
}

bool SystemTrayManager::isEnabled() const {
    return pImpl->m_isEnabled && isSystemTrayAvailable();
}

void SystemTrayManager::setEnabled(bool enabled) {
    if (pImpl->m_isEnabled == enabled) {
        return;
    }

    pImpl->m_logger.info(
        QString("Setting system tray enabled: %1").arg(enabled));

    pImpl->m_isEnabled = enabled;

    if (pImpl->m_isInitialized) {
        if (enabled && isSystemTrayAvailable()) {
            if (!pImpl->m_trayIcon) {
                createTrayIcon();
                if (areEnhancedFeaturesEnabled()) {
                    createEnhancedContextMenu();
                } else {
                    createContextMenu();
                }
            }
            updateTrayIconVisibility();
        } else if (pImpl->m_trayIcon) {
            pImpl->m_trayIcon->hide();
        }
    }

    emit enabledChanged(enabled);
}

bool SystemTrayManager::isMinimizeToTrayEnabled() const {
    return pImpl->m_minimizeToTrayEnabled && isEnabled();
}

void SystemTrayManager::setMinimizeToTrayEnabled(bool enabled) {
    if (pImpl->m_minimizeToTrayEnabled == enabled) {
        return;
    }

    pImpl->m_logger.info(
        QString("Setting minimize to tray enabled: %1").arg(enabled));
    pImpl->m_minimizeToTrayEnabled = enabled;
    emit minimizeToTrayEnabledChanged(enabled);
}

void SystemTrayManager::showMainWindow() {
    if (!pImpl->m_mainWindow) {
        pImpl->m_logger.error("Cannot show main window: mainWindow is null");
        return;
    }

    pImpl->m_logger.debug("Showing main window from system tray");

    // Store current window state for better restoration
    Qt::WindowStates currentState = pImpl->m_mainWindow->windowState();

    // Restore window visibility and state
    if (pImpl->m_mainWindow->isMinimized() ||
        !pImpl->m_mainWindow->isVisible()) {
        // If window was minimized or hidden, restore to normal state
        pImpl->m_mainWindow->setWindowState(currentState &
                                            ~Qt::WindowMinimized);
        pImpl->m_mainWindow->show();
        pImpl->m_mainWindow->showNormal();
    } else {
        // Window is already visible, just bring to front
        pImpl->m_mainWindow->show();
    }

    // Bring window to front and activate (cross-platform compatible)
    pImpl->m_mainWindow->raise();
    pImpl->m_mainWindow->activateWindow();

    // On Windows, additional steps may be needed to properly bring window to
    // front
#ifdef Q_OS_WIN
    // Force window to foreground on Windows
    pImpl->m_mainWindow->setWindowState(pImpl->m_mainWindow->windowState() |
                                        Qt::WindowActive);
#endif

    pImpl->m_isMainWindowHidden = false;
    updateContextMenuState();
    emit mainWindowVisibilityChanged(true);

    pImpl->m_logger.debug("Main window restored and brought to front");
}

void SystemTrayManager::hideMainWindow(bool showNotification) {
    if (!pImpl->m_mainWindow) {
        pImpl->m_logger.error("Cannot hide main window: mainWindow is null");
        return;
    }

    // Check if system tray is available at runtime
    if (!isEnabled()) {
        pImpl->m_logger.debug(
            "System tray not enabled or available, performing normal minimize");
        pImpl->m_mainWindow->showMinimized();
        return;
    }

    // Double-check system tray availability at runtime
    if (!isSystemTrayAvailable()) {
        pImpl->m_logger.warning(
            "System tray became unavailable at runtime, falling back to normal "
            "minimize");
        pImpl->m_mainWindow->showMinimized();
        return;
    }

    pImpl->m_logger.debug("Hiding main window to system tray");

    // Save current window geometry for restoration
    if (pImpl->m_mainWindow->isVisible() &&
        !pImpl->m_mainWindow->isMinimized()) {
        // Window geometry is automatically saved by Qt's QSettings integration
        pImpl->m_logger.debug(
            "Window geometry will be preserved for restoration");
    }

    // Hide the window completely (not just minimize)
    pImpl->m_mainWindow->hide();
    pImpl->m_isMainWindowHidden = true;

    // Update context menu state
    updateContextMenuState();

    // Show first-time notification if needed
    if (showNotification && pImpl->m_showNotifications &&
        !pImpl->m_hasShownFirstTimeNotification) {
        showFirstTimeNotification();
    }

    emit mainWindowVisibilityChanged(false);

    pImpl->m_logger.debug("Main window hidden to system tray");
}

bool SystemTrayManager::isMainWindowHidden() const {
    return pImpl->m_isMainWindowHidden;
}

void SystemTrayManager::requestApplicationExit() {
    pImpl->m_logger.info("Application exit requested from system tray");
    emit applicationExitRequested();
}

bool SystemTrayManager::handleMainWindowCloseEvent() {
    if (!isMinimizeToTrayEnabled()) {
        pImpl->m_logger.debug(
            "Minimize to tray disabled, allowing normal close");
        return false;  // Allow normal close
    }

    pImpl->m_logger.debug(
        "Handling main window close event - minimizing to tray");
    hideMainWindow(true);
    return true;  // Ignore close event
}

void SystemTrayManager::createTrayIcon() {
    if (pImpl->m_trayIcon) {
        return;
    }

    // Final check before creating tray icon
    if (!isSystemTrayAvailable()) {
        pImpl->m_logger.error(
            "Cannot create tray icon: system tray is not available");
        return;
    }

    pImpl->m_logger.debug("Creating system tray icon");

    try {
        pImpl->m_trayIcon = std::make_unique<QSystemTrayIcon>(this);

        // Set application icon with proper sizing for different platforms
        QIcon icon(":/images/icon");
        if (icon.isNull()) {
            pImpl->m_logger.warning(
                "Could not load application icon for system tray");
            // Use default icon as fallback
            icon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);

            if (icon.isNull()) {
                pImpl->m_logger.error(
                    "Could not load fallback icon for system tray");
                // Create a simple default icon with proper sizing
                createDefaultTrayIcon(icon);
            }
        }

        // Ensure icon has appropriate sizes for different platforms
        // Windows: 16x16, X11: 22x22, macOS: varies
        if (!icon.isNull()) {
            QSize iconSize = QSize(16, 16);
#ifdef Q_OS_LINUX
            iconSize = QSize(22, 22);
#endif
            // Qt will automatically scale the icon as needed
            pImpl->m_logger.debug(
                QString("Setting tray icon with preferred size: %1x%2")
                    .arg(iconSize.width())
                    .arg(iconSize.height()));
        }

        pImpl->m_trayIcon->setIcon(icon);
        pImpl->m_trayIcon->setToolTip("SAST Readium - PDF Reader");

        // Connect signals
        connect(pImpl->m_trayIcon.get(), &QSystemTrayIcon::activated, this,
                &SystemTrayManager::onTrayIconActivated);

        pImpl->m_logger.debug("System tray icon created successfully");

    } catch (const std::exception& e) {
        pImpl->m_logger.error(
            QString("Failed to create system tray icon: %1").arg(e.what()));
        pImpl->m_trayIcon.reset();
    }
}

void SystemTrayManager::createContextMenu() {
    if (pImpl->m_contextMenu || !pImpl->m_trayIcon) {
        return;
    }

    pImpl->m_logger.debug("Creating system tray context menu");

    pImpl->m_contextMenu = std::make_unique<QMenu>();

    // Create restore action
    pImpl->m_restoreAction =
        pImpl->m_contextMenu->addAction("&Show SAST Readium");
    pImpl->m_restoreAction->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_TitleBarMaxButton));
    pImpl->m_restoreAction->setToolTip("Restore the main application window");
    connect(pImpl->m_restoreAction, &QAction::triggered, this,
            &SystemTrayManager::onRestoreAction);

    // Add separator
    pImpl->m_contextMenu->addSeparator();

    // Create exit action
    pImpl->m_exitAction = pImpl->m_contextMenu->addAction("E&xit");
    pImpl->m_exitAction->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    pImpl->m_exitAction->setToolTip("Exit SAST Readium completely");
    connect(pImpl->m_exitAction, &QAction::triggered, this,
            &SystemTrayManager::onExitAction);

    // Set context menu
    pImpl->m_trayIcon->setContextMenu(pImpl->m_contextMenu.get());

    pImpl->m_logger.debug(
        "System tray context menu created with restore and exit actions");

    // Update initial menu state
    updateContextMenuState();
}

void SystemTrayManager::updateContextMenuState() {
    if (!pImpl->m_contextMenu || !pImpl->m_restoreAction) {
        return;
    }

    // Update restore action text and state based on window visibility
    if (pImpl->m_isMainWindowHidden) {
        pImpl->m_restoreAction->setText("&Show SAST Readium");
        pImpl->m_restoreAction->setEnabled(true);
    } else {
        pImpl->m_restoreAction->setText("&Hide to Tray");
        pImpl->m_restoreAction->setEnabled(true);
    }
}

void SystemTrayManager::initializeSettings() {
    // Settings are initialized through ConfigurationManager defaults
    // This method can be used for any additional initialization if needed
    pImpl->m_logger.debug("Initializing SystemTrayManager settings");
}

void SystemTrayManager::loadSettings() {
    ConfigurationManager& config = ConfigurationManager::instance();

    pImpl->m_logger.debug("Loading SystemTrayManager settings");

    // Load basic settings
    pImpl->m_isEnabled =
        config
            .getValue(SETTINGS_GROUP + "/" + SETTINGS_ENABLED_KEY,
                      DEFAULT_ENABLED)
            .toBool();
    pImpl->m_minimizeToTrayEnabled =
        config
            .getValue(SETTINGS_GROUP + "/" + SETTINGS_MINIMIZE_TO_TRAY_KEY,
                      DEFAULT_MINIMIZE_TO_TRAY)
            .toBool();
    pImpl->m_showNotifications =
        config
            .getValue(SETTINGS_GROUP + "/" + SETTINGS_SHOW_NOTIFICATIONS_KEY,
                      DEFAULT_SHOW_NOTIFICATIONS)
            .toBool();
    pImpl->m_hasShownFirstTimeNotification =
        config
            .getValue(SETTINGS_GROUP + "/" +
                          SETTINGS_FIRST_TIME_NOTIFICATION_SHOWN_KEY,
                      false)
            .toBool();

    // Load enhanced feature settings
    pImpl->m_showStatusIndicators =
        config
            .getValue(
                SETTINGS_GROUP + "/" + SETTINGS_SHOW_STATUS_INDICATORS_KEY,
                DEFAULT_SHOW_STATUS_INDICATORS)
            .toBool();
    pImpl->m_showRecentFiles =
        config
            .getValue(SETTINGS_GROUP + "/" + SETTINGS_SHOW_RECENT_FILES_KEY,
                      DEFAULT_SHOW_RECENT_FILES)
            .toBool();
    pImpl->m_recentFilesCount =
        config
            .getValue(SETTINGS_GROUP + "/" + SETTINGS_RECENT_FILES_COUNT_KEY,
                      DEFAULT_RECENT_FILES_COUNT)
            .toInt();
    pImpl->m_showQuickActions =
        config
            .getValue(SETTINGS_GROUP + "/" + SETTINGS_SHOW_QUICK_ACTIONS_KEY,
                      DEFAULT_SHOW_QUICK_ACTIONS)
            .toBool();
    pImpl->m_enhancedNotifications =
        config
            .getValue(
                SETTINGS_GROUP + "/" + SETTINGS_ENHANCED_NOTIFICATIONS_KEY,
                DEFAULT_ENHANCED_NOTIFICATIONS)
            .toBool();
    pImpl->m_notificationTypes =
        config
            .getValue(SETTINGS_GROUP + "/" + SETTINGS_NOTIFICATION_TYPES_KEY,
                      "document,status,error")
            .toString();
    pImpl->m_dynamicTooltip =
        config
            .getValue(SETTINGS_GROUP + "/" + SETTINGS_DYNAMIC_TOOLTIP_KEY,
                      DEFAULT_DYNAMIC_TOOLTIP)
            .toBool();

    pImpl->m_logger.debug(
        QString("Settings loaded - enabled: %1, minimizeToTray: %2, "
                "showNotifications: %3, enhanced features: %4")
            .arg(pImpl->m_isEnabled)
            .arg(pImpl->m_minimizeToTrayEnabled)
            .arg(pImpl->m_showNotifications)
            .arg(areEnhancedFeaturesEnabled()));
}

void SystemTrayManager::saveSettings() {
    ConfigurationManager& config = ConfigurationManager::instance();

    pImpl->m_logger.debug("Saving SystemTrayManager settings");

    // Save basic settings
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_ENABLED_KEY,
                    pImpl->m_isEnabled);
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_MINIMIZE_TO_TRAY_KEY,
                    pImpl->m_minimizeToTrayEnabled);
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_SHOW_NOTIFICATIONS_KEY,
                    pImpl->m_showNotifications);
    config.setValue(
        SETTINGS_GROUP + "/" + SETTINGS_FIRST_TIME_NOTIFICATION_SHOWN_KEY,
        pImpl->m_hasShownFirstTimeNotification);

    // Save enhanced feature settings
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_SHOW_STATUS_INDICATORS_KEY,
                    pImpl->m_showStatusIndicators);
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_SHOW_RECENT_FILES_KEY,
                    pImpl->m_showRecentFiles);
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_RECENT_FILES_COUNT_KEY,
                    pImpl->m_recentFilesCount);
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_SHOW_QUICK_ACTIONS_KEY,
                    pImpl->m_showQuickActions);
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_ENHANCED_NOTIFICATIONS_KEY,
                    pImpl->m_enhancedNotifications);
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_NOTIFICATION_TYPES_KEY,
                    pImpl->m_notificationTypes);
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_DYNAMIC_TOOLTIP_KEY,
                    pImpl->m_dynamicTooltip);
}

void SystemTrayManager::applySettingsChange(const QString& settingsGroup,
                                            const QString& key,
                                            const QVariant& value) {
    // Only handle our settings group
    if (settingsGroup != SETTINGS_GROUP) {
        return;
    }

    pImpl->m_logger.debug(QString("Applying settings change: %1/%2 = %3")
                              .arg(settingsGroup)
                              .arg(key)
                              .arg(value.toString()));

    bool oldEnabled = pImpl->m_isEnabled;
    bool oldMinimizeToTray = pImpl->m_minimizeToTrayEnabled;
    bool oldShowNotifications = pImpl->m_showNotifications;

    // Update the appropriate setting
    if (key == SETTINGS_ENABLED_KEY) {
        setEnabled(value.toBool());
    } else if (key == SETTINGS_MINIMIZE_TO_TRAY_KEY) {
        setMinimizeToTrayEnabled(value.toBool());
    } else if (key == SETTINGS_SHOW_NOTIFICATIONS_KEY) {
        pImpl->m_showNotifications = value.toBool();
        emit showNotificationsChanged(pImpl->m_showNotifications);
    } else if (key == SETTINGS_FIRST_TIME_NOTIFICATION_SHOWN_KEY) {
        pImpl->m_hasShownFirstTimeNotification = value.toBool();
    }

    // Log changes
    if (oldEnabled != pImpl->m_isEnabled) {
        pImpl->m_logger.info(QString("System tray enabled changed: %1 -> %2")
                                 .arg(oldEnabled)
                                 .arg(pImpl->m_isEnabled));
    }
    if (oldMinimizeToTray != pImpl->m_minimizeToTrayEnabled) {
        pImpl->m_logger.info(QString("Minimize to tray changed: %1 -> %2")
                                 .arg(oldMinimizeToTray)
                                 .arg(pImpl->m_minimizeToTrayEnabled));
    }
    if (oldShowNotifications != pImpl->m_showNotifications) {
        pImpl->m_logger.info(QString("Show notifications changed: %1 -> %2")
                                 .arg(oldShowNotifications)
                                 .arg(pImpl->m_showNotifications));
    }
}

void SystemTrayManager::updateTrayIconVisibility() {
    if (!pImpl->m_trayIcon) {
        return;
    }

    if (pImpl->m_isEnabled && isSystemTrayAvailable()) {
        pImpl->m_logger.debug("Showing system tray icon");
        pImpl->m_trayIcon->show();
    } else {
        pImpl->m_logger.debug("Hiding system tray icon");
        pImpl->m_trayIcon->hide();
    }
}

void SystemTrayManager::showFirstTimeNotification() {
    if (!pImpl->m_trayIcon || !pImpl->m_showNotifications) {
        return;
    }

    pImpl->m_logger.info("Showing first-time system tray notification");

    QString title = "SAST Readium - Minimized to Tray";
    QString message =
        "The application is now running in the system tray.\n\n"
        "• Left-click the tray icon to restore the window\n"
        "• Double-click to always show the window\n"
        "• Right-click for menu options\n"
        "• Use the tray menu to exit the application";

    // Show notification for 8 seconds to give users time to read the
    // instructions
    pImpl->m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information,
                                   8000);

    // Mark that we've shown the first-time notification
    pImpl->m_hasShownFirstTimeNotification = true;

    // Save this setting immediately so it persists
    ConfigurationManager& config = ConfigurationManager::instance();
    config.setValue(
        SETTINGS_GROUP + "/" + SETTINGS_FIRST_TIME_NOTIFICATION_SHOWN_KEY,
        true);
}

void SystemTrayManager::onTrayIconActivated(
    QSystemTrayIcon::ActivationReason reason) {
    QString reasonStr;
    switch (reason) {
        case QSystemTrayIcon::Trigger:
            reasonStr = "Left Click";
            break;
        case QSystemTrayIcon::DoubleClick:
            reasonStr = "Double Click";
            break;
        case QSystemTrayIcon::MiddleClick:
            reasonStr = "Middle Click";
            break;
        case QSystemTrayIcon::Context:
            reasonStr = "Right Click (Context Menu)";
            break;
        default:
            reasonStr = "Unknown";
            break;
    }

    pImpl->m_logger.debug(QString("Tray icon activated: %1 (reason: %2)")
                              .arg(reasonStr)
                              .arg(static_cast<int>(reason)));

    switch (reason) {
        case QSystemTrayIcon::Trigger:
            // Left click - toggle window visibility
            if (pImpl->m_isMainWindowHidden) {
                showMainWindow();
            } else {
                // Only hide if minimize to tray is enabled
                if (isMinimizeToTrayEnabled()) {
                    hideMainWindow(false);
                } else {
                    showMainWindow();  // Bring to front if already visible
                }
            }
            break;

        case QSystemTrayIcon::DoubleClick:
            // Double click - always show window
            showMainWindow();
            break;

        case QSystemTrayIcon::MiddleClick:
            // Middle click - toggle window visibility
            if (pImpl->m_isMainWindowHidden) {
                showMainWindow();
            } else {
                hideMainWindow(false);
            }
            break;

        case QSystemTrayIcon::Context:
            // Right click - context menu is shown automatically
            pImpl->m_logger.debug("Context menu will be shown automatically");
            break;

        default:
            pImpl->m_logger.debug("Unhandled tray icon activation reason");
            break;
    }
}

void SystemTrayManager::onRestoreAction() {
    pImpl->m_logger.debug("Restore/Hide action triggered from tray menu");

    if (pImpl->m_isMainWindowHidden) {
        showMainWindow();
    } else {
        hideMainWindow(false);  // Don't show notification when manually hiding
    }
}

void SystemTrayManager::onExitAction() {
    pImpl->m_logger.debug("Exit action triggered from tray menu");
    requestApplicationExit();
}

void SystemTrayManager::createDefaultTrayIcon(QIcon& icon) {
    pImpl->m_logger.debug("Creating default tray icon");

    // Create a simple colored icon as fallback
    QPixmap pixmap(16, 16);
    pixmap.fill(QColor(70, 130, 180));  // Steel blue color

    // Add a simple "R" for Readium
    QPainter painter(&pixmap);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(pixmap.rect(), Qt::AlignCenter, "R");
    painter.end();

    icon = QIcon(pixmap);

    // Add larger size for high-DPI displays
    QPixmap pixmap22(22, 22);
    pixmap22.fill(QColor(70, 130, 180));
    QPainter painter22(&pixmap22);
    painter22.setPen(Qt::white);
    painter22.setFont(QFont("Arial", 14, QFont::Bold));
    painter22.drawText(pixmap22.rect(), Qt::AlignCenter, "R");
    painter22.end();

    icon.addPixmap(pixmap22);
}

void SystemTrayManager::checkSystemTrayAvailability() {
    bool currentlyAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    static bool lastKnownAvailability = currentlyAvailable;

    // Check if availability status has changed
    if (currentlyAvailable != lastKnownAvailability) {
        pImpl->m_logger.info(
            QString("System tray availability changed: %1 -> %2")
                .arg(lastKnownAvailability ? "available" : "unavailable")
                .arg(currentlyAvailable ? "available" : "unavailable"));

        if (currentlyAvailable && pImpl->m_isEnabled) {
            // System tray became available - create tray icon if we don't have
            // one
            if (!pImpl->m_trayIcon) {
                pImpl->m_logger.info(
                    "System tray became available - creating tray icon");
                createTrayIcon();
                createContextMenu();
                updateTrayIconVisibility();
            }
        } else if (!currentlyAvailable && pImpl->m_trayIcon) {
            // System tray became unavailable - handle gracefully
            pImpl->m_logger.warning(
                "System tray became unavailable - hiding tray icon");
            pImpl->m_trayIcon->hide();

            // If main window is hidden, show it since tray is no longer
            // available
            if (pImpl->m_isMainWindowHidden) {
                pImpl->m_logger.info(
                    "Restoring main window since system tray is no longer "
                    "available");
                showMainWindow();
            }
        }

        lastKnownAvailability = currentlyAvailable;
    }
}

// Enhanced functionality implementations

void SystemTrayManager::setApplicationStatus(const QString& status,
                                             const QString& message) {
    if (pImpl->m_currentStatus == status &&
        pImpl->m_currentStatusMessage == message) {
        return;
    }

    pImpl->m_logger.debug(
        QString("Setting application status: %1 - %2").arg(status, message));

    pImpl->m_currentStatus = status;
    pImpl->m_currentStatusMessage = message;

    // Update tray icon if status indicators are enabled
    if (pImpl->m_showStatusIndicators && pImpl->m_trayIcon) {
        updateTrayIconForStatus(status);
    }

    // Update dynamic tooltip if enabled
    if (pImpl->m_dynamicTooltip) {
        updateDynamicTooltip();
    }

    // Update status in context menu if enabled
    updateStatusInContextMenu();

    emit applicationStatusChanged(status, message);
}

void SystemTrayManager::showNotification(const QString& title,
                                         const QString& message,
                                         const QString& type, int timeout) {
    if (!pImpl->m_trayIcon || !pImpl->m_enhancedNotifications) {
        return;
    }

    // Check if this notification type is enabled
    if (!isNotificationTypeEnabled(type)) {
        pImpl->m_logger.debug(
            QString("Notification type '%1' is disabled, skipping").arg(type));
        return;
    }

    pImpl->m_logger.info(QString("Showing notification: %1 - %2 (type: %3)")
                             .arg(title, message, type));

    // Determine notification icon based on type
    QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information;
    if (type == "warning") {
        icon = QSystemTrayIcon::Warning;
    } else if (type == "error") {
        icon = QSystemTrayIcon::Critical;
    }

    pImpl->m_trayIcon->showMessage(title, message, icon, timeout);
    emit notificationShown(title, message, type);
}

void SystemTrayManager::updateDynamicTooltip(const QString& tooltip) {
    if (!pImpl->m_trayIcon || !pImpl->m_dynamicTooltip) {
        return;
    }

    QString newTooltip;
    if (!tooltip.isEmpty()) {
        newTooltip = tooltip;
    } else {
        // Generate dynamic tooltip based on current state
        newTooltip = "SAST Readium - PDF Reader";

        if (!pImpl->m_currentStatusMessage.isEmpty()) {
            newTooltip +=
                QString("\nStatus: %1").arg(pImpl->m_currentStatusMessage);
        } else if (pImpl->m_currentStatus != "idle") {
            newTooltip += QString("\nStatus: %1").arg(pImpl->m_currentStatus);
        }

        if (pImpl->m_isMainWindowHidden) {
            newTooltip += "\n(Running in background)";
        }
    }

    pImpl->m_trayIcon->setToolTip(newTooltip);
    pImpl->m_logger.debug(
        QString("Updated dynamic tooltip: %1").arg(newTooltip));
}

QString SystemTrayManager::currentApplicationStatus() const {
    return pImpl->m_currentStatus;
}

bool SystemTrayManager::areEnhancedFeaturesEnabled() const {
    return pImpl->m_showStatusIndicators || pImpl->m_showRecentFiles ||
           pImpl->m_showQuickActions || pImpl->m_enhancedNotifications ||
           pImpl->m_dynamicTooltip;
}

void SystemTrayManager::updateTrayIconForStatus(const QString& status) {
    if (!pImpl->m_trayIcon) {
        return;
    }

    // Get base application icon
    QIcon baseIcon(":/images/icon");
    if (baseIcon.isNull()) {
        baseIcon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
    }

    // Generate status icon with overlay
    QIcon statusIcon = generateStatusIcon(baseIcon, status);
    pImpl->m_trayIcon->setIcon(statusIcon);

    pImpl->m_logger.debug(
        QString("Updated tray icon for status: %1").arg(status));
}

QIcon SystemTrayManager::generateStatusIcon(const QIcon& baseIcon,
                                            const QString& status) {
    if (status == "idle" || !pImpl->m_showStatusIndicators) {
        return baseIcon;
    }

    // Get base pixmap
    QPixmap basePixmap = baseIcon.pixmap(22, 22);  // Standard tray icon size
    if (basePixmap.isNull()) {
        return baseIcon;
    }

    // Create a copy for modification
    QPixmap statusPixmap = basePixmap.copy();
    QPainter painter(&statusPixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Define status colors and overlay positions
    QColor overlayColor;
    QPoint overlayPos(16, 16);  // Bottom-right corner
    int overlaySize = 8;

    if (status == "processing") {
        overlayColor = QColor(255, 165, 0);  // Orange
    } else if (status == "error") {
        overlayColor = QColor(220, 53, 69);  // Red
    } else if (status == "success") {
        overlayColor = QColor(40, 167, 69);  // Green
    } else if (status == "warning") {
        overlayColor = QColor(255, 193, 7);  // Yellow
    } else {
        overlayColor = QColor(108, 117, 125);  // Gray for unknown status
    }

    // Draw status overlay circle
    painter.setBrush(QBrush(overlayColor));
    painter.setPen(QPen(Qt::white, 1));
    painter.drawEllipse(overlayPos.x() - overlaySize / 2,
                        overlayPos.y() - overlaySize / 2, overlaySize,
                        overlaySize);

    painter.end();

    return QIcon(statusPixmap);
}

bool SystemTrayManager::isNotificationTypeEnabled(const QString& type) const {
    if (!pImpl->m_enhancedNotifications) {
        return false;
    }

    // Parse notification types setting
    QStringList enabledTypes =
        pImpl->m_notificationTypes.split(',', Qt::SkipEmptyParts);
    for (QString& enabledType : enabledTypes) {
        enabledType = enabledType.trimmed();
    }

    return enabledTypes.contains(type) || enabledTypes.contains("all");
}

void SystemTrayManager::connectToApplicationEvents() {
    if (!pImpl->m_enhancedNotifications && !pImpl->m_showStatusIndicators) {
        pImpl->m_logger.debug(
            "Enhanced features disabled, skipping event connections");
        return;
    }

    EventBus& eventBus = EventBus::instance();

    // Connect to document events
    eventBus.subscribe(AppEvents::DOCUMENT_OPENED, this, [this](Event* event) {
        if (pImpl->m_showStatusIndicators) {
            setApplicationStatus("processing", "Opening document...");
        }
        if (pImpl->m_enhancedNotifications &&
            isNotificationTypeEnabled("document")) {
            QString fileName = event->data().toString();
            if (!fileName.isEmpty()) {
                QFileInfo fileInfo(fileName);
                showNotification("Document Opened",
                                 QString("Opened: %1").arg(fileInfo.fileName()),
                                 "info", 3000);
            }
        }
        // Reset to idle after a short delay
        QTimer::singleShot(2000, this,
                           [this]() { setApplicationStatus("idle"); });
    });

    eventBus.subscribe(
        AppEvents::DOCUMENT_CLOSED, this, [this](Event* /*event*/) {
            if (pImpl->m_showStatusIndicators) {
                setApplicationStatus("idle", "Ready");
            }
            if (pImpl->m_enhancedNotifications &&
                isNotificationTypeEnabled("document")) {
                showNotification("Document Closed", "Document has been closed",
                                 "info", 2000);
            }
        });

    eventBus.subscribe(AppEvents::DOCUMENT_SAVED, this, [this](Event* event) {
        if (pImpl->m_showStatusIndicators) {
            setApplicationStatus("success", "Document saved");
        }
        if (pImpl->m_enhancedNotifications &&
            isNotificationTypeEnabled("document")) {
            QString fileName = event->data().toString();
            if (!fileName.isEmpty()) {
                QFileInfo fileInfo(fileName);
                showNotification("Document Saved",
                                 QString("Saved: %1").arg(fileInfo.fileName()),
                                 "info", 2000);
            }
        }
        // Reset to idle after showing success
        QTimer::singleShot(3000, this,
                           [this]() { setApplicationStatus("idle"); });
    });

    // Connect to error events (if they exist)
    eventBus.subscribe("error.occurred", this, [this](Event* event) {
        if (pImpl->m_showStatusIndicators) {
            setApplicationStatus("error", "Error occurred");
        }
        if (pImpl->m_enhancedNotifications &&
            isNotificationTypeEnabled("error")) {
            QString errorMessage = event->data().toString();
            showNotification(
                "Error",
                errorMessage.isEmpty() ? "An error occurred" : errorMessage,
                "error", 5000);
        }
        // Reset to idle after showing error
        QTimer::singleShot(5000, this,
                           [this]() { setApplicationStatus("idle"); });
    });

    // Connect to loading progress events
    eventBus.subscribe("document.loading", this, [this](Event* /*event*/) {
        if (pImpl->m_showStatusIndicators) {
            setApplicationStatus("processing", "Loading document...");
        }
    });

    pImpl->m_logger.debug(
        "Connected to application events for enhanced system tray "
        "functionality");
}

void SystemTrayManager::createEnhancedContextMenu() {
    if (pImpl->m_contextMenu || !pImpl->m_trayIcon) {
        return;
    }

    pImpl->m_logger.debug("Creating enhanced system tray context menu");

    pImpl->m_contextMenu = std::make_unique<QMenu>();

    // Create restore action (same as basic menu)
    pImpl->m_restoreAction =
        pImpl->m_contextMenu->addAction("&Show SAST Readium");
    pImpl->m_restoreAction->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_TitleBarMaxButton));
    pImpl->m_restoreAction->setToolTip("Restore the main application window");
    connect(pImpl->m_restoreAction, &QAction::triggered, this,
            &SystemTrayManager::onRestoreAction);

    // Add separator
    pImpl->m_contextMenu->addSeparator();

    // Add recent files submenu if enabled
    if (pImpl->m_showRecentFiles) {
        createRecentFilesMenu();
    }

    // Add quick actions submenu if enabled
    if (pImpl->m_showQuickActions) {
        createQuickActionsMenu();
    }

    // Add status information if status indicators are enabled
    if (pImpl->m_showStatusIndicators) {
        pImpl->m_statusSeparator = pImpl->m_contextMenu->addSeparator();
        pImpl->m_statusAction = pImpl->m_contextMenu->addAction(
            QString("Status: %1").arg(pImpl->m_currentStatus));
        pImpl->m_statusAction->setEnabled(
            false);  // Make it non-clickable, just informational
        if (!pImpl->m_currentStatusMessage.isEmpty()) {
            pImpl->m_statusAction->setToolTip(pImpl->m_currentStatusMessage);
        }
    }

    // Add settings submenu
    createSettingsMenu();

    // Add separator before exit
    pImpl->m_contextMenu->addSeparator();

    // Create exit action (same as basic menu)
    pImpl->m_exitAction = pImpl->m_contextMenu->addAction("&Exit");
    pImpl->m_exitAction->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    pImpl->m_exitAction->setToolTip("Exit the application");
    connect(pImpl->m_exitAction, &QAction::triggered, this,
            &SystemTrayManager::onExitAction);

    // Set context menu for tray icon
    pImpl->m_trayIcon->setContextMenu(pImpl->m_contextMenu.get());

    pImpl->m_logger.debug(
        "Enhanced system tray context menu created successfully");
}

void SystemTrayManager::createRecentFilesMenu() {
    if (!pImpl->m_contextMenu) {
        return;
    }

    pImpl->m_recentFilesMenu = std::make_unique<QMenu>("Recent Files");
    pImpl->m_recentFilesMenu->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView));

    // Get recent files from RecentFilesManager
    // Note: We'll need to get a reference to RecentFilesManager from
    // ApplicationController For now, create a placeholder structure

    // Add "Open File..." action
    pImpl->m_openFileAction =
        pImpl->m_recentFilesMenu->addAction("&Open File...");
    pImpl->m_openFileAction->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton));
    pImpl->m_openFileAction->setToolTip("Open a new document");
    connect(pImpl->m_openFileAction, &QAction::triggered, this,
            [this]() { emit quickActionTriggered("open_file"); });

    pImpl->m_recentFilesMenu->addSeparator();

    // Add placeholder for recent files (will be populated dynamically)
    QAction* noRecentFiles =
        pImpl->m_recentFilesMenu->addAction("No recent files");
    noRecentFiles->setEnabled(false);

    // Add the submenu to main context menu
    pImpl->m_contextMenu->addMenu(pImpl->m_recentFilesMenu.get());
}

void SystemTrayManager::createQuickActionsMenu() {
    if (!pImpl->m_contextMenu) {
        return;
    }

    pImpl->m_quickActionsMenu = std::make_unique<QMenu>("Quick Actions");
    pImpl->m_quickActionsMenu->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));

    // Add common quick actions
    QAction* openFileAction =
        pImpl->m_quickActionsMenu->addAction("&Open File...");
    openFileAction->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton));
    connect(openFileAction, &QAction::triggered, this,
            [this]() { emit quickActionTriggered("open_file"); });

    QAction* settingsAction =
        pImpl->m_quickActionsMenu->addAction("&Settings...");
    settingsAction->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
    connect(settingsAction, &QAction::triggered, this,
            [this]() { emit settingsDialogRequested(); });

    QAction* aboutAction = pImpl->m_quickActionsMenu->addAction("&About...");
    aboutAction->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation));
    connect(aboutAction, &QAction::triggered, this,
            [this]() { emit aboutDialogRequested(); });

    // Add the submenu to main context menu
    pImpl->m_contextMenu->addMenu(pImpl->m_quickActionsMenu.get());
}

void SystemTrayManager::createSettingsMenu() {
    if (!pImpl->m_contextMenu) {
        return;
    }

    pImpl->m_settingsMenu = std::make_unique<QMenu>("Settings");
    pImpl->m_settingsMenu->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));

    // Add tray-specific settings
    QAction* statusIndicatorsAction =
        pImpl->m_settingsMenu->addAction("Show Status Indicators");
    statusIndicatorsAction->setCheckable(true);
    statusIndicatorsAction->setChecked(pImpl->m_showStatusIndicators);
    connect(statusIndicatorsAction, &QAction::toggled, this,
            [this](bool checked) {
                pImpl->m_showStatusIndicators = checked;
                saveSettings();
                emit enhancedFeaturesChanged(areEnhancedFeaturesEnabled());
            });

    QAction* recentFilesAction =
        pImpl->m_settingsMenu->addAction("Show Recent Files");
    recentFilesAction->setCheckable(true);
    recentFilesAction->setChecked(pImpl->m_showRecentFiles);
    connect(recentFilesAction, &QAction::toggled, this, [this](bool checked) {
        pImpl->m_showRecentFiles = checked;
        saveSettings();
        emit enhancedFeaturesChanged(areEnhancedFeaturesEnabled());
    });

    QAction* notificationsAction =
        pImpl->m_settingsMenu->addAction("Enhanced Notifications");
    notificationsAction->setCheckable(true);
    notificationsAction->setChecked(pImpl->m_enhancedNotifications);
    connect(notificationsAction, &QAction::toggled, this, [this](bool checked) {
        pImpl->m_enhancedNotifications = checked;
        saveSettings();
        emit enhancedFeaturesChanged(areEnhancedFeaturesEnabled());
    });

    pImpl->m_settingsMenu->addSeparator();

    // Add link to main settings
    pImpl->m_settingsAction =
        pImpl->m_settingsMenu->addAction("&Open Settings...");
    pImpl->m_settingsAction->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
    connect(pImpl->m_settingsAction, &QAction::triggered, this,
            [this]() { emit settingsDialogRequested(); });

    // Add the submenu to main context menu
    pImpl->m_contextMenu->addMenu(pImpl->m_settingsMenu.get());
}

void SystemTrayManager::updateStatusInContextMenu() {
    if (!pImpl->m_statusAction || !pImpl->m_showStatusIndicators) {
        return;
    }

    QString statusText = QString("Status: %1").arg(pImpl->m_currentStatus);
    if (!pImpl->m_currentStatusMessage.isEmpty()) {
        statusText += QString(" - %1").arg(pImpl->m_currentStatusMessage);
    }

    pImpl->m_statusAction->setText(statusText);
    pImpl->m_statusAction->setToolTip(pImpl->m_currentStatusMessage.isEmpty()
                                          ? pImpl->m_currentStatus
                                          : pImpl->m_currentStatusMessage);

    pImpl->m_logger.debug(
        QString("Updated status in context menu: %1").arg(statusText));
}

void SystemTrayManager::setNotificationTypes(const QString& types) {
    if (pImpl->m_notificationTypes == types) {
        return;
    }

    pImpl->m_logger.info(QString("Setting notification types: %1").arg(types));
    pImpl->m_notificationTypes = types;

    // Save to configuration
    ConfigurationManager& config = ConfigurationManager::instance();
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_NOTIFICATION_TYPES_KEY,
                    types);

    // Emit signal if enhanced notifications are enabled
    if (pImpl->m_enhancedNotifications) {
        emit enhancedFeaturesChanged(areEnhancedFeaturesEnabled());
    }
}

QString SystemTrayManager::getNotificationTypes() const {
    return pImpl->m_notificationTypes;
}

void SystemTrayManager::connectToRecentFilesManager(
    RecentFilesManager* recentFilesManager) {
    if (!recentFilesManager) {
        pImpl->m_logger.warning("Cannot connect to null RecentFilesManager");
        return;
    }

    pImpl->m_recentFilesManager = recentFilesManager;

    // Connect to recent files changes
    connect(pImpl->m_recentFilesManager,
            &RecentFilesManager::recentFilesChanged, this,
            &SystemTrayManager::updateRecentFilesMenu);

    connect(pImpl->m_recentFilesManager, &RecentFilesManager::recentFileAdded,
            this, &SystemTrayManager::updateRecentFilesMenu);

    connect(pImpl->m_recentFilesManager, &RecentFilesManager::recentFileRemoved,
            this, &SystemTrayManager::updateRecentFilesMenu);

    // Initial update
    updateRecentFilesMenu();

    pImpl->m_logger.debug("Connected to RecentFilesManager");
}

void SystemTrayManager::updateRecentFilesMenu() {
    if (!pImpl->m_recentFilesMenu || !pImpl->m_recentFilesManager ||
        !pImpl->m_showRecentFiles) {
        return;
    }

    pImpl->m_logger.debug("Updating recent files menu");

    // Clear existing recent file actions (keep "Open File..." and separator)
    QList<QAction*> actions = pImpl->m_recentFilesMenu->actions();
    for (int i = actions.size() - 1; i >= 0; --i) {
        QAction* action = actions[i];
        if (action != pImpl->m_openFileAction && !action->isSeparator()) {
            pImpl->m_recentFilesMenu->removeAction(action);
            delete action;
        }
    }

    // Get recent files
    QStringList recentFiles = pImpl->m_recentFilesManager->getRecentFilePaths();

    if (recentFiles.isEmpty()) {
        // Add "No recent files" placeholder
        QAction* noFilesAction =
            pImpl->m_recentFilesMenu->addAction("No recent files");
        noFilesAction->setEnabled(false);
    } else {
        // Add recent files (limit to configured count)
        int count = qMin(recentFiles.size(), pImpl->m_recentFilesCount);
        for (int i = 0; i < count; ++i) {
            const QString& filePath = recentFiles[i];
            QFileInfo fileInfo(filePath);

            QString displayName = fileInfo.fileName();
            if (displayName.length() > 30) {
                displayName = displayName.left(27) + "...";
            }

            QAction* fileAction =
                pImpl->m_recentFilesMenu->addAction(displayName);
            fileAction->setToolTip(filePath);
            fileAction->setIcon(
                QApplication::style()->standardIcon(QStyle::SP_FileIcon));

            // Connect to signal emission
            connect(fileAction, &QAction::triggered, this,
                    [this, filePath]() { emit recentFileRequested(filePath); });
        }
    }

    pImpl->m_logger.debug(QString("Updated recent files menu with %1 files")
                              .arg(recentFiles.size()));
}
