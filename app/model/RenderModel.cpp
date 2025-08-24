#include "RenderModel.h"
#include "qimage.h"
#include "qlogging.h"
#include "qmessagebox.h"

RenderModel::RenderModel(Poppler::Document *_document, QObject *parent) : document(_document),QObject(parent){}


QImage RenderModel::renderPage(int pageNum,double xres,double yres,int x,int y,int w,int h){
    if(!document){
        qDebug() << "Document not loaded";
        return QImage();
    }
    std::unique_ptr<Poppler::Page> pdfPage = document->page(pageNum);
    if(!pdfPage){
        qDebug() << "Page not found";
        return QImage();
    }
    QImage image = pdfPage->renderToImage(xres,yres,x,y,w,h);
    if(image.isNull()){
        qDebug() << "Failed to render page";
        return QImage();
    }
    emit renderPageDone(image);
    return image;
}

int RenderModel::getPageCount(){
    if(!document){
        return 0;
    }
    return document->numPages();
}

void RenderModel::setDocument(Poppler::Document * _document){
    if(!_document){
        return;
    }
    document.reset(_document);
    emit documentChanged(document.get());
}
