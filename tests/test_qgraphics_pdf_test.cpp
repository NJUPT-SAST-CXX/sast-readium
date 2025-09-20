#include <QtTest/QtTest>
#include <QApplication>
#include <QTemporaryFile>
#include "../app/ui/viewer/PDFViewer.h"

class TestQGraphicsPDF : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testConditionalCompilation();
    void testRenderingModeSwitch();
    void testFallbackFunctionality();
    void cleanupTestCase();

private:
    PDFViewer* m_viewer;
    Poppler::Document* m_testDocument;
};

void TestQGraphicsPDF::initTestCase()
{
    m_viewer = new PDFViewer(nullptr, false); // Disable styling for tests
    
    // Create a simple test PDF document (this would normally be loaded from a file)
    // For testing purposes, we'll just test the interface without a real document
    m_testDocument = nullptr;
}

void TestQGraphicsPDF::testConditionalCompilation()
{
    // Test that the conditional compilation works correctly
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    // QGraphics support should be available
    QVERIFY(m_viewer != nullptr);
    
    // Test that QGraphics methods are available
    m_viewer->setQGraphicsRenderingEnabled(true);
    bool isEnabled = m_viewer->isQGraphicsRenderingEnabled();
    QVERIFY(isEnabled == true);
    
    // Test disabling
    m_viewer->setQGraphicsRenderingEnabled(false);
    isEnabled = m_viewer->isQGraphicsRenderingEnabled();
    QVERIFY(isEnabled == false);
    
    qDebug() << "QGraphics PDF support is ENABLED and working correctly";
#else
    // QGraphics support should not be available
    QVERIFY(m_viewer != nullptr);
    
    // Traditional rendering should still work
    QVERIFY(m_viewer->getCurrentPage() >= 0);
    QVERIFY(m_viewer->getCurrentZoom() > 0);
    
    qDebug() << "QGraphics PDF support is DISABLED - using traditional rendering";
#endif
}

void TestQGraphicsPDF::testRenderingModeSwitch()
{
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    // Test switching between rendering modes
    m_viewer->setQGraphicsRenderingEnabled(false);
    QVERIFY(m_viewer->isQGraphicsRenderingEnabled() == false);
    
    m_viewer->setQGraphicsRenderingEnabled(true);
    QVERIFY(m_viewer->isQGraphicsRenderingEnabled() == true);
    
    // Test that basic operations work in both modes
    int currentPage = m_viewer->getCurrentPage();
    double currentZoom = m_viewer->getCurrentZoom();
    
    QVERIFY(currentPage >= 0);
    QVERIFY(currentZoom > 0);
    
    qDebug() << "Rendering mode switching works correctly";
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestQGraphicsPDF::testFallbackFunctionality()
{
    // Test that basic PDF viewer functionality works regardless of QGraphics support
    QVERIFY(m_viewer != nullptr);
    
    // Test basic navigation methods exist and don't crash
    m_viewer->nextPage();
    m_viewer->previousPage();
    m_viewer->firstPage();
    m_viewer->lastPage();
    
    // Test zoom methods
    m_viewer->zoomIn();
    m_viewer->zoomOut();
    m_viewer->zoomToFit();
    m_viewer->zoomToWidth();
    m_viewer->zoomToHeight();
    
    // Test rotation methods
    m_viewer->rotateLeft();
    m_viewer->rotateRight();
    m_viewer->resetRotation();
    
    qDebug() << "Fallback functionality works correctly";
}

void TestQGraphicsPDF::cleanupTestCase()
{
    delete m_viewer;
    if (m_testDocument) {
        delete m_testDocument;
    }
}

// Test main function
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    TestQGraphicsPDF test;
    
    qDebug() << "=== QGraphics PDF Support Test ===";
    
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    qDebug() << "Build configuration: QGraphics support ENABLED";
#else
    qDebug() << "Build configuration: QGraphics support DISABLED";
#endif
    
    int result = QTest::qExec(&test, argc, argv);
    
    qDebug() << "=== Test completed ===";
    
    return result;
}

#include "test_qgraphics_pdf.moc"
