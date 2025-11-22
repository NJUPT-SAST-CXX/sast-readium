#pragma once

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>

/**
 * @brief Cache Eviction Strategy
 */
enum class CacheEvictionStrategy {
    LRU,    // Least Recently Used
    LFU,    // Least Frequently Used
    FIFO,   // First In First Out
    ARC,    // Adaptive Replacement Cache
    Custom  // Custom eviction algorithm
};

/**
 * @brief Cache Entry Metadata
 */
struct CacheEntryMetadata {
    QString key;
    qint64 size;
    QDateTime createdAt;
    QDateTime lastAccessedAt;
    int accessCount;
    int priority;
    QVariantMap customData;

    CacheEntryMetadata() : size(0), accessCount(0), priority(0) {}
};

/**
 * @brief ICacheStrategyPlugin - Interface for custom cache strategy plugins
 *
 * Plugins implementing this interface can provide custom caching algorithms,
 * storage backends, and eviction strategies.
 */
class ICacheStrategyPlugin {
public:
    virtual ~ICacheStrategyPlugin() = default;

    /**
     * @brief Get cache strategy name
     */
    virtual QString strategyName() const = 0;

    /**
     * @brief Get eviction strategy
     */
    virtual CacheEvictionStrategy evictionStrategy() const = 0;

    /**
     * @brief Should this item be cached?
     * @param key Cache key
     * @param size Item size in bytes
     * @param metadata Item metadata
     * @return True if item should be cached
     */
    virtual bool shouldCache(const QString& key, qint64 size,
                             const QVariantMap& metadata) const = 0;

    /**
     * @brief Get cache entry to evict
     * @param entries List of cache entries
     * @param newEntrySize Size of new entry to be added
     * @return Key of entry to evict, or empty string if none
     */
    virtual QString selectEvictionCandidate(
        const QList<CacheEntryMetadata>& entries,
        qint64 newEntrySize) const = 0;

    /**
     * @brief Calculate priority for cache entry
     * @param metadata Entry metadata
     * @return Priority value (higher = keep longer)
     */
    virtual int calculatePriority(const CacheEntryMetadata& metadata) const = 0;

    /**
     * @brief Optimize cache (defragment, rebalance, etc.)
     * @param currentSize Current cache size in bytes
     * @param maxSize Maximum cache size in bytes
     * @return Number of optimizations performed
     */
    virtual int optimizeCache(qint64 currentSize, qint64 maxSize) = 0;

    /**
     * @brief Persist cache to storage
     * @param cachePath Path to store cache data
     * @param entries Cache entries to persist
     * @return True if persistence succeeded
     */
    virtual bool persistCache(const QString& cachePath,
                              const QList<CacheEntryMetadata>& entries) = 0;

    /**
     * @brief Load cache from storage
     * @param cachePath Path to stored cache data
     * @return Loaded cache entries
     */
    virtual QList<CacheEntryMetadata> loadCache(const QString& cachePath) = 0;
};

Q_DECLARE_INTERFACE(ICacheStrategyPlugin,
                    "com.sast.readium.ICacheStrategyPlugin/1.0")
