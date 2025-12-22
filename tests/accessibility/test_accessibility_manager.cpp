#include <QSignalSpy>
#include <QTest>
#include "../../app/accessibility/AccessibilityManager.h"
#include "../TestUtilities.h"

class TestAccessibilityManager : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() { m_manager = new AccessibilityManager(); }

    void cleanup() {
        delete m_manager;
        m_manager = nullptr;
    }

    void testConstruction() {
        QVERIFY(m_manager != nullptr);
        QVERIFY(!m_manager->isInitialized());
    }

    void testInitialize() {
        m_manager->initialize();
        QVERIFY(m_manager->isInitialized());
    }

    void testScreenReaderMode() {
        m_manager->initialize();

        QSignalSpy spy(m_manager,
                       &AccessibilityManager::screenReaderModeChanged);

        QVERIFY(!m_manager->isScreenReaderEnabled());

        m_manager->enableScreenReaderMode(true);
        QVERIFY(m_manager->isScreenReaderEnabled());
        QCOMPARE(spy.count(), 1);

        m_manager->enableScreenReaderMode(false);
        QVERIFY(!m_manager->isScreenReaderEnabled());
    }

    void testAnnounceText() {
        m_manager->initialize();
        m_manager->enableScreenReaderMode(true);

        m_manager->announceText("Test announcement");
        m_manager->announceText("Another announcement");
    }

    void testAnnouncePageChange() {
        m_manager->initialize();
        m_manager->enableScreenReaderMode(true);

        m_manager->announcePageChange(1, 10);
        m_manager->announcePageChange(5, 10);
        m_manager->announcePageChange(10, 10);
    }

    void testHighContrastMode() {
        m_manager->initialize();

        QSignalSpy spy(m_manager,
                       &AccessibilityManager::highContrastModeChanged);

        QVERIFY(!m_manager->isHighContrastMode());

        m_manager->setHighContrastMode(true);
        QVERIFY(m_manager->isHighContrastMode());
        QCOMPARE(spy.count(), 1);

        m_manager->setHighContrastMode(false);
        QVERIFY(!m_manager->isHighContrastMode());
    }

    void testHighContrastColors() {
        m_manager->initialize();
        m_manager->setHighContrastMode(true);

        QColor bgColor = m_manager->getBackgroundColor();
        QColor fgColor = m_manager->getForegroundColor();
        QColor hlColor = m_manager->getHighlightColor();

        QVERIFY(bgColor.isValid());
        QVERIFY(fgColor.isValid());
        QVERIFY(hlColor.isValid());
    }

    void testTextToSpeech() {
        m_manager->initialize();

        QSignalSpy stateSpy(m_manager,
                            &AccessibilityManager::textToSpeechStateChanged);

        QVERIFY(!m_manager->isTextToSpeechActive());

        m_manager->startTextToSpeech("Test text to speech");

        m_manager->stopTextToSpeech();
        QVERIFY(!m_manager->isTextToSpeechActive());
    }

    void testTextToSpeechPauseResume() {
        m_manager->initialize();

        m_manager->startTextToSpeech("Long text for pause resume test");
        m_manager->pauseTextToSpeech();
        m_manager->resumeTextToSpeech();
        m_manager->stopTextToSpeech();
    }

    void testTextToSpeechRate() {
        m_manager->initialize();

        m_manager->setTextToSpeechRate(0.5);
        m_manager->setTextToSpeechRate(1.0);
        m_manager->setTextToSpeechRate(-0.5);
    }

    void testTextToSpeechVolume() {
        m_manager->initialize();

        m_manager->setTextToSpeechVolume(1.0);
        m_manager->setTextToSpeechVolume(0.5);
        m_manager->setTextToSpeechVolume(0.0);
    }

    void testControllerAccess() {
        m_manager->initialize();

        AccessibilityController* controller = m_manager->controller();
        QVERIFY(controller != nullptr);
    }

    void testModelAccess() {
        m_manager->initialize();

        AccessibilityModel* model = m_manager->model();
        QVERIFY(model != nullptr);
    }

    void testInitializedSignal() {
        QSignalSpy spy(m_manager, &AccessibilityManager::initialized);

        m_manager->initialize();

        QCOMPARE(spy.count(), 1);
    }

    void testTextToSpeechFinishedSignal() {
        QSignalSpy spy(m_manager, &AccessibilityManager::textToSpeechFinished);
        QVERIFY(spy.isValid());
    }

    void testMultipleInitializations() {
        m_manager->initialize();
        QVERIFY(m_manager->isInitialized());

        m_manager->initialize();
        QVERIFY(m_manager->isInitialized());
    }

    void testFeatureCombinations() {
        m_manager->initialize();

        m_manager->enableScreenReaderMode(true);
        m_manager->setHighContrastMode(true);
        m_manager->startTextToSpeech("Test");

        QVERIFY(m_manager->isScreenReaderEnabled());
        QVERIFY(m_manager->isHighContrastMode());

        m_manager->stopTextToSpeech();
        m_manager->setHighContrastMode(false);
        m_manager->enableScreenReaderMode(false);

        QVERIFY(!m_manager->isScreenReaderEnabled());
        QVERIFY(!m_manager->isHighContrastMode());
    }

private:
    AccessibilityManager* m_manager = nullptr;
};

QTEST_MAIN(TestAccessibilityManager)
#include "test_accessibility_manager.moc"
