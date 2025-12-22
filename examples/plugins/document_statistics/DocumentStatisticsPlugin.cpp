#include "DocumentStatisticsPlugin.h"
#include <QAction>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include "controller/EventBus.h"
#include "plugin/PluginHookRegistry.h"

DocumentStatisticsPlugin::DocumentStatisticsPlugin(QObject* parent)
    : PluginBase(parent),
      m_showStatsAction(nullptr),
      m_exportStatsAction(nullptr),
      m_wordsPerMinute(200) {
    m_metadata.name = "Document Statistics";
    m_metadata.version = "1.0.0";
    m_metadata.description =
        "Document analysis with word count, page statistics, and reading time";
    m_metadata.author = "SAST Readium Team";
    m_metadata.dependencies = QStringList();
    m_capabilities.provides = QStringList()
                              << "document.statistics" << "document.analysis"
                              << "ui.dock" << "export.statistics";
}

DocumentStatisticsPlugin::~DocumentStatisticsPlugin() {
    qDeleteAll(m_menuActions);
}

bool DocumentStatisticsPlugin::onInitialize() {
    m_logger.info("DocumentStatisticsPlugin: Initializing...");

    m_wordsPerMinute = m_configuration.value("wordsPerMinute").toInt(200);

    // Create UI actions
    m_showStatsAction = new QAction("Document Statistics", this);
    m_showStatsAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    connect(m_showStatsAction, &QAction::triggered, this,
            &DocumentStatisticsPlugin::onShowStatistics);
    m_menuActions.append(m_showStatsAction);

    m_exportStatsAction = new QAction("Export Statistics...", this);
    connect(m_exportStatsAction, &QAction::triggered, this,
            &DocumentStatisticsPlugin::onExportStatistics);
    m_menuActions.append(m_exportStatsAction);

    registerHooks();
    setupEventSubscriptions();

    m_logger.info("DocumentStatisticsPlugin: Initialized successfully");
    return true;
}

void DocumentStatisticsPlugin::onShutdown() {
    m_logger.info("DocumentStatisticsPlugin: Shutting down...");
    PluginHookRegistry::instance().unregisterAllCallbacks(name());
    eventBus()->unsubscribeAll(this);
    m_statsCache.clear();
}

void DocumentStatisticsPlugin::handleMessage(const QString& from,
                                             const QVariant& message) {
    QVariantMap msgMap = message.toMap();
    QString action = msgMap.value("action").toString();

    if (action == "get_statistics") {
        QString docPath = msgMap.value("documentPath").toString();
        if (docPath.isEmpty())
            docPath = m_currentDocument;

        DocumentStats stats;
        if (m_statsCache.contains(docPath)) {
            stats = m_statsCache[docPath];
        } else {
            stats = analyzeDocument(docPath);
        }

        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["pageCount"] = stats.pageCount;
        data["wordCount"] = stats.wordCount;
        data["characterCount"] = stats.characterCount;
        data["imageCount"] = stats.imageCount;
        data["readingTime"] = estimateReadingTime(stats.wordCount);
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);

    } else if (action == "analyze") {
        QString docPath = msgMap.value("documentPath").toString();
        DocumentStats stats = analyzeDocument(docPath);
        m_statsCache[docPath] = stats;
    }
}

// ============================================================================
// IUIExtension
// ============================================================================

QList<QAction*> DocumentStatisticsPlugin::menuActions() const {
    return m_menuActions;
}

QList<QAction*> DocumentStatisticsPlugin::toolbarActions() const {
    return QList<QAction*>() << m_showStatsAction;
}

QString DocumentStatisticsPlugin::statusBarMessage() const {
    if (m_currentDocument.isEmpty() ||
        !m_statsCache.contains(m_currentDocument)) {
        return QString();
    }
    const DocumentStats& stats = m_statsCache[m_currentDocument];
    return QString("Words: %1 | Pages: %2 | ~%3 min read")
        .arg(stats.wordCount)
        .arg(stats.pageCount)
        .arg(estimateReadingTime(stats.wordCount));
}

QWidget* DocumentStatisticsPlugin::createDockWidget() {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);

    QLabel* titleLabel = new QLabel("<b>Document Statistics</b>");
    layout->addWidget(titleLabel);

    QLabel* statsLabel = new QLabel("Open a document to see statistics");
    statsLabel->setObjectName("statsLabel");
    layout->addWidget(statsLabel);

    layout->addStretch();
    return widget;
}

// ============================================================================
// IDocumentProcessorPlugin
// ============================================================================

QList<PluginWorkflowStage> DocumentStatisticsPlugin::handledStages() const {
    return QList<PluginWorkflowStage>()
           << PluginWorkflowStage::PostDocumentLoad;
}

DocumentProcessingResult DocumentStatisticsPlugin::processDocument(
    PluginWorkflowStage stage, const QString& filePath,
    const QJsonObject& context) {
    Q_UNUSED(context)

    if (stage == PluginWorkflowStage::PostDocumentLoad) {
        DocumentStats stats = analyzeDocument(filePath);
        m_statsCache[filePath] = stats;
        m_currentDocument = filePath;

        return DocumentProcessingResult::createSuccess(
            QString("Analyzed: %1 words, %2 pages")
                .arg(stats.wordCount)
                .arg(stats.pageCount),
            QVariant::fromValue(extractMetadata(filePath)));
    }

    return DocumentProcessingResult::createSuccess();
}

bool DocumentStatisticsPlugin::canProcessFile(const QString& filePath) const {
    QFileInfo info(filePath);
    return supportedExtensions().contains("." + info.suffix().toLower());
}

QStringList DocumentStatisticsPlugin::supportedExtensions() const {
    return QStringList() << ".pdf";
}

QJsonObject DocumentStatisticsPlugin::extractMetadata(const QString& filePath) {
    if (!m_statsCache.contains(filePath)) {
        analyzeDocument(filePath);
    }

    const DocumentStats& stats = m_statsCache[filePath];
    QJsonObject metadata;
    metadata["pageCount"] = stats.pageCount;
    metadata["wordCount"] = stats.wordCount;
    metadata["characterCount"] = stats.characterCount;
    metadata["imageCount"] = stats.imageCount;
    metadata["linkCount"] = stats.linkCount;
    metadata["annotationCount"] = stats.annotationCount;
    metadata["fileSize"] = stats.fileSize;
    metadata["readingTimeMinutes"] = estimateReadingTime(stats.wordCount);
    metadata["analyzedAt"] = stats.analyzedAt.toString(Qt::ISODate);

    // Per-page word distribution
    QJsonObject perPage;
    for (auto it = stats.wordsPerPage.begin(); it != stats.wordsPerPage.end();
         ++it) {
        perPage[QString::number(it.key())] = it.value();
    }
    metadata["wordsPerPage"] = perPage;

    return metadata;
}

DocumentProcessingResult DocumentStatisticsPlugin::exportDocument(
    const QString& sourcePath, const QString& targetPath, const QString& format,
    const QJsonObject& options) {
    Q_UNUSED(options)

    if (!m_statsCache.contains(sourcePath)) {
        return DocumentProcessingResult::createFailure("Document not analyzed");
    }

    QJsonObject metadata = extractMetadata(sourcePath);

    if (format == "json") {
        QFile file(targetPath);
        if (!file.open(QIODevice::WriteOnly)) {
            return DocumentProcessingResult::createFailure(
                "Cannot open output file");
        }
        file.write(QJsonDocument(metadata).toJson(QJsonDocument::Indented));
        file.close();
        return DocumentProcessingResult::createSuccess(
            "Statistics exported to JSON");
    }

    if (format == "csv") {
        QFile file(targetPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return DocumentProcessingResult::createFailure(
                "Cannot open output file");
        }
        QTextStream out(&file);
        out << "Metric,Value\n";
        out << "Pages," << metadata["pageCount"].toInt() << "\n";
        out << "Words," << metadata["wordCount"].toInt() << "\n";
        out << "Characters," << metadata["characterCount"].toInt() << "\n";
        out << "Images," << metadata["imageCount"].toInt() << "\n";
        out << "Reading Time (min)," << metadata["readingTimeMinutes"].toInt()
            << "\n";
        file.close();
        return DocumentProcessingResult::createSuccess(
            "Statistics exported to CSV");
    }

    return DocumentProcessingResult::createFailure("Unsupported format: " +
                                                   format);
}

// ============================================================================
// Analysis
// ============================================================================

DocumentStats DocumentStatisticsPlugin::analyzeDocument(
    const QString& filePath) {
    m_logger.info(
        QString("DocumentStatisticsPlugin: Analyzing '%1'").arg(filePath));

    DocumentStats stats;
    stats.documentPath = filePath;
    stats.analyzedAt = QDateTime::currentDateTime();

    QFileInfo fileInfo(filePath);
    stats.fileSize = fileInfo.size();

    // Simulated analysis - in real implementation, use Poppler to extract text
    stats.pageCount = 10;    // Would be from PDF
    stats.wordCount = 5000;  // Would be from text extraction
    stats.characterCount = stats.wordCount * 5;
    stats.imageCount = 3;
    stats.linkCount = 15;
    stats.annotationCount = 0;

    // Simulate per-page word distribution
    int avgWordsPerPage = stats.wordCount / stats.pageCount;
    for (int i = 1; i <= stats.pageCount; ++i) {
        stats.wordsPerPage[i] = avgWordsPerPage + (i % 3 - 1) * 50;
    }

    m_statsCache[filePath] = stats;
    return stats;
}

int DocumentStatisticsPlugin::estimateReadingTime(int wordCount) const {
    return qMax(1, wordCount / m_wordsPerMinute);
}

// ============================================================================
// Slots
// ============================================================================

void DocumentStatisticsPlugin::onShowStatistics() {
    m_logger.info("DocumentStatisticsPlugin: Show statistics requested");
    eventBus()->publish(new Event("ui.showStatisticsPanel"));
}

void DocumentStatisticsPlugin::onExportStatistics() {
    m_logger.info("DocumentStatisticsPlugin: Export statistics requested");
    eventBus()->publish(new Event("ui.exportStatisticsDialog"));
}

// ============================================================================
// Hooks
// ============================================================================

void DocumentStatisticsPlugin::registerHooks() {
    auto& registry = PluginHookRegistry::instance();
    registry.registerCallback(
        StandardHooks::DOCUMENT_POST_LOAD, name(),
        [this](const QVariantMap& ctx) { return onDocumentAnalyzed(ctx); });
}

void DocumentStatisticsPlugin::setupEventSubscriptions() {
    eventBus()->subscribe("document.opened", this, [this](Event* event) {
        QString filePath = event->data().toString();
        m_currentDocument = filePath;
        if (!m_statsCache.contains(filePath)) {
            analyzeDocument(filePath);
        }
    });

    eventBus()->subscribe("document.closed", this, [this](Event* event) {
        QString filePath = event->data().toString();
        if (m_currentDocument == filePath) {
            m_currentDocument.clear();
        }
    });
}

QVariant DocumentStatisticsPlugin::onDocumentAnalyzed(
    const QVariantMap& context) {
    QString filePath = context.value("filePath").toString();
    DocumentStats stats = analyzeDocument(filePath);

    QVariantMap result;
    result["wordCount"] = stats.wordCount;
    result["pageCount"] = stats.pageCount;
    result["readingTime"] = estimateReadingTime(stats.wordCount);
    return result;
}
