#include "CacheConfigModel.h"
#include <QMutexLocker>

using enum CacheType;

CacheConfigModel::CacheConfigModel()
    : m_totalMemoryLimit(512LL * 1024 * 1024),
      m_cleanupInterval(30000),
      m_memoryPressureThreshold(0.85),
      m_memoryPressureWarningThreshold(0.75),
      m_memoryPressureCriticalThreshold(0.90),
      m_lruEvictionEnabled(true),
      m_memoryPressureEvictionEnabled(true),
      m_cacheCoordinationEnabled(true),
      m_adaptiveMemoryManagementEnabled(true),
      m_cachePreloadingEnabled(true),
      m_systemMemoryMonitoringEnabled(true),
      m_predictiveEvictionEnabled(true),
      m_memoryCompressionEnabled(false),
      m_emergencyEvictionEnabled(true),
      m_systemMemoryCheckInterval(10000),
      m_systemMemoryPressureThreshold(0.85) {
    initializeDefaults();
}

void CacheConfigModel::initializeDefaults() {
    // Set default limits for each cache type
    m_cacheLimits[SearchResultCache] = 100LL * 1024 * 1024;
    m_cacheLimits[PageTextCache] = 50LL * 1024 * 1024;
    m_cacheLimits[SearchHighlightCache] = 25LL * 1024 * 1024;
    m_cacheLimits[PdfRenderCache] = 256LL * 1024 * 1024;
    m_cacheLimits[ThumbnailCache] = 81LL * 1024 * 1024;

    // Set default eviction strategies
    for (auto type : {SearchResultCache, PageTextCache, SearchHighlightCache,
                      PdfRenderCache, ThumbnailCache}) {
        m_evictionStrategies[type] = "LRU";
        m_cacheEnabled[type] = true;
    }
}

qint64 CacheConfigModel::getTotalMemoryLimit() const {
    QMutexLocker locker(&m_mutex);
    return m_totalMemoryLimit;
}

void CacheConfigModel::setTotalMemoryLimit(qint64 limit) {
    QMutexLocker locker(&m_mutex);
    m_totalMemoryLimit = limit;
}

qint64 CacheConfigModel::getCleanupInterval() const {
    QMutexLocker locker(&m_mutex);
    return m_cleanupInterval;
}

void CacheConfigModel::setCleanupInterval(qint64 interval) {
    QMutexLocker locker(&m_mutex);
    m_cleanupInterval = interval;
}

double CacheConfigModel::getMemoryPressureThreshold() const {
    QMutexLocker locker(&m_mutex);
    return m_memoryPressureThreshold;
}

void CacheConfigModel::setMemoryPressureThreshold(double threshold) {
    QMutexLocker locker(&m_mutex);
    m_memoryPressureThreshold = threshold;
}

double CacheConfigModel::getMemoryPressureWarningThreshold() const {
    QMutexLocker locker(&m_mutex);
    return m_memoryPressureWarningThreshold;
}

void CacheConfigModel::setMemoryPressureWarningThreshold(double threshold) {
    QMutexLocker locker(&m_mutex);
    m_memoryPressureWarningThreshold = threshold;
}

double CacheConfigModel::getMemoryPressureCriticalThreshold() const {
    QMutexLocker locker(&m_mutex);
    return m_memoryPressureCriticalThreshold;
}

void CacheConfigModel::setMemoryPressureCriticalThreshold(double threshold) {
    QMutexLocker locker(&m_mutex);
    m_memoryPressureCriticalThreshold = threshold;
}

qint64 CacheConfigModel::getCacheLimit(CacheType type) const {
    QMutexLocker locker(&m_mutex);
    return m_cacheLimits.value(type, 0);
}

void CacheConfigModel::setCacheLimit(CacheType type, qint64 limit) {
    QMutexLocker locker(&m_mutex);
    m_cacheLimits[type] = limit;
}

QString CacheConfigModel::getEvictionStrategy(CacheType type) const {
    QMutexLocker locker(&m_mutex);
    return m_evictionStrategies.value(type, "LRU");
}

void CacheConfigModel::setEvictionStrategy(CacheType type,
                                           const QString& strategy) {
    QMutexLocker locker(&m_mutex);
    m_evictionStrategies[type] = strategy;
}

bool CacheConfigModel::isCacheEnabled(CacheType type) const {
    QMutexLocker locker(&m_mutex);
    return m_cacheEnabled.value(type, true);
}

void CacheConfigModel::setCacheEnabled(CacheType type, bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_cacheEnabled[type] = enabled;
}

bool CacheConfigModel::isLRUEvictionEnabled() const {
    QMutexLocker locker(&m_mutex);
    return m_lruEvictionEnabled;
}

void CacheConfigModel::setLRUEvictionEnabled(bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_lruEvictionEnabled = enabled;
}

bool CacheConfigModel::isMemoryPressureEvictionEnabled() const {
    QMutexLocker locker(&m_mutex);
    return m_memoryPressureEvictionEnabled;
}

void CacheConfigModel::setMemoryPressureEvictionEnabled(bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_memoryPressureEvictionEnabled = enabled;
}

bool CacheConfigModel::isCacheCoordinationEnabled() const {
    QMutexLocker locker(&m_mutex);
    return m_cacheCoordinationEnabled;
}

void CacheConfigModel::setCacheCoordinationEnabled(bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_cacheCoordinationEnabled = enabled;
}

bool CacheConfigModel::isAdaptiveMemoryManagementEnabled() const {
    QMutexLocker locker(&m_mutex);
    return m_adaptiveMemoryManagementEnabled;
}

void CacheConfigModel::setAdaptiveMemoryManagementEnabled(bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_adaptiveMemoryManagementEnabled = enabled;
}

bool CacheConfigModel::isCachePreloadingEnabled() const {
    QMutexLocker locker(&m_mutex);
    return m_cachePreloadingEnabled;
}

void CacheConfigModel::setCachePreloadingEnabled(bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_cachePreloadingEnabled = enabled;
}

bool CacheConfigModel::isSystemMemoryMonitoringEnabled() const {
    QMutexLocker locker(&m_mutex);
    return m_systemMemoryMonitoringEnabled;
}

void CacheConfigModel::setSystemMemoryMonitoringEnabled(bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_systemMemoryMonitoringEnabled = enabled;
}

bool CacheConfigModel::isPredictiveEvictionEnabled() const {
    QMutexLocker locker(&m_mutex);
    return m_predictiveEvictionEnabled;
}

void CacheConfigModel::setPredictiveEvictionEnabled(bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_predictiveEvictionEnabled = enabled;
}

bool CacheConfigModel::isMemoryCompressionEnabled() const {
    QMutexLocker locker(&m_mutex);
    return m_memoryCompressionEnabled;
}

void CacheConfigModel::setMemoryCompressionEnabled(bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_memoryCompressionEnabled = enabled;
}

bool CacheConfigModel::isEmergencyEvictionEnabled() const {
    QMutexLocker locker(&m_mutex);
    return m_emergencyEvictionEnabled;
}

void CacheConfigModel::setEmergencyEvictionEnabled(bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_emergencyEvictionEnabled = enabled;
}

qint64 CacheConfigModel::getSystemMemoryCheckInterval() const {
    QMutexLocker locker(&m_mutex);
    return m_systemMemoryCheckInterval;
}

void CacheConfigModel::setSystemMemoryCheckInterval(qint64 interval) {
    QMutexLocker locker(&m_mutex);
    m_systemMemoryCheckInterval = interval;
}

double CacheConfigModel::getSystemMemoryPressureThreshold() const {
    QMutexLocker locker(&m_mutex);
    return m_systemMemoryPressureThreshold;
}

void CacheConfigModel::setSystemMemoryPressureThreshold(double threshold) {
    QMutexLocker locker(&m_mutex);
    m_systemMemoryPressureThreshold = threshold;
}

GlobalCacheConfig CacheConfigModel::toGlobalCacheConfig() const {
    QMutexLocker locker(&m_mutex);
    GlobalCacheConfig config;

    config.totalMemoryLimit = m_totalMemoryLimit;
    config.searchResultCacheLimit = m_cacheLimits.value(SearchResultCache, 0);
    config.pageTextCacheLimit = m_cacheLimits.value(PageTextCache, 0);
    config.searchHighlightCacheLimit =
        m_cacheLimits.value(SearchHighlightCache, 0);
    config.pdfRenderCacheLimit = m_cacheLimits.value(PdfRenderCache, 0);
    config.thumbnailCacheLimit = m_cacheLimits.value(ThumbnailCache, 0);

    config.enableLRUEviction = m_lruEvictionEnabled;
    config.enableMemoryPressureEviction = m_memoryPressureEvictionEnabled;
    config.memoryPressureThreshold =
        static_cast<int>(m_memoryPressureThreshold * 100.0);
    config.cleanupInterval = static_cast<int>(m_cleanupInterval);

    config.enableCacheCoordination = m_cacheCoordinationEnabled;
    config.enableAdaptiveMemoryManagement = m_adaptiveMemoryManagementEnabled;
    config.enableCachePreloading = m_cachePreloadingEnabled;

    config.enableSystemMemoryMonitoring = m_systemMemoryMonitoringEnabled;
    config.enablePredictiveEviction = m_predictiveEvictionEnabled;
    config.enableMemoryCompression = m_memoryCompressionEnabled;
    config.enableEmergencyEviction = m_emergencyEvictionEnabled;

    config.memoryPressureWarningThreshold = m_memoryPressureWarningThreshold;
    config.memoryPressureCriticalThreshold = m_memoryPressureCriticalThreshold;

    config.systemMemoryCheckInterval =
        static_cast<int>(m_systemMemoryCheckInterval);
    config.systemMemoryPressureThreshold = m_systemMemoryPressureThreshold;

    return config;
}

void CacheConfigModel::fromGlobalCacheConfig(const GlobalCacheConfig& config) {
    QMutexLocker locker(&m_mutex);

    m_totalMemoryLimit = config.totalMemoryLimit;
    m_cacheLimits[SearchResultCache] = config.searchResultCacheLimit;
    m_cacheLimits[PageTextCache] = config.pageTextCacheLimit;
    m_cacheLimits[SearchHighlightCache] = config.searchHighlightCacheLimit;
    m_cacheLimits[PdfRenderCache] = config.pdfRenderCacheLimit;
    m_cacheLimits[ThumbnailCache] = config.thumbnailCacheLimit;

    m_lruEvictionEnabled = config.enableLRUEviction;
    m_memoryPressureEvictionEnabled = config.enableMemoryPressureEviction;
    m_memoryPressureThreshold =
        static_cast<double>(config.memoryPressureThreshold) / 100.0;
    m_cleanupInterval = config.cleanupInterval;

    m_cacheCoordinationEnabled = config.enableCacheCoordination;
    m_adaptiveMemoryManagementEnabled = config.enableAdaptiveMemoryManagement;
    m_cachePreloadingEnabled = config.enableCachePreloading;

    m_systemMemoryMonitoringEnabled = config.enableSystemMemoryMonitoring;
    m_predictiveEvictionEnabled = config.enablePredictiveEviction;
    m_memoryCompressionEnabled = config.enableMemoryCompression;
    m_emergencyEvictionEnabled = config.enableEmergencyEviction;

    m_memoryPressureWarningThreshold = config.memoryPressureWarningThreshold;
    m_memoryPressureCriticalThreshold = config.memoryPressureCriticalThreshold;

    m_systemMemoryCheckInterval = config.systemMemoryCheckInterval;
    m_systemMemoryPressureThreshold = config.systemMemoryPressureThreshold;
}
