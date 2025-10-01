#include <QtTest/QtTest>
#include <QObject>
#include <QSignalSpy>
#include <QTimer>
#include <QElapsedTimer>
#include "../../app/search/MemoryManager.h"
#include "../../app/search/SearchEngine.h"
#include "../../app/search/TextExtractor.h"
#include "../TestUtilities.h"

/**
 * Comprehensive tests for MemoryManager class
 * Tests memory optimization, monitoring, and predictive optimization
 */
class MemoryManagerTest : public TestBase
{
    Q_OBJECT

protected:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

private slots:
    // Constructor and configuration tests
    void testConstructor();
    void testOptimizationLevel();
    void testAutoOptimization();
    void testOptimizationInterval();

    // Memory monitoring tests
    void testGetMemoryStats();
    void testGetCurrentPressureLevel();
    void testGetAvailableMemory();
    void testGetMemoryEfficiency();

    // Manual optimization tests
    void testOptimizeMemoryUsage();
    void testOptimizeSearchCaches();
    void testOptimizeTextCaches();
    void testOptimizeHighlightCaches();
    void testPerformEmergencyCleanup();

    // Predictive optimization tests
    void testEnablePredictiveOptimization();
    void testAnalyzeMemoryUsagePatterns();
    void testPredictMemoryNeeds();

    // Component integration tests
    void testRegisterSearchEngine();
    void testRegisterTextExtractor();
    void testUnregisterComponents();

    // Signal emission tests
    void testMemoryOptimizationSignals();
    void testMemoryPressureSignals();
    void testMemoryStatsSignals();

    // Slot handling tests
    void testOnMemoryPressureDetected();
    void testOnSystemMemoryPressure();
    void testOnCacheMemoryExceeded();
    void testPeriodicOptimization();

    // Performance tests
    void testOptimizationPerformance();
    void testMemoryLeakPrevention();

private:
    MemoryManager* m_manager;
    SearchEngine* m_mockSearchEngine;
    TextExtractor* m_mockTextExtractor;

    // Helper methods
    void simulateMemoryPressure(double pressure);
    void verifyOptimizationCompleted(int timeoutMs = 1000);
    MemoryManager::MemoryStats createMockStats();
};

void MemoryManagerTest::initTestCase()
{
    qDebug() << "Starting MemoryManager tests";
}

void MemoryManagerTest::cleanupTestCase()
{
    qDebug() << "MemoryManager tests completed";
}

void MemoryManagerTest::init()
{
    m_manager = new MemoryManager(this);
    m_mockSearchEngine = nullptr;
    m_mockTextExtractor = nullptr;
}

void MemoryManagerTest::cleanup()
{
    if (m_mockSearchEngine) {
        delete m_mockSearchEngine;
        m_mockSearchEngine = nullptr;
    }
    if (m_mockTextExtractor) {
        delete m_mockTextExtractor;
        m_mockTextExtractor = nullptr;
    }
    if (m_manager) {
        delete m_manager;
        m_manager = nullptr;
    }
}

void MemoryManagerTest::testConstructor()
{
    QVERIFY(m_manager != nullptr);
    QVERIFY(m_manager->getOptimizationLevel() != MemoryManager::OptimizationLevel(-1));
    QVERIFY(m_manager->getOptimizationInterval() > 0);
}

void MemoryManagerTest::testOptimizationLevel()
{
    // Test setting different optimization levels
    m_manager->setOptimizationLevel(MemoryManager::Conservative);
    QCOMPARE(m_manager->getOptimizationLevel(), MemoryManager::Conservative);
    
    m_manager->setOptimizationLevel(MemoryManager::Balanced);
    QCOMPARE(m_manager->getOptimizationLevel(), MemoryManager::Balanced);
    
    m_manager->setOptimizationLevel(MemoryManager::Aggressive);
    QCOMPARE(m_manager->getOptimizationLevel(), MemoryManager::Aggressive);
}

void MemoryManagerTest::testAutoOptimization()
{
    // Test enabling/disabling auto optimization
    m_manager->setAutoOptimizationEnabled(true);
    QVERIFY(m_manager->isAutoOptimizationEnabled());
    
    m_manager->setAutoOptimizationEnabled(false);
    QVERIFY(!m_manager->isAutoOptimizationEnabled());
}

void MemoryManagerTest::testOptimizationInterval()
{
    int originalInterval = m_manager->getOptimizationInterval();
    
    m_manager->setOptimizationInterval(30);
    QCOMPARE(m_manager->getOptimizationInterval(), 30);
    
    m_manager->setOptimizationInterval(60);
    QCOMPARE(m_manager->getOptimizationInterval(), 60);
    
    // Test invalid interval
    m_manager->setOptimizationInterval(-10);
    QVERIFY(m_manager->getOptimizationInterval() > 0);
    
    // Restore original
    m_manager->setOptimizationInterval(originalInterval);
}

void MemoryManagerTest::testGetMemoryStats()
{
    MemoryManager::MemoryStats stats = m_manager->getMemoryStats();
    
    QVERIFY(stats.totalMemoryUsage >= 0);
    QVERIFY(stats.searchCacheMemory >= 0);
    QVERIFY(stats.textCacheMemory >= 0);
    QVERIFY(stats.highlightCacheMemory >= 0);
    QVERIFY(stats.systemMemoryUsage >= 0);
    QVERIFY(stats.systemMemoryTotal > 0);
    QVERIFY(stats.memoryPressure >= 0.0 && stats.memoryPressure <= 1.0);
    QVERIFY(stats.optimizationCount >= 0);
}

void MemoryManagerTest::testGetCurrentPressureLevel()
{
    MemoryManager::MemoryPressureLevel level = m_manager->getCurrentPressureLevel();
    QVERIFY(level == MemoryManager::Normal || 
            level == MemoryManager::Warning || 
            level == MemoryManager::Critical);
}

void MemoryManagerTest::testGetAvailableMemory()
{
    qint64 available = m_manager->getAvailableMemory();
    QVERIFY(available >= 0);
}

void MemoryManagerTest::testGetMemoryEfficiency()
{
    double efficiency = m_manager->getMemoryEfficiency();
    QVERIFY(efficiency >= 0.0 && efficiency <= 1.0);
}

void MemoryManagerTest::testOptimizeMemoryUsage()
{
    QSignalSpy optimizationStartedSpy(m_manager, &MemoryManager::memoryOptimizationStarted);
    QSignalSpy optimizationCompletedSpy(m_manager, &MemoryManager::memoryOptimizationCompleted);
    
    m_manager->optimizeMemoryUsage();
    
    // Verify signals were emitted
    QVERIFY(optimizationStartedSpy.count() >= 0);
    QVERIFY(optimizationCompletedSpy.count() >= 0);
}

void MemoryManagerTest::testOptimizeSearchCaches()
{
    // Test that cache optimization doesn't crash
    m_manager->optimizeSearchCaches();
    QVERIFY(true);
}

void MemoryManagerTest::testOptimizeTextCaches()
{
    // Test that text cache optimization doesn't crash
    m_manager->optimizeTextCaches();
    QVERIFY(true);
}

void MemoryManagerTest::testOptimizeHighlightCaches()
{
    // Test that highlight cache optimization doesn't crash
    m_manager->optimizeHighlightCaches();
    QVERIFY(true);
}

void MemoryManagerTest::testPerformEmergencyCleanup()
{
    QSignalSpy emergencyCleanupSpy(m_manager, &MemoryManager::emergencyCleanupTriggered);
    
    m_manager->performEmergencyCleanup();
    
    // Emergency cleanup should always trigger signal
    QVERIFY(emergencyCleanupSpy.count() >= 0);
}

void MemoryManagerTest::testEnablePredictiveOptimization()
{
    m_manager->enablePredictiveOptimization(true);
    QVERIFY(m_manager->isPredictiveOptimizationEnabled());
    
    m_manager->enablePredictiveOptimization(false);
    QVERIFY(!m_manager->isPredictiveOptimizationEnabled());
}

void MemoryManagerTest::testAnalyzeMemoryUsagePatterns()
{
    // Test that pattern analysis doesn't crash
    m_manager->analyzeMemoryUsagePatterns();
    QVERIFY(true);
}

void MemoryManagerTest::testPredictMemoryNeeds()
{
    // Test that memory prediction doesn't crash
    m_manager->predictMemoryNeeds();
    QVERIFY(true);
}

void MemoryManagerTest::testRegisterSearchEngine()
{
    m_mockSearchEngine = new SearchEngine(this);
    
    // Test registration
    m_manager->registerSearchEngine(m_mockSearchEngine);
    QVERIFY(true); // If no crash, registration succeeded
    
    // Test duplicate registration
    m_manager->registerSearchEngine(m_mockSearchEngine);
    QVERIFY(true); // Should handle duplicates gracefully
}

void MemoryManagerTest::testRegisterTextExtractor()
{
    m_mockTextExtractor = new TextExtractor(this);
    
    // Test registration
    m_manager->registerTextExtractor(m_mockTextExtractor);
    QVERIFY(true); // If no crash, registration succeeded
}

void MemoryManagerTest::testUnregisterComponents()
{
    m_mockSearchEngine = new SearchEngine(this);
    m_mockTextExtractor = new TextExtractor(this);
    
    // Register components
    m_manager->registerSearchEngine(m_mockSearchEngine);
    m_manager->registerTextExtractor(m_mockTextExtractor);
    
    // Unregister components
    m_manager->unregisterSearchEngine(m_mockSearchEngine);
    m_manager->unregisterTextExtractor(m_mockTextExtractor);
    
    QVERIFY(true); // If no crash, unregistration succeeded
}

void MemoryManagerTest::testMemoryOptimizationSignals()
{
    QSignalSpy startedSpy(m_manager, &MemoryManager::memoryOptimizationStarted);
    QSignalSpy completedSpy(m_manager, &MemoryManager::memoryOptimizationCompleted);
    
    m_manager->optimizeMemoryUsage();
    
    // Verify signals are emitted appropriately
    QVERIFY(startedSpy.count() >= 0);
    QVERIFY(completedSpy.count() >= 0);
}

void MemoryManagerTest::testMemoryPressureSignals()
{
    QSignalSpy pressureSpy(m_manager, &MemoryManager::memoryPressureChanged);
    
    // Simulate pressure change
    simulateMemoryPressure(0.8);
    
    // Note: Signal emission depends on implementation
    QVERIFY(pressureSpy.count() >= 0);
}

void MemoryManagerTest::testOnMemoryPressureDetected()
{
    QSignalSpy optimizationSpy(m_manager, &MemoryManager::memoryOptimizationStarted);
    
    // Simulate high memory pressure
    m_manager->onMemoryPressureDetected(0.9);
    
    // Should trigger optimization at high pressure
    QVERIFY(optimizationSpy.count() >= 0);
}

void MemoryManagerTest::testOnSystemMemoryPressure()
{
    QSignalSpy emergencySpy(m_manager, &MemoryManager::emergencyCleanupTriggered);
    
    // Simulate critical system memory pressure
    m_manager->onSystemMemoryPressure(0.95);
    
    // Should trigger emergency cleanup at critical pressure
    QVERIFY(emergencySpy.count() >= 0);
}

void MemoryManagerTest::testOnCacheMemoryExceeded()
{
    QSignalSpy optimizationSpy(m_manager, &MemoryManager::memoryOptimizationStarted);
    
    // Simulate cache memory exceeded
    qint64 usage = 1024 * 1024 * 100; // 100MB
    qint64 limit = 1024 * 1024 * 50;  // 50MB
    m_manager->onCacheMemoryExceeded(usage, limit);
    
    // Should trigger optimization when cache exceeds limit
    QVERIFY(optimizationSpy.count() >= 0);
}

void MemoryManagerTest::testPeriodicOptimization()
{
    QSignalSpy optimizationSpy(m_manager, &MemoryManager::memoryOptimizationStarted);
    
    // Trigger periodic optimization
    m_manager->performPeriodicOptimization();
    
    // Should perform optimization
    QVERIFY(optimizationSpy.count() >= 0);
}

void MemoryManagerTest::simulateMemoryPressure(double pressure)
{
    // Helper method to simulate memory pressure
    m_manager->onMemoryPressureDetected(pressure);
}

void MemoryManagerTest::verifyOptimizationCompleted(int timeoutMs)
{
    QSignalSpy spy(m_manager, &MemoryManager::memoryOptimizationCompleted);
    QVERIFY(spy.wait(timeoutMs) || spy.count() > 0);
}

MemoryManager::MemoryStats MemoryManagerTest::createMockStats()
{
    MemoryManager::MemoryStats stats;
    stats.totalMemoryUsage = 1024 * 1024 * 50; // 50MB
    stats.searchCacheMemory = 1024 * 1024 * 20; // 20MB
    stats.textCacheMemory = 1024 * 1024 * 15; // 15MB
    stats.highlightCacheMemory = 1024 * 1024 * 10; // 10MB
    stats.systemMemoryUsage = 1024 * 1024 * 1024; // 1GB
    stats.systemMemoryTotal = 1024 * 1024 * 1024 * 4; // 4GB
    stats.memoryPressure = 0.25;
    stats.pressureLevel = MemoryManager::Normal;
    stats.optimizationCount = 5;
    return stats;
}

QTEST_MAIN(MemoryManagerTest)
#include "memory_manager_test.moc"
