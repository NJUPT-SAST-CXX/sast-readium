/**
 * @file CacheTypes.h
 * @brief Type definitions and enumerations for the cache management system
 * @author SAST Team
 * @version 1.0
 * @date 2024
 *
 * This file contains core type definitions, enumerations, and structures
 * used throughout the cache management system in the SAST Readium application.
 *
 * @copyright Copyright (c) 2024 SAST Team
 */

#pragma once

#include <QHash>
#include <QString>
#include <cstdint>

// Forward declarations
class QObject;

/**
 * @brief Enumeration of supported cache types
 *
 * Defines the different types of caches that can be managed by the
 * CacheManager. Each cache type has specific characteristics and
 * memory allocation strategies.
 */
enum class CacheType : std::uint8_t {
    SearchResultCache,     ///< Cache for search query results
    PageTextCache,         ///< Cache for extracted page text content
    SearchHighlightCache,  ///< Cache for search result highlighting data
    PdfRenderCache,        ///< Cache for rendered PDF page images
    ThumbnailCache         ///< Cache for page thumbnail images
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
    qint64 totalMemoryLimit =
        static_cast<qint64>(512) * 1024 * 1024;  ///< Total memory limit (512MB)
    qint64 searchResultCacheLimit =
        static_cast<qint64>(100) * 1024 *
        1024;  ///< Search result cache limit (100MB)
    qint64 pageTextCacheLimit = static_cast<qint64>(50) * 1024 *
                                1024;  ///< Page text cache limit (50MB)
    qint64 searchHighlightCacheLimit = static_cast<qint64>(25) * 1024 *
                                       1024;  ///< Highlight cache limit (25MB)
    qint64 pdfRenderCacheLimit =
        static_cast<qint64>(256) * 1024 * 1024;  ///< Render cache limit (256MB)
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
        true;                           ///< Enable adaptive memory management
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
    double memoryPressureWarningThreshold = 0.75;  ///< Warning threshold (75%)
    double memoryPressureCriticalThreshold =
        0.90;  ///< Critical threshold (90%)

    // System memory monitoring
    int systemMemoryCheckInterval =
        10000;  ///< System memory check interval (10 seconds)
    double systemMemoryPressureThreshold =
        0.85;  ///< System memory pressure threshold (85%)
};

/**
 * @brief Base interface for cache implementations to integrate with
 * CacheManager
 *
 * This interface defines the contract that all cache implementations must
 * follow to integrate with the unified cache management system. It provides
 * methods for memory management, cache operations, statistics, and
 * configuration.
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

// Qt metatype declarations
Q_DECLARE_METATYPE(CacheType)
Q_DECLARE_METATYPE(CacheStats)
