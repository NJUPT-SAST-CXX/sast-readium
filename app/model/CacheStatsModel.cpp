#include "CacheStatsModel.h"
#include <QMutexLocker>

CacheStatsModel::CacheStatsModel() = default;

void CacheStatsModel::recordHit(CacheType type) {
    QMutexLocker locker(&m_mutex);
    m_hits[type]++;
}

void CacheStatsModel::recordMiss(CacheType type) {
    QMutexLocker locker(&m_mutex);
    m_misses[type]++;
}

qint64 CacheStatsModel::getHits(CacheType type) const {
    QMutexLocker locker(&m_mutex);
    return m_hits.value(type, 0);
}

qint64 CacheStatsModel::getMisses(CacheType type) const {
    QMutexLocker locker(&m_mutex);
    return m_misses.value(type, 0);
}

qint64 CacheStatsModel::getTotalHits() const {
    QMutexLocker locker(&m_mutex);
    qint64 total = 0;
    for (auto it = m_hits.constBegin(); it != m_hits.constEnd(); ++it) {
        total += it.value();
    }
    return total;
}

qint64 CacheStatsModel::getTotalMisses() const {
    QMutexLocker locker(&m_mutex);
    qint64 total = 0;
    for (auto it = m_misses.constBegin(); it != m_misses.constEnd(); ++it) {
        total += it.value();
    }
    return total;
}

double CacheStatsModel::getHitRatio(CacheType type) const {
    QMutexLocker locker(&m_mutex);
    qint64 hits = m_hits.value(type, 0);
    qint64 misses = m_misses.value(type, 0);
    qint64 total = hits + misses;
    return total > 0 ? static_cast<double>(hits) / static_cast<double>(total)
                     : 0.0;
}

double CacheStatsModel::getGlobalHitRatio() const {
    qint64 totalHits = getTotalHits();
    qint64 totalMisses = getTotalMisses();
    qint64 total = totalHits + totalMisses;
    return total > 0
               ? static_cast<double>(totalHits) / static_cast<double>(total)
               : 0.0;
}

void CacheStatsModel::recordMemoryUsage(CacheType type, qint64 bytes) {
    QMutexLocker locker(&m_mutex);
    m_memoryUsage[type] = bytes;
}

qint64 CacheStatsModel::getMemoryUsage(CacheType type) const {
    QMutexLocker locker(&m_mutex);
    return m_memoryUsage.value(type, 0);
}

qint64 CacheStatsModel::getTotalMemoryUsage() const {
    QMutexLocker locker(&m_mutex);
    qint64 total = 0;
    for (auto it = m_memoryUsage.constBegin(); it != m_memoryUsage.constEnd();
         ++it) {
        total += it.value();
    }
    return total;
}

void CacheStatsModel::recordEviction(CacheType type, qint64 bytesFreed) {
    QMutexLocker locker(&m_mutex);
    m_evictionCount[type]++;
    m_bytesEvicted[type] += bytesFreed;
}

qint64 CacheStatsModel::getEvictionCount(CacheType type) const {
    QMutexLocker locker(&m_mutex);
    return m_evictionCount.value(type, 0);
}

qint64 CacheStatsModel::getTotalEvictionCount() const {
    QMutexLocker locker(&m_mutex);
    qint64 total = 0;
    for (auto it = m_evictionCount.constBegin();
         it != m_evictionCount.constEnd(); ++it) {
        total += it.value();
    }
    return total;
}

qint64 CacheStatsModel::getBytesEvicted(CacheType type) const {
    QMutexLocker locker(&m_mutex);
    return m_bytesEvicted.value(type, 0);
}

qint64 CacheStatsModel::getTotalBytesEvicted() const {
    QMutexLocker locker(&m_mutex);
    qint64 total = 0;
    for (auto it = m_bytesEvicted.constBegin(); it != m_bytesEvicted.constEnd();
         ++it) {
        total += it.value();
    }
    return total;
}

void CacheStatsModel::recordAccess(CacheType type, const QString& key) {
    QMutexLocker locker(&m_mutex);
    m_accessCount[type]++;

    // Track recent accesses
    QStringList& recent = m_recentAccesses[type];
    recent.removeAll(key);
    recent.prepend(key);
    if (recent.size() > MAX_RECENT_ACCESSES) {
        recent = recent.mid(0, MAX_RECENT_ACCESSES);
    }
}

qint64 CacheStatsModel::getAccessCount(CacheType type) const {
    QMutexLocker locker(&m_mutex);
    return m_accessCount.value(type, 0);
}

qint64 CacheStatsModel::getTotalAccessCount() const {
    QMutexLocker locker(&m_mutex);
    qint64 total = 0;
    for (auto it = m_accessCount.constBegin(); it != m_accessCount.constEnd();
         ++it) {
        total += it.value();
    }
    return total;
}

QStringList CacheStatsModel::getRecentAccesses(CacheType type,
                                               int limit) const {
    QMutexLocker locker(&m_mutex);
    const QStringList& recent = m_recentAccesses.value(type);
    return recent.mid(0, qMin(limit, recent.size()));
}

void CacheStatsModel::recordEntryCount(CacheType type, int count) {
    QMutexLocker locker(&m_mutex);
    m_entryCount[type] = count;
}

int CacheStatsModel::getEntryCount(CacheType type) const {
    QMutexLocker locker(&m_mutex);
    return m_entryCount.value(type, 0);
}

int CacheStatsModel::getTotalEntryCount() const {
    QMutexLocker locker(&m_mutex);
    int total = 0;
    for (auto it = m_entryCount.constBegin(); it != m_entryCount.constEnd();
         ++it) {
        total += it.value();
    }
    return total;
}

void CacheStatsModel::reset() {
    QMutexLocker locker(&m_mutex);
    m_hits.clear();
    m_misses.clear();
    m_memoryUsage.clear();
    m_evictionCount.clear();
    m_bytesEvicted.clear();
    m_accessCount.clear();
    m_recentAccesses.clear();
    m_entryCount.clear();
}

void CacheStatsModel::reset(CacheType type) {
    QMutexLocker locker(&m_mutex);
    m_hits.remove(type);
    m_misses.remove(type);
    m_memoryUsage.remove(type);
    m_evictionCount.remove(type);
    m_bytesEvicted.remove(type);
    m_accessCount.remove(type);
    m_recentAccesses.remove(type);
    m_entryCount.remove(type);
}

CacheStats CacheStatsModel::getStats(CacheType type) const {
    QMutexLocker locker(&m_mutex);
    CacheStats stats;

    stats.memoryUsage = m_memoryUsage.value(type, 0);
    stats.entryCount = m_entryCount.value(type, 0);
    stats.totalHits = m_hits.value(type, 0);
    stats.totalMisses = m_misses.value(type, 0);

    qint64 total = stats.totalHits + stats.totalMisses;
    stats.hitRatio = total > 0 ? static_cast<double>(stats.totalHits) /
                                     static_cast<double>(total)
                               : 0.0;

    return stats;
}

QHash<CacheType, CacheStats> CacheStatsModel::getAllStats() const {
    QMutexLocker locker(&m_mutex);
    QHash<CacheType, CacheStats> allStats;

    // Get all unique cache types from any of the tracking maps
    QSet<CacheType> types;
    for (auto it = m_hits.constBegin(); it != m_hits.constEnd(); ++it) {
        types.insert(it.key());
    }
    for (auto it = m_misses.constBegin(); it != m_misses.constEnd(); ++it) {
        types.insert(it.key());
    }
    for (auto it = m_memoryUsage.constBegin(); it != m_memoryUsage.constEnd();
         ++it) {
        types.insert(it.key());
    }

    // Generate stats for each type
    for (CacheType type : types) {
        CacheStats stats;
        stats.memoryUsage = m_memoryUsage.value(type, 0);
        stats.entryCount = m_entryCount.value(type, 0);
        stats.totalHits = m_hits.value(type, 0);
        stats.totalMisses = m_misses.value(type, 0);

        qint64 total = stats.totalHits + stats.totalMisses;
        stats.hitRatio = total > 0 ? static_cast<double>(stats.totalHits) /
                                         static_cast<double>(total)
                                   : 0.0;

        allStats[type] = stats;
    }

    return allStats;
}
