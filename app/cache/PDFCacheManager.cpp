#include "PDFCacheManager.h"
#include <QApplication>
#include <QDateTime>
#include <QMutexLocker>
#include <QPixmap>
#include <chrono>
// #include <QtConcurrent> // Not available in this MSYS2 setup
#include "../logging/LoggingMacros.h"

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

// PDFCacheManager Implementation class
class PDFCacheManager::Implementation {
public:
    explicit Implementation(PDFCacheManager* q)
        : q_ptr(q),
          maxMemoryUsage(256 * 1024 * 1024)  // 256MB default
          ,
          maxItems(1000),
          itemMaxAge(30 * 60 * 1000)  // 30 minutes
          ,
          evictionPolicy("LRU"),
          lowPriorityWeight(0.1),
          normalPriorityWeight(1.0),
          highPriorityWeight(10.0),
          hitCount(0),
          missCount(0),
          totalAccessTime(0),
          accessCount(0),
          preloadingEnabled(true),
          preloadingStrategy("adaptive"),
          preloadThreadPool(new QThreadPool(q)),
          maintenanceTimer(new QTimer(q)),
          settings(new QSettings("SAST", "Readium-Cache", q)) {
        // Configure thread pool
        preloadThreadPool->setMaxThreadCount(QThread::idealThreadCount());

        // Setup maintenance timer
        maintenanceTimer->setSingleShot(false);
        maintenanceTimer->setInterval(60000);  // 1 minute
        QObject::connect(maintenanceTimer, &QTimer::timeout, q,
                         &PDFCacheManager::performMaintenance);
        maintenanceTimer->start();
    }

    PDFCacheManager* q_ptr;

    // Cache storage
    mutable QMutex cacheMutex;
    QHash<QString, CacheItem> cache;

    // Configuration
    qint64 maxMemoryUsage;
    int maxItems;
    qint64 itemMaxAge;
    QString evictionPolicy;

    // Priority weights for eviction scoring
    double lowPriorityWeight;
    double normalPriorityWeight;
    double highPriorityWeight;

    // Statistics
    mutable QMutex statsMutex;
    qint64 hitCount;
    qint64 missCount;
    qint64 totalAccessTime;
    qint64 accessCount;

    // Preloading
    bool preloadingEnabled;
    QString preloadingStrategy;
    QThreadPool* preloadThreadPool;
    QQueue<QPair<int, CacheItemType>> preloadQueue;
    QSet<QString> preloadingItems;

    // Maintenance
    QTimer* maintenanceTimer;
    QElapsedTimer lastOptimization;

    // Settings
    QSettings* settings;

    // Private methods
    QString generateKey(int pageNumber, CacheItemType type,
                        const QVariant& extra = QVariant()) const;
    void updateStatistics(bool hit);
    void enforceMemoryLimit();
    void enforceItemLimit();
    bool shouldEvict(const CacheItem& item) const;
    double calculateEvictionScore(const CacheItem& item) const;
    void schedulePreload(int pageNumber, CacheItemType type);
};

// Implementation method definitions
QString PDFCacheManager::Implementation::generateKey(
    int pageNumber, CacheItemType type, const QVariant& extra) const {
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

void PDFCacheManager::Implementation::updateStatistics(bool hit) {
    QMutexLocker locker(&statsMutex);
    if (hit) {
        hitCount++;
    } else {
        missCount++;
    }
    accessCount++;
}

void PDFCacheManager::Implementation::schedulePreload(int pageNumber,
                                                      CacheItemType type) {
    QString key = generateKey(pageNumber, type);
    if (q_ptr->contains(key) || preloadingItems.contains(key)) {
        return;  // Already cached or being preloaded
    }

    preloadingItems.insert(key);
    // Note: Actual preloading would require document reference
    // This is a placeholder for the preloading mechanism
}

void PDFCacheManager::Implementation::enforceMemoryLimit() {
    // Implementation for memory limit enforcement
}

void PDFCacheManager::Implementation::enforceItemLimit() {
    // Implementation for item limit enforcement
}

bool PDFCacheManager::Implementation::shouldEvict(const CacheItem& item) const {
    return item.isExpired(itemMaxAge);
}

double PDFCacheManager::Implementation::calculateEvictionScore(
    const CacheItem& item) const {
    double score = 0.0;

    // Priority weight
    switch (item.priority) {
        case CachePriority::Low:
            score += lowPriorityWeight;
            break;
        case CachePriority::Normal:
            score += normalPriorityWeight;
            break;
        case CachePriority::High:
            score += highPriorityWeight;
            break;
        case CachePriority::Critical:
            score += highPriorityWeight * 2.0;  // Even higher priority
            break;
    }

    // Age factor (older items have higher score for eviction)
    qint64 age = QDateTime::currentMSecsSinceEpoch() - item.timestamp;
    score += age / 1000.0;  // Convert to seconds

    // Access frequency factor (less accessed items have higher score)
    score += 1.0 / (item.accessCount + 1);

    return score;
}

// PDFCacheManager Implementation
PDFCacheManager::PDFCacheManager(QObject* parent)
    : QObject(parent), d(std::make_unique<Implementation>(this)) {
    // Load settings
    loadSettings();

    LOG_DEBUG(
        "PDFCacheManager initialized with max memory: {} bytes, max items: {}",
        d->maxMemoryUsage, d->maxItems);
}

PDFCacheManager::~PDFCacheManager() = default;

bool PDFCacheManager::insert(const QString& key, const QVariant& data,
                             CacheItemType type, CachePriority priority,
                             int pageNumber) {
    QMutexLocker locker(&d->cacheMutex);

    CacheItem item;
    item.data = data;
    item.type = type;
    item.priority = priority;
    item.pageNumber = pageNumber;
    item.key = key;
    item.memorySize = item.calculateSize();

    // Check if we need to make room
    while (d->cache.size() >= d->maxItems ||
           (getCurrentMemoryUsage() + item.memorySize) > d->maxMemoryUsage) {
        if (!evictLeastUsedItems(1)) {
            LOG_WARNING("PDFCacheManager: Failed to evict items, cache full");
            return false;
        }
    }

    d->cache[key] = item;

    LOG_DEBUG("PDFCacheManager: Cached item {} type: {} size: {} bytes",
              key.toStdString(), static_cast<int>(type), item.memorySize);

    return true;
}

QVariant PDFCacheManager::get(const QString& key) {
    auto startTime = std::chrono::high_resolution_clock::now();
    QMutexLocker locker(&d->cacheMutex);

    auto it = d->cache.find(key);
    if (it != d->cache.end()) {
        it->updateAccess();
        d->updateStatistics(true);

        auto endTime = std::chrono::high_resolution_clock::now();
        auto accessTime = std::chrono::duration_cast<std::chrono::microseconds>(
                              endTime - startTime)
                              .count();
        emit cacheHit(key, accessTime);
        return it->data;
    }

    d->updateStatistics(false);
    emit cacheMiss(key);
    return QVariant();
}

bool PDFCacheManager::contains(const QString& key) const {
    QMutexLocker locker(&d->cacheMutex);
    return d->cache.contains(key);
}

bool PDFCacheManager::remove(const QString& key) {
    QMutexLocker locker(&d->cacheMutex);

    auto it = d->cache.find(key);
    if (it != d->cache.end()) {
        emit itemEvicted(key, it->type);
        d->cache.erase(it);
        return true;
    }
    return false;
}

void PDFCacheManager::clear() {
    QMutexLocker locker(&d->cacheMutex);
    d->cache.clear();
    LOG_DEBUG("PDFCacheManager: Cache cleared");
}

bool PDFCacheManager::cacheRenderedPage(int pageNumber, const QPixmap& pixmap,
                                        double scaleFactor) {
    QString key =
        d->generateKey(pageNumber, CacheItemType::RenderedPage, scaleFactor);
    return insert(key, pixmap, CacheItemType::RenderedPage,
                  CachePriority::Normal, pageNumber);
}

QPixmap PDFCacheManager::getRenderedPage(int pageNumber, double scaleFactor) {
    QString key =
        d->generateKey(pageNumber, CacheItemType::RenderedPage, scaleFactor);
    QVariant result = get(key);
    return result.canConvert<QPixmap>() ? result.value<QPixmap>() : QPixmap();
}

bool PDFCacheManager::cacheThumbnail(int pageNumber, const QPixmap& thumbnail) {
    QString key = d->generateKey(pageNumber, CacheItemType::Thumbnail);
    return insert(key, thumbnail, CacheItemType::Thumbnail, CachePriority::High,
                  pageNumber);
}

QPixmap PDFCacheManager::getThumbnail(int pageNumber) {
    QString key = d->generateKey(pageNumber, CacheItemType::Thumbnail);
    QVariant result = get(key);
    return result.canConvert<QPixmap>() ? result.value<QPixmap>() : QPixmap();
}

bool PDFCacheManager::cacheTextContent(int pageNumber, const QString& text) {
    QString key = d->generateKey(pageNumber, CacheItemType::TextContent);
    return insert(key, text, CacheItemType::TextContent, CachePriority::Normal,
                  pageNumber);
}

QString PDFCacheManager::getTextContent(int pageNumber) {
    QString key = d->generateKey(pageNumber, CacheItemType::TextContent);
    QVariant result = get(key);
    return result.canConvert<QString>() ? result.toString() : QString();
}

void PDFCacheManager::enablePreloading(bool enabled) {
    d->preloadingEnabled = enabled;
    LOG_DEBUG("PDFCacheManager: Preloading {}",
              enabled ? "enabled" : "disabled");
}

void PDFCacheManager::preloadPages(const QList<int>& pageNumbers,
                                   CacheItemType type) {
    if (!d->preloadingEnabled)
        return;

    for (int pageNumber : pageNumbers) {
        d->schedulePreload(pageNumber, type);
    }
}

void PDFCacheManager::preloadAroundPage(int centerPage, int radius) {
    if (!d->preloadingEnabled)
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
    d->preloadingStrategy = strategy;
    LOG_DEBUG("PDFCacheManager: Preloading strategy set to {}",
              strategy.toStdString());
}

void PDFCacheManager::performMaintenance() {
    cleanupExpiredItems();

    // Perform optimization if needed
    if (d->lastOptimization.elapsed() > 300000) {  // 5 minutes
        optimizeCache();
        d->lastOptimization.restart();
    }
}

void PDFCacheManager::onPreloadTaskCompleted() {
    // Handle preload task completion
    // This would be called by PreloadTask when it finishes
}

void PDFCacheManager::setMaxMemoryUsage(qint64 bytes) {
    d->maxMemoryUsage = bytes;
    d->enforceMemoryLimit();
}

void PDFCacheManager::setMaxItems(int count) {
    d->maxItems = count;
    d->enforceItemLimit();
}

void PDFCacheManager::setItemMaxAge(qint64 milliseconds) {
    d->itemMaxAge = milliseconds;
}

void PDFCacheManager::optimizeCache() {
    QMutexLocker locker(&d->cacheMutex);

    int initialSize = d->cache.size();
    qint64 initialMemory = getCurrentMemoryUsage();

    cleanupExpiredItems();

    // Additional optimization logic could go here

    int itemsRemoved = initialSize - d->cache.size();
    qint64 memoryFreed = initialMemory - getCurrentMemoryUsage();

    if (itemsRemoved > 0 || memoryFreed > 0) {
        emit cacheOptimized(itemsRemoved, memoryFreed);
    }
}

void PDFCacheManager::cleanupExpiredItems() {
    if (d->itemMaxAge <= 0)
        return;

    QMutexLocker locker(&d->cacheMutex);
    auto it = d->cache.begin();
    while (it != d->cache.end()) {
        if (it->isExpired(d->itemMaxAge)) {
            emit itemEvicted(it->key, it->type);
            it = d->cache.erase(it);
        } else {
            ++it;
        }
    }
}

bool PDFCacheManager::evictLeastUsedItems(int count) {
    QMutexLocker locker(&d->cacheMutex);

    if (d->cache.isEmpty() || count <= 0)
        return false;

    // Create list of items with eviction scores
    QList<QPair<double, QString>> candidates;
    for (auto it = d->cache.begin(); it != d->cache.end(); ++it) {
        if (it->priority != CachePriority::Critical) {
            double score = d->calculateEvictionScore(*it);
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

        auto it = d->cache.find(candidate.second);
        if (it != d->cache.end()) {
            emit itemEvicted(it->key, it->type);
            d->cache.erase(it);
            evicted++;
        }
    }
    return evicted > 0;
}

qint64 PDFCacheManager::getCurrentMemoryUsage() const {
    qint64 total = 0;
    for (const auto& item : d->cache) {
        total += item.memorySize;
    }
    return total;
}

CacheStatistics PDFCacheManager::getStatistics() const {
    QMutexLocker locker(&d->cacheMutex);
    QMutexLocker statsLocker(&d->statsMutex);

    CacheStatistics stats;
    stats.totalItems = d->cache.size();
    stats.totalMemoryUsage = getCurrentMemoryUsage();
    stats.hitCount = d->hitCount;
    stats.missCount = d->missCount;
    stats.hitRate =
        (d->hitCount + d->missCount > 0)
            ? static_cast<double>(d->hitCount) / (d->hitCount + d->missCount)
            : 0.0;

    // Calculate items by type
    for (const auto& item : d->cache) {
        int typeIndex = static_cast<int>(item.type);
        if (typeIndex >= 0 && typeIndex < 6) {
            stats.itemsByType[typeIndex]++;
        }
    }

    return stats;
}

double PDFCacheManager::getHitRate() const {
    QMutexLocker locker(&d->statsMutex);
    return (d->hitCount + d->missCount > 0)
               ? static_cast<double>(d->hitCount) / (d->hitCount + d->missCount)
               : 0.0;
}

void PDFCacheManager::resetStatistics() {
    QMutexLocker locker(&d->statsMutex);
    d->hitCount = 0;
    d->missCount = 0;
    d->totalAccessTime = 0;
    d->accessCount = 0;
}

// Getter methods that were converted from inline
qint64 PDFCacheManager::getMaxMemoryUsage() const { return d->maxMemoryUsage; }

int PDFCacheManager::getMaxItems() const { return d->maxItems; }

qint64 PDFCacheManager::getItemMaxAge() const { return d->itemMaxAge; }

QString PDFCacheManager::getEvictionPolicy() const { return d->evictionPolicy; }

bool PDFCacheManager::isPreloadingEnabled() const {
    return d->preloadingEnabled;
}

void PDFCacheManager::loadSettings() {
    d->maxMemoryUsage =
        d->settings->value("maxMemoryUsage", d->maxMemoryUsage).toLongLong();
    d->maxItems = d->settings->value("maxItems", d->maxItems).toInt();
    d->itemMaxAge =
        d->settings->value("itemMaxAge", d->itemMaxAge).toLongLong();
    d->evictionPolicy =
        d->settings->value("evictionPolicy", d->evictionPolicy).toString();
    d->preloadingEnabled =
        d->settings->value("preloadingEnabled", d->preloadingEnabled).toBool();
}

void PDFCacheManager::saveSettings() {
    d->settings->setValue("maxMemoryUsage", d->maxMemoryUsage);
    d->settings->setValue("maxItems", d->maxItems);
    d->settings->setValue("itemMaxAge", d->itemMaxAge);
    d->settings->setValue("evictionPolicy", d->evictionPolicy);
    d->settings->setValue("preloadingEnabled", d->preloadingEnabled);
}
