#include "ThumbnailModel.h"
#include <poppler-qt6.h>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QMutexLocker>
#include "../logging/LoggingMacros.h"
#include "../ui/thumbnail/ThumbnailGenerator.h"

ThumbnailModel::ThumbnailModel(QObject* parent)
    : QAbstractListModel(parent),
      m_thumbnailSize(DEFAULT_THUMBNAIL_WIDTH, DEFAULT_THUMBNAIL_HEIGHT),
      m_thumbnailQuality(THUMBNAIL_DEFAULT_QUALITY),
      m_maxCacheSize(DEFAULT_CACHE_SIZE),
      m_maxMemory(DEFAULT_MEMORY_LIMIT),
      m_preloadRange(DEFAULT_PRELOAD_RANGE),
      m_visibleStart(-1),
      m_visibleEnd(-1),
      m_viewportMargin(2),
      m_lazyLoadingEnabled(true),
      m_cacheCompressionRatio(0.8),
      m_adaptiveCaching(true),
      m_lastCleanupTime(0),
      m_prefetchStrategy(PrefetchStrategy::ADAPTIVE),
      m_prefetchDistance(DEFAULT_PREFETCH_DISTANCE),
      m_compressionMode(CompressionMode::ADAPTIVE),
      m_compressionQuality(DEFAULT_COMPRESSION_QUALITY),
      m_memoryStrategy(MemoryStrategy::BALANCED),
      m_memoryPressureThreshold(DEFAULT_MEMORY_PRESSURE_THRESHOLD),
      m_intelligentPrefetchEnabled(true),
      m_memoryCompressionEnabled(true),
      m_predictiveLoadingEnabled(true),
      m_prefetchTimer(nullptr),
      m_prefetchThreadPool(nullptr),
      m_patternAnalysisTimer(nullptr) {
    // Initialize optimized cache with memory-based cost calculation
    m_optimizedCache.setMaxCost(
        static_cast<int>(m_maxMemory / 1024));  // Cost in KB

    initializeModel();
    initializeAdvancedFeatures();
}

ThumbnailModel::~ThumbnailModel() {
    cleanupAdvancedFeatures();
    if (m_preloadTimer) {
        m_preloadTimer->stop();
    }
    clearCache();
}

void ThumbnailModel::initializeModel() {
    // 创建缩略图生成器
    m_generator = std::make_unique<ThumbnailGenerator>(this);

    // 连接信号
    connect(m_generator.get(), &ThumbnailGenerator::thumbnailGenerated, this,
            &ThumbnailModel::onThumbnailGenerated);
    connect(m_generator.get(), &ThumbnailGenerator::thumbnailError, this,
            &ThumbnailModel::onThumbnailError);

    // 设置预加载定时器
    m_preloadTimer = new QTimer(this);
    m_preloadTimer->setInterval(PRELOAD_TIMER_INTERVAL);
    m_preloadTimer->setSingleShot(false);
    connect(m_preloadTimer, &QTimer::timeout, this,
            &ThumbnailModel::onPreloadTimer);

    // 设置缓存清理定时器
    QTimer* cleanupTimer = new QTimer(this);
    cleanupTimer->setInterval(30000);  // 30秒清理一次
    cleanupTimer->setSingleShot(false);
    connect(cleanupTimer, &QTimer::timeout, this, [this]() {
        cleanupCache();
        cleanupOptimizedCache();
    });
    cleanupTimer->start();

    // 设置优先级更新定时器
    m_priorityUpdateTimer = new QTimer(this);
    m_priorityUpdateTimer->setInterval(200);  // 200ms间隔
    m_priorityUpdateTimer->setSingleShot(false);
    connect(m_priorityUpdateTimer, &QTimer::timeout, this,
            &ThumbnailModel::onPriorityUpdateTimer);
}

int ThumbnailModel::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent)
    return m_document ? m_document->numPages() : 0;
}

QVariant ThumbnailModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || !m_document) {
        return QVariant();
    }

    int pageNumber = index.row();
    if (pageNumber < 0 || pageNumber >= m_document->numPages()) {
        return QVariant();
    }

    QMutexLocker locker(&m_thumbnailsMutex);

    switch (role) {
        case PageNumberRole:
            return pageNumber;

        case PixmapRole: {
            // 记录访问开始时间
            QElapsedTimer accessTimer;
            accessTimer.start();

            // Try optimized cache first
            ThumbnailItem* cachedItem =
                const_cast<ThumbnailModel*>(this)->getFromOptimizedCache(
                    pageNumber);
            if (cachedItem) {
                // 检查是否需要解压缩
                if (cachedItem->isCompressed && cachedItem->pixmap.isNull()) {
                    QPixmap decompressed =
                        const_cast<ThumbnailModel*>(this)->decompressThumbnail(
                            cachedItem->compressedData);
                    if (!decompressed.isNull()) {
                        cachedItem->pixmap = decompressed;
                        cachedItem->isCompressed = false;
                        cachedItem->compressedData.clear();
                    }
                }

                if (!cachedItem->pixmap.isNull()) {
                    // 更新访问统计
                    cachedItem->lastAccessed =
                        QDateTime::currentMSecsSinceEpoch();
                    cachedItem->accessCount++;

                    // 记录访问时间
                    qint64 accessTime = accessTimer.elapsed();
                    const_cast<ThumbnailModel*>(this)->recordAccessTime(
                        accessTime);

                    // 分析访问模式
                    const_cast<ThumbnailModel*>(this)->analyzeAccessPattern(
                        pageNumber);

                    // 检查是否是预取命中
                    if (cachedItem->wasPrefetched) {
                        const_cast<ThumbnailModel*>(this)
                            ->m_prefetchHits.fetch_add(1);
                        cachedItem->wasPrefetched = false;
                    }

                    return cachedItem->pixmap;
                }
            }

            // Fallback to legacy cache for compatibility
            auto it = m_thumbnails.find(pageNumber);
            if (it != m_thumbnails.end()) {
                // 更新访问时间和频率
                it->lastAccessed = QDateTime::currentMSecsSinceEpoch();
                const_cast<ThumbnailModel*>(this)->updateAccessFrequency(
                    pageNumber);

                if (!it->pixmap.isNull()) {
                    // Migrate to optimized cache
                    const_cast<ThumbnailModel*>(this)->insertIntoOptimizedCache(
                        pageNumber, *it);

                    // 记录访问时间和模式
                    qint64 accessTime = accessTimer.elapsed();
                    const_cast<ThumbnailModel*>(this)->recordAccessTime(
                        accessTime);
                    const_cast<ThumbnailModel*>(this)->analyzeAccessPattern(
                        pageNumber);

                    return it->pixmap;
                }
            }

            // 缓存未命中，请求生成缩略图
            const_cast<ThumbnailModel*>(this)->m_cacheMisses.fetch_add(1);
            const_cast<ThumbnailModel*>(this)->requestThumbnail(pageNumber);
            return QVariant();
        }

        case LoadingRole: {
            auto it = m_thumbnails.find(pageNumber);
            return (it != m_thumbnails.end()) ? it->isLoading : false;
        }

        case ErrorRole: {
            auto it = m_thumbnails.find(pageNumber);
            return (it != m_thumbnails.end()) ? it->hasError : false;
        }

        case ErrorMessageRole: {
            auto it = m_thumbnails.find(pageNumber);
            return (it != m_thumbnails.end()) ? it->errorMessage : QString();
        }

        case PageSizeRole: {
            auto it = m_thumbnails.find(pageNumber);
            if (it != m_thumbnails.end() && !it->pageSize.isEmpty()) {
                return it->pageSize;
            }

            // 如果没有缓存页面尺寸，尝试获取
            if (m_document) {
                std::unique_ptr<Poppler::Page> page(
                    m_document->page(pageNumber));
                if (page) {
                    QSizeF pageSize = page->pageSizeF();
                    QSize size = pageSize.toSize();

                    // 缓存页面尺寸
                    ThumbnailItem& item = const_cast<ThumbnailModel*>(this)
                                              ->m_thumbnails[pageNumber];
                    item.pageSize = size;

                    return size;
                }
            }
            return QVariant();
        }

        case CacheHitRole: {
            auto it = m_thumbnails.find(pageNumber);
            return (it != m_thumbnails.end() && !it->pixmap.isNull());
        }

        case CompressionRatioRole: {
            auto it = m_thumbnails.find(pageNumber);
            return (it != m_thumbnails.end()) ? it->compressionRatio : 1.0;
        }

        case LastAccessTimeRole: {
            auto it = m_thumbnails.find(pageNumber);
            return (it != m_thumbnails.end()) ? it->lastAccessed : 0;
        }

        default:
            return QVariant();
    }
}

Qt::ItemFlags ThumbnailModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> ThumbnailModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[PageNumberRole] = "pageNumber";
    roles[PixmapRole] = "pixmap";
    roles[LoadingRole] = "loading";
    roles[ErrorRole] = "error";
    roles[ErrorMessageRole] = "errorMessage";
    roles[PageSizeRole] = "pageSize";
    return roles;
}

void ThumbnailModel::setDocument(std::shared_ptr<Poppler::Document> document) {
    beginResetModel();

    m_document = document;
    clearCache();

    if (m_generator) {
        m_generator->setDocument(document);
    }

    endResetModel();
}

void ThumbnailModel::setThumbnailSize(const QSize& size) {
    if (m_thumbnailSize != size) {
        m_thumbnailSize = size;

        if (m_generator) {
            m_generator->setThumbnailSize(size);
        }

        // 清除现有缓存，因为尺寸改变了
        clearCache();

        // 通知视图刷新
        if (rowCount() > 0) {
            emit dataChanged(index(0), index(rowCount() - 1));
        }
    }
}

void ThumbnailModel::setThumbnailQuality(double quality) {
    if (qAbs(m_thumbnailQuality - quality) > 0.001) {
        m_thumbnailQuality = quality;

        if (m_generator) {
            m_generator->setQuality(quality);
        }

        // 清除现有缓存，因为质量改变了
        clearCache();

        // 通知视图刷新
        if (rowCount() > 0) {
            emit dataChanged(index(0), index(rowCount() - 1));
        }
    }
}

void ThumbnailModel::setCacheSize(int maxItems) {
    m_maxCacheSize = qMax(1, maxItems);

    // 如果当前缓存超过限制，清理多余项
    while (m_thumbnails.size() > m_maxCacheSize) {
        evictLeastRecentlyUsed();
    }
}

void ThumbnailModel::setMemoryLimit(qint64 maxMemory) {
    m_maxMemory = qMax(1024LL * 1024, maxMemory);  // 最少1MB

    // 如果当前内存使用超过限制，清理
    while (m_currentMemory > m_maxMemory && !m_thumbnails.isEmpty()) {
        evictLeastRecentlyUsed();
    }
}

void ThumbnailModel::clearCache() {
    QMutexLocker locker(&m_thumbnailsMutex);

    // Clear optimized cache
    m_optimizedCache.clear();
    m_accessFrequencyIndex.clear();
    m_loadingPages.clear();

    // Clear legacy cache
    m_thumbnails.clear();
    m_preloadQueue.clear();

    // Reset atomic counters
    m_currentMemory.store(0);
    m_cacheHits.store(0);
    m_cacheMisses.store(0);

    emit cacheUpdated();
    emit memoryUsageChanged(0);
}

void ThumbnailModel::setPreloadRange(int range) {
    m_preloadRange = qMax(0, range);
}

void ThumbnailModel::requestThumbnail(int pageNumber) {
    if (!m_document || pageNumber < 0 || pageNumber >= m_document->numPages()) {
        return;
    }

    // 懒加载检查
    if (m_lazyLoadingEnabled && !shouldGenerateThumbnail(pageNumber)) {
        return;
    }

    QMutexLocker locker(&m_thumbnailsMutex);

    // Check if already in optimized cache
    ThumbnailItem* cachedItem = getFromOptimizedCache(pageNumber);
    if (cachedItem && (!cachedItem->pixmap.isNull() || cachedItem->isLoading)) {
        return;  // Already cached or loading
    }

    // Check if already loading
    if (m_loadingPages.contains(pageNumber)) {
        return;  // Already being processed
    }

    // Add to loading set
    m_loadingPages.insert(pageNumber);

    // 标记为加载中
    ThumbnailItem& item = m_thumbnails[pageNumber];
    item.isLoading = true;
    item.hasError = false;
    item.errorMessage.clear();
    item.lastAccessed = QDateTime::currentMSecsSinceEpoch();

    locker.unlock();

    // 发送生成请求，使用优先级
    if (m_generator) {
        int priority = calculatePriority(pageNumber);
        m_generator->generateThumbnail(pageNumber, m_thumbnailSize,
                                       m_thumbnailQuality, priority);
    }

    // 通知加载状态变化
    emit loadingStateChanged(pageNumber, true);

    QModelIndex idx = index(pageNumber);
    emit dataChanged(idx, idx, {LoadingRole});
}

void ThumbnailModel::requestThumbnailRange(int startPage, int endPage) {
    if (!m_document)
        return;

    int numPages = m_document->numPages();
    startPage = qMax(0, startPage);
    endPage = qMin(numPages - 1, endPage);

    for (int i = startPage; i <= endPage; ++i) {
        requestThumbnail(i);
    }
}

bool ThumbnailModel::isLoading(int pageNumber) const {
    QMutexLocker locker(&m_thumbnailsMutex);
    auto it = m_thumbnails.find(pageNumber);
    return (it != m_thumbnails.end()) ? it->isLoading : false;
}

bool ThumbnailModel::hasError(int pageNumber) const {
    QMutexLocker locker(&m_thumbnailsMutex);
    auto it = m_thumbnails.find(pageNumber);
    return (it != m_thumbnails.end()) ? it->hasError : false;
}

QString ThumbnailModel::errorMessage(int pageNumber) const {
    QMutexLocker locker(&m_thumbnailsMutex);
    auto it = m_thumbnails.find(pageNumber);
    return (it != m_thumbnails.end()) ? it->errorMessage : QString();
}

void ThumbnailModel::refreshThumbnail(int pageNumber) {
    if (!m_document || pageNumber < 0 || pageNumber >= m_document->numPages()) {
        return;
    }

    QMutexLocker locker(&m_thumbnailsMutex);

    // 清除现有缓存项
    auto it = m_thumbnails.find(pageNumber);
    if (it != m_thumbnails.end()) {
        m_currentMemory -= it->memorySize;
        m_thumbnails.erase(it);
    }

    locker.unlock();

    // 重新请求
    requestThumbnail(pageNumber);

    emit cacheUpdated();
    emit memoryUsageChanged(m_currentMemory.load());
}

void ThumbnailModel::refreshAllThumbnails() {
    clearCache();

    if (rowCount() > 0) {
        emit dataChanged(index(0), index(rowCount() - 1));
    }
}

void ThumbnailModel::preloadVisibleRange(int firstVisible, int lastVisible) {
    if (!m_document)
        return;

    int numPages = m_document->numPages();

    // 扩展预加载范围
    int startPage = qMax(0, firstVisible - m_preloadRange);
    int endPage = qMin(numPages - 1, lastVisible + m_preloadRange);

    // 添加到预加载队列
    for (int i = startPage; i <= endPage; ++i) {
        if (shouldPreload(i)) {
            m_preloadQueue.insert(i);
        }
    }

    // 启动预加载定时器
    if (!m_preloadQueue.isEmpty() && !m_preloadTimer->isActive()) {
        m_preloadTimer->start();
    }
}

void ThumbnailModel::onThumbnailGenerated(int pageNumber,
                                          const QPixmap& pixmap) {
    QMutexLocker locker(&m_thumbnailsMutex);

    // Create thumbnail item
    ThumbnailItem item;
    item.pixmap = pixmap;
    item.isLoading = false;
    item.hasError = false;
    item.errorMessage.clear();
    item.lastAccessed = QDateTime::currentMSecsSinceEpoch();
    item.memorySize = calculatePixmapMemory(pixmap);
    item.accessCount = 1;

    // Insert into optimized cache first
    insertIntoOptimizedCache(pageNumber, item);

    // Update legacy cache for compatibility
    auto it = m_thumbnails.find(pageNumber);
    if (it != m_thumbnails.end()) {
        *it = item;
    } else {
        m_thumbnails.insert(pageNumber, item);
    }

    // Remove from loading set
    m_loadingPages.remove(pageNumber);

    // Check memory limits using optimized eviction
    qint64 currentMemory = m_currentMemory.load();
    if (currentMemory > m_maxMemory) {
        int itemsToEvict =
            qMax(1, static_cast<int>((currentMemory - m_maxMemory) /
                                     (item.memorySize + 1)));
        evictFromOptimizedCache(itemsToEvict);
    }

    locker.unlock();

    // 通知更新
    emit thumbnailLoaded(pageNumber);
    emit loadingStateChanged(pageNumber, false);
    emit memoryUsageChanged(m_currentMemory.load());

    QModelIndex idx = index(pageNumber);
    emit dataChanged(idx, idx, {PixmapRole, LoadingRole});
}

void ThumbnailModel::onThumbnailError(int pageNumber, const QString& error) {
    using namespace ErrorHandling;

    // Log the thumbnail generation error with structured information
    auto errorInfo = createRenderingError(
        "thumbnail generation",
        QString("Failed to generate thumbnail for page %1: %2")
            .arg(pageNumber)
            .arg(error));
    errorInfo.context =
        QString("ThumbnailModel::onThumbnailError - Page %1").arg(pageNumber);
    logError(errorInfo);

    QMutexLocker locker(&m_thumbnailsMutex);

    auto it = m_thumbnails.find(pageNumber);
    if (it == m_thumbnails.end()) {
        LOG_WARNING("ThumbnailModel: Received error for non-existent page {}",
                    pageNumber);
        return;
    }

    // 更新错误状态
    it->isLoading = false;
    it->hasError = true;
    it->errorMessage = error;
    it->lastAccessed = QDateTime::currentMSecsSinceEpoch();

    locker.unlock();

    // 通知错误
    emit thumbnailError(pageNumber, error);
    emit loadingStateChanged(pageNumber, false);

    QModelIndex idx = index(pageNumber);
    emit dataChanged(idx, idx, {LoadingRole, ErrorRole, ErrorMessageRole});
}

void ThumbnailModel::onPreloadTimer() {
    if (m_preloadQueue.isEmpty()) {
        m_preloadTimer->stop();
        return;
    }

    // 每次处理一个预加载请求
    int pageNumber = *m_preloadQueue.begin();
    m_preloadQueue.remove(pageNumber);

    requestThumbnail(pageNumber);

    // 如果队列为空，停止定时器
    if (m_preloadQueue.isEmpty()) {
        m_preloadTimer->stop();
    }
}

void ThumbnailModel::cleanupCache() {
    QMutexLocker locker(&m_thumbnailsMutex);

    if (m_thumbnails.isEmpty()) {
        return;
    }

    // 自适应缓存大小调整
    adaptCacheSize();

    // 清理超过缓存大小限制的项
    while (m_thumbnails.size() > m_maxCacheSize) {
        evictByAdaptivePolicy();
    }

    // 清理超过内存限制的项
    while (m_currentMemory > m_maxMemory && !m_thumbnails.isEmpty()) {
        evictByAdaptivePolicy();
    }

    emit cacheUpdated();
}

void ThumbnailModel::evictLeastRecentlyUsed() {
    if (m_thumbnails.isEmpty()) {
        return;
    }

    // 找到最久未访问的项
    auto oldestIt = m_thumbnails.begin();
    qint64 oldestTime = oldestIt->lastAccessed;

    for (auto it = m_thumbnails.begin(); it != m_thumbnails.end(); ++it) {
        if (it->lastAccessed < oldestTime) {
            oldestTime = it->lastAccessed;
            oldestIt = it;
        }
    }

    // 更新内存使用
    m_currentMemory -= oldestIt->memorySize;

    // 移除项
    m_thumbnails.erase(oldestIt);
}

qint64 ThumbnailModel::calculatePixmapMemory(const QPixmap& pixmap) const {
    if (pixmap.isNull()) {
        return 0;
    }

    // 估算内存使用：宽度 × 高度 × 4字节(ARGB32)
    return static_cast<qint64>(pixmap.width()) * pixmap.height() * 4;
}

void ThumbnailModel::updateMemoryUsage() {
    QMutexLocker locker(&m_thumbnailsMutex);

    qint64 totalMemory = 0;
    for (const auto& item : m_thumbnails) {
        totalMemory += item.memorySize;
    }

    m_currentMemory.store(totalMemory);
    emit memoryUsageChanged(totalMemory);
}

bool ThumbnailModel::shouldPreload(int pageNumber) const {
    if (!m_document || pageNumber < 0 || pageNumber >= m_document->numPages()) {
        return false;
    }

    QMutexLocker locker(&m_thumbnailsMutex);

    auto it = m_thumbnails.find(pageNumber);
    if (it != m_thumbnails.end()) {
        // 如果已有缓存或正在加载，不需要预加载
        return it->pixmap.isNull() && !it->isLoading && !it->hasError;
    }

    return true;  // 没有缓存项，需要预加载
}

// 懒加载和视口管理实现
void ThumbnailModel::setLazyLoadingEnabled(bool enabled) {
    m_lazyLoadingEnabled = enabled;
    if (enabled && m_priorityUpdateTimer) {
        m_priorityUpdateTimer->start();
    } else if (m_priorityUpdateTimer) {
        m_priorityUpdateTimer->stop();
    }
}

void ThumbnailModel::setViewportRange(int start, int end, int margin) {
    m_visibleStart = start;
    m_visibleEnd = end;
    m_viewportMargin = margin;

    if (m_lazyLoadingEnabled) {
        updateViewportPriorities();
    }
}

void ThumbnailModel::updateViewportPriorities() {
    if (!m_document)
        return;

    m_pagePriorities.clear();

    // 可见区域最高优先级
    for (int i = m_visibleStart; i <= m_visibleEnd; ++i) {
        if (i >= 0 && i < m_document->numPages()) {
            m_pagePriorities[i] = 0;  // 最高优先级
        }
    }

    // 预加载区域次高优先级
    int preloadStart = qMax(0, m_visibleStart - m_viewportMargin);
    int preloadEnd =
        qMin(m_document->numPages() - 1, m_visibleEnd + m_viewportMargin);

    for (int i = preloadStart; i < m_visibleStart; ++i) {
        m_pagePriorities[i] = 1;
    }
    for (int i = m_visibleEnd + 1; i <= preloadEnd; ++i) {
        m_pagePriorities[i] = 1;
    }
}

bool ThumbnailModel::shouldGenerateThumbnail(int pageNumber) const {
    if (!m_lazyLoadingEnabled) {
        return true;
    }

    return isInViewport(pageNumber);
}

int ThumbnailModel::calculatePriority(int pageNumber) const {
    auto it = m_pagePriorities.find(pageNumber);
    if (it != m_pagePriorities.end()) {
        return it.value();
    }

    // 默认低优先级
    return 5;
}

bool ThumbnailModel::isInViewport(int pageNumber) const {
    if (m_visibleStart < 0 || m_visibleEnd < 0) {
        return true;  // 如果没有设置视口，生成所有缩略图
    }

    int expandedStart = qMax(0, m_visibleStart - m_viewportMargin);
    int expandedEnd = m_visibleEnd + m_viewportMargin;

    return pageNumber >= expandedStart && pageNumber <= expandedEnd;
}

void ThumbnailModel::onPriorityUpdateTimer() {
    if (m_lazyLoadingEnabled) {
        updateViewportPriorities();
    }
}

// 高级缓存管理实现
void ThumbnailModel::updateAccessFrequency(int pageNumber) {
    m_accessFrequency[pageNumber]++;

    // 限制频率表大小
    if (m_accessFrequency.size() > m_maxCacheSize * 2) {
        // 清理低频访问的条目
        auto it = m_accessFrequency.begin();
        while (it != m_accessFrequency.end()) {
            if (it.value() <= 1) {
                it = m_accessFrequency.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void ThumbnailModel::evictLeastFrequentlyUsed() {
    if (m_thumbnails.isEmpty())
        return;

    QMutexLocker locker(&m_thumbnailsMutex);

    int leastFrequentPage = -1;
    int minFrequency = INT_MAX;
    qint64 oldestTime = LLONG_MAX;

    for (auto it = m_thumbnails.begin(); it != m_thumbnails.end(); ++it) {
        int pageNumber = it.key();
        int frequency = m_accessFrequency.value(pageNumber, 0);

        if (frequency < minFrequency ||
            (frequency == minFrequency && it->lastAccessed < oldestTime)) {
            minFrequency = frequency;
            oldestTime = it->lastAccessed;
            leastFrequentPage = pageNumber;
        }
    }

    if (leastFrequentPage >= 0) {
        auto it = m_thumbnails.find(leastFrequentPage);
        if (it != m_thumbnails.end()) {
            m_currentMemory -= it->memorySize;
            m_thumbnails.erase(it);
            m_accessFrequency.remove(leastFrequentPage);
        }
    }
}

void ThumbnailModel::evictByAdaptivePolicy() {
    if (!m_adaptiveCaching) {
        evictLeastRecentlyUsed();
        return;
    }

    double efficiency = calculateCacheEfficiency();

    // 根据缓存效率选择驱逐策略
    if (efficiency > 0.7) {
        // 高效率：使用LRU
        evictLeastRecentlyUsed();
    } else {
        // 低效率：使用LFU
        evictLeastFrequentlyUsed();
    }
}

double ThumbnailModel::calculateCacheEfficiency() const {
    int totalAccess = m_cacheHits + m_cacheMisses;
    if (totalAccess == 0)
        return 1.0;

    return static_cast<double>(m_cacheHits.load()) / totalAccess;
}

// Optimized cache implementation methods
void ThumbnailModel::insertIntoOptimizedCache(int pageNumber,
                                              const ThumbnailItem& item) {
    // Calculate memory cost in KB for QCache
    int memoryCostKB = static_cast<int>(item.memorySize / 1024);
    if (memoryCostKB == 0)
        memoryCostKB = 1;  // Minimum cost

    // Create cache entry
    auto* cacheEntry = new CacheEntry(item, pageNumber);

    // Insert into optimized cache (QCache handles LRU automatically)
    if (m_optimizedCache.insert(pageNumber, cacheEntry, memoryCostKB)) {
        // Update atomic memory counter
        m_currentMemory.fetch_add(item.memorySize);

        // Update access frequency index for LFU support
        updateAccessFrequencyOptimized(pageNumber);

        LOG_DEBUG(
            "ThumbnailModel: Inserted page {} into optimized cache ({}KB)",
            pageNumber, memoryCostKB);
    } else {
        delete cacheEntry;  // QCache rejected the entry
        LOG_WARNING("ThumbnailModel: Failed to insert page {} into cache",
                    pageNumber);
    }
}

ThumbnailModel::ThumbnailItem* ThumbnailModel::getFromOptimizedCache(
    int pageNumber) {
    CacheEntry* entry = m_optimizedCache.object(pageNumber);
    if (entry) {
        // Update access statistics
        m_cacheHits.fetch_add(1);
        entry->item.lastAccessed = QDateTime::currentMSecsSinceEpoch();
        entry->item.accessCount++;

        updateAccessFrequencyOptimized(pageNumber);

        return &(entry->item);
    }

    m_cacheMisses.fetch_add(1);
    return nullptr;
}

void ThumbnailModel::evictFromOptimizedCache(int count) {
    // QCache handles LRU eviction automatically when maxCost is exceeded
    // This method provides manual eviction for memory pressure situations

    if (m_optimizedCache.isEmpty()) {
        return;
    }

    // Get current cache keys for manual eviction if needed
    QList<int> keys = m_optimizedCache.keys();

    // Sort by access frequency for LFU eviction when needed
    if (m_adaptiveCaching && calculateCacheEfficiency() < 0.7) {
        std::sort(keys.begin(), keys.end(), [this](int a, int b) {
            CacheEntry* entryA = m_optimizedCache.object(a);
            CacheEntry* entryB = m_optimizedCache.object(b);
            if (!entryA || !entryB)
                return false;

            // Sort by access count (LFU), then by last accessed time
            if (entryA->item.accessCount != entryB->item.accessCount) {
                return entryA->item.accessCount < entryB->item.accessCount;
            }
            return entryA->item.lastAccessed < entryB->item.lastAccessed;
        });
    }

    // Remove the specified number of items
    for (int i = 0; i < count && i < keys.size(); ++i) {
        CacheEntry* entry = m_optimizedCache.object(keys[i]);
        if (entry) {
            m_currentMemory.fetch_sub(entry->item.memorySize);
        }
        m_optimizedCache.remove(keys[i]);
        m_accessFrequencyIndex.remove(keys[i]);
    }

    LOG_DEBUG("ThumbnailModel: Evicted {} items from optimized cache", count);
}

void ThumbnailModel::updateAccessFrequencyOptimized(int pageNumber) {
    // Update frequency index for efficient LFU operations
    CacheEntry* entry = m_optimizedCache.object(pageNumber);
    if (entry) {
        // Remove old frequency mapping
        auto range =
            m_accessFrequencyIndex.equal_range(entry->item.accessCount - 1);
        for (auto it = range.first; it != range.second; ++it) {
            if (it.value() == pageNumber) {
                m_accessFrequencyIndex.erase(it);
                break;
            }
        }

        // Add new frequency mapping
        m_accessFrequencyIndex.insert(entry->item.accessCount, pageNumber);
    }
}

void ThumbnailModel::cleanupOptimizedCache() {
    // Periodic cleanup to maintain cache efficiency
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    // Only cleanup if enough time has passed
    if (currentTime - m_lastCleanupTime < 30000) {  // 30 seconds
        return;
    }

    m_lastCleanupTime = currentTime;

    // Remove expired entries (older than 5 minutes and low access count)
    QList<int> keysToRemove;
    QList<int> keys = m_optimizedCache.keys();

    for (int key : keys) {
        CacheEntry* entry = m_optimizedCache.object(key);
        if (entry &&
            (currentTime - entry->item.lastAccessed > 300000) &&  // 5 minutes
            entry->item.accessCount < 2) {
            keysToRemove.append(key);
        }
    }

    for (int key : keysToRemove) {
        CacheEntry* entry = m_optimizedCache.object(key);
        if (entry) {
            m_currentMemory.fetch_sub(entry->item.memorySize);
        }
        m_optimizedCache.remove(key);
        m_accessFrequencyIndex.remove(key);
    }

    if (!keysToRemove.isEmpty()) {
        LOG_DEBUG("ThumbnailModel: Cleaned up {} expired cache entries",
                  keysToRemove.size());
    }
}

void ThumbnailModel::adaptCacheSize() {
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    // 每30秒调整一次
    if (currentTime - m_lastCleanupTime < 30000) {
        return;
    }

    m_lastCleanupTime = currentTime;

    double efficiency = calculateCacheEfficiency();

    // 根据效率调整缓存大小
    if (efficiency > 0.8 && m_currentMemory < m_maxMemory * 0.8) {
        // 效率高且内存充足，可以增加缓存
        m_maxCacheSize = qMin(m_maxCacheSize + 10, 300);
    } else if (efficiency < 0.5) {
        // 效率低，减少缓存大小
        m_maxCacheSize = qMax(m_maxCacheSize - 5, 50);
    }
}

// ============================================================================
// 新增的高级功能实现
// ============================================================================

void ThumbnailModel::initializeAdvancedFeatures() {
    // 初始化预取线程池
    m_prefetchThreadPool = new QThreadPool(this);
    m_prefetchThreadPool->setMaxThreadCount(2);  // 限制预取线程数

    // 初始化预取定时器
    m_prefetchTimer = new QTimer(this);
    m_prefetchTimer->setInterval(200);  // 200ms间隔
    connect(m_prefetchTimer, &QTimer::timeout, this,
            &ThumbnailModel::processPrefetchQueue);

    // 初始化访问模式分析定时器
    m_patternAnalysisTimer = new QTimer(this);
    m_patternAnalysisTimer->setInterval(PATTERN_ANALYSIS_INTERVAL);
    connect(m_patternAnalysisTimer, &QTimer::timeout, this,
            &ThumbnailModel::updateAccessPattern);
    m_patternAnalysisTimer->start();

    // 启动性能计时器
    m_performanceTimer.start();
}

void ThumbnailModel::cleanupAdvancedFeatures() {
    if (m_prefetchTimer) {
        m_prefetchTimer->stop();
    }

    if (m_patternAnalysisTimer) {
        m_patternAnalysisTimer->stop();
    }

    if (m_prefetchThreadPool) {
        m_prefetchThreadPool->waitForDone(3000);  // 等待3秒
    }

    m_prefetchQueue.clear();
    m_prefetchedPages.clear();
    m_compressedCache.clear();
}

void ThumbnailModel::setPrefetchStrategy(PrefetchStrategy strategy) {
    if (m_prefetchStrategy != strategy) {
        m_prefetchStrategy = strategy;

        // 根据策略调整预取行为
        if (strategy == PrefetchStrategy::NONE) {
            stopIntelligentPrefetch();
        } else if (m_intelligentPrefetchEnabled) {
            startIntelligentPrefetch();
        }
    }
}

void ThumbnailModel::setPrefetchDistance(int distance) {
    m_prefetchDistance = qBound(1, distance, 10);
}

void ThumbnailModel::setCompressionMode(CompressionMode mode) {
    m_compressionMode = mode;
}

void ThumbnailModel::setCompressionQuality(int quality) {
    m_compressionQuality = qBound(1, quality, 100);
}

void ThumbnailModel::setMemoryStrategy(MemoryStrategy strategy) {
    if (m_memoryStrategy != strategy) {
        m_memoryStrategy = strategy;

        // 根据策略调整内存管理
        switch (strategy) {
            case MemoryStrategy::CONSERVATIVE:
                m_memoryPressureThreshold = 0.6;
                break;
            case MemoryStrategy::BALANCED:
                m_memoryPressureThreshold = 0.8;
                break;
            case MemoryStrategy::AGGRESSIVE:
                m_memoryPressureThreshold = 0.95;
                break;
            case MemoryStrategy::ADAPTIVE:
                // 动态调整
                break;
        }
    }
}

void ThumbnailModel::setMemoryPressureThreshold(double threshold) {
    m_memoryPressureThreshold = qBound(0.1, threshold, 0.99);
}

void ThumbnailModel::enableIntelligentPrefetch(bool enabled) {
    if (m_intelligentPrefetchEnabled != enabled) {
        m_intelligentPrefetchEnabled = enabled;

        if (enabled && m_prefetchStrategy != PrefetchStrategy::NONE) {
            startIntelligentPrefetch();
        } else {
            stopIntelligentPrefetch();
        }
    }
}

void ThumbnailModel::enableMemoryCompression(bool enabled) {
    m_memoryCompressionEnabled = enabled;

    if (enabled) {
        // 压缩现有的缓存项
        compressOldEntries();
    }
}

void ThumbnailModel::enablePredictiveLoading(bool enabled) {
    m_predictiveLoadingEnabled = enabled;
}

double ThumbnailModel::compressionRatio() const {
    qint64 original = m_originalSize.load();
    qint64 compressed = m_compressedSize.load();

    if (original == 0)
        return 1.0;
    return static_cast<double>(compressed) / original;
}

double ThumbnailModel::averageAccessTime() const {
    QMutexLocker locker(&m_performanceMutex);

    if (m_accessTimes.isEmpty())
        return 0.0;

    qint64 total = 0;
    for (qint64 time : m_accessTimes) {
        total += time;
    }

    return static_cast<double>(total) / m_accessTimes.size();
}

int ThumbnailModel::prefetchHitRate() const {
    int hits = m_prefetchHits.load();
    int misses = m_prefetchMisses.load();
    int total = hits + misses;

    if (total == 0)
        return 0;
    return (hits * 100) / total;
}

void ThumbnailModel::startIntelligentPrefetch() {
    if (m_prefetchTimer && !m_prefetchTimer->isActive()) {
        m_prefetchTimer->start();
    }
}

void ThumbnailModel::stopIntelligentPrefetch() {
    if (m_prefetchTimer) {
        m_prefetchTimer->stop();
    }
    m_prefetchQueue.clear();
}

void ThumbnailModel::processPrefetchQueue() {
    if (m_prefetchQueue.isEmpty() || !m_document) {
        return;
    }

    // 检查内存压力
    if (isMemoryPressureHigh()) {
        return;
    }

    // 处理队列中的预取请求
    int processed = 0;
    const int maxProcessPerCycle = 2;  // 每次最多处理2个

    while (!m_prefetchQueue.isEmpty() && processed < maxProcessPerCycle) {
        PrefetchEntry entry = m_prefetchQueue.dequeue();

        // 检查是否已经加载或正在加载
        if (!isLoading(entry.pageNumber) &&
            !m_loadingPages.contains(entry.pageNumber)) {
            QMutexLocker locker(&m_thumbnailsMutex);

            ThumbnailItem* item = getFromOptimizedCache(entry.pageNumber);
            if (!item || item->pixmap.isNull()) {
                // 需要预取
                locker.unlock();
                requestThumbnail(entry.pageNumber);
                m_prefetchedPages.insert(entry.pageNumber);
            }
        }

        processed++;
    }
}

void ThumbnailModel::addToPrefetchQueue(int pageNumber,
                                        PrefetchStrategy strategy,
                                        int priority) {
    if (!m_document || pageNumber < 0 || pageNumber >= m_document->numPages()) {
        return;
    }

    // 避免重复添加
    for (const PrefetchEntry& entry : m_prefetchQueue) {
        if (entry.pageNumber == pageNumber) {
            return;
        }
    }

    m_prefetchQueue.enqueue(PrefetchEntry(pageNumber, priority, strategy));
}

void ThumbnailModel::predictNextPages(const QList<int>& recentAccesses,
                                      QList<int>& predictions) {
    predictions.clear();

    if (recentAccesses.size() < 2 || !m_document) {
        return;
    }

    // 简单的预测算法：基于访问模式
    int lastPage = recentAccesses.last();
    int secondLastPage = recentAccesses.size() > 1
                             ? recentAccesses[recentAccesses.size() - 2]
                             : -1;

    // 检测顺序访问模式
    if (secondLastPage >= 0) {
        int direction = lastPage - secondLastPage;

        // 预测接下来的页面
        for (int i = 1; i <= m_prefetchDistance; ++i) {
            int predictedPage = lastPage + (direction * i);
            if (predictedPage >= 0 && predictedPage < m_document->numPages()) {
                predictions.append(predictedPage);
            }
        }
    }

    // 添加相邻页面
    for (int offset = 1; offset <= m_prefetchDistance; ++offset) {
        if (lastPage - offset >= 0) {
            predictions.append(lastPage - offset);
        }
        if (lastPage + offset < m_document->numPages()) {
            predictions.append(lastPage + offset);
        }
    }

    // 去重
    QSet<int> uniqueSet(predictions.begin(), predictions.end());
    predictions = QList<int>(uniqueSet.begin(), uniqueSet.end());
}

QByteArray ThumbnailModel::compressThumbnail(const QPixmap& pixmap) {
    if (!m_memoryCompressionEnabled || pixmap.isNull()) {
        return QByteArray();
    }

    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);

    // 根据压缩模式选择格式和质量
    const char* format = "JPEG";
    int quality = m_compressionQuality;

    switch (m_compressionMode) {
        case CompressionMode::LOSSLESS:
            format = "PNG";
            quality = 100;
            break;
        case CompressionMode::LOSSY:
            format = "JPEG";
            break;
        case CompressionMode::ADAPTIVE:
            // 根据图像大小自适应选择
            if (pixmap.width() * pixmap.height() > 50000) {
                format = "JPEG";
                quality = 80;
            } else {
                format = "PNG";
                quality = 100;
            }
            break;
        case CompressionMode::NONE:
        default:
            return QByteArray();
    }

    if (pixmap.save(&buffer, format, quality)) {
        return data;
    }

    return QByteArray();
}

QPixmap ThumbnailModel::decompressThumbnail(const QByteArray& data) {
    if (data.isEmpty()) {
        return QPixmap();
    }

    QPixmap pixmap;
    if (pixmap.loadFromData(data)) {
        return pixmap;
    }

    return QPixmap();
}

void ThumbnailModel::updateCompressionStats(qint64 originalSize,
                                            qint64 compressedSize) {
    m_originalSize.fetch_add(originalSize);
    m_compressedSize.fetch_add(compressedSize);
}

void ThumbnailModel::analyzeAccessPattern(int pageNumber) {
    // 记录访问
    m_accessPattern.recentAccesses.append(pageNumber);

    // 限制历史记录大小
    if (m_accessPattern.recentAccesses.size() > MAX_ACCESS_HISTORY) {
        m_accessPattern.recentAccesses.removeFirst();
    }

    // 更新访问频率
    m_accessPattern.accessFrequency[pageNumber]++;

    // 分析访问模式
    if (m_accessPattern.recentAccesses.size() >= 2) {
        int lastPage =
            m_accessPattern
                .recentAccesses[m_accessPattern.recentAccesses.size() - 2];
        int currentPage = pageNumber;

        if (qAbs(currentPage - lastPage) == 1) {
            m_accessPattern.sequentialCount++;
        } else {
            m_accessPattern.randomCount++;
        }
    }

    // 触发预测性预取
    if (m_predictiveLoadingEnabled &&
        m_prefetchStrategy == PrefetchStrategy::PREDICTIVE) {
        QList<int> predictions;
        predictNextPages(m_accessPattern.recentAccesses, predictions);

        for (int predictedPage : predictions) {
            addToPrefetchQueue(predictedPage, PrefetchStrategy::PREDICTIVE, 1);
        }
    }
}

void ThumbnailModel::updateAccessPattern() {
    // 计算平均访问间隔
    if (m_accessPattern.recentAccesses.size() >= 2) {
        qint64 totalInterval = 0;
        qint64 currentTime = m_accessPattern.sessionTimer.elapsed();

        // 简化计算：使用最近几次访问的平均间隔
        int recentCount = qMin(10, m_accessPattern.recentAccesses.size());
        if (recentCount > 1) {
            totalInterval = currentTime / recentCount;
            m_accessPattern.averageInterval = totalInterval;
        }
    }

    // 根据访问模式调整策略
    if (m_prefetchStrategy == PrefetchStrategy::ADAPTIVE) {
        PrefetchStrategy bestStrategy = determineBestStrategy();
        if (bestStrategy != m_prefetchStrategy) {
            setPrefetchStrategy(bestStrategy);
        }
    }
}

ThumbnailModel::PrefetchStrategy ThumbnailModel::determineBestStrategy() const {
    // 根据访问模式统计决定最佳策略
    int totalAccesses =
        m_accessPattern.sequentialCount + m_accessPattern.randomCount;

    if (totalAccesses < 10) {
        return PrefetchStrategy::LINEAR;  // 默认线性策略
    }

    double sequentialRatio =
        static_cast<double>(m_accessPattern.sequentialCount) / totalAccesses;

    if (sequentialRatio > 0.7) {
        return PrefetchStrategy::LINEAR;  // 顺序访问较多
    } else if (sequentialRatio > 0.3) {
        return PrefetchStrategy::ADAPTIVE;  // 混合模式
    } else {
        return PrefetchStrategy::PREDICTIVE;  // 随机访问较多，使用预测
    }
}

void ThumbnailModel::optimizeMemoryUsage() {
    if (isMemoryPressureHigh()) {
        handleMemoryPressure();
    }

    // 定期压缩旧条目
    if (m_memoryCompressionEnabled) {
        compressOldEntries();
    }
}

bool ThumbnailModel::isMemoryPressureHigh() const {
    double usage = static_cast<double>(m_currentMemory.load()) / m_maxMemory;
    return usage > m_memoryPressureThreshold;
}

void ThumbnailModel::handleMemoryPressure() {
    // 激进的内存清理
    int itemsToEvict = m_maxCacheSize / 4;  // 清理25%的缓存

    for (int i = 0; i < itemsToEvict; ++i) {
        evictFromOptimizedCache(1);
    }

    // 压缩剩余项目
    if (m_memoryCompressionEnabled) {
        compressOldEntries();
    }
}

void ThumbnailModel::compressOldEntries() {
    QMutexLocker locker(&m_thumbnailsMutex);

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    const qint64 compressionAge = 60000;  // 1分钟

    for (auto it = m_thumbnails.begin(); it != m_thumbnails.end(); ++it) {
        ThumbnailItem& item = it.value();

        // 压缩超过1分钟未访问的项目
        if (!item.isCompressed && !item.pixmap.isNull() &&
            (currentTime - item.lastAccessed) > compressionAge) {
            QByteArray compressed = compressThumbnail(item.pixmap);
            if (!compressed.isEmpty()) {
                qint64 originalSize = item.memorySize;

                item.compressedData = compressed;
                item.isCompressed = true;
                item.pixmap = QPixmap();  // 释放原始像素图
                item.memorySize = compressed.size();
                item.compressionRatio =
                    static_cast<double>(compressed.size()) / originalSize;

                updateCompressionStats(originalSize, compressed.size());

                // 更新内存使用统计
                m_currentMemory.fetch_sub(originalSize - compressed.size());
            }
        }
    }
}

void ThumbnailModel::recordAccessTime(qint64 time) {
    QMutexLocker locker(&m_performanceMutex);

    m_accessTimes.append(time);

    // 限制历史记录大小
    if (m_accessTimes.size() > 100) {
        m_accessTimes.removeFirst();
    }
}

void ThumbnailModel::updatePerformanceMetrics() {
    // 这个方法可以用于定期更新性能指标
    // 目前主要通过其他方法实时更新
}
