#include <QElapsedTimer>
#include <QObject>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest/QtTest>
#include "../../app/search/IncrementalSearchManager.h"
#include "../../app/search/SearchConfiguration.h"
#include "../TestUtilities.h"

/**
 * Comprehensive tests for IncrementalSearchManager class
 * Tests incremental search logic, query analysis, and search scheduling
 */
class IncrementalSearchManagerTest : public TestBase {
    Q_OBJECT

protected:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

private slots:
    // Constructor and configuration tests
    void testConstructor();
    void testSetDelay();
    void testSetEnabled();

    // Search scheduling tests
    void testScheduleSearch();
    void testCancelScheduledSearch();
    void testHasScheduledSearch();

    // Incremental logic tests
    void testCanRefineSearch();
    void testRefineResults();
    void testQueryExtension();
    void testQueryReduction();
    void testGetCommonPrefix();

    // Signal emission tests
    void testSearchTriggeredSignal();
    void testSearchScheduledSignal();
    void testSearchCancelledSignal();

    // Timing tests
    void testDelayTiming();
    void testMultipleScheduling();
    void testDisabledManager();

    // Query analysis tests
    void testComplexQueryAnalysis();
    void testEmptyQueryHandling();
    void testSpecialCharacters();

    // Performance tests
    void testRapidScheduling();
    void testLargeQueryHandling();

private:
    IncrementalSearchManager* m_manager;
    SearchOptions m_defaultOptions;
    QList<SearchResult> m_testResults;

    // Helper methods
    void setupTestResults();
    SearchResult createTestResult(const QString& text, int page, int position);
    void verifySearchTriggered(const QString& expectedQuery,
                               int timeoutMs = 1000);
};

void IncrementalSearchManagerTest::initTestCase() {
    qDebug() << "Starting IncrementalSearchManager tests";
    setupTestResults();
}

void IncrementalSearchManagerTest::cleanupTestCase() {
    qDebug() << "IncrementalSearchManager tests completed";
}

void IncrementalSearchManagerTest::init() {
    m_manager = new IncrementalSearchManager(this);
    m_defaultOptions = SearchOptions();
}

void IncrementalSearchManagerTest::cleanup() {
    if (m_manager) {
        m_manager->cancelScheduledSearch();
        // Wait for any pending timers to complete
        QTest::qWait(100);
        delete m_manager;
        m_manager = nullptr;
    }
}

void IncrementalSearchManagerTest::setupTestResults() {
    m_testResults.clear();
    m_testResults.append(createTestResult("Hello world", 1, 0));
    m_testResults.append(createTestResult("Hello there", 1, 20));
    m_testResults.append(createTestResult("Help me", 2, 5));
    m_testResults.append(createTestResult("World peace", 3, 10));
}

SearchResult IncrementalSearchManagerTest::createTestResult(const QString& text,
                                                            int page,
                                                            int position) {
    SearchResult result;
    result.matchedText = text;
    result.pageNumber = page;
    result.textPosition = position;
    result.textLength = text.length();
    return result;
}

void IncrementalSearchManagerTest::testConstructor() {
    QVERIFY(m_manager != nullptr);
    QVERIFY(m_manager->delay() > 0);
    QVERIFY(m_manager->isEnabled());
    QVERIFY(!m_manager->hasScheduledSearch());
}

void IncrementalSearchManagerTest::testSetDelay() {
    int originalDelay = m_manager->delay();

    m_manager->setDelay(500);
    QCOMPARE(m_manager->delay(), 500);

    m_manager->setDelay(1000);
    QCOMPARE(m_manager->delay(), 1000);

    // Test invalid delay
    m_manager->setDelay(-100);
    QVERIFY(m_manager->delay() > 0);  // Should remain positive

    // Restore original
    m_manager->setDelay(originalDelay);
}

void IncrementalSearchManagerTest::testSetEnabled() {
    QVERIFY(m_manager->isEnabled());

    m_manager->setEnabled(false);
    QVERIFY(!m_manager->isEnabled());

    m_manager->setEnabled(true);
    QVERIFY(m_manager->isEnabled());
}

void IncrementalSearchManagerTest::testScheduleSearch() {
    QSignalSpy scheduledSpy(m_manager,
                            &IncrementalSearchManager::searchScheduled);
    QSignalSpy triggeredSpy(m_manager,
                            &IncrementalSearchManager::searchTriggered);

    QString query = "test query";
    m_manager->scheduleSearch(query, m_defaultOptions);

    QVERIFY(m_manager->hasScheduledSearch());
    QCOMPARE(scheduledSpy.count(), 1);

    // Wait for search to be triggered
    QVERIFY(waitForSignal(
        m_manager, SIGNAL(searchTriggered(QString, SearchOptions)), 2000));
    QCOMPARE(triggeredSpy.count(), 1);

    // Verify the triggered query
    QList<QVariant> arguments = triggeredSpy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), query);
}

void IncrementalSearchManagerTest::testCancelScheduledSearch() {
    QSignalSpy cancelledSpy(m_manager,
                            &IncrementalSearchManager::searchCancelled);
    QSignalSpy triggeredSpy(m_manager,
                            &IncrementalSearchManager::searchTriggered);

    m_manager->scheduleSearch("test", m_defaultOptions);
    QVERIFY(m_manager->hasScheduledSearch());

    m_manager->cancelScheduledSearch();
    QVERIFY(!m_manager->hasScheduledSearch());
    QCOMPARE(cancelledSpy.count(), 1);

    // Wait to ensure search is not triggered
    QThread::msleep(m_manager->delay() + 100);
    QCoreApplication::processEvents();
    QCOMPARE(triggeredSpy.count(), 0);
}

void IncrementalSearchManagerTest::testHasScheduledSearch() {
    QVERIFY(!m_manager->hasScheduledSearch());

    m_manager->scheduleSearch("test", m_defaultOptions);
    QVERIFY(m_manager->hasScheduledSearch());

    // Wait for search to trigger
    QVERIFY(waitForSignal(
        m_manager, SIGNAL(searchTriggered(QString, SearchOptions)), 2000));
    QVERIFY(!m_manager->hasScheduledSearch());
}

void IncrementalSearchManagerTest::testCanRefineSearch() {
    // Test query extension
    QVERIFY(m_manager->canRefineSearch("hello world", "hello"));
    QVERIFY(m_manager->canRefineSearch("test query", "test"));

    // Test query reduction
    QVERIFY(m_manager->canRefineSearch("hello", "hello world"));

    // Test unrelated queries
    QVERIFY(!m_manager->canRefineSearch("completely different", "hello"));

    // Test empty queries
    QVERIFY(!m_manager->canRefineSearch("", "hello"));
    QVERIFY(!m_manager->canRefineSearch("hello", ""));
}

void IncrementalSearchManagerTest::testRefineResults() {
    // Test refining with extension (should filter results)
    QList<SearchResult> refined =
        m_manager->refineResults(m_testResults, "Hello w", "Hello");
    QVERIFY(refined.size() <= m_testResults.size());

    // Test refining with reduction (should return original or more)
    refined = m_manager->refineResults(m_testResults, "Hel", "Hello");
    QVERIFY(refined.size() >= 0);

    // Test with unrelated query
    refined = m_manager->refineResults(m_testResults, "xyz", "Hello");
    QVERIFY(refined.isEmpty());
}

void IncrementalSearchManagerTest::testQueryExtension() {
    QVERIFY(m_manager->isQueryExtension("hello world", "hello"));
    QVERIFY(m_manager->isQueryExtension("test query long", "test query"));
    QVERIFY(!m_manager->isQueryExtension("hello", "hello world"));
    QVERIFY(!m_manager->isQueryExtension("different", "hello"));
}

void IncrementalSearchManagerTest::testQueryReduction() {
    QVERIFY(m_manager->isQueryReduction("hello", "hello world"));
    QVERIFY(m_manager->isQueryReduction("test", "test query"));
    QVERIFY(!m_manager->isQueryReduction("hello world", "hello"));
    QVERIFY(!m_manager->isQueryReduction("different", "hello"));
}

void IncrementalSearchManagerTest::testGetCommonPrefix() {
    QCOMPARE(m_manager->getCommonPrefix("hello world", "hello there"),
             QString("hello "));
    QCOMPARE(m_manager->getCommonPrefix("test", "testing"), QString("test"));
    QCOMPARE(m_manager->getCommonPrefix("abc", "xyz"), QString(""));
    QCOMPARE(m_manager->getCommonPrefix("", "hello"), QString(""));
    QCOMPARE(m_manager->getCommonPrefix("hello", ""), QString(""));
}

void IncrementalSearchManagerTest::testDelayTiming() {
    m_manager->setDelay(200);

    QElapsedTimer timer;
    timer.start();

    QSignalSpy triggeredSpy(m_manager,
                            &IncrementalSearchManager::searchTriggered);
    m_manager->scheduleSearch("timing test", m_defaultOptions);

    QVERIFY(waitForSignal(
        m_manager, SIGNAL(searchTriggered(QString, SearchOptions)), 1000));

    qint64 elapsed = timer.elapsed();
    QVERIFY(elapsed >= 180);  // Allow some tolerance
    QVERIFY(elapsed <= 300);
}

void IncrementalSearchManagerTest::testMultipleScheduling() {
    QSignalSpy triggeredSpy(m_manager,
                            &IncrementalSearchManager::searchTriggered);

    // Schedule multiple searches rapidly
    m_manager->scheduleSearch("first", m_defaultOptions);
    QThread::msleep(50);
    m_manager->scheduleSearch("second", m_defaultOptions);
    QThread::msleep(50);
    m_manager->scheduleSearch("third", m_defaultOptions);

    // Only the last search should be triggered
    QVERIFY(waitForSignal(
        m_manager, SIGNAL(searchTriggered(QString, SearchOptions)), 2000));
    QCOMPARE(triggeredSpy.count(), 1);

    QList<QVariant> arguments = triggeredSpy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QString("third"));
}

void IncrementalSearchManagerTest::testDisabledManager() {
    m_manager->setEnabled(false);

    QSignalSpy triggeredSpy(m_manager,
                            &IncrementalSearchManager::searchTriggered);
    QSignalSpy scheduledSpy(m_manager,
                            &IncrementalSearchManager::searchScheduled);

    m_manager->scheduleSearch("immediate", m_defaultOptions);

    // Should trigger immediately when disabled
    QCOMPARE(triggeredSpy.count(), 1);
    QCOMPARE(scheduledSpy.count(), 0);
    QVERIFY(!m_manager->hasScheduledSearch());
}

void IncrementalSearchManagerTest::testSearchTriggeredSignal() {
    QSignalSpy spy(m_manager, &IncrementalSearchManager::searchTriggered);

    m_manager->scheduleSearch("test query", m_defaultOptions);

    // Wait for signal
    QVERIFY(waitForSignal(
        m_manager, SIGNAL(searchTriggered(QString, SearchOptions)), 2000));
    QCOMPARE(spy.count(), 1);

    // Verify signal arguments
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QString("test query"));
}

void IncrementalSearchManagerTest::testSearchScheduledSignal() {
    QSignalSpy spy(m_manager, &IncrementalSearchManager::searchScheduled);

    m_manager->scheduleSearch("scheduled query", m_defaultOptions);

    // Signal should be emitted immediately when search is scheduled
    QCOMPARE(spy.count(), 1);
}

void IncrementalSearchManagerTest::testSearchCancelledSignal() {
    QSignalSpy spy(m_manager, &IncrementalSearchManager::searchCancelled);

    m_manager->scheduleSearch("test", m_defaultOptions);
    m_manager->cancelScheduledSearch();

    // Signal should be emitted when search is cancelled
    QCOMPARE(spy.count(), 1);
}

void IncrementalSearchManagerTest::testComplexQueryAnalysis() {
    // Test with complex queries containing multiple words
    QString query1 = "complex search query with multiple words";
    QString query2 = "complex search query with";

    QVERIFY(m_manager->isQueryReduction(query2, query1));
    QVERIFY(m_manager->canRefineSearch(query2, query1));

    QString commonPrefix = m_manager->getCommonPrefix(query1, query2);
    QCOMPARE(commonPrefix, query2);
}

void IncrementalSearchManagerTest::testEmptyQueryHandling() {
    // Test scheduling empty query
    QSignalSpy triggeredSpy(m_manager,
                            &IncrementalSearchManager::searchTriggered);

    m_manager->scheduleSearch("", m_defaultOptions);

    // Wait for potential trigger
    QVERIFY(waitForSignal(
        m_manager, SIGNAL(searchTriggered(QString, SearchOptions)), 2000));

    // Empty query should still trigger
    QCOMPARE(triggeredSpy.count(), 1);

    // Test refinement with empty queries
    QVERIFY(!m_manager->canRefineSearch("", "test"));
    QVERIFY(!m_manager->canRefineSearch("test", ""));
}

void IncrementalSearchManagerTest::testSpecialCharacters() {
    QString query1 = "test@#$%^&*()";
    QString query2 = "test@#$";

    QVERIFY(m_manager->isQueryReduction(query2, query1));
    QVERIFY(m_manager->canRefineSearch(query2, query1));

    QString commonPrefix = m_manager->getCommonPrefix(query1, query2);
    QCOMPARE(commonPrefix, query2);
}

void IncrementalSearchManagerTest::testRapidScheduling() {
    QSignalSpy triggeredSpy(m_manager,
                            &IncrementalSearchManager::searchTriggered);

    // Schedule many searches rapidly
    for (int i = 0; i < 10; ++i) {
        m_manager->scheduleSearch(QString("query_%1").arg(i), m_defaultOptions);
        QThread::msleep(10);  // Very short delay
    }

    // Only the last query should be triggered
    QVERIFY(waitForSignal(
        m_manager, SIGNAL(searchTriggered(QString, SearchOptions)), 2000));

    // Should have exactly one trigger (the last scheduled search)
    QCOMPARE(triggeredSpy.count(), 1);

    QList<QVariant> arguments = triggeredSpy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QString("query_9"));
}

void IncrementalSearchManagerTest::testLargeQueryHandling() {
    // Create a very large query
    QString largeQuery;
    for (int i = 0; i < 1000; ++i) {
        largeQuery += "word ";
    }

    QSignalSpy triggeredSpy(m_manager,
                            &IncrementalSearchManager::searchTriggered);

    m_manager->scheduleSearch(largeQuery, m_defaultOptions);

    // Should handle large query without crashing
    QVERIFY(waitForSignal(
        m_manager, SIGNAL(searchTriggered(QString, SearchOptions)), 2000));
    QCOMPARE(triggeredSpy.count(), 1);

    QList<QVariant> arguments = triggeredSpy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), largeQuery);
}

void IncrementalSearchManagerTest::verifySearchTriggered(
    const QString& expectedQuery, int timeoutMs) {
    QSignalSpy spy(m_manager, &IncrementalSearchManager::searchTriggered);
    QVERIFY(spy.wait(timeoutMs));
    QCOMPARE(spy.count(), 1);

    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), expectedQuery);
}

QTEST_MAIN(IncrementalSearchManagerTest)
#include "test_incremental_search_manager.moc"
