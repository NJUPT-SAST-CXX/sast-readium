#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/DocumentTabWidget.h"

class DocumentTabWidgetTest : public QObject {
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
    DocumentTabWidget* m_widget;
};

void DocumentTabWidgetTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void DocumentTabWidgetTest::cleanupTestCase() { delete m_parentWidget; }
void DocumentTabWidgetTest::init() {
    m_widget = new DocumentTabWidget(m_parentWidget);
}
void DocumentTabWidgetTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void DocumentTabWidgetTest::testConstruction() { QVERIFY(m_widget != nullptr); }
void DocumentTabWidgetTest::testDestruction() {
    auto* w = new DocumentTabWidget(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void DocumentTabWidgetTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(DocumentTabWidgetTest)
#include "test_document_tab_widget.moc"
