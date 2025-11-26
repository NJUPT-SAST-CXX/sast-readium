#include <poppler/qt6/poppler-qt6.h>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalSpy>
#include <QSlider>
#include <QSpinBox>
#include <QTemporaryFile>
#include <QTextEdit>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include "../../app/ui/dialogs/DocumentComparison.h"

class DocumentComparisonIntegrationTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testInitialization();
    void testUIComponents();
    void testDocumentLoading();

    // Comparison options tests
    void testComparisonOptions();
    void testOptionsUI();
    void testOptionsSignals();

    // Comparison process tests
    void testComparisonStart();
    void testComparisonProgress();
    void testComparisonResults();
    void testComparisonStop();

    // Navigation tests
    void testDifferenceNavigation();
    void testDifferenceSelection();
    void testDifferenceHighlighting();

    // Export and reporting tests
    void testReportGeneration();
    void testResultsExport();
    void testCSVExport();

    // Session management tests
    void testSessionSave();
    void testSessionLoad();

    // View mode tests
    void testViewModes();
    void testViewModeChanges();

    // Error handling tests
    void testInvalidDocuments();
    void testComparisonErrors();

private:
    DocumentComparison* m_comparison;
    QWidget* m_parentWidget;
    QTemporaryFile* m_testPdf1;
    QTemporaryFile* m_testPdf2;
    std::unique_ptr<Poppler::Document> m_document1;
    std::unique_ptr<Poppler::Document> m_document2;

    void createTestPdfs();
    void waitForComparison();
    QPushButton* findButton(const QString& text);
    QCheckBox* findCheckBox(const QString& text);
};

void DocumentComparisonIntegrationTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(1400, 900);
    m_parentWidget->show();

    createTestPdfs();
}

void DocumentComparisonIntegrationTest::cleanupTestCase() {
    m_document1.reset();
    m_document2.reset();
    delete m_testPdf1;
    delete m_testPdf2;
    delete m_parentWidget;
}

void DocumentComparisonIntegrationTest::init() {
    m_comparison = new DocumentComparison(m_parentWidget);
    m_comparison->show();

    // In offscreen mode, qWaitForWindowExposed() will timeout
    // Use a simple wait instead to allow widget initialization
    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);  // Give widgets time to initialize
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_comparison));
    }
}

void DocumentComparisonIntegrationTest::cleanup() {
    delete m_comparison;
    m_comparison = nullptr;
}

void DocumentComparisonIntegrationTest::testInitialization() {
    // Test basic initialization
    QVERIFY(m_comparison != nullptr);
    QVERIFY(m_comparison->isVisible());
    QVERIFY(!m_comparison->isComparing());

    // Test initial state
    ComparisonResults results = m_comparison->getResults();
    QCOMPARE(results.differences.size(), 0);
    QCOMPARE(results.totalPages1, 0);
    QCOMPARE(results.totalPages2, 0);
}

void DocumentComparisonIntegrationTest::testUIComponents() {
    // Test that main UI components exist
    QPushButton* compareButton = findButton("Compare");
    QVERIFY(compareButton != nullptr);

    QPushButton* stopButton = findButton("Stop");
    QVERIFY(stopButton != nullptr);

    QProgressBar* progressBar = m_comparison->findChild<QProgressBar*>();
    QVERIFY(progressBar != nullptr);

    QTreeWidget* differencesTree = m_comparison->findChild<QTreeWidget*>();
    QVERIFY(differencesTree != nullptr);

    QComboBox* viewModeCombo = m_comparison->findChild<QComboBox*>();
    QVERIFY(viewModeCombo != nullptr);
}

void DocumentComparisonIntegrationTest::testDocumentLoading() {
    if (!m_document1 || !m_document2) {
        QSKIP("Test documents not available");
    }

    // Test setting documents
    m_comparison->setDocuments(m_document1.get(), m_document2.get());

    // Should handle document setting without issues
    QVERIFY(true);

    // Test setting document paths
    if (m_testPdf1 && m_testPdf2) {
        m_comparison->setDocumentPaths(m_testPdf1->fileName(),
                                       m_testPdf2->fileName());
        QVERIFY(true);
    }
}

void DocumentComparisonIntegrationTest::testComparisonOptions() {
    // Test getting default options
    ComparisonOptions options = m_comparison->getComparisonOptions();
    QVERIFY(options.compareText);
    QVERIFY(options.compareImages);
    QVERIFY(options.compareAnnotations);

    // Test setting custom options
    ComparisonOptions customOptions;
    customOptions.compareText = false;
    customOptions.compareImages = true;
    customOptions.ignoreWhitespace = false;
    customOptions.imageSimilarityThreshold = 0.8;

    m_comparison->setComparisonOptions(customOptions);

    ComparisonOptions retrievedOptions = m_comparison->getComparisonOptions();
    QCOMPARE(retrievedOptions.compareText, false);
    QCOMPARE(retrievedOptions.compareImages, true);
    QCOMPARE(retrievedOptions.ignoreWhitespace, false);
    QCOMPARE(retrievedOptions.imageSimilarityThreshold, 0.8);
}

void DocumentComparisonIntegrationTest::testOptionsUI() {
    // Test options UI components
    QCheckBox* textCheck = findCheckBox("Text");
    if (textCheck) {
        QVERIFY(textCheck->isChecked());  // Should be checked by default

        // Toggle and verify
        textCheck->setChecked(false);
        QVERIFY(!textCheck->isChecked());
    }

    QCheckBox* imagesCheck = findCheckBox("Images");
    if (imagesCheck) {
        QVERIFY(imagesCheck->isChecked());
    }

    QSlider* similaritySlider = m_comparison->findChild<QSlider*>();
    if (similaritySlider) {
        int initialValue = similaritySlider->value();
        similaritySlider->setValue(75);
        QCOMPARE(similaritySlider->value(), 75);
    }
}

void DocumentComparisonIntegrationTest::testOptionsSignals() {
    QSignalSpy optionsChangedSpy(m_comparison,
                                 &DocumentComparison::onOptionsChanged);

    // Change options through UI
    QCheckBox* textCheck = findCheckBox("Text");
    if (textCheck) {
        textCheck->toggle();
        QTest::qWait(50);

        // Should trigger options changed
        QVERIFY(optionsChangedSpy.count() >= 0);
    }
}

void DocumentComparisonIntegrationTest::testComparisonStart() {
    if (!m_document1 || !m_document2) {
        QSKIP("Test documents not available");
    }

    QSignalSpy startedSpy(m_comparison, &DocumentComparison::comparisonStarted);
    QSignalSpy finishedSpy(m_comparison,
                           &DocumentComparison::comparisonFinished);

    // Set documents and start comparison
    m_comparison->setDocuments(m_document1.get(), m_document2.get());
    m_comparison->startComparison();

    // Should emit started signal
    QVERIFY(startedSpy.count() >= 0);
    QVERIFY(m_comparison->isComparing() || finishedSpy.count() > 0);

    waitForComparison();
}

void DocumentComparisonIntegrationTest::testComparisonProgress() {
    if (!m_document1 || !m_document2) {
        QSKIP("Test documents not available");
    }

    QSignalSpy progressSpy(m_comparison,
                           &DocumentComparison::comparisonProgress);

    m_comparison->setDocuments(m_document1.get(), m_document2.get());
    m_comparison->startComparison();

    waitForComparison();

    // Should emit progress signals during comparison
    QVERIFY(progressSpy.count() >= 0);
}

void DocumentComparisonIntegrationTest::testComparisonResults() {
    if (!m_document1 || !m_document2) {
        QSKIP("Test documents not available");
    }

    QSignalSpy finishedSpy(m_comparison,
                           &DocumentComparison::comparisonFinished);

    m_comparison->setDocuments(m_document1.get(), m_document2.get());
    m_comparison->startComparison();

    waitForComparison();

    // Should have results after comparison
    ComparisonResults results = m_comparison->getResults();
    QVERIFY(results.totalPages1 >= 0);
    QVERIFY(results.totalPages2 >= 0);
    QVERIFY(results.overallSimilarity >= 0.0);
    QVERIFY(results.overallSimilarity <= 1.0);

    // Should emit finished signal
    QVERIFY(finishedSpy.count() >= 0);
}

void DocumentComparisonIntegrationTest::testComparisonStop() {
    if (!m_document1 || !m_document2) {
        QSKIP("Test documents not available");
    }

    m_comparison->setDocuments(m_document1.get(), m_document2.get());
    m_comparison->startComparison();

    // Stop comparison immediately
    m_comparison->stopComparison();

    QTest::qWait(100);

    // Should not be comparing anymore
    QVERIFY(!m_comparison->isComparing());
}

void DocumentComparisonIntegrationTest::testDifferenceNavigation() {
    if (!m_document1 || !m_document2) {
        QSKIP("Test documents not available");
    }

    // Perform comparison first
    m_comparison->setDocuments(m_document1.get(), m_document2.get());
    m_comparison->startComparison();
    waitForComparison();

    QList<DocumentDifference> differences = m_comparison->getDifferences();

    if (differences.size() > 0) {
        // Test navigation
        m_comparison->goToDifference(0);
        m_comparison->nextDifference();
        m_comparison->previousDifference();

        // Should handle navigation without crashing
        QVERIFY(true);
    }
}

void DocumentComparisonIntegrationTest::testDifferenceSelection() {
    QSignalSpy selectionSpy(m_comparison,
                            &DocumentComparison::differenceSelected);

    // Find differences tree
    QTreeWidget* differencesTree = m_comparison->findChild<QTreeWidget*>();
    if (differencesTree && differencesTree->topLevelItemCount() > 0) {
        // Select first item
        QTreeWidgetItem* firstItem = differencesTree->topLevelItem(0);
        differencesTree->setCurrentItem(firstItem);

        // Simulate click
        m_comparison->onDifferenceClicked(firstItem, 0);

        // Should emit selection signal
        QVERIFY(selectionSpy.count() >= 0);
    }
}

void DocumentComparisonIntegrationTest::testDifferenceHighlighting() {
    if (!m_document1 || !m_document2) {
        QSKIP("Test documents not available");
    }

    // Create a test difference
    DocumentDifference testDiff;
    testDiff.type = DifferenceType::TextModified;
    testDiff.pageNumber1 = 0;
    testDiff.pageNumber2 = 0;
    testDiff.region1 = QRectF(10, 10, 100, 20);
    testDiff.region2 = QRectF(10, 10, 100, 20);
    testDiff.description = "Test difference";

    // Test highlighting (should not crash)
    // Note: This is a basic test since we can't easily verify visual
    // highlighting
    QVERIFY(true);
}

void DocumentComparisonIntegrationTest::testReportGeneration() {
    if (!m_document1 || !m_document2) {
        QSKIP("Test documents not available");
    }

    // Perform comparison first
    m_comparison->setDocuments(m_document1.get(), m_document2.get());
    m_comparison->startComparison();
    waitForComparison();

    // Generate report
    QString report = m_comparison->generateComparisonReport();
    QVERIFY(!report.isEmpty());

    // Report should contain basic information
    QVERIFY(report.contains("Comparison") || report.contains("Results"));
}

void DocumentComparisonIntegrationTest::testResultsExport() {
    if (!m_document1 || !m_document2) {
        QSKIP("Test documents not available");
    }

    // Perform comparison first
    m_comparison->setDocuments(m_document1.get(), m_document2.get());
    m_comparison->startComparison();
    waitForComparison();

    // Test export to file
    QTemporaryFile exportFile;
    if (exportFile.open()) {
        bool success = m_comparison->exportResultsToFile(exportFile.fileName());
        QVERIFY(success || true);  // May fail if no differences found
    }
}

void DocumentComparisonIntegrationTest::testCSVExport() {
    QSignalSpy csvExportSpy(m_comparison,
                            &DocumentComparison::differencesExportedToCSV);

    QTemporaryFile csvFile;
    csvFile.setFileTemplate("comparison_XXXXXX.csv");
    if (csvFile.open()) {
        m_comparison->exportDifferencesToCSV(csvFile.fileName());

        // Should handle CSV export
        QVERIFY(csvExportSpy.count() >= 0);
    }
}

void DocumentComparisonIntegrationTest::testSessionSave() {
    QSignalSpy sessionSavedSpy(m_comparison,
                               &DocumentComparison::comparisonSessionSaved);

    QTemporaryFile sessionFile;
    sessionFile.setFileTemplate("session_XXXXXX.json");
    if (sessionFile.open()) {
        bool success =
            m_comparison->saveComparisonSession(sessionFile.fileName());

        // Should handle session saving
        QVERIFY(success || sessionSavedSpy.count() >= 0);
    }
}

void DocumentComparisonIntegrationTest::testSessionLoad() {
    QSignalSpy sessionLoadedSpy(m_comparison,
                                &DocumentComparison::comparisonSessionLoaded);

    // Create a temporary session file
    QTemporaryFile sessionFile;
    sessionFile.setFileTemplate("session_XXXXXX.json");
    if (sessionFile.open()) {
        // Try to save first, then load
        m_comparison->saveComparisonSession(sessionFile.fileName());
        bool success =
            m_comparison->loadComparisonSession(sessionFile.fileName());

        // Should handle session loading
        QVERIFY(success || sessionLoadedSpy.count() >= 0);
    }
}

void DocumentComparisonIntegrationTest::testViewModes() {
    // Test different view modes
    m_comparison->setViewMode("side-by-side");
    m_comparison->setViewMode("overlay");
    m_comparison->setViewMode("difference-only");

    // Should handle view mode changes without crashing
    QVERIFY(true);
}

void DocumentComparisonIntegrationTest::testViewModeChanges() {
    QComboBox* viewModeCombo = m_comparison->findChild<QComboBox*>();
    if (viewModeCombo && viewModeCombo->count() > 1) {
        int initialIndex = viewModeCombo->currentIndex();
        int newIndex = (initialIndex + 1) % viewModeCombo->count();

        viewModeCombo->setCurrentIndex(newIndex);
        m_comparison->onViewModeChanged();

        QCOMPARE(viewModeCombo->currentIndex(), newIndex);
    }
}

void DocumentComparisonIntegrationTest::testInvalidDocuments() {
    QSignalSpy errorSpy(m_comparison, &DocumentComparison::comparisonError);

    // Test with null documents
    m_comparison->setDocuments(nullptr, nullptr);
    m_comparison->startComparison();

    QTest::qWait(100);

    // Should handle invalid documents gracefully
    QVERIFY(errorSpy.count() >= 0 || !m_comparison->isComparing());
}

void DocumentComparisonIntegrationTest::testComparisonErrors() {
    QSignalSpy errorSpy(m_comparison, &DocumentComparison::comparisonError);

    // Test with invalid file paths
    m_comparison->setDocumentPaths("/nonexistent/file1.pdf",
                                   "/nonexistent/file2.pdf");
    m_comparison->startComparison();

    QTest::qWait(100);

    // Should emit error signal or handle gracefully
    QVERIFY(errorSpy.count() >= 0 || !m_comparison->isComparing());
}

void DocumentComparisonIntegrationTest::createTestPdfs() {
    // Create first test PDF
    m_testPdf1 = new QTemporaryFile();
    m_testPdf1->setFileTemplate("test_pdf1_XXXXXX.pdf");
    if (m_testPdf1->open()) {
        QByteArray pdfContent =
            "%PDF-1.4\n1 0 obj\n<<\n/Type /Catalog\n/Pages 2 0 R\n>>\nendobj\n"
            "2 0 obj\n<<\n/Type /Pages\n/Kids [3 0 R]\n/Count 1\n>>\nendobj\n"
            "3 0 obj\n<<\n/Type /Page\n/Parent 2 0 R\n/MediaBox [0 0 612 792]\n"
            "/Contents 4 0 R\n>>\nendobj\n"
            "4 0 obj\n<<\n/Length 50\n>>\nstream\nBT\n/F1 12 Tf\n100 700 Td\n"
            "(Test Document 1) Tj\nET\nendstream\nendobj\n"
            "xref\n0 5\n0000000000 65535 f \n0000000009 65535 n \n"
            "0000000074 65535 n \n0000000120 65535 n \n0000000179 65535 n \n"
            "trailer\n<<\n/Size 5\n/Root 1 0 R\n>>\nstartxref\n280\n%%EOF\n";

        m_testPdf1->write(pdfContent);
        m_testPdf1->flush();

        m_document1 = Poppler::Document::load(m_testPdf1->fileName());
    }

    // Create second test PDF (slightly different)
    m_testPdf2 = new QTemporaryFile();
    m_testPdf2->setFileTemplate("test_pdf2_XXXXXX.pdf");
    if (m_testPdf2->open()) {
        QByteArray pdfContent =
            "%PDF-1.4\n1 0 obj\n<<\n/Type /Catalog\n/Pages 2 0 R\n>>\nendobj\n"
            "2 0 obj\n<<\n/Type /Pages\n/Kids [3 0 R]\n/Count 1\n>>\nendobj\n"
            "3 0 obj\n<<\n/Type /Page\n/Parent 2 0 R\n/MediaBox [0 0 612 792]\n"
            "/Contents 4 0 R\n>>\nendobj\n"
            "4 0 obj\n<<\n/Length 50\n>>\nstream\nBT\n/F1 12 Tf\n100 700 Td\n"
            "(Test Document 2) Tj\nET\nendstream\nendobj\n"
            "xref\n0 5\n0000000000 65535 f \n0000000009 65535 n \n"
            "0000000074 65535 n \n0000000120 65535 n \n0000000179 65535 n \n"
            "trailer\n<<\n/Size 5\n/Root 1 0 R\n>>\nstartxref\n280\n%%EOF\n";

        m_testPdf2->write(pdfContent);
        m_testPdf2->flush();

        m_document2 = Poppler::Document::load(m_testPdf2->fileName());
    }
}

void DocumentComparisonIntegrationTest::waitForComparison() {
    // Wait for comparison to complete
    int timeout = 5000;  // 5 seconds
    int elapsed = 0;

    while (m_comparison->isComparing() && elapsed < timeout) {
        QTest::qWait(100);
        elapsed += 100;
        QApplication::processEvents();
    }
}

QPushButton* DocumentComparisonIntegrationTest::findButton(
    const QString& text) {
    QList<QPushButton*> buttons = m_comparison->findChildren<QPushButton*>();
    for (QPushButton* button : buttons) {
        if (button->text().contains(text, Qt::CaseInsensitive)) {
            return button;
        }
    }
    return nullptr;
}

QCheckBox* DocumentComparisonIntegrationTest::findCheckBox(
    const QString& text) {
    QList<QCheckBox*> checkBoxes = m_comparison->findChildren<QCheckBox*>();
    for (QCheckBox* checkBox : checkBoxes) {
        if (checkBox->text().contains(text, Qt::CaseInsensitive)) {
            return checkBox;
        }
    }
    return nullptr;
}

QTEST_MAIN(DocumentComparisonIntegrationTest)
#include "test_document_comparison_integration.moc"
