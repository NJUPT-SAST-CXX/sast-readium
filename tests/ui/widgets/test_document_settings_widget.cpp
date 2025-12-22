#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/DocumentSettingsWidget.h"

class DocumentSettingsWidgetTest : public QObject {
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
    DocumentSettingsWidget* m_widget;
};

void DocumentSettingsWidgetTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void DocumentSettingsWidgetTest::cleanupTestCase() { delete m_parentWidget; }
void DocumentSettingsWidgetTest::init() {
    m_widget = new DocumentSettingsWidget(m_parentWidget);
}
void DocumentSettingsWidgetTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void DocumentSettingsWidgetTest::testConstruction() {
    QVERIFY(m_widget != nullptr);
}
void DocumentSettingsWidgetTest::testDestruction() {
    auto* w = new DocumentSettingsWidget(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void DocumentSettingsWidgetTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(DocumentSettingsWidgetTest)
#include "test_document_settings_widget.moc"
