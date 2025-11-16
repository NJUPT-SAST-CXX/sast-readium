#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QCache>
#include <QDateTime>
#include <QDebug>
#include <QFuture>
#include <QFutureWatcher>
#include <QImage>
#include <QMap>
#include <QObject>
#include <QRectF>
#include <QSizeF>
#include <QTimer>
#include "qtmetamacros.h"

class RenderModel : public QObject {
    Q_OBJECT

public:
    enum class RenderQuality { Draft, Normal, High, Ultra };

    RenderModel(double dpiX = 72.0, double dpiY = 72.0,
                Poppler::Document* _document = nullptr,
                QObject* parent = nullptr);
    ~RenderModel() override;

    // Basic rendering (existing)
    QImage renderPage(int pageNum = 0, double xres = 72.0, double yres = 72.0,
                      int x = 0, int y = 0, int w = -1, int h = -1);
    int getPageCount();
    void setDocument(Poppler::Document* _document);

    // Page Information
    QSizeF getPageSize(int pageNum) const;
    QRectF getPageBoundingBox(int pageNum) const;
    double getPageRotation(int pageNum) const;

    // Document Information
    QString getDocumentTitle() const;
    QString getDocumentAuthor() const;
    QString getDocumentSubject() const;
    QString getDocumentCreator() const;
    QDateTime getDocumentCreationDate() const;
    QDateTime getDocumentModificationDate() const;
    QMap<QString, QString> getDocumentInfo() const;

    // DPI Management
    double getDpiX() const;
    double getDpiY() const;
    void setDpiX(double dpi);
    void setDpiY(double dpi);
    void setDpi(double dpiX, double dpiY);

    // Effective DPI calculation with high DPI display support
    // devicePixelRatio: The device pixel ratio from
    // QWidget::devicePixelRatioF()
    //                   Defaults to 1.0 for standard DPI displays
    double getEffectiveDpiX(double scaleFactor = 1.0,
                            double devicePixelRatio = 1.0) const;
    double getEffectiveDpiY(double scaleFactor = 1.0,
                            double devicePixelRatio = 1.0) const;

    // Rendering Quality
    void setRenderQuality(RenderQuality quality);
    RenderQuality getRenderQuality() const;

    // Async Rendering
    void renderPageAsync(int pageNum, double xres = 72.0, double yres = 72.0,
                         int x = 0, int y = 0, int w = -1, int h = -1);
    void cancelAsyncRender(int pageNum);
    bool isRenderingAsync(int pageNum) const;

    // Cache Management
    void clearCache();
    void clearPageFromCache(int pageNum);
    int getCacheSize() const;
    int getMaxCacheSize() const;
    void setMaxCacheSize(int size);
    bool isPageCached(int pageNum, double xres = 72.0,
                      double yres = 72.0) const;

    // Validation
    bool isDocumentValid() const;
    bool hasPage(int pageNum) const;
    bool isPageValid(int pageNum) const;
    QString getLastError() const;

    // Document Configuration
    static void configureDocumentRenderHints(Poppler::Document* doc);

private slots:
    void onAsyncRenderCompleted();

private:
    // Helper methods
    double getQualityMultiplier() const;
    QString generateCacheKey(int pageNum, double xres, double yres, int x,
                             int y, int w, int h) const;

signals:
    void renderPageDone(QImage image);
    void documentChanged(Poppler::Document* document);
    void dpiChanged(double dpiX, double dpiY);
    void asyncRenderCompleted(int pageNum, QImage image);
    void asyncRenderFailed(int pageNum, QString error);
    void cacheUpdated(int pageNum);
    void cacheSizeChanged(int newSize);
    void renderQualityChanged(RenderQuality quality);
    void documentValidationChanged(bool isValid);

private:
    Poppler::Document* document;
    double dpiX;
    double dpiY;
    RenderQuality renderQuality;
    QCache<QString, QImage> pageCache;
    QMap<int, QFutureWatcher<QImage>*> asyncRenders;
    QTimer* cacheCleanupTimer;
    int maxCacheSize;
    mutable QString lastError;
};
