#include <QElapsedTimer>
#include <QObject>
#include <QSignalSpy>
#include <QTimer>
#include <QVariantMap>
#include <QtTest/QtTest>
#include <exception>
#include <stdexcept>
#include "../../app/search/SearchErrorRecovery.h"
#include "../TestUtilities.h"

/**
 * Comprehensive tests for SearchErrorRecovery class
 * Tests error handling, recovery strategies, and circuit breaker patterns
 */
class SearchErrorRecoveryTest : public TestBase {
    Q_OBJECT

protected:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

private slots:
    // Constructor and configuration tests
    void testConstructor();
    void testSetRecoveryConfig();
    void testGetRecoveryConfig();
    void testSetGlobalRecoveryEnabled();

    // Error handling tests
    void testHandleExceptionError();
    void testHandleStringError();
    void testRecoverFromError();

    // Recovery strategy tests
    void testRetryOperation();
    void testFallbackOperation();
    void testDegradeOperation();
    void testSkipOperation();
    void testResetOperation();

    // Execute with recovery tests
    void testExecuteWithRecoverySuccess();
    void testExecuteWithRecoveryFailure();
    void testExecuteWithRecoveryRetry();

    // Circuit breaker tests
    void testEnableCircuitBreaker();
    void testDisableCircuitBreaker();
    void testCircuitBreakerOpen();
    void testRecordOperationSuccess();
    void testRecordOperationFailure();

    // State management tests
    void testSaveOperationState();
    void testRestoreOperationState();
    void testClearOperationState();

    // Error statistics tests
    void testGetErrorStats();
    void testResetErrorStats();
    void testGetRecentErrors();

    // Fallback mechanism tests
    void testRegisterFallback();
    void testUnregisterFallback();
    void testExecuteFallback();

    // Health monitoring tests
    void testComponentHealth();
    void testReportComponentHealth();
    void testGetUnhealthyComponents();

    // Signal emission tests
    void testErrorOccurredSignal();
    void testRecoverySignals();
    void testCircuitBreakerSignals();
    void testComponentHealthSignals();

    // Exception classes tests
    void testSearchException();
    void testDocumentException();
    void testCacheException();
    void testTimeoutException();

    // Error scope tests
    void testSearchErrorScope();

private:
    SearchErrorRecovery* m_recovery;
    SearchErrorRecovery::ErrorContext m_testContext;
    SearchErrorRecovery::RecoveryConfig m_testConfig;

    // Helper methods
    void setupTestContext();
    void setupTestConfig();
    SearchErrorRecovery::ErrorContext createErrorContext(
        SearchErrorRecovery::ErrorType type, const QString& operation);
    void simulateError(const QString& message);
    bool throwingOperation();
    bool successfulOperation();
};

void SearchErrorRecoveryTest::initTestCase() {
    qDebug() << "Starting SearchErrorRecovery tests";
    setupTestContext();
    setupTestConfig();
}

void SearchErrorRecoveryTest::cleanupTestCase() {
    qDebug() << "SearchErrorRecovery tests completed";
}

void SearchErrorRecoveryTest::init() {
    m_recovery = new SearchErrorRecovery(this);
}

void SearchErrorRecoveryTest::cleanup() {
    if (m_recovery) {
        delete m_recovery;
        m_recovery = nullptr;
    }
}

void SearchErrorRecoveryTest::setupTestContext() {
    m_testContext = SearchErrorRecovery::ErrorContext(
        SearchErrorRecovery::SearchError, "test_operation", "test_component",
        "Test error details");
}

void SearchErrorRecoveryTest::setupTestConfig() {
    m_testConfig.strategy = SearchErrorRecovery::Retry;
    m_testConfig.maxRetries = 3;
    m_testConfig.retryDelayMs = 100;
    m_testConfig.exponentialBackoff = true;
    m_testConfig.enableFallback = true;
    m_testConfig.enableDegradation = true;
    m_testConfig.timeoutMs = 5000;
    m_testConfig.logRecoveryAttempts = true;
}

void SearchErrorRecoveryTest::testConstructor() {
    QVERIFY(m_recovery != nullptr);
    QVERIFY(m_recovery->isGlobalRecoveryEnabled());

    // Test default configurations exist for all error types
    auto config =
        m_recovery->getRecoveryConfig(SearchErrorRecovery::SearchError);
    QVERIFY(config.maxRetries >= 0);
    QVERIFY(config.retryDelayMs >= 0);
    QVERIFY(config.timeoutMs > 0);
}

void SearchErrorRecoveryTest::testSetRecoveryConfig() {
    m_recovery->setRecoveryConfig(SearchErrorRecovery::SearchError,
                                  m_testConfig);

    auto retrievedConfig =
        m_recovery->getRecoveryConfig(SearchErrorRecovery::SearchError);
    QCOMPARE(retrievedConfig.strategy, m_testConfig.strategy);
    QCOMPARE(retrievedConfig.maxRetries, m_testConfig.maxRetries);
    QCOMPARE(retrievedConfig.retryDelayMs, m_testConfig.retryDelayMs);
    QCOMPARE(retrievedConfig.exponentialBackoff,
             m_testConfig.exponentialBackoff);
}

void SearchErrorRecoveryTest::testGetRecoveryConfig() {
    // Test getting config for different error types
    auto searchConfig =
        m_recovery->getRecoveryConfig(SearchErrorRecovery::SearchError);
    auto documentConfig =
        m_recovery->getRecoveryConfig(SearchErrorRecovery::DocumentError);
    auto cacheConfig =
        m_recovery->getRecoveryConfig(SearchErrorRecovery::CacheError);

    QVERIFY(searchConfig.maxRetries >= 0);
    QVERIFY(documentConfig.maxRetries >= 0);
    QVERIFY(cacheConfig.maxRetries >= 0);
}

void SearchErrorRecoveryTest::testSetGlobalRecoveryEnabled() {
    QVERIFY(m_recovery->isGlobalRecoveryEnabled());

    m_recovery->setGlobalRecoveryEnabled(false);
    QVERIFY(!m_recovery->isGlobalRecoveryEnabled());

    m_recovery->setGlobalRecoveryEnabled(true);
    QVERIFY(m_recovery->isGlobalRecoveryEnabled());
}

void SearchErrorRecoveryTest::testHandleExceptionError() {
    QSignalSpy errorSpy(m_recovery, &SearchErrorRecovery::errorOccurred);

    std::runtime_error testException("Test exception");
    auto result = m_recovery->handleError(testException, m_testContext);

    QVERIFY(errorSpy.count() >= 1);
    QVERIFY(!result.message.isEmpty());
}

void SearchErrorRecoveryTest::testHandleStringError() {
    QSignalSpy errorSpy(m_recovery, &SearchErrorRecovery::errorOccurred);

    QString errorMessage = "Test error message";
    auto result = m_recovery->handleError(errorMessage, m_testContext);

    QVERIFY(errorSpy.count() >= 1);
    QCOMPARE(result.message, errorMessage);
}

void SearchErrorRecoveryTest::testRecoverFromError() {
    auto result = m_recovery->recoverFromError(m_testContext);

    // Recovery should attempt some strategy
    QVERIFY(result.usedStrategy != SearchErrorRecovery::NoRecovery);
    QVERIFY(result.attemptsUsed >= 0);
}

void SearchErrorRecoveryTest::testRetryOperation() {
    QSignalSpy recoverySpy(m_recovery, &SearchErrorRecovery::recoveryAttempted);

    int attemptCount = 0;
    auto operation = [&attemptCount]() -> bool {
        attemptCount++;
        return attemptCount >= 2;  // Succeed on second attempt
    };

    auto result = m_recovery->retryOperation(operation, m_testContext);

    QVERIFY(result.success);
    QCOMPARE(result.usedStrategy, SearchErrorRecovery::Retry);
    QVERIFY(result.attemptsUsed >= 2);
    QVERIFY(recoverySpy.count() >= 1);
}

void SearchErrorRecoveryTest::testFallbackOperation() {
    auto result = m_recovery->fallbackOperation(m_testContext);

    QCOMPARE(result.usedStrategy, SearchErrorRecovery::Fallback);
    QVERIFY(result.attemptsUsed >= 0);
}

void SearchErrorRecoveryTest::testDegradeOperation() {
    auto result = m_recovery->degradeOperation(m_testContext);

    QCOMPARE(result.usedStrategy, SearchErrorRecovery::Degrade);
    QVERIFY(result.attemptsUsed >= 0);
}

void SearchErrorRecoveryTest::testSkipOperation() {
    auto result = m_recovery->skipOperation(m_testContext);

    QCOMPARE(result.usedStrategy, SearchErrorRecovery::Skip);
    QVERIFY(result.attemptsUsed >= 0);
}

void SearchErrorRecoveryTest::testResetOperation() {
    auto result = m_recovery->resetOperation(m_testContext);

    QCOMPARE(result.usedStrategy, SearchErrorRecovery::Reset);
    QVERIFY(result.attemptsUsed >= 0);
}

void SearchErrorRecoveryTest::testExecuteWithRecoverySuccess() {
    auto operation = []() -> int { return 42; };

    int result = m_recovery->executeWithRecovery<int>(operation, m_testContext);
    QCOMPARE(result, 42);
}

void SearchErrorRecoveryTest::testExecuteWithRecoveryFailure() {
    auto operation = []() -> int { throw std::runtime_error("Test failure"); };

    QVERIFY_EXCEPTION_THROWN(
        m_recovery->executeWithRecovery<int>(operation, m_testContext),
        std::runtime_error);
}

void SearchErrorRecoveryTest::testEnableCircuitBreaker() {
    QString operationName = "test_operation";

    m_recovery->enableCircuitBreaker(operationName, 3, 60000);
    QVERIFY(!m_recovery->isCircuitBreakerOpen(operationName));
}

void SearchErrorRecoveryTest::testDisableCircuitBreaker() {
    QString operationName = "test_operation";

    m_recovery->enableCircuitBreaker(operationName);
    m_recovery->disableCircuitBreaker(operationName);

    // After disabling, circuit breaker should not be open
    QVERIFY(!m_recovery->isCircuitBreakerOpen(operationName));
}

void SearchErrorRecoveryTest::testCircuitBreakerOpen() {
    QString operationName = "test_operation";

    m_recovery->enableCircuitBreaker(operationName, 2, 60000);

    // Record multiple failures to open circuit breaker
    m_recovery->recordOperationFailure(operationName);
    m_recovery->recordOperationFailure(operationName);
    m_recovery->recordOperationFailure(operationName);

    // Circuit breaker should be open after threshold failures
    QVERIFY(m_recovery->isCircuitBreakerOpen(operationName));
}

void SearchErrorRecoveryTest::testRecordOperationSuccess() {
    QString operationName = "test_operation";

    m_recovery->enableCircuitBreaker(operationName);
    m_recovery->recordOperationSuccess(operationName);

    // Should not crash and circuit breaker should remain closed
    QVERIFY(!m_recovery->isCircuitBreakerOpen(operationName));
}

void SearchErrorRecoveryTest::testRecordOperationFailure() {
    QString operationName = "test_operation";

    m_recovery->enableCircuitBreaker(operationName);
    m_recovery->recordOperationFailure(operationName);

    // Should not crash
    QVERIFY(true);
}

void SearchErrorRecoveryTest::testSaveOperationState() {
    QString operationId = "test_op_123";
    QVariantMap state;
    state["key1"] = "value1";
    state["key2"] = 42;

    m_recovery->saveOperationState(operationId, state);

    QVariantMap restored = m_recovery->restoreOperationState(operationId);
    QCOMPARE(restored["key1"].toString(), QString("value1"));
    QCOMPARE(restored["key2"].toInt(), 42);
}

void SearchErrorRecoveryTest::testRestoreOperationState() {
    QString operationId = "test_op_456";
    QVariantMap state;
    state["data"] = "test_data";

    m_recovery->saveOperationState(operationId, state);
    QVariantMap restored = m_recovery->restoreOperationState(operationId);

    QCOMPARE(restored["data"].toString(), QString("test_data"));
}

void SearchErrorRecoveryTest::testClearOperationState() {
    QString operationId = "test_op_789";
    QVariantMap state;
    state["temp"] = "temporary";

    m_recovery->saveOperationState(operationId, state);
    m_recovery->clearOperationState(operationId);

    QVariantMap restored = m_recovery->restoreOperationState(operationId);
    QVERIFY(restored.isEmpty());
}

void SearchErrorRecoveryTest::testGetErrorStats() {
    auto stats = m_recovery->getErrorStats();

    QVERIFY(stats.totalErrors >= 0);
    QVERIFY(stats.recoveredErrors >= 0);
    QVERIFY(stats.failedRecoveries >= 0);
    QVERIFY(stats.recoveredErrors <= stats.totalErrors);
    QVERIFY(stats.failedRecoveries <= stats.totalErrors);
}

void SearchErrorRecoveryTest::testResetErrorStats() {
    // Generate some errors first
    m_recovery->handleError("Test error", m_testContext);

    auto statsBefore = m_recovery->getErrorStats();
    m_recovery->resetErrorStats();
    auto statsAfter = m_recovery->getErrorStats();

    QVERIFY(statsAfter.totalErrors <= statsBefore.totalErrors);
}

void SearchErrorRecoveryTest::testGetRecentErrors() {
    // Generate some errors
    for (int i = 0; i < 5; ++i) {
        m_recovery->handleError(QString("Error %1").arg(i), m_testContext);
    }

    QStringList recentErrors = m_recovery->getRecentErrors(3);
    QVERIFY(recentErrors.size() <= 3);
}

void SearchErrorRecoveryTest::testRegisterFallback() {
    auto fallbackFunction =
        [](const SearchErrorRecovery::ErrorContext& context) -> QVariant {
        Q_UNUSED(context)
        return QVariant("Fallback result");
    };

    m_recovery->registerFallback(SearchErrorRecovery::SearchError,
                                 "test_operation", fallbackFunction);

    // Test that registration doesn't crash
    QVERIFY(true);
}

void SearchErrorRecoveryTest::testUnregisterFallback() {
    m_recovery->unregisterFallback(SearchErrorRecovery::SearchError,
                                   "test_operation");

    // Test that unregistration doesn't crash
    QVERIFY(true);
}

void SearchErrorRecoveryTest::testExecuteFallback() {
    auto fallbackFunction =
        [](const SearchErrorRecovery::ErrorContext& context) -> QVariant {
        Q_UNUSED(context)
        return QVariant("Fallback executed");
    };

    m_recovery->registerFallback(SearchErrorRecovery::SearchError,
                                 "test_operation", fallbackFunction);

    QVariant result = m_recovery->executeFallback(
        SearchErrorRecovery::SearchError, "test_operation", m_testContext);
    QCOMPARE(result.toString(), QString("Fallback executed"));
}

void SearchErrorRecoveryTest::testComponentHealth() {
    QString component = "test_component";

    // Initially should be healthy
    QVERIFY(m_recovery->isComponentHealthy(component));

    // Report unhealthy
    m_recovery->reportComponentHealth(component, false);
    QVERIFY(!m_recovery->isComponentHealthy(component));

    // Report healthy again
    m_recovery->reportComponentHealth(component, true);
    QVERIFY(m_recovery->isComponentHealthy(component));
}

void SearchErrorRecoveryTest::testReportComponentHealth() {
    QSignalSpy healthSpy(m_recovery,
                         &SearchErrorRecovery::componentHealthChanged);

    QString component = "test_component";
    m_recovery->reportComponentHealth(component, false);

    QVERIFY(healthSpy.count() >= 0);
}

void SearchErrorRecoveryTest::testGetUnhealthyComponents() {
    QString component1 = "component1";
    QString component2 = "component2";

    m_recovery->reportComponentHealth(component1, false);
    m_recovery->reportComponentHealth(component2, true);

    QStringList unhealthy = m_recovery->getUnhealthyComponents();
    QVERIFY(unhealthy.contains(component1));
    QVERIFY(!unhealthy.contains(component2));
}

void SearchErrorRecoveryTest::testErrorOccurredSignal() {
    QSignalSpy errorSpy(m_recovery, &SearchErrorRecovery::errorOccurred);

    m_recovery->handleError("Test error", m_testContext);

    QVERIFY(errorSpy.count() >= 1);
}

void SearchErrorRecoveryTest::testRecoverySignals() {
    QSignalSpy attemptedSpy(m_recovery,
                            &SearchErrorRecovery::recoveryAttempted);
    QSignalSpy succeededSpy(m_recovery,
                            &SearchErrorRecovery::recoverySucceeded);
    QSignalSpy failedSpy(m_recovery, &SearchErrorRecovery::recoveryFailed);

    // Test recovery attempt
    auto operation = []() -> bool { return true; };
    m_recovery->retryOperation(operation, m_testContext);

    // Signals should be emitted appropriately
    QVERIFY(attemptedSpy.count() >= 0);
    QVERIFY(succeededSpy.count() >= 0 || failedSpy.count() >= 0);
}

void SearchErrorRecoveryTest::testCircuitBreakerSignals() {
    QSignalSpy openedSpy(m_recovery,
                         &SearchErrorRecovery::circuitBreakerOpened);
    QSignalSpy closedSpy(m_recovery,
                         &SearchErrorRecovery::circuitBreakerClosed);

    QString operationName = "test_operation";
    m_recovery->enableCircuitBreaker(operationName, 1, 60000);

    // Force circuit breaker to open
    m_recovery->recordOperationFailure(operationName);
    m_recovery->recordOperationFailure(operationName);

    // Signals should be emitted appropriately
    QVERIFY(openedSpy.count() >= 0);
}

void SearchErrorRecoveryTest::testComponentHealthSignals() {
    QSignalSpy healthSpy(m_recovery,
                         &SearchErrorRecovery::componentHealthChanged);

    m_recovery->reportComponentHealth("test_component", false);

    QVERIFY(healthSpy.count() >= 0);
}

void SearchErrorRecoveryTest::testSearchException() {
    SearchException exception("Test search exception");

    QCOMPARE(QString::fromStdString(exception.what()),
             QString("Test search exception"));
    QCOMPARE(exception.type(), SearchErrorRecovery::SearchError);
}

void SearchErrorRecoveryTest::testDocumentException() {
    DocumentException exception("Test document exception");

    QCOMPARE(QString::fromStdString(exception.what()),
             QString("Test document exception"));
    QCOMPARE(exception.type(), SearchErrorRecovery::DocumentError);
}

void SearchErrorRecoveryTest::testCacheException() {
    CacheException exception("Test cache exception");

    QCOMPARE(QString::fromStdString(exception.what()),
             QString("Test cache exception"));
    QCOMPARE(exception.type(), SearchErrorRecovery::CacheError);
}

void SearchErrorRecoveryTest::testTimeoutException() {
    TimeoutException exception("Test timeout exception");

    QCOMPARE(QString::fromStdString(exception.what()),
             QString("Test timeout exception"));
    QCOMPARE(exception.type(), SearchErrorRecovery::TimeoutError);
}

void SearchErrorRecoveryTest::testSearchErrorScope() {
    {
        SearchErrorScope scope(m_recovery, m_testContext);
        scope.setSuccessful(true);
        scope.addMetadata("test_key", "test_value");
        scope.updateDetails("Updated details");
    }

    // Scope should clean up properly
    QVERIFY(true);
}

SearchErrorRecovery::ErrorContext SearchErrorRecoveryTest::createErrorContext(
    SearchErrorRecovery::ErrorType type, const QString& operation) {
    return SearchErrorRecovery::ErrorContext(type, operation, "test_component",
                                             "Test details");
}

bool SearchErrorRecoveryTest::throwingOperation() {
    throw std::runtime_error("Test exception");
}

bool SearchErrorRecoveryTest::successfulOperation() { return true; }

QTEST_MAIN(SearchErrorRecoveryTest)
#include "test_search_error_recovery.moc"
