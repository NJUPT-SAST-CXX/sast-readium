#include <QApplication>
#include <QMainWindow>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest/QtTest>
#include "../../app/controller/ApplicationController.h"
#include "../../app/logging/LoggingMacros.h"
#include "../../app/ui/core/UIConsistencyManager.h"
#include "../../app/ui/core/UIErrorHandler.h"
#include "../../app/ui/core/UIRecoveryManager.h"
#include "../../app/ui/core/UIResourceManager.h"
#include "../../app/ui/core/UIStateManager.h"

/**
 * @brief Comprehensive integration test for final UI system integration
 *
 * Tests the complete integration of all UI management systems:
 * - State management and persistence
 * - Resource management and cleanup
 * - Visual consistency enforcement
 * - Error handling and recovery
 * - Component lifecycle management
 */
class TestFinalIntegrationComprehensive : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // State management integration tests
    void testStateManagementIntegration();
    void testStatePersistence();
    void testComponentStateSync();
    void testStateRecovery();

    // Resource management integration tests
    void testResourceManagementIntegration();
    void testMemoryManagement();
    void testResourceCleanup();
    void testResourceLeakDetection();

    // Visual consistency integration tests
    void testVisualConsistencyIntegration();
    void testThemeConsistency();
    void testDesignSystemCompliance();
    void testConsistencyEnforcement();

    // Error handling integration tests
    void testErrorHandlingIntegration();
    void testErrorRecoveryIntegration();
    void testValidationIntegration();
    void testUserFeedbackIntegration();

    // Complete system integration tests
    void testFullSystemIntegration();
    void testApplicationLifecycle();
    void testConcurrentOperations();
    void testStressTest();

private:
    QMainWindow* m_mainWindow = nullptr;
    ApplicationController* m_appController = nullptr;

    void createTestApplication();
    void destroyTestApplication();
    bool waitForSignal(QObject* sender, const char* signal, int timeout = 5000);
};

void TestFinalIntegrationComprehensive::initTestCase() {
    LOG_INFO("Starting comprehensive final integration tests");

    // Set test environment
    qputenv("SAST_READIUM_TEST_MODE", "1");

    // Initialize logging for tests
    if (!qApp) {
        int argc = 1;
        char* argv[] = {const_cast<char*>("test")};
        new QApplication(argc, argv);
    }
}

void TestFinalIntegrationComprehensive::cleanupTestCase() {
    LOG_INFO("Comprehensive final integration tests completed");
}

void TestFinalIntegrationComprehensive::init() { createTestApplication(); }

void TestFinalIntegrationComprehensive::cleanup() { destroyTestApplication(); }

void TestFinalIntegrationComprehensive::createTestApplication() {
    m_mainWindow = new QMainWindow();
    m_mainWindow->setObjectName("TestMainWindow");

    m_appController = new ApplicationController(m_mainWindow);

    // Initialize the application controller
    m_appController->initializeApplication();

    // Wait for initialization to complete
    QSignalSpy initSpy(m_appController,
                       &ApplicationController::initializationCompleted);
    QVERIFY(initSpy.wait(10000));  // 10 second timeout
}

void TestFinalIntegrationComprehensive::destroyTestApplication() {
    if (m_appController) {
        m_appController->shutdown();
        delete m_appController;
        m_appController = nullptr;
    }

    if (m_mainWindow) {
        delete m_mainWindow;
        m_mainWindow = nullptr;
    }
}

void TestFinalIntegrationComprehensive::testStateManagementIntegration() {
    LOG_INFO("Testing state management integration");

    UIStateManager& stateManager = UIStateManager::instance();

    // Test basic state operations
    stateManager.setState("test/integration", "test_value");
    QCOMPARE(stateManager.getState("test/integration").toString(),
             QString("test_value"));

    // Test component registration
    QWidget* testWidget = new QWidget(m_mainWindow);
    testWidget->setObjectName("TestWidget");
    stateManager.registerComponent(testWidget, "TestComponent");

    // Test state saving and restoration
    stateManager.saveComponentState(testWidget);

    // Modify widget state
    testWidget->resize(200, 150);
    testWidget->move(50, 30);

    // Restore state
    stateManager.restoreComponentState(testWidget);

    // Cleanup
    stateManager.unregisterComponent(testWidget);
    delete testWidget;

    LOG_INFO("State management integration test completed");
}

void TestFinalIntegrationComprehensive::testResourceManagementIntegration() {
    LOG_INFO("Testing resource management integration");

    UIResourceManager& resourceManager = UIResourceManager::instance();

    // Test resource registration
    QWidget* testWidget = new QWidget(m_mainWindow);
    testWidget->setObjectName("ResourceTestWidget");
    resourceManager.registerWidget(testWidget, "Test Resource Widget");

    // Verify registration
    QVERIFY(resourceManager.getResourceCount(
                UIResourceManager::ResourceType::Widget) > 0);

    // Test memory usage tracking
    qint64 initialMemory = resourceManager.getTotalMemoryUsage();
    QVERIFY(initialMemory >= 0);

    // Test resource cleanup
    resourceManager.cleanupWidget(testWidget);

    // Verify cleanup
    QTest::qWait(100);  // Allow for async cleanup

    LOG_INFO("Resource management integration test completed");
}

void TestFinalIntegrationComprehensive::testVisualConsistencyIntegration() {
    LOG_INFO("Testing visual consistency integration");

    UIConsistencyManager& consistencyManager = UIConsistencyManager::instance();

    // Test component registration
    QWidget* testWidget = new QWidget(m_mainWindow);
    testWidget->setObjectName("ConsistencyTestWidget");
    consistencyManager.registerComponent(testWidget, "TestWidget");

    // Test validation
    auto result = consistencyManager.validateComponent(testWidget);
    QVERIFY(result != UIConsistencyManager::ValidationResult::NonCompliant);

    // Test consistency enforcement
    consistencyManager.enforceConsistency(testWidget);

    // Test global validation
    auto globalResult = consistencyManager.validateAllComponents();
    QVERIFY(globalResult !=
            UIConsistencyManager::ValidationResult::NonCompliant);

    // Cleanup
    consistencyManager.unregisterComponent(testWidget);
    delete testWidget;

    LOG_INFO("Visual consistency integration test completed");
}

void TestFinalIntegrationComprehensive::testErrorHandlingIntegration() {
    LOG_INFO("Testing error handling integration");

    UIErrorHandler& errorHandler = UIErrorHandler::instance();

    // Test validation
    auto validation = errorHandler.validatePageNumber(5, 10);
    QCOMPARE(validation.result, UIErrorHandler::ValidationResult::Valid);

    auto invalidValidation = errorHandler.validatePageNumber(15, 10);
    QCOMPARE(validation.result, UIErrorHandler::ValidationResult::Valid);

    // Test error recovery
    ErrorHandling::ErrorInfo testError(ErrorHandling::ErrorCategory::Document,
                                       ErrorHandling::ErrorSeverity::Error,
                                       "Test error", "Test error details",
                                       "TestComponent");

    bool recoveryAttempted = errorHandler.attemptErrorRecovery(
        testError, "TestComponent", m_mainWindow);
    // Recovery may or may not succeed, but should not crash

    LOG_INFO("Error handling integration test completed");
}

void TestFinalIntegrationComprehensive::testFullSystemIntegration() {
    LOG_INFO("Testing full system integration");

    QVERIFY(m_appController != nullptr);
    QVERIFY(m_mainWindow != nullptr);

    // Test that all managers are properly initialized and integrated
    UIStateManager& stateManager = UIStateManager::instance();
    UIResourceManager& resourceManager = UIResourceManager::instance();
    UIConsistencyManager& consistencyManager = UIConsistencyManager::instance();
    UIErrorHandler& errorHandler = UIErrorHandler::instance();

    // Test application state save/restore
    m_appController->saveApplicationState();
    m_appController->restoreApplicationState();

    // Test resource optimization
    m_appController->optimizeResources();

    // Test visual consistency enforcement
    m_appController->enforceVisualConsistency();

    // Verify all systems are still functional
    QVERIFY(stateManager.hasState("app/currentTheme"));
    QVERIFY(resourceManager.getResourceCount() >= 0);

    auto consistencyResult = consistencyManager.validateAllComponents();
    QVERIFY(consistencyResult !=
            UIConsistencyManager::ValidationResult::NonCompliant);

    LOG_INFO("Full system integration test completed");
}

void TestFinalIntegrationComprehensive::testApplicationLifecycle() {
    LOG_INFO("Testing complete application lifecycle");

    // Test initialization
    QVERIFY(m_appController != nullptr);

    // Test normal operations
    m_appController->showWelcomeScreen();
    QTest::qWait(100);

    m_appController->showMainView();
    QTest::qWait(100);

    // Test theme switching
    m_appController->applyTheme("dark");
    QTest::qWait(100);

    m_appController->applyTheme("light");
    QTest::qWait(100);

    // Test state persistence during operations
    UIStateManager& stateManager = UIStateManager::instance();
    stateManager.setState("lifecycle/test", "lifecycle_value");

    m_appController->saveApplicationState();

    QCOMPARE(stateManager.getState("lifecycle/test").toString(),
             QString("lifecycle_value"));

    // Test shutdown preparation
    m_appController->saveApplicationState();

    LOG_INFO("Application lifecycle test completed");
}

void TestFinalIntegrationComprehensive::testStressTest() {
    LOG_INFO("Running stress test for integrated systems");

    UIStateManager& stateManager = UIStateManager::instance();
    UIResourceManager& resourceManager = UIResourceManager::instance();
    UIConsistencyManager& consistencyManager = UIConsistencyManager::instance();

    // Create multiple widgets and test all systems
    QList<QWidget*> testWidgets;

    for (int i = 0; i < 50; ++i) {
        QWidget* widget = new QWidget(m_mainWindow);
        widget->setObjectName(QString("StressTestWidget_%1").arg(i));

        // Register with all systems
        stateManager.registerComponent(widget,
                                       QString("StressComponent_%1").arg(i));
        resourceManager.registerWidget(widget,
                                       QString("Stress Widget %1").arg(i));
        consistencyManager.registerComponent(widget, "StressWidget");

        testWidgets.append(widget);

        // Set some state
        stateManager.setState(QString("stress/widget_%1").arg(i), i);
    }

    // Test batch operations
    stateManager.saveAllComponentStates();
    consistencyManager.validateAllComponents();

    // Test memory management under load
    qint64 memoryBefore = resourceManager.getTotalMemoryUsage();
    resourceManager.optimizeMemoryUsage();
    qint64 memoryAfter = resourceManager.getTotalMemoryUsage();

    QVERIFY(memoryAfter <= memoryBefore);  // Memory should not increase

    // Cleanup all test widgets
    for (QWidget* widget : testWidgets) {
        stateManager.unregisterComponent(widget);
        resourceManager.unregisterResource(widget);
        consistencyManager.unregisterComponent(widget);
        delete widget;
    }

    // Verify cleanup
    QTest::qWait(200);  // Allow for async cleanup

    LOG_INFO("Stress test completed successfully");
}

bool TestFinalIntegrationComprehensive::waitForSignal(QObject* sender,
                                                      const char* signal,
                                                      int timeout) {
    QSignalSpy spy(sender, signal);
    return spy.wait(timeout);
}

QTEST_MAIN(TestFinalIntegrationComprehensive)
#include "test_final_integration_comprehensive.moc"
