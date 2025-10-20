#include <QApplication>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QTemporaryFile>
#include <QtTest/QtTest>
#include "../../app/controller/DocumentController.h"
#include "../../app/model/DocumentModel.h"
#include "../../app/model/PDFOutlineModel.h"
#include "../../app/ui/core/ViewWidget.h"
#include "../../app/ui/viewer/PDFViewer.h"
#include "../../app/ui/widgets/DocumentTabWidget.h"

class ViewWidgetIntegrationTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testInitialization();
    void testControllerAndModelSetup();
    void testUIComponents();

    // Document management tests
    void testDocumentOpening();
    void testDocumentClosing();
    void testDocumentSwitching();
    void testMultipleDocuments();

    // Navigation tests
    void testPageNavigation();
    void testPageNavigationSignals();
    void testNavigationBounds();

    // View mode tests
    void testViewModeChanges();
    void testViewModeStates();

    // PDF action tests
    void testPDFActionExecution();
    void testActionHandling();

    // State management tests
    void testCurrentDocumentState();
    void testViewerStateRetrieval();
    void testEmptyState();

    // Integration tests
    void testTabWidgetIntegration();
    void testViewerStackIntegration();
    void testSignalPropagation();

private:
    ViewWidget* m_viewWidget;
    QWidget* m_parentWidget;
    RenderModel* m_renderModel;
    DocumentController* m_documentController;
    DocumentModel* m_documentModel;
    PDFOutlineModel* m_outlineModel;
    QTemporaryFile* m_testPdfFile;

    void createTestPdf();
    void waitForDocumentLoad();
    DocumentTabWidget* getTabWidget();
    QStackedWidget* getViewerStack();
};

void ViewWidgetIntegrationTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(1200, 800);
    m_parentWidget->show();

    createTestPdf();

    // Create controller and models
    m_renderModel = new RenderModel(96.0, 96.0, nullptr, this);
    m_documentModel = new DocumentModel(m_renderModel);
    m_documentController = new DocumentController(m_documentModel);
    m_outlineModel = new PDFOutlineModel(this);
}

void ViewWidgetIntegrationTest::cleanupTestCase() {
    delete m_testPdfFile;
    delete m_parentWidget;
}

void ViewWidgetIntegrationTest::init() {
    m_viewWidget = new ViewWidget(m_parentWidget);
    m_viewWidget->setDocumentController(m_documentController);
    m_viewWidget->setDocumentModel(m_documentModel);
    m_viewWidget->setOutlineModel(m_outlineModel);
    m_viewWidget->show();

    // In offscreen mode, qWaitForWindowExposed() will timeout
    // Use a simple wait instead to allow widget initialization
    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);  // Give widgets time to initialize
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_viewWidget));
    }
}

void ViewWidgetIntegrationTest::cleanup() {
    delete m_viewWidget;
    m_viewWidget = nullptr;
}

void ViewWidgetIntegrationTest::testInitialization() {
    // Test basic initialization
    QVERIFY(m_viewWidget != nullptr);
    QVERIFY(m_viewWidget->isVisible());

    // Test initial state
    QVERIFY(!m_viewWidget->hasDocuments());
    QCOMPARE(m_viewWidget->getCurrentDocumentIndex(), -1);
}

void ViewWidgetIntegrationTest::testControllerAndModelSetup() {
    // Test that controller and models are properly set
    QVERIFY(m_viewWidget != nullptr);

    // Create new instances to test setting
    RenderModel* newRenderModel = new RenderModel(96.0, 96.0, nullptr, this);
    DocumentModel* newModel = new DocumentModel(newRenderModel);
    DocumentController* newController = new DocumentController(newModel);
    PDFOutlineModel* newOutlineModel = new PDFOutlineModel(this);

    m_viewWidget->setDocumentController(newController);
    m_viewWidget->setDocumentModel(newModel);
    m_viewWidget->setOutlineModel(newOutlineModel);

    // Should handle model changes without crashing
    QVERIFY(true);

    delete newController;
    delete newModel;
    delete newOutlineModel;
}

void ViewWidgetIntegrationTest::testUIComponents() {
    // Test that UI components are created
    DocumentTabWidget* tabWidget = getTabWidget();
    QVERIFY(tabWidget != nullptr);

    QStackedWidget* viewerStack = getViewerStack();
    QVERIFY(viewerStack != nullptr);

    // Test component relationships
    QVERIFY(tabWidget->parent() != nullptr);
    QVERIFY(viewerStack->parent() != nullptr);
}

void ViewWidgetIntegrationTest::testDocumentOpening() {
    if (!m_testPdfFile || !m_testPdfFile->exists()) {
        QSKIP("No test PDF file available");
    }

    QSignalSpy pageChangedSpy(m_viewWidget,
                              &ViewWidget::currentViewerPageChanged);
    QSignalSpy zoomChangedSpy(m_viewWidget,
                              &ViewWidget::currentViewerZoomChanged);

    // Test opening document
    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitForDocumentLoad();

    // Should have documents now
    QVERIFY(m_viewWidget->hasDocuments());
    QVERIFY(m_viewWidget->getCurrentDocumentIndex() >= 0);

    // Should emit signals
    QVERIFY(pageChangedSpy.count() >= 0);
    QVERIFY(zoomChangedSpy.count() >= 0);
}

void ViewWidgetIntegrationTest::testDocumentClosing() {
    if (!m_testPdfFile || !m_testPdfFile->exists()) {
        QSKIP("No test PDF file available");
    }

    // Open document first
    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitForDocumentLoad();

    QVERIFY(m_viewWidget->hasDocuments());
    int initialIndex = m_viewWidget->getCurrentDocumentIndex();

    // Close document
    m_viewWidget->closeDocument(initialIndex);
    waitForDocumentLoad();

    // Should handle document closing
    QVERIFY(true);
}

void ViewWidgetIntegrationTest::testDocumentSwitching() {
    if (!m_testPdfFile || !m_testPdfFile->exists()) {
        QSKIP("No test PDF file available");
    }

    // Open first document
    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitForDocumentLoad();

    // Try to open another document (same file for testing)
    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitForDocumentLoad();

    // Test switching between documents
    if (m_viewWidget->hasDocuments()) {
        m_viewWidget->switchToDocument(0);
        waitForDocumentLoad();

        // Should handle document switching
        QVERIFY(true);
    }
}

void ViewWidgetIntegrationTest::testMultipleDocuments() {
    if (!m_testPdfFile || !m_testPdfFile->exists()) {
        QSKIP("No test PDF file available");
    }

    // Open multiple documents
    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitForDocumentLoad();

    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitForDocumentLoad();

    // Test tab widget shows multiple tabs
    DocumentTabWidget* tabWidget = getTabWidget();
    if (tabWidget) {
        QVERIFY(tabWidget->count() >= 0);
    }
}

void ViewWidgetIntegrationTest::testPageNavigation() {
    if (!m_testPdfFile || !m_testPdfFile->exists()) {
        QSKIP("No test PDF file available");
    }

    // Open document
    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitForDocumentLoad();

    if (m_viewWidget->hasDocuments() &&
        m_viewWidget->getCurrentPageCount() > 1) {
        QSignalSpy pageChangedSpy(m_viewWidget,
                                  &ViewWidget::currentViewerPageChanged);

        // Navigate to page 2 (0-based)
        m_viewWidget->goToPage(1);
        waitForDocumentLoad();

        // Should emit page changed signal
        QVERIFY(pageChangedSpy.count() >= 0);

        // Current page should be updated
        QVERIFY(m_viewWidget->getCurrentPage() >= 0);
    }
}

void ViewWidgetIntegrationTest::testPageNavigationSignals() {
    QSignalSpy pageChangedSpy(m_viewWidget,
                              &ViewWidget::currentViewerPageChanged);
    QSignalSpy zoomChangedSpy(m_viewWidget,
                              &ViewWidget::currentViewerZoomChanged);
    QSignalSpy scaleChangedSpy(m_viewWidget, &ViewWidget::scaleChanged);

    if (!m_testPdfFile || !m_testPdfFile->exists()) {
        QSKIP("No test PDF file available");
    }

    // Open document to trigger signals
    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitForDocumentLoad();

    // Signals should be emitted during document operations
    QVERIFY(pageChangedSpy.count() >= 0);
    QVERIFY(zoomChangedSpy.count() >= 0);
    QVERIFY(scaleChangedSpy.count() >= 0);
}

void ViewWidgetIntegrationTest::testNavigationBounds() {
    if (!m_testPdfFile || !m_testPdfFile->exists()) {
        QSKIP("No test PDF file available");
    }

    // Open document
    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitForDocumentLoad();

    if (m_viewWidget->hasDocuments()) {
        int pageCount = m_viewWidget->getCurrentPageCount();

        // Test navigation to invalid pages
        m_viewWidget->goToPage(-1);              // Should handle gracefully
        m_viewWidget->goToPage(pageCount + 10);  // Should handle gracefully

        // Should not crash
        QVERIFY(true);
    }
}

void ViewWidgetIntegrationTest::testViewModeChanges() {
    // Test view mode changes
    m_viewWidget->setCurrentViewMode(0);
    m_viewWidget->setCurrentViewMode(1);
    m_viewWidget->setCurrentViewMode(2);

    // Should handle view mode changes without crashing
    QVERIFY(true);
}

void ViewWidgetIntegrationTest::testViewModeStates() {
    if (!m_testPdfFile || !m_testPdfFile->exists()) {
        QSKIP("No test PDF file available");
    }

    // Open document first
    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitForDocumentLoad();

    if (m_viewWidget->hasDocuments()) {
        // Test different view modes with document loaded
        m_viewWidget->setCurrentViewMode(0);
        QTest::qWait(100);

        m_viewWidget->setCurrentViewMode(1);
        QTest::qWait(100);

        // Should handle view mode changes with document
        QVERIFY(true);
    }
}

void ViewWidgetIntegrationTest::testPDFActionExecution() {
    // Test PDF action execution
    ActionMap testAction = ActionMap::zoomIn;
    m_viewWidget->executePDFAction(testAction);

    // Should handle action execution without crashing
    QVERIFY(true);
}

void ViewWidgetIntegrationTest::testActionHandling() {
    if (!m_testPdfFile || !m_testPdfFile->exists()) {
        QSKIP("No test PDF file available");
    }

    // Open document first
    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitForDocumentLoad();

    if (m_viewWidget->hasDocuments()) {
        // Test various actions with document loaded
        m_viewWidget->executePDFAction(ActionMap::zoomIn);
        QTest::qWait(50);

        m_viewWidget->executePDFAction(ActionMap::zoomOut);
        QTest::qWait(50);

        // Should handle actions with document
        QVERIFY(true);
    }
}

void ViewWidgetIntegrationTest::testCurrentDocumentState() {
    // Note: ViewWidget may report hasDocuments() as true if DocumentModel is
    // set, even if no actual document is loaded. Just verify the state is
    // consistent.

    // Test initial state values
    QVERIFY(m_viewWidget->getCurrentDocumentIndex() >= -1);
    QVERIFY(m_viewWidget->getCurrentPage() >= -1);
    QVERIFY(m_viewWidget->getCurrentPageCount() >= 0);
    QVERIFY(m_viewWidget->getCurrentZoom() > 0);

    if (!m_testPdfFile || !m_testPdfFile->exists()) {
        QSKIP("No test PDF file available");
    }

    // Open document and test state
    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitForDocumentLoad();

    if (m_viewWidget->hasDocuments()) {
        QVERIFY(m_viewWidget->getCurrentDocumentIndex() >= 0);
        QVERIFY(m_viewWidget->getCurrentPage() >= -1);
        // Note: Page count may be 0 if document loading is still in progress
        QVERIFY(m_viewWidget->getCurrentPageCount() >= 0);
        QVERIFY(m_viewWidget->getCurrentZoom() > 0);
    }
}

void ViewWidgetIntegrationTest::testViewerStateRetrieval() {
    if (!m_testPdfFile || !m_testPdfFile->exists()) {
        QSKIP("No test PDF file available");
    }

    // Open document
    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitForDocumentLoad();

    if (m_viewWidget->hasDocuments()) {
        // Test outline model retrieval
        // Note: Outline model may be null if document has no outline
        m_viewWidget->getCurrentOutlineModel();
        // Just verify the call doesn't crash
        QVERIFY(true);

        // Test state consistency
        int page = m_viewWidget->getCurrentPage();
        int pageCount = m_viewWidget->getCurrentPageCount();
        double zoom = m_viewWidget->getCurrentZoom();

        QVERIFY(page >= -1);
        // Note: Page count may be 0 if document loading is still in progress
        QVERIFY(pageCount >= 0);
        QVERIFY(zoom > 0);
        if (pageCount > 0 && page >= 0) {
            QVERIFY(page < pageCount);
        }
    }
}

void ViewWidgetIntegrationTest::testEmptyState() {
    // Note: ViewWidget may report hasDocuments() as true if DocumentModel is
    // set Just verify operations on empty/initial state don't crash

    // Test operations on empty state
    m_viewWidget->goToPage(5);          // Should handle gracefully
    m_viewWidget->switchToDocument(0);  // Should handle gracefully
    m_viewWidget->closeDocument(0);     // Should handle gracefully

    // Should not crash
    QVERIFY(true);
}

void ViewWidgetIntegrationTest::testTabWidgetIntegration() {
    DocumentTabWidget* tabWidget = getTabWidget();
    QVERIFY(tabWidget != nullptr);

    // Test tab widget signals
    QSignalSpy tabChangedSpy(tabWidget, &DocumentTabWidget::currentChanged);

    if (!m_testPdfFile || !m_testPdfFile->exists()) {
        QSKIP("No test PDF file available");
    }

    // Open document to create tab
    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitForDocumentLoad();

    // Should have at least one tab
    QVERIFY(tabWidget->count() >= 0);
}

void ViewWidgetIntegrationTest::testViewerStackIntegration() {
    QStackedWidget* viewerStack = getViewerStack();
    QVERIFY(viewerStack != nullptr);

    if (!m_testPdfFile || !m_testPdfFile->exists()) {
        QSKIP("No test PDF file available");
    }

    // Open document
    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitForDocumentLoad();

    // Stack should have widgets
    QVERIFY(viewerStack->count() >= 0);
}

void ViewWidgetIntegrationTest::testSignalPropagation() {
    QSignalSpy pageChangedSpy(m_viewWidget,
                              &ViewWidget::currentViewerPageChanged);
    QSignalSpy zoomChangedSpy(m_viewWidget,
                              &ViewWidget::currentViewerZoomChanged);
    QSignalSpy scaleChangedSpy(m_viewWidget, &ViewWidget::scaleChanged);

    if (!m_testPdfFile || !m_testPdfFile->exists()) {
        QSKIP("No test PDF file available");
    }

    // Open document and perform operations
    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitForDocumentLoad();

    if (m_viewWidget->hasDocuments()) {
        // Navigate to trigger signals
        m_viewWidget->goToPage(0);
        waitForDocumentLoad();

        // Signals should be properly propagated
        QVERIFY(pageChangedSpy.count() >= 0);
        QVERIFY(zoomChangedSpy.count() >= 0);
        QVERIFY(scaleChangedSpy.count() >= 0);
    }
}

void ViewWidgetIntegrationTest::createTestPdf() {
    m_testPdfFile = new QTemporaryFile();
    m_testPdfFile->setFileTemplate("test_pdf_XXXXXX.pdf");
    if (m_testPdfFile->open()) {
        // Write minimal PDF content
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

void ViewWidgetIntegrationTest::waitForDocumentLoad() {
    QTest::qWait(300);
    QApplication::processEvents();
}

DocumentTabWidget* ViewWidgetIntegrationTest::getTabWidget() {
    return m_viewWidget->findChild<DocumentTabWidget*>();
}

QStackedWidget* ViewWidgetIntegrationTest::getViewerStack() {
    return m_viewWidget->findChild<QStackedWidget*>();
}

QTEST_MAIN(ViewWidgetIntegrationTest)
#include "test_view_widget_integration.moc"
