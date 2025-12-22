#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/DocumentPropertiesPanel.h"

class DocumentPropertiesPanelTest : public QObject {
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
    DocumentPropertiesPanel* m_widget;
};

void DocumentPropertiesPanelTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void DocumentPropertiesPanelTest::cleanupTestCase() { delete m_parentWidget; }
void DocumentPropertiesPanelTest::init() {
    m_widget = new DocumentPropertiesPanel(m_parentWidget);
}
void DocumentPropertiesPanelTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void DocumentPropertiesPanelTest::testConstruction() {
    QVERIFY(m_widget != nullptr);
}
void DocumentPropertiesPanelTest::testDestruction() {
    auto* w = new DocumentPropertiesPanel(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void DocumentPropertiesPanelTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(DocumentPropertiesPanelTest)
#include "test_document_properties_panel.moc"
