#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QStackedWidget>
#include "controller/DocumentController.h"
#include "controller/PageController.h"
#include "controller/tool.hpp"
#include "factory/WidgetFactory.h"
#include "managers/RecentFilesManager.h"
#include "managers/StyleManager.h"
#include "model/DocumentModel.h"
#include "model/PageModel.h"
#include "model/RenderModel.h"
#include "ui/core/MenuBar.h"
#include "ui/core/RightSideBar.h"
#include "ui/core/SideBar.h"
#include "ui/core/StatusBar.h"
#include "ui/core/ToolBar.h"
#include "ui/core/ViewWidget.h"
#include "ui/managers/WelcomeScreenManager.h"
#include "ui/widgets/WelcomeWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() noexcept;

private slots:
    void applyTheme(const QString& theme);
    void onDocumentOperationCompleted(ActionMap action, bool success);
    void onSideBarVisibilityChanged(bool visible);
    void onSplitterMoved(int pos, int index);
    void onCurrentDocumentChangedForOutline(int index);
    void updateStatusBarInfo();
    void onViewModeChangeRequested(int mode);
    void onPageJumpRequested(int pageNumber);
    void onThumbnailPageClicked(int pageNumber);
    void onThumbnailPageDoubleClicked(int pageNumber);
    void onPDFActionRequested(ActionMap action);
    void onThemeToggleRequested();
    void onOpenRecentFileRequested(const QString& filePath);
    void handleActionExecuted(ActionMap id);

    // Welcome screen slots
    void onWelcomeScreenShowRequested();
    void onWelcomeScreenHideRequested();
    void onWelcomeFileOpenRequested(const QString& filePath);
    void onWelcomeNewFileRequested();
    void onWelcomeOpenFileRequested();

private:
    void initWindow();
    void initContent();
    void initModel();
    void initController();
    void initConnection();
    void initWelcomeScreen();
    void initWelcomeScreenConnections();

    MenuBar* menuBar;
    ToolBar* toolBar;
    SideBar* sideBar;
    RightSideBar* rightSideBar;
    StatusBar* statusBar;
    ViewWidget* viewWidget;

    QSplitter* mainSplitter;

    // Welcome screen components
    QStackedWidget* m_contentStack;
    WelcomeWidget* m_welcomeWidget;
    WelcomeScreenManager* m_welcomeScreenManager;

    DocumentController* documentController;
    PageController* pageController;

    DocumentModel* documentModel;
    PageModel* pageModel;
    RenderModel* renderModel;

    RecentFilesManager* recentFilesManager;

    // Theme state tracking
    QString m_currentAppliedTheme;

signals:
    void pdfViewerActionRequested(ActionMap action);
};
