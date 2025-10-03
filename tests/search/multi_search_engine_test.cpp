#include <QObject>
#include <QRandomGenerator>
#include <QSignalSpy>
#include <QStringList>
#include <QtTest/QtTest>
#include "../../app/search/SearchFeatures.h"
#include "../TestUtilities.h"

/**
 * Comprehensive tests for SearchSuggestionEngine class (from
 * SearchFeaturesExtensions.cpp) Tests Trie data structure, search suggestions,
 * and model training
 */
class MultiSearchEngineTest : public TestBase {
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

    // Model training tests
    void testTrainModel();
    void testTrainModelWithMismatchedSizes();
    void testTrainModelWithEmptyData();
    void testTrainModelWithDuplicates();

    // Suggestion generation tests
    void testGenerateSuggestions();
    void testGenerateSuggestionsWithPrefix();
    void testGenerateSuggestionsWithLimit();
    void testGenerateSuggestionsEmptyPrefix();
    void testGenerateSuggestionsNoMatches();

    // Query frequency tests
    void testUpdateQueryFrequency();
    void testGetQueryFrequency();
    void testGetMostFrequentQueries();

    // Trie structure tests
    void testTrieInsertion();
    void testTrieTraversal();
    void testTrieFrequencyOrdering();

    // Advanced suggestion tests
    void testSuggestionRanking();
    void testPartialWordMatching();
    void testCaseInsensitiveSuggestions();

    // Performance tests
    void testLargeDatasetPerformance();
    void testSuggestionGenerationSpeed();
    void testMemoryUsageOptimization();

    // Edge case tests
    void testSpecialCharacterHandling();
    void testUnicodeSupport();
    void testVeryLongQueries();
    void testEmptyStringHandling();

    // Integration tests
    void testRealWorldQueryPatterns();
    void testIncrementalTraining();
    void testModelPersistence();

private:
    SearchSuggestionEngine* m_suggestionEngine;
    QStringList m_testQueries;
    QList<int> m_testFrequencies;

    // Helper methods
    void setupTestData();
    QStringList generateRandomQueries(int count, int averageLength);
    QList<int> generateRandomFrequencies(int count, int maxFrequency);
    void verifyTrieStructure();
    void verifySuggestionQuality(const QStringList& suggestions,
                                 const QString& prefix);
    void benchmarkSuggestionGeneration(int queryCount, int suggestionLimit);
};

void MultiSearchEngineTest::initTestCase() {
    qDebug() << "Starting MultiSearchEngine (SearchSuggestionEngine) tests";
    setupTestData();
}

void MultiSearchEngineTest::cleanupTestCase() {
    qDebug() << "MultiSearchEngine tests completed";
}

void MultiSearchEngineTest::init() {
    m_suggestionEngine = new SearchSuggestionEngine();
}

void MultiSearchEngineTest::cleanup() {
    if (m_suggestionEngine) {
        delete m_suggestionEngine;
        m_suggestionEngine = nullptr;
    }
}

void MultiSearchEngineTest::setupTestData() {
    m_testQueries = {"search",      "search engine",   "search algorithm",
                     "text search", "advanced search", "quick search",
                     "file search", "content search",  "semantic search",
                     "fuzzy search"};

    m_testFrequencies = {10, 8, 6, 12, 4, 15, 7, 9, 3, 5};
}

void MultiSearchEngineTest::testConstructor() {
    QVERIFY(m_suggestionEngine != nullptr);

    // Test that initial state is empty
    QStringList suggestions =
        m_suggestionEngine->generateSuggestions("test", 5);
    QVERIFY(suggestions.isEmpty());
}

void MultiSearchEngineTest::testDestructor() {
    SearchSuggestionEngine* engine = new SearchSuggestionEngine();

    // Train with some data
    engine->trainModel(m_testQueries, m_testFrequencies);

    // Destructor should clean up properly
    delete engine;

    // If we reach here without crashing, destructor works correctly
    QVERIFY(true);
}

void MultiSearchEngineTest::testTrainModel() {
    m_suggestionEngine->trainModel(m_testQueries, m_testFrequencies);

    // After training, should be able to generate suggestions
    QStringList suggestions =
        m_suggestionEngine->generateSuggestions("search", 5);
    QVERIFY(!suggestions.isEmpty());

    // Verify suggestions contain expected queries
    bool foundSearchEngine = false;
    bool foundSearchAlgorithm = false;

    for (const QString& suggestion : suggestions) {
        if (suggestion == "search engine")
            foundSearchEngine = true;
        if (suggestion == "search algorithm")
            foundSearchAlgorithm = true;
    }

    QVERIFY(foundSearchEngine || foundSearchAlgorithm);
}

void MultiSearchEngineTest::testTrainModelWithMismatchedSizes() {
    QStringList queries = {"query1", "query2"};
    QList<int> frequencies = {5};  // Mismatched size

    // Should handle mismatched sizes gracefully
    m_suggestionEngine->trainModel(queries, frequencies);

    // Should not crash
    QVERIFY(true);
}

void MultiSearchEngineTest::testTrainModelWithEmptyData() {
    QStringList emptyQueries;
    QList<int> emptyFrequencies;

    m_suggestionEngine->trainModel(emptyQueries, emptyFrequencies);

    // Should handle empty data gracefully
    QStringList suggestions =
        m_suggestionEngine->generateSuggestions("test", 5);
    QVERIFY(suggestions.isEmpty());
}

void MultiSearchEngineTest::testTrainModelWithDuplicates() {
    QStringList queries = {"search", "search", "test", "search"};
    QList<int> frequencies = {5, 3, 2, 4};

    m_suggestionEngine->trainModel(queries, frequencies);

    // Should handle duplicates by combining frequencies
    QStringList suggestions = m_suggestionEngine->generateSuggestions("s", 5);
    QVERIFY(!suggestions.isEmpty());
}

void MultiSearchEngineTest::testGenerateSuggestions() {
    m_suggestionEngine->trainModel(m_testQueries, m_testFrequencies);

    QStringList suggestions =
        m_suggestionEngine->generateSuggestions("search", 5);

    QVERIFY(!suggestions.isEmpty());
    QVERIFY(suggestions.size() <= 5);

    // All suggestions should start with "search"
    for (const QString& suggestion : suggestions) {
        QVERIFY(suggestion.startsWith("search"));
    }
}

void MultiSearchEngineTest::testGenerateSuggestionsWithPrefix() {
    m_suggestionEngine->trainModel(m_testQueries, m_testFrequencies);

    // Test different prefixes
    QStringList searchSuggestions =
        m_suggestionEngine->generateSuggestions("search", 3);
    QStringList textSuggestions =
        m_suggestionEngine->generateSuggestions("text", 3);
    QStringList fuzzySuggestions =
        m_suggestionEngine->generateSuggestions("fuzzy", 3);

    QVERIFY(!searchSuggestions.isEmpty());
    QVERIFY(!textSuggestions.isEmpty());
    QVERIFY(!fuzzySuggestions.isEmpty());

    // Verify prefix matching
    for (const QString& suggestion : searchSuggestions) {
        QVERIFY(suggestion.startsWith("search"));
    }

    for (const QString& suggestion : textSuggestions) {
        QVERIFY(suggestion.startsWith("text"));
    }
}

void MultiSearchEngineTest::testGenerateSuggestionsWithLimit() {
    m_suggestionEngine->trainModel(m_testQueries, m_testFrequencies);

    // Test different limits
    QStringList suggestions1 =
        m_suggestionEngine->generateSuggestions("search", 1);
    QStringList suggestions3 =
        m_suggestionEngine->generateSuggestions("search", 3);
    QStringList suggestions10 =
        m_suggestionEngine->generateSuggestions("search", 10);

    QVERIFY(suggestions1.size() <= 1);
    QVERIFY(suggestions3.size() <= 3);
    QVERIFY(suggestions10.size() <= 10);

    // More suggestions should include all from fewer suggestions
    for (const QString& suggestion : suggestions1) {
        QVERIFY(suggestions3.contains(suggestion));
    }
}

void MultiSearchEngineTest::testGenerateSuggestionsEmptyPrefix() {
    m_suggestionEngine->trainModel(m_testQueries, m_testFrequencies);

    QStringList suggestions = m_suggestionEngine->generateSuggestions("", 5);

    // Empty prefix should return most frequent queries
    QVERIFY(!suggestions.isEmpty());
    QVERIFY(suggestions.size() <= 5);
}

void MultiSearchEngineTest::testGenerateSuggestionsNoMatches() {
    m_suggestionEngine->trainModel(m_testQueries, m_testFrequencies);

    QStringList suggestions = m_suggestionEngine->generateSuggestions("xyz", 5);

    // No matches should return empty list
    QVERIFY(suggestions.isEmpty());
}

void MultiSearchEngineTest::testUpdateQueryFrequency() {
    QString query = "test query";
    int initialFrequency = 5;

    QStringList queries = {query};
    QList<int> frequencies = {initialFrequency};

    m_suggestionEngine->trainModel(queries, frequencies);

    int retrievedFrequency = m_suggestionEngine->getQueryFrequency(query);
    QCOMPARE(retrievedFrequency, initialFrequency);

    // Update frequency
    m_suggestionEngine->updateQueryFrequency(query, 10);
    int updatedFrequency = m_suggestionEngine->getQueryFrequency(query);
    QCOMPARE(updatedFrequency, 10);
}

void MultiSearchEngineTest::testGetQueryFrequency() {
    m_suggestionEngine->trainModel(m_testQueries, m_testFrequencies);

    // Test getting frequency for existing queries
    for (int i = 0; i < m_testQueries.size(); ++i) {
        int frequency = m_suggestionEngine->getQueryFrequency(m_testQueries[i]);
        QCOMPARE(frequency, m_testFrequencies[i]);
    }

    // Test getting frequency for non-existing query
    int nonExistentFrequency =
        m_suggestionEngine->getQueryFrequency("non-existent query");
    QCOMPARE(nonExistentFrequency, 0);
}

void MultiSearchEngineTest::testGetMostFrequentQueries() {
    m_suggestionEngine->trainModel(m_testQueries, m_testFrequencies);

    QStringList mostFrequent = m_suggestionEngine->getMostFrequentQueries(3);

    QVERIFY(!mostFrequent.isEmpty());
    QVERIFY(mostFrequent.size() <= 3);

    // Verify ordering by frequency (highest first)
    if (mostFrequent.size() >= 2) {
        int freq1 = m_suggestionEngine->getQueryFrequency(mostFrequent[0]);
        int freq2 = m_suggestionEngine->getQueryFrequency(mostFrequent[1]);
        QVERIFY(freq1 >= freq2);
    }
}

void MultiSearchEngineTest::testTrieInsertion() {
    QString testQuery = "test insertion";
    int testFrequency = 7;

    QStringList queries = {testQuery};
    QList<int> frequencies = {testFrequency};

    m_suggestionEngine->trainModel(queries, frequencies);

    // Verify insertion by checking suggestions
    QStringList suggestions =
        m_suggestionEngine->generateSuggestions("test", 5);
    QVERIFY(suggestions.contains(testQuery));
}

void MultiSearchEngineTest::testSuggestionRanking() {
    // Create queries with different frequencies
    QStringList queries = {"search high", "search medium", "search low"};
    QList<int> frequencies = {100, 50, 10};

    m_suggestionEngine->trainModel(queries, frequencies);

    QStringList suggestions =
        m_suggestionEngine->generateSuggestions("search", 3);

    QVERIFY(!suggestions.isEmpty());

    // Higher frequency queries should appear first
    if (suggestions.size() >= 2) {
        QString first = suggestions[0];
        QString second = suggestions[1];

        int freq1 = m_suggestionEngine->getQueryFrequency(first);
        int freq2 = m_suggestionEngine->getQueryFrequency(second);

        QVERIFY(freq1 >= freq2);
    }
}

void MultiSearchEngineTest::testLargeDatasetPerformance() {
    QStringList largeQuerySet = generateRandomQueries(1000, 20);
    QList<int> largeFrequencySet = generateRandomFrequencies(1000, 100);

    QElapsedTimer timer;
    timer.start();

    m_suggestionEngine->trainModel(largeQuerySet, largeFrequencySet);

    qint64 trainingTime = timer.elapsed();
    qDebug() << "Training time for 1000 queries:" << trainingTime << "ms";

    // Training should complete in reasonable time
    QVERIFY(trainingTime < 5000);  // Less than 5 seconds

    // Test suggestion generation performance
    timer.restart();

    for (int i = 0; i < 100; ++i) {
        m_suggestionEngine->generateSuggestions("test", 5);
    }

    qint64 suggestionTime = timer.elapsed();
    qDebug() << "100 suggestion generations:" << suggestionTime << "ms";

    // Suggestion generation should be fast
    QVERIFY(suggestionTime < 1000);  // Less than 1 second for 100 generations
}

QStringList MultiSearchEngineTest::generateRandomQueries(int count,
                                                         int averageLength) {
    QStringList queries;
    const QStringList words = {"search",   "find",   "query",   "text",
                               "document", "file",   "content", "algorithm",
                               "engine",   "system", "data",    "information"};

    for (int i = 0; i < count; ++i) {
        int wordCount = QRandomGenerator::global()->bounded(1, 4);
        QStringList queryWords;

        for (int j = 0; j < wordCount; ++j) {
            queryWords.append(
                words[QRandomGenerator::global()->bounded(words.size())]);
        }

        queries.append(queryWords.join(" "));
    }

    return queries;
}

QList<int> MultiSearchEngineTest::generateRandomFrequencies(int count,
                                                            int maxFrequency) {
    QList<int> frequencies;

    for (int i = 0; i < count; ++i) {
        frequencies.append(
            QRandomGenerator::global()->bounded(1, maxFrequency + 1));
    }

    return frequencies;
}

void MultiSearchEngineTest::verifySuggestionQuality(
    const QStringList& suggestions, const QString& prefix) {
    for (const QString& suggestion : suggestions) {
        QVERIFY(suggestion.startsWith(prefix, Qt::CaseInsensitive));
        QVERIFY(!suggestion.isEmpty());
        QVERIFY(suggestion.length() >= prefix.length());
    }
}

QTEST_MAIN(MultiSearchEngineTest)
#include "multi_search_engine_test.moc"
