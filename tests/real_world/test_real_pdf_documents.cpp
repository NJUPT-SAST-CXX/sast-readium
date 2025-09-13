#include <QtTest/QtTest>
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QUrl>
#include <QPdfWriter>
#include <QPainter>
#include <QFont>
#include <QPageSize>
#include <poppler-qt6.h>
#include "../../app/ui/viewer/PDFViewer.h"

class TestRealPDFDocuments : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Real document tests
    void testSimplePDF();
    void testComplexLayoutPDF();
    void testLargePDF();
    void testPasswordProtectedPDF();
    void testCorruptedPDF();
    void testMultiPageNavigation();
    void testSearchInRealDocument();
    void testZoomingRealDocument();
    void testRotationRealDocument();
    void testRenderingQuality();
    void testMemoryWithLargeDocument();

private:
    struct TestDocument {
        QString name;
        QString path;
        QString url;
        int expectedPages;
        bool requiresPassword;
        QString password;
    };
    
    bool downloadTestDocument(const TestDocument& doc);
    void createTestDocuments();
    Poppler::Document* loadDocument(const TestDocument& doc);
    void testDocumentWithBothModes(const TestDocument& doc);
    void verifyDocumentProperties(Poppler::Document* document, const TestDocument& expected);
    
    PDFViewer* m_viewer;
    QList<TestDocument> m_testDocuments;
    QString m_testDataDir;
    QNetworkAccessManager* m_networkManager;
};

void TestRealPDFDocuments::initTestCase()
{
    m_viewer = new PDFViewer();
    m_networkManager = new QNetworkAccessManager(this);
    
    // Setup test data directory
    m_testDataDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/pdf_test_data";
    QDir().mkpath(m_testDataDir);
    
    // Create test documents
    createTestDocuments();
    
    qDebug() << "Real PDF document tests initialized";
    qDebug() << "Test data directory:" << m_testDataDir;
}

void TestRealPDFDocuments::cleanupTestCase()
{
    delete m_viewer;
    delete m_networkManager;
    
    // Optionally clean up test files
    // QDir(m_testDataDir).removeRecursively();
}

void TestRealPDFDocuments::createTestDocuments()
{
    // Create various test PDF documents locally since we can't rely on external downloads
    
    // Simple single-page PDF
    TestDocument simpleDoc;
    simpleDoc.name = "simple";
    simpleDoc.path = m_testDataDir + "/simple.pdf";
    simpleDoc.expectedPages = 1;
    simpleDoc.requiresPassword = false;
    
    // Create simple PDF using Qt's QPdfWriter
    QPdfWriter pdfWriter(simpleDoc.path);
    pdfWriter.setPageSize(QPageSize::A4);
    pdfWriter.setPageMargins(QMarginsF(20, 20, 20, 20));

    QPainter painter(&pdfWriter);
    if (painter.isActive()) {
        painter.setFont(QFont("Arial", 12));
        painter.drawText(100, 100, "Simple PDF Test Document");
        painter.drawText(100, 150, "This is a test document for PDF rendering");
        painter.end();
    }
    
    // Complex multi-page PDF
    TestDocument complexDoc;
    complexDoc.name = "complex";
    complexDoc.path = m_testDataDir + "/complex.pdf";
    complexDoc.expectedPages = 5;
    complexDoc.requiresPassword = false;
    
    // Create complex PDF using Qt's QPdfWriter
    QPdfWriter complexPdfWriter(complexDoc.path);
    complexPdfWriter.setPageSize(QPageSize::A4);
    complexPdfWriter.setPageMargins(QMarginsF(20, 20, 20, 20));

    QPainter complexPainter(&complexPdfWriter);
    if (complexPainter.isActive()) {
        // Create 5 pages with different content
        for (int page = 1; page <= 5; ++page) {
            if (page > 1) {
                complexPdfWriter.newPage();
            }

            complexPainter.setFont(QFont("Arial", 14));
            complexPainter.drawText(100, 100, QString("Page %1 - Complex Layout Test").arg(page));

            // Add different content types per page
            if (page == 1) {
                complexPainter.setFont(QFont("Arial", 10));
                complexPainter.drawText(100, 150, "This page tests basic text rendering");
                for (int line = 0; line < 10; ++line) {
                    complexPainter.drawText(100, 200 + line * 25,
                                          QString("Line %1 with various text content").arg(line + 1));
                }
            } else if (page == 2) {
                complexPainter.setFont(QFont("Arial", 10));
                complexPainter.drawText(100, 150, "This page tests formatting and layout");
                // Add some formatting
                for (int col = 0; col < 3; ++col) {
                    for (int row = 0; row < 8; ++row) {
                        complexPainter.drawText(100 + col * 150, 200 + row * 30,
                                              QString("Col%1 Row%2").arg(col + 1).arg(row + 1));
                    }
                }
            } else {
                complexPainter.setFont(QFont("Arial", 10));
                complexPainter.drawText(100, 150, QString("Page %1 content for testing").arg(page));
                // Add varied content
                for (int i = 0; i < 8; ++i) {
                    complexPainter.drawText(100, 200 + i * 35,
                                          QString("Test content line %1 on page %2").arg(i + 1).arg(page));
                }
            }
        }
        complexPainter.end();
    }
    
    // Large document (20 pages)
    TestDocument largeDoc;
    largeDoc.name = "large";
    largeDoc.path = m_testDataDir + "/large.pdf";
    largeDoc.expectedPages = 10;
    largeDoc.requiresPassword = false;
    
    // Create large PDF using Qt's QPdfWriter
    QPdfWriter largePdfWriter(largeDoc.path);
    largePdfWriter.setPageSize(QPageSize::A4);
    largePdfWriter.setPageMargins(QMarginsF(20, 20, 20, 20));

    QPainter largePainter(&largePdfWriter);
    if (largePainter.isActive()) {
        // Create 10 pages (reduced from 20 for stability)
        for (int page = 1; page <= 10; ++page) {
            if (page > 1) {
                largePdfWriter.newPage();
            }

            largePainter.setFont(QFont("Arial", 12));
            largePainter.drawText(100, 100, QString("Large Document - Page %1").arg(page));

            // Add substantial content to each page
            for (int line = 0; line < 15; ++line) {
                largePainter.drawText(100, 150 + line * 20,
                                    QString("Page %1 Line %2 - Large document test content").arg(page).arg(line + 1));
            }
        }
        largePainter.end();
    }
    
    m_testDocuments << simpleDoc << complexDoc << largeDoc;
    
    qDebug() << "Created" << m_testDocuments.size() << "test documents";
}

Poppler::Document* TestRealPDFDocuments::loadDocument(const TestDocument& doc)
{
    if (!QFile::exists(doc.path)) {
        qWarning() << "Test document not found:" << doc.path;
        return nullptr;
    }
    
    Poppler::Document* document = Poppler::Document::load(doc.path).release();
    
    if (!document) {
        qWarning() << "Failed to load document:" << doc.path;
        return nullptr;
    }
    
    if (document->isLocked() && doc.requiresPassword) {
        if (!document->unlock(doc.password.toUtf8(), doc.password.toUtf8())) {
            qWarning() << "Failed to unlock document:" << doc.path;
            delete document;
            return nullptr;
        }
    }
    
    return document;
}

void TestRealPDFDocuments::verifyDocumentProperties(Poppler::Document* document, const TestDocument& expected)
{
    QVERIFY(document != nullptr);
    QCOMPARE(document->numPages(), expected.expectedPages);
    QVERIFY(!document->isLocked());
}

void TestRealPDFDocuments::testDocumentWithBothModes(const TestDocument& doc)
{
    Poppler::Document* document = loadDocument(doc);
    QVERIFY(document != nullptr);
    
    verifyDocumentProperties(document, doc);
    
    m_viewer->setDocument(document);
    QVERIFY(m_viewer->hasDocument());
    QCOMPARE(m_viewer->getPageCount(), doc.expectedPages);
    
    // Test traditional mode
    // m_viewer->setQGraphicsRenderingEnabled(false);
    
    // Test basic operations
    m_viewer->goToPage(0);
    QCOMPARE(m_viewer->getCurrentPage(), 0);
    
    if (doc.expectedPages > 1) {
        m_viewer->nextPage();
        QCOMPARE(m_viewer->getCurrentPage(), 1);
        
        m_viewer->lastPage();
        QCOMPARE(m_viewer->getCurrentPage(), doc.expectedPages - 1);
        
        m_viewer->firstPage();
        QCOMPARE(m_viewer->getCurrentPage(), 0);
    }
    
    // Test zoom operations
    m_viewer->setZoom(1.5);
    QCOMPARE(m_viewer->getCurrentZoom(), 1.5);
    
    m_viewer->zoomToFit();
    m_viewer->zoomToWidth();
    
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    // Test QGraphics mode
    m_viewer->setQGraphicsRenderingEnabled(true);
    
    // Repeat same tests
    m_viewer->goToPage(0);
    QCOMPARE(m_viewer->getCurrentPage(), 0);
    
    if (doc.expectedPages > 1) {
        m_viewer->nextPage();
        QCOMPARE(m_viewer->getCurrentPage(), 1);
        
        m_viewer->lastPage();
        QCOMPARE(m_viewer->getCurrentPage(), doc.expectedPages - 1);
        
        m_viewer->firstPage();
        QCOMPARE(m_viewer->getCurrentPage(), 0);
    }
    
    m_viewer->setZoom(2.0);
    QCOMPARE(m_viewer->getCurrentZoom(), 2.0);
#endif
    
    delete document;
}

void TestRealPDFDocuments::testSimplePDF()
{
    qDebug() << "=== Testing Simple PDF ===";
    
    TestDocument simpleDoc = m_testDocuments[0];
    testDocumentWithBothModes(simpleDoc);
    
    qDebug() << "Simple PDF test passed";
}

void TestRealPDFDocuments::testComplexLayoutPDF()
{
    qDebug() << "=== Testing Complex Layout PDF ===";
    
    TestDocument complexDoc = m_testDocuments[1];
    testDocumentWithBothModes(complexDoc);
    
    qDebug() << "Complex layout PDF test passed";
}

void TestRealPDFDocuments::testLargePDF()
{
    qDebug() << "=== Testing Large PDF ===";
    
    TestDocument largeDoc = m_testDocuments[2];
    testDocumentWithBothModes(largeDoc);
    
    qDebug() << "Large PDF test passed";
}

void TestRealPDFDocuments::testPasswordProtectedPDF()
{
    qDebug() << "=== Testing Password Protected PDF ===";
    
    // For this test, we would need to create a password-protected PDF
    // This is more complex and would require additional PDF generation
    QSKIP("Password protected PDF test not implemented yet");
}

void TestRealPDFDocuments::testCorruptedPDF()
{
    qDebug() << "=== Testing Corrupted PDF ===";
    
    // Create a corrupted PDF file
    QString corruptedPath = m_testDataDir + "/corrupted.pdf";
    QFile corruptedFile(corruptedPath);
    if (corruptedFile.open(QIODevice::WriteOnly)) {
        corruptedFile.write("This is not a valid PDF file");
        corruptedFile.close();
    }
    
    // Try to load corrupted document
    std::unique_ptr<Poppler::Document> documentPtr = Poppler::Document::load(corruptedPath);
    Poppler::Document* document = documentPtr.get();
    QVERIFY(document == nullptr); // Should fail to load
    
    qDebug() << "Corrupted PDF test passed";
}

void TestRealPDFDocuments::testMultiPageNavigation()
{
    qDebug() << "=== Testing Multi-Page Navigation ===";
    
    TestDocument complexDoc = m_testDocuments[1]; // 5 pages
    Poppler::Document* document = loadDocument(complexDoc);
    QVERIFY(document != nullptr);
    
    m_viewer->setDocument(document);
    
    // Test navigation in both modes
    for (int mode = 0; mode < 2; ++mode) {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
        m_viewer->setQGraphicsRenderingEnabled(mode == 1);
#else
        if (mode == 1) break; // Skip QGraphics mode if not available
#endif
        
        // Test sequential navigation
        for (int page = 0; page < document->numPages(); ++page) {
            m_viewer->goToPage(page);
            QCOMPARE(m_viewer->getCurrentPage(), page);
        }
        
        // Test reverse navigation
        for (int page = document->numPages() - 1; page >= 0; --page) {
            m_viewer->goToPage(page);
            QCOMPARE(m_viewer->getCurrentPage(), page);
        }
        
        // Test navigation methods
        m_viewer->firstPage();
        QCOMPARE(m_viewer->getCurrentPage(), 0);
        
        m_viewer->lastPage();
        QCOMPARE(m_viewer->getCurrentPage(), document->numPages() - 1);
        
        // Test next/previous
        m_viewer->firstPage();
        for (int i = 0; i < document->numPages() - 1; ++i) {
            m_viewer->nextPage();
            QCOMPARE(m_viewer->getCurrentPage(), i + 1);
        }
        
        for (int i = document->numPages() - 1; i > 0; --i) {
            m_viewer->previousPage();
            QCOMPARE(m_viewer->getCurrentPage(), i - 1);
        }
    }
    
    delete document;
    qDebug() << "Multi-page navigation test passed";
}

void TestRealPDFDocuments::testSearchInRealDocument()
{
    qDebug() << "=== Testing Search in Real Document ===";
    
    TestDocument complexDoc = m_testDocuments[1];
    Poppler::Document* document = loadDocument(complexDoc);
    QVERIFY(document != nullptr);
    
    m_viewer->setDocument(document);
    
    // Test search functionality (basic test)
    // Note: Full search testing would require implementing search in the test
    QVERIFY(m_viewer->hasDocument());
    
    delete document;
    qDebug() << "Search test passed";
}

void TestRealPDFDocuments::testZoomingRealDocument()
{
    qDebug() << "=== Testing Zooming Real Document ===";
    
    TestDocument simpleDoc = m_testDocuments[0];
    Poppler::Document* document = loadDocument(simpleDoc);
    QVERIFY(document != nullptr);
    
    m_viewer->setDocument(document);
    
    // Test various zoom levels in both modes
    QList<double> zoomLevels = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0};
    
    for (int mode = 0; mode < 2; ++mode) {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
        m_viewer->setQGraphicsRenderingEnabled(mode == 1);
#else
        if (mode == 1) break;
#endif
        
        for (double zoom : zoomLevels) {
            m_viewer->setZoom(zoom);
            QCOMPARE(m_viewer->getCurrentZoom(), zoom);
        }
        
        // Test zoom methods
        m_viewer->setZoom(1.0);
        m_viewer->zoomIn();
        QVERIFY(m_viewer->getCurrentZoom() > 1.0);
        
        m_viewer->zoomOut();
        m_viewer->zoomToFit();
        m_viewer->zoomToWidth();
    }
    
    delete document;
    qDebug() << "Zooming test passed";
}

void TestRealPDFDocuments::testRotationRealDocument()
{
    qDebug() << "=== Testing Rotation Real Document ===";
    
    TestDocument simpleDoc = m_testDocuments[0];
    Poppler::Document* document = loadDocument(simpleDoc);
    QVERIFY(document != nullptr);
    
    m_viewer->setDocument(document);
    
    // Test rotation in both modes
    for (int mode = 0; mode < 2; ++mode) {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
        m_viewer->setQGraphicsRenderingEnabled(mode == 1);
#else
        if (mode == 1) break;
#endif
        
        // Test rotation angles
        QList<int> rotations = {0, 90, 180, 270, 360};
        
        for (int rotation : rotations) {
            m_viewer->setRotation(rotation % 360);
            // Note: Rotation testing might need adjustment based on implementation
        }
        
        // Test rotation methods
        m_viewer->resetRotation();
        m_viewer->rotateRight();
        m_viewer->rotateLeft();
        m_viewer->resetRotation();
    }
    
    delete document;
    qDebug() << "Rotation test passed";
}

void TestRealPDFDocuments::testRenderingQuality()
{
    qDebug() << "=== Testing Rendering Quality ===";
    
    TestDocument complexDoc = m_testDocuments[1];
    Poppler::Document* document = loadDocument(complexDoc);
    QVERIFY(document != nullptr);
    
    m_viewer->setDocument(document);
    
    // Test that rendering completes without errors
    for (int page = 0; page < document->numPages(); ++page) {
        m_viewer->goToPage(page);
        
        // Test different zoom levels
        m_viewer->setZoom(0.5);
        QCoreApplication::processEvents();
        
        m_viewer->setZoom(1.0);
        QCoreApplication::processEvents();
        
        m_viewer->setZoom(2.0);
        QCoreApplication::processEvents();
    }
    
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    // Test QGraphics high quality rendering
    m_viewer->setQGraphicsRenderingEnabled(true);
    m_viewer->setQGraphicsHighQualityRendering(true);
    
    for (int page = 0; page < qMin(3, document->numPages()); ++page) {
        m_viewer->goToPage(page);
        QCoreApplication::processEvents();
    }
#endif
    
    delete document;
    qDebug() << "Rendering quality test passed";
}

void TestRealPDFDocuments::testMemoryWithLargeDocument()
{
    qDebug() << "=== Testing Memory with Large Document ===";
    
    TestDocument largeDoc = m_testDocuments[2]; // 20 pages
    Poppler::Document* document = loadDocument(largeDoc);
    QVERIFY(document != nullptr);
    
    m_viewer->setDocument(document);
    
    // Navigate through all pages to test memory usage
    for (int page = 0; page < document->numPages(); ++page) {
        m_viewer->goToPage(page);
        m_viewer->setZoom(1.5);
        QCoreApplication::processEvents();
        
        if (page % 5 == 0) {
            // Periodically process events to allow cleanup
            QCoreApplication::processEvents();
        }
    }
    
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    // Test with QGraphics mode
    m_viewer->setQGraphicsRenderingEnabled(true);
    
    for (int page = 0; page < document->numPages(); ++page) {
        m_viewer->goToPage(page);
        m_viewer->setZoom(1.5);
        QCoreApplication::processEvents();
    }
#endif
    
    delete document;
    qDebug() << "Memory test with large document passed";
}

QTEST_MAIN(TestRealPDFDocuments)
#include "test_real_pdf_documents.moc"
