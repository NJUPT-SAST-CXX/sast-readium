#pragma once

#include <QHash>
#include <QObject>
#include "plugin/IAnnotationPlugin.h"
#include "plugin/PluginInterface.h"

/**
 * @brief AnnotationSyncPlugin - Example annotation plugin
 *
 * This plugin demonstrates the IAnnotationPlugin interface by providing:
 * - **Multi-format Export**: JSON, XFDF, XML annotation export
 * - **Import Support**: Load annotations from various formats
 * - **Cloud Sync Simulation**: Mock cloud synchronization
 * - **Custom Rendering**: Annotation rendering customization
 */
class AnnotationSyncPlugin : public PluginBase, public IAnnotationPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE
                          "annotation_sync.json")
    Q_INTERFACES(IPluginInterface IAnnotationPlugin)

public:
    explicit AnnotationSyncPlugin(QObject* parent = nullptr);
    ~AnnotationSyncPlugin() override;

    void handleMessage(const QString& from, const QVariant& message) override;

protected:
    bool onInitialize() override;
    void onShutdown() override;

    // IAnnotationPlugin interface
    QList<AnnotationType> supportedTypes() const override;
    bool createAnnotation(const AnnotationData& data,
                          const QString& documentPath) override;
    bool updateAnnotation(const QString& annotationId,
                          const AnnotationData& data,
                          const QString& documentPath) override;
    bool deleteAnnotation(const QString& annotationId,
                          const QString& documentPath) override;
    QList<AnnotationData> getAnnotationsForPage(
        int pageNumber, const QString& documentPath) const override;
    bool exportAnnotations(const QString& documentPath,
                           const QString& outputPath,
                           const QString& format) override;
    int importAnnotations(const QString& inputPath, const QString& documentPath,
                          const QString& format) override;
    void renderAnnotation(QPainter* painter, const AnnotationData& annotation,
                          const QRect& pageRect, double zoom) override;

private:
    void registerHooks();
    void unregisterHooks();

    QVariant onAnnotationCreated(const QVariantMap& context);
    QVariant onAnnotationUpdated(const QVariantMap& context);
    QVariant onAnnotationRender(const QVariantMap& context);

    // Export helpers
    bool exportToJson(const QList<AnnotationData>& annotations,
                      const QString& path);
    bool exportToXfdf(const QList<AnnotationData>& annotations,
                      const QString& path);

    // Import helpers
    QList<AnnotationData> importFromJson(const QString& path);

    // Simulated cloud sync
    bool syncToCloud(const QString& documentPath);
    bool syncFromCloud(const QString& documentPath);

    // Storage: documentPath -> list of annotations
    QHash<QString, QList<AnnotationData>> m_annotations;

    // Statistics
    int m_annotationsCreated;
    int m_annotationsExported;
    int m_syncOperations;

    // Configuration
    bool m_autoSync;
    QString m_cloudEndpoint;
};
