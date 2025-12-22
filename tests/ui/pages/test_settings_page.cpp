#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/pages/SettingsPage.h"

class SettingsPageTest : public QObject {
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
    SettingsPage* m_page;
};

void SettingsPageTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
    }
}

void SettingsPageTest::cleanupTestCase() { delete m_parentWidget; }

void SettingsPageTest::init() { m_page = new SettingsPage(m_parentWidget); }

void SettingsPageTest::cleanup() {
    delete m_page;
    m_page = nullptr;
}

void SettingsPageTest::testConstruction() { QVERIFY(m_page != nullptr); }

void SettingsPageTest::testDestruction() {
    auto* page = new SettingsPage(m_parentWidget);
    delete page;
    QVERIFY(true);
}

void SettingsPageTest::testVisibility() {
    m_page->show();
    QVERIFY(m_page->isVisible());
}

QTEST_MAIN(SettingsPageTest)
#include "test_settings_page.moc"
