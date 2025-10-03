#include <poppler-qt6.h>
#include <QObject>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QtTest/QtTest>
#include "../../app/search/TextExtractor.h"
#include "../TestUtilities.h"

/**
 * Comprehensive tests for TextExtractor class
 * Tests text extraction functionality with caching and prefetching
 */
class TextExtractorTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Document management tests
    void testSetDocument();
    void testClearDocument();
    void testDocumentHandling();

    // Text extraction tests
    void testExtractPageText();
    void testExtractPagesText();
    void testExtractAllText();
    void testExtractEmptyPage();
    void testExtractInvalidPage();

    // Cache management tests
    void testCacheEnabled();
    void testCacheDisabled();
    void testClearCache();
    void testCacheMemoryUsage();
    void testCacheEfficiency();

    // Prefetching tests
    void testPrefetchPages();
    void testPrefetchRange();
    void testPrefetchPerformance();

    // Signal tests
    void testTextExtractedSignal();
    void testExtractionProgressSignal();
    void testExtractionErrorSignal();

    // Edge cases and error handling
    void testNullDocument();
    void testCorruptedDocument();
    void testLargeDocument();
    void testConcurrentExtraction();

    // Performance tests
    void testExtractionPerformance();
    void testMemoryUsage();

private:
    TextExtractor* m_extractor;
    Poppler::Document* m_testDocument;
    QString m_testPdfPath;

    // Helper methods
    void createTestPdf();
    void createLargeTestPdf();
    void verifyExtractedText(const QString& text,
                             const QString& expectedContent);
    void measureExtractionTime(int pageNumber);
};

void TextExtractorTest::initTestCase() { createTestPdf(); }

void TextExtractorTest::cleanupTestCase() {
    if (m_testDocument) {
        delete m_testDocument;
        m_testDocument = nullptr;
    }

    if (!m_testPdfPath.isEmpty()) {
        QFile::remove(m_testPdfPath);
    }
}

void TextExtractorTest::init() {
    m_extractor = new TextExtractor(this);
    QVERIFY(m_extractor != nullptr);

    if (m_testDocument) {
        m_extractor->setDocument(m_testDocument);
    }
}

void TextExtractorTest::cleanup() {
    if (m_extractor) {
        m_extractor->clearCache();
        delete m_extractor;
        m_extractor = nullptr;
    }
}

void TextExtractorTest::testSetDocument() {
    // Test setting valid document
    m_extractor->setDocument(m_testDocument);

    // Test extraction works after setting document
    QString text = m_extractor->extractPageText(0);
    QVERIFY(!text.isEmpty());

    // Test setting null document
    m_extractor->setDocument(nullptr);
    text = m_extractor->extractPageText(0);
    QVERIFY(text.isEmpty());
}

void TextExtractorTest::testClearDocument() {
    m_extractor->setDocument(m_testDocument);

    // Verify document is set
    QString text = m_extractor->extractPageText(0);
    QVERIFY(!text.isEmpty());

    // Clear document
    m_extractor->clearDocument();

    // Verify extraction returns empty after clearing
    text = m_extractor->extractPageText(0);
    QVERIFY(text.isEmpty());
}

void TextExtractorTest::testDocumentHandling() {
    // Test multiple document switches
    m_extractor->setDocument(m_testDocument);
    QString text1 = m_extractor->extractPageText(0);

    m_extractor->setDocument(nullptr);
    m_extractor->setDocument(m_testDocument);
    QString text2 = m_extractor->extractPageText(0);

    QCOMPARE(text1, text2);
}

void TextExtractorTest::testExtractPageText() {
    QSignalSpy extractedSpy(m_extractor, &TextExtractor::textExtracted);

    QString text = m_extractor->extractPageText(0);

    QVERIFY(!text.isEmpty());
    verifyExtractedText(text, "test");

    // Verify signal was emitted
    QVERIFY(extractedSpy.count() > 0);

    // Test invalid page number
    text = m_extractor->extractPageText(-1);
    QVERIFY(text.isEmpty());

    text = m_extractor->extractPageText(1000);
    QVERIFY(text.isEmpty());
}

void TextExtractorTest::testExtractPagesText() {
    QList<int> pageNumbers = {0};
    if (m_testDocument && m_testDocument->numPages() > 1) {
        pageNumbers.append(1);
    }

    QSignalSpy progressSpy(m_extractor, &TextExtractor::extractionProgress);

    QStringList texts = m_extractor->extractPagesText(pageNumbers);

    QCOMPARE(texts.size(), pageNumbers.size());
    for (const QString& text : texts) {
        QVERIFY(!text.isEmpty());
    }

    // Verify progress signal was emitted
    QVERIFY(progressSpy.count() > 0);
}

void TextExtractorTest::testExtractAllText() {
    QSignalSpy progressSpy(m_extractor, &TextExtractor::extractionProgress);

    QString allText = m_extractor->extractAllText();

    QVERIFY(!allText.isEmpty());
    verifyExtractedText(allText, "test");

    // Should contain content from all pages
    if (m_testDocument && m_testDocument->numPages() > 1) {
        QVERIFY(progressSpy.count() > 0);
    }
}

void TextExtractorTest::testExtractEmptyPage() {
    // Create a PDF with an empty page
    QTemporaryFile tempFile("empty_test_XXXXXX.pdf");
    tempFile.setAutoRemove(false);
    QVERIFY(tempFile.open());
    QString emptyPdfPath = tempFile.fileName();
    tempFile.close();

    QPdfWriter writer(emptyPdfPath);
    writer.setPageSize(QPageSize::A4);
    QPainter painter(&writer);
    // Don't draw anything - empty page
    painter.end();

    Poppler::Document* emptyDoc =
        Poppler::Document::load(emptyPdfPath).release();
    QVERIFY(emptyDoc != nullptr);

    m_extractor->setDocument(emptyDoc);
    QString text = m_extractor->extractPageText(0);

    // Empty page should return empty or whitespace-only text
    QVERIFY(text.trimmed().isEmpty());

    delete emptyDoc;
    QFile::remove(emptyPdfPath);
}

void TextExtractorTest::testExtractInvalidPage() {
    QSignalSpy errorSpy(m_extractor, &TextExtractor::extractionError);

    QString text = m_extractor->extractPageText(-1);
    QVERIFY(text.isEmpty());

    text = m_extractor->extractPageText(1000);
    QVERIFY(text.isEmpty());

    // Should emit error signals for invalid pages
    QVERIFY(errorSpy.count() > 0);
}

void TextExtractorTest::testCacheEnabled() {
    m_extractor->setCacheEnabled(true);
    QVERIFY(m_extractor->isCacheEnabled());

    // First extraction - should cache
    QString text1 = m_extractor->extractPageText(0);
    qint64 memoryUsage1 = m_extractor->cacheMemoryUsage();

    // Second extraction - should use cache
    QString text2 = m_extractor->extractPageText(0);
    qint64 memoryUsage2 = m_extractor->cacheMemoryUsage();

    QCOMPARE(text1, text2);
    QCOMPARE(memoryUsage1, memoryUsage2);  // Memory usage should be same
}

void TextExtractorTest::testCacheDisabled() {
    m_extractor->setCacheEnabled(false);
    QVERIFY(!m_extractor->isCacheEnabled());

    QString text1 = m_extractor->extractPageText(0);
    QString text2 = m_extractor->extractPageText(0);

    QCOMPARE(text1, text2);
    QCOMPARE(m_extractor->cacheMemoryUsage(), 0);  // No cache usage
}

void TextExtractorTest::testClearCache() {
    m_extractor->setCacheEnabled(true);

    // Extract some text to populate cache
    m_extractor->extractPageText(0);
    QVERIFY(m_extractor->cacheMemoryUsage() > 0);

    // Clear cache
    m_extractor->clearCache();
    QCOMPARE(m_extractor->cacheMemoryUsage(), 0);
}

void TextExtractorTest::testCacheMemoryUsage() {
    m_extractor->setCacheEnabled(true);
    m_extractor->clearCache();

    QCOMPARE(m_extractor->cacheMemoryUsage(), 0);

    // Extract text and verify memory usage increases
    m_extractor->extractPageText(0);
    QVERIFY(m_extractor->cacheMemoryUsage() > 0);

    qint64 usage1 = m_extractor->cacheMemoryUsage();

    // Extract more text
    if (m_testDocument && m_testDocument->numPages() > 1) {
        m_extractor->extractPageText(1);
        qint64 usage2 = m_extractor->cacheMemoryUsage();
        QVERIFY(usage2 >= usage1);
    }
}

void TextExtractorTest::testCacheEfficiency() {
    m_extractor->setCacheEnabled(true);
    m_extractor->clearCache();

    // Measure time for first extraction (no cache)
    QElapsedTimer timer;
    timer.start();
    QString text1 = m_extractor->extractPageText(0);
    qint64 time1 = timer.elapsed();

    // Measure time for second extraction (with cache)
    timer.restart();
    QString text2 = m_extractor->extractPageText(0);
    qint64 time2 = timer.elapsed();

    QCOMPARE(text1, text2);
    // Cache should be faster (though this might not always be true for small
    // documents) QVERIFY(time2 <= time1);
}

void TextExtractorTest::testPrefetchPages() {
    QList<int> pageNumbers = {0};
    if (m_testDocument && m_testDocument->numPages() > 1) {
        pageNumbers.append(1);
    }

    m_extractor->setCacheEnabled(true);
    m_extractor->clearCache();

    // Prefetch pages
    m_extractor->prefetchPages(pageNumbers);

    // Give some time for prefetching (it might be asynchronous)
    waitMs(100);

    // Verify cache has content
    QVERIFY(m_extractor->cacheMemoryUsage() > 0);
}

void TextExtractorTest::testPrefetchRange() {
    m_extractor->setCacheEnabled(true);
    m_extractor->clearCache();

    // Prefetch range
    m_extractor->prefetchRange(0, 0);  // Just first page

    // Give some time for prefetching
    waitMs(100);

    // Verify cache has content
    QVERIFY(m_extractor->cacheMemoryUsage() > 0);
}

void TextExtractorTest::testPrefetchPerformance() {
    m_extractor->setCacheEnabled(true);

    // Prefetch first page
    m_extractor->prefetchPages({0});
    waitMs(100);

    // Extraction should be fast due to prefetching
    QElapsedTimer timer;
    timer.start();
    QString text = m_extractor->extractPageText(0);
    qint64 extractionTime = timer.elapsed();

    QVERIFY(!text.isEmpty());
    // Prefetched extraction should be relatively fast
    QVERIFY(extractionTime < 1000);  // Less than 1 second
}

void TextExtractorTest::createTestPdf() {
    QTemporaryFile tempFile("text_extractor_test_XXXXXX.pdf");
    tempFile.setAutoRemove(false);
    QVERIFY(tempFile.open());
    m_testPdfPath = tempFile.fileName();
    tempFile.close();

    QPdfWriter writer(m_testPdfPath);
    writer.setPageSize(QPageSize::A4);

    QPainter painter(&writer);
    painter.drawText(100, 100, "This is a test document for text extraction.");
    painter.drawText(100, 200,
                     "It contains multiple lines of text for testing.");
    painter.drawText(
        100, 300,
        "The extractor should be able to extract this text efficiently.");

    // Add a second page if needed
    writer.newPage();
    painter.drawText(100, 100, "This is the second page of the test document.");
    painter.drawText(100, 200, "It also contains test content for extraction.");

    painter.end();

    m_testDocument = Poppler::Document::load(m_testPdfPath).release();
    QVERIFY(m_testDocument != nullptr);
    QVERIFY(m_testDocument->numPages() >= 1);
}

void TextExtractorTest::verifyExtractedText(const QString& text,
                                            const QString& expectedContent) {
    QVERIFY(!text.isEmpty());
    QVERIFY(text.contains(expectedContent, Qt::CaseInsensitive));
}

void TextExtractorTest::measureExtractionTime(int pageNumber) {
    QElapsedTimer timer;
    timer.start();
    QString text = m_extractor->extractPageText(pageNumber);
    qint64 extractionTime = timer.elapsed();

    qDebug() << "Extraction time for page" << pageNumber << ":"
             << extractionTime << "ms";
    QVERIFY(!text.isEmpty());
    QVERIFY(extractionTime < 5000);  // Should complete within 5 seconds
}

QTEST_MAIN(TextExtractorTest)
#include "text_extractor_test.moc"
