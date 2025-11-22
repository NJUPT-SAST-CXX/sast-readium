#pragma once

#include <QObject>
#include "plugin/PluginInterface.h"
#include "plugin/SpecializedPlugins.h"

/**
 * @brief MetadataExtractorPlugin - Example document processor plugin
 *
 * This plugin demonstrates the IDocumentProcessorPlugin interface by extracting
 * metadata from PDF documents and providing export functionality.
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

    QVariant onPostDocumentLoad(const QVariantMap& context);
    QVariant onMetadataExtracted(const QVariantMap& context);

    int m_documentsProcessed;
    QHash<QString, QJsonObject> m_metadataCache;
};
