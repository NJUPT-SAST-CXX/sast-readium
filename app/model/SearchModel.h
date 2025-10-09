#pragma once

#include <poppler-qt6.h>
#include <QAbstractListModel>
#include <QFuture>
#include <QFutureWatcher>
#include <QList>
#include <QObject>
#include <QRegularExpression>
#include <QString>
#include <QTimer>

/**
 * Represents a single search result with enhanced coordinate transformation
 * support
 */
struct SearchResult {
    int pageNumber;
    QString text;
    QString context;
    QRectF boundingRect;  // PDF coordinates from Poppler
    int startIndex;
    int length;
    QRectF widgetRect;     // Transformed widget coordinates for highlighting
    bool isCurrentResult;  // Whether this is the currently selected result

    SearchResult()
        : pageNumber(-1), startIndex(-1), length(0), isCurrentResult(false) {}
    SearchResult(int page, const QString& txt, const QString& ctx,
                 const QRectF& rect, int start, int len)
        : pageNumber(page),
          text(txt),
          context(ctx),
          boundingRect(rect),
          startIndex(start),
          length(len),
          isCurrentResult(false) {}

    // Transform PDF coordinates to widget coordinates
    void transformToWidgetCoordinates(double scaleFactor, int rotation,
                                      const QSizeF& pageSize,
                                      const QSize& widgetSize);

    // Check if the result is valid for highlighting
    bool isValidForHighlight() const {
        return pageNumber >= 0 && !boundingRect.isEmpty();
    }
};

/**
 * Search options and parameters
 */
struct SearchOptions {
    bool caseSensitive = false;
    bool wholeWords = false;
    bool useRegex = false;
    bool searchBackward = false;
    int maxResults = 1000;
    QString highlightColor = "#FFFF00";

    SearchOptions() = default;
};

/**
 * Model for managing search results and operations
 */
class SearchModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum SearchRole {
        PageNumberRole = Qt::UserRole + 1,
        TextRole,
        ContextRole,
        BoundingRectRole,
        StartIndexRole,
        LengthRole
    };

    explicit SearchModel(QObject* parent = nullptr);
    ~SearchModel() = default;

    // QAbstractListModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Search operations
    void startSearch(Poppler::Document* document, const QString& query,
                     const SearchOptions& options = SearchOptions());
    void startRealTimeSearch(Poppler::Document* document, const QString& query,
                             const SearchOptions& options = SearchOptions());
    void clearResults();
    void cancelSearch();

    // Result access
    const QList<SearchResult>& getResults() const { return m_results; }
    SearchResult getResult(int index) const;
    int getCurrentResultIndex() const { return m_currentResultIndex; }
    void setCurrentResultIndex(int index);

    // Navigation
    bool hasNext() const;
    bool hasPrevious() const;
    SearchResult nextResult();
    SearchResult previousResult();

    // Search state
    bool isSearching() const { return m_isSearching; }
    const QString& getCurrentQuery() const { return m_currentQuery; }
    const SearchOptions& getCurrentOptions() const { return m_currentOptions; }

signals:
    void searchStarted();
    void searchFinished(int resultCount);
    void searchCancelled();
    void searchError(const QString& error);
    void currentResultChanged(int index);
    void resultsCleared();

    // Real-time search signals
    void realTimeSearchStarted();
    void realTimeResultsUpdated(const QList<SearchResult>& results);
    void realTimeSearchProgress(int currentPage, int totalPages);

private slots:
    void onSearchFinished();

private:
    void performSearch();
    void performRealTimeSearch();
    QList<SearchResult> searchInPage(Poppler::Page* page, int pageNumber,
                                     const QString& query,
                                     const SearchOptions& options);
    QString extractContext(const QString& pageText, int position, int length,
                           int contextLength = 50);
    QRegularExpression createSearchRegex(const QString& query,
                                         const SearchOptions& options);

    QList<SearchResult> m_results;
    int m_currentResultIndex;
    bool m_isSearching;
    QString m_currentQuery;
    SearchOptions m_currentOptions;
    Poppler::Document* m_document;
    QList<SearchResult> m_searchResults;

    QFuture<QList<SearchResult>> m_searchFuture;
    QFutureWatcher<QList<SearchResult>>* m_searchWatcher;

    // Real-time search members
    QTimer* m_realTimeSearchTimer;
    bool m_isRealTimeSearchEnabled;
    int m_realTimeSearchDelay;
};
