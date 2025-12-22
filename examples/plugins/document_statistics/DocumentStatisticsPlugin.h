#pragma once

#include <QHash>
#include <QObject>
#include "plugin/IDocumentProcessorPlugin.h"
#include "plugin/PluginInterface.h"

/**
 * @brief Document statistics data
 */
struct DocumentStats {
    QString documentPath;
    int pageCount;
    qint64 fileSize;
    int wordCount;
    int characterCount;
    int imageCount;
    int linkCount;
    int annotationCount;
    QHash<int, int> wordsPerPage;
    QDateTime analyzedAt;
};

/**
 * @brief DocumentStatisticsPlugin - Document analysis and statistics plugin
 *
 * This plugin demonstrates:
 * - **Text Analysis**: Word count, character count, reading time
 * - **Structure Analysis**: Pages, images, links, annotations
 * - **Per-page Statistics**: Distribution of content
 * - **Dock Widget**: Statistics panel with charts
 * - **Export**: Statistics to JSON/CSV
 */
class DocumentStatisticsPlugin : public PluginBase,
                                 public IDocumentProcessorPlugin,
                                 public IUIExtension {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE
                          "document_statistics.json")
    Q_INTERFACES(IPluginInterface IDocumentProcessorPlugin IUIExtension)

public:
    explicit DocumentStatisticsPlugin(QObject* parent = nullptr);
    ~DocumentStatisticsPlugin() override;

    void handleMessage(const QString& from, const QVariant& message) override;

    // IUIExtension interface
    QList<QAction*> menuActions() const override;
    QList<QAction*> toolbarActions() const override;
    QList<QAction*> contextMenuActions() const override { return {}; }
    QString statusBarMessage() const override;
    QWidget* createDockWidget() override;
    QString menuPath() const override { return "View"; }
    QString toolbarId() const override { return "view_toolbar"; }

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

protected:
    bool onInitialize() override;
    void onShutdown() override;

private slots:
    void onShowStatistics();
    void onExportStatistics();

private:
    void registerHooks();
    void setupEventSubscriptions();
    DocumentStats analyzeDocument(const QString& filePath);
    int estimateReadingTime(int wordCount) const;

    QVariant onDocumentAnalyzed(const QVariantMap& context);

    // Statistics cache
    QHash<QString, DocumentStats> m_statsCache;
    QString m_currentDocument;

    // UI
    QList<QAction*> m_menuActions;
    QAction* m_showStatsAction;
    QAction* m_exportStatsAction;

    // Config
    int m_wordsPerMinute;
};
