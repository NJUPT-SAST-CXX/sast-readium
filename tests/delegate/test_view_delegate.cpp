#include <QMainWindow>
#include <QSignalSpy>
#include <QSplitter>
#include <QTest>
#include "../../app/delegate/ViewDelegate.h"
#include "../TestUtilities.h"

class TestViewDelegate : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() {
        m_mainWindow = new QMainWindow();
        m_delegate = new ViewDelegate(m_mainWindow);
    }

    void cleanup() {
        delete m_delegate;
        m_delegate = nullptr;
        delete m_mainWindow;
        m_mainWindow = nullptr;
    }

    void testConstruction() {
        QVERIFY(m_delegate != nullptr);
        QVERIFY(m_mainWindow != nullptr);
    }

    void testConstructionWithNullWindow() {
        ViewDelegate* delegate = new ViewDelegate(nullptr);
        QVERIFY(delegate != nullptr);
        delete delegate;
    }

    void testSetupMainLayout() { m_delegate->setupMainLayout(); }

    void testSaveRestoreLayoutState() {
        m_delegate->saveLayoutState();
        m_delegate->restoreLayoutState();
    }

    void testSideBarVisibility() {
        m_delegate->showSideBar(true);
        m_delegate->showSideBar(false);

        m_delegate->toggleSideBar();
        m_delegate->toggleSideBar();
    }

    void testRightSideBarVisibility() {
        m_delegate->showRightSideBar(true);
        m_delegate->showRightSideBar(false);

        m_delegate->toggleRightSideBar();
        m_delegate->toggleRightSideBar();
    }

    void testViewModes() {
        m_delegate->setFullScreenMode(true);
        m_delegate->setFullScreenMode(false);

        m_delegate->setPresentationMode(true);
        m_delegate->setPresentationMode(false);

        m_delegate->setFocusMode(true);
        m_delegate->setFocusMode(false);
    }

    void testLayoutPresets() {
        m_delegate->applyDefaultLayout();
        m_delegate->applyReadingLayout();
        m_delegate->applyEditingLayout();
        m_delegate->applyCompactLayout();
    }

    void testSetComponents() {
        m_delegate->setSideBar(nullptr);
        m_delegate->setRightSideBar(nullptr);
        m_delegate->setViewWidget(nullptr);
        m_delegate->setStatusBar(nullptr);
        m_delegate->setToolBar(nullptr);
        m_delegate->setMenuBar(nullptr);
        m_delegate->setSplitter(nullptr);
    }

    void testLayoutChangedSignal() {
        QSignalSpy spy(m_delegate, &ViewDelegate::layoutChanged);
        QVERIFY(spy.isValid());

        m_delegate->applyDefaultLayout();
        processEvents();
    }

    void testVisibilityChangedSignal() {
        QSignalSpy spy(m_delegate, &ViewDelegate::visibilityChanged);
        QVERIFY(spy.isValid());

        m_delegate->showSideBar(true);
        m_delegate->showSideBar(false);
        processEvents();
    }

    void testModeChangedSignal() {
        QSignalSpy spy(m_delegate, &ViewDelegate::modeChanged);
        QVERIFY(spy.isValid());

        m_delegate->setFullScreenMode(true);
        m_delegate->setFullScreenMode(false);
        processEvents();
    }

    void testAdjustSplitterSizes() {
        QSplitter* splitter = new QSplitter(m_mainWindow);
        m_delegate->setSplitter(splitter);
        m_delegate->adjustSplitterSizes();
    }

private:
    void processEvents() {
        QCoreApplication::processEvents();
        waitMs(10);
    }

    QMainWindow* m_mainWindow = nullptr;
    ViewDelegate* m_delegate = nullptr;
};

class TestMainViewDelegate : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void testConstruction() {
        MainViewDelegate* delegate = new MainViewDelegate(nullptr);
        QVERIFY(delegate != nullptr);
        delete delegate;
    }

    void testRenderQuality() {
        MainViewDelegate delegate(nullptr);
        delegate.setRenderQuality(100);
        delegate.setRenderQuality(50);
    }

    void testAntiAliasing() {
        MainViewDelegate delegate(nullptr);
        delegate.setAntiAliasing(true);
        delegate.setAntiAliasing(false);
    }

    void testSmoothPixmapTransform() {
        MainViewDelegate delegate(nullptr);
        delegate.setSmoothPixmapTransform(true);
        delegate.setSmoothPixmapTransform(false);
    }

    void testZoomOperations() {
        MainViewDelegate delegate(nullptr);
        delegate.zoomIn();
        delegate.zoomOut();
        delegate.zoomToFit();
        delegate.zoomToWidth();
        delegate.setZoomLevel(1.5);
        QVERIFY(delegate.zoomLevel() > 0);
    }

    void testViewModes() {
        MainViewDelegate delegate(nullptr);
        delegate.setSinglePageMode();
        delegate.setContinuousMode();
        delegate.setFacingPagesMode();
        delegate.setBookViewMode();
    }

    void testScrollManagement() {
        MainViewDelegate delegate(nullptr);
        delegate.scrollToTop();
        delegate.scrollToBottom();
        delegate.scrollToPage(1);
        delegate.centerOnPage(1);
    }

    void testSelectionAndInteraction() {
        MainViewDelegate delegate(nullptr);
        delegate.enableTextSelection(true);
        delegate.enableTextSelection(false);
        delegate.enableAnnotations(true);
        delegate.enableAnnotations(false);
        delegate.setHighlightCurrentPage(true);
        delegate.setHighlightCurrentPage(false);
    }
};

class TestSideBarDelegate : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void testConstruction() {
        SideBarDelegate* delegate = new SideBarDelegate(nullptr);
        QVERIFY(delegate != nullptr);
        delete delegate;
    }

    void testTabManagement() {
        SideBarDelegate delegate(nullptr);
        delegate.showTab(0);
        delegate.showTab("outline");
        delegate.enableTab(0, true);
        delegate.enableTab(0, false);
        delegate.setTabVisible(0, true);
        delegate.setTabVisible(0, false);
    }

    void testContentManagement() {
        SideBarDelegate delegate(nullptr);
        delegate.updateOutline();
        delegate.updateThumbnails();
        delegate.updateBookmarks();
        delegate.updateAnnotations();
    }

    void testWidthControl() {
        SideBarDelegate delegate(nullptr);
        delegate.setPreferredWidth(250);
        QCOMPARE(delegate.preferredWidth(), 250);

        delegate.setMinimumWidth(150);
        delegate.setMaximumWidth(400);
    }

    void testState() {
        SideBarDelegate delegate(nullptr);
        delegate.saveState();
        delegate.restoreState();
    }
};

QTEST_MAIN(TestViewDelegate)
#include "test_view_delegate.moc"
