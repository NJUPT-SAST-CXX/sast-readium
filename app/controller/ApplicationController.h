#pragma once

#include <QObject>
#include <QPointer>
#include "../logging/SimpleLogging.h"
#include "../ui/core/UIConsistencyManager.h"
#include "../ui/core/UIResourceManager.h"
#include "../ui/core/UIStateManager.h"

// Forward declarations
class QMainWindow;
class QStackedWidget;
class QSplitter;
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
    ~ApplicationController() override;

    // Explicitly delete copy operations due to destructor
    ApplicationController(const ApplicationController&) = delete;
    ApplicationController& operator=(const ApplicationController&) = delete;
    ApplicationController(ApplicationController&&) = delete;
    ApplicationController& operator=(ApplicationController&&) = delete;

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
    [[nodiscard]] DocumentController* documentController() const {
        return m_documentController;
    }
    [[nodiscard]] PageController* pageController() const {
        return m_pageController;
    }
    [[nodiscard]] DocumentModel* documentModel() const {
        return m_documentModel;
    }
    [[nodiscard]] PageModel* pageModel() const { return m_pageModel; }
    [[nodiscard]] RenderModel* renderModel() const { return m_renderModel; }
    [[nodiscard]] RecentFilesManager* recentFilesManager() const {
        return m_recentFilesManager;
    }
    [[nodiscard]] SystemTrayManager* systemTrayManager() const {
        return m_systemTrayManager;
    }

    // View components access
    [[nodiscard]] MenuBar* menuBar() const { return m_menuBar; }
    [[nodiscard]] ToolBar* toolBar() const { return m_toolBar; }
    [[nodiscard]] SideBar* sideBar() const { return m_sideBar; }
    [[nodiscard]] RightSideBar* rightSideBar() const { return m_rightSideBar; }
    [[nodiscard]] StatusBar* statusBar() const { return m_statusBar; }

    // StatusBar integration methods
    void connectStatusBarSignals();
    void updateStatusBarFromDocument();
    [[nodiscard]] ViewWidget* viewWidget() const { return m_viewWidget; }
    [[nodiscard]] QSplitter* mainSplitter() const { return m_mainSplitter; }

    // Application-wide operations
    void applyTheme(const QString& theme);
    void handleError(const QString& context, const QString& error);
    void shutdown();

    // State management integration
    void saveApplicationState();
    void restoreApplicationState();
    void registerUIComponents();

    // Resource management integration
    void registerUIResources();
    void optimizeResources();

    // Visual consistency integration
    void registerUIConsistency();
    void enforceVisualConsistency();

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
    void applyRenderingSettingsFromConfig();

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
    QSplitter* m_mainSplitter = nullptr;

    // State
    bool m_isInitialized = false;
    bool m_isShuttingDown = false;

    // Logging
    SastLogging::CategoryLogger m_logger;
};
