#include <poppler-qt6.h>
#include <QPainter>
#include <QPdfWriter>
#include <QTemporaryFile>
#include <QtTest/QtTest>
#include "../../app/search/BackgroundProcessor.h"
#include "../../app/search/IncrementalSearchManager.h"
#include "../../app/search/SearchConfiguration.h"
#include "../../app/search/SearchExecutor.h"
#include "../../app/search/SearchMetrics.h"
#include "../../app/search/TextExtractor.h"

/**
 * Unit tests for individual search components
 */
class TestSearchComponents : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // TextExtractor tests
    void testTextExtractorBasic();
    void testTextExtractorCache();
    void testTextExtractorPrefetch();
    void testTextExtractorConcurrency();

    // SearchExecutor tests
    void testSearchExecutorBasic();
    void testSearchExecutorPatterns();
    void testSearchExecutorOptions();
    void testSearchExecutorValidation();

    // IncrementalSearchManager tests
    void testIncrementalManagerScheduling();
    void testIncrementalManagerQueryAnalysis();
    void testIncrementalManagerRefinement();
    void testIncrementalManagerCancellation();

    // BackgroundProcessor tests
    void testBackgroundProcessorExecution();
    void testBackgroundProcessorBatch();
    void testBackgroundProcessorCancellation();
    void testBackgroundProcessorThreading();

    // SearchMetrics tests
    void testMetricsRecording();
    void testMetricsCalculations();
    void testMetricsHistory();
    void testMetricsPerformanceAnalysis();

private:
    Poppler::Document* m_testDocument;
    QString m_testPdfPath;

    Poppler::Document* createTestDocument();
    void cleanup();
};

void TestSearchComponents::initTestCase() {
    m_testDocument = createTestDocument();
    QVERIFY(m_testDocument != nullptr);
}

void TestSearchComponents::cleanupTestCase() {
    delete m_testDocument;
    if (!m_testPdfPath.isEmpty()) {
        QFile::remove(m_testPdfPath);
    }
}

void TestSearchComponents::cleanup() {
    // Cleanup after each test if needed
}

// TextExtractor Tests

void TestSearchComponents::testTextExtractorBasic() {
    TextExtractor extractor;
    extractor.setDocument(m_testDocument);

    // Extract text from first page
    QString text = extractor.extractPageText(0);
    QVERIFY(!text.isEmpty());
    QVERIFY(text.contains("test"));

    // Extract from multiple pages
    QStringList texts = extractor.extractPagesText({0, 1, 2});
    QCOMPARE(texts.size(), 3);
    for (const QString& pageText : texts) {
        QVERIFY(!pageText.isEmpty());
    }
}

void TestSearchComponents::testTextExtractorCache() {
    TextExtractor extractor;
    extractor.setDocument(m_testDocument);
    extractor.setCacheEnabled(true);

    // First extraction (cache miss)
    QElapsedTimer timer;
    timer.start();
    QString text1 = extractor.extractPageText(0);
    qint64 firstTime = timer.elapsed();

    // Second extraction (cache hit)
    timer.restart();
    QString text2 = extractor.extractPageText(0);
    qint64 secondTime = timer.elapsed();

    // Cached should be faster
    QVERIFY(secondTime <= firstTime);
    QCOMPARE(text1, text2);

    // Check cache memory usage
    qint64 memoryUsage = extractor.cacheMemoryUsage();
    QVERIFY(memoryUsage > 0);

    // Clear cache
    extractor.clearCache();
    memoryUsage = extractor.cacheMemoryUsage();
    QCOMPARE(memoryUsage, 0);
}

void TestSearchComponents::testTextExtractorPrefetch() {
    TextExtractor extractor;
    extractor.setDocument(m_testDocument);
    extractor.setCacheEnabled(true);

    // Prefetch pages
    extractor.prefetchRange(0, 2);

    // Should be cached now
    QElapsedTimer timer;
    timer.start();
    QString text = extractor.extractPageText(1);
    qint64 extractTime = timer.elapsed();

    // Should be fast (cached)
    QVERIFY(extractTime < 50);
    QVERIFY(!text.isEmpty());
}

void TestSearchComponents::testTextExtractorConcurrency() {
    TextExtractor extractor;
    extractor.setDocument(m_testDocument);

    QSignalSpy extractedSpy(&extractor, &TextExtractor::textExtracted);
    QSignalSpy progressSpy(&extractor, &TextExtractor::extractionProgress);

    // Extract all text
    QString allText = extractor.extractAllText();
    QVERIFY(!allText.isEmpty());

    // Should have received signals
    QVERIFY(extractedSpy.count() > 0);
    QVERIFY(progressSpy.count() > 0);
}

// SearchExecutor Tests

void TestSearchComponents::testSearchExecutorBasic() {
    TextExtractor extractor;
    extractor.setDocument(m_testDocument);

    SearchExecutor executor;
    executor.setTextExtractor(&extractor);

    SearchOptions options;
    executor.setOptions(options);

    // Search in single page
    QList<SearchResult> results = executor.searchInPage(0, "test");
    QVERIFY(!results.isEmpty());

    for (const SearchResult& result : results) {
        QCOMPARE(result.pageNumber, 0);
        QVERIFY(result.matchedText.contains("test", Qt::CaseInsensitive));
    }
}

void TestSearchComponents::testSearchExecutorPatterns() {
    TextExtractor extractor;
    extractor.setDocument(m_testDocument);

    SearchExecutor executor;
    executor.setTextExtractor(&extractor);

    // Test pattern creation
    SearchOptions options;
    options.caseSensitive = false;
    options.wholeWords = false;
    options.useRegex = false;

    QRegularExpression pattern = executor.createSearchPattern("test", options);
    QVERIFY(pattern.isValid());

    // Test with whole words
    options.wholeWords = true;
    pattern = executor.createSearchPattern("test", options);
    QVERIFY(pattern.isValid());
    QVERIFY(pattern.pattern().contains("\\b"));

    // Test with regex
    options.useRegex = true;
    pattern = executor.createSearchPattern("te.*st", options);
    QVERIFY(pattern.isValid());
}

void TestSearchComponents::testSearchExecutorOptions() {
    TextExtractor extractor;
    extractor.setDocument(m_testDocument);

    SearchExecutor executor;
    executor.setTextExtractor(&extractor);

    // Test case sensitive search
    SearchOptions options;
    options.caseSensitive = true;
    executor.setOptions(options);

    QList<SearchResult> results = executor.searchInPage(0, "TEST");
    // Should only find uppercase matches
    for (const SearchResult& result : results) {
        QVERIFY(result.matchedText.contains("TEST"));
    }

    // Test max results
    options.caseSensitive = false;
    options.maxResults = 2;
    executor.setOptions(options);

    results = executor.searchInPages({0, 1, 2}, "e");  // Common letter
    QVERIFY(results.size() <= 2);
}

void TestSearchComponents::testSearchExecutorValidation() {
    SearchExecutor executor;

    // Test query validation
    QVERIFY(executor.validateQuery("test"));
    QVERIFY(!executor.validateQuery(""));

    SearchOptions options;
    options.useRegex = true;
    executor.setOptions(options);

    QVERIFY(executor.validateQuery(".*"));
    QVERIFY(!executor.validateQuery("["));  // Invalid regex
}

// IncrementalSearchManager Tests

void TestSearchComponents::testIncrementalManagerScheduling() {
    IncrementalSearchManager manager;
    manager.setEnabled(true);
    manager.setDelay(100);

    QSignalSpy triggeredSpy(&manager,
                            &IncrementalSearchManager::searchTriggered);
    QSignalSpy scheduledSpy(&manager,
                            &IncrementalSearchManager::searchScheduled);

    // Schedule a search
    manager.scheduleSearch("test", SearchOptions());

    QVERIFY(scheduledSpy.wait(50));
    QVERIFY(manager.hasScheduledSearch());

    // Should trigger after delay
    QVERIFY(triggeredSpy.wait(200));
    QVERIFY(!manager.hasScheduledSearch());

    // Check triggered signal parameters
    QList<QVariant> args = triggeredSpy.takeFirst();
    QCOMPARE(args.at(0).toString(), "test");
}

void TestSearchComponents::testIncrementalManagerQueryAnalysis() {
    IncrementalSearchManager manager;

    // Test query extension detection
    QVERIFY(manager.isQueryExtension("test", "te"));
    QVERIFY(manager.isQueryExtension("testing", "test"));
    QVERIFY(!manager.isQueryExtension("test", "testing"));

    // Test query reduction detection
    QVERIFY(manager.isQueryReduction("te", "test"));
    QVERIFY(!manager.isQueryReduction("test", "te"));

    // Test common prefix
    QCOMPARE(manager.getCommonPrefix("test", "testing"), "test");
    QCOMPARE(manager.getCommonPrefix("abc", "xyz"), "");
}

void TestSearchComponents::testIncrementalManagerRefinement() {
    IncrementalSearchManager manager;

    // Create sample results
    QList<SearchResult> results;
    results.append(SearchResult(0, "test", "This is a test", QRectF(), 10, 4));
    results.append(
        SearchResult(0, "testing", "Testing functionality", QRectF(), 0, 7));

    // Test refinement for extension
    QVERIFY(manager.canRefineSearch("testing", "test"));
    QList<SearchResult> refined =
        manager.refineResults(results, "testing", "test");

    // Should only include results containing "testing"
    QVERIFY(refined.size() <= results.size());
    for (const SearchResult& result : refined) {
        QVERIFY(result.matchedText.contains("testing", Qt::CaseInsensitive));
    }
}

void TestSearchComponents::testIncrementalManagerCancellation() {
    IncrementalSearchManager manager;
    manager.setEnabled(true);
    manager.setDelay(200);

    QSignalSpy cancelledSpy(&manager,
                            &IncrementalSearchManager::searchCancelled);

    // Schedule and cancel
    manager.scheduleSearch("test", SearchOptions());
    QVERIFY(manager.hasScheduledSearch());

    manager.cancelScheduledSearch();
    QVERIFY(!manager.hasScheduledSearch());
    QVERIFY(cancelledSpy.count() > 0);
}

// BackgroundProcessor Tests

void TestSearchComponents::testBackgroundProcessorExecution() {
    BackgroundProcessor processor;
    processor.setMaxThreadCount(2);

    QSignalSpy startedSpy(&processor, &BackgroundProcessor::taskStarted);
    QSignalSpy finishedSpy(&processor, &BackgroundProcessor::taskFinished);

    // Execute async task
    processor.executeAsync([]() { QThread::msleep(50); });

    QVERIFY(startedSpy.wait(100));
    QVERIFY(finishedSpy.wait(200));

    // Should be idle after completion
    QTest::qWait(100);
    QVERIFY(processor.isIdle());
}

void TestSearchComponents::testBackgroundProcessorBatch() {
    BackgroundProcessor processor;

    QSignalSpy progressSpy(&processor, &BackgroundProcessor::progressUpdate);
    QSignalSpy allFinishedSpy(&processor,
                              &BackgroundProcessor::allTasksFinished);

    // Create batch tasks
    QList<std::function<void()>> tasks;
    for (int i = 0; i < 5; ++i) {
        tasks.append([]() { QThread::msleep(10); });
    }

    processor.executeBatch(tasks);

    // Wait for completion
    QVERIFY(allFinishedSpy.wait(1000));
    QVERIFY(progressSpy.count() > 0);
}

void TestSearchComponents::testBackgroundProcessorCancellation() {
    BackgroundProcessor processor;

    // Start long-running task
    processor.executeAsync([]() { QThread::msleep(500); });

    // Cancel immediately
    processor.cancelAll();

    // Should complete quickly
    processor.waitForDone(100);
    QVERIFY(processor.isIdle());
}

void TestSearchComponents::testBackgroundProcessorThreading() {
    BackgroundProcessor processor;

    // Test thread count configuration
    processor.setMaxThreadCount(4);
    QCOMPARE(processor.maxThreadCount(), 4);

    // Test active thread count
    for (int i = 0; i < 3; ++i) {
        processor.executeAsync([]() { QThread::msleep(100); });
    }

    QTest::qWait(10);
    int activeCount = processor.activeThreadCount();
    QVERIFY(activeCount > 0 && activeCount <= 3);

    processor.waitForDone();
}

// SearchMetrics Tests

void TestSearchComponents::testMetricsRecording() {
    SearchMetrics metrics;

    QSignalSpy updatedSpy(&metrics, &SearchMetrics::metricsUpdated);

    // Record a search
    SearchMetrics::Metric metric;
    metric.query = "test";
    metric.duration = 100;
    metric.resultCount = 5;
    metric.pagesSearched = 10;
    metric.cacheHit = false;
    metric.incremental = false;
    metric.timestamp = QDateTime::currentDateTime();

    metrics.recordSearch(metric);

    QVERIFY(updatedSpy.count() > 0);
    QCOMPARE(metrics.totalSearches(), 1);
}

void TestSearchComponents::testMetricsCalculations() {
    SearchMetrics metrics;

    // Record multiple searches
    for (int i = 0; i < 5; ++i) {
        SearchMetrics::Metric m;
        m.query = QString("test%1").arg(i);
        m.duration = 50 + i * 10;
        m.resultCount = i + 1;
        m.cacheHit = (i % 2 == 0);
        m.incremental = (i % 3 == 0);
        m.timestamp = QDateTime::currentDateTime();
        metrics.recordSearch(m);
    }

    // Record cache hits/misses
    metrics.recordCacheHit("test");
    metrics.recordCacheHit("test");
    metrics.recordCacheMiss("new");

    // Test calculations
    double avgTime = metrics.averageSearchTime();
    QVERIFY(avgTime > 0);

    double hitRatio = metrics.cacheHitRatio();
    QVERIFY(hitRatio >= 0.0 && hitRatio <= 1.0);

    double incRatio = metrics.incrementalSearchRatio();
    QVERIFY(incRatio >= 0.0 && incRatio <= 1.0);

    QCOMPARE(metrics.totalCacheHits(), 2);
    QCOMPARE(metrics.totalCacheMisses(), 1);
}

void TestSearchComponents::testMetricsHistory() {
    SearchMetrics metrics;

    // Add metrics
    QDateTime start = QDateTime::currentDateTime();

    for (int i = 0; i < 10; ++i) {
        SearchMetrics::Metric m;
        m.query = QString("query%1").arg(i);
        m.duration = 100;
        m.timestamp = start.addSecs(i);
        metrics.recordSearch(m);
    }

    // Test recent metrics
    QList<SearchMetrics::Metric> recent = metrics.recentMetrics(5);
    QCOMPARE(recent.size(), 5);

    // Test range query
    QDateTime rangeStart = start.addSecs(2);
    QDateTime rangeEnd = start.addSecs(7);
    QList<SearchMetrics::Metric> rangeMetrics =
        metrics.metricsInRange(rangeStart, rangeEnd);
    QVERIFY(rangeMetrics.size() >= 5 && rangeMetrics.size() <= 6);

    // Test clear
    metrics.clearHistory();
    QCOMPARE(metrics.totalSearches(), 0);
}

void TestSearchComponents::testMetricsPerformanceAnalysis() {
    SearchMetrics metrics;

    QSignalSpy warningSpy(&metrics, &SearchMetrics::performanceWarning);

    // Add varied performance metrics
    for (int i = 0; i < 10; ++i) {
        SearchMetrics::Metric m;
        m.query = QString("q%1").arg(i);
        m.duration = (i == 5) ? 2000 : 50;  // One slow search
        m.timestamp = QDateTime::currentDateTime();
        metrics.recordSearch(m);
    }

    // Should have triggered warning for slow search
    QVERIFY(warningSpy.count() > 0);

    // Test performance analysis
    SearchMetrics::Metric fastest = metrics.fastestSearch();
    SearchMetrics::Metric slowest = metrics.slowestSearch();

    QVERIFY(fastest.duration < slowest.duration);
    QCOMPARE(slowest.duration, 2000);

    // Test percentile
    double p50 = metrics.percentile(0.5);
    double p95 = metrics.percentile(0.95);
    QVERIFY(p50 <= p95);
}

// Helper method implementation

Poppler::Document* TestSearchComponents::createTestDocument() {
    QTemporaryFile tempFile;
    tempFile.setFileTemplate(QDir::tempPath() + "/test_components_XXXXXX.pdf");
    if (!tempFile.open()) {
        return nullptr;
    }

    m_testPdfPath = tempFile.fileName();
    tempFile.close();

    QPdfWriter pdfWriter(m_testPdfPath);
    pdfWriter.setPageSize(QPageSize::A4);
    pdfWriter.setResolution(300);

    QPainter painter(&pdfWriter);
    if (!painter.isActive()) {
        return nullptr;
    }

    QFont font = painter.font();
    font.setPointSize(12);
    painter.setFont(font);

    QStringList testTexts = {
        "Page 1: This is a test document.\n"
        "It contains various words for testing.\n"
        "TEST test Test TeSt",

        "Page 2: More content here.\n"
        "Email: test@example.com\n"
        "Numbers: 123 456 789",

        "Page 3: Final page.\n"
        "Special characters: !@#$%\n"
        "Unicode: café naïve"};

    for (int page = 0; page < testTexts.size(); ++page) {
        if (page > 0) {
            pdfWriter.newPage();
        }

        QRect textRect(100, 100, 400, 600);
        painter.drawText(textRect, Qt::TextWordWrap, testTexts[page]);
    }

    painter.end();

    auto doc = Poppler::Document::load(m_testPdfPath);
    if (doc && doc->numPages() > 0) {
        return doc.release();
    }
    return nullptr;
}

QTEST_MAIN(TestSearchComponents)
#include "test_search_components.moc"
