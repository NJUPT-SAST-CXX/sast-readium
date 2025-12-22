#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/AccessibilitySettingsWidget.h"

class AccessibilitySettingsWidgetTest : public QObject {
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
    AccessibilitySettingsWidget* m_widget;
};

void AccessibilitySettingsWidgetTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
    }
}

void AccessibilitySettingsWidgetTest::cleanupTestCase() {
    delete m_parentWidget;
}

void AccessibilitySettingsWidgetTest::init() {
    m_widget = new AccessibilitySettingsWidget(m_parentWidget);
}

void AccessibilitySettingsWidgetTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}

void AccessibilitySettingsWidgetTest::testConstruction() {
    QVERIFY(m_widget != nullptr);
}

void AccessibilitySettingsWidgetTest::testDestruction() {
    auto* widget = new AccessibilitySettingsWidget(m_parentWidget);
    delete widget;
    QVERIFY(true);
}

void AccessibilitySettingsWidgetTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(AccessibilitySettingsWidgetTest)
#include "test_accessibility_settings_widget.moc"
