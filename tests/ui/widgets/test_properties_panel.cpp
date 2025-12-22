#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/PropertiesPanel.h"

class PropertiesPanelTest : public QObject {
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
    PropertiesPanel* m_widget;
};

void PropertiesPanelTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void PropertiesPanelTest::cleanupTestCase() { delete m_parentWidget; }
void PropertiesPanelTest::init() {
    m_widget = new PropertiesPanel(m_parentWidget);
}
void PropertiesPanelTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void PropertiesPanelTest::testConstruction() { QVERIFY(m_widget != nullptr); }
void PropertiesPanelTest::testDestruction() {
    auto* w = new PropertiesPanel(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void PropertiesPanelTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(PropertiesPanelTest)
#include "test_properties_panel.moc"
