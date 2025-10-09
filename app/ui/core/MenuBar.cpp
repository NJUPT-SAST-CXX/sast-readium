#include "MenuBar.h"
#include <QAction>
#include <QActionGroup>
#include <QDebug>
#include <QFileInfo>
#include <QMenu>

MenuBar::MenuBar(QWidget* parent)
    : QMenuBar(parent),
      m_recentFilesManager(nullptr),
      m_recentFilesMenu(nullptr),
      m_clearRecentFilesAction(nullptr) {
    createFileMenu();
    createTabMenu();
    createViewMenu();
    createThemeMenu();
}

void MenuBar::createFileMenu() {
    QMenu* fileMenu = new QMenu(tr("文件(F)"), this);
    addMenu(fileMenu);

    QAction* openAction = new QAction(tr("打开"), this);
    openAction->setShortcut(QKeySequence("Ctrl+O"));

    QAction* openFolderAction = new QAction(tr("打开文件夹"), this);
    openFolderAction->setShortcut(QKeySequence("Ctrl+Shift+O"));

    QAction* saveAction = new QAction(tr("保存"), this);
    saveAction->setShortcut(QKeySequence("Ctrl+S"));

    QAction* saveAsAction = new QAction(tr("另存副本"), this);
    saveAsAction->setShortcut(QKeySequence("Ctrl+Shift+S"));

    QAction* documentPropertiesAction = new QAction(tr("文档属性"), this);
    documentPropertiesAction->setShortcut(QKeySequence("Ctrl+I"));

    QAction* exitAction = new QAction(tr("退出"), this);
    exitAction->setShortcut(QKeySequence("Ctrl+Q"));

    fileMenu->addAction(openAction);
    fileMenu->addAction(openFolderAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveAsAction);
    fileMenu->addSeparator();

    // 添加最近文件菜单
    setupRecentFilesMenu();
    fileMenu->addMenu(m_recentFilesMenu);
    fileMenu->addSeparator();

    fileMenu->addAction(documentPropertiesAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    connect(openAction, &QAction::triggered, this,
            [this]() { emit onExecuted(ActionMap::openFile); });
    connect(openFolderAction, &QAction::triggered, this,
            [this]() { emit onExecuted(ActionMap::openFolder); });
    connect(saveAction, &QAction::triggered, this,
            [this]() { emit onExecuted(ActionMap::save); });
    connect(saveAsAction, &QAction::triggered, this,
            [this]() { emit onExecuted(ActionMap::saveAs); });
    connect(documentPropertiesAction, &QAction::triggered, this,
            [this]() { emit onExecuted(ActionMap::showDocumentMetadata); });
}

void MenuBar::createTabMenu() {
    QMenu* tabMenu = new QMenu(tr("标签页(T)"), this);
    addMenu(tabMenu);

    QAction* newTabAction = new QAction(tr("新建标签页"), this);
    newTabAction->setShortcut(QKeySequence("Ctrl+T"));

    QAction* closeTabAction = new QAction(tr("关闭标签页"), this);
    closeTabAction->setShortcut(QKeySequence("Ctrl+W"));

    QAction* closeAllTabsAction = new QAction(tr("关闭所有标签页"), this);
    closeAllTabsAction->setShortcut(QKeySequence("Ctrl+Shift+W"));

    QAction* nextTabAction = new QAction(tr("下一个标签页"), this);
    nextTabAction->setShortcut(QKeySequence("Ctrl+Tab"));

    QAction* prevTabAction = new QAction(tr("上一个标签页"), this);
    prevTabAction->setShortcut(QKeySequence("Ctrl+Shift+Tab"));

    tabMenu->addAction(newTabAction);
    tabMenu->addSeparator();
    tabMenu->addAction(closeTabAction);
    tabMenu->addAction(closeAllTabsAction);
    tabMenu->addSeparator();
    tabMenu->addAction(nextTabAction);
    tabMenu->addAction(prevTabAction);

    // 连接信号
    connect(newTabAction, &QAction::triggered, this,
            [this]() { emit onExecuted(ActionMap::newTab); });
    connect(closeTabAction, &QAction::triggered, this,
            [this]() { emit onExecuted(ActionMap::closeCurrentTab); });
    connect(closeAllTabsAction, &QAction::triggered, this,
            [this]() { emit onExecuted(ActionMap::closeAllTabs); });
    connect(nextTabAction, &QAction::triggered, this,
            [this]() { emit onExecuted(ActionMap::nextTab); });
    connect(prevTabAction, &QAction::triggered, this,
            [this]() { emit onExecuted(ActionMap::prevTab); });
}

void MenuBar::createViewMenu() {
    QMenu* viewMenu = new QMenu(tr("视图(V)"), this);
    addMenu(viewMenu);

    // 欢迎界面控制
    m_welcomeScreenToggleAction = new QAction(tr("显示欢迎界面"), this);
    m_welcomeScreenToggleAction->setCheckable(true);
    m_welcomeScreenToggleAction->setChecked(true);  // 默认启用
    m_welcomeScreenToggleAction->setToolTip(tr("切换欢迎界面的显示"));

    // 侧边栏控制
    QAction* toggleSideBarAction = new QAction(tr("切换侧边栏"), this);
    toggleSideBarAction->setShortcut(QKeySequence("F9"));
    toggleSideBarAction->setCheckable(true);
    toggleSideBarAction->setChecked(true);  // 默认显示

    QAction* showSideBarAction = new QAction(tr("显示侧边栏"), this);
    QAction* hideSideBarAction = new QAction(tr("隐藏侧边栏"), this);

    // 查看模式控制
    QAction* singlePageAction = new QAction(tr("单页视图"), this);
    singlePageAction->setShortcut(QKeySequence("Ctrl+1"));
    singlePageAction->setCheckable(true);
    singlePageAction->setChecked(true);  // 默认单页视图

    QAction* continuousScrollAction = new QAction(tr("连续滚动"), this);
    continuousScrollAction->setShortcut(QKeySequence("Ctrl+2"));
    continuousScrollAction->setCheckable(true);

    // 创建查看模式动作组
    QActionGroup* viewModeGroup = new QActionGroup(this);
    viewModeGroup->addAction(singlePageAction);
    viewModeGroup->addAction(continuousScrollAction);

    // 视图控制
    QAction* fullScreenAction = new QAction(tr("全屏"), this);
    fullScreenAction->setShortcut(QKeySequence("Ctrl+Shift+F"));

    QAction* zoomInAction = new QAction(tr("放大"), this);
    zoomInAction->setShortcut(QKeySequence("Ctrl++"));

    QAction* zoomOutAction = new QAction(tr("缩小"), this);
    zoomOutAction->setShortcut(QKeySequence("Ctrl+-"));

    // 调试面板控制
    m_debugPanelToggleAction = new QAction(tr("显示调试面板"), this);
    m_debugPanelToggleAction->setShortcut(QKeySequence("F12"));
    m_debugPanelToggleAction->setCheckable(true);
    m_debugPanelToggleAction->setChecked(true);  // 默认显示
    m_debugPanelToggleAction->setToolTip(tr("切换调试日志面板的显示"));

    m_debugPanelClearAction = new QAction(tr("清空调试日志"), this);
    m_debugPanelClearAction->setShortcut(QKeySequence("Ctrl+Shift+L"));
    m_debugPanelClearAction->setToolTip(tr("清空调试面板中的所有日志"));

    m_debugPanelExportAction = new QAction(tr("导出调试日志"), this);
    m_debugPanelExportAction->setShortcut(QKeySequence("Ctrl+Shift+E"));
    m_debugPanelExportAction->setToolTip(tr("将调试日志导出到文件"));

    // 添加到菜单
    viewMenu->addAction(m_welcomeScreenToggleAction);
    viewMenu->addSeparator();
    viewMenu->addAction(toggleSideBarAction);
    viewMenu->addAction(showSideBarAction);
    viewMenu->addAction(hideSideBarAction);
    viewMenu->addSeparator();
    viewMenu->addAction(m_debugPanelToggleAction);
    viewMenu->addAction(m_debugPanelClearAction);
    viewMenu->addAction(m_debugPanelExportAction);
    viewMenu->addSeparator();
    viewMenu->addAction(singlePageAction);
    viewMenu->addAction(continuousScrollAction);
    viewMenu->addSeparator();
    viewMenu->addAction(fullScreenAction);
    viewMenu->addSeparator();
    viewMenu->addAction(zoomInAction);
    viewMenu->addAction(zoomOutAction);

    // 连接信号
    connect(m_welcomeScreenToggleAction, &QAction::triggered, this,
            [this]() { emit welcomeScreenToggleRequested(); });

    connect(toggleSideBarAction, &QAction::triggered, this,
            [this]() { emit onExecuted(ActionMap::toggleSideBar); });
    connect(showSideBarAction, &QAction::triggered, this,
            [this]() { emit onExecuted(ActionMap::showSideBar); });
    connect(hideSideBarAction, &QAction::triggered, this,
            [this]() { emit onExecuted(ActionMap::hideSideBar); });

    // 连接查看模式信号
    connect(singlePageAction, &QAction::triggered, this,
            [this]() { emit onExecuted(ActionMap::setSinglePageMode); });
    connect(continuousScrollAction, &QAction::triggered, this,
            [this]() { emit onExecuted(ActionMap::setContinuousScrollMode); });

    // 连接调试面板信号
    connect(m_debugPanelToggleAction, &QAction::triggered, this,
            [this]() { emit debugPanelToggleRequested(); });
    connect(m_debugPanelClearAction, &QAction::triggered, this,
            [this]() { emit debugPanelClearRequested(); });
    connect(m_debugPanelExportAction, &QAction::triggered, this,
            [this]() { emit debugPanelExportRequested(); });
}

void MenuBar::createThemeMenu() {
    QMenu* themeMenu = new QMenu(tr("主题(T)"), this);
    addMenu(themeMenu);

    QAction* lightThemeAction = new QAction(tr("浅色"), this);
    lightThemeAction->setCheckable(true);

    QAction* darkThemeAction = new QAction(tr("深色"), this);
    darkThemeAction->setCheckable(true);

    QActionGroup* themeGroup = new QActionGroup(this);
    themeGroup->addAction(lightThemeAction);
    themeGroup->addAction(darkThemeAction);

    connect(lightThemeAction, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            emit themeChanged("light");
        }
    });

    connect(darkThemeAction, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            emit themeChanged("dark");
        }
    });
}

void MenuBar::setRecentFilesManager(RecentFilesManager* manager) {
    if (m_recentFilesManager) {
        disconnect(m_recentFilesManager, nullptr, this, nullptr);
    }

    m_recentFilesManager = manager;

    if (m_recentFilesManager) {
        connect(m_recentFilesManager, &RecentFilesManager::recentFilesChanged,
                this, &MenuBar::updateRecentFilesMenu);
        updateRecentFilesMenu();
    }
}

void MenuBar::setWelcomeScreenEnabled(bool enabled) {
    if (m_welcomeScreenToggleAction) {
        m_welcomeScreenToggleAction->setChecked(enabled);
    }
}

void MenuBar::setupRecentFilesMenu() {
    m_recentFilesMenu = new QMenu(tr("最近打开的文件"), this);
    m_recentFilesMenu->setEnabled(false);  // 初始状态禁用，直到有文件

    m_clearRecentFilesAction = new QAction(tr("清空最近文件"), this);
    connect(m_clearRecentFilesAction, &QAction::triggered, this,
            &MenuBar::onClearRecentFilesTriggered);
}

void MenuBar::updateRecentFilesMenu() {
    if (!m_recentFilesMenu || !m_recentFilesManager) {
        return;
    }

    // 清空现有菜单项
    m_recentFilesMenu->clear();

    QList<RecentFileInfo> recentFiles = m_recentFilesManager->getRecentFiles();

    if (recentFiles.isEmpty()) {
        m_recentFilesMenu->setEnabled(false);
        QAction* noFilesAction = m_recentFilesMenu->addAction(tr("无最近文件"));
        noFilesAction->setEnabled(false);
        return;
    }

    m_recentFilesMenu->setEnabled(true);

    // 添加最近文件项
    for (int i = 0; i < recentFiles.size(); ++i) {
        const RecentFileInfo& fileInfo = recentFiles[i];

        // 创建显示文本：序号 + 文件名 + 路径
        QString displayText =
            QString("&%1 %2").arg(i + 1).arg(fileInfo.fileName);
        if (displayText.length() > 50) {
            displayText = displayText.left(47) + "...";
        }

        QAction* fileAction = m_recentFilesMenu->addAction(displayText);
        fileAction->setToolTip(fileInfo.filePath);
        fileAction->setData(fileInfo.filePath);

        connect(fileAction, &QAction::triggered, this,
                &MenuBar::onRecentFileTriggered);
    }

    // 添加分隔符和清空选项
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
                // 文件不存在，从列表中移除
                if (m_recentFilesManager) {
                    m_recentFilesManager->removeRecentFile(filePath);
                }
                qDebug() << "Recent file no longer exists:" << filePath;
            }
        }
    }
}

void MenuBar::onClearRecentFilesTriggered() {
    if (m_recentFilesManager) {
        m_recentFilesManager->clearRecentFiles();
    }
}
