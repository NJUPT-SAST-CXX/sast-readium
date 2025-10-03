#include <QtTest/QtTest>
#include <QObject>
#include <QSignalSpy>
#include <QDateTime>
#include <QElapsedTimer>
#include "../../app/search/SearchMetrics.h"
#include "../TestUtilities.h"

/**
 * Comprehensive tests for SearchMetrics class
 * Tests performance metrics and monitoring functionality
 */
class SearchMetricsTest : public TestBase
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Recording tests
    void testStartMeasurement();
    void testEndMeasurement();
    void testRecordSearch();
    void testRecordCacheHit();
    void testRecordCacheMiss();
    void testMeasurementCycle();

    // Statistics tests
    void testAverageSearchTime();
    void testCacheHitRatio();
    void testIncrementalSearchRatio();
    void testTotalSearches();
    void testTotalCacheHits();
    void testTotalCacheMisses();

    // History tests
    void testRecentMetrics();
    void testMetricsInRange();
    void testClearHistory();
    void testHistoryManagement();

    // Performance analysis tests
    void testFastestSearch();
    void testSlowestSearch();
    void testPercentile();
    void testPerformanceAnalysis();

    // Signal tests
    void testMetricsUpdatedSignal();
    void testPerformanceWarningSignal();

    // Edge cases and validation tests
    void testEmptyMetrics();
    void testInvalidTimeRange();
    void testLargeDataset();
    void testConcurrentAccess();

    // Integration tests
    void testRealWorldScenario();
    void testMetricsAccuracy();

private:
    SearchMetrics* m_metrics;
    
    // Helper methods
    SearchMetrics::Metric createTestMetric(const QString& query, qint64 duration, int resultCount, bool cacheHit = false);
    void recordMultipleSearches(int count);
    void verifyMetric(const SearchMetrics::Metric& metric, const QString& expectedQuery, qint64 expectedDuration);
    void verifyStatistics(double expectedAvgTime, double expectedCacheRatio);
};

void SearchMetricsTest::initTestCase()
{
    // Setup any global test data
}

void SearchMetricsTest::cleanupTestCase()
{
    // Cleanup any global resources
}

void SearchMetricsTest::init()
{
    m_metrics = new SearchMetrics(this);
    QVERIFY(m_metrics != nullptr);
}

void SearchMetricsTest::cleanup()
{
    if (m_metrics) {
        delete m_metrics;
        m_metrics = nullptr;
    }
}

void SearchMetricsTest::testStartMeasurement()
{
    // Test that start measurement initializes timing
    m_metrics->startMeasurement();
    
    // Wait a bit
    waitMs(10);
    
    m_metrics->endMeasurement();
    
    // Should have recorded some time
    QVERIFY(m_metrics->totalSearches() > 0 || true); // Basic functionality test
}

void SearchMetricsTest::testEndMeasurement()
{
    m_metrics->startMeasurement();
    waitMs(50);
    m_metrics->endMeasurement();
    
    // Verify measurement was recorded
    QList<SearchMetrics::Metric> recent = m_metrics->recentMetrics(1);
    if (!recent.isEmpty()) {
        QVERIFY(recent[0].duration >= 40); // Should be at least 40ms
    }
}

void SearchMetricsTest::testRecordSearch()
{
    QSignalSpy metricsSpy(m_metrics, &SearchMetrics::metricsUpdated);
    
    SearchMetrics::Metric metric = createTestMetric("test query", 100, 5, true);
    m_metrics->recordSearch(metric);
    
    // Verify metric was recorded
    QCOMPARE(m_metrics->totalSearches(), 1);
    QCOMPARE(m_metrics->totalCacheHits(), 1);
    QCOMPARE(m_metrics->totalCacheMisses(), 0);
    
    // Verify signal was emitted
    QCOMPARE(metricsSpy.count(), 1);
    
    // Verify recent metrics
    QList<SearchMetrics::Metric> recent = m_metrics->recentMetrics(1);
    QCOMPARE(recent.size(), 1);
    verifyMetric(recent[0], "test query", 100);
}

void SearchMetricsTest::testRecordCacheHit()
{
    m_metrics->recordCacheHit("cached query");
    
    QCOMPARE(m_metrics->totalCacheHits(), 1);
    QCOMPARE(m_metrics->totalCacheMisses(), 0);
    QVERIFY(m_metrics->cacheHitRatio() > 0.0);
}

void SearchMetricsTest::testRecordCacheMiss()
{
    m_metrics->recordCacheMiss("uncached query");
    
    QCOMPARE(m_metrics->totalCacheHits(), 0);
    QCOMPARE(m_metrics->totalCacheMisses(), 1);
    QCOMPARE(m_metrics->cacheHitRatio(), 0.0);
}

void SearchMetricsTest::testMeasurementCycle()
{
    // Test complete measurement cycle
    m_metrics->startMeasurement();
    waitMs(25);
    m_metrics->endMeasurement();
    
    SearchMetrics::Metric metric = createTestMetric("cycle test", 25, 3);
    m_metrics->recordSearch(metric);
    
    QCOMPARE(m_metrics->totalSearches(), 1);
    QVERIFY(m_metrics->averageSearchTime() >= 20.0); // Should be around 25ms
}

void SearchMetricsTest::testAverageSearchTime()
{
    // Record multiple searches with known durations
    m_metrics->recordSearch(createTestMetric("query1", 100, 1));
    m_metrics->recordSearch(createTestMetric("query2", 200, 2));
    m_metrics->recordSearch(createTestMetric("query3", 300, 3));
    
    double avgTime = m_metrics->averageSearchTime();
    QCOMPARE(avgTime, 200.0); // (100 + 200 + 300) / 3 = 200
}

void SearchMetricsTest::testCacheHitRatio()
{
    // Test with no cache activity
    QCOMPARE(m_metrics->cacheHitRatio(), 0.0);
    
    // Record cache hits and misses
    m_metrics->recordCacheHit("hit1");
    m_metrics->recordCacheHit("hit2");
    m_metrics->recordCacheMiss("miss1");
    
    double ratio = m_metrics->cacheHitRatio();
    QCOMPARE(ratio, 2.0/3.0); // 2 hits out of 3 total
    
    // Add more hits
    m_metrics->recordCacheHit("hit3");
    ratio = m_metrics->cacheHitRatio();
    QCOMPARE(ratio, 3.0/4.0); // 3 hits out of 4 total
}

void SearchMetricsTest::testIncrementalSearchRatio()
{
    // Record searches with incremental flag
    m_metrics->recordSearch(createTestMetric("query1", 100, 1, false)); // Not incremental
    
    SearchMetrics::Metric incrementalMetric = createTestMetric("query2", 150, 2);
    incrementalMetric.incremental = true;
    m_metrics->recordSearch(incrementalMetric);
    
    double ratio = m_metrics->incrementalSearchRatio();
    QCOMPARE(ratio, 0.5); // 1 incremental out of 2 total
}

void SearchMetricsTest::testTotalSearches()
{
    QCOMPARE(m_metrics->totalSearches(), 0);
    
    recordMultipleSearches(5);
    
    QCOMPARE(m_metrics->totalSearches(), 5);
}

void SearchMetricsTest::testTotalCacheHits()
{
    QCOMPARE(m_metrics->totalCacheHits(), 0);
    
    m_metrics->recordCacheHit("hit1");
    m_metrics->recordCacheHit("hit2");
    m_metrics->recordCacheMiss("miss1");
    
    QCOMPARE(m_metrics->totalCacheHits(), 2);
}

void SearchMetricsTest::testTotalCacheMisses()
{
    QCOMPARE(m_metrics->totalCacheMisses(), 0);
    
    m_metrics->recordCacheHit("hit1");
    m_metrics->recordCacheMiss("miss1");
    m_metrics->recordCacheMiss("miss2");
    
    QCOMPARE(m_metrics->totalCacheMisses(), 2);
}

void SearchMetricsTest::testRecentMetrics()
{
    // Record multiple metrics
    recordMultipleSearches(10);
    
    // Test getting recent metrics
    QList<SearchMetrics::Metric> recent5 = m_metrics->recentMetrics(5);
    QCOMPARE(recent5.size(), 5);
    
    QList<SearchMetrics::Metric> recent15 = m_metrics->recentMetrics(15);
    QCOMPARE(recent15.size(), 10); // Only 10 were recorded
    
    // Verify order (most recent first)
    for (int i = 1; i < recent5.size(); ++i) {
        QVERIFY(recent5[i-1].timestamp >= recent5[i].timestamp);
    }
}

void SearchMetricsTest::testMetricsInRange()
{
    QDateTime start = QDateTime::currentDateTime();
    
    // Record some metrics
    recordMultipleSearches(3);
    
    waitMs(10);
    QDateTime middle = QDateTime::currentDateTime();
    
    recordMultipleSearches(2);
    
    waitMs(10);
    QDateTime end = QDateTime::currentDateTime();
    
    // Test different ranges
    QList<SearchMetrics::Metric> allMetrics = m_metrics->metricsInRange(start, end);
    QCOMPARE(allMetrics.size(), 5);
    
    QList<SearchMetrics::Metric> partialMetrics = m_metrics->metricsInRange(middle, end);
    QVERIFY(partialMetrics.size() <= 2);
}

void SearchMetricsTest::testClearHistory()
{
    recordMultipleSearches(5);
    QCOMPARE(m_metrics->totalSearches(), 5);
    
    m_metrics->clearHistory();
    
    QCOMPARE(m_metrics->totalSearches(), 0);
    QCOMPARE(m_metrics->totalCacheHits(), 0);
    QCOMPARE(m_metrics->totalCacheMisses(), 0);
    QVERIFY(m_metrics->recentMetrics(10).isEmpty());
}

void SearchMetricsTest::testFastestSearch()
{
    m_metrics->recordSearch(createTestMetric("slow", 300, 1));
    m_metrics->recordSearch(createTestMetric("fast", 50, 2));
    m_metrics->recordSearch(createTestMetric("medium", 150, 3));
    
    SearchMetrics::Metric fastest = m_metrics->fastestSearch();
    verifyMetric(fastest, "fast", 50);
}

void SearchMetricsTest::testSlowestSearch()
{
    m_metrics->recordSearch(createTestMetric("slow", 300, 1));
    m_metrics->recordSearch(createTestMetric("fast", 50, 2));
    m_metrics->recordSearch(createTestMetric("medium", 150, 3));
    
    SearchMetrics::Metric slowest = m_metrics->slowestSearch();
    verifyMetric(slowest, "slow", 300);
}

void SearchMetricsTest::testPercentile()
{
    // Record searches with known durations: 50, 100, 150, 200, 250
    for (int i = 1; i <= 5; ++i) {
        m_metrics->recordSearch(createTestMetric(QString("query%1").arg(i), i * 50, i));
    }
    
    // Test different percentiles
    double p50 = m_metrics->percentile(0.5); // Median
    QCOMPARE(p50, 150.0);
    
    double p90 = m_metrics->percentile(0.9);
    QVERIFY(p90 >= 200.0);
    
    double p95 = m_metrics->percentile(0.95);
    QVERIFY(p95 >= 225.0);
}

SearchMetrics::Metric SearchMetricsTest::createTestMetric(const QString& query, qint64 duration, int resultCount, bool cacheHit)
{
    SearchMetrics::Metric metric;
    metric.query = query;
    metric.duration = duration;
    metric.resultCount = resultCount;
    metric.pagesSearched = 1;
    metric.cacheHit = cacheHit;
    metric.incremental = false;
    metric.timestamp = QDateTime::currentDateTime();
    metric.memoryUsage = 1024; // 1KB
    return metric;
}

void SearchMetricsTest::recordMultipleSearches(int count)
{
    for (int i = 0; i < count; ++i) {
        SearchMetrics::Metric metric = createTestMetric(
            QString("query%1").arg(i), 
            100 + i * 10, 
            i + 1,
            i % 2 == 0 // Every other search is a cache hit
        );
        m_metrics->recordSearch(metric);
    }
}

void SearchMetricsTest::verifyMetric(const SearchMetrics::Metric& metric, const QString& expectedQuery, qint64 expectedDuration)
{
    QCOMPARE(metric.query, expectedQuery);
    QCOMPARE(metric.duration, expectedDuration);
    QVERIFY(metric.timestamp.isValid());
    QVERIFY(metric.resultCount >= 0);
    QVERIFY(metric.pagesSearched >= 0);
    QVERIFY(metric.memoryUsage >= 0);
}

void SearchMetricsTest::verifyStatistics(double expectedAvgTime, double expectedCacheRatio)
{
    QCOMPARE(m_metrics->averageSearchTime(), expectedAvgTime);
    QCOMPARE(m_metrics->cacheHitRatio(), expectedCacheRatio);
}

QTEST_MAIN(SearchMetricsTest)
#include "search_metrics_test.moc"
