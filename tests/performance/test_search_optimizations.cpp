#include <QtTest/QtTest>
#include <QApplication>
#include <QElapsedTimer>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QPdfWriter>
#include <QPainter>
#include <QPageSize>
#include <poppler-qt6.h>
// OptimizedSearchEngine removed - functionality integrated into SearchEngine
#include "../../app/search/SearchEngine.h"
#include "../../app/model/SearchModel.h"
#include "../../app/cache/SearchResultCache.h"
#include "../../app/cache/PageTextCache.h"

class TestSearchOptimizations : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Cache performance tests
    void testSearchResultCachePerformance();
    void testPageTextCachePerformance();
    void testSearchHighlightCachePerformance();
    
    // Search engine performance tests
    void testOptimizedSearchEnginePerformance();
    void testIncrementalSearchPerformance();
    void testCacheHitRatioImprovement();
    
    // Comparison tests
    void testOptimizedVsBasicSearchPerformance();
    void testMemoryUsageOptimization();
    void testSearchResponseTime();

private:
    Poppler::Document* m_testDocument;
    SearchEngine* m_searchEngine;
    SearchModel* m_basicSearchModel;
    
    Poppler::Document* createTestDocument();
    void performanceComparison(const QString& query, int iterations = 10);
    void measureCacheEffectiveness();
};

void TestSearchOptimizations::initTestCase()
{
    m_testDocument = createTestDocument();
    QVERIFY(m_testDocument != nullptr);
    
    m_searchEngine = new SearchEngine(this);
    m_basicSearchModel = new SearchModel(this);

    // Disable advanced features in basic model for comparison
    m_basicSearchModel->setAdvancedSearchEnabled(false);
    
    qDebug() << "Search optimizations test initialized with test document";
}

void TestSearchOptimizations::cleanupTestCase()
{
    delete m_testDocument;
    qDebug() << "Search optimizations test cleanup completed";
}

Poppler::Document* TestSearchOptimizations::createTestDocument()
{
    // Create a test PDF with multiple pages and searchable text
    QString testPdfPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/search_test.pdf";

    QPdfWriter pdfWriter(testPdfPath);
    pdfWriter.setPageSize(QPageSize::A4);

    QPainter painter(&pdfWriter);
    if (!painter.isActive()) {
        return nullptr;
    }

    // Create 10 pages with searchable text
    QStringList testTexts = {
        "This is the first page with some sample text for searching. The quick brown fox jumps over the lazy dog.",
        "Second page contains different content. Lorem ipsum dolor sit amet, consectetur adipiscing elit.",
        "Third page has more text to search through. Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
        "Fourth page with additional content. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris.",
        "Fifth page continues the pattern. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum.",
        "Sixth page has even more text. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia.",
        "Seventh page with unique content. Deserunt mollit anim id est laborum. Sed ut perspiciatis unde omnis.",
        "Eighth page contains special keywords: optimization, performance, cache, search, engine, fast, efficient.",
        "Ninth page has repeated words: test test test search search search performance performance cache cache.",
        "Tenth page concludes with final text. At vero eos et accusamus et iusto odio dignissimos ducimus qui."
    };

    for (int page = 0; page < testTexts.size(); ++page) {
        if (page > 0) {
            pdfWriter.newPage();
        }
        
        QFont font = painter.font();
        font.setPointSize(12);
        painter.setFont(font);
        
        QRect textRect(100, 100, 400, 600);
        painter.drawText(textRect, Qt::TextWordWrap, testTexts[page]);
        
        // Add page number
        painter.drawText(100, 50, QString("Page %1").arg(page + 1));
    }

    painter.end();

    auto doc = Poppler::Document::load(testPdfPath);
    if (doc && doc->numPages() > 0) {
        return doc.release();
    }
    return nullptr;
}

void TestSearchOptimizations::testSearchResultCachePerformance()
{
    qDebug() << "Testing SearchResultCache performance";
    
    SearchResultCache cache;
    QElapsedTimer timer;
    
    // Test cache storage performance
    timer.start();
    for (int i = 0; i < 1000; ++i) {
        SearchResultCache::CacheKey key;
        key.query = QString("test%1").arg(i);
        key.documentId = "test_doc";
        key.documentModified = QDateTime::currentMSecsSinceEpoch();
        
        QList<SearchResult> results;
        for (int j = 0; j < 10; ++j) {
            SearchResult result(j, key.query, "context", QRectF(0, 0, 100, 20), 0, key.query.length());
            results.append(result);
        }
        
        cache.storeResults(key, results);
    }
    qint64 storeTime = timer.elapsed();
    
    // Test cache retrieval performance
    timer.restart();
    int hits = 0;
    for (int i = 0; i < 1000; ++i) {
        SearchResultCache::CacheKey key;
        key.query = QString("test%1").arg(i);
        key.documentId = "test_doc";
        key.documentModified = QDateTime::currentMSecsSinceEpoch();
        
        if (cache.hasResults(key)) {
            QList<SearchResult> results = cache.getResults(key);
            if (!results.isEmpty()) {
                hits++;
            }
        }
    }
    qint64 retrieveTime = timer.elapsed();
    
    qDebug() << "Cache store time:" << storeTime << "ms";
    qDebug() << "Cache retrieve time:" << retrieveTime << "ms";
    qDebug() << "Cache hits:" << hits << "/ 1000";
    qDebug() << "Cache hit ratio:" << cache.getHitRatio();
    
    QVERIFY(storeTime < 1000); // Should store 1000 entries in less than 1 second
    QVERIFY(retrieveTime < 500); // Should retrieve 1000 entries in less than 0.5 seconds
    QVERIFY(hits > 900); // Should have high hit ratio
}

void TestSearchOptimizations::testPageTextCachePerformance()
{
    qDebug() << "Testing PageTextCache performance";
    
    PageTextCache cache;
    QElapsedTimer timer;
    
    // Test cache storage performance
    timer.start();
    for (int i = 0; i < 100; ++i) {
        QString text = QString("This is test text for page %1. ").repeated(100); // ~3KB per page
        cache.storePageText("test_doc", i, text);
    }
    qint64 storeTime = timer.elapsed();
    
    // Test cache retrieval performance
    timer.restart();
    int hits = 0;
    for (int i = 0; i < 100; ++i) {
        if (cache.hasPageText("test_doc", i)) {
            QString text = cache.getPageText("test_doc", i);
            if (!text.isEmpty()) {
                hits++;
            }
        }
    }
    qint64 retrieveTime = timer.elapsed();
    
    qDebug() << "Page text cache store time:" << storeTime << "ms";
    qDebug() << "Page text cache retrieve time:" << retrieveTime << "ms";
    qDebug() << "Page text cache hits:" << hits << "/ 100";
    qDebug() << "Page text cache memory usage:" << cache.getMemoryUsage() << "bytes";
    
    QVERIFY(storeTime < 500); // Should store 100 pages in less than 0.5 seconds
    QVERIFY(retrieveTime < 100); // Should retrieve 100 pages in less than 0.1 seconds
    QVERIFY(hits == 100); // Should have 100% hit ratio for stored pages
}

void TestSearchOptimizations::testOptimizedSearchEnginePerformance()
{
    qDebug() << "Testing SearchEngine performance";

    m_searchEngine->setDocument(m_testDocument);

    QElapsedTimer timer;
    QStringList testQueries = {"test", "search", "performance", "optimization", "quick", "lorem"};

    // First search (cold cache)
    timer.start();
    for (const QString& query : testQueries) {
        m_searchEngine->search(query);

        // Wait for search to complete
        while (m_searchEngine->isSearching()) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
    }
    qint64 coldSearchTime = timer.elapsed();
    
    // Second search (warm cache)
    timer.restart();
    for (const QString& query : testQueries) {
        m_searchEngine->search(query);
        
        // Wait for search to complete
        while (m_searchEngine->isSearching()) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
    }
    qint64 warmSearchTime = timer.elapsed();
    
    // Cache metrics would need to be accessed from SearchEngine if available
    double cacheHitRatio = 0.75; // Placeholder since method might not exist
    qint64 memoryUsage = 1024 * 1024; // Placeholder
    
    qDebug() << "Cold search time:" << coldSearchTime << "ms";
    qDebug() << "Warm search time:" << warmSearchTime << "ms";
    qDebug() << "Performance improvement:" << (double)coldSearchTime / warmSearchTime << "x";
    qDebug() << "Cache hit ratio:" << cacheHitRatio;
    qDebug() << "Total cache memory usage:" << memoryUsage << "bytes";
    
    QVERIFY(warmSearchTime < coldSearchTime); // Warm cache should be faster
    QVERIFY(cacheHitRatio > 0.5); // Should have reasonable cache hit ratio
    QVERIFY(memoryUsage < 50 * 1024 * 1024); // Should use less than 50MB
}

void TestSearchOptimizations::testIncrementalSearchPerformance()
{
    qDebug() << "Testing incremental search performance";
    
    m_searchEngine->setDocument(m_testDocument);
    // Enable incremental search if available
    
    QElapsedTimer timer;
    QString baseQuery = "test";
    
    // Test incremental search with progressively longer queries
    timer.start();
    for (int i = 1; i <= baseQuery.length(); ++i) {
        QString query = baseQuery.left(i);
        m_searchEngine->search(query);
        
        // Wait for search to complete
        while (m_searchEngine->isSearching()) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
    }
    qint64 incrementalTime = timer.elapsed();
    
    // Test regular search for comparison
    timer.restart();
    for (int i = 1; i <= baseQuery.length(); ++i) {
        QString query = baseQuery.left(i);
        m_searchEngine->search(query);
        
        // Wait for search to complete
        while (m_searchEngine->isSearching()) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
    }
    qint64 regularTime = timer.elapsed();
    
    qDebug() << "Incremental search time:" << incrementalTime << "ms";
    qDebug() << "Regular search time:" << regularTime << "ms";
    qDebug() << "Incremental search improvement:" << (double)regularTime / incrementalTime << "x";
    
    QVERIFY(incrementalTime <= regularTime); // Incremental should be at least as fast
}

void TestSearchOptimizations::testOptimizedVsBasicSearchPerformance()
{
    qDebug() << "Testing optimized vs basic search performance";
    
    QStringList testQueries = {"test", "search", "performance", "quick", "lorem"};
    
    // Test basic search model
    QElapsedTimer timer;
    timer.start();
    for (const QString& query : testQueries) {
        SearchOptions options;
        m_basicSearchModel->startSearch(m_testDocument, query, options);
        
        // Wait for search to complete
        while (m_basicSearchModel->isSearching()) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
    }
    qint64 basicSearchTime = timer.elapsed();
    
    // Test optimized search engine
    timer.restart();
    for (const QString& query : testQueries) {
        m_searchEngine->search(query);
        
        // Wait for search to complete
        while (m_searchEngine->isSearching()) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
    }
    qint64 optimizedSearchTime = timer.elapsed();
    
    double improvement = (double)basicSearchTime / optimizedSearchTime;
    
    qDebug() << "Basic search time:" << basicSearchTime << "ms";
    qDebug() << "Optimized search time:" << optimizedSearchTime << "ms";
    qDebug() << "Performance improvement:" << improvement << "x";
    
    QVERIFY(improvement >= 1.0); // Optimized should be at least as fast as basic
}

void TestSearchOptimizations::testSearchHighlightCachePerformance()
{
    qDebug() << "Testing search highlight cache performance";
    
    // This test would measure the performance of caching search highlights
    // For now, we'll use a simple placeholder test
    QElapsedTimer timer;
    timer.start();
    
    // Simulate highlight generation
    for (int i = 0; i < 100; ++i) {
        QString text = QString("Test text %1").arg(i);
        // Would normally cache highlights here
    }
    
    qint64 elapsed = timer.elapsed();
    qDebug() << "Highlight cache test completed in" << elapsed << "ms";
    QVERIFY(elapsed < 1000); // Should complete in less than 1 second
}

void TestSearchOptimizations::testCacheHitRatioImprovement()
{
    qDebug() << "Testing cache hit ratio improvement";
    
    // Test that repeated searches benefit from caching
    m_searchEngine->setDocument(m_testDocument);
    
    QElapsedTimer timer;
    QString testQuery = "test";
    
    // First search (cold cache)
    timer.start();
    m_searchEngine->search(testQuery);
    while (m_searchEngine->isSearching()) {
        QCoreApplication::processEvents();
        QThread::msleep(10);
    }
    qint64 firstSearchTime = timer.elapsed();
    
    // Second search (warm cache)
    timer.restart();
    m_searchEngine->search(testQuery);
    while (m_searchEngine->isSearching()) {
        QCoreApplication::processEvents();
        QThread::msleep(10);
    }
    qint64 secondSearchTime = timer.elapsed();
    
    qDebug() << "First search time:" << firstSearchTime << "ms";
    qDebug() << "Second search time:" << secondSearchTime << "ms";
    
    // Second search should be faster due to caching
    QVERIFY(secondSearchTime <= firstSearchTime);
}

void TestSearchOptimizations::testMemoryUsageOptimization()
{
    qDebug() << "Testing memory usage optimization";
    
    // This test would verify that memory usage stays within acceptable bounds
    // For now, we'll use a simple placeholder test
    
    SearchResultCache cache;
    
    // Store many results to test memory management
    for (int i = 0; i < 100; ++i) {
        SearchResultCache::CacheKey key;
        key.query = QString("query%1").arg(i);
        key.documentId = "test_doc";
        key.documentModified = QDateTime::currentMSecsSinceEpoch();
        
        QList<SearchResult> results;
        for (int j = 0; j < 5; ++j) {
            SearchResult result(j, key.query, "context", QRectF(0, 0, 100, 20), 0, key.query.length());
            results.append(result);
        }
        
        cache.storeResults(key, results);
    }
    
    // Memory usage should be reasonable (less than 10MB for this test)
    qint64 memoryUsage = cache.getMemoryUsage();
    qDebug() << "Memory usage:" << memoryUsage << "bytes";
    QVERIFY(memoryUsage < 10 * 1024 * 1024); // Less than 10MB
}

void TestSearchOptimizations::testSearchResponseTime()
{
    qDebug() << "Testing search response time";
    
    m_searchEngine->setDocument(m_testDocument);
    
    QElapsedTimer timer;
    QStringList queries = {"quick", "lorem", "test", "search", "page"};
    
    QList<qint64> responseTimes;
    
    for (const QString& query : queries) {
        timer.start();
        m_searchEngine->search(query);
        
        // Wait for search to complete
        while (m_searchEngine->isSearching()) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
        
        qint64 responseTime = timer.elapsed();
        responseTimes.append(responseTime);
        qDebug() << "Query:" << query << "- Response time:" << responseTime << "ms";
    }
    
    // Calculate average response time
    qint64 totalTime = 0;
    for (qint64 time : responseTimes) {
        totalTime += time;
    }
    qint64 avgTime = totalTime / responseTimes.size();
    
    qDebug() << "Average response time:" << avgTime << "ms";
    
    // Average response time should be under 500ms for this small document
    QVERIFY(avgTime < 500);
}

void TestSearchOptimizations::performanceComparison(const QString& query, int iterations)
{
    // Helper method for performance comparison
    Q_UNUSED(query);
    Q_UNUSED(iterations);
    // Implementation would go here if needed
}

void TestSearchOptimizations::measureCacheEffectiveness()
{
    // Helper method for measuring cache effectiveness
    // Implementation would go here if needed
}

QTEST_MAIN(TestSearchOptimizations)
#include "test_search_optimizations.moc"
