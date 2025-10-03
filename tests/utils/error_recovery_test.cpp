#include <QSignalSpy>
#include <QTest>
#include <QThread>
#include <chrono>
#include <stdexcept>
#include "../../app/utils/ErrorHandling.h"
#include "../../app/utils/ErrorRecovery.h"
#include "../TestUtilities.h"

class ErrorRecoveryTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

    // RetryPolicy tests
    void testRetryPolicyEnum();
    void testRetryConfig();
    void testRetryConfigConstructor();

    // FallbackStrategy tests
    void testFallbackStrategyEnum();

    // RecoveryResult tests
    void testRecoveryResultEnum();

    // CircuitBreaker tests
    void testCircuitBreakerConstructor();
    void testCircuitBreakerCanExecute();
    void testCircuitBreakerRecordSuccess();
    void testCircuitBreakerRecordFailure();
    void testCircuitBreakerStateTransitions();
    void testCircuitBreakerReset();
    void testCircuitBreakerTimeout();

    // RecoveryContext tests
    void testRecoveryContextConstructor();
    void testRecoveryContextFields();

    // RecoveryManager tests
    void testRecoveryManagerSingleton();
    void testRegisterRecoveryAction();
    void testExecuteRecovery();
    void testRetryWithPolicy();
    void testRetryWithPolicySuccess();
    void testRetryWithPolicyFailure();
    void testRetryWithPolicyNoRetry();
    void testGetCircuitBreaker();
    void testResetCircuitBreaker();
    void testDefaultRetryConfig();
    void testRecoveryStats();
    void testResetStats();

    // Recovery Actions tests
    void testFileSystemRecoveryAction();
    void testDocumentRecoveryAction();
    void testRenderingRecoveryAction();
    void testSearchRecoveryAction();
    void testCacheRecoveryAction();

    // Utility functions tests
    void testCreateQuickRetry();
    void testCreateStandardRetry();
    void testCreatePatientRetry();
    void testCreateNetworkRetry();

    // Signal tests
    void testRecoveryAttemptedSignal();
    void testCircuitBreakerStateChangedSignal();

    // Edge cases and error handling
    void testRetryWithZeroAttempts();
    void testRetryWithNegativeAttempts();
    void testCircuitBreakerWithZeroThreshold();
    void testRecoveryWithNullAction();

    // Integration tests
    void testRetryWithCircuitBreaker();
    void testComplexRecoveryScenario();

private:
    ErrorRecovery::RecoveryManager* m_recoveryManager;

    // Helper methods
    void throwException();
    int successFunction();
    int failingFunction();
    int intermittentFunction();
    ErrorHandling::ErrorInfo createTestError();

    // Test counters
    int m_callCount;
    int m_successAfterAttempts;
};

void ErrorRecoveryTest::initTestCase() {
    // Setup test environment
    m_recoveryManager = &ErrorRecovery::RecoveryManager::instance();
}

void ErrorRecoveryTest::cleanupTestCase() {
    // Cleanup test environment
    m_recoveryManager->resetStats();
}

void ErrorRecoveryTest::init() {
    // Per-test setup
    m_callCount = 0;
    m_successAfterAttempts = 1;
    m_recoveryManager->resetStats();
}

void ErrorRecoveryTest::cleanup() {
    // Per-test cleanup
    m_recoveryManager->resetStats();
}

void ErrorRecoveryTest::testRetryPolicyEnum() {
    // Test that all enum values are distinct
    QVERIFY(ErrorRecovery::RetryPolicy::None !=
            ErrorRecovery::RetryPolicy::Immediate);
    QVERIFY(ErrorRecovery::RetryPolicy::Immediate !=
            ErrorRecovery::RetryPolicy::FixedDelay);
    QVERIFY(ErrorRecovery::RetryPolicy::FixedDelay !=
            ErrorRecovery::RetryPolicy::ExponentialBackoff);
    QVERIFY(ErrorRecovery::RetryPolicy::ExponentialBackoff !=
            ErrorRecovery::RetryPolicy::LinearBackoff);
}

void ErrorRecoveryTest::testRetryConfig() {
    ErrorRecovery::RetryConfig config;

    // Test default values
    QCOMPARE(config.policy, ErrorRecovery::RetryPolicy::ExponentialBackoff);
    QCOMPARE(config.maxAttempts, 3);
    QCOMPARE(config.initialDelay, std::chrono::milliseconds(100));
    QCOMPARE(config.maxDelay, std::chrono::milliseconds(5000));
    QCOMPARE(config.backoffMultiplier, 2.0);
}

void ErrorRecoveryTest::testRetryConfigConstructor() {
    ErrorRecovery::RetryConfig config(ErrorRecovery::RetryPolicy::FixedDelay, 5,
                                      std::chrono::milliseconds(200));

    QCOMPARE(config.policy, ErrorRecovery::RetryPolicy::FixedDelay);
    QCOMPARE(config.maxAttempts, 5);
    QCOMPARE(config.initialDelay, std::chrono::milliseconds(200));
}

void ErrorRecoveryTest::testFallbackStrategyEnum() {
    // Test that all enum values are distinct
    QVERIFY(ErrorRecovery::FallbackStrategy::None !=
            ErrorRecovery::FallbackStrategy::DefaultValue);
    QVERIFY(ErrorRecovery::FallbackStrategy::DefaultValue !=
            ErrorRecovery::FallbackStrategy::CachedValue);
    QVERIFY(ErrorRecovery::FallbackStrategy::CachedValue !=
            ErrorRecovery::FallbackStrategy::AlternativeMethod);
    QVERIFY(ErrorRecovery::FallbackStrategy::AlternativeMethod !=
            ErrorRecovery::FallbackStrategy::GracefulDegradation);
    QVERIFY(ErrorRecovery::FallbackStrategy::GracefulDegradation !=
            ErrorRecovery::FallbackStrategy::UserPrompt);
}

void ErrorRecoveryTest::testRecoveryResultEnum() {
    // Test that all enum values are distinct
    QVERIFY(ErrorRecovery::RecoveryResult::Success !=
            ErrorRecovery::RecoveryResult::Failed);
    QVERIFY(ErrorRecovery::RecoveryResult::Failed !=
            ErrorRecovery::RecoveryResult::Retry);
    QVERIFY(ErrorRecovery::RecoveryResult::Retry !=
            ErrorRecovery::RecoveryResult::Fallback);
    QVERIFY(ErrorRecovery::RecoveryResult::Fallback !=
            ErrorRecovery::RecoveryResult::Abort);
}

void ErrorRecoveryTest::testCircuitBreakerConstructor() {
    ErrorRecovery::CircuitBreaker breaker(3, std::chrono::milliseconds(1000));

    QCOMPARE(breaker.getState(), ErrorRecovery::CircuitState::Closed);
    QCOMPARE(breaker.getFailureCount(), 0);
    QVERIFY(breaker.canExecute());
}

void ErrorRecoveryTest::testCircuitBreakerCanExecute() {
    ErrorRecovery::CircuitBreaker breaker(2, std::chrono::milliseconds(100));

    // Initially should be able to execute
    QVERIFY(breaker.canExecute());
    QCOMPARE(breaker.getState(), ErrorRecovery::CircuitState::Closed);
}

void ErrorRecoveryTest::testCircuitBreakerRecordSuccess() {
    ErrorRecovery::CircuitBreaker breaker(2, std::chrono::milliseconds(100));

    breaker.recordSuccess();
    QCOMPARE(breaker.getState(), ErrorRecovery::CircuitState::Closed);
    QVERIFY(breaker.canExecute());
}

void ErrorRecoveryTest::testCircuitBreakerRecordFailure() {
    ErrorRecovery::CircuitBreaker breaker(2, std::chrono::milliseconds(100));

    // First failure
    breaker.recordFailure();
    QCOMPARE(breaker.getFailureCount(), 1);
    QCOMPARE(breaker.getState(), ErrorRecovery::CircuitState::Closed);
    QVERIFY(breaker.canExecute());

    // Second failure - should open circuit
    breaker.recordFailure();
    QCOMPARE(breaker.getFailureCount(), 2);
    QCOMPARE(breaker.getState(), ErrorRecovery::CircuitState::Open);
    QVERIFY(!breaker.canExecute());
}

void ErrorRecoveryTest::testCircuitBreakerStateTransitions() {
    ErrorRecovery::CircuitBreaker breaker(1, std::chrono::milliseconds(50));

    // Start in Closed state
    QCOMPARE(breaker.getState(), ErrorRecovery::CircuitState::Closed);

    // Record failure to open circuit
    breaker.recordFailure();
    QCOMPARE(breaker.getState(), ErrorRecovery::CircuitState::Open);
    QVERIFY(!breaker.canExecute());

    // Wait for timeout
    QThread::msleep(60);

    // Should transition to HalfOpen
    QVERIFY(breaker.canExecute());
    QCOMPARE(breaker.getState(), ErrorRecovery::CircuitState::HalfOpen);

    // Record success to close circuit
    breaker.recordSuccess();
    QCOMPARE(breaker.getState(), ErrorRecovery::CircuitState::Closed);
}

void ErrorRecoveryTest::testCircuitBreakerReset() {
    ErrorRecovery::CircuitBreaker breaker(1, std::chrono::milliseconds(100));

    // Open the circuit
    breaker.recordFailure();
    QCOMPARE(breaker.getState(), ErrorRecovery::CircuitState::Open);

    // Reset should close the circuit
    breaker.reset();
    QCOMPARE(breaker.getState(), ErrorRecovery::CircuitState::Closed);
    QCOMPARE(breaker.getFailureCount(), 0);
    QVERIFY(breaker.canExecute());
}

void ErrorRecoveryTest::testCircuitBreakerTimeout() {
    ErrorRecovery::CircuitBreaker breaker(1, std::chrono::milliseconds(50));

    // Open the circuit
    breaker.recordFailure();
    QCOMPARE(breaker.getState(), ErrorRecovery::CircuitState::Open);
    QVERIFY(!breaker.canExecute());

    // Before timeout - should still be open
    QThread::msleep(25);
    QVERIFY(!breaker.canExecute());

    // After timeout - should allow execution (HalfOpen)
    QThread::msleep(30);
    QVERIFY(breaker.canExecute());
}

void ErrorRecoveryTest::testRecoveryContextConstructor() {
    ErrorHandling::ErrorInfo error = createTestError();
    ErrorRecovery::RecoveryContext context(error, "TestComponent",
                                           "TestOperation");

    QCOMPARE(context.error.message, error.message);
    QCOMPARE(context.attemptCount, 0);
    QCOMPARE(context.componentName, "TestComponent");
    QCOMPARE(context.operationName, "TestOperation");
    QVERIFY(!context.firstAttempt.isNull());
}

void ErrorRecoveryTest::testRecoveryContextFields() {
    ErrorHandling::ErrorInfo error = createTestError();
    ErrorRecovery::RecoveryContext context(error, "Component", "Operation");

    // Test field modifications
    context.attemptCount = 3;
    context.lastAttempt = QDateTime::currentDateTime();
    context.metadata["key"] = "value";

    QCOMPARE(context.attemptCount, 3);
    QVERIFY(!context.lastAttempt.isNull());
    QCOMPARE(context.metadata["key"].toString(), "value");
}

void ErrorRecoveryTest::testRecoveryManagerSingleton() {
    ErrorRecovery::RecoveryManager& manager1 =
        ErrorRecovery::RecoveryManager::instance();
    ErrorRecovery::RecoveryManager& manager2 =
        ErrorRecovery::RecoveryManager::instance();

    // Should be the same instance
    QCOMPARE(&manager1, &manager2);
}

// Helper method implementations
void ErrorRecoveryTest::throwException() {
    throw std::runtime_error("Test exception");
}

int ErrorRecoveryTest::successFunction() { return 42; }

int ErrorRecoveryTest::failingFunction() {
    throw std::runtime_error("Always fails");
}

int ErrorRecoveryTest::intermittentFunction() {
    m_callCount++;
    if (m_callCount < m_successAfterAttempts) {
        throw std::runtime_error("Intermittent failure");
    }
    return m_callCount;
}

ErrorHandling::ErrorInfo ErrorRecoveryTest::createTestError() {
    return ErrorHandling::ErrorInfo(ErrorHandling::ErrorCategory::FileSystem,
                                    ErrorHandling::ErrorSeverity::Error,
                                    "Test error message", "Test error details",
                                    "Test context", 404);
}

QTEST_MAIN(ErrorRecoveryTest)
#include "error_recovery_test.moc"
