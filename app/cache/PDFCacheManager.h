/**
 * @file PDFCacheManager.h
 * @brief PDF cache manager with intelligent caching strategies
 * @author SAST Team
 * @version 1.0
 * @date 2024
 *
 * This file contains the PDFCacheManager class and related structures for
 * managing PDF-specific caching operations. It provides intelligent caching
 * strategies for rendered pages, thumbnails, text content, and other PDF
 * data with support for preloading, background operations, and cache
 * optimization.
 *
 * The cache manager supports multiple cache item types with different
 * priorities and implements sophisticated eviction policies to optimize
 * memory usage and performance.
 *
 * @copyright Copyright (c) 2024 SAST Team
 */

#pragma once

#include <poppler-qt6.h>
#include <QCache>
#include <QElapsedTimer>
#include <QMutex>
#include <QObject>
#include <QPixmap>
#include <QQueue>
#include <QRunnable>
#include <QSet>
#include <QSettings>
#include <QThread>
#include <QThreadPool>
#include <QTimer>

/**
 * @brief Enumeration of cache item types
 *
 * Defines the different types of data that can be cached by the
 * PDFCacheManager. Each type has specific characteristics and
 * memory usage patterns.
 */
enum class CacheItemType {
    RenderedPage,   ///< Rendered page pixmap for display
    Thumbnail,      ///< Page thumbnail for navigation
    TextContent,    ///< Extracted text content for search
    PageImage,      ///< Raw page image data
    SearchResults,  ///< Search result data and highlights
    Annotations     ///< Page annotations and markup
};

/**
 * @brief Cache priority levels for eviction policies
 *
 * Defines priority levels that influence cache eviction decisions.
 * Higher priority items are kept longer in the cache.
 */
enum class CachePriority {
    Low,      ///< Can be evicted first when memory is needed
    Normal,   ///< Standard priority for most cache items
    High,     ///< Keep longer, evict only under pressure
    Critical  ///< Never evict automatically, manual removal only
};

/**
 * @brief Cached item wrapper with metadata
 *
 * Contains the cached data along with metadata for cache management
 * including access tracking, priority, and memory usage information.
 */
struct CacheItem {
    QVariant data;           ///< The actual cached data
    CacheItemType type;      ///< Type of cached item
    CachePriority priority;  ///< Priority level for eviction
    qint64 timestamp;        ///< Creation timestamp in milliseconds
    qint64 accessCount;      ///< Number of times accessed
    qint64 lastAccessed;     ///< Last access timestamp in milliseconds
    int pageNumber;          ///< Associated page number (-1 if not applicable)
    QString key;             ///< Unique cache key
    qint64 memorySize;       ///< Memory size in bytes

    /**
     * @brief Default constructor
     *
     * Initializes a cache item with default values and current timestamp.
     */
    CacheItem()
        : type(CacheItemType::RenderedPage),
          priority(CachePriority::Normal),
          timestamp(QDateTime::currentMSecsSinceEpoch()),
          accessCount(0),
          lastAccessed(0),
          pageNumber(-1),
          memorySize(0) {}

    /**
     * @brief Updates access information
     *
     * Increments access count and updates last accessed timestamp.
     * Used for LRU tracking and access pattern analysis.
     */
    void updateAccess() {
        accessCount++;
        lastAccessed = QDateTime::currentMSecsSinceEpoch();
    }

    /**
     * @brief Calculates the memory size of the cached item
     * @return Memory size in bytes
     */
    qint64 calculateSize() const;

    /**
     * @brief Checks if the item has expired
     * @param maxAge Maximum age in milliseconds
     * @return true if expired, false otherwise
     */
    bool isExpired(qint64 maxAge) const;
};

/**
 * @brief Cache statistics structure
 *
 * Contains comprehensive statistics about cache performance and usage
 * patterns. Used for monitoring, optimization, and debugging purposes.
 */
struct CacheStatistics {
    int totalItems;            ///< Total number of cached items
    qint64 totalMemoryUsage;   ///< Total memory usage in bytes
    qint64 hitCount;           ///< Total number of cache hits
    qint64 missCount;          ///< Total number of cache misses
    double hitRate;            ///< Cache hit rate (0.0 to 1.0)
    int itemsByType[6];        ///< Number of items by type (one for each
                               ///< CacheItemType)
    qint64 averageAccessTime;  ///< Average access time in milliseconds
    qint64 oldestItemAge;      ///< Age of oldest item in milliseconds
    qint64 newestItemAge;      ///< Age of newest item in milliseconds

    /**
     * @brief Default constructor
     *
     * Initializes all statistics to zero or default values.
     */
    CacheStatistics()
        : totalItems(0),
          totalMemoryUsage(0),
          hitCount(0),
          missCount(0),
          hitRate(0.0),
          averageAccessTime(0),
          oldestItemAge(0),
          newestItemAge(0) {
        for (int i = 0; i < 6; ++i) {
            itemsByType[i] = 0;
        }
    }
};

/**
 * @brief Preloading task for background cache population
 *
 * A QRunnable task that performs background preloading of cache items
 * to improve user experience by preparing data before it's needed.
 * Runs in a separate thread to avoid blocking the UI.
 */
class PreloadTask : public QRunnable {
public:
    /**
     * @brief Constructs a preload task
     * @param document Pointer to the PDF document
     * @param pageNumber Page number to preload
     * @param type Type of cache item to preload
     * @param target Target object to notify when complete
     */
    PreloadTask(Poppler::Document* document, int pageNumber, CacheItemType type,
                QObject* target);

    /**
     * @brief Executes the preload task
     *
     * This method is called by the thread pool to perform the actual
     * preloading operation in the background.
     */
    void run() override;

private:
    Poppler::Document* m_document;  ///< PDF document to preload from
    int m_pageNumber;               ///< Page number to preload
    CacheItemType m_type;           ///< Type of item to preload
    QObject* m_target;  ///< Target object for completion notification
};

/**
 * @brief PDF cache manager with intelligent caching strategies
 *
 * The PDFCacheManager provides sophisticated caching for PDF-related data
 * including rendered pages, thumbnails, text content, and search results.
 * It implements intelligent caching strategies with configurable eviction
 * policies, preloading capabilities, and performance optimization.
 *
 * Key features:
 * - Multiple cache item types with different priorities
 * - Background preloading for improved performance
 * - Configurable eviction policies (LRU, LFU, etc.)
 * - Memory usage tracking and limits
 * - Cache statistics and monitoring
 * - Settings persistence
 *
 * @see CacheItem
 * @see CacheItemType
 * @see CachePriority
 * @since 1.0
 */
class PDFCacheManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructs a new PDFCacheManager
     * @param parent Parent QObject for memory management
     */
    explicit PDFCacheManager(QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~PDFCacheManager();

    // Cache configuration
    /**
     * @brief Sets the maximum memory usage for the cache
     * @param bytes Maximum memory usage in bytes
     */
    void setMaxMemoryUsage(qint64 bytes);

    /**
     * @brief Gets the maximum memory usage limit
     * @return Maximum memory usage in bytes
     */
    qint64 getMaxMemoryUsage() const;

    /**
     * @brief Sets the maximum number of items in the cache
     * @param count Maximum number of cache items
     */
    void setMaxItems(int count);

    /**
     * @brief Gets the maximum number of items allowed
     * @return Maximum number of cache items
     */
    int getMaxItems() const;

    /**
     * @brief Sets the maximum age for cache items
     * @param milliseconds Maximum age in milliseconds
     */
    void setItemMaxAge(qint64 milliseconds);

    /**
     * @brief Gets the maximum age for cache items
     * @return Maximum age in milliseconds
     */
    qint64 getItemMaxAge() const;

    // Cache operations
    /**
     * @brief Inserts an item into the cache
     * @param key Unique cache key
     * @param data Data to cache
     * @param type Type of cache item
     * @param priority Priority level for eviction
     * @param pageNumber Associated page number (-1 if not applicable)
     * @return true if successfully inserted, false otherwise
     */
    bool insert(const QString& key, const QVariant& data, CacheItemType type,
                CachePriority priority = CachePriority::Normal,
                int pageNumber = -1);

    /**
     * @brief Retrieves an item from the cache
     * @param key Cache key to retrieve
     * @return Cached data or invalid QVariant if not found
     */
    QVariant get(const QString& key);

    /**
     * @brief Checks if a key exists in the cache
     * @param key Cache key to check
     * @return true if key exists, false otherwise
     */
    bool contains(const QString& key) const;

    /**
     * @brief Removes an item from the cache
     * @param key Cache key to remove
     * @return true if successfully removed, false if key not found
     */
    bool remove(const QString& key);

    /**
     * @brief Clears all items from the cache
     */
    void clear();

    // Specialized cache operations
    /**
     * @brief Caches a rendered page pixmap
     * @param pageNumber Page number
     * @param pixmap Rendered page pixmap
     * @param scaleFactor Scale factor used for rendering
     * @return true if successfully cached, false otherwise
     */
    bool cacheRenderedPage(int pageNumber, const QPixmap& pixmap,
                           double scaleFactor);

    /**
     * @brief Retrieves a rendered page pixmap
     * @param pageNumber Page number
     * @param scaleFactor Scale factor
     * @return Cached pixmap or null pixmap if not found
     */
    QPixmap getRenderedPage(int pageNumber, double scaleFactor);

    /**
     * @brief Caches a page thumbnail
     * @param pageNumber Page number
     * @param thumbnail Thumbnail pixmap
     * @return true if successfully cached, false otherwise
     */
    bool cacheThumbnail(int pageNumber, const QPixmap& thumbnail);

    /**
     * @brief Retrieves a page thumbnail
     * @param pageNumber Page number
     * @return Cached thumbnail or null pixmap if not found
     */
    QPixmap getThumbnail(int pageNumber);

    /**
     * @brief Caches extracted text content for a page
     * @param pageNumber Page number
     * @param text Extracted text content
     * @return true if successfully cached, false otherwise
     */
    bool cacheTextContent(int pageNumber, const QString& text);

    /**
     * @brief Retrieves cached text content for a page
     * @param pageNumber Page number
     * @return Cached text content or empty string if not found
     */
    QString getTextContent(int pageNumber);

    // Preloading and background operations
    /**
     * @brief Enables or disables background preloading
     * @param enabled true to enable preloading, false to disable
     */
    void enablePreloading(bool enabled);

    /**
     * @brief Checks if preloading is enabled
     * @return true if preloading is enabled, false otherwise
     */
    bool isPreloadingEnabled() const;

    /**
     * @brief Preloads specific pages in the background
     * @param pageNumbers List of page numbers to preload
     * @param type Type of cache items to preload
     */
    void preloadPages(const QList<int>& pageNumbers, CacheItemType type);

    /**
     * @brief Preloads pages around a center page
     * @param centerPage Center page number
     * @param radius Number of pages to preload on each side (default: 2)
     */
    void preloadAroundPage(int centerPage, int radius = 2);

    /**
     * @brief Sets the preloading strategy
     * @param strategy Preloading strategy name (e.g., "sequential", "adaptive")
     */
    void setPreloadingStrategy(const QString& strategy);

    // Cache management
    /**
     * @brief Optimizes cache performance and memory usage
     */
    void optimizeCache();

    /**
     * @brief Removes expired cache items
     */
    void cleanupExpiredItems();

    /**
     * @brief Evicts least used cache items
     * @param count Number of items to evict
     * @return true if items were evicted, false otherwise
     */
    bool evictLeastUsedItems(int count);

    /**
     * @brief Compacts the cache to reduce memory fragmentation
     */
    void compactCache();

    // Statistics and monitoring
    /**
     * @brief Gets comprehensive cache statistics
     * @return Cache statistics structure
     */
    CacheStatistics getStatistics() const;

    /**
     * @brief Gets current memory usage
     * @return Current memory usage in bytes
     */
    qint64 getCurrentMemoryUsage() const;

    /**
     * @brief Gets cache hit rate
     * @return Hit rate as a value between 0.0 and 1.0
     */
    double getHitRate() const;

    /**
     * @brief Resets all cache statistics
     */
    void resetStatistics();

    // Cache policies
    /**
     * @brief Sets the cache eviction policy
     * @param policy Eviction policy name (e.g., "LRU", "LFU", "FIFO")
     */
    void setEvictionPolicy(const QString& policy);

    /**
     * @brief Gets the current eviction policy
     * @return Current eviction policy name
     */
    QString getEvictionPolicy() const;

    /**
     * @brief Sets priority weights for eviction scoring
     * @param lowWeight Weight for low priority items
     * @param normalWeight Weight for normal priority items
     * @param highWeight Weight for high priority items
     */
    void setPriorityWeights(double lowWeight, double normalWeight,
                            double highWeight);

    // Settings persistence
    /**
     * @brief Loads cache settings from persistent storage
     */
    void loadSettings();

    /**
     * @brief Saves cache settings to persistent storage
     */
    void saveSettings();

    // Additional utility functions
    /**
     * @brief Exports cache data to a file
     * @param filePath Path to the export file
     * @return true if export was successful, false otherwise
     */
    bool exportCacheToFile(const QString& filePath) const;

    /**
     * @brief Imports cache data from a file
     * @param filePath Path to the import file
     * @return true if import was successful, false otherwise
     */
    bool importCacheFromFile(const QString& filePath);

    /**
     * @brief Defragments the cache to optimize memory layout
     */
    void defragmentCache();

    // Cache inspection functions
    /**
     * @brief Gets all cache keys
     * @return List of all cache keys
     */
    QStringList getCacheKeys() const;

    /**
     * @brief Gets cache keys filtered by item type
     * @param type Cache item type to filter by
     * @return List of cache keys for the specified type
     */
    QStringList getCacheKeysByType(CacheItemType type) const;

    /**
     * @brief Gets cache keys filtered by priority
     * @param priority Cache priority to filter by
     * @return List of cache keys for the specified priority
     */
    QStringList getCacheKeysByPriority(CachePriority priority) const;

    /**
     * @brief Gets the number of cache items of a specific type
     * @param type Cache item type
     * @return Number of cache items of the specified type
     */
    int getCacheItemCount(CacheItemType type) const;

    /**
     * @brief Gets memory usage for a specific cache item type
     * @param type Cache item type
     * @return Memory usage in bytes for the specified type
     */
    qint64 getCacheMemoryUsage(CacheItemType type) const;

    // Cache management functions
    /**
     * @brief Sets the priority of a cache item
     * @param key Cache key
     * @param priority New priority level
     */
    void setCachePriority(const QString& key, CachePriority priority);

    /**
     * @brief Promotes a cache item to high priority
     * @param key Cache key
     * @return true if promotion was successful, false if key not found
     */
    bool promoteToHighPriority(const QString& key);

    /**
     * @brief Refreshes a cache item's access information
     * @param key Cache key
     */
    void refreshCacheItem(const QString& key);

signals:
    /**
     * @brief Emitted when a cache hit occurs
     * @param key Cache key that was hit
     * @param accessTime Time taken to access the item in milliseconds
     */
    void cacheHit(const QString& key, qint64 accessTime);

    /**
     * @brief Emitted when a cache miss occurs
     * @param key Cache key that was missed
     */
    void cacheMiss(const QString& key);

    /**
     * @brief Emitted when a cache item is evicted
     * @param key Cache key that was evicted
     * @param type Type of the evicted item
     */
    void itemEvicted(const QString& key, CacheItemType type);

    /**
     * @brief Emitted when memory threshold is exceeded
     * @param currentUsage Current memory usage in bytes
     * @param threshold Memory threshold that was exceeded
     */
    void memoryThresholdExceeded(qint64 currentUsage, qint64 threshold);

    /**
     * @brief Emitted when preloading is completed
     * @param pageNumber Page number that was preloaded
     * @param type Type of item that was preloaded
     */
    void preloadCompleted(int pageNumber, CacheItemType type);

    /**
     * @brief Emitted when cache optimization is completed
     * @param itemsRemoved Number of items removed during optimization
     * @param memoryFreed Amount of memory freed in bytes
     */
    void cacheOptimized(int itemsRemoved, qint64 memoryFreed);

    /**
     * @brief Emitted when cache defragmentation is completed
     * @param remainingItems Number of items remaining after defragmentation
     */
    void cacheDefragmented(int remainingItems);

    /**
     * @brief Emitted when cache item priority is changed
     * @param key Cache key whose priority was changed
     * @param newPriority New priority level
     */
    void cachePriorityChanged(const QString& key, CachePriority newPriority);

    /**
     * @brief Emitted when cache item is refreshed
     * @param key Cache key that was refreshed
     */
    void cacheItemRefreshed(const QString& key);

    /**
     * @brief Emitted when cache export operation completes
     * @param filePath Path to the export file
     * @param success true if export was successful, false otherwise
     */
    void cacheExported(const QString& filePath, bool success);

    /**
     * @brief Emitted when cache import operation completes
     * @param filePath Path to the import file
     * @param success true if import was successful, false otherwise
     */
    void cacheImported(const QString& filePath, bool success);

private slots:
    /**
     * @brief Performs periodic cache maintenance
     */
    void performMaintenance();

    /**
     * @brief Handles completion of preload tasks
     */
    void onPreloadTaskCompleted();

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};
