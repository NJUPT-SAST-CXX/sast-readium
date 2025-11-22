#include "CacheDataModel.h"
#include <algorithm>

class CacheDataModel::Implementation {
public:
    mutable QMutex mutex;
    QHash<QString, CacheEntryModel> entries;
};

CacheDataModel::CacheDataModel() : m_impl(std::make_unique<Implementation>()) {}

CacheDataModel::~CacheDataModel() = default;

bool CacheDataModel::insert(const CacheEntryModel& entry) {
    QMutexLocker locker(&m_impl->mutex);
    m_impl->entries[entry.getKey()] = entry;
    return true;
}

CacheEntryModel* CacheDataModel::get(const QString& key) {
    QMutexLocker locker(&m_impl->mutex);
    auto it = m_impl->entries.find(key);
    if (it != m_impl->entries.end()) {
        return &it.value();
    }
    return nullptr;
}

const CacheEntryModel* CacheDataModel::get(const QString& key) const {
    QMutexLocker locker(&m_impl->mutex);
    auto it = m_impl->entries.find(key);
    if (it != m_impl->entries.end()) {
        return &it.value();
    }
    return nullptr;
}

bool CacheDataModel::contains(const QString& key) const {
    QMutexLocker locker(&m_impl->mutex);
    return m_impl->entries.contains(key);
}

bool CacheDataModel::remove(const QString& key) {
    QMutexLocker locker(&m_impl->mutex);
    return m_impl->entries.remove(key) > 0;
}

void CacheDataModel::clear() {
    QMutexLocker locker(&m_impl->mutex);
    m_impl->entries.clear();
}

QStringList CacheDataModel::getAllKeys() const {
    QMutexLocker locker(&m_impl->mutex);
    return m_impl->entries.keys();
}

QList<CacheEntryModel> CacheDataModel::getEntriesByType(CacheType type) const {
    QMutexLocker locker(&m_impl->mutex);
    QList<CacheEntryModel> result;
    for (const auto& entry : m_impl->entries) {
        if (entry.getType() == type) {
            result.append(entry);
        }
    }
    return result;
}

QList<CacheEntryModel> CacheDataModel::getEntriesByLRU() const {
    QMutexLocker locker(&m_impl->mutex);
    QList<CacheEntryModel> result;
    result.reserve(m_impl->entries.size());

    for (const auto& entry : m_impl->entries) {
        result.append(entry);
    }

    // Sort by last accessed time (oldest first)
    std::sort(result.begin(), result.end(),
              [](const CacheEntryModel& a, const CacheEntryModel& b) {
                  return a.getLastAccessed() < b.getLastAccessed();
              });

    return result;
}

const CacheEntryModel* CacheDataModel::getLeastRecentlyUsed() const {
    QMutexLocker locker(&m_impl->mutex);
    if (m_impl->entries.isEmpty()) {
        return nullptr;
    }

    const CacheEntryModel* lruEntry = nullptr;
    qint64 oldestTime = QDateTime::currentMSecsSinceEpoch();

    for (const auto& entry : m_impl->entries) {
        if (entry.getLastAccessed() < oldestTime) {
            oldestTime = entry.getLastAccessed();
            lruEntry = &entry;
        }
    }

    return lruEntry;
}

int CacheDataModel::getEntryCount() const {
    QMutexLocker locker(&m_impl->mutex);
    return m_impl->entries.size();
}

qint64 CacheDataModel::getTotalMemoryUsage() const {
    QMutexLocker locker(&m_impl->mutex);
    qint64 total = 0;
    for (const auto& entry : m_impl->entries) {
        total += entry.getMemorySize();
    }
    return total;
}

int CacheDataModel::getEntryCountByType(CacheType type) const {
    QMutexLocker locker(&m_impl->mutex);
    int count = 0;
    for (const auto& entry : m_impl->entries) {
        if (entry.getType() == type) {
            count++;
        }
    }
    return count;
}

qint64 CacheDataModel::getMemoryUsageByType(CacheType type) const {
    QMutexLocker locker(&m_impl->mutex);
    qint64 total = 0;
    for (const auto& entry : m_impl->entries) {
        if (entry.getType() == type) {
            total += entry.getMemorySize();
        }
    }
    return total;
}

int CacheDataModel::removeExpiredEntries(qint64 maxAge) {
    QMutexLocker locker(&m_impl->mutex);
    int removed = 0;

    auto it = m_impl->entries.begin();
    while (it != m_impl->entries.end()) {
        if (it.value().isExpired(maxAge)) {
            it = m_impl->entries.erase(it);
            removed++;
        } else {
            ++it;
        }
    }

    return removed;
}

qint64 CacheDataModel::evictToTargetMemory(qint64 targetMemory) {
    QMutexLocker locker(&m_impl->mutex);
    qint64 currentMemory = 0;
    for (const auto& entry : m_impl->entries) {
        currentMemory += entry.getMemorySize();
    }

    if (currentMemory <= targetMemory) {
        return 0;
    }

    // Get entries sorted by LRU
    QList<CacheEntryModel> sortedEntries;
    sortedEntries.reserve(m_impl->entries.size());
    for (const auto& entry : m_impl->entries) {
        sortedEntries.append(entry);
    }

    std::sort(sortedEntries.begin(), sortedEntries.end(),
              [](const CacheEntryModel& a, const CacheEntryModel& b) {
                  return a.getLastAccessed() < b.getLastAccessed();
              });

    qint64 freedMemory = 0;
    for (const auto& entry : sortedEntries) {
        if (currentMemory - freedMemory <= targetMemory) {
            break;
        }

        freedMemory += entry.getMemorySize();
        m_impl->entries.remove(entry.getKey());
    }

    return freedMemory;
}

qint64 CacheDataModel::evictLRUEntries(int count) {
    QMutexLocker locker(&m_impl->mutex);
    if (count <= 0 || m_impl->entries.isEmpty()) {
        return 0;
    }

    // Get entries sorted by LRU
    QList<CacheEntryModel> sortedEntries;
    sortedEntries.reserve(m_impl->entries.size());
    for (const auto& entry : m_impl->entries) {
        sortedEntries.append(entry);
    }

    std::sort(sortedEntries.begin(), sortedEntries.end(),
              [](const CacheEntryModel& a, const CacheEntryModel& b) {
                  return a.getLastAccessed() < b.getLastAccessed();
              });

    qint64 freedMemory = 0;
    int evicted = 0;
    for (const auto& entry : sortedEntries) {
        if (evicted >= count) {
            break;
        }

        freedMemory += entry.getMemorySize();
        m_impl->entries.remove(entry.getKey());
        evicted++;
    }

    return freedMemory;
}
