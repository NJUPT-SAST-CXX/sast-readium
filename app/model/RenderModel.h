#pragma once

#include <QObject>
#include <poppler/qt6/poppler-qt6.h>
#include <QImage>
#include <QDebug>
#include "qimage.h"
#include "qtmetamacros.h"

class RenderModel : public QObject{
    Q_OBJECT
public:
    RenderModel(Poppler::Document *_document = nullptr, QObject *parent = nullptr);
    QImage renderPage(int pageNum = 0,double xres=72.0,double yres=72.0,int x=-1,int y=-1,int w=-1,int h=-1);
    int getPageCount();
    void setDocument(Poppler::Document * _document);
    ~RenderModel(){};
signals:
    void renderPageDone(QImage image);
    void documentChanged(Poppler::Document * document);
private:
    std::unique_ptr<Poppler::Document> document;
};
