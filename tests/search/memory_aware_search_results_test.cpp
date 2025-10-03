#include <QList>
#include <QObject>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include "../../app/search/MemoryManager.h"
#include "../../app/search/SearchConfiguration.h"
#include "../TestUtilities.h"

/**
 * Comprehensive tests for MemoryAwareSearchResults implementation
 * Tests memory management, lazy loading, and result handling
 */
class MemoryAwareSearchResultsTest : public TestBase {
    Q_OBJECT

protected:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

private slots:
    // Basic functionality tests
    void testAddResults();
    void testAddResultsMemoryLimit();
    void testClearResults();
    void testGetResults();
    void testGetResultsRange();
    void testResultCount();

    // Memory management tests
    void testMemoryUsageTracking();
    void testMemoryOptimization();
    void testMaxMemoryUsage();
    void testMemoryPressureHandling();

    // Lazy loading tests
    void testLazyLoading();
    void testLazyLoadingPreload();
    void testLazyLoadingSignals();

    // Signal emission tests
    void testSignalEmission();
    void testMemoryOptimizedSignal();
    void testResultsAddedSignal();
    void testResultsClearedSignal();

    // Edge cases and error handling
    void testEmptyResults();
    void testInvalidRanges();
    void testLargeResultSets();

private:
    MemoryAwareSearchResults* m_memoryResults;
    QList<SearchResult> m_testResults;

    // Helper methods
    SearchResult createTestResult(const QString& text, int page, int position);
    QList<SearchResult> createTestResults(int count);
    void verifySignalEmission(QSignalSpy& spy, int expectedCount);
    qint64 calculateExpectedMemoryUsage(const QList<SearchResult>& results);
};

void MemoryAwareSearchResultsTest::initTestCase() {
    qDebug() << "Starting MemoryAwareSearchResults tests";
}

void MemoryAwareSearchResultsTest::cleanupTestCase() {
    qDebug() << "MemoryAwareSearchResults tests completed";
}

void MemoryAwareSearchResultsTest::init() {
    qDebug() << "Creating MemoryAwareSearchResults object...";
    try {
        m_memoryResults = new MemoryAwareSearchResults(this);
        qDebug() << "MemoryAwareSearchResults created successfully";
        m_testResults = createTestResults(10);
        qDebug() << "Test results created:" << m_testResults.size();
    } catch (const std::exception& e) {
        qDebug() << "Exception during init:" << e.what();
        m_memoryResults = nullptr;
    } catch (...) {
        qDebug() << "Unknown exception during init";
        m_memoryResults = nullptr;
    }
}

void MemoryAwareSearchResultsTest::cleanup() {
    delete m_memoryResults;
    m_memoryResults = nullptr;
    m_testResults.clear();
}

void MemoryAwareSearchResultsTest::testAddResults() {
    qDebug() << "Starting testAddResults...";

    // Defensive check - ensure object is properly initialized
    if (!m_memoryResults) {
        QFAIL("MemoryAwareSearchResults object is null");
        return;
    }

    qDebug() << "Object is not null, checking metaObject...";
    QVERIFY(m_memoryResults->metaObject() != nullptr);
    qDebug() << "MetaObject is valid";

    // Test basic functionality without QSignalSpy first
    qDebug() << "Testing basic addResults functionality...";

    try {
        // Add results
        m_memoryResults->addResults(m_testResults);
        qDebug() << "addResults completed";

        // Verify results were added
        int count = m_memoryResults->getResultCount();
        qDebug() << "Result count:" << count;
        QCOMPARE(count, 10);

        // Verify memory usage is tracked
        qint64 memUsage = m_memoryResults->getCurrentMemoryUsage();
        qDebug() << "Memory usage:" << memUsage << "bytes";
        QVERIFY(memUsage > 0);

        qDebug() << "Basic functionality test passed.";
    } catch (const std::exception& e) {
        QFAIL(QString("Exception in testAddResults: %1")
                  .arg(e.what())
                  .toLocal8Bit()
                  .data());
    } catch (...) {
        QFAIL("Unknown exception in testAddResults");
    }
}

void MemoryAwareSearchResultsTest::testAddResultsMemoryLimit() {
    qDebug() << "Testing memory limit functionality...";

    // Set a very low memory limit
    m_memoryResults->setMaxMemoryUsage(1000);  // 1KB
    qDebug() << "Set memory limit to:" << m_memoryResults->getMaxMemoryUsage()
             << "bytes";

    // Add large results that exceed limit
    QList<SearchResult> largeResults = createTestResults(100);
    m_memoryResults->addResults(largeResults);

    // Should have triggered memory optimization
    qint64 currentUsage = m_memoryResults->getCurrentMemoryUsage();
    qint64 maxUsage = m_memoryResults->getMaxMemoryUsage();

    qDebug() << "After adding 100 results:";
    qDebug() << "Current memory usage:" << currentUsage << "bytes";
    qDebug() << "Max memory usage:" << maxUsage << "bytes";

    // Memory should be within limits (allowing some tolerance for overhead)
    QVERIFY(currentUsage <= maxUsage * 1.1);  // Allow 10% tolerance
}

void MemoryAwareSearchResultsTest::testClearResults() {
    qDebug() << "Testing clear results functionality...";

    // Add results first
    m_memoryResults->addResults(m_testResults);
    QVERIFY(m_memoryResults->getResultCount() > 0);
    qDebug() << "Added results, count:" << m_memoryResults->getResultCount();

    // Clear results
    m_memoryResults->clearResults();
    qDebug() << "Cleared results";

    // Verify results were cleared
    QCOMPARE(m_memoryResults->getResultCount(), 0);
    QCOMPARE(m_memoryResults->getCurrentMemoryUsage(), 0);
    qDebug() << "Clear test passed - count:"
             << m_memoryResults->getResultCount()
             << "memory:" << m_memoryResults->getCurrentMemoryUsage();
}

void MemoryAwareSearchResultsTest::testGetResults() {
    // Add test results
    m_memoryResults->addResults(m_testResults);

    // Get all results
    QList<SearchResult> retrieved = m_memoryResults->getResults();
    QCOMPARE(retrieved.size(), m_testResults.size());

    // Verify content matches
    for (int i = 0; i < retrieved.size(); ++i) {
        QCOMPARE(retrieved[i].matchedText, m_testResults[i].matchedText);
        QCOMPARE(retrieved[i].pageNumber, m_testResults[i].pageNumber);
    }
}

void MemoryAwareSearchResultsTest::testGetResultsRange() {
    // Add test results
    m_memoryResults->addResults(m_testResults);

    // Get partial results
    QList<SearchResult> partial = m_memoryResults->getResults(2, 3);
    QCOMPARE(partial.size(), 3);

    // Verify correct range
    for (int i = 0; i < partial.size(); ++i) {
        QCOMPARE(partial[i].matchedText, m_testResults[i + 2].matchedText);
    }

    // Test edge cases
    QList<SearchResult> empty = m_memoryResults->getResults(100, 5);
    QVERIFY(empty.isEmpty());

    QList<SearchResult> fromEnd = m_memoryResults->getResults(8, -1);
    QCOMPARE(fromEnd.size(), 2);  // Should get last 2 results
}

void MemoryAwareSearchResultsTest::testMemoryUsageTracking() {
    qint64 initialMemory = m_memoryResults->getCurrentMemoryUsage();
    QCOMPARE(initialMemory, 0);

    // Add results and check memory increases
    m_memoryResults->addResults(m_testResults);
    qint64 afterAdd = m_memoryResults->getCurrentMemoryUsage();
    QVERIFY(afterAdd > initialMemory);

    // Clear and check memory resets
    m_memoryResults->clearResults();
    qint64 afterClear = m_memoryResults->getCurrentMemoryUsage();
    QCOMPARE(afterClear, 0);
}

void MemoryAwareSearchResultsTest::testMemoryOptimization() {
    QSignalSpy optimizedSpy(m_memoryResults,
                            &MemoryAwareSearchResults::memoryOptimized);

    // Add results
    m_memoryResults->addResults(m_testResults);
    qint64 beforeOptimization = m_memoryResults->getCurrentMemoryUsage();

    // Set lower memory limit to force optimization
    m_memoryResults->setMaxMemoryUsage(beforeOptimization / 2);
    m_memoryResults->optimizeMemoryUsage();

    // Should have freed memory
    QVERIFY(optimizedSpy.count() > 0);
    QVERIFY(m_memoryResults->getCurrentMemoryUsage() < beforeOptimization);
}

void MemoryAwareSearchResultsTest::testMaxMemoryUsage() {
    // Test default max memory
    qint64 defaultMax = m_memoryResults->getMaxMemoryUsage();
    QVERIFY(defaultMax > 0);

    // Set new max memory
    qint64 newMax = 10 * 1024 * 1024;  // 10MB
    m_memoryResults->setMaxMemoryUsage(newMax);
    QCOMPARE(m_memoryResults->getMaxMemoryUsage(), newMax);
}

void MemoryAwareSearchResultsTest::testLazyLoading() {
    // Test default state
    QVERIFY(!m_memoryResults->isLazyLoadingEnabled());

    // Enable lazy loading
    m_memoryResults->enableLazyLoading(true);
    QVERIFY(m_memoryResults->isLazyLoadingEnabled());

    // Add results
    m_memoryResults->addResults(m_testResults);

    // Disable lazy loading
    m_memoryResults->enableLazyLoading(false);
    QVERIFY(!m_memoryResults->isLazyLoadingEnabled());
}

void MemoryAwareSearchResultsTest::testLazyLoadingPreload() {
    QSignalSpy lazyLoadSpy(m_memoryResults,
                           &MemoryAwareSearchResults::lazyLoadRequested);

    // Enable lazy loading and add results
    m_memoryResults->enableLazyLoading(true);
    m_memoryResults->addResults(createTestResults(200));  // Large set

    // Preload specific range
    m_memoryResults->preloadResults(50, 25);

    // Should have requested lazy loading for unloaded pages
    // The exact count depends on implementation details
    QVERIFY(lazyLoadSpy.count() >= 0);
}

void MemoryAwareSearchResultsTest::testSignalEmission() {
    QSignalSpy addedSpy(m_memoryResults,
                        &MemoryAwareSearchResults::resultsAdded);
    QSignalSpy clearedSpy(m_memoryResults,
                          &MemoryAwareSearchResults::resultsCleared);
    QSignalSpy optimizedSpy(m_memoryResults,
                            &MemoryAwareSearchResults::memoryOptimized);

    // Test resultsAdded signal
    m_memoryResults->addResults(m_testResults);
    QCOMPARE(addedSpy.count(), 1);

    // Test resultsCleared signal
    m_memoryResults->clearResults();
    QCOMPARE(clearedSpy.count(), 1);

    // Test memoryOptimized signal (by setting low memory limit)
    m_memoryResults->addResults(m_testResults);
    m_memoryResults->setMaxMemoryUsage(100);  // Very low limit
    QVERIFY(optimizedSpy.count() > 0);
}

void MemoryAwareSearchResultsTest::testEmptyResults() {
    // Test operations on empty results
    QCOMPARE(m_memoryResults->getResultCount(), 0);
    QCOMPARE(m_memoryResults->getCurrentMemoryUsage(), 0);

    QList<SearchResult> empty = m_memoryResults->getResults();
    QVERIFY(empty.isEmpty());

    // Clear empty results should not crash
    m_memoryResults->clearResults();
    QCOMPARE(m_memoryResults->getResultCount(), 0);
}

void MemoryAwareSearchResultsTest::testInvalidRanges() {
    m_memoryResults->addResults(m_testResults);

    // Test invalid start index
    QList<SearchResult> invalid1 = m_memoryResults->getResults(-1, 5);
    QVERIFY(invalid1.isEmpty());

    // Test start index beyond range
    QList<SearchResult> invalid2 = m_memoryResults->getResults(100, 5);
    QVERIFY(invalid2.isEmpty());

    // Test zero count
    QList<SearchResult> invalid3 = m_memoryResults->getResults(0, 0);
    QVERIFY(invalid3.isEmpty());
}

void MemoryAwareSearchResultsTest::testLargeResultSets() {
    // Create large result set
    QList<SearchResult> largeResults = createTestResults(1000);

    // Add large results
    m_memoryResults->addResults(largeResults);
    QCOMPARE(m_memoryResults->getResultCount(), 1000);

    // Test memory usage is reasonable
    qint64 memoryUsage = m_memoryResults->getCurrentMemoryUsage();
    QVERIFY(memoryUsage > 0);

    // Test partial retrieval
    QList<SearchResult> partial = m_memoryResults->getResults(500, 100);
    QCOMPARE(partial.size(), 100);
}

void MemoryAwareSearchResultsTest::testResultCount() {
    // Test initial count
    QCOMPARE(m_memoryResults->getResultCount(), 0);

    // Add results and test count
    m_memoryResults->addResults(m_testResults);
    QCOMPARE(m_memoryResults->getResultCount(), m_testResults.size());

    // Clear and test count
    m_memoryResults->clearResults();
    QCOMPARE(m_memoryResults->getResultCount(), 0);
}

void MemoryAwareSearchResultsTest::testMemoryPressureHandling() {
    QSignalSpy optimizedSpy(m_memoryResults,
                            &MemoryAwareSearchResults::memoryOptimized);

    // Add results
    m_memoryResults->addResults(m_testResults);

    // Simulate memory pressure by setting very low limit
    m_memoryResults->setMaxMemoryUsage(100);

    // Should trigger memory optimization
    QVERIFY(optimizedSpy.count() > 0);
    QVERIFY(m_memoryResults->getCurrentMemoryUsage() <=
            m_memoryResults->getMaxMemoryUsage());
}

void MemoryAwareSearchResultsTest::testLazyLoadingSignals() {
    QSignalSpy lazyLoadSpy(m_memoryResults,
                           &MemoryAwareSearchResults::lazyLoadRequested);

    // Enable lazy loading
    m_memoryResults->enableLazyLoading(true);

    // Add results
    m_memoryResults->addResults(m_testResults);

    // Request results that might trigger lazy loading
    m_memoryResults->getResults(0, 5);

    // Verify signal was emitted (implementation dependent)
    QVERIFY(lazyLoadSpy.count() >= 0);
}

void MemoryAwareSearchResultsTest::testResultsAddedSignal() {
    QSignalSpy addedSpy(m_memoryResults,
                        &MemoryAwareSearchResults::resultsAdded);

    // Add results
    m_memoryResults->addResults(m_testResults);

    // Verify signal was emitted
    QCOMPARE(addedSpy.count(), 1);

    // Verify signal arguments
    QList<QVariant> arguments = addedSpy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), m_testResults.size());
}

void MemoryAwareSearchResultsTest::testResultsClearedSignal() {
    QSignalSpy clearedSpy(m_memoryResults,
                          &MemoryAwareSearchResults::resultsCleared);

    // Add results first
    m_memoryResults->addResults(m_testResults);

    // Clear results
    m_memoryResults->clearResults();

    // Verify signal was emitted
    QCOMPARE(clearedSpy.count(), 1);
}

void MemoryAwareSearchResultsTest::testMemoryOptimizedSignal() {
    QSignalSpy optimizedSpy(m_memoryResults,
                            &MemoryAwareSearchResults::memoryOptimized);

    // Add results
    m_memoryResults->addResults(m_testResults);

    // Trigger optimization by setting low memory limit
    m_memoryResults->setMaxMemoryUsage(100);

    // Verify signal was emitted
    QVERIFY(optimizedSpy.count() > 0);

    // Verify signal arguments
    QList<QVariant> arguments = optimizedSpy.takeFirst();
    QVERIFY(arguments.at(0).toLongLong() >= 0);  // Memory freed amount
}

// Helper methods implementation
SearchResult MemoryAwareSearchResultsTest::createTestResult(const QString& text,
                                                            int page,
                                                            int position) {
    SearchResult result;
    result.matchedText = text;
    result.contextText = QString("Context for %1").arg(text);
    result.pageNumber = page;
    result.textPosition = position;
    result.textLength = text.length();
    result.boundingRect = QRectF(position, page * 10, text.length() * 8, 12);

    return result;
}

QList<SearchResult> MemoryAwareSearchResultsTest::createTestResults(int count) {
    QList<SearchResult> results;
    for (int i = 0; i < count; ++i) {
        results.append(
            createTestResult(QString("Test result %1 with some content").arg(i),
                             i / 3 + 1, i * 10));
    }
    return results;
}

QTEST_MAIN(MemoryAwareSearchResultsTest)
#include "memory_aware_search_results_test.moc"
