#pragma once

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QObject>
#include <QString>
#include "../../controller/tool.hpp"
#include "../../managers/RecentFilesManager.h"

class MenuBar : public QMenuBar {
    Q_OBJECT

public:
    MenuBar(QWidget* parent = nullptr);

signals:
    void themeChanged(const QString& theme);
    void onExecuted(ActionMap id, QWidget* context = nullptr);
    void openRecentFileRequested(const QString& filePath);
    void welcomeScreenToggleRequested();
    void debugPanelToggleRequested();
    void debugPanelClearRequested();
    void debugPanelExportRequested();

public slots:
    void setRecentFilesManager(RecentFilesManager* manager);
    void setWelcomeScreenEnabled(bool enabled);

private slots:
    void updateRecentFilesMenu();
    void onRecentFileTriggered();
    void onClearRecentFilesTriggered();

private:
    void createFileMenu();
    void createTabMenu();
    void createViewMenu();
    void createThemeMenu();
    void setupRecentFilesMenu();

    RecentFilesManager* m_recentFilesManager;
    QMenu* m_recentFilesMenu;
    QAction* m_clearRecentFilesAction;
    QAction* m_welcomeScreenToggleAction;
    QAction* m_debugPanelToggleAction;
    QAction* m_debugPanelClearAction;
    QAction* m_debugPanelExportAction;
};
