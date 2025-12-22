#pragma once

#include <QObject>
#include <QXmlStreamWriter>
#include "plugin/IDocumentProcessorPlugin.h"
#include "plugin/PluginInterface.h"

/**
 * @brief MetadataExtractorPlugin - Example document processor plugin
 *
 * This plugin demonstrates the IDocumentProcessorPlugin interface by extracting
 * metadata from PDF documents and providing export functionality.
 *
 * Enhanced features:
 * - **Multiple Hook Registration**: Pre-load, post-load, pre-export,
 * post-export
 * - **Configuration-driven Extraction**: Select which fields to extract
 * - **Inter-plugin Communication**: Respond to metadata requests
 * - **Multiple Export Formats**: JSON, XML, CSV
 * - **Poppler Integration**: Real PDF metadata extraction
 */
class MetadataExtractorPlugin : public PluginBase,
                                public IDocumentProcessorPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE
                          "metadata_extractor.json")
    Q_INTERFACES(IPluginInterface IDocumentProcessorPlugin)

public:
    explicit MetadataExtractorPlugin(QObject* parent = nullptr);
    ~MetadataExtractorPlugin() override;

    // IPluginInterface override for inter-plugin communication
    void handleMessage(const QString& from, const QVariant& message) override;

protected:
    // PluginBase overrides
    bool onInitialize() override;
    void onShutdown() override;

    // IDocumentProcessorPlugin interface
    QList<PluginWorkflowStage> handledStages() const override;

    DocumentProcessingResult processDocument(
        PluginWorkflowStage stage, const QString& filePath,
        const QJsonObject& context) override;

    bool canProcessFile(const QString& filePath) const override;
    QStringList supportedExtensions() const override;
    QJsonObject extractMetadata(const QString& filePath) override;

    DocumentProcessingResult exportDocument(
        const QString& sourcePath, const QString& targetPath,
        const QString& format, const QJsonObject& options) override;

private:
    void registerHooks();
    void unregisterHooks();
    void setupEventSubscriptions();
    void removeEventSubscriptions();

    // Hook callbacks
    QVariant onDocumentPreLoad(const QVariantMap& context);
    QVariant onPostDocumentLoad(const QVariantMap& context);
    QVariant onMetadataExtracted(const QVariantMap& context);
    QVariant onExportPreExecute(const QVariantMap& context);
    QVariant onExportPostExecute(const QVariantMap& context);

    // Export helpers
    bool exportToJson(const QJsonObject& metadata, const QString& targetPath);
    bool exportToXml(const QJsonObject& metadata, const QString& targetPath);
    bool exportToCsv(const QJsonObject& metadata, const QString& targetPath);

    // Configuration helpers
    QStringList getConfiguredExtractFields() const;
    bool isFieldEnabled(const QString& fieldName) const;

    // Statistics
    int m_documentsProcessed;
    int m_exportCount;
    qint64 m_totalBytesProcessed;

    // Cache
    QHash<QString, QJsonObject> m_metadataCache;
};
