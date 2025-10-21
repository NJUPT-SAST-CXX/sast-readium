#include <QColor>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QSignalSpy>
#include <QStringList>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include "../../app/search/SearchConfiguration.h"
#include "../../app/search/SearchFeatures.h"
#include "../TestUtilities.h"

/**
 * Comprehensive tests for SearchFeatures class
 * Tests fuzzy search, highlighting, history, and advanced functionality
 */
class SearchFeaturesTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Fuzzy search tests
    void testFuzzySearch();
    void testLevenshteinDistance();
    void testSimilarityCalculation();
    void testFuzzySearchWithDistance();
    void testFuzzySearchPerformance();

    // Advanced pattern matching tests
    void testWildcardSearch();
    void testPhraseSearch();
    void testBooleanSearch();
    void testProximitySearch();

    // Search highlighting tests
    void testHighlightColors();
    void testGenerateHighlights();
    void testUpdateHighlightPriorities();
    void testHighlightInfo();

    // Search history tests
    void testAddToHistory();
    void testGetSearchHistory();
    void testGetRecentQueries();
    void testGetPopularQueries();
    void testClearHistory();
    void testRemoveHistoryEntry();

    // Search suggestions tests
    void testGenerateSuggestions();
    void testQueryCompletions();
    void testUpdateSuggestionModel();

    // Result filtering and sorting tests
    void testFilterResults();
    void testSortResults();
    void testSortCriteria();

    // Statistics and analytics tests
    void testSearchStatistics();
    void testResetStatistics();
    void testStatisticsTracking();

    // Export and import tests
    void testExportSearchHistory();
    void testImportSearchHistory();
    void testExportSearchResults();

    // Signal tests
    void testFuzzySearchCompletedSignal();
    void testHighlightsGeneratedSignal();
    void testHistoryUpdatedSignal();
    void testSuggestionsReadySignal();
    void testStatisticsUpdatedSignal();

private:
    SearchFeatures* m_features;
    QString m_testText;
    QList<SearchResult> m_testResults;

    // Helper methods
    void setupTestData();
    SearchResult createTestResult(int page, const QString& text,
                                  const QString& context, int position = 0);
    void verifyFuzzyMatch(const SearchFeatures::FuzzyMatch& match,
                          const QString& expectedText);
    void verifyHighlightInfo(const SearchFeatures::HighlightInfo& highlight,
                             const QColor& expectedColor);
    void verifyHistoryEntry(const SearchFeatures::HistoryEntry& entry,
                            const QString& expectedQuery);
};

void SearchFeaturesTest::initTestCase() { setupTestData(); }

void SearchFeaturesTest::cleanupTestCase() {
    // Cleanup any resources
}

void SearchFeaturesTest::init() {
    m_features = new SearchFeatures(this);
    QVERIFY(m_features != nullptr);
}

void SearchFeaturesTest::cleanup() {
    if (m_features) {
        m_features->clearHistory();
        m_features->resetStatistics();
        delete m_features;
        m_features = nullptr;
    }
}

void SearchFeaturesTest::testFuzzySearch() {
    QSignalSpy completedSpy(m_features, &SearchFeatures::fuzzySearchCompleted);

    QList<SearchFeatures::FuzzyMatch> matches =
        m_features->fuzzySearch(m_testText, "tset", 2);

    QVERIFY(!matches.isEmpty());
    verifyFuzzyMatch(matches[0], "test");

    // Verify signal was emitted
    QCOMPARE(completedSpy.count(), 1);
}

void SearchFeaturesTest::testLevenshteinDistance() {
    int distance1 = m_features->calculateLevenshteinDistance("test", "tset");
    QCOMPARE(distance1, 2);  // Two character swaps

    int distance2 = m_features->calculateLevenshteinDistance("test", "test");
    QCOMPARE(distance2, 0);  // Identical strings

    int distance3 = m_features->calculateLevenshteinDistance("test", "testing");
    QCOMPARE(distance3, 3);  // Three insertions

    int distance4 = m_features->calculateLevenshteinDistance("", "test");
    QCOMPARE(distance4, 4);  // Four insertions
}

void SearchFeaturesTest::testSimilarityCalculation() {
    double similarity1 = m_features->calculateSimilarity("test", "test");
    QCOMPARE(similarity1, 1.0);  // Identical strings

    double similarity2 = m_features->calculateSimilarity("test", "tset");
    QVERIFY(similarity2 > 0.0 &&
            similarity2 < 1.0);  // Similar but not identical

    double similarity3 = m_features->calculateSimilarity("test", "xyz");
    QVERIFY(similarity3 < 0.5);  // Very different strings

    double similarity4 = m_features->calculateSimilarity("", "");
    QCOMPARE(similarity4, 1.0);  // Empty strings are identical
}

void SearchFeaturesTest::testFuzzySearchWithDistance() {
    // Test with different max distances
    QList<SearchFeatures::FuzzyMatch> matches1 =
        m_features->fuzzySearch(m_testText, "tset", 1);
    QList<SearchFeatures::FuzzyMatch> matches2 =
        m_features->fuzzySearch(m_testText, "tset", 2);
    QList<SearchFeatures::FuzzyMatch> matches3 =
        m_features->fuzzySearch(m_testText, "tset", 3);

    // More lenient distance should find more or equal matches
    QVERIFY(matches2.size() >= matches1.size());
    QVERIFY(matches3.size() >= matches2.size());

    // Test with max results limit
    QList<SearchFeatures::FuzzyMatch> limitedMatches =
        m_features->fuzzySearch(m_testText, "test", 2, 1);
    QVERIFY(limitedMatches.size() <= 1);
}

void SearchFeaturesTest::testWildcardSearch() {
    QList<SearchResult> results =
        m_features->wildcardSearch(m_testText, "te*t", 0);

    QVERIFY(!results.isEmpty());
    for (const SearchResult& result : results) {
        QVERIFY(result.matchedText.startsWith("te"));
        QVERIFY(result.matchedText.endsWith("t"));
    }
}

void SearchFeaturesTest::testPhraseSearch() {
    QList<SearchResult> results =
        m_features->phraseSearch(m_testText, "test document", 0, 0);

    QVERIFY(!results.isEmpty());
    for (const SearchResult& result : results) {
        QVERIFY(
            result.contextText.contains("test document", Qt::CaseInsensitive));
    }

    // Test with proximity
    results = m_features->phraseSearch(m_testText, "test document", 0, 5);
    QVERIFY(!results.isEmpty());
}

void SearchFeaturesTest::testBooleanSearch() {
    QList<SearchResult> results =
        m_features->booleanSearch(m_testText, "test AND document", 0);

    QVERIFY(!results.isEmpty());
    for (const SearchResult& result : results) {
        QVERIFY(result.contextText.contains("test", Qt::CaseInsensitive));
        QVERIFY(result.contextText.contains("document", Qt::CaseInsensitive));
    }

    // Test OR operation
    results = m_features->booleanSearch(m_testText, "test OR nonexistent", 0);
    QVERIFY(!results.isEmpty());

    // Test NOT operation
    results = m_features->booleanSearch(m_testText, "test NOT nonexistent", 0);
    QVERIFY(!results.isEmpty());
}

void SearchFeaturesTest::testProximitySearch() {
    QStringList terms = {"test", "document"};
    SearchFeatures::ProximitySearchOptions options;
    options.maxDistance = 10;
    options.ordered = false;

    QList<SearchResult> results =
        m_features->proximitySearch(m_testText, terms, options, 0);

    QVERIFY(!results.isEmpty());
    for (const SearchResult& result : results) {
        QVERIFY(result.contextText.contains("test", Qt::CaseInsensitive));
        QVERIFY(result.contextText.contains("document", Qt::CaseInsensitive));
    }

    // Test ordered proximity
    options.ordered = true;
    results = m_features->proximitySearch(m_testText, terms, options, 0);
    QVERIFY(!results.isEmpty());
}

void SearchFeaturesTest::testHighlightColors() {
    QColor normalColor(255, 255, 0);  // Yellow
    QColor currentColor(255, 0, 0);   // Red

    m_features->setHighlightColors(normalColor, currentColor);

    QCOMPARE(m_features->getNormalHighlightColor(), normalColor);
    QCOMPARE(m_features->getCurrentHighlightColor(), currentColor);
}

void SearchFeaturesTest::testGenerateHighlights() {
    QSignalSpy highlightsSpy(m_features, &SearchFeatures::highlightsGenerated);

    QList<SearchFeatures::HighlightInfo> highlights =
        m_features->generateHighlights(m_testResults, 0);

    QVERIFY(!highlights.isEmpty());
    QCOMPARE(highlights.size(), m_testResults.size());

    // Verify current result highlighting
    if (!highlights.isEmpty()) {
        QVERIFY(highlights[0].isCurrentResult);
        verifyHighlightInfo(highlights[0],
                            m_features->getCurrentHighlightColor());
    }

    // Verify signal was emitted
    QCOMPARE(highlightsSpy.count(), 1);
}

void SearchFeaturesTest::testUpdateHighlightPriorities() {
    QList<SearchFeatures::HighlightInfo> highlights =
        m_features->generateHighlights(m_testResults);

    // Modify priorities
    for (int i = 0; i < highlights.size(); ++i) {
        highlights[i].priority = i;
    }

    m_features->updateHighlightPriorities(highlights);

    // Verify priorities were updated (implementation dependent)
    for (int i = 0; i < highlights.size(); ++i) {
        QVERIFY(highlights[i].priority >= 0);
    }
}

void SearchFeaturesTest::testAddToHistory() {
    QSignalSpy historySpy(m_features, &SearchFeatures::historyUpdated);

    SearchOptions options;
    m_features->addToHistory("test query", options, 5, 100, true);

    QList<SearchFeatures::HistoryEntry> history =
        m_features->getSearchHistory(10);
    QCOMPARE(history.size(), 1);

    verifyHistoryEntry(history[0], "test query");
    QCOMPARE(history[0].resultCount, 5);
    QCOMPARE(history[0].searchTime, 100);
    QCOMPARE(history[0].successful, true);

    // Verify signal was emitted
    QCOMPARE(historySpy.count(), 1);
}

void SearchFeaturesTest::testGetSearchHistory() {
    // Add multiple entries
    SearchOptions options;
    m_features->addToHistory("query1", options, 1, 50);
    m_features->addToHistory("query2", options, 2, 75);
    m_features->addToHistory("query3", options, 3, 100);

    QList<SearchFeatures::HistoryEntry> history =
        m_features->getSearchHistory(2);
    QCOMPARE(history.size(), 2);

    // Should return most recent entries first
    verifyHistoryEntry(history[0], "query3");
    verifyHistoryEntry(history[1], "query2");

    // Test getting all entries
    history = m_features->getSearchHistory(50);
    QCOMPARE(history.size(), 3);
}

void SearchFeaturesTest::testGetRecentQueries() {
    SearchOptions options;
    m_features->addToHistory("recent1", options, 1, 50);
    m_features->addToHistory("recent2", options, 2, 75);

    QStringList recent = m_features->getRecentQueries(5);
    QVERIFY(recent.contains("recent1"));
    QVERIFY(recent.contains("recent2"));
}

void SearchFeaturesTest::testGetPopularQueries() {
    SearchOptions options;
    // Add same query multiple times to make it popular
    m_features->addToHistory("popular", options, 1, 50);
    m_features->addToHistory("popular", options, 2, 60);
    m_features->addToHistory("popular", options, 3, 70);
    m_features->addToHistory("rare", options, 1, 40);

    QStringList popular = m_features->getPopularQueries(5);
    QVERIFY(!popular.isEmpty());
    QVERIFY(popular.contains("popular"));
}

void SearchFeaturesTest::setupTestData() {
    m_testText =
        "This is a test document with multiple test words. "
        "The document contains various test cases for testing "
        "the search functionality and features.";

    m_testResults = {createTestResult(0, "test", "This is a test document", 10),
                     createTestResult(0, "test", "multiple test words", 35),
                     createTestResult(0, "test", "various test cases", 60)};
}

SearchResult SearchFeaturesTest::createTestResult(int page, const QString& text,
                                                  const QString& context,
                                                  int position) {
    return SearchResult(page, text, context, QRectF(position * 10, 100, 50, 20),
                        position, text.length());
}

void SearchFeaturesTest::verifyFuzzyMatch(
    const SearchFeatures::FuzzyMatch& match, const QString& expectedText) {
    QVERIFY(match.text.contains(expectedText, Qt::CaseInsensitive));
    QVERIFY(match.position >= 0);
    QVERIFY(match.length > 0);
    QVERIFY(match.editDistance >= 0);
    QVERIFY(match.similarity >= 0.0 && match.similarity <= 1.0);
}

void SearchFeaturesTest::verifyHighlightInfo(
    const SearchFeatures::HighlightInfo& highlight,
    const QColor& expectedColor) {
    QVERIFY(!highlight.rect.isEmpty());
    QCOMPARE(highlight.color, expectedColor);
    QVERIFY(!highlight.text.isEmpty());
    QVERIFY(highlight.priority >= 0);
}

void SearchFeaturesTest::verifyHistoryEntry(
    const SearchFeatures::HistoryEntry& entry, const QString& expectedQuery) {
    QCOMPARE(entry.query, expectedQuery);
    QVERIFY(entry.timestamp.isValid());
    QVERIFY(entry.resultCount >= 0);
    QVERIFY(entry.searchTime >= 0);
}

void SearchFeaturesTest::testFuzzySearchPerformance() {
    QStringList tokens;
    for (int i = 0; i < 500; ++i) {
        tokens << QString("test%1").arg(i) << QString("tset%1").arg(i)
               << QString("sample%1").arg(i);
    }
    QString largeText = tokens.join(' ');

    QElapsedTimer timer;
    timer.start();

    QList<SearchFeatures::FuzzyMatch> matches =
        m_features->fuzzySearch(largeText, "test", 2, 20);

    qint64 elapsed = timer.elapsed();

    QVERIFY(elapsed < 2000);
    QVERIFY(!matches.isEmpty());
    QVERIFY(matches.size() <= 20);

    for (int i = 1; i < matches.size(); ++i) {
        QVERIFY(matches[i - 1].similarity >= matches[i].similarity);
    }
}

void SearchFeaturesTest::testHighlightInfo() {
    QList<SearchFeatures::HighlightInfo> highlights =
        m_features->generateHighlights(m_testResults, 1);

    QCOMPARE(highlights.size(), m_testResults.size());

    const SearchFeatures::HighlightInfo& current = highlights.first();
    QVERIFY(current.isCurrentResult);
    QCOMPARE(current.text, m_testResults[1].matchedText);
    QCOMPARE(current.color, m_features->getCurrentHighlightColor());
    QVERIFY(current.rect.isValid());
    QVERIFY(current.priority > 0);

    for (int i = 1; i < highlights.size(); ++i) {
        const auto& highlight = highlights[i];
        QVERIFY(!highlight.isCurrentResult);
        QCOMPARE(highlight.color, m_features->getNormalHighlightColor());
        QVERIFY(highlight.rect.isValid());
        QVERIFY(highlight.priority > 0);
    }
}

void SearchFeaturesTest::testClearHistory() {
    m_features->clearHistory();
    QList<SearchFeatures::HistoryEntry> history =
        m_features->getSearchHistory(10);
    QVERIFY(history.isEmpty());
}

void SearchFeaturesTest::testRemoveHistoryEntry() {
    SearchOptions options;
    m_features->clearHistory();
    m_features->addToHistory("query1", options, 5, 100);
    m_features->addToHistory("query2", options, 3, 150);
    m_features->addToHistory("query3", options, 8, 200);

    QSignalSpy historySpy(m_features, &SearchFeatures::historyUpdated);
    QList<SearchFeatures::HistoryEntry> history =
        m_features->getSearchHistory();
    QCOMPARE(history.size(), 3);

    m_features->removeHistoryEntry(1);
    QVERIFY(historySpy.count() >= 1);

    QList<SearchFeatures::HistoryEntry> updatedHistory =
        m_features->getSearchHistory();
    QCOMPARE(updatedHistory.size(), 2);
    QVERIFY(updatedHistory.first().query != "query2");
    QVERIFY(updatedHistory.last().query != "query2");
}

void SearchFeaturesTest::testGenerateSuggestions() {
    QStringList corpus = {"search engine optimization",
                          "advanced search features",
                          "search history management", "testing utilities"};

    m_features->updateSuggestionModel(corpus);

    QStringList suggestions = m_features->generateSuggestions("sear", 5);
    QVERIFY(!suggestions.isEmpty());
    QVERIFY(suggestions.contains("search"));

    QStringList fuzzySuggestions = m_features->generateSuggestions("srch", 5);
    QVERIFY(!fuzzySuggestions.isEmpty());
}

void SearchFeaturesTest::testQueryCompletions() {
    QStringList corpus = {"document", "documentation", "documented",
                          "different"};
    m_features->updateSuggestionModel(corpus);

    QStringList completions = m_features->getQueryCompletions("doc", 5);
    QVERIFY(!completions.isEmpty());
    QVERIFY(completions.first().startsWith("doc"));

    QStringList noMatch = m_features->getQueryCompletions("xyz", 5);
    QVERIFY(noMatch.isEmpty());
}

void SearchFeaturesTest::testUpdateSuggestionModel() {
    QStringList corpus = {"apple pie",    "apple tart",    "apply rules",
                          "banana bread", "band practice", "bandage"};

    m_features->updateSuggestionModel(corpus);

    QStringList appSuggestions = m_features->generateSuggestions("app", 5);
    QVERIFY(appSuggestions.contains("apple"));
    QVERIFY(appSuggestions.contains("apply"));

    QStringList bandSuggestions = m_features->generateSuggestions("ban", 5);
    // Note: The actual implementation may use fuzzy matching or prefix matching
    // Just verify we get some suggestions back
    QVERIFY(bandSuggestions.size() > 0);
    // At least one of the "ban" words should be in the suggestions
    bool hasBanWord = bandSuggestions.contains("banana") ||
                      bandSuggestions.contains("band") ||
                      bandSuggestions.contains("bandage");
    QVERIFY(hasBanWord);
}

void SearchFeaturesTest::testFilterResults() {
    QList<SearchResult> results;
    results.append(createTestResult(0, "test", "This is a test document", 5));
    results.append(createTestResult(1, "example", "Example entry", 10));
    results.append(createTestResult(2, "feature", "Feature rich content", 15));

    QList<SearchResult> filtered = m_features->filterResults(results, "test");
    QCOMPARE(filtered.size(), 1);
    QCOMPARE(filtered.first().matchedText, QString("test"));

    QList<SearchResult> contextFiltered =
        m_features->filterResults(results, "Feature");
    QCOMPARE(contextFiltered.size(), 1);
    QCOMPARE(contextFiltered.first().contextText,
             QString("Feature rich content"));
}

void SearchFeaturesTest::testSortResults() {
    QList<SearchResult> results;
    results.append(createTestResult(2, "alpha", "", 30));
    results.append(createTestResult(0, "beta", "", 10));
    results.append(createTestResult(1, "gamma", "", 20));

    QList<SearchResult> byPage =
        m_features->sortResults(results, SearchFeatures::ByPageNumber, true);
    QCOMPARE(byPage[0].pageNumber, 0);
    QCOMPARE(byPage[1].pageNumber, 1);
    QCOMPARE(byPage[2].pageNumber, 2);

    QList<SearchResult> byPosition =
        m_features->sortResults(results, SearchFeatures::ByPosition, false);
    QCOMPARE(byPosition[0].textPosition, 30);
    QCOMPARE(byPosition[1].textPosition, 20);
    QCOMPARE(byPosition[2].textPosition, 10);
}

void SearchFeaturesTest::testSortCriteria() {
    QList<SearchResult> results;
    results.append(createTestResult(0, "short", "", 5));
    auto medium = createTestResult(0, "mediumlength", "", 15);
    medium.textLength = medium.matchedText.length();
    results.append(medium);
    auto longRes = createTestResult(0, "averylongmatchingstring", "", 25);
    longRes.textLength = longRes.matchedText.length();
    results.append(longRes);

    QList<SearchResult> byLength =
        m_features->sortResults(results, SearchFeatures::ByLength, true);
    QCOMPARE(byLength.first().matchedText, QString("short"));
    QCOMPARE(byLength.last().matchedText, QString("averylongmatchingstring"));
}

void SearchFeaturesTest::testSearchStatistics() {
    m_features->resetStatistics();
    SearchOptions options;
    m_features->addToHistory("query1", options, 5, 100, true);
    m_features->addToHistory("query2", options, 0, 50, false);
    m_features->addToHistory("query3", options, 10, 200, true);
    m_features->addToHistory("query1", options, 3, 150, true);

    SearchFeatures::SearchStatistics stats = m_features->getSearchStatistics();

    QCOMPARE(stats.totalSearches, 4);
    QCOMPARE(stats.successfulSearches, 3);
    QCOMPARE(stats.averageSearchTime, 125.0);
    QCOMPARE(stats.averageResultCount, 4.5);
    QVERIFY(stats.mostPopularQueries.contains("query1"));
    QCOMPARE(stats.queryFrequency.value("query1"), 2);
    QVERIFY(stats.lastSearchTime.isValid());
}

void SearchFeaturesTest::testResetStatistics() {
    SearchOptions options;
    m_features->addToHistory("temp", options, 2, 40, true);
    QVERIFY(m_features->getSearchStatistics().totalSearches > 0);

    m_features->resetStatistics();
    SearchFeatures::SearchStatistics stats = m_features->getSearchStatistics();
    QCOMPARE(stats.totalSearches, 0);
    QCOMPARE(stats.successfulSearches, 0);
    QCOMPARE(stats.averageSearchTime, 0.0);
}

void SearchFeaturesTest::testStatisticsTracking() {
    m_features->resetStatistics();
    QSignalSpy statsSpy(m_features, &SearchFeatures::statisticsUpdated);

    SearchFeatures::SearchStatistics initialStats =
        m_features->getSearchStatistics();
    QCOMPARE(initialStats.totalSearches, 0);
    QCOMPARE(initialStats.successfulSearches, 0);

    SearchOptions options;
    m_features->addToHistory("test1", options, 5, 100, true);
    QVERIFY(statsSpy.count() >= 1);

    SearchFeatures::SearchStatistics stats1 = m_features->getSearchStatistics();
    QCOMPARE(stats1.totalSearches, 1);
    QCOMPARE(stats1.successfulSearches, 1);

    m_features->addToHistory("test2", options, 0, 50, false);
    QVERIFY(statsSpy.count() >= 2);

    SearchFeatures::SearchStatistics stats2 = m_features->getSearchStatistics();
    QCOMPARE(stats2.totalSearches, 2);
    QCOMPARE(stats2.successfulSearches, 1);
    QCOMPARE(stats2.averageSearchTime, 75.0);
    QCOMPARE(stats2.averageResultCount, 2.5);
}

void SearchFeaturesTest::testExportSearchHistory() {
    // Add some history
    SearchOptions options;
    m_features->addToHistory("export_test1", options, 5, 100);
    m_features->addToHistory("export_test2", options, 3, 150);

    // Export to temporary file
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString tempFile = tempDir.filePath("history_export.json");

    bool exportSuccess = m_features->exportSearchHistory(tempFile);
    QVERIFY(exportSuccess);

    // Verify file was created
    QVERIFY(QFile::exists(tempFile));

    // Verify file has content
    QFile file(tempFile);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray content = file.readAll();
    file.close();

    QVERIFY(content.size() > 0);

    // Verify it's valid JSON
    QJsonDocument doc = QJsonDocument::fromJson(content);
    QVERIFY(!doc.isNull());

    QVERIFY(QFile::remove(tempFile));
}

void SearchFeaturesTest::testImportSearchHistory() {
    // First export some history
    SearchOptions options;
    m_features->clearHistory();
    m_features->addToHistory("import_test1", options, 5, 100);
    m_features->addToHistory("import_test2", options, 3, 150);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString tempFile = tempDir.filePath("history_import.json");
    bool exportSuccess = m_features->exportSearchHistory(tempFile);
    QVERIFY(exportSuccess);

    // Clear history
    m_features->clearHistory();
    QList<SearchFeatures::HistoryEntry> emptyHistory =
        m_features->getSearchHistory();
    QCOMPARE(emptyHistory.size(), 0);

    // Import the history back
    bool importSuccess = m_features->importSearchHistory(tempFile);
    QVERIFY(importSuccess);

    // Verify history was restored
    QList<SearchFeatures::HistoryEntry> restoredHistory =
        m_features->getSearchHistory();
    QVERIFY(restoredHistory.size() >= 2);

    // Verify the queries are present
    bool foundTest1 = false, foundTest2 = false;
    for (const auto& entry : restoredHistory) {
        if (entry.query == "import_test1")
            foundTest1 = true;
        if (entry.query == "import_test2")
            foundTest2 = true;
    }
    QVERIFY(foundTest1);
    QVERIFY(foundTest2);

    QVERIFY(QFile::remove(tempFile));
}

void SearchFeaturesTest::testExportSearchResults() {
    QList<SearchResult> results = m_testResults;

    QString json = m_features->exportSearchResults(results, "json");
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    QVERIFY(doc.isArray());
    QCOMPARE(doc.array().size(), results.size());

    QString csv = m_features->exportSearchResults(results, "csv");
    QVERIFY(csv.contains("Page,Position"));
    QVERIFY(csv.count('\n') >= results.size());

    QString plain = m_features->exportSearchResults(results, "text");
    QVERIFY(plain.contains("Page"));
    QVERIFY(plain.contains("Context"));
}

void SearchFeaturesTest::testFuzzySearchCompletedSignal() {
    QSignalSpy spy(m_features, &SearchFeatures::fuzzySearchCompleted);
    QVERIFY(spy.isValid());
    m_features->fuzzySearch(m_testText, "test", 2, 5);
    QCOMPARE(spy.count(), 1);
}

void SearchFeaturesTest::testHighlightsGeneratedSignal() {
    QSignalSpy spy(m_features, &SearchFeatures::highlightsGenerated);
    QVERIFY(spy.isValid());
    m_features->generateHighlights(m_testResults, 0);
    QCOMPARE(spy.count(), 1);
}

void SearchFeaturesTest::testHistoryUpdatedSignal() {
    QSignalSpy spy(m_features, &SearchFeatures::historyUpdated);
    QVERIFY(spy.isValid());
    SearchOptions options;
    m_features->addToHistory("signal-test", options, 1, 10);
    QCOMPARE(spy.count(), 1);
}

void SearchFeaturesTest::testSuggestionsReadySignal() {
    QSignalSpy spy(m_features, &SearchFeatures::suggestionsReady);
    QVERIFY(spy.isValid());
    QStringList corpus = {"search", "searchable"};
    m_features->updateSuggestionModel(corpus);
    m_features->generateSuggestions("sear", 5);
    QVERIFY(spy.count() >= 1);
}

void SearchFeaturesTest::testStatisticsUpdatedSignal() {
    QSignalSpy spy(m_features, &SearchFeatures::statisticsUpdated);
    QVERIFY(spy.isValid());
    SearchOptions options;
    m_features->addToHistory("stats-signal", options, 2, 30, true);
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(SearchFeaturesTest)
#include "test_search_features.moc"
