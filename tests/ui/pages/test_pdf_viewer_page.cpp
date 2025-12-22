#include <QApplication>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include "../../../app/ui/pages/PDFViewerPage.h"

class PDFViewerPageTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testConstruction();
    void testDestruction();
    void testHasDocumentInitially();
    void testHasDocumentsInitially();
    void testGetDocumentCount();
    void testCurrentPage();
    void testPageCount();
    void testZoomLevel();
    void testIsFullScreen();
    void testIsPresentation();
    void testDocumentOpenedSignal();
    void testDocumentClosedSignal();
    void testPageChangedSignal();
    void testZoomChangedSignal();
    void testViewModeChangedSignal();
    void testFullScreenChangedSignal();
    void testErrorOccurredSignal();

private:
    QWidget* m_parentWidget;
    PDFViewerPage* m_page;
};

void PDFViewerPageTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(1024, 768);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
    }
}

void PDFViewerPageTest::cleanupTestCase() { delete m_parentWidget; }

void PDFViewerPageTest::init() { m_page = new PDFViewerPage(m_parentWidget); }

void PDFViewerPageTest::cleanup() {
    delete m_page;
    m_page = nullptr;
}

void PDFViewerPageTest::testConstruction() { QVERIFY(m_page != nullptr); }

void PDFViewerPageTest::testDestruction() {
    auto* page = new PDFViewerPage(m_parentWidget);
    delete page;
    QVERIFY(true);
}

void PDFViewerPageTest::testHasDocumentInitially() {
    QVERIFY(!m_page->hasDocument());
}

void PDFViewerPageTest::testHasDocumentsInitially() {
    QVERIFY(!m_page->hasDocuments());
}

void PDFViewerPageTest::testGetDocumentCount() {
    QCOMPARE(m_page->getDocumentCount(), 0);
}

void PDFViewerPageTest::testCurrentPage() {
    int page = m_page->currentPage();
    QVERIFY(page >= 0);
}

void PDFViewerPageTest::testPageCount() {
    int count = m_page->pageCount();
    QVERIFY(count >= 0);
}

void PDFViewerPageTest::testZoomLevel() {
    double zoom = m_page->zoomLevel();
    QVERIFY(zoom > 0);
}

void PDFViewerPageTest::testIsFullScreen() { QVERIFY(!m_page->isFullScreen()); }

void PDFViewerPageTest::testIsPresentation() {
    QVERIFY(!m_page->isPresentation());
}

void PDFViewerPageTest::testDocumentOpenedSignal() {
    QSignalSpy spy(m_page, &PDFViewerPage::documentOpened);
    QVERIFY(spy.isValid());
}

void PDFViewerPageTest::testDocumentClosedSignal() {
    QSignalSpy spy(m_page, &PDFViewerPage::documentClosed);
    QVERIFY(spy.isValid());
}

void PDFViewerPageTest::testPageChangedSignal() {
    QSignalSpy spy(m_page, &PDFViewerPage::pageChanged);
    QVERIFY(spy.isValid());
}

void PDFViewerPageTest::testZoomChangedSignal() {
    QSignalSpy spy(m_page, &PDFViewerPage::zoomChanged);
    QVERIFY(spy.isValid());
}

void PDFViewerPageTest::testViewModeChangedSignal() {
    QSignalSpy spy(m_page, &PDFViewerPage::viewModeChanged);
    QVERIFY(spy.isValid());
}

void PDFViewerPageTest::testFullScreenChangedSignal() {
    QSignalSpy spy(m_page, &PDFViewerPage::fullScreenChanged);
    QVERIFY(spy.isValid());
}

void PDFViewerPageTest::testErrorOccurredSignal() {
    QSignalSpy spy(m_page, &PDFViewerPage::errorOccurred);
    QVERIFY(spy.isValid());
}

QTEST_MAIN(PDFViewerPageTest)
#include "test_pdf_viewer_page.moc"
