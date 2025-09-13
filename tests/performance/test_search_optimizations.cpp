#include <QtTest/QtTest>
#include <QApplication>
#include <QElapsedTimer>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QPdfWriter>
#include <QPainter>
#include <QPageSize>
#include <poppler-qt6.h>
#include "../../app/search/OptimizedSearchEngine.h"
#include "../../app/model/SearchModel.h"
#include "../../app/cache/SearchResultCache.h"

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
    OptimizedSearchEngine* m_optimizedEngine;
    SearchModel* m_basicSearchModel;
    
    Poppler::Document* createTestDocument();
    void performanceComparison(const QString& query, int iterations = 10);
    void measureCacheEffectiveness();
};

void TestSearchOptimizations::initTestCase()
{
    m_testDocument = createTestDocument();
    QVERIFY(m_testDocument != nullptr);
    
    m_optimizedEngine = new OptimizedSearchEngine(this);
    m_basicSearchModel = new SearchModel(this);
    
    // Disable optimizations in basic model for comparison
    m_basicSearchModel->setOptimizedSearchEnabled(false);
    
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
    qDebug() << "Testing OptimizedSearchEngine performance";
    
    m_optimizedEngine->setDocument(m_testDocument);
    
    QElapsedTimer timer;
    QStringList testQueries = {"test", "search", "performance", "optimization", "quick", "lorem"};
    
    // First search (cold cache)
    timer.start();
    for (const QString& query : testQueries) {
        m_optimizedEngine->startSearch(m_testDocument, query);
        
        // Wait for search to complete
        while (m_optimizedEngine->isSearching()) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
    }
    qint64 coldSearchTime = timer.elapsed();
    
    // Second search (warm cache)
    timer.restart();
    for (const QString& query : testQueries) {
        m_optimizedEngine->startSearch(m_testDocument, query);
        
        // Wait for search to complete
        while (m_optimizedEngine->isSearching()) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
    }
    qint64 warmSearchTime = timer.elapsed();
    
    double cacheHitRatio = m_optimizedEngine->getCacheHitRatio();
    qint64 memoryUsage = m_optimizedEngine->getTotalCacheMemoryUsage();
    
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
    
    m_optimizedEngine->setDocument(m_testDocument);
    m_optimizedEngine->setIncrementalSearchEnabled(true);
    
    QElapsedTimer timer;
    QString baseQuery = "test";
    
    // Test incremental search with progressively longer queries
    timer.start();
    for (int i = 1; i <= baseQuery.length(); ++i) {
        QString query = baseQuery.left(i);
        m_optimizedEngine->startIncrementalSearch(m_testDocument, query);
        
        // Wait for search to complete
        while (m_optimizedEngine->isSearching()) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
    }
    qint64 incrementalTime = timer.elapsed();
    
    // Test regular search for comparison
    timer.restart();
    for (int i = 1; i <= baseQuery.length(); ++i) {
        QString query = baseQuery.left(i);
        m_optimizedEngine->startSearch(m_testDocument, query);
        
        // Wait for search to complete
        while (m_optimizedEngine->isSearching()) {
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
        SearchOptions options;
        m_optimizedEngine->startSearch(m_testDocument, query, options);
        
        // Wait for search to complete
        while (m_optimizedEngine->isSearching()) {
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

QTEST_MAIN(TestSearchOptimizations)
#include "test_search_optimizations.moc"
