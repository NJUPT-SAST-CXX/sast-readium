#include "PageModel.h"
#include "../cache/PDFCacheManager.h"
#include "../logging/LoggingMacros.h"
#include "RenderModel.h"

PageModel::PageModel(int totalPages, QObject* parent)
    : QObject(parent),
      _totalPages(totalPages),
      _currentPage(1),
      _renderModel(nullptr),
      _pdfCacheManager(new PDFCacheManager(this)),
      _document(nullptr),
      _preloadTimer(new QTimer(this)),
      _lastPageChangeTime(QDateTime::currentDateTime()) {
    // Initialize preload timer
    _preloadTimer->setSingleShot(true);
    _preloadTimer->setInterval(500);  // 500ms delay for preloading
    connect(_preloadTimer, &QTimer::timeout, this,
            &PageModel::onPreloadTimerTimeout);

    _preloadEnabled = true;
    _preloadRadius = 2;

    connect(_pdfCacheManager, &PDFCacheManager::preloadRequested, this,
            &PageModel::onPdfCachePreloadRequested);

    initializeMetadata();
    clearError();
    LOG_DEBUG("PageModel: Initialized with {} pages", _totalPages);
}

void PageModel::onPdfCachePreloadRequested(int pageNumber, CacheItemType type) {
    Q_UNUSED(pageNumber)
    Q_UNUSED(type)
    if (_pdfCacheManager && _document) {
        _pdfCacheManager->executePreload(_document);
    }
}

PageModel::PageModel(RenderModel* renderModel, QObject* parent)
    : QObject(parent),
      _totalPages(0),
      _currentPage(1),
      _renderModel(renderModel),
      _pdfCacheManager(new PDFCacheManager(this)),
      _document(nullptr),
      _preloadTimer(new QTimer(this)),
      _lastPageChangeTime(QDateTime::currentDateTime()) {
    // Initialize preload timer
    _preloadTimer->setSingleShot(true);
    _preloadTimer->setInterval(500);  // 500ms delay for preloading
    connect(_preloadTimer, &QTimer::timeout, this,
            &PageModel::onPreloadTimerTimeout);

    _preloadEnabled = true;
    _preloadRadius = 2;

    if (_renderModel) {
        _totalPages = _renderModel->getPageCount();
        connect(_renderModel, &RenderModel::documentChanged, this,
                &PageModel::onRenderModelChanged);
        connect(_renderModel, &RenderModel::documentChanged, this,
                &PageModel::updateInfo);
        LOG_DEBUG("PageModel: Initialized with RenderModel, {} pages",
                  _totalPages);
    } else {
        LOG_WARNING("PageModel: Initialized with null RenderModel");
    }

    connect(_pdfCacheManager, &PDFCacheManager::preloadRequested, this,
            &PageModel::onPdfCachePreloadRequested);

    initializeMetadata();
    clearError();
}

int PageModel::currentPage() const { return _currentPage; }

int PageModel::totalPages() const { return _totalPages; }

void PageModel::setCurrentPage(int pageNum) {
    // Enhanced validation using new validation system
    PageValidationResult validation = validatePage(pageNum);
    if (validation != PageValidationResult::Valid) {
        QString errorMsg = getValidationErrorMessage(validation);
        setError(errorMsg);
        emit pageValidationFailed(pageNum, errorMsg);
        LOG_WARNING("PageModel: Page validation failed for page {}: {}",
                    pageNum, errorMsg.toStdString());
        return;
    }

    if (pageNum == _currentPage) {
        LOG_DEBUG("PageModel: Already on page {}, ignoring", pageNum);
        clearError();
        return;
    }

    int oldPage = _currentPage;
    _lastPageChangeTime = QDateTime::currentDateTime();

    LOG_DEBUG("PageModel: Changing from page {} to page {}", _currentPage,
              pageNum);
    _currentPage = pageNum;

    // Update page metadata
    updateMetadataForPage(_currentPage);

    try {
        if (_renderModel) {
            auto startTime = QDateTime::currentMSecsSinceEpoch();
            _renderModel->renderPage(
                _currentPage -
                1);  // 调用renderPage时页数减1 (0-based indexing)
            auto endTime = QDateTime::currentMSecsSinceEpoch();

            // Track page load time for performance monitoring
            qint64 loadTime = endTime - startTime;
            _pageLoadTimes.append(loadTime);

            // Keep only recent load times (last 50)
            if (_pageLoadTimes.size() > 50) {
                _pageLoadTimes.removeFirst();
            }
        }

        emit pageUpdate(_currentPage, _totalPages);
        emitPageChanged(_currentPage, oldPage);

        // Start preload timer for adjacent pages
        startPreloadTimer();

        clearError();
        LOG_DEBUG("PageModel: Successfully changed to page {}", _currentPage);
    } catch (const std::exception& e) {
        QString errorMsg = QString("Exception while rendering page %1: %2")
                               .arg(_currentPage)
                               .arg(e.what());
        setError(errorMsg);
        LOG_ERROR("PageModel: {}", errorMsg.toStdString());
    } catch (...) {
        QString errorMsg = QString("Unknown exception while rendering page %1")
                               .arg(_currentPage);
        setError(errorMsg);
        LOG_ERROR("PageModel: {}", errorMsg.toStdString());
    }
}

void PageModel::nextPage() {
    if (_currentPage < _totalPages) {
        int nextPage = _currentPage + 1;
        setCurrentPage(nextPage);
    } else if (_currentPage == _totalPages && _totalPages > 0) {
        setCurrentPage(1);  // Wrap to first page
    }
}

void PageModel::prevPage() {
    if (_currentPage > 1) {
        int prevPage = _currentPage - 1;
        setCurrentPage(prevPage);
    } else if (_currentPage == 1 && _totalPages > 0) {
        setCurrentPage(_totalPages);  // Wrap to last page
    }
}

// Enhanced page operations
bool PageModel::goToPage(int pageNum) {
    PageValidationResult validation = validatePage(pageNum);
    if (validation != PageValidationResult::Valid) {
        QString errorMsg = getValidationErrorMessage(validation);
        setError(errorMsg);
        emit pageValidationFailed(pageNum, errorMsg);
        return false;
    }

    setCurrentPage(pageNum);
    return true;
}

bool PageModel::goToFirstPage() { return goToPage(1); }

bool PageModel::goToLastPage() {
    if (_totalPages > 0) {
        return goToPage(_totalPages);
    }
    setError("No pages available");
    return false;
}

// Page validation and information
PageValidationResult PageModel::validatePage(int pageNum) const {
    if (!_renderModel) {
        return PageValidationResult::RenderModelNotSet;
    }

    if (!hasDocument()) {
        return PageValidationResult::DocumentNotLoaded;
    }

    if (pageNum < 1 || pageNum > _totalPages) {
        return PageValidationResult::InvalidPageNumber;
    }

    // Additional validation could be added here
    // For example, checking if the page is accessible or corrupted

    return PageValidationResult::Valid;
}

bool PageModel::isValidPage(int pageNum) const {
    return validatePage(pageNum) == PageValidationResult::Valid;
}

QString PageModel::getValidationErrorMessage(
    PageValidationResult result) const {
    switch (result) {
        case PageValidationResult::Valid:
            return QString();
        case PageValidationResult::InvalidPageNumber:
            return QString("Page number is out of range (1-%1)")
                .arg(_totalPages);
        case PageValidationResult::DocumentNotLoaded:
            return "No document loaded";
        case PageValidationResult::RenderModelNotSet:
            return "Render model not set";
        case PageValidationResult::PageNotAccessible:
            return "Page is not accessible";
        default:
            return "Unknown validation error";
    }
}

// Page metadata and properties
PageMetadata PageModel::getPageMetadata(int pageNum) const {
    if (pageNum < 1 || pageNum > _pageMetadata.size()) {
        return PageMetadata();
    }
    return _pageMetadata[pageNum - 1];  // Convert to 0-based index
}

QSizeF PageModel::getPageSize(int pageNum) const {
    if (!_renderModel || !isValidPage(pageNum)) {
        return QSizeF();
    }

    return _renderModel->getPageSize(pageNum - 1);  // Convert to 0-based index
}

double PageModel::getPageRotation(int pageNum) const {
    if (!_renderModel || !isValidPage(pageNum)) {
        return 0.0;
    }

    return _renderModel->getPageRotation(pageNum -
                                         1);  // Convert to 0-based index
}

bool PageModel::isPageLoaded(int pageNum) const {
    if (pageNum < 1 || pageNum > _pageMetadata.size()) {
        return false;
    }
    return _pageMetadata[pageNum - 1].isLoaded;
}

void PageModel::updatePageMetadata(int pageNum, const PageMetadata& metadata) {
    if (pageNum < 1 || pageNum > _pageMetadata.size()) {
        return;
    }

    _pageMetadata[pageNum - 1] = metadata;
    _pageMetadata[pageNum - 1].lastAccessed = QDateTime::currentDateTime();
    emit pageMetadataUpdated(pageNum, metadata);
}

// Performance optimization
void PageModel::preloadPage(int pageNum) {
    if (!isValidPage(pageNum) || !_renderModel) {
        emit pagePreloadFailed(pageNum, "Invalid page or no render model");
        return;
    }

    if (_preloadedPages.contains(pageNum)) {
        return;  // Already preloaded
    }

    try {
        // Use async rendering if available
        if (_renderModel->isDocumentValid()) {
            _renderModel->renderPageAsync(pageNum -
                                          1);  // Convert to 0-based index
            _preloadedPages.append(pageNum);
            emit pagePreloaded(pageNum);
            emit cacheUpdated(_preloadedPages.size());
            LOG_DEBUG("PageModel: Preloaded page {}", pageNum);
        }
    } catch (const std::exception& e) {
        QString errorMsg =
            QString("Failed to preload page %1: %2").arg(pageNum).arg(e.what());
        emit pagePreloadFailed(pageNum, errorMsg);
        LOG_ERROR("PageModel: {}", errorMsg.toStdString());
    }
}

void PageModel::preloadPages(const QList<int>& pageNumbers) {
    for (int pageNum : pageNumbers) {
        preloadPage(pageNum);
    }
}

void PageModel::preloadAdjacentPages(int centerPage, int radius) {
    if (!isValidPage(centerPage)) {
        return;
    }

    QList<int> pagesToPreload;
    for (int i = -radius; i <= radius; ++i) {
        int pageNum = centerPage + i;
        if (pageNum != centerPage && isValidPage(pageNum)) {
            pagesToPreload.append(pageNum);
        }
    }

    preloadPages(pagesToPreload);
}

void PageModel::clearPageCache() {
    if (_renderModel) {
        _renderModel->clearCache();
    }
    _preloadedPages.clear();
    emit cacheUpdated(0);
    LOG_DEBUG("PageModel: Page cache cleared");
}

void PageModel::clearPageFromCache(int pageNum) {
    if (_renderModel) {
        _renderModel->clearPageFromCache(pageNum -
                                         1);  // Convert to 0-based index
    }
    _preloadedPages.removeAll(pageNum);
    emit cacheUpdated(_preloadedPages.size());
}

void PageModel::setPreloadEnabled(bool enabled) {
    _preloadEnabled = enabled;
    if (!_preloadEnabled) {
        stopPreloadTimer();
    }
}

bool PageModel::isPreloadEnabled() const { return _preloadEnabled; }

void PageModel::setPreloadRadius(int radius) {
    _preloadRadius = qMax(0, radius);
}

int PageModel::preloadRadius() const { return _preloadRadius; }

void PageModel::updateInfo(Poppler::Document* document) {
    int oldTotalPages = _totalPages;
    _document = document;
    _totalPages = document->numPages();
    _currentPage = 1;

    // Reinitialize metadata for new document
    initializeMetadata();

    if (_renderModel && _totalPages > 0) {
        _renderModel->renderPage(
            _currentPage -
            1);  // poppler::document从0开始计数，但为方便page从1开始计数，此处需要-1

        // Update document state
        emit documentStateChanged(true);

        // Clear preloaded pages since document changed
        _preloadedPages.clear();
        emit cacheUpdated(0);
    }

    if (oldTotalPages != _totalPages) {
        emit pageUpdate(_currentPage, _totalPages);
    }

    clearError();
    LOG_DEBUG("PageModel: Document info updated - {} pages", _totalPages);
}

// Render model integration
void PageModel::setRenderModel(RenderModel* renderModel) {
    if (_renderModel) {
        // Disconnect from old render model
        disconnect(_renderModel, &RenderModel::documentChanged, this,
                   &PageModel::onRenderModelChanged);
        disconnect(_renderModel, &RenderModel::asyncRenderCompleted, this,
                   &PageModel::onRenderCompleted);
    }

    _renderModel = renderModel;

    if (_renderModel) {
        // Connect to new render model
        connect(_renderModel, &RenderModel::documentChanged, this,
                &PageModel::onRenderModelChanged);
        connect(_renderModel, &RenderModel::documentChanged, this,
                &PageModel::updateInfo);
        connect(_renderModel, &RenderModel::asyncRenderCompleted, this,
                &PageModel::onRenderCompleted);

        // Update total pages if document is already loaded
        if (_renderModel->isDocumentValid()) {
            _totalPages = _renderModel->getPageCount();
            initializeMetadata();
        }
    }

    emit renderModelChanged(_renderModel);
    clearError();
    LOG_DEBUG("PageModel: Render model changed to {}",
              _renderModel ? "valid" : "null");
}

RenderModel* PageModel::getRenderModel() const { return _renderModel; }

bool PageModel::hasRenderModel() const { return _renderModel != nullptr; }

// Document state
bool PageModel::hasDocument() const {
    return _renderModel && _renderModel->isDocumentValid() && _totalPages > 0;
}

bool PageModel::isDocumentValid() const { return hasDocument(); }

QString PageModel::getLastError() const { return _lastError; }

// Statistics and monitoring
int PageModel::getCacheSize() const {
    return _renderModel ? _renderModel->getCacheSize() : 0;
}

int PageModel::getPreloadedPagesCount() const { return _preloadedPages.size(); }

QList<int> PageModel::getPreloadedPages() const { return _preloadedPages; }

double PageModel::getAveragePageLoadTime() const {
    if (_pageLoadTimes.isEmpty()) {
        return 0.0;
    }

    qint64 total = 0;
    for (qint64 time : _pageLoadTimes) {
        total += time;
    }

    return static_cast<double>(total) / _pageLoadTimes.size();
}

// Public slots
void PageModel::onRenderModelChanged() {
    if (_renderModel && _renderModel->isDocumentValid()) {
        int newTotalPages = _renderModel->getPageCount();
        if (newTotalPages != _totalPages) {
            _totalPages = newTotalPages;
            _currentPage = qMin(_currentPage, _totalPages);
            initializeMetadata();
            emit pageUpdate(_currentPage, _totalPages);
        }
        emit documentStateChanged(true);
    } else {
        emit documentStateChanged(false);
    }
    clearError();
}

void PageModel::onPagePreloadRequested(int pageNum) { preloadPage(pageNum); }

// Helper methods
void PageModel::initializeMetadata() {
    _pageMetadata.clear();
    _pageMetadata.reserve(_totalPages);

    for (int i = 1; i <= _totalPages; ++i) {
        PageMetadata metadata(i);
        if (_renderModel) {
            metadata.pageSize =
                _renderModel->getPageSize(i - 1);  // Convert to 0-based index
            metadata.rotation = _renderModel->getPageRotation(i - 1);
        }
        _pageMetadata.append(metadata);
    }

    LOG_DEBUG("PageModel: Initialized metadata for {} pages", _totalPages);
}

void PageModel::updateMetadataForPage(int pageNum) {
    if (pageNum < 1 || pageNum > _pageMetadata.size() || !_renderModel) {
        return;
    }

    PageMetadata& metadata =
        _pageMetadata[pageNum - 1];  // Convert to 0-based index
    metadata.lastAccessed = QDateTime::currentDateTime();
    metadata.isLoaded = true;

    // Update size and rotation from render model
    metadata.pageSize = _renderModel->getPageSize(pageNum - 1);
    metadata.rotation = _renderModel->getPageRotation(pageNum - 1);

    emit pageMetadataUpdated(pageNum, metadata);
}

void PageModel::setError(const QString& error) const {
    _lastError = error;
    if (!error.isEmpty()) {
        LOG_WARNING("PageModel: Error - {}", error.toStdString());
    }
}

void PageModel::clearError() const { _lastError.clear(); }

bool PageModel::validatePageInternal(int pageNum) const {
    return pageNum >= 1 && pageNum <= _totalPages && hasDocument();
}

void PageModel::emitPageChanged(int newPage, int oldPage) {
    emit pageChanged(newPage, oldPage);
}

void PageModel::startPreloadTimer() {
    if (_preloadTimer && _preloadEnabled && !_preloadTimer->isActive()) {
        _preloadTimer->start();
    }
}

void PageModel::stopPreloadTimer() {
    if (_preloadTimer && _preloadTimer->isActive()) {
        _preloadTimer->stop();
    }
}

// Private slots
void PageModel::onPreloadTimerTimeout() {
    // Preload adjacent pages when timer expires
    if (_currentPage > 0 && _totalPages > 0 && _preloadEnabled) {
        int radius = qMax(0, _preloadRadius);
        preloadAdjacentPages(_currentPage, radius);
        if (_pdfCacheManager && _document) {
            QList<int> pagesToPreload;
            for (int i = -radius; i <= radius; ++i) {
                int zeroBased = (_currentPage - 1) + i;
                if (i != 0 && zeroBased >= 0 && zeroBased < _totalPages) {
                    pagesToPreload.append(zeroBased);
                }
            }
            _pdfCacheManager->preloadPages(pagesToPreload,
                                           CacheItemType::Thumbnail);
            _pdfCacheManager->preloadPages(pagesToPreload,
                                           CacheItemType::TextContent);
            _pdfCacheManager->executePreload(_document);
        }
    }
}

void PageModel::onRenderCompleted(int pageNum, const QImage& image) {
    Q_UNUSED(image)

    // Update metadata when render is completed
    if (pageNum >= 0 && pageNum < _totalPages) {
        int pageNumber = pageNum + 1;  // Convert from 0-based to 1-based
        updateMetadataForPage(pageNumber);

        if (!_preloadedPages.contains(pageNumber)) {
            _preloadedPages.append(pageNumber);
            emit pagePreloaded(pageNumber);
            emit cacheUpdated(_preloadedPages.size());
        }
    }
}

PageModel::~PageModel() {
    // Clean up resources
    if (_preloadTimer) {
        _preloadTimer->stop();
    }

    // Clear preloaded pages
    _preloadedPages.clear();

    // Clear page metadata
    _pageMetadata.clear();

    // Clear page load times
    _pageLoadTimes.clear();

    LOG_DEBUG("PageModel: Destroyed");
}
