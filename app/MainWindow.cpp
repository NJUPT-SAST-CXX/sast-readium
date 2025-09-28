#include "MainWindow.h"
#include <QApplication>
#include <QCloseEvent>
#include <QMessageBox>
#include "controller/ApplicationController.h"
#include "managers/SystemTrayManager.h"
#include "delegate/ViewDelegate.h"
#include "command/InitializationCommand.h"
#include "factory/ModelFactory.h"


MainWindow::MainWindow(QWidget* parent) 
    : QMainWindow(parent)
    , m_logger("MainWindow") 
{
    SLOG_TIMER("MainWindow::Constructor");
    
    m_logger.debug("Creating MainWindow with modular architecture...");
    
    // Set initial window properties
    setWindowTitle("SAST Readium");
    resize(1280, 800);
    
    try {
        // Create the application controller which manages all subsystems
        m_applicationController = std::make_unique<ApplicationController>(this, this);
        
        // Create view delegate for UI management
        m_viewDelegate = std::make_unique<ViewDelegate>(this, this);
        
        // Connect initialization signals
        connect(m_applicationController.get(), &ApplicationController::initializationCompleted,
                this, &MainWindow::onInitializationCompleted);
        connect(m_applicationController.get(), &ApplicationController::initializationFailed,
                this, &MainWindow::onInitializationFailed);
        connect(m_applicationController.get(), &ApplicationController::errorOccurred,
                this, &MainWindow::onApplicationError);
        
        // Use command pattern for initialization
        auto initCommand = InitializationCommandFactory::createFullInitializationSequence(
            m_applicationController.get());
        
        // Connect command progress signals
        connect(initCommand.get(), &InitializationCommand::executionProgress,
                this, [this](const QString& name, int progress) {
                    m_logger.debug(QString("%1: %2%").arg(name).arg(progress));
                });
        
        // Execute initialization
        if (!initCommand->execute()) {
            QString error = initCommand->errorMessage();
            m_logger.error("Initialization failed: " + error);
            onInitializationFailed(error);
        }
        
    } catch (const std::exception& e) {
        QString error = QString::fromStdString(e.what());
        m_logger.error("Failed to create application components: " + error);
        QMessageBox::critical(this, "Initialization Error", 
                            "Failed to initialize application: " + error);
    }
}

MainWindow::~MainWindow() noexcept {
    m_logger.debug("MainWindow destructor called");
    // Cleanup is handled by unique_ptr destructors
}

void MainWindow::closeEvent(QCloseEvent* event) {
    m_logger.debug("Close event received");

    // Check if we should minimize to tray instead of closing
    if (m_applicationController && m_applicationController->systemTrayManager()) {
        bool shouldMinimizeToTray = m_applicationController->systemTrayManager()->handleMainWindowCloseEvent();

        if (shouldMinimizeToTray) {
            m_logger.debug("Minimizing to system tray instead of closing");
            event->ignore(); // Ignore the close event
            return;
        }
    }

    m_logger.debug("Proceeding with normal application shutdown");

    // Save application state before closing
    if (m_viewDelegate) {
        m_viewDelegate->saveLayoutState();
    }

    // Shutdown application controller
    if (m_applicationController) {
        m_applicationController->shutdown();
    }

    event->accept();
}

void MainWindow::onInitializationCompleted() {
    m_logger.info("Application initialization completed successfully");
    
    // Setup view layout after successful initialization
    if (m_viewDelegate && m_applicationController) {
        // Configure view delegate with components from application controller
        m_viewDelegate->setSideBar(m_applicationController->sideBar());
        m_viewDelegate->setRightSideBar(m_applicationController->rightSideBar());
        m_viewDelegate->setViewWidget(m_applicationController->viewWidget());
        m_viewDelegate->setStatusBar(m_applicationController->statusBar());
        m_viewDelegate->setToolBar(m_applicationController->toolBar());
        m_viewDelegate->setMenuBar(m_applicationController->menuBar());
        
        // Setup and restore layout
        m_viewDelegate->setupMainLayout();
        m_viewDelegate->restoreLayoutState();
    }
}

void MainWindow::onInitializationFailed(const QString& error) {
    m_logger.error("Application initialization failed: " + error);
    
    QMessageBox::critical(this, "Initialization Failed",
                         "The application failed to initialize properly:\n\n" + error +
                         "\n\nThe application will now exit.");
    
    // Exit application on initialization failure
    QApplication::exit(1);
}

void MainWindow::onApplicationError(const QString& context, const QString& error) {
    m_logger.error(QString("Application error in %1: %2").arg(context, error));
    
    // Show error to user
    QMessageBox::warning(this, "Application Error",
                        QString("An error occurred in %1:\n\n%2").arg(context, error));
}
