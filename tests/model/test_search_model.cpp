#include <poppler-qt6.h>
#include <QApplication>
#include <QFontDatabase>
#include <QPainter>
#include <QPdfWriter>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QtTest/QtTest>
#include "../../app/model/SearchModel.h"

/**
 * Core Search Functionality Tests
 * Tests basic search operations using the existing SearchModel
 */
class TestSearchEngineCore : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic search functionality tests
    void testBasicTextSearch();
    void testEmptyQueryHandling();
    void testNonExistentTextSearch();

    // Case sensitivity tests
    void testCaseSensitiveSearch();
    void testCaseInsensitiveSearch();

    // Whole word matching tests
    void testWholeWordMatching();

    // Regex pattern tests
    void testBasicRegexPatterns();

    // Advanced search features
    void testFuzzySearch();
    void testPageRangeSearch();

    // Search result validation tests
    void testSearchResultAccuracy();

private:
    Poppler::Document* m_testDocument;
    SearchModel* m_searchModel;
    QString m_testPdfPath;

    // Test document content
    QStringList m_testTexts;

    // Helper methods
    Poppler::Document* createTestDocument();
    void waitForSearchCompletion();
};

void TestSearchEngineCore::initTestCase() {
    // Initialize test texts for document creation
    m_testTexts = {
        "This is the first page with some sample text. "
        "It contains words like 'search', 'test', and 'document'. "
        "The quick brown fox jumps over the lazy dog. "
        "Case sensitivity testing: UPPERCASE, lowercase, MixedCase.",

        "Second page contains different content. "
        "Here we have regex patterns: email@example.com, phone: 123-456-7890. "
        "Special characters: !@#$%^&*()_+-=[]{}|;':\",./<>? "
        "Numbers and dates: 2023-12-25, version 1.2.3.",

        "Third page for comprehensive testing. "
        "Repeated words: test test TEST Test. "
        "Punctuation tests: word, word; word: word! word? word. "
        "Unicode characters: café, naïve, résumé, Москва."};

    m_testDocument = createTestDocument();
    QVERIFY(m_testDocument != nullptr);
    QCOMPARE(m_testDocument->numPages(), 3);

    m_searchModel = new SearchModel(this);
}

void TestSearchEngineCore::cleanupTestCase() {
    delete m_testDocument;
    if (!m_testPdfPath.isEmpty()) {
        QFile::remove(m_testPdfPath);
    }
}

void TestSearchEngineCore::init() {
    // Reset search model before each test
    m_searchModel->clearResults();
}

void TestSearchEngineCore::cleanup() {
    // Cleanup after each test if needed
}

void TestSearchEngineCore::waitForSearchCompletion() {
    // Wait for search to complete using signal spy
    // SearchModel emits searchFinished(int) not
    // searchFinished(QList<SearchResult>)
    QSignalSpy spy(m_searchModel,
                   QOverload<int>::of(&SearchModel::searchFinished));

    // Give a small delay for immediate completions
    QTest::qWait(100);

    // Only wait for signal if no signal was already received
    if (spy.isEmpty()) {
        spy.wait(2000);  // 2 second timeout (reduced from 5 seconds)
    }
}

void TestSearchEngineCore::testBasicTextSearch() {
    SearchOptions options;

    // Test simple word search
    m_searchModel->startSearch(m_testDocument, "test", options);
    waitForSearchCompletion();
    QList<SearchResult> results = m_searchModel->getResults();

    QVERIFY(!results.isEmpty());
    QVERIFY(results.size() >= 1);  // Should find "test"

    // Verify first result
    QVERIFY(results[0].pageNumber >= 0);
    QVERIFY(results[0].pageNumber < 3);
    QVERIFY(results[0].matchedText.contains("test", Qt::CaseInsensitive));
}

void TestSearchEngineCore::testEmptyQueryHandling() {
    SearchOptions options;

    // Test empty query - should return immediately without emitting signals
    m_searchModel->startSearch(m_testDocument, "", options);

    // Give a small delay for processing
    QTest::qWait(100);

    QList<SearchResult> results = m_searchModel->getResults();
    QVERIFY(results.isEmpty());
}

void TestSearchEngineCore::testNonExistentTextSearch() {
    SearchOptions options;

    // Search for text that doesn't exist
    m_searchModel->startSearch(m_testDocument, "nonexistentword12345", options);
    waitForSearchCompletion();
    QList<SearchResult> results = m_searchModel->getResults();

    QVERIFY(results.isEmpty());
}

void TestSearchEngineCore::testCaseSensitiveSearch() {
    SearchOptions options;
    options.caseSensitive = true;

    // Search for "TEST" (uppercase)
    m_searchModel->startSearch(m_testDocument, "TEST", options);
    waitForSearchCompletion();
    QList<SearchResult> results = m_searchModel->getResults();

    QVERIFY(!results.isEmpty());

    // Verify all results are exact case matches
    for (const SearchResult& result : results) {
        QVERIFY(result.matchedText.contains("TEST"));
    }
}

void TestSearchEngineCore::testCaseInsensitiveSearch() {
    SearchOptions options;
    options.caseSensitive = false;

    // Search for "test" (should find TEST, Test, test)
    m_searchModel->startSearch(m_testDocument, "test", options);
    waitForSearchCompletion();
    QList<SearchResult> results = m_searchModel->getResults();

    QVERIFY(results.size() >= 1);  // Should find at least one match

    // Verify results contain the search term (case insensitive)
    for (const SearchResult& result : results) {
        QVERIFY(result.matchedText.contains("test", Qt::CaseInsensitive));
    }
}

void TestSearchEngineCore::testWholeWordMatching() {
    SearchOptions options;
    options.wholeWords = true;

    // Search for "test" as whole word
    m_searchModel->startSearch(m_testDocument, "test", options);
    waitForSearchCompletion();
    QList<SearchResult> results = m_searchModel->getResults();

    QVERIFY(!results.isEmpty());

    // Verify all results contain the search term
    for (const SearchResult& result : results) {
        QVERIFY(result.matchedText.contains("test", Qt::CaseInsensitive));
    }
}

void TestSearchEngineCore::testBasicRegexPatterns() {
    SearchOptions options;
    options.useRegex = true;

    // Test email pattern
    m_searchModel->startSearch(
        m_testDocument, "[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}",
        options);
    waitForSearchCompletion();
    QList<SearchResult> results = m_searchModel->getResults();

    QVERIFY(!results.isEmpty());
    QVERIFY(results[0].matchedText.contains("@"));
}

void TestSearchEngineCore::testFuzzySearch() {
    SearchOptions options;
    options.fuzzySearch = true;
    options.fuzzyThreshold = 2;

    // Test fuzzy search for "document" (should find variations)
    m_searchModel->startFuzzySearch(m_testDocument, "document", options);
    waitForSearchCompletion();
    QList<SearchResult> results = m_searchModel->getResults();

    QVERIFY(!results.isEmpty());

    // Verify results contain the search term or similar
    for (const SearchResult& result : results) {
        QVERIFY(result.matchedText.contains("document", Qt::CaseInsensitive));
    }
}

void TestSearchEngineCore::testPageRangeSearch() {
    SearchOptions options;

    // Search only on page 1 (0-based indexing)
    m_searchModel->startPageRangeSearch(m_testDocument, "page", 0, 0, options);
    waitForSearchCompletion();
    QList<SearchResult> results = m_searchModel->getResults();

    QVERIFY(!results.isEmpty());

    // Verify all results are from page 0
    for (const SearchResult& result : results) {
        QCOMPARE(result.pageNumber, 0);
    }
}

void TestSearchEngineCore::testSearchResultAccuracy() {
    SearchOptions options;

    // Search for a specific term
    m_searchModel->startSearch(m_testDocument, "quick", options);
    waitForSearchCompletion();
    QList<SearchResult> results = m_searchModel->getResults();

    QVERIFY(!results.isEmpty());

    // Verify result accuracy
    SearchResult result = results[0];
    QVERIFY(result.pageNumber >= 0 && result.pageNumber < 3);
    QVERIFY(result.matchedText.contains("quick", Qt::CaseInsensitive));
    QVERIFY(result.textPosition >= 0);
    QVERIFY(result.textLength > 0);
}

Poppler::Document* TestSearchEngineCore::createTestDocument() {
    // Create a temporary PDF file for testing
    QTemporaryFile tempFile;
    tempFile.setFileTemplate(QDir::tempPath() + "/test_search_XXXXXX.pdf");
    if (!tempFile.open()) {
        return nullptr;
    }

    m_testPdfPath = tempFile.fileName();
    tempFile.close();

    // Create PDF with test content
    QPdfWriter pdfWriter(m_testPdfPath);
    pdfWriter.setPageSize(QPageSize::A4);
    pdfWriter.setResolution(300);

    QPainter painter(&pdfWriter);
    if (!painter.isActive()) {
        return nullptr;
    }

    // Ensure a common Windows font is used to improve text extractability by
    // Poppler Ignore return value; font may already be available.
    QFontDatabase::addApplicationFont("C:/Windows/Fonts/arial.ttf");
    QFont font("Arial");
    font.setPointSize(12);
    painter.setFont(font);

    for (int page = 0; page < m_testTexts.size(); ++page) {
        if (page > 0) {
            pdfWriter.newPage();
        }

        QRect textRect(100, 100, 400, 600);
        painter.drawText(textRect, Qt::TextWordWrap, m_testTexts[page]);

        // Add page number
        painter.drawText(100, 50, QString("Page %1").arg(page + 1));
    }

    painter.end();

    // Load the created PDF
    auto doc = Poppler::Document::load(m_testPdfPath);
    if (doc && doc->numPages() > 0) {
        return doc.release();
    }
    return nullptr;
}

QTEST_MAIN(TestSearchEngineCore)
#include "test_search_model.moc"
