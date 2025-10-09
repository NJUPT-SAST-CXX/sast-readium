#include "PDFCacheManager.h"
#include <QApplication>
#include <QDateTime>
#include <QMutexLocker>
#include <QPixmap>
// #include <QtConcurrent> // Not available in this MSYS2 setup
#include "utils/LoggingMacros.h"

// CacheItem Implementation
qint64 CacheItem::calculateSize() const {
    qint64 size = sizeof(CacheItem);

    switch (type) {
        case CacheItemType::RenderedPage:
        case CacheItemType::Thumbnail:
        case CacheItemType::PageImage: {
            if (data.canConvert<QPixmap>()) {
                QPixmap pixmap = data.value<QPixmap>();
                size += pixmap.width() * pixmap.height() * 4;  // 32-bit ARGB
            }
            break;
        }
        case CacheItemType::TextContent: {
            if (data.canConvert<QString>()) {
                size += data.toString().size() * sizeof(QChar);
            }
            break;
        }
        case CacheItemType::SearchResults:
        case CacheItemType::Annotations: {
            // Estimate based on variant size
            size += 1024;  // Conservative estimate
            break;
        }
    }

    return size;
}

bool CacheItem::isExpired(qint64 maxAge) const {
    if (maxAge <= 0)
        return false;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    return (currentTime - timestamp) > maxAge;
}

// PreloadTask Implementation
PreloadTask::PreloadTask(Poppler::Document* document, int pageNumber,
                         CacheItemType type, QObject* target)
    : m_document(document),
      m_pageNumber(pageNumber),
      m_type(type),
      m_target(target) {
    setAutoDelete(true);
}

void PreloadTask::run() {
    if (!m_document || m_pageNumber < 0) {
        return;
    }

    try {
        std::unique_ptr<Poppler::Page> page(m_document->page(m_pageNumber));
        if (!page) {
            return;
        }

        QVariant result;
        switch (m_type) {
            case CacheItemType::RenderedPage: {
                QImage image = page->renderToImage(150.0, 150.0);
                result = QPixmap::fromImage(image);
                break;
            }
            case CacheItemType::Thumbnail: {
                QImage image = page->renderToImage(72.0, 72.0);
                result = QPixmap::fromImage(image).scaled(
                    128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                break;
            }
            case CacheItemType::TextContent: {
                result = page->text(QRectF());
                break;
            }
            default:
                return;
        }

        // Signal completion back to cache manager
        QMetaObject::invokeMethod(m_target, "onPreloadTaskCompleted",
                                  Qt::QueuedConnection);

    } catch (...) {
        LOG_WARNING("PreloadTask: Exception during preload of page {}",
                    m_pageNumber);
    }
}

// PDFCacheManager Implementation
PDFCacheManager::PDFCacheManager(QObject* parent)
    : QObject(parent),
      m_maxMemoryUsage(256 * 1024 * 1024)  // 256MB default
      ,
      m_maxItems(1000),
      m_itemMaxAge(30 * 60 * 1000)  // 30 minutes
      ,
      m_evictionPolicy("LRU"),
      m_lowPriorityWeight(0.1),
      m_normalPriorityWeight(1.0),
      m_highPriorityWeight(10.0),
      m_hitCount(0),
      m_missCount(0),
      m_totalAccessTime(0),
      m_accessCount(0),
      m_preloadingEnabled(true),
      m_preloadingStrategy("adaptive"),
      m_preloadThreadPool(new QThreadPool(this)),
      m_maintenanceTimer(new QTimer(this)),
      m_settings(new QSettings("SAST", "Readium-Cache", this)) {
    // Configure thread pool
    m_preloadThreadPool->setMaxThreadCount(2);

    // Setup maintenance timer
    m_maintenanceTimer->setInterval(60000);  // 1 minute
    connect(m_maintenanceTimer, &QTimer::timeout, this,
            &PDFCacheManager::performMaintenance);
    m_maintenanceTimer->start();

    // Load settings
    loadSettings();

    LOG_DEBUG(
        "PDFCacheManager initialized with max memory: {} bytes, max items: {}",
        m_maxMemoryUsage, m_maxItems);
}

bool PDFCacheManager::insert(const QString& key, const QVariant& data,
                             CacheItemType type, CachePriority priority,
                             int pageNumber) {
    QMutexLocker locker(&m_cacheMutex);

    CacheItem item;
    item.data = data;
    item.type = type;
    item.priority = priority;
    item.pageNumber = pageNumber;
    item.key = key;
    item.memorySize = item.calculateSize();

    // Check if we need to make room
    while (m_cache.size() >= m_maxItems ||
           (getCurrentMemoryUsage() + item.memorySize) > m_maxMemoryUsage) {
        if (!evictLeastUsedItems(1)) {
            LOG_WARNING("PDFCacheManager: Failed to evict items, cache full");
            return false;
        }
    }

    m_cache[key] = item;

    LOG_DEBUG("PDFCacheManager: Cached item {} type: {} size: {} bytes",
              key.toStdString(), static_cast<int>(type), item.memorySize);

    return true;
}

QVariant PDFCacheManager::get(const QString& key) {
    QMutexLocker locker(&m_cacheMutex);

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        it->updateAccess();
        updateStatistics(true);
        emit cacheHit(key, 0);  // TODO: measure actual access time
        return it->data;
    }

    updateStatistics(false);
    emit cacheMiss(key);
    return QVariant();
}

bool PDFCacheManager::contains(const QString& key) const {
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.contains(key);
}

bool PDFCacheManager::remove(const QString& key) {
    QMutexLocker locker(&m_cacheMutex);

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        emit itemEvicted(key, it->type);
        m_cache.erase(it);
        return true;
    }
    return false;
}

void PDFCacheManager::clear() {
    QMutexLocker locker(&m_cacheMutex);
    m_cache.clear();
    LOG_DEBUG("PDFCacheManager: Cache cleared");
}

bool PDFCacheManager::cacheRenderedPage(int pageNumber, const QPixmap& pixmap,
                                        double scaleFactor) {
    QString key =
        generateKey(pageNumber, CacheItemType::RenderedPage, scaleFactor);
    return insert(key, pixmap, CacheItemType::RenderedPage,
                  CachePriority::Normal, pageNumber);
}

QPixmap PDFCacheManager::getRenderedPage(int pageNumber, double scaleFactor) {
    QString key =
        generateKey(pageNumber, CacheItemType::RenderedPage, scaleFactor);
    QVariant result = get(key);
    return result.canConvert<QPixmap>() ? result.value<QPixmap>() : QPixmap();
}

bool PDFCacheManager::cacheThumbnail(int pageNumber, const QPixmap& thumbnail) {
    QString key = generateKey(pageNumber, CacheItemType::Thumbnail);
    return insert(key, thumbnail, CacheItemType::Thumbnail, CachePriority::High,
                  pageNumber);
}

QPixmap PDFCacheManager::getThumbnail(int pageNumber) {
    QString key = generateKey(pageNumber, CacheItemType::Thumbnail);
    QVariant result = get(key);
    return result.canConvert<QPixmap>() ? result.value<QPixmap>() : QPixmap();
}

bool PDFCacheManager::cacheTextContent(int pageNumber, const QString& text) {
    QString key = generateKey(pageNumber, CacheItemType::TextContent);
    return insert(key, text, CacheItemType::TextContent, CachePriority::Normal,
                  pageNumber);
}

QString PDFCacheManager::getTextContent(int pageNumber) {
    QString key = generateKey(pageNumber, CacheItemType::TextContent);
    QVariant result = get(key);
    return result.canConvert<QString>() ? result.toString() : QString();
}

void PDFCacheManager::enablePreloading(bool enabled) {
    m_preloadingEnabled = enabled;
    LOG_DEBUG("PDFCacheManager: Preloading {}",
              enabled ? "enabled" : "disabled");
}

void PDFCacheManager::preloadPages(const QList<int>& pageNumbers,
                                   CacheItemType type) {
    if (!m_preloadingEnabled)
        return;

    for (int pageNumber : pageNumbers) {
        schedulePreload(pageNumber, type);
    }
}

void PDFCacheManager::preloadAroundPage(int centerPage, int radius) {
    if (!m_preloadingEnabled)
        return;

    QList<int> pagesToPreload;
    for (int i = centerPage - radius; i <= centerPage + radius; ++i) {
        if (i >= 0) {  // Assume page numbers start from 0
            pagesToPreload.append(i);
        }
    }

    preloadPages(pagesToPreload, CacheItemType::RenderedPage);
    preloadPages(pagesToPreload, CacheItemType::Thumbnail);
}

void PDFCacheManager::setPreloadingStrategy(const QString& strategy) {
    m_preloadingStrategy = strategy;
    LOG_DEBUG("PDFCacheManager: Preloading strategy set to {}",
              strategy.toStdString());
}

QString PDFCacheManager::generateKey(int pageNumber, CacheItemType type,
                                     const QVariant& extra) const {
    QString typeStr;
    switch (type) {
        case CacheItemType::RenderedPage:
            typeStr = "page";
            break;
        case CacheItemType::Thumbnail:
            typeStr = "thumb";
            break;
        case CacheItemType::TextContent:
            typeStr = "text";
            break;
        case CacheItemType::PageImage:
            typeStr = "image";
            break;
        case CacheItemType::SearchResults:
            typeStr = "search";
            break;
        case CacheItemType::Annotations:
            typeStr = "annot";
            break;
    }

    QString key = QString("%1_%2").arg(typeStr).arg(pageNumber);
    if (extra.isValid()) {
        key += QString("_%1").arg(extra.toString());
    }
    return key;
}

void PDFCacheManager::updateStatistics(bool hit) {
    QMutexLocker locker(&m_statsMutex);
    if (hit) {
        m_hitCount++;
    } else {
        m_missCount++;
    }
    m_accessCount++;
}

void PDFCacheManager::schedulePreload(int pageNumber, CacheItemType type) {
    QString key = generateKey(pageNumber, type);
    if (contains(key) || m_preloadingItems.contains(key)) {
        return;  // Already cached or being preloaded
    }

    m_preloadingItems.insert(key);
    // Note: Actual preloading would require document reference
    // This is a placeholder for the preloading mechanism
}

void PDFCacheManager::performMaintenance() {
    cleanupExpiredItems();

    // Perform optimization if needed
    if (m_lastOptimization.elapsed() > 300000) {  // 5 minutes
        optimizeCache();
        m_lastOptimization.restart();
    }
}

void PDFCacheManager::onPreloadTaskCompleted() {
    // Handle preload task completion
    // This would be called by PreloadTask when it finishes
}

void PDFCacheManager::setMaxMemoryUsage(qint64 bytes) {
    m_maxMemoryUsage = bytes;
    enforceMemoryLimit();
}

void PDFCacheManager::setMaxItems(int count) {
    m_maxItems = count;
    enforceItemLimit();
}

void PDFCacheManager::setItemMaxAge(qint64 milliseconds) {
    m_itemMaxAge = milliseconds;
}

void PDFCacheManager::optimizeCache() {
    QMutexLocker locker(&m_cacheMutex);

    int initialSize = m_cache.size();
    qint64 initialMemory = getCurrentMemoryUsage();

    cleanupExpiredItems();

    // Additional optimization logic could go here

    int itemsRemoved = initialSize - m_cache.size();
    qint64 memoryFreed = initialMemory - getCurrentMemoryUsage();

    if (itemsRemoved > 0 || memoryFreed > 0) {
        emit cacheOptimized(itemsRemoved, memoryFreed);
    }
}

void PDFCacheManager::cleanupExpiredItems() {
    if (m_itemMaxAge <= 0)
        return;

    QMutexLocker locker(&m_cacheMutex);
    auto it = m_cache.begin();
    while (it != m_cache.end()) {
        if (it->isExpired(m_itemMaxAge)) {
            emit itemEvicted(it->key, it->type);
            it = m_cache.erase(it);
        } else {
            ++it;
        }
    }
}

bool PDFCacheManager::evictLeastUsedItems(int count) {
    QMutexLocker locker(&m_cacheMutex);

    if (m_cache.isEmpty() || count <= 0)
        return false;

    // Create list of items with eviction scores
    QList<QPair<double, QString>> candidates;
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it->priority != CachePriority::Critical) {
            double score = calculateEvictionScore(*it);
            candidates.append({score, it->key});
        }
    }

    // Sort by eviction score (lower score = more likely to evict)
    std::sort(candidates.begin(), candidates.end());

    // Evict items
    int evicted = 0;
    for (const auto& candidate : candidates) {
        if (evicted >= count)
            break;

        auto it = m_cache.find(candidate.second);
        if (it != m_cache.end()) {
            emit itemEvicted(it->key, it->type);
            m_cache.erase(it);
            evicted++;
        }
    }
    return evicted > 0;
}

double PDFCacheManager::calculateEvictionScore(const CacheItem& item) const {
    double score = 0.0;

    // Priority weight
    switch (item.priority) {
        case CachePriority::Low:
            score += m_lowPriorityWeight;
            break;
        case CachePriority::Normal:
            score += m_normalPriorityWeight;
            break;
        case CachePriority::High:
            score += m_highPriorityWeight;
            break;
        case CachePriority::Critical:
            score += 1000.0;
            break;  // Should not be evicted
    }

    // Age factor (older items have lower scores)
    qint64 age = QDateTime::currentMSecsSinceEpoch() - item.timestamp;
    score -= age / 1000.0;  // Convert to seconds

    // Access frequency factor
    score += item.accessCount * 10.0;

    // Recent access factor
    qint64 timeSinceLastAccess =
        QDateTime::currentMSecsSinceEpoch() - item.lastAccessed;
    score -= timeSinceLastAccess / 1000.0;

    return score;
}

void PDFCacheManager::enforceMemoryLimit() {
    while (getCurrentMemoryUsage() > m_maxMemoryUsage && !m_cache.isEmpty()) {
        evictLeastUsedItems(1);
    }
}

void PDFCacheManager::enforceItemLimit() {
    while (m_cache.size() > m_maxItems && !m_cache.isEmpty()) {
        evictLeastUsedItems(1);
    }
}

qint64 PDFCacheManager::getCurrentMemoryUsage() const {
    qint64 total = 0;
    for (const auto& item : m_cache) {
        total += item.memorySize;
    }
    return total;
}

CacheStatistics PDFCacheManager::getStatistics() const {
    QMutexLocker locker(&m_cacheMutex);
    QMutexLocker statsLocker(&m_statsMutex);

    CacheStatistics stats;
    stats.totalItems = m_cache.size();
    stats.totalMemoryUsage = getCurrentMemoryUsage();
    stats.hitCount = m_hitCount;
    stats.missCount = m_missCount;
    stats.hitRate =
        (m_hitCount + m_missCount > 0)
            ? static_cast<double>(m_hitCount) / (m_hitCount + m_missCount)
            : 0.0;

    // Calculate items by type
    for (const auto& item : m_cache) {
        int typeIndex = static_cast<int>(item.type);
        if (typeIndex >= 0 && typeIndex < 6) {
            stats.itemsByType[typeIndex]++;
        }
    }

    return stats;
}

double PDFCacheManager::getHitRate() const {
    QMutexLocker locker(&m_statsMutex);
    return (m_hitCount + m_missCount > 0)
               ? static_cast<double>(m_hitCount) / (m_hitCount + m_missCount)
               : 0.0;
}

void PDFCacheManager::resetStatistics() {
    QMutexLocker locker(&m_statsMutex);
    m_hitCount = 0;
    m_missCount = 0;
    m_totalAccessTime = 0;
    m_accessCount = 0;
}

void PDFCacheManager::loadSettings() {
    m_maxMemoryUsage =
        m_settings->value("maxMemoryUsage", m_maxMemoryUsage).toLongLong();
    m_maxItems = m_settings->value("maxItems", m_maxItems).toInt();
    m_itemMaxAge = m_settings->value("itemMaxAge", m_itemMaxAge).toLongLong();
    m_evictionPolicy =
        m_settings->value("evictionPolicy", m_evictionPolicy).toString();
    m_preloadingEnabled =
        m_settings->value("preloadingEnabled", m_preloadingEnabled).toBool();
}

void PDFCacheManager::saveSettings() {
    m_settings->setValue("maxMemoryUsage", m_maxMemoryUsage);
    m_settings->setValue("maxItems", m_maxItems);
    m_settings->setValue("itemMaxAge", m_itemMaxAge);
    m_settings->setValue("evictionPolicy", m_evictionPolicy);
    m_settings->setValue("preloadingEnabled", m_preloadingEnabled);
}
