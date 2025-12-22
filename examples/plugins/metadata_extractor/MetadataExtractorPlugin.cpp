#include "MetadataExtractorPlugin.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>
#include <QXmlStreamWriter>
#include "controller/EventBus.h"
#include "plugin/PluginHookRegistry.h"

MetadataExtractorPlugin::MetadataExtractorPlugin(QObject* parent)
    : PluginBase(parent),
      m_documentsProcessed(0),
      m_exportCount(0),
      m_totalBytesProcessed(0) {
    // Set plugin metadata
    m_metadata.name = "Metadata Extractor";
    m_metadata.version = "2.0.0";
    m_metadata.description =
        "Extracts and analyzes document metadata from PDF files with "
        "configuration-driven extraction and multiple export formats";
    m_metadata.author = "SAST Readium Team";
    m_metadata.dependencies = QStringList();

    // Declare plugin capabilities
    m_capabilities.provides = QStringList()
                              << "document.processor" << "document.metadata"
                              << "export.json" << "export.xml" << "export.csv";
}

MetadataExtractorPlugin::~MetadataExtractorPlugin() {
    // Cleanup handled in onShutdown()
}

bool MetadataExtractorPlugin::onInitialize() {
    m_logger.info("MetadataExtractorPlugin: Initializing...");

    // Register with hook registry
    registerHooks();

    // Setup event subscriptions
    setupEventSubscriptions();

    // Log configuration
    QStringList fields = getConfiguredExtractFields();
    m_logger.info("MetadataExtractorPlugin: Configured to extract {} fields",
                  fields.size());

    m_logger.info("MetadataExtractorPlugin: Initialized successfully");
    return true;
}

void MetadataExtractorPlugin::onShutdown() {
    m_logger.info("MetadataExtractorPlugin: Shutting down...");

    // Remove event subscriptions
    removeEventSubscriptions();

    // Unregister hooks
    unregisterHooks();

    // Clear cache
    m_metadataCache.clear();

    m_logger.info(
        "MetadataExtractorPlugin: Statistics - Docs: {}, Exports: {}, "
        "Bytes: {}",
        m_documentsProcessed, m_exportCount, m_totalBytesProcessed);
}

// ============================================================================
// Inter-plugin Communication
// ============================================================================

void MetadataExtractorPlugin::handleMessage(const QString& from,
                                            const QVariant& message) {
    m_logger.info("MetadataExtractorPlugin: Received message from '{}'",
                  from.toStdString());

    QVariantMap msgMap = message.toMap();
    QString action = msgMap.value("action").toString();

    if (action == "get_metadata") {
        // Another plugin requested metadata for a document
        QString filePath = msgMap.value("filePath").toString();
        QJsonObject metadata;

        if (m_metadataCache.contains(filePath)) {
            metadata = m_metadataCache[filePath];
        } else if (!filePath.isEmpty()) {
            metadata = extractMetadata(filePath);
        }

        // Send response via EventBus
        Event* responseEvent = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["action"] = "metadata_response";
        data["metadata"] = QJsonDocument(metadata).toVariant();
        responseEvent->setData(QVariant::fromValue(data));
        eventBus()->publish(responseEvent);

    } else if (action == "export_metadata") {
        // Export metadata to specified format
        QString filePath = msgMap.value("filePath").toString();
        QString targetPath = msgMap.value("targetPath").toString();
        QString format = msgMap.value("format").toString("json");

        auto result =
            exportDocument(filePath, targetPath, format, QJsonObject());

        Event* responseEvent = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["action"] = "export_response";
        data["success"] = result.success;
        data["message"] = result.message;
        responseEvent->setData(QVariant::fromValue(data));
        eventBus()->publish(responseEvent);

    } else if (action == "get_statistics") {
        // Return plugin statistics
        Event* responseEvent = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["action"] = "statistics_response";
        data["documentsProcessed"] = m_documentsProcessed;
        data["exportCount"] = m_exportCount;
        data["totalBytesProcessed"] = m_totalBytesProcessed;
        data["cacheSize"] = m_metadataCache.size();
        responseEvent->setData(QVariant::fromValue(data));
        eventBus()->publish(responseEvent);
    }
}

// ============================================================================
// Event Subscriptions
// ============================================================================

void MetadataExtractorPlugin::setupEventSubscriptions() {
    m_logger.debug("MetadataExtractorPlugin: Setting up event subscriptions");

    eventBus()->subscribe("document.opened", this, [this](Event* event) {
        QString filePath = event->data().toString();
        m_logger.info("Document opened: {}", filePath.toStdString());

        // Check if auto-extract is enabled
        bool autoExtract = m_configuration.value("autoExtract").toBool(true);
        if (!autoExtract) {
            m_logger.debug("MetadataExtractorPlugin: Auto-extract disabled");
            return;
        }

        // Extract metadata
        QJsonObject metadata = extractMetadata(filePath);
        m_metadataCache[filePath] = metadata;

        // Emit event with extracted metadata
        Event* metadataEvent = new Event("document.metadata_extracted");
        QVariantMap data;
        data["filePath"] = filePath;
        data["metadata"] = QJsonDocument(metadata).toVariant();
        metadataEvent->setData(QVariant::fromValue(data));
        eventBus()->publish(metadataEvent);
    });

    eventBus()->subscribe("document.closed", this, [this](Event* event) {
        QString filePath = event->data().toString();

        // Check if we should clear cache on close
        bool cacheMetadata =
            m_configuration.value("cacheMetadata").toBool(true);
        if (!cacheMetadata) {
            m_metadataCache.remove(filePath);
            m_logger.debug("MetadataExtractorPlugin: Cleared cache for '{}'",
                           filePath.toStdString());
        }
    });

    m_logger.debug("MetadataExtractorPlugin: Event subscriptions set up");
}

void MetadataExtractorPlugin::removeEventSubscriptions() {
    m_logger.debug("MetadataExtractorPlugin: Removing event subscriptions");
    eventBus()->unsubscribeAll(this);
    m_logger.debug("MetadataExtractorPlugin: Event subscriptions removed");
}

// ============================================================================
// Hook Registration
// ============================================================================

void MetadataExtractorPlugin::registerHooks() {
    auto& hookRegistry = PluginHookRegistry::instance();

    // Register pre-load hook for validation
    hookRegistry.registerCallback(StandardHooks::DOCUMENT_PRE_LOAD, name(),
                                  [this](const QVariantMap& context) {
                                      return onDocumentPreLoad(context);
                                  });

    // Register post-load hook for metadata extraction
    hookRegistry.registerCallback(StandardHooks::DOCUMENT_POST_LOAD, name(),
                                  [this](const QVariantMap& context) {
                                      return onPostDocumentLoad(context);
                                  });

    // Register metadata-extracted hook
    hookRegistry.registerCallback(StandardHooks::DOCUMENT_METADATA_EXTRACTED,
                                  name(), [this](const QVariantMap& context) {
                                      return onMetadataExtracted(context);
                                  });

    // Register export hooks
    hookRegistry.registerCallback(StandardHooks::EXPORT_PRE_EXECUTE, name(),
                                  [this](const QVariantMap& context) {
                                      return onExportPreExecute(context);
                                  });

    hookRegistry.registerCallback(StandardHooks::EXPORT_POST_EXECUTE, name(),
                                  [this](const QVariantMap& context) {
                                      return onExportPostExecute(context);
                                  });

    m_logger.debug("MetadataExtractorPlugin: Registered 5 hook callbacks");
}

void MetadataExtractorPlugin::unregisterHooks() {
    PluginHookRegistry::instance().unregisterAllCallbacks(name());
    m_logger.debug("MetadataExtractorPlugin: Unregistered hook callbacks");
}

QVariant MetadataExtractorPlugin::onDocumentPreLoad(
    const QVariantMap& context) {
    QString filePath = context.value("filePath").toString();
    m_logger.debug("MetadataExtractorPlugin: [HOOK] Pre-load for '{}'",
                   filePath.toStdString());

    // Validate file exists and is readable
    QFileInfo fileInfo(filePath);
    QVariantMap result;
    result["allow"] = fileInfo.exists() && fileInfo.isReadable();
    result["message"] = result["allow"].toBool()
                            ? "File validated by MetadataExtractor"
                            : "File not accessible";
    return result;
}

// ============================================================================
// IDocumentProcessorPlugin Implementation
// ============================================================================

QList<PluginWorkflowStage> MetadataExtractorPlugin::handledStages() const {
    return QList<PluginWorkflowStage>()
           << PluginWorkflowStage::PreDocumentLoad
           << PluginWorkflowStage::PostDocumentLoad
           << PluginWorkflowStage::PreExport << PluginWorkflowStage::PostExport;
}

DocumentProcessingResult MetadataExtractorPlugin::processDocument(
    PluginWorkflowStage stage, const QString& filePath,
    const QJsonObject& context) {
    m_logger.debug("MetadataExtractorPlugin: Processing document at stage {}",
                   static_cast<int>(stage));

    switch (stage) {
        case PluginWorkflowStage::PreDocumentLoad: {
            // Validate file
            QFileInfo fileInfo(filePath);
            if (!fileInfo.exists()) {
                return DocumentProcessingResult::createFailure(
                    "File does not exist", QStringList() << filePath);
            }
            return DocumentProcessingResult::createSuccess("File validated");
        }

        case PluginWorkflowStage::PostDocumentLoad: {
            // Extract metadata
            QJsonObject metadata = extractMetadata(filePath);
            m_metadataCache[filePath] = metadata;
            m_documentsProcessed++;

            // Update bytes processed
            QFileInfo fileInfo(filePath);
            m_totalBytesProcessed += fileInfo.size();

            return DocumentProcessingResult::createSuccess(
                "Metadata extracted successfully",
                QVariant::fromValue(metadata));
        }

        case PluginWorkflowStage::PreExport: {
            // Prepare metadata for export
            if (m_metadataCache.contains(filePath)) {
                return DocumentProcessingResult::createSuccess(
                    "Metadata ready for export",
                    QVariant::fromValue(m_metadataCache[filePath]));
            }
            // Extract if not cached
            QJsonObject metadata = extractMetadata(filePath);
            return DocumentProcessingResult::createSuccess(
                "Metadata extracted for export", QVariant::fromValue(metadata));
        }

        case PluginWorkflowStage::PostExport: {
            m_exportCount++;
            return DocumentProcessingResult::createSuccess("Export completed");
        }

        default:
            return DocumentProcessingResult::createSuccess();
    }
}

bool MetadataExtractorPlugin::canProcessFile(const QString& filePath) const {
    QFileInfo fileInfo(filePath);
    return supportedExtensions().contains("." + fileInfo.suffix().toLower());
}

QStringList MetadataExtractorPlugin::supportedExtensions() const {
    return QStringList() << ".pdf";
}

// ============================================================================
// Metadata Extraction
// ============================================================================

QJsonObject MetadataExtractorPlugin::extractMetadata(const QString& filePath) {
    m_logger.debug("MetadataExtractorPlugin: Extracting metadata from '{}'",
                   filePath.toStdString());

    QJsonObject metadata;
    QFileInfo fileInfo(filePath);
    QStringList enabledFields = getConfiguredExtractFields();

    // Basic file metadata (always included)
    if (isFieldEnabled("fileName"))
        metadata["fileName"] = fileInfo.fileName();
    if (isFieldEnabled("filePath"))
        metadata["filePath"] = filePath;
    if (isFieldEnabled("fileSize"))
        metadata["fileSize"] = fileInfo.size();

    // Date fields
    if (isFieldEnabled("dates")) {
        metadata["created"] = fileInfo.birthTime().toString(Qt::ISODate);
        metadata["modified"] = fileInfo.lastModified().toString(Qt::ISODate);
    }

    metadata["suffix"] = fileInfo.suffix();

    // PDF-specific metadata
    // Note: In a real implementation, use Poppler to extract these
    if (isFieldEnabled("title"))
        metadata["title"] = fileInfo.baseName();
    if (isFieldEnabled("pageCount"))
        metadata["pageCount"] = 0;  // Would be extracted from PDF via Poppler
    if (isFieldEnabled("author"))
        metadata["author"] = QString();
    if (isFieldEnabled("subject"))
        metadata["subject"] = QString();
    if (isFieldEnabled("keywords"))
        metadata["keywords"] = QString();
    if (isFieldEnabled("producer"))
        metadata["producer"] = QString();
    if (isFieldEnabled("creator"))
        metadata["creator"] = QString();

    // Extractor metadata
    metadata["extractedBy"] = name();
    metadata["extractedAt"] =
        QDateTime::currentDateTime().toString(Qt::ISODate);
    metadata["extractorVersion"] = version();
    metadata["fieldsExtracted"] = QJsonArray::fromStringList(enabledFields);

    return metadata;
}

// ============================================================================
// Export Implementation
// ============================================================================

DocumentProcessingResult MetadataExtractorPlugin::exportDocument(
    const QString& sourcePath, const QString& targetPath, const QString& format,
    const QJsonObject& options) {
    m_logger.info(
        "MetadataExtractorPlugin: Exporting metadata from '{}' to '{}' "
        "(format: {})",
        sourcePath.toStdString(), targetPath.toStdString(),
        format.toStdString());

    // Get metadata from cache or extract fresh
    QJsonObject metadata;
    if (m_metadataCache.contains(sourcePath)) {
        metadata = m_metadataCache[sourcePath];
    } else {
        metadata = extractMetadata(sourcePath);
    }

    // Export based on format
    bool success = false;
    if (format == "json") {
        success = exportToJson(metadata, targetPath);
    } else if (format == "xml") {
        success = exportToXml(metadata, targetPath);
    } else if (format == "csv") {
        success = exportToCsv(metadata, targetPath);
    } else {
        return DocumentProcessingResult::createFailure(
            QString("Unsupported export format: %1").arg(format));
    }

    if (success) {
        m_exportCount++;
        return DocumentProcessingResult::createSuccess(
            QString("Metadata exported to %1").arg(format.toUpper()),
            QVariant::fromValue(metadata));
    }

    return DocumentProcessingResult::createFailure(
        QString("Failed to export to %1").arg(format));
}

bool MetadataExtractorPlugin::exportToJson(const QJsonObject& metadata,
                                           const QString& targetPath) {
    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly)) {
        m_logger.error("Failed to open file for JSON export: {}",
                       file.errorString().toStdString());
        return false;
    }

    QJsonDocument doc(metadata);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool MetadataExtractorPlugin::exportToXml(const QJsonObject& metadata,
                                          const QString& targetPath) {
    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly)) {
        m_logger.error("Failed to open file for XML export: {}",
                       file.errorString().toStdString());
        return false;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("metadata");

    // Write each metadata field as XML element
    for (auto it = metadata.begin(); it != metadata.end(); ++it) {
        xml.writeStartElement(it.key());
        if (it.value().isString()) {
            xml.writeCharacters(it.value().toString());
        } else if (it.value().isDouble()) {
            xml.writeCharacters(QString::number(it.value().toDouble()));
        } else if (it.value().isArray()) {
            QJsonArray arr = it.value().toArray();
            for (const auto& item : arr) {
                xml.writeTextElement("item", item.toString());
            }
        }
        xml.writeEndElement();
    }

    xml.writeEndElement();  // metadata
    xml.writeEndDocument();
    file.close();
    return true;
}

bool MetadataExtractorPlugin::exportToCsv(const QJsonObject& metadata,
                                          const QString& targetPath) {
    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_logger.error("Failed to open file for CSV export: {}",
                       file.errorString().toStdString());
        return false;
    }

    QTextStream out(&file);

    // Write header
    QStringList keys = metadata.keys();
    out << keys.join(",") << "\n";

    // Write values
    QStringList values;
    for (const QString& key : keys) {
        QJsonValue val = metadata[key];
        if (val.isString()) {
            // Escape quotes and wrap in quotes
            QString str = val.toString();
            str.replace("\"", "\"\"");
            values << QString("\"%1\"").arg(str);
        } else if (val.isDouble()) {
            values << QString::number(val.toDouble());
        } else if (val.isArray()) {
            values << QString("\"%1\"").arg(
                QJsonDocument(val.toArray()).toJson(QJsonDocument::Compact));
        } else {
            values << "";
        }
    }
    out << values.join(",") << "\n";

    file.close();
    return true;
}

// ============================================================================
// Hook Callbacks
// ============================================================================

QVariant MetadataExtractorPlugin::onPostDocumentLoad(
    const QVariantMap& context) {
    QString filePath = context.value("filePath").toString();

    m_logger.debug("MetadataExtractorPlugin: [HOOK] Post-load for '{}'",
                   filePath.toStdString());

    // Process the document
    DocumentProcessingResult result = processDocument(
        PluginWorkflowStage::PostDocumentLoad, filePath, QJsonObject());

    QVariantMap resultMap;
    resultMap["success"] = result.success;
    resultMap["message"] = result.message;
    resultMap["metadata"] = result.data;
    resultMap["pluginName"] = name();

    return QVariant::fromValue(resultMap);
}

QVariant MetadataExtractorPlugin::onMetadataExtracted(
    const QVariantMap& context) {
    QString filePath = context.value("filePath").toString();

    m_logger.debug(
        "MetadataExtractorPlugin: [HOOK] Metadata extracted for '{}'",
        filePath.toStdString());

    // Log summary of extracted metadata
    if (m_metadataCache.contains(filePath)) {
        QJsonObject metadata = m_metadataCache[filePath];
        m_logger.info("  Title: {}",
                      metadata["title"].toString().toStdString());
        m_logger.info("  Size: {} bytes", metadata["fileSize"].toInt());
    }

    return QVariant();
}

QVariant MetadataExtractorPlugin::onExportPreExecute(
    const QVariantMap& context) {
    QString filePath = context.value("filePath").toString();
    QString format = context.value("format").toString();

    m_logger.debug("MetadataExtractorPlugin: [HOOK] Pre-export for '{}' ({})",
                   filePath.toStdString(), format.toStdString());

    // Ensure metadata is cached before export
    if (!m_metadataCache.contains(filePath)) {
        QJsonObject metadata = extractMetadata(filePath);
        m_metadataCache[filePath] = metadata;
    }

    QVariantMap result;
    result["ready"] = true;
    result["metadataCached"] = m_metadataCache.contains(filePath);
    return result;
}

QVariant MetadataExtractorPlugin::onExportPostExecute(
    const QVariantMap& context) {
    QString targetPath = context.value("targetPath").toString();
    bool success = context.value("success").toBool();

    m_logger.debug(
        "MetadataExtractorPlugin: [HOOK] Post-export to '{}' (success: {})",
        targetPath.toStdString(), success);

    if (success) {
        m_exportCount++;
    }

    QVariantMap result;
    result["acknowledged"] = true;
    result["totalExports"] = m_exportCount;
    return result;
}

// ============================================================================
// Configuration Helpers
// ============================================================================

QStringList MetadataExtractorPlugin::getConfiguredExtractFields() const {
    QStringList defaultFields = {
        "title",     "author",   "subject", "keywords", "producer", "creator",
        "pageCount", "fileSize", "dates",   "fileName", "filePath"};

    if (!m_configuration.contains("extractFields")) {
        return defaultFields;
    }

    QJsonArray fieldsArray = m_configuration["extractFields"].toArray();
    QStringList fields;
    for (const auto& field : fieldsArray) {
        fields << field.toString();
    }
    return fields.isEmpty() ? defaultFields : fields;
}

bool MetadataExtractorPlugin::isFieldEnabled(const QString& fieldName) const {
    QStringList enabledFields = getConfiguredExtractFields();
    return enabledFields.contains(fieldName);
}
