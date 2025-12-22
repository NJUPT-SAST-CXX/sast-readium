#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/ShortcutSettingsWidget.h"

class ShortcutSettingsWidgetTest : public QObject {
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
    ShortcutSettingsWidget* m_widget;
};

void ShortcutSettingsWidgetTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void ShortcutSettingsWidgetTest::cleanupTestCase() { delete m_parentWidget; }
void ShortcutSettingsWidgetTest::init() {
    m_widget = new ShortcutSettingsWidget(m_parentWidget);
}
void ShortcutSettingsWidgetTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void ShortcutSettingsWidgetTest::testConstruction() {
    QVERIFY(m_widget != nullptr);
}
void ShortcutSettingsWidgetTest::testDestruction() {
    auto* w = new ShortcutSettingsWidget(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void ShortcutSettingsWidgetTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(ShortcutSettingsWidgetTest)
#include "test_shortcut_settings_widget.moc"
