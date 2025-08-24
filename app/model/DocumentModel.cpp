#include "DocumentModel.h"
#include <algorithm>
#include "RenderModel.h"
#include "qimage.h"
#include "qmessagebox.h"

DocumentModel::DocumentModel(RenderModel* _renderModel): renderModel(_renderModel) {
    qDebug() << "DocumentModel created";
}

bool DocumentModel::isNULL() { return false; }

bool DocumentModel::openFromFile(const QString& filePath) {
    if (filePath.isEmpty() || !QFile::exists(filePath)) {
        qWarning() << "Invalid file path:" << filePath;
        return false;
    }

    auto _document = Poppler::Document::load(filePath);
    if (!_document) {
        qDebug() << "Failed to load document:" << filePath;
        return false;
    }
    if(document){
        document.release();
    }
    document = std::move(_document);
    currentFilePath = filePath;
    qDebug() << "Opened successfully:" << filePath;

    renderModel->setDocument(document.get());
    QImage image = renderModel->renderPage();

    return true;
}
