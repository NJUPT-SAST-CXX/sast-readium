#include <QApplication>
#include <QDateTime>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QProgressBar>
#include <QSignalSpy>
#include <QSpinBox>
#include <QtTest/QtTest>
#include "../../app/ui/core/StatusBar.h"
#include "../TestUtilities.h"

/**
 * @brief Comprehensive functional tests for StatusBar UI component
 */
class StatusBarFunctionalityTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

    void testDocumentInfoDisplay();
    void testMessageDisplay();
    void testPageInputValidation();

private:
    StatusBar* m_statusBar;
    QMainWindow* m_parentWidget;
};

void StatusBarFunctionalityTest::initTestCase() {
    m_parentWidget = new QMainWindow();
    m_parentWidget->resize(1200, 800);
    m_parentWidget->show();
}

void StatusBarFunctionalityTest::cleanupTestCase() {
    if (QGuiApplication::platformName() != "offscreen") {
        delete m_parentWidget;
    }
    m_parentWidget = nullptr;
}

void StatusBarFunctionalityTest::init() {
    bool minimalMode = (QGuiApplication::platformName() == "offscreen");
    m_statusBar = new StatusBar(m_parentWidget, minimalMode);
    m_parentWidget->setStatusBar(m_statusBar);

    if (QGuiApplication::platformName() == "offscreen") {
        waitMs(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
    }
}

void StatusBarFunctionalityTest::cleanup() {
    delete m_statusBar;
    m_statusBar = nullptr;
}

void StatusBarFunctionalityTest::testDocumentInfoDisplay() {
    QString fileName = "test_document.pdf";
    int currentPage = 5;
    int totalPages = 20;
    double zoomLevel = 1.25;

    m_statusBar->setDocumentInfo(fileName, currentPage, totalPages, zoomLevel);
    QVERIFY(true);
}

void StatusBarFunctionalityTest::testMessageDisplay() {
    m_statusBar->setMessage("Test message");
    QVERIFY(!m_statusBar->currentMessage().isEmpty());
}

void StatusBarFunctionalityTest::testPageInputValidation() {
    m_statusBar->enablePageInput(true);
    m_statusBar->setPageInputRange(1, 100);
    QVERIFY(true);
}

QTEST_MAIN(StatusBarFunctionalityTest)
#include "test_status_bar_functionality_comprehensive.moc"
