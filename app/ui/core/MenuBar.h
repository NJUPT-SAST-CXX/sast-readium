#ifndef MENUBAR_H
#define MENUBAR_H

#include <QMap>
#include <QObject>
#include <QString>
#include "ElaDef.h"
#include "ElaIcon.h"
#include "ElaMenuBar.h"  // From ElaWidgetTools library
#include "controller/tool.hpp"

// Helper macro for creating icons
#define ELA_ICON(iconName) \
    ElaIcon::getInstance()->getElaIcon(ElaIconType::iconName)

// Forward declarations
class ElaMenu;
class QAction;
class RecentFilesManager;
class I18nManager;
class StyleManager;

/**
 * @brief ElaMenuBar - 完整的菜单栏实现
 *
 * 实现所有菜单功能：
 * - 文件菜单：打开、关闭、保存、打印等
 * - 标签页菜单：新建、关闭、切换标签页
 * - 视图菜单：缩放、旋转、视图模式、侧边栏
 * - 主题菜单：亮色/暗色主题、语言切换
 */
class MenuBar : public ::ElaMenuBar {
    Q_OBJECT

public:
    explicit MenuBar(QWidget* parent = nullptr);
    ~MenuBar() override;

    // 业务逻辑集成
    void setRecentFilesManager(RecentFilesManager* manager);

    // 状态更新
    void setDocumentOpened(bool opened);
    void updateRecentFilesMenu();
    void setActionEnabled(ActionMap action, bool enabled);
    void setWelcomeScreenEnabled(bool enabled);

signals:
    // 动作触发信号
    void actionTriggered(ActionMap action);

    // 主题和语言信号
    void themeChangeRequested(const QString& theme);
    void languageChangeRequested(const QString& languageCode);

    // Backward-compatible signals for tests and legacy code
    void themeChanged(const QString& theme);
    void languageChanged(const QString& languageCode);
    void onExecuted(ActionMap action);

    // 最近文件信号
    void openRecentFileRequested(const QString& filePath);

    // 欢迎屏幕和调试面板信号
    void welcomeScreenToggleRequested();
    void debugPanelToggleRequested();
    void debugPanelClearRequested();
    void debugPanelExportRequested();

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onRecentFileTriggered();
    void onClearRecentFilesTriggered();

private:
    // 菜单创建
    void createFileMenu();
    void createTabMenu();
    void createViewMenu();
    void createThemeMenu();
    void createHelpMenu();

    // 最近文件菜单
    void setupRecentFilesMenu();
    void setupRecentFileShortcuts();
    void clearRecentFilesMenu();

    // UI 更新
    void retranslateUi();
    void updateMenuStates();

    // 辅助方法
    QAction* createAction(const QString& text, const QString& shortcut,
                          ActionMap actionId,
                          const QString& iconName = QString());
    void connectAction(QAction* action, ActionMap actionId);

private:
    // ========================================================================
    // 菜单
    // ========================================================================
    ElaMenu* m_fileMenu;
    ElaMenu* m_tabMenu;
    ElaMenu* m_viewMenu;
    ElaMenu* m_themeMenu;
    ElaMenu* m_helpMenu;

    // 子菜单
    ElaMenu* m_recentFilesMenu;
    ElaMenu* m_exportMenu;
    ElaMenu* m_zoomMenu;
    ElaMenu* m_rotateMenu;
    ElaMenu* m_viewModeMenu;
    ElaMenu* m_sideBarMenu;
    ElaMenu* m_languageMenu;

    // ========================================================================
    // 文件菜单动作
    // ========================================================================
    QAction* m_openAction;
    QAction* m_openFolderAction;
    QAction* m_closeAction;
    QAction* m_closeAllAction;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_exportAction;
    QAction* m_printAction;
    QAction* m_propertiesAction;
    QAction* m_reloadAction;
    QAction* m_exitAction;

    // 最近文件动作
    QList<QAction*> m_recentFileActions;
    QAction* m_clearRecentFilesAction;

    // ========================================================================
    // 标签页菜单动作
    // ========================================================================
    QAction* m_newTabAction;
    QAction* m_closeTabAction;
    QAction* m_closeOtherTabsAction;
    QAction* m_closeAllTabsAction;
    QAction* m_nextTabAction;
    QAction* m_prevTabAction;

    // ========================================================================
    // 视图菜单动作
    // ========================================================================
    // 缩放
    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QAction* m_zoomResetAction;
    QAction* m_fitWidthAction;
    QAction* m_fitPageAction;
    QAction* m_fitHeightAction;

    // 旋转
    QAction* m_rotateLeftAction;
    QAction* m_rotateRightAction;
    QAction* m_resetRotationAction;

    // 视图模式
    QAction* m_singlePageAction;
    QAction* m_continuousAction;
    QAction* m_twoPageAction;
    QAction* m_bookModeAction;

    // 侧边栏
    QAction* m_toggleLeftSideBarAction;
    QAction* m_toggleRightSideBarAction;
    QAction* m_showSideBarAction;
    QAction* m_hideSideBarAction;
    QAction* m_toggleToolBarAction;
    QAction* m_toggleStatusBarAction;

    // 全屏
    QAction* m_fullScreenAction;
    QAction* m_presentationModeAction;

    // 欢迎屏幕和调试面板
    QAction* m_welcomeScreenToggleAction;
    QAction* m_debugPanelToggleAction;
    QAction* m_debugPanelClearAction;
    QAction* m_debugPanelExportAction;

    // ========================================================================
    // 主题菜单动作
    // ========================================================================
    QAction* m_lightThemeAction;
    QAction* m_darkThemeAction;
    QAction* m_autoThemeAction;

    // 语言
    QAction* m_englishAction;
    QAction* m_chineseAction;

    // ========================================================================
    // 帮助菜单动作
    // ========================================================================
    QAction* m_helpAction;
    QAction* m_aboutAction;
    QAction* m_checkUpdatesAction;

    // ========================================================================
    // 业务逻辑
    // ========================================================================
    RecentFilesManager* m_recentFilesManager;

    // ========================================================================
    // 状态
    // ========================================================================
    bool m_documentOpened;
    QMap<ActionMap, QAction*> m_actionMap;
};

#endif  // ELAMENUBAR_H
