#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/TutorialCard.h"

class TutorialCardTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    void testConstruction();
    void testDestruction();
    void testVisibility();

private:
    QWidget* m_parentWidget;
    TutorialCard* m_widget;
};

void TutorialCardTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void TutorialCardTest::cleanupTestCase() { delete m_parentWidget; }
void TutorialCardTest::init() {
    m_widget = new TutorialCard("test_id", "Test Title", "Test Description",
                                QIcon(), m_parentWidget);
}
void TutorialCardTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void TutorialCardTest::testConstruction() { QVERIFY(m_widget != nullptr); }
void TutorialCardTest::testDestruction() {
    auto* w = new TutorialCard("test_id", "Test Title", "Test Description",
                               QIcon(), m_parentWidget);
    delete w;
    QVERIFY(true);
}
void TutorialCardTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(TutorialCardTest)
#include "test_tutorial_card.moc"
