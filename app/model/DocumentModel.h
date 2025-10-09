#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QFileDialog>
#include <QObject>
#include <QString>
#include <memory>
#include <vector>
#include "AsyncDocumentLoader.h"
#include "RenderModel.h"
#include "qtmetamacros.h"

// Forward declarations
class RecentFilesManager;

struct DocumentInfo {
    QString filePath;
    QString fileName;
    std::unique_ptr<Poppler::Document> document;

    DocumentInfo(const QString& path, std::unique_ptr<Poppler::Document> doc)
        : filePath(path), document(std::move(doc)) {
        fileName = QFileInfo(path).baseName();
    }
};

class DocumentModel : public QObject {
    Q_OBJECT

private:
    std::vector<std::unique_ptr<DocumentInfo>> documents;
    int currentDocumentIndex;

    // 异步加载器
    AsyncDocumentLoader* asyncLoader;

    // 多文档加载队列
    QStringList pendingFiles;

    // 从合并分支添加的成员
    QString currentFilePath;
    std::unique_ptr<Poppler::Document> document;
    RenderModel* renderModel;

private slots:
    void onDocumentLoaded(Poppler::Document* document, const QString& filePath);

public:
    DocumentModel();
    DocumentModel(RenderModel* _renderModel);
    ~DocumentModel() = default;

    // 多文档管理
    bool openFromFile(const QString& filePath);
    bool openFromFiles(const QStringList& filePaths);
    bool closeDocument(int index);
    bool closeCurrentDocument();
    void switchToDocument(int index);

    // 查询方法
    int getDocumentCount() const;
    int getCurrentDocumentIndex() const;
    QString getCurrentFilePath() const;
    QString getCurrentFileName() const;
    QString getDocumentFileName(int index) const;
    QString getDocumentFilePath(int index) const;
    Poppler::Document* getCurrentDocument() const;
    Poppler::Document* getDocument(int index) const;
    bool isEmpty() const;
    bool isValidIndex(int index) const;
    bool isNULL();

    // 最近文件管理器设置
    void setRecentFilesManager(RecentFilesManager* manager);

signals:
    void documentOpened(int index, const QString& fileName);
    void documentClosed(int index);
    void currentDocumentChanged(int index);
    void allDocumentsClosed();

    // 异步加载相关信号
    void loadingProgressChanged(int progress);
    void loadingMessageChanged(const QString& message);
    void loadingStarted(const QString& filePath);
    void loadingFailed(const QString& error, const QString& filePath);

    // 从合并分支添加的信号
    void renderPageDone(QImage image);
    void pageUpdate(int currentPage, int totalPages);
};
