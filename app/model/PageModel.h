#pragma once

#include <QDateTime>
#include <QList>
#include <QMessageBox>
#include <QObject>
#include <QSharedPointer>
#include <QSizeF>
#include <QStack>
#include <QTimer>
#include "RenderModel.h"

// Forward declarations
class QTimer;

// Page metadata structure
struct PageMetadata {
    int pageNumber;
    QSizeF pageSize;
    double rotation;
    bool isLoaded;
    QDateTime lastAccessed;
    QString cacheKey;

    PageMetadata()
        : pageNumber(0),
          rotation(0.0),
          isLoaded(false),
          lastAccessed(QDateTime::currentDateTime()) {}
    PageMetadata(int page, const QSizeF& size = QSizeF(), double rot = 0.0)
        : pageNumber(page),
          pageSize(size),
          rotation(rot),
          isLoaded(false),
          lastAccessed(QDateTime::currentDateTime()) {}
};

// Page validation result
enum class PageValidationResult {
    Valid = 0,
    InvalidPageNumber,
    DocumentNotLoaded,
    RenderModelNotSet,
    PageNotAccessible
};

class PageModel : public QObject {
    Q_OBJECT

public:
    PageModel(int totalPages = 1, QObject* parent = nullptr);
    PageModel(RenderModel* renderModel, QObject* parent = nullptr);

    // Basic page operations (existing - maintained for compatibility)
    int currentPage() const;
    int totalPages() const;
    void setCurrentPage(int pageNum);
    void nextPage();
    void prevPage();

    // Enhanced page operations
    bool goToPage(int pageNum);
    bool goToFirstPage();
    bool goToLastPage();

    // Page validation and information
    virtual PageValidationResult validatePage(int pageNum) const;
    virtual bool isValidPage(int pageNum) const;
    virtual QString getValidationErrorMessage(
        PageValidationResult result) const;

    // Page metadata and properties
    PageMetadata getPageMetadata(int pageNum) const;
    QSizeF getPageSize(int pageNum) const;
    double getPageRotation(int pageNum) const;
    bool isPageLoaded(int pageNum) const;
    void updatePageMetadata(int pageNum, const PageMetadata& metadata);

    // Performance optimization
    void preloadPage(int pageNum);
    void preloadPages(const QList<int>& pageNumbers);
    void preloadAdjacentPages(int centerPage, int radius = 2);
    void clearPageCache();
    void clearPageFromCache(int pageNum);

    // Render model integration
    void setRenderModel(RenderModel* renderModel);
    RenderModel* getRenderModel() const;
    bool hasRenderModel() const;

    // Document state
    virtual bool hasDocument() const;
    virtual bool isDocumentValid() const;
    virtual QString getLastError() const;

    // Statistics and monitoring
    int getCacheSize() const;
    int getPreloadedPagesCount() const;
    QList<int> getPreloadedPages() const;
    double getAveragePageLoadTime() const;

    ~PageModel(){};

public slots:
    void updateInfo(Poppler::Document* document);
    void onRenderModelChanged();
    void onPagePreloadRequested(int pageNum);

signals:
    void pageUpdate(int currentPage, int totalPages);
    void pageChanged(int newPage, int oldPage);
    void pageValidationFailed(int pageNum, const QString& error);
    void pageMetadataUpdated(int pageNum, const PageMetadata& metadata);
    void pagePreloaded(int pageNum);
    void pagePreloadFailed(int pageNum, const QString& error);
    void documentStateChanged(bool isValid);
    void renderModelChanged(RenderModel* newModel);
    void cacheUpdated(int cacheSize);

protected:
    // Core data
    int _totalPages;
    int _currentPage;
    RenderModel* _renderModel;

    // Enhanced features
    QList<PageMetadata> _pageMetadata;
    QList<int> _preloadedPages;
    QTimer* _preloadTimer;

    // Performance tracking
    QDateTime _lastPageChangeTime;
    QList<qint64> _pageLoadTimes;  // in milliseconds

    // Error handling
    mutable QString _lastError;

    // Helper methods
    void initializeMetadata();
    void updateMetadataForPage(int pageNum);
    void setError(const QString& error) const;
    void clearError() const;
    bool validatePageInternal(int pageNum) const;
    void emitPageChanged(int newPage, int oldPage);
    void startPreloadTimer();
    void stopPreloadTimer();

private slots:
    void onPreloadTimerTimeout();
    void onRenderCompleted(int pageNum, const QImage& image);
};
