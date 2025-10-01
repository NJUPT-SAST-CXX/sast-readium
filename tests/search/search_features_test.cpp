#include <QtTest/QtTest>
#include <QObject>
#include <QSignalSpy>
#include <QColor>
#include <QDateTime>
#include <QStringList>
#include "../../app/search/SearchFeatures.h"
#include "../../app/search/SearchConfiguration.h"
#include "../TestUtilities.h"

/**
 * Comprehensive tests for SearchFeatures class
 * Tests fuzzy search, highlighting, history, and advanced functionality
 */
class SearchFeaturesTest : public TestBase
{
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
    SearchResult createTestResult(int page, const QString& text, const QString& context, int position = 0);
    void verifyFuzzyMatch(const SearchFeatures::FuzzyMatch& match, const QString& expectedText);
    void verifyHighlightInfo(const SearchFeatures::HighlightInfo& highlight, const QColor& expectedColor);
    void verifyHistoryEntry(const SearchFeatures::HistoryEntry& entry, const QString& expectedQuery);
};

void SearchFeaturesTest::initTestCase()
{
    setupTestData();
}

void SearchFeaturesTest::cleanupTestCase()
{
    // Cleanup any resources
}

void SearchFeaturesTest::init()
{
    m_features = new SearchFeatures(this);
    QVERIFY(m_features != nullptr);
}

void SearchFeaturesTest::cleanup()
{
    if (m_features) {
        m_features->clearHistory();
        m_features->resetStatistics();
        delete m_features;
        m_features = nullptr;
    }
}

void SearchFeaturesTest::testFuzzySearch()
{
    QSignalSpy completedSpy(m_features, &SearchFeatures::fuzzySearchCompleted);
    
    QList<SearchFeatures::FuzzyMatch> matches = m_features->fuzzySearch(m_testText, "tset", 2);
    
    QVERIFY(!matches.isEmpty());
    verifyFuzzyMatch(matches[0], "test");
    
    // Verify signal was emitted
    QCOMPARE(completedSpy.count(), 1);
}

void SearchFeaturesTest::testLevenshteinDistance()
{
    int distance1 = m_features->calculateLevenshteinDistance("test", "tset");
    QCOMPARE(distance1, 2); // Two character swaps
    
    int distance2 = m_features->calculateLevenshteinDistance("test", "test");
    QCOMPARE(distance2, 0); // Identical strings
    
    int distance3 = m_features->calculateLevenshteinDistance("test", "testing");
    QCOMPARE(distance3, 3); // Three insertions
    
    int distance4 = m_features->calculateLevenshteinDistance("", "test");
    QCOMPARE(distance4, 4); // Four insertions
}

void SearchFeaturesTest::testSimilarityCalculation()
{
    double similarity1 = m_features->calculateSimilarity("test", "test");
    QCOMPARE(similarity1, 1.0); // Identical strings
    
    double similarity2 = m_features->calculateSimilarity("test", "tset");
    QVERIFY(similarity2 > 0.0 && similarity2 < 1.0); // Similar but not identical
    
    double similarity3 = m_features->calculateSimilarity("test", "xyz");
    QVERIFY(similarity3 < 0.5); // Very different strings
    
    double similarity4 = m_features->calculateSimilarity("", "");
    QCOMPARE(similarity4, 1.0); // Empty strings are identical
}

void SearchFeaturesTest::testFuzzySearchWithDistance()
{
    // Test with different max distances
    QList<SearchFeatures::FuzzyMatch> matches1 = m_features->fuzzySearch(m_testText, "tset", 1);
    QList<SearchFeatures::FuzzyMatch> matches2 = m_features->fuzzySearch(m_testText, "tset", 2);
    QList<SearchFeatures::FuzzyMatch> matches3 = m_features->fuzzySearch(m_testText, "tset", 3);
    
    // More lenient distance should find more or equal matches
    QVERIFY(matches2.size() >= matches1.size());
    QVERIFY(matches3.size() >= matches2.size());
    
    // Test with max results limit
    QList<SearchFeatures::FuzzyMatch> limitedMatches = m_features->fuzzySearch(m_testText, "test", 2, 1);
    QVERIFY(limitedMatches.size() <= 1);
}

void SearchFeaturesTest::testWildcardSearch()
{
    QList<SearchResult> results = m_features->wildcardSearch(m_testText, "te*t", 0);
    
    QVERIFY(!results.isEmpty());
    for (const SearchResult& result : results) {
        QVERIFY(result.matchedText.startsWith("te"));
        QVERIFY(result.matchedText.endsWith("t"));
    }
}

void SearchFeaturesTest::testPhraseSearch()
{
    QList<SearchResult> results = m_features->phraseSearch(m_testText, "test document", 0, 0);
    
    QVERIFY(!results.isEmpty());
    for (const SearchResult& result : results) {
        QVERIFY(result.contextText.contains("test document", Qt::CaseInsensitive));
    }
    
    // Test with proximity
    results = m_features->phraseSearch(m_testText, "test document", 0, 5);
    QVERIFY(!results.isEmpty());
}

void SearchFeaturesTest::testBooleanSearch()
{
    QList<SearchResult> results = m_features->booleanSearch(m_testText, "test AND document", 0);
    
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

void SearchFeaturesTest::testProximitySearch()
{
    QStringList terms = {"test", "document"};
    SearchFeatures::ProximitySearchOptions options;
    options.maxDistance = 10;
    options.ordered = false;
    
    QList<SearchResult> results = m_features->proximitySearch(m_testText, terms, options, 0);
    
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

void SearchFeaturesTest::testHighlightColors()
{
    QColor normalColor(255, 255, 0); // Yellow
    QColor currentColor(255, 0, 0);  // Red
    
    m_features->setHighlightColors(normalColor, currentColor);
    
    QCOMPARE(m_features->getNormalHighlightColor(), normalColor);
    QCOMPARE(m_features->getCurrentHighlightColor(), currentColor);
}

void SearchFeaturesTest::testGenerateHighlights()
{
    QSignalSpy highlightsSpy(m_features, &SearchFeatures::highlightsGenerated);
    
    QList<SearchFeatures::HighlightInfo> highlights = m_features->generateHighlights(m_testResults, 0);
    
    QVERIFY(!highlights.isEmpty());
    QCOMPARE(highlights.size(), m_testResults.size());
    
    // Verify current result highlighting
    if (!highlights.isEmpty()) {
        QVERIFY(highlights[0].isCurrentResult);
        verifyHighlightInfo(highlights[0], m_features->getCurrentHighlightColor());
    }
    
    // Verify signal was emitted
    QCOMPARE(highlightsSpy.count(), 1);
}

void SearchFeaturesTest::testUpdateHighlightPriorities()
{
    QList<SearchFeatures::HighlightInfo> highlights = m_features->generateHighlights(m_testResults);
    
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

void SearchFeaturesTest::testAddToHistory()
{
    QSignalSpy historySpy(m_features, &SearchFeatures::historyUpdated);
    
    SearchOptions options;
    m_features->addToHistory("test query", options, 5, 100, true);
    
    QList<SearchFeatures::HistoryEntry> history = m_features->getSearchHistory(10);
    QCOMPARE(history.size(), 1);
    
    verifyHistoryEntry(history[0], "test query");
    QCOMPARE(history[0].resultCount, 5);
    QCOMPARE(history[0].searchTime, 100);
    QCOMPARE(history[0].successful, true);
    
    // Verify signal was emitted
    QCOMPARE(historySpy.count(), 1);
}

void SearchFeaturesTest::testGetSearchHistory()
{
    // Add multiple entries
    SearchOptions options;
    m_features->addToHistory("query1", options, 1, 50);
    m_features->addToHistory("query2", options, 2, 75);
    m_features->addToHistory("query3", options, 3, 100);
    
    QList<SearchFeatures::HistoryEntry> history = m_features->getSearchHistory(2);
    QCOMPARE(history.size(), 2);
    
    // Should return most recent entries first
    verifyHistoryEntry(history[0], "query3");
    verifyHistoryEntry(history[1], "query2");
    
    // Test getting all entries
    history = m_features->getSearchHistory(50);
    QCOMPARE(history.size(), 3);
}

void SearchFeaturesTest::testGetRecentQueries()
{
    SearchOptions options;
    m_features->addToHistory("recent1", options, 1, 50);
    m_features->addToHistory("recent2", options, 2, 75);
    
    QStringList recent = m_features->getRecentQueries(5);
    QVERIFY(recent.contains("recent1"));
    QVERIFY(recent.contains("recent2"));
}

void SearchFeaturesTest::testGetPopularQueries()
{
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

void SearchFeaturesTest::setupTestData()
{
    m_testText = "This is a test document with multiple test words. "
                 "The document contains various test cases for testing "
                 "the search functionality and features.";
    
    m_testResults = {
        createTestResult(0, "test", "This is a test document", 10),
        createTestResult(0, "test", "multiple test words", 35),
        createTestResult(0, "test", "various test cases", 60)
    };
}

SearchResult SearchFeaturesTest::createTestResult(int page, const QString& text, const QString& context, int position)
{
    return SearchResult(page, text, context, QRectF(position * 10, 100, 50, 20), position, text.length());
}

void SearchFeaturesTest::verifyFuzzyMatch(const SearchFeatures::FuzzyMatch& match, const QString& expectedText)
{
    QVERIFY(match.text.contains(expectedText, Qt::CaseInsensitive));
    QVERIFY(match.position >= 0);
    QVERIFY(match.length > 0);
    QVERIFY(match.editDistance >= 0);
    QVERIFY(match.similarity >= 0.0 && match.similarity <= 1.0);
}

void SearchFeaturesTest::verifyHighlightInfo(const SearchFeatures::HighlightInfo& highlight, const QColor& expectedColor)
{
    QVERIFY(!highlight.rect.isEmpty());
    QCOMPARE(highlight.color, expectedColor);
    QVERIFY(!highlight.text.isEmpty());
    QVERIFY(highlight.priority >= 0);
}

void SearchFeaturesTest::verifyHistoryEntry(const SearchFeatures::HistoryEntry& entry, const QString& expectedQuery)
{
    QCOMPARE(entry.query, expectedQuery);
    QVERIFY(entry.timestamp.isValid());
    QVERIFY(entry.resultCount >= 0);
    QVERIFY(entry.searchTime >= 0);
}

QTEST_MAIN(SearchFeaturesTest)
#include "search_features_test.moc"
