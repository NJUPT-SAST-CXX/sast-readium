#pragma once

#include <poppler/qt6/poppler-qt6.h>

#include <QFuture>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QLabel>

#include <QScrollArea>

#include <QSplitter>
#include <QTextEdit>
#include <QThread>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <atomic>

class StyleManager;

/**
 * Types of document differences
 */
enum class DifferenceType {
    TextAdded,          // Text was added
    TextRemoved,        // Text was removed
    TextModified,       // Text was changed
    ImageAdded,         // Image was added
    ImageRemoved,       // Image was removed
    ImageModified,      // Image was changed
    LayoutChanged,      // Page layout changed
    AnnotationAdded,    // Annotation was added
    AnnotationRemoved,  // Annotation was removed
    AnnotationModified  // Annotation was changed
};

/**
 * Represents a single difference between documents
 */
struct DocumentDifference {
    DifferenceType type;
    int pageNumber1;      // Page in first document
    int pageNumber2;      // Page in second document
    QRectF region1;       // Region in first document
    QRectF region2;       // Region in second document
    QString oldText;      // Original text (for text changes)
    QString newText;      // New text (for text changes)
    QString description;  // Human-readable description
    double confidence;    // Confidence level (0.0-1.0)
    QDateTime timestamp;  // When difference was detected

    DocumentDifference()
        : type(DifferenceType::TextModified),
          pageNumber1(-1),
          pageNumber2(-1),
          confidence(1.0),
          timestamp(QDateTime::currentDateTime()) {}
};

/**
 * Comparison options and settings
 */
struct ComparisonOptions {
    bool compareText = true;
    bool compareImages = true;
    bool compareLayout = false;
    bool compareAnnotations = true;
    bool ignoreWhitespace = true;
    bool ignoreCaseChanges = false;
    bool ignoreFormatting = true;
    double imageSimilarityThreshold = 0.95;
    double textSimilarityThreshold = 0.90;
    int maxDifferencesPerPage = 50;
    bool enableProgressReporting = true;

    ComparisonOptions() = default;
};

/**
 * Document comparison results
 */
struct ComparisonResults {
    QList<DocumentDifference> differences;
    int totalPages1;
    int totalPages2;
    int pagesCompared;
    qint64 comparisonTime;
    double overallSimilarity;
    QMap<DifferenceType, int> differenceCountByType;
    QString summary;

    ComparisonResults()
        : totalPages1(0),
          totalPages2(0),
          pagesCompared(0),
          comparisonTime(0),
          overallSimilarity(0.0) {}
};

// Forward declaration
class DocumentComparisonWorker;

class ElaPushButton;
class ElaComboBox;
class ElaText;
class ElaProgressBar;
class ElaCheckBox;
class ElaSlider;
class ElaSpinBox;
class ElaScrollPageArea;
/**
 * Widget for comparing two PDF documents
 */
class DocumentComparison : public QWidget {
    Q_OBJECT

public:
    explicit DocumentComparison(QWidget* parent = nullptr);
    ~DocumentComparison() = default;

    // Document loading
    void setDocuments(Poppler::Document* doc1, Poppler::Document* doc2);
    void setDocumentPaths(const QString& path1, const QString& path2);

    // Comparison operations
    void startComparison();
    void stopComparison();
    bool isComparing() const { return m_isComparing; }

    // Comparison options
    ComparisonOptions getComparisonOptions() const;
    void setComparisonOptions(const ComparisonOptions& options);

    // Results access
    ComparisonResults getResults() const { return m_results; }
    QList<DocumentDifference> getDifferences() const {
        return m_results.differences;
    }

    // Navigation
    void goToDifference(int index);
    void nextDifference();
    void previousDifference();

    // Export and reporting
    QString generateComparisonReport() const;
    bool exportResultsToFile(const QString& filePath) const;

    // Additional comparison functions
    bool compareDocumentMetadata(Poppler::Document* doc1,
                                 Poppler::Document* doc2);
    QList<DocumentDifference> comparePageLayouts(int page1, int page2);
    void generateDetailedReport();
    static QString getDifferenceTypeName(DifferenceType type);
    void exportDifferencesToCSV(const QString& filePath);
    void createVisualDifferenceMap();

    // Session management
    bool saveComparisonSession(const QString& filePath);
    bool loadComparisonSession(const QString& filePath);

    // UI state
    void showDifferenceDetails(bool show);
    void setViewMode(
        const QString& mode);  // "side-by-side", "overlay", "difference-only"

signals:
    void comparisonStarted();
    void comparisonFinished(const ComparisonResults& results);
    void comparisonProgress(int percentage, const QString& status);
    void comparisonError(const QString& error);
    void differenceSelected(const DocumentDifference& difference);
    void detailedReportGenerated(const QString& filePath);
    void differencesExportedToCSV(const QString& filePath);
    void visualDifferenceMapCreated(const QString& filePath);
    void comparisonSessionSaved(const QString& filePath);
    void comparisonSessionLoaded(const QString& filePath);

public slots:
    void onDifferenceClicked(QTreeWidgetItem* item, int column);
    void onViewModeChanged();
    void onOptionsChanged();

private slots:
    void onComparisonFinished();
    void updateProgress();

private:
    void setupUI();
    void setupConnections();
    void updateDifferencesList();
    void updateComparisonView();
    void highlightDifference(const DocumentDifference& diff);
    void clearHighlights();
    void initializeMainLayout(StyleManager& styleManager);
    void initializeToolbar(StyleManager& styleManager);
    void initializeOptionsPanel(StyleManager& styleManager);
    void initializeContentArea(StyleManager& styleManager);
    void initializeProgressComponents();

    // Comparison algorithms
    QFuture<ComparisonResults> performComparison();
    ComparisonResults compareDocuments();
    QList<DocumentDifference> comparePages(int page1, int page2);
    QList<DocumentDifference> compareText(const QString& text1,
                                          const QString& text2, int page1,
                                          int page2) const;
    QList<DocumentDifference> compareImages(const QPixmap& image1,
                                            const QPixmap& image2, int page1,
                                            int page2) const;
    static double calculateTextSimilarity(const QString& text1,
                                          const QString& text2);
    static double calculateImageSimilarity(const QPixmap& image1,
                                           const QPixmap& image2);

    // UI Components
    QVBoxLayout* m_mainLayout = nullptr;
    QHBoxLayout* m_toolbarLayout = nullptr;
    QHBoxLayout* m_contentLayout = nullptr;

    // Toolbar
    ElaPushButton* m_compareButton = nullptr;
    ElaPushButton* m_stopButton = nullptr;
    ElaPushButton* m_optionsButton = nullptr;
    ElaPushButton* m_exportButton = nullptr;
    ElaComboBox* m_viewModeCombo = nullptr;
    ElaText* m_statusLabel = nullptr;
    ElaProgressBar* m_progressBar = nullptr;

    // Options panel
    ElaScrollPageArea* m_optionsGroup = nullptr;
    ElaCheckBox* m_compareTextCheck = nullptr;
    ElaCheckBox* m_compareImagesCheck = nullptr;
    ElaCheckBox* m_compareLayoutCheck = nullptr;
    ElaCheckBox* m_compareAnnotationsCheck = nullptr;
    ElaCheckBox* m_ignoreWhitespaceCheck = nullptr;
    ElaCheckBox* m_ignoreCaseCheck = nullptr;
    ElaSlider* m_similaritySlider = nullptr;
    ElaSpinBox* m_maxDifferencesSpinBox = nullptr;

    // Results panel
    QSplitter* m_resultsSplitter = nullptr;
    QTreeWidget* m_differencesTree = nullptr;
    QTextEdit* m_differenceDetails = nullptr;

    // Comparison view
    QSplitter* m_viewSplitter = nullptr;
    QScrollArea* m_leftView = nullptr;
    QScrollArea* m_rightView = nullptr;
    QLabel* m_leftImageLabel = nullptr;
    QLabel* m_rightImageLabel = nullptr;

    // Data
    Poppler::Document* m_document1;
    Poppler::Document* m_document2;
    QString m_documentPath1;
    QString m_documentPath2;
    ComparisonOptions m_options;
    ComparisonResults m_results;
    int m_currentDifferenceIndex;

    // Comparison state
    bool m_isComparing;
    QFuture<ComparisonResults> m_comparisonFuture;
    QFutureWatcher<ComparisonResults>* m_comparisonWatcher = nullptr;
    QTimer* m_progressTimer = nullptr;

    // Async comparison using QThread
    QThread* m_comparisonThread = nullptr;
    DocumentComparisonWorker* m_comparisonWorker = nullptr;
    std::atomic<bool> m_cancelRequested{false};
};

/**
 * Worker class for running document comparison in background thread
 */
class DocumentComparisonWorker : public QObject {
    Q_OBJECT

public:
    explicit DocumentComparisonWorker(Poppler::Document* doc1,
                                      Poppler::Document* doc2,
                                      const ComparisonOptions& options,
                                      std::atomic<bool>* cancelFlag,
                                      QObject* parent = nullptr);

signals:
    void comparisonComplete(const ComparisonResults& results);
    void progressUpdate(int percentage, const QString& status);
    void errorOccurred(const QString& error);

public slots:
    void doComparison();

private:
    Poppler::Document* m_document1;
    Poppler::Document* m_document2;
    ComparisonOptions m_options;
    std::atomic<bool>* m_cancelFlag;

    ComparisonResults compareDocuments();
    QList<DocumentDifference> comparePages(int page1, int page2);
    QList<DocumentDifference> compareText(const QString& text1,
                                          const QString& text2, int page1,
                                          int page2) const;
    QList<DocumentDifference> compareImages(const QPixmap& image1,
                                            const QPixmap& image2, int page1,
                                            int page2) const;
    static double calculateTextSimilarity(const QString& text1,
                                          const QString& text2);
    static double calculateImageSimilarity(const QPixmap& image1,
                                           const QPixmap& image2);
};
