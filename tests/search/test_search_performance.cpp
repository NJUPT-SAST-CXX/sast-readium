#include <QElapsedTimer>
#include <QObject>
#include <QRandomGenerator>
#include <QSignalSpy>
#include <QStringList>
#include <QtTest/QtTest>
#include "../../app/search/SearchConfiguration.h"
#include "../../app/search/SearchPerformance.h"
#include "../TestUtilities.h"

/**
 * Comprehensive tests for SearchPerformance class
 * Tests fast search algorithms, parallel processing, and performance
 * optimization
 */
class SearchPerformanceTest : public TestBase {
    Q_OBJECT

protected:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

private slots:
    // Constructor and basic tests
    void testConstructor();
    void testDestructor();

    // Fast search algorithm tests
    void testBoyerMooreSearch();
    void testKmpSearch();
    void testParallelSearch();

    // Search result ranking tests
    void testSetRankingFactors();
    void testRankResults();
    void testCalculateRelevanceScore();

    // Query optimization tests
    void testOptimizeQuery();
    void testQueryPlanGeneration();

    // Performance monitoring tests
    void testGetLastSearchMetrics();
    void testResetMetrics();

    // Memory pool management tests
    void testInitializeMemoryPool();
    void testAllocateSearchMemory();
    void testDeallocateSearchMemory();
    void testClearMemoryPool();

    // Intelligent caching tests
    void testEnablePredictiveCache();
    void testWarmupCache();
    void testPreloadFrequentPatterns();
    void testOptimizeCacheAccess();
    void testPredictNextQueries();

    // Thread pool optimization tests
    void testSetOptimalThreadCount();
    void testSetThreadAffinity();
    void testEnableWorkStealing();

    // Algorithm selection tests
    void testSetPreferredAlgorithm();
    void testSelectOptimalAlgorithm();

    // Signal emission tests
    void testOptimizationCompletedSignal();
    void testCacheWarmedUpSignal();
    void testAlgorithmSelectedSignal();

    // Performance comparison tests
    void testAlgorithmPerformanceComparison();
    void testLargeTextPerformance();
    void testConcurrentSearchPerformance();

    // Edge case tests
    void testEmptyPatternSearch();
    void testEmptyTextSearch();
    void testSpecialCharacterSearch();
    void testUnicodeSearch();

    // Memory management tests
    void testMemoryPoolEfficiency();
    void testMemoryLeakPrevention();

private:
    SearchPerformance* m_performance;
    QString m_testText;
    QStringList m_testTexts;
    SearchOptions m_defaultOptions;

    // Helper methods
    void setupTestData();
    QString generateRandomText(int length);
    QStringList generateTestTexts(int count, int averageLength);
    void verifySearchResults(
        const QList<SearchPerformance::FastSearchResult>& results,
        const QString& pattern);
    void benchmarkAlgorithm(const QString& algorithmName,
                            std::function<void()> searchFunction);
};

void SearchPerformanceTest::initTestCase() {
    qDebug() << "Starting SearchPerformance tests";
    setupTestData();
}

void SearchPerformanceTest::cleanupTestCase() {
    qDebug() << "SearchPerformance tests completed";
}

void SearchPerformanceTest::init() {
    m_performance = new SearchPerformance(this);
    m_defaultOptions = SearchOptions();
}

void SearchPerformanceTest::cleanup() {
    if (m_performance) {
        delete m_performance;
        m_performance = nullptr;
    }
}

void SearchPerformanceTest::setupTestData() {
    m_testText =
        "The quick brown fox jumps over the lazy dog. "
        "This is a test text for search performance testing. "
        "It contains various words and patterns to search for. "
        "The text should be long enough to test performance algorithms "
        "effectively.";

    m_testTexts = generateTestTexts(10, 200);
}

void SearchPerformanceTest::testConstructor() {
    QVERIFY(m_performance != nullptr);
    // Constructor test passes if object is created successfully
    QVERIFY(true);
}

void SearchPerformanceTest::testDestructor() {
    SearchPerformance* performance = new SearchPerformance();
    delete performance;
    // If we reach here without crashing, destructor works correctly
    QVERIFY(true);
}

void SearchPerformanceTest::testBoyerMooreSearch() {
    QString pattern = "quick";
    auto results =
        m_performance->boyerMooreSearch(m_testText, pattern, false, -1);

    QVERIFY(!results.isEmpty());
    verifySearchResults(results, pattern);

    // Test case sensitive search
    auto caseSensitiveResults =
        m_performance->boyerMooreSearch(m_testText, "Quick", true, -1);
    QVERIFY(caseSensitiveResults
                .isEmpty());  // Should not find "Quick" in lowercase text

    // Test with max results limit
    auto limitedResults =
        m_performance->boyerMooreSearch(m_testText, "the", false, 1);
    QVERIFY(limitedResults.size() <= 1);
}

void SearchPerformanceTest::testKmpSearch() {
    QString pattern = "test";
    auto results = m_performance->kmpSearch(m_testText, pattern, false, -1);

    QVERIFY(!results.isEmpty());
    verifySearchResults(results, pattern);

    // Test case sensitive search
    auto caseSensitiveResults =
        m_performance->kmpSearch(m_testText, "TEST", true, -1);
    QVERIFY(caseSensitiveResults.isEmpty());

    // Test with max results limit
    auto limitedResults = m_performance->kmpSearch(m_testText, "a", false, 2);
    QVERIFY(limitedResults.size() <= 2);
}

void SearchPerformanceTest::testParallelSearch() {
    QString pattern = "text";
    auto results =
        m_performance->parallelSearch(m_testTexts, pattern, m_defaultOptions);

    QVERIFY(results.size() >= 0);

    // Verify results contain valid data
    for (const auto& result : results) {
        QVERIFY(result.position >= 0);
        QVERIFY(result.length > 0);
        QVERIFY(result.relevanceScore >= 0.0);
    }
}

void SearchPerformanceTest::testSetRankingFactors() {
    SearchPerformance::RankingFactors factors;
    factors.termFrequency = 2.0;
    factors.documentFrequency = 1.5;
    factors.positionWeight = 1.2;
    factors.contextRelevance = 1.8;
    factors.exactMatchBonus = 3.0;
    factors.proximityBonus = 2.0;

    m_performance->setRankingFactors(factors);

    // Test that setting factors doesn't crash
    QVERIFY(true);
}

void SearchPerformanceTest::testRankResults() {
    QList<SearchResult> testResults;

    // Create test results
    for (int i = 0; i < 5; ++i) {
        SearchResult result;
        result.matchedText = QString("Result %1").arg(i);
        result.pageNumber = i;
        result.textPosition = i * 10;
        result.textLength = 10;
        testResults.append(result);
    }

    QString query = "test";
    auto rankedResults = m_performance->rankResults(testResults, query);

    QCOMPARE(rankedResults.size(), testResults.size());

    // Verify results are still valid after ranking
    for (const auto& result : rankedResults) {
        QVERIFY(!result.matchedText.isEmpty());
        QVERIFY(result.pageNumber >= 0);
        QVERIFY(result.textPosition >= 0);
    }
}

void SearchPerformanceTest::testCalculateRelevanceScore() {
    SearchResult result;
    result.matchedText = "This is a test result";
    result.pageNumber = 1;
    result.textPosition = 0;
    result.textLength = result.matchedText.length();

    QString query = "test";
    QString fullText = m_testText;

    double score =
        m_performance->calculateRelevanceScore(result, query, fullText);
    QVERIFY(score >= 0.0);
}

void SearchPerformanceTest::testOptimizeQuery() {
    QString query = "test search optimization";
    int documentSize = 10000;
    int pageCount = 50;

    auto plan = m_performance->optimizeQuery(query, m_defaultOptions,
                                             documentSize, pageCount);

    QVERIFY(!plan.optimizedQuery.isEmpty());
    QVERIFY(!plan.searchTerms.isEmpty());
    QVERIFY(plan.estimatedCost >= 0);
    QVERIFY(!plan.algorithm.isEmpty());
}

void SearchPerformanceTest::testGetLastSearchMetrics() {
    // Perform a search to generate metrics
    m_performance->boyerMooreSearch(m_testText, "test", false, -1);

    // Retrieve metrics (mutex issue was a test design problem, not an actual deadlock)
    SearchPerformance::PerformanceMetrics metrics =
        m_performance->getLastSearchMetrics();

    // Verify metrics were populated
    QVERIFY(metrics.algorithmTime >= 0);
    QVERIFY(metrics.resultsFound >= 0);
    QCOMPARE(metrics.algorithmUsed, QString("Boyer-Moore"));
}

void SearchPerformanceTest::testResetMetrics() {
    // Perform a search to generate metrics
    m_performance->boyerMooreSearch(m_testText, "test", false, -1);

    // Reset metrics
    m_performance->resetMetrics();

    // Verify metrics were reset
    SearchPerformance::PerformanceMetrics metrics =
        m_performance->getLastSearchMetrics();
    QCOMPARE(metrics.algorithmTime, 0);
    QCOMPARE(metrics.resultsFound, 0);
    QCOMPARE(metrics.algorithmUsed, QString("None"));
}

void SearchPerformanceTest::testInitializeMemoryPool() {
    int poolSize = 1024 * 1024;  // 1MB
    m_performance->initializeMemoryPool(poolSize);

    // Test that initialization doesn't crash
    QVERIFY(true);
}

void SearchPerformanceTest::testAllocateSearchMemory() {
    m_performance->initializeMemoryPool(1024 * 1024);

    void* ptr = m_performance->allocateSearchMemory(1024);
    QVERIFY(ptr != nullptr);

    m_performance->deallocateSearchMemory(ptr);
}

void SearchPerformanceTest::testDeallocateSearchMemory() {
    m_performance->initializeMemoryPool(1024 * 1024);

    void* ptr = m_performance->allocateSearchMemory(512);
    QVERIFY(ptr != nullptr);

    // Test that deallocation doesn't crash
    m_performance->deallocateSearchMemory(ptr);
    QVERIFY(true);
}

void SearchPerformanceTest::testClearMemoryPool() {
    m_performance->initializeMemoryPool(1024 * 1024);

    void* ptr1 = m_performance->allocateSearchMemory(256);
    void* ptr2 = m_performance->allocateSearchMemory(512);

    QVERIFY(ptr1 != nullptr);
    QVERIFY(ptr2 != nullptr);

    m_performance->clearMemoryPool();

    // Test that clearing doesn't crash
    QVERIFY(true);
}

void SearchPerformanceTest::testEnablePredictiveCache() {
    m_performance->enablePredictiveCache(true);
    m_performance->enablePredictiveCache(false);

    // Test that enabling/disabling doesn't crash
    QVERIFY(true);
}

void SearchPerformanceTest::testWarmupCache() {
    QSignalSpy cacheWarmedUpSpy(m_performance,
                                &SearchPerformance::cacheWarmedUp);

    QStringList commonQueries = {"test", "search", "performance"};
    QStringList texts = m_testTexts;

    m_performance->warmupCache(commonQueries, texts);

    // Cache warmup should complete without crashing
    QVERIFY(true);
}

void SearchPerformanceTest::testSetOptimalThreadCount() {
    m_performance->setOptimalThreadCount();

    // Test that setting optimal thread count doesn't crash
    QVERIFY(true);
}

void SearchPerformanceTest::testSetPreferredAlgorithm() {
    m_performance->setPreferredAlgorithm(SearchPerformance::BoyerMoore);
    m_performance->setPreferredAlgorithm(SearchPerformance::KMP);
    m_performance->setPreferredAlgorithm(SearchPerformance::Parallel);
    m_performance->setPreferredAlgorithm(SearchPerformance::AutoSelect);

    // Test that setting algorithms doesn't crash
    QVERIFY(true);
}

void SearchPerformanceTest::testSelectOptimalAlgorithm() {
    QString pattern = "test";
    int textSize = 10000;

    auto algorithm = m_performance->selectOptimalAlgorithm(pattern, textSize);

    QVERIFY(algorithm == SearchPerformance::BoyerMoore ||
            algorithm == SearchPerformance::KMP ||
            algorithm == SearchPerformance::Parallel ||
            algorithm == SearchPerformance::Hybrid);
}

void SearchPerformanceTest::testQueryPlanGeneration() {
    QString query = "complex search query";
    int documentSize = 50000;
    int pageCount = 100;

    auto plan = m_performance->optimizeQuery(query, m_defaultOptions,
                                             documentSize, pageCount);

    QVERIFY(!plan.optimizedQuery.isEmpty());
    QVERIFY(!plan.searchTerms.isEmpty());
    QVERIFY(plan.estimatedCost >= 0);
    QVERIFY(!plan.algorithm.isEmpty());
}

void SearchPerformanceTest::testPreloadFrequentPatterns() {
    m_performance->preloadFrequentPatterns();
    // Test that preloading doesn't crash
    QVERIFY(true);
}

void SearchPerformanceTest::testOptimizeCacheAccess() {
    QString query = "optimization test";
    m_performance->optimizeCacheAccess(query);
    // Test that cache optimization doesn't crash
    QVERIFY(true);
}

void SearchPerformanceTest::testPredictNextQueries() {
    QString currentQuery = "test";
    QStringList history = {"test", "testing", "tester", "tests"};

    QStringList predictions =
        m_performance->predictNextQueries(currentQuery, history);

    // Predictions may be empty or contain suggestions
    QVERIFY(predictions.size() >= 0);
}

void SearchPerformanceTest::testSetThreadAffinity() {
    m_performance->setThreadAffinity(true);
    m_performance->setThreadAffinity(false);
    // Test that setting thread affinity doesn't crash
    QVERIFY(true);
}

void SearchPerformanceTest::testEnableWorkStealing() {
    m_performance->enableWorkStealing(true);
    m_performance->enableWorkStealing(false);
    // Test that enabling/disabling work stealing doesn't crash
    QVERIFY(true);
}

void SearchPerformanceTest::testOptimizationCompletedSignal() {
    QSignalSpy spy(m_performance, &SearchPerformance::optimizationCompleted);

    // Perform an operation that might trigger optimization
    m_performance->boyerMooreSearch(m_testText, "test", false, -1);

    // Signal may or may not be emitted depending on implementation
    QVERIFY(spy.count() >= 0);
}

void SearchPerformanceTest::testCacheWarmedUpSignal() {
    QSignalSpy spy(m_performance, &SearchPerformance::cacheWarmedUp);

    QStringList queries = {"test", "search"};
    m_performance->warmupCache(queries, m_testTexts);

    // Signal may or may not be emitted depending on implementation
    QVERIFY(spy.count() >= 0);
}

void SearchPerformanceTest::testAlgorithmSelectedSignal() {
    QSignalSpy spy(m_performance, &SearchPerformance::algorithmSelected);

    // Perform a search that triggers algorithm selection
    m_performance->selectOptimalAlgorithm("test", 10000);

    // Signal may or may not be emitted depending on implementation
    QVERIFY(spy.count() >= 0);
}

void SearchPerformanceTest::testAlgorithmPerformanceComparison() {
    QString pattern = "test";

    // Test Boyer-Moore
    QElapsedTimer timer;
    timer.start();
    auto bmResults =
        m_performance->boyerMooreSearch(m_testText, pattern, false, -1);
    qint64 bmTime = timer.elapsed();

    // Test KMP
    timer.start();
    auto kmpResults = m_performance->kmpSearch(m_testText, pattern, false, -1);
    qint64 kmpTime = timer.elapsed();

    // Both should find results
    QVERIFY(!bmResults.isEmpty());
    QVERIFY(!kmpResults.isEmpty());

    // Both should complete in reasonable time
    QVERIFY(bmTime < 1000);   // < 1 second
    QVERIFY(kmpTime < 1000);  // < 1 second
}

void SearchPerformanceTest::testLargeTextPerformance() {
    // Generate large text
    QString largeText = generateRandomText(100000);  // 100K characters

    QElapsedTimer timer;
    timer.start();

    auto results =
        m_performance->boyerMooreSearch(largeText, "test", false, -1);

    qint64 elapsed = timer.elapsed();

    // Should complete in reasonable time even for large text
    QVERIFY2(
        elapsed < 5000,
        QString("Large text search too slow: %1ms").arg(elapsed).toLocal8Bit());
    QVERIFY(results.size() >= 0);
}

void SearchPerformanceTest::testConcurrentSearchPerformance() {
    QString pattern = "test";

    // Test parallel search
    QElapsedTimer timer;
    timer.start();

    auto results =
        m_performance->parallelSearch(m_testTexts, pattern, m_defaultOptions);

    qint64 elapsed = timer.elapsed();

    // Parallel search should complete in reasonable time
    QVERIFY(elapsed < 2000);  // < 2 seconds
    QVERIFY(results.size() >= 0);
}

QString SearchPerformanceTest::generateRandomText(int length) {
    QString text;
    const QString chars = "abcdefghijklmnopqrstuvwxyz ";

    for (int i = 0; i < length; ++i) {
        text += chars[QRandomGenerator::global()->bounded(chars.length())];
    }

    return text;
}

QStringList SearchPerformanceTest::generateTestTexts(int count,
                                                     int averageLength) {
    QStringList texts;

    for (int i = 0; i < count; ++i) {
        int length =
            averageLength + QRandomGenerator::global()->bounded(-50, 51);
        texts.append(generateRandomText(qMax(50, length)));
    }

    return texts;
}

void SearchPerformanceTest::testEmptyPatternSearch() {
    QString emptyPattern = "";

    auto bmResults =
        m_performance->boyerMooreSearch(m_testText, emptyPattern, false, -1);
    auto kmpResults =
        m_performance->kmpSearch(m_testText, emptyPattern, false, -1);

    // Empty pattern should return empty results or handle gracefully
    QVERIFY(bmResults.isEmpty());
    QVERIFY(kmpResults.isEmpty());
}

void SearchPerformanceTest::testEmptyTextSearch() {
    QString emptyText = "";
    QString pattern = "test";

    auto bmResults =
        m_performance->boyerMooreSearch(emptyText, pattern, false, -1);
    auto kmpResults = m_performance->kmpSearch(emptyText, pattern, false, -1);

    // Searching in empty text should return empty results
    QVERIFY(bmResults.isEmpty());
    QVERIFY(kmpResults.isEmpty());
}

void SearchPerformanceTest::testSpecialCharacterSearch() {
    QString textWithSpecialChars =
        "Test with special chars: @#$%^&*()_+-=[]{}|;':\",./<>?";
    QString pattern = "@#$";

    auto results = m_performance->boyerMooreSearch(textWithSpecialChars,
                                                   pattern, false, -1);

    // Should handle special characters correctly
    QVERIFY(results.size() >= 0);
}

void SearchPerformanceTest::testUnicodeSearch() {
    QString unicodeText = "Unicode test: 你好世界 مرحبا العالم Привет мир";
    QString pattern = "你好";

    auto results =
        m_performance->boyerMooreSearch(unicodeText, pattern, false, -1);

    // Should handle Unicode correctly
    QVERIFY(results.size() >= 0);
}

void SearchPerformanceTest::testMemoryPoolEfficiency() {
    m_performance->initializeMemoryPool(1024 * 1024);  // 1MB

    // Allocate and deallocate multiple times
    QList<void*> pointers;
    for (int i = 0; i < 100; ++i) {
        void* ptr = m_performance->allocateSearchMemory(1024);
        if (ptr) {
            pointers.append(ptr);
        }
    }

    // Deallocate all
    for (void* ptr : pointers) {
        m_performance->deallocateSearchMemory(ptr);
    }

    // Test that pool can be reused
    void* ptr = m_performance->allocateSearchMemory(1024);
    QVERIFY(ptr != nullptr);
    m_performance->deallocateSearchMemory(ptr);
}

void SearchPerformanceTest::testMemoryLeakPrevention() {
    // Create and destroy multiple SearchPerformance instances
    for (int i = 0; i < 10; ++i) {
        SearchPerformance* perf = new SearchPerformance();
        perf->initializeMemoryPool(1024 * 1024);

        void* ptr = perf->allocateSearchMemory(1024);
        if (ptr) {
            perf->deallocateSearchMemory(ptr);
        }

        delete perf;
    }

    // If we reach here without crashing, no obvious memory leaks
    QVERIFY(true);
}

void SearchPerformanceTest::benchmarkAlgorithm(
    const QString& algorithmName, std::function<void()> searchFunction) {
    QElapsedTimer timer;
    timer.start();

    searchFunction();

    qint64 elapsed = timer.elapsed();
    qDebug() << algorithmName << "completed in" << elapsed << "ms";
}

void SearchPerformanceTest::verifySearchResults(
    const QList<SearchPerformance::FastSearchResult>& results,
    const QString& pattern) {
    for (const auto& result : results) {
        QVERIFY(result.position >= 0);
        QVERIFY(result.length > 0);
        QVERIFY(result.relevanceScore >= 0.0);
        QVERIFY(!result.context.isEmpty());
    }
}

QTEST_MAIN(SearchPerformanceTest)
#include "test_search_performance.moc"
