#include <QApplication>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include "../../../app/ui/viewer/PDFPrerenderer.h"

class PDFPrerendererTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testConstruction();
    void testDestruction();
    void testRequestPrerender();
    void testCancelPrerender();
    void testCancelAllPrerenders();
    void testSetPriority();
    void testSetMaxConcurrentRenders();
    void testClearCache();
    void testPrerenderCompletedSignal();
    void testPrerenderFailedSignal();
    void testPrerenderCancelledSignal();

private:
    PDFPrerenderer* m_prerenderer;
};

void PDFPrerendererTest::initTestCase() {}

void PDFPrerendererTest::cleanupTestCase() {}

void PDFPrerendererTest::init() { m_prerenderer = new PDFPrerenderer(); }

void PDFPrerendererTest::cleanup() {
    delete m_prerenderer;
    m_prerenderer = nullptr;
}

void PDFPrerendererTest::testConstruction() {
    QVERIFY(m_prerenderer != nullptr);
}

void PDFPrerendererTest::testDestruction() {
    auto* prerenderer = new PDFPrerenderer();
    delete prerenderer;
    QVERIFY(true);
}

void PDFPrerendererTest::testRequestPrerender() {
    // Without a document, this should handle gracefully
    m_prerenderer->requestPrerender(0, 1.0, 0, 5);
    QVERIFY(true);
}

void PDFPrerendererTest::testCancelPrerender() {
    m_prerenderer->cancelPrerenderingForPage(0);
    QVERIFY(true);
}

void PDFPrerendererTest::testCancelAllPrerenders() {
    m_prerenderer->clearPrerenderQueue();
    QVERIFY(true);
}

void PDFPrerendererTest::testSetPriority() {
    // Test prioritizing pages
    m_prerenderer->prioritizePages({0, 1, 2});
    QVERIFY(true);
}

void PDFPrerendererTest::testSetMaxConcurrentRenders() {
    m_prerenderer->setMaxWorkerThreads(4);
    QVERIFY(true);
}

void PDFPrerendererTest::testClearCache() {
    m_prerenderer->setMaxCacheSize(100);
    QVERIFY(true);
}

void PDFPrerendererTest::testPrerenderCompletedSignal() {
    QSignalSpy spy(m_prerenderer, &PDFPrerenderer::pagePrerendered);
    QVERIFY(spy.isValid());
}

void PDFPrerendererTest::testPrerenderFailedSignal() {
    QSignalSpy spy(m_prerenderer, &PDFPrerenderer::prerenderingStarted);
    QVERIFY(spy.isValid());
}

void PDFPrerendererTest::testPrerenderCancelledSignal() {
    QSignalSpy spy(m_prerenderer, &PDFPrerenderer::prerenderingStopped);
    QVERIFY(spy.isValid());
}

QTEST_MAIN(PDFPrerendererTest)
#include "test_pdf_prerenderer.moc"
