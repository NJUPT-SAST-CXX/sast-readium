#include "PageTextCache.h"
#include <QDateTime>
#include <climits>
#include <memory>

// PageTextCache Implementation class
class PageTextCache::Implementation {
public:
    explicit Implementation(PageTextCache* qParent)
        : qPtr(qParent),
          maxCacheSize(PageTextCache::DEFAULT_MAX_CACHE_SIZE),
          maxMemoryUsage(PageTextCache::DEFAULT_MAX_MEMORY_USAGE) {}

    PageTextCache* qPtr;

    // Cache storage
    mutable QMutex cacheMutex;
    QHash<QString, CacheEntry> cache;

    // Configuration
    int maxCacheSize;
    qint64 maxMemoryUsage;
    qint64 currentMemoryUsage = 0;
    bool enabled = true;

    // Statistics
    qint64 cacheHits = 0;
    qint64 cacheMisses = 0;

    // Private methods
    static QString getCacheKey(const QString& documentId, int pageNumber);
    void evictLeastRecentlyUsed();
    static qint64 calculateTextMemorySize(const QString& text);
    static void updateAccessInfo(CacheEntry& entry);
};

// Implementation method definitions
QString PageTextCache::Implementation::getCacheKey(const QString& documentId,
                                                   int pageNumber) {
    return QString("%1_%2").arg(documentId).arg(pageNumber);
}

qint64 PageTextCache::Implementation::calculateTextMemorySize(
    const QString& text) {
    qsizetype textSize = text.size();
    return (static_cast<qint64>(textSize) *
            static_cast<qint64>(sizeof(QChar))) +
           static_cast<qint64>(sizeof(CacheEntry));
}

void PageTextCache::Implementation::updateAccessInfo(CacheEntry& entry) {
    entry.timestamp = QDateTime::currentMSecsSinceEpoch();
    entry.accessCount++;
}

void PageTextCache::Implementation::evictLeastRecentlyUsed() {
    if (cache.isEmpty()) {
        return;
    }

    QString oldestKey;
    qint64 oldestTime = LLONG_MAX;    // Initialize to maximum value to find the
                                      // actual oldest entry
    int lowestAccessCount = INT_MAX;  // Use access count as first tiebreaker
    int lowestPageNumber =
        INT_MAX;  // Use page number as second tiebreaker for stable ordering

    for (auto entryIt = cache.begin(); entryIt != cache.end(); ++entryIt) {
        const auto& entry = entryIt.value();
        // Find entry with oldest timestamp, using access count and page number
        // as tiebreakers This ensures deterministic eviction when timestamps
        // are identical
        bool shouldEvict = false;
        if (entry.timestamp < oldestTime) {
            shouldEvict = true;
        } else if (entry.timestamp == oldestTime) {
            if (entry.accessCount < lowestAccessCount) {
                shouldEvict = true;
            } else if (entry.accessCount == lowestAccessCount &&
                       entry.pageNumber < lowestPageNumber) {
                shouldEvict = true;
            }
        }

        if (shouldEvict) {
            oldestTime = entry.timestamp;
            lowestAccessCount = entry.accessCount;
            lowestPageNumber = entry.pageNumber;
            oldestKey = entryIt.key();
        }
    }

    if (!oldestKey.isEmpty()) {
        auto entryIt = cache.find(oldestKey);
        if (entryIt != cache.end()) {
            currentMemoryUsage -=
                Implementation::calculateTextMemorySize(entryIt.value().text);
            cache.erase(entryIt);
        }
    }
}

PageTextCache::PageTextCache(QObject* parent)
    : QObject(parent), d(std::make_unique<Implementation>(this)) {}

PageTextCache::~PageTextCache() = default;

bool PageTextCache::hasPageText(const QString& documentId,
                                int pageNumber) const {
    if (!d->enabled) {
        return false;
    }

    QMutexLocker locker(&d->cacheMutex);
    QString key = Implementation::getCacheKey(documentId, pageNumber);
    return d->cache.contains(key);
}

QString PageTextCache::getPageText(const QString& documentId, int pageNumber) {
    if (!d->enabled) {
        return {};
    }

    QMutexLocker locker(&d->cacheMutex);
    QString key = Implementation::getCacheKey(documentId, pageNumber);

    auto entryIt = d->cache.find(key);
    if (entryIt != d->cache.end()) {
        Implementation::updateAccessInfo(entryIt.value());
        d->cacheHits++;
        emit cacheHit(documentId, pageNumber);
        return entryIt.value().text;
    }

    d->cacheMisses++;
    emit cacheMiss(documentId, pageNumber);
    return {};
}

void PageTextCache::storePageText(const QString& documentId, int pageNumber,
                                  const QString& text) {
    if (!d->enabled || text.isEmpty()) {
        return;
    }

    QMutexLocker locker(&d->cacheMutex);

    QString key = Implementation::getCacheKey(documentId, pageNumber);
    qint64 textSize = Implementation::calculateTextMemorySize(text);

    // Check if we need to evict entries
    while ((d->cache.size() >= d->maxCacheSize ||
            d->currentMemoryUsage + textSize > d->maxMemoryUsage) &&
           !d->cache.isEmpty()) {
        d->evictLeastRecentlyUsed();
    }

    CacheEntry entry;
    entry.text = text;
    entry.documentId = documentId;
    entry.pageNumber = pageNumber;
    entry.timestamp = QDateTime::currentMSecsSinceEpoch();
    entry.accessCount = 1;
    entry.memorySize = textSize;

    d->cache[key] = entry;
    d->currentMemoryUsage += textSize;

    emit cacheUpdated(static_cast<int>(d->cache.size()), d->currentMemoryUsage);
}

void PageTextCache::invalidateDocument(const QString& documentId) {
    QMutexLocker locker(&d->cacheMutex);

    auto entryIt = d->cache.begin();
    while (entryIt != d->cache.end()) {
        if (entryIt.value().documentId == documentId) {
            d->currentMemoryUsage -= entryIt.value().memorySize;
            entryIt = d->cache.erase(entryIt);
        } else {
            ++entryIt;
        }
    }

    emit cacheUpdated(static_cast<int>(d->cache.size()), d->currentMemoryUsage);
}

void PageTextCache::clear() {
    QMutexLocker locker(&d->cacheMutex);
    d->cache.clear();
    d->currentMemoryUsage = 0;
    emit cacheUpdated(0, 0);
}

double PageTextCache::getHitRatio() const {
    qint64 total = d->cacheHits + d->cacheMisses;
    return total > 0
               ? static_cast<double>(d->cacheHits) / static_cast<double>(total)
               : 0.0;
}

// ICacheComponent interface implementation
qint64 PageTextCache::getMemoryUsage() const { return d->currentMemoryUsage; }

qint64 PageTextCache::getMaxMemoryLimit() const { return d->maxMemoryUsage; }

void PageTextCache::setMaxMemoryLimit(qint64 limit) {
    d->maxMemoryUsage = limit;
}

int PageTextCache::getEntryCount() const {
    QMutexLocker locker(&d->cacheMutex);
    return static_cast<int>(d->cache.size());
}

void PageTextCache::evictLRU(qint64 bytesToFree) {
    QMutexLocker locker(&d->cacheMutex);

    qint64 freedBytes = 0;
    while (freedBytes < bytesToFree && !d->cache.isEmpty()) {
        qint64 sizeBefore = d->currentMemoryUsage;
        d->evictLeastRecentlyUsed();
        freedBytes += (sizeBefore - d->currentMemoryUsage);
    }
}

qint64 PageTextCache::getHitCount() const { return d->cacheHits; }

qint64 PageTextCache::getMissCount() const { return d->cacheMisses; }

void PageTextCache::resetStatistics() {
    d->cacheHits = 0;
    d->cacheMisses = 0;
}

void PageTextCache::setEnabled(bool enabled) {
    d->enabled = enabled;
    if (!enabled) {
        clear();
    }
}

bool PageTextCache::isEnabled() const { return d->enabled; }

// Getter methods that were converted from inline
void PageTextCache::setMaxCacheSize(int maxEntries) {
    d->maxCacheSize = maxEntries;
}

void PageTextCache::setMaxMemoryUsage(qint64 maxBytes) {
    d->maxMemoryUsage = maxBytes;
}

int PageTextCache::getMaxCacheSize() const { return d->maxCacheSize; }

int PageTextCache::getCacheSize() const {
    QMutexLocker locker(&d->cacheMutex);
    return static_cast<int>(d->cache.size());
}

// TextExtractorCacheAdapter Implementation class
#include "../search/TextExtractor.h"

class TextExtractorCacheAdapter::Implementation {
public:
    explicit Implementation(TextExtractor* extractor)
        : textExtractor(extractor),
          maxMemoryLimit(100 * 1024 * 1024)  // 100MB default
    {}

    TextExtractor* textExtractor;
    qint64 maxMemoryLimit;
};

TextExtractorCacheAdapter::TextExtractorCacheAdapter(
    TextExtractor* textExtractor, QObject* parent)
    : QObject(parent), d(std::make_unique<Implementation>(textExtractor)) {}

TextExtractorCacheAdapter::~TextExtractorCacheAdapter() = default;

qint64 TextExtractorCacheAdapter::getMemoryUsage() const {
    return d->textExtractor ? d->textExtractor->cacheMemoryUsage() : 0;
}

qint64 TextExtractorCacheAdapter::getMaxMemoryLimit() const {
    return d->maxMemoryLimit;
}

void TextExtractorCacheAdapter::setMaxMemoryLimit(qint64 limit) {
    d->maxMemoryLimit = limit;
}

void TextExtractorCacheAdapter::clear() {
    if (d->textExtractor) {
        d->textExtractor->clearCache();
    }
}

int TextExtractorCacheAdapter::getEntryCount() const {
    // TextExtractor doesn't expose entry count, estimate from memory usage
    qint64 memUsage = getMemoryUsage();
    if (memUsage == 0)
        return 0;
    // Rough estimate: average 1KB per entry
    return static_cast<int>(memUsage / 1024);
}

void TextExtractorCacheAdapter::evictLRU(qint64 bytesToFree) {
    // TextExtractor doesn't support selective eviction
    // Clear entire cache if needed
    if (bytesToFree > 0 && d->textExtractor) {
        qint64 currentUsage = getMemoryUsage();
        if (currentUsage > bytesToFree) {
            d->textExtractor->clearCache();
        }
    }
}

qint64 TextExtractorCacheAdapter::getHitCount() const {
    // TextExtractor doesn't track hits/misses
    return 0;
}

qint64 TextExtractorCacheAdapter::getMissCount() const {
    // TextExtractor doesn't track hits/misses
    return 0;
}

void TextExtractorCacheAdapter::resetStatistics() {
    // TextExtractor doesn't have statistics to reset
}

void TextExtractorCacheAdapter::setEnabled(bool enabled) {
    if (d->textExtractor) {
        d->textExtractor->setCacheEnabled(enabled);
    }
}

bool TextExtractorCacheAdapter::isEnabled() const {
    return d->textExtractor ? d->textExtractor->isCacheEnabled() : false;
}
