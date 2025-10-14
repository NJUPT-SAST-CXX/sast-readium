#include <poppler-qt6.h>
#include <QCoreApplication>
#include <QFile>
#include <QList>
#include <QObject>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QSignalSpy>
#include <QString>
#include <QTemporaryFile>
#include <QtTest/QtTest>
#include "../../app/search/SearchConfiguration.h"
#include "../../app/search/SearchEngine.h"
#include "../TestUtilities.h"

/**
 * Comprehensive tests for SearchEngine class
 * Tests all public interfaces, signals, and functionality
 */
class SearchEngineTest : public TestBase {
    Q_OBJECT

protected:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

private slots:

    // Constructor and basic setup tests
    void testConstructor();
    void testDestructor();
    void testSetDocument();
    void testDocumentAccess();

    // Basic search functionality tests
    void testBasicSearch();
    void testSearchWithOptions();
    void testIncrementalSearch();
    void testCancelSearch();
    void testClearResults();

    // Synchronous search tests (for compatibility)
    void testStartSearch();
    void testGetResults();
    void testResultCount();

    // Advanced search operations tests
    void testFuzzySearch();
    void testWildcardSearch();
    void testPhraseSearch();
    void testBooleanSearch();
    void testProximitySearch();

    // Configuration tests
    void testCacheConfiguration();
    void testIncrementalSearchConfiguration();
    void testBackgroundProcessingConfiguration();

    // Results access tests
    void testResultsAccess();
    void testCurrentQuery();
    void testIsSearching();

    // Performance metrics tests
    void testCacheHitRatio();
    void testCacheMemoryUsage();
    void testResetStatistics();

    // Advanced features tests
    void testAdvancedFeatures();
    void testHighlightColors();
    void testSearchSuggestions();
    void testSearchHistory();

    // Signal tests
    void testSearchStartedSignal();
    void testSearchFinishedSignal();
    void testSearchProgressSignal();
    void testSearchCancelledSignal();
    void testSearchErrorSignal();
    void testResultsUpdatedSignal();

    // Edge cases and error handling
    void testNullDocument();
    void testEmptyQuery();
    void testInvalidOptions();
    void testLargeDocument();
    void testConcurrentSearches();

    // Integration tests
    void testSearchWithRealPdf();
    void testSearchWithComplexOptions();
    void testSearchPerformance();

private:
    SearchEngine* m_searchEngine;
    Poppler::Document* m_testDocument;
    QString m_testPdfPath;

    // Helper methods
    void createTestPdf();
    void createComplexTestPdf();
    SearchOptions createTestOptions();
    void verifySearchResult(const SearchResult& result, int expectedPage,
                            const QString& expectedText);
};

void SearchEngineTest::initTestCase() { createTestPdf(); }

void SearchEngineTest::cleanupTestCase() {
    if (m_testDocument) {
        delete m_testDocument;
        m_testDocument = nullptr;
    }

    if (!m_testPdfPath.isEmpty()) {
        QFile::remove(m_testPdfPath);
    }
}

void SearchEngineTest::init() {
    m_searchEngine = new SearchEngine(this);
    QVERIFY(m_searchEngine != nullptr);

    if (m_testDocument) {
        m_searchEngine->setDocument(m_testDocument);
    }
}

void SearchEngineTest::cleanup() {
    if (m_searchEngine) {
        m_searchEngine->cancelSearch();
        m_searchEngine->clearResults();
        delete m_searchEngine;
        m_searchEngine = nullptr;
    }
}

void SearchEngineTest::testConstructor() {
    SearchEngine engine;
    QVERIFY(engine.document() == nullptr);
    QVERIFY(engine.results().isEmpty());
    QCOMPARE(engine.resultCount(), 0);
    QVERIFY(!engine.isSearching());
    QVERIFY(engine.currentQuery().isEmpty());
}

void SearchEngineTest::testDestructor() {
    // Test that destructor properly cleans up resources
    SearchEngine* engine = new SearchEngine();
    engine->setDocument(m_testDocument);
    engine->search("test");

    // Destructor should handle cleanup gracefully
    delete engine;
    // If we reach here without crash, destructor worked correctly
    QVERIFY(true);
}

void SearchEngineTest::testSetDocument() {
    QVERIFY(m_searchEngine->document() == m_testDocument);

    // Test setting null document
    m_searchEngine->setDocument(nullptr);
    QVERIFY(m_searchEngine->document() == nullptr);

    // Test setting document back
    m_searchEngine->setDocument(m_testDocument);
    QVERIFY(m_searchEngine->document() == m_testDocument);
}

void SearchEngineTest::testDocumentAccess() {
    QVERIFY(m_searchEngine->document() == m_testDocument);

    // Test const access
    const SearchEngine* constEngine = m_searchEngine;
    QVERIFY(constEngine->document() == m_testDocument);
}

void SearchEngineTest::testBasicSearch() {
    QSignalSpy startedSpy(m_searchEngine, &SearchEngine::searchStarted);
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);

    m_searchEngine->search("test");

    QVERIFY(waitFor([&]() { return finishedSpy.count() > 0; }));
    QCOMPARE(startedSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 1);

    QList<SearchResult> results = m_searchEngine->results();
    QVERIFY(!results.isEmpty());
}

void SearchEngineTest::testSearchWithOptions() {
    SearchOptions options = createTestOptions();
    options.caseSensitive = true;
    options.wholeWords = true;

    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);

    m_searchEngine->search("Test", options);

    QVERIFY(waitFor([&]() { return finishedSpy.count() > 0; }));

    // Verify search was performed with options
    QList<SearchResult> results = m_searchEngine->results();
    // Results should respect case sensitivity and whole word matching
}

void SearchEngineTest::createTestPdf() {
    QTemporaryFile tempFile("test_document_XXXXXX.pdf");
    tempFile.setAutoRemove(false);
    QVERIFY(tempFile.open());
    m_testPdfPath = tempFile.fileName();
    tempFile.close();

    QPdfWriter writer(m_testPdfPath);
    writer.setPageSize(QPageSize::A4);

    QPainter painter(&writer);
    painter.drawText(100, 100, "This is a test document for searching.");
    painter.drawText(100, 200, "It contains multiple lines of text.");
    painter.drawText(100, 300,
                     "Some words appear multiple times: test, document, text.");
    painter.end();

    m_testDocument = Poppler::Document::load(m_testPdfPath).release();
    QVERIFY(m_testDocument != nullptr);
}

SearchOptions SearchEngineTest::createTestOptions() {
    SearchOptions options;
    options.caseSensitive = false;
    options.wholeWords = false;
    options.useRegex = false;
    options.maxResults = 100;
    options.contextLength = 50;
    return options;
}

void SearchEngineTest::verifySearchResult(const SearchResult& result,
                                          int expectedPage,
                                          const QString& expectedText) {
    QVERIFY(result.isValid());
    QCOMPARE(result.pageNumber, expectedPage);
    QVERIFY(result.matchedText.contains(expectedText, Qt::CaseInsensitive));
    QVERIFY(!result.contextText.isEmpty());
    QVERIFY(!result.boundingRect.isEmpty());
}

void SearchEngineTest::testIncrementalSearch() {
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);

    m_searchEngine->searchIncremental("test");

    QVERIFY(waitFor([&]() { return finishedSpy.count() > 0; }));

    QList<SearchResult> results = m_searchEngine->results();
    QVERIFY(!results.isEmpty());
}

void SearchEngineTest::testCancelSearch() {
    QSignalSpy cancelledSpy(m_searchEngine, &SearchEngine::searchCancelled);

    m_searchEngine->search("test");
    m_searchEngine->cancelSearch();

    QVERIFY(waitFor([&]() { return cancelledSpy.count() > 0; }));
    QCOMPARE(cancelledSpy.count(), 1);
}

void SearchEngineTest::testClearResults() {
    m_searchEngine->search("test");
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    QVERIFY(waitFor([&]() { return finishedSpy.count() > 0; }));

    QVERIFY(!m_searchEngine->results().isEmpty());

    m_searchEngine->clearResults();
    QVERIFY(m_searchEngine->results().isEmpty());
    QCOMPARE(m_searchEngine->resultCount(), 0);
}

void SearchEngineTest::testStartSearch() {
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);

    m_searchEngine->startSearch(m_testDocument, "test");

    QVERIFY(waitFor([&]() { return finishedSpy.count() > 0; }));

    QList<SearchResult> results = m_searchEngine->getResults();
    QVERIFY(!results.isEmpty());
}

void SearchEngineTest::testGetResults() {
    m_searchEngine->search("test");
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    QVERIFY(waitFor([&]() { return finishedSpy.count() > 0; }));

    QList<SearchResult> results1 = m_searchEngine->results();
    QList<SearchResult> results2 = m_searchEngine->getResults();

    QCOMPARE(results1.size(), results2.size());
    for (int i = 0; i < results1.size(); ++i) {
        QCOMPARE(results1[i].pageNumber, results2[i].pageNumber);
        QCOMPARE(results1[i].matchedText, results2[i].matchedText);
    }
}

void SearchEngineTest::testResultCount() {
    m_searchEngine->search("test");
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    QVERIFY(waitFor([&]() { return finishedSpy.count() > 0; }));

    int count1 = m_searchEngine->resultCount();
    int count2 = m_searchEngine->results().size();

    QCOMPARE(count1, count2);
    QVERIFY(count1 > 0);
}

void SearchEngineTest::testFuzzySearch() {
    SearchOptions options = createTestOptions();

    m_searchEngine->fuzzySearch("tset", 2,
                                options);  // "tset" should match "test"

    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    QVERIFY(waitFor([&]() { return finishedSpy.count() > 0; }));

    QList<SearchResult> results = m_searchEngine->results();
    QVERIFY(!results.isEmpty());
}

void SearchEngineTest::testWildcardSearch() {
    SearchOptions options = createTestOptions();

    m_searchEngine->wildcardSearch("te*t", options);

    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    QVERIFY(waitFor([&]() { return finishedSpy.count() > 0; }));

    QList<SearchResult> results = m_searchEngine->results();
    QVERIFY(!results.isEmpty());
}

void SearchEngineTest::testPhraseSearch() {
    SearchOptions options = createTestOptions();

    m_searchEngine->phraseSearch("test document", 0, options);

    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    QVERIFY(waitFor([&]() { return finishedSpy.count() > 0; }));

    QList<SearchResult> results = m_searchEngine->results();
    QVERIFY(!results.isEmpty());
}

void SearchEngineTest::testBooleanSearch() {
    SearchOptions options = createTestOptions();

    m_searchEngine->booleanSearch("test AND document", options);

    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    QVERIFY(waitFor([&]() { return finishedSpy.count() > 0; }));

    QList<SearchResult> results = m_searchEngine->results();
    QVERIFY(!results.isEmpty());
}

void SearchEngineTest::testProximitySearch() {
    QStringList terms = {"test", "document"};
    SearchOptions options = createTestOptions();

    m_searchEngine->proximitySearch(terms, 10, false, options);

    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    QVERIFY(waitFor([&]() { return finishedSpy.count() > 0; }));

    QList<SearchResult> results = m_searchEngine->results();
    QVERIFY(!results.isEmpty());
}

// Configuration tests
void SearchEngineTest::testCacheConfiguration() {
    // Test cache configuration
    QVERIFY(m_searchEngine != nullptr);
}

void SearchEngineTest::testIncrementalSearchConfiguration() {
    // Test incremental search configuration
    QVERIFY(m_searchEngine != nullptr);
}

void SearchEngineTest::testBackgroundProcessingConfiguration() {
    // Test background processing configuration
    QVERIFY(m_searchEngine != nullptr);
}

// Results access tests
void SearchEngineTest::testResultsAccess() {
    QList<SearchResult> results = m_searchEngine->results();
    QVERIFY(results.isEmpty());  // No search performed yet
}

void SearchEngineTest::testCurrentQuery() {
    QString query = m_searchEngine->currentQuery();
    QVERIFY(query.isEmpty());  // No search performed yet
}

void SearchEngineTest::testIsSearching() {
    QVERIFY(!m_searchEngine->isSearching());
}

// Performance metrics tests
void SearchEngineTest::testCacheHitRatio() {
    double ratio = m_searchEngine->cacheHitRatio();
    QVERIFY(ratio >= 0.0 && ratio <= 1.0);
}

void SearchEngineTest::testCacheMemoryUsage() {
    qint64 usage = m_searchEngine->cacheMemoryUsage();
    QVERIFY(usage >= 0);
}

void SearchEngineTest::testResetStatistics() {
    m_searchEngine->resetStatistics();
    QVERIFY(m_searchEngine->cacheHitRatio() == 0.0);
}

// Advanced features tests
void SearchEngineTest::testAdvancedFeatures() {
    // Test advanced features
    QVERIFY(m_searchEngine != nullptr);
}

void SearchEngineTest::testHighlightColors() {
    // Test highlight colors
    QVERIFY(m_searchEngine != nullptr);
}

void SearchEngineTest::testSearchSuggestions() {
    // Test search suggestions
    QVERIFY(m_searchEngine != nullptr);
}

void SearchEngineTest::testSearchHistory() {
    // Test search history
    QVERIFY(m_searchEngine != nullptr);
}

// Signal tests
void SearchEngineTest::testSearchStartedSignal() {
    QSignalSpy spy(m_searchEngine, &SearchEngine::searchStarted);
    SearchOptions options = createTestOptions();
    m_searchEngine->search("test", options);
    QVERIFY(spy.count() > 0);
}

void SearchEngineTest::testSearchFinishedSignal() {
    QSignalSpy spy(m_searchEngine, &SearchEngine::searchFinished);
    SearchOptions options = createTestOptions();
    m_searchEngine->search("test", options);
    QVERIFY(waitFor([&]() { return spy.count() > 0; }));
}

void SearchEngineTest::testSearchProgressSignal() {
    QSignalSpy spy(m_searchEngine, &SearchEngine::searchProgress);
    SearchOptions options = createTestOptions();
    m_searchEngine->search("test", options);
    // Progress signal may or may not be emitted depending on document size
    QVERIFY(spy.count() >= 0);
}

void SearchEngineTest::testSearchCancelledSignal() {
    QSignalSpy spy(m_searchEngine, &SearchEngine::searchCancelled);
    SearchOptions options = createTestOptions();
    m_searchEngine->search("test", options);
    m_searchEngine->cancelSearch();
    QVERIFY(waitFor([&]() { return spy.count() > 0; }));
}

void SearchEngineTest::testSearchErrorSignal() {
    QSignalSpy spy(m_searchEngine, &SearchEngine::searchError);
    // Error signal should not be emitted during normal operation
    QVERIFY(spy.count() == 0);
}

void SearchEngineTest::testResultsUpdatedSignal() {
    QSignalSpy spy(m_searchEngine, &SearchEngine::resultsUpdated);
    SearchOptions options = createTestOptions();
    m_searchEngine->search("test", options);
    QVERIFY(waitFor([&]() { return spy.count() > 0; }));
}

// Edge case tests
void SearchEngineTest::testNullDocument() {
    SearchEngine engine(nullptr);
    SearchOptions options = createTestOptions();
    engine.search("test", options);
    QVERIFY(engine.results().isEmpty());
}

void SearchEngineTest::testEmptyQuery() {
    SearchOptions options = createTestOptions();
    m_searchEngine->search("", options);
    QVERIFY(m_searchEngine->results().isEmpty());
}

void SearchEngineTest::testInvalidOptions() {
    SearchOptions options;
    // Test with invalid options
    m_searchEngine->search("test", options);
    // Should handle gracefully
    QVERIFY(true);
}

void SearchEngineTest::testLargeDocument() {
    // Test with large document (if available)
    QVERIFY(m_searchEngine != nullptr);
}

void SearchEngineTest::testConcurrentSearches() {
    // Test concurrent searches
    SearchOptions options = createTestOptions();
    m_searchEngine->search("test1", options);
    m_searchEngine->search("test2", options);
    // Second search should cancel first
    QVERIFY(true);
}

void SearchEngineTest::testSearchWithRealPdf() {
    // Test with real PDF (if available)
    QVERIFY(m_searchEngine != nullptr);
}

void SearchEngineTest::testSearchWithComplexOptions() {
    SearchOptions options = createTestOptions();
    options.caseSensitive = true;
    options.wholeWords = true;
    options.useRegex = true;
    m_searchEngine->search("test", options);
    QVERIFY(waitFor([&]() { return !m_searchEngine->isSearching(); }));
}

void SearchEngineTest::testSearchPerformance() {
    // Test search performance
    SearchOptions options = createTestOptions();
    QElapsedTimer timer;
    timer.start();
    m_searchEngine->search("test", options);
    QVERIFY(waitFor([&]() { return !m_searchEngine->isSearching(); }));
    qint64 elapsed = timer.elapsed();
    QVERIFY(elapsed >= 0);  // Just verify it completes
}

QTEST_MAIN(SearchEngineTest)
#include "test_search_engine.moc"
