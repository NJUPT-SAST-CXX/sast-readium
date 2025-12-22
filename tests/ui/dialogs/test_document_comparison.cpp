#include <poppler/qt6/poppler-qt6.h>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalSpy>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTextEdit>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include "../../../app/ui/dialogs/DocumentComparison.h"

class DocumentComparisonTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Construction and initialization tests
    void testConstruction();
    void testDestruction();
    void testUIComponents();
    void testInitialState();

    // Document loading tests
    void testSetDocuments();
    void testSetDocumentsNull();
    void testSetDocumentPaths();

    // Comparison options tests
    void testDefaultComparisonOptions();
    void testSetComparisonOptions();
    void testGetComparisonOptions();
    void testOptionsUISync();

    // Comparison state tests
    void testIsComparingInitialState();
    void testStartComparisonWithoutDocuments();
    void testStopComparison();

    // Navigation tests
    void testGoToDifference();
    void testNextDifference();
    void testPreviousDifference();

    // View mode tests
    void testViewModeComboBox();
    void testSetViewMode();
    void testViewModeChanged();

    // Results access tests
    void testGetResults();
    void testGetDifferences();

    // Report generation tests
    void testGenerateComparisonReport();
    void testExportResultsToFile();
    void testExportDifferencesToCSV();

    // Session management tests
    void testSaveComparisonSession();
    void testLoadComparisonSession();

    // Signal tests
    void testComparisonStartedSignal();
    void testComparisonFinishedSignal();
    void testComparisonProgressSignal();
    void testComparisonErrorSignal();
    void testDifferenceSelectedSignal();

    // UI interaction tests
    void testOptionsButtonToggle();
    void testDifferenceDetailsVisibility();
    void testDifferenceTreeClicking();

    // Static utility tests
    void testGetDifferenceTypeName();

    // Data structure tests
    void testDocumentDifferenceDefaults();
    void testComparisonOptionsDefaults();
    void testComparisonResultsDefaults();

private:
    DocumentComparison* m_widget;
    QWidget* m_parentWidget;
    QTemporaryFile* m_testPdfFile1;
    QTemporaryFile* m_testPdfFile2;
    std::unique_ptr<Poppler::Document> m_testDocument1;
    std::unique_ptr<Poppler::Document> m_testDocument2;
    QTemporaryDir* m_tempDir;

    void createTestPdfs();
    void waitForUI();
};

void DocumentComparisonTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(1024, 768);
    m_parentWidget->show();

    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());

    createTestPdfs();
}

void DocumentComparisonTest::cleanupTestCase() {
    m_testDocument1.reset();
    m_testDocument2.reset();
    delete m_testPdfFile1;
    delete m_testPdfFile2;
    delete m_tempDir;
    delete m_parentWidget;
}

void DocumentComparisonTest::init() {
    m_widget = new DocumentComparison(m_parentWidget);
    m_widget->show();

    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_widget));
    }
}

void DocumentComparisonTest::cleanup() {
    if (m_widget->isComparing()) {
        m_widget->stopComparison();
        QTest::qWait(100);
    }
    delete m_widget;
    m_widget = nullptr;
}

void DocumentComparisonTest::createTestPdfs() {
    // Create first test PDF
    m_testPdfFile1 = new QTemporaryFile();
    m_testPdfFile1->setFileTemplate(m_tempDir->path() +
                                    "/comparison_test1_XXXXXX.pdf");
    if (m_testPdfFile1->open()) {
        QByteArray pdfContent =
            "%PDF-1.4\n"
            "1 0 obj\n<<\n/Type /Catalog\n/Pages 2 0 R\n>>\nendobj\n"
            "2 0 obj\n<<\n/Type /Pages\n/Kids [3 0 R]\n/Count 1\n>>\nendobj\n"
            "3 0 obj\n<<\n/Type /Page\n/Parent 2 0 R\n/MediaBox [0 0 612 792]\n"
            "/Contents 4 0 R\n>>\nendobj\n"
            "4 0 obj\n<<\n/Length 50\n>>\nstream\nBT\n/F1 12 Tf\n100 700 Td\n"
            "(Test Document 1) Tj\nET\nendstream\nendobj\n"
            "xref\n0 5\n0000000000 65535 f \n0000000009 65535 n \n"
            "0000000074 65535 n \n0000000120 65535 n \n0000000179 65535 n \n"
            "trailer\n<<\n/Size 5\n/Root 1 0 R\n>>\nstartxref\n280\n%%EOF\n";

        m_testPdfFile1->write(pdfContent);
        m_testPdfFile1->flush();
        m_testDocument1 = Poppler::Document::load(m_testPdfFile1->fileName());
    }

    // Create second test PDF (slightly different)
    m_testPdfFile2 = new QTemporaryFile();
    m_testPdfFile2->setFileTemplate(m_tempDir->path() +
                                    "/comparison_test2_XXXXXX.pdf");
    if (m_testPdfFile2->open()) {
        QByteArray pdfContent =
            "%PDF-1.4\n"
            "1 0 obj\n<<\n/Type /Catalog\n/Pages 2 0 R\n>>\nendobj\n"
            "2 0 obj\n<<\n/Type /Pages\n/Kids [3 0 R]\n/Count 1\n>>\nendobj\n"
            "3 0 obj\n<<\n/Type /Page\n/Parent 2 0 R\n/MediaBox [0 0 612 792]\n"
            "/Contents 4 0 R\n>>\nendobj\n"
            "4 0 obj\n<<\n/Length 50\n>>\nstream\nBT\n/F1 12 Tf\n100 700 Td\n"
            "(Test Document 2) Tj\nET\nendstream\nendobj\n"
            "xref\n0 5\n0000000000 65535 f \n0000000009 65535 n \n"
            "0000000074 65535 n \n0000000120 65535 n \n0000000179 65535 n \n"
            "trailer\n<<\n/Size 5\n/Root 1 0 R\n>>\nstartxref\n280\n%%EOF\n";

        m_testPdfFile2->write(pdfContent);
        m_testPdfFile2->flush();
        m_testDocument2 = Poppler::Document::load(m_testPdfFile2->fileName());
    }
}

void DocumentComparisonTest::waitForUI() {
    QTest::qWait(50);
    QApplication::processEvents();
}

// ============================================================================
// Construction and initialization tests
// ============================================================================

void DocumentComparisonTest::testConstruction() {
    QVERIFY(m_widget != nullptr);
    QVERIFY(m_widget->isVisible());
}

void DocumentComparisonTest::testDestruction() {
    auto* widget = new DocumentComparison(m_parentWidget);
    delete widget;
    QVERIFY(true);
}

void DocumentComparisonTest::testUIComponents() {
    // Test toolbar buttons exist
    QPushButton* compareButton = m_widget->findChild<QPushButton*>();
    QVERIFY(compareButton != nullptr);

    // Test view mode combo box
    QComboBox* viewModeCombo = m_widget->findChild<QComboBox*>();
    QVERIFY(viewModeCombo != nullptr);

    // Test progress bar
    QProgressBar* progressBar = m_widget->findChild<QProgressBar*>();
    QVERIFY(progressBar != nullptr);

    // Test tree widget for differences
    QTreeWidget* differencesTree = m_widget->findChild<QTreeWidget*>();
    QVERIFY(differencesTree != nullptr);

    // Test text edit for details
    QTextEdit* detailsEdit = m_widget->findChild<QTextEdit*>();
    QVERIFY(detailsEdit != nullptr);

    // Test splitters
    QList<QSplitter*> splitters = m_widget->findChildren<QSplitter*>();
    QVERIFY(splitters.size() >= 2);

    // Test scroll areas for document views
    QList<QScrollArea*> scrollAreas = m_widget->findChildren<QScrollArea*>();
    QVERIFY(scrollAreas.size() >= 2);
}

void DocumentComparisonTest::testInitialState() {
    QVERIFY(!m_widget->isComparing());

    ComparisonResults results = m_widget->getResults();
    QCOMPARE(results.totalPages1, 0);
    QCOMPARE(results.totalPages2, 0);
    QCOMPARE(results.pagesCompared, 0);
    QVERIFY(results.differences.isEmpty());

    // Progress bar should be hidden initially
    QProgressBar* progressBar = m_widget->findChild<QProgressBar*>();
    QVERIFY(progressBar != nullptr);
    QVERIFY(!progressBar->isVisible());
}

// ============================================================================
// Document loading tests
// ============================================================================

void DocumentComparisonTest::testSetDocuments() {
    if (!m_testDocument1 || !m_testDocument2) {
        QSKIP("Test documents not available");
    }

    m_widget->setDocuments(m_testDocument1.get(), m_testDocument2.get());
    waitForUI();

    // Compare button should be enabled
    QList<QPushButton*> buttons = m_widget->findChildren<QPushButton*>();
    bool foundEnabledCompareButton = false;
    for (QPushButton* btn : buttons) {
        if (btn->text().contains("Compare", Qt::CaseInsensitive) &&
            btn->isEnabled()) {
            foundEnabledCompareButton = true;
            break;
        }
    }
    QVERIFY(foundEnabledCompareButton);
}

void DocumentComparisonTest::testSetDocumentsNull() {
    m_widget->setDocuments(nullptr, nullptr);
    waitForUI();

    // Compare button should be disabled
    QList<QPushButton*> buttons = m_widget->findChildren<QPushButton*>();
    for (QPushButton* btn : buttons) {
        if (btn->text().contains("Compare", Qt::CaseInsensitive)) {
            QVERIFY(!btn->isEnabled());
            break;
        }
    }
}

void DocumentComparisonTest::testSetDocumentPaths() {
    QString path1 = "/path/to/document1.pdf";
    QString path2 = "/path/to/document2.pdf";

    m_widget->setDocumentPaths(path1, path2);

    // Paths should be stored (verified through report generation)
    QString report = m_widget->generateComparisonReport();
    QVERIFY(!report.isEmpty());
}

// ============================================================================
// Comparison options tests
// ============================================================================

void DocumentComparisonTest::testDefaultComparisonOptions() {
    ComparisonOptions options = m_widget->getComparisonOptions();

    QVERIFY(options.compareText);
    QVERIFY(options.compareImages);
    QVERIFY(options.compareAnnotations);
    QVERIFY(options.ignoreWhitespace);
    QVERIFY(!options.ignoreCaseChanges);
}

void DocumentComparisonTest::testSetComparisonOptions() {
    ComparisonOptions options;
    options.compareText = false;
    options.compareImages = false;
    options.compareLayout = true;
    options.compareAnnotations = false;
    options.ignoreWhitespace = false;
    options.ignoreCaseChanges = true;
    options.textSimilarityThreshold = 0.80;
    options.imageSimilarityThreshold = 0.85;
    options.maxDifferencesPerPage = 100;

    m_widget->setComparisonOptions(options);
    waitForUI();

    ComparisonOptions retrieved = m_widget->getComparisonOptions();
    QCOMPARE(retrieved.compareText, false);
    QCOMPARE(retrieved.compareImages, false);
    QCOMPARE(retrieved.compareLayout, true);
    QCOMPARE(retrieved.compareAnnotations, false);
    QCOMPARE(retrieved.ignoreWhitespace, false);
    QCOMPARE(retrieved.ignoreCaseChanges, true);
    QCOMPARE(retrieved.maxDifferencesPerPage, 100);
}

void DocumentComparisonTest::testGetComparisonOptions() {
    ComparisonOptions options = m_widget->getComparisonOptions();

    QVERIFY(options.textSimilarityThreshold >= 0.0);
    QVERIFY(options.textSimilarityThreshold <= 1.0);
    QVERIFY(options.imageSimilarityThreshold >= 0.0);
    QVERIFY(options.imageSimilarityThreshold <= 1.0);
    QVERIFY(options.maxDifferencesPerPage > 0);
}

void DocumentComparisonTest::testOptionsUISync() {
    QList<QCheckBox*> checkboxes = m_widget->findChildren<QCheckBox*>();
    QVERIFY(checkboxes.size() > 0);

    for (QCheckBox* cb : checkboxes) {
        if (cb->text().contains("Text", Qt::CaseInsensitive)) {
            bool originalState = cb->isChecked();
            cb->setChecked(!originalState);
            waitForUI();

            ComparisonOptions options = m_widget->getComparisonOptions();
            QCOMPARE(options.compareText, !originalState);

            cb->setChecked(originalState);
            break;
        }
    }
}

// ============================================================================
// Comparison state tests
// ============================================================================

void DocumentComparisonTest::testIsComparingInitialState() {
    QVERIFY(!m_widget->isComparing());
}

void DocumentComparisonTest::testStartComparisonWithoutDocuments() {
    QSignalSpy errorSpy(m_widget, &DocumentComparison::comparisonError);

    m_widget->startComparison();
    waitForUI();

    QCOMPARE(errorSpy.count(), 1);
    QVERIFY(!m_widget->isComparing());
}

void DocumentComparisonTest::testStopComparison() {
    m_widget->stopComparison();
    waitForUI();

    QVERIFY(!m_widget->isComparing());
}

// ============================================================================
// Navigation tests
// ============================================================================

void DocumentComparisonTest::testGoToDifference() {
    m_widget->goToDifference(-1);
    m_widget->goToDifference(0);
    m_widget->goToDifference(100);

    QVERIFY(true);
}

void DocumentComparisonTest::testNextDifference() {
    m_widget->nextDifference();
    QVERIFY(true);
}

void DocumentComparisonTest::testPreviousDifference() {
    m_widget->previousDifference();
    QVERIFY(true);
}

// ============================================================================
// View mode tests
// ============================================================================

void DocumentComparisonTest::testViewModeComboBox() {
    QComboBox* viewModeCombo = m_widget->findChild<QComboBox*>();
    QVERIFY(viewModeCombo != nullptr);

    QVERIFY(viewModeCombo->count() >= 2);

    for (int i = 0; i < viewModeCombo->count(); ++i) {
        viewModeCombo->setCurrentIndex(i);
        waitForUI();
        QCOMPARE(viewModeCombo->currentIndex(), i);
    }
}

void DocumentComparisonTest::testSetViewMode() {
    m_widget->setViewMode("Side by Side");
    waitForUI();

    QComboBox* viewModeCombo = m_widget->findChild<QComboBox*>();
    QVERIFY(viewModeCombo != nullptr);
    QVERIFY(viewModeCombo->currentText().contains("Side", Qt::CaseInsensitive));
}

void DocumentComparisonTest::testViewModeChanged() {
    QComboBox* viewModeCombo = m_widget->findChild<QComboBox*>();
    QVERIFY(viewModeCombo != nullptr);

    int initialIndex = viewModeCombo->currentIndex();
    int newIndex = (initialIndex + 1) % viewModeCombo->count();

    viewModeCombo->setCurrentIndex(newIndex);
    waitForUI();

    QCOMPARE(viewModeCombo->currentIndex(), newIndex);
}

// ============================================================================
// Results access tests
// ============================================================================

void DocumentComparisonTest::testGetResults() {
    ComparisonResults results = m_widget->getResults();

    QCOMPARE(results.totalPages1, 0);
    QCOMPARE(results.totalPages2, 0);
    QCOMPARE(results.pagesCompared, 0);
    QCOMPARE(results.comparisonTime, static_cast<qint64>(0));
    QVERIFY(results.differences.isEmpty());
}

void DocumentComparisonTest::testGetDifferences() {
    QList<DocumentDifference> differences = m_widget->getDifferences();
    QVERIFY(differences.isEmpty());
}

// ============================================================================
// Report generation tests
// ============================================================================

void DocumentComparisonTest::testGenerateComparisonReport() {
    QString report = m_widget->generateComparisonReport();

    QVERIFY(report.contains("Document Comparison Report"));
    QVERIFY(report.contains("Documents:"));
    QVERIFY(report.contains("Comparison Summary:"));
}

void DocumentComparisonTest::testExportResultsToFile() {
    QString filePath = m_tempDir->path() + "/test_export.json";

    bool success = m_widget->exportResultsToFile(filePath);
    QVERIFY(success);

    QFile file(filePath);
    QVERIFY(file.exists());

    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QVERIFY(!doc.isNull());
    QVERIFY(doc.isObject());

    QJsonObject obj = doc.object();
    QVERIFY(obj.contains("totalPages1"));
    QVERIFY(obj.contains("totalPages2"));
    QVERIFY(obj.contains("differences"));
}

void DocumentComparisonTest::testExportDifferencesToCSV() {
    QString filePath = m_tempDir->path() + "/test_export.csv";

    QSignalSpy exportSpy(m_widget,
                         &DocumentComparison::differencesExportedToCSV);

    m_widget->exportDifferencesToCSV(filePath);
    waitForUI();

    QFile file(filePath);
    QVERIFY(file.exists());

    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString header = file.readLine();
    QVERIFY(header.contains("Type"));
    QVERIFY(header.contains("Page1"));
    QVERIFY(header.contains("Description"));

    QCOMPARE(exportSpy.count(), 1);
}

// ============================================================================
// Session management tests
// ============================================================================

void DocumentComparisonTest::testSaveComparisonSession() {
    QString filePath = m_tempDir->path() + "/test_session.json";

    QSignalSpy saveSpy(m_widget, &DocumentComparison::comparisonSessionSaved);

    bool success = m_widget->saveComparisonSession(filePath);
    QVERIFY(success);

    QFile file(filePath);
    QVERIFY(file.exists());

    QCOMPARE(saveSpy.count(), 1);
}

void DocumentComparisonTest::testLoadComparisonSession() {
    QString filePath = m_tempDir->path() + "/test_load_session.json";

    ComparisonOptions options;
    options.compareText = false;
    options.compareImages = true;
    options.maxDifferencesPerPage = 75;
    m_widget->setComparisonOptions(options);

    m_widget->saveComparisonSession(filePath);

    ComparisonOptions defaultOptions;
    m_widget->setComparisonOptions(defaultOptions);

    QSignalSpy loadSpy(m_widget, &DocumentComparison::comparisonSessionLoaded);

    bool success = m_widget->loadComparisonSession(filePath);
    QVERIFY(success);

    QCOMPARE(loadSpy.count(), 1);

    ComparisonOptions loaded = m_widget->getComparisonOptions();
    QCOMPARE(loaded.compareText, false);
    QCOMPARE(loaded.compareImages, true);
    QCOMPARE(loaded.maxDifferencesPerPage, 75);
}

// ============================================================================
// Signal tests
// ============================================================================

void DocumentComparisonTest::testComparisonStartedSignal() {
    QSignalSpy spy(m_widget, &DocumentComparison::comparisonStarted);
    QVERIFY(spy.isValid());
}

void DocumentComparisonTest::testComparisonFinishedSignal() {
    QSignalSpy spy(m_widget, &DocumentComparison::comparisonFinished);
    QVERIFY(spy.isValid());
}

void DocumentComparisonTest::testComparisonProgressSignal() {
    QSignalSpy spy(m_widget, &DocumentComparison::comparisonProgress);
    QVERIFY(spy.isValid());
}

void DocumentComparisonTest::testComparisonErrorSignal() {
    QSignalSpy spy(m_widget, &DocumentComparison::comparisonError);
    QVERIFY(spy.isValid());

    m_widget->startComparison();
    waitForUI();

    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(!arguments.at(0).toString().isEmpty());
}

void DocumentComparisonTest::testDifferenceSelectedSignal() {
    QSignalSpy spy(m_widget, &DocumentComparison::differenceSelected);
    QVERIFY(spy.isValid());
}

// ============================================================================
// UI interaction tests
// ============================================================================

void DocumentComparisonTest::testOptionsButtonToggle() {
    QList<QPushButton*> buttons = m_widget->findChildren<QPushButton*>();
    QPushButton* optionsButton = nullptr;

    for (QPushButton* btn : buttons) {
        if (btn->text().contains("Options", Qt::CaseInsensitive)) {
            optionsButton = btn;
            break;
        }
    }

    if (optionsButton) {
        QTest::mouseClick(optionsButton, Qt::LeftButton);
        waitForUI();

        QTest::mouseClick(optionsButton, Qt::LeftButton);
        waitForUI();

        QVERIFY(true);
    }
}

void DocumentComparisonTest::testDifferenceDetailsVisibility() {
    m_widget->showDifferenceDetails(true);
    waitForUI();

    QTextEdit* detailsEdit = m_widget->findChild<QTextEdit*>();
    QVERIFY(detailsEdit != nullptr);
    QVERIFY(detailsEdit->isVisible());

    m_widget->showDifferenceDetails(false);
    waitForUI();

    QVERIFY(!detailsEdit->isVisible());
}

void DocumentComparisonTest::testDifferenceTreeClicking() {
    QTreeWidget* tree = m_widget->findChild<QTreeWidget*>();
    QVERIFY(tree != nullptr);

    QCOMPARE(tree->topLevelItemCount(), 0);

    QTest::mouseClick(tree->viewport(), Qt::LeftButton, Qt::NoModifier,
                      QPoint(50, 50));
    waitForUI();

    QVERIFY(true);
}

// ============================================================================
// Static utility tests
// ============================================================================

void DocumentComparisonTest::testGetDifferenceTypeName() {
    QCOMPARE(
        DocumentComparison::getDifferenceTypeName(DifferenceType::TextAdded),
        QString("Text Added"));
    QCOMPARE(
        DocumentComparison::getDifferenceTypeName(DifferenceType::TextRemoved),
        QString("Text Removed"));
    QCOMPARE(
        DocumentComparison::getDifferenceTypeName(DifferenceType::TextModified),
        QString("Text Modified"));
    QCOMPARE(
        DocumentComparison::getDifferenceTypeName(DifferenceType::ImageAdded),
        QString("Image Added"));
    QCOMPARE(
        DocumentComparison::getDifferenceTypeName(DifferenceType::ImageRemoved),
        QString("Image Removed"));
    QCOMPARE(DocumentComparison::getDifferenceTypeName(
                 DifferenceType::ImageModified),
             QString("Image Modified"));
    QCOMPARE(DocumentComparison::getDifferenceTypeName(
                 DifferenceType::LayoutChanged),
             QString("Layout Changed"));
    QCOMPARE(DocumentComparison::getDifferenceTypeName(
                 DifferenceType::AnnotationAdded),
             QString("Annotation Added"));
    QCOMPARE(DocumentComparison::getDifferenceTypeName(
                 DifferenceType::AnnotationRemoved),
             QString("Annotation Removed"));
    QCOMPARE(DocumentComparison::getDifferenceTypeName(
                 DifferenceType::AnnotationModified),
             QString("Annotation Modified"));
}

// ============================================================================
// Data structure tests
// ============================================================================

void DocumentComparisonTest::testDocumentDifferenceDefaults() {
    DocumentDifference diff;

    QCOMPARE(diff.type, DifferenceType::TextModified);
    QCOMPARE(diff.pageNumber1, -1);
    QCOMPARE(diff.pageNumber2, -1);
    QCOMPARE(diff.confidence, 1.0);
    QVERIFY(diff.oldText.isEmpty());
    QVERIFY(diff.newText.isEmpty());
    QVERIFY(diff.description.isEmpty());
    QVERIFY(diff.timestamp.isValid());
}

void DocumentComparisonTest::testComparisonOptionsDefaults() {
    ComparisonOptions options;

    QCOMPARE(options.compareText, true);
    QCOMPARE(options.compareImages, true);
    QCOMPARE(options.compareLayout, false);
    QCOMPARE(options.compareAnnotations, true);
    QCOMPARE(options.ignoreWhitespace, true);
    QCOMPARE(options.ignoreCaseChanges, false);
    QCOMPARE(options.ignoreFormatting, true);
    QCOMPARE(options.imageSimilarityThreshold, 0.95);
    QCOMPARE(options.textSimilarityThreshold, 0.90);
    QCOMPARE(options.maxDifferencesPerPage, 50);
    QCOMPARE(options.enableProgressReporting, true);
}

void DocumentComparisonTest::testComparisonResultsDefaults() {
    ComparisonResults results;

    QCOMPARE(results.totalPages1, 0);
    QCOMPARE(results.totalPages2, 0);
    QCOMPARE(results.pagesCompared, 0);
    QCOMPARE(results.comparisonTime, static_cast<qint64>(0));
    QCOMPARE(results.overallSimilarity, 0.0);
    QVERIFY(results.differences.isEmpty());
    QVERIFY(results.differenceCountByType.isEmpty());
    QVERIFY(results.summary.isEmpty());
}

QTEST_MAIN(DocumentComparisonTest)
#include "test_document_comparison.moc"
