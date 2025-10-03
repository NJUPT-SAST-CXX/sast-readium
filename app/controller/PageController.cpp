#include "PageController.h"
#include <algorithm>
#include "../logging/LoggingMacros.h"
#include "model/PageModel.h"

PageController::PageController(PageModel* model, QObject* parent)
    : QObject(parent),
      _model(model),
      _isNavigating(false),
      _currentZoomLevel(1.0),
      _currentRotation(0),
      _lastError(PageError::None) {
    if (_model) {
        // Connect to model signals
        connect(_model, &PageModel::pageUpdate, this,
                &PageController::onModelPageUpdate);
    }

    clearError();
    LOG_DEBUG("PageController: Initialized with model: {}",
              _model ? "valid" : "null");
}

// Basic navigation methods (maintained for compatibility)
void PageController::goToNextPage() {
    if (!_model) {
        setError(PageError::ModelNotSet, "No model has been loaded!");
        QMessageBox::warning(nullptr, "Warning", "No model has been loaded!");
        return;
    }

    int currentPage = _model->currentPage();
    int totalPages = _model->totalPages();

    if (currentPage < totalPages) {
        goToPage(currentPage + 1);
    } else {
        // Wrap to first page
        goToPage(1);
    }
}

void PageController::goToPrevPage() {
    if (!_model) {
        setError(PageError::ModelNotSet, "No model has been loaded!");
        QMessageBox::warning(nullptr, "Warning", "No model has been loaded!");
        return;
    }

    int currentPage = _model->currentPage();
    int totalPages = _model->totalPages();

    if (currentPage > 1) {
        goToPage(currentPage - 1);
    } else {
        // Wrap to last page
        goToPage(totalPages);
    }
}

// Enhanced navigation methods
void PageController::goToPage(int pageNumber) {
    if (!validatePageNumber(pageNumber)) {
        return;
    }

    int currentPage = _model->currentPage();

    // Record current page in history before navigation (if not already
    // navigating)
    if (!_isNavigating && currentPage != pageNumber) {
        recordPageInHistory(currentPage);
    }

    _isNavigating = true;
    _model->setCurrentPage(pageNumber);
    _isNavigating = false;

    clearError();
    LOG_DEBUG("PageController: Navigated to page {}", pageNumber);
}

void PageController::goToFirstPage() {
    if (!_model) {
        setError(PageError::ModelNotSet, "No model has been loaded!");
        return;
    }

    goToPage(1);
}

void PageController::goToLastPage() {
    if (!_model) {
        setError(PageError::ModelNotSet, "No model has been loaded!");
        return;
    }

    int totalPages = _model->totalPages();
    if (totalPages > 0) {
        goToPage(totalPages);
    }
}

// Page information retrieval
int PageController::getCurrentPage() const {
    if (!_model) {
        setError(PageError::ModelNotSet, "No model has been loaded!");
        return 0;
    }
    return _model->currentPage();
}

int PageController::getTotalPages() const {
    if (!_model) {
        setError(PageError::ModelNotSet, "No model has been loaded!");
        return 0;
    }
    return _model->totalPages();
}

bool PageController::isValidPage(int pageNumber) const {
    return validatePageNumber(pageNumber);
}

// History management
void PageController::goBack() {
    if (!canGoBack()) {
        setError(PageError::HistoryEmpty, "No previous page in history");
        return;
    }

    int currentPage = _model->currentPage();
    int previousPage = _backHistory.pop();

    // Add current page to forward history
    _forwardHistory.push(currentPage);

    // Navigate to previous page
    _isNavigating = true;
    _model->setCurrentPage(previousPage);
    _isNavigating = false;

    emitNavigationStateChanged();
    clearError();
    LOG_DEBUG("PageController: Went back to page {}", previousPage);
}

void PageController::goForward() {
    if (!canGoForward()) {
        setError(PageError::HistoryEmpty, "No next page in history");
        return;
    }

    int currentPage = _model->currentPage();
    int nextPage = _forwardHistory.pop();

    // Add current page to back history
    _backHistory.push(currentPage);

    // Navigate to next page
    _isNavigating = true;
    _model->setCurrentPage(nextPage);
    _isNavigating = false;

    emitNavigationStateChanged();
    clearError();
    LOG_DEBUG("PageController: Went forward to page {}", nextPage);
}

void PageController::clearHistory() {
    _backHistory.clear();
    _forwardHistory.clear();
    emitNavigationStateChanged();
    LOG_DEBUG("PageController: History cleared");
}

bool PageController::canGoBack() const { return !_backHistory.isEmpty(); }

bool PageController::canGoForward() const { return !_forwardHistory.isEmpty(); }

QList<int> PageController::getNavigationHistory() const {
    QList<int> history;

    // Add back history (in reverse order)
    QStack<int> tempBack = _backHistory;
    while (!tempBack.isEmpty()) {
        history.prepend(tempBack.pop());
    }

    // Add current page
    if (_model) {
        history.append(_model->currentPage());
    }

    // Add forward history
    QStack<int> tempForward = _forwardHistory;
    QList<int> forwardList;
    while (!tempForward.isEmpty()) {
        forwardList.prepend(tempForward.pop());
    }
    history.append(forwardList);

    return history;
}

// Bookmark functionality
void PageController::addBookmark(const QString& title,
                                 const QString& description) {
    if (!_model) {
        setError(PageError::ModelNotSet, "No model has been loaded!");
        return;
    }

    int currentPage = _model->currentPage();
    addBookmarkAtPage(currentPage, title, description);
}

void PageController::addBookmarkAtPage(int pageNumber, const QString& title,
                                       const QString& description) {
    if (!validatePageNumber(pageNumber)) {
        return;
    }

    // Check if bookmark already exists at this page
    if (hasBookmarkAtPage(pageNumber)) {
        setError(PageError::BookmarkNotFound,
                 QString("Bookmark already exists at page %1").arg(pageNumber));
        return;
    }

    QString bookmarkTitle = title;
    if (bookmarkTitle.isEmpty()) {
        bookmarkTitle = QString("Page %1").arg(pageNumber);
    }

    PageBookmark bookmark(pageNumber, bookmarkTitle, description,
                          _currentZoomLevel, _currentRotation);
    _bookmarks.append(bookmark);

    // Sort bookmarks by page number
    std::sort(_bookmarks.begin(), _bookmarks.end(),
              [](const PageBookmark& a, const PageBookmark& b) {
                  return a.pageNumber < b.pageNumber;
              });

    emit bookmarkAdded(pageNumber, bookmarkTitle);
    emit bookmarksChanged();
    clearError();
    LOG_DEBUG("PageController: Added bookmark at page {} with title '{}'",
              pageNumber, bookmarkTitle.toStdString());
}

void PageController::removeBookmark(int index) {
    if (index < 0 || index >= _bookmarks.size()) {
        setError(PageError::BookmarkNotFound,
                 QString("Invalid bookmark index: %1").arg(index));
        return;
    }

    int pageNumber = _bookmarks[index].pageNumber;
    _bookmarks.removeAt(index);

    emit bookmarkRemoved(pageNumber);
    emit bookmarksChanged();
    clearError();
    LOG_DEBUG("PageController: Removed bookmark at index {} (page {})", index,
              pageNumber);
}

void PageController::removeBookmarkAtPage(int pageNumber) {
    int index = findBookmarkIndex(pageNumber);
    if (index >= 0) {
        removeBookmark(index);
    } else {
        setError(PageError::BookmarkNotFound,
                 QString("No bookmark found at page %1").arg(pageNumber));
    }
}

void PageController::goToBookmark(int index) {
    if (index < 0 || index >= _bookmarks.size()) {
        setError(PageError::BookmarkNotFound,
                 QString("Invalid bookmark index: %1").arg(index));
        return;
    }

    const PageBookmark& bookmark = _bookmarks[index];
    goToPage(bookmark.pageNumber);

    // Optionally restore zoom and rotation
    if (bookmark.zoomLevel > 0) {
        setZoomLevel(bookmark.zoomLevel);
    }
    if (bookmark.rotation >= 0) {
        setRotation(bookmark.rotation);
    }

    clearError();
    LOG_DEBUG("PageController: Navigated to bookmark at page {}",
              bookmark.pageNumber);
}

void PageController::goToBookmarkAtPage(int pageNumber) {
    int index = findBookmarkIndex(pageNumber);
    if (index >= 0) {
        goToBookmark(index);
    } else {
        setError(PageError::BookmarkNotFound,
                 QString("No bookmark found at page %1").arg(pageNumber));
    }
}

QList<PageBookmark> PageController::getBookmarks() const { return _bookmarks; }

int PageController::getBookmarkCount() const { return _bookmarks.size(); }

bool PageController::hasBookmarkAtPage(int pageNumber) const {
    return findBookmarkIndex(pageNumber) >= 0;
}

// Zoom and rotation control
void PageController::setZoomLevel(double zoomLevel) {
    if (zoomLevel <= 0) {
        setError(PageError::InvalidPageNumber,
                 QString("Invalid zoom level: %1").arg(zoomLevel));
        return;
    }

    if (qAbs(_currentZoomLevel - zoomLevel) > 0.001) {
        _currentZoomLevel = zoomLevel;
        emit zoomChanged(_currentZoomLevel);
        clearError();
        LOG_DEBUG("PageController: Zoom level changed to {}",
                  _currentZoomLevel);
    }
}

void PageController::setRotation(int degrees) {
    // Normalize rotation to 0-359 degrees
    degrees = ((degrees % 360) + 360) % 360;

    if (_currentRotation != degrees) {
        _currentRotation = degrees;
        emit rotationChanged(_currentRotation);
        clearError();
        LOG_DEBUG("PageController: Rotation changed to {} degrees",
                  _currentRotation);
    }
}

void PageController::resetView() {
    setZoomLevel(1.0);
    setRotation(0);
    LOG_DEBUG("PageController: View reset to default zoom and rotation");
}

double PageController::getCurrentZoomLevel() const { return _currentZoomLevel; }

int PageController::getCurrentRotation() const { return _currentRotation; }

// Error handling
PageError PageController::getLastError() const { return _lastError; }

QString PageController::getLastErrorMessage() const {
    return _lastErrorMessage;
}

// Model management
void PageController::setModel(PageModel* model) {
    if (_model) {
        // Disconnect from old model
        disconnect(_model, &PageModel::pageUpdate, this,
                   &PageController::onModelPageUpdate);
    }

    _model = model;

    if (_model) {
        // Connect to new model
        connect(_model, &PageModel::pageUpdate, this,
                &PageController::onModelPageUpdate);
    }

    // Clear history and bookmarks when model changes
    clearHistory();
    _bookmarks.clear();
    emit bookmarksChanged();

    clearError();
    LOG_DEBUG("PageController: Model changed to {}", _model ? "valid" : "null");
}

PageModel* PageController::getModel() const { return _model; }

// Helper methods
void PageController::recordPageInHistory(int pageNumber) {
    if (pageNumber > 0 && pageNumber <= getTotalPages()) {
        _backHistory.push(pageNumber);

        // Limit history size to prevent memory issues
        const int MAX_HISTORY_SIZE = 100;
        if (_backHistory.size() > MAX_HISTORY_SIZE) {
            // Remove oldest entries
            QStack<int> tempStack;
            for (int i = 0; i < MAX_HISTORY_SIZE - 1; ++i) {
                if (!_backHistory.isEmpty()) {
                    tempStack.push(_backHistory.pop());
                }
            }
            _backHistory.clear();
            while (!tempStack.isEmpty()) {
                _backHistory.push(tempStack.pop());
            }
        }

        clearForwardHistory();
        emitNavigationStateChanged();
    }
}

void PageController::clearForwardHistory() { _forwardHistory.clear(); }

void PageController::setError(PageError error, const QString& message) const {
    _lastError = error;
    _lastErrorMessage = message;

    if (error != PageError::None) {
        LOG_WARNING("PageController: Error occurred - {}",
                    message.toStdString());
        emit const_cast<PageController*>(this)->errorOccurred(error, message);
    }
}

void PageController::clearError() const {
    _lastError = PageError::None;
    _lastErrorMessage.clear();
}

bool PageController::validatePageNumber(int pageNumber) const {
    if (!_model) {
        setError(PageError::ModelNotSet, "No model has been loaded!");
        return false;
    }

    int totalPages = _model->totalPages();
    if (totalPages <= 0) {
        setError(PageError::DocumentNotLoaded,
                 "No document loaded or document is empty");
        return false;
    }

    if (pageNumber < 1 || pageNumber > totalPages) {
        setError(PageError::InvalidPageNumber,
                 QString("Page number %1 is out of range (1-%2)")
                     .arg(pageNumber)
                     .arg(totalPages));
        return false;
    }

    return true;
}

void PageController::emitNavigationStateChanged() {
    emit navigationStateChanged(canGoBack(), canGoForward());
}

int PageController::findBookmarkIndex(int pageNumber) const {
    for (int i = 0; i < _bookmarks.size(); ++i) {
        if (_bookmarks[i].pageNumber == pageNumber) {
            return i;
        }
    }
    return -1;
}

// Private slots
void PageController::onModelPageUpdate(int currentPage, int totalPages) {
    emit pageChanged(currentPage, totalPages);
    clearError();
}
