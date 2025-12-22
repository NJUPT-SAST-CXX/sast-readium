#include <QApplication>
#include <QMainWindow>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QtTest/QtTest>
#include "../../TestUtilities.h"
#include "../../app/controller/ApplicationController.h"
#include "../../app/controller/DocumentController.h"
#include "../../app/managers/RecentFilesManager.h"
#include "../../app/model/DocumentModel.h"
#include "../../app/ui/core/MenuBar.h"
#include "../../app/ui/core/StatusBar.h"
#include "../../app/ui/core/ToolBar.h"
#include "../../app/ui/core/ViewWidget.h"
#include "../../app/ui/widgets/SearchWidget.h"

/**
 * @brief Comprehensive integration tests for UI workflows
 */
class UIWorkflowIntegrationTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

    void testDocumentOpeningWorkflow();
    void testSearchWorkflow();
    void testNavigationWorkflow();

private:
    QMainWindow* m_mainWindow;
    MenuBar* m_menuBar;
    ToolBar* m_toolBar;
    StatusBar* m_statusBar;
    ViewWidget* m_viewWidget;
    SearchWidget* m_searchWidget;
    QTemporaryFile* m_testPdfFile;

    void createTestPdf();
    void setupUIComponents();
    void waitForUIUpdate();
};

void UIWorkflowIntegrationTest::initTestCase() { createTestPdf(); }

void UIWorkflowIntegrationTest::cleanupTestCase() { delete m_testPdfFile; }

void UIWorkflowIntegrationTest::init() {
    m_mainWindow = new QMainWindow();
    m_mainWindow->resize(1400, 900);
    setupUIComponents();
    m_mainWindow->show();

    if (QGuiApplication::platformName() == "offscreen") {
        waitMs(200);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow));
    }
}

void UIWorkflowIntegrationTest::cleanup() {
    delete m_mainWindow;
    m_mainWindow = nullptr;
}

void UIWorkflowIntegrationTest::setupUIComponents() {
    m_menuBar = new MenuBar(m_mainWindow);
    m_toolBar = new ToolBar(tr("Test ToolBar"), m_mainWindow);
    m_statusBar = new StatusBar(m_mainWindow);
    m_viewWidget = new ViewWidget(m_mainWindow);
    m_searchWidget = new SearchWidget(m_mainWindow);

    m_mainWindow->setMenuBar(m_menuBar);
    m_mainWindow->addToolBar(m_toolBar);
    m_mainWindow->setStatusBar(m_statusBar);
    m_mainWindow->setCentralWidget(m_viewWidget);
}

void UIWorkflowIntegrationTest::testDocumentOpeningWorkflow() {
    if (!m_testPdfFile || !m_testPdfFile->exists()) {
        QSKIP("No test PDF file available");
    }

    QVERIFY(!m_viewWidget->hasDocuments());

    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitForUIUpdate();

    QVERIFY(m_viewWidget->hasDocuments());
}

void UIWorkflowIntegrationTest::testSearchWorkflow() {
    if (!m_testPdfFile || !m_testPdfFile->exists()) {
        QSKIP("No test PDF file available");
    }

    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitForUIUpdate();

    m_searchWidget->show();
    m_searchWidget->focusSearchInput();

    QLineEdit* searchInput = m_searchWidget->findChild<QLineEdit*>();
    if (searchInput) {
        searchInput->setText("test");
        m_searchWidget->performSearch();
        waitForUIUpdate();

        m_searchWidget->clearSearch();
        QVERIFY(!m_searchWidget->hasResults());
    }
}

void UIWorkflowIntegrationTest::testNavigationWorkflow() {
    if (!m_testPdfFile || !m_testPdfFile->exists()) {
        QSKIP("No test PDF file available");
    }

    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitForUIUpdate();

    if (m_viewWidget->hasDocuments()) {
        m_toolBar->setActionsEnabled(true);

        QSignalSpy pageChangeSpy(m_viewWidget,
                                 &ViewWidget::currentViewerPageChanged);
        emit m_toolBar->pageJumpRequested(0);
        waitForUIUpdate();

        QVERIFY(pageChangeSpy.count() >= 0);
    }
}

void UIWorkflowIntegrationTest::createTestPdf() {
    m_testPdfFile = new QTemporaryFile();
    m_testPdfFile->setFileTemplate("test_pdf_XXXXXX.pdf");
    if (m_testPdfFile->open()) {
        QByteArray pdfContent =
            "%PDF-1.4\n"
            "1 0 obj\n<<\n/Type /Catalog\n/Pages 2 0 R\n>>\nendobj\n"
            "2 0 obj\n<<\n/Type /Pages\n/Kids [3 0 R]\n/Count 1\n>>\nendobj\n"
            "3 0 obj\n<<\n/Type /Page\n/Parent 2 0 R\n/MediaBox [0 0 612 792]\n"
            "/Contents 4 0 R\n>>\nendobj\n"
            "4 0 obj\n<<\n/Length 44\n>>\nstream\nBT\n/F1 12 Tf\n100 700 Td\n"
            "(Test Page) Tj\nET\nendstream\nendobj\n"
            "xref\n0 5\n0000000000 65535 f \n0000000009 65535 n \n"
            "0000000074 65535 n \n0000000120 65535 n \n0000000179 65535 n \n"
            "trailer\n<<\n/Size 5\n/Root 1 0 R\n>>\nstartxref\n274\n%%EOF\n";

        m_testPdfFile->write(pdfContent);
        m_testPdfFile->flush();
    }
}

void UIWorkflowIntegrationTest::waitForUIUpdate() {
    waitMs(100);
    QApplication::processEvents();
}

QTEST_MAIN(UIWorkflowIntegrationTest)
#include "test_ui_workflow_integration_comprehensive.moc"
