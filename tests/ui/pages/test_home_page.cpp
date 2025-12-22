#include <QApplication>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include "../../../app/ui/pages/HomePage.h"

class HomePageTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testConstruction();
    void testDestruction();
    void testVisibility();
    void testSize();

private:
    QWidget* m_parentWidget;
    HomePage* m_page;
};

void HomePageTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
    }
}

void HomePageTest::cleanupTestCase() { delete m_parentWidget; }

void HomePageTest::init() { m_page = new HomePage(m_parentWidget); }

void HomePageTest::cleanup() {
    delete m_page;
    m_page = nullptr;
}

void HomePageTest::testConstruction() { QVERIFY(m_page != nullptr); }

void HomePageTest::testDestruction() {
    auto* page = new HomePage(m_parentWidget);
    delete page;
    QVERIFY(true);
}

void HomePageTest::testVisibility() {
    m_page->show();
    QVERIFY(m_page->isVisible());
}

void HomePageTest::testSize() {
    m_page->show();
    QVERIFY(m_page->width() >= 0);
    QVERIFY(m_page->height() >= 0);
}

QTEST_MAIN(HomePageTest)
#include "test_home_page.moc"
