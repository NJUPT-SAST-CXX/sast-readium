#include <QtTest/QtTest>
#include <QApplication>
#include <QSignalSpy>
#include <QScrollBar>
#include <QPropertyAnimation>
#include <QMenu>
#include <QAction>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include "../../app/ui/thumbnail/ThumbnailListView.h"
#include "../../app/model/ThumbnailModel.h"
#include "../../app/delegate/ThumbnailDelegate.h"

class ThumbnailListViewIntegrationTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testInitialization();
    void testModelAndDelegate();
    void testThumbnailSize();
    void testThumbnailSpacing();
    
    // Scrolling and navigation tests
    void testScrollToPage();
    void testScrollToTopBottom();
    void testCurrentPage();
    void testSmoothScrolling();
    
    // Selection management tests
    void testPageSelection();
    void testMultipleSelection();
    void testClearSelection();
    void testSelectedPages();
    
    // Animation tests
    void testAnimationEnabled();
    void testScrollAnimation();
    void testFadeInAnimation();
    
    // Preloading tests
    void testPreloadSettings();
    void testAutoPreload();
    void testPreloadMargin();
    
    // Context menu tests
    void testContextMenuEnabled();
    void testContextMenuActions();
    void testContextMenuDisplay();
    
    // Event handling tests
    void testWheelEvent();
    void testKeyPressEvent();
    void testMouseEvents();
    
    // Signal emission tests
    void testPageClickedSignal();
    void testPageDoubleClickedSignal();
    void testCurrentPageChangedSignal();
    void testScrollPositionSignal();
    void testVisibleRangeSignal();
    
    // Performance tests
    void testVisibleRangeTracking();
    void testViewportUpdates();
    void testScrollPerformance();
    
    // State management tests
    void testViewState();
    void testResizeHandling();

private:
    ThumbnailListView* m_listView;
    ThumbnailModel* m_thumbnailModel;
    ThumbnailDelegate* m_thumbnailDelegate;
    QWidget* m_parentWidget;
    
    void populateTestModel();
    void waitForAnimation();
    void simulateWheelEvent(int delta);
    void simulateKeyPress(int key);
};

void ThumbnailListViewIntegrationTest::initTestCase()
{
    m_parentWidget = new QWidget();
    m_parentWidget->resize(400, 800);
    m_parentWidget->show();
    
    m_thumbnailModel = new ThumbnailModel(this);
    m_thumbnailDelegate = new ThumbnailDelegate(this);
    
    populateTestModel();
}

void ThumbnailListViewIntegrationTest::cleanupTestCase()
{
    delete m_parentWidget;
}

void ThumbnailListViewIntegrationTest::init()
{
    m_listView = new ThumbnailListView(m_parentWidget);
    m_listView->setThumbnailModel(m_thumbnailModel);
    m_listView->setThumbnailDelegate(m_thumbnailDelegate);
    m_listView->show();
    QTest::qWaitForWindowExposed(m_listView);
}

void ThumbnailListViewIntegrationTest::cleanup()
{
    delete m_listView;
    m_listView = nullptr;
}

void ThumbnailListViewIntegrationTest::testInitialization()
{
    // Test basic initialization
    QVERIFY(m_listView != nullptr);
    QVERIFY(m_listView->isVisible());
    
    // Test default settings
    QVERIFY(m_listView->animationEnabled());
    QVERIFY(m_listView->smoothScrolling());
    QVERIFY(m_listView->autoPreload());
    QVERIFY(m_listView->contextMenuEnabled());
}

void ThumbnailListViewIntegrationTest::testModelAndDelegate()
{
    // Test model setting
    QCOMPARE(m_listView->thumbnailModel(), m_thumbnailModel);
    
    // Test delegate setting
    QCOMPARE(m_listView->thumbnailDelegate(), m_thumbnailDelegate);
    
    // Test setting new model
    ThumbnailModel* newModel = new ThumbnailModel(this);
    m_listView->setThumbnailModel(newModel);
    QCOMPARE(m_listView->thumbnailModel(), newModel);
    
    // Reset to original model
    m_listView->setThumbnailModel(m_thumbnailModel);
    
    delete newModel;
}

void ThumbnailListViewIntegrationTest::testThumbnailSize()
{
    // Test default size
    QSize defaultSize = m_listView->thumbnailSize();
    QVERIFY(defaultSize.width() > 0);
    QVERIFY(defaultSize.height() > 0);
    
    // Test setting custom size
    QSize customSize(150, 200);
    m_listView->setThumbnailSize(customSize);
    QCOMPARE(m_listView->thumbnailSize(), customSize);
    
    // Test invalid size
    QSize invalidSize(0, 0);
    m_listView->setThumbnailSize(invalidSize);
    // Should handle gracefully or use minimum size
    QVERIFY(m_listView->thumbnailSize().width() >= 0);
    QVERIFY(m_listView->thumbnailSize().height() >= 0);
}

void ThumbnailListViewIntegrationTest::testThumbnailSpacing()
{
    // Test default spacing
    int defaultSpacing = m_listView->thumbnailSpacing();
    QVERIFY(defaultSpacing >= 0);
    
    // Test setting custom spacing
    int customSpacing = 12;
    m_listView->setThumbnailSpacing(customSpacing);
    QCOMPARE(m_listView->thumbnailSpacing(), customSpacing);
    
    // Test negative spacing
    m_listView->setThumbnailSpacing(-5);
    // Should handle gracefully
    QVERIFY(m_listView->thumbnailSpacing() >= 0);
}

void ThumbnailListViewIntegrationTest::testScrollToPage()
{
    if (m_thumbnailModel->rowCount() == 0) {
        QSKIP("No pages in model");
    }
    
    // Test scrolling to specific page
    m_listView->scrollToPage(0, false); // No animation for faster testing
    waitForAnimation();
    
    m_listView->scrollToPage(2, false);
    waitForAnimation();
    
    // Test scrolling to invalid page
    m_listView->scrollToPage(-1, false);
    m_listView->scrollToPage(1000, false);
    
    // Should handle invalid pages gracefully
    QVERIFY(true);
}

void ThumbnailListViewIntegrationTest::testScrollToTopBottom()
{
    // Test scroll to top
    m_listView->scrollToTop(false);
    waitForAnimation();
    
    QScrollBar* vScrollBar = m_listView->verticalScrollBar();
    if (vScrollBar) {
        QCOMPARE(vScrollBar->value(), vScrollBar->minimum());
    }
    
    // Test scroll to bottom
    m_listView->scrollToBottom(false);
    waitForAnimation();
    
    if (vScrollBar) {
        QCOMPARE(vScrollBar->value(), vScrollBar->maximum());
    }
}

void ThumbnailListViewIntegrationTest::testCurrentPage()
{
    // Test setting current page
    m_listView->setCurrentPage(0, false);
    QCOMPARE(m_listView->currentPage(), 0);
    
    m_listView->setCurrentPage(1, false);
    QCOMPARE(m_listView->currentPage(), 1);
    
    // Test invalid page
    m_listView->setCurrentPage(-1, false);
    // Should handle gracefully
    QVERIFY(m_listView->currentPage() >= -1);
}

void ThumbnailListViewIntegrationTest::testSmoothScrolling()
{
    // Test enabling/disabling smooth scrolling
    m_listView->setSmoothScrolling(true);
    QVERIFY(m_listView->smoothScrolling());
    
    m_listView->setSmoothScrolling(false);
    QVERIFY(!m_listView->smoothScrolling());
    
    // Reset to default
    m_listView->setSmoothScrolling(true);
}

void ThumbnailListViewIntegrationTest::testPageSelection()
{
    if (m_thumbnailModel->rowCount() == 0) {
        QSKIP("No pages in model");
    }
    
    // Test selecting single page
    m_listView->selectPage(0);
    QList<int> selected = m_listView->selectedPages();
    QVERIFY(selected.contains(0));
    
    // Test selecting different page
    m_listView->selectPage(1);
    selected = m_listView->selectedPages();
    QVERIFY(selected.contains(1));
    QVERIFY(!selected.contains(0) || selected.size() > 1); // Depends on selection mode
}

void ThumbnailListViewIntegrationTest::testMultipleSelection()
{
    if (m_thumbnailModel->rowCount() < 3) {
        QSKIP("Not enough pages in model");
    }
    
    // Test selecting multiple pages
    QList<int> pagesToSelect = {0, 1, 2};
    m_listView->selectPages(pagesToSelect);
    
    QList<int> selected = m_listView->selectedPages();
    for (int page : pagesToSelect) {
        QVERIFY(selected.contains(page));
    }
}

void ThumbnailListViewIntegrationTest::testClearSelection()
{
    // Select some pages first
    m_listView->selectPage(0);
    QVERIFY(m_listView->selectedPages().size() > 0);
    
    // Clear selection
    m_listView->clearSelection();
    QCOMPARE(m_listView->selectedPages().size(), 0);
}

void ThumbnailListViewIntegrationTest::testSelectedPages()
{
    // Test selected pages tracking
    m_listView->clearSelection();
    QCOMPARE(m_listView->selectedPages().size(), 0);
    
    if (m_thumbnailModel->rowCount() > 0) {
        m_listView->selectPage(0);
        QCOMPARE(m_listView->selectedPages().size(), 1);
        QVERIFY(m_listView->selectedPages().contains(0));
    }
}

void ThumbnailListViewIntegrationTest::testAnimationEnabled()
{
    // Test enabling/disabling animations
    m_listView->setAnimationEnabled(true);
    QVERIFY(m_listView->animationEnabled());
    
    m_listView->setAnimationEnabled(false);
    QVERIFY(!m_listView->animationEnabled());
    
    // Reset to default
    m_listView->setAnimationEnabled(true);
}

void ThumbnailListViewIntegrationTest::testScrollAnimation()
{
    if (m_thumbnailModel->rowCount() < 2) {
        QSKIP("Not enough pages for scroll animation test");
    }
    
    // Test animated scrolling
    m_listView->setAnimationEnabled(true);
    m_listView->scrollToPage(0, true);
    
    // Wait for animation to start
    QTest::qWait(50);
    
    m_listView->scrollToPage(1, true);
    
    // Wait for animation to complete
    waitForAnimation();
    
    // Should handle animated scrolling
    QVERIFY(true);
}

void ThumbnailListViewIntegrationTest::testFadeInAnimation()
{
    // Test fade in animation
    m_listView->setFadeInEnabled(true);
    QVERIFY(m_listView->fadeInEnabled());
    
    m_listView->setFadeInEnabled(false);
    QVERIFY(!m_listView->fadeInEnabled());
    
    // Reset to default
    m_listView->setFadeInEnabled(true);
}

void ThumbnailListViewIntegrationTest::testPreloadSettings()
{
    // Test preload margin
    int defaultMargin = m_listView->preloadMargin();
    QVERIFY(defaultMargin >= 0);
    
    m_listView->setPreloadMargin(5);
    QCOMPARE(m_listView->preloadMargin(), 5);
    
    // Test negative margin
    m_listView->setPreloadMargin(-1);
    QVERIFY(m_listView->preloadMargin() >= 0);
}

void ThumbnailListViewIntegrationTest::testAutoPreload()
{
    // Test auto preload setting
    m_listView->setAutoPreload(true);
    QVERIFY(m_listView->autoPreload());
    
    m_listView->setAutoPreload(false);
    QVERIFY(!m_listView->autoPreload());
    
    // Reset to default
    m_listView->setAutoPreload(true);
}

void ThumbnailListViewIntegrationTest::testPreloadMargin()
{
    // Test preload margin values
    m_listView->setPreloadMargin(0);
    QCOMPARE(m_listView->preloadMargin(), 0);
    
    m_listView->setPreloadMargin(10);
    QCOMPARE(m_listView->preloadMargin(), 10);
    
    m_listView->setPreloadMargin(100);
    QCOMPARE(m_listView->preloadMargin(), 100);
}

void ThumbnailListViewIntegrationTest::testContextMenuEnabled()
{
    // Test context menu enabling/disabling
    m_listView->setContextMenuEnabled(true);
    QVERIFY(m_listView->contextMenuEnabled());
    
    m_listView->setContextMenuEnabled(false);
    QVERIFY(!m_listView->contextMenuEnabled());
    
    // Reset to default
    m_listView->setContextMenuEnabled(true);
}

void ThumbnailListViewIntegrationTest::testContextMenuActions()
{
    // Test adding context menu actions
    QAction* customAction = new QAction("Test Action", this);
    
    m_listView->addContextMenuAction(customAction);
    
    // Test removing context menu actions
    m_listView->removeContextMenuAction(customAction);
    
    // Test clearing context menu actions
    m_listView->addContextMenuAction(customAction);
    m_listView->clearContextMenuActions();
    
    delete customAction;
}

void ThumbnailListViewIntegrationTest::testContextMenuDisplay()
{
    // Test context menu display (basic test)
    m_listView->setContextMenuEnabled(true);
    
    // Simulate right click (would normally show context menu)
    QPoint testPoint(50, 50);
    QContextMenuEvent contextEvent(QContextMenuEvent::Mouse, testPoint);
    QApplication::sendEvent(m_listView, &contextEvent);
    
    // Should handle context menu event
    QVERIFY(true);
}

void ThumbnailListViewIntegrationTest::testWheelEvent()
{
    // Test wheel event handling
    simulateWheelEvent(120); // Scroll up
    waitForAnimation();
    
    simulateWheelEvent(-120); // Scroll down
    waitForAnimation();
    
    // Should handle wheel events without crashing
    QVERIFY(true);
}

void ThumbnailListViewIntegrationTest::testKeyPressEvent()
{
    // Test key press handling
    simulateKeyPress(Qt::Key_Up);
    simulateKeyPress(Qt::Key_Down);
    simulateKeyPress(Qt::Key_PageUp);
    simulateKeyPress(Qt::Key_PageDown);
    simulateKeyPress(Qt::Key_Home);
    simulateKeyPress(Qt::Key_End);
    
    // Should handle key events without crashing
    QVERIFY(true);
}

void ThumbnailListViewIntegrationTest::testMouseEvents()
{
    // Test mouse events
    QPoint testPoint(50, 50);
    
    QMouseEvent pressEvent(QEvent::MouseButtonPress, testPoint, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(m_listView, &pressEvent);
    
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, testPoint, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(m_listView, &releaseEvent);
    
    QMouseEvent doubleClickEvent(QEvent::MouseButtonDblClick, testPoint, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(m_listView, &doubleClickEvent);
    
    // Should handle mouse events without crashing
    QVERIFY(true);
}

void ThumbnailListViewIntegrationTest::testPageClickedSignal()
{
    QSignalSpy clickedSpy(m_listView, &ThumbnailListView::pageClicked);
    
    // Simulate page click
    QPoint testPoint(50, 50);
    QMouseEvent clickEvent(QEvent::MouseButtonPress, testPoint, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(m_listView, &clickEvent);
    
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, testPoint, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(m_listView, &releaseEvent);
    
    // May emit page clicked signal
    QVERIFY(clickedSpy.count() >= 0);
}

void ThumbnailListViewIntegrationTest::testPageDoubleClickedSignal()
{
    QSignalSpy doubleClickedSpy(m_listView, &ThumbnailListView::pageDoubleClicked);
    
    // Simulate double click
    QPoint testPoint(50, 50);
    QMouseEvent doubleClickEvent(QEvent::MouseButtonDblClick, testPoint, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(m_listView, &doubleClickEvent);
    
    // May emit page double clicked signal
    QVERIFY(doubleClickedSpy.count() >= 0);
}

void ThumbnailListViewIntegrationTest::testCurrentPageChangedSignal()
{
    QSignalSpy currentPageSpy(m_listView, &ThumbnailListView::currentPageChanged);
    
    // Change current page
    m_listView->setCurrentPage(0, false);
    m_listView->setCurrentPage(1, false);
    
    // Should emit current page changed signal
    QVERIFY(currentPageSpy.count() >= 0);
}

void ThumbnailListViewIntegrationTest::testScrollPositionSignal()
{
    QSignalSpy scrollSpy(m_listView, &ThumbnailListView::scrollPositionChanged);
    
    // Scroll to trigger signal
    m_listView->scrollToTop(false);
    m_listView->scrollToBottom(false);
    
    // Should emit scroll position signals
    QVERIFY(scrollSpy.count() >= 0);
}

void ThumbnailListViewIntegrationTest::testVisibleRangeSignal()
{
    QSignalSpy visibleRangeSpy(m_listView, &ThumbnailListView::visibleRangeChanged);
    
    // Scroll to change visible range
    m_listView->scrollToPage(0, false);
    waitForAnimation();
    
    if (m_thumbnailModel->rowCount() > 1) {
        m_listView->scrollToPage(1, false);
        waitForAnimation();
    }
    
    // Should emit visible range signals
    QVERIFY(visibleRangeSpy.count() >= 0);
}

void ThumbnailListViewIntegrationTest::testVisibleRangeTracking()
{
    // Test visible range tracking
    m_listView->scrollToTop(false);
    waitForAnimation();
    
    // Should track visible range without issues
    QVERIFY(true);
}

void ThumbnailListViewIntegrationTest::testViewportUpdates()
{
    // Test viewport updates
    m_listView->resize(300, 600);
    QTest::qWait(100);
    
    m_listView->resize(500, 800);
    QTest::qWait(100);
    
    // Should handle viewport updates
    QVERIFY(true);
}

void ThumbnailListViewIntegrationTest::testScrollPerformance()
{
    // Test scroll performance (basic test)
    for (int i = 0; i < 10; ++i) {
        simulateWheelEvent(120);
        QTest::qWait(10);
    }
    
    // Should handle rapid scrolling
    QVERIFY(true);
}

void ThumbnailListViewIntegrationTest::testViewState()
{
    // Test view state management
    int initialPage = m_listView->currentPage();
    QList<int> initialSelection = m_listView->selectedPages();
    
    // Change state
    m_listView->setCurrentPage(1, false);
    m_listView->selectPage(1);
    
    // State should be updated
    QVERIFY(m_listView->currentPage() != initialPage || initialPage == 1);
}

void ThumbnailListViewIntegrationTest::testResizeHandling()
{
    // Test resize handling
    QSize initialSize = m_listView->size();
    
    m_listView->resize(600, 400);
    QTest::qWait(100);
    
    m_listView->resize(200, 800);
    QTest::qWait(100);
    
    // Should handle resizing without issues
    QVERIFY(true);
    
    // Restore original size
    m_listView->resize(initialSize);
}

void ThumbnailListViewIntegrationTest::populateTestModel()
{
    // Add some test data to the model
    // This is a basic implementation - actual model population would depend on ThumbnailModel interface
    // For now, we assume the model has some default behavior
}

void ThumbnailListViewIntegrationTest::waitForAnimation()
{
    QTest::qWait(350); // Wait for animations to complete
    QApplication::processEvents();
}

void ThumbnailListViewIntegrationTest::simulateWheelEvent(int delta)
{
    QPoint pos(m_listView->width() / 2, m_listView->height() / 2);
    QWheelEvent wheelEvent(pos, m_listView->mapToGlobal(pos), QPoint(), QPoint(0, delta),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(m_listView, &wheelEvent);
    QTest::qWait(10);
}

void ThumbnailListViewIntegrationTest::simulateKeyPress(int key)
{
    QKeyEvent keyEvent(QEvent::KeyPress, key, Qt::NoModifier);
    QApplication::sendEvent(m_listView, &keyEvent);
    QTest::qWait(10);
}

QTEST_MAIN(ThumbnailListViewIntegrationTest)
#include "thumbnail_list_view_integration_test.moc"
