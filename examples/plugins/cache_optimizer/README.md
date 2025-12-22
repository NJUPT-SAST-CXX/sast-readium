# Cache Optimizer Plugin

This plugin demonstrates the `ICacheStrategyPlugin` interface with LFU-based caching.

## Features

- **LFU Eviction**: Least Frequently Used eviction algorithm
- **Priority Calculation**: Smart scoring based on access patterns and recency
- **Cache Persistence**: Save and load cache state to/from JSON
- **Optimization**: Periodic cache optimization suggestions

## ICacheStrategyPlugin Interface

```cpp
class ICacheStrategyPlugin {
    QString strategyName() const;
    CacheEvictionStrategy evictionStrategy() const;
    bool shouldCache(const QString& key, qint64 size, const QVariantMap& metadata) const;
    QString selectEvictionCandidate(const QList<CacheEntryMetadata>& entries, qint64 newEntrySize) const;
    int calculatePriority(const CacheEntryMetadata& metadata) const;
    int optimizeCache(qint64 currentSize, qint64 maxSize);
    bool persistCache(const QString& cachePath, const QList<CacheEntryMetadata>& entries);
    QList<CacheEntryMetadata> loadCache(const QString& cachePath);
};
```

## Priority Calculation

Priority = `(accessCount * 10) + recencyBoost + customPriority`

- **Access Count**: Higher frequency = higher priority
- **Recency Boost**: Recently accessed items get bonus points
- **Decay Rate**: Priority decreases over time without access

## Configuration

```json
{
    "minSizeToCache": 1024,
    "maxSizeToCache": 104857600,
    "priorityBoostForRecent": 10,
    "priorityDecayRate": 1
}
```

## Hook Registration

- `cache.pre_add`: Validate before adding to cache
- `cache.pre_evict`: Called before eviction
- `cache.optimize`: Called during optimization

## Persistence Format

```json
{
    "version": "1.0",
    "strategy": "LFU-Optimized",
    "entries": [
        {
            "key": "page_1_thumbnail",
            "size": 102400,
            "createdAt": "2024-01-01T00:00:00",
            "lastAccessedAt": "2024-01-01T12:00:00",
            "accessCount": 15,
            "priority": 5
        }
    ],
    "savedAt": "2024-01-01T23:59:59"
}
```

## Building

```bash
mkdir build && cd build
cmake .. && cmake --build .
```
