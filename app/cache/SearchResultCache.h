/**
 * @file SearchResultCache.h
 * @brief LRU Cache for search results to avoid repeated searches
 * @author SAST Team
 * @version 1.0
 * @date 2024
 *
 * This file contains the SearchResultCache class which provides an LRU
 * (Least Recently Used) cache for search results to avoid repeated searches
 * and significantly improve performance for repeated queries. It implements
 * the ICacheComponent interface for unified cache management.
 *
 * The cache supports incremental search optimization, expiration policies,
 * and sophisticated cache key generation based on search parameters and
 * document state.
 *
 * @copyright Copyright (c) 2024 SAST Team
 */

#pragma once

#include <QCryptographicHash>
#include <QDateTime>
#include <QHash>
#include <QMutex>
#include <QObject>
#include <QTimer>
#include "../model/SearchModel.h"
#include "CacheManager.h"

/**
 * @brief LRU Cache for search results to avoid repeated searches
 *
 * The SearchResultCache class provides an LRU (Least Recently Used) cache
 * for search results to avoid repeated searches and significantly improve
 * performance for repeated queries. It implements the ICacheComponent
 * interface for unified cache management.
 *
 * Key features:
 * - LRU eviction policy for optimal memory usage
 * - Incremental search support for progressive queries
 * - Cache key generation based on search parameters and document state
 * - Expiration policies for cache freshness
 * - Memory usage tracking and limits
 * - Performance statistics and monitoring
 *
 * @see ICacheComponent
 * @see CacheManager
 * @see SearchModel
 * @since 1.0
 */
class SearchResultCache : public QObject, public ICacheComponent {
    Q_OBJECT

public:
    /**
     * @brief Cache key structure for search results
     *
     * Contains all parameters that uniquely identify a search operation
     * including query text, search options, document information, and
     * modification timestamp for cache invalidation.
     */
    struct CacheKey {
        QString query;  ///< Search query text
        SearchOptions
            options;         ///< Search options (case sensitivity, regex, etc.)
        QString documentId;  ///< Document identifier
        qint64 documentModified;  ///< Document modification timestamp

        /**
         * @brief Generates a hash string for the cache key
         * @return MD5 hash of the combined key parameters
         *
         * Creates a unique hash by combining all key parameters to ensure
         * that different search configurations produce different cache keys.
         */
        QString toHash() const {
            QString combined = QString("%1|%2|%3|%4|%5|%6|%7")
                                   .arg(query)
                                   .arg(options.caseSensitive ? "1" : "0")
                                   .arg(options.wholeWords ? "1" : "0")
                                   .arg(options.useRegex ? "1" : "0")
                                   .arg(options.searchBackward ? "1" : "0")
                                   .arg(documentId)
                                   .arg(documentModified);
            return QCryptographicHash::hash(combined.toUtf8(),
                                            QCryptographicHash::Md5)
                .toHex();
        }

        /**
         * @brief Equality operator for cache key comparison
         * @param other Other cache key to compare with
         * @return true if keys are equal, false otherwise
         */
        bool operator==(const CacheKey& other) const {
            return query == other.query &&
                   options.caseSensitive == other.options.caseSensitive &&
                   options.wholeWords == other.options.wholeWords &&
                   options.useRegex == other.options.useRegex &&
                   options.searchBackward == other.options.searchBackward &&
                   documentId == other.documentId &&
                   documentModified == other.documentModified;
        }
    };

    /**
     * @brief Cache entry structure for search results
     *
     * Contains the cached search results along with metadata for
     * cache management, access tracking, and memory usage monitoring.
     */
    struct CacheEntry {
        QList<SearchResult> results;  ///< Cached search results
        qint64 timestamp;             ///< Creation timestamp in milliseconds
        qint64 accessCount;           ///< Number of times accessed
        qint64 memorySize;            ///< Memory size in bytes
        QString queryHash;   ///< Hash of the search query for quick lookup
        QString documentId;  ///< Document ID for invalidation

        /**
         * @brief Default constructor
         *
         * Initializes entry with default values.
         */
        CacheEntry() : timestamp(0), accessCount(0), memorySize(0) {}
    };

    /**
     * @brief Constructs a new SearchResultCache
     * @param parent Parent QObject for memory management
     */
    explicit SearchResultCache(QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~SearchResultCache();

    // Cache operations
    /**
     * @brief Checks if search results are cached for the given key
     * @param key Cache key to check
     * @return true if results are cached, false otherwise
     */
    bool hasResults(const CacheKey& key) const;

    /**
     * @brief Retrieves cached search results
     * @param key Cache key for the search
     * @return Cached search results or empty list if not found
     */
    QList<SearchResult> getResults(const CacheKey& key);

    /**
     * @brief Stores search results in the cache
     * @param key Cache key for the search
     * @param results Search results to cache
     */
    void storeResults(const CacheKey& key, const QList<SearchResult>& results);

    /**
     * @brief Invalidates all cached results for a document
     * @param documentId Document identifier
     */
    void invalidateDocument(const QString& documentId);

    /**
     * @brief Clears all cached search results
     */
    void clear() override;

    // Configuration
    /**
     * @brief Sets the maximum number of cache entries
     * @param maxEntries Maximum number of entries
     */
    void setMaxCacheSize(int maxEntries);

    /**
     * @brief Sets the maximum memory usage
     * @param maxBytes Maximum memory usage in bytes
     */
    void setMaxMemoryUsage(qint64 maxBytes);

    /**
     * @brief Sets the expiration time for cache entries
     * @param milliseconds Expiration time in milliseconds
     */
    void setExpirationTime(qint64 milliseconds);

    // Statistics
    /**
     * @brief Gets the current cache size
     * @return Number of cached entries
     */
    int getCacheSize() const;

    /**
     * @brief Gets current memory usage
     * @return Memory usage in bytes
     */
    qint64 getMemoryUsage() const override;

    /**
     * @brief Gets the cache hit ratio
     * @return Hit ratio as a value between 0.0 and 1.0
     */
    double getHitRatio() const;

    /**
     * @brief Resets cache statistics
     */
    void resetStatistics() override;

    // Incremental search support
    /**
     * @brief Checks if incremental search can be used
     * @param newKey New search key
     * @param previousKey Previous search key
     * @return true if incremental search is possible, false otherwise
     *
     * Determines if the new search can reuse results from a previous
     * search by filtering or extending existing results.
     */
    bool canUseIncrementalSearch(const CacheKey& newKey,
                                 const CacheKey& previousKey) const;

    /**
     * @brief Gets incremental search results
     * @param newKey New search key
     * @param previousKey Previous search key
     * @return Filtered search results for incremental search
     */
    QList<SearchResult> getIncrementalResults(const CacheKey& newKey,
                                              const CacheKey& previousKey);

    // ICacheComponent interface implementation
    /**
     * @brief Gets maximum memory limit
     * @return Maximum memory limit in bytes
     */
    qint64 getMaxMemoryLimit() const override;

    /**
     * @brief Sets maximum memory limit
     * @param limit Maximum memory limit in bytes
     */
    void setMaxMemoryLimit(qint64 limit) override;

    /**
     * @brief Gets number of cache entries
     * @return Number of cache entries
     */
    int getEntryCount() const override;

    /**
     * @brief Evicts least recently used entries
     * @param bytesToFree Number of bytes to free
     */
    void evictLRU(qint64 bytesToFree) override;

    /**
     * @brief Gets total cache hits
     * @return Total cache hits
     */
    qint64 getHitCount() const override;

    /**
     * @brief Gets total cache misses
     * @return Total cache misses
     */
    qint64 getMissCount() const override;

    /**
     * @brief Enables or disables the cache
     * @param enabled true to enable, false to disable
     */
    void setEnabled(bool enabled) override;

    /**
     * @brief Checks if cache is enabled
     * @return true if enabled, false otherwise
     */
    bool isEnabled() const override;

signals:
    /**
     * @brief Emitted when cache is updated
     * @param size Current cache size
     * @param memoryUsage Current memory usage in bytes
     */
    void cacheUpdated(int size, qint64 memoryUsage);

    /**
     * @brief Emitted on cache hit
     * @param queryHash Hash of the search query
     */
    void cacheHit(const QString& queryHash);

    /**
     * @brief Emitted on cache miss
     * @param queryHash Hash of the search query
     */
    void cacheMiss(const QString& queryHash);

private slots:
    /**
     * @brief Performs periodic cache maintenance
     *
     * Removes expired entries and optimizes cache performance.
     * Called automatically by the maintenance timer.
     */
    void performMaintenance();

private:
    class Implementation;
    std::unique_ptr<Implementation> d;

    // Default values
    static const int DEFAULT_MAX_CACHE_SIZE = 100;
    static const qint64 DEFAULT_MAX_MEMORY_USAGE = 64 * 1024 * 1024;
    static const qint64 DEFAULT_EXPIRATION_TIME = 30 * 60 * 1000;
    static const int MAINTENANCE_INTERVAL = 5 * 60 * 1000;
};

/**
 * @brief Cache for search highlight rendering data
 *
 * Provides caching for search highlight rendering information including
 * bounding rectangles, colors, and access tracking. This cache improves
 * performance when displaying search results with highlighting.
 */
class SearchHighlightCache : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Highlight data structure
     *
     * Contains rendering information for search result highlights
     * including bounding rectangles and visual properties.
     */
    struct HighlightData {
        QList<QRectF> boundingRects;  ///< Bounding rectangles for highlights
        QString highlightColor;       ///< Highlight color specification
        qint64 timestamp;             ///< Creation timestamp in milliseconds
        qint64 accessCount;           ///< Number of times accessed

        /**
         * @brief Default constructor
         *
         * Initializes highlight data with default values.
         */
        HighlightData() : timestamp(0), accessCount(0) {}
    };

    /**
     * @brief Constructs a new SearchHighlightCache
     * @param parent Parent QObject for memory management
     */
    explicit SearchHighlightCache(QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~SearchHighlightCache();

    // Cache operations
    /**
     * @brief Checks if highlight data is cached
     * @param documentId Document identifier
     * @param pageNumber Page number
     * @param query Search query
     * @return true if highlight data is cached, false otherwise
     */
    bool hasHighlightData(const QString& documentId, int pageNumber,
                          const QString& query) const;

    /**
     * @brief Retrieves cached highlight data
     * @param documentId Document identifier
     * @param pageNumber Page number
     * @param query Search query
     * @return Cached highlight data or default-constructed data if not found
     */
    HighlightData getHighlightData(const QString& documentId, int pageNumber,
                                   const QString& query);

    /**
     * @brief Stores highlight data in the cache
     * @param documentId Document identifier
     * @param pageNumber Page number
     * @param query Search query
     * @param data Highlight data to cache
     */
    void storeHighlightData(const QString& documentId, int pageNumber,
                            const QString& query, const HighlightData& data);

    /**
     * @brief Invalidates all cached highlight data for a document
     * @param documentId Document identifier
     */
    void invalidateDocument(const QString& documentId);

    /**
     * @brief Clears all cached highlight data
     */
    void clear();

    // Configuration
    /**
     * @brief Sets the maximum number of cache entries
     * @param maxEntries Maximum number of entries
     */
    void setMaxCacheSize(int maxEntries);

    // Statistics
    /**
     * @brief Gets the current cache size
     * @return Number of cached entries
     */
    int getCacheSize() const;

    /**
     * @brief Gets the cache hit ratio
     * @return Hit ratio as a value between 0.0 and 1.0
     */
    double getHitRatio() const;

signals:
    /**
     * @brief Emitted when cache is updated
     * @param size Current cache size
     */
    void cacheUpdated(int size);

private:
    class Implementation;
    std::unique_ptr<Implementation> d;

    // Default values
    static const int DEFAULT_MAX_CACHE_SIZE = 200;
};
