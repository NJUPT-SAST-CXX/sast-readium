#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/OutlinePanel.h"

class OutlinePanelTest : public QObject {
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
    OutlinePanel* m_widget;
};

void OutlinePanelTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void OutlinePanelTest::cleanupTestCase() { delete m_parentWidget; }
void OutlinePanelTest::init() { m_widget = new OutlinePanel(m_parentWidget); }
void OutlinePanelTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void OutlinePanelTest::testConstruction() { QVERIFY(m_widget != nullptr); }
void OutlinePanelTest::testDestruction() {
    auto* w = new OutlinePanel(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void OutlinePanelTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(OutlinePanelTest)
#include "test_outline_panel.moc"
