/**
 * @file CacheEvents.h
 * @brief Cache event constants for EventBus integration
 * @author SAST Team
 * @version 1.0
 * @date 2024
 *
 * This file defines the event types published by the cache system
 * to the EventBus for decoupled component communication.
 *
 * @copyright Copyright (c) 2024 SAST Team
 */

#pragma once

#include <QString>

/**
 * @namespace CacheEvents
 * @brief Namespace containing cache event type constants
 *
 * All cache-related events published to the EventBus are defined here.
 * Components can subscribe to these events to react to cache operations
 * without direct coupling to the cache system.
 *
 * Event data is passed as QVariantMap with documented keys.
 */
namespace CacheEvents {

// Memory pressure events
/**
 * @brief Emitted when cache memory limit is exceeded
 * Data keys: "currentUsage" (qint64), "limit" (qint64)
 */
inline const QString& MEMORY_LIMIT_EXCEEDED() {
    static const QString kValue = "cache.memory.limitExceeded";
    return kValue;
}

/**
 * @brief Emitted when memory pressure is detected
 * Data keys: "usageRatio" (double, 0.0-1.0)
 */
inline const QString& MEMORY_PRESSURE_DETECTED() {
    static const QString kValue = "cache.memory.pressureDetected";
    return kValue;
}

/**
 * @brief Emitted when memory usage reaches warning threshold
 * Data keys: "usageRatio" (double, 0.0-1.0)
 */
inline const QString& MEMORY_PRESSURE_WARNING() {
    static const QString kValue = "cache.memory.pressureWarning";
    return kValue;
}

/**
 * @brief Emitted when memory usage reaches critical threshold
 * Data keys: "usageRatio" (double, 0.0-1.0)
 */
inline const QString& MEMORY_PRESSURE_CRITICAL() {
    static const QString kValue = "cache.memory.pressureCritical";
    return kValue;
}

/**
 * @brief Emitted when system-wide memory pressure is detected
 * Data keys: "systemUsageRatio" (double, 0.0-1.0)
 */
inline const QString& SYSTEM_MEMORY_PRESSURE() {
    static const QString kValue = "cache.system.memoryPressure";
    return kValue;
}

// Cache statistics events
/**
 * @brief Emitted when cache statistics are updated
 * Data keys: "cacheType" (int), "memoryUsage" (qint64), "entryCount" (qint64),
 *            "hitRatio" (double), "totalHits" (qint64), "totalMisses" (qint64)
 */
inline const QString& STATS_UPDATED() {
    static const QString kValue = "cache.stats.updated";
    return kValue;
}

/**
 * @brief Emitted when global statistics are updated
 * Data keys: "totalMemory" (qint64), "hitRatio" (double)
 */
inline const QString& GLOBAL_STATS_UPDATED() {
    static const QString kValue = "cache.stats.global";
    return kValue;
}

// Cache operation events
/**
 * @brief Emitted when cache eviction is requested
 * Data keys: "cacheType" (int), "bytesToFree" (qint64)
 */
inline const QString& EVICTION_REQUESTED() {
    static const QString kValue = "cache.eviction.requested";
    return kValue;
}

/**
 * @brief Emitted when emergency eviction is triggered
 * Data keys: "bytesFreed" (qint64)
 */
inline const QString& EMERGENCY_EVICTION() {
    static const QString kValue = "cache.eviction.emergency";
    return kValue;
}

/**
 * @brief Emitted when cache configuration changes
 * Data: No additional data
 */
inline const QString& CONFIG_CHANGED() {
    static const QString kValue = "cache.config.changed";
    return kValue;
}

/**
 * @brief Emitted when memory optimization is completed
 * Data keys: "memoryFreed" (qint64)
 */
inline const QString& OPTIMIZATION_COMPLETED() {
    static const QString kValue = "cache.optimization.completed";
    return kValue;
}

/**
 * @brief Emitted when cache compression is completed
 * Data keys: "memorySaved" (qint64)
 */
inline const QString& COMPRESSION_COMPLETED() {
    static const QString kValue = "cache.compression.completed";
    return kValue;
}

}  // namespace CacheEvents

/**
 * @example CacheEvents Usage
 *
 * To subscribe to cache events:
 * @code
 * EventBus::instance().subscribe(CacheEvents::MEMORY_PRESSURE_WARNING(),
 *                                 this,
 *                                 [](Event* event) {
 *                                     QVariantMap data = event->data().toMap();
 *                                     double ratio =
 * data["usageRatio"].toDouble(); qDebug() << "Memory pressure:" << ratio;
 *                                 });
 * @endcode
 */
