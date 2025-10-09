#pragma once

#include <poppler-qt6.h>
#include <QElapsedTimer>
#include <QFuture>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

/**
 * Advanced document analyzer with batch processing capabilities
 */
class DocumentAnalyzer : public QObject {
    Q_OBJECT

public:
    enum AnalysisType {
        BasicAnalysis = 0x01,
        TextAnalysis = 0x02,
        ImageAnalysis = 0x04,
        StructureAnalysis = 0x08,
        SecurityAnalysis = 0x10,
        QualityAnalysis = 0x20,
        AccessibilityAnalysis = 0x40,
        FullAnalysis = 0xFF
    };
    Q_DECLARE_FLAGS(AnalysisTypes, AnalysisType)

    struct AnalysisResult {
        QString documentPath;
        QJsonObject analysis;
        qint64 processingTime;
        bool success;
        QString errorMessage;
        QDateTime timestamp;
    };

    struct BatchAnalysisSettings {
        AnalysisTypes analysisTypes;
        int maxConcurrentJobs;
        bool generateReport;
        bool exportIndividualResults;
        QString outputDirectory;
        bool includeImages;
        bool includeFullText;
        double qualityThreshold;
        int maxKeywords;

        // Constructor with default values
        BatchAnalysisSettings()
            : analysisTypes(FullAnalysis),
              maxConcurrentJobs(4),
              generateReport(true),
              exportIndividualResults(false),
              includeImages(false),
              includeFullText(false),
              qualityThreshold(0.7),
              maxKeywords(20) {}
    };

    explicit DocumentAnalyzer(QObject* parent = nullptr);
    ~DocumentAnalyzer();

    // Single document analysis
    AnalysisResult analyzeDocument(const QString& filePath,
                                   AnalysisTypes types = FullAnalysis);
    AnalysisResult analyzeDocument(Poppler::Document* document,
                                   AnalysisTypes types = FullAnalysis);

    // Batch analysis
    void startBatchAnalysis(
        const QStringList& filePaths,
        const BatchAnalysisSettings& settings = BatchAnalysisSettings());
    void stopBatchAnalysis();
    bool isBatchAnalysisRunning() const;

    // Progress and status
    int getTotalDocuments() const;
    int getProcessedDocuments() const;
    int getFailedDocuments() const;
    double getProgressPercentage() const;
    QStringList getFailedDocumentPaths() const;

    // Results management
    QList<AnalysisResult> getAllResults() const;
    AnalysisResult getResult(const QString& filePath) const;
    void clearResults();

    // Export and reporting
    bool exportBatchReport(const QString& filePath) const;
    bool exportResultsToJson(const QString& filePath) const;
    bool exportResultsToCSV(const QString& filePath) const;
    QString generateSummaryReport() const;

    // Comparison utilities
    double compareDocuments(const QString& filePath1, const QString& filePath2);
    QJsonObject generateComparisonReport(const QString& filePath1,
                                         const QString& filePath2);
    QStringList findSimilarDocuments(const QString& referenceDocument,
                                     double threshold = 0.8);

    // Advanced analysis functions
    QJsonObject performTextAnalysis(Poppler::Document* document);
    QJsonObject performImageAnalysis(Poppler::Document* document);
    QJsonObject performStructureAnalysis(Poppler::Document* document);
    QJsonObject performSecurityAnalysis(Poppler::Document* document);
    QJsonObject performQualityAnalysis(Poppler::Document* document);
    QJsonObject performAccessibilityAnalysis(Poppler::Document* document);

    // Statistical functions
    QJsonObject generateDocumentStatistics(
        const QList<AnalysisResult>& results);
    QJsonObject generateCorrelationAnalysis(
        const QList<AnalysisResult>& results);
    QStringList identifyOutliers(const QList<AnalysisResult>& results);
    QJsonObject generateTrendAnalysis(const QList<AnalysisResult>& results);

    // Machine learning utilities
    QJsonObject trainDocumentClassifier(
        const QList<AnalysisResult>& trainingData);
    QString classifyDocument(const AnalysisResult& result,
                             const QJsonObject& classifier);
    QStringList extractFeatures(const AnalysisResult& result);
    double calculateDocumentSimilarity(const AnalysisResult& result1,
                                       const AnalysisResult& result2);

    // Optimization and recommendations
    QJsonObject generateOptimizationRecommendations(
        const AnalysisResult& result);
    QStringList identifyDuplicateDocuments(const QList<AnalysisResult>& results,
                                           double threshold = 0.95);
    QJsonObject suggestDocumentImprovements(const AnalysisResult& result);
    QStringList recommendCompressionStrategies(const AnalysisResult& result);

    // Validation and quality assurance
    bool validateAnalysisResult(const AnalysisResult& result);
    QStringList identifyAnalysisIssues(const AnalysisResult& result);
    double calculateAnalysisConfidence(const AnalysisResult& result);
    bool isAnalysisReliable(const AnalysisResult& result,
                            double confidenceThreshold = 0.8);

    // Settings and configuration
    void setAnalysisSettings(const BatchAnalysisSettings& settings);
    BatchAnalysisSettings getAnalysisSettings() const;
    void setMaxConcurrentJobs(int maxJobs);
    int getMaxConcurrentJobs() const;

    // Caching and performance
    void enableResultCaching(bool enabled);
    bool isResultCachingEnabled() const;
    void clearCache();
    qint64 getCacheSize() const;
    void setMaxCacheSize(qint64 maxSize);

    // Plugin integration
    void registerAnalysisPlugin(const QString& pluginName, QObject* plugin);
    void unregisterAnalysisPlugin(const QString& pluginName);
    QStringList getRegisteredPlugins() const;
    bool isPluginRegistered(const QString& pluginName) const;

signals:
    void batchAnalysisStarted(int totalDocuments);
    void batchAnalysisFinished();
    void batchAnalysisProgress(int processed, int total, double percentage);
    void documentAnalyzed(const QString& filePath,
                          const AnalysisResult& result);
    void documentAnalysisFailed(const QString& filePath, const QString& error);
    void analysisError(const QString& error);
    void reportGenerated(const QString& filePath);
    void cacheUpdated(qint64 cacheSize);

private slots:
    void onDocumentAnalysisFinished();
    void onBatchProgressUpdate();

private:
    // Internal analysis functions
    AnalysisResult performAnalysis(Poppler::Document* document,
                                   const QString& filePath,
                                   AnalysisTypes types);
    QJsonObject combineAnalysisResults(const QList<QJsonObject>& results);
    void updateBatchProgress();
    void finalizeBatchAnalysis();

    // Helper functions
    QString generateAnalysisId() const;
    QJsonObject createErrorResult(const QString& error) const;
    bool isValidDocument(Poppler::Document* document) const;
    QString formatAnalysisTime(qint64 milliseconds) const;

    // Cache management
    void cacheResult(const QString& key, const AnalysisResult& result);
    AnalysisResult getCachedResult(const QString& key) const;
    bool hasCachedResult(const QString& key) const;
    void evictOldCacheEntries();

    // Data members
    QList<AnalysisResult> m_results;
    QHash<QString, AnalysisResult> m_resultCache;
    QHash<QString, QObject*> m_analysisPlugins;

    BatchAnalysisSettings m_settings;
    QStringList m_batchFilePaths;
    QStringList m_failedPaths;

    int m_totalDocuments;
    int m_processedDocuments;
    int m_failedDocuments;

    bool m_batchRunning;
    bool m_cachingEnabled;
    qint64 m_maxCacheSize;

    QTimer* m_progressTimer;
    QElapsedTimer m_batchTimer;

    // For future concurrent processing
    QFutureWatcher<AnalysisResult>* m_analysisWatcher;

    static const int DEFAULT_MAX_CONCURRENT_JOBS = 4;
    static const qint64 DEFAULT_MAX_CACHE_SIZE = 100 * 1024 * 1024;  // 100MB
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DocumentAnalyzer::AnalysisTypes)
