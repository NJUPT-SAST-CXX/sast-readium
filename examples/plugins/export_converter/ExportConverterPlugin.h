#pragma once

#include <QObject>
#include "plugin/IDocumentProcessorPlugin.h"
#include "plugin/PluginInterface.h"

/**
 * @brief ExportConverterPlugin - Multi-format document export plugin
 *
 * This plugin demonstrates:
 * - **Text Export**: Plain text extraction
 * - **HTML Export**: Formatted HTML with styling
 * - **Markdown Export**: Markdown formatted output
 * - **Batch Export**: Export multiple pages/documents
 * - **Template Support**: Customizable export templates
 */
class ExportConverterPlugin : public PluginBase,
                              public IDocumentProcessorPlugin,
                              public IUIExtension {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE
                          "export_converter.json")
    Q_INTERFACES(IPluginInterface IDocumentProcessorPlugin IUIExtension)

public:
    explicit ExportConverterPlugin(QObject* parent = nullptr);
    ~ExportConverterPlugin() override;

    void handleMessage(const QString& from, const QVariant& message) override;

    // IUIExtension
    QList<QAction*> menuActions() const override;
    QList<QAction*> toolbarActions() const override { return {}; }
    QList<QAction*> contextMenuActions() const override { return {}; }
    QString statusBarMessage() const override { return QString(); }
    QWidget* createDockWidget() override { return nullptr; }
    QString menuPath() const override { return "File/Export As"; }
    QString toolbarId() const override { return QString(); }

    // IDocumentProcessorPlugin
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
    void onExportToText();
    void onExportToHtml();
    void onExportToMarkdown();

private:
    void registerHooks();

    // Export implementations
    bool exportToText(const QString& sourcePath, const QString& targetPath,
                      const QJsonObject& options);
    bool exportToHtml(const QString& sourcePath, const QString& targetPath,
                      const QJsonObject& options);
    bool exportToMarkdown(const QString& sourcePath, const QString& targetPath,
                          const QJsonObject& options);

    // Text extraction (simulated)
    QString extractText(const QString& sourcePath, int pageNumber = -1);
    QString generateHtmlTemplate(const QString& title, const QString& content);
    QString textToMarkdown(const QString& text);

    QVariant onExportPreExecute(const QVariantMap& context);
    QVariant onExportPostExecute(const QVariantMap& context);

    // UI
    QList<QAction*> m_menuActions;
    QString m_currentDocument;

    // Statistics
    int m_exportsCompleted;
};
