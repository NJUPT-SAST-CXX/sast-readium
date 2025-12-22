#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/SkeletonWidget.h"

class SkeletonWidgetTest : public QObject {
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
    SkeletonWidget* m_widget;
};

void SkeletonWidgetTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void SkeletonWidgetTest::cleanupTestCase() { delete m_parentWidget; }
void SkeletonWidgetTest::init() {
    m_widget =
        new SkeletonWidget(SkeletonWidget::Shape::Rectangle, m_parentWidget);
}
void SkeletonWidgetTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void SkeletonWidgetTest::testConstruction() { QVERIFY(m_widget != nullptr); }
void SkeletonWidgetTest::testDestruction() {
    auto* w =
        new SkeletonWidget(SkeletonWidget::Shape::Rectangle, m_parentWidget);
    delete w;
    QVERIFY(true);
}
void SkeletonWidgetTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(SkeletonWidgetTest)
#include "test_skeleton_widget.moc"
