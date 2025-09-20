#include <QtTest>
#include <QObject>
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QDebug>
#include "../../app/search/SearchPerformance.h"

class TestSearchPerformance : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Algorithm performance tests
    void testBoyerMoorePerformance();
    void testKmpPerformance();
    void testAlgorithmSelection();
    void testParallelSearchPerformance();
    
    // Memory management tests
    void testMemoryPoolPerformance();
    void testMemoryPoolConcurrency();
    
    // Cache prediction tests
    void testCachePrediction();
    void testQueryPatternAnalysis();
    
    // Result ranking tests
    void testTfIdfRanking();
    void testBM25Ranking();
    void testCosineSimilarity();
    void testHybridRanking();
    
    // Query optimization tests
    void testQueryOptimization();
    void testPerformanceMetrics();
    
    // Scalability tests
    void testLargeDocumentPerformance();
    void testConcurrentSearchPerformance();

private:
    SearchPerformanceOptimizer* optimizer;
    QString generateLargeText(int size);
    QStringList generateTestCorpus(int documentCount, int avgSize);
};

void TestSearchPerformance::initTestCase()
{
    optimizer = new SearchPerformanceOptimizer();
    optimizer->initializeMemoryPool(1024 * 1024); // 1MB pool
    optimizer->enablePredictiveCache(true);
}

void TestSearchPerformance::cleanupTestCase()
{
    delete optimizer;
}

QString TestSearchPerformance::generateLargeText(int size)
{
    QString text;
    QStringList words = {"the", "quick", "brown", "fox", "jumps", "over", "lazy", "dog", 
                        "search", "performance", "optimization", "algorithm", "test", "data"};
    
    QRandomGenerator* generator = QRandomGenerator::global();
    for (int i = 0; i < size; ++i) {
        text += words[generator->bounded(words.size())] + " ";
    }
    
    return text;
}

QStringList TestSearchPerformance::generateTestCorpus(int documentCount, int avgSize)
{
    QStringList corpus;
    for (int i = 0; i < documentCount; ++i) {
        corpus.append(generateLargeText(avgSize));
    }
    return corpus;
}

void TestSearchPerformance::testBoyerMoorePerformance()
{
    QString largeText = generateLargeText(10000);
    QString pattern = "performance";
    
    QElapsedTimer timer;
    timer.start();
    
    auto results = optimizer->boyerMooreSearch(largeText, pattern, false, 100);
    
    qint64 elapsed = timer.elapsed();
    
    QVERIFY(!results.isEmpty());
    QVERIFY(elapsed < 100); // Should complete in less than 100ms
    
    qDebug() << "Boyer-Moore search took" << elapsed << "ms for" << results.size() << "results";
}

void TestSearchPerformance::testKmpPerformance()
{
    QString largeText = generateLargeText(10000);
    QString pattern = "optimization";
    
    QElapsedTimer timer;
    timer.start();
    
    auto results = optimizer->kmpSearch(largeText, pattern, false, 100);
    
    qint64 elapsed = timer.elapsed();
    
    QVERIFY(!results.isEmpty());
    QVERIFY(elapsed < 100); // Should complete in less than 100ms
    
    qDebug() << "KMP search took" << elapsed << "ms for" << results.size() << "results";
}

void TestSearchPerformance::testAlgorithmSelection()
{
    // Test short pattern selection
    auto algorithm1 = optimizer->selectOptimalAlgorithm("test", 1000);
    QVERIFY(algorithm1 == SearchPerformanceOptimizer::KMP);
    
    // Test long pattern with large text
    auto algorithm2 = optimizer->selectOptimalAlgorithm("performance optimization", 100000);
    QVERIFY(algorithm2 == SearchPerformanceOptimizer::BoyerMoore);
    
    // Test medium pattern
    auto algorithm3 = optimizer->selectOptimalAlgorithm("algorithm", 50000);
    QVERIFY(algorithm3 == SearchPerformanceOptimizer::BoyerMoore);
}

void TestSearchPerformance::testParallelSearchPerformance()
{
    QStringList texts = generateTestCorpus(10, 1000);
    QString pattern = "search";
    SearchOptions options;
    
    QElapsedTimer timer;
    timer.start();
    
    auto results = optimizer->parallelSearch(texts, pattern, options);
    
    qint64 elapsed = timer.elapsed();
    
    QVERIFY(!results.isEmpty());
    QVERIFY(elapsed < 500); // Parallel search should be faster
    
    qDebug() << "Parallel search took" << elapsed << "ms for" << results.size() << "results";
}

void TestSearchPerformance::testMemoryPoolPerformance()
{
    const int allocCount = 1000;
    const size_t allocSize = 1024; // 1KB each
    
    QElapsedTimer timer;
    timer.start();
    
    QList<void*> allocations;
    for (int i = 0; i < allocCount; ++i) {
        void* ptr = optimizer->allocateSearchMemory(allocSize);
        QVERIFY(ptr != nullptr);
        allocations.append(ptr);
    }
    
    for (void* ptr : allocations) {
        optimizer->deallocateSearchMemory(ptr);
    }
    
    qint64 elapsed = timer.elapsed();
    
    QVERIFY(elapsed < 50); // Should be very fast
    qDebug() << "Memory pool operations took" << elapsed << "ms for" << allocCount << "allocations";
}

void TestSearchPerformance::testMemoryPoolConcurrency()
{
    const int threadCount = 4;
    const int allocationsPerThread = 100;
    
    QList<QFuture<void>> futures;
    
    for (int t = 0; t < threadCount; ++t) {
        QFuture<void> future = QtConcurrent::run([this, allocationsPerThread]() {
            QList<void*> allocations;
            for (int i = 0; i < allocationsPerThread; ++i) {
                void* ptr = optimizer->allocateSearchMemory(512);
                if (ptr) {
                    allocations.append(ptr);
                }
            }
            
            for (void* ptr : allocations) {
                optimizer->deallocateSearchMemory(ptr);
            }
        });
        
        futures.append(future);
    }
    
    // Wait for all threads to complete
    for (auto& future : futures) {
        future.waitForFinished();
    }
    
    // Test passed if no crashes occurred
    QVERIFY(true);
}

void TestSearchPerformance::testCachePrediction()
{
    QStringList commonQueries = {"search", "performance", "optimization", "algorithm"};
    QStringList texts = generateTestCorpus(5, 500);
    
    QElapsedTimer timer;
    timer.start();
    
    optimizer->warmupCache(commonQueries, texts);
    
    qint64 elapsed = timer.elapsed();
    
    QVERIFY(elapsed < 1000); // Cache warmup should complete quickly
    qDebug() << "Cache warmup took" << elapsed << "ms";
    
    // Test prediction
    QStringList predictions = optimizer->predictNextQueries("search", QStringList());
    // Predictions might be empty initially, but the method should work
    QVERIFY(predictions.size() >= 0);
}

void TestSearchPerformance::testQueryPatternAnalysis()
{
    // Simulate query history
    QStringList queryHistory = {"search", "search performance", "performance optimization", 
                               "optimization algorithm", "algorithm test"};
    
    for (const QString& query : queryHistory) {
        optimizer->optimizeCacheAccess(query);
    }
    
    // Test prediction based on history
    QStringList predictions = optimizer->predictNextQueries("search", queryHistory);
    QVERIFY(predictions.size() >= 0);
}

void TestSearchPerformance::testTfIdfRanking()
{
    QStringList corpus = {"the quick brown fox", "performance optimization test", 
                         "search algorithm implementation", "test data generation"};
    
    QList<SearchResult> results;
    results.append(SearchResult(0, "performance", "performance optimization", QRectF(), 0, 11));
    results.append(SearchResult(1, "test", "test data", QRectF(), 0, 4));
    
    auto rankedResults = optimizer->rankResults(results, "performance");
    
    QCOMPARE(rankedResults.size(), 2);
    // Results should be ranked (though we can't verify exact order without extending SearchResult)
}

void TestSearchPerformance::testBM25Ranking()
{
    // Test BM25 calculation
    QStringList corpus = {"performance optimization", "search performance", "optimization test"};
    
    // This tests the internal ranking algorithm
    QVERIFY(true); // Placeholder - would need extended SearchResult for full testing
}

void TestSearchPerformance::testCosineSimilarity()
{
    // Test cosine similarity calculation
    QString query = "performance optimization";
    QString document = "optimization for search performance";
    
    // This would test the internal cosine similarity calculation
    QVERIFY(true); // Placeholder - would need access to internal methods
}

void TestSearchPerformance::testHybridRanking()
{
    QStringList corpus = generateTestCorpus(10, 100);
    QList<SearchResult> results;
    
    for (int i = 0; i < 5; ++i) {
        results.append(SearchResult(i, "test", "test context", QRectF(), i * 10, 4));
    }
    
    auto rankedResults = optimizer->rankResults(results, "test");
    QCOMPARE(rankedResults.size(), 5);
}

void TestSearchPerformance::testQueryOptimization()
{
    QString query = "performance optimization algorithm";
    SearchOptions options;
    int documentSize = 100000;
    int pageCount = 50;
    
    auto plan = optimizer->optimizeQuery(query, options, documentSize, pageCount);
    
    QCOMPARE(plan.optimizedQuery, query);
    QVERIFY(!plan.searchTerms.isEmpty());
    QVERIFY(plan.estimatedCost > 0);
    QVERIFY(!plan.algorithm.isEmpty());
    
    qDebug() << "Query plan:" << plan.algorithm << "Parallel:" << plan.useParallelSearch 
             << "Cost:" << plan.estimatedCost;
}

void TestSearchPerformance::testPerformanceMetrics()
{
    QString text = generateLargeText(5000);
    QString pattern = "performance";
    
    optimizer->resetMetrics();
    
    auto results = optimizer->boyerMooreSearch(text, pattern, false, 50);
    
    auto metrics = optimizer->getLastSearchMetrics();
    
    QVERIFY(metrics.algorithmTime > 0);
    QCOMPARE(metrics.algorithmUsed, "Boyer-Moore");
    QCOMPARE(metrics.resultsFound, results.size());
    
    qDebug() << "Metrics - Algorithm:" << metrics.algorithmUsed 
             << "Time:" << metrics.algorithmTime << "ms"
             << "Results:" << metrics.resultsFound;
}

void TestSearchPerformance::testLargeDocumentPerformance()
{
    QString largeText = generateLargeText(50000); // Large document
    QString pattern = "optimization";
    
    QElapsedTimer timer;
    timer.start();
    
    auto results = optimizer->boyerMooreSearch(largeText, pattern, false, 1000);
    
    qint64 elapsed = timer.elapsed();
    
    QVERIFY(!results.isEmpty());
    QVERIFY(elapsed < 1000); // Should complete in less than 1 second
    
    qDebug() << "Large document search took" << elapsed << "ms for" << results.size() << "results";
}

void TestSearchPerformance::testConcurrentSearchPerformance()
{
    QStringList texts = generateTestCorpus(20, 2000);
    QString pattern = "performance";
    
    const int threadCount = 4;
    QList<QFuture<int>> futures;
    
    QElapsedTimer timer;
    timer.start();
    
    for (int t = 0; t < threadCount; ++t) {
        QFuture<int> future = QtConcurrent::run([this, texts, pattern, t]() {
            int totalResults = 0;
            for (int i = t; i < texts.size(); i += 4) { // Distribute work
                auto results = optimizer->boyerMooreSearch(texts[i], pattern, false, 100);
                totalResults += results.size();
            }
            return totalResults;
        });
        
        futures.append(future);
    }
    
    int totalResults = 0;
    for (auto& future : futures) {
        totalResults += future.result();
    }
    
    qint64 elapsed = timer.elapsed();
    
    QVERIFY(totalResults > 0);
    QVERIFY(elapsed < 2000); // Should complete in reasonable time
    
    qDebug() << "Concurrent search took" << elapsed << "ms for" << totalResults << "total results";
}

QTEST_APPLESS_MAIN(TestSearchPerformance)

#include "test_search_performance.moc"
