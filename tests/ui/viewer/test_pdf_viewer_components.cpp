#include <QApplication>
#include <QtTest/QtTest>
#include "../../../app/ui/viewer/PDFViewerComponents.h"

class PDFViewerComponentsTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testComponentsExist();

private:
    QWidget* m_parentWidget;
};

void PDFViewerComponentsTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
    }
}

void PDFViewerComponentsTest::cleanupTestCase() { delete m_parentWidget; }

void PDFViewerComponentsTest::init() {}

void PDFViewerComponentsTest::cleanup() {}

void PDFViewerComponentsTest::testComponentsExist() {
    QVERIFY(true);  // Header exists and compiles
}

QTEST_MAIN(PDFViewerComponentsTest)
#include "test_pdf_viewer_components.moc"
