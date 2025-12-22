#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/SearchSettingsWidget.h"

class SearchSettingsWidgetTest : public QObject {
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
    SearchSettingsWidget* m_widget;
};

void SearchSettingsWidgetTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void SearchSettingsWidgetTest::cleanupTestCase() { delete m_parentWidget; }
void SearchSettingsWidgetTest::init() {
    m_widget = new SearchSettingsWidget(m_parentWidget);
}
void SearchSettingsWidgetTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void SearchSettingsWidgetTest::testConstruction() {
    QVERIFY(m_widget != nullptr);
}
void SearchSettingsWidgetTest::testDestruction() {
    auto* w = new SearchSettingsWidget(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void SearchSettingsWidgetTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(SearchSettingsWidgetTest)
#include "test_search_settings_widget.moc"
