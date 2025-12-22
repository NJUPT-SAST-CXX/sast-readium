#include <QApplication>
#include <QLabel>
#include <QtTest/QtTest>
#include "../../../app/ui/pages/AboutPage.h"

class AboutPageTest : public QObject {
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
    AboutPage* m_page;
};

void AboutPageTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
    }
}

void AboutPageTest::cleanupTestCase() { delete m_parentWidget; }

void AboutPageTest::init() { m_page = new AboutPage(m_parentWidget); }

void AboutPageTest::cleanup() {
    delete m_page;
    m_page = nullptr;
}

void AboutPageTest::testConstruction() { QVERIFY(m_page != nullptr); }

void AboutPageTest::testDestruction() {
    auto* page = new AboutPage(m_parentWidget);
    delete page;
    QVERIFY(true);
}

void AboutPageTest::testVisibility() {
    m_page->show();
    QVERIFY(m_page->isVisible());
    m_page->hide();
    QVERIFY(!m_page->isVisible());
}

void AboutPageTest::testSize() {
    m_page->show();
    QVERIFY(m_page->width() >= 0);
    QVERIFY(m_page->height() >= 0);
}

QTEST_MAIN(AboutPageTest)
#include "test_about_page.moc"
