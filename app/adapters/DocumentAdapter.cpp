#include "DocumentAdapter.h"

// Controllers
#include "controller/DocumentController.h"

// Pages
#include "ui/pages/PDFViewerPage.h"

// Logging
#include "logging/SimpleLogging.h"

// Qt
#include <QFileInfo>
#include <QMessageBox>

DocumentAdapter::DocumentAdapter(QObject* parent)
    : QObject(parent), m_documentController(nullptr), m_pdfViewerPage(nullptr) {
    SLOG_INFO("DocumentAdapter: Constructor");
}

DocumentAdapter::~DocumentAdapter() {
    SLOG_INFO("DocumentAdapter: Destructor");
}

void DocumentAdapter::setDocumentController(DocumentController* controller) {
    if (m_documentController) {
        disconnect(m_documentController, nullptr, this, nullptr);
    }

    m_documentController = controller;

    if (m_documentController) {
        connectControllerSignals();
    }
}

void DocumentAdapter::setPDFViewerPage(PDFViewerPage* page) {
    m_pdfViewerPage = page;
}

void DocumentAdapter::connectControllerSignals() {
    if (!m_documentController) {
        return;
    }

    // 获取 DocumentModel
    DocumentModel* documentModel = m_documentController->getDocumentModel();
    if (!documentModel) {
        return;
    }

    // 连接文档模型的信号
    connect(documentModel, &DocumentModel::documentOpened, this,
            [this](int index, const QString& fileName) {
                SLOG_INFO_F("DocumentAdapter: Document opened: {} (index: {})",
                            fileName.toStdString(), index);

                if (m_documentController) {
                    DocumentModel* model =
                        m_documentController->getDocumentModel();
                    if (model) {
                        Poppler::Document* doc = model->getCurrentDocument();
                        QString filePath = model->getCurrentFilePath();

                        // 转换为
                        // shared_ptr（注意：这里不拥有所有权，只是为了接口兼容）
                        std::shared_ptr<Poppler::Document> sharedDoc(
                            doc, [](Poppler::Document*) {});
                        emit documentOpened(sharedDoc, filePath);
                    }
                }
            });

    connect(
        documentModel, &DocumentModel::documentClosed, this, [this](int index) {
            SLOG_INFO_F("DocumentAdapter: Document closed (index: {})", index);
            emit documentClosed();
        });

    connect(documentModel, &DocumentModel::loadingFailed, this,
            [this](const QString& error, const QString& filePath) {
                SLOG_ERROR_F("DocumentAdapter: Loading failed: {} ({})",
                             error.toStdString(), filePath.toStdString());
                emit errorOccurred(error);
            });
}

void DocumentAdapter::openFile(const QString& filePath) {
    SLOG_INFO_F("DocumentAdapter: Opening file: {}", filePath.toStdString());

    if (!m_documentController) {
        SLOG_ERROR("DocumentAdapter: DocumentController is null");
        emit errorOccurred(tr("Document controller not initialized"));
        return;
    }

    if (filePath.isEmpty()) {
        SLOG_ERROR("DocumentAdapter: File path is empty");
        emit errorOccurred(tr("File path is empty"));
        return;
    }

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        SLOG_ERROR_F("DocumentAdapter: File does not exist: {}",
                     filePath.toStdString());
        emit errorOccurred(tr("File does not exist: %1").arg(filePath));
        return;
    }

    // 使用文档控制器打开文件
    m_documentController->openDocument(filePath);
}

void DocumentAdapter::closeDocument() {
    SLOG_INFO("DocumentAdapter: Closing document");

    if (!m_documentController) {
        SLOG_ERROR("DocumentAdapter: DocumentController is null");
        return;
    }

    m_documentController->closeCurrentDocument();
}

void DocumentAdapter::saveDocumentCopy(const QString& filePath) {
    SLOG_INFO_F("DocumentAdapter: Saving document copy to: {}",
                filePath.toStdString());

    if (!m_documentController) {
        SLOG_ERROR("DocumentAdapter: DocumentController is null");
        emit errorOccurred(tr("Document controller not initialized"));
        return;
    }

    if (filePath.isEmpty()) {
        SLOG_ERROR("DocumentAdapter: File path is empty");
        emit errorOccurred(tr("File path is empty"));
        return;
    }

    // 使用文档控制器保存文档副本（需要父窗口）
    m_documentController->saveDocumentCopy(m_pdfViewerPage);

    // 注意：实际的保存操作由 DocumentController 处理，这里只是触发
    SLOG_INFO("DocumentAdapter: Save document copy dialog triggered");
}

void DocumentAdapter::printDocument() {
    SLOG_INFO("DocumentAdapter: Printing document");

    if (!m_documentController) {
        SLOG_ERROR("DocumentAdapter: DocumentController is null");
        emit errorOccurred(tr("Document controller not initialized"));
        return;
    }

    // 使用文档控制器打印文档（需要父窗口）
    m_documentController->printDocument(m_pdfViewerPage);
}

void DocumentAdapter::exportDocument(const QString& filePath,
                                     const QString& format) {
    SLOG_INFO_F("DocumentAdapter: Exporting document to: {}",
                filePath.toStdString());

    if (!m_documentController) {
        SLOG_ERROR("DocumentAdapter: DocumentController is null");
        emit errorOccurred(tr("Document controller not initialized"));
        return;
    }

    // 使用文档控制器导出文档（需要父窗口）
    m_documentController->exportDocument(m_pdfViewerPage);

    // 注意：实际的导出操作由 DocumentController 处理，这里只是触发
    SLOG_INFO("DocumentAdapter: Export document dialog triggered");
}

void DocumentAdapter::showDocumentProperties() {
    SLOG_INFO("DocumentAdapter: Showing document properties");

    if (!m_documentController) {
        SLOG_ERROR("DocumentAdapter: DocumentController is null");
        emit errorOccurred(tr("Document controller not initialized"));
        return;
    }

    // 使用文档控制器显示文档属性（需要父窗口）
    m_documentController->showDocumentMetadata(m_pdfViewerPage);
}
