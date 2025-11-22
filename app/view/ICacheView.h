/**
 * @file ICacheView.h
 * @brief View interface for cache consumers in MVP architecture
 * @author SAST Team
 * @version 2.0
 * @date 2024
 *
 * This file contains the view interfaces for cache consumers following
 * the MVP (Model-View-Presenter) architecture pattern. These interfaces
 * define how cache consumers interact with cache data and receive updates.
 *
 * @copyright Copyright (c) 2024 SAST Team
 */

#pragma once

#include <QString>
#include <QVariant>
#include "cache/CacheTypes.h"

/**
 * @brief Base view interface for cache consumers
 *
 * Defines the contract for cache consumers to receive cache-related updates
 * and notifications. This is the View layer in the MVP architecture.
 */
class ICacheView {
public:
    virtual ~ICacheView() = default;

    /**
     * @brief Called when cache data is updated
     * @param type Cache type that was updated
     * @param key Cache key that was updated
     */
    virtual void onCacheUpdated(CacheType type, const QString& key) = 0;

    /**
     * @brief Called when cache is cleared
     * @param type Cache type that was cleared
     */
    virtual void onCacheCleared(CacheType type) = 0;

    /**
     * @brief Called when cache entry is evicted
     * @param type Cache type
     * @param key Cache key that was evicted
     * @param reason Eviction reason
     */
    virtual void onCacheEvicted(CacheType type, const QString& key,
                                const QString& reason) = 0;
};

/**
 * @brief View interface for cache statistics observers
 *
 * Defines the contract for components that need to observe cache performance
 * metrics and statistics.
 */
class ICacheStatsView {
public:
    virtual ~ICacheStatsView() = default;

    /**
     * @brief Called when cache statistics are updated
     * @param type Cache type
     * @param stats Updated statistics
     */
    virtual void onStatsUpdated(CacheType type, const CacheStats& stats) = 0;

    /**
     * @brief Called when global cache statistics are updated
     * @param totalMemory Total memory usage across all caches
     * @param hitRatio Global hit ratio
     */
    virtual void onGlobalStatsUpdated(qint64 totalMemory, double hitRatio) = 0;
};

/**
 * @brief View interface for cache configuration observers
 *
 * Defines the contract for components that need to be notified of
 * cache configuration changes.
 */
class ICacheConfigView {
public:
    virtual ~ICacheConfigView() = default;

    /**
     * @brief Called when cache configuration changes
     * @param type Cache type that was configured
     */
    virtual void onConfigChanged(CacheType type) = 0;

    /**
     * @brief Called when global cache configuration changes
     */
    virtual void onGlobalConfigChanged() = 0;
};

/**
 * @brief View interface for memory pressure observers
 *
 * Defines the contract for components that need to be notified of
 * memory pressure situations.
 */
class ICacheMemoryView {
public:
    virtual ~ICacheMemoryView() = default;

    /**
     * @brief Called when memory limit is exceeded
     * @param currentUsage Current memory usage
     * @param limit Memory limit that was exceeded
     */
    virtual void onMemoryLimitExceeded(qint64 currentUsage, qint64 limit) = 0;

    /**
     * @brief Called when memory pressure is detected
     * @param usageRatio Memory usage ratio (0.0 to 1.0)
     */
    virtual void onMemoryPressureDetected(double usageRatio) = 0;

    /**
     * @brief Called when system memory pressure is detected
     * @param systemUsageRatio System memory usage ratio (0.0 to 1.0)
     */
    virtual void onSystemMemoryPressureDetected(double systemUsageRatio) = 0;
};
