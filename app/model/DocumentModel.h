#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QFileDialog>
#include <QObject>
#include <QString>
#include <memory>
#include <vector>
#include "../utils/ErrorHandling.h"
#include "../utils/ErrorRecovery.h"
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
    void initializeErrorRecovery();

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
    ~DocumentModel() override;

    // 多文档管理
    virtual bool openFromFile(const QString& filePath);
    virtual bool openFromFiles(const QStringList& filePaths);
    virtual bool closeDocument(int index);
    virtual bool closeCurrentDocument();
    virtual void switchToDocument(int index);

    // 查询方法
    virtual int getDocumentCount() const;
    virtual int getCurrentDocumentIndex() const;
    virtual QString getCurrentFilePath() const;
    virtual QString getCurrentFileName() const;
    virtual QString getDocumentFileName(int index) const;
    virtual QString getDocumentFilePath(int index) const;
    virtual Poppler::Document* getCurrentDocument() const;
    virtual Poppler::Document* getDocument(int index) const;
    virtual bool isEmpty() const;
    virtual bool isValidIndex(int index) const;
    virtual bool isNULL();

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
