#pragma once

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>

/**
 * @brief Plugin Workflow Stages
 *
 * Defines the stages in the document workflow where plugins can hook in.
 */
enum class PluginWorkflowStage {
    // Document loading workflow
    PreDocumentLoad,    // Before document is loaded
    PostDocumentLoad,   // After document is loaded
    PreDocumentClose,   // Before document is closed
    PostDocumentClose,  // After document is closed

    // Rendering workflow
    PrePageRender,   // Before page is rendered
    PostPageRender,  // After page is rendered

    // Search workflow
    PreSearch,   // Before search is executed
    PostSearch,  // After search is executed

    // Cache workflow
    PreCache,   // Before item is cached
    PostCache,  // After item is cached

    // Export workflow
    PreExport,  // Before document is exported
    PostExport  // After document is exported
};

/**
 * @brief Document Processing Result
 *
 * Contains the result of a document processing operation.
 */
struct DocumentProcessingResult {
    bool success;
    QString message;
    QVariant data;
    QStringList warnings;
    QStringList errors;

    DocumentProcessingResult() : success(false) {}

    static DocumentProcessingResult createSuccess(
        const QString& msg = QString(), const QVariant& d = QVariant()) {
        DocumentProcessingResult result;
        result.success = true;
        result.message = msg;
        result.data = d;
        return result;
    }

    static DocumentProcessingResult createFailure(
        const QString& msg, const QStringList& errs = QStringList()) {
        DocumentProcessingResult result;
        result.success = false;
        result.message = msg;
        result.errors = errs;
        return result;
    }
};

/**
 * @brief IDocumentProcessorPlugin - Interface for document processing plugins
 *
 * Plugins implementing this interface can transform, analyze, and export
 * documents. They hook into the document workflow at various stages.
 */
class IDocumentProcessorPlugin {
public:
    virtual ~IDocumentProcessorPlugin() = default;

    /**
     * @brief Get the workflow stages this plugin handles
     */
    virtual QList<PluginWorkflowStage> handledStages() const = 0;

    /**
     * @brief Process document at a specific workflow stage
     * @param stage The workflow stage
     * @param filePath The document file path
     * @param context Additional context data
     * @return Processing result
     */
    virtual DocumentProcessingResult processDocument(
        PluginWorkflowStage stage, const QString& filePath,
        const QJsonObject& context) = 0;

    /**
     * @brief Check if plugin can process a file type
     * @param filePath The file path to check
     * @return True if plugin can process this file
     */
    virtual bool canProcessFile(const QString& filePath) const = 0;

    /**
     * @brief Get supported file extensions
     * @return List of file extensions (e.g., ".pdf", ".epub")
     */
    virtual QStringList supportedExtensions() const = 0;

    /**
     * @brief Extract metadata from document
     * @param filePath The document file path
     * @return Metadata as JSON object
     */
    virtual QJsonObject extractMetadata(const QString& filePath) = 0;

    /**
     * @brief Export document to a specific format
     * @param sourcePath Source document path
     * @param targetPath Target file path
     * @param format Export format (e.g., "pdf", "html", "text")
     * @param options Export options
     * @return Export result
     */
    virtual DocumentProcessingResult exportDocument(
        const QString& sourcePath, const QString& targetPath,
        const QString& format, const QJsonObject& options) = 0;
};

Q_DECLARE_INTERFACE(IDocumentProcessorPlugin,
                    "com.sast.readium.IDocumentProcessorPlugin/1.0")
