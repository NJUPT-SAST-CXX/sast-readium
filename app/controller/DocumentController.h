#pragma once

#include <QHash>
#include <QObject>
#include <QStandardPaths>
#include <QString>
#include <QWidget>
#include <functional>
#include "../managers/RecentFilesManager.h"
#include "../model/DocumentModel.h"
#include "tool.hpp"

// 前向声明
class QWidget;

class DocumentController : public QObject {
    Q_OBJECT

private:
    DocumentModel* documentModel;
    RecentFilesManager* recentFilesManager;
    QHash<ActionMap, std::function<void(QWidget*)>> commandMap;
    void initializeCommandMap();

public:
    DocumentController(DocumentModel* model);
    ~DocumentController() = default;
    void execute(ActionMap actionID, QWidget* context);

    // 多文档操作方法
    bool openDocument(const QString& filePath);
    bool openDocuments(const QStringList& filePaths);
    bool closeDocument(int index);
    bool closeCurrentDocument();
    void switchToDocument(int index);
    void showDocumentMetadata(QWidget* parent);
    void saveDocumentCopy(QWidget* parent);

    // 文件夹扫描功能
    QStringList scanFolderForPDFs(const QString& folderPath);

    // 最近文件管理
    void setRecentFilesManager(RecentFilesManager* manager);
    RecentFilesManager* getRecentFilesManager() const {
        return recentFilesManager;
    }

    // 获取文档模型
    DocumentModel* getDocumentModel() const { return documentModel; }

signals:
    void documentOperationCompleted(ActionMap action, bool success);
    void sideBarToggleRequested();
    void sideBarShowRequested();
    void sideBarHideRequested();
    void viewModeChangeRequested(int mode);  // 0=SinglePage, 1=ContinuousScroll
    void pdfActionRequested(ActionMap action);
    void themeToggleRequested();
};
