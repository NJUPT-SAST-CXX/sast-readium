#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QString>
#include <QList>
#include <QRegularExpression>
#include <QFuture>
#include <QFutureWatcher>
#include <QTimer>
#include <poppler-qt6.h>
#include "../utils/ErrorHandling.h"

// Forward declaration for optimized search engine
class OptimizedSearchEngine;

/**
 * Represents a single search result with enhanced coordinate transformation support
 */
struct SearchResult {
    int pageNumber;
    QString text;
    QString context;
    QRectF boundingRect;        // PDF coordinates from Poppler
    int startIndex;
    int length;
    QRectF widgetRect;          // Transformed widget coordinates for highlighting
    bool isCurrentResult;       // Whether this is the currently selected result

    SearchResult() : pageNumber(-1), startIndex(-1), length(0), isCurrentResult(false) {}
    SearchResult(int page, const QString& txt, const QString& ctx,
                const QRectF& rect, int start, int len)
        : pageNumber(page), text(txt), context(ctx), boundingRect(rect),
          startIndex(start), length(len), isCurrentResult(false) {}

    // Transform PDF coordinates to widget coordinates
    void transformToWidgetCoordinates(double scaleFactor, int rotation, const QSizeF& pageSize, const QSize& widgetSize);

    // Check if the result is valid for highlighting
    bool isValidForHighlight() const { return pageNumber >= 0 && !boundingRect.isEmpty(); }
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

    // Advanced search features
    bool fuzzySearch = false;
    int fuzzyThreshold = 2; // Maximum edit distance for fuzzy search
    int startPage = -1; // -1 means search all pages
    int endPage = -1;   // -1 means search all pages
    bool searchInSelection = false;
    QRectF selectionRect; // For search within selection

    // Performance options
    bool useIndexedSearch = true;
    bool enableSearchCache = true;
    bool enableIncrementalSearch = true;
    int searchTimeout = 30000; // 30 seconds timeout

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
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Search operations
    void startSearch(Poppler::Document* document, const QString& query, const SearchOptions& options = SearchOptions());
    void startRealTimeSearch(Poppler::Document* document, const QString& query, const SearchOptions& options = SearchOptions());
    void clearResults();
    void cancelSearch();

    // Advanced search operations
    void startFuzzySearch(Poppler::Document* document, const QString& query, const SearchOptions& options = SearchOptions());
    void startPageRangeSearch(Poppler::Document* document, const QString& query, int startPage, int endPage, const SearchOptions& options = SearchOptions());

    // Search history management
    void addToSearchHistory(const QString& query);
    QStringList getSearchHistory() const { return m_searchHistory; }
    void clearSearchHistory();
    void setMaxHistorySize(int size) { m_maxHistorySize = size; }

    // Optimized search operations
    void setOptimizedSearchEnabled(bool enabled);
    bool isOptimizedSearchEnabled() const { return m_optimizedSearchEnabled; }
    OptimizedSearchEngine* getOptimizedSearchEngine() const { return m_optimizedSearchEngine; }

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

    // Public methods for testing
    int calculateLevenshteinDistance(const QString& str1, const QString& str2);
    bool isFuzzyMatch(const QString& text, const QString& query, int threshold);

signals:
    void searchStarted();
    void searchFinished(int resultCount);
    void searchCancelled();
    void searchError(const QString& error);
    void currentResultChanged(int index);
    void resultsCleared();
    void searchProgress(int currentPage, int totalPages);

    // Real-time search signals
    void realTimeSearchStarted();
    void realTimeResultsUpdated(const QList<SearchResult>& results);
    void realTimeSearchProgress(int currentPage, int totalPages);

private slots:
    void onSearchFinished();
    void onOptimizedSearchFinished(const QList<SearchResult>& results);

private:
    void performSearch();
    void performRealTimeSearch();
    QList<SearchResult> searchInPage(Poppler::Page* page, int pageNumber,
                                   const QString& query, const SearchOptions& options);
    QString extractContext(const QString& pageText, int position, int length, int contextLength = 50);
    QRegularExpression createSearchRegex(const QString& query, const SearchOptions& options);

    // Advanced search algorithms
    QList<SearchResult> performFuzzySearch(const QString& query, const SearchOptions& options);
    QList<SearchResult> performPageRangeSearch(const QString& query, int startPage, int endPage, const SearchOptions& options);

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

    // Optimized search members
    OptimizedSearchEngine* m_optimizedSearchEngine;
    bool m_optimizedSearchEnabled;

    // Search history and navigation
    QStringList m_searchHistory;
    int m_maxHistorySize;
};
