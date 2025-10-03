#pragma once

#include <QObject>
#include <QPointer>
#include <memory>
#include "../logging/SimpleLogging.h"

// Forward declarations
class QMainWindow;
class QStackedWidget;
class DocumentController;
class PageController;
class DocumentModel;
class PageModel;
class RenderModel;
class RecentFilesManager;
class WelcomeScreenManager;
class SystemTrayManager;
class MenuBar;
class ToolBar;
class SideBar;
class RightSideBar;
class StatusBar;
class ViewWidget;
class WelcomeWidget;
class StyleManager;

/**
 * @brief ApplicationController - Central coordinator for the application
 *
 * This controller follows the Single Responsibility Principle by managing
 * only the coordination between different subsystems of the application.
 * It does not contain business logic but orchestrates the interaction
 * between controllers, models, and views.
 */
class ApplicationController : public QObject {
    Q_OBJECT

public:
    explicit ApplicationController(QMainWindow* mainWindow,
                                   QObject* parent = nullptr);
    ~ApplicationController();

    // Initialization methods (following Interface Segregation)
    void initializeApplication();
    void initializeModels();
    void initializeControllers();
    void initializeViews();
    void initializeConnections();

    // State management
    void showWelcomeScreen();
    void showMainView();
    void toggleView();

    // Component access (for dependency injection)
    DocumentController* documentController() const {
        return m_documentController;
    }
    PageController* pageController() const { return m_pageController; }
    DocumentModel* documentModel() const { return m_documentModel; }
    PageModel* pageModel() const { return m_pageModel; }
    RenderModel* renderModel() const { return m_renderModel; }
    RecentFilesManager* recentFilesManager() const {
        return m_recentFilesManager;
    }
    SystemTrayManager* systemTrayManager() const { return m_systemTrayManager; }

    // View components access
    MenuBar* menuBar() const { return m_menuBar; }
    ToolBar* toolBar() const { return m_toolBar; }
    SideBar* sideBar() const { return m_sideBar; }
    RightSideBar* rightSideBar() const { return m_rightSideBar; }
    StatusBar* statusBar() const { return m_statusBar; }
    ViewWidget* viewWidget() const { return m_viewWidget; }

    // Application-wide operations
    void applyTheme(const QString& theme);
    void handleError(const QString& context, const QString& error);
    void shutdown();

signals:
    void initializationCompleted();
    void initializationFailed(const QString& error);
    void viewChanged(bool isWelcomeScreen);
    void errorOccurred(const QString& context, const QString& error);

private slots:
    void onAsyncInitializationCompleted();
    void onComponentError(const QString& component, const QString& error);

private:
    // Helper methods
    void connectModelSignals();
    void connectControllerSignals();
    void connectViewSignals();
    void setupErrorHandling();

    // Main window reference
    QPointer<QMainWindow> m_mainWindow;

    // Models (owned by this controller)
    RenderModel* m_renderModel = nullptr;
    DocumentModel* m_documentModel = nullptr;
    PageModel* m_pageModel = nullptr;

    // Controllers (owned by this controller)
    DocumentController* m_documentController = nullptr;
    PageController* m_pageController = nullptr;

    // Managers
    RecentFilesManager* m_recentFilesManager = nullptr;
    WelcomeScreenManager* m_welcomeScreenManager = nullptr;
    SystemTrayManager* m_systemTrayManager = nullptr;

    // View components (owned by main window)
    MenuBar* m_menuBar = nullptr;
    ToolBar* m_toolBar = nullptr;
    SideBar* m_sideBar = nullptr;
    RightSideBar* m_rightSideBar = nullptr;
    StatusBar* m_statusBar = nullptr;
    ViewWidget* m_viewWidget = nullptr;
    WelcomeWidget* m_welcomeWidget = nullptr;
    QStackedWidget* m_contentStack = nullptr;

    // State
    bool m_isInitialized = false;
    bool m_isShuttingDown = false;

    // Logging
    SastLogging::CategoryLogger m_logger;
};
