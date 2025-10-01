#include <QtTest/QtTest>
#include <QObject>
#include <QSignalSpy>
#include <QRegularExpression>
#include <QRectF>
#include "../../app/search/SearchExecutor.h"
#include "../../app/search/SearchConfiguration.h"
#include "../../app/search/TextExtractor.h"
#include "../TestUtilities.h"

/**
 * Comprehensive tests for SearchExecutor class
 * Tests search execution logic and pattern matching
 */
class SearchExecutorTest : public TestBase
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Configuration tests
    void testSetTextExtractor();
    void testSetOptions();

    // Search operation tests
    void testSearchInPage();
    void testSearchInPages();
    void testSearchInText();
    void testSearchWithDifferentOptions();

    // Pattern management tests
    void testValidateQuery();
    void testCreateSearchPattern();
    void testCreateSearchPatternWithOptions();
    void testRegexPatterns();
    void testCaseSensitivePatterns();
    void testWholeWordPatterns();

    // Bounding rect calculation tests
    void testCalculateBoundingRect();
    void testBoundingRectAccuracy();

    // Signal tests
    void testSearchProgressSignal();
    void testResultFoundSignal();
    void testSearchErrorSignal();

    // Edge cases and error handling
    void testEmptyQuery();
    void testInvalidQuery();
    void testEmptyText();
    void testLargeText();
    void testSpecialCharacters();
    void testUnicodeText();

    // Performance tests
    void testSearchPerformance();
    void testLargeDocumentSearch();

private:
    SearchExecutor* m_executor;
    TextExtractor* m_textExtractor;
    SearchOptions m_defaultOptions;
    
    // Test data
    QString m_testText;
    QStringList m_testTexts;
    
    // Helper methods
    void setupTestData();
    SearchOptions createTestOptions(bool caseSensitive = false, bool wholeWords = false, bool useRegex = false);
    void verifySearchResult(const SearchResult& result, const QString& expectedText, int expectedPage = 0);
    void verifySearchResults(const QList<SearchResult>& results, int expectedCount, const QString& query);
};

void SearchExecutorTest::initTestCase()
{
    setupTestData();
}

void SearchExecutorTest::cleanupTestCase()
{
    // Cleanup any resources
}

void SearchExecutorTest::init()
{
    m_executor = new SearchExecutor(this);
    m_textExtractor = new TextExtractor(this);
    m_defaultOptions = createTestOptions();
    
    QVERIFY(m_executor != nullptr);
    QVERIFY(m_textExtractor != nullptr);
    
    m_executor->setTextExtractor(m_textExtractor);
    m_executor->setOptions(m_defaultOptions);
}

void SearchExecutorTest::cleanup()
{
    delete m_executor;
    delete m_textExtractor;
    m_executor = nullptr;
    m_textExtractor = nullptr;
}

void SearchExecutorTest::testSetTextExtractor()
{
    TextExtractor* extractor = new TextExtractor(this);
    m_executor->setTextExtractor(extractor);
    
    // Test that the extractor was set (we can't directly verify this without access to internals)
    // But we can test that search operations work
    QList<SearchResult> results = m_executor->searchInText(m_testText, "test");
    QVERIFY(!results.isEmpty());
    
    delete extractor;
}

void SearchExecutorTest::testSetOptions()
{
    SearchOptions options = createTestOptions(true, true, false);
    m_executor->setOptions(options);
    
    // Test that options were applied by performing a search
    QList<SearchResult> results = m_executor->searchInText("Test word", "test");
    // With case sensitive, "test" should not match "Test"
    QVERIFY(results.isEmpty());
    
    results = m_executor->searchInText("Test word", "Test");
    // With case sensitive, "Test" should match "Test"
    QVERIFY(!results.isEmpty());
}

void SearchExecutorTest::testSearchInPage()
{
    QSignalSpy progressSpy(m_executor, &SearchExecutor::searchProgress);
    QSignalSpy resultSpy(m_executor, &SearchExecutor::resultFound);
    
    QList<SearchResult> results = m_executor->searchInPage(0, "test");
    
    QVERIFY(!results.isEmpty());
    verifySearchResults(results, -1, "test"); // -1 means any count >= 1
    
    // Verify signals were emitted
    QVERIFY(progressSpy.count() >= 0); // May or may not emit progress for single page
    QVERIFY(resultSpy.count() >= 1);
}

void SearchExecutorTest::testSearchInPages()
{
    QList<int> pageNumbers = {0, 1, 2};
    QSignalSpy progressSpy(m_executor, &SearchExecutor::searchProgress);
    
    QList<SearchResult> results = m_executor->searchInPages(pageNumbers, "test");
    
    QVERIFY(!results.isEmpty());
    verifySearchResults(results, -1, "test");
    
    // Should emit progress for multiple pages
    QVERIFY(progressSpy.count() > 0);
}

void SearchExecutorTest::testSearchInText()
{
    QList<SearchResult> results = m_executor->searchInText(m_testText, "test");
    
    QVERIFY(!results.isEmpty());
    verifySearchResults(results, -1, "test");
    
    // Test with specific page number
    results = m_executor->searchInText(m_testText, "test", 5);
    for (const SearchResult& result : results) {
        QCOMPARE(result.pageNumber, 5);
    }
}

void SearchExecutorTest::testSearchWithDifferentOptions()
{
    // Test case sensitive search
    SearchOptions caseSensitiveOptions = createTestOptions(true, false, false);
    m_executor->setOptions(caseSensitiveOptions);
    
    QList<SearchResult> results1 = m_executor->searchInText("Test test TEST", "test");
    QList<SearchResult> results2 = m_executor->searchInText("Test test TEST", "Test");
    QList<SearchResult> results3 = m_executor->searchInText("Test test TEST", "TEST");
    
    QCOMPARE(results1.size(), 1); // Only "test"
    QCOMPARE(results2.size(), 1); // Only "Test"
    QCOMPARE(results3.size(), 1); // Only "TEST"
    
    // Test whole word search
    SearchOptions wholeWordOptions = createTestOptions(false, true, false);
    m_executor->setOptions(wholeWordOptions);
    
    results1 = m_executor->searchInText("test testing tested", "test");
    QCOMPARE(results1.size(), 1); // Only "test", not "testing" or "tested"
}

void SearchExecutorTest::testValidateQuery()
{
    // Test valid queries
    QVERIFY(m_executor->validateQuery("test"));
    QVERIFY(m_executor->validateQuery("multiple words"));
    QVERIFY(m_executor->validateQuery("123"));
    QVERIFY(m_executor->validateQuery("special!@#$%"));
    
    // Test invalid queries
    QVERIFY(!m_executor->validateQuery(""));
    QVERIFY(!m_executor->validateQuery("   ")); // Only whitespace
}

void SearchExecutorTest::testCreateSearchPattern()
{
    QRegularExpression pattern = m_executor->createSearchPattern("test");
    QVERIFY(pattern.isValid());
    QVERIFY(pattern.match("test").hasMatch());
    QVERIFY(pattern.match("Test").hasMatch()); // Default is case insensitive
    
    // Test with special regex characters
    pattern = m_executor->createSearchPattern("test.*");
    QVERIFY(pattern.isValid());
    // Should escape special characters by default
    QVERIFY(!pattern.match("testing").hasMatch());
}

void SearchExecutorTest::testCreateSearchPatternWithOptions()
{
    // Test case sensitive pattern
    SearchOptions options = createTestOptions(true, false, false);
    QRegularExpression pattern = m_executor->createSearchPattern("test", options);
    
    QVERIFY(pattern.isValid());
    QVERIFY(pattern.match("test").hasMatch());
    QVERIFY(!pattern.match("Test").hasMatch());
    
    // Test whole word pattern
    options = createTestOptions(false, true, false);
    pattern = m_executor->createSearchPattern("test", options);
    
    QVERIFY(pattern.isValid());
    QVERIFY(pattern.match("test word").hasMatch());
    QVERIFY(!pattern.match("testing").hasMatch());
    
    // Test regex pattern
    options = createTestOptions(false, false, true);
    pattern = m_executor->createSearchPattern("test.*", options);
    
    QVERIFY(pattern.isValid());
    QVERIFY(pattern.match("testing").hasMatch());
    QVERIFY(pattern.match("test123").hasMatch());
}

void SearchExecutorTest::testRegexPatterns()
{
    SearchOptions regexOptions = createTestOptions(false, false, true);
    m_executor->setOptions(regexOptions);
    
    // Test basic regex
    QList<SearchResult> results = m_executor->searchInText("test123 test456", "test\\d+");
    QCOMPARE(results.size(), 2);
    
    // Test character classes
    results = m_executor->searchInText("test Test TEST", "[Tt]est");
    QCOMPARE(results.size(), 2); // "test" and "Test"
    
    // Test quantifiers
    results = m_executor->searchInText("test te test", "te+st");
    QCOMPARE(results.size(), 2); // Both "test" instances
}

void SearchExecutorTest::testCaseSensitivePatterns()
{
    SearchOptions options = createTestOptions(true, false, false);
    m_executor->setOptions(options);
    
    QList<SearchResult> results = m_executor->searchInText("Test test TEST", "test");
    QCOMPARE(results.size(), 1);
    verifySearchResult(results[0], "test");
    
    results = m_executor->searchInText("Test test TEST", "Test");
    QCOMPARE(results.size(), 1);
    verifySearchResult(results[0], "Test");
}

void SearchExecutorTest::testWholeWordPatterns()
{
    SearchOptions options = createTestOptions(false, true, false);
    m_executor->setOptions(options);
    
    QList<SearchResult> results = m_executor->searchInText("test testing tested", "test");
    QCOMPARE(results.size(), 1);
    verifySearchResult(results[0], "test");
    
    results = m_executor->searchInText("word1 word word2", "word");
    QCOMPARE(results.size(), 1);
    verifySearchResult(results[0], "word");
}

void SearchExecutorTest::setupTestData()
{
    m_testText = "This is a test document with multiple test words. "
                 "It contains various test cases for testing the search functionality. "
                 "Test, TEST, and test should all be found in case-insensitive mode.";
    
    m_testTexts = {
        "First page with test content",
        "Second page also has test data",
        "Third page contains more test information"
    };
}

SearchOptions SearchExecutorTest::createTestOptions(bool caseSensitive, bool wholeWords, bool useRegex)
{
    SearchOptions options;
    options.caseSensitive = caseSensitive;
    options.wholeWords = wholeWords;
    options.useRegex = useRegex;
    options.maxResults = 1000;
    options.contextLength = 50;
    return options;
}

void SearchExecutorTest::verifySearchResult(const SearchResult& result, const QString& expectedText, int expectedPage)
{
    QVERIFY(result.isValid());
    QCOMPARE(result.pageNumber, expectedPage);
    QVERIFY(result.matchedText.contains(expectedText, Qt::CaseInsensitive));
    QVERIFY(!result.contextText.isEmpty());
    QVERIFY(result.textPosition >= 0);
    QVERIFY(result.textLength > 0);
}

void SearchExecutorTest::verifySearchResults(const QList<SearchResult>& results, int expectedCount, const QString& query)
{
    if (expectedCount >= 0) {
        QCOMPARE(results.size(), expectedCount);
    } else {
        QVERIFY(results.size() > 0);
    }
    
    for (const SearchResult& result : results) {
        QVERIFY(result.isValid());
        QVERIFY(result.matchedText.contains(query, Qt::CaseInsensitive));
    }
}

QTEST_MAIN(SearchExecutorTest)
#include "search_executor_test.moc"
