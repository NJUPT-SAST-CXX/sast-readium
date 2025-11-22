#include "CachePresenter.h"
#include <QElapsedTimer>
#include <QList>
#include "logging/SimpleLogging.h"
#include "model/CacheEntryModel.h"

class CachePresenter::Implementation {
public:
    std::unique_ptr<CacheDataModel> dataModel;
    std::unique_ptr<CacheConfigModel> configModel;
    std::unique_ptr<CacheStatsModel> statsModel;

    QList<ICacheView*> views;
    QList<ICacheStatsView*> statsViews;
    QList<ICacheConfigView*> configViews;
    QList<ICacheMemoryView*> memoryViews;

    QMutex viewsMutex;

    explicit Implementation()
        : dataModel(std::make_unique<CacheDataModel>()),
          configModel(std::make_unique<CacheConfigModel>()),
          statsModel(std::make_unique<CacheStatsModel>()) {}
};

CachePresenter::CachePresenter(QObject* parent)
    : QObject(parent), m_impl(std::make_unique<Implementation>()) {}

CachePresenter::~CachePresenter() = default;

void CachePresenter::registerView(ICacheView* view) {
    if (view != nullptr) {
        QMutexLocker locker(&m_impl->viewsMutex);
        if (!m_impl->views.contains(view)) {
            m_impl->views.append(view);
        }
    }
}

void CachePresenter::registerStatsView(ICacheStatsView* view) {
    if (view != nullptr) {
        QMutexLocker locker(&m_impl->viewsMutex);
        if (!m_impl->statsViews.contains(view)) {
            m_impl->statsViews.append(view);
        }
    }
}

void CachePresenter::registerConfigView(ICacheConfigView* view) {
    if (view != nullptr) {
        QMutexLocker locker(&m_impl->viewsMutex);
        if (!m_impl->configViews.contains(view)) {
            m_impl->configViews.append(view);
        }
    }
}

void CachePresenter::registerMemoryView(ICacheMemoryView* view) {
    if (view != nullptr) {
        QMutexLocker locker(&m_impl->viewsMutex);
        if (!m_impl->memoryViews.contains(view)) {
            m_impl->memoryViews.append(view);
        }
    }
}

void CachePresenter::unregisterView(ICacheView* view) {
    QMutexLocker locker(&m_impl->viewsMutex);
    m_impl->views.removeAll(view);
}

void CachePresenter::unregisterStatsView(ICacheStatsView* view) {
    QMutexLocker locker(&m_impl->viewsMutex);
    m_impl->statsViews.removeAll(view);
}

void CachePresenter::unregisterConfigView(ICacheConfigView* view) {
    QMutexLocker locker(&m_impl->viewsMutex);
    m_impl->configViews.removeAll(view);
}

void CachePresenter::unregisterMemoryView(ICacheMemoryView* view) {
    QMutexLocker locker(&m_impl->viewsMutex);
    m_impl->memoryViews.removeAll(view);
}

CacheDataModel* CachePresenter::getDataModel() const {
    return m_impl->dataModel.get();
}

CacheConfigModel* CachePresenter::getConfigModel() const {
    return m_impl->configModel.get();
}

CacheStatsModel* CachePresenter::getStatsModel() const {
    return m_impl->statsModel.get();
}

bool CachePresenter::insert(const QString& key, const QVariant& data,
                            CacheType type, int priority) {
    // Create cache entry
    CacheEntryModel entry(key, data, type);
    entry.setPriority(priority);

    // Insert into data model
    bool success = m_impl->dataModel->insert(entry);

    if (success) {
        // Update statistics
        m_impl->statsModel->recordEntryCount(
            type, m_impl->dataModel->getEntryCountByType(type));
        m_impl->statsModel->recordMemoryUsage(
            type, m_impl->dataModel->getMemoryUsageByType(type));

        // Notify views
        notifyViews(type, key);
        notifyStatsViews();

        // Check memory limits
        checkMemoryPressure();

        SLOG_DEBUG_F("CachePresenter: Inserted entry {} for type {}",
                     key.toStdString(), static_cast<int>(type));
    }

    return success;
}

QVariant CachePresenter::get(const QString& key, CacheType type) {
    CacheEntryModel* entry = m_impl->dataModel->get(key);

    if (entry != nullptr && entry->getType() == type) {
        // Update access information
        entry->updateAccess();

        // Record hit
        m_impl->statsModel->recordHit(type);
        m_impl->statsModel->recordAccess(type, key);

        emit cacheHit(type, key);

        SLOG_DEBUG_F("CachePresenter: Cache hit for key {}", key.toStdString());
        return entry->getData();
    }

    // Record miss
    m_impl->statsModel->recordMiss(type);
    emit cacheMiss(type, key);

    SLOG_DEBUG_F("CachePresenter: Cache miss for key {}", key.toStdString());
    return QVariant();
}

bool CachePresenter::contains(const QString& key, CacheType type) const {
    const CacheEntryModel* entry = m_impl->dataModel->get(key);
    return entry != nullptr && entry->getType() == type;
}

bool CachePresenter::remove(const QString& key, CacheType type) {
    const CacheEntryModel* entry = m_impl->dataModel->get(key);
    if (entry == nullptr || entry->getType() != type) {
        return false;
    }

    bool success = m_impl->dataModel->remove(key);

    if (success) {
        // Update statistics
        m_impl->statsModel->recordEntryCount(
            type, m_impl->dataModel->getEntryCountByType(type));
        m_impl->statsModel->recordMemoryUsage(
            type, m_impl->dataModel->getMemoryUsageByType(type));

        // Notify views
        QMutexLocker locker(&m_impl->viewsMutex);
        for (auto* view : m_impl->views) {
            view->onCacheEvicted(type, key, "Manual removal");
        }

        notifyStatsViews();
    }

    return success;
}

void CachePresenter::clear(CacheType type) {
    // Get all keys of this type before clearing
    QList<CacheEntryModel> entries = m_impl->dataModel->getEntriesByType(type);

    // Remove each entry
    for (const auto& entry : entries) {
        m_impl->dataModel->remove(entry.getKey());
    }

    // Update statistics
    m_impl->statsModel->recordEntryCount(type, 0);
    m_impl->statsModel->recordMemoryUsage(type, 0);

    // Notify views
    QMutexLocker locker(&m_impl->viewsMutex);
    for (auto* view : m_impl->views) {
        view->onCacheCleared(type);
    }

    notifyStatsViews();

    SLOG_INFO_F("CachePresenter: Cleared cache for type {}",
                static_cast<int>(type));
}

void CachePresenter::clearAll() {
    m_impl->dataModel->clear();
    m_impl->statsModel->reset();

    // Notify all views for all types
    QMutexLocker locker(&m_impl->viewsMutex);
    for (auto* view : m_impl->views) {
        view->onCacheCleared(CacheType::SearchResultCache);
        view->onCacheCleared(CacheType::PageTextCache);
        view->onCacheCleared(CacheType::SearchHighlightCache);
        view->onCacheCleared(CacheType::PdfRenderCache);
        view->onCacheCleared(CacheType::ThumbnailCache);
    }

    notifyStatsViews();

    SLOG_INFO("CachePresenter: Cleared all caches");
}

void CachePresenter::enforceMemoryLimits() {
    qint64 totalUsage = m_impl->dataModel->getTotalMemoryUsage();
    qint64 totalLimit = m_impl->configModel->getTotalMemoryLimit();

    if (totalUsage <= totalLimit) {
        return;
    }

    SLOG_INFO_F(
        "CachePresenter: Enforcing memory limits - usage: {}, limit: {}",
        totalUsage, totalLimit);

    // Calculate target usage (90% of limit)
    qint64 targetUsage =
        static_cast<qint64>(static_cast<double>(totalLimit) * 0.9);

    // Evict to target
    qint64 freedBytes = m_impl->dataModel->evictToTargetMemory(targetUsage);

    if (freedBytes > 0) {
        emit cacheEvictionOccurred(CacheType::SearchResultCache, freedBytes);

        // Update statistics for all types
        for (auto type :
             {CacheType::SearchResultCache, CacheType::PageTextCache,
              CacheType::SearchHighlightCache, CacheType::PdfRenderCache,
              CacheType::ThumbnailCache}) {
            m_impl->statsModel->recordEntryCount(
                type, m_impl->dataModel->getEntryCountByType(type));
            m_impl->statsModel->recordMemoryUsage(
                type, m_impl->dataModel->getMemoryUsageByType(type));
        }

        notifyStatsViews();
    }
}

void CachePresenter::handleMemoryPressure() {
    QElapsedTimer timer;
    timer.start();

    double usageRatio = getMemoryUsageRatio();

    double warningThreshold =
        m_impl->configModel->getMemoryPressureWarningThreshold();
    double criticalThreshold =
        m_impl->configModel->getMemoryPressureCriticalThreshold();

    if (usageRatio >= warningThreshold) {
        emit memoryPressureWarning(usageRatio);
        notifyMemoryViews(getTotalMemoryUsage(),
                          m_impl->configModel->getTotalMemoryLimit());
    }

    if (usageRatio >= criticalThreshold) {
        emit memoryPressureCritical(usageRatio);
        enforceMemoryLimits();
    }

    qint64 elapsed = timer.elapsed();
    SLOG_DEBUG_F("CachePresenter::handleMemoryPressure completed in {} ms",
                 elapsed);
}

qint64 CachePresenter::getTotalMemoryUsage() const {
    return m_impl->dataModel->getTotalMemoryUsage();
}

double CachePresenter::getMemoryUsageRatio() const {
    qint64 usage = getTotalMemoryUsage();
    qint64 limit = m_impl->configModel->getTotalMemoryLimit();
    return limit > 0 ? static_cast<double>(usage) / static_cast<double>(limit)
                     : 0.0;
}

CacheStats CachePresenter::getStats(CacheType type) const {
    return m_impl->statsModel->getStats(type);
}

QHash<CacheType, CacheStats> CachePresenter::getAllStats() const {
    return m_impl->statsModel->getAllStats();
}

double CachePresenter::getGlobalHitRatio() const {
    return m_impl->statsModel->getGlobalHitRatio();
}

void CachePresenter::setGlobalConfig(const GlobalCacheConfig& config) {
    m_impl->configModel->fromGlobalCacheConfig(config);
    notifyConfigViews(CacheType::SearchResultCache);  // Notify all types
}

GlobalCacheConfig CachePresenter::getGlobalConfig() const {
    return m_impl->configModel->toGlobalCacheConfig();
}

void CachePresenter::setCacheLimit(CacheType type, qint64 limit) {
    m_impl->configModel->setCacheLimit(type, limit);
    notifyConfigViews(type);
}

qint64 CachePresenter::getCacheLimit(CacheType type) const {
    return m_impl->configModel->getCacheLimit(type);
}

void CachePresenter::evictLRU(CacheType type, qint64 bytesToFree) {
    QList<CacheEntryModel> entries = m_impl->dataModel->getEntriesByType(type);

    // Sort by LRU
    std::sort(entries.begin(), entries.end(),
              [](const CacheEntryModel& a, const CacheEntryModel& b) {
                  return a.getLastAccessed() < b.getLastAccessed();
              });

    qint64 freedBytes = 0;
    for (const auto& entry : entries) {
        if (freedBytes >= bytesToFree) {
            break;
        }

        qint64 entrySize = entry.getMemorySize();
        m_impl->dataModel->remove(entry.getKey());
        freedBytes += entrySize;

        m_impl->statsModel->recordEviction(type, entrySize);

        // Notify views
        QMutexLocker locker(&m_impl->viewsMutex);
        for (auto* view : m_impl->views) {
            view->onCacheEvicted(type, entry.getKey(), "LRU eviction");
        }
    }

    if (freedBytes > 0) {
        // Update statistics
        m_impl->statsModel->recordEntryCount(
            type, m_impl->dataModel->getEntryCountByType(type));
        m_impl->statsModel->recordMemoryUsage(
            type, m_impl->dataModel->getMemoryUsageByType(type));

        emit cacheEvictionOccurred(type, freedBytes);
        notifyStatsViews();
    }
}

void CachePresenter::evictExpired(qint64 maxAge) {
    int removed = m_impl->dataModel->removeExpiredEntries(maxAge);

    if (removed > 0) {
        // Update statistics for all types
        for (auto type :
             {CacheType::SearchResultCache, CacheType::PageTextCache,
              CacheType::SearchHighlightCache, CacheType::PdfRenderCache,
              CacheType::ThumbnailCache}) {
            m_impl->statsModel->recordEntryCount(
                type, m_impl->dataModel->getEntryCountByType(type));
            m_impl->statsModel->recordMemoryUsage(
                type, m_impl->dataModel->getMemoryUsageByType(type));
        }

        notifyStatsViews();
        SLOG_INFO_F("CachePresenter: Evicted {} expired entries", removed);
    }
}

void CachePresenter::performAdaptiveEviction() {
    // Analyze usage patterns and evict accordingly
    for (auto type : {CacheType::SearchResultCache, CacheType::PageTextCache,
                      CacheType::SearchHighlightCache,
                      CacheType::PdfRenderCache, CacheType::ThumbnailCache}) {
        qint64 usage = m_impl->dataModel->getMemoryUsageByType(type);
        qint64 limit = m_impl->configModel->getCacheLimit(type);

        if (usage > limit) {
            qint64 toFree = usage - limit;
            evictLRU(type, toFree);
        }
    }
}

void CachePresenter::notifyViews(CacheType type, const QString& key) {
    QMutexLocker locker(&m_impl->viewsMutex);
    for (auto* view : m_impl->views) {
        view->onCacheUpdated(type, key);
    }
}

void CachePresenter::notifyStatsViews() {
    QMutexLocker locker(&m_impl->viewsMutex);

    for (auto* view : m_impl->statsViews) {
        // Notify for each cache type
        for (auto type :
             {CacheType::SearchResultCache, CacheType::PageTextCache,
              CacheType::SearchHighlightCache, CacheType::PdfRenderCache,
              CacheType::ThumbnailCache}) {
            CacheStats stats = m_impl->statsModel->getStats(type);
            view->onStatsUpdated(type, stats);
        }

        // Notify global stats
        view->onGlobalStatsUpdated(getTotalMemoryUsage(), getGlobalHitRatio());
    }
}

void CachePresenter::notifyConfigViews(CacheType type) {
    QMutexLocker locker(&m_impl->viewsMutex);
    for (auto* view : m_impl->configViews) {
        view->onConfigChanged(type);
        view->onGlobalConfigChanged();
    }
}

void CachePresenter::notifyMemoryViews(qint64 usage, qint64 limit) {
    QMutexLocker locker(&m_impl->viewsMutex);
    for (auto* view : m_impl->memoryViews) {
        view->onMemoryLimitExceeded(usage, limit);
    }
}

void CachePresenter::checkMemoryPressure() {
    double usageRatio = getMemoryUsageRatio();
    double threshold = m_impl->configModel->getMemoryPressureThreshold();

    if (usageRatio > threshold) {
        handleMemoryPressure();
    }
}
