#include <QApplication>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/AnnotationToolbar.h"

class AnnotationToolbarTest : public QObject {
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
    AnnotationToolbar* m_widget;
};

void AnnotationToolbarTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}

void AnnotationToolbarTest::cleanupTestCase() { delete m_parentWidget; }
void AnnotationToolbarTest::init() {
    m_widget = new AnnotationToolbar(m_parentWidget);
}
void AnnotationToolbarTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void AnnotationToolbarTest::testConstruction() { QVERIFY(m_widget != nullptr); }
void AnnotationToolbarTest::testDestruction() {
    auto* w = new AnnotationToolbar(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void AnnotationToolbarTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(AnnotationToolbarTest)
#include "test_annotation_toolbar.moc"
