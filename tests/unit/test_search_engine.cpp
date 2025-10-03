#include <QtTest/QtTest>
#include <QApplication>
#include <QTemporaryFile>
#include <QPainter>
#include <QPdfWriter>
#include <QSignalSpy>
#include <poppler-qt6.h>
#include "../../app/search/SearchEngine.h"
#include "../../app/search/SearchConfiguration.h"

/**
 * Comprehensive test suite for the new modular SearchEngine
 */
class TestSearchEngineNew : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testSearchEngineInitialization();
    void testDocumentManagement();
    void testBasicSearch();
    void testEmptySearch();
    void testSearchCancellation();
    
    // Search options tests
    void testCaseSensitiveSearch();
    void testWholeWordSearch();
    void testRegexSearch();
    void testContextExtraction();
    
    // Incremental search tests
    void testIncrementalSearchEnabled();
    void testIncrementalSearchDelay();
    void testIncrementalSearchQueryExtension();
    void testIncrementalSearchQueryReduction();
    
    // Cache tests
    void testCacheEnabled();
    void testCacheHitRatio();
    void testCacheMemoryUsage();
    void testCacheInvalidation();
    
    // Background processing tests
    void testBackgroundSearchEnabled();
    void testBackgroundSearchCancellation();
    void testMultipleBackgroundSearches();
    
    // Performance metrics tests
    void testMetricsRecording();
    void testMetricsStatistics();
    void testPerformanceWarnings();
    
    // Signal/slot tests
    void testSearchStartedSignal();
    void testSearchFinishedSignal();
    void testSearchProgressSignal();
    void testSearchErrorSignal();
    
    // Edge cases
    void testNullDocument();
    void testEmptyDocument();
    void testLargeDocument();
    void testSpecialCharacters();
    void testUnicodeSearch();

private:
    SearchEngine* m_searchEngine;
    Poppler::Document* m_testDocument;
    QString m_testPdfPath;
    QStringList m_testTexts;

    // Helper methods
    Poppler::Document* createTestDocument(int pageCount = 3);
    Poppler::Document* createLargeDocument();
    void waitForSearchCompletion();
    bool waitForSignal(QSignalSpy& spy, int timeout = 5000);
};

void TestSearchEngineNew::initTestCase()
{
    // Initialize test texts
    m_testTexts = {
        "Page 1: Basic search test content.\n"
        "This page contains simple text for testing.\n"
        "Words: search, find, locate, discover.\n"
        "Case test: UPPER, lower, MiXeD.",
        
        "Page 2: Advanced patterns and special characters.\n"
        "Email: test@example.com\n"
        "Phone: +1-234-567-8900\n"
        "Special: !@#$%^&*()_+-=[]{}|;':\",./<>?\n"
        "Unicode: café, naïve, 北京, مرحبا",
        
        "Page 3: Performance testing content.\n"
        "This page has repeated words: test test test.\n"
        "Long text for context extraction testing.\n"
        "The quick brown fox jumps over the lazy dog."
    };

    m_searchEngine = new SearchEngine(this);
    QVERIFY(m_searchEngine != nullptr);
}

void TestSearchEngineNew::cleanupTestCase()
{
    delete m_searchEngine;
    if (m_testDocument) {
        delete m_testDocument;
    }
    if (!m_testPdfPath.isEmpty()) {
        QFile::remove(m_testPdfPath);
    }
}

void TestSearchEngineNew::init()
{
    // Create fresh test document for each test
    m_testDocument = createTestDocument();
    QVERIFY(m_testDocument != nullptr);
    m_searchEngine->setDocument(m_testDocument);
}

void TestSearchEngineNew::cleanup()
{
    // Clean up after each test
    m_searchEngine->cancelSearch();
    m_searchEngine->clearResults();
    m_searchEngine->resetStatistics();
    
    if (m_testDocument) {
        delete m_testDocument;
        m_testDocument = nullptr;
    }
}

void TestSearchEngineNew::testSearchEngineInitialization()
{
    // Test default configuration
    QVERIFY(m_searchEngine->isCacheEnabled());
    QVERIFY(m_searchEngine->isIncrementalSearchEnabled());
    QVERIFY(m_searchEngine->isBackgroundProcessingEnabled());
    QCOMPARE(m_searchEngine->resultCount(), 0);
    QVERIFY(!m_searchEngine->isSearching());
}

void TestSearchEngineNew::testDocumentManagement()
{
    // Test setting and getting document
    QCOMPARE(m_searchEngine->document(), m_testDocument);
    
    // Test setting null document
    m_searchEngine->setDocument(nullptr);
    QCOMPARE(m_searchEngine->document(), nullptr);
    
    // Restore document
    m_searchEngine->setDocument(m_testDocument);
    QCOMPARE(m_searchEngine->document(), m_testDocument);
}

void TestSearchEngineNew::testBasicSearch()
{
    QSignalSpy startedSpy(m_searchEngine, &SearchEngine::searchStarted);
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    SearchOptions options;
    m_searchEngine->search("test", options);
    
    QVERIFY(waitForSignal(finishedSpy));
    QCOMPARE(startedSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 1);
    
    QList<SearchResult> results = m_searchEngine->results();
    QVERIFY(!results.isEmpty());
    QVERIFY(m_searchEngine->resultCount() > 0);
    
    // Verify all results contain the search term
    for (const SearchResult& result : results) {
        QVERIFY(result.matchedText.contains("test", Qt::CaseInsensitive));
        QVERIFY(result.pageNumber >= 0 && result.pageNumber < 3);
    }
}

void TestSearchEngineNew::testEmptySearch()
{
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    SearchOptions options;
    m_searchEngine->search("", options);
    
    // Empty search should clear results immediately
    QTest::qWait(100);
    
    QCOMPARE(m_searchEngine->resultCount(), 0);
    QVERIFY(m_searchEngine->results().isEmpty());
}

void TestSearchEngineNew::testSearchCancellation()
{
    QSignalSpy cancelledSpy(m_searchEngine, &SearchEngine::searchCancelled);
    
    // Start a search
    SearchOptions options;
    m_searchEngine->search("test", options);
    
    // Cancel immediately
    m_searchEngine->cancelSearch();
    
    QVERIFY(waitForSignal(cancelledSpy, 1000));
    QVERIFY(!m_searchEngine->isSearching());
}

void TestSearchEngineNew::testCaseSensitiveSearch()
{
    SearchOptions options;
    options.caseSensitive = true;
    
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    // Search for uppercase
    m_searchEngine->search("UPPER", options);
    QVERIFY(waitForSignal(finishedSpy));
    
    QList<SearchResult> results = m_searchEngine->results();
    QVERIFY(!results.isEmpty());
    
    // Verify exact case match
    for (const SearchResult& result : results) {
        QVERIFY(result.matchedText.contains("UPPER"));
    }
    
    // Search for lowercase (should find different results)
    finishedSpy.clear();
    m_searchEngine->search("lower", options);
    QVERIFY(waitForSignal(finishedSpy));
    
    results = m_searchEngine->results();
    QVERIFY(!results.isEmpty());
    
    for (const SearchResult& result : results) {
        QVERIFY(result.matchedText.contains("lower"));
    }
}

void TestSearchEngineNew::testWholeWordSearch()
{
    SearchOptions options;
    options.wholeWords = true;
    
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    m_searchEngine->search("test", options);
    QVERIFY(waitForSignal(finishedSpy));
    
    QList<SearchResult> results = m_searchEngine->results();
    QVERIFY(!results.isEmpty());
    
    // Verify whole word matches
    for (const SearchResult& result : results) {
        // The word "test" should be surrounded by word boundaries
        QString context = result.contextText;
        int pos = context.indexOf("test", 0, Qt::CaseInsensitive);
        if (pos > 0) {
            QChar before = context[pos - 1];
            QVERIFY(!before.isLetterOrNumber());
        }
        if (pos + 4 < context.length()) {
            QChar after = context[pos + 4];
            QVERIFY(!after.isLetterOrNumber());
        }
    }
}

void TestSearchEngineNew::testRegexSearch()
{
    SearchOptions options;
    options.useRegex = true;
    
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    // Search for email pattern
    m_searchEngine->search("\\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Z|a-z]{2,}\\b", options);
    QVERIFY(waitForSignal(finishedSpy));
    
    QList<SearchResult> results = m_searchEngine->results();
    QVERIFY(!results.isEmpty());
    
    // Verify email pattern match
    for (const SearchResult& result : results) {
        QVERIFY(result.matchedText.contains("@"));
        QVERIFY(result.matchedText.contains("."));
    }
}

void TestSearchEngineNew::testContextExtraction()
{
    SearchOptions options;
    options.contextLength = 30; // Set specific context length
    
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    m_searchEngine->search("fox", options);
    QVERIFY(waitForSignal(finishedSpy));
    
    QList<SearchResult> results = m_searchEngine->results();
    QVERIFY(!results.isEmpty());
    
    // Verify context contains surrounding text
    for (const SearchResult& result : results) {
        QVERIFY(result.contextText.contains("fox"));
        QVERIFY(result.contextText.length() > result.matchedText.length());
        // Should contain some surrounding text
        QVERIFY(result.contextText.contains("brown") || result.contextText.contains("jumps"));
    }
}

void TestSearchEngineNew::testIncrementalSearchEnabled()
{
    m_searchEngine->setIncrementalSearchEnabled(true);
    QVERIFY(m_searchEngine->isIncrementalSearchEnabled());
    
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    // Incremental search should delay execution
    m_searchEngine->searchIncremental("t", SearchOptions());
    QTest::qWait(100);
    QCOMPARE(finishedSpy.count(), 0); // Should not finish immediately
    
    m_searchEngine->searchIncremental("te", SearchOptions());
    QTest::qWait(100);
    QCOMPARE(finishedSpy.count(), 0); // Still waiting
    
    m_searchEngine->searchIncremental("test", SearchOptions());
    QVERIFY(waitForSignal(finishedSpy)); // Should trigger after delay
    
    QVERIFY(!m_searchEngine->results().isEmpty());
}

void TestSearchEngineNew::testIncrementalSearchDelay()
{
    m_searchEngine->setIncrementalSearchEnabled(true);
    
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    QElapsedTimer timer;
    
    timer.start();
    m_searchEngine->searchIncremental("test", SearchOptions());
    
    QVERIFY(waitForSignal(finishedSpy));
    
    // Should have a delay (default 300ms)
    qint64 elapsed = timer.elapsed();
    QVERIFY(elapsed >= 250); // Allow some margin
}

void TestSearchEngineNew::testIncrementalSearchQueryExtension()
{
    m_searchEngine->setIncrementalSearchEnabled(true);
    
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    // First search
    m_searchEngine->search("te", SearchOptions());
    QVERIFY(waitForSignal(finishedSpy));
    int firstResultCount = m_searchEngine->resultCount();
    
    finishedSpy.clear();
    
    // Extended search (should refine results)
    m_searchEngine->searchIncremental("test", SearchOptions());
    QVERIFY(waitForSignal(finishedSpy));
    int secondResultCount = m_searchEngine->resultCount();
    
    // Extended query should have fewer or equal results
    QVERIFY(secondResultCount <= firstResultCount);
}

void TestSearchEngineNew::testIncrementalSearchQueryReduction()
{
    m_searchEngine->setIncrementalSearchEnabled(true);
    
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    // First search with longer query
    m_searchEngine->search("test", SearchOptions());
    QVERIFY(waitForSignal(finishedSpy));
    int firstResultCount = m_searchEngine->resultCount();
    
    finishedSpy.clear();
    
    // Reduced search
    m_searchEngine->searchIncremental("te", SearchOptions());
    QVERIFY(waitForSignal(finishedSpy));
    int secondResultCount = m_searchEngine->resultCount();
    
    // Reduced query should have more or equal results
    QVERIFY(secondResultCount >= firstResultCount);
}

void TestSearchEngineNew::testCacheEnabled()
{
    m_searchEngine->setCacheEnabled(true);
    QVERIFY(m_searchEngine->isCacheEnabled());
    
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    // First search (cache miss)
    QElapsedTimer timer;
    timer.start();
    m_searchEngine->search("cache", SearchOptions());
    QVERIFY(waitForSignal(finishedSpy));
    qint64 firstSearchTime = timer.elapsed();
    
    finishedSpy.clear();
    
    // Second identical search (cache hit)
    timer.restart();
    m_searchEngine->search("cache", SearchOptions());
    QVERIFY(waitForSignal(finishedSpy));
    qint64 secondSearchTime = timer.elapsed();
    
    // Cached search should be faster
    QVERIFY(secondSearchTime < firstSearchTime);
}

void TestSearchEngineNew::testCacheHitRatio()
{
    m_searchEngine->setCacheEnabled(true);
    m_searchEngine->resetStatistics();
    
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    // Perform searches
    m_searchEngine->search("test1", SearchOptions());
    QVERIFY(waitForSignal(finishedSpy));
    
    finishedSpy.clear();
    m_searchEngine->search("test1", SearchOptions()); // Cache hit
    QVERIFY(waitForSignal(finishedSpy));
    
    finishedSpy.clear();
    m_searchEngine->search("test2", SearchOptions()); // Cache miss
    QVERIFY(waitForSignal(finishedSpy));
    
    finishedSpy.clear();
    m_searchEngine->search("test1", SearchOptions()); // Cache hit
    QVERIFY(waitForSignal(finishedSpy));
    
    double hitRatio = m_searchEngine->cacheHitRatio();
    QVERIFY(hitRatio > 0.0 && hitRatio <= 1.0);
}

void TestSearchEngineNew::testCacheMemoryUsage()
{
    m_searchEngine->setCacheEnabled(true);
    
    qint64 initialMemory = m_searchEngine->cacheMemoryUsage();
    
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    // Perform multiple searches to populate cache
    for (int i = 0; i < 5; ++i) {
        m_searchEngine->search(QString("test%1").arg(i), SearchOptions());
        QVERIFY(waitForSignal(finishedSpy));
        finishedSpy.clear();
    }
    
    qint64 finalMemory = m_searchEngine->cacheMemoryUsage();
    QVERIFY(finalMemory > initialMemory);
}

void TestSearchEngineNew::testCacheInvalidation()
{
    m_searchEngine->setCacheEnabled(true);
    
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    // Cache a search
    m_searchEngine->search("test", SearchOptions());
    QVERIFY(waitForSignal(finishedSpy));
    
    // Change document (should invalidate cache)
    Poppler::Document* newDoc = createTestDocument();
    m_searchEngine->setDocument(newDoc);
    
    finishedSpy.clear();
    
    // Same search should not use cache
    QElapsedTimer timer;
    timer.start();
    m_searchEngine->search("test", SearchOptions());
    QVERIFY(waitForSignal(finishedSpy));
    
    // Should take normal time (not cached)
    QVERIFY(timer.elapsed() > 10);
    
    delete newDoc;
}

void TestSearchEngineNew::testBackgroundSearchEnabled()
{
    m_searchEngine->setBackgroundProcessingEnabled(true);
    QVERIFY(m_searchEngine->isBackgroundProcessingEnabled());
    
    QSignalSpy startedSpy(m_searchEngine, &SearchEngine::searchStarted);
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    m_searchEngine->search("background", SearchOptions());
    
    // Should start immediately
    QVERIFY(startedSpy.wait(100));
    
    // Should be searching
    QVERIFY(m_searchEngine->isSearching());
    
    // Wait for completion
    QVERIFY(waitForSignal(finishedSpy));
    QVERIFY(!m_searchEngine->isSearching());
}

void TestSearchEngineNew::testBackgroundSearchCancellation()
{
    m_searchEngine->setBackgroundProcessingEnabled(true);
    
    QSignalSpy cancelledSpy(m_searchEngine, &SearchEngine::searchCancelled);

    // Start a long search
    SearchOptions regexOptions;
    regexOptions.useRegex = true;
    m_searchEngine->search(".*", regexOptions);

    // Cancel while in progress
    QTest::qWait(10);
    m_searchEngine->cancelSearch();
    
    QVERIFY(waitForSignal(cancelledSpy, 2000));
    QVERIFY(!m_searchEngine->isSearching());
}

void TestSearchEngineNew::testMultipleBackgroundSearches()
{
    m_searchEngine->setBackgroundProcessingEnabled(true);
    
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    // Start first search
    m_searchEngine->search("first", SearchOptions());
    
    // Start second search (should cancel first)
    m_searchEngine->search("second", SearchOptions());
    
    QVERIFY(waitForSignal(finishedSpy));
    
    // Results should be from second search
    QList<SearchResult> results = m_searchEngine->results();
    if (!results.isEmpty()) {
        QVERIFY(m_searchEngine->currentQuery() == "second");
    }
}

void TestSearchEngineNew::testMetricsRecording()
{
    m_searchEngine->resetStatistics();
    
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    // Perform searches
    for (int i = 0; i < 3; ++i) {
        m_searchEngine->search(QString("metric%1").arg(i), SearchOptions());
        QVERIFY(waitForSignal(finishedSpy));
        finishedSpy.clear();
    }
    
    // Metrics should be recorded
    // (Actual metric retrieval would depend on SearchMetrics interface)
}

void TestSearchEngineNew::testMetricsStatistics()
{
    m_searchEngine->resetStatistics();
    
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    // Perform varied searches
    SearchOptions options;
    
    // Fast search
    options.maxResults = 1;
    m_searchEngine->search("quick", options);
    QVERIFY(waitForSignal(finishedSpy));
    
    finishedSpy.clear();
    
    // Slower search
    options.maxResults = 1000;
    m_searchEngine->search("e", options); // Common letter
    QVERIFY(waitForSignal(finishedSpy));
}

void TestSearchEngineNew::testPerformanceWarnings()
{
    // This would require access to the metrics component
    // For now, just test that slow searches complete
    
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    // Perform a potentially slow search
    SearchOptions options;
    options.useRegex = true;
    m_searchEngine->search(".*test.*", options);
    
    QVERIFY(waitForSignal(finishedSpy, 10000));
}

void TestSearchEngineNew::testSearchStartedSignal()
{
    QSignalSpy startedSpy(m_searchEngine, &SearchEngine::searchStarted);
    
    m_searchEngine->search("signal", SearchOptions());
    
    QVERIFY(startedSpy.wait(1000));
    QCOMPARE(startedSpy.count(), 1);
}

void TestSearchEngineNew::testSearchFinishedSignal()
{
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    m_searchEngine->search("signal", SearchOptions());
    
    QVERIFY(waitForSignal(finishedSpy));
    QCOMPARE(finishedSpy.count(), 1);
    
    // Check signal arguments
    QList<QVariant> arguments = finishedSpy.takeFirst();
    QVERIFY(arguments.at(0).canConvert<QList<SearchResult>>());
}

void TestSearchEngineNew::testSearchProgressSignal()
{
    QSignalSpy progressSpy(m_searchEngine, &SearchEngine::searchProgress);
    
    m_searchEngine->search("progress", SearchOptions());
    
    // Wait for search to complete
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    QVERIFY(waitForSignal(finishedSpy));
    
    // Should have received progress updates
    QVERIFY(progressSpy.count() > 0);
    
    // Check progress values
    for (const QList<QVariant>& args : progressSpy) {
        int current = args.at(0).toInt();
        int total = args.at(1).toInt();
        QVERIFY(current > 0 && current <= total);
        QVERIFY(total > 0);
    }
}

void TestSearchEngineNew::testSearchErrorSignal()
{
    QSignalSpy errorSpy(m_searchEngine, &SearchEngine::searchError);
    
    // Search with no document
    m_searchEngine->setDocument(nullptr);
    m_searchEngine->search("error", SearchOptions());
    
    QVERIFY(errorSpy.wait(1000));
    QCOMPARE(errorSpy.count(), 1);
    
    // Check error message
    QList<QVariant> arguments = errorSpy.takeFirst();
    QString errorMsg = arguments.at(0).toString();
    QVERIFY(!errorMsg.isEmpty());
}

void TestSearchEngineNew::testNullDocument()
{
    m_searchEngine->setDocument(nullptr);
    
    QSignalSpy errorSpy(m_searchEngine, &SearchEngine::searchError);
    
    m_searchEngine->search("test", SearchOptions());
    
    QVERIFY(errorSpy.wait(1000));
    QCOMPARE(m_searchEngine->resultCount(), 0);
}

void TestSearchEngineNew::testEmptyDocument()
{
    // Create document with no text
    Poppler::Document* emptyDoc = createTestDocument(0);
    m_searchEngine->setDocument(emptyDoc);
    
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    m_searchEngine->search("test", SearchOptions());
    
    QVERIFY(waitForSignal(finishedSpy));
    QCOMPARE(m_searchEngine->resultCount(), 0);
    
    delete emptyDoc;
}

void TestSearchEngineNew::testLargeDocument()
{
    Poppler::Document* largeDoc = createLargeDocument();
    QVERIFY(largeDoc != nullptr);
    
    m_searchEngine->setDocument(largeDoc);
    
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    QElapsedTimer timer;
    
    timer.start();
    m_searchEngine->search("test", SearchOptions());
    
    QVERIFY(waitForSignal(finishedSpy, 30000)); // 30 second timeout
    
    qint64 elapsed = timer.elapsed();
    qDebug() << "Large document search took" << elapsed << "ms";
    
    delete largeDoc;
}

void TestSearchEngineNew::testSpecialCharacters()
{
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    // Search for special characters
    m_searchEngine->search("@#$", SearchOptions());
    
    QVERIFY(waitForSignal(finishedSpy));
    
    // Should find special characters
    QList<SearchResult> results = m_searchEngine->results();
    if (!results.isEmpty()) {
        QVERIFY(results[0].matchedText.contains("@") || 
                results[0].matchedText.contains("#") ||
                results[0].matchedText.contains("$"));
    }
}

void TestSearchEngineNew::testUnicodeSearch()
{
    QSignalSpy finishedSpy(m_searchEngine, &SearchEngine::searchFinished);
    
    // Search for Unicode text
    m_searchEngine->search("café", SearchOptions());
    
    QVERIFY(waitForSignal(finishedSpy));
    
    QList<SearchResult> results = m_searchEngine->results();
    if (!results.isEmpty()) {
        QVERIFY(results[0].matchedText.contains("café"));
    }
    
    finishedSpy.clear();
    
    // Search for Chinese characters
    m_searchEngine->search("北京", SearchOptions());
    QVERIFY(waitForSignal(finishedSpy));
    
    results = m_searchEngine->results();
    if (!results.isEmpty()) {
        QVERIFY(results[0].matchedText.contains("北京"));
    }
}

// Helper method implementations

Poppler::Document* TestSearchEngineNew::createTestDocument(int pageCount)
{
    QTemporaryFile tempFile;
    tempFile.setFileTemplate(QDir::tempPath() + "/test_search_engine_XXXXXX.pdf");
    if (!tempFile.open()) {
        return nullptr;
    }
    
    QString pdfPath = tempFile.fileName();
    tempFile.close();
    
    QPdfWriter pdfWriter(pdfPath);
    pdfWriter.setPageSize(QPageSize::A4);
    pdfWriter.setResolution(300);
    
    QPainter painter(&pdfWriter);
    if (!painter.isActive()) {
        return nullptr;
    }
    
    QFont font = painter.font();
    font.setPointSize(12);
    painter.setFont(font);
    
    for (int page = 0; page < qMin(pageCount, m_testTexts.size()); ++page) {
        if (page > 0) {
            pdfWriter.newPage();
        }
        
        QRect textRect(100, 100, 400, 600);
        painter.drawText(textRect, Qt::TextWordWrap, m_testTexts[page]);
    }
    
    painter.end();
    
    m_testPdfPath = pdfPath;
    auto doc = Poppler::Document::load(pdfPath);
    if (doc && doc->numPages() > 0) {
        return doc.release();
    }
    return nullptr;
}

Poppler::Document* TestSearchEngineNew::createLargeDocument()
{
    QTemporaryFile tempFile;
    tempFile.setFileTemplate(QDir::tempPath() + "/test_large_doc_XXXXXX.pdf");
    if (!tempFile.open()) {
        return nullptr;
    }
    
    QString pdfPath = tempFile.fileName();
    tempFile.close();
    
    QPdfWriter pdfWriter(pdfPath);
    pdfWriter.setPageSize(QPageSize::A4);
    pdfWriter.setResolution(300);
    
    QPainter painter(&pdfWriter);
    if (!painter.isActive()) {
        return nullptr;
    }
    
    QFont font = painter.font();
    font.setPointSize(10);
    painter.setFont(font);
    
    // Create 100 pages
    for (int page = 0; page < 100; ++page) {
        if (page > 0) {
            pdfWriter.newPage();
        }
        
        QString pageText = QString("Page %1\n").arg(page + 1);
        pageText += "This is a test page with some content.\n";
        pageText += "Search term: test\n";
        pageText += "Lorem ipsum dolor sit amet, consectetur adipiscing elit.\n";
        
        QRect textRect(50, 50, 500, 700);
        painter.drawText(textRect, Qt::TextWordWrap, pageText);
    }
    
    painter.end();
    
    auto doc = Poppler::Document::load(pdfPath);
    if (doc && doc->numPages() > 0) {
        return doc.release();
    }
    return nullptr;
}

void TestSearchEngineNew::waitForSearchCompletion()
{
    QSignalSpy spy(m_searchEngine, &SearchEngine::searchFinished);
    spy.wait(5000);
}

bool TestSearchEngineNew::waitForSignal(QSignalSpy& spy, int timeout)
{
    if (!spy.isEmpty()) {
        return true;
    }
    return spy.wait(timeout);
}

QTEST_MAIN(TestSearchEngineNew)
#include "test_search_engine.moc"
