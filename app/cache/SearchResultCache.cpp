#include "SearchResultCache.h"
#include <QCoreApplication>
#include <QDebug>
#include <memory>

// SearchResultCache Implementation class
class SearchResultCache::Implementation {
public:
    explicit Implementation(SearchResultCache* q)
        : q_ptr(q),
          maxCacheSize(SearchResultCache::DEFAULT_MAX_CACHE_SIZE),
          maxMemoryUsage(SearchResultCache::DEFAULT_MAX_MEMORY_USAGE),
          currentMemoryUsage(0),
          expirationTime(SearchResultCache::DEFAULT_EXPIRATION_TIME),
          cacheHits(0),
          cacheMisses(0),
          enabled(true),
          maintenanceTimer(new QTimer(q)) {
        // Setup maintenance timer
        maintenanceTimer->setInterval(SearchResultCache::MAINTENANCE_INTERVAL);
        QObject::connect(maintenanceTimer, &QTimer::timeout, q,
                         &SearchResultCache::performMaintenance);
        maintenanceTimer->start();
    }

    SearchResultCache* q_ptr;

    // Cache storage
    mutable QMutex cacheMutex;
    QHash<QString, CacheEntry> cache;

    // Configuration
    int maxCacheSize;
    qint64 maxMemoryUsage;
    qint64 currentMemoryUsage;
    qint64 expirationTime;

    // Statistics
    qint64 cacheHits;
    qint64 cacheMisses;

    // Cache state
    bool enabled;

    // Maintenance
    QTimer* maintenanceTimer;

    // Private methods
    void evictLeastRecentlyUsed();
    void evictExpiredEntries();
    qint64 calculateMemorySize(const QList<SearchResult>& results) const;
    void updateAccessInfo(CacheEntry& entry);
    bool isExpired(const CacheEntry& entry) const;
};

// Implementation method definitions
void SearchResultCache::Implementation::evictLeastRecentlyUsed() {
    if (cache.isEmpty())
        return;

    QString oldestKey;
    qint64 oldestTime = QDateTime::currentMSecsSinceEpoch();

    for (auto it = cache.begin(); it != cache.end(); ++it) {
        if (it.value().timestamp < oldestTime) {
            oldestTime = it.value().timestamp;
            oldestKey = it.key();
        }
    }

    if (!oldestKey.isEmpty()) {
        auto it = cache.find(oldestKey);
        if (it != cache.end()) {
            currentMemoryUsage -= calculateMemorySize(it.value().results);
            cache.erase(it);
        }
    }
}

void SearchResultCache::Implementation::evictExpiredEntries() {
    auto it = cache.begin();
    while (it != cache.end()) {
        if (isExpired(it.value())) {
            currentMemoryUsage -= calculateMemorySize(it.value().results);
            it = cache.erase(it);
        } else {
            ++it;
        }
    }
}

qint64 SearchResultCache::Implementation::calculateMemorySize(
    const QList<SearchResult>& results) const {
    qint64 size = sizeof(CacheEntry);
    for (const auto& result : results) {
        size += sizeof(SearchResult);
        size += result.matchedText.size() * sizeof(QChar);
        size += result.contextText.size() * sizeof(QChar);
    }
    return size;
}

void SearchResultCache::Implementation::updateAccessInfo(CacheEntry& entry) {
    entry.timestamp = QDateTime::currentMSecsSinceEpoch();
    entry.accessCount++;
}

bool SearchResultCache::Implementation::isExpired(
    const CacheEntry& entry) const {
    if (expirationTime <= 0)
        return false;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    return (currentTime - entry.timestamp) > expirationTime;
}

// SearchResultCache Implementation
SearchResultCache::SearchResultCache(QObject* parent)
    : QObject(parent), d(std::make_unique<Implementation>(this)) {}

SearchResultCache::~SearchResultCache() = default;

bool SearchResultCache::hasResults(const CacheKey& key) const {
    QMutexLocker locker(&d->cacheMutex);
    QString hash = key.toHash();
    return d->cache.contains(hash) && !d->isExpired(d->cache[hash]);
}

QList<SearchResult> SearchResultCache::getResults(const CacheKey& key) {
    QMutexLocker locker(&d->cacheMutex);
    QString hash = key.toHash();

    auto it = d->cache.find(hash);
    if (it != d->cache.end() && !d->isExpired(it.value())) {
        d->updateAccessInfo(it.value());
        d->cacheHits++;
        emit cacheHit(hash);
        return it.value().results;
    }

    d->cacheMisses++;
    emit cacheMiss(hash);
    return QList<SearchResult>();
}

void SearchResultCache::storeResults(const CacheKey& key,
                                     const QList<SearchResult>& results) {
    QMutexLocker locker(&d->cacheMutex);

    // Don't store if cache is disabled
    if (!d->enabled) {
        return;
    }

    QString hash = key.toHash();
    qint64 memorySize = d->calculateMemorySize(results);

    // Check if we need to evict entries to make room
    while ((d->cache.size() >= d->maxCacheSize ||
            d->currentMemoryUsage + memorySize > d->maxMemoryUsage) &&
           !d->cache.isEmpty()) {
        d->evictLeastRecentlyUsed();
    }

    CacheEntry entry;
    entry.results = results;
    entry.timestamp = QDateTime::currentMSecsSinceEpoch();
    entry.accessCount = 1;
    entry.memorySize = memorySize;
    entry.queryHash = hash;
    entry.documentId = key.documentId;

    d->cache[hash] = entry;
    d->currentMemoryUsage += memorySize;

    emit cacheUpdated(d->cache.size(), d->currentMemoryUsage);
}

void SearchResultCache::invalidateDocument(const QString& documentId) {
    QMutexLocker locker(&d->cacheMutex);

    auto it = d->cache.begin();
    while (it != d->cache.end()) {
        // Check if this entry belongs to the document
        if (it.value().documentId == documentId) {
            d->currentMemoryUsage -= it.value().memorySize;
            it = d->cache.erase(it);
        } else {
            ++it;
        }
    }

    emit cacheUpdated(d->cache.size(), d->currentMemoryUsage);
}

void SearchResultCache::clear() {
    QMutexLocker locker(&d->cacheMutex);
    d->cache.clear();
    d->currentMemoryUsage = 0;
    emit cacheUpdated(0, 0);
}

double SearchResultCache::getHitRatio() const {
    qint64 total = d->cacheHits + d->cacheMisses;
    return total > 0 ? static_cast<double>(d->cacheHits) / total : 0.0;
}

void SearchResultCache::resetStatistics() {
    d->cacheHits = 0;
    d->cacheMisses = 0;
}

bool SearchResultCache::canUseIncrementalSearch(
    const CacheKey& newKey, const CacheKey& previousKey) const {
    // Check if new query is an extension of the previous query
    return newKey.documentId == previousKey.documentId &&
           newKey.documentModified == previousKey.documentModified &&
           newKey.options.caseSensitive == previousKey.options.caseSensitive &&
           newKey.options.wholeWords == previousKey.options.wholeWords &&
           newKey.options.useRegex == previousKey.options.useRegex &&
           !newKey.options
                .useRegex &&  // Incremental search only for simple text
           newKey.query.startsWith(previousKey.query) &&
           newKey.query.length() > previousKey.query.length();
}

QList<SearchResult> SearchResultCache::getIncrementalResults(
    const CacheKey& newKey, const CacheKey& previousKey) {
    if (!canUseIncrementalSearch(newKey, previousKey)) {
        return QList<SearchResult>();
    }

    QList<SearchResult> previousResults = getResults(previousKey);
    if (previousResults.isEmpty()) {
        return QList<SearchResult>();
    }

    // Filter previous results for the new query
    QList<SearchResult> filteredResults;
    QString additionalText = newKey.query.mid(previousKey.query.length());

    for (const SearchResult& result : previousResults) {
        // Check if the result context contains the additional text
        QString searchText = newKey.options.caseSensitive
                                 ? result.contextText
                                 : result.contextText.toLower();
        QString queryText = newKey.options.caseSensitive
                                ? newKey.query
                                : newKey.query.toLower();

        if (searchText.contains(queryText)) {
            filteredResults.append(result);
        }
    }

    // Store the filtered results for future use
    storeResults(newKey, filteredResults);

    return filteredResults;
}

void SearchResultCache::performMaintenance() {
    QMutexLocker locker(&d->cacheMutex);
    d->evictExpiredEntries();
}

// ICacheComponent interface implementation
qint64 SearchResultCache::getMaxMemoryLimit() const {
    return d->maxMemoryUsage;
}

void SearchResultCache::setMaxMemoryLimit(qint64 limit) {
    d->maxMemoryUsage = limit;
}

int SearchResultCache::getEntryCount() const {
    QMutexLocker locker(&d->cacheMutex);
    return d->cache.size();
}

void SearchResultCache::evictLRU(qint64 bytesToFree) {
    QMutexLocker locker(&d->cacheMutex);

    qint64 initialMemory = d->currentMemoryUsage;
    while (d->currentMemoryUsage > (initialMemory - bytesToFree) &&
           !d->cache.isEmpty()) {
        d->evictLeastRecentlyUsed();
    }
}

qint64 SearchResultCache::getHitCount() const { return d->cacheHits; }

qint64 SearchResultCache::getMissCount() const { return d->cacheMisses; }

void SearchResultCache::setEnabled(bool enabled) {
    d->enabled = enabled;
    if (!enabled) {
        clear();
    }
}

bool SearchResultCache::isEnabled() const { return d->enabled; }

// Getter methods that were converted from inline
void SearchResultCache::setMaxCacheSize(int maxEntries) {
    d->maxCacheSize = maxEntries;
}

void SearchResultCache::setMaxMemoryUsage(qint64 maxBytes) {
    d->maxMemoryUsage = maxBytes;
}

void SearchResultCache::setExpirationTime(qint64 milliseconds) {
    d->expirationTime = milliseconds;
}

int SearchResultCache::getCacheSize() const {
    QMutexLocker locker(&d->cacheMutex);
    return d->cache.size();
}

qint64 SearchResultCache::getMemoryUsage() const {
    return d->currentMemoryUsage;
}

// SearchHighlightCache Implementation class
class SearchHighlightCache::Implementation {
public:
    explicit Implementation(SearchHighlightCache* q)
        : q_ptr(q),
          maxCacheSize(SearchHighlightCache::DEFAULT_MAX_CACHE_SIZE),
          cacheHits(0),
          cacheMisses(0) {}

    SearchHighlightCache* q_ptr;

    // Cache storage
    mutable QMutex cacheMutex;
    QHash<QString, HighlightData> cache;

    // Configuration
    int maxCacheSize;

    // Statistics
    qint64 cacheHits;
    qint64 cacheMisses;

    // Private methods
    QString getCacheKey(const QString& documentId, int pageNumber,
                        const QString& query) const;
    void evictLeastRecentlyUsed();
    void updateAccessInfo(HighlightData& data);
};

// Implementation method definitions
QString SearchHighlightCache::Implementation::getCacheKey(
    const QString& documentId, int pageNumber, const QString& query) const {
    return QString("%1_%2_%3").arg(documentId).arg(pageNumber).arg(query);
}

void SearchHighlightCache::Implementation::updateAccessInfo(
    SearchHighlightCache::HighlightData& data) {
    data.timestamp = QDateTime::currentMSecsSinceEpoch();
    data.accessCount++;
}

void SearchHighlightCache::Implementation::evictLeastRecentlyUsed() {
    if (cache.isEmpty())
        return;

    QString oldestKey;
    qint64 oldestTime = QDateTime::currentMSecsSinceEpoch();

    for (auto it = cache.begin(); it != cache.end(); ++it) {
        if (it.value().timestamp < oldestTime) {
            oldestTime = it.value().timestamp;
            oldestKey = it.key();
        }
    }

    if (!oldestKey.isEmpty()) {
        cache.remove(oldestKey);
    }
}

// SearchHighlightCache Implementation
SearchHighlightCache::SearchHighlightCache(QObject* parent)
    : QObject(parent), d(std::make_unique<Implementation>(this)) {}

SearchHighlightCache::~SearchHighlightCache() = default;

bool SearchHighlightCache::hasHighlightData(const QString& documentId,
                                            int pageNumber,
                                            const QString& query) const {
    QMutexLocker locker(&d->cacheMutex);
    QString key = d->getCacheKey(documentId, pageNumber, query);
    return d->cache.contains(key);
}

SearchHighlightCache::HighlightData SearchHighlightCache::getHighlightData(
    const QString& documentId, int pageNumber, const QString& query) {
    QMutexLocker locker(&d->cacheMutex);
    QString key = d->getCacheKey(documentId, pageNumber, query);

    auto it = d->cache.find(key);
    if (it != d->cache.end()) {
        d->updateAccessInfo(it.value());
        d->cacheHits++;
        return it.value();
    }

    d->cacheMisses++;
    return HighlightData();
}

void SearchHighlightCache::storeHighlightData(const QString& documentId,
                                              int pageNumber,
                                              const QString& query,
                                              const HighlightData& data) {
    QMutexLocker locker(&d->cacheMutex);

    QString key = d->getCacheKey(documentId, pageNumber, query);

    // Check if we need to evict entries
    while (d->cache.size() >= d->maxCacheSize && !d->cache.isEmpty()) {
        d->evictLeastRecentlyUsed();
    }

    HighlightData entry = data;
    entry.timestamp = QDateTime::currentMSecsSinceEpoch();
    entry.accessCount = 1;

    d->cache[key] = entry;
    emit cacheUpdated(d->cache.size());
}

void SearchHighlightCache::invalidateDocument(const QString& documentId) {
    QMutexLocker locker(&d->cacheMutex);

    auto it = d->cache.begin();
    while (it != d->cache.end()) {
        if (it.key().startsWith(documentId + "_")) {
            it = d->cache.erase(it);
        } else {
            ++it;
        }
    }

    emit cacheUpdated(d->cache.size());
}

void SearchHighlightCache::clear() {
    QMutexLocker locker(&d->cacheMutex);
    d->cache.clear();
    emit cacheUpdated(0);
}

double SearchHighlightCache::getHitRatio() const {
    qint64 total = d->cacheHits + d->cacheMisses;
    return total > 0 ? static_cast<double>(d->cacheHits) / total : 0.0;
}

int SearchHighlightCache::getCacheSize() const {
    QMutexLocker locker(&d->cacheMutex);
    return d->cache.size();
}
