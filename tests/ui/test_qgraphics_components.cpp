#include <poppler-qt6.h>
#include <QApplication>
#include <QGraphicsView>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTimer>
#include <QtTest/QtTest>

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
#include "../../app/ui/viewer/QGraphicsPDFViewer.h"
#endif

class TestQGraphicsComponents : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // QGraphicsPDFPageItem tests
    void testPageItemCreation();
    void testPageItemScaling();
    void testPageItemRotation();
    void testPageItemRendering();
    void testPageItemSearchHighlights();

    // QGraphicsPDFScene tests
    void testSceneCreation();
    void testSceneDocumentManagement();
    void testScenePageLayout();
    void testSceneSignals();

    // QGraphicsPDFViewer tests
    void testViewerCreation();
    void testViewerNavigation();
    void testViewerZooming();
    void testViewerViewModes();
    void testViewerInteraction();

private:
    Poppler::Document* createTestDocument();
    void cleanupTestDocument(Poppler::Document* doc);

    Poppler::Document* m_testDocument;
    QApplication* m_app;
};

void TestQGraphicsComponents::initTestCase() {
    // Create a simple test PDF document
    m_testDocument = createTestDocument();
    QVERIFY(m_testDocument != nullptr);
}

void TestQGraphicsComponents::cleanupTestCase() {
    cleanupTestDocument(m_testDocument);
}

Poppler::Document* TestQGraphicsComponents::createTestDocument() {
    // Create a minimal PDF content for testing
    QString testPdfPath =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
        "/test.pdf";

    // Create a simple PDF file with basic content
    QFile file(testPdfPath);
    if (file.open(QIODevice::WriteOnly)) {
        // Minimal PDF structure for testing
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
            "/Kids [3 0 R]\n"
            "/Count 1\n"
            ">>\n"
            "endobj\n"
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
            "/Length 44\n"
            ">>\n"
            "stream\n"
            "BT\n"
            "/F1 12 Tf\n"
            "100 700 Td\n"
            "(Test PDF) Tj\n"
            "ET\n"
            "endstream\n"
            "endobj\n"
            "xref\n"
            "0 5\n"
            "0000000000 65535 f \n"
            "0000000009 65535 n \n"
            "0000000074 65535 n \n"
            "0000000120 65535 n \n"
            "0000000179 65535 n \n"
            "trailer\n"
            "<<\n"
            "/Size 5\n"
            "/Root 1 0 R\n"
            ">>\n"
            "startxref\n"
            "274\n"
            "%%EOF\n";

        file.write(pdfContent);
        file.close();

        return Poppler::Document::load(testPdfPath).release();
    }

    return nullptr;
}

void TestQGraphicsComponents::cleanupTestDocument(Poppler::Document* doc) {
    if (doc) {
        delete doc;
    }
}

void TestQGraphicsComponents::testPageItemCreation() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    QGraphicsPDFPageItem* pageItem = new QGraphicsPDFPageItem();
    QVERIFY(pageItem != nullptr);

    // Test initial state
    QCOMPARE(pageItem->getScaleFactor(), 1.0);
    QCOMPARE(pageItem->getRotation(), 0);
    QCOMPARE(pageItem->getPageNumber(), -1);

    delete pageItem;
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestQGraphicsComponents::testPageItemScaling() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    QGraphicsPDFPageItem* pageItem = new QGraphicsPDFPageItem();

    // Test scaling
    pageItem->setScaleFactor(2.0);
    QCOMPARE(pageItem->getScaleFactor(), 2.0);

    // Test scale bounds
    pageItem->setScaleFactor(0.05);  // Below minimum
    QVERIFY(pageItem->getScaleFactor() >= 0.1);

    pageItem->setScaleFactor(15.0);  // Above maximum
    QVERIFY(pageItem->getScaleFactor() <= 10.0);

    delete pageItem;
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestQGraphicsComponents::testPageItemRotation() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    QGraphicsPDFPageItem* pageItem = new QGraphicsPDFPageItem();

    // Test rotation
    pageItem->setRotation(90);
    QCOMPARE(pageItem->getRotation(), 90);

    pageItem->setRotation(180);
    QCOMPARE(pageItem->getRotation(), 180);

    pageItem->setRotation(270);
    QCOMPARE(pageItem->getRotation(), 270);

    // Test rotation normalization
    pageItem->setRotation(450);  // 450 degrees should become 90
    QCOMPARE(pageItem->getRotation(), 90);

    pageItem->setRotation(-90);  // -90 degrees should become 270
    QCOMPARE(pageItem->getRotation(), 270);

    delete pageItem;
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestQGraphicsComponents::testPageItemRendering() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    if (!m_testDocument) {
        QSKIP("Test document not available");
    }

    QGraphicsPDFPageItem* pageItem = new QGraphicsPDFPageItem();
    std::unique_ptr<Poppler::Page> page(m_testDocument->page(0));

    if (page) {
        pageItem->setPage(page.get(), 1.0, 0);
        QCOMPARE(pageItem->getPageNumber(), 0);

        // Render synchronously for testing
        pageItem->renderPageSync();

        // Test that bounding rect is valid after rendering
        QRectF bounds = pageItem->boundingRect();
        QVERIFY(!bounds.isEmpty());
        QVERIFY(bounds.width() > 0);
        QVERIFY(bounds.height() > 0);
    }

    delete pageItem;
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestQGraphicsComponents::testPageItemSearchHighlights() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    QGraphicsPDFPageItem* pageItem = new QGraphicsPDFPageItem();

    // Test search highlights
    QList<QRectF> searchResults;
    searchResults << QRectF(10, 10, 50, 20);
    searchResults << QRectF(100, 100, 30, 15);

    pageItem->setSearchResults(searchResults);

    // Test current search result
    pageItem->setCurrentSearchResult(0);
    pageItem->setCurrentSearchResult(1);

    // Test clearing highlights
    pageItem->clearSearchHighlights();

    delete pageItem;
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestQGraphicsComponents::testSceneCreation() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    QGraphicsPDFScene* scene = new QGraphicsPDFScene();
    QVERIFY(scene != nullptr);

    // Test initial state
    QCOMPARE(scene->getPageCount(), 0);
    QVERIFY(scene->getPageItem(0) == nullptr);

    delete scene;
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestQGraphicsComponents::testSceneDocumentManagement() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    if (!m_testDocument) {
        QSKIP("Test document not available");
    }

    QGraphicsPDFScene* scene = new QGraphicsPDFScene();

    // Test setting document
    scene->setDocument(m_testDocument);
    QCOMPARE(scene->getPageCount(), m_testDocument->numPages());

    // Test getting page items
    QGraphicsPDFPageItem* pageItem = scene->getPageItem(0);
    QVERIFY(pageItem != nullptr);
    QCOMPARE(pageItem->getPageNumber(), 0);

    // Test clearing document
    scene->clearDocument();
    QCOMPARE(scene->getPageCount(), 0);

    delete scene;
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestQGraphicsComponents::testScenePageLayout() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    if (!m_testDocument) {
        QSKIP("Test document not available");
    }

    QGraphicsPDFScene* scene = new QGraphicsPDFScene();
    scene->setDocument(m_testDocument);

    // Test page spacing
    scene->setPageSpacing(30);
    scene->updateLayout();

    // Test page margin
    scene->setPageMargin(60);
    scene->updateLayout();

    // Verify scene rect is updated
    QRectF sceneRect = scene->sceneRect();
    QVERIFY(!sceneRect.isEmpty());

    delete scene;
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestQGraphicsComponents::testSceneSignals() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    if (!m_testDocument) {
        QSKIP("Test document not available");
    }

    QGraphicsPDFScene* scene = new QGraphicsPDFScene();
    scene->setDocument(m_testDocument);

    // Test scale changed signal
    QSignalSpy scaleChangedSpy(scene, &QGraphicsPDFScene::scaleChanged);
    scene->setScaleFactor(1.5);
    QCOMPARE(scaleChangedSpy.count(), 1);

    delete scene;
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestQGraphicsComponents::testViewerCreation() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    QGraphicsPDFViewer* viewer = new QGraphicsPDFViewer();
    QVERIFY(viewer != nullptr);

    // Test initial state
    QCOMPARE(viewer->getCurrentPage(), 0);
    QCOMPARE(viewer->getZoomFactor(), 1.0);
    QCOMPARE(viewer->getRotation(), 0);
    QCOMPARE(viewer->getPageCount(), 0);
    QVERIFY(!viewer->hasDocument());

    delete viewer;
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestQGraphicsComponents::testViewerNavigation() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    if (!m_testDocument) {
        QSKIP("Test document not available");
    }

    QGraphicsPDFViewer* viewer = new QGraphicsPDFViewer();
    viewer->setDocument(m_testDocument);

    QVERIFY(viewer->hasDocument());
    QCOMPARE(viewer->getPageCount(), m_testDocument->numPages());

    // Test navigation (for single page document, these should not crash)
    viewer->nextPage();
    viewer->previousPage();
    viewer->firstPage();
    viewer->lastPage();
    viewer->goToPage(0);

    delete viewer;
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestQGraphicsComponents::testViewerZooming() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    QGraphicsPDFViewer* viewer = new QGraphicsPDFViewer();

    // Test zoom operations
    double initialZoom = viewer->getZoomFactor();

    viewer->zoomIn();
    QVERIFY(viewer->getZoomFactor() > initialZoom);

    viewer->zoomOut();
    viewer->resetZoom();
    QCOMPARE(viewer->getZoomFactor(), 1.0);

    viewer->setZoom(2.0);
    QCOMPARE(viewer->getZoomFactor(), 2.0);

    delete viewer;
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestQGraphicsComponents::testViewerViewModes() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    QGraphicsPDFViewer* viewer = new QGraphicsPDFViewer();

    // Test view modes
    viewer->setViewMode(QGraphicsPDFViewer::SinglePage);
    QCOMPARE(viewer->getViewMode(), QGraphicsPDFViewer::SinglePage);

    viewer->setViewMode(QGraphicsPDFViewer::ContinuousPage);
    QCOMPARE(viewer->getViewMode(), QGraphicsPDFViewer::ContinuousPage);

    delete viewer;
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

void TestQGraphicsComponents::testViewerInteraction() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    QGraphicsPDFViewer* viewer = new QGraphicsPDFViewer();

    // Test high quality rendering
    viewer->setHighQualityRendering(true);
    viewer->setHighQualityRendering(false);

    // Test page spacing and margins
    viewer->setPageSpacing(25);
    viewer->setPageMargin(40);

    // Test smooth scrolling
    viewer->setSmoothScrolling(true);
    viewer->setSmoothScrolling(false);

    delete viewer;
#else
    QSKIP("QGraphics support not compiled in");
#endif
}

QTEST_MAIN(TestQGraphicsComponents)
#include "test_qgraphics_components.moc"
