#include "DocumentAnalyzer.h"
#include <poppler-qt6.h>
#include <QBuffer>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <memory>
#include <numeric>
#include "../logging/Logger.h"
#include "PDFUtilities.h"

DocumentAnalyzer::DocumentAnalyzer(QObject* parent)
    : QObject(parent),
      m_totalDocuments(0),
      m_processedDocuments(0),
      m_failedDocuments(0),
      m_batchRunning(false),
      m_cachingEnabled(true),
      m_maxCacheSize(DEFAULT_MAX_CACHE_SIZE),
      m_progressTimer(nullptr),
      m_analysisWatcher(nullptr) {
    m_progressTimer = new QTimer(this);
    m_progressTimer->setInterval(1000);  // Update progress every second
    connect(m_progressTimer, &QTimer::timeout, this,
            &DocumentAnalyzer::onBatchProgressUpdate);

    // Initialize default settings
    m_settings.analysisTypes = FullAnalysis;
    m_settings.maxConcurrentJobs = DEFAULT_MAX_CONCURRENT_JOBS;
    m_settings.generateReport = true;
    m_settings.exportIndividualResults = false;
    m_settings.includeImages = false;
    m_settings.includeFullText = false;
    m_settings.qualityThreshold = 0.7;
    m_settings.maxKeywords = 20;
}

DocumentAnalyzer::~DocumentAnalyzer() {
    if (m_batchRunning) {
        stopBatchAnalysis();
    }
}

DocumentAnalyzer::AnalysisResult DocumentAnalyzer::analyzeDocument(
    const QString& filePath, AnalysisTypes types) {
    QElapsedTimer timer;
    timer.start();

    AnalysisResult result;
    result.documentPath = filePath;
    result.timestamp = QDateTime::currentDateTime();
    result.success = false;

    // Check cache first
    if (m_cachingEnabled) {
        QString cacheKey =
            QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Md5)
                .toHex();
        if (hasCachedResult(cacheKey)) {
            return getCachedResult(cacheKey);
        }
    }

    // Load document
    std::unique_ptr<Poppler::Document> document(
        Poppler::Document::load(filePath));
    if (!document) {
        result.errorMessage = "Failed to load document";
        result.processingTime = timer.elapsed();
        return result;
    }

    if (document->isLocked()) {
        result.errorMessage = "Document is password protected";
        result.processingTime = timer.elapsed();
        return result;
    }

    result = performAnalysis(document.get(), filePath, types);
    result.processingTime = timer.elapsed();

    // Cache result
    if (m_cachingEnabled && result.success) {
        QString cacheKey =
            QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Md5)
                .toHex();
        cacheResult(cacheKey, result);
    }

    return result;
}

DocumentAnalyzer::AnalysisResult DocumentAnalyzer::analyzeDocument(
    Poppler::Document* document, AnalysisTypes types) {
    QElapsedTimer timer;
    timer.start();

    AnalysisResult result;
    result.documentPath = "memory_document";
    result.timestamp = QDateTime::currentDateTime();
    result.success = false;

    if (!document) {
        result.errorMessage = "Invalid document pointer";
        result.processingTime = timer.elapsed();
        return result;
    }

    result = performAnalysis(document, "memory_document", types);
    result.processingTime = timer.elapsed();

    return result;
}

void DocumentAnalyzer::startBatchAnalysis(
    const QStringList& filePaths, const BatchAnalysisSettings& settings) {
    if (m_batchRunning) {
        Logger::instance().warning("[utils] Batch analysis already running");
        return;
    }

    m_settings = settings;
    m_batchFilePaths = filePaths;
    m_failedPaths.clear();
    m_results.clear();

    m_totalDocuments = filePaths.size();
    m_processedDocuments = 0;
    m_failedDocuments = 0;
    m_batchRunning = true;

    m_batchTimer.start();
    m_progressTimer->start();

    emit batchAnalysisStarted(m_totalDocuments);

    // For now, process documents sequentially
    // In a real implementation, you would use QThreadPool for concurrent
    // processing
    for (const QString& filePath : filePaths) {
        if (!m_batchRunning) {
            break;  // Analysis was stopped
        }

        AnalysisResult result =
            analyzeDocument(filePath, m_settings.analysisTypes);
        m_results.append(result);

        if (result.success) {
            emit documentAnalyzed(filePath, result);
        } else {
            m_failedPaths.append(filePath);
            m_failedDocuments++;
            emit documentAnalysisFailed(filePath, result.errorMessage);
        }

        m_processedDocuments++;
        updateBatchProgress();
    }

    finalizeBatchAnalysis();
}

void DocumentAnalyzer::stopBatchAnalysis() {
    if (!m_batchRunning) {
        return;
    }

    m_batchRunning = false;
    m_progressTimer->stop();

    finalizeBatchAnalysis();
}

bool DocumentAnalyzer::isBatchAnalysisRunning() const { return m_batchRunning; }

int DocumentAnalyzer::getTotalDocuments() const { return m_totalDocuments; }

int DocumentAnalyzer::getProcessedDocuments() const {
    return m_processedDocuments;
}

int DocumentAnalyzer::getFailedDocuments() const { return m_failedDocuments; }

double DocumentAnalyzer::getProgressPercentage() const {
    if (m_totalDocuments == 0) {
        return 0.0;
    }
    return (static_cast<double>(m_processedDocuments) / m_totalDocuments) *
           100.0;
}

QStringList DocumentAnalyzer::getFailedDocumentPaths() const {
    return m_failedPaths;
}

QList<DocumentAnalyzer::AnalysisResult> DocumentAnalyzer::getAllResults()
    const {
    return m_results;
}

DocumentAnalyzer::AnalysisResult DocumentAnalyzer::getResult(
    const QString& filePath) const {
    for (const AnalysisResult& result : m_results) {
        if (result.documentPath == filePath) {
            return result;
        }
    }

    return AnalysisResult();  // Return empty result if not found
}

void DocumentAnalyzer::clearResults() {
    m_results.clear();
    m_failedPaths.clear();
    m_processedDocuments = 0;
    m_failedDocuments = 0;
    m_totalDocuments = 0;
}

bool DocumentAnalyzer::exportBatchReport(const QString& filePath) const {
    QString report = generateSummaryReport();

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << report;

    return true;
}

bool DocumentAnalyzer::exportResultsToJson(const QString& filePath) const {
    QJsonObject root;
    QJsonArray resultsArray;

    for (const AnalysisResult& result : m_results) {
        QJsonObject resultObj;
        resultObj["documentPath"] = result.documentPath;
        resultObj["analysis"] = result.analysis;
        resultObj["processingTime"] = result.processingTime;
        resultObj["success"] = result.success;
        resultObj["errorMessage"] = result.errorMessage;
        resultObj["timestamp"] = result.timestamp.toString(Qt::ISODate);

        resultsArray.append(resultObj);
    }

    root["results"] = resultsArray;
    root["totalDocuments"] = m_totalDocuments;
    root["processedDocuments"] = m_processedDocuments;
    root["failedDocuments"] = m_failedDocuments;
    root["exportTimestamp"] =
        QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonDocument doc(root);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson());
    return true;
}

QString DocumentAnalyzer::generateSummaryReport() const {
    QString report;
    QTextStream stream(&report);

    stream << "Document Analysis Summary Report\n";
    stream << "================================\n\n";

    stream << "Analysis Overview:\n";
    stream << "  Total documents: " << m_totalDocuments << "\n";
    stream << "  Successfully processed: "
           << (m_processedDocuments - m_failedDocuments) << "\n";
    stream << "  Failed: " << m_failedDocuments << "\n";
    stream << "  Success rate: "
           << QString::number((1.0 - static_cast<double>(m_failedDocuments) /
                                         m_totalDocuments) *
                                  100,
                              'f', 1)
           << "%\n\n";

    if (!m_failedPaths.isEmpty()) {
        stream << "Failed Documents:\n";
        for (const QString& path : m_failedPaths) {
            stream << "  - " << path << "\n";
        }
        stream << "\n";
    }

    // Calculate statistics
    qint64 totalProcessingTime = 0;
    int totalPages = 0;
    int totalWords = 0;

    for (const AnalysisResult& result : m_results) {
        if (result.success) {
            totalProcessingTime += result.processingTime;

            if (result.analysis.contains("pageCount")) {
                totalPages += result.analysis["pageCount"].toInt();
            }

            if (result.analysis.contains("totalWords")) {
                totalWords += result.analysis["totalWords"].toInt();
            }
        }
    }

    stream << "Processing Statistics:\n";
    stream << "  Total processing time: "
           << formatAnalysisTime(totalProcessingTime) << "\n";
    stream << "  Average time per document: "
           << formatAnalysisTime(totalProcessingTime /
                                 qMax(1, m_results.size()))
           << "\n";
    stream << "  Total pages processed: " << totalPages << "\n";
    stream << "  Total words analyzed: " << totalWords << "\n\n";

    stream << "Report generated: "
           << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";

    return report;
}

// Private helper functions
DocumentAnalyzer::AnalysisResult DocumentAnalyzer::performAnalysis(
    Poppler::Document* document, const QString& filePath, AnalysisTypes types) {
    AnalysisResult result;
    result.documentPath = filePath;
    result.timestamp = QDateTime::currentDateTime();
    result.success = true;

    QJsonObject analysis;

    try {
        if (types & BasicAnalysis) {
            analysis["basic"] =
                QJsonObject{{"pageCount", document->numPages()},
                            {"title", document->info("Title")},
                            {"author", document->info("Author")},
                            {"subject", document->info("Subject")},
                            {"creator", document->info("Creator")},
                            {"producer", document->info("Producer")},
                            {"creationDate", document->info("CreationDate")},
                            {"modificationDate", document->info("ModDate")}};
        }

        if (types & TextAnalysis) {
            analysis["text"] = performTextAnalysisImpl(document);
        }

        if (types & ImageAnalysis) {
            analysis["images"] = performImageAnalysisImpl(document);
        }

        if (types & StructureAnalysis) {
            analysis["structure"] = performStructureAnalysisImpl(document);
        }

        if (types & SecurityAnalysis) {
            analysis["security"] = performSecurityAnalysisImpl(document);
        }

        if (types & QualityAnalysis) {
            analysis["quality"] = performQualityAnalysisImpl(document);
        }

        if (types & AccessibilityAnalysis) {
            analysis["accessibility"] =
                performAccessibilityAnalysisImpl(document);
        }

        result.analysis = analysis;

    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = QString("Analysis failed: %1").arg(e.what());
    } catch (...) {
        result.success = false;
        result.errorMessage = "Unknown error during analysis";
    }

    return result;
}

void DocumentAnalyzer::updateBatchProgress() {
    double percentage = getProgressPercentage();
    emit batchAnalysisProgress(m_processedDocuments, m_totalDocuments,
                               percentage);
}

void DocumentAnalyzer::finalizeBatchAnalysis() {
    m_batchRunning = false;
    m_progressTimer->stop();

    if (m_settings.generateReport) {
        QString reportPath =
            m_settings.outputDirectory + "/analysis_report.txt";
        if (exportBatchReport(reportPath)) {
            emit reportGenerated(reportPath);
        }
    }

    emit batchAnalysisFinished();
}

QString DocumentAnalyzer::formatAnalysisTime(qint64 milliseconds) const {
    if (milliseconds < 1000) {
        return QString("%1 ms").arg(milliseconds);
    }
    if (milliseconds < 60000) {
        return QString("%1.%2 s")
            .arg(milliseconds / 1000)
            .arg((milliseconds % 1000) / 100);
    }
    int minutes = milliseconds / 60000;
    int seconds = (milliseconds % 60000) / 1000;
    return QString("%1m %2s").arg(minutes).arg(seconds);
}

void DocumentAnalyzer::onBatchProgressUpdate() { updateBatchProgress(); }

// Static convenience wrappers for backward compatibility
QJsonObject DocumentAnalyzer::performTextAnalysis(Poppler::Document* document) {
    DocumentAnalyzer analyzer;
    return analyzer.DocumentAnalyzer::performTextAnalysisImpl(document);
}

QJsonObject DocumentAnalyzer::performImageAnalysis(
    Poppler::Document* document) {
    DocumentAnalyzer analyzer;
    return analyzer.DocumentAnalyzer::performImageAnalysisImpl(document);
}

QJsonObject DocumentAnalyzer::performStructureAnalysis(
    Poppler::Document* document) {
    DocumentAnalyzer analyzer;
    return analyzer.DocumentAnalyzer::performStructureAnalysisImpl(document);
}

QJsonObject DocumentAnalyzer::performSecurityAnalysis(
    Poppler::Document* document) {
    DocumentAnalyzer analyzer;
    return analyzer.DocumentAnalyzer::performSecurityAnalysisImpl(document);
}

QJsonObject DocumentAnalyzer::performQualityAnalysis(
    Poppler::Document* document) {
    DocumentAnalyzer analyzer;
    return analyzer.DocumentAnalyzer::performQualityAnalysisImpl(document);
}

QJsonObject DocumentAnalyzer::performAccessibilityAnalysis(
    Poppler::Document* document) {
    DocumentAnalyzer analyzer;
    return analyzer.DocumentAnalyzer::performAccessibilityAnalysisImpl(
        document);
}

// Analysis function implementations
QJsonObject DocumentAnalyzer::performTextAnalysisImpl(
    Poppler::Document* document) {
    QJsonObject textAnalysis;

    if (!document) {
        return textAnalysis;
    }

    QStringList allText;
    int totalWords = 0;
    int totalSentences = 0;
    int totalParagraphs = 0;

    for (int i = 0; i < document->numPages(); ++i) {
        std::unique_ptr<Poppler::Page> page(document->page(i));
        if (page) {
            QString pageText = page->text(QRectF());
            allText.append(pageText);

            // Simple word counting
            QStringList words =
                pageText.split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);
            totalWords += words.size();

            // Simple sentence counting
            totalSentences += pageText.count(QRegularExpression("[.!?]+"));

            // Simple paragraph counting
            totalParagraphs +=
                pageText.count(QRegularExpression("\\n\\s*\\n")) + 1;
        }
    }

    QString fullText = allText.join(" ");

    textAnalysis["totalWords"] = totalWords;
    textAnalysis["totalSentences"] = totalSentences;
    textAnalysis["totalParagraphs"] = totalParagraphs;
    textAnalysis["totalCharacters"] = fullText.length();
    textAnalysis["averageWordsPerPage"] =
        document->numPages() > 0 ? totalWords / document->numPages() : 0;
    textAnalysis["estimatedReadingTime"] =
        totalWords / 200.0;  // 200 words per minute

    // Simple language detection
    QString language = "unknown";
    if (fullText.contains(
            QRegularExpression("\\b(the|and|that|have|for)\\b",
                               QRegularExpression::CaseInsensitiveOption))) {
        language = "english";
    } else if (fullText.contains(QRegularExpression("[\\u4e00-\\u9fff]"))) {
        language = "chinese";
    }
    textAnalysis["detectedLanguage"] = language;

    return textAnalysis;
}

QJsonObject DocumentAnalyzer::performImageAnalysisImpl(
    Poppler::Document* document) {
    QJsonObject imageAnalysis;

    if (!document) {
        return imageAnalysis;
    }

    int totalImages = 0;
    qint64 totalImageSize = 0;

    // This is a simplified implementation
    // In a real implementation, you would extract actual embedded images
    for (int i = 0; i < document->numPages(); ++i) {
        std::unique_ptr<Poppler::Page> page(document->page(i));
        if (page) {
            // Render page to estimate image content
            QImage pageImage = page->renderToImage(150, 150);
            if (!pageImage.isNull()) {
                totalImages++;

                // Estimate image size
                QByteArray imageData;
                QBuffer buffer(&imageData);
                buffer.open(QIODevice::WriteOnly);
                pageImage.save(&buffer, "PNG");
                totalImageSize += imageData.size();
            }
        }
    }

    imageAnalysis["totalImages"] = totalImages;
    imageAnalysis["estimatedTotalSize"] = totalImageSize;
    imageAnalysis["averageImageSize"] =
        totalImages > 0 ? totalImageSize / totalImages : 0;
    imageAnalysis["imagesPerPage"] =
        document->numPages() > 0
            ? static_cast<double>(totalImages) / document->numPages()
            : 0.0;

    return imageAnalysis;
}

QJsonObject DocumentAnalyzer::performStructureAnalysisImpl(
    Poppler::Document* document) {
    QJsonObject structureAnalysis;

    if (!document) {
        return structureAnalysis;
    }

    structureAnalysis["pageCount"] = document->numPages();

    // Analyze page sizes
    QList<QSizeF> pageSizes;
    bool uniformSize = true;
    QSizeF firstPageSize;

    for (int i = 0; i < document->numPages(); ++i) {
        std::unique_ptr<Poppler::Page> page(document->page(i));
        if (page) {
            QSizeF pageSize = page->pageSizeF();
            pageSizes.append(pageSize);

            if (i == 0) {
                firstPageSize = pageSize;
            } else if (pageSize != firstPageSize) {
                uniformSize = false;
            }
        }
    }

    structureAnalysis["uniformPageSize"] = uniformSize;
    if (!pageSizes.isEmpty()) {
        structureAnalysis["pageWidth"] = firstPageSize.width();
        structureAnalysis["pageHeight"] = firstPageSize.height();
    }

    return structureAnalysis;
}

QJsonObject DocumentAnalyzer::performSecurityAnalysisImpl(
    Poppler::Document* document) {
    QJsonObject securityAnalysis;

    if (!document) {
        return securityAnalysis;
    }

    securityAnalysis["isEncrypted"] = document->isEncrypted();
    securityAnalysis["isLocked"] = document->isLocked();

    // These would need proper permission checking in a real implementation
    securityAnalysis["canPrint"] = true;
    securityAnalysis["canCopy"] = true;
    securityAnalysis["canModify"] = false;
    securityAnalysis["canExtractText"] = true;

    return securityAnalysis;
}

QJsonObject DocumentAnalyzer::performQualityAnalysisImpl(
    Poppler::Document* document) {
    QJsonObject qualityAnalysis;

    if (!document) {
        return qualityAnalysis;
    }

    // Simple quality metrics
    double qualityScore = 1.0;
    QStringList issues;

    // Check for very small or very large documents
    if (document->numPages() < 1) {
        qualityScore -= 0.5;
        issues.append("No pages found");
    } else if (document->numPages() > 1000) {
        qualityScore -= 0.1;
        issues.append("Very large document (>1000 pages)");
    }

    // Check for text content
    bool hasText = false;
    for (int i = 0; i < qMin(5, document->numPages()); ++i) {
        std::unique_ptr<Poppler::Page> page(document->page(i));
        if (page) {
            QString pageText = page->text(QRectF());
            if (!pageText.trimmed().isEmpty()) {
                hasText = true;
                break;
            }
        }
    }

    if (!hasText) {
        qualityScore -= 0.3;
        issues.append("No extractable text found");
    }

    qualityAnalysis["qualityScore"] = qMax(0.0, qualityScore);
    qualityAnalysis["issues"] = QJsonArray::fromStringList(issues);
    qualityAnalysis["hasText"] = hasText;

    return qualityAnalysis;
}

QJsonObject DocumentAnalyzer::performAccessibilityAnalysisImpl(
    Poppler::Document* document) {
    QJsonObject accessibilityAnalysis;

    if (!document) {
        return accessibilityAnalysis;
    }

    // Simple accessibility checks
    QStringList accessibilityIssues;
    double accessibilityScore = 1.0;

    // Check if document has text (important for screen readers)
    bool hasExtractableText = false;
    for (int i = 0; i < qMin(3, document->numPages()); ++i) {
        std::unique_ptr<Poppler::Page> page(document->page(i));
        if (page) {
            QString pageText = page->text(QRectF());
            if (!pageText.trimmed().isEmpty()) {
                hasExtractableText = true;
                break;
            }
        }
    }

    if (!hasExtractableText) {
        accessibilityScore -= 0.5;
        accessibilityIssues.append("No extractable text for screen readers");
    }

    // Check document metadata
    if (document->info("Title").isEmpty()) {
        accessibilityScore -= 0.2;
        accessibilityIssues.append("Missing document title");
    }

    if (document->info("Author").isEmpty()) {
        accessibilityScore -= 0.1;
        accessibilityIssues.append("Missing author information");
    }

    accessibilityAnalysis["accessibilityScore"] = qMax(0.0, accessibilityScore);
    accessibilityAnalysis["issues"] =
        QJsonArray::fromStringList(accessibilityIssues);
    accessibilityAnalysis["hasExtractableText"] = hasExtractableText;
    accessibilityAnalysis["hasTitle"] = !document->info("Title").isEmpty();
    accessibilityAnalysis["hasAuthor"] = !document->info("Author").isEmpty();

    return accessibilityAnalysis;
}

// Cache management functions
void DocumentAnalyzer::cacheResult(const QString& key,
                                   const AnalysisResult& result) {
    if (!m_cachingEnabled) {
        return;
    }

    m_resultCache[key] = result;

    // Simple cache size management
    if (m_resultCache.size() * 1024 > m_maxCacheSize) {  // Rough estimate
        evictOldCacheEntries();
    }

    emit cacheUpdated(m_resultCache.size() * 1024);
}

DocumentAnalyzer::AnalysisResult DocumentAnalyzer::getCachedResult(
    const QString& key) const {
    return m_resultCache.value(key, AnalysisResult());
}

bool DocumentAnalyzer::hasCachedResult(const QString& key) const {
    return m_resultCache.contains(key);
}

void DocumentAnalyzer::evictOldCacheEntries() {
    // Simple eviction: remove half of the cache entries
    int targetSize = m_resultCache.size() / 2;
    auto it = m_resultCache.begin();

    while (m_resultCache.size() > targetSize && it != m_resultCache.end()) {
        it = m_resultCache.erase(it);
    }
}

void DocumentAnalyzer::enableResultCaching(bool enabled) {
    m_cachingEnabled = enabled;
    if (!enabled) {
        clearCache();
    }
}

bool DocumentAnalyzer::isResultCachingEnabled() const {
    return m_cachingEnabled;
}

void DocumentAnalyzer::clearCache() {
    m_resultCache.clear();
    emit cacheUpdated(0);
}

qint64 DocumentAnalyzer::getCacheSize() const {
    return m_resultCache.size() * 1024;  // Rough estimate
}

void DocumentAnalyzer::setMaxCacheSize(qint64 maxSize) {
    m_maxCacheSize = maxSize;
    if (getCacheSize() > maxSize) {
        evictOldCacheEntries();
    }
}

void DocumentAnalyzer::onDocumentAnalysisFinished() {
    // Handle completion of document analysis
    Logger::instance().debug("[utils] Document analysis completed");

    // Update progress and notify completion
    updateBatchProgress();

    // This slot is called when analysis is finished
    // It can be used to perform cleanup or trigger additional processing
    Logger::instance().debug(
        "[utils] Analysis processing completed successfully");
}

bool DocumentAnalyzer::exportResultsToCSV(const QString& filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Logger::instance().warning("[utils] Failed to open CSV file: {}",
                                   filePath.toStdString());
        return false;
    }

    QTextStream out(&file);

    // Write CSV header
    out << "Document Path,Page Count,Total Words,Total Sentences,Total "
           "Paragraphs,"
        << "Total Characters,Total Images,Total Annotations,Success,Error "
           "Message,"
        << "Processing Time (ms),Timestamp\n";

    // Write each result
    for (const AnalysisResult& result : m_results) {
        out << QString("\"%1\"").arg(result.documentPath) << ","
            << result.analysis.value("basic")
                   .toObject()
                   .value("pageCount")
                   .toInt()
            << ","
            << result.analysis.value("text")
                   .toObject()
                   .value("totalWords")
                   .toInt()
            << ","
            << result.analysis.value("text")
                   .toObject()
                   .value("totalSentences")
                   .toInt()
            << ","
            << result.analysis.value("text")
                   .toObject()
                   .value("totalParagraphs")
                   .toInt()
            << ","
            << result.analysis.value("text")
                   .toObject()
                   .value("totalCharacters")
                   .toInt()
            << ","
            << result.analysis.value("images")
                   .toObject()
                   .value("totalImages")
                   .toInt()
            << ","
            << result.analysis.value("basic")
                   .toObject()
                   .value("annotationCount")
                   .toInt()
            << "," << (result.success ? "Yes" : "No") << ","
            << QString("\"%1\"").arg(result.errorMessage) << ","
            << result.processingTime << ","
            << result.timestamp.toString(Qt::ISODate) << "\n";
    }

    return true;
}

double DocumentAnalyzer::compareDocuments(const QString& filePath1,
                                          const QString& filePath2) {
    std::unique_ptr<Poppler::Document> doc1(Poppler::Document::load(filePath1));
    std::unique_ptr<Poppler::Document> doc2(Poppler::Document::load(filePath2));

    if (!doc1 || !doc2) {
        Logger::instance().warning(
            "[utils] Failed to load documents for comparison");
        return 0.0;
    }

    return PDFUtilities::calculateDocumentSimilarity(doc1.get(), doc2.get());
}

QJsonObject DocumentAnalyzer::generateComparisonReport(
    const QString& filePath1, const QString& filePath2) {
    QJsonObject report;

    std::unique_ptr<Poppler::Document> doc1(Poppler::Document::load(filePath1));
    std::unique_ptr<Poppler::Document> doc2(Poppler::Document::load(filePath2));

    if (!doc1) {
        report["error"] =
            QString("Failed to load document 1: %1").arg(filePath1);
        return report;
    }

    if (!doc2) {
        report["error"] =
            QString("Failed to load document 2: %1").arg(filePath2);
        return report;
    }

    report["document1"] = filePath1;
    report["document2"] = filePath2;
    report["similarity"] =
        PDFUtilities::calculateDocumentSimilarity(doc1.get(), doc2.get());
    report["metadataComparison"] =
        PDFUtilities::compareDocumentMetadata(doc1.get(), doc2.get());
    report["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    return report;
}

QStringList DocumentAnalyzer::findSimilarDocuments(
    const QString& referenceDocument, double threshold) {
    QStringList similarDocs;

    std::unique_ptr<Poppler::Document> refDoc(
        Poppler::Document::load(referenceDocument));
    if (!refDoc) {
        return similarDocs;
    }

    for (const AnalysisResult& result : m_results) {
        if (result.documentPath == referenceDocument) {
            continue;  // Skip the reference document itself
        }

        double similarity =
            compareDocuments(referenceDocument, result.documentPath);
        if (similarity >= threshold) {
            similarDocs.append(result.documentPath);
        }
    }

    return similarDocs;
}

QJsonObject DocumentAnalyzer::generateDocumentStatistics(
    const QList<AnalysisResult>& results) {
    QJsonObject stats;

    if (results.isEmpty()) {
        stats["totalDocuments"] = 0;
        return stats;
    }

    int totalPages = 0;
    int totalWords = 0;
    int totalImages = 0;
    int totalAnnotations = 0;
    qint64 totalProcessingTime = 0;
    int successCount = 0;

    for (const AnalysisResult& result : results) {
        if (result.success) {
            successCount++;
            totalPages += result.analysis.value("basic")
                              .toObject()
                              .value("pageCount")
                              .toInt();
            totalWords += result.analysis.value("text")
                              .toObject()
                              .value("totalWords")
                              .toInt();
            totalImages += result.analysis.value("images")
                               .toObject()
                               .value("totalImages")
                               .toInt();
            totalProcessingTime += result.processingTime;
        }
    }

    stats["totalDocuments"] = results.size();
    stats["successfulDocuments"] = successCount;
    stats["failedDocuments"] = results.size() - successCount;
    stats["successRate"] =
        results.size() > 0 ? (double)successCount / results.size() : 0.0;
    stats["totalPages"] = totalPages;
    stats["totalWords"] = totalWords;
    stats["totalImages"] = totalImages;
    stats["averagePages"] =
        results.size() > 0 ? (double)totalPages / results.size() : 0.0;
    stats["averageWords"] =
        results.size() > 0 ? (double)totalWords / results.size() : 0.0;
    stats["totalProcessingTime"] = totalProcessingTime;
    stats["averageProcessingTime"] =
        results.size() > 0 ? (double)totalProcessingTime / results.size() : 0.0;

    return stats;
}

QJsonObject DocumentAnalyzer::generateCorrelationAnalysis(
    const QList<AnalysisResult>& results) {
    QJsonObject correlations;

    if (results.size() < 2) {
        correlations["error"] = "Not enough data for correlation analysis";
        return correlations;
    }

    // Calculate correlation between page count and word count
    QList<double> pageCounts;
    QList<double> wordCounts;

    for (const AnalysisResult& result : results) {
        if (result.success) {
            pageCounts.append(result.analysis.value("basic")
                                  .toObject()
                                  .value("pageCount")
                                  .toDouble());
            wordCounts.append(result.analysis.value("text")
                                  .toObject()
                                  .value("totalWords")
                                  .toDouble());
        }
    }

    double correlation = 0.0;
    if (pageCounts.size() > 1) {
        // Simple Pearson correlation calculation
        double meanPages =
            std::accumulate(pageCounts.begin(), pageCounts.end(), 0.0) /
            pageCounts.size();
        double meanWords =
            std::accumulate(wordCounts.begin(), wordCounts.end(), 0.0) /
            wordCounts.size();

        double numerator = 0.0;
        double denomPages = 0.0;
        double denomWords = 0.0;

        for (int i = 0; i < pageCounts.size(); ++i) {
            double diffPages = pageCounts[i] - meanPages;
            double diffWords = wordCounts[i] - meanWords;
            numerator += diffPages * diffWords;
            denomPages += diffPages * diffPages;
            denomWords += diffWords * diffWords;
        }

        if (denomPages > 0 && denomWords > 0) {
            correlation = numerator / std::sqrt(denomPages * denomWords);
        }
    }

    correlations["pagesWordCount"] = correlation;
    correlations["sampleSize"] = pageCounts.size();

    return correlations;
}

QStringList DocumentAnalyzer::identifyOutliers(
    const QList<AnalysisResult>& results) {
    QStringList outliers;

    if (results.size() < 3) {
        return outliers;  // Not enough data for outlier detection
    }

    QList<int> pageCounts;
    QList<int> wordCounts;

    for (const AnalysisResult& result : results) {
        if (result.success) {
            pageCounts.append(result.analysis.value("basic")
                                  .toObject()
                                  .value("pageCount")
                                  .toInt());
            wordCounts.append(result.analysis.value("text")
                                  .toObject()
                                  .value("totalWords")
                                  .toInt());
        }
    }

    if (pageCounts.isEmpty()) {
        return outliers;
    }

    // Calculate mean and standard deviation for pages
    double meanPages =
        std::accumulate(pageCounts.begin(), pageCounts.end(), 0.0) /
        pageCounts.size();
    double variance = 0.0;
    for (int count : pageCounts) {
        variance += (count - meanPages) * (count - meanPages);
    }
    double stdDev = std::sqrt(variance / pageCounts.size());

    // Identify outliers (values > 2 standard deviations from mean)
    for (int i = 0; i < results.size() && i < pageCounts.size(); ++i) {
        if (results[i].success) {
            double zScore =
                std::abs(pageCounts[i] - meanPages) / (stdDev + 0.001);
            if (zScore > 2.0) {
                outliers.append(QString("%1 (page count: %2, z-score: %3)")
                                    .arg(results[i].documentPath)
                                    .arg(pageCounts[i])
                                    .arg(zScore, 0, 'f', 2));
            }
        }
    }

    return outliers;
}

QJsonObject DocumentAnalyzer::generateTrendAnalysis(
    const QList<AnalysisResult>& results) {
    QJsonObject trends;

    if (results.isEmpty()) {
        trends["error"] = "No data for trend analysis";
        return trends;
    }

    // Analyze trends over time
    QList<QPair<QDateTime, int>> timeSeries;
    for (const AnalysisResult& result : results) {
        if (result.success && result.timestamp.isValid()) {
            int pageCount = result.analysis.value("basic")
                                .toObject()
                                .value("pageCount")
                                .toInt();
            timeSeries.append(qMakePair(result.timestamp, pageCount));
        }
    }

    std::sort(timeSeries.begin(), timeSeries.end(),
              [](const QPair<QDateTime, int>& a,
                 const QPair<QDateTime, int>& b) { return a.first < b.first; });

    if (timeSeries.size() > 1) {
        trends["dataPoints"] = timeSeries.size();
        trends["firstTimestamp"] =
            timeSeries.first().first.toString(Qt::ISODate);
        trends["lastTimestamp"] = timeSeries.last().first.toString(Qt::ISODate);
        trends["earliestValue"] = timeSeries.first().second;
        trends["latestValue"] = timeSeries.last().second;

        // Simple trend indicator
        double avgFirst = 0.0, avgLast = 0.0;
        int halfPoint = timeSeries.size() / 2;

        for (int i = 0; i < halfPoint; ++i) {
            avgFirst += timeSeries[i].second;
        }
        for (int i = halfPoint; i < timeSeries.size(); ++i) {
            avgLast += timeSeries[i].second;
        }

        avgFirst /= halfPoint;
        avgLast /= (timeSeries.size() - halfPoint);

        trends["trend"] = avgLast > avgFirst
                              ? "increasing"
                              : (avgLast < avgFirst ? "decreasing" : "stable");
    }

    return trends;
}

QJsonObject DocumentAnalyzer::trainDocumentClassifier(
    const QList<AnalysisResult>& trainingData) {
    QJsonObject classifier;

    if (trainingData.isEmpty()) {
        classifier["error"] = "No training data provided";
        return classifier;
    }

    // Simple classifier based on document features
    // In production, use proper ML library like TensorFlow or PyTorch
    classifier["type"] = "simple_feature_based";
    classifier["trainingSize"] = trainingData.size();
    classifier["trainedAt"] =
        QDateTime::currentDateTime().toString(Qt::ISODate);

    // Calculate feature statistics for classification
    QJsonArray features;
    QJsonObject pageCountFeature;
    pageCountFeature["name"] = "pageCount";
    pageCountFeature["weight"] = 0.3;
    features.append(pageCountFeature);

    QJsonObject wordCountFeature;
    wordCountFeature["name"] = "wordCount";
    wordCountFeature["weight"] = 0.4;
    features.append(wordCountFeature);

    QJsonObject imageCountFeature;
    imageCountFeature["name"] = "imageCount";
    imageCountFeature["weight"] = 0.3;
    features.append(imageCountFeature);

    classifier["features"] = features;

    return classifier;
}

QString DocumentAnalyzer::classifyDocument(const AnalysisResult& result,
                                           const QJsonObject& classifier) {
    if (!result.success || classifier.isEmpty()) {
        return "unclassified";
    }

    // Simple classification based on document characteristics
    int pageCount =
        result.analysis.value("basic").toObject().value("pageCount").toInt();
    int wordCount =
        result.analysis.value("text").toObject().value("totalWords").toInt();
    int imageCount =
        result.analysis.value("images").toObject().value("totalImages").toInt();

    // Simple rule-based classification
    if (pageCount < 5 && wordCount < 1000) {
        return "short_document";
    }
    if (pageCount > 100) {
        return "book";
    }
    if (imageCount > pageCount * 2) {
        return "image_heavy";
    } else if (wordCount > 10000) {
        return "text_heavy";
    } else {
        return "standard_document";
    }
}

QStringList DocumentAnalyzer::extractFeatures(const AnalysisResult& result) {
    QStringList features;

    if (!result.success) {
        return features;
    }

    QJsonObject basic = result.analysis.value("basic").toObject();
    QJsonObject text = result.analysis.value("text").toObject();
    QJsonObject images = result.analysis.value("images").toObject();

    features.append(QString("pages:%1").arg(basic.value("pageCount").toInt()));
    features.append(QString("words:%1").arg(text.value("totalWords").toInt()));
    features.append(
        QString("sentences:%1").arg(text.value("totalSentences").toInt()));
    features.append(
        QString("paragraphs:%1").arg(text.value("totalParagraphs").toInt()));
    features.append(
        QString("images:%1").arg(images.value("totalImages").toInt()));
    features.append(
        QString("language:%1").arg(text.value("detectedLanguage").toString()));

    return features;
}

double DocumentAnalyzer::calculateDocumentSimilarity(
    const AnalysisResult& result1, const AnalysisResult& result2) {
    if (!result1.success || !result2.success) {
        return 0.0;
    }

    QStringList features1 = extractFeatures(result1);
    QStringList features2 = extractFeatures(result2);

    // Calculate Jaccard similarity
    int commonFeatures = 0;
    for (const QString& feature : features1) {
        if (features2.contains(feature)) {
            commonFeatures++;
        }
    }

    int totalFeatures = features1.size() + features2.size() - commonFeatures;
    return totalFeatures > 0 ? (double)commonFeatures / totalFeatures : 0.0;
}

QJsonObject DocumentAnalyzer::generateOptimizationRecommendations(
    const AnalysisResult& result) {
    QJsonObject recommendations;
    QJsonArray suggestions;

    if (!result.success) {
        recommendations["error"] =
            "Cannot generate recommendations for failed analysis";
        return recommendations;
    }

    QJsonObject basic = result.analysis.value("basic").toObject();
    QJsonObject images = result.analysis.value("images").toObject();
    QJsonObject quality = result.analysis.value("quality").toObject();

    int pageCount = basic.value("pageCount").toInt();
    int imageCount = images.value("totalImages").toInt();
    double qualityScore = quality.value("qualityScore").toDouble();

    if (pageCount > 100) {
        QJsonObject suggestion;
        suggestion["type"] = "compression";
        suggestion["description"] =
            "Consider compressing the document to reduce file size";
        suggestion["priority"] = "medium";
        suggestions.append(suggestion);
    }

    if (imageCount > pageCount * 3) {
        QJsonObject suggestion;
        suggestion["type"] = "image_optimization";
        suggestion["description"] = "Optimize images to reduce file size";
        suggestion["priority"] = "high";
        suggestions.append(suggestion);
    }

    if (qualityScore < 0.7) {
        QJsonObject suggestion;
        suggestion["type"] = "quality_improvement";
        suggestion["description"] =
            "Improve document quality by addressing identified issues";
        suggestion["priority"] = "high";
        suggestions.append(suggestion);
    }

    recommendations["suggestions"] = suggestions;
    recommendations["count"] = suggestions.size();

    return recommendations;
}

QStringList DocumentAnalyzer::identifyDuplicateDocuments(
    const QList<AnalysisResult>& results, double threshold) {
    QStringList duplicates;

    for (int i = 0; i < results.size(); ++i) {
        for (int j = i + 1; j < results.size(); ++j) {
            double similarity =
                calculateDocumentSimilarity(results[i], results[j]);
            if (similarity >= threshold) {
                duplicates.append(
                    QString("Duplicate: %1 <-> %2 (similarity: %3)")
                        .arg(results[i].documentPath)
                        .arg(results[j].documentPath)
                        .arg(similarity, 0, 'f', 3));
            }
        }
    }

    return duplicates;
}

QJsonObject DocumentAnalyzer::suggestDocumentImprovements(
    const AnalysisResult& result) {
    QJsonObject improvements;
    QJsonArray suggestions;

    if (!result.success) {
        improvements["error"] =
            "Cannot suggest improvements for failed analysis";
        return improvements;
    }

    QJsonObject quality = result.analysis.value("quality").toObject();
    QJsonObject accessibility =
        result.analysis.value("accessibility").toObject();
    QJsonObject security = result.analysis.value("security").toObject();

    // Quality improvements
    QJsonArray qualityIssues = quality.value("issues").toArray();
    for (const QJsonValue& issue : qualityIssues) {
        QJsonObject suggestion;
        suggestion["category"] = "quality";
        suggestion["issue"] = issue.toString();
        suggestion["improvement"] =
            "Address quality issue: " + issue.toString();
        suggestions.append(suggestion);
    }

    // Accessibility improvements
    QJsonArray accessibilityIssues = accessibility.value("issues").toArray();
    for (const QJsonValue& issue : accessibilityIssues) {
        QJsonObject suggestion;
        suggestion["category"] = "accessibility";
        suggestion["issue"] = issue.toString();
        suggestion["improvement"] =
            "Improve accessibility: " + issue.toString();
        suggestions.append(suggestion);
    }

    improvements["suggestions"] = suggestions;
    improvements["count"] = suggestions.size();

    return improvements;
}

QStringList DocumentAnalyzer::recommendCompressionStrategies(
    const AnalysisResult& result) {
    QStringList strategies;

    if (!result.success) {
        return strategies;
    }

    QJsonObject basic = result.analysis.value("basic").toObject();
    QJsonObject images = result.analysis.value("images").toObject();
    QJsonObject text = result.analysis.value("text").toObject();

    int pageCount = basic.value("pageCount").toInt();
    int imageCount = images.value("totalImages").toInt();
    qint64 estimatedSize =
        static_cast<qint64>(images.value("estimatedTotalSize").toDouble());

    if (imageCount > 0 && estimatedSize > 10 * 1024 * 1024) {
        strategies.append("Reduce image quality to 72-150 DPI");
        strategies.append("Convert images to JPEG with 85% quality");
    }

    if (pageCount > 50) {
        strategies.append("Use PDF/A format for better compression");
        strategies.append("Remove embedded fonts if not needed");
    }

    if (text.value("totalWords").toInt() > 50000) {
        strategies.append("Enable text compression");
    }

    if (strategies.isEmpty()) {
        strategies.append("Document is already well-optimized");
    }

    return strategies;
}

bool DocumentAnalyzer::validateAnalysisResult(const AnalysisResult& result) {
    if (!result.success) {
        return false;
    }

    if (result.analysis.isEmpty()) {
        return false;
    }

    if (result.documentPath.isEmpty()) {
        return false;
    }

    if (!result.timestamp.isValid()) {
        return false;
    }

    return true;
}

QStringList DocumentAnalyzer::identifyAnalysisIssues(
    const AnalysisResult& result) {
    QStringList issues;

    if (!validateAnalysisResult(result)) {
        issues.append("Analysis result validation failed");
    }

    if (!result.success) {
        issues.append("Analysis was not successful: " + result.errorMessage);
    }

    if (result.analysis.isEmpty()) {
        issues.append("Analysis data is empty");
    }

    if (result.processingTime > 60000) {  // More than 1 minute
        issues.append(QString("Very long processing time: %1 ms")
                          .arg(result.processingTime));
    }

    return issues;
}

double DocumentAnalyzer::calculateAnalysisConfidence(
    const AnalysisResult& result) {
    if (!result.success) {
        return 0.0;
    }

    double confidence = 1.0;

    // Reduce confidence based on various factors
    if (result.analysis.isEmpty()) {
        confidence -= 0.5;
    }

    if (result.processingTime < 100) {
        // Too fast might indicate incomplete analysis
        confidence -= 0.1;
    } else if (result.processingTime > 60000) {
        // Very slow might indicate issues
        confidence -= 0.2;
    }

    QJsonObject quality = result.analysis.value("quality").toObject();
    double qualityScore = quality.value("qualityScore").toDouble();
    if (qualityScore < 0.5) {
        confidence -= 0.2;
    }

    return qMax(0.0, qMin(1.0, confidence));
}

bool DocumentAnalyzer::isAnalysisReliable(const AnalysisResult& result,
                                          double confidenceThreshold) {
    return calculateAnalysisConfidence(result) >= confidenceThreshold;
}

void DocumentAnalyzer::setAnalysisSettings(
    const BatchAnalysisSettings& settings) {
    m_settings = settings;
}

DocumentAnalyzer::BatchAnalysisSettings DocumentAnalyzer::getAnalysisSettings()
    const {
    return m_settings;
}

void DocumentAnalyzer::setMaxConcurrentJobs(int maxJobs) {
    m_settings.maxConcurrentJobs = qMax(1, maxJobs);
}

int DocumentAnalyzer::getMaxConcurrentJobs() const {
    return m_settings.maxConcurrentJobs;
}

void DocumentAnalyzer::registerAnalysisPlugin(const QString& pluginName,
                                              QObject* plugin) {
    if (pluginName.isEmpty() || !plugin) {
        Logger::instance().warning(
            "[utils] Invalid plugin registration attempted");
        return;
    }

    m_analysisPlugins[pluginName] = plugin;
    Logger::instance().info("[utils] Registered analysis plugin: {}",
                            pluginName.toStdString());
}

void DocumentAnalyzer::unregisterAnalysisPlugin(const QString& pluginName) {
    if (m_analysisPlugins.remove(pluginName)) {
        Logger::instance().info("[utils] Unregistered analysis plugin: {}",
                                pluginName.toStdString());
    }
}

QStringList DocumentAnalyzer::getRegisteredPlugins() const {
    return m_analysisPlugins.keys();
}

bool DocumentAnalyzer::isPluginRegistered(const QString& pluginName) const {
    return m_analysisPlugins.contains(pluginName);
}

QJsonObject DocumentAnalyzer::combineAnalysisResults(
    const QList<QJsonObject>& results) {
    QJsonObject combined;

    if (results.isEmpty()) {
        return combined;
    }

    // Merge all results into a single object
    for (const QJsonObject& result : results) {
        for (const QString& key : result.keys()) {
            if (!combined.contains(key)) {
                combined[key] = result[key];
            }
        }
    }

    return combined;
}

QString DocumentAnalyzer::generateAnalysisId() const {
    QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    QString random = QString::number(QRandomGenerator::global()->generate());
    QString id = timestamp + "_" + random;

    QByteArray hash =
        QCryptographicHash::hash(id.toUtf8(), QCryptographicHash::Md5);
    return QString(hash.toHex());
}

QJsonObject DocumentAnalyzer::createErrorResult(const QString& error) const {
    QJsonObject errorResult;
    errorResult["error"] = error;
    errorResult["success"] = false;
    errorResult["timestamp"] =
        QDateTime::currentDateTime().toString(Qt::ISODate);
    return errorResult;
}

bool DocumentAnalyzer::isValidDocument(Poppler::Document* document) const {
    if (!document) {
        return false;
    }

    if (document->isLocked()) {
        return false;
    }

    if (document->numPages() <= 0) {
        return false;
    }

    return true;
}
