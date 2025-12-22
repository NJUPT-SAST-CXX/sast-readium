#include <QColor>
#include <QSignalSpy>
#include <QTest>
#include "../../app/model/AccessibilityModel.h"
#include "../TestUtilities.h"

class TestAccessibilityModel : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() { m_model = new AccessibilityModel(); }

    void cleanup() {
        delete m_model;
        m_model = nullptr;
    }

    void testConstruction() {
        QVERIFY(m_model != nullptr);
        QVERIFY(!m_model->isScreenReaderEnabled());
        QVERIFY(!m_model->isHighContrastMode());
        QVERIFY(!m_model->isTtsEnabled());
    }

    void testScreenReaderEnabled() {
        QSignalSpy spy(m_model,
                       &AccessibilityModel::screenReaderEnabledChanged);

        QVERIFY(!m_model->isScreenReaderEnabled());

        m_model->setScreenReaderEnabled(true);
        QVERIFY(m_model->isScreenReaderEnabled());
        QCOMPARE(spy.count(), 1);

        m_model->setScreenReaderEnabled(false);
        QVERIFY(!m_model->isScreenReaderEnabled());
        QCOMPARE(spy.count(), 2);
    }

    void testAnnounceSettings() {
        m_model->setShouldAnnouncePageChanges(true);
        QVERIFY(m_model->shouldAnnouncePageChanges());

        m_model->setShouldAnnouncePageChanges(false);
        QVERIFY(!m_model->shouldAnnouncePageChanges());

        m_model->setShouldAnnounceZoomChanges(true);
        QVERIFY(m_model->shouldAnnounceZoomChanges());

        m_model->setShouldAnnounceZoomChanges(false);
        QVERIFY(!m_model->shouldAnnounceZoomChanges());
    }

    void testHighContrastMode() {
        QSignalSpy spy(m_model, &AccessibilityModel::highContrastModeChanged);

        QVERIFY(!m_model->isHighContrastMode());

        m_model->setHighContrastMode(true);
        QVERIFY(m_model->isHighContrastMode());
        QCOMPARE(spy.count(), 1);

        m_model->setHighContrastMode(false);
        QVERIFY(!m_model->isHighContrastMode());
    }

    void testColorSettings() {
        QColor bgColor(0, 0, 0);
        m_model->setBackgroundColor(bgColor);
        QCOMPARE(m_model->backgroundColor(), bgColor);

        QColor fgColor(255, 255, 255);
        m_model->setForegroundColor(fgColor);
        QCOMPARE(m_model->foregroundColor(), fgColor);

        QColor hlColor(255, 255, 0);
        m_model->setHighlightColor(hlColor);
        QCOMPARE(m_model->highlightColor(), hlColor);

        QColor selColor(0, 120, 215);
        m_model->setSelectionColor(selColor);
        QCOMPARE(m_model->selectionColor(), selColor);
    }

    void testTtsEnabled() {
        QSignalSpy spy(m_model, &AccessibilityModel::ttsEnabledChanged);

        QVERIFY(!m_model->isTtsEnabled());

        m_model->setTtsEnabled(true);
        QVERIFY(m_model->isTtsEnabled());
        QCOMPARE(spy.count(), 1);

        m_model->setTtsEnabled(false);
        QVERIFY(!m_model->isTtsEnabled());
    }

    void testTtsRate() {
        QSignalSpy spy(m_model, &AccessibilityModel::ttsRateChanged);

        m_model->setTtsRate(0.5);
        QCOMPARE(m_model->ttsRate(), 0.5);
        QCOMPARE(spy.count(), 1);

        m_model->setTtsRate(-0.5);
        QCOMPARE(m_model->ttsRate(), -0.5);

        m_model->setTtsRate(1.0);
        QCOMPARE(m_model->ttsRate(), 1.0);
    }

    void testTtsPitch() {
        m_model->setTtsPitch(0.0);
        QCOMPARE(m_model->ttsPitch(), 0.0);

        m_model->setTtsPitch(0.5);
        QCOMPARE(m_model->ttsPitch(), 0.5);

        m_model->setTtsPitch(-0.5);
        QCOMPARE(m_model->ttsPitch(), -0.5);
    }

    void testTtsVolume() {
        QSignalSpy spy(m_model, &AccessibilityModel::ttsVolumeChanged);

        m_model->setTtsVolume(1.0);
        QCOMPARE(m_model->ttsVolume(), 1.0);

        m_model->setTtsVolume(0.5);
        QCOMPARE(m_model->ttsVolume(), 0.5);

        m_model->setTtsVolume(0.0);
        QCOMPARE(m_model->ttsVolume(), 0.0);
    }

    void testTtsEngine() {
        m_model->setTtsEngine("default");
        QCOMPARE(m_model->ttsEngine(), QString("default"));

        m_model->setTtsEngine("custom");
        QCOMPARE(m_model->ttsEngine(), QString("custom"));
    }

    void testTextEnlargement() {
        QVERIFY(!m_model->isTextEnlargementEnabled());

        m_model->setTextEnlargementEnabled(true);
        QVERIFY(m_model->isTextEnlargementEnabled());

        m_model->setTextEnlargementEnabled(false);
        QVERIFY(!m_model->isTextEnlargementEnabled());
    }

    void testTextScaleFactor() {
        QSignalSpy spy(m_model, &AccessibilityModel::textScaleFactorChanged);

        m_model->setTextScaleFactor(1.0);
        QCOMPARE(m_model->textScaleFactor(), 1.0);

        m_model->setTextScaleFactor(1.5);
        QCOMPARE(m_model->textScaleFactor(), 1.5);

        m_model->setTextScaleFactor(2.0);
        QCOMPARE(m_model->textScaleFactor(), 2.0);
    }

    void testBoldText() {
        QVERIFY(!m_model->isBoldTextEnabled());

        m_model->setBoldTextEnabled(true);
        QVERIFY(m_model->isBoldTextEnabled());

        m_model->setBoldTextEnabled(false);
        QVERIFY(!m_model->isBoldTextEnabled());
    }

    void testReduceMotion() {
        QSignalSpy spy(m_model, &AccessibilityModel::reduceMotionChanged);

        QVERIFY(!m_model->shouldReduceMotion());

        m_model->setReduceMotion(true);
        QVERIFY(m_model->shouldReduceMotion());
        QCOMPARE(spy.count(), 1);

        m_model->setReduceMotion(false);
        QVERIFY(!m_model->shouldReduceMotion());
    }

    void testReduceTransparency() {
        QVERIFY(!m_model->shouldReduceTransparency());

        m_model->setReduceTransparency(true);
        QVERIFY(m_model->shouldReduceTransparency());

        m_model->setReduceTransparency(false);
        QVERIFY(!m_model->shouldReduceTransparency());
    }

    void testEnhancedKeyboardNavigation() {
        QVERIFY(!m_model->isEnhancedKeyboardNavigationEnabled());

        m_model->setEnhancedKeyboardNavigationEnabled(true);
        QVERIFY(m_model->isEnhancedKeyboardNavigationEnabled());

        m_model->setEnhancedKeyboardNavigationEnabled(false);
        QVERIFY(!m_model->isEnhancedKeyboardNavigationEnabled());
    }

    void testFocusIndicator() {
        m_model->setFocusIndicatorVisible(true);
        QVERIFY(m_model->isFocusIndicatorVisible());

        m_model->setFocusIndicatorVisible(false);
        QVERIFY(!m_model->isFocusIndicatorVisible());

        m_model->setFocusIndicatorWidth(3);
        QCOMPARE(m_model->focusIndicatorWidth(), 3);

        m_model->setFocusIndicatorWidth(5);
        QCOMPARE(m_model->focusIndicatorWidth(), 5);
    }

    void testFeatureChecking() {
        m_model->setScreenReaderEnabled(true);
        QVERIFY(m_model->isFeatureEnabled(AccessibilityModel::ScreenReader));

        m_model->setHighContrastMode(true);
        QVERIFY(m_model->isFeatureEnabled(AccessibilityModel::HighContrast));

        m_model->setTtsEnabled(true);
        QVERIFY(m_model->isFeatureEnabled(AccessibilityModel::TextToSpeech));

        m_model->setEnhancedKeyboardNavigationEnabled(true);
        QVERIFY(
            m_model->isFeatureEnabled(AccessibilityModel::EnhancedKeyboard));

        m_model->setTextEnlargementEnabled(true);
        QVERIFY(m_model->isFeatureEnabled(AccessibilityModel::TextEnlargement));

        m_model->setReduceMotion(true);
        QVERIFY(m_model->isFeatureEnabled(AccessibilityModel::ReduceMotion));
    }

    void testResetToDefaults() {
        m_model->setScreenReaderEnabled(true);
        m_model->setHighContrastMode(true);
        m_model->setTtsEnabled(true);
        m_model->setTextScaleFactor(2.0);

        QSignalSpy spy(m_model, &AccessibilityModel::settingsReset);

        m_model->resetToDefaults();

        QCOMPARE(spy.count(), 1);
    }

    void testAutoSave() {
        m_model->setAutoSave(true);
        QVERIFY(m_model->isAutoSaveEnabled());

        m_model->setAutoSave(false);
        QVERIFY(!m_model->isAutoSaveEnabled());
    }

    void testSettings() {
        AccessibilitySettings settings = m_model->settings();

        settings.screenReaderEnabled = true;
        settings.highContrastMode = true;
        settings.ttsRate = 0.5;

        m_model->setSettings(settings);

        QVERIFY(m_model->isScreenReaderEnabled());
        QVERIFY(m_model->isHighContrastMode());
        QCOMPARE(m_model->ttsRate(), 0.5);
    }

    void testAccessibilitySettingsStruct() {
        AccessibilitySettings settings1;
        AccessibilitySettings settings2;

        QVERIFY(settings1 == settings2);

        settings1.screenReaderEnabled = true;
        QVERIFY(settings1 != settings2);

        QJsonObject json = settings1.toJson();
        QVERIFY(!json.isEmpty());

        AccessibilitySettings loaded = AccessibilitySettings::fromJson(json);
        QCOMPARE(loaded.screenReaderEnabled, settings1.screenReaderEnabled);
    }

private:
    AccessibilityModel* m_model = nullptr;
};

QTEST_MAIN(TestAccessibilityModel)
#include "test_accessibility_model.moc"
