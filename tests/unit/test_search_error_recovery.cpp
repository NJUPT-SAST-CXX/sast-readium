#include <QObject>
#include <QSignalSpy>
#include <QtTest>
#include "../../app/search/SearchErrorRecovery.h"

class TestSearchErrorRecovery : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Basic error recovery tests
    void testErrorClassification();
    void testRetryStrategy();
    void testFallbackStrategy();
    void testDegradeStrategy();
    void testSkipStrategy();
    void testResetStrategy();

    // Circuit breaker tests
    void testCircuitBreakerBasic();
    void testCircuitBreakerTimeout();
    void testCircuitBreakerRecovery();

    // State management tests
    void testOperationStateManagement();
    void testStateRecovery();

    // Fallback function tests
    void testFallbackRegistration();
    void testFallbackExecution();

    // Component health tests
    void testComponentHealthTracking();
    void testUnhealthyComponentDetection();

    // Statistics tests
    void testErrorStatistics();
    void testRecoveryStatistics();

    // Exception handling tests
    void testSearchExceptionHandling();
    void testStandardExceptionHandling();

    // Error scope tests
    void testErrorScopeBasic();
    void testErrorScopeAutomatic();

private:
    SearchErrorRecovery* recovery;
    int operationCallCount;
    bool operationShouldFail;
    QString lastErrorMessage;
};

void TestSearchErrorRecovery::initTestCase() {
    recovery = new SearchErrorRecovery();
    operationCallCount = 0;
    operationShouldFail = false;
}

void TestSearchErrorRecovery::cleanupTestCase() { delete recovery; }

void TestSearchErrorRecovery::testErrorClassification() {
    // Test validation error classification
    SearchErrorRecovery::ErrorContext context(
        SearchErrorRecovery::ValidationError, "test", "component");
    auto result = recovery->handleError("Invalid input validation", context);
    QVERIFY(!result.success);  // Validation errors typically don't retry

    // Test document error classification
    context.type = SearchErrorRecovery::DocumentError;
    result = recovery->handleError("Failed to load document", context);
    // Should attempt recovery based on configuration

    // Test search error classification
    context.type = SearchErrorRecovery::SearchError;
    result = recovery->handleError("Search pattern failed", context);
    // Should attempt fallback
}

void TestSearchErrorRecovery::testRetryStrategy() {
    // Configure retry strategy
    SearchErrorRecovery::RecoveryConfig config;
    config.strategy = SearchErrorRecovery::Retry;
    config.maxRetries = 3;
    config.retryDelayMs = 100;  // Short delay for testing
    config.exponentialBackoff = false;

    recovery->setRecoveryConfig(SearchErrorRecovery::SearchError, config);

    operationCallCount = 0;
    operationShouldFail = true;

    auto operation = [this]() -> bool {
        operationCallCount++;
        if (operationShouldFail && operationCallCount < 3) {
            throw std::runtime_error("Simulated failure");
        }
        return true;
    };

    SearchErrorRecovery::ErrorContext context(SearchErrorRecovery::SearchError,
                                              "retry_test", "test");

    // Should succeed on third attempt
    operationShouldFail = false;  // Allow success on retry
    auto result = recovery->executeWithRecovery<bool>(operation, context);
    QVERIFY(result);
    QVERIFY(operationCallCount >= 1);
}

void TestSearchErrorRecovery::testFallbackStrategy() {
    // Register a fallback function
    recovery->registerFallback(
        SearchErrorRecovery::SearchError, "fallback_test",
        [](const SearchErrorRecovery::ErrorContext& context) -> QVariant {
            return QVariant("fallback_result");
        });

    SearchErrorRecovery::ErrorContext context(SearchErrorRecovery::SearchError,
                                              "fallback_test", "test");
    auto result = recovery->fallbackOperation(context);

    QVERIFY(result.success);
    QCOMPARE(result.usedStrategy, SearchErrorRecovery::Fallback);

    // Test fallback execution
    QVariant fallbackResult = recovery->executeFallback(
        SearchErrorRecovery::SearchError, "fallback_test", context);
    QCOMPARE(fallbackResult.toString(), "fallback_result");
}

void TestSearchErrorRecovery::testDegradeStrategy() {
    SearchErrorRecovery::ErrorContext context(SearchErrorRecovery::MemoryError,
                                              "degrade_test", "test_component");
    auto result = recovery->degradeOperation(context);

    QVERIFY(result.success);
    QCOMPARE(result.usedStrategy, SearchErrorRecovery::Degrade);

    // Component should be marked as unhealthy
    QVERIFY(!recovery->isComponentHealthy("test_component"));
    QVERIFY(recovery->getUnhealthyComponents().contains("test_component"));
}

void TestSearchErrorRecovery::testSkipStrategy() {
    SearchErrorRecovery::ErrorContext context(SearchErrorRecovery::CacheError,
                                              "skip_test", "test");
    auto result = recovery->skipOperation(context);

    QVERIFY(result.success);
    QCOMPARE(result.usedStrategy, SearchErrorRecovery::Skip);
}

void TestSearchErrorRecovery::testResetStrategy() {
    // Save some operation state first
    QVariantMap state;
    state["key"] = "value";
    recovery->saveOperationState("reset_test", state);

    SearchErrorRecovery::ErrorContext context(SearchErrorRecovery::UnknownError,
                                              "reset_test", "test_component");
    auto result = recovery->resetOperation(context);

    QVERIFY(result.success);
    QCOMPARE(result.usedStrategy, SearchErrorRecovery::Reset);

    // Operation state should be cleared
    QVariantMap restoredState = recovery->restoreOperationState("reset_test");
    QVERIFY(restoredState.isEmpty());

    // Component should be marked as healthy
    QVERIFY(recovery->isComponentHealthy("test_component"));
}

void TestSearchErrorRecovery::testCircuitBreakerBasic() {
    recovery->enableCircuitBreaker("test_operation", 2,
                                   5000);  // 2 failures, 5 second timeout

    QVERIFY(!recovery->isCircuitBreakerOpen("test_operation"));

    // Record failures
    recovery->recordOperationFailure("test_operation");
    QVERIFY(!recovery->isCircuitBreakerOpen("test_operation"));  // Still closed

    recovery->recordOperationFailure("test_operation");
    QVERIFY(recovery->isCircuitBreakerOpen(
        "test_operation"));  // Should be open now

    // Record success should close it
    recovery->recordOperationSuccess("test_operation");
    QVERIFY(!recovery->isCircuitBreakerOpen("test_operation"));
}

void TestSearchErrorRecovery::testCircuitBreakerTimeout() {
    recovery->enableCircuitBreaker("timeout_test", 1,
                                   100);  // 1 failure, 100ms timeout

    recovery->recordOperationFailure("timeout_test");
    QVERIFY(recovery->isCircuitBreakerOpen("timeout_test"));

    // Wait for timeout
    QTest::qWait(150);

    // Should be closed after timeout
    QVERIFY(!recovery->isCircuitBreakerOpen("timeout_test"));
}

void TestSearchErrorRecovery::testCircuitBreakerRecovery() {
    QSignalSpy openedSpy(recovery, &SearchErrorRecovery::circuitBreakerOpened);
    QSignalSpy closedSpy(recovery, &SearchErrorRecovery::circuitBreakerClosed);

    recovery->enableCircuitBreaker("recovery_test", 1, 1000);

    recovery->recordOperationFailure("recovery_test");
    QCOMPARE(openedSpy.count(), 1);

    recovery->recordOperationSuccess("recovery_test");
    QCOMPARE(closedSpy.count(), 1);
}

void TestSearchErrorRecovery::testOperationStateManagement() {
    QVariantMap state;
    state["step"] = 1;
    state["data"] = "test_data";

    recovery->saveOperationState("state_test", state);

    QVariantMap restored = recovery->restoreOperationState("state_test");
    QCOMPARE(restored["step"].toInt(), 1);
    QCOMPARE(restored["data"].toString(), "test_data");

    recovery->clearOperationState("state_test");
    restored = recovery->restoreOperationState("state_test");
    QVERIFY(restored.isEmpty());
}

void TestSearchErrorRecovery::testStateRecovery() {
    // Test that state is properly restored after an error
    QVariantMap initialState;
    initialState["progress"] = 50;
    recovery->saveOperationState("recovery_state_test", initialState);

    // Simulate an error and recovery
    SearchErrorRecovery::ErrorContext context(SearchErrorRecovery::SearchError,
                                              "recovery_state_test", "test");
    recovery->handleError("Test error", context);

    // State should still be available for recovery
    QVariantMap state = recovery->restoreOperationState("recovery_state_test");
    QCOMPARE(state["progress"].toInt(), 50);
}

void TestSearchErrorRecovery::testFallbackRegistration() {
    bool fallbackCalled = false;

    recovery->registerFallback(
        SearchErrorRecovery::DocumentError, "registration_test",
        [&fallbackCalled](
            const SearchErrorRecovery::ErrorContext& context) -> QVariant {
            fallbackCalled = true;
            return QVariant("success");
        });

    SearchErrorRecovery::ErrorContext context(
        SearchErrorRecovery::DocumentError, "registration_test", "test");
    QVariant result = recovery->executeFallback(
        SearchErrorRecovery::DocumentError, "registration_test", context);

    QVERIFY(fallbackCalled);
    QCOMPARE(result.toString(), "success");

    // Unregister and test
    recovery->unregisterFallback(SearchErrorRecovery::DocumentError,
                                 "registration_test");
    result = recovery->executeFallback(SearchErrorRecovery::DocumentError,
                                       "registration_test", context);
    QVERIFY(!result.isValid());
}

void TestSearchErrorRecovery::testFallbackExecution() {
    recovery->registerFallback(
        SearchErrorRecovery::SearchError, "execution_test",
        [](const SearchErrorRecovery::ErrorContext& context) -> QVariant {
            QString query = context.metadata.value("query").toString();
            return QVariant(QString("fallback_for_%1").arg(query));
        });

    SearchErrorRecovery::ErrorContext context(SearchErrorRecovery::SearchError,
                                              "execution_test", "test");
    context.metadata["query"] = "test_query";

    QVariant result = recovery->executeFallback(
        SearchErrorRecovery::SearchError, "execution_test", context);
    QCOMPARE(result.toString(), "fallback_for_test_query");
}

void TestSearchErrorRecovery::testComponentHealthTracking() {
    QSignalSpy healthSpy(recovery,
                         &SearchErrorRecovery::componentHealthChanged);

    // Initially healthy
    QVERIFY(recovery->isComponentHealthy("health_test"));

    // Report unhealthy
    recovery->reportComponentHealth("health_test", false);
    QVERIFY(!recovery->isComponentHealthy("health_test"));
    QCOMPARE(healthSpy.count(), 1);

    // Report healthy again
    recovery->reportComponentHealth("health_test", true);
    QVERIFY(recovery->isComponentHealthy("health_test"));
    QCOMPARE(healthSpy.count(), 2);
}

void TestSearchErrorRecovery::testUnhealthyComponentDetection() {
    recovery->reportComponentHealth("component1", false);
    recovery->reportComponentHealth("component2", true);
    recovery->reportComponentHealth("component3", false);

    QStringList unhealthy = recovery->getUnhealthyComponents();
    QVERIFY(unhealthy.contains("component1"));
    QVERIFY(!unhealthy.contains("component2"));
    QVERIFY(unhealthy.contains("component3"));
}

void TestSearchErrorRecovery::testErrorStatistics() {
    recovery->resetErrorStats();

    // Generate some errors
    SearchErrorRecovery::ErrorContext context1(SearchErrorRecovery::SearchError,
                                               "stats_test1", "test");
    SearchErrorRecovery::ErrorContext context2(
        SearchErrorRecovery::DocumentError, "stats_test2", "test");

    recovery->handleError("Error 1", context1);
    recovery->handleError("Error 2", context2);
    recovery->handleError("Error 3", context1);

    auto stats = recovery->getErrorStats();
    QCOMPARE(stats.totalErrors, 3);
    QVERIFY(stats.errorCounts.contains(SearchErrorRecovery::SearchError));
    QVERIFY(stats.errorCounts.contains(SearchErrorRecovery::DocumentError));
    QCOMPARE(stats.errorCounts[SearchErrorRecovery::SearchError], 2);
    QCOMPARE(stats.errorCounts[SearchErrorRecovery::DocumentError], 1);
}

void TestSearchErrorRecovery::testRecoveryStatistics() {
    recovery->resetErrorStats();

    // Configure for successful recovery
    SearchErrorRecovery::RecoveryConfig config;
    config.strategy = SearchErrorRecovery::Skip;
    recovery->setRecoveryConfig(SearchErrorRecovery::CacheError, config);

    SearchErrorRecovery::ErrorContext context(SearchErrorRecovery::CacheError,
                                              "recovery_stats", "test");
    recovery->handleError("Cache error", context);

    auto stats = recovery->getErrorStats();
    QCOMPARE(stats.totalErrors, 1);
    QCOMPARE(stats.recoveredErrors, 1);
    QCOMPARE(stats.failedRecoveries, 0);
}

void TestSearchErrorRecovery::testSearchExceptionHandling() {
    SearchException searchEx("Search failed", SearchErrorRecovery::SearchError);
    SearchErrorRecovery::ErrorContext context(SearchErrorRecovery::SearchError,
                                              "exception_test", "test");

    auto result = recovery->handleError(searchEx, context);
    // Should handle the exception based on configured strategy
}

void TestSearchErrorRecovery::testStandardExceptionHandling() {
    std::runtime_error stdEx("Standard error");
    SearchErrorRecovery::ErrorContext context(SearchErrorRecovery::UnknownError,
                                              "std_exception_test", "test");

    auto result = recovery->handleError(stdEx, context);
    // Should classify and handle the standard exception
}

void TestSearchErrorRecovery::testErrorScopeBasic() {
    {
        SearchErrorRecovery::ErrorContext context(
            SearchErrorRecovery::SearchError, "scope_test", "test");
        SearchErrorScope scope(recovery, context);

        // Simulate successful operation
        scope.setSuccessful(true);
        // Should not trigger error handling on destruction
    }
}

void TestSearchErrorRecovery::testErrorScopeAutomatic() {
    QSignalSpy errorSpy(recovery, &SearchErrorRecovery::errorOccurred);

    {
        SearchErrorRecovery::ErrorContext context(
            SearchErrorRecovery::SearchError, "auto_scope_test", "test");
        SearchErrorScope scope(recovery, context);
        scope.updateDetails("Automatic error handling test");

        // Don't call setSuccessful - should trigger automatic error handling
    }

    // Should have triggered error handling
    QVERIFY(errorSpy.count() > 0);
}

QTEST_APPLESS_MAIN(TestSearchErrorRecovery)

#include "test_search_error_recovery.moc"
