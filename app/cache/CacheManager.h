/**
 * @file CacheManager.h
 * @brief Unified cache management system for coordinating all cache types
 * @author SAST Team
 * @version 1.0
 * @date 2024
 *
 * This file contains the CacheManager class which provides centralized cache
 * configuration, monitoring, and coordination for all cache types in the
 * SAST Readium application. It implements a sophisticated cache management
 * system with memory pressure handling, adaptive management, and performance
 * optimization features.
 *
 * @copyright Copyright (c) 2024 SAST Team
 */

#pragma once

#include <QHash>
#include <QMetaType>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QTimer>
#include <cstdint>
#include <memory>

/**
 * @brief Unified cache management system for coordinating all cache types
 *
 * The CacheManager class provides centralized cache configuration, monitoring,
 * and coordination for all cache types in the application. It implements
 * sophisticated memory management, adaptive cache distribution, and performance
 * optimization features.
 *
 * Key features:
 * - Centralized cache configuration and limits
 * - Memory pressure detection and handling
 * - Adaptive cache size management
 * - Performance monitoring and statistics
 * - Cache coordination and eviction strategies
 * - System memory monitoring
 *
 * @see ICacheComponent
 * @since 1.0
 */
class CacheManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Enumeration of supported cache types
     *
     * Defines the different types of caches that can be managed by the
     * CacheManager. Each cache type has specific characteristics and
     * memory allocation strategies.
     */
    enum CacheType : std::uint8_t {
        SEARCH_RESULT_CACHE,     ///< Cache for search query results
        PAGE_TEXT_CACHE,         ///< Cache for extracted page text content
        SEARCH_HIGHLIGHT_CACHE,  ///< Cache for search result highlighting data
        PDF_RENDER_CACHE,        ///< Cache for rendered PDF page images
        THUMBNAIL_CACHE          ///< Cache for page thumbnail images
    };

    /**
     * @brief Statistics structure for individual cache performance monitoring
     *
     * Contains comprehensive statistics about cache performance including
     * memory usage, hit ratios, and entry counts. Used for monitoring
     * and optimizing cache behavior.
     */
    struct CacheStats {
        qint64 memoryUsage = 0;     ///< Current memory usage in bytes
        qint64 maxMemoryLimit = 0;  ///< Maximum allowed memory in bytes
        int entryCount = 0;         ///< Current number of cached entries
        int maxEntryLimit = 0;      ///< Maximum allowed number of entries
        double hitRatio = 0.0;      ///< Cache hit ratio (0.0 to 1.0)
        qint64 totalHits = 0;       ///< Total number of cache hits
        qint64 totalMisses = 0;     ///< Total number of cache misses
    };

    /**
     * @brief Global cache configuration structure
     *
     * Contains comprehensive configuration settings for all cache types
     * including memory limits, eviction policies, performance settings,
     * and advanced memory management options.
     */
    struct GlobalCacheConfig {
        // Memory limits for different cache types
        qint64 totalMemoryLimit = static_cast<qint64>(512) * 1024 *
                                  1024;  ///< Total memory limit (512MB)
        qint64 searchResultCacheLimit =
            static_cast<qint64>(100) * 1024 *
            1024;  ///< Search result cache limit (100MB)
        qint64 pageTextCacheLimit = static_cast<qint64>(50) * 1024 *
                                    1024;  ///< Page text cache limit (50MB)
        qint64 searchHighlightCacheLimit =
            static_cast<qint64>(25) * 1024 *
            1024;  ///< Highlight cache limit (25MB)
        qint64 pdfRenderCacheLimit = static_cast<qint64>(256) * 1024 *
                                     1024;  ///< Render cache limit (256MB)
        qint64 thumbnailCacheLimit = static_cast<qint64>(81) * 1024 *
                                     1024;  ///< Thumbnail cache limit (81MB)

        // Eviction policies
        bool enableLRUEviction =
            true;  ///< Enable LRU (Least Recently Used) eviction
        bool enableMemoryPressureEviction =
            true;  ///< Enable memory pressure-based eviction
        int memoryPressureThreshold =
            85;  ///< Memory pressure threshold (percentage)
        int cleanupInterval =
            30000;  ///< Cleanup interval in milliseconds (30 seconds)

        // Performance settings
        bool enableCacheCoordination =
            true;  ///< Enable coordination between caches
        bool enableAdaptiveMemoryManagement =
            true;  ///< Enable adaptive memory management
        bool enableCachePreloading = true;  ///< Enable cache preloading

        // Advanced memory management
        bool enableSystemMemoryMonitoring =
            true;  ///< Enable system memory monitoring
        bool enablePredictiveEviction =
            true;  ///< Enable predictive eviction strategies
        bool enableMemoryCompression =
            false;  ///< Enable memory compression (experimental)
        bool enableEmergencyEviction =
            true;  ///< Enable emergency eviction under pressure

        // Memory pressure thresholds
        double memoryPressureWarningThreshold =
            0.75;  ///< Warning threshold (75%)
        double memoryPressureCriticalThreshold =
            0.90;  ///< Critical threshold (90%)

        // System memory monitoring
        int systemMemoryCheckInterval =
            10000;  ///< System memory check interval (10 seconds)
        double systemMemoryPressureThreshold =
            0.85;  ///< System memory pressure threshold (85%)
    };

    /**
     * @brief Constructs a new CacheManager instance
     * @param parent Parent QObject for memory management
     */
    explicit CacheManager(QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~CacheManager() override;

    // Disable copy and move operations for QObject-derived class
    CacheManager(const CacheManager&) = delete;
    CacheManager& operator=(const CacheManager&) = delete;
    CacheManager(CacheManager&&) = delete;
    CacheManager& operator=(CacheManager&&) = delete;

    /**
     * @brief Gets the singleton instance of CacheManager
     * @return Reference to the singleton CacheManager instance
     * @note This method is thread-safe
     */
    static CacheManager& instance();

    // Configuration
    /**
     * @brief Sets the global cache configuration
     * @param config The new global cache configuration
     * @see GlobalCacheConfig
     */
    void setGlobalConfig(const GlobalCacheConfig& config);

    /**
     * @brief Gets the current global cache configuration
     * @return The current global cache configuration
     */
    [[nodiscard]] GlobalCacheConfig getGlobalConfig() const;

    /**
     * @brief Sets the memory limit for a specific cache type
     * @param type The cache type to configure
     * @param memoryLimit The new memory limit in bytes
     */
    void setCacheLimit(CacheType type, qint64 memoryLimit);

    /**
     * @brief Gets the memory limit for a specific cache type
     * @param type The cache type to query
     * @return The memory limit in bytes for the specified cache type
     */
    [[nodiscard]] qint64 getCacheLimit(CacheType type) const;

    // Cache registration and management
    /**
     * @brief Registers a cache component with the manager
     * @param type The type of cache being registered
     * @param cache Pointer to the cache object implementing ICacheComponent
     * @note The cache object must implement the ICacheComponent interface
     */
    void registerCache(CacheType type, QObject* cache);

    /**
     * @brief Unregisters a cache component from the manager
     * @param type The type of cache to unregister
     */
    void unregisterCache(CacheType type);

    /**
     * @brief Checks if a cache type is registered
     * @param type The cache type to check
     * @return true if the cache type is registered, false otherwise
     */
    [[nodiscard]] bool isCacheRegistered(CacheType type) const;

    // Global cache operations
    /**
     * @brief Clears all registered caches
     * @note This operation cannot be undone
     */
    void clearAllCaches();

    /**
     * @brief Clears a specific cache type
     * @param type The cache type to clear
     */
    void clearCache(CacheType type);

    /**
     * @brief Enables or disables a specific cache type
     * @param type The cache type to configure
     * @param enabled true to enable, false to disable
     */
    void enableCache(CacheType type, bool enabled);

    /**
     * @brief Checks if a cache type is enabled
     * @param type The cache type to check
     * @return true if enabled, false otherwise
     */
    [[nodiscard]] bool isCacheEnabled(CacheType type) const;

    // Memory management
    /**
     * @brief Gets the total memory usage across all caches
     * @return Total memory usage in bytes
     */
    [[nodiscard]] qint64 getTotalMemoryUsage() const;

    /**
     * @brief Gets the total memory limit across all caches
     * @return Total memory limit in bytes
     */
    [[nodiscard]] qint64 getTotalMemoryLimit() const;

    /**
     * @brief Gets the global memory usage ratio
     * @return Memory usage ratio as a value between 0.0 and 1.0
     */
    [[nodiscard]] double getGlobalMemoryUsageRatio() const;

    /**
     * @brief Enforces memory limits across all caches
     * @note This may trigger cache eviction if limits are exceeded
     */
    void enforceMemoryLimits();

    /**
     * @brief Handles memory pressure situations
     * @note Triggers appropriate eviction strategies based on configuration
     */
    void handleMemoryPressure();

    // Statistics and monitoring
    /**
     * @brief Gets statistics for a specific cache type
     * @param type The cache type to query
     * @return Cache statistics for the specified type
     */
    [[nodiscard]] CacheStats getCacheStats(CacheType type) const;

    /**
     * @brief Gets statistics for all cache types
     * @return Hash map of cache statistics indexed by cache type
     */
    [[nodiscard]] QHash<CacheType, CacheStats> getAllCacheStats() const;

    /**
     * @brief Gets the global hit ratio across all caches
     * @return Global hit ratio as a value between 0.0 and 1.0
     */
    [[nodiscard]] double getGlobalHitRatio() const;

    /**
     * @brief Gets the total number of cache hits across all caches
     * @return Total cache hits
     */
    [[nodiscard]] qint64 getTotalCacheHits() const;

    /**
     * @brief Gets the total number of cache misses across all caches
     * @return Total cache misses
     */
    [[nodiscard]] qint64 getTotalCacheMisses() const;

    // Cache coordination
    /**
     * @brief Notifies the manager of a cache access
     * @param type The cache type that was accessed
     * @param key The cache key that was accessed
     */
    void notifyCacheAccess(CacheType type, const QString& key);

    /**
     * @brief Notifies the manager of a cache hit
     * @param type The cache type that had a hit
     * @param key The cache key that was hit
     */
    void notifyCacheHit(CacheType type, const QString& key);

    /**
     * @brief Notifies the manager of a cache miss
     * @param type The cache type that had a miss
     * @param key The cache key that was missed
     */
    void notifyCacheMiss(CacheType type, const QString& key);

    /**
     * @brief Requests cache eviction for a specific cache type
     * @param type The cache type to evict from
     * @param bytesToFree The number of bytes to free
     */
    void requestCacheEviction(CacheType type, qint64 bytesToFree);

    // Adaptive management
    /**
     * @brief Enables or disables adaptive cache management
     * @param enabled true to enable adaptive management, false to disable
     */
    void enableAdaptiveManagement(bool enabled);

    /**
     * @brief Checks if adaptive management is enabled
     * @return true if adaptive management is enabled, false otherwise
     */
    [[nodiscard]] bool isAdaptiveManagementEnabled() const;

    /**
     * @brief Analyzes cache usage patterns for optimization
     * @note This method collects statistics to improve cache performance
     */
    void analyzeUsagePatterns();

    /**
     * @brief Optimizes cache memory distribution based on usage patterns
     * @note This may adjust memory limits for different cache types
     */
    void optimizeCacheDistribution();

    // Advanced memory management
    /**
     * @brief Enables or disables system memory monitoring
     * @param enabled true to enable system memory monitoring, false to disable
     */
    void enableSystemMemoryMonitoring(bool enabled);

    /**
     * @brief Checks if system memory monitoring is enabled
     * @return true if system memory monitoring is enabled, false otherwise
     */
    [[nodiscard]] bool isSystemMemoryMonitoringEnabled() const;

    /**
     * @brief Gets the current system memory usage
     * @return System memory usage in bytes
     */
    [[nodiscard]] qint64 getSystemMemoryUsage() const;

    /**
     * @brief Gets the total system memory
     * @return Total system memory in bytes
     */
    [[nodiscard]] qint64 getSystemMemoryTotal() const;

    /**
     * @brief Gets the system memory pressure ratio
     * @return System memory pressure as a value between 0.0 and 1.0
     */
    [[nodiscard]] double getSystemMemoryPressure() const;

    /**
     * @brief Handles system-wide memory pressure
     * @note Triggers aggressive cache eviction when system memory is low
     */
    void handleSystemMemoryPressure();

    // Intelligent eviction strategies
    /**
     * @brief Sets the eviction strategy for a specific cache type
     * @param type The cache type to configure
     * @param strategy The eviction strategy name (e.g., "LRU", "LFU", "FIFO")
     */
    void setEvictionStrategy(CacheType type, const QString& strategy);

    /**
     * @brief Gets the eviction strategy for a specific cache type
     * @param type The cache type to query
     * @return The eviction strategy name
     */
    [[nodiscard]] QString getEvictionStrategy(CacheType type) const;

    /**
     * @brief Enables or disables predictive eviction
     * @param enabled true to enable predictive eviction, false to disable
     */
    void enablePredictiveEviction(bool enabled);

    /**
     * @brief Checks if predictive eviction is enabled
     * @return true if predictive eviction is enabled, false otherwise
     */
    [[nodiscard]] bool isPredictiveEvictionEnabled() const;

    // Performance optimization
    /**
     * @brief Enables or disables memory compression
     * @param enabled true to enable memory compression, false to disable
     * @note Memory compression is experimental and may affect performance
     */
    void enableMemoryCompression(bool enabled);

    /**
     * @brief Checks if memory compression is enabled
     * @return true if memory compression is enabled, false otherwise
     */
    [[nodiscard]] bool isMemoryCompressionEnabled() const;

    /**
     * @brief Compresses inactive cache data to save memory
     * @note This operation may take time and affect performance temporarily
     */
    void compressInactiveCaches();

    /**
     * @brief Optimizes memory layout for better performance
     * @note This may trigger cache reorganization
     */
    void optimizeMemoryLayout();

    /**
     * @brief Stops all internal timers
     * @note This is useful for test cleanup to prevent crashes during static
     * destruction
     */
    void stopAllTimers();

    // Memory pressure handling
    /**
     * @brief Sets memory pressure thresholds
     * @param warning Warning threshold (0.0 to 1.0)
     * @param critical Critical threshold (0.0 to 1.0)
     */
    void setMemoryPressureThresholds(double warning, double critical);

    /**
     * @brief Gets memory pressure thresholds
     * @param warning Reference to store warning threshold
     * @param critical Reference to store critical threshold
     */
    void getMemoryPressureThresholds(double& warning, double& critical) const;

    /**
     * @brief Enables or disables emergency eviction
     * @param enabled true to enable emergency eviction, false to disable
     */
    void enableEmergencyEviction(bool enabled);

    /**
     * @brief Checks if emergency eviction is enabled
     * @return true if emergency eviction is enabled, false otherwise
     */
    [[nodiscard]] bool isEmergencyEvictionEnabled() const;

signals:
    /**
     * @brief Emitted when memory limit is exceeded
     * @param currentUsage Current memory usage in bytes
     * @param limit Memory limit that was exceeded in bytes
     */
    void memoryLimitExceeded(qint64 currentUsage, qint64 limit);

    /**
     * @brief Emitted when memory pressure is detected
     * @param usageRatio Memory usage ratio (0.0 to 1.0)
     */
    void memoryPressureDetected(double usageRatio);

    /**
     * @brief Emitted when system memory pressure is detected
     * @param systemUsageRatio System memory usage ratio (0.0 to 1.0)
     */
    void systemMemoryPressureDetected(double systemUsageRatio);

    /**
     * @brief Emitted when memory usage reaches warning threshold
     * @param usageRatio Memory usage ratio (0.0 to 1.0)
     */
    void memoryPressureWarning(double usageRatio);

    /**
     * @brief Emitted when memory usage reaches critical threshold
     * @param usageRatio Memory usage ratio (0.0 to 1.0)
     */
    void memoryPressureCritical(double usageRatio);

    /**
     * @brief Emitted when cache statistics are updated
     * @param type The cache type that was updated
     * @param stats The updated cache statistics
     */
    void cacheStatsUpdated(CacheType type, const CacheStats& stats);

    /**
     * @brief Emitted when global statistics are updated
     * @param totalMemory Total memory usage across all caches
     * @param hitRatio Global hit ratio across all caches
     */
    void globalStatsUpdated(qint64 totalMemory, double hitRatio);

    /**
     * @brief Emitted when cache eviction is requested
     * @param type The cache type to evict from
     * @param bytesToFree The number of bytes to free
     */
    void cacheEvictionRequested(CacheType type, qint64 bytesToFree);

    /**
     * @brief Emitted when emergency eviction is triggered
     * @param bytesFreed The number of bytes that were freed
     */
    void emergencyEvictionTriggered(qint64 bytesFreed);

    /**
     * @brief Emitted when cache configuration changes
     */
    void cacheConfigurationChanged();

    /**
     * @brief Emitted when memory optimization is completed
     * @param memoryFreed The amount of memory freed in bytes
     */
    void memoryOptimizationCompleted(qint64 memoryFreed);

    /**
     * @brief Emitted when cache compression is completed
     * @param memorySaved The amount of memory saved in bytes
     */
    void cacheCompressionCompleted(qint64 memorySaved);

private slots:
    /**
     * @brief Performs periodic cache cleanup operations
     */
    void performPeriodicCleanup();

    /**
     * @brief Handles memory pressure timer events
     */
    void onMemoryPressureTimer();

    /**
     * @brief Updates cache statistics
     */
    void updateCacheStatistics();

private:
    class Implementation;  ///< Private implementation class (PIMPL idiom)
    std::unique_ptr<Implementation> m_d;  ///< Pointer to private implementation
};

/**
 * @brief Base interface for cache implementations to integrate with
 * CacheManager
 *
 * This interface defines the contract that all cache implementations must
 * follow to integrate with the unified cache management system. It provides
 * methods for memory management, cache operations, statistics, and
 * configuration.
 *
 * @see CacheManager
 * @since 1.0
 */
class ICacheComponent {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~ICacheComponent() = default;

    // Disable copy and move operations for interface
    ICacheComponent(const ICacheComponent&) = delete;
    ICacheComponent& operator=(const ICacheComponent&) = delete;
    ICacheComponent(ICacheComponent&&) = delete;
    ICacheComponent& operator=(ICacheComponent&&) = delete;

    // Memory management
    /**
     * @brief Gets the current memory usage of the cache
     * @return Current memory usage in bytes
     */
    [[nodiscard]] virtual qint64 getMemoryUsage() const = 0;

    /**
     * @brief Gets the maximum memory limit for the cache
     * @return Maximum memory limit in bytes
     */
    [[nodiscard]] virtual qint64 getMaxMemoryLimit() const = 0;

    /**
     * @brief Sets the maximum memory limit for the cache
     * @param limit Maximum memory limit in bytes
     */
    virtual void setMaxMemoryLimit(qint64 limit) = 0;

    // Cache operations
    /**
     * @brief Clears all entries from the cache
     */
    virtual void clear() = 0;

    /**
     * @brief Gets the number of entries in the cache
     * @return Number of cache entries
     */
    [[nodiscard]] virtual int getEntryCount() const = 0;

    /**
     * @brief Evicts least recently used entries to free memory
     * @param bytesToFree Number of bytes to free
     */
    virtual void evictLRU(qint64 bytesToFree) = 0;

    // Statistics
    /**
     * @brief Gets the total number of cache hits
     * @return Total cache hits
     */
    [[nodiscard]] virtual qint64 getHitCount() const = 0;

    /**
     * @brief Gets the total number of cache misses
     * @return Total cache misses
     */
    [[nodiscard]] virtual qint64 getMissCount() const = 0;

    /**
     * @brief Resets all cache statistics
     */
    virtual void resetStatistics() = 0;

    // Configuration
    /**
     * @brief Enables or disables the cache
     * @param enabled true to enable, false to disable
     */
    virtual void setEnabled(bool enabled) = 0;

    /**
     * @brief Checks if the cache is enabled
     * @return true if enabled, false otherwise
     */
    [[nodiscard]] virtual bool isEnabled() const = 0;

protected:
    /**
     * @brief Protected default constructor
     */
    ICacheComponent() = default;
};

/// @brief Declares CacheType as a Qt metatype for use in signals/slots and
/// QVariant
Q_DECLARE_METATYPE(CacheManager::CacheType)

/// @brief Declares CacheStats as a Qt metatype for use in signals/slots and
/// QVariant
Q_DECLARE_METATYPE(CacheManager::CacheStats)
