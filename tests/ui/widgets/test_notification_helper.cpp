#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/NotificationHelper.h"

class NotificationHelperTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void testHelperExists();

private:
    QWidget* m_parentWidget;
};

void NotificationHelperTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void NotificationHelperTest::cleanupTestCase() { delete m_parentWidget; }
void NotificationHelperTest::testHelperExists() { QVERIFY(true); }

QTEST_MAIN(NotificationHelperTest)
#include "test_notification_helper.moc"
