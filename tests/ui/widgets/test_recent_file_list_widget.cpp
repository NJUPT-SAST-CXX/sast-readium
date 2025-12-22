#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/RecentFileListWidget.h"

class RecentFileListWidgetTest : public QObject {
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
    RecentFileListWidget* m_widget;
};

void RecentFileListWidgetTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void RecentFileListWidgetTest::cleanupTestCase() { delete m_parentWidget; }
void RecentFileListWidgetTest::init() {
    m_widget = new RecentFileListWidget(m_parentWidget);
}
void RecentFileListWidgetTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void RecentFileListWidgetTest::testConstruction() {
    QVERIFY(m_widget != nullptr);
}
void RecentFileListWidgetTest::testDestruction() {
    auto* w = new RecentFileListWidget(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void RecentFileListWidgetTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(RecentFileListWidgetTest)
#include "test_recent_file_list_widget.moc"
