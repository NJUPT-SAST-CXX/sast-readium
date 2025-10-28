#include <QSignalSpy>
#include <QtTest/QtTest>
#include "../../app/managers/StyleManager.h"
#include "../TestUtilities.h"

/**
 * @brief Comprehensive tests for StyleManager
 *
 * Tests theme management, stylesheet generation, color/font retrieval,
 * singleton pattern, and signal emissions.
 */
class StyleManagerTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Singleton tests
    void testSingletonInstance();
    void testSingletonConsistency();

    // Theme management tests
    void testSetLightTheme();
    void testSetDarkTheme();
    void testCurrentTheme();
    void testThemeChangedSignal();
    void testNoSignalOnSameTheme();

    // Stylesheet tests
    void testGetApplicationStyleSheet();
    void testGetToolbarStyleSheet();
    void testGetStatusBarStyleSheet();
    void testGetPDFViewerStyleSheet();
    void testGetButtonStyleSheet();
    void testGetScrollBarStyleSheet();
    void testGetQssStyleSheet();

    // Color tests
    void testPrimaryColor();
    void testBackgroundColor();
    void testTextColor();
    void testSemanticColors();
    void testColorConsistency();

    // Font tests
    void testDefaultFont();
    void testTitleFont();
    void testButtonFont();
    void testMonospaceFont();

    // Style creation tests
    void testCreateButtonStyle();
    void testCreateScrollBarStyle();
    void testCreateInputStyle();
    void testCreateCardStyle();

    // Spacing and sizing tests
    void testSpacingScale();
    void testBorderRadiusScale();
    void testAnimationDurations();
    void testShadowLevels();

    // Edge cases
    void testRapidThemeSwitching();
    void testThemePersistence();

private:
    StyleManager* m_manager;
};

void StyleManagerTest::initTestCase() {
    // Singleton instance will be created on first access
}

void StyleManagerTest::cleanupTestCase() {
    // Singleton cleanup handled automatically
}

void StyleManagerTest::init() { m_manager = &StyleManager::instance(); }

void StyleManagerTest::cleanup() {
    // Don't delete singleton
    m_manager = nullptr;
}

void StyleManagerTest::testSingletonInstance() {
    StyleManager& instance1 = StyleManager::instance();
    StyleManager& instance2 = StyleManager::instance();

    QCOMPARE(&instance1, &instance2);
}

void StyleManagerTest::testSingletonConsistency() {
    StyleManager& instance = StyleManager::instance();

    // Set a theme
    instance.setTheme(Theme::Dark);

    // Get instance again and verify state persists
    StyleManager& instance2 = StyleManager::instance();
    QCOMPARE(instance2.currentTheme(), Theme::Dark);
}

void StyleManagerTest::testSetLightTheme() {
    QSignalSpy spy(m_manager, &StyleManager::themeChanged);

    m_manager->setTheme(Theme::Light);

    QCOMPARE(m_manager->currentTheme(), Theme::Light);
    QVERIFY(spy.count() > 0);
}

void StyleManagerTest::testSetDarkTheme() {
    QSignalSpy spy(m_manager, &StyleManager::themeChanged);

    m_manager->setTheme(Theme::Dark);

    QCOMPARE(m_manager->currentTheme(), Theme::Dark);
    QVERIFY(spy.count() > 0);
}

void StyleManagerTest::testCurrentTheme() {
    m_manager->setTheme(Theme::Light);
    QCOMPARE(m_manager->currentTheme(), Theme::Light);

    m_manager->setTheme(Theme::Dark);
    QCOMPARE(m_manager->currentTheme(), Theme::Dark);
}

void StyleManagerTest::testThemeChangedSignal() {
    QSignalSpy spy(m_manager, &StyleManager::themeChanged);

    m_manager->setTheme(Theme::Light);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<Theme>(), Theme::Light);

    m_manager->setTheme(Theme::Dark);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(0).value<Theme>(), Theme::Dark);
}

void StyleManagerTest::testNoSignalOnSameTheme() {
    m_manager->setTheme(Theme::Light);

    QSignalSpy spy(m_manager, &StyleManager::themeChanged);

    // Set same theme again
    m_manager->setTheme(Theme::Light);

    // Should not emit signal if theme didn't change
    QCOMPARE(spy.count(), 0);
}

void StyleManagerTest::testGetApplicationStyleSheet() {
    QString stylesheet = m_manager->getApplicationStyleSheet();
    QVERIFY(!stylesheet.isEmpty());
}

void StyleManagerTest::testGetToolbarStyleSheet() {
    QString stylesheet = m_manager->getToolbarStyleSheet();
    QVERIFY(!stylesheet.isEmpty());
}

void StyleManagerTest::testGetStatusBarStyleSheet() {
    QString stylesheet = m_manager->getStatusBarStyleSheet();
    QVERIFY(!stylesheet.isEmpty());
}

void StyleManagerTest::testGetPDFViewerStyleSheet() {
    QString stylesheet = m_manager->getPDFViewerStyleSheet();
    QVERIFY(!stylesheet.isEmpty());
}

void StyleManagerTest::testGetButtonStyleSheet() {
    QString stylesheet = m_manager->getButtonStyleSheet();
    QVERIFY(!stylesheet.isEmpty());
}

void StyleManagerTest::testGetScrollBarStyleSheet() {
    QString stylesheet = m_manager->getScrollBarStyleSheet();
    QVERIFY(!stylesheet.isEmpty());
}

void StyleManagerTest::testGetQssStyleSheet() {
    QString stylesheet = m_manager->getQssStyleSheet();
    QVERIFY(!stylesheet.isEmpty());
}

void StyleManagerTest::testPrimaryColor() {
    QColor color = m_manager->primaryColor();
    QVERIFY(color.isValid());
}

void StyleManagerTest::testBackgroundColor() {
    m_manager->setTheme(Theme::Light);
    QColor lightBg = m_manager->backgroundColor();
    QVERIFY(lightBg.isValid());

    m_manager->setTheme(Theme::Dark);
    QColor darkBg = m_manager->backgroundColor();
    QVERIFY(darkBg.isValid());

    // Light and dark backgrounds should be different
    QVERIFY(lightBg != darkBg);
}

void StyleManagerTest::testTextColor() {
    m_manager->setTheme(Theme::Light);
    QColor lightText = m_manager->textColor();
    QVERIFY(lightText.isValid());

    m_manager->setTheme(Theme::Dark);
    QColor darkText = m_manager->textColor();
    QVERIFY(darkText.isValid());

    // Light and dark text colors should be different
    QVERIFY(lightText != darkText);
}

void StyleManagerTest::testSemanticColors() {
    QColor success = m_manager->successColor();
    QColor warning = m_manager->warningColor();
    QColor error = m_manager->errorColor();
    QColor info = m_manager->infoColor();

    QVERIFY(success.isValid());
    QVERIFY(warning.isValid());
    QVERIFY(error.isValid());
    QVERIFY(info.isValid());

    // Semantic colors should be distinct
    QVERIFY(success != warning);
    QVERIFY(warning != error);
    QVERIFY(error != info);
}

void StyleManagerTest::testColorConsistency() {
    // All color getters should return valid colors
    QVERIFY(m_manager->primaryColor().isValid());
    QVERIFY(m_manager->secondaryColor().isValid());
    QVERIFY(m_manager->surfaceColor().isValid());
    QVERIFY(m_manager->borderColor().isValid());
    QVERIFY(m_manager->hoverColor().isValid());
    QVERIFY(m_manager->accentColor().isValid());
}

void StyleManagerTest::testDefaultFont() {
    QFont font = m_manager->defaultFont();
    QVERIFY(!font.family().isEmpty());
}

void StyleManagerTest::testTitleFont() {
    QFont font = m_manager->titleFont();
    QVERIFY(!font.family().isEmpty());
}

void StyleManagerTest::testButtonFont() {
    QFont font = m_manager->buttonFont();
    QVERIFY(!font.family().isEmpty());
}

void StyleManagerTest::testMonospaceFont() {
    QFont font = m_manager->monospaceFont();
    QVERIFY(!font.family().isEmpty());
    QVERIFY(font.fixedPitch() || font.styleHint() == QFont::Monospace);
}

void StyleManagerTest::testCreateButtonStyle() {
    QString style = m_manager->createButtonStyle();
    QVERIFY(!style.isEmpty());
}

void StyleManagerTest::testCreateScrollBarStyle() {
    QString style = m_manager->createScrollBarStyle();
    QVERIFY(!style.isEmpty());
}

void StyleManagerTest::testCreateInputStyle() {
    QString style = m_manager->createInputStyle();
    QVERIFY(!style.isEmpty());
}

void StyleManagerTest::testCreateCardStyle() {
    QString style = m_manager->createCardStyle();
    QVERIFY(!style.isEmpty());
}

void StyleManagerTest::testSpacingScale() {
    // Test 8pt grid system
    QCOMPARE(m_manager->spacingXS(), 4);
    QCOMPARE(m_manager->spacingSM(), 8);
    QCOMPARE(m_manager->spacingMD(), 16);
    QCOMPARE(m_manager->spacingLG(), 24);
    QCOMPARE(m_manager->spacingXL(), 32);
    QCOMPARE(m_manager->spacingXXL(), 48);
}

void StyleManagerTest::testBorderRadiusScale() {
    QCOMPARE(m_manager->radiusSM(), 4);
    QCOMPARE(m_manager->radiusMD(), 6);
    QCOMPARE(m_manager->radiusLG(), 8);
    QCOMPARE(m_manager->radiusXL(), 12);
    QCOMPARE(m_manager->radiusFull(), 9999);
}

void StyleManagerTest::testAnimationDurations() {
    QCOMPARE(m_manager->animationFast(), 150);
    QCOMPARE(m_manager->animationNormal(), 250);
    QCOMPARE(m_manager->animationSlow(), 400);
}

void StyleManagerTest::testShadowLevels() {
    QVERIFY(!m_manager->shadowSM().isEmpty());
    QVERIFY(!m_manager->shadowMD().isEmpty());
    QVERIFY(!m_manager->shadowLG().isEmpty());
    QVERIFY(!m_manager->shadowXL().isEmpty());
}

void StyleManagerTest::testRapidThemeSwitching() {
    // Test rapid switching doesn't cause issues
    for (int i = 0; i < 10; ++i) {
        m_manager->setTheme(Theme::Light);
        m_manager->setTheme(Theme::Dark);
    }

    // Should still be in valid state
    Theme theme = m_manager->currentTheme();
    QVERIFY(theme == Theme::Light || theme == Theme::Dark);
}

void StyleManagerTest::testThemePersistence() {
    m_manager->setTheme(Theme::Dark);

    // Get instance again
    StyleManager& instance = StyleManager::instance();

    // Theme should persist
    QCOMPARE(instance.currentTheme(), Theme::Dark);
}

QTEST_MAIN(StyleManagerTest)
#include "test_style_manager.moc"
