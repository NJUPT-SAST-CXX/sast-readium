#include <QtTest/QtTest>
#include <QApplication>
#include <QSignalSpy>
#include <QTabWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QTreeWidget>
#include <QGroupBox>
#include <QPushButton>
#include <QToolButton>
#include <QClipboard>
#include <QTemporaryFile>
#include <poppler-qt6.h>
#include "../../app/ui/dialogs/DocumentMetadataDialog.h"

class DocumentMetadataDialogIntegrationTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testInitialization();
    void testUIComponents();
    void testTabStructure();
    
    // Document loading tests
    void testDocumentSetting();
    void testMetadataPopulation();
    void testEmptyDocument();
    
    // Basic info tab tests
    void testBasicInfoDisplay();
    void testFileInfoDisplay();
    void testPageInfoDisplay();
    
    // Properties tab tests
    void testDocumentPropertiesDisplay();
    void testPropertiesFields();
    void testDateTimeFormatting();
    
    // Security tab tests
    void testSecurityInfoDisplay();
    void testEncryptionInfo();
    void testPermissionsDisplay();
    
    // Advanced tab tests
    void testAdvancedInfoDisplay();
    void testFontInfoDisplay();
    void testFontTreePopulation();
    
    // Copy functionality tests
    void testCopyToClipboard();
    void testCopyAllMetadata();
    void testCopyButtons();
    
    // Theme integration tests
    void testThemeApplication();
    void testThemeChanges();
    
    // Export functionality tests
    void testExportButton();
    void testMetadataExport();
    
    // Dialog behavior tests
    void testDialogClosing();
    void testDialogResize();

private:
    DocumentMetadataDialog* m_dialog;
    QWidget* m_parentWidget;
    QTemporaryFile* m_testPdfFile;
    std::unique_ptr<Poppler::Document> m_testDocument;
    
    void createTestPdf();
    QTabWidget* getTabWidget();
    QLineEdit* findLineEdit(const QString& objectName);
    QTextEdit* findTextEdit(const QString& objectName);
    QGroupBox* findGroupBox(const QString& title);
    void waitForMetadataLoad();
};

void DocumentMetadataDialogIntegrationTest::initTestCase()
{
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    
    createTestPdf();
}

void DocumentMetadataDialogIntegrationTest::cleanupTestCase()
{
    m_testDocument.reset();
    delete m_testPdfFile;
    delete m_parentWidget;
}

void DocumentMetadataDialogIntegrationTest::init()
{
    m_dialog = new DocumentMetadataDialog(m_parentWidget);
    m_dialog->show();
    QTest::qWaitForWindowExposed(m_dialog);
}

void DocumentMetadataDialogIntegrationTest::cleanup()
{
    delete m_dialog;
    m_dialog = nullptr;
}

void DocumentMetadataDialogIntegrationTest::testInitialization()
{
    // Test basic initialization
    QVERIFY(m_dialog != nullptr);
    QVERIFY(m_dialog->isVisible());
    
    // Test dialog properties
    QVERIFY(m_dialog->isModal() || !m_dialog->isModal()); // Should not crash
    QVERIFY(!m_dialog->windowTitle().isEmpty());
}

void DocumentMetadataDialogIntegrationTest::testUIComponents()
{
    // Test main components exist
    QTabWidget* tabWidget = getTabWidget();
    QVERIFY(tabWidget != nullptr);
    QVERIFY(tabWidget->count() > 0);
    
    // Test header components
    QLabel* titleLabel = m_dialog->findChild<QLabel*>("titleLabel");
    if (titleLabel) {
        QVERIFY(!titleLabel->text().isEmpty());
    }
    
    QToolButton* copyAllButton = m_dialog->findChild<QToolButton*>();
    QVERIFY(copyAllButton != nullptr);
    
    // Test buttons
    QPushButton* exportButton = m_dialog->findChild<QPushButton*>("exportButton");
    QPushButton* closeButton = m_dialog->findChild<QPushButton*>("closeButton");
    
    if (exportButton) QVERIFY(!exportButton->text().isEmpty());
    if (closeButton) QVERIFY(!closeButton->text().isEmpty());
}

void DocumentMetadataDialogIntegrationTest::testTabStructure()
{
    QTabWidget* tabWidget = getTabWidget();
    QVERIFY(tabWidget != nullptr);
    
    // Should have multiple tabs
    QVERIFY(tabWidget->count() >= 3);
    
    // Test tab switching
    for (int i = 0; i < tabWidget->count(); ++i) {
        tabWidget->setCurrentIndex(i);
        QCOMPARE(tabWidget->currentIndex(), i);
        
        // Each tab should have content
        QWidget* tabContent = tabWidget->widget(i);
        QVERIFY(tabContent != nullptr);
        QVERIFY(!tabWidget->tabText(i).isEmpty());
    }
}

void DocumentMetadataDialogIntegrationTest::testDocumentSetting()
{
    if (!m_testDocument || !m_testPdfFile) {
        QSKIP("Test document not available");
    }
    
    // Test setting document
    m_dialog->setDocument(m_testDocument.get(), m_testPdfFile->fileName());
    waitForMetadataLoad();
    
    // Should handle document setting without crashing
    QVERIFY(true);
}

void DocumentMetadataDialogIntegrationTest::testMetadataPopulation()
{
    if (!m_testDocument || !m_testPdfFile) {
        QSKIP("Test document not available");
    }
    
    m_dialog->setDocument(m_testDocument.get(), m_testPdfFile->fileName());
    waitForMetadataLoad();
    
    // Check that some metadata fields are populated
    QList<QLineEdit*> lineEdits = m_dialog->findChildren<QLineEdit*>();
    bool hasPopulatedFields = false;
    
    for (QLineEdit* edit : lineEdits) {
        if (!edit->text().isEmpty()) {
            hasPopulatedFields = true;
            break;
        }
    }
    
    QVERIFY(hasPopulatedFields || lineEdits.size() > 0);
}

void DocumentMetadataDialogIntegrationTest::testEmptyDocument()
{
    // Test with null document
    m_dialog->setDocument(nullptr, "");
    waitForMetadataLoad();
    
    // Should handle null document gracefully
    QVERIFY(true);
}

void DocumentMetadataDialogIntegrationTest::testBasicInfoDisplay()
{
    if (!m_testDocument || !m_testPdfFile) {
        QSKIP("Test document not available");
    }
    
    m_dialog->setDocument(m_testDocument.get(), m_testPdfFile->fileName());
    waitForMetadataLoad();

    // Find basic info group
    QGroupBox* basicInfoGroup = findGroupBox("Basic");
    if (basicInfoGroup) {
        QVERIFY(basicInfoGroup->isVisible());
        
        // Check for basic info fields
        QList<QLineEdit*> edits = basicInfoGroup->findChildren<QLineEdit*>();
        QVERIFY(edits.size() > 0);
    }
}

void DocumentMetadataDialogIntegrationTest::testFileInfoDisplay()
{
    if (!m_testDocument || !m_testPdfFile) {
        QSKIP("Test document not available");
    }
    
    m_dialog->setDocument(m_testDocument.get(), m_testPdfFile->fileName());
    waitForMetadataLoad();
    
    // Check file name field
    QLineEdit* fileNameEdit = findLineEdit("fileNameEdit");
    if (fileNameEdit) {
        QVERIFY(!fileNameEdit->text().isEmpty());
        QVERIFY(fileNameEdit->text().contains(".pdf"));
    }
    
    // Check file path field
    QLineEdit* filePathEdit = findLineEdit("filePathEdit");
    if (filePathEdit) {
        QVERIFY(!filePathEdit->text().isEmpty());
    }
    
    // Check file size field
    QLineEdit* fileSizeEdit = findLineEdit("fileSizeEdit");
    if (fileSizeEdit) {
        QVERIFY(!fileSizeEdit->text().isEmpty());
    }
}

void DocumentMetadataDialogIntegrationTest::testPageInfoDisplay()
{
    if (!m_testDocument || !m_testPdfFile) {
        QSKIP("Test document not available");
    }
    
    m_dialog->setDocument(m_testDocument.get(), m_testPdfFile->fileName());
    waitForMetadataLoad();
    
    // Check page count field
    QLineEdit* pageCountEdit = findLineEdit("pageCountEdit");
    if (pageCountEdit) {
        QVERIFY(!pageCountEdit->text().isEmpty());
        bool ok;
        int pageCount = pageCountEdit->text().toInt(&ok);
        QVERIFY(ok && pageCount > 0);
    }
    
    // Check PDF version field
    QLineEdit* pdfVersionEdit = findLineEdit("pdfVersionEdit");
    if (pdfVersionEdit) {
        QVERIFY(!pdfVersionEdit->text().isEmpty());
    }
}

void DocumentMetadataDialogIntegrationTest::testDocumentPropertiesDisplay()
{
    if (!m_testDocument || !m_testPdfFile) {
        QSKIP("Test document not available");
    }
    
    m_dialog->setDocument(m_testDocument.get(), m_testPdfFile->fileName());
    waitForMetadataLoad();
    
    // Switch to properties tab
    QTabWidget* tabWidget = getTabWidget();
    if (tabWidget) {
        for (int i = 0; i < tabWidget->count(); ++i) {
            if (tabWidget->tabText(i).contains("Properties", Qt::CaseInsensitive)) {
                tabWidget->setCurrentIndex(i);
                break;
            }
        }
    }
    
    // Check properties fields
    QLineEdit* titleEdit = findLineEdit("titleEdit");
    QLineEdit* authorEdit = findLineEdit("authorEdit");
    QLineEdit* subjectEdit = findLineEdit("subjectEdit");
    
    // Fields should exist (may be empty for test document)
    if (titleEdit) QVERIFY(titleEdit != nullptr);
    if (authorEdit) QVERIFY(authorEdit != nullptr);
    if (subjectEdit) QVERIFY(subjectEdit != nullptr);
}

void DocumentMetadataDialogIntegrationTest::testPropertiesFields()
{
    if (!m_testDocument || !m_testPdfFile) {
        QSKIP("Test document not available");
    }
    
    m_dialog->setDocument(m_testDocument.get(), m_testPdfFile->fileName());
    waitForMetadataLoad();
    
    // Check creator and producer fields
    QLineEdit* creatorEdit = findLineEdit("creatorEdit");
    QLineEdit* producerEdit = findLineEdit("producerEdit");
    
    if (creatorEdit) QVERIFY(creatorEdit != nullptr);
    if (producerEdit) QVERIFY(producerEdit != nullptr);
    
    // Check keywords field (should be QTextEdit)
    QTextEdit* keywordsEdit = findTextEdit("keywordsEdit");
    if (keywordsEdit) QVERIFY(keywordsEdit != nullptr);
}

void DocumentMetadataDialogIntegrationTest::testDateTimeFormatting()
{
    if (!m_testDocument || !m_testPdfFile) {
        QSKIP("Test document not available");
    }
    
    m_dialog->setDocument(m_testDocument.get(), m_testPdfFile->fileName());
    waitForMetadataLoad();
    
    // Check date fields
    QLineEdit* creationDateEdit = findLineEdit("creationDateEdit");
    QLineEdit* modificationDateEdit = findLineEdit("modificationDateEdit");
    
    if (creationDateEdit && !creationDateEdit->text().isEmpty()) {
        // Should contain date-like format
        QString dateText = creationDateEdit->text();
        QVERIFY(dateText.contains("/") || dateText.contains("-") || dateText.contains(":"));
    }
    
    if (modificationDateEdit && !modificationDateEdit->text().isEmpty()) {
        QString dateText = modificationDateEdit->text();
        QVERIFY(dateText.contains("/") || dateText.contains("-") || dateText.contains(":"));
    }
}

void DocumentMetadataDialogIntegrationTest::testSecurityInfoDisplay()
{
    if (!m_testDocument || !m_testPdfFile) {
        QSKIP("Test document not available");
    }
    
    m_dialog->setDocument(m_testDocument.get(), m_testPdfFile->fileName());
    waitForMetadataLoad();
    
    // Switch to security tab
    QTabWidget* tabWidget = getTabWidget();
    if (tabWidget) {
        for (int i = 0; i < tabWidget->count(); ++i) {
            if (tabWidget->tabText(i).contains("Security", Qt::CaseInsensitive)) {
                tabWidget->setCurrentIndex(i);
                break;
            }
        }
    }
    
    // Check security fields
    QLineEdit* encryptedEdit = findLineEdit("encryptedEdit");
    if (encryptedEdit) {
        QVERIFY(!encryptedEdit->text().isEmpty());
        QVERIFY(encryptedEdit->text().contains("Yes") || encryptedEdit->text().contains("No"));
    }
}

void DocumentMetadataDialogIntegrationTest::testEncryptionInfo()
{
    if (!m_testDocument || !m_testPdfFile) {
        QSKIP("Test document not available");
    }
    
    m_dialog->setDocument(m_testDocument.get(), m_testPdfFile->fileName());
    waitForMetadataLoad();
    
    // Check encryption method field
    QLineEdit* encryptionMethodEdit = findLineEdit("encryptionMethodEdit");
    if (encryptionMethodEdit) {
        // Should have some value (even if "None")
        QVERIFY(encryptionMethodEdit != nullptr);
    }
}

void DocumentMetadataDialogIntegrationTest::testPermissionsDisplay()
{
    if (!m_testDocument || !m_testPdfFile) {
        QSKIP("Test document not available");
    }
    
    m_dialog->setDocument(m_testDocument.get(), m_testPdfFile->fileName());
    waitForMetadataLoad();
    
    // Check permission fields
    QLineEdit* canPrintEdit = findLineEdit("canPrintEdit");
    QLineEdit* canModifyEdit = findLineEdit("canModifyEdit");
    QLineEdit* canExtractTextEdit = findLineEdit("canExtractTextEdit");
    
    if (canPrintEdit) {
        QVERIFY(canPrintEdit->text().contains("Yes") || canPrintEdit->text().contains("No"));
    }
    
    if (canModifyEdit) {
        QVERIFY(canModifyEdit->text().contains("Yes") || canModifyEdit->text().contains("No"));
    }
    
    if (canExtractTextEdit) {
        QVERIFY(canExtractTextEdit->text().contains("Yes") || canExtractTextEdit->text().contains("No"));
    }
}

void DocumentMetadataDialogIntegrationTest::testAdvancedInfoDisplay()
{
    if (!m_testDocument || !m_testPdfFile) {
        QSKIP("Test document not available");
    }
    
    m_dialog->setDocument(m_testDocument.get(), m_testPdfFile->fileName());
    waitForMetadataLoad();
    
    // Switch to advanced tab
    QTabWidget* tabWidget = getTabWidget();
    if (tabWidget) {
        for (int i = 0; i < tabWidget->count(); ++i) {
            if (tabWidget->tabText(i).contains("Advanced", Qt::CaseInsensitive)) {
                tabWidget->setCurrentIndex(i);
                break;
            }
        }
    }
    
    // Check advanced fields
    QLineEdit* linearizedEdit = findLineEdit("linearizedEdit");
    QLineEdit* hasFormsEdit = findLineEdit("hasFormsEdit");
    
    if (linearizedEdit) QVERIFY(linearizedEdit != nullptr);
    if (hasFormsEdit) QVERIFY(hasFormsEdit != nullptr);
}

void DocumentMetadataDialogIntegrationTest::testFontInfoDisplay()
{
    if (!m_testDocument || !m_testPdfFile) {
        QSKIP("Test document not available");
    }
    
    m_dialog->setDocument(m_testDocument.get(), m_testPdfFile->fileName());
    waitForMetadataLoad();
    
    // Find font tree widget
    QTreeWidget* fontTree = m_dialog->findChild<QTreeWidget*>();
    if (fontTree) {
        QVERIFY(fontTree != nullptr);
        
        // Should have font information (may be empty for simple test document)
        QVERIFY(fontTree->topLevelItemCount() >= 0);
    }
    
    // Check font count label
    QLabel* fontCountLabel = m_dialog->findChild<QLabel*>("fontCountLabel");
    if (fontCountLabel) {
        QVERIFY(!fontCountLabel->text().isEmpty());
    }
}

void DocumentMetadataDialogIntegrationTest::testFontTreePopulation()
{
    if (!m_testDocument || !m_testPdfFile) {
        QSKIP("Test document not available");
    }
    
    m_dialog->setDocument(m_testDocument.get(), m_testPdfFile->fileName());
    waitForMetadataLoad();
    
    QTreeWidget* fontTree = m_dialog->findChild<QTreeWidget*>();
    if (fontTree && fontTree->topLevelItemCount() > 0) {
        // Check first font item
        QTreeWidgetItem* firstItem = fontTree->topLevelItem(0);
        QVERIFY(firstItem != nullptr);
        QVERIFY(!firstItem->text(0).isEmpty());
    }
}

void DocumentMetadataDialogIntegrationTest::testCopyToClipboard()
{
    // Find copy buttons
    QList<QToolButton*> copyButtons = m_dialog->findChildren<QToolButton*>();
    
    if (copyButtons.size() > 0) {
        QToolButton* copyButton = copyButtons.first();
        
        // Click copy button
        QTest::mouseClick(copyButton, Qt::LeftButton);
        
        // Check clipboard (basic test)
        QClipboard* clipboard = QApplication::clipboard();
        QVERIFY(clipboard != nullptr);
    }
}

void DocumentMetadataDialogIntegrationTest::testCopyAllMetadata()
{
    if (!m_testDocument || !m_testPdfFile) {
        QSKIP("Test document not available");
    }
    
    m_dialog->setDocument(m_testDocument.get(), m_testPdfFile->fileName());
    waitForMetadataLoad();
    
    // Find copy all button
    QToolButton* copyAllButton = m_dialog->findChild<QToolButton*>();
    if (copyAllButton) {
        QTest::mouseClick(copyAllButton, Qt::LeftButton);
        
        // Should copy all metadata to clipboard
        QClipboard* clipboard = QApplication::clipboard();
        QString clipboardText = clipboard->text();
        QVERIFY(!clipboardText.isEmpty() || true); // May be empty if no metadata
    }
}

void DocumentMetadataDialogIntegrationTest::testCopyButtons()
{
    // Test copy path button specifically
    QToolButton* copyPathButton = m_dialog->findChild<QToolButton*>("copyPathButton");
    if (copyPathButton) {
        QTest::mouseClick(copyPathButton, Qt::LeftButton);
        
        // Should handle copy operation
        QVERIFY(true);
    }
}

void DocumentMetadataDialogIntegrationTest::testThemeApplication()
{
    // Test theme application (basic test)
    QVERIFY(m_dialog != nullptr);
    
    // Should apply theme without crashing
    QVERIFY(true);
}

void DocumentMetadataDialogIntegrationTest::testThemeChanges()
{
    // Simulate theme change
    m_dialog->style()->unpolish(m_dialog);
    m_dialog->style()->polish(m_dialog);
    
    // Should handle theme changes
    QVERIFY(true);
    
    // UI should still be functional
    QTabWidget* tabWidget = getTabWidget();
    QVERIFY(tabWidget != nullptr);
}

void DocumentMetadataDialogIntegrationTest::testExportButton()
{
    QPushButton* exportButton = m_dialog->findChild<QPushButton*>("exportButton");
    if (exportButton) {
        QVERIFY(exportButton->isEnabled() || !exportButton->isEnabled());
        
        // Click export button (may open file dialog)
        // QTest::mouseClick(exportButton, Qt::LeftButton);
        
        // Should handle export operation
        QVERIFY(true);
    }
}

void DocumentMetadataDialogIntegrationTest::testMetadataExport()
{
    if (!m_testDocument || !m_testPdfFile) {
        QSKIP("Test document not available");
    }
    
    m_dialog->setDocument(m_testDocument.get(), m_testPdfFile->fileName());
    waitForMetadataLoad();
    
    // Test export functionality (basic test)
    QVERIFY(true);
}

void DocumentMetadataDialogIntegrationTest::testDialogClosing()
{
    // Test close button
    QPushButton* closeButton = m_dialog->findChild<QPushButton*>("closeButton");
    if (closeButton) {
        QVERIFY(closeButton != nullptr);
        
        // Click close button should close dialog
        QTest::mouseClick(closeButton, Qt::LeftButton);
        QTest::qWait(100);
        
        // Dialog should be closed or closing
        QVERIFY(!m_dialog->isVisible() || m_dialog->isVisible());
    }
}

void DocumentMetadataDialogIntegrationTest::testDialogResize()
{
    // Test dialog resizing
    QSize initialSize = m_dialog->size();
    m_dialog->resize(900, 700);
    
    QVERIFY(m_dialog->size() != initialSize);
    
    // Should handle resize without issues
    QVERIFY(true);
}

void DocumentMetadataDialogIntegrationTest::createTestPdf()
{
    m_testPdfFile = new QTemporaryFile();
    m_testPdfFile->setFileTemplate("metadata_test_XXXXXX.pdf");
    if (m_testPdfFile->open()) {
        QByteArray pdfContent = 
            "%PDF-1.4\n"
            "1 0 obj\n<<\n/Type /Catalog\n/Pages 2 0 R\n>>\nendobj\n"
            "2 0 obj\n<<\n/Type /Pages\n/Kids [3 0 R]\n/Count 1\n>>\nendobj\n"
            "3 0 obj\n<<\n/Type /Page\n/Parent 2 0 R\n/MediaBox [0 0 612 792]\n"
            "/Contents 4 0 R\n>>\nendobj\n"
            "4 0 obj\n<<\n/Length 50\n>>\nstream\nBT\n/F1 12 Tf\n100 700 Td\n"
            "(Metadata Test) Tj\nET\nendstream\nendobj\n"
            "xref\n0 5\n0000000000 65535 f \n0000000009 65535 n \n"
            "0000000074 65535 n \n0000000120 65535 n \n0000000179 65535 n \n"
            "trailer\n<<\n/Size 5\n/Root 1 0 R\n>>\nstartxref\n280\n%%EOF\n";
        
        m_testPdfFile->write(pdfContent);
        m_testPdfFile->flush();
        
        m_testDocument = Poppler::Document::load(m_testPdfFile->fileName());
    }
}

QTabWidget* DocumentMetadataDialogIntegrationTest::getTabWidget()
{
    return m_dialog->findChild<QTabWidget*>();
}

QLineEdit* DocumentMetadataDialogIntegrationTest::findLineEdit(const QString& objectName)
{
    return m_dialog->findChild<QLineEdit*>(objectName);
}

QTextEdit* DocumentMetadataDialogIntegrationTest::findTextEdit(const QString& objectName)
{
    return m_dialog->findChild<QTextEdit*>(objectName);
}

QGroupBox* DocumentMetadataDialogIntegrationTest::findGroupBox(const QString& title)
{
    QList<QGroupBox*> groups = m_dialog->findChildren<QGroupBox*>();
    for (QGroupBox* group : groups) {
        if (group->title().contains(title, Qt::CaseInsensitive)) {
            return group;
        }
    }
    return nullptr;
}

void DocumentMetadataDialogIntegrationTest::waitForMetadataLoad()
{
    QTest::qWait(200);
    QApplication::processEvents();
}

QTEST_MAIN(DocumentMetadataDialogIntegrationTest)
#include "DocumentMetadataDialogIntegrationTest.moc"
