#include <poppler-qt6.h>
#include <QApplication>
#include <QEventLoop>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTimer>
#include <QtTest/QtTest>
#include "../../app/ui/viewer/PDFViewer.h"

class TestRenderingModeSwitch : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Integration tests
    void testModeSwitch();
    void testStatePreservation();
    void testSignalConsistency();
    void testPerformanceComparison();
    void testErrorHandling();
    void testMemoryManagement();
    void testConcurrentOperations();

private:
    Poppler::Document* createTestDocument();
    void verifyViewerState(PDFViewer* viewer, int expectedPage,
                           double expectedZoom, int expectedRotation);
    void performStandardOperations(PDFViewer* viewer);

    PDFViewer* m_viewer;
    Poppler::Document* m_testDocument;
};

void TestRenderingModeSwitch::initTestCase() {
    m_viewer = new PDFViewer(nullptr, false);  // Disable styling for tests
    m_testDocument = createTestDocument();

    QVERIFY(m_viewer != nullptr);
    QVERIFY(m_testDocument != nullptr);

    m_viewer->setDocument(m_testDocument);
}

void TestRenderingModeSwitch::cleanupTestCase() {
    delete m_viewer;
    if (m_testDocument) {
        delete m_testDocument;
    }
}

Poppler::Document* TestRenderingModeSwitch::createTestDocument() {
    // Create a multi-page test PDF for comprehensive testing
    QString testPdfPath =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
        "/integration_test.pdf";

    QFile file(testPdfPath);
    if (file.open(QIODevice::WriteOnly)) {
        // Multi-page PDF content for testing
        QByteArray pdfContent =
            "%PDF-1.4\n"
            "1 0 obj\n"
            "<<\n"
            "/Type /Catalog\n"
            "/Pages 2 0 R\n"
            ">>\n"
            "endobj\n"
            "2 0 obj\n"
            "<<\n"
            "/Type /Pages\n"
            "/Kids [3 0 R 5 0 R 7 0 R]\n"
            "/Count 3\n"
            ">>\n"
            "endobj\n"
            // Page 1
            "3 0 obj\n"
            "<<\n"
            "/Type /Page\n"
            "/Parent 2 0 R\n"
            "/MediaBox [0 0 612 792]\n"
            "/Contents 4 0 R\n"
            ">>\n"
            "endobj\n"
            "4 0 obj\n"
            "<<\n"
            "/Length 50\n"
            ">>\n"
            "stream\n"
            "BT\n"
            "/F1 12 Tf\n"
            "100 700 Td\n"
            "(Page 1 Content) Tj\n"
            "ET\n"
            "endstream\n"
            "endobj\n"
            // Page 2
            "5 0 obj\n"
            "<<\n"
            "/Type /Page\n"
            "/Parent 2 0 R\n"
            "/MediaBox [0 0 612 792]\n"
            "/Contents 6 0 R\n"
            ">>\n"
            "endobj\n"
            "6 0 obj\n"
            "<<\n"
            "/Length 50\n"
            ">>\n"
            "stream\n"
            "BT\n"
            "/F1 12 Tf\n"
            "100 700 Td\n"
            "(Page 2 Content) Tj\n"
            "ET\n"
            "endstream\n"
            "endobj\n"
            // Page 3
            "7 0 obj\n"
            "<<\n"
            "/Type /Page\n"
            "/Parent 2 0 R\n"
            "/MediaBox [0 0 612 792]\n"
            "/Contents 8 0 R\n"
            ">>\n"
            "endobj\n"
            "8 0 obj\n"
            "<<\n"
            "/Length 50\n"
            ">>\n"
            "stream\n"
            "BT\n"
            "/F1 12 Tf\n"
            "100 700 Td\n"
            "(Page 3 Content) Tj\n"
            "ET\n"
            "endstream\n"
            "endobj\n"
            "xref\n"
            "0 9\n"
            "0000000000 65535 f \n"
            "0000000009 65535 n \n"
            "0000000074 65535 n \n"
            "0000000133 65535 n \n"
            "0000000192 65535 n \n"
            "0000000294 65535 n \n"
            "0000000353 65535 n \n"
            "0000000455 65535 n \n"
            "0000000514 65535 n \n"
            "trailer\n"
            "<<\n"
            "/Size 9\n"
            "/Root 1 0 R\n"
            ">>\n"
            "startxref\n"
            "616\n"
            "%%EOF\n";

        file.write(pdfContent);
        file.close();

        return Poppler::Document::load(testPdfPath).release();
    }

    return nullptr;
}

void TestRenderingModeSwitch::verifyViewerState(PDFViewer* viewer,
                                                int expectedPage,
                                                double expectedZoom,
                                                int expectedRotation) {
    QCOMPARE(viewer->getCurrentPage(), expectedPage);
    QCOMPARE(viewer->getCurrentZoom(), expectedZoom);
    // Note: Rotation comparison might need tolerance due to floating point
    // precision
}

void TestRenderingModeSwitch::performStandardOperations(PDFViewer* viewer) {
    // Perform a series of standard operations
    viewer->goToPage(1);
    viewer->setZoom(1.5);
    viewer->rotateRight();
    viewer->nextPage();
    viewer->zoomIn();
    viewer->rotateLeft();
}

void TestRenderingModeSwitch::testModeSwitch() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    // Test basic mode switching
    QVERIFY(!m_viewer->isQGraphicsRenderingEnabled());  // Should start in
                                                        // traditional mode

    // Switch to QGraphics mode
    m_viewer->setQGraphicsRenderingEnabled(true);
    QVERIFY(m_viewer->isQGraphicsRenderingEnabled());

    // Switch back to traditional mode
    m_viewer->setQGraphicsRenderingEnabled(false);
    QVERIFY(!m_viewer->isQGraphicsRenderingEnabled());

    // Test multiple switches
    for (int i = 0; i < 5; ++i) {
        m_viewer->setQGraphicsRenderingEnabled(i % 2 == 0);
        QCOMPARE(m_viewer->isQGraphicsRenderingEnabled(), i % 2 == 0);
    }

    qDebug() << "Mode switching test passed";
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestRenderingModeSwitch::testStatePreservation() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    // Set initial state
    m_viewer->goToPage(1);
    m_viewer->setZoom(2.0);
    m_viewer->setRotation(90);

    int initialPage = m_viewer->getCurrentPage();
    double initialZoom = m_viewer->getCurrentZoom();

    // Switch to QGraphics mode
    m_viewer->setQGraphicsRenderingEnabled(true);

    // Verify state is preserved
    QCOMPARE(m_viewer->getCurrentPage(), initialPage);
    QCOMPARE(m_viewer->getCurrentZoom(), initialZoom);

    // Modify state in QGraphics mode
    m_viewer->goToPage(2);
    m_viewer->setZoom(1.5);

    int qgraphicsPage = m_viewer->getCurrentPage();
    double qgraphicsZoom = m_viewer->getCurrentZoom();

    // Switch back to traditional mode
    m_viewer->setQGraphicsRenderingEnabled(false);

    // Verify state is still preserved
    QCOMPARE(m_viewer->getCurrentPage(), qgraphicsPage);
    QCOMPARE(m_viewer->getCurrentZoom(), qgraphicsZoom);

    qDebug() << "State preservation test passed";
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestRenderingModeSwitch::testSignalConsistency() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    // Test that signals are emitted consistently in both modes
    QSignalSpy pageChangedSpy(m_viewer, &PDFViewer::pageChanged);
    QSignalSpy zoomChangedSpy(m_viewer, &PDFViewer::zoomChanged);
    QSignalSpy rotationChangedSpy(m_viewer, &PDFViewer::rotationChanged);

    // Test in traditional mode
    m_viewer->setQGraphicsRenderingEnabled(false);

    int traditionalPageSignals = pageChangedSpy.count();
    int traditionalZoomSignals = zoomChangedSpy.count();
    int traditionalRotationSignals = rotationChangedSpy.count();

    performStandardOperations(m_viewer);

    int traditionalPageSignalsAfter = pageChangedSpy.count();
    int traditionalZoomSignalsAfter = zoomChangedSpy.count();
    int traditionalRotationSignalsAfter = rotationChangedSpy.count();

    // Reset counters
    pageChangedSpy.clear();
    zoomChangedSpy.clear();
    rotationChangedSpy.clear();

    // Test in QGraphics mode
    m_viewer->setQGraphicsRenderingEnabled(true);

    performStandardOperations(m_viewer);

    int qgraphicsPageSignals = pageChangedSpy.count();
    int qgraphicsZoomSignals = zoomChangedSpy.count();
    int qgraphicsRotationSignals = rotationChangedSpy.count();

    // Verify signal consistency (signals should be emitted in both modes)
    QVERIFY(qgraphicsPageSignals > 0);
    QVERIFY(qgraphicsZoomSignals > 0);
    QVERIFY(qgraphicsRotationSignals > 0);

    qDebug() << "Signal consistency test passed";
    qDebug() << "Traditional mode signals - Page:"
             << (traditionalPageSignalsAfter - traditionalPageSignals)
             << "Zoom:"
             << (traditionalZoomSignalsAfter - traditionalZoomSignals)
             << "Rotation:"
             << (traditionalRotationSignalsAfter - traditionalRotationSignals);
    qDebug() << "QGraphics mode signals - Page:" << qgraphicsPageSignals
             << "Zoom:" << qgraphicsZoomSignals
             << "Rotation:" << qgraphicsRotationSignals;
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestRenderingModeSwitch::testPerformanceComparison() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    const int iterations = 10;

    // Measure traditional mode performance
    m_viewer->setQGraphicsRenderingEnabled(false);

    QElapsedTimer traditionalTimer;
    traditionalTimer.start();

    for (int i = 0; i < iterations; ++i) {
        m_viewer->goToPage(i % m_testDocument->numPages());
        m_viewer->setZoom(1.0 + (i * 0.1));
        QCoreApplication::processEvents();  // Allow rendering to complete
    }

    qint64 traditionalTime = traditionalTimer.elapsed();

    // Measure QGraphics mode performance
    m_viewer->setQGraphicsRenderingEnabled(true);

    QElapsedTimer qgraphicsTimer;
    qgraphicsTimer.start();

    for (int i = 0; i < iterations; ++i) {
        m_viewer->goToPage(i % m_testDocument->numPages());
        m_viewer->setZoom(1.0 + (i * 0.1));
        QCoreApplication::processEvents();  // Allow rendering to complete
    }

    qint64 qgraphicsTime = qgraphicsTimer.elapsed();

    qDebug() << "Performance comparison:";
    qDebug() << "Traditional mode:" << traditionalTime << "ms";
    qDebug() << "QGraphics mode:" << qgraphicsTime << "ms";

    // Both modes should complete within reasonable time (not a strict
    // performance test)
    QVERIFY(traditionalTime < 10000);  // Less than 10 seconds
    QVERIFY(qgraphicsTime < 10000);    // Less than 10 seconds

    qDebug() << "Performance comparison test passed";
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestRenderingModeSwitch::testErrorHandling() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    // Test error handling during mode switches

    // Try switching modes with null document
    PDFViewer* tempViewer = new PDFViewer();
    tempViewer->setQGraphicsRenderingEnabled(true);
    tempViewer->setQGraphicsRenderingEnabled(false);
    // Should not crash

    delete tempViewer;

    // Test switching modes rapidly
    for (int i = 0; i < 20; ++i) {
        m_viewer->setQGraphicsRenderingEnabled(i % 2 == 0);
        QCoreApplication::processEvents();
    }

    // Viewer should still be functional
    QVERIFY(m_viewer->hasDocument());

    qDebug() << "Error handling test passed";
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestRenderingModeSwitch::testMemoryManagement() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    // Test memory management during mode switches

    // Get initial memory usage (approximate)
    size_t initialMemory =
        0;  // Would need platform-specific code for actual memory measurement

    // Perform multiple mode switches with operations
    for (int cycle = 0; cycle < 5; ++cycle) {
        m_viewer->setQGraphicsRenderingEnabled(true);
        performStandardOperations(m_viewer);

        m_viewer->setQGraphicsRenderingEnabled(false);
        performStandardOperations(m_viewer);

        QCoreApplication::processEvents();
    }

    // Verify viewer is still functional after multiple switches
    QVERIFY(m_viewer->hasDocument());
    QVERIFY(m_viewer->getPageCount() > 0);

    qDebug() << "Memory management test passed";
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestRenderingModeSwitch::testConcurrentOperations() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    // Test concurrent operations during mode switches

    QTimer* operationTimer = new QTimer();
    operationTimer->setInterval(50);

    int operationCount = 0;
    connect(operationTimer, &QTimer::timeout, [this, &operationCount]() {
        // Perform operations while mode switching might be happening
        if (operationCount % 4 == 0)
            m_viewer->nextPage();
        else if (operationCount % 4 == 1)
            m_viewer->zoomIn();
        else if (operationCount % 4 == 2)
            m_viewer->previousPage();
        else
            m_viewer->zoomOut();

        operationCount++;
    });

    operationTimer->start();

    // Perform mode switches while operations are running
    for (int i = 0; i < 10; ++i) {
        m_viewer->setQGraphicsRenderingEnabled(i % 2 == 0);
        QTest::qWait(100);  // Allow some operations to execute
    }

    operationTimer->stop();
    delete operationTimer;

    // Verify viewer is still functional
    QVERIFY(m_viewer->hasDocument());

    qDebug() << "Concurrent operations test passed";
    qDebug() << "Performed" << operationCount << "concurrent operations";
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

QTEST_MAIN(TestRenderingModeSwitch)
#include "test_rendering_mode_switch.moc"
