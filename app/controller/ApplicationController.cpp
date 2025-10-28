#include "ApplicationController.h"
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMessageBox>
#include <QPixmap>
#include <QSplitter>
#include <QStackedWidget>
#include "../MainWindow.h"
#include "../factory/WidgetFactory.h"
#include "../logging/LoggingMacros.h"
#include "../managers/FileTypeIconManager.h"
#include "../managers/I18nManager.h"
#include "../managers/RecentFilesManager.h"
#include "../managers/StyleManager.h"
#include "../managers/SystemTrayManager.h"
#include "../model/DocumentModel.h"
#include "../model/PageModel.h"
#include "../model/RenderModel.h"
#include "../ui/core/MenuBar.h"
#include "../ui/core/RightSideBar.h"
#include "../ui/core/SideBar.h"
#include "../ui/core/StatusBar.h"
#include "../ui/core/ToolBar.h"
#include "../ui/core/UIErrorHandler.h"
#include "../ui/core/ViewWidget.h"
#include "../ui/dialogs/SettingsDialog.h"
#include "../ui/managers/WelcomeScreenManager.h"
#include "../ui/widgets/WelcomeWidget.h"
#include "../utils/ErrorHandling.h"
#include "../utils/ErrorRecovery.h"
#include "DocumentController.h"
#include "PageController.h"

ApplicationController::ApplicationController(QMainWindow* mainWindow,
                                             QObject* parent)
    : QObject(parent),
      m_mainWindow(mainWindow),
      m_logger("ApplicationController") {
    SLOG_TIMER("ApplicationController::Constructor");

    if (!m_mainWindow) {
        m_logger.error("MainWindow is null");
        throw std::invalid_argument("MainWindow cannot be null");
    }

    m_logger.debug("ApplicationController created");
}

ApplicationController::~ApplicationController() {
    if (!m_isShuttingDown) {
        shutdown();
    }
}

void ApplicationController::initializeApplication() {
    SLOG_TIMER("ApplicationController::initializeApplication");

    if (m_isInitialized) {
        m_logger.warning("Application already initialized");
        return;
    }

    try {
        m_logger.info("Starting application initialization...");

        // Apply initial theme
        QString defaultTheme =
            (STYLE.currentTheme() == Theme::Light) ? "light" : "dark";
        applyTheme(defaultTheme);
        m_logger.debug("Theme applied: " + defaultTheme);

        // Initialize components in correct order
        initializeModels();
        initializeControllers();
        initializeViews();
        initializeConnections();

        // Setup error handling
        setupErrorHandling();

        // Register UI components for state management
        registerUIComponents();

        // Register UI resources for memory management
        registerUIResources();

        // Register UI components for visual consistency
        registerUIConsistency();

        // Start async operations
        if (m_recentFilesManager) {
            m_recentFilesManager->initializeAsync();
            m_logger.debug("Async initialization started");
        }

        // Restore application state after all components are initialized
        QTimer::singleShot(100, this, [this]() {
            restoreApplicationState();

            // Enforce visual consistency after restoration
            QTimer::singleShot(200, this,
                               [this]() { enforceVisualConsistency(); });
        });

        m_isInitialized = true;
        m_logger.info("Application initialization completed successfully");

        emit initializationCompleted();

    } catch (const std::exception& e) {
        QString error = QString::fromStdString(e.what());
        m_logger.error("Initialization failed: " + error);
        handleError("Initialization", error);
        emit initializationFailed(error);
    } catch (...) {
        QString error = "Unknown error during initialization";
        m_logger.error(error);
        handleError("Initialization", error);
        emit initializationFailed(error);
    }
}

void ApplicationController::initializeModels() {
    SLOG_TIMER("ApplicationController::initializeModels");
    m_logger.info("========== initializeModels() STARTED ==========");

    try {
        // Create models with proper error handling
        m_logger.debug("Creating RenderModel...");
        m_renderModel = new RenderModel(m_mainWindow->logicalDpiX(),
                                        m_mainWindow->logicalDpiY());
        m_logger.debug("RenderModel created");

        m_logger.debug("Creating DocumentModel...");
        m_documentModel = new DocumentModel(m_renderModel);
        m_logger.debug("DocumentModel created");

        m_logger.debug("Creating PageModel...");
        m_pageModel = new PageModel(m_renderModel);
        m_logger.debug("PageModel created");

        // Create managers
        m_logger.debug("Creating RecentFilesManager...");
        m_recentFilesManager = new RecentFilesManager(this);
        m_logger.debug("RecentFilesManager created");

        m_logger.info("========== initializeModels() COMPLETED ==========");

    } catch (const std::exception& e) {
        QString error =
            "Failed to initialize models: " + QString::fromStdString(e.what());
        m_logger.error("========== initializeModels() FAILED: " + error +
                       " ==========");
        throw std::runtime_error(error.toStdString());
    }
}

void ApplicationController::initializeControllers() {
    SLOG_TIMER("ApplicationController::initializeControllers");
    m_logger.info("========== initializeControllers() STARTED ==========");

    try {
        m_logger.debug("Creating DocumentController...");
        m_documentController = new DocumentController(m_documentModel);
        m_logger.debug("DocumentController created");

        m_logger.debug("Creating PageController...");
        m_pageController = new PageController(m_pageModel);
        m_logger.debug("PageController created");

        // Setup dependencies
        m_logger.debug("Setting up controller dependencies...");
        m_documentController->setRecentFilesManager(m_recentFilesManager);
        m_logger.debug("Controller dependencies set up");

        m_logger.info(
            "========== initializeControllers() COMPLETED ==========");

    } catch (const std::exception& e) {
        QString error = "Failed to initialize controllers: " +
                        QString::fromStdString(e.what());
        m_logger.error("========== initializeControllers() FAILED: " + error +
                       " ==========");
        throw std::runtime_error(error.toStdString());
    }
}

void ApplicationController::initializeViews() {
    SLOG_TIMER("ApplicationController::initializeViews");
    m_logger.info("========== initializeViews() STARTED ==========");

    try {
        // Create factory for widget creation
        m_logger.debug("Creating WidgetFactory...");
        WidgetFactory factory(m_pageController, m_mainWindow);
        m_logger.debug("WidgetFactory created");

        // Create UI components
        m_logger.debug("Creating MenuBar...");
        m_menuBar = new MenuBar(m_mainWindow);
        m_logger.debug("MenuBar created");

        m_logger.debug("Creating ToolBar...");
        m_toolBar = new ToolBar(m_mainWindow);
        m_logger.debug("ToolBar created");

        m_logger.info("Creating SideBar...");
        m_sideBar = new SideBar(m_mainWindow);
        m_logger.info("SideBar created successfully");

        m_logger.info("Creating RightSideBar...");
        m_rightSideBar = new RightSideBar(m_mainWindow);
        m_logger.info("RightSideBar created successfully");

        m_logger.info("Creating StatusBar...");
        // Check if we're in test/minimal mode (offscreen platform or test
        // environment)
        QString platformName = QGuiApplication::platformName();
        bool isTestMode = (platformName == "offscreen" ||
                           qEnvironmentVariableIsSet("SAST_READIUM_TEST_MODE"));
        if (isTestMode) {
            m_logger.info(
                "Detected test/offscreen mode - creating minimal StatusBar");
            m_statusBar =
                new StatusBar(m_mainWindow, true);  // true = minimal mode
        } else {
            m_statusBar = new StatusBar(&factory, m_mainWindow);
        }
        m_logger.info("StatusBar created successfully");

        m_logger.info("Creating ViewWidget...");
        m_viewWidget = new ViewWidget(m_mainWindow);
        m_logger.info("ViewWidget created successfully");

        // Configure components
        m_logger.info("Configuring components...");
        m_logger.info("Setting RecentFilesManager on MenuBar...");
        m_menuBar->setRecentFilesManager(m_recentFilesManager);
        m_logger.info("Setting DocumentController on ViewWidget...");
        m_viewWidget->setDocumentController(m_documentController);
        m_logger.info("Setting DocumentModel on ViewWidget...");
        m_viewWidget->setDocumentModel(m_documentModel);
        m_logger.info("Components configured successfully");

        // Setup main window
        m_logger.info("Setting up main window...");
        m_logger.info("Setting MenuBar...");
        m_mainWindow->setMenuBar(m_menuBar);
        m_logger.info("Adding ToolBar...");
        m_mainWindow->addToolBar(m_toolBar);
        m_logger.info("Setting StatusBar...");
        m_mainWindow->setStatusBar(m_statusBar);

        // Connect StatusBar signals for proper integration
        connectStatusBarSignals();

        // Set StatusBar reference in DocumentController for progress indication
        if (m_documentController && m_statusBar) {
            m_documentController->setStatusBar(m_statusBar);
            m_logger.debug("StatusBar reference set in DocumentController");
        }

        m_logger.info("Main window set up successfully");

        // Initialize welcome screen
        // NOTE: Icon preloading is now deferred automatically by
        // FileTypeIconManager to avoid initialization hangs. Icons will load
        // asynchronously after the event loop starts.
        m_logger.debug(
            "Accessing FileTypeIconManager to trigger initialization...");
        (void)FILE_ICON_MANAGER;  // Trigger singleton initialization
        m_logger.debug(
            "FileTypeIconManager initialized (icons will preload "
            "asynchronously)");

        m_logger.debug("Creating WelcomeWidget...");
        m_welcomeWidget = new WelcomeWidget(m_mainWindow);
        m_logger.debug("WelcomeWidget created");

        m_logger.debug("Setting RecentFilesManager on WelcomeWidget...");
        m_welcomeWidget->setRecentFilesManager(m_recentFilesManager);
        m_logger.debug("RecentFilesManager set on WelcomeWidget");

        m_logger.debug("Creating WelcomeScreenManager...");
        m_welcomeScreenManager = new WelcomeScreenManager(m_mainWindow);
        m_logger.debug("WelcomeScreenManager created");

        m_logger.debug("Configuring WelcomeScreenManager...");
        m_welcomeScreenManager->setMainWindow(
            qobject_cast<MainWindow*>(m_mainWindow.data()));
        m_welcomeScreenManager->setWelcomeWidget(m_welcomeWidget);
        m_welcomeScreenManager->setDocumentModel(m_documentModel);
        m_welcomeWidget->setWelcomeScreenManager(m_welcomeScreenManager);
        m_logger.debug("WelcomeScreenManager configured");

        m_logger.debug("Applying theme to WelcomeWidget...");
        m_welcomeWidget->applyTheme();
        m_logger.debug("Theme applied to WelcomeWidget");

        // Initialize system tray manager
        m_logger.debug("Initializing SystemTrayManager...");
        m_systemTrayManager = &SystemTrayManager::instance();
        if (!m_systemTrayManager->initialize(m_mainWindow)) {
            m_logger.warning("Failed to initialize SystemTrayManager");
        } else {
            m_logger.debug("SystemTrayManager initialized successfully");
            // Connect system tray exit request to application exit
            connect(m_systemTrayManager,
                    &SystemTrayManager::applicationExitRequested, this,
                    [this]() {
                        m_logger.info(
                            "Application exit requested from system tray");
                        QApplication::quit();
                    });

            // Connect RecentFilesManager to system tray for enhanced
            // functionality
            if (m_recentFilesManager) {
                m_systemTrayManager->connectToRecentFilesManager(
                    m_recentFilesManager);
            }

            // Connect enhanced system tray signals
            connect(m_systemTrayManager,
                    &SystemTrayManager::recentFileRequested, this,
                    [this](const QString& filePath) {
                        if (m_documentController) {
                            m_documentController->openDocument(filePath);
                        }
                    });

            connect(m_systemTrayManager,
                    &SystemTrayManager::quickActionTriggered, this,
                    [this](const QString& actionId) {
                        if (actionId == "open_file") {
                            // Trigger file open dialog through
                            // DocumentController
                            m_logger.debug("Quick action: open file requested");
                            if (m_documentController && m_mainWindow) {
                                m_documentController->execute(
                                    ActionMap::openFile, m_mainWindow);
                            }
                        }
                    });

            connect(
                m_systemTrayManager,
                &SystemTrayManager::settingsDialogRequested, this, [this]() {
                    m_logger.debug(
                        "Settings dialog requested from system tray");
                    // Show settings dialog
                    SettingsDialog* dialog = new SettingsDialog(m_mainWindow);

                    // Connect theme changes
                    connect(
                        dialog, &SettingsDialog::themeChanged, this,
                        [this](const QString& theme) { applyTheme(theme); });

                    // Connect language changes
                    connect(
                        dialog, &SettingsDialog::languageChanged, this,
                        [](const QString& languageCode) {
                            I18nManager::instance().loadLanguage(languageCode);
                        });

                    dialog->exec();
                    dialog->deleteLater();
                });

            connect(
                m_systemTrayManager, &SystemTrayManager::aboutDialogRequested,
                this, [this]() {
                    m_logger.debug("About dialog requested from system tray");
                    // Show about dialog with application information
                    QString aboutText =
                        tr("<h2>SAST Readium</h2>"
                           "<p>Version: %1</p>"
                           "<p>A modern PDF viewer built with Qt6 and "
                           "Poppler.</p>"
                           "<p><b>Features:</b></p>"
                           "<ul>"
                           "<li>Fast PDF rendering</li>"
                           "<li>Multiple viewing modes</li>"
                           "<li>Search functionality</li>"
                           "<li>Annotation support</li>"
                           "<li>Bookmark management</li>"
                           "</ul>"
                           "<p><b>Developed by:</b> SAST Team</p>"
                           "<p>Built with Qt %2 and Poppler-Qt6</p>")
                            .arg(QApplication::applicationVersion())
                            .arg(QT_VERSION_STR);

                    QMessageBox aboutBox(m_mainWindow);
                    aboutBox.setWindowTitle(tr("About SAST Readium"));
                    aboutBox.setTextFormat(Qt::RichText);
                    aboutBox.setText(aboutText);
                    aboutBox.setIconPixmap(
                        QPixmap(":/icons/app-icon.png")
                            .scaled(64, 64, Qt::KeepAspectRatio,
                                    Qt::SmoothTransformation));
                    aboutBox.setStandardButtons(QMessageBox::Ok);
                    aboutBox.exec();
                });
        }

        // Create stacked widget for view switching
        m_logger.debug("Creating content stack widget...");
        m_contentStack = new QStackedWidget(m_mainWindow);

        // Set consistent background for better visual hierarchy
        // No padding on the stack itself - padding is applied to individual
        // views
        m_contentStack->setStyleSheet(
            QString("QStackedWidget { background-color: %1; }")
                .arg(STYLE.backgroundColor().name()));
        m_contentStack->setContentsMargins(0, 0, 0, 0);
        m_logger.debug("Content stack widget created");

        // Create main viewer area
        m_logger.debug("Creating main viewer area...");
        QWidget* mainViewerWidget = new QWidget();
        mainViewerWidget->setStyleSheet(
            QString("QWidget { background-color: %1; }")
                .arg(STYLE.backgroundColor().name()));
        QHBoxLayout* mainViewerLayout = new QHBoxLayout(mainViewerWidget);

        // Add subtle margins for visual breathing room
        // Top margin provides separation from toolbar
        // Bottom margin provides separation from status bar
        // Left/right margins kept at 0 to maximize content area
        mainViewerLayout->setContentsMargins(0, STYLE.spacingXS(), 0,
                                             STYLE.spacingXS());
        mainViewerLayout->setSpacing(
            0);  // No spacing between splitter sections

        m_mainSplitter = new QSplitter(Qt::Horizontal, mainViewerWidget);
        m_mainSplitter->addWidget(m_sideBar);
        m_mainSplitter->addWidget(m_viewWidget);
        m_mainSplitter->addWidget(m_rightSideBar);

        // Configure splitter behavior for responsive design
        m_mainSplitter->setCollapsible(0, true);   // Left sidebar can collapse
        m_mainSplitter->setCollapsible(1, false);  // Main view cannot collapse
        m_mainSplitter->setCollapsible(2, true);   // Right sidebar can collapse

        // Set stretch factors for proper resizing behavior
        m_mainSplitter->setStretchFactor(
            0, 0);  // Sidebar maintains preferred width
        m_mainSplitter->setStretchFactor(1,
                                         1);  // Main view expands to fill space
        m_mainSplitter->setStretchFactor(
            2, 0);  // Right sidebar maintains preferred width

        // Set handle width for better usability and visual feedback
        // Width of 6px provides good balance between visibility and space
        // efficiency
        m_mainSplitter->setHandleWidth(6);

        // Enable child widgets to have proper mouse tracking for splitter
        // interaction
        m_mainSplitter->setChildrenCollapsible(true);

        // Set object name for QSS styling
        m_mainSplitter->setObjectName("MainContentSplitter");

        // Calculate optimal initial sizes
        int leftWidth =
            m_sideBar->isVisible() ? m_sideBar->getPreferredWidth() : 0;
        int rightWidth = m_rightSideBar->isVisible()
                             ? m_rightSideBar->getPreferredWidth()
                             : 0;
        // Center panel gets remaining space (will be calculated by splitter)
        m_mainSplitter->setSizes({leftWidth, 800, rightWidth});

        mainViewerLayout->addWidget(m_mainSplitter);
        m_logger.debug("Main viewer area created");

        // Add views to stack
        m_logger.debug("Adding views to content stack...");
        m_contentStack->addWidget(m_welcomeWidget);
        m_contentStack->addWidget(mainViewerWidget);
        m_logger.debug("Views added to content stack");

        m_logger.debug("Setting central widget...");
        m_mainWindow->setCentralWidget(m_contentStack);
        m_logger.debug("Central widget set");

        // Set initial view
        m_logger.debug("Setting initial view...");
        if (m_welcomeScreenManager &&
            m_welcomeScreenManager->shouldShowWelcomeScreen()) {
            m_logger.debug("Showing welcome screen");
            showWelcomeScreen();
        } else {
            m_logger.debug("Showing main view");
            showMainView();
        }
        m_logger.debug("Initial view set");

        m_logger.info("========== initializeViews() COMPLETED ==========");

    } catch (const std::exception& e) {
        QString error =
            "Failed to initialize views: " + QString::fromStdString(e.what());
        m_logger.error("========== initializeViews() FAILED: " + error +
                       " ==========");
        throw std::runtime_error(error.toStdString());
    }
}

void ApplicationController::initializeConnections() {
    SLOG_TIMER("ApplicationController::initializeConnections");
    m_logger.info("========== initializeConnections() STARTED ==========");

    try {
        m_logger.debug("Connecting model signals...");
        connectModelSignals();
        m_logger.debug("Model signals connected");

        m_logger.debug("Connecting controller signals...");
        connectControllerSignals();
        m_logger.debug("Controller signals connected");

        m_logger.debug("Connecting view signals...");
        connectViewSignals();
        m_logger.debug("View signals connected");

        m_logger.info(
            "========== initializeConnections() COMPLETED ==========");

    } catch (const std::exception& e) {
        QString error = "Failed to initialize connections: " +
                        QString::fromStdString(e.what());
        m_logger.error("========== initializeConnections() FAILED: " + error +
                       " ==========");
        throw std::runtime_error(error.toStdString());
    }
}

void ApplicationController::connectModelSignals() {
    // Connect model signals for state changes
    if (m_documentModel) {
        connect(m_documentModel, &DocumentModel::documentOpened, this,
                [this](int index, const QString& fileName) {
                    Q_UNUSED(index)
                    showMainView();
                    m_logger.debug("Document opened: " + fileName +
                                   ", switching to main view");

                    // Update RightSideBar with document properties
                    if (m_rightSideBar && m_documentModel) {
                        auto* doc = m_documentModel->getCurrentDocument();
                        m_rightSideBar->setDocument(doc, fileName);
                    }
                });

        connect(m_documentModel, &DocumentModel::documentClosed, this,
                [this]() {
                    // Clear RightSideBar properties when document is closed
                    if (m_rightSideBar) {
                        m_rightSideBar->clearDocument();
                    }
                });
    }
}

void ApplicationController::connectControllerSignals() {
    // Connect controller signals
    if (m_documentController) {
        connect(
            m_documentController,
            &DocumentController::documentOperationCompleted, this,
            [this](ActionMap action, bool success) {
                if (success) {
                    m_logger.debug("Document operation completed successfully");
                } else {
                    m_logger.warning("Document operation failed");
                }
            });
    }
}

void ApplicationController::connectViewSignals() {
    // Connect menu bar signals
    if (m_menuBar) {
        connect(m_menuBar, &MenuBar::themeChanged, this,
                &ApplicationController::applyTheme);

        connect(m_menuBar, &MenuBar::onExecuted, m_documentController,
                &DocumentController::execute);

        connect(m_menuBar, &MenuBar::openRecentFileRequested, this,
                [this](const QString& filePath) {
                    if (m_documentController) {
                        m_documentController->openDocument(filePath);
                    }
                });

        connect(m_menuBar, &MenuBar::languageChanged, this,
                [this](const QString& languageCode) {
                    I18nManager::instance().loadLanguage(languageCode);
                    // The I18nManager will emit languageChanged signal which
                    // will trigger UI retranslation through changeEvent
                });
    }

    // Connect toolbar signals
    if (m_toolBar) {
        connect(m_toolBar, &ToolBar::actionTriggered, this,
                [this](ActionMap action) {
                    if (m_documentController) {
                        m_documentController->execute(action, m_mainWindow);
                    }
                });
    }

    // Connect welcome widget signals
    if (m_welcomeWidget) {
        connect(m_welcomeWidget, &WelcomeWidget::fileOpenRequested, this,
                [this](const QString& filePath) {
                    if (m_documentController) {
                        m_documentController->openDocument(filePath);
                    }
                });
    }

    // Connect document controller signals
    if (m_documentController) {
        // Sidebar control signals
        connect(m_documentController,
                &DocumentController::sideBarToggleRequested, this, [this]() {
                    if (m_sideBar) {
                        bool isVisible = m_sideBar->isVisible();
                        m_sideBar->QWidget::setVisible(!isVisible);
                    }
                });

        connect(m_documentController, &DocumentController::sideBarShowRequested,
                this, [this]() {
                    if (m_sideBar) {
                        m_sideBar->QWidget::setVisible(true);
                    }
                });

        connect(m_documentController, &DocumentController::sideBarHideRequested,
                this, [this]() {
                    if (m_sideBar) {
                        m_sideBar->QWidget::setVisible(false);
                    }
                });

        // Search control signals
        connect(m_documentController,
                &DocumentController::searchToggleRequested, this,
                [this](bool show) {
                    // TODO: Connect to search widget when implemented
                    QString message = QString("Search toggle requested: %1")
                                          .arg(show ? "show" : "hide");
                    m_logger.debug(message);
                });

        connect(m_documentController,
                &DocumentController::searchNavigationRequested, this,
                [this](bool forward) {
                    // TODO: Connect to search widget navigation
                    QString message =
                        QString("Search navigation requested: %1")
                            .arg(forward ? "forward" : "backward");
                    m_logger.debug(message);
                });

        connect(m_documentController, &DocumentController::searchClearRequested,
                this, [this]() {
                    // TODO: Connect to search widget clear
                    m_logger.debug("Search clear requested");
                });

        // Full screen toggle signal
        connect(m_documentController,
                &DocumentController::fullScreenToggleRequested, this, [this]() {
                    if (m_mainWindow) {
                        if (m_mainWindow->isFullScreen()) {
                            m_mainWindow->showNormal();
                        } else {
                            m_mainWindow->showFullScreen();
                        }
                    }
                });

        // Tab switch signal
        connect(m_documentController, &DocumentController::tabSwitchRequested,
                this, [this]() {
                    // TODO: Connect to tab widget when implemented
                    m_logger.debug("Tab switch requested");
                });

        // Theme toggle signal
        connect(
            m_documentController, &DocumentController::themeToggleRequested,
            this, [this]() {
                QString currentTheme =
                    (STYLE.currentTheme() == Theme::Light) ? "light" : "dark";
                QString newTheme = (currentTheme == "light") ? "dark" : "light";
                applyTheme(newTheme);
            });
    }
}

void ApplicationController::setupErrorHandling() {
    // Setup global error handling using existing utilities
    m_logger.debug("Setting up error handling and recovery system");

    // Register recovery actions for different error categories
    auto& recoveryManager = ErrorRecovery::RecoveryManager::instance();

    // Register document recovery action
    recoveryManager.registerRecoveryAction(
        ErrorHandling::ErrorCategory::Document,
        std::make_shared<ErrorRecovery::DocumentRecoveryAction>());

    // Register rendering recovery action
    recoveryManager.registerRecoveryAction(
        ErrorHandling::ErrorCategory::Rendering,
        std::make_shared<ErrorRecovery::RenderingRecoveryAction>());

    // Register search recovery action
    recoveryManager.registerRecoveryAction(
        ErrorHandling::ErrorCategory::Search,
        std::make_shared<ErrorRecovery::SearchRecoveryAction>());

    // Register file system recovery action
    recoveryManager.registerRecoveryAction(
        ErrorHandling::ErrorCategory::FileSystem,
        std::make_shared<ErrorRecovery::FileSystemRecoveryAction>());

    // Configure default retry policy
    ErrorRecovery::RetryConfig retryConfig;
    retryConfig.maxRetries = 3;
    retryConfig.policy = ErrorRecovery::RetryPolicy::ExponentialBackoff;
    retryConfig.initialDelay = std::chrono::milliseconds(100);
    retryConfig.maxDelay = std::chrono::milliseconds(5000);
    recoveryManager.setDefaultRetryConfig(retryConfig);

    m_logger.info("Error handling system configured successfully");
}

void ApplicationController::showWelcomeScreen() {
    if (m_contentStack) {
        m_contentStack->setCurrentIndex(0);
        emit viewChanged(true);
        m_logger.debug("Switched to welcome screen");
    }
}

void ApplicationController::showMainView() {
    if (m_contentStack) {
        m_contentStack->setCurrentIndex(1);
        emit viewChanged(false);
        m_logger.debug("Switched to main view");
    }
}

void ApplicationController::toggleView() {
    if (m_contentStack) {
        int current = m_contentStack->currentIndex();
        if (current == 0) {
            showMainView();
        } else {
            showWelcomeScreen();
        }
    }
}

void ApplicationController::applyTheme(const QString& theme) {
    m_logger.debug("Applying theme: " + theme);

    // Convert theme string to Theme enum and set in StyleManager
    Theme themeEnum = (theme.toLower() == "dark") ? Theme::Dark : Theme::Light;
    STYLE.setTheme(themeEnum);
    m_logger.debug("StyleManager theme set to: " + theme);

    // Get and apply application-wide stylesheet
    QString appStyleSheet = STYLE.getApplicationStyleSheet();
    qApp->setStyleSheet(appStyleSheet);
    m_logger.debug("Application stylesheet applied");

    // Apply specific stylesheets to components
    if (m_toolBar) {
        QString toolbarStyle = STYLE.getToolbarStyleSheet();
        m_toolBar->setStyleSheet(toolbarStyle);
        m_logger.debug("Toolbar stylesheet applied");
    }

    if (m_statusBar) {
        QString statusBarStyle = STYLE.getStatusBarStyleSheet();
        m_statusBar->setStyleSheet(statusBarStyle);
        m_logger.debug("StatusBar stylesheet applied");
    }

    if (m_viewWidget) {
        QString viewerStyle = STYLE.getPDFViewerStyleSheet();
        m_viewWidget->setStyleSheet(viewerStyle);
        m_logger.debug("ViewWidget stylesheet applied");
    }

    // Apply theme to welcome widget
    if (m_welcomeWidget) {
        m_welcomeWidget->applyTheme();
        m_logger.debug("WelcomeWidget theme applied");
    }

    m_logger.info("Theme application completed: " + theme);
}

void ApplicationController::handleError(const QString& context,
                                        const QString& error) {
    m_logger.error(QString("Error in %1: %2").arg(context, error));

    // Create error info for recovery attempt
    ErrorHandling::ErrorInfo errorInfo(
        ErrorHandling::ErrorCategory::Unknown,
        ErrorHandling::ErrorSeverity::Error, error,
        QString("Context: %1").arg(context), context);

    // Use UIErrorHandler for comprehensive error handling
    UIErrorHandler::instance().handleSystemError(m_mainWindow, errorInfo);

    // Attempt error recovery using RecoveryManager
    auto& recoveryManager = ErrorRecovery::RecoveryManager::instance();
    ErrorRecovery::RecoveryResult result = recoveryManager.executeRecovery(
        errorInfo, "ApplicationController", context);

    // Log recovery result
    switch (result) {
        case ErrorRecovery::RecoveryResult::Success:
            m_logger.info(QString("Successfully recovered from error in %1")
                              .arg(context));
            UIErrorHandler::instance().showFeedback(
                m_mainWindow, tr("Error recovered: %1").arg(context),
                UIErrorHandler::FeedbackType::Success);
            return;  // Don't emit error signal if recovered

        case ErrorRecovery::RecoveryResult::Retry:
            m_logger.info(
                QString("Error recovery suggests retry for %1").arg(context));
            UIErrorHandler::instance().showFeedback(
                m_mainWindow, tr("Retrying operation: %1").arg(context),
                UIErrorHandler::FeedbackType::Info);
            break;

        case ErrorRecovery::RecoveryResult::Fallback:
            m_logger.info(
                QString("Error recovery using fallback for %1").arg(context));
            break;

        case ErrorRecovery::RecoveryResult::Failed:
        case ErrorRecovery::RecoveryResult::Abort:
        default:
            m_logger.warning(
                QString("Error recovery failed for %1").arg(context));
            break;
    }

    // Emit error signal for UI notification
    emit errorOccurred(context, error);
}

void ApplicationController::shutdown() {
    if (m_isShuttingDown) {
        return;
    }

    m_isShuttingDown = true;
    m_logger.info("Shutting down application controller...");

    // Save application state before shutdown
    try {
        saveApplicationState();
    } catch (const std::exception& e) {
        m_logger.error(
            QString("Error saving application state during shutdown: %1")
                .arg(e.what()));
    }

    // Cleanup UI resources
    UIResourceManager::instance().cleanupAllResources();

    // Cleanup in reverse order of initialization
    // Connections are automatically cleaned up by Qt

    // Views are owned by MainWindow, no need to delete

    // Delete controllers
    delete m_documentController;
    delete m_pageController;

    // Delete models
    delete m_documentModel;
    delete m_pageModel;
    delete m_renderModel;

    // Delete managers
    delete m_recentFilesManager;
    delete m_welcomeScreenManager;

    // Shutdown system tray manager (singleton, don't delete)
    if (m_systemTrayManager) {
        m_systemTrayManager->shutdown();
        m_systemTrayManager = nullptr;
    }

    m_logger.info("Application controller shutdown complete");
}

void ApplicationController::onAsyncInitializationCompleted() {
    m_logger.debug("Async initialization completed");
    // Handle any post-initialization tasks
}

void ApplicationController::onComponentError(const QString& component,
                                             const QString& error) {
    handleError(component, error);
}

void ApplicationController::connectStatusBarSignals() {
    if (!m_statusBar || !m_viewWidget) {
        m_logger.warning(
            "StatusBar or ViewWidget not available for signal connections");
        return;
    }

    m_logger.info("Connecting StatusBar signals...");

    // Connect ViewWidget signals to StatusBar updates
    connect(m_viewWidget, &ViewWidget::currentViewerPageChanged, m_statusBar,
            [this](int pageNumber, int totalPages) {
                m_statusBar->setPageInfo(pageNumber, totalPages);
                LOG_DEBUG("StatusBar updated: page {}/{}", pageNumber + 1,
                          totalPages);
            });

    connect(m_viewWidget, &ViewWidget::currentViewerZoomChanged, m_statusBar,
            [this](double zoomFactor) {
                m_statusBar->setZoomLevel(zoomFactor);
                LOG_DEBUG("StatusBar updated: zoom {:.1f}%", zoomFactor * 100);
            });

    // Connect StatusBar page jump signal to ViewWidget
    connect(m_statusBar, &StatusBar::pageJumpRequested, m_viewWidget,
            [this](int pageNumber) {
                // Use ViewWidget's public interface to jump to page
                m_viewWidget->goToPage(pageNumber);
                LOG_DEBUG("Page jump requested: {}", pageNumber + 1);
            });

    // Connect StatusBar zoom change signal to ViewWidget
    connect(m_statusBar, &StatusBar::zoomLevelChangeRequested, m_viewWidget,
            [this](double zoomLevel) {
                // Use ViewWidget's public interface to set zoom
                m_viewWidget->setZoom(zoomLevel);
                LOG_DEBUG("Zoom change requested: {:.1f}%", zoomLevel * 100);
            });

    // Connect document model signals for file information updates
    if (m_documentModel) {
        connect(m_documentModel, &DocumentModel::documentOpened, this,
                [this](int index, const QString& fileName) {
                    Q_UNUSED(index)
                    updateStatusBarFromDocument();
                    m_statusBar->setSuccessMessage(
                        tr("Document opened: %1")
                            .arg(QFileInfo(fileName).fileName()));
                });

        connect(m_documentModel, &DocumentModel::documentClosed, this,
                [this](int index) {
                    Q_UNUSED(index)
                    if (m_documentModel->getDocumentCount() == 0) {
                        m_statusBar->clearDocumentInfo();
                        m_statusBar->setMessage(tr("No documents open"));
                    } else {
                        updateStatusBarFromDocument();
                    }
                });

        connect(m_documentModel, &DocumentModel::currentDocumentChanged, this,
                [this](int index) {
                    Q_UNUSED(index)
                    updateStatusBarFromDocument();
                });
    }

    m_logger.info("StatusBar signal connections established successfully");
}

void ApplicationController::updateStatusBarFromDocument() {
    if (!m_statusBar || !m_documentModel || !m_viewWidget) {
        return;
    }

    int currentIndex = m_documentModel->getCurrentDocumentIndex();
    if (currentIndex < 0) {
        m_statusBar->clearDocumentInfo();
        return;
    }

    QString fileName = m_documentModel->getDocumentFileName(currentIndex);
    if (fileName.isEmpty()) {
        m_statusBar->clearDocumentInfo();
        return;
    }

    // Get file size
    QFileInfo fileInfo(fileName);
    qint64 fileSize = fileInfo.exists() ? fileInfo.size() : 0;

    // Get current page and zoom from ViewWidget
    int currentPage = m_viewWidget->getCurrentPage();
    int totalPages = m_viewWidget->getCurrentPageCount();
    double zoomLevel = m_viewWidget->getCurrentZoom();

    // Update StatusBar with complete document information
    m_statusBar->setDocumentInfo(fileName, currentPage, totalPages, zoomLevel,
                                 fileSize);

    // Update document metadata if available
    if (auto* document = m_documentModel->getDocument(currentIndex)) {
        // Get metadata from document using Poppler's info() method
        QString title = document->info("Title");
        QString author = document->info("Author");
        QString subject = document->info("Subject");
        QString keywords = document->info("Keywords");

        QDateTime created, modified;
        QString createdStr = document->info("CreationDate");
        QString modifiedStr = document->info("ModDate");

        // Parse PDF date format if available
        if (!createdStr.isEmpty()) {
            // Try to parse PDF date format (D:YYYYMMDDHHmmSSOHH'mm')
            created = QDateTime::fromString(createdStr, Qt::ISODate);
            if (!created.isValid()) {
                // Try alternative parsing for PDF date format
                created = QDateTime::fromString(createdStr.mid(2, 14),
                                                "yyyyMMddHHmmss");
            }
        }
        if (!modifiedStr.isEmpty()) {
            modified = QDateTime::fromString(modifiedStr, Qt::ISODate);
            if (!modified.isValid()) {
                modified = QDateTime::fromString(modifiedStr.mid(2, 14),
                                                 "yyyyMMddHHmmss");
            }
        }

        // Fallback to file system dates if document dates not available
        if (!created.isValid()) {
            created = fileInfo.birthTime();
        }
        if (!modified.isValid()) {
            modified = fileInfo.lastModified();
        }

        m_statusBar->setDocumentMetadata(title, author, subject, keywords,
                                         created, modified);

        // Set document statistics (placeholder values - would need actual text
        // analysis) For now, estimate based on page count
        int estimatedWords = totalPages * 250;    // Rough estimate
        int estimatedChars = estimatedWords * 6;  // Rough estimate
        m_statusBar->setDocumentStatistics(estimatedWords, estimatedChars,
                                           totalPages);

        // Set security information (placeholder - would need actual PDF
        // security analysis)
        m_statusBar->setDocumentSecurity(
            false, true, true);  // Default: not encrypted, copy/print allowed
    }

    LOG_DEBUG("StatusBar updated from document: {} ({} pages, {:.1f}% zoom)",
              fileName.toStdString(), totalPages, zoomLevel * 100);
}

// State management integration methods

void ApplicationController::saveApplicationState() {
    m_logger.info("Saving application state...");

    UIStateManager& stateManager = UIStateManager::instance();

    // Save window state
    if (m_mainWindow) {
        stateManager.saveWindowState(m_mainWindow);
    }

    // Save splitter state
    if (m_mainSplitter) {
        stateManager.saveSplitterState(m_mainSplitter, "mainSplitter");
    }

    // Save all registered component states
    stateManager.saveAllComponentStates();

    // Save application-specific state
    stateManager.setState(
        "app/currentTheme",
        (STYLE.currentTheme() == Theme::Light) ? "light" : "dark");

    // Save document-related state
    if (m_documentModel) {
        int currentDoc = m_documentModel->getCurrentDocumentIndex();
        stateManager.setState("document/currentIndex", currentDoc);

        if (currentDoc >= 0) {
            QString fileName = m_documentModel->getDocumentFileName(currentDoc);
            stateManager.setState("document/currentFile", fileName);
        }
    }

    // Save view state
    if (m_viewWidget) {
        stateManager.setState("view/currentPage",
                              m_viewWidget->getCurrentPage());
        stateManager.setState("view/zoomLevel", m_viewWidget->getCurrentZoom());
    }

    // Force save to ensure persistence
    stateManager.forceSave();

    m_logger.info("Application state saved successfully");
}

void ApplicationController::restoreApplicationState() {
    m_logger.info("Restoring application state...");

    UIStateManager& stateManager = UIStateManager::instance();

    // Restore window state
    if (m_mainWindow) {
        stateManager.restoreWindowState(m_mainWindow);
    }

    // Restore splitter state
    if (m_mainSplitter) {
        stateManager.restoreSplitterState(m_mainSplitter, "mainSplitter");
    }

    // Restore theme
    QString savedTheme =
        stateManager.getState("app/currentTheme", "light").toString();
    applyTheme(savedTheme);

    // Restore all registered component states
    stateManager.restoreAllComponentStates();

    m_logger.info("Application state restored successfully");
}

void ApplicationController::registerUIComponents() {
    m_logger.info("Registering UI components for state management...");

    UIStateManager& stateManager = UIStateManager::instance();

    // Register main UI components
    if (m_menuBar) {
        stateManager.registerComponent(m_menuBar, "menuBar");
    }
    if (m_toolBar) {
        stateManager.registerComponent(m_toolBar, "toolBar");
    }
    if (m_sideBar) {
        stateManager.registerComponent(m_sideBar, "sideBar");
    }
    if (m_rightSideBar) {
        stateManager.registerComponent(m_rightSideBar, "rightSideBar");
    }
    if (m_statusBar) {
        stateManager.registerComponent(m_statusBar, "statusBar");
    }
    if (m_viewWidget) {
        stateManager.registerComponent(m_viewWidget, "viewWidget");
    }
    if (m_welcomeWidget) {
        stateManager.registerComponent(m_welcomeWidget, "welcomeWidget");
    }

    // Enable autosave with 30-second interval
    stateManager.enableAutosave(true, 30000);

    m_logger.info("UI components registered for state management");
}

void ApplicationController::registerUIResources() {
    m_logger.info("Registering UI resources for memory management...");

    UIResourceManager& resourceManager = UIResourceManager::instance();

    // Register main UI components for resource tracking
    if (m_menuBar) {
        resourceManager.registerWidget(m_menuBar, "Main MenuBar");
    }
    if (m_toolBar) {
        resourceManager.registerWidget(m_toolBar, "Main ToolBar");
    }
    if (m_sideBar) {
        resourceManager.registerWidget(m_sideBar, "Left SideBar");
    }
    if (m_rightSideBar) {
        resourceManager.registerWidget(m_rightSideBar, "Right SideBar");
    }
    if (m_statusBar) {
        resourceManager.registerWidget(m_statusBar, "Main StatusBar");
    }
    if (m_viewWidget) {
        resourceManager.registerWidget(m_viewWidget, "Document ViewWidget");
    }
    if (m_welcomeWidget) {
        resourceManager.registerWidget(m_welcomeWidget,
                                       "Welcome Screen Widget");
    }
    if (m_contentStack) {
        resourceManager.registerWidget(m_contentStack, "Content Stack Widget");
    }
    if (m_mainSplitter) {
        resourceManager.registerWidget(m_mainSplitter, "Main Splitter Widget");
    }

    // Configure resource management
    resourceManager.setAutoCleanupEnabled(true);
    resourceManager.setMemoryThreshold(150 * 1024 * 1024);  // 150MB threshold
    resourceManager.setCleanupInterval(120000);             // 2 minutes

    // Connect memory pressure signal
    connect(
        &resourceManager, &UIResourceManager::memoryThresholdExceeded, this,
        [this](qint64 current, qint64 threshold) {
            m_logger.warning(QString("Memory threshold exceeded: %1 MB / %2 MB")
                                 .arg(current / (1024 * 1024))
                                 .arg(threshold / (1024 * 1024)));
            optimizeResources();
        });

    m_logger.info("UI resources registered for memory management");
}

void ApplicationController::optimizeResources() {
    m_logger.info("Optimizing application resources...");

    UIResourceManager& resourceManager = UIResourceManager::instance();

    // Optimize memory usage
    resourceManager.optimizeMemoryUsage();

    // Clear document caches if no documents are open
    if (m_documentModel && m_documentModel->getDocumentCount() == 0) {
        // Clear render caches
        if (m_renderModel) {
            // Render model should have cache clearing methods
            m_logger.debug("Clearing render caches");
        }
    }

    // Force garbage collection
    QApplication::processEvents();

    qint64 memoryUsage = resourceManager.getTotalMemoryUsage();
    m_logger.info(
        QString("Resource optimization completed. Memory usage: %1 MB")
            .arg(memoryUsage / (1024 * 1024)));
}

void ApplicationController::registerUIConsistency() {
    m_logger.info("Registering UI components for visual consistency...");

    UIConsistencyManager& consistencyManager = UIConsistencyManager::instance();

    // Register main UI components for consistency checking
    if (m_menuBar) {
        consistencyManager.registerComponent(m_menuBar, "MenuBar");
    }
    if (m_toolBar) {
        consistencyManager.registerComponent(m_toolBar, "ToolBar");
    }
    if (m_sideBar) {
        consistencyManager.registerComponent(m_sideBar, "SideBar");
    }
    if (m_rightSideBar) {
        consistencyManager.registerComponent(m_rightSideBar, "RightSideBar");
    }
    if (m_statusBar) {
        consistencyManager.registerComponent(m_statusBar, "StatusBar");
    }
    if (m_viewWidget) {
        consistencyManager.registerComponent(m_viewWidget, "ViewWidget");
    }
    if (m_welcomeWidget) {
        consistencyManager.registerComponent(m_welcomeWidget, "WelcomeWidget");
    }

    // Configure consistency management
    consistencyManager.setConsistencyLevel(
        UIConsistencyManager::ConsistencyLevel::Moderate);
    consistencyManager.enableAutoCorrection(true);
    consistencyManager.enableContinuousValidation(true,
                                                  60000);  // Check every minute

    // Connect consistency signals
    connect(
        &consistencyManager, &UIConsistencyManager::validationCompleted, this,
        [this](UIConsistencyManager::ValidationResult result, int issueCount) {
            if (result != UIConsistencyManager::ValidationResult::Compliant) {
                m_logger.warning(QString("UI consistency validation found %1 "
                                         "issues (result: %2)")
                                     .arg(issueCount)
                                     .arg(static_cast<int>(result)));
            } else {
                m_logger.debug("UI consistency validation passed");
            }
        });

    connect(
        &consistencyManager, &UIConsistencyManager::consistencyIssueFound, this,
        [this](const UIConsistencyManager::StyleIssue& issue) {
            m_logger.debug(
                QString("Consistency issue: %1 - %2 (expected: %3, actual: %4)")
                    .arg(issue.component, issue.property, issue.expected,
                         issue.actual));
        });

    m_logger.info("UI components registered for visual consistency");
}

void ApplicationController::enforceVisualConsistency() {
    m_logger.info("Enforcing visual consistency across all components...");

    UIConsistencyManager& consistencyManager = UIConsistencyManager::instance();

    // Validate and enforce consistency
    auto result = consistencyManager.validateAllComponents();
    consistencyManager.enforceGlobalConsistency();

    m_logger.info(
        QString("Visual consistency enforcement completed (result: %1)")
            .arg(static_cast<int>(result)));
}
