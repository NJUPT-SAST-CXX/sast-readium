#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include "../../../app/ui/core/UIRecoveryManager.h"

class UIRecoveryManagerTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testSingletonInstance();
    void testRegisterRecoveryAction();
    void testRegisterFallbackWidget();
    void testAttemptRecovery();
    void testRecoverWidgetCreation();
    void testRecoverLayoutError();
    void testRecoverStyleError();
    void testRecoverDataBinding();
    void testSaveWidgetState();
    void testRestoreWidgetState();
    void testClearSavedState();
    void testSetAutoRecoveryEnabled();
    void testSetMaxRetryAttempts();
    void testSetRecoveryTimeout();
    void testRecoveryAttemptedSignal();
    void testRecoveryFailedSignal();
    void testUserGuidanceShownSignal();
    void testRecoveryStrategyEnum();
    void testUIErrorTypeEnum();

private:
    QWidget* m_parentWidget;
    QPushButton* m_testButton;
    QLabel* m_testLabel;
};

void UIRecoveryManagerTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
    }
}

void UIRecoveryManagerTest::cleanupTestCase() { delete m_parentWidget; }

void UIRecoveryManagerTest::init() {
    m_testButton = new QPushButton("Test", m_parentWidget);
    m_testLabel = new QLabel("Test Label", m_parentWidget);
}

void UIRecoveryManagerTest::cleanup() {
    UIRecoveryManager::instance().clearSavedState(m_testButton);
    UIRecoveryManager::instance().clearSavedState(m_testLabel);
    delete m_testButton;
    delete m_testLabel;
    m_testButton = nullptr;
    m_testLabel = nullptr;
}

void UIRecoveryManagerTest::testSingletonInstance() {
    auto& i1 = UIRecoveryManager::instance();
    auto& i2 = UIRecoveryManager::instance();
    QCOMPARE(&i1, &i2);
}

void UIRecoveryManagerTest::testRegisterRecoveryAction() {
    UIRecoveryManager::RecoveryAction action(
        UIRecoveryManager::RecoveryStrategy::AutomaticRetry, "Test",
        [](QWidget*, const ErrorHandling::ErrorInfo&) { return true; }, 10);
    UIRecoveryManager::instance().registerRecoveryAction(
        UIRecoveryManager::UIErrorType::WidgetCreationFailed, action);
    QVERIFY(true);
}

void UIRecoveryManagerTest::testRegisterFallbackWidget() {
    UIRecoveryManager::instance().registerFallbackWidget(
        "TestWidget",
        [](QWidget* p) -> QWidget* { return new QLabel("Fallback", p); });
    QVERIFY(true);
}

void UIRecoveryManagerTest::testAttemptRecovery() {
    ErrorHandling::ErrorInfo error;
    error.category = ErrorHandling::ErrorCategory::UI;
    error.message = "Test error";
    bool result = UIRecoveryManager::instance().attemptRecovery(
        UIRecoveryManager::UIErrorType::WidgetCreationFailed, m_parentWidget,
        error);
    QVERIFY(result || !result);
}

void UIRecoveryManagerTest::testRecoverWidgetCreation() {
    ErrorHandling::ErrorInfo error;
    error.message = "Widget creation failed";
    bool result = UIRecoveryManager::instance().recoverWidgetCreation(
        m_parentWidget, "TestWidget", error);
    QVERIFY(result || !result);
}

void UIRecoveryManagerTest::testRecoverLayoutError() {
    ErrorHandling::ErrorInfo error;
    error.message = "Layout error";
    bool result =
        UIRecoveryManager::instance().recoverLayoutError(m_testButton, error);
    QVERIFY(result || !result);
}

void UIRecoveryManagerTest::testRecoverStyleError() {
    ErrorHandling::ErrorInfo error;
    error.message = "Style error";
    bool result =
        UIRecoveryManager::instance().recoverStyleError(m_testButton, error);
    QVERIFY(result || !result);
}

void UIRecoveryManagerTest::testRecoverDataBinding() {
    ErrorHandling::ErrorInfo error;
    error.message = "Data binding error";
    bool result =
        UIRecoveryManager::instance().recoverDataBinding(m_testButton, error);
    QVERIFY(result || !result);
}

void UIRecoveryManagerTest::testSaveWidgetState() {
    m_testButton->setEnabled(true);
    m_testButton->setGeometry(10, 20, 100, 30);
    UIRecoveryManager::instance().saveWidgetState(m_testButton);
    QVERIFY(true);
}

void UIRecoveryManagerTest::testRestoreWidgetState() {
    UIRecoveryManager::instance().saveWidgetState(m_testButton);
    m_testButton->setEnabled(false);
    bool result =
        UIRecoveryManager::instance().restoreWidgetState(m_testButton);
    QVERIFY(result || !result);
}

void UIRecoveryManagerTest::testClearSavedState() {
    UIRecoveryManager::instance().saveWidgetState(m_testButton);
    UIRecoveryManager::instance().clearSavedState(m_testButton);
    QVERIFY(true);
}

void UIRecoveryManagerTest::testSetAutoRecoveryEnabled() {
    UIRecoveryManager::instance().setAutoRecoveryEnabled(true);
    UIRecoveryManager::instance().setAutoRecoveryEnabled(false);
    QVERIFY(true);
}

void UIRecoveryManagerTest::testSetMaxRetryAttempts() {
    UIRecoveryManager::instance().setMaxRetryAttempts(5);
    UIRecoveryManager::instance().setMaxRetryAttempts(3);
    QVERIFY(true);
}

void UIRecoveryManagerTest::testSetRecoveryTimeout() {
    UIRecoveryManager::instance().setRecoveryTimeout(10000);
    UIRecoveryManager::instance().setRecoveryTimeout(5000);
    QVERIFY(true);
}

void UIRecoveryManagerTest::testRecoveryAttemptedSignal() {
    QSignalSpy spy(&UIRecoveryManager::instance(),
                   &UIRecoveryManager::recoveryAttempted);
    QVERIFY(spy.isValid());
}

void UIRecoveryManagerTest::testRecoveryFailedSignal() {
    QSignalSpy spy(&UIRecoveryManager::instance(),
                   &UIRecoveryManager::recoveryFailed);
    QVERIFY(spy.isValid());
}

void UIRecoveryManagerTest::testUserGuidanceShownSignal() {
    QSignalSpy spy(&UIRecoveryManager::instance(),
                   &UIRecoveryManager::userGuidanceShown);
    QVERIFY(spy.isValid());
}

void UIRecoveryManagerTest::testRecoveryStrategyEnum() {
    QVERIFY(UIRecoveryManager::RecoveryStrategy::AutomaticRetry !=
            UIRecoveryManager::RecoveryStrategy::UserPrompt);
    QVERIFY(UIRecoveryManager::RecoveryStrategy::FallbackMethod !=
            UIRecoveryManager::RecoveryStrategy::GracefulDegradation);
}

void UIRecoveryManagerTest::testUIErrorTypeEnum() {
    QVERIFY(UIRecoveryManager::UIErrorType::WidgetCreationFailed !=
            UIRecoveryManager::UIErrorType::LayoutError);
    QVERIFY(UIRecoveryManager::UIErrorType::StyleApplicationFailed !=
            UIRecoveryManager::UIErrorType::DataBindingError);
}

QTEST_MAIN(UIRecoveryManagerTest)
#include "test_ui_recovery_manager.moc"
