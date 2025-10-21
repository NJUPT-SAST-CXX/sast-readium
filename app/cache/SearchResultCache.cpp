#include "SearchResultCache.h"
#include <QCoreApplication>
#include <QDebug>
#include "../logging/LoggingMacros.h"

// SearchResultCache Implementation class
class SearchResultCache::Implementation {
public:
    explicit Implementation(SearchResultCache* parent)
        : q_ptr(parent),
          maxCacheSize(SearchResultCache::DEFAULT_MAX_CACHE_SIZE),
          maxMemoryUsage(SearchResultCache::DEFAULT_MAX_MEMORY_USAGE),
          currentMemoryUsage(0),
          expirationTime(SearchResultCache::DEFAULT_EXPIRATION_TIME),
          cacheHits(0),
          cacheMisses(0),
          enabled(true),
          maintenanceTimer(new QTimer(parent)) {
        // Setup maintenance timer
        maintenanceTimer->setInterval(SearchResultCache::MAINTENANCE_INTERVAL);
        QObject::connect(maintenanceTimer, &QTimer::timeout, parent,
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
    static void updateAccessInfo(CacheEntry& entry);
    bool isExpired(const CacheEntry& entry) const;
};

// Implementation method definitions
void SearchResultCache::Implementation::evictLeastRecentlyUsed() {
    if (cache.isEmpty()) {
        return;
    }

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
    if (expirationTime <= 0) {
        return false;
    }
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    return (currentTime - entry.timestamp) > expirationTime;
}

// SearchResultCache Implementation
SearchResultCache::SearchResultCache(QObject* parent)
    : QObject(parent), m_pimpl(std::make_unique<Implementation>(this)) {}

SearchResultCache::~SearchResultCache() = default;

bool SearchResultCache::hasResults(const CacheKey& key) const {
    QMutexLocker locker(&m_pimpl->cacheMutex);
    QString hash = key.toHash();
    return m_pimpl->cache.contains(hash) &&
           !m_pimpl->isExpired(m_pimpl->cache[hash]);
}

QList<SearchResult> SearchResultCache::getResults(const CacheKey& key) {
    QMutexLocker locker(&m_pimpl->cacheMutex);
    QString hash = key.toHash();

    auto it = m_pimpl->cache.find(hash);
    if (it != m_pimpl->cache.end() && !m_pimpl->isExpired(it.value())) {
        SearchResultCache::Implementation::updateAccessInfo(it.value());
        m_pimpl->cacheHits++;
        emit cacheHit(hash);
        return it.value().results;
    }

    m_pimpl->cacheMisses++;
    emit cacheMiss(hash);
    return QList<SearchResult>();
}

void SearchResultCache::storeResults(const CacheKey& key,
                                     const QList<SearchResult>& results) {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    // Don't store if cache is disabled
    if (!m_pimpl->enabled) {
        return;
    }

    QString hash = key.toHash();
    qint64 memorySize = m_pimpl->calculateMemorySize(results);

    // Check if we need to evict entries to make room
    while (
        (m_pimpl->cache.size() >= m_pimpl->maxCacheSize ||
         m_pimpl->currentMemoryUsage + memorySize > m_pimpl->maxMemoryUsage) &&
        !m_pimpl->cache.isEmpty()) {
        m_pimpl->evictLeastRecentlyUsed();
    }

    CacheEntry entry;
    entry.results = results;
    entry.timestamp = QDateTime::currentMSecsSinceEpoch();
    entry.accessCount = 1;
    entry.memorySize = memorySize;
    entry.queryHash = hash;
    entry.documentId = key.documentId;

    m_pimpl->cache[hash] = entry;
    m_pimpl->currentMemoryUsage += memorySize;

    emit cacheUpdated(m_pimpl->cache.size(), m_pimpl->currentMemoryUsage);
}

void SearchResultCache::invalidateDocument(const QString& documentId) {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    auto it = m_pimpl->cache.begin();
    while (it != m_pimpl->cache.end()) {
        // Check if this entry belongs to the document
        if (it.value().documentId == documentId) {
            m_pimpl->currentMemoryUsage -= it.value().memorySize;
            it = m_pimpl->cache.erase(it);
        } else {
            ++it;
        }
    }

    emit cacheUpdated(m_pimpl->cache.size(), m_pimpl->currentMemoryUsage);
}

void SearchResultCache::clear() {
    QMutexLocker locker(&m_pimpl->cacheMutex);
    m_pimpl->cache.clear();
    m_pimpl->currentMemoryUsage = 0;
    emit cacheUpdated(0, 0);
}

double SearchResultCache::getHitRatio() const {
    qint64 total = m_pimpl->cacheHits + m_pimpl->cacheMisses;
    return total > 0 ? static_cast<double>(m_pimpl->cacheHits) / total : 0.0;
}

void SearchResultCache::resetStatistics() {
    m_pimpl->cacheHits = 0;
    m_pimpl->cacheMisses = 0;
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
    QMutexLocker locker(&m_pimpl->cacheMutex);
    m_pimpl->evictExpiredEntries();
}

// ICacheComponent interface implementation
qint64 SearchResultCache::getMaxMemoryLimit() const {
    return m_pimpl->maxMemoryUsage;
}

void SearchResultCache::setMaxMemoryLimit(qint64 limit) {
    m_pimpl->maxMemoryUsage = limit;
}

int SearchResultCache::getEntryCount() const {
    QMutexLocker locker(&m_pimpl->cacheMutex);
    return m_pimpl->cache.size();
}

void SearchResultCache::evictLRU(qint64 bytesToFree) {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    qint64 initialMemory = m_pimpl->currentMemoryUsage;
    while (m_pimpl->currentMemoryUsage > (initialMemory - bytesToFree) &&
           !m_pimpl->cache.isEmpty()) {
        m_pimpl->evictLeastRecentlyUsed();
    }
}

qint64 SearchResultCache::getHitCount() const { return m_pimpl->cacheHits; }

qint64 SearchResultCache::getMissCount() const { return m_pimpl->cacheMisses; }

void SearchResultCache::setEnabled(bool enabled) {
    m_pimpl->enabled = enabled;
    if (!enabled) {
        clear();
    }
}

bool SearchResultCache::isEnabled() const { return m_pimpl->enabled; }

// Getter methods that were converted from inline
void SearchResultCache::setMaxCacheSize(int maxEntries) {
    m_pimpl->maxCacheSize = maxEntries;
}

void SearchResultCache::setMaxMemoryUsage(qint64 maxBytes) {
    m_pimpl->maxMemoryUsage = maxBytes;
}

void SearchResultCache::setExpirationTime(qint64 milliseconds) {
    m_pimpl->expirationTime = milliseconds;
}

int SearchResultCache::getCacheSize() const {
    QMutexLocker locker(&m_pimpl->cacheMutex);
    return m_pimpl->cache.size();
}

qint64 SearchResultCache::getMemoryUsage() const {
    return m_pimpl->currentMemoryUsage;
}

// SearchHighlightCache Implementation class
class SearchHighlightCache::Implementation {
public:
    explicit Implementation(SearchHighlightCache* parent)
        : q_ptr(parent),
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
    static void updateAccessInfo(HighlightData& data);
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
    if (cache.isEmpty()) {
        return;
    }

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
    : QObject(parent), m_pimpl(std::make_unique<Implementation>(this)) {}

SearchHighlightCache::~SearchHighlightCache() = default;

bool SearchHighlightCache::hasHighlightData(const QString& documentId,
                                            int pageNumber,
                                            const QString& query) const {
    QMutexLocker locker(&m_pimpl->cacheMutex);
    QString key = m_pimpl->getCacheKey(documentId, pageNumber, query);
    return m_pimpl->cache.contains(key);
}

SearchHighlightCache::HighlightData SearchHighlightCache::getHighlightData(
    const QString& documentId, int pageNumber, const QString& query) {
    QMutexLocker locker(&m_pimpl->cacheMutex);
    QString key = m_pimpl->getCacheKey(documentId, pageNumber, query);

    auto it = m_pimpl->cache.find(key);
    if (it != m_pimpl->cache.end()) {
        SearchHighlightCache::Implementation::updateAccessInfo(it.value());
        m_pimpl->cacheHits++;
        return it.value();
    }

    m_pimpl->cacheMisses++;
    return HighlightData();
}

void SearchHighlightCache::storeHighlightData(const QString& documentId,
                                              int pageNumber,
                                              const QString& query,
                                              const HighlightData& data) {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    QString key = m_pimpl->getCacheKey(documentId, pageNumber, query);

    // Check if we need to evict entries
    while (m_pimpl->cache.size() >= m_pimpl->maxCacheSize &&
           !m_pimpl->cache.isEmpty()) {
        m_pimpl->evictLeastRecentlyUsed();
    }

    HighlightData entry = data;
    entry.timestamp = QDateTime::currentMSecsSinceEpoch();
    entry.accessCount = 1;

    m_pimpl->cache[key] = entry;
    emit cacheUpdated(m_pimpl->cache.size());
}

void SearchHighlightCache::invalidateDocument(const QString& documentId) {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    auto it = m_pimpl->cache.begin();
    while (it != m_pimpl->cache.end()) {
        if (it.key().startsWith(documentId + "_")) {
            it = m_pimpl->cache.erase(it);
        } else {
            ++it;
        }
    }

    emit cacheUpdated(m_pimpl->cache.size());
}

void SearchHighlightCache::clear() {
    QMutexLocker locker(&m_pimpl->cacheMutex);
    m_pimpl->cache.clear();
    emit cacheUpdated(0);
}

double SearchHighlightCache::getHitRatio() const {
    qint64 total = m_pimpl->cacheHits + m_pimpl->cacheMisses;
    return total > 0 ? static_cast<double>(m_pimpl->cacheHits) / total : 0.0;
}

int SearchHighlightCache::getCacheSize() const {
    QMutexLocker locker(&m_pimpl->cacheMutex);
    return m_pimpl->cache.size();
}

void SearchHighlightCache::setMaxCacheSize(int maxEntries) {
    QMutexLocker locker(&m_pimpl->cacheMutex);

    if (maxEntries <= 0) {
        LOG_WARNING(
            "SearchHighlightCache: Invalid max cache size {}, using default",
            maxEntries);
        m_pimpl->maxCacheSize = DEFAULT_MAX_CACHE_SIZE;
        return;
    }

    m_pimpl->maxCacheSize = maxEntries;
    LOG_INFO("SearchHighlightCache: Max cache size set to {}", maxEntries);

    // Evict items if current size exceeds new limit
    while (m_pimpl->cache.size() > m_pimpl->maxCacheSize) {
        m_pimpl->evictLeastRecentlyUsed();
    }
}
