#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/OnboardingWidget.h"

class OnboardingWidgetTest : public QObject {
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
    OnboardingWidget* m_widget;
};

void OnboardingWidgetTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void OnboardingWidgetTest::cleanupTestCase() { delete m_parentWidget; }
void OnboardingWidgetTest::init() {
    m_widget = new OnboardingWidget(m_parentWidget);
}
void OnboardingWidgetTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void OnboardingWidgetTest::testConstruction() { QVERIFY(m_widget != nullptr); }
void OnboardingWidgetTest::testDestruction() {
    auto* w = new OnboardingWidget(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void OnboardingWidgetTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(OnboardingWidgetTest)
#include "test_onboarding_widget.moc"
