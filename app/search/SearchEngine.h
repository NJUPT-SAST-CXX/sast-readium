#pragma once

#include <QObject>
#include <memory>

namespace Poppler {
    class Document;
}

class SearchResult;
class SearchOptions;

/**
 * Main search engine interface with pimpl idiom for clean API
 * Coordinates search operations across PDF documents
 */
class SearchEngine : public QObject
{
    Q_OBJECT

public:
    explicit SearchEngine(QObject* parent = nullptr);
    ~SearchEngine();

    // Document management
    void setDocument(Poppler::Document* document);
    Poppler::Document* document() const;

    // Search operations
    void search(const QString& query, const SearchOptions& options = SearchOptions());
    void searchIncremental(const QString& query, const SearchOptions& options = SearchOptions());
    void cancelSearch();
    void clearResults();

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
