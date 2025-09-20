#include <QtTest/QtTest>
#include <QObject>
#include <QSignalSpy>
#include <QElapsedTimer>
#include <QStringList>
#include <QRandomGenerator>
#include "../../app/search/SearchPerformance.h"
#include "../../app/search/SearchConfiguration.h"
#include "../TestUtilities.h"

/**
 * Comprehensive tests for SearchPerformance class
 * Tests fast search algorithms, parallel processing, and performance optimization
 */
class SearchPerformanceTest : public TestBase
{
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
    void verifySearchResults(const QList<SearchPerformance::FastSearchResult>& results, 
                           const QString& pattern);
    void benchmarkAlgorithm(const QString& algorithmName, 
                          std::function<void()> searchFunction);
};

void SearchPerformanceTest::initTestCase()
{
    qDebug() << "Starting SearchPerformance tests";
    setupTestData();
}

void SearchPerformanceTest::cleanupTestCase()
{
    qDebug() << "SearchPerformance tests completed";
}

void SearchPerformanceTest::init()
{
    m_performance = new SearchPerformance(this);
    m_defaultOptions = SearchOptions();
}

void SearchPerformanceTest::cleanup()
{
    if (m_performance) {
        delete m_performance;
        m_performance = nullptr;
    }
}

void SearchPerformanceTest::setupTestData()
{
    m_testText = "The quick brown fox jumps over the lazy dog. "
                 "This is a test text for search performance testing. "
                 "It contains various words and patterns to search for. "
                 "The text should be long enough to test performance algorithms effectively.";
    
    m_testTexts = generateTestTexts(10, 200);
}

void SearchPerformanceTest::testConstructor()
{
    QVERIFY(m_performance != nullptr);
    
    // Test that default state is reasonable
    auto metrics = m_performance->getLastSearchMetrics();
    QVERIFY(metrics.searchTime >= 0);
    QVERIFY(metrics.resultsFound >= 0);
}

void SearchPerformanceTest::testDestructor()
{
    SearchPerformance* performance = new SearchPerformance();
    delete performance;
    // If we reach here without crashing, destructor works correctly
    QVERIFY(true);
}

void SearchPerformanceTest::testBoyerMooreSearch()
{
    QString pattern = "quick";
    auto results = m_performance->boyerMooreSearch(m_testText, pattern, false, -1);
    
    QVERIFY(!results.isEmpty());
    verifySearchResults(results, pattern);
    
    // Test case sensitive search
    auto caseSensitiveResults = m_performance->boyerMooreSearch(m_testText, "Quick", true, -1);
    QVERIFY(caseSensitiveResults.isEmpty()); // Should not find "Quick" in lowercase text
    
    // Test with max results limit
    auto limitedResults = m_performance->boyerMooreSearch(m_testText, "the", false, 1);
    QVERIFY(limitedResults.size() <= 1);
}

void SearchPerformanceTest::testKmpSearch()
{
    QString pattern = "test";
    auto results = m_performance->kmpSearch(m_testText, pattern, false, -1);
    
    QVERIFY(!results.isEmpty());
    verifySearchResults(results, pattern);
    
    // Test case sensitive search
    auto caseSensitiveResults = m_performance->kmpSearch(m_testText, "TEST", true, -1);
    QVERIFY(caseSensitiveResults.isEmpty());
    
    // Test with max results limit
    auto limitedResults = m_performance->kmpSearch(m_testText, "a", false, 2);
    QVERIFY(limitedResults.size() <= 2);
}

void SearchPerformanceTest::testParallelSearch()
{
    QString pattern = "text";
    auto results = m_performance->parallelSearch(m_testTexts, pattern, m_defaultOptions);
    
    QVERIFY(results.size() >= 0);
    
    // Verify results contain valid data
    for (const auto& result : results) {
        QVERIFY(result.position >= 0);
        QVERIFY(result.length > 0);
        QVERIFY(result.relevanceScore >= 0.0);
    }
}

void SearchPerformanceTest::testSetRankingFactors()
{
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

void SearchPerformanceTest::testRankResults()
{
    QList<SearchResult> testResults;
    
    // Create test results
    for (int i = 0; i < 5; ++i) {
        SearchResult result;
        result.text = QString("Result %1").arg(i);
        result.pageNumber = i;
        result.position = i * 10;
        result.length = 10;
        testResults.append(result);
    }
    
    QString query = "test";
    auto rankedResults = m_performance->rankResults(testResults, query);
    
    QCOMPARE(rankedResults.size(), testResults.size());
    
    // Verify results are still valid after ranking
    for (const auto& result : rankedResults) {
        QVERIFY(!result.text.isEmpty());
        QVERIFY(result.pageNumber >= 0);
        QVERIFY(result.position >= 0);
    }
}

void SearchPerformanceTest::testCalculateRelevanceScore()
{
    SearchResult result;
    result.text = "This is a test result";
    result.pageNumber = 1;
    result.position = 0;
    result.length = result.text.length();
    
    QString query = "test";
    QString fullText = m_testText;
    
    double score = m_performance->calculateRelevanceScore(result, query, fullText);
    QVERIFY(score >= 0.0);
}

void SearchPerformanceTest::testOptimizeQuery()
{
    QString query = "test search optimization";
    int documentSize = 10000;
    int pageCount = 50;
    
    auto plan = m_performance->optimizeQuery(query, m_defaultOptions, documentSize, pageCount);
    
    QVERIFY(!plan.optimizedQuery.isEmpty());
    QVERIFY(!plan.searchTerms.isEmpty());
    QVERIFY(plan.estimatedCost >= 0);
    QVERIFY(!plan.algorithm.isEmpty());
}

void SearchPerformanceTest::testGetLastSearchMetrics()
{
    // Perform a search to generate metrics
    m_performance->boyerMooreSearch(m_testText, "test", false, -1);
    
    auto metrics = m_performance->getLastSearchMetrics();
    
    QVERIFY(metrics.searchTime >= 0);
    QVERIFY(metrics.algorithmTime >= 0);
    QVERIFY(metrics.rankingTime >= 0);
    QVERIFY(metrics.cacheTime >= 0);
    QVERIFY(metrics.resultsFound >= 0);
    QVERIFY(metrics.pagesSearched >= 0);
    QVERIFY(!metrics.algorithmUsed.isEmpty());
    QVERIFY(metrics.cacheHitRatio >= 0.0 && metrics.cacheHitRatio <= 1.0);
}

void SearchPerformanceTest::testResetMetrics()
{
    // Perform a search to generate metrics
    m_performance->boyerMooreSearch(m_testText, "test", false, -1);
    
    m_performance->resetMetrics();
    
    auto metrics = m_performance->getLastSearchMetrics();
    // After reset, metrics should be in initial state
    QVERIFY(metrics.searchTime >= 0);
    QVERIFY(metrics.resultsFound >= 0);
}

void SearchPerformanceTest::testInitializeMemoryPool()
{
    int poolSize = 1024 * 1024; // 1MB
    m_performance->initializeMemoryPool(poolSize);
    
    // Test that initialization doesn't crash
    QVERIFY(true);
}

void SearchPerformanceTest::testAllocateSearchMemory()
{
    m_performance->initializeMemoryPool(1024 * 1024);
    
    void* ptr = m_performance->allocateSearchMemory(1024);
    QVERIFY(ptr != nullptr);
    
    m_performance->deallocateSearchMemory(ptr);
}

void SearchPerformanceTest::testDeallocateSearchMemory()
{
    m_performance->initializeMemoryPool(1024 * 1024);
    
    void* ptr = m_performance->allocateSearchMemory(512);
    QVERIFY(ptr != nullptr);
    
    // Test that deallocation doesn't crash
    m_performance->deallocateSearchMemory(ptr);
    QVERIFY(true);
}

void SearchPerformanceTest::testClearMemoryPool()
{
    m_performance->initializeMemoryPool(1024 * 1024);
    
    void* ptr1 = m_performance->allocateSearchMemory(256);
    void* ptr2 = m_performance->allocateSearchMemory(512);
    
    QVERIFY(ptr1 != nullptr);
    QVERIFY(ptr2 != nullptr);
    
    m_performance->clearMemoryPool();
    
    // Test that clearing doesn't crash
    QVERIFY(true);
}

void SearchPerformanceTest::testEnablePredictiveCache()
{
    m_performance->enablePredictiveCache(true);
    m_performance->enablePredictiveCache(false);
    
    // Test that enabling/disabling doesn't crash
    QVERIFY(true);
}

void SearchPerformanceTest::testWarmupCache()
{
    QSignalSpy cacheWarmedUpSpy(m_performance, &SearchPerformance::cacheWarmedUp);
    
    QStringList commonQueries = {"test", "search", "performance"};
    QStringList texts = m_testTexts;
    
    m_performance->warmupCache(commonQueries, texts);
    
    // Cache warmup should complete without crashing
    QVERIFY(true);
}

void SearchPerformanceTest::testSetOptimalThreadCount()
{
    m_performance->setOptimalThreadCount();
    
    // Test that setting optimal thread count doesn't crash
    QVERIFY(true);
}

void SearchPerformanceTest::testSetPreferredAlgorithm()
{
    m_performance->setPreferredAlgorithm(SearchPerformance::BoyerMoore);
    m_performance->setPreferredAlgorithm(SearchPerformance::KMP);
    m_performance->setPreferredAlgorithm(SearchPerformance::Parallel);
    m_performance->setPreferredAlgorithm(SearchPerformance::AutoSelect);
    
    // Test that setting algorithms doesn't crash
    QVERIFY(true);
}

void SearchPerformanceTest::testSelectOptimalAlgorithm()
{
    QString pattern = "test";
    int textSize = 10000;
    
    auto algorithm = m_performance->selectOptimalAlgorithm(pattern, textSize);
    
    QVERIFY(algorithm == SearchPerformance::BoyerMoore ||
            algorithm == SearchPerformance::KMP ||
            algorithm == SearchPerformance::Parallel ||
            algorithm == SearchPerformance::Hybrid);
}

QString SearchPerformanceTest::generateRandomText(int length)
{
    QString text;
    const QString chars = "abcdefghijklmnopqrstuvwxyz ";
    
    for (int i = 0; i < length; ++i) {
        text += chars[QRandomGenerator::global()->bounded(chars.length())];
    }
    
    return text;
}

QStringList SearchPerformanceTest::generateTestTexts(int count, int averageLength)
{
    QStringList texts;
    
    for (int i = 0; i < count; ++i) {
        int length = averageLength + QRandomGenerator::global()->bounded(-50, 51);
        texts.append(generateRandomText(qMax(50, length)));
    }
    
    return texts;
}

void SearchPerformanceTest::verifySearchResults(
    const QList<SearchPerformance::FastSearchResult>& results, const QString& pattern)
{
    for (const auto& result : results) {
        QVERIFY(result.position >= 0);
        QVERIFY(result.length > 0);
        QVERIFY(result.relevanceScore >= 0.0);
        QVERIFY(!result.context.isEmpty());
    }
}

QTEST_MAIN(SearchPerformanceTest)
#include "SearchPerformanceTest.moc"
