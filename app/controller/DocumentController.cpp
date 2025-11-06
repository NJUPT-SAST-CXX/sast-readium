#include "DocumentController.h"
#include <poppler/qt6/poppler-qt6.h>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QPainter>
#include <QPrintDialog>
#include <QPrinter>
#include <QProgressDialog>
#include <QStandardPaths>
#include <QStringList>
#include <QTimer>
#include "../logging/LoggingMacros.h"
#include "../managers/I18nManager.h"
#include "../managers/StyleManager.h"
#include "../ui/core/StatusBar.h"
#include "../ui/dialogs/DocumentMetadataDialog.h"
#include "../ui/dialogs/SettingsDialog.h"
#include "../ui/widgets/ToastNotification.h"

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
        {ActionMap::save,
         [this](QWidget* ctx) {
             // PDF viewers typically don't modify the original file
             // Implement "save" as "save as" to preserve the original
             saveDocumentCopy(ctx);
         }},
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
             Q_UNUSED(ctx)
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
         [this](QWidget* ctx) {
             Q_UNUSED(ctx)
             emit sideBarToggleRequested();
         }},
        {ActionMap::showSideBar,
         [this](QWidget* ctx) {
             Q_UNUSED(ctx)
             emit sideBarShowRequested();
         }},
        {ActionMap::hideSideBar,
         [this](QWidget* ctx) {
             Q_UNUSED(ctx)
             emit sideBarHideRequested();
         }},
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
        // 设置操作
        {ActionMap::showSettings, [this](QWidget* ctx) { showSettings(ctx); }},
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
        // saveFile is an alias for save - both use saveDocumentCopy
        {ActionMap::saveFile, [this](QWidget* ctx) { saveDocumentCopy(ctx); }},

        // Tab switching operation
        {ActionMap::switchToTab,
         [this](QWidget* ctx) {
             // This requires tab index from context - emit signal for UI to
             // handle
             emit tabSwitchRequested();
         }},

        // Search operations
        {ActionMap::showSearch,
         [this](QWidget* ctx) { emit searchToggleRequested(true); }},
        {ActionMap::hideSearch,
         [this](QWidget* ctx) { emit searchToggleRequested(false); }},
        {ActionMap::toggleSearch,
         [this](QWidget* ctx) { emit searchToggleRequested(); }},
        {ActionMap::findNext,
         [this](QWidget* ctx) { emit searchNavigationRequested(true); }},
        {ActionMap::findPrevious,
         [this](QWidget* ctx) { emit searchNavigationRequested(false); }},
        {ActionMap::clearSearch,
         [this](QWidget* ctx) { emit searchClearRequested(); }},

        // File operations
        {ActionMap::closeFile,
         [this](QWidget* ctx) {
             bool success = closeCurrentDocument();
             emit documentOperationCompleted(ActionMap::closeFile, success);
         }},
        {ActionMap::fullScreen,
         [this](QWidget* ctx) { emit fullScreenToggleRequested(); }},

        // Document operations
        {ActionMap::exportFile, [this](QWidget* ctx) { exportDocument(ctx); }},
        {ActionMap::printFile, [this](QWidget* ctx) { printDocument(ctx); }},
        {ActionMap::reloadFile, [this](QWidget* ctx) { reloadDocument(ctx); }}};
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
    if (!documentModel) {
        // DocumentModel is null - cannot proceed
        LOG_ERROR("DocumentController::openDocument() - DocumentModel is null");
        return false;
    }

    // Validate file path
    if (filePath.isEmpty()) {
        LOG_ERROR(
            "DocumentController::openDocument() - Empty file path provided");
        return false;
    }

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        LOG_ERROR(
            "DocumentController::openDocument() - File does not exist: {}",
            filePath.toStdString());
        return false;
    }

    if (!fileInfo.isReadable()) {
        LOG_ERROR(
            "DocumentController::openDocument() - File is not readable: {}",
            filePath.toStdString());
        return false;
    }

    // Show progress indication for document loading
    if (statusBar) {
        statusBar->showProgress(
            tr("Opening document: %1").arg(fileInfo.fileName()), 10);
        statusBar->updateProgress(10);
    }

    LOG_INFO("DocumentController::openDocument() - Opening document: {}",
             filePath.toStdString());

    bool success = documentModel->openFromFile(filePath);

    if (success) {
        // Update progress
        if (statusBar) {
            statusBar->updateProgress(80, tr("Document loaded successfully"));
        }

        // 如果文件打开成功，添加到最近文件列表
        if (recentFilesManager) {
            recentFilesManager->addRecentFile(filePath);
        }

        // Complete progress
        if (statusBar) {
            statusBar->updateProgress(100, tr("Ready"));
            // Hide progress after a short delay
            QTimer::singleShot(1000, [this]() {
                if (statusBar) {
                    statusBar->hideProgress();
                }
            });
        }

        LOG_INFO(
            "DocumentController::openDocument() - Document opened "
            "successfully: {}",
            filePath.toStdString());
    } else {
        // Hide progress on failure
        if (statusBar) {
            statusBar->hideProgress();
            statusBar->setErrorMessage(
                tr("Failed to open document: %1").arg(fileInfo.fileName()),
                5000);
        }
        LOG_ERROR(
            "DocumentController::openDocument() - Failed to open document: {}",
            filePath.toStdString());
    }

    // Emit signal for document operation completion
    emit documentOperationCompleted(ActionMap::openFile, success);

    return success;
}

bool DocumentController::openDocuments(const QStringList& filePaths) {
    if (filePaths.isEmpty()) {
        LOG_WARNING(
            "DocumentController::openDocuments() - Empty file paths list "
            "provided");
        return false;
    }

    if (!documentModel) {
        // DocumentModel is null - cannot proceed
        LOG_ERROR(
            "DocumentController::openDocuments() - DocumentModel is null");
        return false;
    }

    // Show progress for multiple document loading
    if (statusBar) {
        statusBar->showProgress(
            tr("Opening %1 documents...").arg(filePaths.size()), 10);
        statusBar->updateProgress(10);
    }

    LOG_INFO("DocumentController::openDocuments() - Opening {} documents",
             filePaths.size());

    // Let the DocumentModel handle validation - it knows best what files it can
    // open
    bool success = documentModel->openFromFiles(filePaths);

    if (success) {
        // Update progress
        if (statusBar) {
            statusBar->updateProgress(80, tr("Documents loaded successfully"));
        }

        // If files opened successfully, add them to recent files
        // Only add files that the model actually accepted
        if (recentFilesManager) {
            // Filter to only add valid files that were actually opened
            for (const QString& filePath : filePaths) {
                if (!filePath.isEmpty() &&
                    filePath.toLower().endsWith(".pdf")) {
                    recentFilesManager->addRecentFile(filePath);
                }
            }
        }

        // Complete progress
        if (statusBar) {
            statusBar->updateProgress(100, tr("Ready"));
            // Hide progress after a short delay
            QTimer::singleShot(1000, [this]() {
                if (statusBar) {
                    statusBar->hideProgress();
                }
            });
        }

        LOG_INFO(
            "DocumentController::openDocuments() - {} documents opened "
            "successfully",
            filePaths.size());
    } else {
        // Hide progress on failure
        if (statusBar) {
            statusBar->hideProgress();
            statusBar->setErrorMessage(tr("Failed to open documents"), 5000);
        }
        LOG_ERROR(
            "DocumentController::openDocuments() - Failed to open documents");
    }

    return success;
}

bool DocumentController::closeDocument(int index) {
    if (!documentModel) {
        // DocumentModel is null - cannot proceed
        return false;
    }
    bool result = documentModel->closeDocument(index);
    emit documentOperationCompleted(ActionMap::closeTab, result);
    return result;
}

bool DocumentController::closeCurrentDocument() {
    if (!documentModel) {
        // DocumentModel is null - cannot proceed
        return false;
    }
    bool result = documentModel->closeCurrentDocument();
    emit documentOperationCompleted(ActionMap::closeCurrentTab, result);
    return result;
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
        TOAST_INFO(parent, tr("请先打开一个PDF文档"));
        return;
    }

    // 获取当前文档信息
    QString currentFilePath = documentModel->getCurrentFilePath();

    // Get the current document from the model
    Poppler::Document* currentDoc = documentModel->getCurrentDocument();

    // Display comprehensive document metadata using DocumentMetadataDialog
    DocumentMetadataDialog* dialog = new DocumentMetadataDialog(parent);
    dialog->setDocument(currentDoc, currentFilePath);
    dialog->exec();
    dialog->deleteLater();
}

void DocumentController::showSettings(QWidget* parent) {
    // Create and show the Settings dialog
    SettingsDialog* dialog = new SettingsDialog(parent);

    // Connect settings signals to application components
    connect(dialog, &SettingsDialog::themeChanged, this,
            [this](const QString& theme) {
                // Apply theme change through StyleManager
                StyleManager& styleManager = StyleManager::instance();
                Theme newTheme = (theme == "dark") ? Theme::Dark : Theme::Light;
                styleManager.setTheme(newTheme);
                emit themeToggleRequested();
            });

    connect(dialog, &SettingsDialog::languageChanged, this,
            [this](const QString& languageCode) {
                // Apply language change through I18nManager
                I18nManager::instance().loadLanguage(languageCode);
                emit languageChanged(languageCode);
            });

    connect(dialog, &SettingsDialog::settingsApplied, this, [this]() {
        // Handle any additional settings application logic
        emit settingsChanged();
    });

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
    QDirIterator it(folderPath, QStringList() << "*.pdf" << "*.PDF",
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
void DocumentController::exportDocument(QWidget* parent) {
    // Check if there's a current document
    if (documentModel->isEmpty()) {
        QMessageBox::information(parent, tr("提示"), tr("请先打开一个PDF文档"));
        return;
    }

    // Get current document
    Poppler::Document* currentDoc = documentModel->getCurrentDocument();
    if (!currentDoc) {
        QMessageBox::warning(parent, tr("错误"), tr("无法获取当前文档"));
        return;
    }

    // Show export options dialog
    QStringList exportFormats;
    exportFormats << tr("PDF文件 (*.pdf)") << tr("图片文件 (*.png)")
                  << tr("文本文件 (*.txt)");

    bool ok;
    QString selectedFormat =
        QInputDialog::getItem(parent, tr("导出文档"), tr("选择导出格式:"),
                              exportFormats, 0, false, &ok);

    if (!ok || selectedFormat.isEmpty()) {
        return;
    }

    QString filter;
    QString defaultExt;
    if (selectedFormat.contains("*.pdf")) {
        filter = tr("PDF文件 (*.pdf)");
        defaultExt = ".pdf";
    } else if (selectedFormat.contains("*.png")) {
        filter = tr("图片文件 (*.png)");
        defaultExt = ".png";
    } else if (selectedFormat.contains("*.txt")) {
        filter = tr("文本文件 (*.txt)");
        defaultExt = ".txt";
    }

    QString fileName = QFileDialog::getSaveFileName(
        parent, tr("导出文档"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) +
            "/" + documentModel->getCurrentFileName() + defaultExt,
        filter);

    if (fileName.isEmpty()) {
        return;
    }

    // Use the currentDoc already retrieved at the beginning of the function
    int totalPages = currentDoc->numPages();
    bool exportSuccess = false;

    if (defaultExt == ".pdf") {
        // PDF export - just copy the file
        QString originalPath = documentModel->getCurrentFilePath();
        if (QFile::copy(originalPath, fileName)) {
            QMessageBox::information(
                parent, tr("导出成功"),
                tr("文档已成功导出到：\n%1").arg(fileName));
            exportSuccess = true;
        } else {
            QMessageBox::critical(parent, tr("导出失败"),
                                  tr("无法导出文档到指定位置"));
        }
    } else if (defaultExt == ".png") {
        // PNG export - export all pages as images
        QFileInfo fileInfo(fileName);
        QString baseName = fileInfo.completeBaseName();
        QString dirPath = fileInfo.absolutePath();

        // Create progress dialog
        QProgressDialog progressDialog(tr("正在导出为图片..."), tr("取消"), 0,
                                       totalPages, parent);
        progressDialog.setWindowModality(Qt::WindowModal);
        progressDialog.setMinimumDuration(500);

        int exportedCount = 0;

        for (int pageIndex = 0; pageIndex < totalPages; ++pageIndex) {
            // Check if user cancelled
            if (progressDialog.wasCanceled()) {
                break;
            }

            // Update progress
            progressDialog.setValue(pageIndex + 1);
            progressDialog.setLabelText(tr("正在导出第 %1/%2 页...")
                                            .arg(pageIndex + 1)
                                            .arg(totalPages));

            // Get page
            std::unique_ptr<Poppler::Page> page(currentDoc->page(pageIndex));
            if (!page) {
                qWarning() << "Failed to load page" << pageIndex + 1
                           << ", skipping";
                continue;
            }

            // Render page to image at 150 DPI (good quality for images)
            QImage pageImage = page->renderToImage(150.0, 150.0);
            if (pageImage.isNull()) {
                qWarning() << "Failed to render page" << pageIndex + 1
                           << ", skipping";
                continue;
            }

            // Generate filename for this page
            QString pageFileName;
            if (totalPages == 1) {
                pageFileName = fileName;
            } else {
                pageFileName = QString("%1/%2_page_%3.png")
                                   .arg(dirPath)
                                   .arg(baseName)
                                   .arg(pageIndex + 1, 3, 10, QChar('0'));
            }

            // Save image
            if (pageImage.save(pageFileName, "PNG")) {
                exportedCount++;
            } else {
                qWarning() << "Failed to save page" << pageIndex + 1 << "to"
                           << pageFileName;
            }
        }

        progressDialog.setValue(totalPages);

        if (exportedCount > 0 && !progressDialog.wasCanceled()) {
            QString message;
            if (totalPages == 1) {
                message = tr("文档已成功导出到：\n%1").arg(fileName);
            } else {
                message = tr("文档已成功导出 %1 页到：\n%2")
                              .arg(exportedCount)
                              .arg(dirPath);
            }
            QMessageBox::information(parent, tr("导出成功"), message);
            exportSuccess = true;
        } else if (progressDialog.wasCanceled()) {
            QMessageBox::information(parent, tr("导出已取消"),
                                     tr("已导出 %1 页").arg(exportedCount));
        } else {
            QMessageBox::critical(parent, tr("导出失败"),
                                  tr("无法导出任何页面"));
        }
    } else if (defaultExt == ".txt") {
        // TXT export - extract text from all pages
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(parent, tr("导出失败"),
                                  tr("无法创建文件：%1").arg(fileName));
            emit documentOperationCompleted(ActionMap::exportFile, false);
            return;
        }

        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8);

        // Create progress dialog
        QProgressDialog progressDialog(tr("正在提取文本..."), tr("取消"), 0,
                                       totalPages, parent);
        progressDialog.setWindowModality(Qt::WindowModal);
        progressDialog.setMinimumDuration(500);

        int extractedCount = 0;

        for (int pageIndex = 0; pageIndex < totalPages; ++pageIndex) {
            // Check if user cancelled
            if (progressDialog.wasCanceled()) {
                break;
            }

            // Update progress
            progressDialog.setValue(pageIndex + 1);
            progressDialog.setLabelText(tr("正在提取第 %1/%2 页...")
                                            .arg(pageIndex + 1)
                                            .arg(totalPages));

            // Get page
            std::unique_ptr<Poppler::Page> page(currentDoc->page(pageIndex));
            if (!page) {
                qWarning() << "Failed to load page" << pageIndex + 1
                           << ", skipping";
                continue;
            }

            // Extract text
            QString pageText = page->text(QRectF());
            if (!pageText.isEmpty()) {
                out << "=== Page " << (pageIndex + 1) << " ===\n\n";
                out << pageText << "\n\n";
                extractedCount++;
            }
        }

        file.close();
        progressDialog.setValue(totalPages);

        if (extractedCount > 0 && !progressDialog.wasCanceled()) {
            QMessageBox::information(
                parent, tr("导出成功"),
                tr("文本已成功导出到：\n%1\n\n提取了 %2 页的文本")
                    .arg(fileName)
                    .arg(extractedCount));
            exportSuccess = true;
        } else if (progressDialog.wasCanceled()) {
            QMessageBox::information(
                parent, tr("导出已取消"),
                tr("已提取 %1 页的文本").arg(extractedCount));
        } else {
            QMessageBox::warning(parent, tr("导出警告"),
                                 tr("文档中没有可提取的文本"));
        }
    }

    emit documentOperationCompleted(ActionMap::exportFile, exportSuccess);
}

void DocumentController::printDocument(QWidget* parent) {
    // Check if there's a current document
    if (documentModel->isEmpty()) {
        QMessageBox::information(parent, tr("提示"), tr("请先打开一个PDF文档"));
        return;
    }

    // Get current document
    Poppler::Document* currentDoc = documentModel->getCurrentDocument();
    if (!currentDoc) {
        QMessageBox::warning(parent, tr("错误"), tr("无法获取当前文档"));
        return;
    }

    int totalPages = currentDoc->numPages();

    // Create printer with high resolution
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::NativeFormat);
    printer.setColorMode(QPrinter::Color);

    // Show print dialog
    QPrintDialog printDialog(&printer, parent);
    printDialog.setWindowTitle(tr("打印文档"));
    printDialog.setMinMax(1, totalPages);

    if (printDialog.exec() != QDialog::Accepted) {
        emit documentOperationCompleted(ActionMap::printFile, false);
        return;
    }

    // Get page range from dialog
    int startPage = 0;
    int endPage = totalPages - 1;

    if (printDialog.printRange() == QAbstractPrintDialog::PageRange) {
        startPage = printDialog.fromPage() - 1;
        endPage = printDialog.toPage() - 1;
    }

    // Validate page range
    if (startPage < 0 || endPage >= totalPages || startPage > endPage) {
        QMessageBox::warning(
            parent, tr("错误"),
            tr("无效的页面范围：%1-%2").arg(startPage + 1).arg(endPage + 1));
        emit documentOperationCompleted(ActionMap::printFile, false);
        return;
    }

    // Create progress dialog
    QProgressDialog progressDialog(tr("正在打印..."), tr("取消"), 0,
                                   endPage - startPage + 1, parent);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setMinimumDuration(500);

    // Start printing
    QPainter painter;
    if (!painter.begin(&printer)) {
        QMessageBox::critical(parent, tr("打印错误"), tr("无法启动打印任务"));
        emit documentOperationCompleted(ActionMap::printFile, false);
        return;
    }

    // Configure rendering for high quality
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    bool printSuccess = true;

    // Print each page
    for (int pageIndex = startPage; pageIndex <= endPage; ++pageIndex) {
        // Check if user cancelled
        if (progressDialog.wasCanceled()) {
            printSuccess = false;
            break;
        }

        // Update progress
        progressDialog.setValue(pageIndex - startPage + 1);
        progressDialog.setLabelText(tr("正在打印第 %1/%2 页...")
                                        .arg(pageIndex - startPage + 1)
                                        .arg(endPage - startPage + 1));

        // Get page
        std::unique_ptr<Poppler::Page> page(currentDoc->page(pageIndex));
        if (!page) {
            qWarning() << "Failed to load page" << pageIndex + 1
                       << ", skipping";
            continue;
        }

        // Render page to image at 300 DPI for high quality printing
        QImage pageImage = page->renderToImage(300.0, 300.0);
        if (pageImage.isNull()) {
            qWarning() << "Failed to render page" << pageIndex + 1
                       << ", skipping";
            continue;
        }

        // Calculate scaling to fit page on printer
        QRectF printerRect = printer.pageRect(QPrinter::DevicePixel);

        // Scale to fit while maintaining aspect ratio
        double scaleX = printerRect.width() / pageImage.width();
        double scaleY = printerRect.height() / pageImage.height();
        double scale = qMin(scaleX, scaleY);

        // Center the image on the page
        double x = (printerRect.width() - pageImage.width() * scale) / 2.0;
        double y = (printerRect.height() - pageImage.height() * scale) / 2.0;

        // Draw the page
        painter.drawImage(
            QRectF(x, y, pageImage.width() * scale, pageImage.height() * scale),
            pageImage);

        // Start new page if not the last page
        if (pageIndex < endPage) {
            if (!printer.newPage()) {
                QMessageBox::critical(parent, tr("打印错误"),
                                      tr("无法创建新页面，打印已中止"));
                printSuccess = false;
                break;
            }
        }
    }

    painter.end();
    progressDialog.setValue(endPage - startPage + 1);

    if (printSuccess && !progressDialog.wasCanceled()) {
        QMessageBox::information(parent, tr("打印完成"),
                                 tr("文档已成功发送到打印机\n\n打印页数：%1")
                                     .arg(endPage - startPage + 1));
        emit documentOperationCompleted(ActionMap::printFile, true);
    } else {
        emit documentOperationCompleted(ActionMap::printFile, false);
    }
}

void DocumentController::reloadDocument(QWidget* parent) {
    // Check if there's a current document
    if (documentModel->isEmpty()) {
        QMessageBox::information(parent, tr("提示"), tr("请先打开一个PDF文档"));
        return;
    }

    QString currentFilePath = documentModel->getCurrentFilePath();
    if (currentFilePath.isEmpty()) {
        QMessageBox::warning(parent, tr("错误"), tr("无法获取当前文档路径"));
        emit documentOperationCompleted(ActionMap::reloadFile, false);
        return;
    }

    // Check if file still exists
    if (!QFile::exists(currentFilePath)) {
        QMessageBox::warning(parent, tr("文件不存在"),
                             tr("原始文件已不存在：\n%1\n\n无法重新加载文档")
                                 .arg(currentFilePath));
        emit documentOperationCompleted(ActionMap::reloadFile, false);
        return;
    }

    // Ask user for confirmation
    int result = QMessageBox::question(
        parent, tr("重新加载文档"),
        tr("确定要重新加载当前文档吗？\n\n这将丢失所有未保存的更改。\n\n文档：%"
           "1")
            .arg(QFileInfo(currentFilePath).fileName()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (result != QMessageBox::Yes) {
        emit documentOperationCompleted(ActionMap::reloadFile, false);
        return;
    }

    // Emit signal to request reload with state preservation
    // The ApplicationController will handle getting the current state from
    // ViewWidget, closing the document, reopening it, and restoring the state
    emit documentReloadRequested(currentFilePath, 0, 1.0);

    // Show success message - the actual reload is handled asynchronously by
    // ApplicationController
    QMessageBox::information(parent, tr("重新加载成功"),
                             tr("文档已成功重新加载"));

    // Note: documentOperationCompleted is emitted by ApplicationController
    // after reload completes
}
