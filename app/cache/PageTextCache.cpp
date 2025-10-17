#include "PageTextCache.h"

#include <QDateTime>
#include <climits>

// PageTextCache Implementation class
class PageTextCache::Implementation {
public:
    explicit Implementation(PageTextCache* qParent)
        : qPtr(qParent)

    {}

    PageTextCache* qPtr;

    // Cache storage
    mutable QMutex cacheMutex;
    QHash<QString, CacheEntry> cache;

    // Configuration
    int maxCacheSize = PageTextCache::DEFAULT_MAX_CACHE_SIZE;
    qint64 maxMemoryUsage = PageTextCache::DEFAULT_MAX_MEMORY_USAGE;
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
    qint64 oldestTime = LLONG_MAX;
    int lowestAccessCount = INT_MAX;
    int lowestPageNumber = INT_MAX;

    for (auto entryIt = cache.begin(); entryIt != cache.end(); ++entryIt) {
        const auto& entry = entryIt.value();

        // Find entry with oldest timestamp, using access count and page number
        // as tiebreakers for deterministic eviction
        bool isOlder = entry.timestamp < oldestTime;
        bool isSameTimeButLessAccessed = entry.timestamp == oldestTime &&
                                         entry.accessCount < lowestAccessCount;
        bool isSameTimeAndAccessButLowerPage =
            entry.timestamp == oldestTime &&
            entry.accessCount == lowestAccessCount &&
            entry.pageNumber < lowestPageNumber;

        if (isOlder || isSameTimeButLessAccessed ||
            isSameTimeAndAccessButLowerPage) {
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
    : QObject(parent),
      m_implementation(std::make_unique<Implementation>(this)) {}

PageTextCache::~PageTextCache() = default;

bool PageTextCache::hasPageText(const QString& documentId,
                                int pageNumber) const {
    if (!m_implementation->enabled) {
        return false;
    }

    QMutexLocker locker(&m_implementation->cacheMutex);
    QString key = Implementation::getCacheKey(documentId, pageNumber);
    return m_implementation->cache.contains(key);
}

QString PageTextCache::getPageText(const QString& documentId, int pageNumber) {
    if (!m_implementation->enabled) {
        return {};
    }

    QMutexLocker locker(&m_implementation->cacheMutex);
    QString key = Implementation::getCacheKey(documentId, pageNumber);

    auto entryIt = m_implementation->cache.find(key);
    if (entryIt != m_implementation->cache.end()) {
        Implementation::updateAccessInfo(entryIt.value());
        m_implementation->cacheHits++;
        emit cacheHit(documentId, pageNumber);
        return entryIt.value().text;
    }

    m_implementation->cacheMisses++;
    emit cacheMiss(documentId, pageNumber);
    return {};
}

void PageTextCache::storePageText(const QString& documentId, int pageNumber,
                                  const QString& text) {
    if (!m_implementation->enabled || text.isEmpty()) {
        return;
    }

    QMutexLocker locker(&m_implementation->cacheMutex);

    QString key = Implementation::getCacheKey(documentId, pageNumber);
    qint64 textSize = Implementation::calculateTextMemorySize(text);

    // Check if we need to evict entries
    while ((m_implementation->cache.size() >= m_implementation->maxCacheSize ||
            m_implementation->currentMemoryUsage + textSize >
                m_implementation->maxMemoryUsage) &&
           !m_implementation->cache.isEmpty()) {
        m_implementation->evictLeastRecentlyUsed();
    }

    CacheEntry entry;
    entry.text = text;
    entry.documentId = documentId;
    entry.pageNumber = pageNumber;
    entry.timestamp = QDateTime::currentMSecsSinceEpoch();
    entry.accessCount = 1;
    entry.memorySize = textSize;

    m_implementation->cache[key] = entry;
    m_implementation->currentMemoryUsage += textSize;

    emit cacheUpdated(static_cast<int>(m_implementation->cache.size()),
                      m_implementation->currentMemoryUsage);
}

void PageTextCache::invalidateDocument(const QString& documentId) {
    QMutexLocker locker(&m_implementation->cacheMutex);

    auto entryIt = m_implementation->cache.begin();
    while (entryIt != m_implementation->cache.end()) {
        if (entryIt.value().documentId == documentId) {
            m_implementation->currentMemoryUsage -= entryIt.value().memorySize;
            entryIt = m_implementation->cache.erase(entryIt);
        } else {
            ++entryIt;
        }
    }

    emit cacheUpdated(static_cast<int>(m_implementation->cache.size()),
                      m_implementation->currentMemoryUsage);
}

void PageTextCache::clear() {
    QMutexLocker locker(&m_implementation->cacheMutex);
    m_implementation->cache.clear();
    m_implementation->currentMemoryUsage = 0;
    emit cacheUpdated(0, 0);
}

double PageTextCache::getHitRatio() const {
    qint64 total = m_implementation->cacheHits + m_implementation->cacheMisses;
    return total > 0 ? static_cast<double>(m_implementation->cacheHits) /
                           static_cast<double>(total)
                     : 0.0;
}

// ICacheComponent interface implementation
qint64 PageTextCache::getMemoryUsage() const {
    return m_implementation->currentMemoryUsage;
}

qint64 PageTextCache::getMaxMemoryLimit() const {
    return m_implementation->maxMemoryUsage;
}

void PageTextCache::setMaxMemoryLimit(qint64 limit) {
    m_implementation->maxMemoryUsage = limit;
}

int PageTextCache::getEntryCount() const {
    QMutexLocker locker(&m_implementation->cacheMutex);
    return static_cast<int>(m_implementation->cache.size());
}

void PageTextCache::evictLRU(qint64 bytesToFree) {
    QMutexLocker locker(&m_implementation->cacheMutex);

    qint64 freedBytes = 0;
    while (freedBytes < bytesToFree && !m_implementation->cache.isEmpty()) {
        qint64 sizeBefore = m_implementation->currentMemoryUsage;
        m_implementation->evictLeastRecentlyUsed();
        freedBytes += (sizeBefore - m_implementation->currentMemoryUsage);
    }
}

qint64 PageTextCache::getHitCount() const {
    return m_implementation->cacheHits;
}

qint64 PageTextCache::getMissCount() const {
    return m_implementation->cacheMisses;
}

void PageTextCache::resetStatistics() {
    m_implementation->cacheHits = 0;
    m_implementation->cacheMisses = 0;
}

void PageTextCache::setEnabled(bool enabled) {
    m_implementation->enabled = enabled;
    if (!enabled) {
        clear();
    }
}

bool PageTextCache::isEnabled() const { return m_implementation->enabled; }

// Getter methods that were converted from inline
void PageTextCache::setMaxCacheSize(int maxEntries) {
    m_implementation->maxCacheSize = maxEntries;
}

void PageTextCache::setMaxMemoryUsage(qint64 maxBytes) {
    m_implementation->maxMemoryUsage = maxBytes;
}

int PageTextCache::getMaxCacheSize() const {
    return m_implementation->maxCacheSize;
}

int PageTextCache::getCacheSize() const {
    QMutexLocker locker(&m_implementation->cacheMutex);
    return static_cast<int>(m_implementation->cache.size());
}

// TextExtractorCacheAdapter Implementation class
#include "../search/TextExtractor.h"

class TextExtractorCacheAdapter::Implementation {
public:
    explicit Implementation(TextExtractor* extractor)
        : textExtractor(extractor) {}

    TextExtractor* textExtractor;
    qint64 maxMemoryLimit = 100LL * 1024 * 1024;
};

TextExtractorCacheAdapter::TextExtractorCacheAdapter(
    TextExtractor* textExtractor, QObject* parent)
    : QObject(parent),
      m_implementation(std::make_unique<Implementation>(textExtractor)) {}

TextExtractorCacheAdapter::~TextExtractorCacheAdapter() = default;

qint64 TextExtractorCacheAdapter::getMemoryUsage() const {
    return m_implementation->textExtractor != nullptr
               ? m_implementation->textExtractor->cacheMemoryUsage()
               : 0;
}

qint64 TextExtractorCacheAdapter::getMaxMemoryLimit() const {
    return m_implementation->maxMemoryLimit;
}

void TextExtractorCacheAdapter::setMaxMemoryLimit(qint64 limit) {
    m_implementation->maxMemoryLimit = limit;
}

void TextExtractorCacheAdapter::clear() {
    if (m_implementation->textExtractor != nullptr) {
        m_implementation->textExtractor->clearCache();
    }
}

int TextExtractorCacheAdapter::getEntryCount() const {
    // TextExtractor doesn't expose entry count, estimate from memory usage
    qint64 memUsage = getMemoryUsage();
    if (memUsage == 0) {
        return 0;
    }
    // Rough estimate: average 1KB per entry
    return static_cast<int>(memUsage / 1024);
}

void TextExtractorCacheAdapter::evictLRU(qint64 bytesToFree) {
    // TextExtractor doesn't support selective eviction
    // Clear entire cache if needed
    if (bytesToFree > 0 && m_implementation->textExtractor != nullptr) {
        qint64 currentUsage = getMemoryUsage();
        if (currentUsage > bytesToFree) {
            m_implementation->textExtractor->clearCache();
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
    if (m_implementation->textExtractor != nullptr) {
        m_implementation->textExtractor->setCacheEnabled(enabled);
    }
}

bool TextExtractorCacheAdapter::isEnabled() const {
    return m_implementation->textExtractor != nullptr
               ? m_implementation->textExtractor->isCacheEnabled()
               : false;
}
