#include "MenuBar.h"
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include "../../logging/LoggingMacros.h"
#include "../../managers/FileTypeIconManager.h"
#include "../../managers/I18nManager.h"
#include "../widgets/ToastNotification.h"

MenuBar::MenuBar(QWidget* parent)
    : QMenuBar(parent),
      m_recentFilesManager(nullptr),
      m_recentFilesMenu(nullptr),
      m_clearRecentFilesAction(nullptr) {
    // Initialize shortcut array
    for (auto& m_recentFileShortcut : m_recentFileShortcuts) {
        m_recentFileShortcut = nullptr;
    }

    createFileMenu();
    createTabMenu();
    createViewMenu();
    createThemeMenu();
    setupRecentFileShortcuts();
}

MenuBar::~MenuBar() {
    // Clean up shortcuts (will be deleted by Qt parent-child ownership)
    // All menus and actions are deleted automatically by Qt parent-child
    // ownership No manual deletion needed for widgets/actions created with
    // 'this' as parent

    LOG_DEBUG("MenuBar destroyed successfully");
}

void MenuBar::createFileMenu() {
    QMenu* fileMenu = new QMenu(tr("&File"), this);
    addMenu(fileMenu);

    QAction* openAction = new QAction(tr("&Open"), this);
    openAction->setShortcut(QKeySequence("Ctrl+O"));

    QAction* openFolderAction = new QAction(tr("Open &Folder"), this);
    openFolderAction->setShortcut(QKeySequence("Ctrl+Shift+O"));

    QAction* saveAction = new QAction(tr("&Save"), this);
    saveAction->setShortcut(QKeySequence("Ctrl+S"));

    QAction* saveAsAction = new QAction(tr("Save &As..."), this);
    saveAsAction->setShortcut(QKeySequence("Ctrl+Shift+S"));

    QAction* documentPropertiesAction =
        new QAction(tr("Document &Properties"), this);
    documentPropertiesAction->setShortcut(QKeySequence("Ctrl+I"));

    QAction* exitAction = new QAction(tr("E&xit"), this);
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
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
}

void MenuBar::createTabMenu() {
    QMenu* tabMenu = new QMenu(tr("&Tabs"), this);
    addMenu(tabMenu);

    QAction* newTabAction = new QAction(tr("&New Tab"), this);
    newTabAction->setShortcut(QKeySequence("Ctrl+T"));

    QAction* closeTabAction = new QAction(tr("&Close Tab"), this);
    closeTabAction->setShortcut(QKeySequence("Ctrl+W"));

    QAction* closeAllTabsAction = new QAction(tr("Close &All Tabs"), this);
    closeAllTabsAction->setShortcut(QKeySequence("Ctrl+Shift+W"));

    QAction* nextTabAction = new QAction(tr("Ne&xt Tab"), this);
    nextTabAction->setShortcut(QKeySequence("Ctrl+Tab"));

    QAction* prevTabAction = new QAction(tr("&Previous Tab"), this);
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
    QMenu* viewMenu = new QMenu(tr("&View"), this);
    addMenu(viewMenu);

    // Welcome screen control
    m_welcomeScreenToggleAction = new QAction(tr("Show &Welcome Screen"), this);
    m_welcomeScreenToggleAction->setCheckable(true);
    m_welcomeScreenToggleAction->setChecked(true);  // Default enabled
    m_welcomeScreenToggleAction->setToolTip(
        tr("Toggle welcome screen display"));

    // Sidebar control
    QAction* toggleSideBarAction = new QAction(tr("Toggle &Sidebar"), this);
    toggleSideBarAction->setShortcut(QKeySequence("F9"));
    toggleSideBarAction->setCheckable(true);
    toggleSideBarAction->setChecked(true);  // Default show

    QAction* showSideBarAction = new QAction(tr("Show Sidebar"), this);
    QAction* hideSideBarAction = new QAction(tr("Hide Sidebar"), this);

    // View mode control
    QAction* singlePageAction = new QAction(tr("Single &Page View"), this);
    singlePageAction->setShortcut(QKeySequence("Ctrl+1"));
    singlePageAction->setCheckable(true);
    singlePageAction->setChecked(true);  // Default single page view

    QAction* continuousScrollAction =
        new QAction(tr("&Continuous Scroll"), this);
    continuousScrollAction->setShortcut(QKeySequence("Ctrl+2"));
    continuousScrollAction->setCheckable(true);

    // Create view mode action group
    QActionGroup* viewModeGroup = new QActionGroup(this);
    viewModeGroup->addAction(singlePageAction);
    viewModeGroup->addAction(continuousScrollAction);

    // View control
    QAction* fullScreenAction = new QAction(tr("&Full Screen"), this);
    fullScreenAction->setShortcut(QKeySequence("Ctrl+Shift+F"));

    QAction* zoomInAction = new QAction(tr("Zoom &In"), this);
    zoomInAction->setShortcut(QKeySequence("Ctrl++"));

    QAction* zoomOutAction = new QAction(tr("Zoom &Out"), this);
    zoomOutAction->setShortcut(QKeySequence("Ctrl+-"));

    // Debug panel control
    m_debugPanelToggleAction = new QAction(tr("Show &Debug Panel"), this);
    m_debugPanelToggleAction->setShortcut(QKeySequence("F12"));
    m_debugPanelToggleAction->setCheckable(true);
    m_debugPanelToggleAction->setChecked(true);  // Default show
    m_debugPanelToggleAction->setToolTip(tr("Toggle debug log panel display"));

    m_debugPanelClearAction = new QAction(tr("&Clear Debug Log"), this);
    m_debugPanelClearAction->setShortcut(QKeySequence("Ctrl+Shift+L"));
    m_debugPanelClearAction->setToolTip(tr("Clear all logs in debug panel"));

    m_debugPanelExportAction = new QAction(tr("&Export Debug Log"), this);
    m_debugPanelExportAction->setShortcut(QKeySequence("Ctrl+Shift+E"));
    m_debugPanelExportAction->setToolTip(tr("Export debug log to file"));

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
    QMenu* themeMenu = new QMenu(tr("&Settings"), this);
    addMenu(themeMenu);

    // Theme submenu
    QMenu* themeSubMenu = new QMenu(tr("&Theme"), this);

    QAction* lightThemeAction = new QAction(tr("&Light"), this);
    lightThemeAction->setCheckable(true);

    QAction* darkThemeAction = new QAction(tr("&Dark"), this);
    darkThemeAction->setCheckable(true);

    QActionGroup* themeGroup = new QActionGroup(this);
    themeGroup->addAction(lightThemeAction);
    themeGroup->addAction(darkThemeAction);

    themeSubMenu->addAction(lightThemeAction);
    themeSubMenu->addAction(darkThemeAction);

    // Language submenu
    QMenu* languageSubMenu = new QMenu(tr("&Language"), this);

    QAction* englishAction = new QAction(tr("&English"), this);
    englishAction->setCheckable(true);
    englishAction->setData("en");

    QAction* chineseAction = new QAction(tr("简体中文(&C)"), this);
    chineseAction->setCheckable(true);
    chineseAction->setData("zh");

    QActionGroup* languageGroup = new QActionGroup(this);
    languageGroup->addAction(englishAction);
    languageGroup->addAction(chineseAction);

    languageSubMenu->addAction(englishAction);
    languageSubMenu->addAction(chineseAction);

    // Set current language as checked
    QString currentLang = I18nManager::instance().currentLanguageCode();
    if (currentLang == "zh") {
        chineseAction->setChecked(true);
    } else {
        englishAction->setChecked(true);
    }

    // Add submenus to main menu
    themeMenu->addMenu(themeSubMenu);
    themeMenu->addMenu(languageSubMenu);
    themeMenu->addSeparator();

    // Add Settings action
    QAction* settingsAction = new QAction(tr("&Settings..."), this);
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    themeMenu->addAction(settingsAction);

    connect(settingsAction, &QAction::triggered, this,
            [this]() { emit onExecuted(ActionMap::showSettings); });

    // Connect theme signals
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

    // Connect language signals
    connect(englishAction, &QAction::triggered, this, [this]() {
        I18nManager::instance().loadLanguage("en");
        emit languageChanged("en");
    });

    connect(chineseAction, &QAction::triggered, this, [this]() {
        I18nManager::instance().loadLanguage("zh");
        emit languageChanged("zh");
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
    m_recentFilesMenu = new QMenu(tr("&Recent Files"), this);
    m_recentFilesMenu->setEnabled(
        false);  // Initially disabled until there are files

    m_clearRecentFilesAction = new QAction(tr("&Clear Recent Files"), this);
    connect(m_clearRecentFilesAction, &QAction::triggered, this,
            &MenuBar::onClearRecentFilesTriggered);
}

void MenuBar::setupRecentFileShortcuts() {
    // Create keyboard shortcuts for first 9 recent files (Ctrl+Alt+1-9)
    for (int i = 0; i < 9; ++i) {
        QString shortcutKey = QString("Ctrl+Alt+%1").arg(i + 1);
        m_recentFileShortcuts[i] =
            new QShortcut(QKeySequence(shortcutKey), this);

        // Connect each shortcut to open the corresponding recent file
        int fileIndex = i;  // Capture index for lambda
        connect(m_recentFileShortcuts[i], &QShortcut::activated, this,
                [this, fileIndex]() {
                    if (!m_recentFilesManager) {
                        return;
                    }

                    QList<RecentFileInfo> recentFiles =
                        m_recentFilesManager->getRecentFiles();
                    if (fileIndex < recentFiles.size()) {
                        const QString& filePath =
                            recentFiles[fileIndex].filePath;

                        // Check if file exists before opening
                        QFileInfo fileInfo(filePath);
                        if (fileInfo.exists()) {
                            emit openRecentFileRequested(filePath);
                        } else {
                            // File doesn't exist, remove from list
                            m_recentFilesManager->removeRecentFile(filePath);
                        }
                    }
                });
    }
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
        QAction* noFilesAction =
            m_recentFilesMenu->addAction(tr("No Recent Files"));
        noFilesAction->setEnabled(false);
        return;
    }

    m_recentFilesMenu->setEnabled(true);

    // 添加最近文件项
    for (int i = 0; i < recentFiles.size(); ++i) {
        const RecentFileInfo& fileInfo = recentFiles[i];

        // 创建显示文本：序号 + 智能截断的路径
        QString displayText = QString("&%1 ").arg(i + 1);

        // Intelligent path truncation: show filename + parent folder if too
        // long
        QFileInfo qFileInfo(fileInfo.filePath);
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

        QAction* fileAction = m_recentFilesMenu->addAction(displayText);
        fileAction->setToolTip(fileInfo.filePath);
        fileAction->setData(fileInfo.filePath);

        // Add file type icon
        QIcon fileIcon = FileTypeIconManager::instance().getFileTypeIcon(
            fileInfo.filePath, 16);
        fileAction->setIcon(fileIcon);

        // Add keyboard shortcut hint for first 9 files
        if (i < 9) {
            fileAction->setShortcut(
                QKeySequence(QString("Ctrl+Alt+%1").arg(i + 1)));
        }

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

void MenuBar::retranslateUi() {
    // Simplest approach: rebuild the menus so all tr() calls apply in new
    // language
    this->clear();
    createFileMenu();
    createTabMenu();
    createViewMenu();
    createThemeMenu();

    // Refresh recent files submenu contents if manager is present
    if (m_recentFilesManager) {
        updateRecentFilesMenu();
    }
}

void MenuBar::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QMenuBar::changeEvent(event);
}

void MenuBar::onClearRecentFilesTriggered() {
    if (!m_recentFilesManager || !m_recentFilesManager->hasRecentFiles()) {
        return;
    }

    // Show confirmation dialog
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Clear Recent Files"),
        tr("Are you sure you want to clear all recent files?\n\n"
           "This action cannot be undone."),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No  // Default button
    );

    if (reply == QMessageBox::Yes) {
        m_recentFilesManager->clearRecentFiles();
    }
}
