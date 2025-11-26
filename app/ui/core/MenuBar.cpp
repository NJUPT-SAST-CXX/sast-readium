#include "MenuBar.h"

// ElaWidgetTools
#include "ElaIcon.h"
#include "ElaMenu.h"

// Business Logic
#include "logging/SimpleLogging.h"
#include "managers/FileTypeIconManager.h"
#include "managers/I18nManager.h"
#include "managers/RecentFilesManager.h"
#include "managers/StyleManager.h"

// Qt
#include <QAction>
#include <QActionGroup>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QKeySequence>
#include <QVBoxLayout>

// Widgets
#include "ElaContentDialog.h"
#include "ElaText.h"
#include "ui/widgets/ToastNotification.h"

MenuBar::MenuBar(QWidget* parent)
    : ::ElaMenuBar(parent),
      m_fileMenu(nullptr),
      m_tabMenu(nullptr),
      m_viewMenu(nullptr),
      m_themeMenu(nullptr),
      m_helpMenu(nullptr),
      m_recentFilesMenu(nullptr),
      m_exportMenu(nullptr),
      m_zoomMenu(nullptr),
      m_rotateMenu(nullptr),
      m_viewModeMenu(nullptr),
      m_sideBarMenu(nullptr),
      m_languageMenu(nullptr),
      m_openAction(nullptr),
      m_openFolderAction(nullptr),
      m_closeAction(nullptr),
      m_closeAllAction(nullptr),
      m_saveAction(nullptr),
      m_saveAsAction(nullptr),
      m_exportAction(nullptr),
      m_printAction(nullptr),
      m_propertiesAction(nullptr),
      m_reloadAction(nullptr),
      m_exitAction(nullptr),
      m_clearRecentFilesAction(nullptr),
      m_newTabAction(nullptr),
      m_closeTabAction(nullptr),
      m_closeOtherTabsAction(nullptr),
      m_closeAllTabsAction(nullptr),
      m_nextTabAction(nullptr),
      m_prevTabAction(nullptr),
      m_zoomInAction(nullptr),
      m_zoomOutAction(nullptr),
      m_zoomResetAction(nullptr),
      m_fitWidthAction(nullptr),
      m_fitPageAction(nullptr),
      m_fitHeightAction(nullptr),
      m_rotateLeftAction(nullptr),
      m_rotateRightAction(nullptr),
      m_resetRotationAction(nullptr),
      m_singlePageAction(nullptr),
      m_continuousAction(nullptr),
      m_twoPageAction(nullptr),
      m_bookModeAction(nullptr),
      m_toggleLeftSideBarAction(nullptr),
      m_toggleRightSideBarAction(nullptr),
      m_showSideBarAction(nullptr),
      m_hideSideBarAction(nullptr),
      m_toggleToolBarAction(nullptr),
      m_toggleStatusBarAction(nullptr),
      m_fullScreenAction(nullptr),
      m_presentationModeAction(nullptr),
      m_welcomeScreenToggleAction(nullptr),
      m_debugPanelToggleAction(nullptr),
      m_debugPanelClearAction(nullptr),
      m_debugPanelExportAction(nullptr),
      m_lightThemeAction(nullptr),
      m_darkThemeAction(nullptr),
      m_autoThemeAction(nullptr),
      m_englishAction(nullptr),
      m_chineseAction(nullptr),
      m_helpAction(nullptr),
      m_aboutAction(nullptr),
      m_checkUpdatesAction(nullptr),
      m_recentFilesManager(nullptr),
      m_documentOpened(false) {
    SLOG_INFO("MenuBar: Constructor started");

    // 创建所有菜单
    createFileMenu();
    createTabMenu();
    createViewMenu();
    createThemeMenu();
    createHelpMenu();

    // 初始化状态
    updateMenuStates();

    SLOG_INFO("MenuBar: Constructor completed");
}

MenuBar::~MenuBar() { SLOG_INFO("MenuBar: Destructor called"); }

void MenuBar::setRecentFilesManager(RecentFilesManager* manager) {
    m_recentFilesManager = manager;

    if (m_recentFilesManager) {
        // 连接信号
        connect(m_recentFilesManager, &RecentFilesManager::recentFilesChanged,
                this, &MenuBar::updateRecentFilesMenu);

        // 初始更新
        updateRecentFilesMenu();
    }
}

void MenuBar::setDocumentOpened(bool opened) {
    m_documentOpened = opened;
    updateMenuStates();
}

void MenuBar::updateRecentFilesMenu() {
    if (!m_recentFilesMenu || !m_recentFilesManager) {
        return;
    }

    // 清除现有项
    clearRecentFilesMenu();

    // 获取最近文件列表
    QStringList recentFiles = m_recentFilesManager->getRecentFilePaths();

    if (recentFiles.isEmpty()) {
        m_recentFilesMenu->setEnabled(false);
        QAction* emptyAction =
            m_recentFilesMenu->addAction(tr("No Recent Files"));
        emptyAction->setEnabled(false);
        return;
    }

    m_recentFilesMenu->setEnabled(true);

    // 添加最近文件 (up to 10 files)
    for (int i = 0; i < recentFiles.size() && i < 10; ++i) {
        const QString& filePath = recentFiles[i];

        // 创建显示文本：序号 + 智能截断的路径
        QString displayText = QString("&%1 ").arg(i + 1);

        // Intelligent path truncation: show filename + parent folder if too
        // long
        QFileInfo qFileInfo(filePath);
        QString fileName = qFileInfo.fileName();
        QString parentDir = qFileInfo.dir().dirName();

        // Build display text: "...parentDir/filename.pdf"
        QString pathDisplay;
        if (!parentDir.isEmpty() && parentDir != ".") {
            pathDisplay = QString("...%1/%2").arg(parentDir).arg(fileName);
        } else {
            pathDisplay = fileName;
        }

        displayText += pathDisplay;

        // Truncate if still too long (max 60 chars)
        if (displayText.length() > 60) {
            displayText = displayText.left(57) + "...";
        }

        QAction* action = m_recentFilesMenu->addAction(displayText);
        action->setData(filePath);
        action->setToolTip(filePath);

        // Add file type icon
        QIcon fileIcon =
            FileTypeIconManager::instance().getFileTypeIcon(filePath, 16);
        action->setIcon(fileIcon);

        // 设置快捷键 (Ctrl+1 到 Ctrl+9, Ctrl+0)
        if (i < 9) {
            action->setShortcut(QKeySequence(QString("Ctrl+%1").arg(i + 1)));
        } else if (i == 9) {
            action->setShortcut(QKeySequence("Ctrl+0"));
        }

        connect(action, &QAction::triggered, this,
                &MenuBar::onRecentFileTriggered);

        m_recentFileActions.append(action);
    }

    // 添加分隔符和清除动作
    m_recentFilesMenu->addSeparator();
    m_recentFilesMenu->addAction(m_clearRecentFilesAction);
}

void MenuBar::onRecentFileTriggered() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        QString filePath = action->data().toString();
        if (!filePath.isEmpty()) {
            // 检查文件是否仍然存在
            QFileInfo fileInfo(filePath);
            if (fileInfo.exists()) {
                emit openRecentFileRequested(filePath);
            } else {
                // 文件不存在，显示用户友好的错误消息
                QString fileName = fileInfo.fileName();
                TOAST_WARNING(
                    this,
                    tr("The file \"%1\" could not be found.\n\n"
                       "It may have been moved, renamed, or deleted.\n"
                       "The file has been removed from the recent files list.")
                        .arg(fileName));

                // 从列表中移除
                if (m_recentFilesManager) {
                    m_recentFilesManager->removeRecentFile(filePath);
                }
            }
        }
    }
}

void MenuBar::onClearRecentFilesTriggered() {
    if (!m_recentFilesManager || !m_recentFilesManager->hasRecentFiles()) {
        return;
    }

    // Show confirmation dialog
    auto* dialog = new ElaContentDialog(this);
    dialog->setWindowTitle(tr("Clear Recent Files"));
    auto* w = new QWidget(dialog);
    auto* l = new QVBoxLayout(w);
    l->addWidget(
        new ElaText(tr("Are you sure you want to clear all recent files?\n\n"
                       "This action cannot be undone."),
                    w));
    dialog->setCentralWidget(w);
    dialog->setLeftButtonText(tr("Cancel"));
    dialog->setRightButtonText(tr("Clear"));

    bool confirmed = false;
    connect(dialog, &ElaContentDialog::rightButtonClicked, this,
            [&confirmed, dialog]() {
                confirmed = true;
                dialog->close();
            });
    connect(dialog, &ElaContentDialog::leftButtonClicked, dialog,
            &ElaContentDialog::close);
    dialog->exec();
    dialog->deleteLater();

    if (confirmed) {
        m_recentFilesManager->clearRecentFiles();
    }
}

void MenuBar::setActionEnabled(ActionMap action, bool enabled) {
    if (m_actionMap.contains(action)) {
        m_actionMap[action]->setEnabled(enabled);
    }
}

void MenuBar::setWelcomeScreenEnabled(bool enabled) {
    if (m_welcomeScreenToggleAction) {
        m_welcomeScreenToggleAction->setChecked(enabled);
    }
}

void MenuBar::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    ::ElaMenuBar::changeEvent(event);  // Call base class, not self
}

void MenuBar::createFileMenu() {
    m_fileMenu = new ElaMenu(tr("&File"), this);
    addMenu(m_fileMenu);

    // 打开
    m_openAction =
        createAction(tr("&Open..."), "Ctrl+O", ActionMap::openFile, "Folder");
    m_fileMenu->addAction(m_openAction);

    // 打开文件夹
    m_openFolderAction = createAction(tr("Open &Folder..."), "Ctrl+Shift+O",
                                      ActionMap::openFolder, "FolderOpen");
    m_fileMenu->addAction(m_openFolderAction);

    // 最近文件
    setupRecentFilesMenu();

    m_fileMenu->addSeparator();

    // 关闭
    m_closeAction = createAction(tr("&Close"), "Ctrl+W", ActionMap::closeFile,
                                 "Xmark");  // closeDocument -> closeFile
    m_fileMenu->addAction(m_closeAction);

    // 关闭所有
    m_closeAllAction = createAction(
        tr("Close &All"), "Ctrl+Shift+W",
        ActionMap::closeAllTabs);  // closeAllDocuments -> closeAllTabs
    m_fileMenu->addAction(m_closeAllAction);

    m_fileMenu->addSeparator();

    // 保存副本
    m_saveAsAction =
        createAction(tr("&Save Copy As..."), "Ctrl+Shift+S", ActionMap::saveAs,
                     "FloppyDisk");  // saveDocumentCopy -> saveAs
    m_fileMenu->addAction(m_saveAsAction);

    // 导出
    m_exportMenu = new ElaMenu(tr("&Export"), m_fileMenu);
    m_exportMenu->setIcon(ELA_ICON(FileExport));
    m_fileMenu->addMenu(m_exportMenu);

    QAction* exportPdfAction =
        createAction(tr("As PDF..."), "",
                     ActionMap::exportFile);  // exportDocument -> exportFile
    m_exportMenu->addAction(exportPdfAction);

    QAction* exportTextAction =
        createAction(tr("As Text..."), "",
                     ActionMap::exportFile);  // exportAsText -> exportFile
    m_exportMenu->addAction(exportTextAction);

    QAction* exportImagesAction =
        createAction(tr("Extract Images..."), "",
                     ActionMap::exportFile);  // exportImages -> exportFile
    m_exportMenu->addAction(exportImagesAction);

    m_fileMenu->addSeparator();

    // 打印
    m_printAction =
        createAction(tr("&Print..."), "Ctrl+P", ActionMap::printFile,
                     "Print");  // printDocument -> printFile
    m_fileMenu->addAction(m_printAction);

    m_fileMenu->addSeparator();

    // 属性
    m_propertiesAction =
        createAction(tr("P&roperties"), "Alt+Return",
                     ActionMap::showDocumentMetadata, "CircleInfo");
    m_fileMenu->addAction(m_propertiesAction);

    // 重新加载
    m_reloadAction =
        createAction(tr("&Reload"), "F5", ActionMap::reloadFile,
                     "ArrowsRotate");  // reloadDocument -> reloadFile
    m_fileMenu->addAction(m_reloadAction);

    m_fileMenu->addSeparator();

    // 退出
    m_exitAction = createAction(tr("E&xit"), "Alt+F4", ActionMap::quit,
                                "RightFromBracket");  // exitApplication -> quit
    m_fileMenu->addAction(m_exitAction);
}

void MenuBar::createTabMenu() {
    m_tabMenu = new ElaMenu(tr("&Tabs"), this);
    addMenu(m_tabMenu);

    // 新建标签页
    m_newTabAction =
        createAction(tr("&New Tab"), "Ctrl+T", ActionMap::newTab, "Plus");
    m_tabMenu->addAction(m_newTabAction);

    m_tabMenu->addSeparator();

    // 关闭标签页
    m_closeTabAction =
        createAction(tr("&Close Tab"), "Ctrl+W", ActionMap::closeTab);
    m_tabMenu->addAction(m_closeTabAction);

    // 注释掉不存在的 closeOtherTabs
    // m_closeOtherTabsAction = createAction(tr("Close &Other Tabs"), "",
    //                                      ActionMap::closeOtherTabs);
    // m_tabMenu->addAction(m_closeOtherTabsAction);

    // 关闭所有标签页
    m_closeAllTabsAction = createAction(tr("Close &All Tabs"), "Ctrl+Shift+W",
                                        ActionMap::closeAllTabs);
    m_tabMenu->addAction(m_closeAllTabsAction);

    m_tabMenu->addSeparator();

    // 下一个标签页
    m_nextTabAction = createAction(tr("&Next Tab"), "Ctrl+Tab",
                                   ActionMap::nextTab, "ChevronRight");
    m_tabMenu->addAction(m_nextTabAction);

    // 注释掉不存在的 previousTab
    // m_prevTabAction = createAction(tr("&Previous Tab"), "Ctrl+Shift+Tab",
    //                               ActionMap::previousTab, "ChevronLeft");
    // m_tabMenu->addAction(m_prevTabAction);
}

void MenuBar::createViewMenu() {
    m_viewMenu = new ElaMenu(tr("&View"), this);
    addMenu(m_viewMenu);

    // 欢迎屏幕控制
    m_welcomeScreenToggleAction = new QAction(tr("Show &Welcome Screen"), this);
    m_welcomeScreenToggleAction->setCheckable(true);
    m_welcomeScreenToggleAction->setChecked(true);  // Default enabled
    m_welcomeScreenToggleAction->setToolTip(
        tr("Toggle welcome screen display"));
    m_viewMenu->addAction(m_welcomeScreenToggleAction);
    connect(m_welcomeScreenToggleAction, &QAction::triggered, this,
            [this]() { emit welcomeScreenToggleRequested(); });

    m_viewMenu->addSeparator();

    // 侧边栏控制
    m_toggleLeftSideBarAction = new QAction(tr("&Toggle Sidebar"), this);
    m_toggleLeftSideBarAction->setShortcut(QKeySequence("F9"));
    m_toggleLeftSideBarAction->setCheckable(true);
    m_toggleLeftSideBarAction->setChecked(true);
    m_toggleLeftSideBarAction->setIcon(ELA_ICON(Sidebar));
    m_viewMenu->addAction(m_toggleLeftSideBarAction);
    connect(m_toggleLeftSideBarAction, &QAction::triggered, this,
            [this]() { emit actionTriggered(ActionMap::toggleSideBar); });

    m_showSideBarAction = new QAction(tr("&Show Sidebar"), this);
    m_viewMenu->addAction(m_showSideBarAction);
    connect(m_showSideBarAction, &QAction::triggered, this,
            [this]() { emit actionTriggered(ActionMap::showSideBar); });

    m_hideSideBarAction = new QAction(tr("&Hide Sidebar"), this);
    m_viewMenu->addAction(m_hideSideBarAction);
    connect(m_hideSideBarAction, &QAction::triggered, this,
            [this]() { emit actionTriggered(ActionMap::hideSideBar); });

    m_viewMenu->addSeparator();

    // 调试面板控制
    m_debugPanelToggleAction = new QAction(tr("Show &Debug Panel"), this);
    m_debugPanelToggleAction->setShortcut(QKeySequence("F12"));
    m_debugPanelToggleAction->setCheckable(true);
    m_debugPanelToggleAction->setChecked(true);  // Default show
    m_debugPanelToggleAction->setToolTip(tr("Toggle debug log panel display"));
    m_viewMenu->addAction(m_debugPanelToggleAction);
    connect(m_debugPanelToggleAction, &QAction::triggered, this,
            [this]() { emit debugPanelToggleRequested(); });

    m_debugPanelClearAction = new QAction(tr("&Clear Debug Log"), this);
    m_debugPanelClearAction->setShortcut(QKeySequence("Ctrl+Shift+L"));
    m_debugPanelClearAction->setToolTip(tr("Clear all logs in debug panel"));
    m_viewMenu->addAction(m_debugPanelClearAction);
    connect(m_debugPanelClearAction, &QAction::triggered, this,
            [this]() { emit debugPanelClearRequested(); });

    m_debugPanelExportAction = new QAction(tr("&Export Debug Log"), this);
    m_debugPanelExportAction->setShortcut(QKeySequence("Ctrl+Shift+E"));
    m_debugPanelExportAction->setToolTip(tr("Export debug log to file"));
    m_viewMenu->addAction(m_debugPanelExportAction);
    connect(m_debugPanelExportAction, &QAction::triggered, this,
            [this]() { emit debugPanelExportRequested(); });

    m_viewMenu->addSeparator();

    // 缩放子菜单
    m_zoomMenu = new ElaMenu(tr("&Zoom"), m_viewMenu);
    m_zoomMenu->setIcon(ELA_ICON(MagnifyingGlass));
    m_viewMenu->addMenu(m_zoomMenu);

    m_zoomInAction = createAction(tr("Zoom &In"), "Ctrl++", ActionMap::zoomIn,
                                  "MagnifyingGlassPlus");
    m_zoomMenu->addAction(m_zoomInAction);

    m_zoomOutAction = createAction(tr("Zoom &Out"), "Ctrl+-",
                                   ActionMap::zoomOut, "MagnifyingGlassMinus");
    m_zoomMenu->addAction(m_zoomOutAction);

    // 注释掉不存在的 resetZoom
    // m_zoomResetAction = createAction(tr("&Reset Zoom"), "Ctrl+0",
    //                                 ActionMap::resetZoom);
    // m_zoomMenu->addAction(m_zoomResetAction);

    m_zoomMenu->addSeparator();

    m_fitWidthAction =
        createAction(tr("Fit &Width"), "Ctrl+1",
                     ActionMap::fitToWidth);  // fitWidth -> fitToWidth
    m_zoomMenu->addAction(m_fitWidthAction);

    m_fitPageAction =
        createAction(tr("Fit &Page"), "Ctrl+2",
                     ActionMap::fitToPage);  // fitPage -> fitToPage
    m_zoomMenu->addAction(m_fitPageAction);

    m_fitHeightAction =
        createAction(tr("Fit &Height"), "Ctrl+3",
                     ActionMap::fitToHeight);  // fitHeight -> fitToHeight
    m_zoomMenu->addAction(m_fitHeightAction);

    // 旋转子菜单
    m_rotateMenu = new ElaMenu(tr("&Rotate"), m_viewMenu);
    m_rotateMenu->setIcon(ELA_ICON(ArrowsRotate));
    m_viewMenu->addMenu(m_rotateMenu);

    m_rotateLeftAction = createAction(tr("Rotate &Left"), "Ctrl+L",
                                      ActionMap::rotateLeft, "RotateLeft");
    m_rotateMenu->addAction(m_rotateLeftAction);

    m_rotateRightAction = createAction(tr("Rotate &Right"), "Ctrl+R",
                                       ActionMap::rotateRight, "RotateRight");
    m_rotateMenu->addAction(m_rotateRightAction);

    // 注释掉不存在的 resetRotation
    // m_resetRotationAction = createAction(tr("Reset &Rotation"),
    // "Ctrl+Shift+R",
    //                                     ActionMap::resetRotation);
    // m_rotateMenu->addAction(m_resetRotationAction);

    m_viewMenu->addSeparator();

    // 视图模式子菜单
    m_viewModeMenu = new ElaMenu(tr("View &Mode"), m_viewMenu);
    m_viewModeMenu->setIcon(ELA_ICON(TableCells));
    m_viewMenu->addMenu(m_viewModeMenu);

    QActionGroup* viewModeGroup = new QActionGroup(this);
    viewModeGroup->setExclusive(true);

    m_singlePageAction =
        createAction(tr("&Single Page"), "", ActionMap::setSinglePageMode);
    m_singlePageAction->setCheckable(true);
    m_singlePageAction->setChecked(true);
    viewModeGroup->addAction(m_singlePageAction);
    m_viewModeMenu->addAction(m_singlePageAction);

    m_continuousAction = createAction(
        tr("&Continuous"), "",
        ActionMap::setContinuousScrollMode);  // setContinuousMode ->
                                              // setContinuousScrollMode
    m_continuousAction->setCheckable(true);
    viewModeGroup->addAction(m_continuousAction);
    m_viewModeMenu->addAction(m_continuousAction);

    m_twoPageAction = createAction(
        tr("&Two Pages"), "",
        ActionMap::setTwoPagesMode);  // setTwoPageMode -> setTwoPagesMode
    m_twoPageAction->setCheckable(true);
    viewModeGroup->addAction(m_twoPageAction);
    m_viewModeMenu->addAction(m_twoPageAction);

    m_bookModeAction = createAction(
        tr("&Book Mode"), "",
        ActionMap::setBookViewMode);  // setBookMode -> setBookViewMode
    m_bookModeAction->setCheckable(true);
    viewModeGroup->addAction(m_bookModeAction);
    m_viewModeMenu->addAction(m_bookModeAction);

    m_viewMenu->addSeparator();

    // 注释掉不存在的 toggleToolBar 和 toggleStatusBar
    // m_toggleToolBarAction = createAction(tr("&Toolbar"), "F11",
    //                                     ActionMap::toggleToolBar);
    // m_toggleToolBarAction->setCheckable(true);
    // m_toggleToolBarAction->setChecked(true);
    // m_viewMenu->addAction(m_toggleToolBarAction);

    // m_toggleStatusBarAction = createAction(tr("&Status Bar"), "F12",
    //                                       ActionMap::toggleStatusBar);
    // m_toggleStatusBarAction->setCheckable(true);
    // m_toggleStatusBarAction->setChecked(true);
    // m_viewMenu->addAction(m_toggleStatusBarAction);

    m_viewMenu->addSeparator();

    // 全屏
    m_fullScreenAction =
        createAction(tr("&Full Screen"), "F11",
                     ActionMap::fullScreen);  // toggleFullScreen -> fullScreen
    m_fullScreenAction->setCheckable(true);
    m_viewMenu->addAction(m_fullScreenAction);

    // 注释掉不存在的 presentationMode
    // m_presentationModeAction = createAction(tr("&Presentation Mode"), "F5",
    //                                        ActionMap::presentationMode);
    // m_viewMenu->addAction(m_presentationModeAction);
}

void MenuBar::createThemeMenu() {
    m_themeMenu = new ElaMenu(tr("&Theme"), this);
    addMenu(m_themeMenu);

    // 主题选择
    QActionGroup* themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);

    m_lightThemeAction = new QAction(tr("&Light Theme"), this);
    m_lightThemeAction->setCheckable(true);
    m_lightThemeAction->setIcon(ELA_ICON(Sun));
    themeGroup->addAction(m_lightThemeAction);
    m_themeMenu->addAction(m_lightThemeAction);
    connect(m_lightThemeAction, &QAction::triggered, this, [this]() {
        emit themeChangeRequested("light");
        emit themeChanged("light");
    });

    m_darkThemeAction = new QAction(tr("&Dark Theme"), this);
    m_darkThemeAction->setCheckable(true);
    m_darkThemeAction->setIcon(ELA_ICON(Moon));
    themeGroup->addAction(m_darkThemeAction);
    m_themeMenu->addAction(m_darkThemeAction);
    connect(m_darkThemeAction, &QAction::triggered, this, [this]() {
        emit themeChangeRequested("dark");
        emit themeChanged("dark");
    });

    m_autoThemeAction = new QAction(tr("&Auto (System)"), this);
    m_autoThemeAction->setCheckable(true);
    m_autoThemeAction->setIcon(ELA_ICON(CircleHalfStroke));
    themeGroup->addAction(m_autoThemeAction);
    m_themeMenu->addAction(m_autoThemeAction);
    connect(m_autoThemeAction, &QAction::triggered, this, [this]() {
        emit themeChangeRequested("auto");
        emit themeChanged("auto");
    });

    // 根据当前主题设置选中状态
    Theme currentTheme = StyleManager::instance().currentTheme();
    if (currentTheme == Theme::Light) {
        m_lightThemeAction->setChecked(true);
    } else {
        m_darkThemeAction->setChecked(true);
    }

    m_themeMenu->addSeparator();

    // 语言选择
    m_languageMenu = new ElaMenu(tr("&Language"), m_themeMenu);
    m_languageMenu->setIcon(ELA_ICON(Language));
    m_themeMenu->addMenu(m_languageMenu);

    QActionGroup* languageGroup = new QActionGroup(this);
    languageGroup->setExclusive(true);

    m_englishAction = new QAction(tr("&English"), this);
    m_englishAction->setCheckable(true);
    languageGroup->addAction(m_englishAction);
    m_languageMenu->addAction(m_englishAction);
    connect(m_englishAction, &QAction::triggered, this, [this]() {
        emit languageChangeRequested("en");
        emit languageChanged("en");
    });

    m_chineseAction = new QAction(tr("&Chinese (简体中文)"), this);
    m_chineseAction->setCheckable(true);
    languageGroup->addAction(m_chineseAction);
    m_languageMenu->addAction(m_chineseAction);
    connect(m_chineseAction, &QAction::triggered, this, [this]() {
        emit languageChangeRequested("zh_CN");
        emit languageChanged("zh");
    });

    // 根据当前语言设置选中状态
    QString currentLanguage =
        I18nManager::instance()
            .currentLanguageCode();  // Use currentLanguageCode() instead
    if (currentLanguage == "en") {
        m_englishAction->setChecked(true);
    } else {
        m_chineseAction->setChecked(true);
    }
}

void MenuBar::createHelpMenu() {
    m_helpMenu = new ElaMenu(tr("&Help"), this);
    addMenu(m_helpMenu);

    // 帮助文档
    m_helpAction =
        createAction(tr("&Help"), "F1", ActionMap::showHelp, "CircleQuestion");
    m_helpMenu->addAction(m_helpAction);

    m_helpMenu->addSeparator();

    // 注释掉不存在的 checkUpdates
    // m_checkUpdatesAction = createAction(tr("Check for &Updates..."), "",
    //                                    ActionMap::checkUpdates,
    //                                    "ArrowsRotate");
    // m_helpMenu->addAction(m_checkUpdatesAction);

    // m_helpMenu->addSeparator();

    // 注释掉不存在的 showAboutDialog
    // m_aboutAction = createAction(tr("&About SAST Readium"), "",
    //                             ActionMap::showAboutDialog, "CircleInfo");
    // m_helpMenu->addAction(m_aboutAction);
}

void MenuBar::setupRecentFilesMenu() {
    m_recentFilesMenu = new ElaMenu(tr("Recent &Files"), m_fileMenu);
    m_recentFilesMenu->setIcon(ELA_ICON(ClockRotateLeft));

    // Simply add the menu - it will be positioned correctly in the menu
    // structure since this is called during menu construction in
    // createFileMenu()
    m_fileMenu->addMenu(m_recentFilesMenu);

    // 清除最近文件动作
    m_clearRecentFilesAction = new QAction(tr("&Clear Recent Files"), this);
    connect(m_clearRecentFilesAction, &QAction::triggered, this,
            &MenuBar::onClearRecentFilesTriggered);
}

void MenuBar::clearRecentFilesMenu() {
    // 清除所有最近文件动作
    for (QAction* action : m_recentFileActions) {
        m_recentFilesMenu->removeAction(action);
        delete action;
    }
    m_recentFileActions.clear();
}

void MenuBar::retranslateUi() {
    SLOG_INFO("MenuBar: Retranslating UI");

    // 更新菜单标题
    m_fileMenu->setTitle(tr("&File"));
    m_tabMenu->setTitle(tr("&Tabs"));
    m_viewMenu->setTitle(tr("&View"));
    m_themeMenu->setTitle(tr("&Theme"));
    m_helpMenu->setTitle(tr("&Help"));

    // 更新文件菜单
    m_openAction->setText(tr("&Open..."));
    m_openFolderAction->setText(tr("Open &Folder..."));
    m_closeAction->setText(tr("&Close"));
    m_closeAllAction->setText(tr("Close &All"));
    m_saveAsAction->setText(tr("&Save Copy As..."));
    m_printAction->setText(tr("&Print..."));
    m_propertiesAction->setText(tr("P&roperties"));
    m_reloadAction->setText(tr("&Reload"));
    m_exitAction->setText(tr("E&xit"));

    // 更新标签页菜单
    m_newTabAction->setText(tr("&New Tab"));
    m_closeTabAction->setText(tr("&Close Tab"));
    if (m_closeOtherTabsAction != nullptr) {
        m_closeOtherTabsAction->setText(tr("Close &Other Tabs"));
    }
    m_closeAllTabsAction->setText(tr("Close &All Tabs"));
    m_nextTabAction->setText(tr("&Next Tab"));
    if (m_prevTabAction != nullptr) {
        m_prevTabAction->setText(tr("&Previous Tab"));
    }

    // 更新视图菜单
    m_welcomeScreenToggleAction->setText(tr("Show &Welcome Screen"));
    m_toggleLeftSideBarAction->setText(tr("&Toggle Sidebar"));
    m_showSideBarAction->setText(tr("&Show Sidebar"));
    m_hideSideBarAction->setText(tr("&Hide Sidebar"));
    m_debugPanelToggleAction->setText(tr("Show &Debug Panel"));
    m_debugPanelClearAction->setText(tr("&Clear Debug Log"));
    m_debugPanelExportAction->setText(tr("&Export Debug Log"));
    m_zoomInAction->setText(tr("Zoom &In"));
    m_zoomOutAction->setText(tr("Zoom &Out"));
    if (m_zoomResetAction != nullptr) {
        m_zoomResetAction->setText(tr("&Reset Zoom"));
    }
    m_fitWidthAction->setText(tr("Fit &Width"));
    m_fitPageAction->setText(tr("Fit &Page"));
    m_fitHeightAction->setText(tr("Fit &Height"));

    // 更新主题菜单
    m_lightThemeAction->setText(tr("&Light Theme"));
    m_darkThemeAction->setText(tr("&Dark Theme"));
    m_autoThemeAction->setText(tr("&Auto (System)"));
    m_englishAction->setText(tr("&English"));
    m_chineseAction->setText(tr("&Chinese (简体中文)"));

    // 更新帮助菜单
    m_helpAction->setText(tr("&Help"));
    if (m_checkUpdatesAction != nullptr) {
        m_checkUpdatesAction->setText(tr("Check for &Updates..."));
    }
    if (m_aboutAction != nullptr) {
        m_aboutAction->setText(tr("&About SAST Readium"));
    }

    // 更新最近文件菜单
    updateRecentFilesMenu();
}

void MenuBar::updateMenuStates() {
    // 根据文档是否打开更新菜单项状态
    bool hasDocument = m_documentOpened;

    // 文件菜单
    if (m_closeAction != nullptr) {
        m_closeAction->setEnabled(hasDocument);
    }
    if (m_closeAllAction != nullptr) {
        m_closeAllAction->setEnabled(hasDocument);
    }
    if (m_saveAsAction != nullptr) {
        m_saveAsAction->setEnabled(hasDocument);
    }
    if (m_exportMenu != nullptr) {
        m_exportMenu->setEnabled(hasDocument);
    }
    if (m_printAction != nullptr) {
        m_printAction->setEnabled(hasDocument);
    }
    if (m_propertiesAction != nullptr) {
        m_propertiesAction->setEnabled(hasDocument);
    }
    if (m_reloadAction != nullptr) {
        m_reloadAction->setEnabled(hasDocument);
    }

    // 标签页菜单
    if (m_closeTabAction != nullptr) {
        m_closeTabAction->setEnabled(hasDocument);
    }
    if (m_closeOtherTabsAction != nullptr) {
        m_closeOtherTabsAction->setEnabled(hasDocument);
    }
    if (m_closeAllTabsAction != nullptr) {
        m_closeAllTabsAction->setEnabled(hasDocument);
    }
    if (m_nextTabAction != nullptr) {
        m_nextTabAction->setEnabled(hasDocument);
    }
    if (m_prevTabAction != nullptr) {
        m_prevTabAction->setEnabled(hasDocument);
    }

    // 视图菜单
    if (m_zoomMenu != nullptr) {
        m_zoomMenu->setEnabled(hasDocument);
    }
    if (m_rotateMenu != nullptr) {
        m_rotateMenu->setEnabled(hasDocument);
    }
    if (m_viewModeMenu != nullptr) {
        m_viewModeMenu->setEnabled(hasDocument);
    }
    if (m_fullScreenAction != nullptr) {
        m_fullScreenAction->setEnabled(hasDocument);
    }
    if (m_presentationModeAction != nullptr) {
        m_presentationModeAction->setEnabled(hasDocument);
    }
}

QAction* MenuBar::createAction(const QString& text, const QString& shortcut,
                               ActionMap actionId, const QString& iconName) {
    QAction* action = new QAction(text, this);

    if (!shortcut.isEmpty()) {
        action->setShortcut(QKeySequence(shortcut));
    }

    if (!iconName.isEmpty()) {
        // 使用字符串到枚举的映射
        static QMap<QString, ElaIconType::IconName> iconMap = {
            {"File", ElaIconType::File},
            {"FilePdf", ElaIconType::FilePdf},
            {"Folder", ElaIconType::Folder},
            {"FolderOpen", ElaIconType::FolderOpen},
            {"FloppyDisk", ElaIconType::FloppyDisk},
            {"FileExport", ElaIconType::FileExport},
            {"Print", ElaIconType::Print},
            {"Xmark", ElaIconType::Xmark},
            {"MagnifyingGlassPlus", ElaIconType::MagnifyingGlassPlus},
            {"MagnifyingGlassMinus", ElaIconType::MagnifyingGlassMinus},
            {"RotateLeft", ElaIconType::RotateLeft},
            {"RotateRight", ElaIconType::RotateRight},
            {"Sun", ElaIconType::Sun},
            {"Moon", ElaIconType::Moon},
            {"CircleHalfStroke", ElaIconType::CircleHalfStroke},
            {"Language", ElaIconType::Language},
            {"ClockRotateLeft", ElaIconType::ClockRotateLeft},
        };

        if (iconMap.contains(iconName)) {
            action->setIcon(
                ElaIcon::getInstance()->getElaIcon(iconMap[iconName]));
        }
    }

    connectAction(action, actionId);
    m_actionMap[actionId] = action;

    return action;
}

void MenuBar::connectAction(QAction* action, ActionMap actionId) {
    connect(action, &QAction::triggered, this, [this, actionId]() {
        emit actionTriggered(actionId);
        emit onExecuted(actionId);
    });
}
