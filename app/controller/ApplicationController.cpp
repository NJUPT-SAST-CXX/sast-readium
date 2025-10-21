#include "ApplicationController.h"
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMessageBox>
#include <QPixmap>
#include <QSplitter>
#include <QStackedWidget>
#include <iostream>
#include "../MainWindow.h"
#include "../factory/WidgetFactory.h"
#include "../managers/FileTypeIconManager.h"
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
#include "../ui/core/ViewWidget.h"
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

        // Start async operations
        if (m_recentFilesManager) {
            m_recentFilesManager->initializeAsync();
            m_logger.debug("Async initialization started");
        }

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
        WidgetFactory* factory =
            new WidgetFactory(m_pageController, m_mainWindow);
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
            m_statusBar = new StatusBar(factory, m_mainWindow);
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

            connect(m_systemTrayManager,
                    &SystemTrayManager::settingsDialogRequested, this,
                    [this]() {
                        m_logger.debug(
                            "Settings dialog requested from system tray");
                        // Show simple settings information dialog
                        // TODO: Replace with full settings dialog when
                        // implemented
                        QMessageBox::information(
                            m_mainWindow, tr("Settings"),
                            tr("Settings dialog will be available in a future "
                               "version.\n\n"
                               "Current settings are managed through:\n"
                               "- Theme menu (View → Theme)\n"
                               "- Language menu (View → Language)\n"
                               "- Configuration files in application data "
                               "directory"));
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
        m_logger.debug("Content stack widget created");

        // Create main viewer area
        m_logger.debug("Creating main viewer area...");
        QWidget* mainViewerWidget = new QWidget();
        QHBoxLayout* mainViewerLayout = new QHBoxLayout(mainViewerWidget);
        mainViewerLayout->setContentsMargins(0, 0, 0, 0);

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

        // Set handle width for better usability
        m_mainSplitter->setHandleWidth(1);

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
                    showMainView();
                    m_logger.debug("Document opened: " + fileName +
                                   ", switching to main view");
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

    // Attempt error recovery using RecoveryManager
    auto& recoveryManager = ErrorRecovery::RecoveryManager::instance();
    ErrorRecovery::RecoveryResult result = recoveryManager.executeRecovery(
        errorInfo, "ApplicationController", context);

    // Log recovery result
    switch (result) {
        case ErrorRecovery::RecoveryResult::Success:
            m_logger.info(QString("Successfully recovered from error in %1")
                              .arg(context));
            return;  // Don't emit error signal if recovered

        case ErrorRecovery::RecoveryResult::Retry:
            m_logger.info(
                QString("Error recovery suggests retry for %1").arg(context));
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
