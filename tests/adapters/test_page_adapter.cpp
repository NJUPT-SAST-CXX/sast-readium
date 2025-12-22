#include <QSignalSpy>
#include <QTest>
#include "../../app/adapters/PageAdapter.h"
#include "../TestUtilities.h"

class TestPageAdapter : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() { m_adapter = new PageAdapter(); }

    void cleanup() {
        delete m_adapter;
        m_adapter = nullptr;
    }

    void testConstruction() { QVERIFY(m_adapter != nullptr); }

    void testSetPageController() { m_adapter->setPageController(nullptr); }

    void testSetPDFViewerPage() { m_adapter->setPDFViewerPage(nullptr); }

    void testGoToPageWithoutController() {
        m_adapter->goToPage(1);
        m_adapter->goToPage(5);
        m_adapter->goToPage(100);
    }

    void testGoToNextPageWithoutController() { m_adapter->goToNextPage(); }

    void testGoToPreviousPageWithoutController() {
        m_adapter->goToPreviousPage();
    }

    void testGoToFirstPageWithoutController() { m_adapter->goToFirstPage(); }

    void testGoToLastPageWithoutController() { m_adapter->goToLastPage(); }

    void testGoBackWithoutController() { m_adapter->goBack(); }

    void testGoForwardWithoutController() { m_adapter->goForward(); }

    void testSetZoomWithoutController() {
        m_adapter->setZoom(1.0);
        m_adapter->setZoom(1.5);
        m_adapter->setZoom(2.0);
        m_adapter->setZoom(0.5);
    }

    void testZoomInWithoutController() { m_adapter->zoomIn(); }

    void testZoomOutWithoutController() { m_adapter->zoomOut(); }

    void testFitToWidthWithoutController() { m_adapter->fitToWidth(); }

    void testFitToPageWithoutController() { m_adapter->fitToPage(); }

    void testFitToHeightWithoutController() { m_adapter->fitToHeight(); }

    void testRotateLeftWithoutController() { m_adapter->rotateLeft(); }

    void testRotateRightWithoutController() { m_adapter->rotateRight(); }

    void testResetRotationWithoutController() { m_adapter->resetRotation(); }

    void testAddBookmarkWithoutController() { m_adapter->addBookmark(); }

    void testRemoveBookmarkWithoutController() { m_adapter->removeBookmark(); }

    void testToggleBookmarkWithoutController() { m_adapter->toggleBookmark(); }

    void testPageChangedSignal() {
        QSignalSpy spy(m_adapter, &PageAdapter::pageChanged);
        QVERIFY(spy.isValid());
    }

    void testZoomChangedSignal() {
        QSignalSpy spy(m_adapter, &PageAdapter::zoomChanged);
        QVERIFY(spy.isValid());
    }

    void testRotationChangedSignal() {
        QSignalSpy spy(m_adapter, &PageAdapter::rotationChanged);
        QVERIFY(spy.isValid());
    }

    void testBookmarkAddedSignal() {
        QSignalSpy spy(m_adapter, &PageAdapter::bookmarkAdded);
        QVERIFY(spy.isValid());
    }

    void testBookmarkRemovedSignal() {
        QSignalSpy spy(m_adapter, &PageAdapter::bookmarkRemoved);
        QVERIFY(spy.isValid());
    }

    void testGoToInvalidPage() {
        m_adapter->goToPage(-1);
        m_adapter->goToPage(0);
        m_adapter->goToPage(INT_MAX);
    }

    void testSetInvalidZoom() {
        m_adapter->setZoom(-1.0);
        m_adapter->setZoom(0.0);
        m_adapter->setZoom(100.0);
    }

    void testNavigationSequence() {
        m_adapter->goToPage(1);
        m_adapter->goToNextPage();
        m_adapter->goToNextPage();
        m_adapter->goToPreviousPage();
        m_adapter->goToLastPage();
        m_adapter->goToFirstPage();
        m_adapter->goBack();
        m_adapter->goForward();
    }

    void testZoomSequence() {
        m_adapter->setZoom(1.0);
        m_adapter->zoomIn();
        m_adapter->zoomIn();
        m_adapter->zoomOut();
        m_adapter->fitToWidth();
        m_adapter->fitToPage();
        m_adapter->fitToHeight();
    }

    void testRotationSequence() {
        m_adapter->rotateRight();
        m_adapter->rotateRight();
        m_adapter->rotateRight();
        m_adapter->rotateRight();
        m_adapter->rotateLeft();
        m_adapter->resetRotation();
    }

    void testBookmarkSequence() {
        m_adapter->addBookmark();
        m_adapter->toggleBookmark();
        m_adapter->removeBookmark();
        m_adapter->toggleBookmark();
    }

private:
    PageAdapter* m_adapter = nullptr;
};

QTEST_MAIN(TestPageAdapter)
#include "test_page_adapter.moc"
