/**
 * @file PageTextCache.h
 * @brief Dedicated cache for page text extraction results
 * @author SAST Team
 * @version 1.0
 * @date 2024
 *
 * This file contains the PageTextCache class which provides specialized
 * caching for extracted text content from PDF pages. It implements the
 * ICacheComponent interface for integration with the unified cache
 * management system.
 *
 * The cache optimizes text extraction performance by storing previously
 * extracted text content and provides efficient retrieval mechanisms
 * with LRU eviction policies.
 *
 * @copyright Copyright (c) 2024 SAST Team
 */

#pragma once

#include <QDateTime>
#include <QHash>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QStringList>
#include "CacheManager.h"

/**
 * @brief Dedicated cache for page text extraction results
 *
 * The PageTextCache class provides specialized caching for extracted text
 * content from PDF pages. It implements the ICacheComponent interface for
 * integration with the unified cache management system provided by
 * CacheManager.
 *
 * This cache optimizes text extraction performance by storing previously
 * extracted text content and provides efficient retrieval mechanisms with
 * LRU eviction policies and memory usage tracking.
 *
 * @see ICacheComponent
 * @see CacheManager
 * @since 1.0
 */
class PageTextCache : public QObject, public ICacheComponent {
    Q_OBJECT

public:
    /**
     * @brief Cache entry structure for page text data
     *
     * Contains the cached text content along with metadata for
     * cache management and access tracking.
     */
    struct CacheEntry {
        QString text;          ///< Extracted text content
        QString documentId;    ///< Document identifier
        int pageNumber{-1};    ///< Page number
        qint64 timestamp{0};   ///< Creation timestamp in milliseconds
        int accessCount{0};    ///< Number of times accessed
        qint64 memorySize{0};  ///< Memory size in bytes

        /**
         * @brief Default constructor
         *
         * Initializes entry with default values.
         */
        CacheEntry() = default;
    };

    /**
     * @brief Constructs a new PageTextCache
     * @param parent Parent QObject for memory management
     */
    explicit PageTextCache(QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~PageTextCache() override;

    // Cache operations
    /**
     * @brief Checks if page text is cached
     * @param documentId Document identifier
     * @param pageNumber Page number
     * @return true if text is cached, false otherwise
     */
    [[nodiscard]] bool hasPageText(const QString& documentId,
                                   int pageNumber) const;

    /**
     * @brief Retrieves cached page text
     * @param documentId Document identifier
     * @param pageNumber Page number
     * @return Cached text content or empty string if not found
     */
    QString getPageText(const QString& documentId, int pageNumber);

    /**
     * @brief Stores page text in the cache
     * @param documentId Document identifier
     * @param pageNumber Page number
     * @param text Text content to cache
     */
    void storePageText(const QString& documentId, int pageNumber,
                       const QString& text);

    /**
     * @brief Invalidates all cached text for a document
     * @param documentId Document identifier
     */
    void invalidateDocument(const QString& documentId);

    /**
     * @brief Clears all cached text entries
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
     * @brief Gets the maximum cache size
     * @return Maximum number of cache entries
     */
    int getMaxCacheSize() const;

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

    // ICacheComponent interface implementation
    /**
     * @brief Gets current memory usage
     * @return Memory usage in bytes
     */
    qint64 getMemoryUsage() const override;

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
     * @brief Resets cache statistics
     */
    void resetStatistics() override;

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
     * @param documentId Document identifier
     * @param pageNumber Page number
     */
    void cacheHit(const QString& documentId, int pageNumber);

    /**
     * @brief Emitted on cache miss
     * @param documentId Document identifier
     * @param pageNumber Page number
     */
    void cacheMiss(const QString& documentId, int pageNumber);

private:
    class Implementation;
    std::unique_ptr<Implementation> d;

    // Default values
    static const int DEFAULT_MAX_CACHE_SIZE = 200;
    static const qint64 DEFAULT_MAX_MEMORY_USAGE = 50 * 1024 * 1024;
};

/**
 * @brief Adapter class to integrate TextExtractor's internal cache with unified
 * cache management
 *
 * This adapter class provides a bridge between the TextExtractor's internal
 * caching mechanism and the unified cache management system. It implements
 * the ICacheComponent interface to allow the TextExtractor cache to be
 * managed alongside other cache types.
 *
 * @see ICacheComponent
 * @see TextExtractor
 * @since 1.0
 */
class TextExtractorCacheAdapter : public QObject, public ICacheComponent {
    Q_OBJECT

public:
    /**
     * @brief Constructs a new TextExtractorCacheAdapter
     * @param textExtractor Pointer to the TextExtractor to wrap
     * @param parent Parent QObject for memory management
     */
    explicit TextExtractorCacheAdapter(class TextExtractor* textExtractor,
                                       QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~TextExtractorCacheAdapter() override;

    // ICacheComponent interface implementation
    qint64 getMemoryUsage() const override;
    qint64 getMaxMemoryLimit() const override;
    void setMaxMemoryLimit(qint64 limit) override;
    void clear() override;
    int getEntryCount() const override;
    void evictLRU(qint64 bytesToFree) override;
    qint64 getHitCount() const override;
    qint64 getMissCount() const override;
    void resetStatistics() override;
    void setEnabled(bool enabled) override;
    bool isEnabled() const override;

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};
