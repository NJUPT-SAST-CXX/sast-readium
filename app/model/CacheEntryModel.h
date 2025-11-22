/**
 * @file CacheEntryModel.h
 * @brief Model class representing a single cache entry with metadata
 * @author SAST Team
 * @version 2.0
 * @date 2024
 *
 * This file contains the CacheEntryModel class which represents a single
 * cache entry with associated metadata for tracking, eviction, and memory
 * management. Part of the MVP architecture refactoring.
 *
 * @copyright Copyright (c) 2024 SAST Team
 */

#pragma once

#include <QDateTime>
#include <QString>
#include <QVariant>
#include "cache/CacheTypes.h"

/**
 * @brief Model class representing a single cache entry
 *
 * Encapsulates the data and metadata for a single cache entry including
 * access tracking, priority, memory size, and expiration information.
 * This class is part of the Model layer in the MVP architecture.
 */
class CacheEntryModel {
public:
    /**
     * @brief Constructs a new cache entry
     * @param key Unique cache key
     * @param data Cached data
     * @param type Cache type
     */
    explicit CacheEntryModel(QString key = QString(),
                             QVariant data = QVariant(),
                             CacheType type = CacheType::SearchResultCache);

    /**
     * @brief Default destructor
     */
    ~CacheEntryModel() = default;

    // Getters
    [[nodiscard]] const QString& getKey() const { return m_key; }
    [[nodiscard]] const QVariant& getData() const { return m_data; }
    [[nodiscard]] CacheType getType() const { return m_type; }
    [[nodiscard]] qint64 getTimestamp() const { return m_timestamp; }
    [[nodiscard]] qint64 getLastAccessed() const { return m_lastAccessed; }
    [[nodiscard]] qint64 getAccessCount() const { return m_accessCount; }
    [[nodiscard]] qint64 getMemorySize() const { return m_memorySize; }
    [[nodiscard]] int getPriority() const { return m_priority; }
    [[nodiscard]] bool isExpired(qint64 maxAge) const;

    // Setters
    void setData(const QVariant& data);
    void setPriority(int priority);
    void setMemorySize(qint64 size) { m_memorySize = size; }

    // Operations
    void updateAccess();
    void resetAccessCount();
    [[nodiscard]] qint64 getAge() const;
    [[nodiscard]] double calculateEvictionScore(double priorityWeight) const;

private:
    QString m_key;
    QVariant m_data;
    CacheType m_type;
    qint64 m_timestamp;
    qint64 m_lastAccessed;
    qint64 m_accessCount;
    qint64 m_memorySize;
    int m_priority;

    [[nodiscard]] qint64 calculateDataSize() const;
};
