#include "ContextMenuManager.h"
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QStandardPaths>
#include "../../logging/LoggingMacros.h"

ContextMenuManager::ContextMenuManager(QObject* parent)
    : QObject(parent), m_errorHandlingEnabled(true) {
    // Initialize menu styling
    m_menuStyleSheet = QString(
        "QMenu {"
        "    background-color: #ffffff;"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 6px;"
        "    padding: 4px 0px;"
        "    font-size: 13px;"
        "}"
        "QMenu::item {"
        "    padding: 8px 16px;"
        "    border: none;"
        "    background-color: transparent;"
        "}"
        "QMenu::item:selected {"
        "    background-color: #f8f9fa;"
        "    color: #495057;"
        "}"
        "QMenu::item:disabled {"
        "    color: #6c757d;"
        "}"
        "QMenu::separator {"
        "    height: 1px;"
        "    background-color: #dee2e6;"
        "    margin: 4px 8px;"
        "}"
        "QMenu::icon {"
        "    padding-left: 4px;"
        "}");

    LOG_DEBUG("ContextMenuManager initialized");
}

ContextMenuManager::~ContextMenuManager() {
    clearMenuCache();
    LOG_DEBUG("ContextMenuManager destroyed");
}
void ContextMenuManager::showDocumentViewerMenu(const QPoint& position,
                                                const DocumentContext& context,
                                                QWidget* parent) {
    if (!validateContext(context)) {
        LOG_WARNING(
            "ContextMenuManager::showDocumentViewerMenu() - Invalid context");
        return;
    }

    m_currentDocumentContext = context;

    ElaMenu* menu = createDocumentViewerMenu(context, parent);
    if (menu) {
        applyMenuStyling(menu);
        menu->exec(position);
        menu->deleteLater();
    }
}

void ContextMenuManager::showDocumentTabMenu(const QPoint& position,
                                             int tabIndex,
                                             const UIElementContext& context,
                                             QWidget* parent) {
    if (!validateUIContext(context) || tabIndex < 0) {
        LOG_WARNING(
            "ContextMenuManager::showDocumentTabMenu() - Invalid context or "
            "tab index");
        return;
    }

    m_currentUIContext = context;

    ElaMenu* menu = createDocumentTabMenu(tabIndex, context, parent);
    if (menu) {
        applyMenuStyling(menu);
        menu->exec(position);
        menu->deleteLater();
    }
}

void ContextMenuManager::showSidebarMenu(const QPoint& position,
                                         MenuType menuType,
                                         const UIElementContext& context,
                                         QWidget* parent) {
    if (!validateUIContext(context)) {
        LOG_WARNING("ContextMenuManager::showSidebarMenu() - Invalid context");
        return;
    }

    m_currentUIContext = context;

    ElaMenu* menu = createSidebarMenu(menuType, context, parent);
    if (menu) {
        applyMenuStyling(menu);
        menu->exec(position);
        menu->deleteLater();
    }
}

void ContextMenuManager::showToolbarMenu(const QPoint& position,
                                         const UIElementContext& context,
                                         QWidget* parent) {
    if (!validateUIContext(context)) {
        LOG_WARNING("ContextMenuManager::showToolbarMenu() - Invalid context");
        return;
    }

    m_currentUIContext = context;

    ElaMenu* menu = createToolbarMenu(context, parent);
    if (menu) {
        applyMenuStyling(menu);
        menu->exec(position);
        menu->deleteLater();
    }
}

void ContextMenuManager::showSearchMenu(const QPoint& position,
                                        const UIElementContext& context,
                                        QWidget* parent) {
    if (!validateUIContext(context)) {
        LOG_WARNING("ContextMenuManager::showSearchMenu() - Invalid context");
        return;
    }

    m_currentUIContext = context;

    ElaMenu* menu = createSearchMenu(context, parent);
    if (menu) {
        applyMenuStyling(menu);
        menu->exec(position);
        menu->deleteLater();
    }
}

void ContextMenuManager::showStatusBarMenu(const QPoint& position,
                                           const UIElementContext& context,
                                           QWidget* parent) {
    if (!validateUIContext(context)) {
        LOG_WARNING(
            "ContextMenuManager::showStatusBarMenu() - Invalid context");
        return;
    }

    m_currentUIContext = context;

    ElaMenu* menu = createStatusBarMenu(context, parent);
    if (menu) {
        applyMenuStyling(menu);
        menu->exec(position);
        menu->deleteLater();
    }
}

void ContextMenuManager::showRightSidebarMenu(const QPoint& position,
                                              const UIElementContext& context,
                                              QWidget* parent) {
    if (!validateUIContext(context)) {
        LOG_WARNING(
            "ContextMenuManager::showRightSidebarMenu() - Invalid context");
        return;
    }

    m_currentUIContext = context;

    ElaMenu* menu = createRightSidebarMenu(context, parent);
    if (menu) {
        applyMenuStyling(menu);
        menu->exec(position);
        menu->deleteLater();
    }
}

ElaMenu* ContextMenuManager::createDocumentViewerMenu(
    const DocumentContext& context, QWidget* parent) {
    ElaMenu* menu = new ElaMenu(parent);
    menu->setTitle(tr("Document"));

    // Copy operations
    if (context.hasSelection && context.canCopy) {
        QAction* copyAction = menu->addAction(tr("Copy Text"));
        copyAction->setShortcut(QKeySequence::Copy);
        m_customActionMap[copyAction] = "copyText";
        connect(copyAction, &QAction::triggered, this,
                &ContextMenuManager::onDocumentViewerAction);

        QAction* copyImageAction = menu->addAction(tr("Copy as Image"));
        m_customActionMap[copyImageAction] = "copyAsImage";
        connect(copyImageAction, &QAction::triggered, this,
                &ContextMenuManager::onDocumentViewerAction);

        menu->addSeparator();
    }

    // Page operations submenu
    if (context.hasDocument) {
        ElaMenu* pageSubmenu = createPageSubmenu(menu, context);
        menu->addMenu(pageSubmenu);
    }

    // Zoom operations submenu
    if (context.hasDocument && context.canZoom) {
        ElaMenu* zoomSubmenu = createZoomSubmenu(menu, context);
        menu->addMenu(zoomSubmenu);
    }

    // View operations submenu
    if (context.hasDocument) {
        ElaMenu* viewSubmenu = createViewSubmenu(menu, context);
        menu->addMenu(viewSubmenu);
    }

    menu->addSeparator();

    // Rotation operations
    if (context.hasDocument && context.canRotate) {
        QAction* rotateLeftAction = menu->addAction(tr("Rotate Left"));
        rotateLeftAction->setShortcut(QKeySequence("Ctrl+L"));
        m_actionMap[rotateLeftAction] = ActionMap::rotateLeft;
        connect(rotateLeftAction, &QAction::triggered, this,
                &ContextMenuManager::onDocumentViewerAction);

        QAction* rotateRightAction = menu->addAction(tr("Rotate Right"));
        rotateRightAction->setShortcut(QKeySequence("Ctrl+R"));
        m_actionMap[rotateRightAction] = ActionMap::rotateRight;
        connect(rotateRightAction, &QAction::triggered, this,
                &ContextMenuManager::onDocumentViewerAction);

        menu->addSeparator();
    }

    // Document properties
    if (context.hasDocument) {
        QAction* propertiesAction = menu->addAction(tr("Document Properties"));
        m_customActionMap[propertiesAction] = "showProperties";
        connect(propertiesAction, &QAction::triggered, this,
                &ContextMenuManager::onDocumentViewerAction);
    }

    return menu;
}

ElaMenu* ContextMenuManager::createZoomSubmenu(ElaMenu* parent,
                                               const DocumentContext& context) {
    ElaMenu* zoomMenu = new ElaMenu(tr("Zoom"), parent);

    QAction* zoomInAction = zoomMenu->addAction(tr("Zoom In"));
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    zoomInAction->setEnabled(context.zoomLevel < 4.0);
    m_actionMap[zoomInAction] = ActionMap::zoomIn;
    connect(zoomInAction, &QAction::triggered, this,
            &ContextMenuManager::onDocumentViewerAction);

    QAction* zoomOutAction = zoomMenu->addAction(tr("Zoom Out"));
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    zoomOutAction->setEnabled(context.zoomLevel > 0.25);
    m_actionMap[zoomOutAction] = ActionMap::zoomOut;
    connect(zoomOutAction, &QAction::triggered, this,
            &ContextMenuManager::onDocumentViewerAction);

    zoomMenu->addSeparator();

    QAction* fitWidthAction = zoomMenu->addAction(tr("Fit to Width"));
    fitWidthAction->setShortcut(QKeySequence("Ctrl+1"));
    m_actionMap[fitWidthAction] = ActionMap::fitToWidth;
    connect(fitWidthAction, &QAction::triggered, this,
            &ContextMenuManager::onDocumentViewerAction);

    QAction* fitPageAction = zoomMenu->addAction(tr("Fit to Page"));
    fitPageAction->setShortcut(QKeySequence("Ctrl+0"));
    m_actionMap[fitPageAction] = ActionMap::fitToPage;
    connect(fitPageAction, &QAction::triggered, this,
            &ContextMenuManager::onDocumentViewerAction);

    QAction* fitHeightAction = zoomMenu->addAction(tr("Fit to Height"));
    fitHeightAction->setShortcut(QKeySequence("Ctrl+2"));
    m_actionMap[fitHeightAction] = ActionMap::fitToHeight;
    connect(fitHeightAction, &QAction::triggered, this,
            &ContextMenuManager::onDocumentViewerAction);

    zoomMenu->addSeparator();

    // Zoom presets
    QStringList zoomLevels = {"25%",  "50%",  "75%",  "100%", "125%",
                              "150%", "200%", "300%", "400%"};
    for (const QString& level : zoomLevels) {
        QAction* zoomAction = zoomMenu->addAction(level);
        bool isCurrentLevel =
            qAbs(context.zoomLevel -
                 level.left(level.length() - 1).toDouble() / 100.0) < 0.01;
        zoomAction->setCheckable(true);
        zoomAction->setChecked(isCurrentLevel);

        m_customActionMap[zoomAction] =
            QString("setZoom_%1").arg(level.left(level.length() - 1));
        connect(zoomAction, &QAction::triggered, this,
                &ContextMenuManager::onDocumentViewerAction);
    }

    return zoomMenu;
}

ElaMenu* ContextMenuManager::createPageSubmenu(ElaMenu* parent,
                                               const DocumentContext& context) {
    ElaMenu* pageMenu = new ElaMenu(tr("Page"), parent);

    QAction* firstPageAction = pageMenu->addAction(tr("First Page"));
    firstPageAction->setShortcut(QKeySequence("Ctrl+Home"));
    firstPageAction->setEnabled(context.currentPage > 0);
    m_actionMap[firstPageAction] = ActionMap::firstPage;
    connect(firstPageAction, &QAction::triggered, this,
            &ContextMenuManager::onDocumentViewerAction);

    QAction* prevPageAction = pageMenu->addAction(tr("Previous Page"));
    prevPageAction->setShortcut(QKeySequence("Page Up"));
    prevPageAction->setEnabled(context.currentPage > 0);
    m_actionMap[prevPageAction] = ActionMap::previousPage;
    connect(prevPageAction, &QAction::triggered, this,
            &ContextMenuManager::onDocumentViewerAction);

    QAction* nextPageAction = pageMenu->addAction(tr("Next Page"));
    nextPageAction->setShortcut(QKeySequence("Page Down"));
    nextPageAction->setEnabled(context.currentPage < context.totalPages - 1);
    m_actionMap[nextPageAction] = ActionMap::nextPage;
    connect(nextPageAction, &QAction::triggered, this,
            &ContextMenuManager::onDocumentViewerAction);

    QAction* lastPageAction = pageMenu->addAction(tr("Last Page"));
    lastPageAction->setShortcut(QKeySequence("Ctrl+End"));
    lastPageAction->setEnabled(context.currentPage < context.totalPages - 1);
    m_actionMap[lastPageAction] = ActionMap::lastPage;
    connect(lastPageAction, &QAction::triggered, this,
            &ContextMenuManager::onDocumentViewerAction);

    pageMenu->addSeparator();

    QAction* goToPageAction = pageMenu->addAction(tr("Go to Page..."));
    goToPageAction->setShortcut(QKeySequence("Ctrl+G"));
    m_customActionMap[goToPageAction] = "goToPage";
    connect(goToPageAction, &QAction::triggered, this,
            &ContextMenuManager::onDocumentViewerAction);

    return pageMenu;
}

ElaMenu* ContextMenuManager::createViewSubmenu(ElaMenu* parent,
                                               const DocumentContext& context) {
    Q_UNUSED(context)

    ElaMenu* viewMenu = new ElaMenu(tr("View"), parent);

    QAction* singlePageAction = viewMenu->addAction(tr("Single Page"));
    singlePageAction->setCheckable(true);
    m_actionMap[singlePageAction] = ActionMap::setSinglePageMode;
    connect(singlePageAction, &QAction::triggered, this,
            &ContextMenuManager::onDocumentViewerAction);

    QAction* continuousAction = viewMenu->addAction(tr("Continuous Scroll"));
    continuousAction->setCheckable(true);
    m_actionMap[continuousAction] = ActionMap::setContinuousScrollMode;
    connect(continuousAction, &QAction::triggered, this,
            &ContextMenuManager::onDocumentViewerAction);

    viewMenu->addSeparator();

    QAction* fullscreenAction = viewMenu->addAction(tr("Fullscreen"));
    fullscreenAction->setShortcut(QKeySequence("F11"));
    fullscreenAction->setCheckable(true);
    m_customActionMap[fullscreenAction] = "toggleFullscreen";
    connect(fullscreenAction, &QAction::triggered, this,
            &ContextMenuManager::onDocumentViewerAction);

    return viewMenu;
}

ElaMenu* ContextMenuManager::createDocumentTabMenu(
    int tabIndex, const UIElementContext& context, QWidget* parent) {
    ElaMenu* menu = new ElaMenu(parent);
    menu->setTitle(tr("Tab"));

    // Store tab index in context for actions
    QVariantMap tabContext;
    tabContext["tabIndex"] = tabIndex;

    // Close operations
    QAction* closeAction = menu->addAction(tr("Close"));
    closeAction->setShortcut(QKeySequence::Close);
    m_customActionMap[closeAction] = "closeTab";
    m_actionContextMap[closeAction] = tabContext;
    connect(closeAction, &QAction::triggered, this,
            &ContextMenuManager::onTabAction);

    QAction* closeOthersAction = menu->addAction(tr("Close Others"));
    m_customActionMap[closeOthersAction] = "closeOtherTabs";
    m_actionContextMap[closeOthersAction] = tabContext;
    connect(closeOthersAction, &QAction::triggered, this,
            &ContextMenuManager::onTabAction);

    QAction* closeAllAction = menu->addAction(tr("Close All"));
    m_customActionMap[closeAllAction] = "closeAllTabs";
    connect(closeAllAction, &QAction::triggered, this,
            &ContextMenuManager::onTabAction);

    menu->addSeparator();

    // Tab management
    QAction* newTabAction = menu->addAction(tr("New Tab"));
    newTabAction->setShortcut(QKeySequence::New);
    m_customActionMap[newTabAction] = "newTab";
    connect(newTabAction, &QAction::triggered, this,
            &ContextMenuManager::onTabAction);

    QAction* duplicateAction = menu->addAction(tr("Duplicate Tab"));
    m_customActionMap[duplicateAction] = "duplicateTab";
    m_actionContextMap[duplicateAction] = tabContext;
    connect(duplicateAction, &QAction::triggered, this,
            &ContextMenuManager::onTabAction);

    menu->addSeparator();

    // Recent files submenu
    ElaMenu* recentMenu = new ElaMenu(tr("Recent Files"), menu);

    // Add placeholder for recent files (would be populated from recent files
    // manager)
    QAction* noRecentAction = recentMenu->addAction(tr("No recent files"));
    noRecentAction->setEnabled(false);

    menu->addMenu(recentMenu);

    menu->addSeparator();

    // Tab properties
    QAction* renameAction = menu->addAction(tr("Rename Tab"));
    renameAction->setShortcut(QKeySequence("F2"));
    m_customActionMap[renameAction] = "renameTab";
    m_actionContextMap[renameAction] = tabContext;
    connect(renameAction, &QAction::triggered, this,
            &ContextMenuManager::onTabAction);

    QAction* propertiesAction = menu->addAction(tr("Tab Properties"));
    m_customActionMap[propertiesAction] = "tabProperties";
    m_actionContextMap[propertiesAction] = tabContext;
    connect(propertiesAction, &QAction::triggered, this,
            &ContextMenuManager::onTabAction);

    return menu;
}

ElaMenu* ContextMenuManager::createSidebarMenu(MenuType menuType,
                                               const UIElementContext& context,
                                               QWidget* parent) {
    ElaMenu* menu = new ElaMenu(parent);

    if (menuType == MenuType::SidebarThumbnail) {
        menu->setTitle(tr("Thumbnails"));

        // Thumbnail size options
        ElaMenu* sizeMenu = new ElaMenu(tr("Thumbnail Size"), menu);

        QStringList sizes = {"Small", "Medium", "Large", "Extra Large"};
        QList<QSize> sizeValues = {QSize(80, 100), QSize(120, 160),
                                   QSize(160, 200), QSize(200, 260)};

        for (int i = 0; i < sizes.size(); ++i) {
            QAction* sizeAction = sizeMenu->addAction(sizes[i]);
            sizeAction->setCheckable(true);

            QVariantMap sizeContext;
            sizeContext["width"] = sizeValues[i].width();
            sizeContext["height"] = sizeValues[i].height();

            m_customActionMap[sizeAction] = "setThumbnailSize";
            m_actionContextMap[sizeAction] = sizeContext;
            connect(sizeAction, &QAction::triggered, this,
                    &ContextMenuManager::onSidebarAction);
        }

        menu->addMenu(sizeMenu);
        menu->addSeparator();

        // Thumbnail operations
        if (context.elementIndex >= 0) {
            QVariantMap pageContext;
            pageContext["pageIndex"] = context.elementIndex;

            QAction* goToPageAction = menu->addAction(tr("Go to Page"));
            m_customActionMap[goToPageAction] = "goToThumbnailPage";
            m_actionContextMap[goToPageAction] = pageContext;
            connect(goToPageAction, &QAction::triggered, this,
                    &ContextMenuManager::onSidebarAction);

            QAction* copyPageAction = menu->addAction(tr("Copy Page"));
            m_customActionMap[copyPageAction] = "copyThumbnailPage";
            m_actionContextMap[copyPageAction] = pageContext;
            connect(copyPageAction, &QAction::triggered, this,
                    &ContextMenuManager::onSidebarAction);

            menu->addSeparator();
        }

        // View options
        QAction* refreshAction = menu->addAction(tr("Refresh Thumbnails"));
        refreshAction->setShortcut(QKeySequence::Refresh);
        m_customActionMap[refreshAction] = "refreshThumbnails";
        connect(refreshAction, &QAction::triggered, this,
                &ContextMenuManager::onSidebarAction);

    } else if (menuType == MenuType::SidebarBookmark) {
        menu->setTitle(tr("Bookmarks"));

        // Bookmark operations
        QAction* addBookmarkAction = menu->addAction(tr("Add Bookmark"));
        addBookmarkAction->setShortcut(QKeySequence("Ctrl+D"));
        m_customActionMap[addBookmarkAction] = "addBookmark";
        connect(addBookmarkAction, &QAction::triggered, this,
                &ContextMenuManager::onSidebarAction);

        if (context.elementIndex >= 0) {
            QVariantMap bookmarkContext;
            bookmarkContext["bookmarkIndex"] = context.elementIndex;

            menu->addSeparator();

            QAction* editBookmarkAction = menu->addAction(tr("Edit Bookmark"));
            m_customActionMap[editBookmarkAction] = "editBookmark";
            m_actionContextMap[editBookmarkAction] = bookmarkContext;
            connect(editBookmarkAction, &QAction::triggered, this,
                    &ContextMenuManager::onSidebarAction);

            QAction* deleteBookmarkAction =
                menu->addAction(tr("Delete Bookmark"));
            deleteBookmarkAction->setShortcut(QKeySequence::Delete);
            m_customActionMap[deleteBookmarkAction] = "deleteBookmark";
            m_actionContextMap[deleteBookmarkAction] = bookmarkContext;
            connect(deleteBookmarkAction, &QAction::triggered, this,
                    &ContextMenuManager::onSidebarAction);

            menu->addSeparator();

            QAction* goToBookmarkAction = menu->addAction(tr("Go to Bookmark"));
            m_customActionMap[goToBookmarkAction] = "goToBookmark";
            m_actionContextMap[goToBookmarkAction] = bookmarkContext;
            connect(goToBookmarkAction, &QAction::triggered, this,
                    &ContextMenuManager::onSidebarAction);
        }

        menu->addSeparator();

        // Bookmark management
        QAction* importBookmarksAction =
            menu->addAction(tr("Import Bookmarks..."));
        m_customActionMap[importBookmarksAction] = "importBookmarks";
        connect(importBookmarksAction, &QAction::triggered, this,
                &ContextMenuManager::onSidebarAction);

        QAction* exportBookmarksAction =
            menu->addAction(tr("Export Bookmarks..."));
        m_customActionMap[exportBookmarksAction] = "exportBookmarks";
        connect(exportBookmarksAction, &QAction::triggered, this,
                &ContextMenuManager::onSidebarAction);
    }

    return menu;
}
ElaMenu* ContextMenuManager::createToolbarMenu(const UIElementContext& context,
                                               QWidget* parent) {
    Q_UNUSED(context)

    ElaMenu* menu = new ElaMenu(parent);
    menu->setTitle(tr("Toolbar"));

    // Toolbar customization
    QAction* customizeAction = menu->addAction(tr("Customize Toolbar..."));
    m_customActionMap[customizeAction] = "customizeToolbar";
    connect(customizeAction, &QAction::triggered, this,
            &ContextMenuManager::onToolbarAction);

    QAction* resetToolbarAction = menu->addAction(tr("Reset Toolbar"));
    m_customActionMap[resetToolbarAction] = "resetToolbar";
    connect(resetToolbarAction, &QAction::triggered, this,
            &ContextMenuManager::onToolbarAction);

    menu->addSeparator();

    // Toolbar visibility
    QAction* showToolbarAction = menu->addAction(tr("Show Toolbar"));
    showToolbarAction->setCheckable(true);
    showToolbarAction->setChecked(true);
    m_customActionMap[showToolbarAction] = "toggleToolbar";
    connect(showToolbarAction, &QAction::triggered, this,
            &ContextMenuManager::onToolbarAction);

    QAction* lockToolbarAction = menu->addAction(tr("Lock Toolbar"));
    lockToolbarAction->setCheckable(true);
    m_customActionMap[lockToolbarAction] = "lockToolbar";
    connect(lockToolbarAction, &QAction::triggered, this,
            &ContextMenuManager::onToolbarAction);

    return menu;
}

ElaMenu* ContextMenuManager::createSearchMenu(const UIElementContext& context,
                                              QWidget* parent) {
    Q_UNUSED(context)

    ElaMenu* menu = new ElaMenu(parent);
    menu->setTitle(tr("Search"));

    // Search options
    QAction* caseSensitiveAction = menu->addAction(tr("Case Sensitive"));
    caseSensitiveAction->setCheckable(true);
    m_customActionMap[caseSensitiveAction] = "toggleCaseSensitive";
    connect(caseSensitiveAction, &QAction::triggered, this,
            &ContextMenuManager::onSearchAction);

    QAction* wholeWordsAction = menu->addAction(tr("Whole Words"));
    wholeWordsAction->setCheckable(true);
    m_customActionMap[wholeWordsAction] = "toggleWholeWords";
    connect(wholeWordsAction, &QAction::triggered, this,
            &ContextMenuManager::onSearchAction);

    QAction* regexAction = menu->addAction(tr("Regular Expression"));
    regexAction->setCheckable(true);
    m_customActionMap[regexAction] = "toggleRegex";
    connect(regexAction, &QAction::triggered, this,
            &ContextMenuManager::onSearchAction);

    menu->addSeparator();

    // Search history
    ElaMenu* historyMenu = new ElaMenu(tr("Search History"), menu);

    QAction* clearHistoryAction = historyMenu->addAction(tr("Clear History"));
    m_customActionMap[clearHistoryAction] = "clearSearchHistory";
    connect(clearHistoryAction, &QAction::triggered, this,
            &ContextMenuManager::onSearchAction);

    menu->addMenu(historyMenu);

    menu->addSeparator();

    // Advanced search
    QAction* advancedSearchAction = menu->addAction(tr("Advanced Search..."));
    m_customActionMap[advancedSearchAction] = "showAdvancedSearch";
    connect(advancedSearchAction, &QAction::triggered, this,
            &ContextMenuManager::onSearchAction);

    return menu;
}

ElaMenu* ContextMenuManager::createStatusBarMenu(
    const UIElementContext& context, QWidget* parent) {
    Q_UNUSED(context)

    ElaMenu* menu = new ElaMenu(parent);
    menu->setTitle(tr("Status Bar"));

    // Status bar elements
    QAction* showPageInfoAction = menu->addAction(tr("Show Page Info"));
    showPageInfoAction->setCheckable(true);
    showPageInfoAction->setChecked(true);
    m_customActionMap[showPageInfoAction] = "togglePageInfo";
    connect(showPageInfoAction, &QAction::triggered, this,
            &ContextMenuManager::onToolbarAction);

    QAction* showZoomInfoAction = menu->addAction(tr("Show Zoom Info"));
    showZoomInfoAction->setCheckable(true);
    showZoomInfoAction->setChecked(true);
    m_customActionMap[showZoomInfoAction] = "toggleZoomInfo";
    connect(showZoomInfoAction, &QAction::triggered, this,
            &ContextMenuManager::onToolbarAction);

    QAction* showDocumentInfoAction = menu->addAction(tr("Show Document Info"));
    showDocumentInfoAction->setCheckable(true);
    showDocumentInfoAction->setChecked(true);
    m_customActionMap[showDocumentInfoAction] = "toggleDocumentInfo";
    connect(showDocumentInfoAction, &QAction::triggered, this,
            &ContextMenuManager::onToolbarAction);

    menu->addSeparator();

    // Status bar visibility
    QAction* hideStatusBarAction = menu->addAction(tr("Hide Status Bar"));
    m_customActionMap[hideStatusBarAction] = "hideStatusBar";
    connect(hideStatusBarAction, &QAction::triggered, this,
            &ContextMenuManager::onToolbarAction);

    return menu;
}

ElaMenu* ContextMenuManager::createRightSidebarMenu(
    const UIElementContext& context, QWidget* parent) {
    Q_UNUSED(context)

    ElaMenu* menu = new ElaMenu(parent);
    menu->setTitle(tr("Right Sidebar"));

    // Panel visibility
    QAction* showAnnotationsAction = menu->addAction(tr("Show Annotations"));
    showAnnotationsAction->setCheckable(true);
    m_customActionMap[showAnnotationsAction] = "toggleAnnotations";
    connect(showAnnotationsAction, &QAction::triggered, this,
            &ContextMenuManager::onSidebarAction);

    QAction* showMetadataAction = menu->addAction(tr("Show Metadata"));
    showMetadataAction->setCheckable(true);
    m_customActionMap[showMetadataAction] = "toggleMetadata";
    connect(showMetadataAction, &QAction::triggered, this,
            &ContextMenuManager::onSidebarAction);

    QAction* showSearchResultsAction =
        menu->addAction(tr("Show Search Results"));
    showSearchResultsAction->setCheckable(true);
    m_customActionMap[showSearchResultsAction] = "toggleSearchResults";
    connect(showSearchResultsAction, &QAction::triggered, this,
            &ContextMenuManager::onSidebarAction);

    menu->addSeparator();

    // Sidebar management
    QAction* hideSidebarAction = menu->addAction(tr("Hide Right Sidebar"));
    m_customActionMap[hideSidebarAction] = "hideRightSidebar";
    connect(hideSidebarAction, &QAction::triggered, this,
            &ContextMenuManager::onSidebarAction);

    return menu;
}

void ContextMenuManager::onDocumentViewerAction() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) {
        LOG_WARNING(
            "ContextMenuManager::onDocumentViewerAction() - Invalid sender");
        return;
    }

    // Handle standard actions
    if (m_actionMap.contains(action)) {
        ActionMap actionType = m_actionMap[action];
        QVariantMap context = m_actionContextMap.value(action);
        executeAction(actionType, context);
        return;
    }

    // Handle custom actions
    if (m_customActionMap.contains(action)) {
        QString actionId = m_customActionMap[action];
        QVariantMap context = m_actionContextMap.value(action);
        executeCustomAction(actionId, context);
        return;
    }

    LOG_WARNING(
        "ContextMenuManager::onDocumentViewerAction() - Unknown action");
}

void ContextMenuManager::onTabAction() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) {
        LOG_WARNING("ContextMenuManager::onTabAction() - Invalid sender");
        return;
    }

    if (m_customActionMap.contains(action)) {
        QString actionId = m_customActionMap[action];
        QVariantMap context = m_actionContextMap.value(action);
        executeCustomAction(actionId, context);
    } else {
        LOG_WARNING("ContextMenuManager::onTabAction() - Unknown action");
    }
}

void ContextMenuManager::onSidebarAction() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) {
        LOG_WARNING("ContextMenuManager::onSidebarAction() - Invalid sender");
        return;
    }

    if (m_customActionMap.contains(action)) {
        QString actionId = m_customActionMap[action];
        QVariantMap context = m_actionContextMap.value(action);
        executeCustomAction(actionId, context);
    } else {
        LOG_WARNING("ContextMenuManager::onSidebarAction() - Unknown action");
    }
}

void ContextMenuManager::onToolbarAction() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) {
        LOG_WARNING("ContextMenuManager::onToolbarAction() - Invalid sender");
        return;
    }

    if (m_customActionMap.contains(action)) {
        QString actionId = m_customActionMap[action];
        QVariantMap context = m_actionContextMap.value(action);
        executeCustomAction(actionId, context);
    } else {
        LOG_WARNING("ContextMenuManager::onToolbarAction() - Unknown action");
    }
}

void ContextMenuManager::onSearchAction() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) {
        LOG_WARNING("ContextMenuManager::onSearchAction() - Invalid sender");
        return;
    }

    if (m_customActionMap.contains(action)) {
        QString actionId = m_customActionMap[action];
        QVariantMap context = m_actionContextMap.value(action);
        executeCustomAction(actionId, context);
    } else {
        LOG_WARNING("ContextMenuManager::onSearchAction() - Unknown action");
    }
}

void ContextMenuManager::executeAction(ActionMap action,
                                       const QVariantMap& context) {
    if (!m_errorHandlingEnabled) {
        emit actionTriggered(action, context);
        return;
    }

    try {
        LOG_DEBUG("ContextMenuManager::executeAction() - Executing action: {}",
                  static_cast<int>(action));
        emit actionTriggered(action, context);
    } catch (const std::exception& e) {
        LOG_ERROR(
            "ContextMenuManager::executeAction() - Error executing action {}: "
            "{}",
            static_cast<int>(action), e.what());

        QMessageBox::warning(nullptr, tr("Action Error"),
                             tr("Failed to execute action: %1").arg(e.what()));
    } catch (...) {
        LOG_ERROR(
            "ContextMenuManager::executeAction() - Unknown error executing "
            "action: {}",
            static_cast<int>(action));

        QMessageBox::warning(
            nullptr, tr("Action Error"),
            tr("An unknown error occurred while executing the action."));
    }
}

void ContextMenuManager::executeCustomAction(const QString& actionId,
                                             const QVariantMap& context) {
    if (!m_errorHandlingEnabled) {
        emit customActionTriggered(actionId, context);
        return;
    }

    try {
        LOG_DEBUG(
            "ContextMenuManager::executeCustomAction() - Executing custom "
            "action: {}",
            actionId.toStdString());
        emit customActionTriggered(actionId, context);
    } catch (const std::exception& e) {
        LOG_ERROR(
            "ContextMenuManager::executeCustomAction() - Error executing "
            "custom action {}: {}",
            actionId.toStdString(), e.what());

        QMessageBox::warning(
            nullptr, tr("Action Error"),
            tr("Failed to execute action '%1': %2").arg(actionId, e.what()));
    } catch (...) {
        LOG_ERROR(
            "ContextMenuManager::executeCustomAction() - Unknown error "
            "executing custom action: {}",
            actionId.toStdString());

        QMessageBox::warning(
            nullptr, tr("Action Error"),
            tr("An unknown error occurred while executing action '%1'.")
                .arg(actionId));
    }
}

void ContextMenuManager::updateMenuStates(
    const DocumentContext& documentContext) {
    m_currentDocumentContext = documentContext;

    // Clear cached menus to force recreation with updated states
    clearMenuCache();

    LOG_DEBUG("ContextMenuManager::updateMenuStates() - Menu states updated");
}

void ContextMenuManager::clearMenuCache() {
    for (auto it = m_menuCache.begin(); it != m_menuCache.end(); ++it) {
        if (it.value()) {
            it.value()->deleteLater();
        }
    }
    m_menuCache.clear();

    // Clear action mappings
    m_actionMap.clear();
    m_customActionMap.clear();
    m_actionContextMap.clear();

    LOG_DEBUG("ContextMenuManager::clearMenuCache() - Menu cache cleared");
}

void ContextMenuManager::applyMenuStyling(ElaMenu* menu) {
    // Delegate to the QMenu* overload for unified handling
    applyMenuStyling(static_cast<QMenu*>(menu));
}

void ContextMenuManager::applyMenuStyling(QMenu* menu) {
    if (!menu) {
        return;
    }

    menu->setStyleSheet(m_menuStyleSheet);

    // Apply styling to submenus recursively
    for (QAction* action : menu->actions()) {
        if (action->menu()) {
            applyMenuStyling(action->menu());
        }
    }
}

bool ContextMenuManager::validateContext(const DocumentContext& context) const {
    // Basic validation - can be extended as needed
    if (context.hasDocument && context.totalPages <= 0) {
        LOG_WARNING(
            "ContextMenuManager::validateContext() - Invalid document context: "
            "has document but no pages");
        return false;
    }

    if (context.hasDocument && (context.currentPage < 0 ||
                                context.currentPage >= context.totalPages)) {
        LOG_WARNING(
            "ContextMenuManager::validateContext() - Invalid document context: "
            "current page out of range");
        return false;
    }

    if (context.zoomLevel <= 0.0) {
        LOG_WARNING(
            "ContextMenuManager::validateContext() - Invalid document context: "
            "invalid zoom level");
        return false;
    }

    return true;
}

bool ContextMenuManager::validateUIContext(
    const UIElementContext& context) const {
    // Basic validation - can be extended as needed
    if (!context.targetWidget) {
        LOG_WARNING(
            "ContextMenuManager::validateUIContext() - Invalid UI context: no "
            "target widget");
        return false;
    }

    return true;
}

// MOC file will be generated automatically by the build system
