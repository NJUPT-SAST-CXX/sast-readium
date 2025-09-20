#include "ApplicationController.h"
#include "../MainWindow.h"
#include <QMainWindow>
#include <QStackedWidget>
#include <QSplitter>
#include <QHBoxLayout>
#include "DocumentController.h"
#include "PageController.h"
#include "../model/DocumentModel.h"
#include "../model/PageModel.h"
#include "../model/RenderModel.h"
#include "../managers/RecentFilesManager.h"
#include "../managers/StyleManager.h"
#include "../managers/FileTypeIconManager.h"
#include "../ui/managers/WelcomeScreenManager.h"
#include "../ui/core/MenuBar.h"
#include "../ui/core/ToolBar.h"
#include "../ui/core/SideBar.h"
#include "../ui/core/RightSideBar.h"
#include "../ui/core/StatusBar.h"
#include "../ui/core/ViewWidget.h"
#include "../ui/widgets/WelcomeWidget.h"
#include "../factory/WidgetFactory.h"
#include "../utils/ErrorHandling.h"
#include "../utils/ErrorRecovery.h"

ApplicationController::ApplicationController(QMainWindow* mainWindow, QObject* parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
    , m_logger("ApplicationController")
{
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
        QString defaultTheme = (STYLE.currentTheme() == Theme::Light) ? "light" : "dark";
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
    m_logger.debug("Initializing models...");
    
    try {
        // Create models with proper error handling
        m_renderModel = new RenderModel(
            m_mainWindow->logicalDpiX(), 
            m_mainWindow->logicalDpiY()
        );
        
        m_documentModel = new DocumentModel(m_renderModel);
        m_pageModel = new PageModel(m_renderModel);
        
        // Create managers
        m_recentFilesManager = new RecentFilesManager(this);
        
        m_logger.debug("Models initialized successfully");
        
    } catch (const std::exception& e) {
        QString error = "Failed to initialize models: " + QString::fromStdString(e.what());
        m_logger.error(error);
        throw std::runtime_error(error.toStdString());
    }
}

void ApplicationController::initializeControllers() {
    SLOG_TIMER("ApplicationController::initializeControllers");
    m_logger.debug("Initializing controllers...");
    
    try {
        m_documentController = new DocumentController(m_documentModel);
        m_pageController = new PageController(m_pageModel);
        
        // Setup dependencies
        m_documentController->setRecentFilesManager(m_recentFilesManager);
        
        m_logger.debug("Controllers initialized successfully");
        
    } catch (const std::exception& e) {
        QString error = "Failed to initialize controllers: " + QString::fromStdString(e.what());
        m_logger.error(error);
        throw std::runtime_error(error.toStdString());
    }
}

void ApplicationController::initializeViews() {
    SLOG_TIMER("ApplicationController::initializeViews");
    m_logger.debug("Initializing views...");
    
    try {
        // Create factory for widget creation
        WidgetFactory* factory = new WidgetFactory(m_pageController, m_mainWindow);
        
        // Create UI components
        m_menuBar = new MenuBar(m_mainWindow);
        m_toolBar = new ToolBar(m_mainWindow);
        m_sideBar = new SideBar(m_mainWindow);
        m_rightSideBar = new RightSideBar(m_mainWindow);
        m_statusBar = new StatusBar(factory, m_mainWindow);
        m_viewWidget = new ViewWidget(m_mainWindow);
        
        // Configure components
        m_menuBar->setRecentFilesManager(m_recentFilesManager);
        m_viewWidget->setDocumentController(m_documentController);
        m_viewWidget->setDocumentModel(m_documentModel);
        
        // Setup main window
        m_mainWindow->setMenuBar(m_menuBar);
        m_mainWindow->addToolBar(m_toolBar);
        m_mainWindow->setStatusBar(m_statusBar);
        
        // Initialize welcome screen
        FILE_ICON_MANAGER.preloadIcons();
        m_welcomeWidget = new WelcomeWidget(m_mainWindow);
        m_welcomeWidget->setRecentFilesManager(m_recentFilesManager);
        
        m_welcomeScreenManager = new WelcomeScreenManager(m_mainWindow);
        m_welcomeScreenManager->setMainWindow(qobject_cast<MainWindow*>(m_mainWindow.data()));
        m_welcomeScreenManager->setWelcomeWidget(m_welcomeWidget);
        m_welcomeScreenManager->setDocumentModel(m_documentModel);
        m_welcomeWidget->setWelcomeScreenManager(m_welcomeScreenManager);
        m_welcomeWidget->applyTheme();
        
        // Create stacked widget for view switching
        m_contentStack = new QStackedWidget(m_mainWindow);
        
        // Create main viewer area
        QWidget* mainViewerWidget = new QWidget();
        QHBoxLayout* mainViewerLayout = new QHBoxLayout(mainViewerWidget);
        mainViewerLayout->setContentsMargins(0, 0, 0, 0);
        
        QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, mainViewerWidget);
        mainSplitter->addWidget(m_sideBar);
        mainSplitter->addWidget(m_viewWidget);
        mainSplitter->addWidget(m_rightSideBar);
        mainSplitter->setCollapsible(0, true);
        mainSplitter->setCollapsible(1, false);
        mainSplitter->setCollapsible(2, true);
        mainSplitter->setStretchFactor(1, 1);
        
        int leftWidth = m_sideBar->isVisible() ? m_sideBar->getPreferredWidth() : 0;
        int rightWidth = m_rightSideBar->isVisible() ? m_rightSideBar->getPreferredWidth() : 0;
        mainSplitter->setSizes({leftWidth, 1000, rightWidth});
        
        mainViewerLayout->addWidget(mainSplitter);
        
        // Add views to stack
        m_contentStack->addWidget(m_welcomeWidget);
        m_contentStack->addWidget(mainViewerWidget);
        
        m_mainWindow->setCentralWidget(m_contentStack);
        
        // Set initial view
        if (m_welcomeScreenManager && m_welcomeScreenManager->shouldShowWelcomeScreen()) {
            showWelcomeScreen();
        } else {
            showMainView();
        }
        
        m_logger.debug("Views initialized successfully");
        
    } catch (const std::exception& e) {
        QString error = "Failed to initialize views: " + QString::fromStdString(e.what());
        m_logger.error(error);
        throw std::runtime_error(error.toStdString());
    }
}

void ApplicationController::initializeConnections() {
    SLOG_TIMER("ApplicationController::initializeConnections");
    m_logger.debug("Initializing connections...");
    
    try {
        connectModelSignals();
        connectControllerSignals();
        connectViewSignals();
        
        m_logger.debug("Connections initialized successfully");
        
    } catch (const std::exception& e) {
        QString error = "Failed to initialize connections: " + QString::fromStdString(e.what());
        m_logger.error(error);
        throw std::runtime_error(error.toStdString());
    }
}

void ApplicationController::connectModelSignals() {
    // Connect model signals for state changes
    if (m_documentModel) {
        connect(m_documentModel, &DocumentModel::documentOpened,
                this, [this](int index, const QString& fileName) {
                    showMainView();
                    m_logger.debug("Document opened: " + fileName + ", switching to main view");
                });
    }
}

void ApplicationController::connectControllerSignals() {
    // Connect controller signals
    if (m_documentController) {
        connect(m_documentController, &DocumentController::documentOperationCompleted,
                this, [this](ActionMap action, bool success) {
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
        connect(m_menuBar, &MenuBar::themeChanged, 
                this, &ApplicationController::applyTheme);
        
        connect(m_menuBar, &MenuBar::onExecuted,
                m_documentController, &DocumentController::execute);
        
        connect(m_menuBar, &MenuBar::openRecentFileRequested,
                this, [this](const QString& filePath) {
                    if (m_documentController) {
                        m_documentController->openDocument(filePath);
                    }
                });
    }
    
    // Connect toolbar signals
    if (m_toolBar) {
        connect(m_toolBar, &ToolBar::actionTriggered,
                this, [this](ActionMap action) {
                    if (m_documentController) {
                        m_documentController->execute(action, m_mainWindow);
                    }
                });
    }
    
    // Connect welcome widget signals
    if (m_welcomeWidget) {
        connect(m_welcomeWidget, &WelcomeWidget::fileOpenRequested,
                this, [this](const QString& filePath) {
                    if (m_documentController) {
                        m_documentController->openDocument(filePath);
                    }
                });
    }
}

void ApplicationController::setupErrorHandling() {
    // Setup global error handling using existing utilities
    // Setup global error handling
    // Note: ErrorHandler needs to be implemented or use existing error utilities
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
    
    // Apply theme to all components
    if (m_welcomeWidget) {
        m_welcomeWidget->applyTheme();
    }
    
    // Additional theme application logic can be added here
}

void ApplicationController::handleError(const QString& context, const QString& error) {
    m_logger.error(QString("Error in %1: %2").arg(context, error));
    
    // Use error recovery utilities
    // Attempt error recovery if available
    // ErrorRecovery::attemptRecovery(context, error);
    
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
    
    m_logger.info("Application controller shutdown complete");
}

void ApplicationController::onAsyncInitializationCompleted() {
    m_logger.debug("Async initialization completed");
    // Handle any post-initialization tasks
}

void ApplicationController::onComponentError(const QString& component, const QString& error) {
    handleError(component, error);
}
