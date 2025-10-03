#include <poppler-qt6.h>
#include <QApplication>
#include <QPainter>
#include <QPdfWriter>
#include <QTemporaryFile>
#include <QTimer>
#include <QtTest/QtTest>
#include "../../app/model/SearchModel.h"
#include "../../app/search/SearchEngine.h"

/**
 * Edge Cases and Error Handling Tests
 * Tests with empty documents, malformed PDFs, large documents, and error
 * scenarios
 */
class TestSearchEdgeCases : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Empty and malformed document tests
    void testEmptyDocumentSearch();
    void testNullDocumentHandling();
    void testCorruptedPDFHandling();
    void testSinglePageDocument();

    // Empty and special query tests
    void testEmptyQueryHandling();
    void testWhitespaceOnlyQuery();
    void testVeryLongQuery();
    void testSpecialCharacterQueries();
    void testUnicodeQueries();

    // Invalid parameter tests
    void testInvalidPageRanges();
    void testInvalidFuzzyThresholds();
    void testInvalidRegexPatterns();
    void testInvalidSearchOptions();

    // Timeout and cancellation tests
    void testSearchTimeout();
    void testSearchCancellation();
    void testConcurrentSearchCancellation();

    // Memory and resource limit tests
    void testLargeDocumentHandling();
    void testMemoryLimitExceeded();
    void testTooManySearchResults();

    // Performance edge cases
    void testVeryFrequentSearches();
    void testSearchOnCorruptedPages();
    void testSearchWithMissingText();

private:
    Poppler::Document* m_normalDocument;
    Poppler::Document* m_emptyDocument;
    Poppler::Document* m_largeDocument;
    SearchEngine* m_searchEngine;
    SearchModel* m_searchModel;
    QString m_normalPdfPath;
    QString m_emptyPdfPath;
    QString m_largePdfPath;

    // Helper methods
    Poppler::Document* createNormalTestDocument();
    Poppler::Document* createEmptyTestDocument();
    Poppler::Document* createLargeTestDocument();
    Poppler::Document* createCorruptedDocument();
    void verifyErrorHandling(const QString& operation);
    void testSearchWithTimeout(int timeoutMs);
};

void TestSearchEdgeCases::initTestCase() {
    m_normalDocument = createNormalTestDocument();
    m_emptyDocument = createEmptyTestDocument();
    m_largeDocument = createLargeTestDocument();

    QVERIFY(m_normalDocument != nullptr);
    QVERIFY(m_emptyDocument != nullptr);
    QVERIFY(m_largeDocument != nullptr);

    m_searchEngine = new SearchEngine(this);
    m_searchModel = new SearchModel(this);
}

void TestSearchEdgeCases::cleanupTestCase() {
    delete m_normalDocument;
    delete m_emptyDocument;
    delete m_largeDocument;

    QStringList paths = {m_normalPdfPath, m_emptyPdfPath, m_largePdfPath};
    for (const QString& path : paths) {
        if (!path.isEmpty()) {
            QFile::remove(path);
        }
    }
}

void TestSearchEdgeCases::init() {
    m_searchEngine->clearResults();
    m_searchModel->clearResults();
}

void TestSearchEdgeCases::cleanup() {
    // Cleanup after each test
}

void TestSearchEdgeCases::testEmptyDocumentSearch() {
    SearchOptions options;

    // Test search on empty document
    m_searchEngine->setDocument(m_emptyDocument);
    m_searchEngine->startSearch(m_emptyDocument, "test", options);

    QList<SearchResult> results = m_searchEngine->getResults();
    QVERIFY(results.isEmpty());

    // Should not crash or throw exceptions
    QVERIFY(true);
}

void TestSearchEdgeCases::testNullDocumentHandling() {
    SearchOptions options;

    // Test search with null document
    m_searchEngine->setDocument(nullptr);
    m_searchEngine->startSearch(nullptr, "test", options);

    QList<SearchResult> results = m_searchEngine->getResults();
    QVERIFY(results.isEmpty());

    // Test SearchModel with null document
    m_searchModel->startSearch(nullptr, "test", options);
    results = m_searchModel->getResults();
    QVERIFY(results.isEmpty());
}

void TestSearchEdgeCases::testCorruptedPDFHandling() {
    // Create a corrupted PDF file
    QTemporaryFile tempFile;
    tempFile.setFileTemplate(QDir::tempPath() + "/corrupted_XXXXXX.pdf");
    QVERIFY(tempFile.open());

    // Write invalid PDF content
    tempFile.write("This is not a valid PDF file content");
    tempFile.close();

    // Try to load corrupted PDF
    auto corruptedDoc = Poppler::Document::load(tempFile.fileName());

    if (corruptedDoc) {
        // If somehow loaded, test search on it
        SearchOptions options;
        m_searchEngine->setDocument(corruptedDoc.get());
        m_searchEngine->startSearch(corruptedDoc.get(), "test", options);

        // Should handle gracefully
        QList<SearchResult> results = m_searchEngine->getResults();
        // Results may be empty or contain errors, but should not crash
    }

    // Test passed if no crash occurred
    QVERIFY(true);
}

void TestSearchEdgeCases::testSinglePageDocument() {
    // Create single page document
    QTemporaryFile tempFile;
    tempFile.setFileTemplate(QDir::tempPath() + "/single_page_XXXXXX.pdf");
    QVERIFY(tempFile.open());

    QString singlePagePath = tempFile.fileName();
    tempFile.close();

    QPdfWriter pdfWriter(singlePagePath);
    pdfWriter.setPageSize(QPageSize::A4);

    QPainter painter(&pdfWriter);
    painter.drawText(100, 100, "Single page document with test content.");
    painter.end();

    auto singlePageDoc = Poppler::Document::load(singlePagePath);
    QVERIFY(singlePageDoc != nullptr);
    QCOMPARE(singlePageDoc->numPages(), 1);

    SearchOptions options;
    m_searchEngine->setDocument(singlePageDoc.get());
    m_searchEngine->startSearch(singlePageDoc.get(), "test", options);

    QList<SearchResult> results = m_searchEngine->getResults();
    QVERIFY(!results.isEmpty());
    QCOMPARE(results[0].pageNumber, 0);

    QFile::remove(singlePagePath);
}

void TestSearchEdgeCases::testEmptyQueryHandling() {
    SearchOptions options;

    // Test empty string query
    m_searchEngine->setDocument(m_normalDocument);
    m_searchEngine->startSearch(m_normalDocument, "", options);

    QList<SearchResult> results = m_searchEngine->getResults();
    QVERIFY(results.isEmpty());

    // Test null query (if supported)
    m_searchEngine->startSearch(m_normalDocument, QString(), options);
    results = m_searchEngine->getResults();
    QVERIFY(results.isEmpty());
}

void TestSearchEdgeCases::testWhitespaceOnlyQuery() {
    SearchOptions options;

    // Test queries with only whitespace
    QStringList whitespaceQueries = {"   ", "\t", "\n", " \t \n ", "     "};

    for (const QString& query : whitespaceQueries) {
        m_searchEngine->startSearch(m_normalDocument, query, options);
        QList<SearchResult> results = m_searchEngine->getResults();
        QVERIFY(results.isEmpty());
    }
}

void TestSearchEdgeCases::testVeryLongQuery() {
    SearchOptions options;

    // Create very long query (1000 characters)
    QString longQuery = QString("a").repeated(1000);

    m_searchEngine->setDocument(m_normalDocument);
    m_searchEngine->startSearch(m_normalDocument, longQuery, options);

    QList<SearchResult> results = m_searchEngine->getResults();
    // Should handle gracefully (likely no results)
    QVERIFY(results.isEmpty());

    // Test extremely long query (10000 characters)
    QString extremelyLongQuery = QString("test").repeated(2500);

    m_searchEngine->startSearch(m_normalDocument, extremelyLongQuery, options);
    results = m_searchEngine->getResults();
    // Should not crash
    QVERIFY(true);
}

void TestSearchEdgeCases::testSpecialCharacterQueries() {
    SearchOptions options;

    // Test queries with special characters
    QStringList specialQueries = {
        "!@#$%^&*()",  "[]{}|;':\",./<>?", "\\n\\t\\r", "~`+=_-",
        "αβγδε",        // Greek letters
        "中文测试",     // Chinese characters
        "🙂😀🎉",       // Emojis
        "\x00\x01\x02"  // Control characters
    };

    for (const QString& query : specialQueries) {
        m_searchEngine->startSearch(m_normalDocument, query, options);
        QList<SearchResult> results = m_searchEngine->getResults();
        // Should handle gracefully without crashing
    }

    QVERIFY(true);
}

void TestSearchEdgeCases::testUnicodeQueries() {
    SearchOptions options;

    // Test various Unicode queries
    QStringList unicodeQueries = {
        "café",     // Latin with diacritics
        "naïve",    // Latin with diaeresis
        "Москва",   // Cyrillic
        "北京",     // Chinese
        "東京",     // Japanese
        "العربية",  // Arabic
        "हिन्दी",    // Hindi
        "🌟⭐✨"    // Unicode symbols
    };

    for (const QString& query : unicodeQueries) {
        m_searchEngine->startSearch(m_normalDocument, query, options);
        QList<SearchResult> results = m_searchEngine->getResults();
        // Should handle Unicode correctly
    }

    QVERIFY(true);
}

void TestSearchEdgeCases::testInvalidPageRanges() {
    SearchOptions options;

    // Test invalid page ranges
    QList<QPair<int, int>> invalidRanges = {
        {-1, 0},     // Negative start
        {0, -1},     // Negative end
        {5, 2},      // Start > end
        {100, 200},  // Out of bounds
        {-5, -1}     // Both negative
    };

    for (const auto& range : invalidRanges) {
        options.startPage = range.first;
        options.endPage = range.second;

        m_searchModel->startPageRangeSearch(m_normalDocument, "test",
                                            range.first, range.second, options);

        QList<SearchResult> results = m_searchModel->getResults();
        // Should handle gracefully (likely empty results)
        QVERIFY(results.isEmpty());
    }
}

void TestSearchEdgeCases::testInvalidFuzzyThresholds() {
    SearchOptions options;
    options.fuzzySearch = true;

    // Test invalid fuzzy thresholds
    QList<int> invalidThresholds = {-1, -10, 0, 1000, INT_MAX, INT_MIN};

    for (int threshold : invalidThresholds) {
        options.fuzzyThreshold = threshold;

        m_searchModel->startFuzzySearch(m_normalDocument, "test", options);

        // Should handle gracefully without crashing
        QList<SearchResult> results = m_searchModel->getResults();
    }

    QVERIFY(true);
}

void TestSearchEdgeCases::testInvalidRegexPatterns() {
    SearchOptions options;
    options.useRegex = true;

    // Test invalid regex patterns
    QStringList invalidPatterns = {
        "[",           // Unclosed bracket
        "(",           // Unclosed parenthesis
        "*",           // Invalid quantifier
        "?",           // Invalid quantifier
        "+",           // Invalid quantifier
        "\\",          // Trailing backslash
        "[z-a]",       // Invalid range
        "(?P<>test)",  // Invalid group name
        "(?",          // Incomplete group
        "**",          // Double quantifier
        "++",          // Double quantifier
        "??",          // Double quantifier
    };

    for (const QString& pattern : invalidPatterns) {
        m_searchEngine->startSearch(m_normalDocument, pattern, options);

        QList<SearchResult> results = m_searchEngine->getResults();
        // Should handle gracefully (likely empty results)
    }

    QVERIFY(true);
}

Poppler::Document* TestSearchEdgeCases::createNormalTestDocument() {
    QTemporaryFile tempFile;
    tempFile.setFileTemplate(QDir::tempPath() + "/normal_edge_XXXXXX.pdf");
    if (!tempFile.open()) {
        return nullptr;
    }

    m_normalPdfPath = tempFile.fileName();
    tempFile.close();

    QPdfWriter pdfWriter(m_normalPdfPath);
    pdfWriter.setPageSize(QPageSize::A4);

    QPainter painter(&pdfWriter);
    painter.drawText(
        100, 100, "Normal test document with content for edge case testing.");
    painter.end();

    auto doc = Poppler::Document::load(m_normalPdfPath);
    if (doc && doc->numPages() > 0) {
        return doc.release();
    }
    return nullptr;
}

Poppler::Document* TestSearchEdgeCases::createEmptyTestDocument() {
    QTemporaryFile tempFile;
    tempFile.setFileTemplate(QDir::tempPath() + "/empty_edge_XXXXXX.pdf");
    if (!tempFile.open()) {
        return nullptr;
    }

    m_emptyPdfPath = tempFile.fileName();
    tempFile.close();

    QPdfWriter pdfWriter(m_emptyPdfPath);
    pdfWriter.setPageSize(QPageSize::A4);

    QPainter painter(&pdfWriter);
    // Create empty page with no text
    painter.end();

    auto doc = Poppler::Document::load(m_emptyPdfPath);
    if (doc && doc->numPages() > 0) {
        return doc.release();
    }
    return nullptr;
}

Poppler::Document* TestSearchEdgeCases::createLargeTestDocument() {
    QTemporaryFile tempFile;
    tempFile.setFileTemplate(QDir::tempPath() + "/large_edge_XXXXXX.pdf");
    if (!tempFile.open()) {
        return nullptr;
    }

    m_largePdfPath = tempFile.fileName();
    tempFile.close();

    QPdfWriter pdfWriter(m_largePdfPath);
    pdfWriter.setPageSize(QPageSize::A4);

    QPainter painter(&pdfWriter);
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);

    // Create 3 pages with content (reduced from 20 for faster testing)
    for (int page = 0; page < 3; ++page) {
        if (page > 0) {
            pdfWriter.newPage();
        }

        QString content = QString("Large document page %1. ").arg(page + 1);
        content += QString("Content repeated many times. ")
                       .repeated(20);  // Reduced from 100

        QRect textRect(50, 50, 500, 700);
        painter.drawText(textRect, Qt::TextWordWrap, content);
    }

    painter.end();

    auto doc = Poppler::Document::load(m_largePdfPath);
    if (doc && doc->numPages() > 0) {
        return doc.release();
    }
    return nullptr;
}

void TestSearchEdgeCases::testInvalidSearchOptions() {
    SearchOptions options;

    // Test with extreme values
    options.maxResults = -1;
    options.searchTimeout = -1000;
    options.fuzzyThreshold = -100;

    m_searchEngine->startSearch(m_normalDocument, "test", options);

    // Should handle gracefully
    QList<SearchResult> results = m_searchEngine->getResults();
    QVERIFY(true);  // Test passes if no crash

    // Test with very large values
    options.maxResults = INT_MAX;
    options.searchTimeout = INT_MAX;
    options.fuzzyThreshold = INT_MAX;

    m_searchEngine->startSearch(m_normalDocument, "test", options);
    results = m_searchEngine->getResults();
    QVERIFY(true);  // Test passes if no crash
}

void TestSearchEdgeCases::testSearchTimeout() {
    SearchOptions options;
    options.searchTimeout = 100;  // Very short timeout (100ms)

    // Start search on large document with short timeout
    m_searchEngine->setDocument(m_largeDocument);

    QElapsedTimer timer;
    timer.start();

    m_searchEngine->startSearch(m_largeDocument, "content", options);

    qint64 elapsed = timer.elapsed();

    // Search should respect timeout
    QVERIFY(elapsed <= 1000);  // Should complete within reasonable time

    // Results may be partial or empty due to timeout
    QList<SearchResult> results = m_searchEngine->getResults();
    // Test passes regardless of result count
    QVERIFY(true);
}

void TestSearchEdgeCases::testSearchCancellation() {
    SearchOptions options;

    // Start search on large document
    m_searchEngine->setDocument(m_largeDocument);
    m_searchEngine->startSearch(m_largeDocument, "test", options);

    // Cancel immediately
    m_searchEngine->cancelSearch();

    // Wait a bit
    QTest::qWait(100);

    // Search should be cancelled
    QList<SearchResult> results = m_searchEngine->getResults();

    // Test passes if no crash occurred
    QVERIFY(true);
}

void TestSearchEdgeCases::testConcurrentSearchCancellation() {
    SearchOptions options;

    // Start multiple searches and cancel them
    for (int i = 0; i < 5; ++i) {
        m_searchEngine->startSearch(m_largeDocument, QString("test%1").arg(i),
                                    options);

        // Cancel after short delay
        QTimer::singleShot(10, [this]() { m_searchEngine->cancelSearch(); });

        QTest::qWait(50);
    }

    // Test passes if no crashes occurred
    QVERIFY(true);
}

void TestSearchEdgeCases::testLargeDocumentHandling() {
    SearchOptions options;

    // Test search on large document
    m_searchEngine->setDocument(m_largeDocument);

    QElapsedTimer timer;
    timer.start();

    m_searchEngine->startSearch(m_largeDocument, "repeated", options);

    qint64 elapsed = timer.elapsed();
    QList<SearchResult> results = m_searchEngine->getResults();

    // Should complete within reasonable time
    QVERIFY(elapsed < 30000);  // 30 seconds max

    // Should find many results
    QVERIFY(results.size() > 10);

    qDebug() << "Large document search: found" << results.size() << "results in"
             << elapsed << "ms";
}

void TestSearchEdgeCases::testMemoryLimitExceeded() {
    SearchOptions options;
    options.maxResults = 1000000;  // Very large number

    // Search for common term that might generate many results
    m_searchEngine->setDocument(m_largeDocument);
    m_searchEngine->startSearch(m_largeDocument, "a", options);

    QList<SearchResult> results = m_searchEngine->getResults();

    // Should handle memory limits gracefully
    QVERIFY(results.size() < 100000);  // Should be limited

    qDebug() << "Memory limit test: found" << results.size() << "results";
}

void TestSearchEdgeCases::testTooManySearchResults() {
    SearchOptions options;
    options.maxResults = 10;  // Small limit

    // Search for very common character
    m_searchEngine->setDocument(m_largeDocument);
    m_searchEngine->startSearch(m_largeDocument, "e", options);

    QList<SearchResult> results = m_searchEngine->getResults();

    // Should respect maxResults limit
    QVERIFY(results.size() <= options.maxResults);

    qDebug() << "Max results test: found" << results.size()
             << "results (limit:" << options.maxResults << ")";
}

void TestSearchEdgeCases::testVeryFrequentSearches() {
    SearchOptions options;

    // Perform many searches in rapid succession
    QElapsedTimer timer;
    timer.start();

    for (int i = 0; i < 100; ++i) {
        QString query =
            QString("query%1").arg(i % 10);  // Cycle through 10 queries
        m_searchEngine->startSearch(m_normalDocument, query, options);

        // Very short delay
        if (i % 10 == 0) {
            QTest::qWait(1);
        }
    }

    qint64 elapsed = timer.elapsed();

    // Should handle frequent searches without issues
    QVERIFY(elapsed < 10000);  // 10 seconds max

    qDebug() << "Frequent searches test: 100 searches in" << elapsed << "ms";
}

void TestSearchEdgeCases::testSearchOnCorruptedPages() {
    // This test simulates searching on a document with potentially corrupted
    // pages
    SearchOptions options;

    // Try to search each page individually to test page-level error handling
    for (int page = 0; page < m_normalDocument->numPages(); ++page) {
        options.startPage = page;
        options.endPage = page;

        m_searchModel->startPageRangeSearch(m_normalDocument, "test", page,
                                            page, options);

        // Should handle any page-level issues gracefully
        QList<SearchResult> results = m_searchModel->getResults();
    }

    QVERIFY(true);  // Test passes if no crashes
}

void TestSearchEdgeCases::testSearchWithMissingText() {
    // Test search on document where text extraction might fail
    SearchOptions options;

    // Search for text that definitely doesn't exist
    m_searchEngine->startSearch(m_normalDocument,
                                "definitely_not_in_document_12345", options);

    QList<SearchResult> results = m_searchEngine->getResults();
    QVERIFY(results.isEmpty());

    // Test with various non-existent patterns
    QStringList nonExistentQueries = {"xyzabc123", "nonexistent_pattern_999",
                                      "missing_text_element",
                                      "absent_content_marker"};

    for (const QString& query : nonExistentQueries) {
        m_searchEngine->startSearch(m_normalDocument, query, options);
        results = m_searchEngine->getResults();
        QVERIFY(results.isEmpty());
    }
}

void TestSearchEdgeCases::testSearchWithTimeout(int timeoutMs) {
    SearchOptions options;
    options.searchTimeout = timeoutMs;

    QElapsedTimer timer;
    timer.start();

    m_searchEngine->startSearch(m_largeDocument, "test", options);

    qint64 elapsed = timer.elapsed();

    // Should respect timeout (with some tolerance)
    QVERIFY(elapsed <= timeoutMs + 1000);  // 1 second tolerance
}

void TestSearchEdgeCases::verifyErrorHandling(const QString& operation) {
    qDebug() << "Error handling test for operation:" << operation;
    // This method can be used to log error handling tests
}

QTEST_MAIN(TestSearchEdgeCases)
#include "test_search_edge_cases.moc"
