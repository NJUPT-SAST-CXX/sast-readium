#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QFileDialog>
#include <QObject>
#include <QString>
#include "qtmetamacros.h"
#include "RenderModel.h"

class DocumentModel : public QObject {
    Q_OBJECT

private:
    QString currentFilePath;
    std::unique_ptr<Poppler::Document> document;
    RenderModel* renderModel;

public:
    DocumentModel() {};
    DocumentModel(RenderModel* _renderModel);
    ~DocumentModel() {};
    bool isNULL();
    bool openFromFile(const QString& filePath);
signals:
    void renderPageDone(QImage image);
};
