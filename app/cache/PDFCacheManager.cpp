#include "PDFCacheManager.h"

#include <QApplication>
#include <QDataStream>
#include <QDateTime>
#include <QFile>
#include <QImage>
#include <QMutexLocker>
#include <QPixmap>
#include <chrono>
#include <limits>

#include "../logging/LoggingMacros.h"

#include "../utils/SafePDFRenderer.h"

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
    if (maxAge <= 0) {
        return false;
    }
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    return (currentTime - timestamp) > maxAge;
}

// PreloadTask Implementation
PreloadTask::PreloadTask(Poppler::Document* document, int pageNumber,
                         CacheItemType type, QObject* target, QMutex* mutex)
    : m_document(document),
      m_pageNumber(pageNumber),
      m_type(type),
      m_target(target),
      m_mutex(mutex) {
    setAutoDelete(true);
}

void PreloadTask::run() {
    if (!m_document || m_pageNumber < 0) {
        return;
    }

    try {
        std::unique_ptr<Poppler::Page> page;

        // Scope for mutex lock - critical section for document access
        {
            std::unique_ptr<QMutexLocker<QMutex>> locker;
            if (m_mutex) {
                locker = std::make_unique<QMutexLocker<QMutex>>(m_mutex);
            }
            page = m_document->page(m_pageNumber);
        }

        if (!page) {
            return;
        }

        QVariant result;
        switch (m_type) {
            case CacheItemType::RenderedPage: {
                QImage image = SafePDFRendering::renderPage(page.get(), 150.0);
                result = image;
                break;
            }
            case CacheItemType::Thumbnail: {
                // Use SafePDFRenderer for thumbnail generation too
                // First render at 72 DPI
                QImage image = SafePDFRendering::renderPage(page.get(), 72.0);
                if (!image.isNull()) {
                    // Then scale down
                    image = image.scaled(128, 128, Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);
                }
                result = image;
                break;
            }
            case CacheItemType::TextContent: {
                result = page->text(QRectF());
                break;
            }
            default:
                return;
        }

        QMetaObject::invokeMethod(
            m_target, "onPreloadTaskCompleted", Qt::QueuedConnection,
            Q_ARG(int, m_pageNumber), Q_ARG(int, static_cast<int>(m_type)),
            Q_ARG(QVariant, result));

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
          maxMemoryUsage(256 * 1024 * 1024),  // 256MB default
          maxItems(1000),
          itemMaxAge(30 * 60 * 1000),  // 30 minutes
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
    mutable QMutex documentMutex;  // Mutex for thread-safe document access
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

    // Add to preload queue
    preloadQueue.enqueue(qMakePair(pageNumber, type));
    preloadingItems.insert(key);

    // Note: Actual preloading requires a document reference which should be
    // provided by the caller (e.g., DocumentModel or RenderModel). The cache
    // manager doesn't own the document to maintain proper separation of
    // concerns.
    //
    // To execute preloading, the document owner should:
    // 1. Call preloadPages() or preloadAroundPage()
    // 2. Provide the document reference via a new method like
    // executePreload(document)
    // 3. Or use the PreloadTask directly with their document reference
    //
    // Example usage from DocumentModel:
    //   cacheManager->preloadAroundPage(currentPage);
    //   cacheManager->executePreload(m_document.get());
    //
    // For now, we queue the request and emit a signal that preloading is
    // needed
    emit q_ptr->preloadRequested(pageNumber, type);
}

void PDFCacheManager::Implementation::enforceMemoryLimit() {
    // Calculate current memory usage
    qint64 currentUsage = 0;
    for (const auto& item : cache) {
        currentUsage += item.memorySize;
    }

    // Check if we need to evict items
    if (currentUsage <= maxMemoryUsage) {
        return;  // Within limits, nothing to do
    }

    LOG_INFO(
        "PDFCacheManager: Enforcing memory limit - current: {} bytes, limit: "
        "{} "
        "bytes",
        currentUsage, maxMemoryUsage);

    int itemsEvicted = 0;
    qint64 memoryFreed = 0;

    // Evict items until we're within the memory limit
    while (currentUsage > maxMemoryUsage && !cache.isEmpty()) {
        // Build list of evictable candidates (exclude Critical priority)
        QList<QPair<double, QString>> candidates;
        for (auto& it : cache) {
            if (it.priority != CachePriority::Critical) {
                double score = calculateEvictionScore(it);
                candidates.append({score, it.key});
            }
        }

        // Check if we have any evictable items
        if (candidates.isEmpty()) {
            LOG_WARNING(
                "PDFCacheManager: Cannot enforce memory limit - all items are "
                "Critical priority");
            break;
        }

        // Sort by eviction score (lower score = evict first)
        std::sort(candidates.begin(), candidates.end());

        // Evict the item with the lowest score
        auto it = cache.find(candidates.first().second);
        if (it != cache.end()) {
            qint64 itemSize = it->memorySize;
            emit q_ptr->itemEvicted(it->key, it->type);
            cache.erase(it);
            currentUsage -= itemSize;
            memoryFreed += itemSize;
            itemsEvicted++;
        } else {
            LOG_WARNING(
                "PDFCacheManager: Failed to find item for eviction during "
                "memory "
                "limit enforcement");
            break;
        }
    }

    LOG_INFO(
        "PDFCacheManager: Memory limit enforced - evicted {} items, freed {} "
        "bytes",
        itemsEvicted, memoryFreed);
}

void PDFCacheManager::Implementation::enforceItemLimit() {
    // Check if we need to evict items
    if (cache.size() <= maxItems) {
        return;  // Within limits, nothing to do
    }

    LOG_INFO(
        "PDFCacheManager: Enforcing item limit - current: {} items, limit: "
        "{} items",
        cache.size(), maxItems);

    int itemsEvicted = 0;
    qint64 memoryFreed = 0;

    // Evict items until we're within the item limit
    while (cache.size() > maxItems && !cache.isEmpty()) {
        // Build list of evictable candidates (exclude Critical priority)
        QList<QPair<double, QString>> candidates;
        for (auto& it : cache) {
            if (it.priority != CachePriority::Critical) {
                double score = calculateEvictionScore(it);
                candidates.append({score, it.key});
            }
        }

        // Check if we have any evictable items
        if (candidates.isEmpty()) {
            LOG_WARNING(
                "PDFCacheManager: Cannot enforce item limit - all items are "
                "Critical priority");
            break;
        }

        // Sort by eviction score (lower score = evict first)
        std::sort(candidates.begin(), candidates.end());

        // Evict the item with the lowest score
        auto it = cache.find(candidates.first().second);
        if (it != cache.end()) {
            qint64 itemSize = it->memorySize;
            emit q_ptr->itemEvicted(it->key, it->type);
            cache.erase(it);
            memoryFreed += itemSize;
            itemsEvicted++;
        } else {
            LOG_WARNING(
                "PDFCacheManager: Failed to find item for eviction during item "
                "limit enforcement");
            break;
        }
    }

    LOG_INFO(
        "PDFCacheManager: Item limit enforced - evicted {} items, freed {} "
        "bytes",
        itemsEvicted, memoryFreed);
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
    : QObject(parent), m_pimpl(std::make_unique<Implementation>(this)) {
    // Load settings
    loadSettings();

    LOG_DEBUG(
        "PDFCacheManager initialized with max memory: {} bytes, max items: {}",
        m_pimpl->maxMemoryUsage, m_pimpl->maxItems);
}

PDFCacheManager::~PDFCacheManager() = default;

bool PDFCacheManager::insert(const QString& key, const QVariant& data,
                             CacheItemType type, CachePriority priority,
                             int pageNumber) {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    CacheItem item;
    item.data = data;
    item.type = type;
    item.priority = priority;
    item.pageNumber = pageNumber;
    item.key = key;
    item.memorySize = item.calculateSize();

    // Check if we need to make room
    while (m_pimpl->cache.size() >= m_pimpl->maxItems ||
           (getCurrentMemoryUsage() + item.memorySize) >
               m_pimpl->maxMemoryUsage) {
        // Evict inline to avoid mutex deadlock
        if (m_pimpl->cache.isEmpty()) {
            LOG_WARNING("PDFCacheManager: Cache is empty, cannot evict");
            return false;
        }

        // Create list of items with eviction scores
        QList<QPair<double, QString>> candidates;
        for (auto it = m_pimpl->cache.begin(); it != m_pimpl->cache.end();
             ++it) {
            if (it->priority != CachePriority::Critical) {
                double score = m_pimpl->calculateEvictionScore(*it);
                candidates.append({score, it->key});
            }
        }

        if (candidates.isEmpty()) {
            LOG_WARNING("PDFCacheManager: No evictable items (all critical)");
            return false;
        }

        // Sort by eviction score (lower score = more likely to evict)
        std::sort(candidates.begin(), candidates.end());

        // Evict the least used item
        auto it = m_pimpl->cache.find(candidates.first().second);
        if (it != m_pimpl->cache.end()) {
            emit itemEvicted(it->key, it->type);
            m_pimpl->cache.erase(it);
        } else {
            LOG_WARNING("PDFCacheManager: Failed to evict item");
            return false;
        }
    }

    m_pimpl->cache[key] = item;

    LOG_DEBUG("PDFCacheManager: Cached item {} type: {} size: {} bytes",
              key.toStdString(), static_cast<int>(type), item.memorySize);

    return true;
}

QVariant PDFCacheManager::get(const QString& key) {
    auto startTime = std::chrono::high_resolution_clock::now();
    QMutexLocker locker(&m_pimpl->cacheMutex);

    auto it = m_pimpl->cache.find(key);
    if (it != m_pimpl->cache.end()) {
        it->updateAccess();
        m_pimpl->updateStatistics(true);

        auto endTime = std::chrono::high_resolution_clock::now();
        auto accessTimeMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(endTime -
                                                                  startTime)
                .count();
        {
            QMutexLocker statsLocker(&m_pimpl->statsMutex);
            m_pimpl->totalAccessTime += accessTimeMs;
        }
        emit cacheHit(key, accessTimeMs);
        return it->data;
    }

    m_pimpl->updateStatistics(false);
    emit cacheMiss(key);
    return QVariant();
}

bool PDFCacheManager::contains(const QString& key) const {
    QMutexLocker locker(&m_pimpl->cacheMutex);
    return m_pimpl->cache.contains(key);
}

bool PDFCacheManager::remove(const QString& key) {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    auto it = m_pimpl->cache.find(key);
    if (it != m_pimpl->cache.end()) {
        emit itemEvicted(key, it->type);
        m_pimpl->cache.erase(it);
        return true;
    }
    return false;
}

void PDFCacheManager::clear() {
    QMutexLocker locker(&m_pimpl->cacheMutex);
    m_pimpl->cache.clear();
    LOG_DEBUG("PDFCacheManager: Cache cleared");
}

bool PDFCacheManager::cacheRenderedPage(int pageNumber, const QPixmap& pixmap,
                                        double scaleFactor) {
    QString key = m_pimpl->generateKey(pageNumber, CacheItemType::RenderedPage,
                                       scaleFactor);
    return insert(key, pixmap, CacheItemType::RenderedPage,
                  CachePriority::Normal, pageNumber);
}

QPixmap PDFCacheManager::getRenderedPage(int pageNumber, double scaleFactor) {
    QString key = m_pimpl->generateKey(pageNumber, CacheItemType::RenderedPage,
                                       scaleFactor);
    QVariant result = get(key);
    return result.canConvert<QPixmap>() ? result.value<QPixmap>() : QPixmap();
}

bool PDFCacheManager::cacheThumbnail(int pageNumber, const QPixmap& thumbnail) {
    QString key = m_pimpl->generateKey(pageNumber, CacheItemType::Thumbnail);
    return insert(key, thumbnail, CacheItemType::Thumbnail, CachePriority::High,
                  pageNumber);
}

QPixmap PDFCacheManager::getThumbnail(int pageNumber) {
    QString key = m_pimpl->generateKey(pageNumber, CacheItemType::Thumbnail);
    QVariant result = get(key);
    return result.canConvert<QPixmap>() ? result.value<QPixmap>() : QPixmap();
}

bool PDFCacheManager::cacheTextContent(int pageNumber, const QString& text) {
    QString key = m_pimpl->generateKey(pageNumber, CacheItemType::TextContent);
    return insert(key, text, CacheItemType::TextContent, CachePriority::Normal,
                  pageNumber);
}

QString PDFCacheManager::getTextContent(int pageNumber) {
    QString key = m_pimpl->generateKey(pageNumber, CacheItemType::TextContent);
    QVariant result = get(key);
    return result.canConvert<QString>() ? result.toString() : QString();
}

void PDFCacheManager::enablePreloading(bool enabled) {
    m_pimpl->preloadingEnabled = enabled;
    LOG_DEBUG("PDFCacheManager: Preloading {}",
              enabled ? "enabled" : "disabled");
}

void PDFCacheManager::preloadPages(const QList<int>& pageNumbers,
                                   CacheItemType type) {
    if (!m_pimpl->preloadingEnabled) {
        return;
    }

    for (int pageNumber : pageNumbers) {
        m_pimpl->schedulePreload(pageNumber, type);
    }
}

void PDFCacheManager::preloadAroundPage(int centerPage, int radius) {
    if (!m_pimpl->preloadingEnabled) {
        return;
    }

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
    m_pimpl->preloadingStrategy = strategy;
    LOG_DEBUG("PDFCacheManager: Preloading strategy set to {}",
              strategy.toStdString());
}

void PDFCacheManager::executePreload(Poppler::Document* document) {
    if (!m_pimpl->preloadingEnabled || document == nullptr) {
        return;
    }

    while (!m_pimpl->preloadQueue.isEmpty()) {
        auto taskInfo = m_pimpl->preloadQueue.dequeue();
        int pageNumber = taskInfo.first;
        CacheItemType type = taskInfo.second;

        if (type == CacheItemType::Thumbnail ||
            type == CacheItemType::TextContent) {
            auto* task = new PreloadTask(document, pageNumber, type, this,
                                         &m_pimpl->documentMutex);
            m_pimpl->preloadThreadPool->start(task);
        } else {
            QString k = m_pimpl->generateKey(pageNumber, type);
            m_pimpl->preloadingItems.remove(k);
        }
    }
}

void PDFCacheManager::performMaintenance() {
    cleanupExpiredItems();

    // Perform optimization if needed
    if (m_pimpl->lastOptimization.elapsed() > 300000) {  // 5 minutes
        optimizeCache();
        m_pimpl->lastOptimization.restart();
    }
}

void PDFCacheManager::onPreloadTaskCompleted(int pageNumber, int typeValue,
                                             QVariant value) {
    CacheItemType type = static_cast<CacheItemType>(typeValue);

    QString baseKey = m_pimpl->generateKey(pageNumber, type);
    m_pimpl->preloadingItems.remove(baseKey);

    switch (type) {
        case CacheItemType::Thumbnail: {
            QPixmap pix;
            if (value.canConvert<QImage>()) {
                pix = QPixmap::fromImage(value.value<QImage>());
            } else if (value.canConvert<QPixmap>()) {
                pix = value.value<QPixmap>();
            }
            if (!pix.isNull()) {
                QString key =
                    m_pimpl->generateKey(pageNumber, CacheItemType::Thumbnail);
                insert(key, pix, CacheItemType::Thumbnail, CachePriority::High,
                       pageNumber);
                emit preloadCompleted(pageNumber, CacheItemType::Thumbnail);
            }
            break;
        }
        case CacheItemType::TextContent: {
            QString text = value.toString();
            if (!text.isEmpty()) {
                QString key = m_pimpl->generateKey(pageNumber,
                                                   CacheItemType::TextContent);
                insert(key, text, CacheItemType::TextContent,
                       CachePriority::Normal, pageNumber);
                emit preloadCompleted(pageNumber, CacheItemType::TextContent);
            }
            break;
        }
        default:
            break;
    }
}

void PDFCacheManager::setMaxMemoryUsage(qint64 bytes) {
    QMutexLocker locker(&m_pimpl->cacheMutex);
    m_pimpl->maxMemoryUsage = bytes;
    m_pimpl->enforceMemoryLimit();
}

void PDFCacheManager::setMaxItems(int count) {
    QMutexLocker locker(&m_pimpl->cacheMutex);
    m_pimpl->maxItems = count;
    m_pimpl->enforceItemLimit();
}

void PDFCacheManager::setItemMaxAge(qint64 milliseconds) {
    m_pimpl->itemMaxAge = milliseconds;
}

void PDFCacheManager::optimizeCache() {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    int initialSize = m_pimpl->cache.size();
    qint64 initialMemory = getCurrentMemoryUsage();

    // Cleanup expired items inline to avoid mutex deadlock
    if (m_pimpl->itemMaxAge > 0) {
        auto it = m_pimpl->cache.begin();
        while (it != m_pimpl->cache.end()) {
            if (it->isExpired(m_pimpl->itemMaxAge)) {
                emit itemEvicted(it->key, it->type);
                it = m_pimpl->cache.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Additional optimization logic could go here

    int itemsRemoved = initialSize - m_pimpl->cache.size();
    qint64 memoryFreed = initialMemory - getCurrentMemoryUsage();

    if (itemsRemoved > 0 || memoryFreed > 0) {
        emit cacheOptimized(itemsRemoved, memoryFreed);
    }
}

void PDFCacheManager::cleanupExpiredItems() {
    if (m_pimpl->itemMaxAge <= 0) {
        return;
    }

    QMutexLocker locker(&m_pimpl->cacheMutex);
    auto it = m_pimpl->cache.begin();
    while (it != m_pimpl->cache.end()) {
        if (it->isExpired(m_pimpl->itemMaxAge)) {
            emit itemEvicted(it->key, it->type);
            it = m_pimpl->cache.erase(it);
        } else {
            ++it;
        }
    }
}

bool PDFCacheManager::evictLeastUsedItems(int count) {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    if (m_pimpl->cache.isEmpty() || count <= 0) {
        return false;
    }

    // Create list of items with eviction scores
    QList<QPair<double, QString>> candidates;
    for (auto it = m_pimpl->cache.begin(); it != m_pimpl->cache.end(); ++it) {
        if (it->priority != CachePriority::Critical) {
            double score = m_pimpl->calculateEvictionScore(*it);
            candidates.append({score, it->key});
        }
    }

    // Sort by eviction score (lower score = more likely to evict)
    std::sort(candidates.begin(), candidates.end());

    // Evict items
    int evicted = 0;
    for (const auto& candidate : candidates) {
        if (evicted >= count) {
            break;
        }

        auto it = m_pimpl->cache.find(candidate.second);
        if (it != m_pimpl->cache.end()) {
            emit itemEvicted(it->key, it->type);
            m_pimpl->cache.erase(it);
            evicted++;
        }
    }
    return evicted > 0;
}

qint64 PDFCacheManager::getCurrentMemoryUsage() const {
    qint64 total = 0;
    for (const auto& item : m_pimpl->cache) {
        total += item.memorySize;
    }
    return total;
}

CacheStatistics PDFCacheManager::getStatistics() const {
    QMutexLocker locker(&m_pimpl->cacheMutex);
    QMutexLocker statsLocker(&m_pimpl->statsMutex);

    CacheStatistics stats;
    stats.totalItems = m_pimpl->cache.size();
    stats.totalMemoryUsage = getCurrentMemoryUsage();
    stats.hitCount = m_pimpl->hitCount;
    stats.missCount = m_pimpl->missCount;
    stats.hitRate = (m_pimpl->hitCount + m_pimpl->missCount > 0)
                        ? static_cast<double>(m_pimpl->hitCount) /
                              (m_pimpl->hitCount + m_pimpl->missCount)
                        : 0.0;

    // Calculate items by type
    for (const auto& item : m_pimpl->cache) {
        int typeIndex = static_cast<int>(item.type);
        if (typeIndex >= 0 && typeIndex < 6) {
            stats.itemsByType[typeIndex]++;
        }
    }

    if (m_pimpl->accessCount > 0) {
        stats.averageAccessTime =
            m_pimpl->totalAccessTime / m_pimpl->accessCount;
    }

    if (!m_pimpl->cache.isEmpty()) {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        qint64 minTs = std::numeric_limits<qint64>::max();
        qint64 maxTs = 0;
        for (const auto& item : m_pimpl->cache) {
            if (item.timestamp < minTs)
                minTs = item.timestamp;
            if (item.timestamp > maxTs)
                maxTs = item.timestamp;
        }
        if (minTs != std::numeric_limits<qint64>::max()) {
            stats.oldestItemAge = now - minTs;
        }
        if (maxTs != 0) {
            stats.newestItemAge = now - maxTs;
        }
    }

    return stats;
}

double PDFCacheManager::getHitRate() const {
    QMutexLocker locker(&m_pimpl->statsMutex);
    return (m_pimpl->hitCount + m_pimpl->missCount > 0)
               ? static_cast<double>(m_pimpl->hitCount) /
                     (m_pimpl->hitCount + m_pimpl->missCount)
               : 0.0;
}

void PDFCacheManager::resetStatistics() {
    QMutexLocker locker(&m_pimpl->statsMutex);
    m_pimpl->hitCount = 0;
    m_pimpl->missCount = 0;
    m_pimpl->totalAccessTime = 0;
    m_pimpl->accessCount = 0;
}

// Getter methods that were converted from inline
qint64 PDFCacheManager::getMaxMemoryUsage() const {
    return m_pimpl->maxMemoryUsage;
}

int PDFCacheManager::getMaxItems() const { return m_pimpl->maxItems; }

qint64 PDFCacheManager::getItemMaxAge() const { return m_pimpl->itemMaxAge; }

QString PDFCacheManager::getEvictionPolicy() const {
    return m_pimpl->evictionPolicy;
}

bool PDFCacheManager::isPreloadingEnabled() const {
    return m_pimpl->preloadingEnabled;
}

void PDFCacheManager::loadSettings() {
    m_pimpl->maxMemoryUsage =
        m_pimpl->settings->value("maxMemoryUsage", m_pimpl->maxMemoryUsage)
            .toLongLong();
    m_pimpl->maxItems =
        m_pimpl->settings->value("maxItems", m_pimpl->maxItems).toInt();
    m_pimpl->itemMaxAge =
        m_pimpl->settings->value("itemMaxAge", m_pimpl->itemMaxAge)
            .toLongLong();
    m_pimpl->evictionPolicy =
        m_pimpl->settings->value("evictionPolicy", m_pimpl->evictionPolicy)
            .toString();
    m_pimpl->preloadingEnabled =
        m_pimpl->settings
            ->value("preloadingEnabled", m_pimpl->preloadingEnabled)
            .toBool();
}

void PDFCacheManager::saveSettings() {
    m_pimpl->settings->setValue("maxMemoryUsage", m_pimpl->maxMemoryUsage);
    m_pimpl->settings->setValue("maxItems", m_pimpl->maxItems);
    m_pimpl->settings->setValue("itemMaxAge", m_pimpl->itemMaxAge);
    m_pimpl->settings->setValue("evictionPolicy", m_pimpl->evictionPolicy);
    m_pimpl->settings->setValue("preloadingEnabled",
                                m_pimpl->preloadingEnabled);
}

// ============================================================================
// Phase 1: Core Cache Management Methods
// ============================================================================

void PDFCacheManager::compactCache() {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    LOG_DEBUG("PDFCacheManager: Compacting cache");

    int initialSize = m_pimpl->cache.size();
    qint64 initialMemory = getCurrentMemoryUsage();

    // Remove expired items
    if (m_pimpl->itemMaxAge > 0) {
        auto it = m_pimpl->cache.begin();
        while (it != m_pimpl->cache.end()) {
            if (it->isExpired(m_pimpl->itemMaxAge)) {
                emit itemEvicted(it->key, it->type);
                it = m_pimpl->cache.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Remove items with zero access count that are older than 5 minutes
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 minAge = 5 * 60 * 1000;  // 5 minutes

    auto it = m_pimpl->cache.begin();
    while (it != m_pimpl->cache.end()) {
        if (it->accessCount == 0 && (currentTime - it->timestamp) > minAge) {
            emit itemEvicted(it->key, it->type);
            it = m_pimpl->cache.erase(it);
        } else {
            ++it;
        }
    }

    int itemsRemoved = initialSize - m_pimpl->cache.size();
    qint64 memoryFreed = initialMemory - getCurrentMemoryUsage();

    LOG_INFO(
        "PDFCacheManager: Cache compacted - removed {} items, freed {} bytes",
        itemsRemoved, memoryFreed);

    emit cacheOptimized(itemsRemoved, memoryFreed);
}

void PDFCacheManager::setEvictionPolicy(const QString& policy) {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    // Validate policy
    QStringList validPolicies = {"LRU", "LFU", "FIFO", "Priority"};
    if (!validPolicies.contains(policy)) {
        LOG_WARNING("PDFCacheManager: Invalid eviction policy '{}', using LRU",
                    policy.toStdString());
        m_pimpl->evictionPolicy = "LRU";
        return;
    }

    m_pimpl->evictionPolicy = policy;
    LOG_INFO("PDFCacheManager: Eviction policy set to {}",
             policy.toStdString());
}

void PDFCacheManager::setPriorityWeights(double lowWeight, double normalWeight,
                                         double highWeight) {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    // Validate weights (must be positive)
    if (lowWeight < 0.0 || normalWeight < 0.0 || highWeight < 0.0) {
        LOG_WARNING(
            "PDFCacheManager: Invalid priority weights (must be >= 0), "
            "keeping current values");
        return;
    }

    m_pimpl->lowPriorityWeight = lowWeight;
    m_pimpl->normalPriorityWeight = normalWeight;
    m_pimpl->highPriorityWeight = highWeight;

    LOG_INFO(
        "PDFCacheManager: Priority weights set to Low={}, Normal={}, High={}",
        lowWeight, normalWeight, highWeight);
}

// ============================================================================
// Phase 2: Utility Methods
// ============================================================================

bool PDFCacheManager::exportCacheToFile(const QString& filePath) {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    LOG_INFO("PDFCacheManager: Exporting cache to {}", filePath.toStdString());

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR("PDFCacheManager: Failed to open file for export: {}",
                  file.errorString().toStdString());
        emit cacheExported(filePath, false);
        return false;
    }

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_0);

    // Write header
    out << QString("PDFCacheExport");
    out << qint32(1);  // Version
    out << QDateTime::currentDateTime();

    // Write cache configuration
    out << m_pimpl->maxMemoryUsage;
    out << m_pimpl->maxItems;
    out << m_pimpl->itemMaxAge;
    out << m_pimpl->evictionPolicy;

    // Write cache items (only metadata, not actual data for size reasons)
    out << qint32(m_pimpl->cache.size());
    for (auto it = m_pimpl->cache.constBegin(); it != m_pimpl->cache.constEnd();
         ++it) {
        out << it->key;
        out << static_cast<qint32>(it->type);
        out << static_cast<qint32>(it->priority);
        out << it->timestamp;
        out << it->accessCount;
        out << it->lastAccessed;
        out << it->pageNumber;
        out << it->memorySize;
    }

    file.close();

    LOG_INFO("PDFCacheManager: Cache exported successfully");
    emit cacheExported(filePath, true);
    return true;
}

bool PDFCacheManager::importCacheFromFile(const QString& filePath) {
    LOG_INFO("PDFCacheManager: Importing cache from {}",
             filePath.toStdString());

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR("PDFCacheManager: Failed to open file for import: {}",
                  file.errorString().toStdString());
        emit cacheImported(filePath, false);
        return false;
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_0);

    // Read and validate header
    QString header;
    in >> header;
    if (header != "PDFCacheExport") {
        LOG_ERROR("PDFCacheManager: Invalid cache export file format");
        file.close();
        emit cacheImported(filePath, false);
        return false;
    }

    qint32 version;
    QDateTime exportTime;
    in >> version >> exportTime;

    if (version != 1) {
        LOG_ERROR("PDFCacheManager: Unsupported cache export version: {}",
                  version);
        file.close();
        emit cacheImported(filePath, false);
        return false;
    }

    // Read cache configuration (but don't apply it, just log)
    qint64 importedMaxMemory;
    int importedMaxItems;
    qint64 importedMaxAge;
    QString importedPolicy;

    in >> importedMaxMemory >> importedMaxItems >> importedMaxAge >>
        importedPolicy;

    LOG_INFO(
        "PDFCacheManager: Import file created at {}, contains config: "
        "maxMemory={}, maxItems={}, maxAge={}, policy={}",
        exportTime.toString().toStdString(), importedMaxMemory,
        importedMaxItems, importedMaxAge, importedPolicy.toStdString());

    // Note: We only import metadata, not actual cached data
    // This is intentional as cached data may be stale or invalid
    qint32 itemCount;
    in >> itemCount;

    LOG_INFO(
        "PDFCacheManager: Import file contains {} cache item metadata entries",
        itemCount);

    // Skip reading the actual items since we can't restore the data
    // This import is mainly for configuration and statistics purposes

    file.close();

    LOG_INFO("PDFCacheManager: Cache metadata imported successfully");
    emit cacheImported(filePath, true);
    return true;
}

void PDFCacheManager::defragmentCache() {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    LOG_DEBUG("PDFCacheManager: Defragmenting cache");

    int initialSize = m_pimpl->cache.size();

    // Create a new hash and copy items in order of priority and access
    QHash<QString, CacheItem> newCache;
    newCache.reserve(m_pimpl->cache.size());

    // Sort items by priority (high to low) and access count (high to low)
    QList<CacheItem> sortedItems;
    for (const auto& item : m_pimpl->cache) {
        sortedItems.append(item);
    }

    std::sort(sortedItems.begin(), sortedItems.end(),
              [](const CacheItem& a, const CacheItem& b) {
                  // First sort by priority
                  if (a.priority != b.priority) {
                      return static_cast<int>(a.priority) >
                             static_cast<int>(b.priority);
                  }
                  // Then by access count
                  return a.accessCount > b.accessCount;
              });

    // Rebuild cache with sorted items
    for (const auto& item : sortedItems) {
        newCache.insert(item.key, item);
    }

    m_pimpl->cache = std::move(newCache);

    LOG_INFO("PDFCacheManager: Cache defragmented - {} items reorganized",
             initialSize);

    emit cacheDefragmented(m_pimpl->cache.size());
}

// ============================================================================
// Phase 3: Cache Inspection Methods
// ============================================================================

QStringList PDFCacheManager::getCacheKeys() const {
    QMutexLocker locker(&m_pimpl->cacheMutex);
    return m_pimpl->cache.keys();
}

QStringList PDFCacheManager::getCacheKeysByType(CacheItemType type) const {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    QStringList keys;
    for (auto it = m_pimpl->cache.constBegin(); it != m_pimpl->cache.constEnd();
         ++it) {
        if (it->type == type) {
            keys.append(it->key);
        }
    }
    return keys;
}

QStringList PDFCacheManager::getCacheKeysByPriority(
    CachePriority priority) const {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    QStringList keys;
    for (auto it = m_pimpl->cache.constBegin(); it != m_pimpl->cache.constEnd();
         ++it) {
        if (it->priority == priority) {
            keys.append(it->key);
        }
    }
    return keys;
}

int PDFCacheManager::getCacheItemCount(CacheItemType type) const {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    int count = 0;
    for (const auto& item : m_pimpl->cache) {
        if (item.type == type) {
            count++;
        }
    }
    return count;
}

qint64 PDFCacheManager::getCacheMemoryUsage(CacheItemType type) const {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    qint64 totalMemory = 0;
    for (const auto& item : m_pimpl->cache) {
        if (item.type == type) {
            totalMemory += item.memorySize;
        }
    }
    return totalMemory;
}

// ============================================================================
// Phase 4: Cache Management Methods
// ============================================================================

void PDFCacheManager::setCachePriority(const QString& key,
                                       CachePriority priority) {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    auto it = m_pimpl->cache.find(key);
    if (it != m_pimpl->cache.end()) {
        it->priority = priority;
        LOG_DEBUG("PDFCacheManager: Set priority for key '{}' to {}",
                  key.toStdString(), static_cast<int>(priority));
        emit cachePriorityChanged(key, priority);
    } else {
        LOG_WARNING("PDFCacheManager: Cannot set priority - key '{}' not found",
                    key.toStdString());
    }
}

bool PDFCacheManager::promoteToHighPriority(const QString& key) {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    auto it = m_pimpl->cache.find(key);
    if (it != m_pimpl->cache.end()) {
        it->priority = CachePriority::High;
        LOG_DEBUG("PDFCacheManager: Promoted key '{}' to high priority",
                  key.toStdString());
        emit cachePriorityChanged(key, CachePriority::High);
        return true;
    }

    LOG_WARNING("PDFCacheManager: Cannot promote - key '{}' not found",
                key.toStdString());
    return false;
}

void PDFCacheManager::refreshCacheItem(const QString& key) {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    auto it = m_pimpl->cache.find(key);
    if (it != m_pimpl->cache.end()) {
        it->updateAccess();
        LOG_DEBUG("PDFCacheManager: Refreshed cache item '{}'",
                  key.toStdString());
        emit cacheItemRefreshed(key);
    } else {
        LOG_WARNING("PDFCacheManager: Cannot refresh - key '{}' not found",
                    key.toStdString());
    }
}
