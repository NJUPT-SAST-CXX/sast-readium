#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/LayersPanel.h"

class LayersPanelTest : public QObject {
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
    LayersPanel* m_widget;
};

void LayersPanelTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void LayersPanelTest::cleanupTestCase() { delete m_parentWidget; }
void LayersPanelTest::init() { m_widget = new LayersPanel(m_parentWidget); }
void LayersPanelTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void LayersPanelTest::testConstruction() { QVERIFY(m_widget != nullptr); }
void LayersPanelTest::testDestruction() {
    auto* w = new LayersPanel(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void LayersPanelTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(LayersPanelTest)
#include "test_layers_panel.moc"
