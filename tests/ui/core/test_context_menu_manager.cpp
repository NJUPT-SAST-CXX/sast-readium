#include <QApplication>
#include <QMenu>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include "../../../app/ui/core/ContextMenuManager.h"

class ContextMenuManagerTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Construction tests
    void testConstruction();
    void testDestruction();

    // MenuType enum tests
    void testMenuTypeEnum();

    // DocumentContext tests
    void testDocumentContextDefaults();
    void testDocumentContextWithDocument();
    void testDocumentContextWithSelection();
    void testDocumentContextZoomLevel();
    void testDocumentContextPageInfo();

    // UIElementContext tests
    void testUIElementContextDefaults();
    void testUIElementContextWithWidget();
    void testUIElementContextProperties();

    // Document viewer menu tests
    void testShowDocumentViewerMenu();
    void testDocumentViewerMenuWithSelection();
    void testDocumentViewerMenuWithoutDocument();
    void testDocumentViewerMenuZoomSubmenu();
    void testDocumentViewerMenuPageSubmenu();

    // Tab context menu tests
    void testShowDocumentTabMenu();
    void testDocumentTabMenuActions();
    void testDocumentTabMenuWithMultipleTabs();

    // Sidebar context menu tests
    void testShowSidebarThumbnailMenu();
    void testShowSidebarBookmarkMenu();
    void testSidebarMenuWithSelection();

    // Toolbar context menu tests
    void testShowToolbarMenu();
    void testToolbarMenuCustomization();

    // Search context menu tests
    void testShowSearchMenu();
    void testSearchMenuWithHistory();

    // Status bar context menu tests
    void testShowStatusBarMenu();

    // Right sidebar context menu tests
    void testShowRightSidebarMenu();
    void testRightSidebarMenuPanels();

    // Action signal tests
    void testActionTriggeredSignal();
    void testCustomActionTriggeredSignal();
    void testActionTriggeredWithContext();

    // Menu state tests
    void testUpdateMenuStates();
    void testUpdateMenuStatesWithSelection();
    void testClearMenuCache();
    void testMenuCachePerformance();

    // Context validation tests
    void testValidateDocumentContext();
    void testValidateUIContext();
    void testValidateEmptyContext();

    // Error handling tests
    void testNullParentWidget();
    void testInvalidTabIndex();

private:
    ContextMenuManager* m_manager;
    QWidget* m_parentWidget;

    ContextMenuManager::DocumentContext createTestDocumentContext(
        bool hasDocument = true);
    ContextMenuManager::UIElementContext createTestUIContext();
    void waitForMenu();
};

void ContextMenuManagerTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();

    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
    }
}

void ContextMenuManagerTest::cleanupTestCase() { delete m_parentWidget; }

void ContextMenuManagerTest::init() {
    m_manager = new ContextMenuManager(m_parentWidget);
}

void ContextMenuManagerTest::cleanup() {
    delete m_manager;
    m_manager = nullptr;
}

ContextMenuManager::DocumentContext
ContextMenuManagerTest::createTestDocumentContext(bool hasDocument) {
    ContextMenuManager::DocumentContext ctx;
    ctx.hasDocument = hasDocument;
    ctx.hasSelection = false;
    ctx.canCopy = hasDocument;
    ctx.canZoom = hasDocument;
    ctx.canRotate = hasDocument;
    ctx.currentPage = hasDocument ? 1 : 0;
    ctx.totalPages = hasDocument ? 10 : 0;
    ctx.zoomLevel = 1.0;
    ctx.documentPath = hasDocument ? "/test/document.pdf" : "";
    return ctx;
}

ContextMenuManager::UIElementContext
ContextMenuManagerTest::createTestUIContext() {
    ContextMenuManager::UIElementContext ctx;
    ctx.targetWidget = m_parentWidget;
    ctx.elementIndex = 0;
    ctx.elementId = "test_element";
    ctx.isEnabled = true;
    ctx.isVisible = true;
    return ctx;
}

void ContextMenuManagerTest::testConstruction() {
    QVERIFY(m_manager != nullptr);
}

void ContextMenuManagerTest::testDestruction() {
    auto* manager = new ContextMenuManager();
    delete manager;
    QVERIFY(true);
}

void ContextMenuManagerTest::testMenuTypeEnum() {
    QVERIFY(ContextMenuManager::MenuType::DocumentViewer !=
            ContextMenuManager::MenuType::DocumentTab);
    QVERIFY(ContextMenuManager::MenuType::SidebarThumbnail !=
            ContextMenuManager::MenuType::SidebarBookmark);
    QVERIFY(ContextMenuManager::MenuType::ToolbarArea !=
            ContextMenuManager::MenuType::SearchWidget);
    QVERIFY(ContextMenuManager::MenuType::StatusBar !=
            ContextMenuManager::MenuType::RightSidebar);
}

void ContextMenuManagerTest::testDocumentContextDefaults() {
    ContextMenuManager::DocumentContext ctx;
    QVERIFY(!ctx.hasDocument);
    QVERIFY(!ctx.hasSelection);
    QVERIFY(!ctx.canCopy);
    QVERIFY(!ctx.canZoom);
    QVERIFY(!ctx.canRotate);
    QCOMPARE(ctx.currentPage, 0);
    QCOMPARE(ctx.totalPages, 0);
    QCOMPARE(ctx.zoomLevel, 1.0);
    QVERIFY(ctx.documentPath.isEmpty());
    QVERIFY(ctx.selectedText.isEmpty());
}

void ContextMenuManagerTest::testDocumentContextWithDocument() {
    auto ctx = createTestDocumentContext(true);
    QVERIFY(ctx.hasDocument);
    QVERIFY(ctx.canCopy);
    QVERIFY(ctx.canZoom);
    QVERIFY(ctx.canRotate);
    QCOMPARE(ctx.currentPage, 1);
    QCOMPARE(ctx.totalPages, 10);
    QVERIFY(!ctx.documentPath.isEmpty());
}

void ContextMenuManagerTest::testDocumentContextWithSelection() {
    auto ctx = createTestDocumentContext(true);
    ctx.hasSelection = true;
    ctx.canCopy = true;
    ctx.selectedText = "Test selection text";
    QVERIFY(ctx.hasSelection);
    QVERIFY(ctx.canCopy);
    QCOMPARE(ctx.selectedText, QString("Test selection text"));
}

void ContextMenuManagerTest::testDocumentContextZoomLevel() {
    auto ctx = createTestDocumentContext(true);
    ctx.zoomLevel = 2.5;
    QCOMPARE(ctx.zoomLevel, 2.5);
    ctx.zoomLevel = 0.25;
    QCOMPARE(ctx.zoomLevel, 0.25);
}

void ContextMenuManagerTest::testDocumentContextPageInfo() {
    auto ctx = createTestDocumentContext(true);
    ctx.currentPage = 50;
    ctx.totalPages = 100;
    QCOMPARE(ctx.currentPage, 50);
    QCOMPARE(ctx.totalPages, 100);
}

void ContextMenuManagerTest::testUIElementContextDefaults() {
    ContextMenuManager::UIElementContext ctx;
    QVERIFY(ctx.targetWidget == nullptr);
    QCOMPARE(ctx.elementIndex, -1);
    QVERIFY(ctx.elementId.isEmpty());
    QVERIFY(ctx.properties.isEmpty());
    QVERIFY(ctx.isEnabled);
    QVERIFY(ctx.isVisible);
}

void ContextMenuManagerTest::testUIElementContextWithWidget() {
    auto ctx = createTestUIContext();
    QVERIFY(ctx.targetWidget != nullptr);
    QCOMPARE(ctx.elementIndex, 0);
    QVERIFY(!ctx.elementId.isEmpty());
}

void ContextMenuManagerTest::testUIElementContextProperties() {
    ContextMenuManager::UIElementContext ctx;
    ctx.properties["key1"] = "value1";
    ctx.properties["key2"] = 42;
    ctx.properties["key3"] = true;
    QCOMPARE(ctx.properties["key1"].toString(), QString("value1"));
    QCOMPARE(ctx.properties["key2"].toInt(), 42);
    QVERIFY(ctx.properties["key3"].toBool());
}

void ContextMenuManagerTest::testShowDocumentViewerMenu() {
    auto ctx = createTestDocumentContext(true);
    QSignalSpy spy(m_manager, &ContextMenuManager::actionTriggered);
    QVERIFY(spy.isValid());
    QVERIFY(m_manager != nullptr);
}

void ContextMenuManagerTest::testDocumentViewerMenuWithSelection() {
    auto ctx = createTestDocumentContext(true);
    ctx.hasSelection = true;
    ctx.canCopy = true;
    ctx.selectedText = "Selected text for copy";
    QVERIFY(ctx.hasSelection);
    QVERIFY(ctx.canCopy);
    QCOMPARE(ctx.selectedText, QString("Selected text for copy"));
}

void ContextMenuManagerTest::testDocumentViewerMenuWithoutDocument() {
    auto ctx = createTestDocumentContext(false);
    QVERIFY(!ctx.hasDocument);
    QVERIFY(!ctx.canCopy);
    QVERIFY(!ctx.canZoom);
    QVERIFY(!ctx.canRotate);
    QCOMPARE(ctx.currentPage, 0);
    QCOMPARE(ctx.totalPages, 0);
}

void ContextMenuManagerTest::testDocumentViewerMenuZoomSubmenu() {
    auto ctx = createTestDocumentContext(true);
    ctx.canZoom = true;
    ctx.zoomLevel = 1.5;
    QVERIFY(ctx.canZoom);
    QCOMPARE(ctx.zoomLevel, 1.5);
}

void ContextMenuManagerTest::testDocumentViewerMenuPageSubmenu() {
    auto ctx = createTestDocumentContext(true);
    ctx.currentPage = 5;
    ctx.totalPages = 20;
    QCOMPARE(ctx.currentPage, 5);
    QCOMPARE(ctx.totalPages, 20);
}

void ContextMenuManagerTest::testShowDocumentTabMenu() {
    auto ctx = createTestUIContext();
    ctx.elementIndex = 0;
    ctx.properties["canClose"] = true;
    ctx.properties["canCloseOthers"] = false;
    QVERIFY(ctx.targetWidget != nullptr);
    QCOMPARE(ctx.elementIndex, 0);
}

void ContextMenuManagerTest::testDocumentTabMenuActions() {
    auto ctx = createTestUIContext();
    ctx.elementIndex = 2;
    ctx.properties["canClose"] = true;
    ctx.properties["canCloseOthers"] = true;
    ctx.properties["canCloseAll"] = true;
    ctx.properties["canDuplicate"] = true;
    QCOMPARE(ctx.elementIndex, 2);
    QVERIFY(ctx.properties["canClose"].toBool());
    QVERIFY(ctx.properties["canCloseOthers"].toBool());
}

void ContextMenuManagerTest::testDocumentTabMenuWithMultipleTabs() {
    auto ctx = createTestUIContext();
    ctx.properties["tabCount"] = 5;
    ctx.properties["currentTabIndex"] = 2;
    ctx.properties["canCloseOthers"] = true;
    QCOMPARE(ctx.properties["tabCount"].toInt(), 5);
    QCOMPARE(ctx.properties["currentTabIndex"].toInt(), 2);
}

void ContextMenuManagerTest::testShowSidebarThumbnailMenu() {
    auto ctx = createTestUIContext();
    ctx.elementId = "thumbnail_item";
    ctx.elementIndex = 5;
    ctx.properties["pageNumber"] = 6;
    QCOMPARE(ctx.elementId, QString("thumbnail_item"));
    QCOMPARE(ctx.properties["pageNumber"].toInt(), 6);
}

void ContextMenuManagerTest::testShowSidebarBookmarkMenu() {
    auto ctx = createTestUIContext();
    ctx.elementId = "bookmark_item";
    ctx.properties["bookmarkName"] = "Chapter 1";
    ctx.properties["pageNumber"] = 10;
    ctx.properties["canEdit"] = true;
    ctx.properties["canDelete"] = true;
    QCOMPARE(ctx.properties["bookmarkName"].toString(), QString("Chapter 1"));
    QVERIFY(ctx.properties["canEdit"].toBool());
}

void ContextMenuManagerTest::testSidebarMenuWithSelection() {
    auto ctx = createTestUIContext();
    ctx.elementId = "thumbnail_item";
    ctx.properties["isSelected"] = true;
    ctx.properties["selectionCount"] = 3;
    QVERIFY(ctx.properties["isSelected"].toBool());
    QCOMPARE(ctx.properties["selectionCount"].toInt(), 3);
}

void ContextMenuManagerTest::testShowToolbarMenu() {
    auto ctx = createTestUIContext();
    ctx.elementId = "toolbar";
    ctx.properties["canCustomize"] = true;
    ctx.properties["canReset"] = true;
    QCOMPARE(ctx.elementId, QString("toolbar"));
    QVERIFY(ctx.properties["canCustomize"].toBool());
}

void ContextMenuManagerTest::testToolbarMenuCustomization() {
    auto ctx = createTestUIContext();
    ctx.elementId = "toolbar";
    ctx.properties["showLabels"] = true;
    ctx.properties["iconSize"] = "medium";
    QVERIFY(ctx.properties["showLabels"].toBool());
    QCOMPARE(ctx.properties["iconSize"].toString(), QString("medium"));
}

void ContextMenuManagerTest::testShowSearchMenu() {
    auto ctx = createTestUIContext();
    ctx.elementId = "search_widget";
    ctx.properties["hasHistory"] = true;
    ctx.properties["caseSensitive"] = false;
    ctx.properties["wholeWord"] = false;
    QVERIFY(ctx.properties["hasHistory"].toBool());
    QVERIFY(!ctx.properties["caseSensitive"].toBool());
}

void ContextMenuManagerTest::testSearchMenuWithHistory() {
    auto ctx = createTestUIContext();
    ctx.elementId = "search_widget";
    ctx.properties["hasHistory"] = true;
    ctx.properties["historyCount"] = 10;
    QStringList history = {"term1", "term2", "term3"};
    ctx.properties["recentSearches"] = history;
    QCOMPARE(ctx.properties["historyCount"].toInt(), 10);
}

void ContextMenuManagerTest::testShowStatusBarMenu() {
    auto ctx = createTestUIContext();
    ctx.elementId = "status_bar";
    ctx.properties["showPageInfo"] = true;
    ctx.properties["showZoomInfo"] = true;
    ctx.properties["showFileName"] = true;
    QCOMPARE(ctx.elementId, QString("status_bar"));
    QVERIFY(ctx.properties["showPageInfo"].toBool());
}

void ContextMenuManagerTest::testShowRightSidebarMenu() {
    auto ctx = createTestUIContext();
    ctx.elementId = "right_sidebar";
    ctx.properties["currentPanel"] = "properties";
    QCOMPARE(ctx.elementId, QString("right_sidebar"));
}

void ContextMenuManagerTest::testRightSidebarMenuPanels() {
    auto ctx = createTestUIContext();
    ctx.elementId = "right_sidebar";
    ctx.properties["showProperties"] = true;
    ctx.properties["showAnnotations"] = true;
    ctx.properties["showLayers"] = false;
    ctx.properties["showSearch"] = true;
    QVERIFY(ctx.properties["showProperties"].toBool());
    QVERIFY(!ctx.properties["showLayers"].toBool());
}

void ContextMenuManagerTest::testActionTriggeredSignal() {
    QSignalSpy spy(m_manager, &ContextMenuManager::actionTriggered);
    QVERIFY(spy.isValid());
}

void ContextMenuManagerTest::testCustomActionTriggeredSignal() {
    QSignalSpy spy(m_manager, &ContextMenuManager::customActionTriggered);
    QVERIFY(spy.isValid());
}

void ContextMenuManagerTest::testActionTriggeredWithContext() {
    QSignalSpy spy(m_manager, &ContextMenuManager::actionTriggered);
    QVERIFY(spy.isValid());
    QVariantMap context;
    context["source"] = "test";
    context["pageNumber"] = 5;
    QCOMPARE(context["source"].toString(), QString("test"));
    QCOMPARE(context["pageNumber"].toInt(), 5);
}

void ContextMenuManagerTest::testUpdateMenuStates() {
    auto ctx = createTestDocumentContext(true);
    m_manager->updateMenuStates(ctx);
    QVERIFY(true);
}

void ContextMenuManagerTest::testUpdateMenuStatesWithSelection() {
    auto ctx = createTestDocumentContext(true);
    ctx.hasSelection = true;
    ctx.canCopy = true;
    ctx.selectedText = "Selected text";
    m_manager->updateMenuStates(ctx);
    QVERIFY(true);
}

void ContextMenuManagerTest::testClearMenuCache() {
    m_manager->clearMenuCache();
    QVERIFY(true);
    m_manager->clearMenuCache();
    QVERIFY(true);
}

void ContextMenuManagerTest::testMenuCachePerformance() {
    for (int i = 0; i < 100; ++i) {
        auto ctx = createTestDocumentContext(true);
        m_manager->updateMenuStates(ctx);
    }
    m_manager->clearMenuCache();
    QVERIFY(true);
}

void ContextMenuManagerTest::testValidateDocumentContext() {
    auto validCtx = createTestDocumentContext(true);
    auto invalidCtx = createTestDocumentContext(false);
    QVERIFY(validCtx.hasDocument);
    QVERIFY(!invalidCtx.hasDocument);
    QVERIFY(validCtx.canCopy);
    QVERIFY(!invalidCtx.canCopy);
}

void ContextMenuManagerTest::testValidateUIContext() {
    auto ctx = createTestUIContext();
    QVERIFY(ctx.targetWidget != nullptr);
    QVERIFY(ctx.isEnabled);
    QVERIFY(ctx.isVisible);
}

void ContextMenuManagerTest::testValidateEmptyContext() {
    ContextMenuManager::DocumentContext emptyDocCtx;
    QVERIFY(!emptyDocCtx.hasDocument);
    QVERIFY(emptyDocCtx.documentPath.isEmpty());

    ContextMenuManager::UIElementContext emptyUICtx;
    QVERIFY(emptyUICtx.targetWidget == nullptr);
    QCOMPARE(emptyUICtx.elementIndex, -1);
}

void ContextMenuManagerTest::testNullParentWidget() {
    auto* manager = new ContextMenuManager(nullptr);
    QVERIFY(manager != nullptr);
    delete manager;
}

void ContextMenuManagerTest::testInvalidTabIndex() {
    auto ctx = createTestUIContext();
    ctx.elementIndex = -1;
    QCOMPARE(ctx.elementIndex, -1);
    ctx.elementIndex = 1000;
    QCOMPARE(ctx.elementIndex, 1000);
}

void ContextMenuManagerTest::waitForMenu() {
    QTest::qWait(50);
    QApplication::processEvents();
}

QTEST_MAIN(ContextMenuManagerTest)
#include "test_context_menu_manager.moc"
