#include "ExportConverterPlugin.h"
#include <QAction>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>
#include "controller/EventBus.h"
#include "plugin/PluginHookRegistry.h"

ExportConverterPlugin::ExportConverterPlugin(QObject* parent)
    : PluginBase(parent), m_exportsCompleted(0) {
    m_metadata.name = "Export Converter";
    m_metadata.version = "1.0.0";
    m_metadata.description =
        "Multi-format document export (TXT, HTML, Markdown)";
    m_metadata.author = "SAST Readium Team";
    m_metadata.dependencies = QStringList();
    m_capabilities.provides = QStringList()
                              << "export.text" << "export.html"
                              << "export.markdown" << "document.processor";
}

ExportConverterPlugin::~ExportConverterPlugin() { qDeleteAll(m_menuActions); }

bool ExportConverterPlugin::onInitialize() {
    m_logger.info("ExportConverterPlugin: Initializing...");

    // Create export actions
    QAction* textAction = new QAction("Plain Text (.txt)", this);
    connect(textAction, &QAction::triggered, this,
            &ExportConverterPlugin::onExportToText);
    m_menuActions.append(textAction);

    QAction* htmlAction = new QAction("HTML Document (.html)", this);
    connect(htmlAction, &QAction::triggered, this,
            &ExportConverterPlugin::onExportToHtml);
    m_menuActions.append(htmlAction);

    QAction* mdAction = new QAction("Markdown (.md)", this);
    connect(mdAction, &QAction::triggered, this,
            &ExportConverterPlugin::onExportToMarkdown);
    m_menuActions.append(mdAction);

    registerHooks();

    eventBus()->subscribe("document.opened", this, [this](Event* event) {
        m_currentDocument = event->data().toString();
    });

    m_logger.info("ExportConverterPlugin: Initialized successfully");
    return true;
}

void ExportConverterPlugin::onShutdown() {
    m_logger.info("ExportConverterPlugin: Shutting down...");
    PluginHookRegistry::instance().unregisterAllCallbacks(name());
    eventBus()->unsubscribeAll(this);
    m_logger.info(QString("ExportConverterPlugin: Exports completed: %1")
                      .arg(m_exportsCompleted));
}

void ExportConverterPlugin::handleMessage(const QString& from,
                                          const QVariant& message) {
    QVariantMap msgMap = message.toMap();
    QString action = msgMap.value("action").toString();

    if (action == "export") {
        QString sourcePath = msgMap.value("sourcePath").toString();
        QString targetPath = msgMap.value("targetPath").toString();
        QString format = msgMap.value("format").toString();
        QJsonObject options =
            QJsonObject::fromVariantMap(msgMap.value("options").toMap());

        auto result = exportDocument(sourcePath, targetPath, format, options);

        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["success"] = result.success;
        data["message"] = result.message;
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);

    } else if (action == "get_formats") {
        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["formats"] = QStringList() << "txt" << "html" << "markdown";
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);
    }
}

// ============================================================================
// IUIExtension
// ============================================================================

QList<QAction*> ExportConverterPlugin::menuActions() const {
    return m_menuActions;
}

// ============================================================================
// IDocumentProcessorPlugin
// ============================================================================

QList<PluginWorkflowStage> ExportConverterPlugin::handledStages() const {
    return QList<PluginWorkflowStage>() << PluginWorkflowStage::PreExport;
}

DocumentProcessingResult ExportConverterPlugin::processDocument(
    PluginWorkflowStage stage, const QString& filePath,
    const QJsonObject& context) {
    Q_UNUSED(context)
    if (stage == PluginWorkflowStage::PreExport) {
        return DocumentProcessingResult::createSuccess(
            "Export converter ready");
    }
    return DocumentProcessingResult::createSuccess();
}

bool ExportConverterPlugin::canProcessFile(const QString& filePath) const {
    QFileInfo info(filePath);
    return supportedExtensions().contains("." + info.suffix().toLower());
}

QStringList ExportConverterPlugin::supportedExtensions() const {
    return QStringList() << ".pdf";
}

QJsonObject ExportConverterPlugin::extractMetadata(const QString& filePath) {
    Q_UNUSED(filePath)
    QJsonObject metadata;
    metadata["supportedExportFormats"] = QJsonArray::fromStringList(
        QStringList() << "txt" << "html" << "markdown");
    return metadata;
}

DocumentProcessingResult ExportConverterPlugin::exportDocument(
    const QString& sourcePath, const QString& targetPath, const QString& format,
    const QJsonObject& options) {
    m_logger.info(QString("ExportConverterPlugin: Exporting '%1' to '%2' as %3")
                      .arg(sourcePath)
                      .arg(targetPath)
                      .arg(format));

    bool success = false;
    if (format == "txt" || format == "text") {
        success = exportToText(sourcePath, targetPath, options);
    } else if (format == "html") {
        success = exportToHtml(sourcePath, targetPath, options);
    } else if (format == "markdown" || format == "md") {
        success = exportToMarkdown(sourcePath, targetPath, options);
    } else {
        return DocumentProcessingResult::createFailure(
            QString("Unsupported export format: %1").arg(format));
    }

    if (success) {
        m_exportsCompleted++;
        return DocumentProcessingResult::createSuccess(
            QString("Exported to %1").arg(format.toUpper()));
    }

    return DocumentProcessingResult::createFailure("Export failed");
}

// ============================================================================
// Export Implementations
// ============================================================================

bool ExportConverterPlugin::exportToText(const QString& sourcePath,
                                         const QString& targetPath,
                                         const QJsonObject& options) {
    Q_UNUSED(options)

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_logger.error(
            QString("Cannot open file for writing: %1").arg(targetPath));
        return false;
    }

    QTextStream out(&file);
    out << extractText(sourcePath);
    file.close();

    return true;
}

bool ExportConverterPlugin::exportToHtml(const QString& sourcePath,
                                         const QString& targetPath,
                                         const QJsonObject& options) {
    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QFileInfo info(sourcePath);
    QString title = options.value("title").toString(info.baseName());
    QString text = extractText(sourcePath);
    QString html = generateHtmlTemplate(title, text);

    QTextStream out(&file);
    out << html;
    file.close();

    return true;
}

bool ExportConverterPlugin::exportToMarkdown(const QString& sourcePath,
                                             const QString& targetPath,
                                             const QJsonObject& options) {
    Q_UNUSED(options)

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QFileInfo info(sourcePath);
    QString text = extractText(sourcePath);
    QString markdown =
        QString("# %1\n\n%2").arg(info.baseName()).arg(textToMarkdown(text));

    QTextStream out(&file);
    out << markdown;
    file.close();

    return true;
}

QString ExportConverterPlugin::extractText(const QString& sourcePath,
                                           int pageNumber) {
    Q_UNUSED(pageNumber)
    // Simulated text extraction - in real implementation, use Poppler
    QFileInfo info(sourcePath);
    return QString(
               "Document: %1\n\n"
               "This is simulated text content extracted from the PDF "
               "document.\n"
               "In a real implementation, Poppler would be used to extract "
               "actual text.\n\n"
               "Page 1:\nLorem ipsum dolor sit amet, consectetur adipiscing "
               "elit.\n\n"
               "Page 2:\nSed do eiusmod tempor incididunt ut labore et dolore "
               "magna aliqua.\n")
        .arg(info.fileName());
}

QString ExportConverterPlugin::generateHtmlTemplate(const QString& title,
                                                    const QString& content) {
    return QString(R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>%1</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            line-height: 1.6;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            color: #333;
        }
        h1 { color: #2c3e50; border-bottom: 2px solid #3498db; padding-bottom: 10px; }
        .content { white-space: pre-wrap; }
        .footer { margin-top: 40px; color: #666; font-size: 0.9em; }
    </style>
</head>
<body>
    <h1>%1</h1>
    <div class="content">%2</div>
    <div class="footer">
        Exported by SAST Readium Export Converter Plugin
    </div>
</body>
</html>)")
        .arg(title)
        .arg(content.toHtmlEscaped());
}

QString ExportConverterPlugin::textToMarkdown(const QString& text) {
    QString md = text;
    // Basic paragraph formatting
    md.replace("\n\n", "\n\n---\n\n");
    return md;
}

// ============================================================================
// Slots
// ============================================================================

void ExportConverterPlugin::onExportToText() {
    Event* event = new Event("ui.showExportDialog");
    QVariantMap data;
    data["format"] = "txt";
    data["filter"] = "Text Files (*.txt)";
    event->setData(QVariant::fromValue(data));
    eventBus()->publish(event);
}

void ExportConverterPlugin::onExportToHtml() {
    Event* event = new Event("ui.showExportDialog");
    QVariantMap data;
    data["format"] = "html";
    data["filter"] = "HTML Files (*.html)";
    event->setData(QVariant::fromValue(data));
    eventBus()->publish(event);
}

void ExportConverterPlugin::onExportToMarkdown() {
    Event* event = new Event("ui.showExportDialog");
    QVariantMap data;
    data["format"] = "markdown";
    data["filter"] = "Markdown Files (*.md)";
    event->setData(QVariant::fromValue(data));
    eventBus()->publish(event);
}

// ============================================================================
// Hooks
// ============================================================================

void ExportConverterPlugin::registerHooks() {
    auto& registry = PluginHookRegistry::instance();
    registry.registerCallback(
        StandardHooks::EXPORT_PRE_EXECUTE, name(),
        [this](const QVariantMap& ctx) { return onExportPreExecute(ctx); });
    registry.registerCallback(
        StandardHooks::EXPORT_POST_EXECUTE, name(),
        [this](const QVariantMap& ctx) { return onExportPostExecute(ctx); });
}

QVariant ExportConverterPlugin::onExportPreExecute(const QVariantMap& context) {
    QString format = context.value("format").toString();
    m_logger.debug(
        QString("ExportConverterPlugin: [HOOK] Pre-export for format '%1'")
            .arg(format));

    QVariantMap result;
    result["canHandle"] =
        (format == "txt" || format == "html" || format == "markdown");
    return result;
}

QVariant ExportConverterPlugin::onExportPostExecute(
    const QVariantMap& context) {
    bool success = context.value("success").toBool();
    if (success) {
        m_exportsCompleted++;
    }

    QVariantMap result;
    result["acknowledged"] = true;
    result["totalExports"] = m_exportsCompleted;
    return result;
}
