#pragma once

#include <QList>
#include <QMessageBox>
#include <QObject>
#include <QStack>
#include <cstdint>
#include "../model/PageModel.h"

// Forward declarations
class RenderModel;

// Bookmark structure for page bookmarks
struct PageBookmark {
    int pageNumber;
    QString title;
    QString description;
    QDateTime createdAt;
    double zoomLevel;
    int rotation;

    PageBookmark()
        : pageNumber(0),
          createdAt(QDateTime::currentDateTime()),
          zoomLevel(1.0),
          rotation(0) {}
    explicit PageBookmark(int page, QString t = "", QString desc = "",
                          double zoom = 1.0, int rot = 0)
        : pageNumber(page),
          title(std::move(t)),
          description(std::move(desc)),
          createdAt(QDateTime::currentDateTime()),
          zoomLevel(zoom),
          rotation(rot) {}
};

// Error codes for page operations
enum class PageError : std::uint8_t {
    None = 0,
    InvalidPageNumber,
    DocumentNotLoaded,
    ModelNotSet,
    HistoryEmpty,
    BookmarkNotFound,
    RenderError
};

class PageController : public QObject {
    Q_OBJECT

public:
    explicit PageController(PageModel* model, QObject* parent = nullptr);

    // Special member functions
    PageController(const PageController&) = delete;
    PageController& operator=(const PageController&) = delete;
    PageController(PageController&&) = delete;
    PageController& operator=(PageController&&) = delete;
    ~PageController() override = default;

    // Basic navigation (existing methods - maintained for compatibility)
public slots:
    void goToNextPage();
    void goToPrevPage();

    // Enhanced navigation methods
    void goToPage(int pageNumber);
    void goToFirstPage();
    void goToLastPage();

    // Page information retrieval
    int getCurrentPage() const;
    int getTotalPages() const;
    bool isValidPage(int pageNumber) const;

    // History management
    void goBack();
    void goForward();
    void clearHistory();
    bool canGoBack() const;
    bool canGoForward() const;
    QList<int> getNavigationHistory() const;

    // Bookmark functionality
    void addBookmark(const QString& title = "",
                     const QString& description = "");
    void addBookmarkAtPage(int pageNumber, const QString& title = "",
                           const QString& description = "");
    void removeBookmark(int index);
    void removeBookmarkAtPage(int pageNumber);
    void goToBookmark(int index);
    void goToBookmarkAtPage(int pageNumber);
    QList<PageBookmark> getBookmarks() const;
    int getBookmarkCount() const;
    bool hasBookmarkAtPage(int pageNumber) const;

    // Zoom and rotation control
    void setZoomLevel(double zoomLevel);
    void setRotation(int degrees);
    void resetView();
    double getCurrentZoomLevel() const;
    int getCurrentRotation() const;

    // Error handling
    PageError getLastError() const;
    QString getLastErrorMessage() const;

    // Model management
    void setModel(PageModel* model);
    PageModel* getModel() const;

signals:
    // Navigation signals
    void pageChanged(int currentPage, int totalPages);
    void navigationStateChanged(bool canGoBack, bool canGoForward);

    // Bookmark signals
    void bookmarkAdded(int pageNumber, const QString& title);
    void bookmarkRemoved(int pageNumber);
    void bookmarksChanged();

    // View state signals
    void zoomChanged(double zoomLevel);
    void rotationChanged(int degrees);

    // Error signals
    void errorOccurred(PageError error, const QString& message);

private:
    // Core data
    PageModel* _model;

    // History management
    QStack<int> _backHistory;
    QStack<int> _forwardHistory;
    bool _isNavigating;  // Flag to prevent history recording during
                         // programmatic navigation

    // Bookmark management
    QList<PageBookmark> _bookmarks;

    // View state
    double _currentZoomLevel;
    int _currentRotation;

    // Error handling
    mutable PageError _lastError;
    mutable QString _lastErrorMessage;

    // Helper methods
    void recordPageInHistory(int pageNumber);
    void clearForwardHistory();
    void setError(PageError error, const QString& message) const;
    void clearError() const;
    bool validatePageNumber(int pageNumber) const;
    void emitNavigationStateChanged();
    int findBookmarkIndex(int pageNumber) const;

private slots:
    void onModelPageUpdate(int currentPage, int totalPages);
};
