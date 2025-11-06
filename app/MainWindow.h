#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <memory>
#include "ElaWindow.h"

// Forward declarations - Business Logic Layer
class ApplicationController;
class DocumentController;
class SearchEngine;
class RecentFilesManager;
class I18nManager;
class StyleManager;

// Forward declarations - Pages
class HomePage;
class PDFViewerPage;
class SettingsPage;
class AboutPage;

// Forward declarations - Adapters
class DocumentAdapter;
class ViewAdapter;

/**
 * @brief MainWindow - 主窗口（使用 ElaWidgetTools）
 *
 * 这是使用 ElaWidgetTools 重新实现的主窗口，继承自 ElaWindow。
 * 复用现有的业务逻辑层（app_lib），仅重新实现 UI 层。
 *
 * 架构设计：
 * - UI 层：使用 ElaWidgetTools 组件
 * - 业务逻辑层：复用 app_lib 中的 Controller、Model、Service
 * - 适配层：ElaDocumentController、ElaViewDelegate 等适配器
 */
class MainWindow : public ElaWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // Command-line integration methods (compatible with existing app)
    void openFileFromCommandLine(const QString& filePath);
    void setViewModeFromCommandLine(int mode);
    void setZoomLevelFromCommandLine(double zoom);
    void goToPageFromCommandLine(int page);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    // Initialization methods
    void initWindow();
    void initTheme();
    void initNavigation();
    void initPages();
    void initBusinessLogic();
    void connectSignals();

    // UI update methods
    void retranslateUi();
    void updateTheme();

    // State management
    void restoreWindowState();

private slots:
    // Business logic slots
    void onDocumentLoaded(const QString& filePath);
    void onDocumentClosed();
    void onPageChanged(int currentPage, int totalPages);
    void onZoomChanged(double zoomFactor);
    void onSearchCompleted(int resultCount);
    void onError(const QString& context, const QString& error);

    // Navigation slots
    void onNavigationNodeClicked(ElaNavigationType::NavigationNodeType nodeType,
                                 QString nodeKey);
    void onThemeChanged(ElaThemeType::ThemeMode themeMode);
    void onLanguageChanged(const QString& languageCode);

private:
    // ========================================================================
    // Pages (UI Layer)
    // ========================================================================
    HomePage* m_homePage;
    PDFViewerPage* m_pdfViewerPage;
    SettingsPage* m_settingsPage;
    AboutPage* m_aboutPage;

    // ========================================================================
    // Business Logic Layer (from app_lib)
    // ========================================================================
    std::unique_ptr<ApplicationController> m_applicationController;
    std::unique_ptr<DocumentController> m_documentController;
    std::unique_ptr<SearchEngine> m_searchEngine;
    std::unique_ptr<RecentFilesManager> m_recentFilesManager;

    // ========================================================================
    // Adapters (Bridge between UI and Business Logic)
    // ========================================================================
    std::unique_ptr<DocumentAdapter> m_elaDocumentAdapter;
    std::unique_ptr<ViewAdapter> m_elaViewAdapter;

    // ========================================================================
    // Navigation Keys
    // ========================================================================
    QString m_homeKey;
    QString m_documentsKey;
    QString m_pdfViewerKey;
    QString m_recentFilesKey;
    QString m_toolsKey;
    QString m_searchKey;
    QString m_bookmarksKey;
    QString m_annotationsKey;
    QString m_settingsKey;
    QString m_aboutKey;

    // ========================================================================
    // State
    // ========================================================================
    bool m_isInitialized;
    QString m_currentDocumentPath;
    int m_currentPage;
    int m_totalPages;
    double m_currentZoom;
};

#endif  // MAINWINDOW_H
