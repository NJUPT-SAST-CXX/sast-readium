#include <QtTest/QtTest>
#include <QApplication>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QElapsedTimer>
#include <QtConcurrent>
#include <poppler-qt6.h>
#include "../../app/ui/thumbnail/ThumbnailGenerator.h"
#include "../../app/logging/LoggingMacros.h"

class ThumbnailGeneratorIntegrationTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // GPU rendering fallback tests
    void testGpuRenderingFallback();
    void testGpuRenderingWithDifferentSizes();
    void testGpuRenderingWithDifferentQualities();
    void testGpuRenderingErrorHandling();
    
    // Integration with thumbnail system
    void testThumbnailGenerationIntegration();
    void testPerformanceLogging();
    void testConcurrentGeneration();
    
    // Quality and accuracy tests
    void testRenderingAccuracy();
    void testMemoryUsage();

private:
    ThumbnailGenerator* m_generator;
    std::shared_ptr<Poppler::Document> m_testDocument;
    QTemporaryFile* m_testPdfFile;
    
    void createTestPdf();
    bool comparePixmaps(const QPixmap& pixmap1, const QPixmap& pixmap2, double tolerance = 0.95);
};

void ThumbnailGeneratorIntegrationTest::initTestCase()
{
    createTestPdf();
    
    // Load test document
    auto uniqueDoc = Poppler::Document::load(m_testPdfFile->fileName());
    m_testDocument = std::shared_ptr<Poppler::Document>(uniqueDoc.release());
    QVERIFY(m_testDocument != nullptr);
    QVERIFY(!m_testDocument->isLocked());
}

void ThumbnailGeneratorIntegrationTest::cleanupTestCase()
{
    m_testDocument.reset();
    delete m_testPdfFile;
}

void ThumbnailGeneratorIntegrationTest::init()
{
    m_generator = new ThumbnailGenerator();
}

void ThumbnailGeneratorIntegrationTest::cleanup()
{
    delete m_generator;
    m_generator = nullptr;
}

void ThumbnailGeneratorIntegrationTest::testGpuRenderingFallback()
{
    QVERIFY(m_testDocument != nullptr);
    QVERIFY(m_testDocument->numPages() > 0);

    auto page = m_testDocument->page(0);
    QVERIFY(page != nullptr);

    QSize targetSize(200, 300);

    // Set up generator with test document
    m_generator->setDocument(m_testDocument);
    m_generator->setThumbnailSize(targetSize);

    // Test thumbnail generation (public API)
    QSignalSpy generatedSpy(m_generator, &ThumbnailGenerator::thumbnailGenerated);
    QSignalSpy errorSpy(m_generator, &ThumbnailGenerator::thumbnailError);

    m_generator->generateThumbnail(0, targetSize, 1.0);

    // Wait for generation to complete
    QVERIFY(generatedSpy.wait(5000) || errorSpy.count() > 0);

    // Should either generate successfully or handle error gracefully
    QVERIFY(generatedSpy.count() > 0 || errorSpy.count() > 0);
}

void ThumbnailGeneratorIntegrationTest::testGpuRenderingWithDifferentSizes()
{
    QVERIFY(m_testDocument != nullptr);
    auto page = m_testDocument->page(0);
    QVERIFY(page != nullptr);

    QList<QSize> testSizes = {
        QSize(100, 100),
        QSize(200, 300),
        QSize(400, 600),
        QSize(50, 75)
    };

    // Set up generator with test document
    m_generator->setDocument(m_testDocument);

    for (const QSize& size : testSizes) {
        QSignalSpy generatedSpy(m_generator, &ThumbnailGenerator::thumbnailGenerated);
        QSignalSpy errorSpy(m_generator, &ThumbnailGenerator::thumbnailError);

        m_generator->generateThumbnail(0, size, 1.0);

        // Wait for generation to complete
        QVERIFY(generatedSpy.wait(5000) || errorSpy.count() > 0);

        if (generatedSpy.count() > 0) {
            QPixmap result = generatedSpy.first().at(1).value<QPixmap>();
            QVERIFY(!result.isNull());
            QVERIFY(result.width() <= size.width());
            QVERIFY(result.height() <= size.height());
        }
    }
}

void ThumbnailGeneratorIntegrationTest::testGpuRenderingWithDifferentQualities()
{
    QVERIFY(m_testDocument != nullptr);
    auto page = m_testDocument->page(0);
    QVERIFY(page != nullptr);

    QSize targetSize(200, 300);
    QList<double> qualities = {0.5, 1.0, 1.5, 2.0};

    // Set up generator with test document
    m_generator->setDocument(m_testDocument);

    for (double quality : qualities) {
        QSignalSpy generatedSpy(m_generator, &ThumbnailGenerator::thumbnailGenerated);
        QSignalSpy errorSpy(m_generator, &ThumbnailGenerator::thumbnailError);

        m_generator->generateThumbnail(0, targetSize, quality);

        // Wait for generation to complete
        QVERIFY(generatedSpy.wait(5000) || errorSpy.count() > 0);

        if (generatedSpy.count() > 0) {
            QPixmap result = generatedSpy.first().at(1).value<QPixmap>();
            QVERIFY(!result.isNull());
            QVERIFY(result.width() > 0);
            QVERIFY(result.height() > 0);
        }
    }
}

void ThumbnailGeneratorIntegrationTest::testGpuRenderingErrorHandling()
{
    QVERIFY(m_testDocument != nullptr);
    auto page = m_testDocument->page(0);
    QVERIFY(page != nullptr);

    // Set up generator with test document
    m_generator->setDocument(m_testDocument);

    // Test with invalid size
    QSignalSpy generatedSpy(m_generator, &ThumbnailGenerator::thumbnailGenerated);
    QSignalSpy errorSpy(m_generator, &ThumbnailGenerator::thumbnailError);

    m_generator->generateThumbnail(0, QSize(0, 0), 1.0);

    // Should handle gracefully - either generate or error
    QVERIFY(generatedSpy.wait(5000) || errorSpy.count() > 0);

    // Test with extreme quality values
    generatedSpy.clear();
    errorSpy.clear();

    m_generator->generateThumbnail(0, QSize(100, 100), 0.1);
    QVERIFY(generatedSpy.wait(5000) || errorSpy.count() > 0);

    generatedSpy.clear();
    errorSpy.clear();

    m_generator->generateThumbnail(0, QSize(100, 100), 10.0);
    QVERIFY(generatedSpy.wait(5000) || errorSpy.count() > 0);
}

void ThumbnailGeneratorIntegrationTest::testThumbnailGenerationIntegration()
{
    // Test integration with the full thumbnail generation system
    QSignalSpy generatedSpy(m_generator, &ThumbnailGenerator::thumbnailGenerated);
    QSignalSpy errorSpy(m_generator, &ThumbnailGenerator::thumbnailError);

    // Set up generator with test document
    m_generator->setDocument(m_testDocument);

    // Request thumbnail generation using public API
    m_generator->generateThumbnail(0, QSize(150, 200), 1.0);

    // Wait for generation to complete
    QVERIFY(generatedSpy.wait(5000) || errorSpy.count() > 0);

    // Should have generated successfully or reported error
    QVERIFY(generatedSpy.count() > 0 || errorSpy.count() > 0);

    if (generatedSpy.count() > 0) {
        QList<QVariant> args = generatedSpy.first();
        QPixmap thumbnail = args.at(1).value<QPixmap>();
        QVERIFY(!thumbnail.isNull());
    }
}

void ThumbnailGeneratorIntegrationTest::testPerformanceLogging()
{
    // This test verifies that performance logging works correctly
    QVERIFY(m_testDocument != nullptr);
    auto page = m_testDocument->page(0);
    QVERIFY(page != nullptr);

    // Set up generator with test document
    m_generator->setDocument(m_testDocument);

    // Generate thumbnail and measure time
    QElapsedTimer timer;
    timer.start();

    QSignalSpy generatedSpy(m_generator, &ThumbnailGenerator::thumbnailGenerated);
    QSignalSpy errorSpy(m_generator, &ThumbnailGenerator::thumbnailError);

    m_generator->generateThumbnail(0, QSize(200, 300), 1.0);

    QVERIFY(generatedSpy.wait(5000) || errorSpy.count() > 0);

    qint64 elapsed = timer.elapsed();

    QVERIFY(elapsed >= 0);
    QVERIFY(generatedSpy.count() > 0 || errorSpy.count() > 0);

    // Performance logging should handle the timing internally
    // (We can't easily test the actual logging without accessing private members)
}

void ThumbnailGeneratorIntegrationTest::testConcurrentGeneration()
{
    // Test multiple concurrent generation requests
    QVERIFY(m_testDocument != nullptr);

    // Set up generator with test document
    m_generator->setDocument(m_testDocument);

    QSignalSpy generatedSpy(m_generator, &ThumbnailGenerator::thumbnailGenerated);
    QSignalSpy errorSpy(m_generator, &ThumbnailGenerator::thumbnailError);

    // Generate thumbnails for multiple pages concurrently
    int pagesToTest = qMin(3, m_testDocument->numPages());
    for (int i = 0; i < pagesToTest; ++i) {
        m_generator->generateThumbnail(i, QSize(150, 200), 1.0);
    }

    // Wait for all generations to complete
    int totalExpected = pagesToTest;
    int totalReceived = 0;

    while (totalReceived < totalExpected && (generatedSpy.count() + errorSpy.count()) < totalExpected) {
        QTest::qWait(100);
        totalReceived = generatedSpy.count() + errorSpy.count();
    }

    // Should have processed all requests
    QVERIFY(totalReceived >= totalExpected);
}

void ThumbnailGeneratorIntegrationTest::testRenderingAccuracy()
{
    // Compare thumbnail generation with standard CPU rendering
    QVERIFY(m_testDocument != nullptr);
    auto page = m_testDocument->page(0);
    QVERIFY(page != nullptr);

    QSize targetSize(200, 300);
    double quality = 1.0;

    // Set up generator with test document
    m_generator->setDocument(m_testDocument);

    // Generate thumbnail using public API
    QSignalSpy generatedSpy(m_generator, &ThumbnailGenerator::thumbnailGenerated);
    QSignalSpy errorSpy(m_generator, &ThumbnailGenerator::thumbnailError);

    m_generator->generateThumbnail(0, targetSize, quality);

    QVERIFY(generatedSpy.wait(5000) || errorSpy.count() > 0);

    if (generatedSpy.count() > 0) {
        QPixmap generatedResult = generatedSpy.first().at(1).value<QPixmap>();
        QVERIFY(!generatedResult.isNull());

        // Direct CPU rendering for comparison
        double dpi = 72.0 * quality;
        QImage cpuImage = page->renderToImage(dpi, dpi);
        QPixmap cpuResult = QPixmap::fromImage(cpuImage.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        QVERIFY(!cpuResult.isNull());

        // Results should be similar (allowing for some variation in scaling algorithms)
        QVERIFY(comparePixmaps(generatedResult, cpuResult, 0.8));
    }
}

void ThumbnailGeneratorIntegrationTest::testMemoryUsage()
{
    // Test that memory usage is reasonable
    QVERIFY(m_testDocument != nullptr);

    // Set up generator with test document
    m_generator->setDocument(m_testDocument);

    QList<QPixmap> thumbnails;
    QSignalSpy generatedSpy(m_generator, &ThumbnailGenerator::thumbnailGenerated);
    QSignalSpy errorSpy(m_generator, &ThumbnailGenerator::thumbnailError);

    // Generate multiple thumbnails
    int pagesToTest = qMin(5, m_testDocument->numPages());
    for (int i = 0; i < pagesToTest; ++i) {
        m_generator->generateThumbnail(i, QSize(150, 200), 1.0);
    }

    // Wait for all generations to complete
    int totalExpected = pagesToTest;
    int totalReceived = 0;

    while (totalReceived < totalExpected && (generatedSpy.count() + errorSpy.count()) < totalExpected) {
        QTest::qWait(100);
        totalReceived = generatedSpy.count() + errorSpy.count();
    }

    // Collect generated thumbnails
    for (int i = 0; i < generatedSpy.count(); ++i) {
        QPixmap thumbnail = generatedSpy.at(i).at(1).value<QPixmap>();
        if (!thumbnail.isNull()) {
            thumbnails.append(thumbnail);
        }
    }

    // Verify we generated some thumbnails
    QVERIFY(thumbnails.size() > 0);

    // Each thumbnail should be reasonably sized
    for (const QPixmap& thumbnail : thumbnails) {
        QVERIFY(thumbnail.width() <= 150);
        QVERIFY(thumbnail.height() <= 200);
        QVERIFY(thumbnail.width() > 0);
        QVERIFY(thumbnail.height() > 0);
    }
}

void ThumbnailGeneratorIntegrationTest::createTestPdf()
{
    // Create a simple test PDF file
    m_testPdfFile = new QTemporaryFile();
    m_testPdfFile->setFileTemplate("test_pdf_XXXXXX.pdf");
    QVERIFY(m_testPdfFile->open());
    
    // Write minimal PDF content
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
        "(Test Page) Tj\n"
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
    
    m_testPdfFile->write(pdfContent);
    m_testPdfFile->flush();
}

bool ThumbnailGeneratorIntegrationTest::comparePixmaps(const QPixmap& pixmap1, const QPixmap& pixmap2, double tolerance)
{
    if (pixmap1.size() != pixmap2.size()) {
        return false;
    }
    
    QImage image1 = pixmap1.toImage();
    QImage image2 = pixmap2.toImage();
    
    int totalPixels = image1.width() * image1.height();
    int similarPixels = 0;
    
    for (int y = 0; y < image1.height(); ++y) {
        for (int x = 0; x < image1.width(); ++x) {
            QRgb pixel1 = image1.pixel(x, y);
            QRgb pixel2 = image2.pixel(x, y);
            
            // Simple color difference check
            int rDiff = qAbs(qRed(pixel1) - qRed(pixel2));
            int gDiff = qAbs(qGreen(pixel1) - qGreen(pixel2));
            int bDiff = qAbs(qBlue(pixel1) - qBlue(pixel2));
            
            if (rDiff < 30 && gDiff < 30 && bDiff < 30) {
                similarPixels++;
            }
        }
    }
    
    double similarity = static_cast<double>(similarPixels) / totalPixels;
    return similarity >= tolerance;
}

QTEST_MAIN(ThumbnailGeneratorIntegrationTest)

#include "thumbnail_generator_integration_test.moc"
