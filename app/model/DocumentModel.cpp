#include "DocumentModel.h"
#include <QFileInfo>
#include "../controller/EventBus.h"
#include "../logging/LoggingMacros.h"
#include "RenderModel.h"
#include "utils/ErrorHandling.h"
#include "utils/ErrorRecovery.h"

void DocumentModel::initializeErrorRecovery() {
    // Register recovery actions for document-related errors
    auto& recoveryManager = ErrorRecovery::RecoveryManager::instance();

    // Register file system recovery action
    recoveryManager.registerRecoveryAction(
        ErrorHandling::ErrorCategory::FileSystem,
        std::make_shared<ErrorRecovery::FileSystemRecoveryAction>());

    // Register document recovery action
    recoveryManager.registerRecoveryAction(
        ErrorHandling::ErrorCategory::Document,
        std::make_shared<ErrorRecovery::DocumentRecoveryAction>());

    LOG_DEBUG("DocumentModel: Error recovery actions registered");
}

// 添加支持RenderModel的构造函数
DocumentModel::DocumentModel(RenderModel* _renderModel)
    : currentDocumentIndex(-1), renderModel(_renderModel) {
    LOG_DEBUG("DocumentModel created with RenderModel");

    // Register error recovery actions
    initializeErrorRecovery();

    // 初始化异步加载器
    asyncLoader = new AsyncDocumentLoader(this);

    // 连接异步加载器信号
    connect(asyncLoader, &AsyncDocumentLoader::documentLoaded, this,
            &DocumentModel::onDocumentLoaded);
    connect(asyncLoader, &AsyncDocumentLoader::loadingProgressChanged, this,
            &DocumentModel::loadingProgressChanged);
    connect(asyncLoader, &AsyncDocumentLoader::loadingMessageChanged, this,
            &DocumentModel::loadingMessageChanged);
    connect(asyncLoader, &AsyncDocumentLoader::loadingFailed, this,
            &DocumentModel::loadingFailed);
}

DocumentModel::DocumentModel()
    : currentDocumentIndex(-1), renderModel(nullptr) {
    // Register error recovery actions
    initializeErrorRecovery();

    // 初始化异步加载器
    asyncLoader = new AsyncDocumentLoader(this);

    // 连接异步加载器信号
    connect(asyncLoader, &AsyncDocumentLoader::loadingProgressChanged, this,
            &DocumentModel::loadingProgressChanged);
    connect(asyncLoader, &AsyncDocumentLoader::loadingMessageChanged, this,
            &DocumentModel::loadingMessageChanged);
    connect(asyncLoader, &AsyncDocumentLoader::documentLoaded, this,
            &DocumentModel::onDocumentLoaded);
    connect(asyncLoader, &AsyncDocumentLoader::loadingFailed, this,
            &DocumentModel::loadingFailed);
}

DocumentModel::~DocumentModel() {
    LOG_DEBUG("DocumentModel: Destructor called");
    // Proactively disconnect to avoid late signal deliveries during teardown
    disconnect(this, nullptr, nullptr, nullptr);
    if (asyncLoader) {
        asyncLoader->disconnect(this);
        asyncLoader->cancelLoading();
        // let QObject parent-child delete handle actual deletion
    }
}

bool DocumentModel::openFromFile(const QString& filePath) {
    using namespace ErrorHandling;
    using namespace ErrorRecovery;

    // Use retry policy for file operations
    auto retryConfig = Utils::createStandardRetry();

    auto result = RecoveryManager::instance().retryWithPolicy(
        [&]() -> bool {
            // Input validation with standardized error handling
            if (filePath.isEmpty()) {
                throw ApplicationException(createFileSystemError(
                    "open document", filePath, "File path is empty"));
            }

            if (!QFile::exists(filePath)) {
                throw ApplicationException(createFileSystemError(
                    "open document", filePath, "File does not exist"));
            }

            // 检查文档是否已经打开
            for (size_t i = 0; i < documents.size(); ++i) {
                if (documents[i]->filePath == filePath) {
                    LOG_INFO("Document already open, switching to existing: {}",
                             filePath.toStdString());
                    switchToDocument(static_cast<int>(i));
                    return true;
                }
            }

            // 发送加载开始信号
            LOG_INFO("Starting document load: {}", filePath.toStdString());
            emit loadingStarted(filePath);
            PUBLISH_EVENT("document.loading", filePath);

            // 使用异步加载器加载文档
            asyncLoader->loadDocument(filePath);
            return true;  // 异步加载，立即返回true
        },
        retryConfig, QString("DocumentModel::openFromFile(%1)").arg(filePath));

    if (isError(result)) {
        const auto& error = getError(result);

        // Attempt recovery
        auto recoveryResult = RecoveryManager::instance().executeRecovery(
            error, "DocumentModel", "openFromFile");

        if (recoveryResult == RecoveryResult::Retry) {
            // Retry the operation once more
            return openFromFile(filePath);
        }
        if (recoveryResult == RecoveryResult::Fallback) {
            // Try alternative loading method or provide user feedback
            emit loadingFailed(
                "Document loading failed, but recovery options available",
                filePath);
            return false;
        }

        // Recovery failed, emit error
        emit loadingFailed(error.message, filePath);
        return false;
    }

    return getValue(result);
}

bool DocumentModel::openFromFiles(const QStringList& filePaths) {
    if (filePaths.isEmpty()) {
        return false;
    }

    // 过滤掉已经打开的文档
    QStringList newFilePaths;
    for (const QString& filePath : filePaths) {
        if (filePath.isEmpty() || !QFile::exists(filePath)) {
            continue;
        }

        // 检查是否已经打开
        bool alreadyOpen = false;
        for (const auto& document : documents) {
            if (document->filePath == filePath) {
                alreadyOpen = true;
                break;
            }
        }

        if (!alreadyOpen) {
            newFilePaths.append(filePath);
        }
    }

    if (newFilePaths.isEmpty()) {
        // 如果没有新文档需要打开，切换到第一个已存在的文档
        if (!filePaths.isEmpty()) {
            for (size_t i = 0; i < documents.size(); ++i) {
                if (documents[i]->filePath == filePaths.first()) {
                    switchToDocument(static_cast<int>(i));
                    break;
                }
            }
        }
        return true;
    }

    // 优化加载策略：先加载第一个文档
    QString firstFile = newFilePaths.first();
    emit loadingStarted(firstFile);
    asyncLoader->loadDocument(firstFile);

    // 如果有多个文档，暂时简化实现：逐个加载其他文档
    if (newFilePaths.size() > 1) {
        QStringList remainingFiles = newFilePaths.mid(1);
        // 为每个文档发送加载开始信号，创建占位标签页
        for (const QString& filePath : remainingFiles) {
            emit loadingStarted(filePath);
        }
        // 暂时简化：将其他文件路径存储起来，等第一个加载完成后再处理
        pendingFiles = remainingFiles;
    }

    return true;
}

void DocumentModel::onDocumentLoaded(Poppler::Document* document,
                                     const QString& filePath) {
    if (!document) {
        emit loadingFailed("文档加载失败", filePath);
        return;
    }

    // 创建unique_ptr管理文档
    std::unique_ptr<Poppler::Document> popplerDoc(document);

    // 创建文档信息
    auto docInfo =
        std::make_unique<DocumentInfo>(filePath, std::move(popplerDoc));
    documents.push_back(std::move(docInfo));

    int newIndex = static_cast<int>(documents.size() - 1);
    currentDocumentIndex = newIndex;

    if (renderModel) {
        renderModel->setDocument(documents[newIndex]->document.get());
    }

    LOG_INFO("Async loaded successfully: {}", filePath.toStdString());
    emit documentOpened(newIndex, documents[newIndex]->fileName);
    PUBLISH_EVENT(AppEvents::DOCUMENT_OPENED(), filePath);
    emit currentDocumentChanged(newIndex);

    // 检查是否还有待加载的文件
    if (!pendingFiles.isEmpty()) {
        QString nextFile = pendingFiles.takeFirst();
        LOG_DEBUG("Loading next file from queue: {}", nextFile.toStdString());

        // 发送加载开始信号
        emit loadingStarted(nextFile);

        // 开始加载下一个文档
        asyncLoader->loadDocument(nextFile);
    }
}

bool DocumentModel::closeDocument(int index) {
    if (!isValidIndex(index)) {
        return false;
    }

    const QString closedFilePath = documents[index]->filePath;

    documents.erase(documents.begin() + index);
    emit documentClosed(index);

    // 调整当前文档索引
    if (documents.empty()) {
        currentDocumentIndex = -1;
        emit allDocumentsClosed();
        if (renderModel) {
            renderModel->setDocument(nullptr);
        }
    } else if (index <= currentDocumentIndex) {
        if (currentDocumentIndex >= static_cast<int>(documents.size())) {
            currentDocumentIndex = static_cast<int>(documents.size()) - 1;
        }
        emit currentDocumentChanged(currentDocumentIndex);
        if (renderModel && isValidIndex(currentDocumentIndex)) {
            renderModel->setDocument(
                documents[currentDocumentIndex]->document.get());
        }
    }

    PUBLISH_EVENT(AppEvents::DOCUMENT_CLOSED(), closedFilePath);

    return true;
}

bool DocumentModel::closeCurrentDocument() {
    return closeDocument(currentDocumentIndex);
}

void DocumentModel::switchToDocument(int index) {
    if (isValidIndex(index) && index != currentDocumentIndex) {
        currentDocumentIndex = index;
        if (renderModel) {
            renderModel->setDocument(documents[index]->document.get());
        }
        emit currentDocumentChanged(index);
    }
}

int DocumentModel::getDocumentCount() const {
    return static_cast<int>(documents.size());
}

int DocumentModel::getCurrentDocumentIndex() const {
    return currentDocumentIndex;
}

QString DocumentModel::getCurrentFilePath() const {
    if (isValidIndex(currentDocumentIndex)) {
        return documents[currentDocumentIndex]->filePath;
    }
    return QString();
}

QString DocumentModel::getCurrentFileName() const {
    if (isValidIndex(currentDocumentIndex)) {
        return documents[currentDocumentIndex]->fileName;
    }
    return QString();
}

QString DocumentModel::getDocumentFileName(int index) const {
    if (isValidIndex(index)) {
        return documents[index]->fileName;
    }
    return QString();
}

QString DocumentModel::getDocumentFilePath(int index) const {
    if (isValidIndex(index)) {
        return documents[index]->filePath;
    }
    return QString();
}

Poppler::Document* DocumentModel::getCurrentDocument() const {
    if (isValidIndex(currentDocumentIndex)) {
        return documents[currentDocumentIndex]->document.get();
    }
    return nullptr;
}

Poppler::Document* DocumentModel::getDocument(int index) const {
    if (isValidIndex(index)) {
        return documents[index]->document.get();
    }
    return nullptr;
}

bool DocumentModel::isEmpty() const { return documents.empty(); }

bool DocumentModel::isValidIndex(int index) const {
    return index >= 0 && index < static_cast<int>(documents.size());
}

bool DocumentModel::isNULL() {
    return false;  // DocumentModel is not null when this method is called
}

void DocumentModel::setRecentFilesManager(RecentFilesManager* manager) {
    recentFilesManager = manager;
}
