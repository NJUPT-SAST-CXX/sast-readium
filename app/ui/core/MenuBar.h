#pragma once

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QObject>
#include <QShortcut>
#include <QString>
#include "../../controller/tool.hpp"
#include "../../managers/RecentFilesManager.h"

/**
 * @brief Main menu bar with File, Tabs, View, and Settings menus
 *
 * @details Provides comprehensive menu functionality including:
 * - File operations (open, save, print, email)
 * - Recent files management with intelligent path truncation
 * - Tab management with keyboard shortcuts (Ctrl+Alt+1-9)
 * - View controls (sidebar, fullscreen, theme, language)
 * - Settings and help menus
 *
 * @note All menu items are properly internationalized and update on language changes
 */
class MenuBar : public QMenuBar {
    Q_OBJECT

public:
    /**
     * @brief Construct a new Menu Bar object
     * @param parent Parent widget (optional)
     */
    MenuBar(QWidget* parent = nullptr);

    /**
     * @brief Destroy the Menu Bar object and clean up resources
     */
    ~MenuBar();

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
