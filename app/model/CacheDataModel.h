/**
 * @file CacheDataModel.h
 * @brief Data model for cache storage and retrieval
 * @author SAST Team
 * @version 2.0
 * @date 2024
 *
 * This file contains the CacheDataModel class which manages the actual
 * cache data storage, retrieval, and eviction operations. Part of the MVP
 * architecture refactoring.
 *
 * @copyright Copyright (c) 2024 SAST Team
 */

#pragma once

#include <QHash>
#include <QMutex>
#include <QString>
#include <QVariant>
#include <memory>
#include "CacheEntryModel.h"
#include "cache/CacheTypes.h"

/**
 * @brief Data model for cache storage operations
 *
 * Manages the actual cache data storage using hash-based data structures
 * for O(1) lookups. Provides thread-safe operations for inserting,
 * retrieving, and removing cache entries. This is the core data layer
 * in the MVP architecture.
 */
class CacheDataModel {
public:
    /**
     * @brief Constructs a new CacheDataModel
     */
    CacheDataModel();

    /**
     * @brief Destructor
     */
    ~CacheDataModel();

    // Disable copy and move
    CacheDataModel(const CacheDataModel&) = delete;
    CacheDataModel& operator=(const CacheDataModel&) = delete;
    CacheDataModel(CacheDataModel&&) = delete;
    CacheDataModel& operator=(CacheDataModel&&) = delete;

    // Cache operations
    /**
     * @brief Inserts an entry into the cache
     * @param entry Cache entry to insert
     * @return true if successful, false otherwise
     */
    bool insert(const CacheEntryModel& entry);

    /**
     * @brief Retrieves an entry from the cache
     * @param key Cache key
     * @return Pointer to entry if found, nullptr otherwise
     */
    [[nodiscard]] CacheEntryModel* get(const QString& key);

    /**
     * @brief Retrieves an entry from the cache (const version)
     * @param key Cache key
     * @return Pointer to entry if found, nullptr otherwise
     */
    [[nodiscard]] const CacheEntryModel* get(const QString& key) const;

    /**
     * @brief Checks if a key exists in the cache
     * @param key Cache key
     * @return true if exists, false otherwise
     */
    [[nodiscard]] bool contains(const QString& key) const;

    /**
     * @brief Removes an entry from the cache
     * @param key Cache key
     * @return true if removed, false if not found
     */
    bool remove(const QString& key);

    /**
     * @brief Clears all entries from the cache
     */
    void clear();

    // Query operations
    /**
     * @brief Gets all cache keys
     * @return List of all cache keys
     */
    [[nodiscard]] QStringList getAllKeys() const;

    /**
     * @brief Gets entries of a specific type
     * @param type Cache type to filter by
     * @return List of entries matching the type
     */
    [[nodiscard]] QList<CacheEntryModel> getEntriesByType(CacheType type) const;

    /**
     * @brief Gets entries sorted by last access time (LRU)
     * @return List of entries sorted by LRU
     */
    [[nodiscard]] QList<CacheEntryModel> getEntriesByLRU() const;

    /**
     * @brief Gets the least recently used entry
     * @return Pointer to LRU entry, nullptr if cache is empty
     */
    [[nodiscard]] const CacheEntryModel* getLeastRecentlyUsed() const;

    // Statistics
    /**
     * @brief Gets the number of entries in the cache
     * @return Entry count
     */
    [[nodiscard]] int getEntryCount() const;

    /**
     * @brief Gets the total memory usage of all entries
     * @return Total memory usage in bytes
     */
    [[nodiscard]] qint64 getTotalMemoryUsage() const;

    /**
     * @brief Gets the number of entries of a specific type
     * @param type Cache type
     * @return Number of entries of the specified type
     */
    [[nodiscard]] int getEntryCountByType(CacheType type) const;

    /**
     * @brief Gets the memory usage of a specific type
     * @param type Cache type
     * @return Memory usage in bytes for the specified type
     */
    [[nodiscard]] qint64 getMemoryUsageByType(CacheType type) const;

    // Maintenance operations
    /**
     * @brief Removes expired entries
     * @param maxAge Maximum age in milliseconds
     * @return Number of entries removed
     */
    int removeExpiredEntries(qint64 maxAge);

    /**
     * @brief Evicts entries until target memory is reached
     * @param targetMemory Target memory usage in bytes
     * @return Number of bytes freed
     */
    qint64 evictToTargetMemory(qint64 targetMemory);

    /**
     * @brief Evicts a specific number of LRU entries
     * @param count Number of entries to evict
     * @return Number of bytes freed
     */
    qint64 evictLRUEntries(int count);

private:
    class Implementation;
    std::unique_ptr<Implementation> m_impl;
};
