#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/LoggingSettingsWidget.h"

class LoggingSettingsWidgetTest : public QObject {
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
    LoggingSettingsWidget* m_widget;
};

void LoggingSettingsWidgetTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void LoggingSettingsWidgetTest::cleanupTestCase() { delete m_parentWidget; }
void LoggingSettingsWidgetTest::init() {
    m_widget = new LoggingSettingsWidget(m_parentWidget);
}
void LoggingSettingsWidgetTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void LoggingSettingsWidgetTest::testConstruction() {
    QVERIFY(m_widget != nullptr);
}
void LoggingSettingsWidgetTest::testDestruction() {
    auto* w = new LoggingSettingsWidget(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void LoggingSettingsWidgetTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(LoggingSettingsWidgetTest)
#include "test_logging_settings_widget.moc"
