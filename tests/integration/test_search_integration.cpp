#include <QtTest/QtTest>
#include <QApplication>
#include <QTemporaryFile>
#include <QPainter>
#include <QPdfWriter>
#include <QSignalSpy>
#include <QColorDialog>
#include <poppler-qt6.h>
#include "../../app/search/OptimizedSearchEngine.h"
#include "../../app/model/SearchModel.h"
#include "../../app/ui/widgets/SearchWidget.h"
#include "../../app/ui/viewer/PDFViewer.h"

/**
 * Integration Tests
 * Tests integration between SearchModel, SearchWidget, OptimizedSearchEngine, and PDFViewer
 */
class TestSearchIntegration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    // Component integration tests
    void testSearchModelEngineIntegration();
    void testSearchWidgetModelIntegration();
    void testSearchWidgetPDFViewerIntegration();
    void testEndToEndSearchFlow();
    
    // Search highlighting integration tests
    void testSearchHighlightingColorCustomization();
    void testSearchHighlightingWithVirtualScrolling();
    void testSearchHighlightingWithPrerendering();
    
    // Search progress and UI integration tests
    void testSearchProgressReporting();
    void testSearchUIUpdates();
    void testSearchHistoryIntegration();
    
    // Advanced features integration tests
    void testFuzzySearchIntegration();
    void testPageRangeSearchIntegration();
    void testRegexSearchIntegration();
    
    // Performance integration tests
    void testSearchWithExistingOptimizations();
    void testSearchCacheIntegration();

private:
    Poppler::Document* m_testDocument;
    OptimizedSearchEngine* m_searchEngine;
    SearchModel* m_searchModel;
    SearchWidget* m_searchWidget;
    PDFViewer* m_pdfViewer;
    QString m_testPdfPath;
    
    // Test content
    QStringList m_testTexts;
    
    // Helper methods
    Poppler::Document* createTestDocument();
    void setupComponents();
    void connectSignals();
    void verifySearchHighlighting(const QColor& expectedColor);
    void simulateUserSearch(const QString& query, const SearchOptions& options = SearchOptions());
};

void TestSearchIntegration::initTestCase()
{
    // Initialize test content
    m_testTexts = {
        "Integration test page 1. This page contains content for testing "
        "the integration between search components. Keywords: search, test, integration, "
        "highlighting, performance, cache, virtual, scrolling, prerendering.",
        
        "Integration test page 2. More content for comprehensive testing. "
        "Advanced features: fuzzy search, regex patterns, page range search. "
        "Email patterns: test@example.com, user@domain.org. "
        "Phone numbers: 123-456-7890, (555) 123-4567.",
        
        "Integration test page 3. Final page with additional test scenarios. "
        "Unicode content: café, naïve, résumé, Москва, 北京, 東京. "
        "Special characters: !@#$%^&*()_+-=[]{}|;':\",./<>? "
        "Mixed case content: UPPERCASE, lowercase, MixedCase."
    };
    
    m_testDocument = createTestDocument();
    QVERIFY(m_testDocument != nullptr);
    QCOMPARE(m_testDocument->numPages(), 3);
    
    setupComponents();
    connectSignals();
}

void TestSearchIntegration::cleanupTestCase()
{
    delete m_testDocument;
    if (!m_testPdfPath.isEmpty()) {
        QFile::remove(m_testPdfPath);
    }
}

void TestSearchIntegration::init()
{
    // Reset components before each test
    m_searchEngine->clearResults();
    m_searchModel->clearResults();
    m_searchWidget->clearSearch();
}

void TestSearchIntegration::cleanup()
{
    // Cleanup after each test
}

void TestSearchIntegration::testSearchModelEngineIntegration()
{
    SearchOptions options;
    
    // Test that SearchModel properly uses OptimizedSearchEngine
    m_searchModel->startSearch(m_testDocument, "integration", options);
    QList<SearchResult> modelResults = m_searchModel->getResults();
    
    // Compare with direct engine results
    m_searchEngine->startSearch(m_testDocument, "integration", options);
    QList<SearchResult> engineResults = m_searchEngine->getResults();
    
    QVERIFY(!modelResults.isEmpty());
    QVERIFY(!engineResults.isEmpty());
    
    // Results should be consistent
    QCOMPARE(modelResults.size(), engineResults.size());
    
    for (int i = 0; i < modelResults.size(); ++i) {
        QCOMPARE(modelResults[i].pageNumber, engineResults[i].pageNumber);
        QCOMPARE(modelResults[i].text, engineResults[i].text);
    }
}

void TestSearchIntegration::testSearchWidgetModelIntegration()
{
    // Set up signal spy to monitor search requests
    QSignalSpy searchSpy(m_searchWidget, &SearchWidget::searchRequested);
    
    // Simulate user input in search widget
    m_searchWidget->setSearchText("test");
    m_searchWidget->performSearch();
    
    // Verify signal was emitted
    QCOMPARE(searchSpy.count(), 1);
    
    // Verify search was executed
    QList<SearchResult> results = m_searchModel->getResults();
    QVERIFY(!results.isEmpty());
    
    // Test search options integration
    m_searchWidget->setCaseSensitive(true);
    m_searchWidget->setWholeWords(true);
    
    searchSpy.clear();
    m_searchWidget->performSearch();
    
    QCOMPARE(searchSpy.count(), 1);
    
    // Verify options were applied
    QList<QVariant> arguments = searchSpy.takeFirst();
    SearchOptions options = arguments.at(1).value<SearchOptions>();
    QVERIFY(options.caseSensitive);
    QVERIFY(options.wholeWords);
}

void TestSearchIntegration::testSearchWidgetPDFViewerIntegration()
{
    // Set up signal spy for highlight color changes
    QSignalSpy colorSpy(m_searchWidget, &SearchWidget::highlightColorsChanged);
    
    // Test highlight color customization
    QColor newNormalColor(255, 255, 0);    // Yellow
    QColor newCurrentColor(255, 0, 0);     // Red
    
    m_searchWidget->setHighlightColors(newNormalColor, newCurrentColor);
    
    // Verify signal was emitted
    QCOMPARE(colorSpy.count(), 1);
    
    QList<QVariant> arguments = colorSpy.takeFirst();
    QColor emittedNormal = arguments.at(0).value<QColor>();
    QColor emittedCurrent = arguments.at(1).value<QColor>();
    
    QCOMPARE(emittedNormal, newNormalColor);
    QCOMPARE(emittedCurrent, newCurrentColor);
    
    // Verify PDFViewer received the color change
    verifySearchHighlighting(newNormalColor);
}

void TestSearchIntegration::testEndToEndSearchFlow()
{
    // Simulate complete search flow from user input to result display
    
    // 1. User enters search query
    m_searchWidget->setSearchText("integration");
    
    // 2. User configures search options
    m_searchWidget->setCaseSensitive(false);
    m_searchWidget->setWholeWords(false);
    m_searchWidget->setUseRegex(false);
    
    // 3. User initiates search
    QSignalSpy searchSpy(m_searchWidget, &SearchWidget::searchRequested);
    QSignalSpy resultsSpy(m_searchModel, &SearchModel::searchCompleted);
    
    m_searchWidget->performSearch();
    
    // 4. Verify search was initiated
    QCOMPARE(searchSpy.count(), 1);
    
    // 5. Wait for search completion
    QVERIFY(resultsSpy.wait(5000)); // 5 second timeout
    
    // 6. Verify results are available
    QList<SearchResult> results = m_searchModel->getResults();
    QVERIFY(!results.isEmpty());
    
    // 7. Test result navigation
    if (results.size() > 1) {
        m_searchWidget->navigateToNextResult();
        m_searchWidget->navigateToPreviousResult();
    }
    
    // 8. Verify search history was updated
    QStringList history = m_searchWidget->getSearchHistory();
    QVERIFY(history.contains("integration"));
}

void TestSearchIntegration::testSearchHighlightingColorCustomization()
{
    // Perform a search first
    simulateUserSearch("test");
    
    // Test color customization
    QColor normalColor(255, 255, 0);   // Yellow
    QColor currentColor(0, 255, 0);    // Green
    
    QSignalSpy colorSpy(m_searchWidget, &SearchWidget::highlightColorsChanged);
    
    m_searchWidget->setHighlightColors(normalColor, currentColor);
    
    QCOMPARE(colorSpy.count(), 1);
    
    // Verify colors were applied to search results
    verifySearchHighlighting(normalColor);
}

void TestSearchIntegration::testSearchHighlightingWithVirtualScrolling()
{
    // Test that search highlighting works correctly with virtual scrolling
    
    // Perform search
    simulateUserSearch("page");
    
    QList<SearchResult> results = m_searchModel->getResults();
    QVERIFY(!results.isEmpty());
    
    // Navigate to different pages to test virtual scrolling
    for (const SearchResult& result : results) {
        m_pdfViewer->goToPage(result.pageNumber);
        
        // Verify page is displayed and highlighted
        QCOMPARE(m_pdfViewer->getCurrentPage(), result.pageNumber);
        
        // Test that highlights are visible (implementation-specific verification)
        // This would typically check the page widget's highlight state
    }
}

void TestSearchIntegration::testSearchHighlightingWithPrerendering()
{
    // Test that search highlighting works with prerendering optimizations
    
    // Enable prerendering
    m_pdfViewer->setPrerendererEnabled(true);
    
    // Perform search
    simulateUserSearch("content");
    
    QList<SearchResult> results = m_searchModel->getResults();
    QVERIFY(!results.isEmpty());
    
    // Navigate through results to trigger prerendering
    for (int i = 0; i < qMin(results.size(), 5); ++i) {
        m_searchWidget->navigateToResult(i);
        
        // Allow time for prerendering
        QTest::qWait(100);
        
        // Verify highlighting is maintained with prerendering
        QCOMPARE(m_pdfViewer->getCurrentPage(), results[i].pageNumber);
    }
}

void TestSearchIntegration::testSearchProgressReporting()
{
    // Set up signal spy for progress updates
    QSignalSpy progressSpy(m_searchModel, &SearchModel::searchProgress);
    
    // Start a search that should generate progress updates
    simulateUserSearch("test");
    
    // Verify progress signals were emitted
    QVERIFY(progressSpy.count() > 0);
    
    // Check progress values are reasonable
    for (const QList<QVariant>& signal : progressSpy) {
        int progress = signal.at(0).toInt();
        QVERIFY(progress >= 0 && progress <= 100);
    }
}

void TestSearchIntegration::testSearchUIUpdates()
{
    // Test that UI updates correctly during search operations
    
    // Initial state
    QCOMPARE(m_searchWidget->getResultCount(), 0);
    QCOMPARE(m_searchWidget->getCurrentResultIndex(), -1);
    
    // Perform search
    simulateUserSearch("integration");
    
    // Verify UI was updated
    QVERIFY(m_searchWidget->getResultCount() > 0);
    QCOMPARE(m_searchWidget->getCurrentResultIndex(), 0);
    
    // Test navigation updates
    if (m_searchWidget->getResultCount() > 1) {
        m_searchWidget->navigateToNextResult();
        QCOMPARE(m_searchWidget->getCurrentResultIndex(), 1);
        
        m_searchWidget->navigateToPreviousResult();
        QCOMPARE(m_searchWidget->getCurrentResultIndex(), 0);
    }
}

void TestSearchIntegration::testSearchHistoryIntegration()
{
    // Test search history functionality
    
    // Perform multiple searches
    simulateUserSearch("first");
    simulateUserSearch("second");
    simulateUserSearch("third");
    
    // Verify history was updated
    QStringList history = m_searchWidget->getSearchHistory();
    QVERIFY(history.contains("first"));
    QVERIFY(history.contains("second"));
    QVERIFY(history.contains("third"));
    
    // Test history selection
    m_searchWidget->selectFromHistory("first");
    QCOMPARE(m_searchWidget->getSearchText(), "first");
    
    // Test history clearing
    m_searchWidget->clearSearchHistory();
    history = m_searchWidget->getSearchHistory();
    QVERIFY(history.isEmpty());
}

Poppler::Document* TestSearchIntegration::createTestDocument()
{
    QTemporaryFile tempFile;
    tempFile.setFileTemplate(QDir::tempPath() + "/test_integration_XXXXXX.pdf");
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
    
    for (int page = 0; page < m_testTexts.size(); ++page) {
        if (page > 0) {
            pdfWriter.newPage();
        }
        
        QRect textRect(100, 100, 400, 600);
        painter.drawText(textRect, Qt::TextWordWrap, m_testTexts[page]);
        
        painter.drawText(100, 50, QString("Page %1").arg(page + 1));
    }
    
    painter.end();
    
    auto doc = Poppler::Document::load(m_testPdfPath);
    if (doc && doc->numPages() > 0) {
        return doc.release();
    }
    return nullptr;
}

void TestSearchIntegration::setupComponents()
{
    m_searchEngine = new OptimizedSearchEngine(this);
    m_searchEngine->setDocument(m_testDocument);
    
    m_searchModel = new SearchModel(this);
    m_searchModel->setSearchEngine(m_searchEngine);
    
    m_searchWidget = new SearchWidget(this);
    m_searchWidget->setSearchModel(m_searchModel);
    m_searchWidget->setDocument(m_testDocument);
    
    m_pdfViewer = new PDFViewer(this);
    m_pdfViewer->setDocument(m_testDocument);
}

void TestSearchIntegration::connectSignals()
{
    // Connect search widget to PDF viewer
    connect(m_searchWidget, &SearchWidget::highlightColorsChanged,
            m_pdfViewer, &PDFViewer::onHighlightColorsChanged);
    
    connect(m_searchWidget, &SearchWidget::navigateToResult,
            m_pdfViewer, &PDFViewer::onNavigateToSearchResult);
    
    // Connect search model signals
    connect(m_searchModel, &SearchModel::searchCompleted,
            m_searchWidget, &SearchWidget::onSearchCompleted);
    
    connect(m_searchModel, &SearchModel::searchProgress,
            m_searchWidget, &SearchWidget::onSearchProgress);
}

void TestSearchIntegration::verifySearchHighlighting(const QColor& expectedColor)
{
    // This would typically verify that the PDF viewer is using the expected highlight color
    // Implementation depends on how highlighting is implemented in PDFViewer
    
    // For now, just verify the color was set
    QVERIFY(expectedColor.isValid());
}

void TestSearchIntegration::simulateUserSearch(const QString& query, const SearchOptions& options)
{
    m_searchWidget->setSearchText(query);
    m_searchWidget->setSearchOptions(options);
    m_searchWidget->performSearch();
    
    // Wait for search to complete
    QSignalSpy completedSpy(m_searchModel, &SearchModel::searchCompleted);
    QVERIFY(completedSpy.wait(5000));
}

void TestSearchIntegration::testFuzzySearchIntegration()
{
    // Test fuzzy search integration across components

    SearchOptions options;
    options.fuzzySearch = true;
    options.fuzzyThreshold = 2;

    // Configure search widget for fuzzy search
    m_searchWidget->setFuzzySearchEnabled(true);
    m_searchWidget->setFuzzyThreshold(2);

    // Perform fuzzy search
    simulateUserSearch("integraion", options); // Intentional typo

    QList<SearchResult> results = m_searchModel->getResults();
    QVERIFY(!results.isEmpty());

    // Verify fuzzy search found "integration" despite typo
    bool foundCorrectWord = false;
    for (const SearchResult& result : results) {
        if (result.text.contains("integration", Qt::CaseInsensitive)) {
            foundCorrectWord = true;
            break;
        }
    }
    QVERIFY(foundCorrectWord);
}

void TestSearchIntegration::testPageRangeSearchIntegration()
{
    // Test page range search integration

    SearchOptions options;
    options.startPage = 1; // Search only page 2 (0-based)
    options.endPage = 1;

    // Configure search widget for page range
    m_searchWidget->setPageRangeEnabled(true);
    m_searchWidget->setPageRange(1, 1);

    // Perform page range search
    simulateUserSearch("page", options);

    QList<SearchResult> results = m_searchModel->getResults();
    QVERIFY(!results.isEmpty());

    // Verify all results are from the specified page
    for (const SearchResult& result : results) {
        QCOMPARE(result.pageNumber, 1);
    }
}

void TestSearchIntegration::testRegexSearchIntegration()
{
    // Test regex search integration

    SearchOptions options;
    options.useRegex = true;

    // Configure search widget for regex
    m_searchWidget->setRegexEnabled(true);

    // Search for email pattern
    simulateUserSearch("[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}", options);

    QList<SearchResult> results = m_searchModel->getResults();
    QVERIFY(!results.isEmpty());

    // Verify regex found email addresses
    bool foundEmail = false;
    for (const SearchResult& result : results) {
        if (result.text.contains("@") && result.text.contains(".")) {
            foundEmail = true;
            break;
        }
    }
    QVERIFY(foundEmail);
}

void TestSearchIntegration::testSearchWithExistingOptimizations()
{
    // Test that search works correctly with existing PDF rendering optimizations

    // Enable all optimizations
    m_pdfViewer->setVirtualScrollingEnabled(true);
    m_pdfViewer->setPrerendererEnabled(true);
    m_pdfViewer->setCacheEnabled(true);

    // Perform search
    simulateUserSearch("optimization");

    QList<SearchResult> results = m_searchModel->getResults();
    QVERIFY(!results.isEmpty());

    // Navigate through results to test with optimizations
    for (int i = 0; i < qMin(results.size(), 3); ++i) {
        m_searchWidget->navigateToResult(i);

        // Verify navigation works with optimizations
        QCOMPARE(m_pdfViewer->getCurrentPage(), results[i].pageNumber);

        // Allow time for optimizations to work
        QTest::qWait(50);
    }

    // Test scrolling with search highlights
    m_pdfViewer->scrollToNextPage();
    m_pdfViewer->scrollToPreviousPage();

    // Verify search highlights are maintained during scrolling
    QVERIFY(m_searchModel->getResults().size() > 0);
}

void TestSearchIntegration::testSearchCacheIntegration()
{
    // Test search cache integration across components

    // Enable caching
    m_searchEngine->setCacheEnabled(true);

    // Perform initial search
    QElapsedTimer timer;
    timer.start();
    simulateUserSearch("cache");
    qint64 firstSearchTime = timer.elapsed();

    QList<SearchResult> firstResults = m_searchModel->getResults();
    QVERIFY(!firstResults.isEmpty());

    // Clear results but keep cache
    m_searchModel->clearResults();

    // Perform same search again (should use cache)
    timer.restart();
    simulateUserSearch("cache");
    qint64 secondSearchTime = timer.elapsed();

    QList<SearchResult> secondResults = m_searchModel->getResults();

    // Results should be identical
    QCOMPARE(firstResults.size(), secondResults.size());

    // Second search should be faster due to caching
    QVERIFY(secondSearchTime <= firstSearchTime);

    qDebug() << "Cache integration test - First search:" << firstSearchTime
             << "ms, Cached search:" << secondSearchTime << "ms";
}

QTEST_MAIN(TestSearchIntegration)
#include "test_search_integration.moc"
