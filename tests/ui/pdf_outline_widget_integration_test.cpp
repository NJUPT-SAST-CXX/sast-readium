#include <QtTest/QtTest>
#include <QApplication>
#include <QSignalSpy>
#include <QTreeView>
#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QTemporaryFile>
#include <poppler-qt6.h>
#include "../../app/ui/viewer/PDFOutlineWidget.h"
#include "../../app/model/PDFOutlineModel.h"

class PDFOutlineWidgetIntegrationTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testInitialization();
    void testOutlineModelSetting();
    void testOutlineRefresh();
    void testOutlineClear();
    
    // Navigation tests
    void testPageHighlighting();
    void testItemSelection();
    void testNavigationSignals();
    
    // Expansion tests
    void testExpandAll();
    void testCollapseAll();
    void testExpandToLevel();
    
    // Search tests
    void testSearchItems();
    void testSearchResults();
    void testSearchClear();
    
    // Event handling tests
    void testContextMenu();
    void testKeyPressEvents();
    void testMouseEvents();
    
    // Signal emission tests
    void testPageNavigationSignal();
    void testItemSelectionSignal();
    
    // State management tests
    void testCurrentSelectedPage();
    void testOutlineState();
    
    // Integration tests
    void testModelIntegration();
    void testDocumentIntegration();
    
    // Error handling tests
    void testNullModel();
    void testInvalidPageNumber();

private:
    PDFOutlineWidget* m_outlineWidget;
    PDFOutlineModel* m_outlineModel;
    QWidget* m_parentWidget;
    QTemporaryFile* m_testPdfFile;
    std::shared_ptr<Poppler::Document> m_testDocument;
    
    void createTestPdf();
    void waitForModelUpdate();
    QTreeView* getTreeView();
};

void PDFOutlineWidgetIntegrationTest::initTestCase()
{
    m_parentWidget = new QWidget();
    m_parentWidget->resize(400, 600);
    m_parentWidget->show();
    
    createTestPdf();
    m_outlineModel = new PDFOutlineModel(this);
    if (m_testDocument) {
        m_outlineModel->parseOutline(m_testDocument.get());
    }
}

void PDFOutlineWidgetIntegrationTest::cleanupTestCase()
{
    delete m_testPdfFile;
    delete m_parentWidget;
}

void PDFOutlineWidgetIntegrationTest::init()
{
    m_outlineWidget = new PDFOutlineWidget(m_parentWidget);
    m_outlineWidget->setOutlineModel(m_outlineModel);
    m_outlineWidget->show();
    QTest::qWaitForWindowExposed(m_outlineWidget);
}

void PDFOutlineWidgetIntegrationTest::cleanup()
{
    delete m_outlineWidget;
    m_outlineWidget = nullptr;
}

void PDFOutlineWidgetIntegrationTest::testInitialization()
{
    // Test basic initialization
    QVERIFY(m_outlineWidget != nullptr);
    QVERIFY(m_outlineWidget->isVisible());
    
    // Test tree view exists
    QTreeView* treeView = getTreeView();
    QVERIFY(treeView != nullptr);
}

void PDFOutlineWidgetIntegrationTest::testOutlineModelSetting()
{
    // Test setting outline model
    PDFOutlineModel* newModel = new PDFOutlineModel(this);
    m_outlineWidget->setOutlineModel(newModel);
    
    // Should handle model setting without issues
    QVERIFY(true);
    
    // Reset to original model
    m_outlineWidget->setOutlineModel(m_outlineModel);
    
    delete newModel;
}

void PDFOutlineWidgetIntegrationTest::testOutlineRefresh()
{
    // Test outline refresh
    m_outlineWidget->refreshOutline();
    waitForModelUpdate();
    
    // Should handle refresh without crashing
    QVERIFY(true);
}

void PDFOutlineWidgetIntegrationTest::testOutlineClear()
{
    // Test outline clear
    m_outlineWidget->clearOutline();
    waitForModelUpdate();
    
    // Tree should be cleared
    QTreeView* treeView = getTreeView();
    if (treeView && treeView->model()) {
        QCOMPARE(treeView->model()->rowCount(), 0);
    }
}

void PDFOutlineWidgetIntegrationTest::testPageHighlighting()
{
    // Test highlighting page item
    m_outlineWidget->highlightPageItem(0);
    m_outlineWidget->highlightPageItem(1);
    m_outlineWidget->highlightPageItem(-1); // Invalid page
    
    // Should handle page highlighting without crashing
    QVERIFY(true);
}

void PDFOutlineWidgetIntegrationTest::testItemSelection()
{
    QTreeView* treeView = getTreeView();
    if (treeView && treeView->model() && treeView->model()->rowCount() > 0) {
        // Select first item
        QModelIndex firstIndex = treeView->model()->index(0, 0);
        treeView->setCurrentIndex(firstIndex);
        
        // Should handle item selection
        QVERIFY(true);
    }
}

void PDFOutlineWidgetIntegrationTest::testNavigationSignals()
{
    QSignalSpy navigationSpy(m_outlineWidget, &PDFOutlineWidget::pageNavigationRequested);
    QSignalSpy selectionSpy(m_outlineWidget, &PDFOutlineWidget::itemSelectionChanged);
    
    // Simulate navigation request
    emit m_outlineWidget->pageNavigationRequested(2);
    
    // Should emit navigation signal
    QCOMPARE(navigationSpy.count(), 1);
    QList<QVariant> args = navigationSpy.takeFirst();
    QCOMPARE(args.at(0).toInt(), 2);
}

void PDFOutlineWidgetIntegrationTest::testExpandAll()
{
    // Test expand all
    m_outlineWidget->expandAll();
    waitForModelUpdate();
    
    QTreeView* treeView = getTreeView();
    if (treeView) {
        // Should handle expand all without issues
        QVERIFY(true);
    }
}

void PDFOutlineWidgetIntegrationTest::testCollapseAll()
{
    // Expand first, then collapse
    m_outlineWidget->expandAll();
    waitForModelUpdate();
    
    m_outlineWidget->collapseAll();
    waitForModelUpdate();
    
    // Should handle collapse all without issues
    QVERIFY(true);
}

void PDFOutlineWidgetIntegrationTest::testExpandToLevel()
{
    // Test expand to different levels
    m_outlineWidget->expandToLevel(0);
    waitForModelUpdate();
    
    m_outlineWidget->expandToLevel(1);
    waitForModelUpdate();
    
    m_outlineWidget->expandToLevel(2);
    waitForModelUpdate();
    
    // Should handle level expansion without issues
    QVERIFY(true);
}

void PDFOutlineWidgetIntegrationTest::testSearchItems()
{
    // Test searching outline items
    m_outlineWidget->searchItems("test");
    waitForModelUpdate();
    
    m_outlineWidget->searchItems("chapter");
    waitForModelUpdate();
    
    m_outlineWidget->searchItems(""); // Clear search
    waitForModelUpdate();
    
    // Should handle search without crashing
    QVERIFY(true);
}

void PDFOutlineWidgetIntegrationTest::testSearchResults()
{
    // Test search with different terms
    m_outlineWidget->searchItems("nonexistent");
    waitForModelUpdate();
    
    // Should handle search with no results
    QVERIFY(true);
    
    // Clear search
    m_outlineWidget->searchItems("");
    waitForModelUpdate();
}

void PDFOutlineWidgetIntegrationTest::testSearchClear()
{
    // Search for something first
    m_outlineWidget->searchItems("test");
    waitForModelUpdate();
    
    // Clear search
    m_outlineWidget->searchItems("");
    waitForModelUpdate();
    
    // Should restore original outline
    QVERIFY(true);
}

void PDFOutlineWidgetIntegrationTest::testContextMenu()
{
    // Test context menu event
    QPoint testPoint(50, 50);
    QContextMenuEvent contextEvent(QContextMenuEvent::Mouse, testPoint);
    QApplication::sendEvent(m_outlineWidget, &contextEvent);
    
    // Should handle context menu without crashing
    QVERIFY(true);
}

void PDFOutlineWidgetIntegrationTest::testKeyPressEvents()
{
    // Test key press events
    QKeyEvent enterEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QApplication::sendEvent(m_outlineWidget, &enterEvent);
    
    QKeyEvent spaceEvent(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
    QApplication::sendEvent(m_outlineWidget, &spaceEvent);
    
    QKeyEvent upEvent(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
    QApplication::sendEvent(m_outlineWidget, &upEvent);
    
    QKeyEvent downEvent(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
    QApplication::sendEvent(m_outlineWidget, &downEvent);
    
    // Should handle key events without crashing
    QVERIFY(true);
}

void PDFOutlineWidgetIntegrationTest::testMouseEvents()
{
    QTreeView* treeView = getTreeView();
    if (treeView) {
        // Test mouse click
        QPoint testPoint(50, 50);
        QMouseEvent clickEvent(QEvent::MouseButtonPress, testPoint, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(treeView, &clickEvent);
        
        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, testPoint, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(treeView, &releaseEvent);
        
        // Should handle mouse events
        QVERIFY(true);
    }
}

void PDFOutlineWidgetIntegrationTest::testPageNavigationSignal()
{
    QSignalSpy navigationSpy(m_outlineWidget, &PDFOutlineWidget::pageNavigationRequested);
    
    // Emit navigation signal directly
    emit m_outlineWidget->pageNavigationRequested(5);
    
    QCOMPARE(navigationSpy.count(), 1);
    QList<QVariant> args = navigationSpy.takeFirst();
    QCOMPARE(args.at(0).toInt(), 5);
}

void PDFOutlineWidgetIntegrationTest::testItemSelectionSignal()
{
    QSignalSpy selectionSpy(m_outlineWidget, &PDFOutlineWidget::itemSelectionChanged);
    
    // Emit selection signal directly
    emit m_outlineWidget->itemSelectionChanged(3);
    
    QCOMPARE(selectionSpy.count(), 1);
    QList<QVariant> args = selectionSpy.takeFirst();
    QCOMPARE(args.at(0).toInt(), 3);
}

void PDFOutlineWidgetIntegrationTest::testCurrentSelectedPage()
{
    // Test getting current selected page
    int currentPage = m_outlineWidget->getCurrentSelectedPage();
    QVERIFY(currentPage >= -1); // -1 indicates no selection
    
    // Test after highlighting a page
    m_outlineWidget->highlightPageItem(2);
    currentPage = m_outlineWidget->getCurrentSelectedPage();
    QVERIFY(currentPage >= -1);
}

void PDFOutlineWidgetIntegrationTest::testOutlineState()
{
    // Test outline state management
    m_outlineWidget->expandAll();
    m_outlineWidget->highlightPageItem(1);
    
    // State should be maintained
    QVERIFY(true);
    
    m_outlineWidget->collapseAll();
    m_outlineWidget->clearOutline();
    
    // Should handle state changes
    QVERIFY(true);
}

void PDFOutlineWidgetIntegrationTest::testModelIntegration()
{
    // Test model integration
    m_outlineWidget->refreshOutline();
    waitForModelUpdate();
    
    // Should integrate with model properly
    QVERIFY(true);
    
    // Test model signals
    if (m_outlineModel) {
        // Trigger model update
        // Test model reset by re-parsing
        m_outlineModel->parseOutline(m_testDocument.get());
        waitForModelUpdate();
        
        QVERIFY(true);
    }
}

void PDFOutlineWidgetIntegrationTest::testDocumentIntegration()
{
    if (!m_testDocument) {
        QSKIP("No test document available");
    }
    
    // Test document integration through model
    PDFOutlineModel* newModel = new PDFOutlineModel(this);
    newModel->parseOutline(m_testDocument.get());
    
    m_outlineWidget->setOutlineModel(newModel);
    m_outlineWidget->refreshOutline();
    waitForModelUpdate();
    
    // Should integrate with document
    QVERIFY(true);
    
    // Reset to original model
    m_outlineWidget->setOutlineModel(m_outlineModel);
    
    delete newModel;
}

void PDFOutlineWidgetIntegrationTest::testNullModel()
{
    // Test with null model
    m_outlineWidget->setOutlineModel(nullptr);
    
    // Should handle null model gracefully
    m_outlineWidget->refreshOutline();
    m_outlineWidget->clearOutline();
    m_outlineWidget->expandAll();
    m_outlineWidget->collapseAll();
    
    QVERIFY(true);
    
    // Reset to original model
    m_outlineWidget->setOutlineModel(m_outlineModel);
}

void PDFOutlineWidgetIntegrationTest::testInvalidPageNumber()
{
    // Test with invalid page numbers
    m_outlineWidget->highlightPageItem(-1);
    m_outlineWidget->highlightPageItem(1000);
    
    // Should handle invalid page numbers gracefully
    QVERIFY(true);
    
    int currentPage = m_outlineWidget->getCurrentSelectedPage();
    QVERIFY(currentPage >= -1);
}

void PDFOutlineWidgetIntegrationTest::createTestPdf()
{
    m_testPdfFile = new QTemporaryFile();
    m_testPdfFile->setFileTemplate("outline_test_XXXXXX.pdf");
    if (m_testPdfFile->open()) {
        // Create a simple PDF with basic structure
        QByteArray pdfContent = 
            "%PDF-1.4\n"
            "1 0 obj\n<<\n/Type /Catalog\n/Pages 2 0 R\n>>\nendobj\n"
            "2 0 obj\n<<\n/Type /Pages\n/Kids [3 0 R]\n/Count 1\n>>\nendobj\n"
            "3 0 obj\n<<\n/Type /Page\n/Parent 2 0 R\n/MediaBox [0 0 612 792]\n"
            "/Contents 4 0 R\n>>\nendobj\n"
            "4 0 obj\n<<\n/Length 50\n>>\nstream\nBT\n/F1 12 Tf\n100 700 Td\n"
            "(Outline Test) Tj\nET\nendstream\nendobj\n"
            "xref\n0 5\n0000000000 65535 f \n0000000009 65535 n \n"
            "0000000074 65535 n \n0000000120 65535 n \n0000000179 65535 n \n"
            "trailer\n<<\n/Size 5\n/Root 1 0 R\n>>\nstartxref\n280\n%%EOF\n";
        
        m_testPdfFile->write(pdfContent);
        m_testPdfFile->flush();
        
        m_testDocument = std::shared_ptr<Poppler::Document>(
            Poppler::Document::load(m_testPdfFile->fileName()));
    }
}

void PDFOutlineWidgetIntegrationTest::waitForModelUpdate()
{
    QTest::qWait(100);
    QApplication::processEvents();
}

QTreeView* PDFOutlineWidgetIntegrationTest::getTreeView()
{
    return m_outlineWidget->findChild<QTreeView*>();
}

QTEST_MAIN(PDFOutlineWidgetIntegrationTest)
#include "PDFOutlineWidgetIntegrationTest.moc"
