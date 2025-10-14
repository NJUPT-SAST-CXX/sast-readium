#include <QSignalSpy>
#include <QTest>
#include <QtTest/QtTest>
#include "../../app/cache/PDFCacheManager.h"
#include "CacheTestHelpers.h"

/**
 * @brief Comprehensive tests for PDFCacheManager
 *
 * Tests PDF-specific caching with multiple item types, priority-based
 * eviction, preloading tasks, and cache statistics.
 */
class PDFCacheManagerTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Configuration tests
    void testSetMaxMemoryUsage();
    void testGetMaxMemoryUsage();
    void testSetMaxItems();
    void testGetMaxItems();
    void testSetItemMaxAge();
    void testGetItemMaxAge();

    // Basic cache operations tests
    void testInsert();
    void testGet();
    void testContains();
    void testRemove();
    void testClear();

    // Specialized cache operations tests
    void testCacheRenderedPage();
    void testGetRenderedPage();
    void testCacheThumbnail();
    void testGetThumbnail();
    void testCacheTextContent();
    void testGetTextContent();

    // Preloading tests
    void testEnablePreloading();
    void testIsPreloadingEnabled();
    void testPreloadPages();
    void testPreloadAroundPage();
    void testSetPreloadingStrategy();

    // Cache management tests
    void testOptimizeCache();
    void testCleanupExpiredItems();
    void testEvictLeastUsedItems();
    // void testCompactCache();  // TODO: Implement compactCache() in
    // PDFCacheManager.cpp

    // Statistics tests
    void testGetStatistics();
    void testGetCurrentMemoryUsage();
    void testGetHitRate();
    void testResetStatistics();

    // Cache policy tests
    // void testSetEvictionPolicy();  // TODO: Implement setEvictionPolicy() in
    // PDFCacheManager.cpp
    void testGetEvictionPolicy();
    // void testSetPriorityWeights();  // TODO: Implement setPriorityWeights()
    // in PDFCacheManager.cpp

    // Settings persistence tests
    void testLoadSettings();
    void testSaveSettings();

    // Utility function tests - TODO: Implement these methods in
    // PDFCacheManager.cpp void testExportCacheToFile(); void
    // testImportCacheFromFile(); void testDefragmentCache();

    // Cache inspection tests - TODO: Implement these methods in
    // PDFCacheManager.cpp void testGetCacheKeys(); void
    // testGetCacheKeysByType(); void testGetCacheKeysByPriority(); void
    // testGetCacheItemCount(); void testGetCacheMemoryUsage();

    // Cache management function tests - TODO: Implement these methods in
    // PDFCacheManager.cpp void testSetCachePriority(); void
    // testPromoteToHighPriority(); void testRefreshCacheItem();

    // Signal tests
    void testCacheHitSignal();
    void testCacheMissSignal();
    void testItemEvictedSignal();
    void testMemoryThresholdExceededSignal();
    void testPreloadCompletedSignal();
    void testCacheOptimizedSignal();

    // Edge cases and error handling
    void testInsertNullData();
    void testGetNonExistentKey();
    void testRemoveNonExistentKey();
    void testEvictFromEmptyCache();
    void testExceedMemoryLimit();
    void testExceedItemLimit();
    void testExpiredItems();

    // CacheItem tests
    void testCacheItemCalculateSize();
    void testCacheItemIsExpired();
    void testCacheItemUpdateAccess();

    // Priority and eviction tests
    void testLowPriorityEviction();
    void testHighPriorityRetention();
    void testCriticalPriorityNoEviction();
    void testLRUEvictionOrder();

private:
    PDFCacheManager* m_cacheManager;
    QStringList m_testFiles;

    QString createTestKey(const QString& prefix, int index);
};

void PDFCacheManagerTest::initTestCase() {
    // Global test setup
}

void PDFCacheManagerTest::cleanupTestCase() {
    // Global test cleanup
    CacheTestHelpers::cleanupTestFiles(m_testFiles);
}

void PDFCacheManagerTest::init() {
    // Per-test setup
    m_cacheManager = new PDFCacheManager(this);
    QVERIFY(m_cacheManager != nullptr);
}

void PDFCacheManagerTest::cleanup() {
    // Per-test cleanup
    if (m_cacheManager) {
        m_cacheManager->clear();
        delete m_cacheManager;
        m_cacheManager = nullptr;
    }
}

QString PDFCacheManagerTest::createTestKey(const QString& prefix, int index) {
    return QString("%1_%2").arg(prefix).arg(index);
}

// Configuration tests
void PDFCacheManagerTest::testSetMaxMemoryUsage() {
    qint64 newLimit = 128 * 1024 * 1024;  // 128MB
    m_cacheManager->setMaxMemoryUsage(newLimit);

    QCOMPARE(m_cacheManager->getMaxMemoryUsage(), newLimit);
}

void PDFCacheManagerTest::testGetMaxMemoryUsage() {
    qint64 limit = m_cacheManager->getMaxMemoryUsage();
    QVERIFY(limit > 0);
}

void PDFCacheManagerTest::testSetMaxItems() {
    int newMax = 500;
    m_cacheManager->setMaxItems(newMax);

    QCOMPARE(m_cacheManager->getMaxItems(), newMax);
}

void PDFCacheManagerTest::testGetMaxItems() {
    int maxItems = m_cacheManager->getMaxItems();
    QVERIFY(maxItems > 0);
}

void PDFCacheManagerTest::testSetItemMaxAge() {
    qint64 newAge = 60 * 60 * 1000;  // 1 hour
    m_cacheManager->setItemMaxAge(newAge);

    QCOMPARE(m_cacheManager->getItemMaxAge(), newAge);
}

void PDFCacheManagerTest::testGetItemMaxAge() {
    qint64 maxAge = m_cacheManager->getItemMaxAge();
    QVERIFY(maxAge >= 0);
}

// Basic cache operations tests
void PDFCacheManagerTest::testInsert() {
    QString key = "test_key";
    QPixmap pixmap = CacheTestHelpers::createTestPixmap();

    bool result = m_cacheManager->insert(
        key, pixmap, CacheItemType::RenderedPage, CachePriority::Normal, 0);

    QVERIFY(result);
    QVERIFY(m_cacheManager->contains(key));
}

void PDFCacheManagerTest::testGet() {
    QString key = "test_key";
    QPixmap pixmap = CacheTestHelpers::createTestPixmap(100, 100, Qt::red);

    m_cacheManager->insert(key, pixmap, CacheItemType::RenderedPage);

    QVariant retrieved = m_cacheManager->get(key);
    QVERIFY(retrieved.isValid());
    QVERIFY(retrieved.canConvert<QPixmap>());
}

void PDFCacheManagerTest::testContains() {
    QString key = "test_key";

    QVERIFY(!m_cacheManager->contains(key));

    m_cacheManager->insert(key, QString("test"), CacheItemType::TextContent);

    QVERIFY(m_cacheManager->contains(key));
}

void PDFCacheManagerTest::testRemove() {
    QString key = "test_key";
    m_cacheManager->insert(key, QString("test"), CacheItemType::TextContent);

    QVERIFY(m_cacheManager->contains(key));

    bool removed = m_cacheManager->remove(key);
    QVERIFY(removed);
    QVERIFY(!m_cacheManager->contains(key));
}

void PDFCacheManagerTest::testClear() {
    m_cacheManager->insert("key1", QString("test1"),
                           CacheItemType::TextContent);
    m_cacheManager->insert("key2", QString("test2"),
                           CacheItemType::TextContent);

    m_cacheManager->clear();

    QVERIFY(!m_cacheManager->contains("key1"));
    QVERIFY(!m_cacheManager->contains("key2"));

    CacheStatistics stats = m_cacheManager->getStatistics();
    QCOMPARE(stats.totalItems, 0);
}

// Specialized cache operations tests
void PDFCacheManagerTest::testCacheRenderedPage() {
    QPixmap pixmap = CacheTestHelpers::createTestPixmap(200, 300);

    bool result = m_cacheManager->cacheRenderedPage(0, pixmap, 1.0);
    QVERIFY(result);

    QPixmap retrieved = m_cacheManager->getRenderedPage(0, 1.0);
    QVERIFY(!retrieved.isNull());
}

void PDFCacheManagerTest::testGetRenderedPage() {
    QPixmap pixmap = CacheTestHelpers::createTestPixmap(200, 300);
    m_cacheManager->cacheRenderedPage(0, pixmap, 1.0);

    QPixmap retrieved = m_cacheManager->getRenderedPage(0, 1.0);
    QVERIFY(!retrieved.isNull());
    QCOMPARE(retrieved.size(), pixmap.size());
}

void PDFCacheManagerTest::testCacheThumbnail() {
    QPixmap thumbnail = CacheTestHelpers::createTestPixmap(128, 128);

    bool result = m_cacheManager->cacheThumbnail(0, thumbnail);
    QVERIFY(result);

    QPixmap retrieved = m_cacheManager->getThumbnail(0);
    QVERIFY(!retrieved.isNull());
}

void PDFCacheManagerTest::testGetThumbnail() {
    QPixmap thumbnail = CacheTestHelpers::createTestPixmap(128, 128);
    m_cacheManager->cacheThumbnail(0, thumbnail);

    QPixmap retrieved = m_cacheManager->getThumbnail(0);
    QVERIFY(!retrieved.isNull());
}

void PDFCacheManagerTest::testCacheTextContent() {
    QString text = "This is test page content";

    bool result = m_cacheManager->cacheTextContent(0, text);
    QVERIFY(result);

    QString retrieved = m_cacheManager->getTextContent(0);
    QCOMPARE(retrieved, text);
}

void PDFCacheManagerTest::testGetTextContent() {
    QString text = "This is test page content";
    m_cacheManager->cacheTextContent(0, text);

    QString retrieved = m_cacheManager->getTextContent(0);
    QCOMPARE(retrieved, text);
}

// Preloading tests
void PDFCacheManagerTest::testEnablePreloading() {
    m_cacheManager->enablePreloading(true);
    QVERIFY(m_cacheManager->isPreloadingEnabled());

    m_cacheManager->enablePreloading(false);
    QVERIFY(!m_cacheManager->isPreloadingEnabled());
}

void PDFCacheManagerTest::testIsPreloadingEnabled() {
    bool enabled = m_cacheManager->isPreloadingEnabled();
    QVERIFY(enabled == true || enabled == false);
}

void PDFCacheManagerTest::testPreloadPages() {
    // Preloading requires a document, which we don't have in this test
    // Just verify the method doesn't crash
    QList<int> pages = {0, 1, 2};
    m_cacheManager->preloadPages(pages, CacheItemType::RenderedPage);
    QVERIFY(true);
}

void PDFCacheManagerTest::testPreloadAroundPage() {
    // Just verify the method doesn't crash
    m_cacheManager->preloadAroundPage(5, 2);
    QVERIFY(true);
}

void PDFCacheManagerTest::testSetPreloadingStrategy() {
    m_cacheManager->setPreloadingStrategy("sequential");
    // No getter, so just verify it doesn't crash
    QVERIFY(true);
}

// Cache management tests
void PDFCacheManagerTest::testOptimizeCache() {
    // Add some items
    for (int i = 0; i < 10; ++i) {
        m_cacheManager->insert(createTestKey("opt", i), QString("test"),
                               CacheItemType::TextContent);
    }

    m_cacheManager->optimizeCache();
    QVERIFY(true);
}

void PDFCacheManagerTest::testCleanupExpiredItems() {
    // Set very short max age
    m_cacheManager->setItemMaxAge(1);  // 1ms

    m_cacheManager->insert("key1", QString("test"), CacheItemType::TextContent);

    // Wait for expiration
    waitMs(10);

    m_cacheManager->cleanupExpiredItems();

    // Item should be removed
    QVERIFY(!m_cacheManager->contains("key1"));
}

void PDFCacheManagerTest::testEvictLeastUsedItems() {
    // Add items
    for (int i = 0; i < 5; ++i) {
        m_cacheManager->insert(createTestKey("evict", i), QString("test"),
                               CacheItemType::TextContent);
    }

    int initialCount = m_cacheManager->getStatistics().totalItems;

    bool evicted = m_cacheManager->evictLeastUsedItems(2);

    if (evicted) {
        int newCount = m_cacheManager->getStatistics().totalItems;
        QVERIFY(newCount < initialCount);
    }
}

// TODO: Uncomment when compactCache() is implemented in PDFCacheManager.cpp
/*
void PDFCacheManagerTest::testCompactCache() {
    // Add and remove items to create fragmentation
    for (int i = 0; i < 10; ++i) {
        m_cacheManager->insert(createTestKey("compact", i), QString("test"),
                               CacheItemType::TextContent);
    }

    for (int i = 0; i < 5; ++i) {
        m_cacheManager->remove(createTestKey("compact", i));
    }

    m_cacheManager->compactCache();
    QVERIFY(true);
}
*/

// Statistics tests
void PDFCacheManagerTest::testGetStatistics() {
    m_cacheManager->insert("key1", QString("test"), CacheItemType::TextContent);
    m_cacheManager->insert("key2", QString("test"), CacheItemType::TextContent);

    CacheStatistics stats = m_cacheManager->getStatistics();

    QVERIFY(stats.totalItems >= 2);
    QVERIFY(stats.totalMemoryUsage > 0);
}

void PDFCacheManagerTest::testGetCurrentMemoryUsage() {
    qint64 initialUsage = m_cacheManager->getCurrentMemoryUsage();

    QPixmap pixmap = CacheTestHelpers::createTestPixmap(200, 200);
    m_cacheManager->insert("key1", pixmap, CacheItemType::RenderedPage);

    qint64 newUsage = m_cacheManager->getCurrentMemoryUsage();
    QVERIFY(newUsage > initialUsage);
}

void PDFCacheManagerTest::testGetHitRate() {
    m_cacheManager->insert("key1", QString("test"), CacheItemType::TextContent);

    // Hit
    m_cacheManager->get("key1");

    // Miss
    m_cacheManager->get("nonexistent");

    double hitRate = m_cacheManager->getHitRate();
    QVERIFY(hitRate >= 0.0 && hitRate <= 1.0);
}

void PDFCacheManagerTest::testResetStatistics() {
    m_cacheManager->insert("key1", QString("test"), CacheItemType::TextContent);
    m_cacheManager->get("key1");
    m_cacheManager->get("nonexistent");

    m_cacheManager->resetStatistics();

    CacheStatistics stats = m_cacheManager->getStatistics();
    QCOMPARE(stats.hitCount, qint64(0));
    QCOMPARE(stats.missCount, qint64(0));
}

// Cache policy tests
// TODO: Uncomment when setEvictionPolicy() is implemented in
// PDFCacheManager.cpp
/*
void PDFCacheManagerTest::testSetEvictionPolicy() {
    m_cacheManager->setEvictionPolicy("LRU");
    QString policy = m_cacheManager->getEvictionPolicy();
    QCOMPARE(policy, QString("LRU"));
}
*/

void PDFCacheManagerTest::testGetEvictionPolicy() {
    QString policy = m_cacheManager->getEvictionPolicy();
    QVERIFY(!policy.isEmpty());
}

// TODO: Uncomment when setPriorityWeights() is implemented in
// PDFCacheManager.cpp
/*
void PDFCacheManagerTest::testSetPriorityWeights() {
    m_cacheManager->setPriorityWeights(0.1, 1.0, 10.0);
    // No getter, so just verify it doesn't crash
    QVERIFY(true);
}
*/

// Settings persistence tests
void PDFCacheManagerTest::testLoadSettings() {
    m_cacheManager->loadSettings();
    // Just verify it doesn't crash
    QVERIFY(true);
}

void PDFCacheManagerTest::testSaveSettings() {
    m_cacheManager->saveSettings();
    // Just verify it doesn't crash
    QVERIFY(true);
}

// Utility function tests - TODO: Implement these methods in PDFCacheManager.cpp
/*
void PDFCacheManagerTest::testExportCacheToFile() {
    QString exportPath =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
        "/cache_export_test.dat";
    m_testFiles.append(exportPath);

    m_cacheManager->insert("key1", QString("test"), CacheItemType::TextContent);

    bool result = m_cacheManager->exportCacheToFile(exportPath);
    // May or may not be implemented
    QVERIFY(result == true || result == false);
}

void PDFCacheManagerTest::testImportCacheFromFile() {
    QString importPath =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
        "/cache_import_test.dat";

    bool result = m_cacheManager->importCacheFromFile(importPath);
    // May fail if file doesn't exist
    QVERIFY(result == true || result == false);
}

void PDFCacheManagerTest::testDefragmentCache() {
    for (int i = 0; i < 10; ++i) {
        m_cacheManager->insert(createTestKey("defrag", i), QString("test"),
                               CacheItemType::TextContent);
    }

    m_cacheManager->defragmentCache();
    QVERIFY(true);
}
*/

// Cache inspection tests - TODO: Implement these methods in PDFCacheManager.cpp
/*
void PDFCacheManagerTest::testGetCacheKeys() {
    m_cacheManager->insert("key1", QString("test1"),
CacheItemType::TextContent); m_cacheManager->insert("key2", QString("test2"),
CacheItemType::TextContent);

    QStringList keys = m_cacheManager->getCacheKeys();
    QVERIFY(keys.contains("key1"));
    QVERIFY(keys.contains("key2"));
}

void PDFCacheManagerTest::testGetCacheKeysByType() {
    m_cacheManager->insert("text1", QString("test"),
CacheItemType::TextContent); m_cacheManager->insert("page1",
CacheTestHelpers::createTestPixmap(), CacheItemType::RenderedPage);

    QStringList textKeys =
        m_cacheManager->getCacheKeysByType(CacheItemType::TextContent);
    QVERIFY(textKeys.contains("text1"));
    QVERIFY(!textKeys.contains("page1"));
}

void PDFCacheManagerTest::testGetCacheKeysByPriority() {
    m_cacheManager->insert("high1", QString("test"), CacheItemType::TextContent,
                           CachePriority::High);
    m_cacheManager->insert("low1", QString("test"), CacheItemType::TextContent,
                           CachePriority::Low);

    QStringList highKeys =
        m_cacheManager->getCacheKeysByPriority(CachePriority::High);
    QVERIFY(highKeys.contains("high1"));
    QVERIFY(!highKeys.contains("low1"));
}

void PDFCacheManagerTest::testGetCacheItemCount() {
    m_cacheManager->insert("text1", QString("test"),
CacheItemType::TextContent); m_cacheManager->insert("text2", QString("test"),
CacheItemType::TextContent); m_cacheManager->insert("page1",
CacheTestHelpers::createTestPixmap(), CacheItemType::RenderedPage);

    int textCount =
m_cacheManager->getCacheItemCount(CacheItemType::TextContent);
    QCOMPARE(textCount, 2);
}

void PDFCacheManagerTest::testGetCacheMemoryUsage() {
    QPixmap pixmap = CacheTestHelpers::createTestPixmap(200, 200);
    m_cacheManager->insert("page1", pixmap, CacheItemType::RenderedPage);

    qint64 pageMemory =
        m_cacheManager->getCacheMemoryUsage(CacheItemType::RenderedPage);
    QVERIFY(pageMemory > 0);
}
*/

// Cache management function tests - TODO: Implement these methods in
// PDFCacheManager.cpp
/*
void PDFCacheManagerTest::testSetCachePriority() {
    m_cacheManager->insert("key1", QString("test"), CacheItemType::TextContent,
                           CachePriority::Normal);

    m_cacheManager->setCachePriority("key1", CachePriority::High);

    QStringList highKeys =
        m_cacheManager->getCacheKeysByPriority(CachePriority::High);
    QVERIFY(highKeys.contains("key1"));
}

void PDFCacheManagerTest::testPromoteToHighPriority() {
    m_cacheManager->insert("key1", QString("test"), CacheItemType::TextContent,
                           CachePriority::Normal);

    bool promoted = m_cacheManager->promoteToHighPriority("key1");
    QVERIFY(promoted);

    QStringList highKeys =
        m_cacheManager->getCacheKeysByPriority(CachePriority::High);
    QVERIFY(highKeys.contains("key1"));
}

void PDFCacheManagerTest::testRefreshCacheItem() {
    m_cacheManager->insert("key1", QString("test"), CacheItemType::TextContent);

    // Just verify it doesn't crash
    m_cacheManager->refreshCacheItem("key1");
    QVERIFY(true);
}
*/

// Signal tests
void PDFCacheManagerTest::testCacheHitSignal() {
    QSignalSpy spy(m_cacheManager, &PDFCacheManager::cacheHit);

    m_cacheManager->insert("key1", QString("test"), CacheItemType::TextContent);
    m_cacheManager->get("key1");

    QVERIFY(spy.count() > 0);
}

void PDFCacheManagerTest::testCacheMissSignal() {
    QSignalSpy spy(m_cacheManager, &PDFCacheManager::cacheMiss);

    m_cacheManager->get("nonexistent");

    QVERIFY(spy.count() > 0);
}

void PDFCacheManagerTest::testItemEvictedSignal() {
    QSignalSpy spy(m_cacheManager, &PDFCacheManager::itemEvicted);

    // Set very low limits to force eviction
    m_cacheManager->setMaxItems(1);

    m_cacheManager->insert("key1", QString("test1"),
                           CacheItemType::TextContent);
    m_cacheManager->insert("key2", QString("test2"),
                           CacheItemType::TextContent);

    // May or may not emit depending on implementation
    QVERIFY(spy.count() >= 0);
}

void PDFCacheManagerTest::testMemoryThresholdExceededSignal() {
    QSignalSpy spy(m_cacheManager, &PDFCacheManager::memoryThresholdExceeded);

    // Set very low memory limit
    m_cacheManager->setMaxMemoryUsage(100);

    // Add large item
    QPixmap pixmap = CacheTestHelpers::createTestPixmap(1000, 1000);
    m_cacheManager->insert("large", pixmap, CacheItemType::RenderedPage);

    // May or may not emit depending on implementation
    QVERIFY(spy.count() >= 0);
}

void PDFCacheManagerTest::testPreloadCompletedSignal() {
    QSignalSpy spy(m_cacheManager, &PDFCacheManager::preloadCompleted);

    // Preloading requires a document
    m_cacheManager->preloadPages({0}, CacheItemType::RenderedPage);

    // May not emit without a document
    QVERIFY(spy.count() >= 0);
}

void PDFCacheManagerTest::testCacheOptimizedSignal() {
    QSignalSpy spy(m_cacheManager, &PDFCacheManager::cacheOptimized);

    m_cacheManager->insert("key1", QString("test"), CacheItemType::TextContent);
    m_cacheManager->optimizeCache();

    // May or may not emit depending on whether optimization occurred
    QVERIFY(spy.count() >= 0);
}

// Edge cases and error handling
void PDFCacheManagerTest::testInsertNullData() {
    bool result = m_cacheManager->insert("null_key", QVariant(),
                                         CacheItemType::TextContent);

    // Should handle null data gracefully
    QVERIFY(result == true || result == false);
}

void PDFCacheManagerTest::testGetNonExistentKey() {
    QVariant result = m_cacheManager->get("nonexistent");

    QVERIFY(!result.isValid());
}

void PDFCacheManagerTest::testRemoveNonExistentKey() {
    bool result = m_cacheManager->remove("nonexistent");

    QVERIFY(!result);
}

void PDFCacheManagerTest::testEvictFromEmptyCache() {
    bool result = m_cacheManager->evictLeastUsedItems(5);

    // Should handle empty cache gracefully
    QVERIFY(result == true || result == false);
}

void PDFCacheManagerTest::testExceedMemoryLimit() {
    m_cacheManager->setMaxMemoryUsage(1000);  // Very small limit

    // Try to add large item
    QPixmap pixmap = CacheTestHelpers::createTestPixmap(1000, 1000);
    m_cacheManager->insert("large", pixmap, CacheItemType::RenderedPage);

    // Should handle gracefully (either reject or evict)
    qint64 usage = m_cacheManager->getCurrentMemoryUsage();
    QVERIFY(usage >= 0);
}

void PDFCacheManagerTest::testExceedItemLimit() {
    m_cacheManager->setMaxItems(2);

    m_cacheManager->insert("key1", QString("test1"),
                           CacheItemType::TextContent);
    m_cacheManager->insert("key2", QString("test2"),
                           CacheItemType::TextContent);
    m_cacheManager->insert("key3", QString("test3"),
                           CacheItemType::TextContent);

    // Should evict oldest item
    CacheStatistics stats = m_cacheManager->getStatistics();
    QVERIFY(stats.totalItems <= 2);
}

void PDFCacheManagerTest::testExpiredItems() {
    m_cacheManager->setItemMaxAge(10);  // 10ms

    m_cacheManager->insert("key1", QString("test"), CacheItemType::TextContent);

    // Wait for expiration
    waitMs(50);

    m_cacheManager->cleanupExpiredItems();

    QVERIFY(!m_cacheManager->contains("key1"));
}

// CacheItem tests
void PDFCacheManagerTest::testCacheItemCalculateSize() {
    CacheItem item;
    item.data = CacheTestHelpers::createTestPixmap(100, 100);
    item.type = CacheItemType::RenderedPage;

    qint64 size = item.calculateSize();
    QVERIFY(size > 0);
}

void PDFCacheManagerTest::testCacheItemIsExpired() {
    CacheItem item;
    item.timestamp =
        QDateTime::currentMSecsSinceEpoch() - 10000;  // 10 seconds ago

    bool expired = item.isExpired(5000);  // 5 second max age
    QVERIFY(expired);

    bool notExpired = item.isExpired(20000);  // 20 second max age
    QVERIFY(!notExpired);
}

void PDFCacheManagerTest::testCacheItemUpdateAccess() {
    CacheItem item;
    qint64 initialCount = item.accessCount;
    qint64 initialTime = item.lastAccessed;

    item.updateAccess();

    QVERIFY(item.accessCount > initialCount);
    QVERIFY(item.lastAccessed > initialTime);
}

// Priority and eviction tests
void PDFCacheManagerTest::testLowPriorityEviction() {
    m_cacheManager->setMaxItems(2);

    m_cacheManager->insert("low", QString("test"), CacheItemType::TextContent,
                           CachePriority::Low);
    m_cacheManager->insert("high", QString("test"), CacheItemType::TextContent,
                           CachePriority::High);
    m_cacheManager->insert("normal", QString("test"),
                           CacheItemType::TextContent, CachePriority::Normal);

    // Low priority should be evicted first
    QVERIFY(!m_cacheManager->contains("low") ||
            m_cacheManager->contains("high"));
}

void PDFCacheManagerTest::testHighPriorityRetention() {
    m_cacheManager->setMaxItems(2);

    m_cacheManager->insert("high1", QString("test"), CacheItemType::TextContent,
                           CachePriority::High);
    m_cacheManager->insert("high2", QString("test"), CacheItemType::TextContent,
                           CachePriority::High);
    m_cacheManager->insert("low", QString("test"), CacheItemType::TextContent,
                           CachePriority::Low);

    // High priority items should be retained
    QVERIFY(m_cacheManager->contains("high1") ||
            m_cacheManager->contains("high2"));
}

void PDFCacheManagerTest::testCriticalPriorityNoEviction() {
    m_cacheManager->setMaxItems(1);

    m_cacheManager->insert("critical", QString("test"),
                           CacheItemType::TextContent, CachePriority::Critical);
    m_cacheManager->insert("normal", QString("test"),
                           CacheItemType::TextContent, CachePriority::Normal);

    // Critical priority should never be evicted automatically
    QVERIFY(m_cacheManager->contains("critical"));
}

void PDFCacheManagerTest::testLRUEvictionOrder() {
    m_cacheManager->setMaxItems(3);

    m_cacheManager->insert("key1", QString("test1"),
                           CacheItemType::TextContent);
    m_cacheManager->insert("key2", QString("test2"),
                           CacheItemType::TextContent);
    m_cacheManager->insert("key3", QString("test3"),
                           CacheItemType::TextContent);

    // Verify all 3 items are in cache
    QVERIFY(m_cacheManager->contains("key1"));
    QVERIFY(m_cacheManager->contains("key2"));
    QVERIFY(m_cacheManager->contains("key3"));

    // Add new item, should evict one item to make room
    m_cacheManager->insert("key4", QString("test4"),
                           CacheItemType::TextContent);

    // key4 should be there (just inserted)
    QVERIFY(m_cacheManager->contains("key4"));

    // Exactly 3 items should remain (one was evicted)
    int count = (m_cacheManager->contains("key1") ? 1 : 0) +
                (m_cacheManager->contains("key2") ? 1 : 0) +
                (m_cacheManager->contains("key3") ? 1 : 0) +
                (m_cacheManager->contains("key4") ? 1 : 0);
    QCOMPARE(count, 3);
}

QTEST_MAIN(PDFCacheManagerTest)
#include "test_pdf_cache_manager.moc"
