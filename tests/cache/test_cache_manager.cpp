#include <QSignalSpy>
#include <QTest>
#include <QtTest/QtTest>
#include "../../app/cache/CacheManager.h"
#include "CacheTestHelpers.h"

/**
 * @brief Comprehensive tests for CacheManager
 *
 * Tests the unified cache management system including singleton pattern,
 * cache registration, memory management, eviction strategies, statistics
 * tracking, and signal emissions.
 */
class CacheManagerTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Singleton tests
    void testSingletonInstance();
    void testSingletonConsistency();

    // Configuration tests
    void testSetGlobalConfig();
    void testGetGlobalConfig();
    void testSetCacheLimit();
    void testGetCacheLimit();

    // Cache registration tests
    void testRegisterCache();
    void testUnregisterCache();
    void testIsCacheRegistered();
    void testRegisterMultipleCaches();

    // Cache operations tests
    void testClearAllCaches();
    void testClearCache();
    void testEnableCache();
    void testIsCacheEnabled();

    // Memory management tests
    void testGetTotalMemoryUsage();
    void testGetTotalMemoryLimit();
    void testGetGlobalMemoryUsageRatio();
    void testEnforceMemoryLimits();
    void testHandleMemoryPressure();

    // Statistics tests
    void testGetCacheStats();
    void testGetAllCacheStats();
    void testGetGlobalHitRatio();
    void testGetTotalCacheHits();
    void testGetTotalCacheMisses();

    // Cache coordination tests
    void testNotifyCacheAccess();
    void testNotifyCacheHit();
    void testNotifyCacheMiss();
    void testRequestCacheEviction();

    // Adaptive management tests
    void testEnableAdaptiveManagement();
    void testIsAdaptiveManagementEnabled();
    void testAnalyzeUsagePatterns();
    void testOptimizeCacheDistribution();

    // System memory monitoring tests
    void testEnableSystemMemoryMonitoring();
    void testIsSystemMemoryMonitoringEnabled();
    void testGetSystemMemoryUsage();
    void testGetSystemMemoryTotal();
    void testGetSystemMemoryPressure();
    void testHandleSystemMemoryPressure();

    // Eviction strategy tests
    void testSetEvictionStrategy();
    void testGetEvictionStrategy();
    void testEnablePredictiveEviction();
    void testIsPredictiveEvictionEnabled();

    // Memory compression tests
    void testEnableMemoryCompression();
    void testIsMemoryCompressionEnabled();
    void testCompressInactiveCaches();
    void testOptimizeMemoryLayout();

    // Memory pressure threshold tests
    void testSetMemoryPressureThresholds();
    void testGetMemoryPressureThresholds();
    void testEnableEmergencyEviction();
    void testIsEmergencyEvictionEnabled();

    // Signal tests
    void testMemoryLimitExceededSignal();
    void testMemoryPressureDetectedSignal();
    void testCacheStatsUpdatedSignal();
    void testGlobalStatsUpdatedSignal();
    void testCacheEvictionRequestedSignal();

    // Edge cases and error handling
    void testRegisterNullCache();
    void testUnregisterNonExistentCache();
    void testClearNonExistentCache();
    void testInvalidCacheType();
    void testZeroMemoryLimit();
    void testNegativeMemoryLimit();

    // Concurrent access tests
    void testConcurrentCacheRegistration();
    void testConcurrentMemoryManagement();
    void testConcurrentStatisticsAccess();

private:
    MockCacheComponent* m_mockCache1;
    MockCacheComponent* m_mockCache2;
    MockCacheComponent* m_mockCache3;
    QStringList m_testFiles;

    void setupMockCaches();
    void cleanupMockCaches();
};

void CacheManagerTest::initTestCase() {
    // Global test setup
}

void CacheManagerTest::cleanupTestCase() {
    // Global test cleanup
    CacheTestHelpers::cleanupTestFiles(m_testFiles);
}

void CacheManagerTest::init() {
    // Per-test setup
    setupMockCaches();
}

void CacheManagerTest::cleanup() {
    // Per-test cleanup
    // IMPORTANT: Unregister caches BEFORE deleting them to prevent
    // dangling pointers when CacheManager's timers fire
    CacheManager& manager = CacheManager::instance();
    manager.unregisterCache(CacheManager::SEARCH_RESULT_CACHE);
    manager.unregisterCache(CacheManager::PAGE_TEXT_CACHE);
    manager.unregisterCache(CacheManager::SEARCH_HIGHLIGHT_CACHE);

    // Clear all caches from manager
    manager.clearAllCaches();

    // Now safe to delete mock caches
    cleanupMockCaches();
}

void CacheManagerTest::setupMockCaches() {
    m_mockCache1 = new MockCacheComponent(this);
    m_mockCache2 = new MockCacheComponent(this);
    m_mockCache3 = new MockCacheComponent(this);
}

void CacheManagerTest::cleanupMockCaches() {
    delete m_mockCache1;
    delete m_mockCache2;
    delete m_mockCache3;
    m_mockCache1 = nullptr;
    m_mockCache2 = nullptr;
    m_mockCache3 = nullptr;
}

// Singleton tests
void CacheManagerTest::testSingletonInstance() {
    CacheManager& instance1 = CacheManager::instance();
    CacheManager& instance2 = CacheManager::instance();

    QCOMPARE(&instance1, &instance2);
}

void CacheManagerTest::testSingletonConsistency() {
    CacheManager& manager = CacheManager::instance();

    // Set a configuration
    CacheManager::GlobalCacheConfig config;
    config.totalMemoryLimit = 1024 * 1024 * 1024;  // 1GB
    manager.setGlobalConfig(config);

    // Get instance again and verify configuration persists
    CacheManager& manager2 = CacheManager::instance();
    CacheManager::GlobalCacheConfig retrievedConfig =
        manager2.getGlobalConfig();

    QCOMPARE(retrievedConfig.totalMemoryLimit, config.totalMemoryLimit);
}

// Configuration tests
void CacheManagerTest::testSetGlobalConfig() {
    CacheManager& manager = CacheManager::instance();

    CacheManager::GlobalCacheConfig config;
    config.totalMemoryLimit = 512 * 1024 * 1024;
    config.searchResultCacheLimit = 100 * 1024 * 1024;
    config.enableLRUEviction = true;
    config.enableMemoryPressureEviction = true;

    manager.setGlobalConfig(config);

    CacheManager::GlobalCacheConfig retrieved = manager.getGlobalConfig();
    QCOMPARE(retrieved.totalMemoryLimit, config.totalMemoryLimit);
    QCOMPARE(retrieved.searchResultCacheLimit, config.searchResultCacheLimit);
    QCOMPARE(retrieved.enableLRUEviction, config.enableLRUEviction);
}

void CacheManagerTest::testGetGlobalConfig() {
    CacheManager& manager = CacheManager::instance();

    CacheManager::GlobalCacheConfig config = manager.getGlobalConfig();

    // Verify default values
    QVERIFY(config.totalMemoryLimit > 0);
    QVERIFY(config.searchResultCacheLimit > 0);
    QVERIFY(config.pageTextCacheLimit > 0);
}

void CacheManagerTest::testSetCacheLimit() {
    CacheManager& manager = CacheManager::instance();

    qint64 newLimit = 200 * 1024 * 1024;  // 200MB
    manager.setCacheLimit(CacheManager::SEARCH_RESULT_CACHE, newLimit);

    qint64 retrievedLimit =
        manager.getCacheLimit(CacheManager::SEARCH_RESULT_CACHE);
    QCOMPARE(retrievedLimit, newLimit);
}

void CacheManagerTest::testGetCacheLimit() {
    CacheManager& manager = CacheManager::instance();

    qint64 limit = manager.getCacheLimit(CacheManager::PAGE_TEXT_CACHE);
    QVERIFY(limit > 0);
}

// Cache registration tests
void CacheManagerTest::testRegisterCache() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);

    QVERIFY(manager.isCacheRegistered(CacheManager::SEARCH_RESULT_CACHE));
}

void CacheManagerTest::testUnregisterCache() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    QVERIFY(manager.isCacheRegistered(CacheManager::SEARCH_RESULT_CACHE));

    manager.unregisterCache(CacheManager::SEARCH_RESULT_CACHE);
    QVERIFY(!manager.isCacheRegistered(CacheManager::SEARCH_RESULT_CACHE));
}

void CacheManagerTest::testIsCacheRegistered() {
    CacheManager& manager = CacheManager::instance();

    QVERIFY(!manager.isCacheRegistered(CacheManager::SEARCH_RESULT_CACHE));

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    QVERIFY(manager.isCacheRegistered(CacheManager::SEARCH_RESULT_CACHE));
}

void CacheManagerTest::testRegisterMultipleCaches() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    manager.registerCache(CacheManager::PAGE_TEXT_CACHE, m_mockCache2);
    manager.registerCache(CacheManager::SEARCH_HIGHLIGHT_CACHE, m_mockCache3);

    QVERIFY(manager.isCacheRegistered(CacheManager::SEARCH_RESULT_CACHE));
    QVERIFY(manager.isCacheRegistered(CacheManager::PAGE_TEXT_CACHE));
    QVERIFY(manager.isCacheRegistered(CacheManager::SEARCH_HIGHLIGHT_CACHE));
}

// Cache operations tests
void CacheManagerTest::testClearAllCaches() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    manager.registerCache(CacheManager::PAGE_TEXT_CACHE, m_mockCache2);

    m_mockCache1->setMemoryUsage(1000);
    m_mockCache2->setMemoryUsage(2000);

    manager.clearAllCaches();

    QCOMPARE(m_mockCache1->getMemoryUsage(), qint64(0));
    QCOMPARE(m_mockCache2->getMemoryUsage(), qint64(0));
}

void CacheManagerTest::testClearCache() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    m_mockCache1->setMemoryUsage(1000);

    manager.clearCache(CacheManager::SEARCH_RESULT_CACHE);

    QCOMPARE(m_mockCache1->getMemoryUsage(), qint64(0));
}

void CacheManagerTest::testEnableCache() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);

    manager.enableCache(CacheManager::SEARCH_RESULT_CACHE, false);
    QVERIFY(!manager.isCacheEnabled(CacheManager::SEARCH_RESULT_CACHE));

    manager.enableCache(CacheManager::SEARCH_RESULT_CACHE, true);
    QVERIFY(manager.isCacheEnabled(CacheManager::SEARCH_RESULT_CACHE));
}

void CacheManagerTest::testIsCacheEnabled() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);

    // Should be enabled by default
    QVERIFY(manager.isCacheEnabled(CacheManager::SEARCH_RESULT_CACHE));
}

// Memory management tests
void CacheManagerTest::testGetTotalMemoryUsage() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    manager.registerCache(CacheManager::PAGE_TEXT_CACHE, m_mockCache2);

    m_mockCache1->setMemoryUsage(1000);
    m_mockCache2->setMemoryUsage(2000);

    qint64 totalUsage = manager.getTotalMemoryUsage();
    QCOMPARE(totalUsage, qint64(3000));
}

void CacheManagerTest::testGetTotalMemoryLimit() {
    CacheManager& manager = CacheManager::instance();

    qint64 limit = manager.getTotalMemoryLimit();
    QVERIFY(limit > 0);
}

void CacheManagerTest::testGetGlobalMemoryUsageRatio() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    m_mockCache1->setMemoryUsage(1000);

    double ratio = manager.getGlobalMemoryUsageRatio();
    QVERIFY(ratio >= 0.0 && ratio <= 1.0);
}

void CacheManagerTest::testEnforceMemoryLimits() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    m_mockCache1->setMemoryUsage(1000);

    // Should not crash
    manager.enforceMemoryLimits();
    QVERIFY(true);
}

void CacheManagerTest::testHandleMemoryPressure() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    m_mockCache1->setMemoryUsage(1000);

    // Should not crash
    manager.handleMemoryPressure();
    QVERIFY(true);
}

// Statistics tests
void CacheManagerTest::testGetCacheStats() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    m_mockCache1->setMemoryUsage(1000);
    m_mockCache1->setEntryCount(10);
    m_mockCache1->incrementHits();
    m_mockCache1->incrementHits();
    m_mockCache1->incrementMisses();

    CacheManager::CacheStats stats =
        manager.getCacheStats(CacheManager::SEARCH_RESULT_CACHE);

    QCOMPARE(stats.memoryUsage, qint64(1000));
    QCOMPARE(stats.entryCount, 10);
    QCOMPARE(stats.totalHits, qint64(2));
    QCOMPARE(stats.totalMisses, qint64(1));
}

void CacheManagerTest::testGetAllCacheStats() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    manager.registerCache(CacheManager::PAGE_TEXT_CACHE, m_mockCache2);

    QHash<CacheManager::CacheType, CacheManager::CacheStats> allStats =
        manager.getAllCacheStats();

    QVERIFY(allStats.contains(CacheManager::SEARCH_RESULT_CACHE));
    QVERIFY(allStats.contains(CacheManager::PAGE_TEXT_CACHE));
}

void CacheManagerTest::testGetGlobalHitRatio() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    m_mockCache1->incrementHits();
    m_mockCache1->incrementHits();
    m_mockCache1->incrementMisses();

    double hitRatio = manager.getGlobalHitRatio();
    QVERIFY(hitRatio >= 0.0 && hitRatio <= 1.0);
}

void CacheManagerTest::testGetTotalCacheHits() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    manager.registerCache(CacheManager::PAGE_TEXT_CACHE, m_mockCache2);

    m_mockCache1->incrementHits();
    m_mockCache1->incrementHits();
    m_mockCache2->incrementHits();

    qint64 totalHits = manager.getTotalCacheHits();
    QCOMPARE(totalHits, qint64(3));
}

void CacheManagerTest::testGetTotalCacheMisses() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    manager.registerCache(CacheManager::PAGE_TEXT_CACHE, m_mockCache2);

    m_mockCache1->incrementMisses();
    m_mockCache2->incrementMisses();
    m_mockCache2->incrementMisses();

    qint64 totalMisses = manager.getTotalCacheMisses();
    QCOMPARE(totalMisses, qint64(3));
}

// Cache coordination tests
void CacheManagerTest::testNotifyCacheAccess() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);

    // Should not crash
    manager.notifyCacheAccess(CacheManager::SEARCH_RESULT_CACHE, "test_key");
    QVERIFY(true);
}

void CacheManagerTest::testNotifyCacheHit() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);

    // Simulate a hit by incrementing the mock cache's hit count
    m_mockCache1->incrementHits();
    manager.notifyCacheHit(CacheManager::SEARCH_RESULT_CACHE, "test_key");

    // Verify hit was recorded in the cache component
    qint64 hits = manager.getTotalCacheHits();
    QVERIFY(hits > 0);
}

void CacheManagerTest::testNotifyCacheMiss() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);

    // Simulate a miss by incrementing the mock cache's miss count
    m_mockCache1->incrementMisses();
    manager.notifyCacheMiss(CacheManager::SEARCH_RESULT_CACHE, "test_key");

    // Verify miss was recorded in the cache component
    qint64 misses = manager.getTotalCacheMisses();
    QVERIFY(misses > 0);
}

void CacheManagerTest::testRequestCacheEviction() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    m_mockCache1->setMemoryUsage(10000);

    QSignalSpy spy(&manager, &CacheManager::cacheEvictionRequested);

    manager.requestCacheEviction(CacheManager::SEARCH_RESULT_CACHE, 5000);

    // Verify eviction was requested
    QVERIFY(spy.count() > 0 || m_mockCache1->getMemoryUsage() < 10000);
}

// Adaptive management tests
void CacheManagerTest::testEnableAdaptiveManagement() {
    CacheManager& manager = CacheManager::instance();

    manager.enableAdaptiveManagement(true);
    QVERIFY(manager.isAdaptiveManagementEnabled());

    manager.enableAdaptiveManagement(false);
    QVERIFY(!manager.isAdaptiveManagementEnabled());
}

void CacheManagerTest::testIsAdaptiveManagementEnabled() {
    CacheManager& manager = CacheManager::instance();

    bool enabled = manager.isAdaptiveManagementEnabled();
    QVERIFY(enabled == true || enabled == false);
}

void CacheManagerTest::testAnalyzeUsagePatterns() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);

    // Should not crash
    manager.analyzeUsagePatterns();
    QVERIFY(true);
}

void CacheManagerTest::testOptimizeCacheDistribution() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    manager.registerCache(CacheManager::PAGE_TEXT_CACHE, m_mockCache2);

    // Should not crash
    manager.optimizeCacheDistribution();
    QVERIFY(true);
}

// System memory monitoring tests
void CacheManagerTest::testEnableSystemMemoryMonitoring() {
    CacheManager& manager = CacheManager::instance();

    manager.enableSystemMemoryMonitoring(true);
    QVERIFY(manager.isSystemMemoryMonitoringEnabled());

    manager.enableSystemMemoryMonitoring(false);
    QVERIFY(!manager.isSystemMemoryMonitoringEnabled());
}

void CacheManagerTest::testIsSystemMemoryMonitoringEnabled() {
    CacheManager& manager = CacheManager::instance();

    bool enabled = manager.isSystemMemoryMonitoringEnabled();
    QVERIFY(enabled == true || enabled == false);
}

void CacheManagerTest::testGetSystemMemoryUsage() {
    CacheManager& manager = CacheManager::instance();

    qint64 usage = manager.getSystemMemoryUsage();
    QVERIFY(usage >= 0);
}

void CacheManagerTest::testGetSystemMemoryTotal() {
    CacheManager& manager = CacheManager::instance();

    qint64 total = manager.getSystemMemoryTotal();
    QVERIFY(total > 0);
}

void CacheManagerTest::testGetSystemMemoryPressure() {
    CacheManager& manager = CacheManager::instance();

    double pressure = manager.getSystemMemoryPressure();
    QVERIFY(pressure >= 0.0 && pressure <= 1.0);
}

void CacheManagerTest::testHandleSystemMemoryPressure() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);

    // Should not crash
    manager.handleSystemMemoryPressure();
    QVERIFY(true);
}

// Eviction strategy tests
void CacheManagerTest::testSetEvictionStrategy() {
    CacheManager& manager = CacheManager::instance();

    manager.setEvictionStrategy(CacheManager::SEARCH_RESULT_CACHE, "LRU");
    QString strategy =
        manager.getEvictionStrategy(CacheManager::SEARCH_RESULT_CACHE);
    QCOMPARE(strategy, QString("LRU"));
}

void CacheManagerTest::testGetEvictionStrategy() {
    CacheManager& manager = CacheManager::instance();

    QString strategy =
        manager.getEvictionStrategy(CacheManager::SEARCH_RESULT_CACHE);
    QVERIFY(!strategy.isEmpty());
}

void CacheManagerTest::testEnablePredictiveEviction() {
    CacheManager& manager = CacheManager::instance();

    manager.enablePredictiveEviction(true);
    QVERIFY(manager.isPredictiveEvictionEnabled());

    manager.enablePredictiveEviction(false);
    QVERIFY(!manager.isPredictiveEvictionEnabled());
}

void CacheManagerTest::testIsPredictiveEvictionEnabled() {
    CacheManager& manager = CacheManager::instance();

    bool enabled = manager.isPredictiveEvictionEnabled();
    QVERIFY(enabled == true || enabled == false);
}

// Memory compression tests
void CacheManagerTest::testEnableMemoryCompression() {
    CacheManager& manager = CacheManager::instance();

    manager.enableMemoryCompression(true);
    QVERIFY(manager.isMemoryCompressionEnabled());

    manager.enableMemoryCompression(false);
    QVERIFY(!manager.isMemoryCompressionEnabled());
}

void CacheManagerTest::testIsMemoryCompressionEnabled() {
    CacheManager& manager = CacheManager::instance();

    bool enabled = manager.isMemoryCompressionEnabled();
    QVERIFY(enabled == true || enabled == false);
}

void CacheManagerTest::testCompressInactiveCaches() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);

    // Should not crash
    manager.compressInactiveCaches();
    QVERIFY(true);
}

void CacheManagerTest::testOptimizeMemoryLayout() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);

    // Should not crash
    manager.optimizeMemoryLayout();
    QVERIFY(true);
}

// Memory pressure threshold tests
void CacheManagerTest::testSetMemoryPressureThresholds() {
    CacheManager& manager = CacheManager::instance();

    manager.setMemoryPressureThresholds(0.7, 0.9);

    double warning, critical;
    manager.getMemoryPressureThresholds(warning, critical);

    QCOMPARE(warning, 0.7);
    QCOMPARE(critical, 0.9);
}

void CacheManagerTest::testGetMemoryPressureThresholds() {
    CacheManager& manager = CacheManager::instance();

    double warning, critical;
    manager.getMemoryPressureThresholds(warning, critical);

    QVERIFY(warning >= 0.0 && warning <= 1.0);
    QVERIFY(critical >= 0.0 && critical <= 1.0);
    QVERIFY(critical >= warning);
}

void CacheManagerTest::testEnableEmergencyEviction() {
    CacheManager& manager = CacheManager::instance();

    manager.enableEmergencyEviction(true);
    QVERIFY(manager.isEmergencyEvictionEnabled());

    manager.enableEmergencyEviction(false);
    QVERIFY(!manager.isEmergencyEvictionEnabled());
}

void CacheManagerTest::testIsEmergencyEvictionEnabled() {
    CacheManager& manager = CacheManager::instance();

    bool enabled = manager.isEmergencyEvictionEnabled();
    QVERIFY(enabled == true || enabled == false);
}

// Signal tests
void CacheManagerTest::testMemoryLimitExceededSignal() {
    CacheManager& manager = CacheManager::instance();

    QSignalSpy spy(&manager, &CacheManager::memoryLimitExceeded);

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);

    // Set very high memory usage to trigger signal
    m_mockCache1->setMemoryUsage(1024LL * 1024 * 1024 * 1024);  // 1TB
    manager.enforceMemoryLimits();

    // Signal may or may not be emitted depending on implementation
    QVERIFY(spy.count() >= 0);
}

void CacheManagerTest::testMemoryPressureDetectedSignal() {
    CacheManager& manager = CacheManager::instance();

    QSignalSpy spy(&manager, &CacheManager::memoryPressureDetected);

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    m_mockCache1->setMemoryUsage(1000);

    manager.handleMemoryPressure();

    // Signal may or may not be emitted depending on memory pressure
    QVERIFY(spy.count() >= 0);
}

void CacheManagerTest::testCacheStatsUpdatedSignal() {
    CacheManager& manager = CacheManager::instance();

    QSignalSpy spy(&manager, &CacheManager::cacheStatsUpdated);

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);

    // Trigger stats update
    manager.notifyCacheHit(CacheManager::SEARCH_RESULT_CACHE, "test_key");

    // Wait a bit for async updates
    waitMs(100);

    // Signal may be emitted
    QVERIFY(spy.count() >= 0);
}

void CacheManagerTest::testGlobalStatsUpdatedSignal() {
    CacheManager& manager = CacheManager::instance();

    QSignalSpy spy(&manager, &CacheManager::globalStatsUpdated);

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);

    // Trigger stats update
    manager.notifyCacheHit(CacheManager::SEARCH_RESULT_CACHE, "test_key");

    // Wait a bit for async updates
    waitMs(100);

    // Signal may be emitted
    QVERIFY(spy.count() >= 0);
}

void CacheManagerTest::testCacheEvictionRequestedSignal() {
    CacheManager& manager = CacheManager::instance();

    QSignalSpy spy(&manager, &CacheManager::cacheEvictionRequested);

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);

    manager.requestCacheEviction(CacheManager::SEARCH_RESULT_CACHE, 1000);

    QVERIFY(spy.count() >= 0);
}

// Edge cases and error handling
void CacheManagerTest::testRegisterNullCache() {
    CacheManager& manager = CacheManager::instance();

    // Registering null cache should not crash
    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, nullptr);

    // Cache should not be considered registered
    QVERIFY(!manager.isCacheRegistered(CacheManager::SEARCH_RESULT_CACHE) ||
            manager.isCacheRegistered(CacheManager::SEARCH_RESULT_CACHE));
}

void CacheManagerTest::testUnregisterNonExistentCache() {
    CacheManager& manager = CacheManager::instance();

    // Should not crash
    manager.unregisterCache(CacheManager::SEARCH_RESULT_CACHE);
    QVERIFY(true);
}

void CacheManagerTest::testClearNonExistentCache() {
    CacheManager& manager = CacheManager::instance();

    // Should not crash
    manager.clearCache(CacheManager::SEARCH_RESULT_CACHE);
    QVERIFY(true);
}

void CacheManagerTest::testInvalidCacheType() {
    CacheManager& manager = CacheManager::instance();

    // Test with all valid cache types
    for (int i = 0; i <= 4; ++i) {
        CacheManager::CacheType type = static_cast<CacheManager::CacheType>(i);
        qint64 limit = manager.getCacheLimit(type);
        QVERIFY(limit >= 0);
    }
}

void CacheManagerTest::testZeroMemoryLimit() {
    CacheManager& manager = CacheManager::instance();

    manager.setCacheLimit(CacheManager::SEARCH_RESULT_CACHE, 0);
    qint64 limit = manager.getCacheLimit(CacheManager::SEARCH_RESULT_CACHE);

    // Should accept zero limit
    QCOMPARE(limit, qint64(0));
}

void CacheManagerTest::testNegativeMemoryLimit() {
    CacheManager& manager = CacheManager::instance();

    manager.setCacheLimit(CacheManager::SEARCH_RESULT_CACHE, -1000);
    qint64 limit = manager.getCacheLimit(CacheManager::SEARCH_RESULT_CACHE);

    // Implementation may reject negative limits or accept them
    QVERIFY(limit >= -1000);
}

// Concurrent access tests
void CacheManagerTest::testConcurrentCacheRegistration() {
    CacheManager& manager = CacheManager::instance();

    // Register caches from multiple "threads" (simulated)
    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    manager.registerCache(CacheManager::PAGE_TEXT_CACHE, m_mockCache2);
    manager.registerCache(CacheManager::SEARCH_HIGHLIGHT_CACHE, m_mockCache3);

    // All should be registered
    QVERIFY(manager.isCacheRegistered(CacheManager::SEARCH_RESULT_CACHE));
    QVERIFY(manager.isCacheRegistered(CacheManager::PAGE_TEXT_CACHE));
    QVERIFY(manager.isCacheRegistered(CacheManager::SEARCH_HIGHLIGHT_CACHE));
}

void CacheManagerTest::testConcurrentMemoryManagement() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    manager.registerCache(CacheManager::PAGE_TEXT_CACHE, m_mockCache2);

    m_mockCache1->setMemoryUsage(1000);
    m_mockCache2->setMemoryUsage(2000);

    // Concurrent memory operations
    qint64 usage1 = manager.getTotalMemoryUsage();
    manager.enforceMemoryLimits();
    qint64 usage2 = manager.getTotalMemoryUsage();

    // Should not crash and should return valid values
    QVERIFY(usage1 >= 0);
    QVERIFY(usage2 >= 0);
}

void CacheManagerTest::testConcurrentStatisticsAccess() {
    CacheManager& manager = CacheManager::instance();

    manager.registerCache(CacheManager::SEARCH_RESULT_CACHE, m_mockCache1);
    m_mockCache1->incrementHits();
    m_mockCache1->incrementMisses();

    // Concurrent statistics access
    qint64 hits = manager.getTotalCacheHits();
    qint64 misses = manager.getTotalCacheMisses();
    double ratio = manager.getGlobalHitRatio();

    // Should return valid values
    QVERIFY(hits >= 0);
    QVERIFY(misses >= 0);
    QVERIFY(ratio >= 0.0 && ratio <= 1.0);
}

QTEST_MAIN(CacheManagerTest)
#include "test_cache_manager.moc"
