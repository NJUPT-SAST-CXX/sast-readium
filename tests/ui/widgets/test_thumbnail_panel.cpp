#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/widgets/ThumbnailPanel.h"

class ThumbnailPanelTest : public QObject {
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
    ThumbnailPanel* m_widget;
};

void ThumbnailPanelTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen")
        QTest::qWait(100);
    else
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
}
void ThumbnailPanelTest::cleanupTestCase() { delete m_parentWidget; }
void ThumbnailPanelTest::init() {
    m_widget = new ThumbnailPanel(m_parentWidget);
}
void ThumbnailPanelTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}
void ThumbnailPanelTest::testConstruction() { QVERIFY(m_widget != nullptr); }
void ThumbnailPanelTest::testDestruction() {
    auto* w = new ThumbnailPanel(m_parentWidget);
    delete w;
    QVERIFY(true);
}
void ThumbnailPanelTest::testVisibility() {
    m_widget->show();
    QVERIFY(m_widget->isVisible());
}

QTEST_MAIN(ThumbnailPanelTest)
#include "test_thumbnail_panel.moc"
