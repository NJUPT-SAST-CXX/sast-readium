#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/pages/PluginManagerPage.h"

class PluginManagerPageTest : public QObject {
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
    PluginManagerPage* m_page;
};

void PluginManagerPageTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
    }
}

void PluginManagerPageTest::cleanupTestCase() { delete m_parentWidget; }

void PluginManagerPageTest::init() {
    m_page = new PluginManagerPage(m_parentWidget);
}

void PluginManagerPageTest::cleanup() {
    delete m_page;
    m_page = nullptr;
}

void PluginManagerPageTest::testConstruction() { QVERIFY(m_page != nullptr); }

void PluginManagerPageTest::testDestruction() {
    auto* page = new PluginManagerPage(m_parentWidget);
    delete page;
    QVERIFY(true);
}

void PluginManagerPageTest::testVisibility() {
    m_page->show();
    QVERIFY(m_page->isVisible());
}

QTEST_MAIN(PluginManagerPageTest)
#include "test_plugin_manager_page.moc"
