/**
 * @file CacheManager.h
 * @brief Main header for the unified cache management system
 * @author SAST Team
 * @version 1.0
 * @date 2024
 *
 * This file provides the main entry point for the cache management system.
 * It includes the interface and type definitions needed for cache management.
 *
 * @copyright Copyright (c) 2024 SAST Team
 */

#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <memory>
#include "CacheTypes.h"

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
    using CacheType = ::CacheType;
    using CacheStats = ::CacheStats;
    using GlobalCacheConfig = ::GlobalCacheConfig;

    // Constants for cache types
    static constexpr CacheType SEARCH_RESULT_CACHE =
        CacheType::SearchResultCache;
    static constexpr CacheType PAGE_TEXT_CACHE = CacheType::PageTextCache;
    static constexpr CacheType SEARCH_HIGHLIGHT_CACHE =
        CacheType::SearchHighlightCache;
    static constexpr CacheType PDF_RENDER_CACHE = CacheType::PdfRenderCache;
    static constexpr CacheType THUMBNAIL_CACHE = CacheType::ThumbnailCache;

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
     * @param type The cache type to unregister
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

    // System memory monitoring
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
    [[nodiscard]] static qint64 getSystemMemoryUsage();

    /**
     * @brief Gets the total system memory
     * @return Total system memory in bytes
     */
    [[nodiscard]] static qint64 getSystemMemoryTotal();

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

    // Performance optimization
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
