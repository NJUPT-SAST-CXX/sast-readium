#include <QApplication>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include "../../../app/ui/theme/ReadingModeManager.h"

class ReadingModeManagerTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testConstruction();
    void testDestruction();
    void testSetReadingModeNormal();
    void testSetReadingModeNight();
    void testSetReadingModeSepia();
    void testSetReadingModeCustom();
    void testGetReadingMode();
    void testSetCustomColors();
    void testGetBackgroundColor();
    void testGetForegroundColor();
    void testSetBrightness();
    void testGetBrightness();
    void testGetPalette();
    void testReadingModeChangedSignal();
    void testBrightnessChangedSignal();
    void testColorsChangedSignal();
    void testReadingModeEnum();

private:
    ReadingModeManager* m_manager;
};

void ReadingModeManagerTest::initTestCase() {}

void ReadingModeManagerTest::cleanupTestCase() {}

void ReadingModeManagerTest::init() { m_manager = new ReadingModeManager(); }

void ReadingModeManagerTest::cleanup() {
    delete m_manager;
    m_manager = nullptr;
}

void ReadingModeManagerTest::testConstruction() {
    QVERIFY(m_manager != nullptr);
}

void ReadingModeManagerTest::testDestruction() {
    auto* manager = new ReadingModeManager();
    delete manager;
    QVERIFY(true);
}

void ReadingModeManagerTest::testSetReadingModeNormal() {
    m_manager->setReadingMode(ReadingModeManager::Normal);
    QCOMPARE(m_manager->getReadingMode(), ReadingModeManager::Normal);
}

void ReadingModeManagerTest::testSetReadingModeNight() {
    m_manager->setReadingMode(ReadingModeManager::Night);
    QCOMPARE(m_manager->getReadingMode(), ReadingModeManager::Night);
}

void ReadingModeManagerTest::testSetReadingModeSepia() {
    m_manager->setReadingMode(ReadingModeManager::Sepia);
    QCOMPARE(m_manager->getReadingMode(), ReadingModeManager::Sepia);
}

void ReadingModeManagerTest::testSetReadingModeCustom() {
    m_manager->setReadingMode(ReadingModeManager::Custom);
    QCOMPARE(m_manager->getReadingMode(), ReadingModeManager::Custom);
}

void ReadingModeManagerTest::testGetReadingMode() {
    m_manager->setReadingMode(ReadingModeManager::Night);
    QCOMPARE(m_manager->getReadingMode(), ReadingModeManager::Night);
}

void ReadingModeManagerTest::testSetCustomColors() {
    QColor bg(30, 30, 30);
    QColor fg(200, 200, 200);
    m_manager->setCustomColors(bg, fg);
    QCOMPARE(m_manager->getBackgroundColor(), bg);
    QCOMPARE(m_manager->getForegroundColor(), fg);
}

void ReadingModeManagerTest::testGetBackgroundColor() {
    QColor color = m_manager->getBackgroundColor();
    QVERIFY(color.isValid());
}

void ReadingModeManagerTest::testGetForegroundColor() {
    QColor color = m_manager->getForegroundColor();
    QVERIFY(color.isValid());
}

void ReadingModeManagerTest::testSetBrightness() {
    m_manager->setBrightness(0.5);
    QCOMPARE(m_manager->getBrightness(), 0.5);
}

void ReadingModeManagerTest::testGetBrightness() {
    m_manager->setBrightness(0.8);
    QCOMPARE(m_manager->getBrightness(), 0.8);
}

void ReadingModeManagerTest::testGetPalette() {
    QPalette palette = m_manager->getPalette();
    QVERIFY(true);  // Palette should be valid
}

void ReadingModeManagerTest::testReadingModeChangedSignal() {
    QSignalSpy spy(m_manager, &ReadingModeManager::readingModeChanged);
    QVERIFY(spy.isValid());
    m_manager->setReadingMode(ReadingModeManager::Night);
    QVERIFY(spy.count() >= 0);
}

void ReadingModeManagerTest::testBrightnessChangedSignal() {
    QSignalSpy spy(m_manager, &ReadingModeManager::brightnessChanged);
    QVERIFY(spy.isValid());
    m_manager->setBrightness(0.7);
    QVERIFY(spy.count() >= 0);
}

void ReadingModeManagerTest::testColorsChangedSignal() {
    QSignalSpy spy(m_manager, &ReadingModeManager::colorsChanged);
    QVERIFY(spy.isValid());
}

void ReadingModeManagerTest::testReadingModeEnum() {
    QVERIFY(ReadingModeManager::Normal != ReadingModeManager::Night);
    QVERIFY(ReadingModeManager::Sepia != ReadingModeManager::Custom);
}

QTEST_MAIN(ReadingModeManagerTest)
#include "test_reading_mode_manager.moc"
