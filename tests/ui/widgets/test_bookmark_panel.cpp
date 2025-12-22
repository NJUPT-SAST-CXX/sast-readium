#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/BookmarkPanel.h"

class BookmarkPanelTest : public QObject {
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
    BookmarkPanel* m_widget;
};

void BookmarkPanelTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void BookmarkPanelTest::cleanupTestCase() { delete m_parentWidget; }
void BookmarkPanelTest::init() { m_widget = new BookmarkPanel(m_parentWidget); }
void BookmarkPanelTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void BookmarkPanelTest::testConstruction() { QVERIFY(m_widget != nullptr); }
void BookmarkPanelTest::testDestruction() {
    auto* w = new BookmarkPanel(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void BookmarkPanelTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(BookmarkPanelTest)
#include "test_bookmark_panel.moc"
