#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QTemporaryFile>
#include <QtTest/QtTest>
#include "../../app/model/DocumentModel.h"
#include "../../app/utils/ErrorRecovery.h"

/**
 * Test suite for error recovery mechanisms
 */
class TestErrorRecovery : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Core recovery framework tests
    void testRetryPolicies();
    void testCircuitBreaker();
    void testRecoveryActions();
    void testRecoveryManager();

    // Component integration tests
    void testDocumentModelRecovery();
    void testFileSystemRecovery();
    void testRenderingRecovery();

    // Advanced recovery tests
    void testRetryWithBackoff();
    void testFallbackStrategies();
    void testRecoveryStatistics();

private:
    QApplication* m_app;
    QString m_testDataDir;
};

void TestErrorRecovery::initTestCase() {
    qDebug() << "=== Error Recovery Test Suite ===";
    qDebug() << "Testing error recovery mechanisms and strategies";

    // Create test data directory
    m_testDataDir = QDir::tempPath() + "/error_recovery_tests";
    QDir().mkpath(m_testDataDir);
}

void TestErrorRecovery::cleanupTestCase() {
    // Clean up test data
    QDir(m_testDataDir).removeRecursively();
    qDebug() << "Error recovery tests completed";
}

void TestErrorRecovery::testRetryPolicies() {
    using namespace ErrorRecovery;

    // Test immediate retry
    auto quickConfig = Utils::createQuickRetry();
    QCOMPARE(quickConfig.policy, RetryPolicy::Immediate);
    QCOMPARE(quickConfig.maxAttempts, 2);

    // Test standard retry
    auto standardConfig = Utils::createStandardRetry();
    QCOMPARE(standardConfig.policy, RetryPolicy::ExponentialBackoff);
    QCOMPARE(standardConfig.maxAttempts, 3);

    // Test patient retry
    auto patientConfig = Utils::createPatientRetry();
    QCOMPARE(patientConfig.policy, RetryPolicy::ExponentialBackoff);
    QCOMPARE(patientConfig.maxAttempts, 5);

    // Test network retry
    auto networkConfig = Utils::createNetworkRetry();
    QCOMPARE(networkConfig.policy, RetryPolicy::ExponentialBackoff);
    QCOMPARE(networkConfig.maxAttempts, 4);
}

void TestErrorRecovery::testCircuitBreaker() {
    using namespace ErrorRecovery;

    CircuitBreaker breaker(
        3, std::chrono::milliseconds(100));  // 3 failures, 100ms timeout

    // Initially closed
    QVERIFY(breaker.canExecute());
    QCOMPARE(breaker.getState(), CircuitState::Closed);

    // Record failures
    breaker.recordFailure();
    QVERIFY(breaker.canExecute());
    QCOMPARE(breaker.getState(), CircuitState::Closed);

    breaker.recordFailure();
    QVERIFY(breaker.canExecute());
    QCOMPARE(breaker.getState(), CircuitState::Closed);

    breaker.recordFailure();
    QVERIFY(!breaker.canExecute());  // Should be open now
    QCOMPARE(breaker.getState(), CircuitState::Open);

    // Wait for timeout
    QThread::msleep(150);
    QVERIFY(breaker.canExecute());  // Should be half-open
    QCOMPARE(breaker.getState(), CircuitState::HalfOpen);

    // Success should close it
    breaker.recordSuccess();
    QVERIFY(breaker.canExecute());
    QCOMPARE(breaker.getState(), CircuitState::Closed);
}

void TestErrorRecovery::testRecoveryActions() {
    using namespace ErrorRecovery;
    using namespace ErrorHandling;

    // Test file system recovery action
    FileSystemRecoveryAction fsAction;
    QVERIFY(!fsAction.getDescription().isEmpty());

    auto fsError = createFileSystemError("test operation", "/nonexistent/path",
                                         "File not found");
    auto fsResult = fsAction.execute(fsError);
    QVERIFY(fsResult == RecoveryResult::Failed ||
            fsResult == RecoveryResult::Fallback);

    // Test document recovery action
    DocumentRecoveryAction docAction;
    QVERIFY(!docAction.getDescription().isEmpty());

    auto docError = createDocumentError("parse document", "Invalid PDF format");
    auto docResult = docAction.execute(docError);
    QCOMPARE(docResult, RecoveryResult::Fallback);

    // Test rendering recovery action
    RenderingRecoveryAction renderAction;
    QVERIFY(!renderAction.getDescription().isEmpty());

    auto renderError = createRenderingError("render page", "DPI 600 too high");
    auto renderResult = renderAction.execute(renderError);
    QCOMPARE(renderResult, RecoveryResult::Fallback);

    // Test search recovery action
    SearchRecoveryAction searchAction;
    QVERIFY(!searchAction.getDescription().isEmpty());

    auto searchError = createSearchError("search text", "Timeout occurred");
    auto searchResult = searchAction.execute(searchError);
    QCOMPARE(searchResult, RecoveryResult::Fallback);

    // Test cache recovery action
    CacheRecoveryAction cacheAction;
    QVERIFY(!cacheAction.getDescription().isEmpty());

    auto cacheError = createCacheError("cache operation", "Cache full");
    auto cacheResult = cacheAction.execute(cacheError);
    QCOMPARE(cacheResult, RecoveryResult::Fallback);
}

void TestErrorRecovery::testRecoveryManager() {
    using namespace ErrorRecovery;
    using namespace ErrorHandling;

    auto& manager = RecoveryManager::instance();

    // Register a test recovery action
    auto testAction = std::make_shared<FileSystemRecoveryAction>();
    manager.registerRecoveryAction(ErrorCategory::FileSystem, testAction);

    // Test recovery execution
    auto error = createFileSystemError("test", "/test/path", "Test error");
    auto result =
        manager.executeRecovery(error, "TestComponent", "testOperation");

    // Should attempt recovery (result depends on specific action
    // implementation)
    QVERIFY(result == RecoveryResult::Failed ||
            result == RecoveryResult::Fallback ||
            result == RecoveryResult::Retry ||
            result == RecoveryResult::Success);

    // Test circuit breaker creation
    auto& breaker = manager.getCircuitBreaker("test_breaker");
    QVERIFY(breaker.canExecute());

    // Test statistics
    auto stats = manager.getStats("TestComponent");
    QVERIFY(stats.totalAttempts >= 1);
}

void TestErrorRecovery::testDocumentModelRecovery() {
    // Test DocumentModel with error recovery
    DocumentModel model;

    // Test with non-existent file (should trigger recovery)
    bool result = model.openFromFile("/nonexistent/file.pdf");
    QVERIFY(!result);  // Should fail gracefully with recovery

    // The recovery system should have logged the attempt
    // (We can't easily test the internal recovery without more complex setup)
}

void TestErrorRecovery::testFileSystemRecovery() {
    using namespace ErrorRecovery;
    using namespace ErrorHandling;

    FileSystemRecoveryAction action;

    // Test directory creation scenario
    QString testPath = m_testDataDir + "/subdir/file.txt";
    auto error = createFileSystemError("create file", testPath,
                                       "Directory does not exist");

    auto result = action.execute(error);
    // Should attempt to create directory and suggest retry
    QVERIFY(result == RecoveryResult::Retry ||
            result == RecoveryResult::Failed);
}

void TestErrorRecovery::testRenderingRecovery() {
    using namespace ErrorRecovery;
    using namespace ErrorHandling;

    RenderingRecoveryAction action;

    // Test DPI fallback scenario
    auto dpiError = createRenderingError("render page", "DPI 600 too high");
    auto result = action.execute(dpiError);
    QCOMPARE(result, RecoveryResult::Fallback);

    // Test memory fallback scenario
    auto memError = createRenderingError("render page", "Out of memory");
    auto result2 = action.execute(memError);
    QCOMPARE(result2, RecoveryResult::Fallback);

    // Test timeout retry scenario
    auto timeoutError =
        createRenderingError("render page", "Operation timeout");
    auto result3 = action.execute(timeoutError);
    QCOMPARE(result3, RecoveryResult::Retry);
}

void TestErrorRecovery::testRetryWithBackoff() {
    using namespace ErrorRecovery;

    auto& manager = RecoveryManager::instance();

    int attempts = 0;
    auto config = RetryConfig(RetryPolicy::ExponentialBackoff, 3,
                              std::chrono::milliseconds(10));

    auto result = manager.retryWithPolicy(
        [&]() -> int {
            attempts++;
            if (attempts < 3) {
                throw std::runtime_error("Temporary failure");
            }
            return 42;
        },
        config, "test_backoff");

    QVERIFY(ErrorHandling::isSuccess(result));
    QCOMPARE(ErrorHandling::getValue(result), 42);
    QCOMPARE(attempts, 3);
}

void TestErrorRecovery::testFallbackStrategies() {
    using namespace ErrorRecovery;
    using namespace ErrorHandling;

    // Test that recovery actions provide appropriate fallback strategies
    RenderingRecoveryAction renderAction;

    // High DPI should fallback to lower quality
    auto dpiError = createRenderingError("render", "DPI too high");
    QCOMPARE(renderAction.execute(dpiError), RecoveryResult::Fallback);

    // Memory issues should fallback to simpler rendering
    auto memError = createRenderingError("render", "Out of memory");
    QCOMPARE(renderAction.execute(memError), RecoveryResult::Fallback);
}

void TestErrorRecovery::testRecoveryStatistics() {
    using namespace ErrorRecovery;
    using namespace ErrorHandling;

    auto& manager = RecoveryManager::instance();
    manager.resetStats();

    // Register and execute some recovery actions
    auto testAction = std::make_shared<CacheRecoveryAction>();
    manager.registerRecoveryAction(ErrorCategory::Cache, testAction);

    auto error = createCacheError("test", "Test cache error");
    manager.executeRecovery(error, "TestStats", "testOp");

    // Check statistics
    auto stats = manager.getStats("TestStats");
    QCOMPARE(stats.totalAttempts, 1);
    QVERIFY(stats.lastRecovery.isValid());

    // Check global statistics
    auto globalStats = manager.getStats();
    QVERIFY(globalStats.totalAttempts >= 1);
}

QTEST_MAIN(TestErrorRecovery)
#include "test_error_recovery.moc"
