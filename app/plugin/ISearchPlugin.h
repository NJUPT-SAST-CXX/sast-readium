#pragma once

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QRect>
#include <QString>
#include <QVariant>
#include <QVariantMap>

/**
 * @brief Search Result Ranking Strategy
 */
enum class SearchRankingStrategy {
    Frequency,  // Rank by term frequency
    Position,   // Rank by position in document
    Relevance,  // Rank by relevance score
    Custom      // Custom ranking algorithm
};

/**
 * @brief Search Result
 *
 * Represents a single search result with ranking information.
 */
struct PluginSearchResult {
    QString text;           // Matched text
    int pageNumber;         // Page number (0-based)
    QRect boundingRect;     // Bounding rectangle on page
    double relevanceScore;  // Relevance score (0.0-1.0)
    QVariantMap metadata;   // Additional metadata

    PluginSearchResult() : pageNumber(-1), relevanceScore(0.0) {}
};

/**
 * @brief ISearchPlugin - Interface for search enhancement plugins
 *
 * Plugins implementing this interface can provide custom search algorithms,
 * result post-processing, and index optimization.
 */
class ISearchPlugin {
public:
    virtual ~ISearchPlugin() = default;

    /**
     * @brief Get search algorithm name
     */
    virtual QString algorithmName() const = 0;

    /**
     * @brief Check if plugin can handle this search query
     * @param query The search query
     * @param options Search options
     * @return True if plugin can handle this query
     */
    virtual bool canHandleQuery(const QString& query,
                                const QJsonObject& options) const = 0;

    /**
     * @brief Execute custom search algorithm
     * @param query The search query
     * @param documentPath Document to search in
     * @param options Search options
     * @return List of search results
     */
    virtual QList<PluginSearchResult> executeSearch(
        const QString& query, const QString& documentPath,
        const QJsonObject& options) = 0;

    /**
     * @brief Post-process search results
     * @param results Input search results
     * @param query Original query
     * @param strategy Ranking strategy to apply
     * @return Processed and ranked results
     */
    virtual QList<PluginSearchResult> postProcessResults(
        const QList<PluginSearchResult>& results, const QString& query,
        SearchRankingStrategy strategy) = 0;

    /**
     * @brief Build search index for document
     * @param documentPath Document path
     * @param options Indexing options
     * @return True if index was built successfully
     */
    virtual bool buildSearchIndex(const QString& documentPath,
                                  const QJsonObject& options) = 0;

    /**
     * @brief Get index size for document
     * @param documentPath Document path
     * @return Index size in bytes
     */
    virtual qint64 getIndexSize(const QString& documentPath) const = 0;

    /**
     * @brief Clear search index for document
     * @param documentPath Document path
     */
    virtual void clearIndex(const QString& documentPath) = 0;
};

Q_DECLARE_INTERFACE(ISearchPlugin, "com.sast.readium.ISearchPlugin/1.0")
