#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/SearchPanel.h"

class SearchPanelTest : public QObject {
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
    SearchPanel* m_widget;
};

void SearchPanelTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void SearchPanelTest::cleanupTestCase() { delete m_parentWidget; }
void SearchPanelTest::init() { m_widget = new SearchPanel(m_parentWidget); }
void SearchPanelTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void SearchPanelTest::testConstruction() { QVERIFY(m_widget != nullptr); }
void SearchPanelTest::testDestruction() {
    auto* w = new SearchPanel(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void SearchPanelTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(SearchPanelTest)
#include "test_search_panel.moc"
