/**
 * @file CacheStatsModel.h
 * @brief Statistics model for cache performance tracking
 * @author SAST Team
 * @version 2.0
 * @date 2024
 *
 * This file contains the CacheStatsModel class which tracks and aggregates
 * cache performance statistics including hits, misses, memory usage, and
 * eviction metrics. Part of the MVP architecture refactoring.
 *
 * @copyright Copyright (c) 2024 SAST Team
 */

#pragma once

#include <QHash>
#include <QMutex>
#include <QString>
#include "cache/CacheTypes.h"

/**
 * @brief Statistics model for cache performance tracking
 *
 * Tracks comprehensive cache performance metrics including hit/miss ratios,
 * memory usage patterns, eviction counts, and access patterns. This is part
 * of the Model layer in the MVP architecture.
 */
class CacheStatsModel {
public:
    /**
     * @brief Constructs a new CacheStatsModel
     */
    CacheStatsModel();

    /**
     * @brief Destructor
     */
    ~CacheStatsModel() = default;

    // Hit/Miss tracking
    void recordHit(CacheType type);
    void recordMiss(CacheType type);
    [[nodiscard]] qint64 getHits(CacheType type) const;
    [[nodiscard]] qint64 getMisses(CacheType type) const;
    [[nodiscard]] qint64 getTotalHits() const;
    [[nodiscard]] qint64 getTotalMisses() const;
    [[nodiscard]] double getHitRatio(CacheType type) const;
    [[nodiscard]] double getGlobalHitRatio() const;

    // Memory tracking
    void recordMemoryUsage(CacheType type, qint64 bytes);
    [[nodiscard]] qint64 getMemoryUsage(CacheType type) const;
    [[nodiscard]] qint64 getTotalMemoryUsage() const;

    // Eviction tracking
    void recordEviction(CacheType type, qint64 bytesFreed);
    [[nodiscard]] qint64 getEvictionCount(CacheType type) const;
    [[nodiscard]] qint64 getTotalEvictionCount() const;
    [[nodiscard]] qint64 getBytesEvicted(CacheType type) const;
    [[nodiscard]] qint64 getTotalBytesEvicted() const;

    // Access pattern tracking
    void recordAccess(CacheType type, const QString& key);
    [[nodiscard]] qint64 getAccessCount(CacheType type) const;
    [[nodiscard]] qint64 getTotalAccessCount() const;
    [[nodiscard]] QStringList getRecentAccesses(CacheType type,
                                                int limit = 100) const;

    // Entry count tracking
    void recordEntryCount(CacheType type, int count);
    [[nodiscard]] int getEntryCount(CacheType type) const;
    [[nodiscard]] int getTotalEntryCount() const;

    // Reset operations
    void reset();
    void reset(CacheType type);

    // Statistics export
    [[nodiscard]] CacheStats getStats(CacheType type) const;
    [[nodiscard]] QHash<CacheType, CacheStats> getAllStats() const;

private:
    mutable QMutex m_mutex;

    // Hit/Miss counters
    QHash<CacheType, qint64> m_hits;
    QHash<CacheType, qint64> m_misses;

    // Memory usage
    QHash<CacheType, qint64> m_memoryUsage;

    // Eviction counters
    QHash<CacheType, qint64> m_evictionCount;
    QHash<CacheType, qint64> m_bytesEvicted;

    // Access tracking
    QHash<CacheType, qint64> m_accessCount;
    QHash<CacheType, QStringList> m_recentAccesses;

    // Entry counts
    QHash<CacheType, int> m_entryCount;

    static constexpr int MAX_RECENT_ACCESSES = 1000;
};
