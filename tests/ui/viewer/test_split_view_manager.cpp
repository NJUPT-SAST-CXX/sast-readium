#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/viewer/SplitViewManager.h"

class SplitViewManagerTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testConstruction();
    void testDestruction();

private:
    QWidget* m_parentWidget;
    SplitViewManager* m_manager;
};

void SplitViewManagerTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
    }
}

void SplitViewManagerTest::cleanupTestCase() { delete m_parentWidget; }

void SplitViewManagerTest::init() {
    m_manager = new SplitViewManager(m_parentWidget);
}

void SplitViewManagerTest::cleanup() {
    delete m_manager;
    m_manager = nullptr;
}

void SplitViewManagerTest::testConstruction() { QVERIFY(m_manager != nullptr); }

void SplitViewManagerTest::testDestruction() {
    auto* manager = new SplitViewManager(m_parentWidget);
    delete manager;
    QVERIFY(true);
}

QTEST_MAIN(SplitViewManagerTest)
#include "test_split_view_manager.moc"
