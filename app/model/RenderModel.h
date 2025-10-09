#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QDebug>
#include <QImage>
#include <QObject>
#include "qtmetamacros.h"

class RenderModel : public QObject {
    Q_OBJECT

public:
    RenderModel(double dpiX = 72.0, double dpiY = 72.0,
                Poppler::Document* _document = nullptr,
                QObject* parent = nullptr);
    QImage renderPage(int pageNum = 0, double xres = 72.0, double yres = 72.0,
                      int x = 0, int y = 0, int w = -1, int h = -1);
    int getPageCount();
    void setDocument(Poppler::Document* _document);
    ~RenderModel() {}

signals:
    void renderPageDone(QImage image);
    void documentChanged(Poppler::Document* document);

private:
    Poppler::Document* document;
    double dpiX;
    double dpiY;
};
