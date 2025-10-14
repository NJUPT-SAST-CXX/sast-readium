#include <QColor>
#include <QObject>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QtTest/QtTest>
#include "../../app/search/SearchConfiguration.h"
#include "../TestUtilities.h"

/**
 * Comprehensive tests for SearchConfiguration structures and classes
 * Tests SearchOptions, SearchResult, and SearchEngineConfig
 */
class SearchConfigurationTest : public TestBase {
    Q_OBJECT

private slots:
    void testSearchOptionsDefaults();
    void testSearchOptionsEquality();
    void testSearchOptionsConfiguration();
    void testSearchOptionsValidation();

    void testSearchResultConstruction();
    void testSearchResultValidation();
    void testSearchResultTransformation();
    void testSearchResultCompatibility();

    void testSearchEngineConfigDefaults();
    void testSearchEngineConfigValidation();

private:
    SearchOptions createBasicOptions();
    SearchOptions createAdvancedOptions();
    SearchResult createTestResult();
    void verifySearchOptions(const SearchOptions& options, bool caseSensitive,
                             bool wholeWords);
    void verifySearchResult(const SearchResult& result, int page,
                            const QString& text);
};

void SearchConfigurationTest::testSearchOptionsDefaults() {
    SearchOptions options;

    // Test default values
    QCOMPARE(options.caseSensitive, false);
    QCOMPARE(options.wholeWords, false);
    QCOMPARE(options.useRegex, false);
    QCOMPARE(options.searchBackward, false);
    QCOMPARE(options.maxResults, 1000);
    QCOMPARE(options.contextLength, 50);
    QCOMPARE(options.highlightColor, QString("#FFFF00"));

    // Test advanced features defaults
    QCOMPARE(options.fuzzySearch, false);
    QCOMPARE(options.fuzzyThreshold, 2);
    QCOMPARE(options.startPage, -1);
    QCOMPARE(options.endPage, -1);
    QCOMPARE(options.searchInSelection, false);

    // Test performance options defaults
    QCOMPARE(options.useIndexedSearch, true);
    QCOMPARE(options.enableSearchCache, true);
    QCOMPARE(options.enableIncrementalSearch, true);
    QCOMPARE(options.searchTimeout, 30000);
}

void SearchConfigurationTest::testSearchOptionsEquality() {
    SearchOptions options1;
    SearchOptions options2;

    // Test equality of default options
    QVERIFY(options1 == options2);

    // Test inequality after modification
    options2.caseSensitive = true;
    QVERIFY(!(options1 == options2));

    // Test equality after matching modification
    options1.caseSensitive = true;
    QVERIFY(options1 == options2);

    // Test complex equality
    options1.maxResults = 500;
    options1.highlightColor = "#FF0000";
    options1.fuzzySearch = true;

    options2.maxResults = 500;
    options2.highlightColor = "#FF0000";
    options2.fuzzySearch = true;

    QVERIFY(options1 == options2);
}

void SearchConfigurationTest::testSearchOptionsConfiguration() {
    SearchOptions options;

    // Test basic configuration
    options.caseSensitive = true;
    options.wholeWords = true;
    options.useRegex = true;
    options.maxResults = 100;
    options.contextLength = 25;

    verifySearchOptions(options, true, true);
    QCOMPARE(options.useRegex, true);
    QCOMPARE(options.maxResults, 100);
    QCOMPARE(options.contextLength, 25);

    // Test advanced configuration
    options.fuzzySearch = true;
    options.fuzzyThreshold = 3;
    options.startPage = 1;
    options.endPage = 10;
    options.searchInSelection = true;
    options.selectionRect = QRectF(10, 10, 100, 100);

    QCOMPARE(options.fuzzySearch, true);
    QCOMPARE(options.fuzzyThreshold, 3);
    QCOMPARE(options.startPage, 1);
    QCOMPARE(options.endPage, 10);
    QCOMPARE(options.searchInSelection, true);
    QCOMPARE(options.selectionRect, QRectF(10, 10, 100, 100));
}

void SearchConfigurationTest::testSearchOptionsValidation() {
    SearchOptions options;

    // Test valid ranges
    options.maxResults = 1;
    QVERIFY(options.maxResults > 0);

    options.maxResults = 10000;
    QVERIFY(options.maxResults <= 10000);

    options.contextLength = 0;
    QVERIFY(options.contextLength >= 0);

    options.fuzzyThreshold = 1;
    QVERIFY(options.fuzzyThreshold >= 1);

    // Test page range validation
    options.startPage = 1;
    options.endPage = 5;
    QVERIFY(options.startPage <= options.endPage || options.endPage == -1);
}

void SearchConfigurationTest::testSearchResultConstruction() {
    // Test default construction
    SearchResult result1;
    QCOMPARE(result1.pageNumber, -1);
    QVERIFY(result1.matchedText.isEmpty());
    QVERIFY(result1.contextText.isEmpty());
    QVERIFY(result1.boundingRect.isEmpty());
    QCOMPARE(result1.textPosition, 0);
    QCOMPARE(result1.textLength, 0);
    QCOMPARE(result1.isCurrentResult, false);

    // Test parameterized construction
    SearchResult result2(1, "test", "This is a test", QRectF(10, 10, 50, 20),
                         10, 4);
    QCOMPARE(result2.pageNumber, 1);
    QCOMPARE(result2.matchedText, QString("test"));
    QCOMPARE(result2.contextText, QString("This is a test"));
    QCOMPARE(result2.boundingRect, QRectF(10, 10, 50, 20));
    QCOMPARE(result2.textPosition, 10);
    QCOMPARE(result2.textLength, 4);
}

void SearchConfigurationTest::testSearchResultValidation() {
    SearchResult result;

    // Test invalid result
    QVERIFY(!result.isValid());
    QVERIFY(!result.isValidForHighlight());

    // Test valid result
    result.pageNumber = 1;
    QVERIFY(result.isValid());
    QVERIFY(!result.isValidForHighlight());  // Still invalid for highlight
                                             // (empty rect)

    // Test valid for highlight
    result.boundingRect = QRectF(10, 10, 50, 20);
    QVERIFY(result.isValidForHighlight());
}

void SearchConfigurationTest::testSearchResultTransformation() {
    SearchResult result(1, "test", "This is a test", QRectF(10, 10, 50, 20), 10,
                        4);

    // Test coordinate transformation
    double scaleFactor = 2.0;
    int rotation = 0;
    QSizeF pageSize(200, 300);
    QSize widgetSize(400, 600);

    result.transformToWidgetCoordinates(scaleFactor, rotation, pageSize,
                                        widgetSize);

    // Verify transformation was applied (widgetRect should be set)
    QVERIFY(!result.widgetRect.isEmpty());
}

void SearchConfigurationTest::testSearchResultCompatibility() {
    SearchResult result(1, "test", "This is a test", QRectF(10, 10, 50, 20), 10,
                        4);

    // Test that the constructor properly initializes all members
    QCOMPARE(result.matchedText, QString("test"));
    QCOMPARE(result.contextText, QString("This is a test"));
    QCOMPARE(result.textPosition, 10);
    QCOMPARE(result.textLength, 4);
    QCOMPARE(result.pageNumber, 1);
    QCOMPARE(result.boundingRect, QRectF(10, 10, 50, 20));

    // Test modification
    result.matchedText = "modified";
    QCOMPARE(result.matchedText, QString("modified"));

    result.contextText = "modified context";
    QCOMPARE(result.contextText, QString("modified context"));
}

void SearchConfigurationTest::testSearchEngineConfigDefaults() {
    SearchEngineConfig config;

    // Test caching defaults
    QCOMPARE(config.enableCache, true);
    QCOMPARE(config.maxCacheMemory, 100 * 1024 * 1024);  // 100MB
    QCOMPARE(config.maxCacheEntries, 1000);

    // Test incremental search defaults
    QCOMPARE(config.enableIncrementalSearch, true);
    QCOMPARE(config.incrementalSearchDelay, 300);

    // Test background processing defaults
    QCOMPARE(config.enableBackgroundProcessing, true);
    QCOMPARE(config.maxBackgroundThreads, 4);
    QCOMPARE(config.textExtractionBatchSize, 10);

    // Test performance defaults
    QCOMPARE(config.searchProgressInterval, 10);
    QCOMPARE(config.prefetchAdjacentPages, true);
}

void SearchConfigurationTest::testSearchEngineConfigValidation() {
    SearchEngineConfig config;

    // Test valid memory limits
    config.maxCacheMemory = 50 * 1024 * 1024;  // 50MB
    QVERIFY(config.maxCacheMemory > 0);

    // Test valid thread counts
    config.maxBackgroundThreads = 2;
    QVERIFY(config.maxBackgroundThreads > 0);
    QVERIFY(config.maxBackgroundThreads <= 16);  // Reasonable upper limit

    // Test valid batch sizes
    config.textExtractionBatchSize = 5;
    QVERIFY(config.textExtractionBatchSize > 0);

    // Test valid intervals
    config.incrementalSearchDelay = 100;
    QVERIFY(config.incrementalSearchDelay >= 0);

    config.searchProgressInterval = 5;
    QVERIFY(config.searchProgressInterval > 0);
}

SearchOptions SearchConfigurationTest::createBasicOptions() {
    SearchOptions options;
    options.caseSensitive = false;
    options.wholeWords = false;
    options.maxResults = 100;
    return options;
}

SearchOptions SearchConfigurationTest::createAdvancedOptions() {
    SearchOptions options = createBasicOptions();
    options.fuzzySearch = true;
    options.fuzzyThreshold = 2;
    options.useIndexedSearch = true;
    options.enableSearchCache = true;
    return options;
}

SearchResult SearchConfigurationTest::createTestResult() {
    return SearchResult(1, "test", "This is a test document",
                        QRectF(10, 10, 50, 20), 10, 4);
}

void SearchConfigurationTest::verifySearchOptions(const SearchOptions& options,
                                                  bool caseSensitive,
                                                  bool wholeWords) {
    QCOMPARE(options.caseSensitive, caseSensitive);
    QCOMPARE(options.wholeWords, wholeWords);
}

void SearchConfigurationTest::verifySearchResult(const SearchResult& result,
                                                 int page,
                                                 const QString& text) {
    QCOMPARE(result.pageNumber, page);
    QVERIFY(result.matchedText.contains(text, Qt::CaseInsensitive));
}

QTEST_MAIN(SearchConfigurationTest)
#include "test_search_configuration.moc"
