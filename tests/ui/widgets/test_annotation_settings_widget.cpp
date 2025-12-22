#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/AnnotationSettingsWidget.h"

class AnnotationSettingsWidgetTest : public QObject {
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
    AnnotationSettingsWidget* m_widget;
};

void AnnotationSettingsWidgetTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}

void AnnotationSettingsWidgetTest::cleanupTestCase() { delete m_parentWidget; }
void AnnotationSettingsWidgetTest::init() {
    m_widget = new AnnotationSettingsWidget(m_parentWidget);
}
void AnnotationSettingsWidgetTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void AnnotationSettingsWidgetTest::testConstruction() {
    QVERIFY(m_widget != nullptr);
}
void AnnotationSettingsWidgetTest::testDestruction() {
    auto* w = new AnnotationSettingsWidget(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void AnnotationSettingsWidgetTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(AnnotationSettingsWidgetTest)
#include "test_annotation_settings_widget.moc"
