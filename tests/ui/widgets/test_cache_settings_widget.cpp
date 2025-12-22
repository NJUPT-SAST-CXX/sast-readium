#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/CacheSettingsWidget.h"

class CacheSettingsWidgetTest : public QObject {
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
    CacheSettingsWidget* m_widget;
};

void CacheSettingsWidgetTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void CacheSettingsWidgetTest::cleanupTestCase() { delete m_parentWidget; }
void CacheSettingsWidgetTest::init() {
    m_widget = new CacheSettingsWidget(m_parentWidget);
}
void CacheSettingsWidgetTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void CacheSettingsWidgetTest::testConstruction() {
    QVERIFY(m_widget != nullptr);
}
void CacheSettingsWidgetTest::testDestruction() {
    auto* w = new CacheSettingsWidget(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void CacheSettingsWidgetTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(CacheSettingsWidgetTest)
#include "test_cache_settings_widget.moc"
