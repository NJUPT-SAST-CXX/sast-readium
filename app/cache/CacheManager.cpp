#include "CacheManager.h"
#include <QCoreApplication>
#include <QDebug>
#include <QMutexLocker>
#include <QProcess>
#include <QThread>
#include <algorithm>

#ifdef Q_OS_WIN
#include <psapi.h>
#include <windows.h>
#elif defined(Q_OS_LINUX)
#include <unistd.h>
#include <fstream>
#include <sstream>
#elif defined(Q_OS_MACOS)
#include <mach/mach.h>
#include <mach/mach_init.h>
#include <mach/task.h>
#include <sys/sysctl.h>
#endif

class CacheManager::Implementation {
public:
    Implementation(CacheManager* q)
        : q_ptr(q),
          cleanupTimer(new QTimer(q)),
          memoryPressureTimer(new QTimer(q)),
          statsUpdateTimer(new QTimer(q)),
          systemMemoryTimer(new QTimer(q)),
          adaptiveManagementEnabled(true),
          systemMemoryMonitoringEnabled(true),
          predictiveEvictionEnabled(true),
          memoryCompressionEnabled(false),
          emergencyEvictionEnabled(true) {
        // Setup timers
        cleanupTimer->setSingleShot(false);
        memoryPressureTimer->setSingleShot(false);
        statsUpdateTimer->setSingleShot(false);
        systemMemoryTimer->setSingleShot(false);

        QObject::connect(cleanupTimer, &QTimer::timeout, q,
                         &CacheManager::performPeriodicCleanup);
        QObject::connect(memoryPressureTimer, &QTimer::timeout, q,
                         &CacheManager::onMemoryPressureTimer);
        QObject::connect(statsUpdateTimer, &QTimer::timeout, q,
                         &CacheManager::updateCacheStatistics);
        QObject::connect(systemMemoryTimer, &QTimer::timeout, q,
                         &CacheManager::handleSystemMemoryPressure);

        // Start timers
        cleanupTimer->start(config.cleanupInterval);
        memoryPressureTimer->start(5000);  // Check every 5 seconds
        statsUpdateTimer->start(10000);    // Update stats every 10 seconds
        systemMemoryTimer->start(config.systemMemoryCheckInterval);
    }

    ~Implementation() {
        // Stop and disconnect timers explicitly to prevent crashes during
        // static destruction Don't delete them here - they will be deleted by
        // Qt's parent-child mechanism
        if (cleanupTimer) {
            cleanupTimer->stop();
            cleanupTimer->disconnect();
        }
        if (memoryPressureTimer) {
            memoryPressureTimer->stop();
            memoryPressureTimer->disconnect();
        }
        if (statsUpdateTimer) {
            statsUpdateTimer->stop();
            statsUpdateTimer->disconnect();
        }
        if (systemMemoryTimer) {
            systemMemoryTimer->stop();
            systemMemoryTimer->disconnect();
        }

        // Clear registered caches to prevent dangling pointers
        registeredCaches.clear();
    }

    CacheManager* q_ptr;
    GlobalCacheConfig config;
    QHash<CacheType, ICacheComponent*> registeredCaches;
    QHash<CacheType, bool> cacheEnabled;
    QHash<CacheType, qint64> cacheMemoryLimits;

    // Statistics tracking
    QHash<CacheType, qint64> cacheHits;
    QHash<CacheType, qint64> cacheMisses;
    QHash<CacheType, QStringList> recentAccesses;  // For LRU tracking

    // Timers
    QTimer* cleanupTimer;
    QTimer* memoryPressureTimer;
    QTimer* statsUpdateTimer;
    QTimer* systemMemoryTimer;

    // Thread safety
    mutable QMutex mutex;

    // Adaptive management
    bool adaptiveManagementEnabled;
    QHash<CacheType, double> usagePatterns;

    // Advanced memory management
    bool systemMemoryMonitoringEnabled;
    bool predictiveEvictionEnabled;
    bool memoryCompressionEnabled;
    bool emergencyEvictionEnabled;

    // Eviction strategies
    QHash<CacheType, QString> evictionStrategies;

    // Memory pressure thresholds
    double memoryPressureWarningThreshold = 0.75;
    double memoryPressureCriticalThreshold = 0.90;

    void initializeDefaultLimits() {
        cacheMemoryLimits[SEARCH_RESULT_CACHE] = config.searchResultCacheLimit;
        cacheMemoryLimits[PAGE_TEXT_CACHE] = config.pageTextCacheLimit;
        cacheMemoryLimits[SEARCH_HIGHLIGHT_CACHE] =
            config.searchHighlightCacheLimit;
        cacheMemoryLimits[PDF_RENDER_CACHE] = config.pdfRenderCacheLimit;
        cacheMemoryLimits[THUMBNAIL_CACHE] = config.thumbnailCacheLimit;

        // Enable all caches by default
        for (auto type :
             {SEARCH_RESULT_CACHE, PAGE_TEXT_CACHE, SEARCH_HIGHLIGHT_CACHE,
              PDF_RENDER_CACHE, THUMBNAIL_CACHE}) {
            cacheEnabled[type] = true;
        }
    }

    qint64 calculateTotalMemoryUsage() const {
        qint64 total = 0;
        for (auto it = registeredCaches.constBegin();
             it != registeredCaches.constEnd(); ++it) {
            if (it.value() && cacheEnabled.value(it.key(), true)) {
                total += it.value()->getMemoryUsage();
            }
        }
        return total;
    }

    void performMemoryPressureEviction() {
        qint64 totalUsage = calculateTotalMemoryUsage();
        qint64 targetUsage = config.totalMemoryLimit * 0.7;  // Target 70% usage

        if (totalUsage <= targetUsage) {
            return;
        }

        qint64 bytesToFree = totalUsage - targetUsage;

        // Prioritize eviction based on cache importance and usage patterns
        QList<QPair<CacheType, double>> evictionPriority;

        for (auto it = registeredCaches.constBegin();
             it != registeredCaches.constEnd(); ++it) {
            CacheType type = it.key();
            if (!it.value() || !cacheEnabled.value(type, true))
                continue;

            double priority = calculateEvictionPriority(type);
            evictionPriority.append({type, priority});
        }

        // Sort by priority (lower priority = evict first)
        std::sort(evictionPriority.begin(), evictionPriority.end(),
                  [](const QPair<CacheType, double>& a,
                     const QPair<CacheType, double>& b) {
                      return a.second < b.second;
                  });

        // Evict from lowest priority caches first
        for (const auto& pair : evictionPriority) {
            if (bytesToFree <= 0)
                break;

            CacheType type = pair.first;
            ICacheComponent* cache = registeredCaches.value(type);
            if (!cache)
                continue;

            qint64 cacheUsage = cache->getMemoryUsage();
            qint64 toEvictFromCache =
                std::min(bytesToFree,
                         cacheUsage / 2);  // Evict up to 50% from each cache

            cache->evictLRU(toEvictFromCache);
            bytesToFree -= toEvictFromCache;

            emit q_ptr->cacheEvictionRequested(type, toEvictFromCache);
        }
    }

    double calculateEvictionPriority(CacheType type) const {
        // Higher values = higher priority (less likely to be evicted)
        switch (type) {
            case SEARCH_RESULT_CACHE:
                return 0.9;  // High priority - expensive to regenerate
            case PAGE_TEXT_CACHE:
                return 0.8;  // High priority - expensive extraction
            case PDF_RENDER_CACHE:
                return 0.7;  // Medium-high priority - expensive rendering
            case SEARCH_HIGHLIGHT_CACHE:
                return 0.5;  // Medium priority - can be regenerated
            case THUMBNAIL_CACHE:
                return 0.3;  // Lower priority - less critical
            default:
                return 0.1;
        }
    }

    void updateUsagePatterns() {
        for (auto it = registeredCaches.constBegin();
             it != registeredCaches.constEnd(); ++it) {
            CacheType type = it.key();
            ICacheComponent* cache = it.value();
            if (!cache)
                continue;

            qint64 hits = cache->getHitCount();
            qint64 misses = cache->getMissCount();
            qint64 total = hits + misses;

            if (total > 0) {
                usagePatterns[type] = static_cast<double>(hits) / total;
            }
        }
    }
};

CacheManager::CacheManager(QObject* parent)
    : QObject(parent), m_d(std::make_unique<Implementation>(this)) {
    m_d->initializeDefaultLimits();
}

CacheManager::~CacheManager() {
    // During static destruction, QCoreApplication might already be destroyed
    // In that case, we can't safely do anything, so just return
    if (!QCoreApplication::instance()) {
        return;
    }

    // Stop all timers before destruction to prevent crashes
    if (m_d) {
        // Stop timers first
        if (m_d->cleanupTimer) {
            m_d->cleanupTimer->stop();
            disconnect(m_d->cleanupTimer, nullptr, this, nullptr);
        }
        if (m_d->memoryPressureTimer) {
            m_d->memoryPressureTimer->stop();
            disconnect(m_d->memoryPressureTimer, nullptr, this, nullptr);
        }
        if (m_d->statsUpdateTimer) {
            m_d->statsUpdateTimer->stop();
            disconnect(m_d->statsUpdateTimer, nullptr, this, nullptr);
        }
        if (m_d->systemMemoryTimer) {
            m_d->systemMemoryTimer->stop();
            disconnect(m_d->systemMemoryTimer, nullptr, this, nullptr);
        }

        // Process any pending events before destruction
        QCoreApplication::processEvents();

        // Clear all registered caches to prevent dangling pointers
        m_d->registeredCaches.clear();
    }
}

CacheManager& CacheManager::instance() {
    static CacheManager instance;
    return instance;
}

void CacheManager::setGlobalConfig(const GlobalCacheConfig& config) {
    QMutexLocker locker(&m_d->mutex);
    m_d->config = config;
    m_d->initializeDefaultLimits();

    // Update timer intervals
    m_d->cleanupTimer->setInterval(config.cleanupInterval);

    emit cacheConfigurationChanged();
}

CacheManager::GlobalCacheConfig CacheManager::getGlobalConfig() const {
    QMutexLocker locker(&m_d->mutex);
    return m_d->config;
}

void CacheManager::setCacheLimit(CacheType type, qint64 memoryLimit) {
    QMutexLocker locker(&m_d->mutex);
    m_d->cacheMemoryLimits[type] = memoryLimit;

    ICacheComponent* cache = m_d->registeredCaches.value(type);
    if (cache) {
        cache->setMaxMemoryLimit(memoryLimit);
    }
}

qint64 CacheManager::getCacheLimit(CacheType type) const {
    QMutexLocker locker(&m_d->mutex);
    return m_d->cacheMemoryLimits.value(type, 0);
}

void CacheManager::registerCache(CacheType type, QObject* cache) {
    QMutexLocker locker(&m_d->mutex);

    ICacheComponent* cacheComponent = dynamic_cast<ICacheComponent*>(cache);
    if (!cacheComponent) {
        qWarning()
            << "Cache object does not implement ICacheComponent interface";
        return;
    }

    m_d->registeredCaches[type] = cacheComponent;

    // Apply memory limit
    qint64 limit = m_d->cacheMemoryLimits.value(type, 0);
    if (limit > 0) {
        cacheComponent->setMaxMemoryLimit(limit);
    }

    qDebug() << "Registered cache type:" << type;
}

void CacheManager::unregisterCache(CacheType type) {
    QMutexLocker locker(&m_d->mutex);
    m_d->registeredCaches.remove(type);
    m_d->cacheEnabled.remove(type);
    m_d->cacheHits.remove(type);
    m_d->cacheMisses.remove(type);
    m_d->recentAccesses.remove(type);
}

bool CacheManager::isCacheRegistered(CacheType type) const {
    QMutexLocker locker(&m_d->mutex);
    return m_d->registeredCaches.contains(type);
}

void CacheManager::clearAllCaches() {
    QMutexLocker locker(&m_d->mutex);
    for (auto cache : m_d->registeredCaches) {
        if (cache) {
            cache->clear();
        }
    }

    // Reset statistics
    m_d->cacheHits.clear();
    m_d->cacheMisses.clear();
    m_d->recentAccesses.clear();
}

void CacheManager::clearCache(CacheType type) {
    QMutexLocker locker(&m_d->mutex);
    ICacheComponent* cache = m_d->registeredCaches.value(type);
    if (cache) {
        cache->clear();
        m_d->cacheHits[type] = 0;
        m_d->cacheMisses[type] = 0;
        m_d->recentAccesses[type].clear();
    }
}

void CacheManager::enableCache(CacheType type, bool enabled) {
    QMutexLocker locker(&m_d->mutex);
    m_d->cacheEnabled[type] = enabled;

    ICacheComponent* cache = m_d->registeredCaches.value(type);
    if (cache) {
        cache->setEnabled(enabled);
    }
}

bool CacheManager::isCacheEnabled(CacheType type) const {
    QMutexLocker locker(&m_d->mutex);
    return m_d->cacheEnabled.value(type, true);
}

qint64 CacheManager::getTotalMemoryUsage() const {
    QMutexLocker locker(&m_d->mutex);
    return m_d->calculateTotalMemoryUsage();
}

qint64 CacheManager::getTotalMemoryLimit() const {
    QMutexLocker locker(&m_d->mutex);
    return m_d->config.totalMemoryLimit;
}

double CacheManager::getGlobalMemoryUsageRatio() const {
    qint64 usage = getTotalMemoryUsage();
    qint64 limit = getTotalMemoryLimit();
    return limit > 0 ? static_cast<double>(usage) / limit : 0.0;
}

void CacheManager::enforceMemoryLimits() {
    QMutexLocker locker(&m_d->mutex);

    qint64 totalUsage = m_d->calculateTotalMemoryUsage();
    if (totalUsage > m_d->config.totalMemoryLimit) {
        emit memoryLimitExceeded(totalUsage, m_d->config.totalMemoryLimit);
        m_d->performMemoryPressureEviction();
    }
}

void CacheManager::handleMemoryPressure() {
    double usageRatio = getGlobalMemoryUsageRatio();
    if (usageRatio > m_d->config.memoryPressureThreshold / 100.0) {
        emit memoryPressureDetected(usageRatio);

        QMutexLocker locker(&m_d->mutex);
        m_d->performMemoryPressureEviction();
    }
}

void CacheManager::performPeriodicCleanup() {
    if (m_d->config.enableMemoryPressureEviction) {
        handleMemoryPressure();
    }

    if (m_d->adaptiveManagementEnabled &&
        m_d->config.enableAdaptiveMemoryManagement) {
        analyzeUsagePatterns();
        optimizeCacheDistribution();
    }
}

void CacheManager::onMemoryPressureTimer() { handleMemoryPressure(); }

void CacheManager::updateCacheStatistics() {
    QMutexLocker locker(&m_d->mutex);

    for (auto it = m_d->registeredCaches.constBegin();
         it != m_d->registeredCaches.constEnd(); ++it) {
        CacheType type = it.key();
        ICacheComponent* cache = it.value();
        if (!cache)
            continue;

        CacheStats stats;
        stats.memoryUsage = cache->getMemoryUsage();
        stats.maxMemoryLimit = cache->getMaxMemoryLimit();
        stats.entryCount = cache->getEntryCount();
        stats.totalHits = cache->getHitCount();
        stats.totalMisses = cache->getMissCount();

        qint64 total = stats.totalHits + stats.totalMisses;
        stats.hitRatio =
            total > 0 ? static_cast<double>(stats.totalHits) / total : 0.0;

        emit cacheStatsUpdated(type, stats);
    }

    qint64 totalMemory = m_d->calculateTotalMemoryUsage();
    double globalHitRatio = getGlobalHitRatio();
    emit globalStatsUpdated(totalMemory, globalHitRatio);
}

CacheManager::CacheStats CacheManager::getCacheStats(CacheType type) const {
    QMutexLocker locker(&m_d->mutex);

    CacheStats stats;
    ICacheComponent* cache = m_d->registeredCaches.value(type);
    if (!cache)
        return stats;

    stats.memoryUsage = cache->getMemoryUsage();
    stats.maxMemoryLimit = cache->getMaxMemoryLimit();
    stats.entryCount = cache->getEntryCount();
    stats.totalHits = cache->getHitCount();
    stats.totalMisses = cache->getMissCount();

    qint64 total = stats.totalHits + stats.totalMisses;
    stats.hitRatio =
        total > 0 ? static_cast<double>(stats.totalHits) / total : 0.0;

    return stats;
}

double CacheManager::getGlobalHitRatio() const {
    QMutexLocker locker(&m_d->mutex);

    qint64 totalHits = 0;
    qint64 totalMisses = 0;

    for (auto cache : m_d->registeredCaches) {
        if (cache) {
            totalHits += cache->getHitCount();
            totalMisses += cache->getMissCount();
        }
    }

    qint64 total = totalHits + totalMisses;
    return total > 0 ? static_cast<double>(totalHits) / total : 0.0;
}

QHash<CacheManager::CacheType, CacheManager::CacheStats>
CacheManager::getAllCacheStats() const {
    QMutexLocker locker(&m_d->mutex);

    QHash<CacheType, CacheStats> allStats;

    for (auto it = m_d->registeredCaches.constBegin();
         it != m_d->registeredCaches.constEnd(); ++it) {
        CacheType type = it.key();
        ICacheComponent* cache = it.value();

        if (cache) {
            CacheStats stats;
            stats.memoryUsage = cache->getMemoryUsage();
            stats.maxMemoryLimit = cache->getMaxMemoryLimit();
            stats.entryCount = cache->getEntryCount();
            stats.totalHits = cache->getHitCount();
            stats.totalMisses = cache->getMissCount();

            qint64 total = stats.totalHits + stats.totalMisses;
            stats.hitRatio =
                total > 0 ? static_cast<double>(stats.totalHits) / total : 0.0;

            allStats[type] = stats;
        }
    }

    return allStats;
}

qint64 CacheManager::getTotalCacheHits() const {
    QMutexLocker locker(&m_d->mutex);

    qint64 totalHits = 0;

    for (auto cache : m_d->registeredCaches) {
        if (cache) {
            totalHits += cache->getHitCount();
        }
    }

    return totalHits;
}

qint64 CacheManager::getTotalCacheMisses() const {
    QMutexLocker locker(&m_d->mutex);

    qint64 totalMisses = 0;

    for (auto cache : m_d->registeredCaches) {
        if (cache) {
            totalMisses += cache->getMissCount();
        }
    }

    return totalMisses;
}

void CacheManager::notifyCacheAccess(CacheType type, const QString& key) {
    QMutexLocker locker(&m_d->mutex);

    // Track recent accesses for LRU
    QStringList& accesses = m_d->recentAccesses[type];
    accesses.removeAll(key);
    accesses.prepend(key);

    // Limit the size of recent accesses tracking
    const int maxTrackedAccesses = 1000;
    if (accesses.size() > maxTrackedAccesses) {
        accesses = accesses.mid(0, maxTrackedAccesses);
    }
}

void CacheManager::notifyCacheHit(CacheType type, const QString& key) {
    QMutexLocker locker(&m_d->mutex);
    m_d->cacheHits[type]++;

    // Update recent access for LRU tracking
    QStringList& recent = m_d->recentAccesses[type];
    recent.removeAll(key);
    recent.prepend(key);
    if (recent.size() > 1000) {  // Limit recent access tracking
        recent = recent.mid(0, 1000);
    }
}

void CacheManager::notifyCacheMiss(CacheType type, const QString& key) {
    QMutexLocker locker(&m_d->mutex);
    m_d->cacheMisses[type]++;
}

void CacheManager::analyzeUsagePatterns() {
    QMutexLocker locker(&m_d->mutex);
    m_d->updateUsagePatterns();
}

void CacheManager::optimizeCacheDistribution() {
    QMutexLocker locker(&m_d->mutex);

    // Redistribute memory based on usage patterns
    qint64 totalLimit = m_d->config.totalMemoryLimit;
    qint64 allocatedMemory = 0;

    // Calculate new limits based on hit ratios and importance
    for (auto it = m_d->usagePatterns.constBegin();
         it != m_d->usagePatterns.constEnd(); ++it) {
        CacheType type = it.key();
        double hitRatio = it.value();
        double importance = m_d->calculateEvictionPriority(type);

        // Allocate more memory to caches with higher hit ratios and importance
        double factor = (hitRatio * 0.7 + importance * 0.3);
        qint64 newLimit = static_cast<qint64>(totalLimit * factor *
                                              0.15);  // Max 15% per cache

        // Ensure minimum limits
        qint64 minLimit = totalLimit * 0.05;  // At least 5% per cache
        newLimit = std::max(newLimit, minLimit);

        setCacheLimit(type, newLimit);
        allocatedMemory += newLimit;
    }
}

void CacheManager::enableAdaptiveManagement(bool enabled) {
    QMutexLocker locker(&m_d->mutex);
    m_d->adaptiveManagementEnabled = enabled;
}

bool CacheManager::isAdaptiveManagementEnabled() const {
    QMutexLocker locker(&m_d->mutex);
    return m_d->adaptiveManagementEnabled;
}

// Advanced memory management implementation
void CacheManager::enableSystemMemoryMonitoring(bool enabled) {
    QMutexLocker locker(&m_d->mutex);
    m_d->systemMemoryMonitoringEnabled = enabled;

    if (enabled) {
        m_d->systemMemoryTimer->start(m_d->config.systemMemoryCheckInterval);
    } else {
        m_d->systemMemoryTimer->stop();
    }
}

bool CacheManager::isSystemMemoryMonitoringEnabled() const {
    QMutexLocker locker(&m_d->mutex);
    return m_d->systemMemoryMonitoringEnabled;
}

qint64 CacheManager::getSystemMemoryUsage() const {
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast<qint64>(pmc.WorkingSetSize);
    }
#elif defined(Q_OS_LINUX)
    std::ifstream file("/proc/self/status");
    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            std::istringstream iss(line);
            std::string key, value, unit;
            iss >> key >> value >> unit;
            return std::stoll(value) * 1024;  // Convert KB to bytes
        }
    }
#elif defined(Q_OS_MACOS)
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info,
                  &infoCount) == KERN_SUCCESS) {
        return static_cast<qint64>(info.resident_size);
    }
#endif
    return -1;  // Failed to get memory usage
}

qint64 CacheManager::getSystemMemoryTotal() const {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        return static_cast<qint64>(memInfo.ullTotalPhys);
    }
#elif defined(Q_OS_LINUX)
    std::ifstream file("/proc/meminfo");
    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 9) == "MemTotal:") {
            std::istringstream iss(line);
            std::string key, value, unit;
            iss >> key >> value >> unit;
            return std::stoll(value) * 1024;  // Convert KB to bytes
        }
    }
#elif defined(Q_OS_MACOS)
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    uint64_t memsize;
    size_t length = sizeof(memsize);
    if (sysctl(mib, 2, &memsize, &length, NULL, 0) == 0) {
        return static_cast<qint64>(memsize);
    }
#endif
    return -1;  // Failed to get total memory
}

double CacheManager::getSystemMemoryPressure() const {
    qint64 usage = getSystemMemoryUsage();
    qint64 total = getSystemMemoryTotal();

    if (usage > 0 && total > 0) {
        return static_cast<double>(usage) / total;
    }
    return 0.0;
}

void CacheManager::handleSystemMemoryPressure() {
    if (!m_d->systemMemoryMonitoringEnabled)
        return;

    double systemPressure = getSystemMemoryPressure();

    if (systemPressure > m_d->config.systemMemoryPressureThreshold) {
        emit systemMemoryPressureDetected(systemPressure);

        // Trigger aggressive cache eviction
        QMutexLocker locker(&m_d->mutex);

        // Calculate how much memory to free (aim for 10% below threshold)
        qint64 totalSystemMemory = getSystemMemoryTotal();
        qint64 targetUsage = static_cast<qint64>(
            totalSystemMemory *
            (m_d->config.systemMemoryPressureThreshold - 0.1));
        qint64 currentUsage = getSystemMemoryUsage();
        qint64 bytesToFree = currentUsage - targetUsage;

        if (bytesToFree > 0) {
            // Free memory from all caches proportionally
            qint64 totalCacheMemory = m_d->calculateTotalMemoryUsage();

            for (auto it = m_d->registeredCaches.constBegin();
                 it != m_d->registeredCaches.constEnd(); ++it) {
                CacheType type = it.key();
                ICacheComponent* cache = it.value();
                if (!cache || !m_d->cacheEnabled.value(type, true))
                    continue;

                qint64 cacheMemory = cache->getMemoryUsage();
                if (totalCacheMemory > 0) {
                    qint64 cacheShare =
                        (bytesToFree * cacheMemory) / totalCacheMemory;
                    cache->evictLRU(cacheShare);
                }
            }

            emit emergencyEvictionTriggered(bytesToFree);
        }
    }
}

void CacheManager::enablePredictiveEviction(bool enabled) {
    QMutexLocker locker(&m_d->mutex);
    m_d->predictiveEvictionEnabled = enabled;
}

bool CacheManager::isPredictiveEvictionEnabled() const {
    QMutexLocker locker(&m_d->mutex);
    return m_d->predictiveEvictionEnabled;
}

void CacheManager::setEvictionStrategy(CacheType type,
                                       const QString& strategy) {
    QMutexLocker locker(&m_d->mutex);
    m_d->evictionStrategies[type] = strategy;
}

QString CacheManager::getEvictionStrategy(CacheType type) const {
    QMutexLocker locker(&m_d->mutex);
    return m_d->evictionStrategies.value(type, "LRU");
}

void CacheManager::enableMemoryCompression(bool enabled) {
    QMutexLocker locker(&m_d->mutex);
    m_d->memoryCompressionEnabled = enabled;
}

bool CacheManager::isMemoryCompressionEnabled() const {
    QMutexLocker locker(&m_d->mutex);
    return m_d->memoryCompressionEnabled;
}

void CacheManager::compressInactiveCaches() {
    // This would implement cache compression for inactive entries
    // For now, just emit a signal indicating compression is complete
    emit cacheCompressionCompleted(0);
}

void CacheManager::optimizeMemoryLayout() {
    QMutexLocker locker(&m_d->mutex);

    qint64 memoryFreed = 0;

    // Perform memory optimization across all caches
    for (auto it = m_d->registeredCaches.constBegin();
         it != m_d->registeredCaches.constEnd(); ++it) {
        CacheType type = it.key();
        ICacheComponent* cache = it.value();
        if (!cache || !m_d->cacheEnabled.value(type, true))
            continue;

        qint64 beforeOptimization = cache->getMemoryUsage();

        // Trigger cache-specific optimization
        // This could include defragmentation, compression, etc.
        // For now, we'll just ensure memory limits are enforced
        qint64 limit = cache->getMaxMemoryLimit();
        if (beforeOptimization > limit) {
            cache->evictLRU(beforeOptimization - limit);
        }

        qint64 afterOptimization = cache->getMemoryUsage();
        memoryFreed += (beforeOptimization - afterOptimization);
    }

    emit memoryOptimizationCompleted(memoryFreed);
}

void CacheManager::setMemoryPressureThresholds(double warning,
                                               double critical) {
    QMutexLocker locker(&m_d->mutex);
    m_d->memoryPressureWarningThreshold = warning;
    m_d->memoryPressureCriticalThreshold = critical;
}

void CacheManager::getMemoryPressureThresholds(double& warning,
                                               double& critical) const {
    QMutexLocker locker(&m_d->mutex);
    warning = m_d->memoryPressureWarningThreshold;
    critical = m_d->memoryPressureCriticalThreshold;
}

void CacheManager::enableEmergencyEviction(bool enabled) {
    QMutexLocker locker(&m_d->mutex);
    m_d->emergencyEvictionEnabled = enabled;
}

bool CacheManager::isEmergencyEvictionEnabled() const {
    QMutexLocker locker(&m_d->mutex);
    return m_d->emergencyEvictionEnabled;
}

void CacheManager::requestCacheEviction(CacheType type, qint64 bytesToFree) {
    QMutexLocker locker(&m_d->mutex);

    if (!m_d->registeredCaches.contains(type)) {
        return;
    }

    ICacheComponent* cache = m_d->registeredCaches[type];
    if (!cache) {
        return;
    }

    // Calculate how many items to evict based on bytes to free
    qint64 currentUsage = cache->getMemoryUsage();
    if (currentUsage > 0) {
        // Try to free the requested amount of memory
        cache->evictLRU(bytesToFree);
    }
}

void CacheManager::stopAllTimers() {
    QMutexLocker locker(&m_d->mutex);

    // Stop all timers to prevent crashes during static destruction
    if (m_d->cleanupTimer) {
        m_d->cleanupTimer->stop();
    }
    if (m_d->memoryPressureTimer) {
        m_d->memoryPressureTimer->stop();
    }
    if (m_d->statsUpdateTimer) {
        m_d->statsUpdateTimer->stop();
    }
    if (m_d->systemMemoryTimer) {
        m_d->systemMemoryTimer->stop();
    }
}
