#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/ToastNotification.h"

class ToastNotificationTest : public QObject {
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
    ToastNotification* m_widget;
};

void ToastNotificationTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void ToastNotificationTest::cleanupTestCase() { delete m_parentWidget; }
void ToastNotificationTest::init() {
    m_widget = new ToastNotification(m_parentWidget);
}
void ToastNotificationTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void ToastNotificationTest::testConstruction() { QVERIFY(m_widget != nullptr); }
void ToastNotificationTest::testDestruction() {
    auto* w = new ToastNotification(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void ToastNotificationTest::testVisibility() {
    m_widget->setVisible(true);
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(ToastNotificationTest)
#include "test_toast_notification.moc"
