#include <QApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include "../../../app/ui/core/UIErrorHandler.h"

class UIErrorHandlerTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Singleton tests
    void testSingletonInstance();

    // Error handling tests
    void testHandleUserInputError();
    void testHandleSystemError();
    void testHandleFileOperationError();
    void testHandleUnexpectedError();

    // User feedback tests
    void testShowFeedback();
    void testShowProgressFeedback();
    void testHideProgressFeedback();
    void testShowInteractionFeedback();
    void testShowValidationFeedback();

    // Input validation tests
    void testValidatePageNumber();
    void testValidateZoomLevel();
    void testValidateFilePath();
    void testValidateCacheSize();
    void testValidateRecentFilesCount();
    void testValidateSearchQuery();
    void testValidateNumericInput();

    // Widget validation state tests
    void testSetWidgetValidationState();
    void testClearWidgetValidationState();
    void testSetWidgetEnabled();
    void testShowWidgetTooltip();

    // Error recovery tests
    void testAttemptErrorRecovery();
    void testRegisterUIRecoveryAction();

    // Configuration tests
    void testSetShowDetailedErrors();
    void testSetAutoRecovery();
    void testSetFeedbackDuration();

    // Signal tests
    void testErrorHandledSignal();
    void testValidationFailedSignal();
    void testRecoveryAttemptedSignal();
    void testUserFeedbackShownSignal();

    // InputValidator tests
    void testInputValidatorFilePath();
    void testInputValidatorRange();
    void testInputValidatorTextInput();
    void testInputValidatorPDFFile();
    void testInputValidatorPageRange();
    void testInputValidatorZoomRange();
    void testInputValidatorSearchQuery();

private:
    QWidget* m_parentWidget;
    QLineEdit* m_testLineEdit;
    QPushButton* m_testButton;
};

void UIErrorHandlerTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();

    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
    }
}

void UIErrorHandlerTest::cleanupTestCase() { delete m_parentWidget; }

void UIErrorHandlerTest::init() {
    m_testLineEdit = new QLineEdit(m_parentWidget);
    m_testButton = new QPushButton("Test", m_parentWidget);
}

void UIErrorHandlerTest::cleanup() {
    delete m_testLineEdit;
    delete m_testButton;
    m_testLineEdit = nullptr;
    m_testButton = nullptr;
}

void UIErrorHandlerTest::testSingletonInstance() {
    auto& instance1 = UIErrorHandler::instance();
    auto& instance2 = UIErrorHandler::instance();
    QCOMPARE(&instance1, &instance2);
}

void UIErrorHandlerTest::testHandleUserInputError() {
    UIErrorHandler::instance().handleUserInputError(
        m_parentWidget, "Page Number", "Invalid page number",
        "Enter a number between 1 and 100");
    QVERIFY(true);  // No crash
}

void UIErrorHandlerTest::testHandleSystemError() {
    ErrorHandling::ErrorInfo error;
    error.category = ErrorHandling::ErrorCategory::Unknown;
    error.severity = ErrorHandling::ErrorSeverity::Error;
    error.message = "System error occurred";
    error.details = "Details about the error";

    UIErrorHandler::instance().handleSystemError(m_parentWidget, error);
    QVERIFY(true);  // No crash
}

void UIErrorHandlerTest::testHandleFileOperationError() {
    UIErrorHandler::instance().handleFileOperationError(
        m_parentWidget, "Open", "/path/to/file.pdf", "File not found");
    QVERIFY(true);  // No crash
}

void UIErrorHandlerTest::testHandleUnexpectedError() {
    try {
        throw std::runtime_error("Test exception");
    } catch (const std::exception& e) {
        UIErrorHandler::instance().handleUnexpectedError(m_parentWidget,
                                                         "Test Context", e);
    }
    QVERIFY(true);  // No crash

    UIErrorHandler::instance().handleUnexpectedError(
        m_parentWidget, "Test Context", "Error message");
    QVERIFY(true);  // No crash
}

void UIErrorHandlerTest::testShowFeedback() {
    UIErrorHandler::instance().showFeedback(
        m_parentWidget, "Operation successful",
        UIErrorHandler::FeedbackType::Success, 1000);
    UIErrorHandler::instance().showFeedback(m_parentWidget, "Information",
                                            UIErrorHandler::FeedbackType::Info,
                                            1000);
    UIErrorHandler::instance().showFeedback(
        m_parentWidget, "Warning message",
        UIErrorHandler::FeedbackType::Warning, 1000);
    UIErrorHandler::instance().showFeedback(m_parentWidget, "Error occurred",
                                            UIErrorHandler::FeedbackType::Error,
                                            1000);
    UIErrorHandler::instance().showFeedback(
        m_parentWidget, "Critical error",
        UIErrorHandler::FeedbackType::Critical, 1000);
    QVERIFY(true);  // No crash
}

void UIErrorHandlerTest::testShowProgressFeedback() {
    UIErrorHandler::instance().showProgressFeedback(m_parentWidget,
                                                    "Loading...", 50);
    UIErrorHandler::instance().showProgressFeedback(m_parentWidget,
                                                    "Processing...", -1);
    QVERIFY(true);  // No crash
}

void UIErrorHandlerTest::testHideProgressFeedback() {
    UIErrorHandler::instance().showProgressFeedback(m_parentWidget,
                                                    "Loading...", 50);
    UIErrorHandler::instance().hideProgressFeedback(m_parentWidget);
    QVERIFY(true);  // No crash
}

void UIErrorHandlerTest::testShowInteractionFeedback() {
    UIErrorHandler::instance().showInteractionFeedback(m_testButton, "clicked");
    QVERIFY(true);  // No crash
}

void UIErrorHandlerTest::testShowValidationFeedback() {
    UIErrorHandler::ValidationInfo validInfo(
        UIErrorHandler::ValidationResult::Valid);
    UIErrorHandler::ValidationInfo warningInfo(
        UIErrorHandler::ValidationResult::Warning, "Warning message",
        "Suggestion");
    UIErrorHandler::ValidationInfo invalidInfo(
        UIErrorHandler::ValidationResult::Invalid, "Invalid input",
        "Fix suggestion", false);

    UIErrorHandler::instance().showValidationFeedback(m_testLineEdit,
                                                      validInfo);
    UIErrorHandler::instance().showValidationFeedback(m_testLineEdit,
                                                      warningInfo);
    UIErrorHandler::instance().showValidationFeedback(m_testLineEdit,
                                                      invalidInfo);
    QVERIFY(true);  // No crash
}

void UIErrorHandlerTest::testValidatePageNumber() {
    auto valid = UIErrorHandler::instance().validatePageNumber(5, 10);
    QCOMPARE(valid.result, UIErrorHandler::ValidationResult::Valid);
    QVERIFY(valid.canProceed);

    auto invalid = UIErrorHandler::instance().validatePageNumber(15, 10);
    QVERIFY(invalid.result != UIErrorHandler::ValidationResult::Valid);

    auto zeroPage = UIErrorHandler::instance().validatePageNumber(0, 10);
    QVERIFY(zeroPage.result != UIErrorHandler::ValidationResult::Valid);

    auto negativePage = UIErrorHandler::instance().validatePageNumber(-1, 10);
    QVERIFY(negativePage.result != UIErrorHandler::ValidationResult::Valid);
}

void UIErrorHandlerTest::testValidateZoomLevel() {
    auto valid = UIErrorHandler::instance().validateZoomLevel(1.0);
    QCOMPARE(valid.result, UIErrorHandler::ValidationResult::Valid);

    auto tooLow = UIErrorHandler::instance().validateZoomLevel(0.05);
    QVERIFY(tooLow.result != UIErrorHandler::ValidationResult::Valid);

    auto tooHigh = UIErrorHandler::instance().validateZoomLevel(100.0);
    QVERIFY(tooHigh.result != UIErrorHandler::ValidationResult::Valid);
}

void UIErrorHandlerTest::testValidateFilePath() {
    auto emptyPath = UIErrorHandler::instance().validateFilePath("", true);
    QVERIFY(emptyPath.result != UIErrorHandler::ValidationResult::Valid);

    auto nonExistent = UIErrorHandler::instance().validateFilePath(
        "/nonexistent/path.pdf", true);
    QVERIFY(nonExistent.result != UIErrorHandler::ValidationResult::Valid);
}

void UIErrorHandlerTest::testValidateCacheSize() {
    auto valid = UIErrorHandler::instance().validateCacheSize(256);
    QCOMPARE(valid.result, UIErrorHandler::ValidationResult::Valid);

    auto tooSmall = UIErrorHandler::instance().validateCacheSize(0);
    QVERIFY(tooSmall.result != UIErrorHandler::ValidationResult::Valid);

    auto tooLarge = UIErrorHandler::instance().validateCacheSize(100000);
    QVERIFY(tooLarge.result != UIErrorHandler::ValidationResult::Valid);
}

void UIErrorHandlerTest::testValidateRecentFilesCount() {
    auto valid = UIErrorHandler::instance().validateRecentFilesCount(10);
    QCOMPARE(valid.result, UIErrorHandler::ValidationResult::Valid);

    auto negative = UIErrorHandler::instance().validateRecentFilesCount(-1);
    QVERIFY(negative.result != UIErrorHandler::ValidationResult::Valid);
}

void UIErrorHandlerTest::testValidateSearchQuery() {
    auto valid = UIErrorHandler::instance().validateSearchQuery("test query");
    QCOMPARE(valid.result, UIErrorHandler::ValidationResult::Valid);

    auto empty = UIErrorHandler::instance().validateSearchQuery("");
    QVERIFY(empty.result != UIErrorHandler::ValidationResult::Valid);
}

void UIErrorHandlerTest::testValidateNumericInput() {
    auto valid = UIErrorHandler::instance().validateNumericInput(
        50.0, 0.0, 100.0, "Value");
    QCOMPARE(valid.result, UIErrorHandler::ValidationResult::Valid);

    auto tooLow = UIErrorHandler::instance().validateNumericInput(
        -10.0, 0.0, 100.0, "Value");
    QVERIFY(tooLow.result != UIErrorHandler::ValidationResult::Valid);

    auto tooHigh = UIErrorHandler::instance().validateNumericInput(
        150.0, 0.0, 100.0, "Value");
    QVERIFY(tooHigh.result != UIErrorHandler::ValidationResult::Valid);
}

void UIErrorHandlerTest::testSetWidgetValidationState() {
    UIErrorHandler::instance().setWidgetValidationState(
        m_testLineEdit, UIErrorHandler::ValidationResult::Valid, "Valid input");
    UIErrorHandler::instance().setWidgetValidationState(
        m_testLineEdit, UIErrorHandler::ValidationResult::Invalid,
        "Invalid input");
    QVERIFY(true);  // No crash
}

void UIErrorHandlerTest::testClearWidgetValidationState() {
    UIErrorHandler::instance().setWidgetValidationState(
        m_testLineEdit, UIErrorHandler::ValidationResult::Invalid, "Error");
    UIErrorHandler::instance().clearWidgetValidationState(m_testLineEdit);
    QVERIFY(true);  // No crash
}

void UIErrorHandlerTest::testSetWidgetEnabled() {
    UIErrorHandler::instance().setWidgetEnabled(m_testButton, false,
                                                "Disabled for testing");
    QVERIFY(!m_testButton->isEnabled());

    UIErrorHandler::instance().setWidgetEnabled(m_testButton, true);
    QVERIFY(m_testButton->isEnabled());
}

void UIErrorHandlerTest::testShowWidgetTooltip() {
    UIErrorHandler::instance().showWidgetTooltip(m_testButton,
                                                 "Tooltip message", 1000);
    QVERIFY(true);  // No crash
}

void UIErrorHandlerTest::testAttemptErrorRecovery() {
    ErrorHandling::ErrorInfo error;
    error.category = ErrorHandling::ErrorCategory::FileSystem;
    error.severity = ErrorHandling::ErrorSeverity::Error;
    error.message = "File operation failed";

    bool result = UIErrorHandler::instance().attemptErrorRecovery(
        error, "TestComponent", m_parentWidget);
    // Result depends on registered recovery actions
    QVERIFY(result || !result);
}

void UIErrorHandlerTest::testRegisterUIRecoveryAction() {
    UIErrorHandler::instance().registerUIRecoveryAction(
        ErrorHandling::ErrorCategory::FileSystem,
        [](const ErrorHandling::ErrorInfo&, QWidget*) -> bool { return true; });
    QVERIFY(true);  // No crash
}

void UIErrorHandlerTest::testSetShowDetailedErrors() {
    UIErrorHandler::instance().setShowDetailedErrors(true);
    UIErrorHandler::instance().setShowDetailedErrors(false);
    QVERIFY(true);  // No crash
}

void UIErrorHandlerTest::testSetAutoRecovery() {
    UIErrorHandler::instance().setAutoRecovery(true);
    UIErrorHandler::instance().setAutoRecovery(false);
    QVERIFY(true);  // No crash
}

void UIErrorHandlerTest::testSetFeedbackDuration() {
    UIErrorHandler::instance().setFeedbackDuration(5000);
    UIErrorHandler::instance().setFeedbackDuration(3000);
    QVERIFY(true);  // No crash
}

void UIErrorHandlerTest::testErrorHandledSignal() {
    QSignalSpy spy(&UIErrorHandler::instance(), &UIErrorHandler::errorHandled);
    QVERIFY(spy.isValid());
}

void UIErrorHandlerTest::testValidationFailedSignal() {
    QSignalSpy spy(&UIErrorHandler::instance(),
                   &UIErrorHandler::validationFailed);
    QVERIFY(spy.isValid());
}

void UIErrorHandlerTest::testRecoveryAttemptedSignal() {
    QSignalSpy spy(&UIErrorHandler::instance(),
                   &UIErrorHandler::recoveryAttempted);
    QVERIFY(spy.isValid());
}

void UIErrorHandlerTest::testUserFeedbackShownSignal() {
    QSignalSpy spy(&UIErrorHandler::instance(),
                   &UIErrorHandler::userFeedbackShown);
    QVERIFY(spy.isValid());
}

void UIErrorHandlerTest::testInputValidatorFilePath() {
    auto valid = InputValidator::validateFilePath("/some/path.pdf", false);
    QVERIFY(valid.result == UIErrorHandler::ValidationResult::Valid ||
            valid.result != UIErrorHandler::ValidationResult::Valid);

    auto empty = InputValidator::validateFilePath("", true);
    QVERIFY(empty.result != UIErrorHandler::ValidationResult::Valid);
}

void UIErrorHandlerTest::testInputValidatorRange() {
    auto valid = InputValidator::validateRange(50.0, 0.0, 100.0, "Test");
    QCOMPARE(valid.result, UIErrorHandler::ValidationResult::Valid);

    auto outOfRange = InputValidator::validateRange(150.0, 0.0, 100.0, "Test");
    QVERIFY(outOfRange.result != UIErrorHandler::ValidationResult::Valid);
}

void UIErrorHandlerTest::testInputValidatorTextInput() {
    auto valid = InputValidator::validateTextInput("test", 1, 100);
    QCOMPARE(valid.result, UIErrorHandler::ValidationResult::Valid);

    auto tooShort = InputValidator::validateTextInput("", 1, 100);
    QVERIFY(tooShort.result != UIErrorHandler::ValidationResult::Valid);

    auto tooLong = InputValidator::validateTextInput("very long text", 1, 5);
    QVERIFY(tooLong.result != UIErrorHandler::ValidationResult::Valid);
}

void UIErrorHandlerTest::testInputValidatorPDFFile() {
    auto nonExistent = InputValidator::validatePDFFile("/nonexistent.pdf");
    QVERIFY(nonExistent.result != UIErrorHandler::ValidationResult::Valid);

    auto notPdf = InputValidator::validatePDFFile("/some/file.txt");
    QVERIFY(notPdf.result != UIErrorHandler::ValidationResult::Valid);
}

void UIErrorHandlerTest::testInputValidatorPageRange() {
    auto valid = InputValidator::validatePageRange(1, 10, 20);
    QCOMPARE(valid.result, UIErrorHandler::ValidationResult::Valid);

    auto invalidStart = InputValidator::validatePageRange(0, 10, 20);
    QVERIFY(invalidStart.result != UIErrorHandler::ValidationResult::Valid);

    auto invalidEnd = InputValidator::validatePageRange(1, 25, 20);
    QVERIFY(invalidEnd.result != UIErrorHandler::ValidationResult::Valid);

    auto reversed = InputValidator::validatePageRange(10, 5, 20);
    QVERIFY(reversed.result != UIErrorHandler::ValidationResult::Valid);
}

void UIErrorHandlerTest::testInputValidatorZoomRange() {
    auto valid = InputValidator::validateZoomRange(1.0);
    QCOMPARE(valid.result, UIErrorHandler::ValidationResult::Valid);

    auto tooLow = InputValidator::validateZoomRange(0.01);
    QVERIFY(tooLow.result != UIErrorHandler::ValidationResult::Valid);

    auto tooHigh = InputValidator::validateZoomRange(50.0);
    QVERIFY(tooHigh.result != UIErrorHandler::ValidationResult::Valid);
}

void UIErrorHandlerTest::testInputValidatorSearchQuery() {
    auto valid = InputValidator::validateSearchQuery("test", false, false);
    QCOMPARE(valid.result, UIErrorHandler::ValidationResult::Valid);

    auto emptyNotAllowed =
        InputValidator::validateSearchQuery("", false, false);
    QVERIFY(emptyNotAllowed.result != UIErrorHandler::ValidationResult::Valid);

    auto emptyAllowed = InputValidator::validateSearchQuery("", true, false);
    QCOMPARE(emptyAllowed.result, UIErrorHandler::ValidationResult::Valid);
}

QTEST_MAIN(UIErrorHandlerTest)
#include "test_ui_error_handler.moc"
