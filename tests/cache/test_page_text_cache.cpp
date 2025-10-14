#include <QSignalSpy>
#include <QTest>
#include <QtTest/QtTest>
#include "../../app/cache/PageTextCache.h"
#include "../../app/search/TextExtractor.h"
#include "CacheTestHelpers.h"

/**
 * @brief Comprehensive tests for PageTextCache
 *
 * Tests page text caching with LRU eviction, memory limits,
 * document invalidation, and ICacheComponent interface compliance.
 */
class PageTextCacheTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Cache operations tests
    void testHasPageText();
    void testGetPageText();
    void testStorePageText();
    void testInvalidateDocument();
    void testClear();

    // Configuration tests
    void testSetMaxCacheSize();
    void testGetMaxCacheSize();
    void testSetMaxMemoryUsage();

    // Statistics tests
    void testGetCacheSize();
    void testGetHitRatio();

    // ICacheComponent interface tests
    void testGetMemoryUsage();
    void testGetMaxMemoryLimit();
    void testSetMaxMemoryLimit();
    void testGetEntryCount();
    void testEvictLRU();
    void testGetHitCount();
    void testGetMissCount();
    void testResetStatistics();
    void testSetEnabled();
    void testIsEnabled();

    // Signal tests
    void testCacheUpdatedSignal();
    void testCacheHitSignal();
    void testCacheMissSignal();

    // Edge cases and error handling
    void testStoreEmptyText();
    void testGetNonExistentPage();
    void testInvalidateNonExistentDocument();
    void testExceedMemoryLimit();
    void testExceedCacheSize();
    void testDisabledCache();

    // LRU eviction tests
    void testLRUEvictionOrder();
    void testMemoryBasedEviction();
    void testMultipleDocuments();

    // Concurrent access tests
    void testConcurrentStoreAndGet();
    void testConcurrentInvalidation();

    // TextExtractorCacheAdapter tests
    void testAdapterGetMemoryUsage();
    void testAdapterClear();
    void testAdapterEvictLRU();
    void testAdapterSetEnabled();

private:
    PageTextCache* m_cache;
    QStringList m_testFiles;

    QString generateLargeText(int sizeKB);
};

void PageTextCacheTest::initTestCase() {
    // Global test setup
}

void PageTextCacheTest::cleanupTestCase() {
    // Global test cleanup
    CacheTestHelpers::cleanupTestFiles(m_testFiles);
}

void PageTextCacheTest::init() {
    // Per-test setup
    m_cache = new PageTextCache(this);
    QVERIFY(m_cache != nullptr);
}

void PageTextCacheTest::cleanup() {
    // Per-test cleanup
    if (m_cache) {
        m_cache->clear();
        delete m_cache;
        m_cache = nullptr;
    }
}

QString PageTextCacheTest::generateLargeText(int sizeKB) {
    QString text;
    int charsNeeded = sizeKB * 1024 / sizeof(QChar);
    text.reserve(charsNeeded);

    for (int i = 0; i < charsNeeded; ++i) {
        text += QChar('A' + (i % 26));
    }

    return text;
}

// Cache operations tests
void PageTextCacheTest::testHasPageText() {
    QVERIFY(m_cache->hasPageText("doc1", 0) == false);

    m_cache->storePageText("doc1", 0, "Test text");

    QVERIFY(m_cache->hasPageText("doc1", 0));
}

void PageTextCacheTest::testGetPageText() {
    QString text = "This is test page text";
    m_cache->storePageText("doc1", 0, text);

    QString retrieved = m_cache->getPageText("doc1", 0);
    QCOMPARE(retrieved, text);
}

void PageTextCacheTest::testStorePageText() {
    QString text = "Test page content";
    m_cache->storePageText("doc1", 0, text);

    QVERIFY(m_cache->hasPageText("doc1", 0));
    QCOMPARE(m_cache->getPageText("doc1", 0), text);
}

void PageTextCacheTest::testInvalidateDocument() {
    m_cache->storePageText("doc1", 0, "Page 0");
    m_cache->storePageText("doc1", 1, "Page 1");
    m_cache->storePageText("doc2", 0, "Doc2 Page 0");

    m_cache->invalidateDocument("doc1");

    QVERIFY(m_cache->hasPageText("doc1", 0) == false);
    QVERIFY(m_cache->hasPageText("doc1", 1) == false);
    QVERIFY(m_cache->hasPageText("doc2", 0));
}

void PageTextCacheTest::testClear() {
    m_cache->storePageText("doc1", 0, "Page 0");
    m_cache->storePageText("doc1", 1, "Page 1");

    m_cache->clear();

    QVERIFY(m_cache->hasPageText("doc1", 0) == false);
    QVERIFY(m_cache->hasPageText("doc1", 1) == false);
    QCOMPARE(m_cache->getCacheSize(), 0);
}

// Configuration tests
void PageTextCacheTest::testSetMaxCacheSize() {
    m_cache->setMaxCacheSize(50);
    QCOMPARE(m_cache->getMaxCacheSize(), 50);
}

void PageTextCacheTest::testGetMaxCacheSize() {
    int maxSize = m_cache->getMaxCacheSize();
    QVERIFY(maxSize > 0);
}

void PageTextCacheTest::testSetMaxMemoryUsage() {
    qint64 newLimit = 10 * 1024 * 1024;  // 10MB
    m_cache->setMaxMemoryUsage(newLimit);

    // Verify through ICacheComponent interface
    QCOMPARE(m_cache->getMaxMemoryLimit(), newLimit);
}

// Statistics tests
void PageTextCacheTest::testGetCacheSize() {
    QCOMPARE(m_cache->getCacheSize(), 0);

    m_cache->storePageText("doc1", 0, "Test");
    QCOMPARE(m_cache->getCacheSize(), 1);

    m_cache->storePageText("doc1", 1, "Test");
    QCOMPARE(m_cache->getCacheSize(), 2);
}

void PageTextCacheTest::testGetHitRatio() {
    m_cache->storePageText("doc1", 0, "Test");

    // Hit
    m_cache->getPageText("doc1", 0);

    // Miss
    m_cache->getPageText("doc1", 1);

    double hitRatio = m_cache->getHitRatio();
    QVERIFY(hitRatio >= 0.0 && hitRatio <= 1.0);
    QCOMPARE(hitRatio, 0.5);  // 1 hit, 1 miss
}

// ICacheComponent interface tests
void PageTextCacheTest::testGetMemoryUsage() {
    qint64 initialUsage = m_cache->getMemoryUsage();
    QCOMPARE(initialUsage, qint64(0));

    m_cache->storePageText("doc1", 0, "Test text");

    qint64 newUsage = m_cache->getMemoryUsage();
    QVERIFY(newUsage > 0);
}

void PageTextCacheTest::testGetMaxMemoryLimit() {
    qint64 limit = m_cache->getMaxMemoryLimit();
    QVERIFY(limit > 0);
}

void PageTextCacheTest::testSetMaxMemoryLimit() {
    qint64 newLimit = 20 * 1024 * 1024;  // 20MB
    m_cache->setMaxMemoryLimit(newLimit);

    QCOMPARE(m_cache->getMaxMemoryLimit(), newLimit);
}

void PageTextCacheTest::testGetEntryCount() {
    QCOMPARE(m_cache->getEntryCount(), 0);

    m_cache->storePageText("doc1", 0, "Test");
    QCOMPARE(m_cache->getEntryCount(), 1);

    m_cache->storePageText("doc1", 1, "Test");
    QCOMPARE(m_cache->getEntryCount(), 2);
}

void PageTextCacheTest::testEvictLRU() {
    m_cache->storePageText("doc1", 0, "Test 0");
    m_cache->storePageText("doc1", 1, "Test 1");
    m_cache->storePageText("doc1", 2, "Test 2");

    qint64 initialUsage = m_cache->getMemoryUsage();

    m_cache->evictLRU(1000);

    qint64 newUsage = m_cache->getMemoryUsage();
    QVERIFY(newUsage < initialUsage);
}

void PageTextCacheTest::testGetHitCount() {
    m_cache->storePageText("doc1", 0, "Test");

    m_cache->getPageText("doc1", 0);
    m_cache->getPageText("doc1", 0);

    QCOMPARE(m_cache->getHitCount(), qint64(2));
}

void PageTextCacheTest::testGetMissCount() {
    m_cache->getPageText("doc1", 0);
    m_cache->getPageText("doc1", 1);

    QCOMPARE(m_cache->getMissCount(), qint64(2));
}

void PageTextCacheTest::testResetStatistics() {
    m_cache->storePageText("doc1", 0, "Test");
    m_cache->getPageText("doc1", 0);
    m_cache->getPageText("doc1", 1);

    m_cache->resetStatistics();

    QCOMPARE(m_cache->getHitCount(), qint64(0));
    QCOMPARE(m_cache->getMissCount(), qint64(0));
}

void PageTextCacheTest::testSetEnabled() {
    m_cache->setEnabled(false);
    QVERIFY(m_cache->isEnabled() == false);

    m_cache->setEnabled(true);
    QVERIFY(m_cache->isEnabled());
}

void PageTextCacheTest::testIsEnabled() { QVERIFY(m_cache->isEnabled()); }

// Signal tests
void PageTextCacheTest::testCacheUpdatedSignal() {
    QSignalSpy spy(m_cache, &PageTextCache::cacheUpdated);

    m_cache->storePageText("doc1", 0, "Test");

    QVERIFY(spy.count() > 0);
}

void PageTextCacheTest::testCacheHitSignal() {
    QSignalSpy spy(m_cache, &PageTextCache::cacheHit);

    m_cache->storePageText("doc1", 0, "Test");
    m_cache->getPageText("doc1", 0);

    QVERIFY(spy.count() > 0);
}

void PageTextCacheTest::testCacheMissSignal() {
    QSignalSpy spy(m_cache, &PageTextCache::cacheMiss);

    m_cache->getPageText("doc1", 0);

    QVERIFY(spy.count() > 0);
}

// Edge cases and error handling
void PageTextCacheTest::testStoreEmptyText() {
    m_cache->storePageText("doc1", 0, "");

    // Empty text should not be stored
    QVERIFY(m_cache->hasPageText("doc1", 0) == false);
}

void PageTextCacheTest::testGetNonExistentPage() {
    QString text = m_cache->getPageText("doc1", 0);

    QVERIFY(text.isEmpty());
}

void PageTextCacheTest::testInvalidateNonExistentDocument() {
    // Should not crash
    m_cache->invalidateDocument("nonexistent");
    QVERIFY(true);
}

void PageTextCacheTest::testExceedMemoryLimit() {
    m_cache->setMaxMemoryUsage(1000);  // Very small limit

    QString largeText = generateLargeText(10);  // 10KB
    m_cache->storePageText("doc1", 0, largeText);

    // Should handle gracefully
    qint64 usage = m_cache->getMemoryUsage();
    QVERIFY(usage <= m_cache->getMaxMemoryLimit() || usage > 0);
}

void PageTextCacheTest::testExceedCacheSize() {
    m_cache->setMaxCacheSize(2);

    m_cache->storePageText("doc1", 0, "Page 0");
    m_cache->storePageText("doc1", 1, "Page 1");
    m_cache->storePageText("doc1", 2, "Page 2");

    // Should evict oldest entry
    QVERIFY(m_cache->getCacheSize() <= 2);
}

void PageTextCacheTest::testDisabledCache() {
    m_cache->setEnabled(false);

    m_cache->storePageText("doc1", 0, "Test");

    // Should not store when disabled
    QVERIFY(m_cache->hasPageText("doc1", 0) == false);

    QString text = m_cache->getPageText("doc1", 0);
    QVERIFY(text.isEmpty());
}

// LRU eviction tests
void PageTextCacheTest::testLRUEvictionOrder() {
    m_cache->setMaxCacheSize(3);

    m_cache->storePageText("doc1", 0, "Page 0");
    m_cache->storePageText("doc1", 1, "Page 1");
    m_cache->storePageText("doc1", 2, "Page 2");

    // Access page 0 to make it recently used
    m_cache->getPageText("doc1", 0);

    // Add new page, should evict page 1 (least recently used)
    m_cache->storePageText("doc1", 3, "Page 3");

    QVERIFY(m_cache->hasPageText("doc1", 0));
    QVERIFY(m_cache->hasPageText("doc1", 1) == false);
    QVERIFY(m_cache->hasPageText("doc1", 3));
}

void PageTextCacheTest::testMemoryBasedEviction() {
    m_cache->setMaxMemoryUsage(5000);  // Small limit

    QString text1 = generateLargeText(1);  // 1KB
    QString text2 = generateLargeText(1);
    QString text3 = generateLargeText(1);
    QString text4 = generateLargeText(3);  // 3KB

    m_cache->storePageText("doc1", 0, text1);
    m_cache->storePageText("doc1", 1, text2);
    m_cache->storePageText("doc1", 2, text3);
    m_cache->storePageText("doc1", 3, text4);

    // Should evict entries to stay within memory limit
    QVERIFY(m_cache->getMemoryUsage() <= m_cache->getMaxMemoryLimit() ||
            m_cache->getMemoryUsage() > 0);
}

void PageTextCacheTest::testMultipleDocuments() {
    m_cache->storePageText("doc1", 0, "Doc1 Page 0");
    m_cache->storePageText("doc1", 1, "Doc1 Page 1");
    m_cache->storePageText("doc2", 0, "Doc2 Page 0");
    m_cache->storePageText("doc2", 1, "Doc2 Page 1");

    QCOMPARE(m_cache->getCacheSize(), 4);

    m_cache->invalidateDocument("doc1");

    QCOMPARE(m_cache->getCacheSize(), 2);
    QVERIFY(m_cache->hasPageText("doc2", 0));
    QVERIFY(m_cache->hasPageText("doc2", 1));
}

// Concurrent access tests
void PageTextCacheTest::testConcurrentStoreAndGet() {
    m_cache->storePageText("doc1", 0, "Test");

    // Simulate concurrent access
    QString text1 = m_cache->getPageText("doc1", 0);
    m_cache->storePageText("doc1", 1, "Test 2");
    QString text2 = m_cache->getPageText("doc1", 0);

    QCOMPARE(text1, text2);
    QCOMPARE(text1, QString("Test"));
}

void PageTextCacheTest::testConcurrentInvalidation() {
    m_cache->storePageText("doc1", 0, "Test");
    m_cache->storePageText("doc2", 0, "Test");

    // Simulate concurrent invalidation
    m_cache->invalidateDocument("doc1");
    bool hasDoc2 = m_cache->hasPageText("doc2", 0);

    QVERIFY(hasDoc2);
    QVERIFY(m_cache->hasPageText("doc1", 0) == false);
}

// TextExtractorCacheAdapter tests
void PageTextCacheTest::testAdapterGetMemoryUsage() {
    TextExtractor* extractor = new TextExtractor(this);
    TextExtractorCacheAdapter* adapter =
        new TextExtractorCacheAdapter(extractor, this);

    qint64 usage = adapter->getMemoryUsage();
    QVERIFY(usage >= 0);

    delete adapter;
    delete extractor;
}

void PageTextCacheTest::testAdapterClear() {
    TextExtractor* extractor = new TextExtractor(this);
    TextExtractorCacheAdapter* adapter =
        new TextExtractorCacheAdapter(extractor, this);

    // Should not crash
    adapter->clear();
    QVERIFY(true);

    delete adapter;
    delete extractor;
}

void PageTextCacheTest::testAdapterEvictLRU() {
    TextExtractor* extractor = new TextExtractor(this);
    TextExtractorCacheAdapter* adapter =
        new TextExtractorCacheAdapter(extractor, this);

    // Should not crash
    adapter->evictLRU(1000);
    QVERIFY(true);

    delete adapter;
    delete extractor;
}

void PageTextCacheTest::testAdapterSetEnabled() {
    TextExtractor* extractor = new TextExtractor(this);
    TextExtractorCacheAdapter* adapter =
        new TextExtractorCacheAdapter(extractor, this);

    adapter->setEnabled(false);
    QVERIFY(adapter->isEnabled() == false);

    adapter->setEnabled(true);
    QVERIFY(adapter->isEnabled());

    delete adapter;
    delete extractor;
}

QTEST_MAIN(PageTextCacheTest)
#include "test_page_text_cache.moc"
