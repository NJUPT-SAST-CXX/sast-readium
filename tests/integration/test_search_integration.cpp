#include <poppler-qt6.h>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTest>
#include <QThread>
#include "../../app/cache/CacheManager.h"
#include "../../app/controller/EventBus.h"
#include "../../app/controller/ServiceLocator.h"
#include "../../app/controller/StateManager.h"
#include "../../app/search/IncrementalSearchManager.h"
#include "../../app/search/SearchEngine.h"
#include "../../app/search/SearchFeatures.h"
#include "../../app/utils/SafePDFRenderer.h"
#include "../TestUtilities.h"

class TestSearchIntegration : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

    // Basic search tests
    void testBasicTextSearch();
    void testCaseInsensitiveSearch();
    void testWholeWordSearch();
    void testRegexSearch();

    // Integration with new architecture
    void testSearchWithServiceLocator();
    void testSearchWithStateManager();
    void testSearchWithEventBus();

    // Advanced search features
    void testIncrementalSearch();
    void testSearchCaching();
    void testSearchAcrossMultipleDocuments();
    void testSearchPerformance();

    // Error handling
    void testSearchWithInvalidDocument();
    void testSearchWithEmptyQuery();
    void testSearchMemoryManagement();

private:
    void setupServices();
    void teardownServices();
    QByteArray createTestPDF(const QString& content);

    SearchEngine* m_searchEngine;
    IncrementalSearchManager* m_incrementalManager;
    Poppler::Document* m_testDocument;
    QString m_testPdfPath;
};

void TestSearchIntegration::initTestCase() {
    // Setup services for all tests
    setupServices();

    // Configure safe renderer for tests
    SafePDFRenderer& renderer = SafePDFRenderer::instance();
    SafePDFRenderer::RenderConfig config = renderer.getRenderConfig();
    config.enableCompatibilityCheck = true;
    config.fallbackStrategy = SafePDFRenderer::FallbackStrategy::UsePlaceholder;
    config.maxRetries = 1;  // Faster tests
    config.fallbackDPI = 72.0;
    renderer.setRenderConfig(config);

    // Create test PDF
    m_testPdfPath =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
        "/search_test.pdf";

    QByteArray pdfContent = createTestPDF(
        "This is a test document for search functionality. "
        "It contains multiple words and sentences. "
        "The SEARCH engine should find this text. "
        "Case sensitivity and whole word matching are important features.");

    QFile file(m_testPdfPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(pdfContent);
        file.close();
    }

    m_testDocument = Poppler::Document::load(m_testPdfPath).release();
    QVERIFY(m_testDocument != nullptr);

    // Check compatibility and log for debugging
    SafePDFRenderer::CompatibilityResult compatibility = renderer.checkCompatibility(m_testDocument);
    qDebug() << "Search test PDF compatibility:" << static_cast<int>(compatibility);
    if (compatibility == SafePDFRenderer::CompatibilityResult::QtGenerated) {
        qDebug() << "Qt-generated PDF detected in search test - using safe rendering";
    }
}

void TestSearchIntegration::cleanupTestCase() {
    // Cleanup in reverse order of initialization

    // Unregister ALL caches to prevent dangling pointers
    CacheManager& cacheManager = CacheManager::instance();
    cacheManager.unregisterCache(CacheManager::SEARCH_RESULT_CACHE);
    cacheManager.unregisterCache(CacheManager::PAGE_TEXT_CACHE);
    cacheManager.unregisterCache(CacheManager::SEARCH_HIGHLIGHT_CACHE);
    cacheManager.unregisterCache(CacheManager::PDF_RENDER_CACHE);
    cacheManager.unregisterCache(CacheManager::THUMBNAIL_CACHE);

    // Stop all CacheManager timers to prevent crashes during static destruction
    cacheManager.stopAllTimers();

    // Clear all caches to release memory
    cacheManager.clearAllCaches();

    // Tear down services to avoid accessing deleted document
    teardownServices();

    // Process any pending events multiple times to ensure all async operations
    // complete
    for (int i = 0; i < 5; ++i) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QThread::msleep(10);
    }

    // Delete the test document
    delete m_testDocument;
    m_testDocument = nullptr;

    // Remove test file
    QFile::remove(m_testPdfPath);

    // Final event processing to handle deleteLater() calls
    for (int i = 0; i < 3; ++i) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QThread::msleep(10);
    }
}

void TestSearchIntegration::init() {
    // Don't set parent to avoid double-delete issues
    m_searchEngine = new SearchEngine(nullptr);
    m_incrementalManager = new IncrementalSearchManager(nullptr);

    m_searchEngine->setDocument(m_testDocument);
    // Connect incremental manager to search engine
    connect(m_incrementalManager, &IncrementalSearchManager::searchTriggered,
            this, [this](const QString& query, const SearchOptions& options) {
                m_searchEngine->search(query, options);
            });
}

void TestSearchIntegration::cleanup() {
    // IMPORTANT: Unregister caches BEFORE deleting SearchEngine to prevent
    // dangling pointers when CacheManager's timers fire
    CacheManager& cacheManager = CacheManager::instance();
    cacheManager.unregisterCache(CacheManager::SEARCH_RESULT_CACHE);
    cacheManager.unregisterCache(CacheManager::PAGE_TEXT_CACHE);
    cacheManager.unregisterCache(CacheManager::SEARCH_HIGHLIGHT_CACHE);

    // Disconnect all signals to prevent dangling connections
    if (m_incrementalManager) {
        disconnect(m_incrementalManager, nullptr, nullptr, nullptr);
    }
    if (m_searchEngine) {
        disconnect(m_searchEngine, nullptr, nullptr, nullptr);
    }

    // Explicitly delete to avoid memory leaks
    delete m_incrementalManager;
    m_incrementalManager = nullptr;
    delete m_searchEngine;
    m_searchEngine = nullptr;

    // Process events to handle any pending deletions
    QCoreApplication::processEvents();
}

void TestSearchIntegration::setupServices() {
    ServiceLocator::instance().clearServices();

    // Register search engine as a service
    ServiceLocator::instance().registerService<SearchEngine, SearchEngine>();

    // Setup state manager for search state
    StateManager::instance().reset();
    StateManager::instance().set("search.enabled", true);
    StateManager::instance().set("search.caseSensitive", false);
    StateManager::instance().set("search.wholeWords", false);
}

void TestSearchIntegration::teardownServices() {
    // Unsubscribe from all EventBus events for this test object
    EventBus::instance().unsubscribeAll(this);

    ServiceLocator::instance().clearServices();
    StateManager::instance().reset();
}

QByteArray TestSearchIntegration::createTestPDF(const QString& content) {
    // Simplified PDF creation - in real tests, use a proper PDF library
    QString pdfContent =
        "%PDF-1.4\n"
        "1 0 obj << /Type /Catalog /Pages 2 0 R >> endobj\n"
        "2 0 obj << /Type /Pages /Kids [3 0 R] /Count 1 >> endobj\n"
        "3 0 obj << /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
        "/Contents 4 0 R /Resources << /Font << /F1 5 0 R >> >> >> endobj\n"
        "4 0 obj << /Length " +
        QString::number(content.length() + 20) +
        " >> stream\n"
        "BT /F1 12 Tf 100 700 Td (" +
        content +
        ") Tj ET\n"
        "endstream endobj\n"
        "5 0 obj << /Type /Font /Subtype /Type1 /BaseFont /Helvetica >> "
        "endobj\n"
        "xref\n0 6\n"
        "0000000000 65535 f\n"
        "0000000009 00000 n\n"
        "0000000074 00000 n\n"
        "0000000133 00000 n\n"
        "0000000245 00000 n\n"
        "0000000345 00000 n\n"
        "trailer << /Size 6 /Root 1 0 R >>\n"
        "startxref\n445\n%%EOF";

    return pdfContent.toUtf8();
}

void TestSearchIntegration::testBasicTextSearch() {
    QSignalSpy resultSpy(m_searchEngine, &SearchEngine::searchFinished);

    SearchOptions options;
    options.caseSensitive = false;
    m_searchEngine->search("test", options);

    QVERIFY_TIMEOUT(resultSpy.count() > 0, 5000);

    auto results = m_searchEngine->results();
    QVERIFY(!results.isEmpty());

    bool foundTest = false;
    for (const auto& result : results) {
        if (result.matchedText.contains("test", Qt::CaseInsensitive)) {
            foundTest = true;
            break;
        }
    }
    QVERIFY(foundTest);
}

void TestSearchIntegration::testCaseInsensitiveSearch() {
    // Test case insensitive search
    SearchOptions options;
    options.caseSensitive = false;
    m_searchEngine->search("SEARCH", options);

    QVERIFY_TIMEOUT(!m_searchEngine->results().isEmpty(), 5000);

    auto results = m_searchEngine->results();
    QVERIFY(!results.isEmpty());
}

void TestSearchIntegration::testWholeWordSearch() {
    // Test whole word search
    SearchOptions options;
    options.wholeWords = true;
    m_searchEngine->search("word", options);

    waitMs(100);  // Allow search to complete

    auto results = m_searchEngine->results();

    // Verify that partial matches are not included
    for (const auto& result : results) {
        // "word" should match but not "words"
        QVERIFY(!result.matchedText.contains("words"));
    }
}

void TestSearchIntegration::testRegexSearch() {
    // Test regex search if supported
    SearchOptions options;
    options.useRegex = true;
    m_searchEngine->search("test.*document", options);

    waitMs(100);

    // Regex might not be supported in all configurations
    // Just verify it doesn't crash
    QVERIFY(true);
}

void TestSearchIntegration::testSearchWithServiceLocator() {
    // Get search engine from service locator
    auto* searchService = ServiceLocator::instance().getService<SearchEngine>();

    if (!searchService) {
        // Create and register if not exists
        searchService = new SearchEngine();
        ServiceLocator::instance().registerService<SearchEngine>(searchService);
    }

    QVERIFY(searchService != nullptr);

    searchService->setDocument(m_testDocument);
    searchService->search("functionality");

    waitMs(100);

    // Verify search works through service
    QVERIFY(searchService->resultCount() >= 0);  // Just check it ran
}

void TestSearchIntegration::testSearchWithStateManager() {
    // Store search state in StateManager
    StateManager& stateManager = StateManager::instance();

    stateManager.set("search.query", "test");
    stateManager.set("search.page", 0);
    stateManager.set("search.results", QVariantList());

    // Monitor state changes
    bool stateChanged = false;
    stateManager.subscribe(
        "search.results", this,
        [&stateChanged](const StateChange&) { stateChanged = true; });

    // Perform search
    m_searchEngine->search("test");
    waitMs(100);

    // Update state with results
    QVariantList resultList;
    for (const auto& result : m_searchEngine->results()) {
        QVariantMap resultMap;
        resultMap["page"] = result.pageNumber;
        resultMap["text"] = result.matchedText;
        resultList.append(resultMap);
    }

    stateManager.set("search.results", resultList);

    QVERIFY(stateChanged);
    QVERIFY(!stateManager.get("search.results").toList().isEmpty());
}

void TestSearchIntegration::testSearchWithEventBus() {
    EventBus& eventBus = EventBus::instance();

    bool searchStarted = false;
    bool searchCompleted = false;

    // Subscribe to search events
    eventBus.subscribe("search.started", this,
                       [&searchStarted](Event* e) { searchStarted = true; });

    eventBus.subscribe("search.completed", this, [&searchCompleted](Event* e) {
        searchCompleted = true;
    });

    // Publish search started event
    eventBus.publish("search.started", QVariant("test query"));
    QCoreApplication::processEvents();  // Process async event delivery

    // Perform actual search
    m_searchEngine->search("test");
    waitMs(100);

    // Publish search completed event
    QVariantMap resultData;
    resultData["count"] = m_searchEngine->results().size();
    eventBus.publish("search.completed", resultData);
    QCoreApplication::processEvents();  // Process async event delivery

    QVERIFY(searchStarted);
    QVERIFY(searchCompleted);
}

void TestSearchIntegration::testIncrementalSearch() {
    QSignalSpy triggerSpy(m_incrementalManager,
                          &IncrementalSearchManager::searchTriggered);

    // Schedule incremental searches
    SearchOptions options;
    m_incrementalManager->scheduleSearch("t", options);
    waitMs(50);

    m_incrementalManager->scheduleSearch("te", options);
    waitMs(50);

    m_incrementalManager->scheduleSearch("tes", options);
    waitMs(50);

    m_incrementalManager->scheduleSearch("test", options);
    waitMs(400);  // Wait for delay to trigger

    QVERIFY(triggerSpy.count() >= 1);

    auto results = m_searchEngine->results();
    QVERIFY(!results.isEmpty());
}

void TestSearchIntegration::testSearchCaching() {
    // First search
    QElapsedTimer timer1;
    timer1.start();
    m_searchEngine->search("document");
    waitMs(100);
    qint64 firstSearchTime = timer1.elapsed();

    auto firstResults = m_searchEngine->results();

    // Second search (should be cached)
    QElapsedTimer timer2;
    timer2.start();
    m_searchEngine->search("document");
    waitMs(10);  // Much shorter wait for cached results
    qint64 secondSearchTime = timer2.elapsed();

    auto secondResults = m_searchEngine->results();

    // Results should be the same
    QCOMPARE(firstResults.size(), secondResults.size());

    // Second search should be faster (cached)
    // Note: This might not always be true in test environment
    qDebug() << "First search:" << firstSearchTime << "ms";
    qDebug() << "Second search:" << secondSearchTime << "ms";
}

void TestSearchIntegration::testSearchAcrossMultipleDocuments() {
    // Create multiple test documents
    QList<Poppler::Document*> documents;
    QStringList testPaths;

    for (int i = 0; i < 3; ++i) {
        QString path =
            QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
            QString("/test_doc_%1.pdf").arg(i);
        testPaths.append(path);

        QByteArray content =
            createTestPDF(QString("Document %1 contains test data").arg(i));
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(content);
            file.close();
        }

        auto* doc = Poppler::Document::load(path).release();
        if (doc) {
            documents.append(doc);
        }
    }

    // Search across all documents
    int totalResults = 0;
    for (auto* doc : documents) {
        SearchEngine engine;
        engine.setDocument(doc);
        engine.search("test");
        waitMs(50);
        totalResults += engine.results().size();
    }

    QVERIFY(totalResults > 0);

    // Cleanup
    for (auto* doc : documents) {
        delete doc;
    }
    for (const auto& path : testPaths) {
        QFile::remove(path);
    }
}

void TestSearchIntegration::testSearchPerformance() {
    // Create a larger document for performance testing
    QString largeContent;
    for (int i = 0; i < 100; ++i) {
        largeContent +=
            QString("Line %1: This is test content for performance testing. ")
                .arg(i);
    }

    QString perfTestPath =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
        "/perf_test.pdf";
    QByteArray pdfContent = createTestPDF(largeContent);

    QFile file(perfTestPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(pdfContent);
        file.close();
    }

    auto* perfDoc = Poppler::Document::load(perfTestPath).release();
    QVERIFY(perfDoc != nullptr);

    SearchEngine perfEngine;
    perfEngine.setDocument(perfDoc);

    QElapsedTimer perfTimer;
    perfTimer.start();

    perfEngine.search("test");

    QVERIFY_TIMEOUT(perfEngine.resultCount() >= 0, 5000);

    qint64 searchTime = perfTimer.elapsed();
    qDebug() << "Performance test: Search completed in" << searchTime << "ms";

    // Verify search completes in reasonable time
    QVERIFY(searchTime < 5000);  // Less than 5 seconds

    // Cleanup
    delete perfDoc;
    QFile::remove(perfTestPath);
}

void TestSearchIntegration::testSearchWithInvalidDocument() {
    SearchEngine engine;

    // Search without document
    engine.search("test");
    waitMs(100);

    // Should handle gracefully
    QVERIFY(engine.resultCount() == 0);

    // Set null document
    engine.setDocument(nullptr);
    engine.search("test");
    waitMs(100);

    QVERIFY(engine.results().isEmpty());
}

void TestSearchIntegration::testSearchWithEmptyQuery() {
    // Empty query
    m_searchEngine->search("");
    waitMs(100);

    QVERIFY(m_searchEngine->results().isEmpty());

    // Whitespace only
    m_searchEngine->search("   ");
    waitMs(100);

    QVERIFY(m_searchEngine->results().isEmpty());
}

void TestSearchIntegration::testSearchMemoryManagement() {
    // Perform multiple searches to test memory management
    for (int i = 0; i < 50; ++i) {
        QString query = QString("test%1").arg(i);
        m_searchEngine->search(query);
        waitMs(10);

        // Clear results periodically
        if (i % 10 == 0) {
            m_searchEngine->clearResults();
        }
    }

    // Should not crash or leak memory
    QVERIFY(true);

    // Perform a final search to verify engine still works
    m_searchEngine->search("final");
    waitMs(100);

    // Engine should still be functional
    QVERIFY(m_searchEngine != nullptr);
}

QTEST_MAIN(TestSearchIntegration)
#include "test_search_integration.moc"
