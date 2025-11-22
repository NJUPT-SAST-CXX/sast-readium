#include "MetadataExtractorPlugin.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include "plugin/PluginHookRegistry.h"

MetadataExtractorPlugin::MetadataExtractorPlugin(QObject* parent)
    : PluginBase(parent), m_documentsProcessed(0) {
    // Set plugin metadata
    m_metadata.name = "Metadata Extractor";
    m_metadata.version = "1.0.0";
    m_metadata.description =
        "Extracts and analyzes document metadata from PDF files";
    m_metadata.author = "SAST Readium Team";
    m_metadata.dependencies = QStringList();

    // Declare plugin capabilities
    m_capabilities.provides = QStringList()
                              << "document.processor" << "document.metadata"
                              << "export.json" << "export.xml";
}

MetadataExtractorPlugin::~MetadataExtractorPlugin() {
    // Cleanup handled in onShutdown()
}

bool MetadataExtractorPlugin::onInitialize() {
    m_logger.info("MetadataExtractorPlugin: Initializing...");

    // Register with hook registry
    registerHooks();

    // Subscribe to events
    eventBus()->subscribe("document.opened", this, [this](Event* event) {
        QString filePath = event->data().toString();
        m_logger.info("Document opened: {}", filePath.toStdString());

        // Extract metadata asynchronously
        QJsonObject metadata = extractMetadata(filePath);
        m_metadataCache[filePath] = metadata;

        // Emit event with extracted metadata
        Event* metadataEvent = new Event("document.metadata_extracted");
        QVariantMap data;
        data["filePath"] = filePath;
        data["metadata"] = metadata;
        metadataEvent->setData(QVariant::fromValue(data));
        eventBus()->publish(metadataEvent);
    });

    m_logger.info("MetadataExtractorPlugin: Initialized successfully");
    return true;
}

void MetadataExtractorPlugin::onShutdown() {
    m_logger.info("MetadataExtractorPlugin: Shutting down...");

    // Unsubscribe from events
    eventBus()->unsubscribeAll(this);

    // Unregister hooks
    unregisterHooks();

    // Clear cache
    m_metadataCache.clear();

    m_logger.info("MetadataExtractorPlugin: Processed {} document(s) total",
                  m_documentsProcessed);
}

void MetadataExtractorPlugin::registerHooks() {
    auto& hookRegistry = PluginHookRegistry::instance();

    // Register callback for post-document-load hook
    hookRegistry.registerCallback(StandardHooks::DOCUMENT_POST_LOAD, name(),
                                  [this](const QVariantMap& context) {
                                      return onPostDocumentLoad(context);
                                  });

    // Register callback for metadata-extracted hook
    hookRegistry.registerCallback(StandardHooks::DOCUMENT_METADATA_EXTRACTED,
                                  name(), [this](const QVariantMap& context) {
                                      return onMetadataExtracted(context);
                                  });

    m_logger.debug("MetadataExtractorPlugin: Registered hook callbacks");
}

void MetadataExtractorPlugin::unregisterHooks() {
    auto& hookRegistry = PluginHookRegistry::instance();
    hookRegistry.unregisterAllCallbacks(name());
    m_logger.debug("MetadataExtractorPlugin: Unregistered hook callbacks");
}

QList<PluginWorkflowStage> MetadataExtractorPlugin::handledStages() const {
    return QList<PluginWorkflowStage>() << PluginWorkflowStage::PostDocumentLoad
                                        << PluginWorkflowStage::PreExport;
}

DocumentProcessingResult MetadataExtractorPlugin::processDocument(
    PluginWorkflowStage stage, const QString& filePath,
    const QJsonObject& context) {
    m_logger.debug("MetadataExtractorPlugin: Processing document at stage {}",
                   static_cast<int>(stage));

    if (stage == PluginWorkflowStage::PostDocumentLoad) {
        // Extract metadata
        QJsonObject metadata = extractMetadata(filePath);
        m_metadataCache[filePath] = metadata;

        m_documentsProcessed++;

        return DocumentProcessingResult::createSuccess(
            "Metadata extracted successfully", QVariant::fromValue(metadata));
    }

    if (stage == PluginWorkflowStage::PreExport) {
        // Prepare metadata for export
        if (m_metadataCache.contains(filePath)) {
            return DocumentProcessingResult::createSuccess(
                "Metadata ready for export",
                QVariant::fromValue(m_metadataCache[filePath]));
        }
    }

    return DocumentProcessingResult::createSuccess();
}

bool MetadataExtractorPlugin::canProcessFile(const QString& filePath) const {
    QFileInfo fileInfo(filePath);
    return supportedExtensions().contains("." + fileInfo.suffix().toLower());
}

QStringList MetadataExtractorPlugin::supportedExtensions() const {
    return QStringList() << ".pdf";
}

QJsonObject MetadataExtractorPlugin::extractMetadata(const QString& filePath) {
    m_logger.debug("MetadataExtractorPlugin: Extracting metadata from '{}'",
                   filePath.toStdString());

    QJsonObject metadata;
    QFileInfo fileInfo(filePath);

    // Basic file metadata
    metadata["fileName"] = fileInfo.fileName();
    metadata["filePath"] = filePath;
    metadata["fileSize"] = fileInfo.size();
    metadata["created"] = fileInfo.birthTime().toString(Qt::ISODate);
    metadata["modified"] = fileInfo.lastModified().toString(Qt::ISODate);
    metadata["suffix"] = fileInfo.suffix();

    // TODO: Extract PDF-specific metadata using Poppler
    // For this example, we'll add placeholder data
    metadata["title"] = fileInfo.baseName();
    metadata["pageCount"] = 0;  // Would be extracted from PDF
    metadata["author"] = QString();
    metadata["subject"] = QString();
    metadata["keywords"] = QString();
    metadata["producer"] = QString();
    metadata["creator"] = QString();

    // Custom metadata
    metadata["extractedBy"] = name();
    metadata["extractedAt"] =
        QDateTime::currentDateTime().toString(Qt::ISODate);
    metadata["extractorVersion"] = version();

    return metadata;
}

DocumentProcessingResult MetadataExtractorPlugin::exportDocument(
    const QString& sourcePath, const QString& targetPath, const QString& format,
    const QJsonObject& options) {
    m_logger.info(
        "MetadataExtractorPlugin: Exporting metadata from '{}' to '{}'",
        sourcePath.toStdString(), targetPath.toStdString());

    // Get metadata from cache or extract fresh
    QJsonObject metadata;
    if (m_metadataCache.contains(sourcePath)) {
        metadata = m_metadataCache[sourcePath];
    } else {
        metadata = extractMetadata(sourcePath);
    }

    // Export based on format
    if (format == "json") {
        QFile file(targetPath);
        if (!file.open(QIODevice::WriteOnly)) {
            return DocumentProcessingResult::createFailure(
                "Failed to open output file", QStringList()
                                                  << file.errorString());
        }

        QJsonDocument doc(metadata);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();

        return DocumentProcessingResult::createSuccess(
            "Metadata exported to JSON", QVariant::fromValue(metadata));
    }

    if (format == "xml") {
        // TODO: Implement XML export
        return DocumentProcessingResult::createFailure(
            "XML export not yet implemented");
    }

    return DocumentProcessingResult::createFailure(
        QString("Unsupported export format: %1").arg(format));
}

QVariant MetadataExtractorPlugin::onPostDocumentLoad(
    const QVariantMap& context) {
    QString filePath = context.value("filePath").toString();

    m_logger.debug("MetadataExtractorPlugin: Post-load hook for '{}'",
                   filePath.toStdString());

    // Process the document
    DocumentProcessingResult result = processDocument(
        PluginWorkflowStage::PostDocumentLoad, filePath, QJsonObject());

    QVariantMap resultMap;
    resultMap["success"] = result.success;
    resultMap["message"] = result.message;
    resultMap["metadata"] = result.data;

    return QVariant::fromValue(resultMap);
}

QVariant MetadataExtractorPlugin::onMetadataExtracted(
    const QVariantMap& context) {
    QString filePath = context.value("filePath").toString();
    QJsonObject metadata = context.value("metadata").toJsonObject();

    m_logger.debug("MetadataExtractorPlugin: Metadata extracted for '{}'",
                   filePath.toStdString());

    // Log extracted metadata
    m_logger.info("  Title: {}", metadata["title"].toString().toStdString());
    m_logger.info("  Author: {}", metadata["author"].toString().toStdString());
    m_logger.info("  Pages: {}", metadata["pageCount"].toInt());

    return QVariant();
}
