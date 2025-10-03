#pragma once

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QObject>
#include <QShortcut>
#include <QString>
#include "../../controller/tool.hpp"
#include "../../managers/RecentFilesManager.h"

class MenuBar : public QMenuBar {
    Q_OBJECT

public:
    MenuBar(QWidget* parent = nullptr);

signals:
    void themeChanged(const QString& theme);
    void languageChanged(const QString& languageCode);
    void onExecuted(ActionMap id, QWidget* context = nullptr);
    void openRecentFileRequested(const QString& filePath);
    void welcomeScreenToggleRequested();
    void debugPanelToggleRequested();
    void debugPanelClearRequested();
    void debugPanelExportRequested();

public slots:
    void setRecentFilesManager(RecentFilesManager* manager);
    void setWelcomeScreenEnabled(bool enabled);

protected:
    void changeEvent(QEvent* event) override;

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
    void setupRecentFileShortcuts();
    void retranslateUi();

    RecentFilesManager* m_recentFilesManager;
    QMenu* m_recentFilesMenu;
    QAction* m_clearRecentFilesAction;
    QAction* m_welcomeScreenToggleAction;
    QAction* m_debugPanelToggleAction;
    QAction* m_debugPanelClearAction;
    QAction* m_debugPanelExportAction;

    // Keyboard shortcuts for recent files (Ctrl+Alt+1-9)
    QShortcut* m_recentFileShortcuts[9];
};
