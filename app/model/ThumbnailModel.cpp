#include "ThumbnailModel.h"
#include <poppler-qt6.h>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QMutexLocker>
#include "../ui/thumbnail/ThumbnailGenerator.h"

ThumbnailModel::ThumbnailModel(QObject* parent)
    : QAbstractListModel(parent),
      m_thumbnailSize(DEFAULT_THUMBNAIL_WIDTH, DEFAULT_THUMBNAIL_HEIGHT),
      m_thumbnailQuality(DEFAULT_QUALITY),
      m_maxCacheSize(DEFAULT_CACHE_SIZE),
      m_maxMemory(DEFAULT_MEMORY_LIMIT),
      m_currentMemory(0),
      m_cacheHits(0),
      m_cacheMisses(0),
      m_preloadRange(DEFAULT_PRELOAD_RANGE),
      m_visibleStart(-1),
      m_visibleEnd(-1),
      m_viewportMargin(2),
      m_lazyLoadingEnabled(true),
      m_cacheCompressionRatio(0.8),
      m_adaptiveCaching(true),
      m_lastCleanupTime(0) {
    initializeModel();
}

ThumbnailModel::~ThumbnailModel() {
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
    connect(cleanupTimer, &QTimer::timeout, this,
            &ThumbnailModel::cleanupCache);
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
            auto it = m_thumbnails.find(pageNumber);
            if (it != m_thumbnails.end()) {
                // 更新访问时间和频率
                it->lastAccessed = QDateTime::currentMSecsSinceEpoch();
                const_cast<ThumbnailModel*>(this)->updateAccessFrequency(
                    pageNumber);
                if (!it->pixmap.isNull()) {
                    const_cast<ThumbnailModel*>(this)->m_cacheHits++;
                    return it->pixmap;
                }
            }

            // 缓存未命中，请求生成缩略图
            const_cast<ThumbnailModel*>(this)->m_cacheMisses++;
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

    m_thumbnails.clear();
    m_currentMemory = 0;
    m_preloadQueue.clear();

    emit cacheUpdated();
    emit memoryUsageChanged(m_currentMemory);
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

    // 检查是否已经在加载
    auto it = m_thumbnails.find(pageNumber);
    if (it != m_thumbnails.end() && it->isLoading) {
        return;
    }

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
    emit memoryUsageChanged(m_currentMemory);
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

    auto it = m_thumbnails.find(pageNumber);
    if (it == m_thumbnails.end()) {
        return;  // 项目可能已被清理
    }

    // 更新缓存项
    it->pixmap = pixmap;
    it->isLoading = false;
    it->hasError = false;
    it->errorMessage.clear();
    it->lastAccessed = QDateTime::currentMSecsSinceEpoch();
    it->memorySize = calculatePixmapMemory(pixmap);

    m_currentMemory += it->memorySize;

    // 检查内存限制 - 使用自适应策略
    while (m_currentMemory > m_maxMemory && m_thumbnails.size() > 1) {
        evictByAdaptivePolicy();
    }

    locker.unlock();

    // 通知更新
    emit thumbnailLoaded(pageNumber);
    emit loadingStateChanged(pageNumber, false);
    emit memoryUsageChanged(m_currentMemory);

    QModelIndex idx = index(pageNumber);
    emit dataChanged(idx, idx, {PixmapRole, LoadingRole});
}

void ThumbnailModel::onThumbnailError(int pageNumber, const QString& error) {
    QMutexLocker locker(&m_thumbnailsMutex);

    auto it = m_thumbnails.find(pageNumber);
    if (it == m_thumbnails.end()) {
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

    m_currentMemory = totalMemory;
    emit memoryUsageChanged(m_currentMemory);
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

    return static_cast<double>(m_cacheHits) / totalAccess;
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
