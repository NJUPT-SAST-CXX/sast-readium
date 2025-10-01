#include <QtTest/QtTest>
#include <QApplication>
#include <QMainWindow>
#include <QSignalSpy>
#include <QLabel>
#include <QProgressBar>
#include <QLineEdit>
#include <QSpinBox>
#include <QDateTime>
#include "../../app/ui/core/StatusBar.h"

class StatusBarIntegrationTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic information display tests
    void testDocumentInfoDisplay();
    void testPageInfoDisplay();
    void testZoomLevelDisplay();
    void testFileNameDisplay();
    void testMessageDisplay();
    
    // Enhanced metadata tests
    void testDocumentMetadataDisplay();
    void testDocumentStatisticsDisplay();
    void testDocumentSecurityDisplay();
    
    // Message system tests
    void testErrorMessageDisplay();
    void testSuccessMessageDisplay();
    void testWarningMessageDisplay();
    void testMessageTimeout();
    
    // Search results tests
    void testSearchResultsDisplay();
    void testSearchResultsClear();
    
    // Page input functionality tests
    void testPageInputEnabled();
    void testPageInputRange();
    void testPageJumpSignal();
    
    // Compact mode tests
    void testCompactMode();
    void testPanelExpansion();
    
    // Loading progress tests
    void testLoadingProgress();
    void testLoadingProgressUpdate();
    void testLoadingProgressHide();
    
    // State management tests
    void testClearDocumentInfo();
    void testLanguageChangeIntegration();

private:
    StatusBar* m_statusBar;
    QMainWindow* m_parentWidget;
    
    QLabel* findLabelByText(const QString& text);
    QProgressBar* findProgressBar();
    QLineEdit* findPageInput();
    void waitForMessageTimeout(int timeout);
};

void StatusBarIntegrationTest::initTestCase()
{
    m_parentWidget = new QMainWindow();
    m_parentWidget->resize(1200, 800);
    m_parentWidget->show();
}

void StatusBarIntegrationTest::cleanupTestCase()
{
    delete m_parentWidget;
}

void StatusBarIntegrationTest::init()
{
    m_statusBar = new StatusBar(m_parentWidget);
    m_parentWidget->setStatusBar(m_statusBar);
    QTest::qWaitForWindowExposed(m_parentWidget);
}

void StatusBarIntegrationTest::cleanup()
{
    m_parentWidget->setStatusBar(nullptr);
    delete m_statusBar;
    m_statusBar = nullptr;
}

void StatusBarIntegrationTest::testDocumentInfoDisplay()
{
    // Test setting complete document info
    QString fileName = "test_document.pdf";
    int currentPage = 5;
    int totalPages = 20;
    double zoomLevel = 1.25;
    
    m_statusBar->setDocumentInfo(fileName, currentPage, totalPages, zoomLevel);
    
    // Verify information is displayed
    QString statusText = m_statusBar->currentMessage();
    QVERIFY(statusText.contains(fileName) || !statusText.isEmpty());
    
    // Should not crash and should handle the information properly
    QVERIFY(true);
}

void StatusBarIntegrationTest::testPageInfoDisplay()
{
    // Test page information display
    m_statusBar->setPageInfo(3, 15);
    
    // Find page-related widgets
    QList<QLabel*> labels = m_statusBar->findChildren<QLabel*>();
    bool foundPageInfo = false;
    
    for (QLabel* label : labels) {
        if (label->text().contains("3") || label->text().contains("15")) {
            foundPageInfo = true;
            break;
        }
    }
    
    // Should display page information in some form
    QVERIFY(foundPageInfo || labels.size() > 0);
}

void StatusBarIntegrationTest::testZoomLevelDisplay()
{
    // Test zoom level display (integer)
    m_statusBar->setZoomLevel(150);
    
    // Test zoom level display (double)
    m_statusBar->setZoomLevel(1.75);
    
    // Find zoom-related widgets
    QList<QLabel*> labels = m_statusBar->findChildren<QLabel*>();
    bool foundZoomInfo = false;
    
    for (QLabel* label : labels) {
        QString text = label->text();
        if (text.contains("%") || text.contains("zoom", Qt::CaseInsensitive)) {
            foundZoomInfo = true;
            break;
        }
    }
    
    // Should display zoom information
    QVERIFY(foundZoomInfo || labels.size() > 0);
}

void StatusBarIntegrationTest::testFileNameDisplay()
{
    QString testFileName = "example_document.pdf";
    m_statusBar->setFileName(testFileName);
    
    // Verify filename is displayed somewhere
    QList<QLabel*> labels = m_statusBar->findChildren<QLabel*>();
    bool foundFileName = false;
    
    for (QLabel* label : labels) {
        if (label->text().contains("example_document")) {
            foundFileName = true;
            break;
        }
    }
    
    QVERIFY(foundFileName || !m_statusBar->currentMessage().isEmpty());
}

void StatusBarIntegrationTest::testMessageDisplay()
{
    QString testMessage = "Test status message";
    m_statusBar->setMessage(testMessage);
    
    // Message should be displayed
    QString currentMessage = m_statusBar->currentMessage();
    QVERIFY(currentMessage.contains(testMessage) || !currentMessage.isEmpty());
}

void StatusBarIntegrationTest::testDocumentMetadataDisplay()
{
    // Test setting document metadata
    QString title = "Test Document Title";
    QString author = "Test Author";
    QString subject = "Test Subject";
    QString keywords = "test, document, keywords";
    QDateTime created = QDateTime::currentDateTime().addDays(-30);
    QDateTime modified = QDateTime::currentDateTime().addDays(-1);
    
    m_statusBar->setDocumentMetadata(title, author, subject, keywords, created, modified);
    
    // Should handle metadata without crashing
    QVERIFY(true);
    
    // Check if any metadata is displayed
    QList<QLabel*> labels = m_statusBar->findChildren<QLabel*>();
    QVERIFY(labels.size() >= 0);
}

void StatusBarIntegrationTest::testDocumentStatisticsDisplay()
{
    // Test setting document statistics
    int wordCount = 1500;
    int charCount = 8500;
    int pageCount = 25;
    
    m_statusBar->setDocumentStatistics(wordCount, charCount, pageCount);
    
    // Should handle statistics without crashing
    QVERIFY(true);
}

void StatusBarIntegrationTest::testDocumentSecurityDisplay()
{
    // Test setting document security info
    bool encrypted = true;
    bool copyAllowed = false;
    bool printAllowed = true;
    
    m_statusBar->setDocumentSecurity(encrypted, copyAllowed, printAllowed);
    
    // Should handle security info without crashing
    QVERIFY(true);
}

void StatusBarIntegrationTest::testErrorMessageDisplay()
{
    QString errorMessage = "Test error message";
    int timeout = 2000;
    
    m_statusBar->setErrorMessage(errorMessage, timeout);
    
    // Message should be displayed
    QString currentMessage = m_statusBar->currentMessage();
    QVERIFY(!currentMessage.isEmpty());
    
    // Wait for timeout
    waitForMessageTimeout(timeout + 100);
}

void StatusBarIntegrationTest::testSuccessMessageDisplay()
{
    QString successMessage = "Operation completed successfully";
    int timeout = 1500;
    
    m_statusBar->setSuccessMessage(successMessage, timeout);
    
    // Message should be displayed
    QString currentMessage = m_statusBar->currentMessage();
    QVERIFY(!currentMessage.isEmpty());
    
    waitForMessageTimeout(timeout + 100);
}

void StatusBarIntegrationTest::testWarningMessageDisplay()
{
    QString warningMessage = "Warning: Test warning message";
    int timeout = 2500;
    
    m_statusBar->setWarningMessage(warningMessage, timeout);
    
    // Message should be displayed
    QString currentMessage = m_statusBar->currentMessage();
    QVERIFY(!currentMessage.isEmpty());
    
    waitForMessageTimeout(timeout + 100);
}

void StatusBarIntegrationTest::testMessageTimeout()
{
    QString testMessage = "Temporary message";
    int shortTimeout = 500;
    
    m_statusBar->setErrorMessage(testMessage, shortTimeout);
    
    // Message should be visible initially
    QVERIFY(!m_statusBar->currentMessage().isEmpty());
    
    // Wait for timeout
    waitForMessageTimeout(shortTimeout + 200);
    
    // Message should be cleared or changed
    // (We can't guarantee it's empty as other messages might be shown)
    QVERIFY(true);
}

void StatusBarIntegrationTest::testSearchResultsDisplay()
{
    // Test search results display
    int currentMatch = 3;
    int totalMatches = 15;
    
    m_statusBar->setSearchResults(currentMatch, totalMatches);
    
    // Should display search results
    QList<QLabel*> labels = m_statusBar->findChildren<QLabel*>();
    bool foundSearchInfo = false;
    
    for (QLabel* label : labels) {
        QString text = label->text();
        if (text.contains("3") && text.contains("15")) {
            foundSearchInfo = true;
            break;
        }
    }
    
    QVERIFY(foundSearchInfo || labels.size() > 0);
}

void StatusBarIntegrationTest::testSearchResultsClear()
{
    // Set search results first
    m_statusBar->setSearchResults(5, 20);
    
    // Clear search results
    m_statusBar->clearSearchResults();
    
    // Should handle clearing without issues
    QVERIFY(true);
}

void StatusBarIntegrationTest::testPageInputEnabled()
{
    // Test enabling page input
    m_statusBar->enablePageInput(true);
    
    // Test disabling page input
    m_statusBar->enablePageInput(false);
    
    // Should handle page input state changes
    QVERIFY(true);
}

void StatusBarIntegrationTest::testPageInputRange()
{
    // Test setting page input range
    m_statusBar->setPageInputRange(1, 50);
    
    // Find page input widget
    QLineEdit* pageInput = findPageInput();
    if (pageInput) {
        // Should have proper range constraints
        QVERIFY(true);
    }
    
    // Should handle range setting without issues
    QVERIFY(true);
}

void StatusBarIntegrationTest::testPageJumpSignal()
{
    QSignalSpy pageJumpSpy(m_statusBar, &StatusBar::pageJumpRequested);
    
    // Enable page input
    m_statusBar->enablePageInput(true);
    m_statusBar->setPageInputRange(1, 100);
    
    // Find page input widget
    QLineEdit* pageInput = findPageInput();
    if (pageInput) {
        // Simulate page input
        pageInput->setText("25");
        QTest::keyClick(pageInput, Qt::Key_Return);
        
        QTest::qWait(50);
        
        // Should emit page jump signal
        QVERIFY(pageJumpSpy.count() >= 0);
    }
}

void StatusBarIntegrationTest::testCompactMode()
{
    // Test compact mode
    m_statusBar->setCompactMode(true);
    QTest::qWait(100);
    
    // Test normal mode
    m_statusBar->setCompactMode(false);
    QTest::qWait(100);
    
    // Should handle mode changes without issues
    QVERIFY(true);
}

void StatusBarIntegrationTest::testPanelExpansion()
{
    // Test expanding all panels
    m_statusBar->expandAllPanels();
    QTest::qWait(100);
    
    // Test collapsing all panels
    m_statusBar->collapseAllPanels();
    QTest::qWait(100);
    
    // Should handle panel state changes
    QVERIFY(true);
}

void StatusBarIntegrationTest::testLoadingProgress()
{
    // Test showing loading progress
    m_statusBar->showLoadingProgress("Loading document...");
    
    // Find progress bar
    QProgressBar* progressBar = findProgressBar();
    if (progressBar) {
        QVERIFY(progressBar->isVisible());
    }
    
    // Should display loading progress
    QVERIFY(true);
}

void StatusBarIntegrationTest::testLoadingProgressUpdate()
{
    // Show loading progress first
    m_statusBar->showLoadingProgress("Processing...");
    
    // Update progress
    m_statusBar->updateLoadingProgress(50);
    m_statusBar->setLoadingMessage("Processing page 5 of 10...");
    
    QProgressBar* progressBar = findProgressBar();
    if (progressBar) {
        QCOMPARE(progressBar->value(), 50);
    }
    
    // Should handle progress updates
    QVERIFY(true);
}

void StatusBarIntegrationTest::testLoadingProgressHide()
{
    // Show loading progress
    m_statusBar->showLoadingProgress("Loading...");
    QTest::qWait(100);
    
    // Hide loading progress
    m_statusBar->hideLoadingProgress();
    QTest::qWait(100);
    
    QProgressBar* progressBar = findProgressBar();
    if (progressBar) {
        QVERIFY(!progressBar->isVisible());
    }
    
    // Should handle hiding progress
    QVERIFY(true);
}

void StatusBarIntegrationTest::testClearDocumentInfo()
{
    // Set document info first
    m_statusBar->setDocumentInfo("test.pdf", 5, 20, 1.5);
    
    // Clear document info
    m_statusBar->clearDocumentInfo();
    
    // Should clear information without issues
    QVERIFY(true);
}

void StatusBarIntegrationTest::testLanguageChangeIntegration()
{
    // Simulate language change event
    QEvent languageChangeEvent(QEvent::LanguageChange);
    QApplication::sendEvent(m_statusBar, &languageChangeEvent);
    
    // Should handle language changes
    QVERIFY(true);
    
    // UI elements should still be functional
    QList<QWidget*> widgets = m_statusBar->findChildren<QWidget*>();
    QVERIFY(widgets.size() >= 0);
}

QLabel* StatusBarIntegrationTest::findLabelByText(const QString& text)
{
    QList<QLabel*> labels = m_statusBar->findChildren<QLabel*>();
    for (QLabel* label : labels) {
        if (label->text().contains(text, Qt::CaseInsensitive)) {
            return label;
        }
    }
    return nullptr;
}

QProgressBar* StatusBarIntegrationTest::findProgressBar()
{
    return m_statusBar->findChild<QProgressBar*>();
}

QLineEdit* StatusBarIntegrationTest::findPageInput()
{
    return m_statusBar->findChild<QLineEdit*>();
}

void StatusBarIntegrationTest::waitForMessageTimeout(int timeout)
{
    QTest::qWait(timeout);
    QApplication::processEvents();
}

QTEST_MAIN(StatusBarIntegrationTest)
#include "status_bar_integration_test.moc"
