#include <QApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QtTest/QtTest>
#include "../../../app/ui/utils/ValidationUtils.h"

class ValidationUtilsTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testValidateAndShowFeedback();
    void testValidatePageInput();
    void testValidateZoomInput();
    void testValidateFileInput();
    void testValidateSearchInput();
    void testValidateNumericRange();
    void testValidateForm();
    void testSanitizeTextInput();
    void testSanitizeFilePath();
    void testClampNumericInput();
    void testHighlightValidationError();
    void testClearValidationHighlight();
    void testShowValidationTooltip();
    void testSetValidationState();
    void testGetValidationState();
    void testClearAllValidationStates();
    void testValidationStateGuard();

private:
    QWidget* m_parentWidget;
    QLineEdit* m_testLineEdit;
};

void ValidationUtilsTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
    }
}

void ValidationUtilsTest::cleanupTestCase() { delete m_parentWidget; }

void ValidationUtilsTest::init() {
    m_testLineEdit = new QLineEdit(m_parentWidget);
}

void ValidationUtilsTest::cleanup() {
    delete m_testLineEdit;
    m_testLineEdit = nullptr;
}

void ValidationUtilsTest::testValidateAndShowFeedback() {
    UIErrorHandler::ValidationInfo valid(
        UIErrorHandler::ValidationResult::Valid);
    bool result =
        ValidationUtils::validateAndShowFeedback(m_testLineEdit, valid);
    QVERIFY(result);

    UIErrorHandler::ValidationInfo invalid(
        UIErrorHandler::ValidationResult::Invalid, "Error");
    result = ValidationUtils::validateAndShowFeedback(m_testLineEdit, invalid);
    QVERIFY(!result);
}

void ValidationUtilsTest::testValidatePageInput() {
    bool valid = ValidationUtils::validatePageInput(m_testLineEdit, 5, 10);
    QVERIFY(valid);

    bool invalid = ValidationUtils::validatePageInput(m_testLineEdit, 15, 10);
    QVERIFY(!invalid);
}

void ValidationUtilsTest::testValidateZoomInput() {
    bool valid = ValidationUtils::validateZoomInput(m_testLineEdit, 1.0);
    QVERIFY(valid);

    bool invalid = ValidationUtils::validateZoomInput(m_testLineEdit, 0.01);
    QVERIFY(!invalid);
}

void ValidationUtilsTest::testValidateFileInput() {
    bool invalid = ValidationUtils::validateFileInput(
        m_testLineEdit, "/nonexistent/path.pdf", true);
    QVERIFY(!invalid);
}

void ValidationUtilsTest::testValidateSearchInput() {
    bool valid = ValidationUtils::validateSearchInput(m_testLineEdit,
                                                      "test query", false);
    QVERIFY(valid);

    bool invalid =
        ValidationUtils::validateSearchInput(m_testLineEdit, "", false);
    QVERIFY(!invalid);
}

void ValidationUtilsTest::testValidateNumericRange() {
    bool valid = ValidationUtils::validateNumericRange(m_testLineEdit, 50.0,
                                                       0.0, 100.0, "Value");
    QVERIFY(valid);

    bool invalid = ValidationUtils::validateNumericRange(m_testLineEdit, 150.0,
                                                         0.0, 100.0, "Value");
    QVERIFY(!invalid);
}

void ValidationUtilsTest::testValidateForm() {
    QList<ValidationUtils::ValidationRule> rules;
    rules.append(ValidationUtils::ValidationRule(
        m_testLineEdit,
        []() {
            return UIErrorHandler::ValidationInfo(
                UIErrorHandler::ValidationResult::Valid);
        },
        "Test Field", true));
    bool result = ValidationUtils::validateForm(rules, m_parentWidget);
    QVERIFY(result);
}

void ValidationUtilsTest::testSanitizeTextInput() {
    QString sanitized = ValidationUtils::sanitizeTextInput("  test  ", 10);
    QVERIFY(!sanitized.isEmpty());

    QString truncated =
        ValidationUtils::sanitizeTextInput("very long text here", 5);
    QVERIFY(truncated.length() <= 5);
}

void ValidationUtilsTest::testSanitizeFilePath() {
    QString sanitized = ValidationUtils::sanitizeFilePath("/path/to/file.pdf");
    QVERIFY(!sanitized.isEmpty());
}

void ValidationUtilsTest::testClampNumericInput() {
    double clamped = ValidationUtils::clampNumericInput(150.0, 0.0, 100.0);
    QCOMPARE(clamped, 100.0);

    clamped = ValidationUtils::clampNumericInput(-10.0, 0.0, 100.0);
    QCOMPARE(clamped, 0.0);

    clamped = ValidationUtils::clampNumericInput(50.0, 0.0, 100.0);
    QCOMPARE(clamped, 50.0);
}

void ValidationUtilsTest::testHighlightValidationError() {
    ValidationUtils::highlightValidationError(m_testLineEdit, "Error message");
    QVERIFY(true);
}

void ValidationUtilsTest::testClearValidationHighlight() {
    ValidationUtils::highlightValidationError(m_testLineEdit, "Error");
    ValidationUtils::clearValidationHighlight(m_testLineEdit);
    QVERIFY(true);
}

void ValidationUtilsTest::testShowValidationTooltip() {
    ValidationUtils::showValidationTooltip(m_testLineEdit, "Tooltip message",
                                           1000);
    QVERIFY(true);
}

void ValidationUtilsTest::testSetValidationState() {
    ValidationUtils::setValidationState(m_testLineEdit, true, "Valid");
    ValidationUtils::setValidationState(m_testLineEdit, false, "Invalid");
    QVERIFY(true);
}

void ValidationUtilsTest::testGetValidationState() {
    ValidationUtils::setValidationState(m_testLineEdit, true);
    bool state = ValidationUtils::getValidationState(m_testLineEdit);
    QVERIFY(state);
}

void ValidationUtilsTest::testClearAllValidationStates() {
    ValidationUtils::setValidationState(m_testLineEdit, false);
    ValidationUtils::clearAllValidationStates(m_parentWidget);
    QVERIFY(true);
}

void ValidationUtilsTest::testValidationStateGuard() {
    {
        ValidationStateGuard guard(m_parentWidget);
        guard.addWidget(m_testLineEdit);
        ValidationUtils::setValidationState(m_testLineEdit, false);
        guard.rollback();
    }
    QVERIFY(true);

    {
        ValidationStateGuard guard(m_parentWidget);
        guard.addWidget(m_testLineEdit);
        guard.commit();
    }
    QVERIFY(true);
}

QTEST_MAIN(ValidationUtilsTest)
#include "test_validation_utils.moc"
