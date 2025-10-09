#include "DocumentController.h"
#include <poppler/qt6/poppler-qt6.h>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStringList>
#include "../ui/dialogs/DocumentMetadataDialog.h"
#include "utils/LoggingMacros.h"

void DocumentController::initializeCommandMap() {
    commandMap = {
        {ActionMap::openFile,
         [this](QWidget* ctx) {
             QStringList filePaths = QFileDialog::getOpenFileNames(
                 ctx, tr("Open PDF Files"),
                 QStandardPaths::writableLocation(
                     QStandardPaths::DocumentsLocation),
                 tr("PDF Files (*.pdf)"));
             if (!filePaths.isEmpty()) {
                 bool success = openDocuments(filePaths);
                 emit documentOperationCompleted(ActionMap::openFile, success);
             }
         }},
        {ActionMap::openFolder,
         [this](QWidget* ctx) {
             QString folderPath = QFileDialog::getExistingDirectory(
                 ctx, tr("Open Folder"),
                 QStandardPaths::writableLocation(
                     QStandardPaths::DocumentsLocation),
                 QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
             if (!folderPath.isEmpty()) {
                 QStringList pdfFiles = scanFolderForPDFs(folderPath);
                 if (!pdfFiles.isEmpty()) {
                     bool success = openDocuments(pdfFiles);
                     emit documentOperationCompleted(ActionMap::openFolder,
                                                     success);
                 } else {
                     emit documentOperationCompleted(ActionMap::openFolder,
                                                     false);
                 }
             }
         }},
        {ActionMap::save, [this](QWidget* ctx) { /*....save()....*/ }},
        {ActionMap::saveAs, [this](QWidget* ctx) { saveDocumentCopy(ctx); }},
        {ActionMap::newTab,
         [this](QWidget* ctx) {
             QString filePath = QFileDialog::getOpenFileName(
                 ctx, tr("Open PDF in New Tab"),
                 QStandardPaths::writableLocation(
                     QStandardPaths::DocumentsLocation),
                 tr("PDF Files (*.pdf)"));
             if (!filePath.isEmpty()) {
                 bool success = openDocument(filePath);
                 emit documentOperationCompleted(ActionMap::newTab, success);
             }
         }},
        {ActionMap::closeTab,
         [this](QWidget* ctx) {
             // 这里需要从上下文获取要关闭的标签页索引
             // 暂时关闭当前文档
             bool success = closeCurrentDocument();
             emit documentOperationCompleted(ActionMap::closeTab, success);
         }},
        {ActionMap::closeCurrentTab,
         [this](QWidget* ctx) {
             bool success = closeCurrentDocument();
             emit documentOperationCompleted(ActionMap::closeCurrentTab,
                                             success);
         }},
        {ActionMap::closeAllTabs,
         [this](QWidget* ctx) {
             bool success = true;
             while (!documentModel->isEmpty()) {
                 if (!closeDocument(0)) {
                     success = false;
                     break;
                 }
             }
             emit documentOperationCompleted(ActionMap::closeAllTabs, success);
         }},
        {ActionMap::nextTab,
         [this](QWidget* ctx) {
             int current = documentModel->getCurrentDocumentIndex();
             int count = documentModel->getDocumentCount();
             if (count > 1) {
                 int next = (current + 1) % count;
                 switchToDocument(next);
                 emit documentOperationCompleted(ActionMap::nextTab, true);
             }
         }},
        {ActionMap::prevTab,
         [this](QWidget* ctx) {
             int current = documentModel->getCurrentDocumentIndex();
             int count = documentModel->getDocumentCount();
             if (count > 1) {
                 int prev = (current - 1 + count) % count;
                 switchToDocument(prev);
                 emit documentOperationCompleted(ActionMap::prevTab, true);
             }
         }},
        {ActionMap::toggleSideBar,
         [this](QWidget* ctx) { emit sideBarToggleRequested(); }},
        {ActionMap::showSideBar,
         [this](QWidget* ctx) { emit sideBarShowRequested(); }},
        {ActionMap::hideSideBar,
         [this](QWidget* ctx) { emit sideBarHideRequested(); }},
        {ActionMap::setSinglePageMode,
         [this](QWidget* ctx) {
             emit viewModeChangeRequested(0);  // SinglePage
         }},
        {ActionMap::setContinuousScrollMode,
         [this](QWidget* ctx) {
             emit viewModeChangeRequested(1);  // ContinuousScroll
         }},
        // 页面导航操作
        {ActionMap::firstPage,
         [this](QWidget* ctx) {
             emit pdfActionRequested(ActionMap::firstPage);
         }},
        {ActionMap::previousPage,
         [this](QWidget* ctx) {
             emit pdfActionRequested(ActionMap::previousPage);
         }},
        {ActionMap::nextPage,
         [this](QWidget* ctx) {
             emit pdfActionRequested(ActionMap::nextPage);
         }},
        {ActionMap::lastPage,
         [this](QWidget* ctx) {
             emit pdfActionRequested(ActionMap::lastPage);
         }},
        {ActionMap::goToPage,
         [this](QWidget* ctx) {
             emit pdfActionRequested(ActionMap::goToPage);
         }},
        // 缩放操作
        {ActionMap::zoomIn,
         [this](QWidget* ctx) { emit pdfActionRequested(ActionMap::zoomIn); }},
        {ActionMap::zoomOut,
         [this](QWidget* ctx) { emit pdfActionRequested(ActionMap::zoomOut); }},
        {ActionMap::fitToWidth,
         [this](QWidget* ctx) {
             emit pdfActionRequested(ActionMap::fitToWidth);
         }},
        {ActionMap::fitToPage,
         [this](QWidget* ctx) {
             emit pdfActionRequested(ActionMap::fitToPage);
         }},
        {ActionMap::fitToHeight,
         [this](QWidget* ctx) {
             emit pdfActionRequested(ActionMap::fitToHeight);
         }},
        // 旋转操作
        {ActionMap::rotateLeft,
         [this](QWidget* ctx) {
             emit pdfActionRequested(ActionMap::rotateLeft);
         }},
        {ActionMap::rotateRight,
         [this](QWidget* ctx) {
             emit pdfActionRequested(ActionMap::rotateRight);
         }},
        // 主题操作
        {ActionMap::toggleTheme,
         [this](QWidget* ctx) { emit themeToggleRequested(); }},
        // 文档信息操作
        {ActionMap::showDocumentMetadata,
         [this](QWidget* ctx) { showDocumentMetadata(ctx); }},
        // 最近文件操作
        {ActionMap::openRecentFile,
         [this](QWidget* ctx) {
             // 这个操作通过信号处理，不在这里直接实现
             LOG_DEBUG("openRecentFile action triggered");
         }},
        {ActionMap::clearRecentFiles,
         [this](QWidget* ctx) {
             if (recentFilesManager) {
                 recentFilesManager->clearRecentFiles();
             }
         }},
        // 从合并分支添加的操作
        {ActionMap::saveFile, [this](QWidget* ctx) { /*....save()....*/ }}};
}

DocumentController::DocumentController(DocumentModel* model)
    : documentModel(model), recentFilesManager(nullptr) {
    initializeCommandMap();
}

void DocumentController::execute(ActionMap actionID, QWidget* context) {
    LOG_DEBUG("EventID: {} context: {}", static_cast<int>(actionID),
              static_cast<void*>(context));

    auto it = commandMap.find(actionID);
    if (it != commandMap.end()) {
        (*it)(context);
    } else {
        LOG_WARNING("Unknown action ID: {}", static_cast<int>(actionID));
    }
}

bool DocumentController::openDocument(const QString& filePath) {
    bool success = documentModel->openFromFile(filePath);

    // 如果文件打开成功，添加到最近文件列表
    if (success && recentFilesManager) {
        recentFilesManager->addRecentFile(filePath);
    }

    return success;
}

bool DocumentController::openDocuments(const QStringList& filePaths) {
    if (filePaths.isEmpty()) {
        return false;
    }

    // 过滤有效的PDF文件
    QStringList validPaths;
    for (const QString& filePath : filePaths) {
        if (!filePath.isEmpty() && QFile::exists(filePath) &&
            filePath.toLower().endsWith(".pdf")) {
            validPaths.append(filePath);
        }
    }

    if (validPaths.isEmpty()) {
        LOG_WARNING("No valid PDF files found in the selection");
        return false;
    }

    // 使用DocumentModel的批量打开方法
    bool success = documentModel->openFromFiles(validPaths);

    // 如果文件打开成功，添加到最近文件列表
    if (success && recentFilesManager) {
        for (const QString& filePath : validPaths) {
            recentFilesManager->addRecentFile(filePath);
        }
    }

    return success;
}

bool DocumentController::closeDocument(int index) {
    return documentModel->closeDocument(index);
}

bool DocumentController::closeCurrentDocument() {
    return documentModel->closeCurrentDocument();
}

void DocumentController::switchToDocument(int index) {
    documentModel->switchToDocument(index);
}

void DocumentController::setRecentFilesManager(RecentFilesManager* manager) {
    recentFilesManager = manager;
}

void DocumentController::showDocumentMetadata(QWidget* parent) {
    // 检查是否有当前文档
    if (documentModel->isEmpty()) {
        QMessageBox::information(parent, tr("提示"), tr("请先打开一个PDF文档"));
        return;
    }

    // 获取当前文档信息
    QString currentFilePath = documentModel->getCurrentFilePath();
    QString currentFileName = documentModel->getCurrentFileName();

    // 暂时使用简单的消息框显示基本信息，避免复杂的对话框导致编译问题
    QString info =
        QString("文档信息:\n文件名: %1\n路径: %2")
            .arg(currentFileName.isEmpty() ? tr("未知") : currentFileName)
            .arg(currentFilePath.isEmpty() ? tr("未知") : currentFilePath);

    // Get the current document from the model
    Poppler::Document* currentDoc = documentModel->getCurrentDocument();

    // Use the full DocumentMetadataDialog instead of simple message box
    DocumentMetadataDialog* dialog = new DocumentMetadataDialog(parent);
    dialog->setDocument(currentDoc, currentFilePath);
    dialog->exec();
    dialog->deleteLater();
}

void DocumentController::saveDocumentCopy(QWidget* parent) {
    // 检查是否有当前文档
    if (documentModel->isEmpty()) {
        QMessageBox::information(parent, tr("提示"), tr("请先打开一个PDF文档"));
        return;
    }

    // 获取当前文档
    Poppler::Document* currentDoc = documentModel->getCurrentDocument();
    if (!currentDoc) {
        QMessageBox::warning(parent, tr("错误"), tr("无法获取当前文档"));
        return;
    }

    // 获取当前文档信息
    QString currentFileName = documentModel->getCurrentFileName();
    QString suggestedName = currentFileName.isEmpty()
                                ? "document_copy.pdf"
                                : currentFileName + "_copy.pdf";

    // 显示保存对话框
    QString filePath = QFileDialog::getSaveFileName(
        parent, tr("另存副本"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) +
            "/" + suggestedName,
        tr("PDF Files (*.pdf)"));

    if (filePath.isEmpty()) {
        return;  // 用户取消了操作
    }

    // 确保文件扩展名为.pdf
    if (!filePath.toLower().endsWith(".pdf")) {
        filePath += ".pdf";
    }

    // 尝试保存文档副本
    // 注意：当前实现是一个简化版本，实际的注释嵌入需要额外的PDF处理库
    bool success = false;
    QString errorMessage;

    try {
        // 验证目标路径
        QFileInfo targetInfo(filePath);
        QDir targetDir = targetInfo.dir();

        if (!targetDir.exists()) {
            if (!targetDir.mkpath(targetDir.absolutePath())) {
                errorMessage =
                    tr("无法创建目标目录：%1").arg(targetDir.absolutePath());
                throw std::runtime_error(errorMessage.toStdString());
            }
        }

        // 检查目标目录是否可写
        if (!targetInfo.dir().isReadable()) {
            errorMessage =
                tr("目标目录不可访问：%1").arg(targetDir.absolutePath());
            throw std::runtime_error(errorMessage.toStdString());
        }

        // 获取原始文档路径进行复制
        QString originalPath = documentModel->getCurrentFilePath();
        if (originalPath.isEmpty()) {
            errorMessage = tr("无法获取当前文档的文件路径");
            throw std::runtime_error(errorMessage.toStdString());
        }

        if (!QFile::exists(originalPath)) {
            errorMessage = tr("原始文档文件不存在：%1").arg(originalPath);
            throw std::runtime_error(errorMessage.toStdString());
        }

        // 检查原始文件是否可读
        QFileInfo originalInfo(originalPath);
        if (!originalInfo.isReadable()) {
            errorMessage = tr("无法读取原始文档文件：%1").arg(originalPath);
            throw std::runtime_error(errorMessage.toStdString());
        }

        // 如果目标文件已存在，询问用户是否覆盖
        if (QFile::exists(filePath)) {
            int result = QMessageBox::question(
                parent, tr("文件已存在"),
                tr("目标文件已存在：\n%1\n\n是否要覆盖现有文件？")
                    .arg(filePath),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

            if (result != QMessageBox::Yes) {
                // 用户选择不覆盖，操作取消
                emit documentOperationCompleted(ActionMap::saveAs, false);
                return;
            }

            // 尝试删除现有文件
            if (!QFile::remove(filePath)) {
                errorMessage = tr("无法删除现有文件：%1").arg(filePath);
                throw std::runtime_error(errorMessage.toStdString());
            }
        }

        // 复制文件
        success = QFile::copy(originalPath, filePath);

        if (!success) {
            errorMessage =
                tr("文件复制失败。可能的原因：\n- 磁盘空间不足\n- "
                   "文件权限问题\n- 目标路径无效");
            throw std::runtime_error(errorMessage.toStdString());
        }

        // 验证复制是否成功
        if (!QFile::exists(filePath)) {
            errorMessage = tr("文件复制完成但无法验证结果文件");
            throw std::runtime_error(errorMessage.toStdString());
        }

        // 检查文件大小是否匹配
        QFileInfo originalFileInfo(originalPath);
        QFileInfo copiedFileInfo(filePath);

        if (originalFileInfo.size() != copiedFileInfo.size()) {
            errorMessage = tr("复制的文件大小不匹配，可能复制不完整");
            QFile::remove(filePath);  // 清理不完整的文件
            throw std::runtime_error(errorMessage.toStdString());
        }

        // 成功完成
        QMessageBox::information(
            parent, tr("保存成功"),
            tr("文档副本已成功保存到：\n%1\n\n文件大小：%"
               "2\n\n注意：当前版本将原始PDF文件复制为副本。如需将当前的标注和"
               "修改嵌入到副本中，需要使用专门的PDF编辑功能。")
                .arg(filePath)
                .arg(copiedFileInfo.size()));

    } catch (const std::exception& e) {
        success = false;
        if (errorMessage.isEmpty()) {
            errorMessage = tr("保存过程中发生未知错误：%1")
                               .arg(QString::fromStdString(e.what()));
        }

        QMessageBox::critical(parent, tr("保存失败"), errorMessage);

        // 清理可能创建的不完整文件
        if (QFile::exists(filePath)) {
            QFile::remove(filePath);
        }
    } catch (...) {
        success = false;
        errorMessage = tr("保存过程中发生未知错误");

        QMessageBox::critical(parent, tr("保存失败"), errorMessage);

        // 清理可能创建的不完整文件
        if (QFile::exists(filePath)) {
            QFile::remove(filePath);
        }
    }

    // 发送操作完成信号
    emit documentOperationCompleted(ActionMap::saveAs, success);
}

QStringList DocumentController::scanFolderForPDFs(const QString& folderPath) {
    QStringList pdfFiles;

    if (folderPath.isEmpty()) {
        LOG_WARNING(
            "DocumentController::scanFolderForPDFs: Empty folder path "
            "provided");
        return pdfFiles;
    }

    QDir dir(folderPath);
    if (!dir.exists()) {
        LOG_WARNING(
            "DocumentController::scanFolderForPDFs: Folder does not exist: {}",
            folderPath.toStdString());
        return pdfFiles;
    }

    LOG_DEBUG("DocumentController: Scanning folder for PDFs: {}",
              folderPath.toStdString());

    // 使用QDirIterator递归扫描文件夹中的所有PDF文件
    QDirIterator it(folderPath,
                    QStringList() << "*.pdf"
                                  << "*.PDF",
                    QDir::Files | QDir::Readable, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString filePath = it.next();

        // 验证文件是否存在且可读
        QFileInfo fileInfo(filePath);
        if (fileInfo.exists() && fileInfo.isReadable() && fileInfo.size() > 0) {
            pdfFiles.append(filePath);
            LOG_DEBUG("DocumentController: Found PDF file: {}",
                      filePath.toStdString());
        }
    }

    LOG_DEBUG("DocumentController: Found {} PDF files in folder",
              pdfFiles.size());
    return pdfFiles;
}
