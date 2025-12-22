#pragma once

#include <QList>
#include <QObject>
#include "plugin/PluginInterface.h"

/**
 * @brief Page navigation history entry
 */
struct NavigationEntry {
    QString documentPath;
    int pageNumber;
    double scrollPosition;
    double zoomLevel;
    QDateTime timestamp;
};

/**
 * @brief PageNavigatorPlugin - Advanced page navigation plugin
 *
 * This plugin demonstrates:
 * - **Navigation History**: Back/forward navigation with history stack
 * - **Quick Jump**: Jump to page by number or percentage
 * - **Minimap**: Document overview with current position
 * - **Page Slider**: Visual page navigation slider
 * - **Keyboard Navigation**: Extended keyboard shortcuts
 */
class PageNavigatorPlugin : public PluginBase, public IUIExtension {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE
                          "page_navigator.json")
    Q_INTERFACES(IPluginInterface IUIExtension)

public:
    explicit PageNavigatorPlugin(QObject* parent = nullptr);
    ~PageNavigatorPlugin() override;

    void handleMessage(const QString& from, const QVariant& message) override;

    // IUIExtension
    QList<QAction*> menuActions() const override;
    QList<QAction*> toolbarActions() const override;
    QList<QAction*> contextMenuActions() const override { return {}; }
    QString statusBarMessage() const override;
    QWidget* createDockWidget() override;
    QString menuPath() const override { return "Navigate"; }
    QString toolbarId() const override { return "navigation_toolbar"; }

    // Navigation API
    void goToPage(int pageNumber);
    void goToPercentage(double percentage);
    void goBack();
    void goForward();
    void goToFirstPage();
    void goToLastPage();
    bool canGoBack() const;
    bool canGoForward() const;

protected:
    bool onInitialize() override;
    void onShutdown() override;

private slots:
    void onGoBack();
    void onGoForward();
    void onGoToPage();
    void onGoToFirstPage();
    void onGoToLastPage();

private:
    void registerHooks();
    void setupEventSubscriptions();
    void recordNavigation(int pageNumber);

    QVariant onPageChanged(const QVariantMap& context);

    // Navigation history
    QList<NavigationEntry> m_historyBack;
    QList<NavigationEntry> m_historyForward;
    int m_maxHistorySize;

    // Current state
    QString m_currentDocument;
    int m_currentPage;
    int m_totalPages;

    // UI
    QList<QAction*> m_menuActions;
    QList<QAction*> m_toolbarActions;
    QAction* m_backAction;
    QAction* m_forwardAction;

    // Stats
    int m_navigations;
};
