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
#include <memory>
#include "Logger.h"
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
            analysis["text"] = performTextAnalysis(document);
        }

        if (types & ImageAnalysis) {
            analysis["images"] = performImageAnalysis(document);
        }

        if (types & StructureAnalysis) {
            analysis["structure"] = performStructureAnalysis(document);
        }

        if (types & SecurityAnalysis) {
            analysis["security"] = performSecurityAnalysis(document);
        }

        if (types & QualityAnalysis) {
            analysis["quality"] = performQualityAnalysis(document);
        }

        if (types & AccessibilityAnalysis) {
            analysis["accessibility"] = performAccessibilityAnalysis(document);
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
    } else if (milliseconds < 60000) {
        return QString("%1.%2 s")
            .arg(milliseconds / 1000)
            .arg((milliseconds % 1000) / 100);
    } else {
        int minutes = milliseconds / 60000;
        int seconds = (milliseconds % 60000) / 1000;
        return QString("%1m %2s").arg(minutes).arg(seconds);
    }
}

void DocumentAnalyzer::onBatchProgressUpdate() { updateBatchProgress(); }

// Analysis function implementations
QJsonObject DocumentAnalyzer::performTextAnalysis(Poppler::Document* document) {
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

QJsonObject DocumentAnalyzer::performImageAnalysis(
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

QJsonObject DocumentAnalyzer::performStructureAnalysis(
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

QJsonObject DocumentAnalyzer::performSecurityAnalysis(
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

QJsonObject DocumentAnalyzer::performQualityAnalysis(
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

QJsonObject DocumentAnalyzer::performAccessibilityAnalysis(
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
