#include "CacheManager.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QMutexLocker>
#include <QProcess>
#include <QThread>
#include <QTimer>
#include <algorithm>
#include <atomic>
#include "controller/CachePresenter.h"
#include "controller/EventBus.h"
#include "logging/SimpleLogging.h"
#include "plugin/ICacheStrategyPlugin.h"
#include "plugin/PluginHookRegistry.h"
#include "plugin/PluginManager.h"

using enum CacheType;

#ifdef Q_OS_LINUX
#include <unistd.h>
#include <fstream>
#include <sstream>
#elif defined(Q_OS_MACOS)
#include <mach/mach.h>
#include <mach/mach_init.h>
#include <mach/task.h>
#include <sys/sysctl.h>
#elif defined(Q_OS_WIN)
#include <psapi.h>
#include <windows.h>
#endif

constexpr int MAX_TRACKED_ACCESSES = 1000;

class CacheManager::Implementation {
public:
    explicit Implementation(CacheManager* qPtr)
        : q_ptr(qPtr),
          cleanupTimer(new QTimer(qPtr)),
          memoryPressureTimer(new QTimer(qPtr)),
          statsUpdateTimer(new QTimer(qPtr)),
          systemMemoryTimer(new QTimer(qPtr)),
          adaptiveManagementEnabled(true),
          systemMemoryMonitoringEnabled(true),
          predictiveEvictionEnabled(true),
          memoryCompressionEnabled(false),
          emergencyEvictionEnabled(true),
          presenter(std::make_unique<CachePresenter>(qPtr)) {
        // Setup timers
        cleanupTimer->setSingleShot(false);
        memoryPressureTimer->setSingleShot(false);
        statsUpdateTimer->setSingleShot(false);
        systemMemoryTimer->setSingleShot(false);

        QObject::connect(cleanupTimer, &QTimer::timeout, qPtr,
                         &CacheManager::performPeriodicCleanup);
        QObject::connect(memoryPressureTimer, &QTimer::timeout, qPtr,
                         &CacheManager::onMemoryPressureTimer);
        QObject::connect(statsUpdateTimer, &QTimer::timeout, qPtr,
                         &CacheManager::updateCacheStatistics);
        QObject::connect(systemMemoryTimer, &QTimer::timeout, qPtr,
                         &CacheManager::handleSystemMemoryPressure);

        // Connect CachePresenter signals to CacheManager signals
        QObject::connect(presenter.get(), &CachePresenter::cacheHit, qPtr,
                         [qPtr](CacheType type, const QString& key) {
                             qPtr->notifyCacheHit(type, key);
                         });
        QObject::connect(presenter.get(), &CachePresenter::cacheMiss, qPtr,
                         [qPtr](CacheType type, const QString& key) {
                             qPtr->notifyCacheMiss(type, key);
                         });
        QObject::connect(presenter.get(),
                         &CachePresenter::memoryPressureWarning, qPtr,
                         &CacheManager::memoryPressureWarning);
        QObject::connect(presenter.get(),
                         &CachePresenter::memoryPressureCritical, qPtr,
                         &CacheManager::memoryPressureCritical);

        // Connect CacheManager signals to EventBus for decoupled communication
        connectToEventBus(qPtr);

        // Start timers
        cleanupTimer->start(config.cleanupInterval);
        memoryPressureTimer->start(5000);  // Check every 5 seconds
        statsUpdateTimer->start(10000);    // Update stats every 10 seconds
        systemMemoryTimer->start(config.systemMemoryCheckInterval);
    }

    ~Implementation() {
        // Stop and disconnect timers explicitly to prevent crashes during
        // static destruction. Don't delete them here - they will be deleted by
        // Qt's parent-child mechanism
        if (cleanupTimer != nullptr) {
            cleanupTimer->stop();
            cleanupTimer->disconnect();
        }
        if (memoryPressureTimer != nullptr) {
            memoryPressureTimer->stop();
            memoryPressureTimer->disconnect();
        }
        if (statsUpdateTimer != nullptr) {
            statsUpdateTimer->stop();
            statsUpdateTimer->disconnect();
        }
        if (systemMemoryTimer != nullptr) {
            systemMemoryTimer->stop();
            systemMemoryTimer->disconnect();
        }

        // Clear registered caches to prevent dangling pointers
        registeredCaches.clear();
    }

    // Disable copy and move operations
    Implementation(const Implementation&) = delete;
    Implementation& operator=(const Implementation&) = delete;
    Implementation(Implementation&&) = delete;
    Implementation& operator=(Implementation&&) = delete;

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

    // PERFORMANCE FIX: Prevent timer callback overlap to avoid event loop
    // starvation
    std::atomic<bool> timerCallbackActive{false};

    // Adaptive management
    bool adaptiveManagementEnabled = true;
    QHash<CacheType, double> usagePatterns;

    // Advanced memory management
    bool systemMemoryMonitoringEnabled = true;
    bool predictiveEvictionEnabled = true;
    bool memoryCompressionEnabled = false;
    bool emergencyEvictionEnabled = true;

    // Eviction strategies
    QHash<CacheType, QString> evictionStrategies;

    // Memory pressure thresholds
    double memoryPressureWarningThreshold = 0.75;
    double memoryPressureCriticalThreshold = 0.90;

    // MVP Architecture: CachePresenter for new cache operations
    std::unique_ptr<CachePresenter> presenter;

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
            if (it.value() != nullptr && cacheEnabled.value(it.key(), true)) {
                total += it.value()->getMemoryUsage();
            }
        }
        return total;
    }

    void performMemoryPressureEviction() const {
        qint64 totalUsage = calculateTotalMemoryUsage();
        qint64 targetUsage = static_cast<qint64>(config.totalMemoryLimit *
                                                 0.7);  // Target 70% usage

        if (totalUsage <= targetUsage) {
            return;
        }

        qint64 bytesToFree = totalUsage - targetUsage;

        // Check if any cache strategy plugin wants to handle eviction
        QList<ICacheStrategyPlugin*> cachePlugins =
            PluginManager::instance().getCacheStrategyPlugins();
        for (ICacheStrategyPlugin* plugin : cachePlugins) {
            if (plugin != nullptr) {
                // Build cache entry metadata for plugin decision
                QList<CacheEntryMetadata> entries;
                for (auto it = registeredCaches.constBegin();
                     it != registeredCaches.constEnd(); ++it) {
                    ICacheComponent* cache = it.value();
                    if (cache != nullptr) {
                        CacheEntryMetadata meta;
                        meta.key = QString::number(static_cast<int>(it.key()));
                        meta.size = cache->getMemoryUsage();
                        meta.accessCount = cache->getHitCount();
                        entries.append(meta);
                    }
                }

                // Let plugin suggest eviction candidate
                QString evictKey =
                    plugin->selectEvictionCandidate(entries, bytesToFree);
                if (!evictKey.isEmpty()) {
                    // Plugin provided a suggestion - log it for now
                    // In full implementation, would use this to guide eviction
                }
            }
        }

        // Prioritize eviction based on cache importance and usage patterns
        QList<QPair<CacheType, double>> evictionPriority;

        for (auto it = registeredCaches.constBegin();
             it != registeredCaches.constEnd(); ++it) {
            CacheType type = it.key();
            if (it.value() == nullptr || !cacheEnabled.value(type, true)) {
                continue;
            }

            double priority = calculateEvictionPriority(type);
            evictionPriority.append({type, priority});
        }

        // Sort by priority (lower priority = evict first)
        std::ranges::sort(evictionPriority,
                          [](const QPair<CacheType, double>& first,
                             const QPair<CacheType, double>& second) {
                              return first.second < second.second;
                          });

        // Evict from lowest priority caches first
        for (const auto& pair : evictionPriority) {
            if (bytesToFree <= 0) {
                break;
            }

            CacheType type = pair.first;
            ICacheComponent* cache = registeredCaches.value(type);
            if (cache == nullptr) {
                continue;
            }

            qint64 cacheUsage = cache->getMemoryUsage();
            qint64 toEvictFromCache =
                std::min(bytesToFree,
                         cacheUsage / 2);  // Evict up to 50% from each cache

            // Execute pre-evict hook for plugins
            PluginHookRegistry::instance().executeHook(
                StandardHooks::CACHE_PRE_EVICT,
                {{"cacheType", static_cast<int>(type)},
                 {"bytesToEvict", toEvictFromCache}});

            cache->evictLRU(toEvictFromCache);
            bytesToFree -= toEvictFromCache;

            // Execute post-evict hook for plugins
            PluginHookRegistry::instance().executeHook(
                StandardHooks::CACHE_POST_EVICT,
                {{"cacheType", static_cast<int>(type)},
                 {"bytesEvicted", toEvictFromCache}});

            emit q_ptr->cacheEvictionRequested(type, toEvictFromCache);
        }
    }

    static double calculateEvictionPriority(CacheType type) {
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
            if (cache == nullptr) {
                continue;
            }

            qint64 hits = cache->getHitCount();
            qint64 misses = cache->getMissCount();
            qint64 total = hits + misses;

            if (total > 0) {
                usagePatterns[type] =
                    static_cast<double>(hits) / static_cast<double>(total);
            }
        }
    }

    static void connectToEventBus(CacheManager* manager) {
        // Connect cache events to EventBus for decoupled communication
        EventBus& eventBus = EventBus::instance();

        // Memory pressure events
        QObject::connect(manager, &CacheManager::memoryLimitExceeded,
                         [](qint64 currentUsage, qint64 limit) {
                             QVariantMap data;
                             data["currentUsage"] = currentUsage;
                             data["limit"] = limit;
                             EventBus::instance().publish(
                                 "cache.memory.limitExceeded", data);
                         });

        QObject::connect(manager, &CacheManager::memoryPressureDetected,
                         [](double usageRatio) {
                             QVariantMap data;
                             data["usageRatio"] = usageRatio;
                             EventBus::instance().publish(
                                 "cache.memory.pressureDetected", data);
                         });

        QObject::connect(manager, &CacheManager::memoryPressureWarning,
                         [](double usageRatio) {
                             QVariantMap data;
                             data["usageRatio"] = usageRatio;
                             EventBus::instance().publish(
                                 "cache.memory.pressureWarning", data);
                         });

        QObject::connect(manager, &CacheManager::memoryPressureCritical,
                         [](double usageRatio) {
                             QVariantMap data;
                             data["usageRatio"] = usageRatio;
                             EventBus::instance().publish(
                                 "cache.memory.pressureCritical", data);
                         });

        QObject::connect(manager, &CacheManager::systemMemoryPressureDetected,
                         [](double systemUsageRatio) {
                             QVariantMap data;
                             data["systemUsageRatio"] = systemUsageRatio;
                             EventBus::instance().publish(
                                 "cache.system.memoryPressure", data);
                         });

        // Cache statistics events
        QObject::connect(manager, &CacheManager::cacheStatsUpdated,
                         [](CacheType type, const CacheStats& stats) {
                             QVariantMap data;
                             data["cacheType"] = static_cast<int>(type);
                             data["memoryUsage"] = stats.memoryUsage;
                             data["entryCount"] = stats.entryCount;
                             data["hitRatio"] = stats.hitRatio;
                             data["totalHits"] = stats.totalHits;
                             data["totalMisses"] = stats.totalMisses;
                             EventBus::instance().publish("cache.stats.updated",
                                                          data);
                         });

        QObject::connect(manager, &CacheManager::globalStatsUpdated,
                         [](qint64 totalMemory, double hitRatio) {
                             QVariantMap data;
                             data["totalMemory"] = totalMemory;
                             data["hitRatio"] = hitRatio;
                             EventBus::instance().publish("cache.stats.global",
                                                          data);
                         });

        // Cache operation events
        QObject::connect(manager, &CacheManager::cacheEvictionRequested,
                         [](CacheType type, qint64 bytesToFree) {
                             QVariantMap data;
                             data["cacheType"] = static_cast<int>(type);
                             data["bytesToFree"] = bytesToFree;
                             EventBus::instance().publish(
                                 "cache.eviction.requested", data);
                         });

        QObject::connect(manager, &CacheManager::emergencyEvictionTriggered,
                         [](qint64 bytesFreed) {
                             QVariantMap data;
                             data["bytesFreed"] = bytesFreed;
                             EventBus::instance().publish(
                                 "cache.eviction.emergency", data);
                         });

        QObject::connect(manager, &CacheManager::cacheConfigurationChanged,
                         []() {
                             EventBus::instance().publish(
                                 "cache.config.changed", QVariant());
                         });

        QObject::connect(manager, &CacheManager::memoryOptimizationCompleted,
                         [](qint64 memoryFreed) {
                             QVariantMap data;
                             data["memoryFreed"] = memoryFreed;
                             EventBus::instance().publish(
                                 "cache.optimization.completed", data);
                         });

        QObject::connect(manager, &CacheManager::cacheCompressionCompleted,
                         [](qint64 memorySaved) {
                             QVariantMap data;
                             data["memorySaved"] = memorySaved;
                             EventBus::instance().publish(
                                 "cache.compression.completed", data);
                         });
    }
};

CacheManager::CacheManager(QObject* parent)
    : QObject(parent), m_d(std::make_unique<Implementation>(this)) {
    m_d->initializeDefaultLimits();
}

CacheManager::~CacheManager() {
    // During static destruction, QCoreApplication might already be destroyed
    // In that case, we can't safely do anything, so just return
    if (QCoreApplication::instance() == nullptr) {
        return;
    }

    // Stop all timers before destruction to prevent crashes
    if (m_d) {
        // Stop timers first
        if (m_d->cleanupTimer != nullptr) {
            m_d->cleanupTimer->stop();
            disconnect(m_d->cleanupTimer, nullptr, this, nullptr);
        }
        if (m_d->memoryPressureTimer != nullptr) {
            m_d->memoryPressureTimer->stop();
            disconnect(m_d->memoryPressureTimer, nullptr, this, nullptr);
        }
        if (m_d->statsUpdateTimer != nullptr) {
            m_d->statsUpdateTimer->stop();
            disconnect(m_d->statsUpdateTimer, nullptr, this, nullptr);
        }
        if (m_d->systemMemoryTimer != nullptr) {
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

    // Delegate configuration to presenter
    m_d->presenter->setGlobalConfig(config);

    emit cacheConfigurationChanged();
}

CacheManager::GlobalCacheConfig CacheManager::getGlobalConfig() const {
    QMutexLocker locker(&m_d->mutex);
    // Use presenter's config for MVP-based caches if available
    // Fall back to local config for legacy caches
    return m_d->config;
}

void CacheManager::setCacheLimit(CacheType type, qint64 memoryLimit) {
    QMutexLocker locker(&m_d->mutex);
    m_d->cacheMemoryLimits[type] = memoryLimit;

    // Delegate to presenter for MVP-based caches
    m_d->presenter->setCacheLimit(type, memoryLimit);

    // Handle legacy ICacheComponent caches
    ICacheComponent* cache = m_d->registeredCaches.value(type);
    if (cache != nullptr) {
        cache->setMaxMemoryLimit(memoryLimit);
    }
}

qint64 CacheManager::getCacheLimit(CacheType type) const {
    QMutexLocker locker(&m_d->mutex);
    // Try MVP-based cache limit first
    qint64 mvpLimit = m_d->presenter->getCacheLimit(type);
    if (mvpLimit > 0) {
        return mvpLimit;
    }
    // Fall back to legacy limit
    return m_d->cacheMemoryLimits.value(type, 0);
}

void CacheManager::registerCache(CacheType type, QObject* cache) {
    QMutexLocker locker(&m_d->mutex);

    auto cacheComponent = dynamic_cast<ICacheComponent*>(cache);
    if (cacheComponent == nullptr) {
        SLOG_WARNING(
            "Cache object does not implement ICacheComponent interface");
        return;
    }

    m_d->registeredCaches[type] = cacheComponent;

    // Apply memory limit
    qint64 limit = m_d->cacheMemoryLimits.value(type, 0);
    if (limit > 0) {
        cacheComponent->setMaxMemoryLimit(limit);
    }

    SLOG_DEBUG_F("Registered cache type: {}", static_cast<int>(type));
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
    for (auto* cache : m_d->registeredCaches) {
        if (cache != nullptr) {
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
    if (cache != nullptr) {
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
    if (cache != nullptr) {
        cache->setEnabled(enabled);
    }
}

bool CacheManager::isCacheEnabled(CacheType type) const {
    QMutexLocker locker(&m_d->mutex);
    return m_d->cacheEnabled.value(type, true);
}

qint64 CacheManager::getTotalMemoryUsage() const {
    // For MVP-based caches, delegate to presenter
    // For legacy caches, use old calculation
    QMutexLocker locker(&m_d->mutex);

    // Get MVP-based cache usage
    qint64 mvpUsage = m_d->presenter->getTotalMemoryUsage();

    // Get legacy ICacheComponent usage
    qint64 legacyUsage = m_d->calculateTotalMemoryUsage();

    // Return the sum (MVP will be 0 until specialized caches migrate)
    return mvpUsage + legacyUsage;
}

qint64 CacheManager::getTotalMemoryLimit() const {
    QMutexLocker locker(&m_d->mutex);
    return m_d->config.totalMemoryLimit;
}

double CacheManager::getGlobalMemoryUsageRatio() const {
    qint64 usage = getTotalMemoryUsage();
    qint64 limit = getTotalMemoryLimit();
    return limit > 0 ? static_cast<double>(usage) / static_cast<double>(limit)
                     : 0.0;
}

void CacheManager::enforceMemoryLimits() {
    QMutexLocker locker(&m_d->mutex);

    // Delegate MVP-based cache management to presenter
    m_d->presenter->enforceMemoryLimits();

    // Handle legacy ICacheComponent caches
    qint64 totalUsage = m_d->calculateTotalMemoryUsage();
    if (totalUsage > m_d->config.totalMemoryLimit) {
        emit memoryLimitExceeded(totalUsage, m_d->config.totalMemoryLimit);
        m_d->performMemoryPressureEviction();
    }
}

void CacheManager::handleMemoryPressure() {
    QElapsedTimer timer;
    timer.start();

    // Delegate MVP-based memory pressure handling to presenter
    {
        QMutexLocker locker(&m_d->mutex);
        m_d->presenter->handleMemoryPressure();
    }

    // Handle legacy ICacheComponent caches
    double usageRatio = getGlobalMemoryUsageRatio();
    if (usageRatio >= m_d->memoryPressureWarningThreshold) {
        emit memoryPressureWarning(usageRatio);
    }
    if (usageRatio >= m_d->memoryPressureCriticalThreshold) {
        emit memoryPressureCritical(usageRatio);
    }
    if (usageRatio > m_d->config.memoryPressureThreshold / 100.0) {
        emit memoryPressureDetected(usageRatio);

        QMutexLocker locker(&m_d->mutex);
        m_d->performMemoryPressureEviction();
    }

    qint64 elapsedMs = timer.elapsed();
    SLOG_DEBUG_F(
        "CacheManager::handleMemoryPressure completed in {} ms "
        "(usageRatio={})",
        elapsedMs, usageRatio);
}

void CacheManager::performPeriodicCleanup() {
    // PERFORMANCE FIX: Skip if another timer callback is running
    bool expected = false;
    if (!m_d->timerCallbackActive.compare_exchange_strong(expected, true)) {
        SLOG_DEBUG(
            "CacheManager::performPeriodicCleanup skipped - another callback "
            "active");
        return;
    }

    QElapsedTimer timer;
    timer.start();

    if (m_d->config.enableMemoryPressureEviction) {
        handleMemoryPressure();
    }

    if (m_d->adaptiveManagementEnabled &&
        m_d->config.enableAdaptiveMemoryManagement) {
        analyzeUsagePatterns();
        optimizeCacheDistribution();
    }

    qint64 elapsedMs = timer.elapsed();
    SLOG_DEBUG_F("CacheManager::performPeriodicCleanup completed in {} ms",
                 elapsedMs);

    m_d->timerCallbackActive.store(false);
}

void CacheManager::onMemoryPressureTimer() {
    // PERFORMANCE FIX: Skip if another timer callback is running
    bool expected = false;
    if (!m_d->timerCallbackActive.compare_exchange_strong(expected, true)) {
        SLOG_DEBUG(
            "CacheManager::onMemoryPressureTimer skipped - another callback "
            "active");
        return;
    }

    handleMemoryPressure();
    m_d->timerCallbackActive.store(false);
}

void CacheManager::updateCacheStatistics() {
    // PERFORMANCE FIX: Skip if another timer callback is running
    bool expected = false;
    if (!m_d->timerCallbackActive.compare_exchange_strong(expected, true)) {
        SLOG_DEBUG(
            "CacheManager::updateCacheStatistics skipped - another callback "
            "active");
        return;
    }

    QMutexLocker locker(&m_d->mutex);

    for (auto it = m_d->registeredCaches.constBegin();
         it != m_d->registeredCaches.constEnd(); ++it) {
        CacheType type = it.key();
        ICacheComponent* cache = it.value();
        if (cache == nullptr) {
            continue;
        }

        CacheStats stats;
        stats.memoryUsage = cache->getMemoryUsage();
        stats.maxMemoryLimit = cache->getMaxMemoryLimit();
        stats.entryCount = cache->getEntryCount();
        stats.totalHits = cache->getHitCount();
        stats.totalMisses = cache->getMissCount();

        qint64 total = stats.totalHits + stats.totalMisses;
        stats.hitRatio = total > 0 ? static_cast<double>(stats.totalHits) /
                                         static_cast<double>(total)
                                   : 0.0;

        emit cacheStatsUpdated(type, stats);
    }

    qint64 totalMemory = m_d->calculateTotalMemoryUsage();
    double globalHitRatio = getGlobalHitRatio();
    emit globalStatsUpdated(totalMemory, globalHitRatio);

    m_d->timerCallbackActive.store(false);
}

CacheStats CacheManager::getCacheStats(CacheType type) const {
    QMutexLocker locker(&m_d->mutex);

    CacheStats stats;

    // Try MVP-based cache first
    CacheStats mvpStats = m_d->presenter->getStats(type);
    if (mvpStats.entryCount > 0 || mvpStats.memoryUsage > 0) {
        return mvpStats;
    }

    // Fall back to legacy ICacheComponent
    ICacheComponent* cache = m_d->registeredCaches.value(type);
    if (cache == nullptr) {
        return stats;
    }

    stats.memoryUsage = cache->getMemoryUsage();
    stats.maxMemoryLimit = cache->getMaxMemoryLimit();
    stats.entryCount = cache->getEntryCount();
    stats.totalHits = cache->getHitCount();
    stats.totalMisses = cache->getMissCount();

    qint64 total = stats.totalHits + stats.totalMisses;
    stats.hitRatio = total > 0 ? static_cast<double>(stats.totalHits) /
                                     static_cast<double>(total)
                               : 0.0;

    return stats;
}

double CacheManager::getGlobalHitRatio() const {
    QMutexLocker locker(&m_d->mutex);

    // Get MVP-based cache hit ratio
    double mvpRatio = m_d->presenter->getGlobalHitRatio();

    // Get legacy cache hit ratio
    qint64 totalHits = 0;
    qint64 totalMisses = 0;

    for (auto* cache : m_d->registeredCaches) {
        if (cache != nullptr) {
            totalHits += cache->getHitCount();
            totalMisses += cache->getMissCount();
        }
    }

    qint64 total = totalHits + totalMisses;
    double legacyRatio =
        total > 0 ? static_cast<double>(totalHits) / static_cast<double>(total)
                  : 0.0;

    // Aggregate both sources (weight by usage if both have data)
    if (mvpRatio > 0 && legacyRatio > 0) {
        return (mvpRatio + legacyRatio) / 2.0;  // Simple average
    }
    return mvpRatio > 0 ? mvpRatio : legacyRatio;
}

QHash<CacheType, CacheStats> CacheManager::getAllCacheStats() const {
    QMutexLocker locker(&m_d->mutex);

    QHash<CacheType, CacheStats> allStats;

    // Get MVP-based cache stats
    QHash<CacheType, CacheStats> mvpStats = m_d->presenter->getAllStats();
    for (auto it = mvpStats.constBegin(); it != mvpStats.constEnd(); ++it) {
        allStats.insert(it.key(), it.value());
    }

    // Add or update with legacy ICacheComponent stats
    for (auto it = m_d->registeredCaches.constBegin();
         it != m_d->registeredCaches.constEnd(); ++it) {
        CacheType type = it.key();
        ICacheComponent* cache = it.value();

        if (cache != nullptr) {
            CacheStats stats;
            stats.memoryUsage = cache->getMemoryUsage();
            stats.maxMemoryLimit = cache->getMaxMemoryLimit();
            stats.entryCount = cache->getEntryCount();
            stats.totalHits = cache->getHitCount();
            stats.totalMisses = cache->getMissCount();

            qint64 total = stats.totalHits + stats.totalMisses;
            stats.hitRatio = total > 0 ? static_cast<double>(stats.totalHits) /
                                             static_cast<double>(total)
                                       : 0.0;

            // Only add if not already in MVP stats
            if (!allStats.contains(type)) {
                allStats[type] = stats;
            }
        }
    }

    return allStats;
}

qint64 CacheManager::getTotalCacheHits() const {
    QMutexLocker locker(&m_d->mutex);

    qint64 totalHits = 0;

    for (auto* cache : m_d->registeredCaches) {
        if (cache != nullptr) {
            totalHits += cache->getHitCount();
        }
    }

    return totalHits;
}

qint64 CacheManager::getTotalCacheMisses() const {
    QMutexLocker locker(&m_d->mutex);

    qint64 totalMisses = 0;

    for (auto* cache : m_d->registeredCaches) {
        if (cache != nullptr) {
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
    constexpr int MAX_TRACKED_ACCESSES = 1000;
    if (accesses.size() > MAX_TRACKED_ACCESSES) {
        accesses = accesses.mid(0, MAX_TRACKED_ACCESSES);
    }
}

void CacheManager::notifyCacheHit(CacheType type, const QString& key) {
    QMutexLocker locker(&m_d->mutex);
    m_d->cacheHits[type]++;

    // Update recent access for LRU tracking
    QStringList& recent = m_d->recentAccesses[type];
    recent.removeAll(key);
    recent.prepend(key);
    if (recent.size() > MAX_TRACKED_ACCESSES) {
        recent = recent.mid(0, MAX_TRACKED_ACCESSES);
    }
}

void CacheManager::notifyCacheMiss(CacheType /*type*/, const QString& /*key*/) {
    QMutexLocker locker(&m_d->mutex);
    // Parameters 'type' and 'key' are intentionally unused for cache miss
    // tracking
}

void CacheManager::analyzeUsagePatterns() {
    QMutexLocker locker(&m_d->mutex);
    m_d->updateUsagePatterns();
}

void CacheManager::optimizeCacheDistribution() {
    QMutexLocker locker(&m_d->mutex);

    // Redistribute memory based on usage patterns
    qint64 totalLimit = m_d->config.totalMemoryLimit;

    // Calculate new limits based on hit ratios and importance
    for (auto it = m_d->usagePatterns.constBegin();
         it != m_d->usagePatterns.constEnd(); ++it) {
        CacheType type = it.key();
        double hitRatio = it.value();
        double importance =
            CacheManager::Implementation::calculateEvictionPriority(type);

        // Allocate more memory to caches with higher hit ratios and importance
        double factor = ((hitRatio * 0.7) + (importance * 0.3));
        auto newLimit =
            static_cast<qint64>(static_cast<double>(totalLimit) * factor *
                                0.15);  // Max 15% per cache

        // Ensure minimum limits
        qint64 minLimit = static_cast<qint64>(static_cast<double>(totalLimit) *
                                              0.05);  // At least 5% per cache
        newLimit = std::max(newLimit, minLimit);

        m_d->cacheMemoryLimits[type] = newLimit;

        ICacheComponent* cache = m_d->registeredCaches.value(type);
        if (cache != nullptr) {
            cache->setMaxMemoryLimit(newLimit);
        }
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

qint64 CacheManager::getSystemMemoryUsage() {
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

qint64 CacheManager::getSystemMemoryTotal() {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo) != 0) {
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
        return static_cast<double>(usage) / static_cast<double>(total);
    }
    return 0.0;
}

void CacheManager::handleSystemMemoryPressure() {
    // PERFORMANCE FIX: Skip if another timer callback is running
    bool expected = false;
    if (!m_d->timerCallbackActive.compare_exchange_strong(expected, true)) {
        SLOG_DEBUG(
            "CacheManager::handleSystemMemoryPressure skipped - another "
            "callback active");
        return;
    }

    if (!m_d->systemMemoryMonitoringEnabled) {
        m_d->timerCallbackActive.store(false);
        return;
    }

    double systemPressure = getSystemMemoryPressure();

    if (systemPressure > m_d->config.systemMemoryPressureThreshold) {
        emit systemMemoryPressureDetected(systemPressure);

        // Trigger aggressive cache eviction
        QMutexLocker locker(&m_d->mutex);

        if (!m_d->emergencyEvictionEnabled) {
            return;
        }

        // Calculate how much memory to free (aim for 10% below threshold)
        qint64 totalSystemMemory = getSystemMemoryTotal();
        auto targetUsage = static_cast<qint64>(
            static_cast<double>(totalSystemMemory) *
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
                if (cache == nullptr || !m_d->cacheEnabled.value(type, true)) {
                    continue;
                }

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

    m_d->timerCallbackActive.store(false);
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
    QMutexLocker locker(&m_d->mutex);

    qint64 memoryFreed = 0;

    for (auto it = m_d->registeredCaches.constBegin();
         it != m_d->registeredCaches.constEnd(); ++it) {
        CacheType type = it.key();
        ICacheComponent* cache = it.value();
        if (cache == nullptr || !m_d->cacheEnabled.value(type, true)) {
            continue;
        }

        qint64 before = cache->getMemoryUsage();
        if (before <= 0) {
            continue;
        }

        qint64 toFree = before / 5;
        if (toFree > 0) {
            cache->evictLRU(toFree);
        }

        qint64 after = cache->getMemoryUsage();
        if (after < before) {
            memoryFreed += (before - after);
        }
    }

    emit cacheCompressionCompleted(memoryFreed);
}

void CacheManager::optimizeMemoryLayout() {
    QMutexLocker locker(&m_d->mutex);

    qint64 memoryFreed = 0;

    // Perform memory optimization across all caches
    for (auto it = m_d->registeredCaches.constBegin();
         it != m_d->registeredCaches.constEnd(); ++it) {
        CacheType type = it.key();
        ICacheComponent* cache = it.value();
        if (cache == nullptr || !m_d->cacheEnabled.value(type, true)) {
            continue;
        }

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
    if (cache == nullptr) {
        return;
    }

    // Calculate how many items to evict based on bytes to free
    qint64 currentUsage = cache->getMemoryUsage();
    if (currentUsage > 0) {
        // Execute pre-evict hook for plugins
        PluginHookRegistry::instance().executeHook(
            StandardHooks::CACHE_PRE_EVICT,
            {{"cacheType", static_cast<int>(type)},
             {"bytesToEvict", bytesToFree},
             {"currentUsage", currentUsage}});

        // Try to free the requested amount of memory
        cache->evictLRU(bytesToFree);

        // Execute post-evict hook for plugins
        PluginHookRegistry::instance().executeHook(
            StandardHooks::CACHE_POST_EVICT,
            {{"cacheType", static_cast<int>(type)},
             {"bytesEvicted", bytesToFree}});
    }
}

void CacheManager::stopAllTimers() {
    QMutexLocker locker(&m_d->mutex);

    // Stop all timers to prevent crashes during static destruction
    if (m_d->cleanupTimer != nullptr) {
        m_d->cleanupTimer->stop();
    }
    if (m_d->memoryPressureTimer != nullptr) {
        m_d->memoryPressureTimer->stop();
    }
    if (m_d->statsUpdateTimer != nullptr) {
        m_d->statsUpdateTimer->stop();
    }
    if (m_d->systemMemoryTimer != nullptr) {
        m_d->systemMemoryTimer->stop();
    }
}
