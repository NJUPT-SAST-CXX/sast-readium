#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/SystemTraySettingsWidget.h"

class SystemTraySettingsWidgetTest : public QObject {
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
    SystemTraySettingsWidget* m_widget;
};

void SystemTraySettingsWidgetTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void SystemTraySettingsWidgetTest::cleanupTestCase() { delete m_parentWidget; }
void SystemTraySettingsWidgetTest::init() {
    m_widget = new SystemTraySettingsWidget(m_parentWidget);
}
void SystemTraySettingsWidgetTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void SystemTraySettingsWidgetTest::testConstruction() {
    QVERIFY(m_widget != nullptr);
}
void SystemTraySettingsWidgetTest::testDestruction() {
    auto* w = new SystemTraySettingsWidget(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void SystemTraySettingsWidgetTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(SystemTraySettingsWidgetTest)
#include "test_system_tray_settings_widget.moc"
