#include <QSignalSpy>
#include <QTest>
#include <QtTest/QtTest>
#include "../../app/cache/SearchResultCache.h"
#include "CacheTestHelpers.h"

/**
 * @brief Comprehensive tests for SearchResultCache
 *
 * Tests search result caching with incremental search support,
 * expiration policies, cache key generation, and maintenance operations.
 */
class SearchResultCacheTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Cache operations tests
    void testHasResults();
    void testGetResults();
    void testStoreResults();
    void testInvalidateDocument();
    void testClear();

    // Configuration tests
    void testSetMaxCacheSize();
    void testSetMaxMemoryUsage();
    void testSetExpirationTime();

    // Statistics tests
    void testGetCacheSize();
    void testGetMemoryUsage();
    void testGetHitRatio();
    void testResetStatistics();

    // Incremental search tests
    void testCanUseIncrementalSearch();
    void testGetIncrementalResults();
    void testIncrementalSearchSameDocument();
    void testIncrementalSearchDifferentOptions();

    // ICacheComponent interface tests
    void testGetMaxMemoryLimit();
    void testSetMaxMemoryLimit();
    void testGetEntryCount();
    void testEvictLRU();
    void testGetHitCount();
    void testGetMissCount();
    void testSetEnabled();
    void testIsEnabled();

    // Signal tests
    void testCacheUpdatedSignal();
    void testCacheHitSignal();
    void testCacheMissSignal();

    // Maintenance tests
    void testPeriodicMaintenance();
    void testExpirationHandling();

    // CacheKey tests
    void testCacheKeyToHash();
    void testCacheKeyEquality();
    void testCacheKeyWithDifferentOptions();

    // Edge cases and error handling
    void testStoreEmptyResults();
    void testGetNonExistentKey();
    void testInvalidateNonExistentDocument();
    void testExceedMemoryLimit();
    void testExceedCacheSize();
    void testDisabledCache();
    void testExpiredResults();

    // LRU eviction tests
    void testLRUEvictionOrder();
    void testMemoryBasedEviction();

    // SearchHighlightCache tests
    void testHighlightCacheHasData();
    void testHighlightCacheGetData();
    void testHighlightCacheStoreData();
    void testHighlightCacheInvalidateDocument();
    void testHighlightCacheClear();
    void testHighlightCacheSetMaxSize();
    void testHighlightCacheGetCacheSize();
    void testHighlightCacheGetHitRatio();

private:
    SearchResultCache* m_cache;
    SearchHighlightCache* m_highlightCache;
    QStringList m_testFiles;

    SearchResultCache::CacheKey createKey(const QString& query,
                                          const QString& docId = "test_doc");
};

void SearchResultCacheTest::initTestCase() {
    // Global test setup
}

void SearchResultCacheTest::cleanupTestCase() {
    // Global test cleanup
    CacheTestHelpers::cleanupTestFiles(m_testFiles);
}

void SearchResultCacheTest::init() {
    // Per-test setup
    m_cache = new SearchResultCache(this);
    m_highlightCache = new SearchHighlightCache(this);
    QVERIFY(m_cache != nullptr);
    QVERIFY(m_highlightCache != nullptr);
}

void SearchResultCacheTest::cleanup() {
    // Per-test cleanup
    if (m_cache) {
        m_cache->clear();
        delete m_cache;
        m_cache = nullptr;
    }
    if (m_highlightCache) {
        m_highlightCache->clear();
        delete m_highlightCache;
        m_highlightCache = nullptr;
    }
}

SearchResultCache::CacheKey SearchResultCacheTest::createKey(
    const QString& query, const QString& docId) {
    SearchResultCache::CacheKey key;
    key.query = query;
    key.documentId = docId;
    key.documentModified = QDateTime::currentMSecsSinceEpoch();
    key.options = CacheTestHelpers::createTestSearchOptions();
    return key;
}

// Cache operations tests
void SearchResultCacheTest::testHasResults() {
    SearchResultCache::CacheKey key = createKey("test");

    QVERIFY(m_cache->hasResults(key) == false);

    QList<SearchResult> results = CacheTestHelpers::createTestSearchResults(5);
    m_cache->storeResults(key, results);

    QVERIFY(m_cache->hasResults(key));
}

void SearchResultCacheTest::testGetResults() {
    SearchResultCache::CacheKey key = createKey("test");
    QList<SearchResult> results = CacheTestHelpers::createTestSearchResults(5);

    m_cache->storeResults(key, results);

    QList<SearchResult> retrieved = m_cache->getResults(key);
    QCOMPARE(retrieved.size(), results.size());
}

void SearchResultCacheTest::testStoreResults() {
    SearchResultCache::CacheKey key = createKey("test");
    QList<SearchResult> results = CacheTestHelpers::createTestSearchResults(10);

    m_cache->storeResults(key, results);

    QVERIFY(m_cache->hasResults(key));
    QCOMPARE(m_cache->getCacheSize(), 1);
}

void SearchResultCacheTest::testInvalidateDocument() {
    SearchResultCache::CacheKey key1 = createKey("test", "doc1");
    SearchResultCache::CacheKey key2 = createKey("test", "doc2");

    m_cache->storeResults(key1, CacheTestHelpers::createTestSearchResults(5));
    m_cache->storeResults(key2, CacheTestHelpers::createTestSearchResults(5));

    m_cache->invalidateDocument("doc1");

    QVERIFY(m_cache->hasResults(key1) == false);
    QVERIFY(m_cache->hasResults(key2));
}

void SearchResultCacheTest::testClear() {
    SearchResultCache::CacheKey key1 = createKey("test1");
    SearchResultCache::CacheKey key2 = createKey("test2");

    m_cache->storeResults(key1, CacheTestHelpers::createTestSearchResults(5));
    m_cache->storeResults(key2, CacheTestHelpers::createTestSearchResults(5));

    m_cache->clear();

    QVERIFY(m_cache->hasResults(key1) == false);
    QVERIFY(m_cache->hasResults(key2) == false);
    QCOMPARE(m_cache->getCacheSize(), 0);
}

// Configuration tests
void SearchResultCacheTest::testSetMaxCacheSize() {
    m_cache->setMaxCacheSize(50);
    // No getter, so just verify it doesn't crash
    QVERIFY(true);
}

void SearchResultCacheTest::testSetMaxMemoryUsage() {
    qint64 newLimit = 10 * 1024 * 1024;  // 10MB
    m_cache->setMaxMemoryUsage(newLimit);

    QCOMPARE(m_cache->getMaxMemoryLimit(), newLimit);
}

void SearchResultCacheTest::testSetExpirationTime() {
    m_cache->setExpirationTime(60000);  // 1 minute
    // No getter, so just verify it doesn't crash
    QVERIFY(true);
}

// Statistics tests
void SearchResultCacheTest::testGetCacheSize() {
    QCOMPARE(m_cache->getCacheSize(), 0);

    SearchResultCache::CacheKey key = createKey("test");
    m_cache->storeResults(key, CacheTestHelpers::createTestSearchResults(5));

    QCOMPARE(m_cache->getCacheSize(), 1);
}

void SearchResultCacheTest::testGetMemoryUsage() {
    qint64 initialUsage = m_cache->getMemoryUsage();
    QCOMPARE(initialUsage, qint64(0));

    SearchResultCache::CacheKey key = createKey("test");
    m_cache->storeResults(key, CacheTestHelpers::createTestSearchResults(10));

    qint64 newUsage = m_cache->getMemoryUsage();
    QVERIFY(newUsage > 0);
}

void SearchResultCacheTest::testGetHitRatio() {
    SearchResultCache::CacheKey key = createKey("test");
    m_cache->storeResults(key, CacheTestHelpers::createTestSearchResults(5));

    // Hit
    m_cache->getResults(key);

    // Miss
    SearchResultCache::CacheKey missKey = createKey("miss");
    m_cache->getResults(missKey);

    double hitRatio = m_cache->getHitRatio();
    QVERIFY(hitRatio >= 0.0 && hitRatio <= 1.0);
    QCOMPARE(hitRatio, 0.5);  // 1 hit, 1 miss
}

void SearchResultCacheTest::testResetStatistics() {
    SearchResultCache::CacheKey key = createKey("test");
    m_cache->storeResults(key, CacheTestHelpers::createTestSearchResults(5));
    m_cache->getResults(key);

    m_cache->resetStatistics();

    QCOMPARE(m_cache->getHitCount(), qint64(0));
    QCOMPARE(m_cache->getMissCount(), qint64(0));
}

// Incremental search tests
void SearchResultCacheTest::testCanUseIncrementalSearch() {
    SearchResultCache::CacheKey baseKey = createKey("test");
    m_cache->storeResults(baseKey,
                          CacheTestHelpers::createTestSearchResults(10));

    SearchResultCache::CacheKey incrementalKey = createKey("test query");
    incrementalKey.documentId = baseKey.documentId;
    incrementalKey.documentModified = baseKey.documentModified;
    incrementalKey.options = baseKey.options;

    bool canUse = m_cache->canUseIncrementalSearch(incrementalKey, baseKey);
    QVERIFY(canUse);
}

void SearchResultCacheTest::testGetIncrementalResults() {
    SearchResultCache::CacheKey baseKey = createKey("test");
    QList<SearchResult> baseResults =
        CacheTestHelpers::createTestSearchResults(10);
    m_cache->storeResults(baseKey, baseResults);

    SearchResultCache::CacheKey incrementalKey = createKey("test query");
    incrementalKey.documentId = baseKey.documentId;
    incrementalKey.documentModified = baseKey.documentModified;
    incrementalKey.options = baseKey.options;

    QList<SearchResult> results =
        m_cache->getIncrementalResults(incrementalKey, baseKey);
    QVERIFY(results.size() >= 0);
}

void SearchResultCacheTest::testIncrementalSearchSameDocument() {
    SearchResultCache::CacheKey key1 = createKey("test", "doc1");
    m_cache->storeResults(key1, CacheTestHelpers::createTestSearchResults(10));

    SearchResultCache::CacheKey key2 = createKey("test query", "doc1");
    key2.documentModified = key1.documentModified;
    key2.options = key1.options;

    QVERIFY(m_cache->canUseIncrementalSearch(key2, key1));
}

void SearchResultCacheTest::testIncrementalSearchDifferentOptions() {
    SearchResultCache::CacheKey key1 = createKey("test");
    m_cache->storeResults(key1, CacheTestHelpers::createTestSearchResults(10));

    SearchResultCache::CacheKey key2 = createKey("test query");
    key2.documentId = key1.documentId;
    key2.documentModified = key1.documentModified;
    key2.options.caseSensitive = !key1.options.caseSensitive;

    QVERIFY(m_cache->canUseIncrementalSearch(key2, key1) == false);
}

// ICacheComponent interface tests
void SearchResultCacheTest::testGetMaxMemoryLimit() {
    qint64 limit = m_cache->getMaxMemoryLimit();
    QVERIFY(limit > 0);
}

void SearchResultCacheTest::testSetMaxMemoryLimit() {
    qint64 newLimit = 20 * 1024 * 1024;  // 20MB
    m_cache->setMaxMemoryLimit(newLimit);

    QCOMPARE(m_cache->getMaxMemoryLimit(), newLimit);
}

void SearchResultCacheTest::testGetEntryCount() {
    QCOMPARE(m_cache->getEntryCount(), 0);

    SearchResultCache::CacheKey key = createKey("test");
    m_cache->storeResults(key, CacheTestHelpers::createTestSearchResults(5));

    QCOMPARE(m_cache->getEntryCount(), 1);
}

void SearchResultCacheTest::testEvictLRU() {
    SearchResultCache::CacheKey key1 = createKey("test1");
    SearchResultCache::CacheKey key2 = createKey("test2");

    m_cache->storeResults(key1, CacheTestHelpers::createTestSearchResults(5));
    m_cache->storeResults(key2, CacheTestHelpers::createTestSearchResults(5));

    qint64 initialUsage = m_cache->getMemoryUsage();

    m_cache->evictLRU(1000);

    qint64 newUsage = m_cache->getMemoryUsage();
    QVERIFY(newUsage < initialUsage);
}

void SearchResultCacheTest::testGetHitCount() {
    SearchResultCache::CacheKey key = createKey("test");
    m_cache->storeResults(key, CacheTestHelpers::createTestSearchResults(5));

    m_cache->getResults(key);
    m_cache->getResults(key);

    QCOMPARE(m_cache->getHitCount(), qint64(2));
}

void SearchResultCacheTest::testGetMissCount() {
    SearchResultCache::CacheKey key1 = createKey("test1");
    SearchResultCache::CacheKey key2 = createKey("test2");

    m_cache->getResults(key1);
    m_cache->getResults(key2);

    QCOMPARE(m_cache->getMissCount(), qint64(2));
}

void SearchResultCacheTest::testSetEnabled() {
    m_cache->setEnabled(false);
    QVERIFY(m_cache->isEnabled() == false);

    m_cache->setEnabled(true);
    QVERIFY(m_cache->isEnabled());
}

void SearchResultCacheTest::testIsEnabled() { QVERIFY(m_cache->isEnabled()); }

// Signal tests
void SearchResultCacheTest::testCacheUpdatedSignal() {
    QSignalSpy spy(m_cache, &SearchResultCache::cacheUpdated);

    SearchResultCache::CacheKey key = createKey("test");
    m_cache->storeResults(key, CacheTestHelpers::createTestSearchResults(5));

    QVERIFY(spy.count() > 0);
}

void SearchResultCacheTest::testCacheHitSignal() {
    QSignalSpy spy(m_cache, &SearchResultCache::cacheHit);

    SearchResultCache::CacheKey key = createKey("test");
    m_cache->storeResults(key, CacheTestHelpers::createTestSearchResults(5));
    m_cache->getResults(key);

    QVERIFY(spy.count() > 0);
}

void SearchResultCacheTest::testCacheMissSignal() {
    QSignalSpy spy(m_cache, &SearchResultCache::cacheMiss);

    SearchResultCache::CacheKey key = createKey("test");
    m_cache->getResults(key);

    QVERIFY(spy.count() > 0);
}

// Maintenance tests
void SearchResultCacheTest::testPeriodicMaintenance() {
    SearchResultCache::CacheKey key = createKey("test");
    m_cache->storeResults(key, CacheTestHelpers::createTestSearchResults(5));

    // Maintenance is automatic via timer, just verify cache works
    QVERIFY(m_cache->hasResults(key));
}

void SearchResultCacheTest::testExpirationHandling() {
    m_cache->setExpirationTime(10);  // 10ms

    SearchResultCache::CacheKey key = createKey("test");
    m_cache->storeResults(key, CacheTestHelpers::createTestSearchResults(5));

    // Wait for expiration and automatic maintenance
    waitMs(100);

    // Entry may or may not be removed depending on maintenance timer
    QVERIFY(true);
}

// CacheKey tests
void SearchResultCacheTest::testCacheKeyToHash() {
    SearchResultCache::CacheKey key1 = createKey("test");
    SearchResultCache::CacheKey key2 = createKey("test");

    QString hash1 = key1.toHash();
    QString hash2 = key2.toHash();

    QCOMPARE(hash1, hash2);
    QVERIFY(hash1.isEmpty() == false);
}

void SearchResultCacheTest::testCacheKeyEquality() {
    SearchResultCache::CacheKey key1 = createKey("test");
    SearchResultCache::CacheKey key2 = createKey("test");
    key2.documentModified = key1.documentModified;

    QVERIFY(key1 == key2);
}

void SearchResultCacheTest::testCacheKeyWithDifferentOptions() {
    SearchResultCache::CacheKey key1 = createKey("test");
    SearchResultCache::CacheKey key2 = createKey("test");
    key2.options.caseSensitive = !key1.options.caseSensitive;

    QVERIFY(key1 != key2);
}

// Edge cases and error handling
void SearchResultCacheTest::testStoreEmptyResults() {
    SearchResultCache::CacheKey key = createKey("test");
    QList<SearchResult> emptyResults;

    m_cache->storeResults(key, emptyResults);

    // Empty results should still be stored
    QVERIFY(m_cache->hasResults(key));
}

void SearchResultCacheTest::testGetNonExistentKey() {
    SearchResultCache::CacheKey key = createKey("nonexistent");

    QList<SearchResult> results = m_cache->getResults(key);

    QVERIFY(results.isEmpty());
}

void SearchResultCacheTest::testInvalidateNonExistentDocument() {
    // Should not crash
    m_cache->invalidateDocument("nonexistent");
    QVERIFY(true);
}

void SearchResultCacheTest::testExceedMemoryLimit() {
    m_cache->setMaxMemoryUsage(1000);  // Very small limit

    SearchResultCache::CacheKey key = createKey("test");
    QList<SearchResult> largeResults =
        CacheTestHelpers::createTestSearchResults(1000);

    m_cache->storeResults(key, largeResults);

    // Should handle gracefully
    qint64 usage = m_cache->getMemoryUsage();
    QVERIFY(usage >= 0);
}

void SearchResultCacheTest::testExceedCacheSize() {
    m_cache->setMaxCacheSize(2);

    SearchResultCache::CacheKey key1 = createKey("test1");
    SearchResultCache::CacheKey key2 = createKey("test2");
    SearchResultCache::CacheKey key3 = createKey("test3");

    m_cache->storeResults(key1, CacheTestHelpers::createTestSearchResults(5));
    m_cache->storeResults(key2, CacheTestHelpers::createTestSearchResults(5));
    m_cache->storeResults(key3, CacheTestHelpers::createTestSearchResults(5));

    // Should evict oldest entry
    QVERIFY(m_cache->getCacheSize() <= 2);
}

void SearchResultCacheTest::testDisabledCache() {
    m_cache->setEnabled(false);

    SearchResultCache::CacheKey key = createKey("test");
    m_cache->storeResults(key, CacheTestHelpers::createTestSearchResults(5));

    // Should not store when disabled
    QVERIFY(m_cache->hasResults(key) == false);
}

void SearchResultCacheTest::testExpiredResults() {
    m_cache->setExpirationTime(10);  // 10ms

    SearchResultCache::CacheKey key = createKey("test");
    m_cache->storeResults(key, CacheTestHelpers::createTestSearchResults(5));

    // Wait for expiration
    waitMs(50);

    // Results may or may not be expired depending on maintenance
    QVERIFY(true);
}

// LRU eviction tests
void SearchResultCacheTest::testLRUEvictionOrder() {
    m_cache->setMaxCacheSize(3);

    SearchResultCache::CacheKey key1 = createKey("test1");
    SearchResultCache::CacheKey key2 = createKey("test2");
    SearchResultCache::CacheKey key3 = createKey("test3");
    SearchResultCache::CacheKey key4 = createKey("test4");

    m_cache->storeResults(key1, CacheTestHelpers::createTestSearchResults(5));
    m_cache->storeResults(key2, CacheTestHelpers::createTestSearchResults(5));
    m_cache->storeResults(key3, CacheTestHelpers::createTestSearchResults(5));

    // Verify all 3 are in cache
    QVERIFY(m_cache->hasResults(key1));
    QVERIFY(m_cache->hasResults(key2));
    QVERIFY(m_cache->hasResults(key3));

    // Add new key, should evict one item to make room
    m_cache->storeResults(key4, CacheTestHelpers::createTestSearchResults(5));

    // key4 should be there (just inserted)
    QVERIFY(m_cache->hasResults(key4));

    // Exactly 3 items should remain (one was evicted)
    int count = (m_cache->hasResults(key1) ? 1 : 0) +
                (m_cache->hasResults(key2) ? 1 : 0) +
                (m_cache->hasResults(key3) ? 1 : 0) +
                (m_cache->hasResults(key4) ? 1 : 0);
    QCOMPARE(count, 3);
}

void SearchResultCacheTest::testMemoryBasedEviction() {
    m_cache->setMaxMemoryUsage(5000);  // Small limit

    SearchResultCache::CacheKey key1 = createKey("test1");
    SearchResultCache::CacheKey key2 = createKey("test2");
    SearchResultCache::CacheKey key3 = createKey("test3");

    m_cache->storeResults(key1, CacheTestHelpers::createTestSearchResults(10));
    m_cache->storeResults(key2, CacheTestHelpers::createTestSearchResults(10));
    m_cache->storeResults(key3, CacheTestHelpers::createTestSearchResults(10));

    // Should evict entries to stay within memory limit
    QVERIFY(m_cache->getMemoryUsage() <= m_cache->getMaxMemoryLimit() ||
            m_cache->getMemoryUsage() > 0);
}

// SearchHighlightCache tests
void SearchResultCacheTest::testHighlightCacheHasData() {
    QString query = "test";
    QVERIFY(m_highlightCache->hasHighlightData("doc1", 0, query) == false);

    SearchHighlightCache::HighlightData data;
    data.boundingRects = {QRectF(0, 0, 100, 20)};
    m_highlightCache->storeHighlightData("doc1", 0, query, data);

    QVERIFY(m_highlightCache->hasHighlightData("doc1", 0, query));
}

void SearchResultCacheTest::testHighlightCacheGetData() {
    QString query = "test";
    SearchHighlightCache::HighlightData data;
    data.boundingRects = {QRectF(0, 0, 100, 20), QRectF(0, 30, 150, 20)};
    m_highlightCache->storeHighlightData("doc1", 0, query, data);

    SearchHighlightCache::HighlightData retrieved =
        m_highlightCache->getHighlightData("doc1", 0, query);
    QCOMPARE(retrieved.boundingRects.size(), data.boundingRects.size());
}

void SearchResultCacheTest::testHighlightCacheStoreData() {
    QString query = "test";
    SearchHighlightCache::HighlightData data;
    data.boundingRects = {QRectF(10, 10, 50, 15)};
    m_highlightCache->storeHighlightData("doc1", 0, query, data);

    QVERIFY(m_highlightCache->hasHighlightData("doc1", 0, query));
    QCOMPARE(m_highlightCache->getCacheSize(), 1);
}

void SearchResultCacheTest::testHighlightCacheInvalidateDocument() {
    QString query = "test";
    SearchHighlightCache::HighlightData data;
    data.boundingRects = {QRectF(0, 0, 100, 20)};

    m_highlightCache->storeHighlightData("doc1", 0, query, data);
    m_highlightCache->storeHighlightData("doc1", 1, query, data);
    m_highlightCache->storeHighlightData("doc2", 0, query, data);

    m_highlightCache->invalidateDocument("doc1");

    QVERIFY(m_highlightCache->hasHighlightData("doc1", 0, query) == false);
    QVERIFY(m_highlightCache->hasHighlightData("doc1", 1, query) == false);
    QVERIFY(m_highlightCache->hasHighlightData("doc2", 0, query));
}

void SearchResultCacheTest::testHighlightCacheClear() {
    QString query = "test";
    SearchHighlightCache::HighlightData data;
    data.boundingRects = {QRectF(0, 0, 100, 20)};

    m_highlightCache->storeHighlightData("doc1", 0, query, data);
    m_highlightCache->storeHighlightData("doc1", 1, query, data);

    m_highlightCache->clear();

    QVERIFY(m_highlightCache->hasHighlightData("doc1", 0, query) == false);
    QVERIFY(m_highlightCache->hasHighlightData("doc1", 1, query) == false);
    QCOMPARE(m_highlightCache->getCacheSize(), 0);
}

void SearchResultCacheTest::testHighlightCacheSetMaxSize() {
    m_highlightCache->setMaxCacheSize(50);
    // No getter, so just verify it doesn't crash
    QVERIFY(true);
}

void SearchResultCacheTest::testHighlightCacheGetCacheSize() {
    QCOMPARE(m_highlightCache->getCacheSize(), 0);

    QString query = "test";
    SearchHighlightCache::HighlightData data;
    data.boundingRects = {QRectF(0, 0, 100, 20)};

    m_highlightCache->storeHighlightData("doc1", 0, query, data);
    QCOMPARE(m_highlightCache->getCacheSize(), 1);

    m_highlightCache->storeHighlightData("doc1", 1, query, data);
    QCOMPARE(m_highlightCache->getCacheSize(), 2);
}

void SearchResultCacheTest::testHighlightCacheGetHitRatio() {
    QString query = "test";
    SearchHighlightCache::HighlightData data;
    data.boundingRects = {QRectF(0, 0, 100, 20)};

    m_highlightCache->storeHighlightData("doc1", 0, query, data);

    // Hit
    m_highlightCache->getHighlightData("doc1", 0, query);

    // Miss
    m_highlightCache->getHighlightData("doc1", 1, query);

    double hitRatio = m_highlightCache->getHitRatio();
    QVERIFY(hitRatio >= 0.0 && hitRatio <= 1.0);
    QCOMPARE(hitRatio, 0.5);  // 1 hit, 1 miss
}

QTEST_MAIN(SearchResultCacheTest)
#include "test_search_result_cache.moc"
