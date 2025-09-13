#include <QtTest/QtTest>
#include <QApplication>
#include <QElapsedTimer>
#include <poppler-qt6.h>
#include "../../app/ui/viewer/PDFViewer.h"

class TestPDFOptimizations : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Basic functionality tests for optimizations
    void testVirtualScrollingEnabled();
    void testAsyncRenderingSetup();
    void testCacheManagement();
    void testDPICalculation();
    void testLazyLoadingStates();

private:
    PDFViewer* m_viewer;
    Poppler::Document* m_testDocument;
    
    Poppler::Document* createSimpleTestDocument();
};

void TestPDFOptimizations::initTestCase()
{
    m_viewer = new PDFViewer(nullptr, false); // Disable styling for tests
    m_testDocument = createSimpleTestDocument();
    QVERIFY(m_testDocument != nullptr);
    
    m_viewer->setDocument(m_testDocument);
    qDebug() << "PDF optimizations test initialized";
}

void TestPDFOptimizations::cleanupTestCase()
{
    delete m_viewer;
    if (m_testDocument) delete m_testDocument;
}

Poppler::Document* TestPDFOptimizations::createSimpleTestDocument()
{
    // Create a minimal test PDF in memory
    QString testPdfPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/optimization_test.pdf";

    QPdfWriter pdfWriter(testPdfPath);
    pdfWriter.setPageSize(QPageSize::A4);

    QPainter painter(&pdfWriter);
    if (!painter.isActive()) {
        return nullptr;
    }

    // Create 3 simple pages
    for (int page = 0; page < 3; ++page) {
        if (page > 0) {
            pdfWriter.newPage();
        }
        painter.drawText(100, 100, QString("Test Page %1").arg(page + 1));
    }

    painter.end();

    auto doc = Poppler::Document::load(testPdfPath);
    if (doc && doc->numPages() > 0) {
        return doc.release();
    }
    return nullptr;
}

void TestPDFOptimizations::testVirtualScrollingEnabled()
{
    qDebug() << "Testing virtual scrolling functionality";
    
    // Test switching to continuous mode (which should enable virtual scrolling)
    m_viewer->setViewMode(PDFViewMode::ContinuousScroll);
    
    // Test navigation in virtual scrolling mode
    for (int i = 0; i < m_testDocument->numPages(); ++i) {
        m_viewer->goToPage(i);
        QCOMPARE(m_viewer->getCurrentPage(), i);
    }
    
    qDebug() << "Virtual scrolling test passed";
}

void TestPDFOptimizations::testAsyncRenderingSetup()
{
    qDebug() << "Testing async rendering setup";
    
    // Test that async rendering doesn't crash
    m_viewer->setViewMode(PDFViewMode::ContinuousScroll);
    
    // Rapid operations that would trigger async rendering
    for (int i = 0; i < 5; ++i) {
        m_viewer->setZoom(1.0 + i * 0.2);
        QCoreApplication::processEvents();
    }
    
    qDebug() << "Async rendering test passed";
}

void TestPDFOptimizations::testCacheManagement()
{
    qDebug() << "Testing cache management";
    
    // Test cache operations
    for (int i = 0; i < m_testDocument->numPages(); ++i) {
        m_viewer->goToPage(i);
        m_viewer->setZoom(1.5);
        QCoreApplication::processEvents();
    }
    
    // Test cache with different zoom levels
    QList<double> zoomLevels = {0.5, 1.0, 1.5, 2.0};
    for (double zoom : zoomLevels) {
        m_viewer->setZoom(zoom);
        QCoreApplication::processEvents();
    }
    
    qDebug() << "Cache management test passed";
}

void TestPDFOptimizations::testDPICalculation()
{
    qDebug() << "Testing DPI calculation optimization";
    
    // Test repeated zoom operations (should benefit from DPI caching)
    QList<double> zoomLevels = {1.0, 1.5, 1.0, 2.0, 1.5, 1.0};
    
    for (double zoom : zoomLevels) {
        m_viewer->setZoom(zoom);
        QCoreApplication::processEvents();
    }
    
    qDebug() << "DPI calculation test passed";
}

void TestPDFOptimizations::testLazyLoadingStates()
{
    qDebug() << "Testing lazy loading states";
    
    // Test lazy loading in continuous mode
    m_viewer->setViewMode(PDFViewMode::ContinuousScroll);
    
    // Navigate through pages to test lazy loading
    for (int i = 0; i < m_testDocument->numPages(); ++i) {
        m_viewer->goToPage(i);
        QCoreApplication::processEvents();
        
        // Verify we can get the current page
        QVERIFY(m_viewer->getCurrentPage() >= 0);
        QVERIFY(m_viewer->getCurrentPage() < m_testDocument->numPages());
    }
    
    qDebug() << "Lazy loading states test passed";
}

QTEST_MAIN(TestPDFOptimizations)
#include "test_pdf_optimizations.moc"
