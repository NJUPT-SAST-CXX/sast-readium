#include <QSignalSpy>
#include <QTest>
#include "../../app/adapters/ViewAdapter.h"
#include "../TestUtilities.h"

class TestViewAdapter : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() { m_adapter = new ViewAdapter(); }

    void cleanup() {
        delete m_adapter;
        m_adapter = nullptr;
    }

    void testConstruction() { QVERIFY(m_adapter != nullptr); }

    void testSetViewDelegate() { m_adapter->setViewDelegate(nullptr); }

    void testSetPDFViewerPage() { m_adapter->setPDFViewerPage(nullptr); }

    void testSetViewModeWithoutDelegate() {
        m_adapter->setViewMode(0);
        m_adapter->setViewMode(1);
        m_adapter->setViewMode(2);
        m_adapter->setViewMode(3);
    }

    void testToggleFullScreenWithoutDelegate() {
        m_adapter->toggleFullScreen();
        m_adapter->toggleFullScreen();
    }

    void testTogglePresentationWithoutDelegate() {
        m_adapter->togglePresentation();
        m_adapter->togglePresentation();
    }

    void testToggleLeftSideBarWithoutDelegate() {
        m_adapter->toggleLeftSideBar();
        m_adapter->toggleLeftSideBar();
    }

    void testToggleRightSideBarWithoutDelegate() {
        m_adapter->toggleRightSideBar();
        m_adapter->toggleRightSideBar();
    }

    void testToggleToolBarWithoutDelegate() {
        m_adapter->toggleToolBar();
        m_adapter->toggleToolBar();
    }

    void testToggleStatusBarWithoutDelegate() {
        m_adapter->toggleStatusBar();
        m_adapter->toggleStatusBar();
    }

    void testViewModeChangedSignal() {
        QSignalSpy spy(m_adapter, &ViewAdapter::viewModeChanged);
        QVERIFY(spy.isValid());
    }

    void testFullScreenChangedSignal() {
        QSignalSpy spy(m_adapter, &ViewAdapter::fullScreenChanged);
        QVERIFY(spy.isValid());
    }

    void testPresentationChangedSignal() {
        QSignalSpy spy(m_adapter, &ViewAdapter::presentationChanged);
        QVERIFY(spy.isValid());
    }

    void testSetInvalidViewMode() {
        m_adapter->setViewMode(-1);
        m_adapter->setViewMode(100);
        m_adapter->setViewMode(INT_MAX);
        m_adapter->setViewMode(INT_MIN);
    }

    void testViewModeSequence() {
        m_adapter->setViewMode(0);
        m_adapter->setViewMode(1);
        m_adapter->setViewMode(2);
        m_adapter->setViewMode(3);
        m_adapter->setViewMode(0);
    }

    void testToggleSequence() {
        m_adapter->toggleFullScreen();
        m_adapter->togglePresentation();
        m_adapter->toggleLeftSideBar();
        m_adapter->toggleRightSideBar();
        m_adapter->toggleToolBar();
        m_adapter->toggleStatusBar();
    }

    void testRapidToggling() {
        for (int i = 0; i < 20; ++i) {
            m_adapter->toggleFullScreen();
        }

        for (int i = 0; i < 20; ++i) {
            m_adapter->togglePresentation();
        }

        for (int i = 0; i < 20; ++i) {
            m_adapter->toggleLeftSideBar();
        }
    }

    void testMixedOperations() {
        m_adapter->setViewMode(1);
        m_adapter->toggleFullScreen();
        m_adapter->setViewMode(2);
        m_adapter->togglePresentation();
        m_adapter->toggleLeftSideBar();
        m_adapter->setViewMode(0);
        m_adapter->toggleRightSideBar();
        m_adapter->toggleFullScreen();
    }

private:
    ViewAdapter* m_adapter = nullptr;
};

QTEST_MAIN(TestViewAdapter)
#include "test_view_adapter.moc"
