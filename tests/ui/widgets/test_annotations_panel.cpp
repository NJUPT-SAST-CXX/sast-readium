#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/AnnotationsPanel.h"

class AnnotationsPanelTest : public QObject {
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
    AnnotationsPanel* m_widget;
};

void AnnotationsPanelTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void AnnotationsPanelTest::cleanupTestCase() { delete m_parentWidget; }
void AnnotationsPanelTest::init() {
    m_widget = new AnnotationsPanel(m_parentWidget);
}
void AnnotationsPanelTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void AnnotationsPanelTest::testConstruction() { QVERIFY(m_widget != nullptr); }
void AnnotationsPanelTest::testDestruction() {
    auto* w = new AnnotationsPanel(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void AnnotationsPanelTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(AnnotationsPanelTest)
#include "test_annotations_panel.moc"
