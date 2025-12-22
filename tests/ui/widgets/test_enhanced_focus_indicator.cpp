#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/EnhancedFocusIndicator.h"

class EnhancedFocusIndicatorTest : public QObject {
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
    EnhancedFocusIndicator* m_widget;
};

void EnhancedFocusIndicatorTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void EnhancedFocusIndicatorTest::cleanupTestCase() { delete m_parentWidget; }
void EnhancedFocusIndicatorTest::init() {
    m_widget = new EnhancedFocusIndicator(m_parentWidget);
}
void EnhancedFocusIndicatorTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void EnhancedFocusIndicatorTest::testConstruction() {
    QVERIFY(m_widget != nullptr);
}
void EnhancedFocusIndicatorTest::testDestruction() {
    auto* w = new EnhancedFocusIndicator(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void EnhancedFocusIndicatorTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(EnhancedFocusIndicatorTest)
#include "test_enhanced_focus_indicator.moc"
