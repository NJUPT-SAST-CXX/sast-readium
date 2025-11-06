#ifndef DOCUMENTADAPTER_H
#define DOCUMENTADAPTER_H

#include <QObject>
#include <QString>
#include <memory>

// Forward declarations
class DocumentController;
class PDFViewerPage;

namespace Poppler {
class Document;
}

/**
 * @brief DocumentAdapter - 文档控制器适配器
 *
 * 桥接 ElaWidgetTools UI 和现有的 DocumentController
 * 负责：
 * - 文档打开/关闭
 * - 文档保存
 * - 文档打印
 * - 文档导出
 * - 文档属性
 */
class DocumentAdapter : public QObject {
    Q_OBJECT

public:
    explicit DocumentAdapter(QObject* parent = nullptr);
    ~DocumentAdapter() override;

    void setDocumentController(DocumentController* controller);
    void setPDFViewerPage(PDFViewerPage* page);

public slots:
    void openFile(const QString& filePath);
    void closeDocument();
    void saveDocumentCopy(const QString& filePath);
    void printDocument();
    void exportDocument(const QString& filePath, const QString& format);
    void showDocumentProperties();

signals:
    void documentOpened(std::shared_ptr<Poppler::Document> document,
                        const QString& filePath);
    void documentClosed();
    void documentSaved(const QString& filePath);
    void errorOccurred(const QString& error);

private:
    DocumentController* m_documentController;
    PDFViewerPage* m_pdfViewerPage;

    void connectControllerSignals();
};

#endif  // DOCUMENTADAPTER_H
