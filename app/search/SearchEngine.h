#pragma once

#include <QObject>
#include <memory>
#include "SearchConfiguration.h"

// Forward declarations for advanced features
class SearchFeatures;

namespace Poppler {
class Document;
}

/**
 * Main search engine interface with pimpl idiom for clean API
 * Coordinates search operations across PDF documents
 */
class SearchEngine : public QObject {
    Q_OBJECT

public:
    explicit SearchEngine(QObject* parent = nullptr);
    ~SearchEngine();

    // Document management
    void setDocument(Poppler::Document* document);
    // Safe accessors
    Poppler::Document* document() const;

    // Search operations
    void search(const QString& query,
                const SearchOptions& options = SearchOptions());
    void searchIncremental(const QString& query,
                           const SearchOptions& options = SearchOptions());
    void cancelSearch();
    void clearResults();

    // Synchronous search operations for testing compatibility
    void startSearch(Poppler::Document* document, const QString& query,
                     const SearchOptions& options = SearchOptions());
    QList<SearchResult> getResults() const;

    // Advanced search operations
    void fuzzySearch(const QString& query, int maxDistance = 2,
                     const SearchOptions& options = SearchOptions());
    void wildcardSearch(const QString& pattern,
                        const SearchOptions& options = SearchOptions());
    void phraseSearch(const QString& phrase, int proximity = 0,
                      const SearchOptions& options = SearchOptions());
    void booleanSearch(const QString& query,
                       const SearchOptions& options = SearchOptions());
    void proximitySearch(const QStringList& terms, int maxDistance = 10,
                         bool ordered = false,
                         const SearchOptions& options = SearchOptions());

    // Configuration
    void setCacheEnabled(bool enabled);
    bool isCacheEnabled() const;

    void setIncrementalSearchEnabled(bool enabled);
    bool isIncrementalSearchEnabled() const;

    void setBackgroundProcessingEnabled(bool enabled);
    bool isBackgroundProcessingEnabled() const;

    // Results access
    QList<SearchResult> results() const;
    int resultCount() const;
    bool isSearching() const;
    QString currentQuery() const;

    // Performance metrics
    double cacheHitRatio() const;
    qint64 cacheMemoryUsage() const;
    void resetStatistics();

    // Advanced features access
    SearchFeatures* advancedFeatures() const;
    void setHighlightColors(const QColor& normalColor,
                            const QColor& currentColor);
    QStringList getSearchSuggestions(const QString& partialQuery,
                                     int maxSuggestions = 5);
    QStringList getSearchHistory(int maxEntries = 10);
    void clearSearchHistory();

signals:
    void searchStarted();
    void searchFinished(const QList<SearchResult>& results);
    void searchProgress(int currentPage, int totalPages);
    void searchCancelled();
    void searchError(const QString& error);
    void resultsUpdated(const QList<SearchResult>& results);

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};
