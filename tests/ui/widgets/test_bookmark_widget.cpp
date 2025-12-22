#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/BookmarkWidget.h"

class BookmarkWidgetTest : public QObject {
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
    BookmarkWidget* m_widget;
};

void BookmarkWidgetTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void BookmarkWidgetTest::cleanupTestCase() { delete m_parentWidget; }
void BookmarkWidgetTest::init() {
    m_widget = new BookmarkWidget(m_parentWidget);
}
void BookmarkWidgetTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void BookmarkWidgetTest::testConstruction() { QVERIFY(m_widget != nullptr); }
void BookmarkWidgetTest::testDestruction() {
    auto* w = new BookmarkWidget(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void BookmarkWidgetTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(BookmarkWidgetTest)
#include "test_bookmark_widget.moc"
