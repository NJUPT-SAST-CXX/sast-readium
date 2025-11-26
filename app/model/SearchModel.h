#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QAbstractListModel>
#include <QFuture>
#include <QFutureWatcher>
#include <QList>
#include <QObject>
#include <QRegularExpression>
#include <QString>
#include <QTimer>
#include "../search/SearchConfiguration.h"
#include "../utils/ErrorHandling.h"

// SearchResult and SearchOptions now defined in SearchConfiguration.h

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
    ~SearchModel() override = default;

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

    // Advanced search operations
    void startFuzzySearch(Poppler::Document* document, const QString& query,
                          const SearchOptions& options = SearchOptions());
    void startPageRangeSearch(Poppler::Document* document, const QString& query,
                              int startPage, int endPage,
                              const SearchOptions& options = SearchOptions());

    // Search history management
    void addToSearchHistory(const QString& query);
    QStringList getSearchHistory() const { return m_searchHistory; }
    void clearSearchHistory();
    void setMaxHistorySize(int size) { m_maxHistorySize = size; }

    // Advanced search operations
    void setAdvancedSearchEnabled(bool enabled);
    bool isAdvancedSearchEnabled() const { return m_advancedSearchEnabled; }

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
    void onAdvancedSearchFinished(const QList<SearchResult>& results);

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

    // Advanced search algorithms
    QList<SearchResult> performFuzzySearch(const QString& query,
                                           const SearchOptions& options);
    QList<SearchResult> performPageRangeSearch(const QString& query,
                                               int startPage, int endPage,
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

    // Advanced search members
    bool m_advancedSearchEnabled;

    // Search history and navigation
    QStringList m_searchHistory;
    int m_maxHistorySize;
};
