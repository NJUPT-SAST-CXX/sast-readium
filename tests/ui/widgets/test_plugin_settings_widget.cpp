#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/PluginSettingsWidget.h"

class PluginSettingsWidgetTest : public QObject {
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
    PluginSettingsWidget* m_widget;
};

void PluginSettingsWidgetTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void PluginSettingsWidgetTest::cleanupTestCase() { delete m_parentWidget; }
void PluginSettingsWidgetTest::init() {
    m_widget = new PluginSettingsWidget(m_parentWidget);
}
void PluginSettingsWidgetTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void PluginSettingsWidgetTest::testConstruction() {
    QVERIFY(m_widget != nullptr);
}
void PluginSettingsWidgetTest::testDestruction() {
    auto* w = new PluginSettingsWidget(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void PluginSettingsWidgetTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(PluginSettingsWidgetTest)
#include "test_plugin_settings_widget.moc"
