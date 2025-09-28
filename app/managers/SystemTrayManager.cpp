#include "SystemTrayManager.h"
#include <QMainWindow>
#include <QApplication>
#include <QIcon>
#include <QMessageBox>
#include <QPixmap>
#include <QPainter>
#include <QStyle>
#include <QFont>
#include <QColor>
#include <QFileInfo>
#include <QTimer>
#include "../controller/ConfigurationManager.h"
#include "../controller/EventBus.h"
#include "RecentFilesManager.h"

// Static constants
const QString SystemTrayManager::SETTINGS_GROUP = "UI";
const QString SystemTrayManager::SETTINGS_ENABLED_KEY = "system_tray_enabled";
const QString SystemTrayManager::SETTINGS_MINIMIZE_TO_TRAY_KEY = "minimize_to_tray";
const QString SystemTrayManager::SETTINGS_SHOW_NOTIFICATIONS_KEY = "show_tray_notifications";
const QString SystemTrayManager::SETTINGS_FIRST_TIME_NOTIFICATION_SHOWN_KEY = "first_time_tray_notification_shown";

// Enhanced feature settings keys
const QString SystemTrayManager::SETTINGS_SHOW_STATUS_INDICATORS_KEY = "show_status_indicators";
const QString SystemTrayManager::SETTINGS_SHOW_RECENT_FILES_KEY = "show_recent_files";
const QString SystemTrayManager::SETTINGS_RECENT_FILES_COUNT_KEY = "recent_files_count";
const QString SystemTrayManager::SETTINGS_SHOW_QUICK_ACTIONS_KEY = "show_quick_actions";
const QString SystemTrayManager::SETTINGS_ENHANCED_NOTIFICATIONS_KEY = "enhanced_notifications";
const QString SystemTrayManager::SETTINGS_NOTIFICATION_TYPES_KEY = "notification_types";
const QString SystemTrayManager::SETTINGS_DYNAMIC_TOOLTIP_KEY = "dynamic_tooltip";

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
    : QObject(parent)
    , m_mainWindow(nullptr)
    , m_recentFilesManager(nullptr)
    , m_restoreAction(nullptr)
    , m_exitAction(nullptr)
    , m_openFileAction(nullptr)
    , m_settingsAction(nullptr)
    , m_aboutAction(nullptr)
    , m_statusSeparator(nullptr)
    , m_statusAction(nullptr)
    , m_isInitialized(false)
    , m_isEnabled(DEFAULT_ENABLED)
    , m_minimizeToTrayEnabled(DEFAULT_MINIMIZE_TO_TRAY)
    , m_showNotifications(DEFAULT_SHOW_NOTIFICATIONS)
    , m_hasShownFirstTimeNotification(false)
    , m_isMainWindowHidden(false)
    , m_showStatusIndicators(DEFAULT_SHOW_STATUS_INDICATORS)
    , m_showRecentFiles(DEFAULT_SHOW_RECENT_FILES)
    , m_recentFilesCount(DEFAULT_RECENT_FILES_COUNT)
    , m_showQuickActions(DEFAULT_SHOW_QUICK_ACTIONS)
    , m_enhancedNotifications(DEFAULT_ENHANCED_NOTIFICATIONS)
    , m_notificationTypes("document,status,error")
    , m_dynamicTooltip(DEFAULT_DYNAMIC_TOOLTIP)
    , m_currentStatus("idle")
    , m_currentStatusMessage("")
    , m_logger("SystemTrayManager")
{
    m_logger.debug("SystemTrayManager constructor called");
}

SystemTrayManager::~SystemTrayManager() {
    m_logger.debug("SystemTrayManager destructor called");
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
            qDebug() << "SystemTrayManager: System tray is available on this platform";
        } else {
            qDebug() << "SystemTrayManager: System tray is NOT available on this platform";
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
    if (m_isInitialized) {
        m_logger.warning("SystemTrayManager already initialized");
        return true;
    }

    if (!mainWindow) {
        m_logger.error("Cannot initialize SystemTrayManager: mainWindow is null");
        return false;
    }

    m_logger.info("Initializing SystemTrayManager...");

    // Check system tray availability
    if (!isSystemTrayAvailable()) {
        m_logger.warning("System tray is not available on this platform");
        // Don't return false - we can still function without system tray
        m_isEnabled = false;
    }

    m_mainWindow = mainWindow;
    
    // Initialize settings
    initializeSettings();
    loadSettings();

    // Create tray components if enabled and available
    if (m_isEnabled && isSystemTrayAvailable()) {
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

    m_isInitialized = true;
    m_logger.info("SystemTrayManager initialized successfully");
    return true;
}

void SystemTrayManager::shutdown() {
    if (!m_isInitialized) {
        return;
    }

    m_logger.info("Shutting down SystemTrayManager...");
    
    // Save current settings
    saveSettings();

    // Hide tray icon
    if (m_trayIcon) {
        m_trayIcon->hide();
    }

    // Clean up resources
    m_contextMenu.reset();
    m_trayIcon.reset();
    
    m_restoreAction = nullptr;
    m_exitAction = nullptr;
    m_mainWindow = nullptr;
    
    m_isInitialized = false;
    m_logger.info("SystemTrayManager shutdown complete");
}

bool SystemTrayManager::isEnabled() const {
    return m_isEnabled && isSystemTrayAvailable();
}

void SystemTrayManager::setEnabled(bool enabled) {
    if (m_isEnabled == enabled) {
        return;
    }

    m_logger.info(QString("Setting system tray enabled: %1").arg(enabled));
    
    m_isEnabled = enabled;
    
    if (m_isInitialized) {
        if (enabled && isSystemTrayAvailable()) {
            if (!m_trayIcon) {
                createTrayIcon();
                if (areEnhancedFeaturesEnabled()) {
                    createEnhancedContextMenu();
                } else {
                    createContextMenu();
                }
            }
            updateTrayIconVisibility();
        } else if (m_trayIcon) {
            m_trayIcon->hide();
        }
    }
    
    emit enabledChanged(enabled);
}

bool SystemTrayManager::isMinimizeToTrayEnabled() const {
    return m_minimizeToTrayEnabled && isEnabled();
}

void SystemTrayManager::setMinimizeToTrayEnabled(bool enabled) {
    if (m_minimizeToTrayEnabled == enabled) {
        return;
    }

    m_logger.info(QString("Setting minimize to tray enabled: %1").arg(enabled));
    m_minimizeToTrayEnabled = enabled;
    emit minimizeToTrayEnabledChanged(enabled);
}

void SystemTrayManager::showMainWindow() {
    if (!m_mainWindow) {
        m_logger.error("Cannot show main window: mainWindow is null");
        return;
    }

    m_logger.debug("Showing main window from system tray");

    // Store current window state for better restoration
    Qt::WindowStates currentState = m_mainWindow->windowState();

    // Restore window visibility and state
    if (m_mainWindow->isMinimized() || !m_mainWindow->isVisible()) {
        // If window was minimized or hidden, restore to normal state
        m_mainWindow->setWindowState(currentState & ~Qt::WindowMinimized);
        m_mainWindow->show();
        m_mainWindow->showNormal();
    } else {
        // Window is already visible, just bring to front
        m_mainWindow->show();
    }

    // Bring window to front and activate (cross-platform compatible)
    m_mainWindow->raise();
    m_mainWindow->activateWindow();

    // On Windows, additional steps may be needed to properly bring window to front
#ifdef Q_OS_WIN
    // Force window to foreground on Windows
    m_mainWindow->setWindowState(m_mainWindow->windowState() | Qt::WindowActive);
#endif

    m_isMainWindowHidden = false;
    updateContextMenuState();
    emit mainWindowVisibilityChanged(true);

    m_logger.debug("Main window restored and brought to front");
}

void SystemTrayManager::hideMainWindow(bool showNotification) {
    if (!m_mainWindow) {
        m_logger.error("Cannot hide main window: mainWindow is null");
        return;
    }

    // Check if system tray is available at runtime
    if (!isEnabled()) {
        m_logger.debug("System tray not enabled or available, performing normal minimize");
        m_mainWindow->showMinimized();
        return;
    }

    // Double-check system tray availability at runtime
    if (!isSystemTrayAvailable()) {
        m_logger.warning("System tray became unavailable at runtime, falling back to normal minimize");
        m_mainWindow->showMinimized();
        return;
    }

    m_logger.debug("Hiding main window to system tray");

    // Save current window geometry for restoration
    if (m_mainWindow->isVisible() && !m_mainWindow->isMinimized()) {
        // Window geometry is automatically saved by Qt's QSettings integration
        m_logger.debug("Window geometry will be preserved for restoration");
    }

    // Hide the window completely (not just minimize)
    m_mainWindow->hide();
    m_isMainWindowHidden = true;

    // Update context menu state
    updateContextMenuState();

    // Show first-time notification if needed
    if (showNotification && m_showNotifications && !m_hasShownFirstTimeNotification) {
        showFirstTimeNotification();
    }

    emit mainWindowVisibilityChanged(false);

    m_logger.debug("Main window hidden to system tray");
}

bool SystemTrayManager::isMainWindowHidden() const {
    return m_isMainWindowHidden;
}

void SystemTrayManager::requestApplicationExit() {
    m_logger.info("Application exit requested from system tray");
    emit applicationExitRequested();
}

bool SystemTrayManager::handleMainWindowCloseEvent() {
    if (!isMinimizeToTrayEnabled()) {
        m_logger.debug("Minimize to tray disabled, allowing normal close");
        return false; // Allow normal close
    }

    m_logger.debug("Handling main window close event - minimizing to tray");
    hideMainWindow(true);
    return true; // Ignore close event
}

void SystemTrayManager::createTrayIcon() {
    if (m_trayIcon) {
        return;
    }

    // Final check before creating tray icon
    if (!isSystemTrayAvailable()) {
        m_logger.error("Cannot create tray icon: system tray is not available");
        return;
    }

    m_logger.debug("Creating system tray icon");

    try {
        m_trayIcon = std::make_unique<QSystemTrayIcon>(this);

        // Set application icon with proper sizing for different platforms
        QIcon icon(":/images/icon");
        if (icon.isNull()) {
            m_logger.warning("Could not load application icon for system tray");
            // Use default icon as fallback
            icon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);

            if (icon.isNull()) {
                m_logger.error("Could not load fallback icon for system tray");
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
            m_logger.debug(QString("Setting tray icon with preferred size: %1x%2")
                          .arg(iconSize.width()).arg(iconSize.height()));
        }

        m_trayIcon->setIcon(icon);
        m_trayIcon->setToolTip("SAST Readium - PDF Reader");

        // Connect signals
        connect(m_trayIcon.get(), &QSystemTrayIcon::activated,
                this, &SystemTrayManager::onTrayIconActivated);

        m_logger.debug("System tray icon created successfully");

    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to create system tray icon: %1").arg(e.what()));
        m_trayIcon.reset();
    }
}

void SystemTrayManager::createContextMenu() {
    if (m_contextMenu || !m_trayIcon) {
        return;
    }

    m_logger.debug("Creating system tray context menu");

    m_contextMenu = std::make_unique<QMenu>();

    // Create restore action
    m_restoreAction = m_contextMenu->addAction("&Show SAST Readium");
    m_restoreAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_TitleBarMaxButton));
    m_restoreAction->setToolTip("Restore the main application window");
    connect(m_restoreAction, &QAction::triggered, this, &SystemTrayManager::onRestoreAction);

    // Add separator
    m_contextMenu->addSeparator();

    // Create exit action
    m_exitAction = m_contextMenu->addAction("E&xit");
    m_exitAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    m_exitAction->setToolTip("Exit SAST Readium completely");
    connect(m_exitAction, &QAction::triggered, this, &SystemTrayManager::onExitAction);

    // Set context menu
    m_trayIcon->setContextMenu(m_contextMenu.get());

    m_logger.debug("System tray context menu created with restore and exit actions");

    // Update initial menu state
    updateContextMenuState();
}

void SystemTrayManager::updateContextMenuState() {
    if (!m_contextMenu || !m_restoreAction) {
        return;
    }

    // Update restore action text and state based on window visibility
    if (m_isMainWindowHidden) {
        m_restoreAction->setText("&Show SAST Readium");
        m_restoreAction->setEnabled(true);
    } else {
        m_restoreAction->setText("&Hide to Tray");
        m_restoreAction->setEnabled(true);
    }
}

void SystemTrayManager::initializeSettings() {
    // Settings are initialized through ConfigurationManager defaults
    // This method can be used for any additional initialization if needed
    m_logger.debug("Initializing SystemTrayManager settings");
}

void SystemTrayManager::loadSettings() {
    ConfigurationManager& config = ConfigurationManager::instance();

    m_logger.debug("Loading SystemTrayManager settings");

    // Load basic settings
    m_isEnabled = config.getValue(SETTINGS_GROUP + "/" + SETTINGS_ENABLED_KEY, DEFAULT_ENABLED).toBool();
    m_minimizeToTrayEnabled = config.getValue(SETTINGS_GROUP + "/" + SETTINGS_MINIMIZE_TO_TRAY_KEY, DEFAULT_MINIMIZE_TO_TRAY).toBool();
    m_showNotifications = config.getValue(SETTINGS_GROUP + "/" + SETTINGS_SHOW_NOTIFICATIONS_KEY, DEFAULT_SHOW_NOTIFICATIONS).toBool();
    m_hasShownFirstTimeNotification = config.getValue(SETTINGS_GROUP + "/" + SETTINGS_FIRST_TIME_NOTIFICATION_SHOWN_KEY, false).toBool();

    // Load enhanced feature settings
    m_showStatusIndicators = config.getValue(SETTINGS_GROUP + "/" + SETTINGS_SHOW_STATUS_INDICATORS_KEY, DEFAULT_SHOW_STATUS_INDICATORS).toBool();
    m_showRecentFiles = config.getValue(SETTINGS_GROUP + "/" + SETTINGS_SHOW_RECENT_FILES_KEY, DEFAULT_SHOW_RECENT_FILES).toBool();
    m_recentFilesCount = config.getValue(SETTINGS_GROUP + "/" + SETTINGS_RECENT_FILES_COUNT_KEY, DEFAULT_RECENT_FILES_COUNT).toInt();
    m_showQuickActions = config.getValue(SETTINGS_GROUP + "/" + SETTINGS_SHOW_QUICK_ACTIONS_KEY, DEFAULT_SHOW_QUICK_ACTIONS).toBool();
    m_enhancedNotifications = config.getValue(SETTINGS_GROUP + "/" + SETTINGS_ENHANCED_NOTIFICATIONS_KEY, DEFAULT_ENHANCED_NOTIFICATIONS).toBool();
    m_notificationTypes = config.getValue(SETTINGS_GROUP + "/" + SETTINGS_NOTIFICATION_TYPES_KEY, "document,status,error").toString();
    m_dynamicTooltip = config.getValue(SETTINGS_GROUP + "/" + SETTINGS_DYNAMIC_TOOLTIP_KEY, DEFAULT_DYNAMIC_TOOLTIP).toBool();

    m_logger.debug(QString("Settings loaded - enabled: %1, minimizeToTray: %2, showNotifications: %3, enhanced features: %4")
                   .arg(m_isEnabled).arg(m_minimizeToTrayEnabled).arg(m_showNotifications).arg(areEnhancedFeaturesEnabled()));
}

void SystemTrayManager::saveSettings() {
    ConfigurationManager& config = ConfigurationManager::instance();

    m_logger.debug("Saving SystemTrayManager settings");

    // Save basic settings
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_ENABLED_KEY, m_isEnabled);
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_MINIMIZE_TO_TRAY_KEY, m_minimizeToTrayEnabled);
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_SHOW_NOTIFICATIONS_KEY, m_showNotifications);
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_FIRST_TIME_NOTIFICATION_SHOWN_KEY, m_hasShownFirstTimeNotification);

    // Save enhanced feature settings
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_SHOW_STATUS_INDICATORS_KEY, m_showStatusIndicators);
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_SHOW_RECENT_FILES_KEY, m_showRecentFiles);
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_RECENT_FILES_COUNT_KEY, m_recentFilesCount);
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_SHOW_QUICK_ACTIONS_KEY, m_showQuickActions);
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_ENHANCED_NOTIFICATIONS_KEY, m_enhancedNotifications);
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_NOTIFICATION_TYPES_KEY, m_notificationTypes);
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_DYNAMIC_TOOLTIP_KEY, m_dynamicTooltip);
}

void SystemTrayManager::applySettingsChange(const QString& settingsGroup, const QString& key, const QVariant& value) {
    // Only handle our settings group
    if (settingsGroup != SETTINGS_GROUP) {
        return;
    }

    m_logger.debug(QString("Applying settings change: %1/%2 = %3")
                   .arg(settingsGroup).arg(key).arg(value.toString()));

    bool oldEnabled = m_isEnabled;
    bool oldMinimizeToTray = m_minimizeToTrayEnabled;
    bool oldShowNotifications = m_showNotifications;

    // Update the appropriate setting
    if (key == SETTINGS_ENABLED_KEY) {
        setEnabled(value.toBool());
    } else if (key == SETTINGS_MINIMIZE_TO_TRAY_KEY) {
        setMinimizeToTrayEnabled(value.toBool());
    } else if (key == SETTINGS_SHOW_NOTIFICATIONS_KEY) {
        m_showNotifications = value.toBool();
        emit showNotificationsChanged(m_showNotifications);
    } else if (key == SETTINGS_FIRST_TIME_NOTIFICATION_SHOWN_KEY) {
        m_hasShownFirstTimeNotification = value.toBool();
    }

    // Log changes
    if (oldEnabled != m_isEnabled) {
        m_logger.info(QString("System tray enabled changed: %1 -> %2").arg(oldEnabled).arg(m_isEnabled));
    }
    if (oldMinimizeToTray != m_minimizeToTrayEnabled) {
        m_logger.info(QString("Minimize to tray changed: %1 -> %2").arg(oldMinimizeToTray).arg(m_minimizeToTrayEnabled));
    }
    if (oldShowNotifications != m_showNotifications) {
        m_logger.info(QString("Show notifications changed: %1 -> %2").arg(oldShowNotifications).arg(m_showNotifications));
    }
}

void SystemTrayManager::updateTrayIconVisibility() {
    if (!m_trayIcon) {
        return;
    }

    if (m_isEnabled && isSystemTrayAvailable()) {
        m_logger.debug("Showing system tray icon");
        m_trayIcon->show();
    } else {
        m_logger.debug("Hiding system tray icon");
        m_trayIcon->hide();
    }
}

void SystemTrayManager::showFirstTimeNotification() {
    if (!m_trayIcon || !m_showNotifications) {
        return;
    }

    m_logger.info("Showing first-time system tray notification");

    QString title = "SAST Readium - Minimized to Tray";
    QString message = "The application is now running in the system tray.\n\n"
                     "• Left-click the tray icon to restore the window\n"
                     "• Double-click to always show the window\n"
                     "• Right-click for menu options\n"
                     "• Use the tray menu to exit the application";

    // Show notification for 8 seconds to give users time to read the instructions
    m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 8000);

    // Mark that we've shown the first-time notification
    m_hasShownFirstTimeNotification = true;

    // Save this setting immediately so it persists
    ConfigurationManager& config = ConfigurationManager::instance();
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_FIRST_TIME_NOTIFICATION_SHOWN_KEY, true);
}

void SystemTrayManager::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
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

    m_logger.debug(QString("Tray icon activated: %1 (reason: %2)")
                   .arg(reasonStr).arg(static_cast<int>(reason)));

    switch (reason) {
    case QSystemTrayIcon::Trigger:
        // Left click - toggle window visibility
        if (m_isMainWindowHidden) {
            showMainWindow();
        } else {
            // Only hide if minimize to tray is enabled
            if (isMinimizeToTrayEnabled()) {
                hideMainWindow(false);
            } else {
                showMainWindow(); // Bring to front if already visible
            }
        }
        break;

    case QSystemTrayIcon::DoubleClick:
        // Double click - always show window
        showMainWindow();
        break;

    case QSystemTrayIcon::MiddleClick:
        // Middle click - toggle window visibility
        if (m_isMainWindowHidden) {
            showMainWindow();
        } else {
            hideMainWindow(false);
        }
        break;

    case QSystemTrayIcon::Context:
        // Right click - context menu is shown automatically
        m_logger.debug("Context menu will be shown automatically");
        break;

    default:
        m_logger.debug("Unhandled tray icon activation reason");
        break;
    }
}

void SystemTrayManager::onRestoreAction() {
    m_logger.debug("Restore/Hide action triggered from tray menu");

    if (m_isMainWindowHidden) {
        showMainWindow();
    } else {
        hideMainWindow(false); // Don't show notification when manually hiding
    }
}

void SystemTrayManager::onExitAction() {
    m_logger.debug("Exit action triggered from tray menu");
    requestApplicationExit();
}

void SystemTrayManager::createDefaultTrayIcon(QIcon& icon) {
    m_logger.debug("Creating default tray icon");

    // Create a simple colored icon as fallback
    QPixmap pixmap(16, 16);
    pixmap.fill(QColor(70, 130, 180)); // Steel blue color

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
        m_logger.info(QString("System tray availability changed: %1 -> %2")
                      .arg(lastKnownAvailability ? "available" : "unavailable")
                      .arg(currentlyAvailable ? "available" : "unavailable"));

        if (currentlyAvailable && m_isEnabled) {
            // System tray became available - create tray icon if we don't have one
            if (!m_trayIcon) {
                m_logger.info("System tray became available - creating tray icon");
                createTrayIcon();
                createContextMenu();
                updateTrayIconVisibility();
            }
        } else if (!currentlyAvailable && m_trayIcon) {
            // System tray became unavailable - handle gracefully
            m_logger.warning("System tray became unavailable - hiding tray icon");
            m_trayIcon->hide();

            // If main window is hidden, show it since tray is no longer available
            if (m_isMainWindowHidden) {
                m_logger.info("Restoring main window since system tray is no longer available");
                showMainWindow();
            }
        }

        lastKnownAvailability = currentlyAvailable;
    }
}

// Enhanced functionality implementations

void SystemTrayManager::setApplicationStatus(const QString& status, const QString& message) {
    if (m_currentStatus == status && m_currentStatusMessage == message) {
        return;
    }

    m_logger.debug(QString("Setting application status: %1 - %2").arg(status, message));

    m_currentStatus = status;
    m_currentStatusMessage = message;

    // Update tray icon if status indicators are enabled
    if (m_showStatusIndicators && m_trayIcon) {
        updateTrayIconForStatus(status);
    }

    // Update dynamic tooltip if enabled
    if (m_dynamicTooltip) {
        updateDynamicTooltip();
    }

    // Update status in context menu if enabled
    updateStatusInContextMenu();

    emit applicationStatusChanged(status, message);
}

void SystemTrayManager::showNotification(const QString& title, const QString& message,
                                       const QString& type, int timeout) {
    if (!m_trayIcon || !m_enhancedNotifications) {
        return;
    }

    // Check if this notification type is enabled
    if (!isNotificationTypeEnabled(type)) {
        m_logger.debug(QString("Notification type '%1' is disabled, skipping").arg(type));
        return;
    }

    m_logger.info(QString("Showing notification: %1 - %2 (type: %3)").arg(title, message, type));

    // Determine notification icon based on type
    QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information;
    if (type == "warning") {
        icon = QSystemTrayIcon::Warning;
    } else if (type == "error") {
        icon = QSystemTrayIcon::Critical;
    }

    m_trayIcon->showMessage(title, message, icon, timeout);
    emit notificationShown(title, message, type);
}

void SystemTrayManager::updateDynamicTooltip(const QString& tooltip) {
    if (!m_trayIcon || !m_dynamicTooltip) {
        return;
    }

    QString newTooltip;
    if (!tooltip.isEmpty()) {
        newTooltip = tooltip;
    } else {
        // Generate dynamic tooltip based on current state
        newTooltip = "SAST Readium - PDF Reader";

        if (!m_currentStatusMessage.isEmpty()) {
            newTooltip += QString("\nStatus: %1").arg(m_currentStatusMessage);
        } else if (m_currentStatus != "idle") {
            newTooltip += QString("\nStatus: %1").arg(m_currentStatus);
        }

        if (m_isMainWindowHidden) {
            newTooltip += "\n(Running in background)";
        }
    }

    m_trayIcon->setToolTip(newTooltip);
    m_logger.debug(QString("Updated dynamic tooltip: %1").arg(newTooltip));
}

QString SystemTrayManager::currentApplicationStatus() const {
    return m_currentStatus;
}

bool SystemTrayManager::areEnhancedFeaturesEnabled() const {
    return m_showStatusIndicators || m_showRecentFiles || m_showQuickActions ||
           m_enhancedNotifications || m_dynamicTooltip;
}

void SystemTrayManager::updateTrayIconForStatus(const QString& status) {
    if (!m_trayIcon) {
        return;
    }

    // Get base application icon
    QIcon baseIcon(":/images/icon");
    if (baseIcon.isNull()) {
        baseIcon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
    }

    // Generate status icon with overlay
    QIcon statusIcon = generateStatusIcon(baseIcon, status);
    m_trayIcon->setIcon(statusIcon);

    m_logger.debug(QString("Updated tray icon for status: %1").arg(status));
}

QIcon SystemTrayManager::generateStatusIcon(const QIcon& baseIcon, const QString& status) {
    if (status == "idle" || !m_showStatusIndicators) {
        return baseIcon;
    }

    // Get base pixmap
    QPixmap basePixmap = baseIcon.pixmap(22, 22); // Standard tray icon size
    if (basePixmap.isNull()) {
        return baseIcon;
    }

    // Create a copy for modification
    QPixmap statusPixmap = basePixmap.copy();
    QPainter painter(&statusPixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Define status colors and overlay positions
    QColor overlayColor;
    QPoint overlayPos(16, 16); // Bottom-right corner
    int overlaySize = 8;

    if (status == "processing") {
        overlayColor = QColor(255, 165, 0); // Orange
    } else if (status == "error") {
        overlayColor = QColor(220, 53, 69); // Red
    } else if (status == "success") {
        overlayColor = QColor(40, 167, 69); // Green
    } else if (status == "warning") {
        overlayColor = QColor(255, 193, 7); // Yellow
    } else {
        overlayColor = QColor(108, 117, 125); // Gray for unknown status
    }

    // Draw status overlay circle
    painter.setBrush(QBrush(overlayColor));
    painter.setPen(QPen(Qt::white, 1));
    painter.drawEllipse(overlayPos.x() - overlaySize/2, overlayPos.y() - overlaySize/2,
                       overlaySize, overlaySize);

    painter.end();

    return QIcon(statusPixmap);
}

bool SystemTrayManager::isNotificationTypeEnabled(const QString& type) const {
    if (!m_enhancedNotifications) {
        return false;
    }

    // Parse notification types setting
    QStringList enabledTypes = m_notificationTypes.split(',', Qt::SkipEmptyParts);
    for (QString& enabledType : enabledTypes) {
        enabledType = enabledType.trimmed();
    }

    return enabledTypes.contains(type) || enabledTypes.contains("all");
}

void SystemTrayManager::connectToApplicationEvents() {
    if (!m_enhancedNotifications && !m_showStatusIndicators) {
        m_logger.debug("Enhanced features disabled, skipping event connections");
        return;
    }

    EventBus& eventBus = EventBus::instance();

    // Connect to document events
    eventBus.subscribe(AppEvents::DOCUMENT_OPENED, this, [this](Event* event) {
        if (m_showStatusIndicators) {
            setApplicationStatus("processing", "Opening document...");
        }
        if (m_enhancedNotifications && isNotificationTypeEnabled("document")) {
            QString fileName = event->data().toString();
            if (!fileName.isEmpty()) {
                QFileInfo fileInfo(fileName);
                showNotification("Document Opened",
                               QString("Opened: %1").arg(fileInfo.fileName()),
                               "info", 3000);
            }
        }
        // Reset to idle after a short delay
        QTimer::singleShot(2000, this, [this]() {
            setApplicationStatus("idle");
        });
    });

    eventBus.subscribe(AppEvents::DOCUMENT_CLOSED, this, [this](Event* event) {
        if (m_showStatusIndicators) {
            setApplicationStatus("idle", "Ready");
        }
        if (m_enhancedNotifications && isNotificationTypeEnabled("document")) {
            showNotification("Document Closed", "Document has been closed", "info", 2000);
        }
    });

    eventBus.subscribe(AppEvents::DOCUMENT_SAVED, this, [this](Event* event) {
        if (m_showStatusIndicators) {
            setApplicationStatus("success", "Document saved");
        }
        if (m_enhancedNotifications && isNotificationTypeEnabled("document")) {
            QString fileName = event->data().toString();
            if (!fileName.isEmpty()) {
                QFileInfo fileInfo(fileName);
                showNotification("Document Saved",
                               QString("Saved: %1").arg(fileInfo.fileName()),
                               "info", 2000);
            }
        }
        // Reset to idle after showing success
        QTimer::singleShot(3000, this, [this]() {
            setApplicationStatus("idle");
        });
    });

    // Connect to error events (if they exist)
    eventBus.subscribe("error.occurred", this, [this](Event* event) {
        if (m_showStatusIndicators) {
            setApplicationStatus("error", "Error occurred");
        }
        if (m_enhancedNotifications && isNotificationTypeEnabled("error")) {
            QString errorMessage = event->data().toString();
            showNotification("Error", errorMessage.isEmpty() ? "An error occurred" : errorMessage,
                           "error", 5000);
        }
        // Reset to idle after showing error
        QTimer::singleShot(5000, this, [this]() {
            setApplicationStatus("idle");
        });
    });

    // Connect to loading progress events
    eventBus.subscribe("document.loading", this, [this](Event* event) {
        if (m_showStatusIndicators) {
            setApplicationStatus("processing", "Loading document...");
        }
    });

    m_logger.debug("Connected to application events for enhanced system tray functionality");
}

void SystemTrayManager::createEnhancedContextMenu() {
    if (m_contextMenu || !m_trayIcon) {
        return;
    }

    m_logger.debug("Creating enhanced system tray context menu");

    m_contextMenu = std::make_unique<QMenu>();

    // Create restore action (same as basic menu)
    m_restoreAction = m_contextMenu->addAction("&Show SAST Readium");
    m_restoreAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_TitleBarMaxButton));
    m_restoreAction->setToolTip("Restore the main application window");
    connect(m_restoreAction, &QAction::triggered, this, &SystemTrayManager::onRestoreAction);

    // Add separator
    m_contextMenu->addSeparator();

    // Add recent files submenu if enabled
    if (m_showRecentFiles) {
        createRecentFilesMenu();
    }

    // Add quick actions submenu if enabled
    if (m_showQuickActions) {
        createQuickActionsMenu();
    }

    // Add status information if status indicators are enabled
    if (m_showStatusIndicators) {
        m_statusSeparator = m_contextMenu->addSeparator();
        m_statusAction = m_contextMenu->addAction(QString("Status: %1").arg(m_currentStatus));
        m_statusAction->setEnabled(false); // Make it non-clickable, just informational
        if (!m_currentStatusMessage.isEmpty()) {
            m_statusAction->setToolTip(m_currentStatusMessage);
        }
    }

    // Add settings submenu
    createSettingsMenu();

    // Add separator before exit
    m_contextMenu->addSeparator();

    // Create exit action (same as basic menu)
    m_exitAction = m_contextMenu->addAction("&Exit");
    m_exitAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    m_exitAction->setToolTip("Exit the application");
    connect(m_exitAction, &QAction::triggered, this, &SystemTrayManager::onExitAction);

    // Set context menu for tray icon
    m_trayIcon->setContextMenu(m_contextMenu.get());

    m_logger.debug("Enhanced system tray context menu created successfully");
}

void SystemTrayManager::createRecentFilesMenu() {
    if (!m_contextMenu) {
        return;
    }

    m_recentFilesMenu = std::make_unique<QMenu>("Recent Files");
    m_recentFilesMenu->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView));

    // Get recent files from RecentFilesManager
    // Note: We'll need to get a reference to RecentFilesManager from ApplicationController
    // For now, create a placeholder structure

    // Add "Open File..." action
    m_openFileAction = m_recentFilesMenu->addAction("&Open File...");
    m_openFileAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton));
    m_openFileAction->setToolTip("Open a new document");
    connect(m_openFileAction, &QAction::triggered, this, [this]() {
        emit quickActionTriggered("open_file");
    });

    m_recentFilesMenu->addSeparator();

    // Add placeholder for recent files (will be populated dynamically)
    QAction* noRecentFiles = m_recentFilesMenu->addAction("No recent files");
    noRecentFiles->setEnabled(false);

    // Add the submenu to main context menu
    m_contextMenu->addMenu(m_recentFilesMenu.get());
}

void SystemTrayManager::createQuickActionsMenu() {
    if (!m_contextMenu) {
        return;
    }

    m_quickActionsMenu = std::make_unique<QMenu>("Quick Actions");
    m_quickActionsMenu->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));

    // Add common quick actions
    QAction* openFileAction = m_quickActionsMenu->addAction("&Open File...");
    openFileAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton));
    connect(openFileAction, &QAction::triggered, this, [this]() {
        emit quickActionTriggered("open_file");
    });

    QAction* settingsAction = m_quickActionsMenu->addAction("&Settings...");
    settingsAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
    connect(settingsAction, &QAction::triggered, this, [this]() {
        emit settingsDialogRequested();
    });

    QAction* aboutAction = m_quickActionsMenu->addAction("&About...");
    aboutAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation));
    connect(aboutAction, &QAction::triggered, this, [this]() {
        emit aboutDialogRequested();
    });

    // Add the submenu to main context menu
    m_contextMenu->addMenu(m_quickActionsMenu.get());
}

void SystemTrayManager::createSettingsMenu() {
    if (!m_contextMenu) {
        return;
    }

    m_settingsMenu = std::make_unique<QMenu>("Settings");
    m_settingsMenu->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));

    // Add tray-specific settings
    QAction* statusIndicatorsAction = m_settingsMenu->addAction("Show Status Indicators");
    statusIndicatorsAction->setCheckable(true);
    statusIndicatorsAction->setChecked(m_showStatusIndicators);
    connect(statusIndicatorsAction, &QAction::toggled, this, [this](bool checked) {
        m_showStatusIndicators = checked;
        saveSettings();
        emit enhancedFeaturesChanged(areEnhancedFeaturesEnabled());
    });

    QAction* recentFilesAction = m_settingsMenu->addAction("Show Recent Files");
    recentFilesAction->setCheckable(true);
    recentFilesAction->setChecked(m_showRecentFiles);
    connect(recentFilesAction, &QAction::toggled, this, [this](bool checked) {
        m_showRecentFiles = checked;
        saveSettings();
        emit enhancedFeaturesChanged(areEnhancedFeaturesEnabled());
    });

    QAction* notificationsAction = m_settingsMenu->addAction("Enhanced Notifications");
    notificationsAction->setCheckable(true);
    notificationsAction->setChecked(m_enhancedNotifications);
    connect(notificationsAction, &QAction::toggled, this, [this](bool checked) {
        m_enhancedNotifications = checked;
        saveSettings();
        emit enhancedFeaturesChanged(areEnhancedFeaturesEnabled());
    });

    m_settingsMenu->addSeparator();

    // Add link to main settings
    m_settingsAction = m_settingsMenu->addAction("&Open Settings...");
    m_settingsAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
    connect(m_settingsAction, &QAction::triggered, this, [this]() {
        emit settingsDialogRequested();
    });

    // Add the submenu to main context menu
    m_contextMenu->addMenu(m_settingsMenu.get());
}

void SystemTrayManager::updateStatusInContextMenu() {
    if (!m_statusAction || !m_showStatusIndicators) {
        return;
    }

    QString statusText = QString("Status: %1").arg(m_currentStatus);
    if (!m_currentStatusMessage.isEmpty()) {
        statusText += QString(" - %1").arg(m_currentStatusMessage);
    }

    m_statusAction->setText(statusText);
    m_statusAction->setToolTip(m_currentStatusMessage.isEmpty() ? m_currentStatus : m_currentStatusMessage);

    m_logger.debug(QString("Updated status in context menu: %1").arg(statusText));
}

void SystemTrayManager::setNotificationTypes(const QString& types) {
    if (m_notificationTypes == types) {
        return;
    }

    m_logger.info(QString("Setting notification types: %1").arg(types));
    m_notificationTypes = types;

    // Save to configuration
    ConfigurationManager& config = ConfigurationManager::instance();
    config.setValue(SETTINGS_GROUP + "/" + SETTINGS_NOTIFICATION_TYPES_KEY, types);

    // Emit signal if enhanced notifications are enabled
    if (m_enhancedNotifications) {
        emit enhancedFeaturesChanged(areEnhancedFeaturesEnabled());
    }
}

QString SystemTrayManager::getNotificationTypes() const {
    return m_notificationTypes;
}

void SystemTrayManager::connectToRecentFilesManager(RecentFilesManager* recentFilesManager) {
    if (!recentFilesManager) {
        m_logger.warning("Cannot connect to null RecentFilesManager");
        return;
    }

    m_recentFilesManager = recentFilesManager;

    // Connect to recent files changes
    connect(m_recentFilesManager, &RecentFilesManager::recentFilesChanged,
            this, &SystemTrayManager::updateRecentFilesMenu);

    connect(m_recentFilesManager, &RecentFilesManager::recentFileAdded,
            this, &SystemTrayManager::updateRecentFilesMenu);

    connect(m_recentFilesManager, &RecentFilesManager::recentFileRemoved,
            this, &SystemTrayManager::updateRecentFilesMenu);

    // Initial update
    updateRecentFilesMenu();

    m_logger.debug("Connected to RecentFilesManager");
}

void SystemTrayManager::updateRecentFilesMenu() {
    if (!m_recentFilesMenu || !m_recentFilesManager || !m_showRecentFiles) {
        return;
    }

    m_logger.debug("Updating recent files menu");

    // Clear existing recent file actions (keep "Open File..." and separator)
    QList<QAction*> actions = m_recentFilesMenu->actions();
    for (int i = actions.size() - 1; i >= 0; --i) {
        QAction* action = actions[i];
        if (action != m_openFileAction && !action->isSeparator()) {
            m_recentFilesMenu->removeAction(action);
            delete action;
        }
    }

    // Get recent files
    QStringList recentFiles = m_recentFilesManager->getRecentFilePaths();

    if (recentFiles.isEmpty()) {
        // Add "No recent files" placeholder
        QAction* noFilesAction = m_recentFilesMenu->addAction("No recent files");
        noFilesAction->setEnabled(false);
    } else {
        // Add recent files (limit to configured count)
        int count = qMin(recentFiles.size(), m_recentFilesCount);
        for (int i = 0; i < count; ++i) {
            const QString& filePath = recentFiles[i];
            QFileInfo fileInfo(filePath);

            QString displayName = fileInfo.fileName();
            if (displayName.length() > 30) {
                displayName = displayName.left(27) + "...";
            }

            QAction* fileAction = m_recentFilesMenu->addAction(displayName);
            fileAction->setToolTip(filePath);
            fileAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileIcon));

            // Connect to signal emission
            connect(fileAction, &QAction::triggered, this, [this, filePath]() {
                emit recentFileRequested(filePath);
            });
        }
    }

    m_logger.debug(QString("Updated recent files menu with %1 files").arg(recentFiles.size()));
}
