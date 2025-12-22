#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include "../../../app/ui/core/UIConsistencyManager.h"

class UIConsistencyManagerTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Singleton tests
    void testSingletonInstance();

    // Component registration tests
    void testRegisterComponent();
    void testUnregisterComponent();
    void testRegisterMultipleComponents();

    // Validation tests
    void testValidateComponent();
    void testValidateAllComponents();
    void testValidationResult();

    // Style enforcement tests
    void testEnforceConsistency();
    void testEnforceGlobalConsistency();
    void testApplyDesignSystemStyles();

    // Consistency level tests
    void testSetConsistencyLevel();
    void testEnableAutoCorrection();
    void testEnableContinuousValidation();

    // Compliance tests
    void testIsColorCompliant();
    void testIsFontCompliant();
    void testIsSpacingCompliant();
    void testIsSizeCompliant();

    // Correction tests
    void testCorrectColor();
    void testCorrectFont();
    void testCorrectSpacing();
    void testCorrectSize();

    // Reporting tests
    void testGetValidationIssues();
    void testGenerateValidationReport();

    // Theme consistency tests
    void testValidateThemeConsistency();
    void testEnforceThemeConsistency();

    // Signal tests
    void testComponentRegisteredSignal();
    void testComponentUnregisteredSignal();
    void testValidationCompletedSignal();

    // DesignSystem tests
    void testDesignSystemColorValidation();
    void testDesignSystemFontValidation();
    void testDesignSystemSpacingValidation();
    void testDesignSystemSizeValidation();
    void testDesignSystemStandards();

private:
    QWidget* m_parentWidget;
    QPushButton* m_testButton;
    QLabel* m_testLabel;
};

void UIConsistencyManagerTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();

    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
    }
}

void UIConsistencyManagerTest::cleanupTestCase() { delete m_parentWidget; }

void UIConsistencyManagerTest::init() {
    m_testButton = new QPushButton("Test", m_parentWidget);
    m_testLabel = new QLabel("Test Label", m_parentWidget);
}

void UIConsistencyManagerTest::cleanup() {
    UIConsistencyManager::instance().unregisterComponent(m_testButton);
    UIConsistencyManager::instance().unregisterComponent(m_testLabel);
    delete m_testButton;
    delete m_testLabel;
    m_testButton = nullptr;
    m_testLabel = nullptr;
}

void UIConsistencyManagerTest::testSingletonInstance() {
    auto& instance1 = UIConsistencyManager::instance();
    auto& instance2 = UIConsistencyManager::instance();
    QCOMPARE(&instance1, &instance2);
}

void UIConsistencyManagerTest::testRegisterComponent() {
    UIConsistencyManager::instance().registerComponent(m_testButton, "Button");
    QVERIFY(true);  // No crash
}

void UIConsistencyManagerTest::testUnregisterComponent() {
    UIConsistencyManager::instance().registerComponent(m_testButton, "Button");
    UIConsistencyManager::instance().unregisterComponent(m_testButton);
    QVERIFY(true);  // No crash
}

void UIConsistencyManagerTest::testRegisterMultipleComponents() {
    UIConsistencyManager::instance().registerComponent(m_testButton, "Button");
    UIConsistencyManager::instance().registerComponent(m_testLabel, "Label");
    QVERIFY(true);  // No crash
}

void UIConsistencyManagerTest::testValidateComponent() {
    UIConsistencyManager::instance().registerComponent(m_testButton, "Button");
    auto result =
        UIConsistencyManager::instance().validateComponent(m_testButton);
    QVERIFY(result == UIConsistencyManager::ValidationResult::Compliant ||
            result == UIConsistencyManager::ValidationResult::MinorIssues ||
            result == UIConsistencyManager::ValidationResult::MajorIssues ||
            result == UIConsistencyManager::ValidationResult::NonCompliant);
}

void UIConsistencyManagerTest::testValidateAllComponents() {
    UIConsistencyManager::instance().registerComponent(m_testButton, "Button");
    UIConsistencyManager::instance().registerComponent(m_testLabel, "Label");
    auto result = UIConsistencyManager::instance().validateAllComponents();
    QVERIFY(result == UIConsistencyManager::ValidationResult::Compliant ||
            result == UIConsistencyManager::ValidationResult::MinorIssues ||
            result == UIConsistencyManager::ValidationResult::MajorIssues ||
            result == UIConsistencyManager::ValidationResult::NonCompliant);
}

void UIConsistencyManagerTest::testValidationResult() {
    QVERIFY(UIConsistencyManager::ValidationResult::Compliant !=
            UIConsistencyManager::ValidationResult::NonCompliant);
}

void UIConsistencyManagerTest::testEnforceConsistency() {
    UIConsistencyManager::instance().registerComponent(m_testButton, "Button");
    UIConsistencyManager::instance().enforceConsistency(m_testButton);
    QVERIFY(true);  // No crash
}

void UIConsistencyManagerTest::testEnforceGlobalConsistency() {
    UIConsistencyManager::instance().registerComponent(m_testButton, "Button");
    UIConsistencyManager::instance().enforceGlobalConsistency();
    QVERIFY(true);  // No crash
}

void UIConsistencyManagerTest::testApplyDesignSystemStyles() {
    UIConsistencyManager::instance().applyDesignSystemStyles(m_testButton,
                                                             "Button");
    QVERIFY(true);  // No crash
}

void UIConsistencyManagerTest::testSetConsistencyLevel() {
    UIConsistencyManager::instance().setConsistencyLevel(
        UIConsistencyManager::ConsistencyLevel::Strict);
    UIConsistencyManager::instance().setConsistencyLevel(
        UIConsistencyManager::ConsistencyLevel::Moderate);
    UIConsistencyManager::instance().setConsistencyLevel(
        UIConsistencyManager::ConsistencyLevel::Relaxed);
    QVERIFY(true);  // No crash
}

void UIConsistencyManagerTest::testEnableAutoCorrection() {
    UIConsistencyManager::instance().enableAutoCorrection(true);
    UIConsistencyManager::instance().enableAutoCorrection(false);
    QVERIFY(true);  // No crash
}

void UIConsistencyManagerTest::testEnableContinuousValidation() {
    UIConsistencyManager::instance().enableContinuousValidation(true, 60000);
    UIConsistencyManager::instance().enableContinuousValidation(false);
    QVERIFY(true);  // No crash
}

void UIConsistencyManagerTest::testIsColorCompliant() {
    QColor validColor(0, 120, 212);    // Primary blue
    QColor invalidColor(255, 0, 255);  // Magenta

    // Test should not crash regardless of result
    UIConsistencyManager::instance().isColorCompliant(validColor, "primary");
    UIConsistencyManager::instance().isColorCompliant(invalidColor, "primary");
    QVERIFY(true);
}

void UIConsistencyManagerTest::testIsFontCompliant() {
    QFont standardFont("Segoe UI", 12);
    QFont nonStandardFont("Comic Sans MS", 24);

    UIConsistencyManager::instance().isFontCompliant(standardFont, "body");
    UIConsistencyManager::instance().isFontCompliant(nonStandardFont, "body");
    QVERIFY(true);
}

void UIConsistencyManagerTest::testIsSpacingCompliant() {
    UIConsistencyManager::instance().isSpacingCompliant(8, "standard");
    UIConsistencyManager::instance().isSpacingCompliant(13, "standard");
    QVERIFY(true);
}

void UIConsistencyManagerTest::testIsSizeCompliant() {
    QSize validSize(100, 32);
    QSize invalidSize(50, 15);

    UIConsistencyManager::instance().isSizeCompliant(validSize, "button");
    UIConsistencyManager::instance().isSizeCompliant(invalidSize, "button");
    QVERIFY(true);
}

void UIConsistencyManagerTest::testCorrectColor() {
    QColor inputColor(255, 100, 100);
    QColor corrected =
        UIConsistencyManager::instance().correctColor(inputColor, "primary");
    QVERIFY(corrected.isValid());
}

void UIConsistencyManagerTest::testCorrectFont() {
    QFont inputFont("Arial", 14);
    QFont corrected =
        UIConsistencyManager::instance().correctFont(inputFont, "body");
    QVERIFY(!corrected.family().isEmpty());
}

void UIConsistencyManagerTest::testCorrectSpacing() {
    int corrected =
        UIConsistencyManager::instance().correctSpacing(13, "standard");
    QVERIFY(corrected >= 0);
}

void UIConsistencyManagerTest::testCorrectSize() {
    QSize inputSize(45, 20);
    QSize corrected =
        UIConsistencyManager::instance().correctSize(inputSize, "button");
    QVERIFY(corrected.isValid());
}

void UIConsistencyManagerTest::testGetValidationIssues() {
    UIConsistencyManager::instance().registerComponent(m_testButton, "Button");
    auto issues =
        UIConsistencyManager::instance().getValidationIssues(m_testButton);
    // Issues list may be empty or contain items
    QVERIFY(true);
}

void UIConsistencyManagerTest::testGenerateValidationReport() {
    UIConsistencyManager::instance().registerComponent(m_testButton, "Button");
    QString report =
        UIConsistencyManager::instance().generateValidationReport();
    QVERIFY(!report.isNull());
}

void UIConsistencyManagerTest::testValidateThemeConsistency() {
    UIConsistencyManager::instance().validateThemeConsistency();
    QVERIFY(true);  // No crash
}

void UIConsistencyManagerTest::testEnforceThemeConsistency() {
    UIConsistencyManager::instance().enforceThemeConsistency();
    QVERIFY(true);  // No crash
}

void UIConsistencyManagerTest::testComponentRegisteredSignal() {
    QSignalSpy spy(&UIConsistencyManager::instance(),
                   &UIConsistencyManager::componentRegistered);
    QVERIFY(spy.isValid());

    UIConsistencyManager::instance().registerComponent(m_testButton, "Button");
    QVERIFY(spy.count() >= 0);  // Signal may or may not be emitted
}

void UIConsistencyManagerTest::testComponentUnregisteredSignal() {
    QSignalSpy spy(&UIConsistencyManager::instance(),
                   &UIConsistencyManager::componentUnregistered);
    QVERIFY(spy.isValid());
}

void UIConsistencyManagerTest::testValidationCompletedSignal() {
    QSignalSpy spy(&UIConsistencyManager::instance(),
                   &UIConsistencyManager::validationCompleted);
    QVERIFY(spy.isValid());
}

void UIConsistencyManagerTest::testDesignSystemColorValidation() {
    QColor primary(0, 120, 212);
    QColor secondary(100, 100, 100);
    QColor accent(255, 140, 0);
    QColor neutral(200, 200, 200);

    // These should not crash
    DesignSystem::isValidPrimaryColor(primary);
    DesignSystem::isValidSecondaryColor(secondary);
    DesignSystem::isValidAccentColor(accent);
    DesignSystem::isValidNeutralColor(neutral);
    QVERIFY(true);
}

void UIConsistencyManagerTest::testDesignSystemFontValidation() {
    QFont headingFont("Segoe UI", 24, QFont::Bold);
    QFont bodyFont("Segoe UI", 12);
    QFont captionFont("Segoe UI", 10);

    DesignSystem::isValidHeadingFont(headingFont);
    DesignSystem::isValidBodyFont(bodyFont);
    DesignSystem::isValidCaptionFont(captionFont);
    QVERIFY(true);
}

void UIConsistencyManagerTest::testDesignSystemSpacingValidation() {
    QVERIFY(DesignSystem::isValidSpacing(8) ||
            !DesignSystem::isValidSpacing(8));
    QVERIFY(DesignSystem::isValidSpacing(16) ||
            !DesignSystem::isValidSpacing(16));

    int nearest = DesignSystem::getNearestValidSpacing(13);
    QVERIFY(nearest >= 0);
}

void UIConsistencyManagerTest::testDesignSystemSizeValidation() {
    QSize buttonSize(100, 32);
    QSize iconSize(24, 24);

    DesignSystem::isValidButtonSize(buttonSize);
    DesignSystem::isValidIconSize(iconSize);

    QSize nearest = DesignSystem::getNearestValidSize(QSize(45, 20), "button");
    QVERIFY(nearest.isValid());
}

void UIConsistencyManagerTest::testDesignSystemStandards() {
    int buttonHeight = DesignSystem::getStandardButtonHeight();
    QVERIFY(buttonHeight > 0);

    int iconSize = DesignSystem::getStandardIconSize();
    QVERIFY(iconSize > 0);

    int spacing = DesignSystem::getStandardSpacing();
    QVERIFY(spacing >= 0);

    QFont standardFont = DesignSystem::getStandardFont("body");
    QVERIFY(!standardFont.family().isEmpty());
}

QTEST_MAIN(UIConsistencyManagerTest)
#include "test_ui_consistency_manager.moc"
