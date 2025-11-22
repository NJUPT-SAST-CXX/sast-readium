#include "MemoryManager.h"
#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QMutexLocker>
#include <algorithm>
#include "SearchConfiguration.h"
#include "SearchEngine.h"
#include "TextExtractor.h"
#include "logging/SimpleLogging.h"

class MemoryManager::Implementation {
public:
    explicit Implementation(MemoryManager* q)
        : q_ptr(q),
          optimizationTimer(new QTimer(q)),
          statsUpdateTimer(new QTimer(q)) {
        // Setup timers
        optimizationTimer->setSingleShot(false);
        statsUpdateTimer->setSingleShot(false);

        QObject::connect(optimizationTimer, &QTimer::timeout, q,
                         &MemoryManager::performPeriodicOptimization);
        QObject::connect(statsUpdateTimer, &QTimer::timeout, q,
                         &MemoryManager::updateMemoryStats);

        // Connect to cache manager signals
        CacheManager& cacheManager = CacheManager::instance();
        QObject::connect(&cacheManager, &CacheManager::memoryPressureDetected,
                         q, &MemoryManager::onMemoryPressureDetected);
        QObject::connect(&cacheManager,
                         &CacheManager::systemMemoryPressureDetected, q,
                         &MemoryManager::onSystemMemoryPressure);
        QObject::connect(&cacheManager, &CacheManager::memoryLimitExceeded, q,
                         &MemoryManager::onCacheMemoryExceeded);

        // Start timers
        optimizationTimer->start(optimizationInterval * 1000);
        statsUpdateTimer->start(5000);  // Update stats every 5 seconds
    }

    MemoryManager* q_ptr;
    OptimizationLevel optimizationLevel{Balanced};
    bool autoOptimizationEnabled{true};
    int optimizationInterval{30};  // 30 seconds
    bool predictiveOptimizationEnabled{true};

    // Timers
    QTimer* optimizationTimer;
    QTimer* statsUpdateTimer;

    // Registered components
    QList<SearchEngine*> searchEngines;
    QList<TextExtractor*> textExtractors;

    // Statistics
    MemoryStats currentStats;
    QHash<QString, qint64> memoryUsageHistory;
    QHash<QString, int> accessPatterns;

    // Thread safety
    mutable QMutex mutex;

    void updateOptimizationInterval() const {
        optimizationTimer->setInterval(optimizationInterval * 1000);
    }

    static MemoryPressureLevel calculatePressureLevel(double pressure) {
        if (pressure < 0.70) {
            return Normal;
        }
        if (pressure < 0.85) {
            return Warning;
        }
        return Critical;
    }

    qint64 calculateTotalCacheMemory() const {
        CacheManager& cacheManager = CacheManager::instance();
        return cacheManager.getTotalMemoryUsage();
    }

    void performOptimizationByLevel(OptimizationLevel level) const {
        CacheManager& cacheManager = CacheManager::instance();

        auto requestFractionalEviction = [&](CacheType type, double fraction) {
            auto stats = cacheManager.getCacheStats(type);
            auto bytesToFree = static_cast<qint64>(
                static_cast<double>(stats.memoryUsage) * fraction);
            if (bytesToFree > 0) {
                cacheManager.requestCacheEviction(type, bytesToFree);
            }
        };

        switch (level) {
            case Conservative:
                // Only optimize if memory pressure is critical
                if (currentStats.pressureLevel == Critical) {
                    cacheManager.enforceMemoryLimits();
                    cacheManager.handleMemoryPressure();
                }
                break;

            case Balanced:
                // Optimize if memory pressure is warning or critical
                if (currentStats.pressureLevel >= Warning) {
                    cacheManager.enforceMemoryLimits();
                    cacheManager.analyzeUsagePatterns();
                    cacheManager.optimizeCacheDistribution();
                    if (currentStats.pressureLevel == Critical) {
                        requestFractionalEviction(CacheType::SearchResultCache,
                                                  0.15);
                        requestFractionalEviction(CacheType::PageTextCache,
                                                  0.15);
                    }
                }
                break;

            case Aggressive:
                // Always optimize
                cacheManager.analyzeUsagePatterns();
                cacheManager.optimizeCacheDistribution();
                cacheManager.handleMemoryPressure();
                requestFractionalEviction(CacheType::SearchResultCache, 0.25);
                requestFractionalEviction(CacheType::PageTextCache, 0.25);
                requestFractionalEviction(CacheType::SearchHighlightCache,
                                          0.25);
                if (cacheManager.isMemoryCompressionEnabled()) {
                    cacheManager.compressInactiveCaches();
                }
                break;
        }
    }
};

MemoryManager::MemoryManager(QObject* parent)
    : QObject(parent), d(std::make_unique<Implementation>(this)) {}

MemoryManager::~MemoryManager() {
    // If QCoreApplication is already torn down, avoid interacting with globals
    if (QCoreApplication::instance() == nullptr) {
        return;
    }
    if (d) {
        if (d->optimizationTimer) {
            d->optimizationTimer->stop();
            d->optimizationTimer->disconnect(this);
        }
        if (d->statsUpdateTimer) {
            d->statsUpdateTimer->stop();
            d->statsUpdateTimer->disconnect(this);
        }
        // Disconnect from CacheManager signals to prevent callbacks during
        // teardown
        QObject::disconnect(&CacheManager::instance(), nullptr, this, nullptr);
    }
}

MemoryAwareSearchResults::~MemoryAwareSearchResults() = default;

SmartEvictionPolicy::~SmartEvictionPolicy() = default;

void MemoryManager::setOptimizationLevel(OptimizationLevel level) {
    QMutexLocker locker(&d->mutex);
    if (d->optimizationLevel != level) {
        d->optimizationLevel = level;

        // Adjust optimization interval based on level
        switch (level) {
            case Conservative:
                d->optimizationInterval = 60;  // 1 minute
                break;
            case Balanced:
                d->optimizationInterval = 30;  // 30 seconds
                break;
            case Aggressive:
                d->optimizationInterval = 15;  // 15 seconds
                break;
        }

        d->updateOptimizationInterval();
    }
}

MemoryManager::OptimizationLevel MemoryManager::getOptimizationLevel() const {
    QMutexLocker locker(&d->mutex);
    return d->optimizationLevel;
}

void MemoryManager::setAutoOptimizationEnabled(bool enabled) {
    QMutexLocker locker(&d->mutex);
    d->autoOptimizationEnabled = enabled;

    if (enabled) {
        d->optimizationTimer->start();
    } else {
        d->optimizationTimer->stop();
    }
}

bool MemoryManager::isAutoOptimizationEnabled() const {
    QMutexLocker locker(&d->mutex);
    return d->autoOptimizationEnabled;
}

void MemoryManager::setOptimizationInterval(int seconds) {
    QMutexLocker locker(&d->mutex);
    d->optimizationInterval = qMax(5, seconds);  // Minimum 5 seconds
    d->updateOptimizationInterval();
}

int MemoryManager::getOptimizationInterval() const {
    QMutexLocker locker(&d->mutex);
    return d->optimizationInterval;
}

MemoryManager::MemoryStats MemoryManager::getMemoryStats() const {
    QMutexLocker locker(&d->mutex);
    return d->currentStats;
}

MemoryManager::MemoryPressureLevel MemoryManager::getCurrentPressureLevel()
    const {
    QMutexLocker locker(&d->mutex);
    return d->currentStats.pressureLevel;
}

qint64 MemoryManager::getAvailableMemory() const {
    qint64 totalSystem = CacheManager::getSystemMemoryTotal();
    qint64 currentUsage = CacheManager::getSystemMemoryUsage();

    return totalSystem - currentUsage;
}

double MemoryManager::getMemoryEfficiency() const {
    CacheManager& cacheManager = CacheManager::instance();
    CacheStats stats = cacheManager.getCacheStats(CacheType::SearchResultCache);

    qint64 totalHits = stats.totalHits;
    qint64 totalMisses = stats.totalMisses;
    qint64 total = totalHits + totalMisses;

    return total > 0
               ? static_cast<double>(totalHits) / static_cast<double>(total)
               : 0.0;
}

void MemoryManager::optimizeMemoryUsage() {
    QElapsedTimer timer;
    timer.start();

    QMutexLocker locker(&d->mutex);

    emit memoryOptimizationStarted(d->optimizationLevel);

    qint64 memoryBefore = d->calculateTotalCacheMemory();
    d->performOptimizationByLevel(d->optimizationLevel);
    qint64 memoryAfter = d->calculateTotalCacheMemory();

    qint64 memoryFreed = memoryBefore - memoryAfter;
    d->currentStats.optimizationCount++;
    d->currentStats.lastOptimization = QDateTime::currentDateTime();

    emit memoryOptimizationCompleted(memoryFreed);

    qint64 elapsedMs = timer.elapsed();
    locker.unlock();
    SLOG_DEBUG_F(
        "MemoryManager::optimizeMemoryUsage freed {} bytes in {} ms "
        "(level={})",
        memoryFreed, elapsedMs, static_cast<int>(d->optimizationLevel));
}

void MemoryManager::optimizeSearchCaches() {
    CacheManager& cacheManager = CacheManager::instance();
    auto stats = cacheManager.getCacheStats(CacheType::SearchResultCache);
    cacheManager.requestCacheEviction(
        CacheType::SearchResultCache,
        static_cast<qint64>(static_cast<double>(stats.memoryUsage) * 0.25));
}

void MemoryManager::optimizeTextCaches() {
    CacheManager& cacheManager = CacheManager::instance();
    auto stats = cacheManager.getCacheStats(CacheType::PageTextCache);
    cacheManager.requestCacheEviction(
        CacheType::PageTextCache,
        static_cast<qint64>(static_cast<double>(stats.memoryUsage) * 0.25));
}

void MemoryManager::optimizeHighlightCaches() {
    CacheManager& cacheManager = CacheManager::instance();
    if (!cacheManager.isCacheRegistered(CacheType::SearchHighlightCache)) {
        return;
    }

    auto stats = cacheManager.getCacheStats(CacheType::SearchHighlightCache);
    if (stats.memoryUsage <= 0) {
        return;
    }

    cacheManager.requestCacheEviction(
        CacheType::SearchHighlightCache,
        static_cast<qint64>(static_cast<double>(stats.memoryUsage) * 0.25));
}

void MemoryManager::performEmergencyCleanup() {
    QMutexLocker locker(&d->mutex);

    qint64 memoryBefore = d->calculateTotalCacheMemory();

    // Aggressive cleanup - request eviction across caches
    CacheManager& cacheManager = CacheManager::instance();
    auto req = [&](CacheType t) {
        if (!cacheManager.isCacheRegistered(t)) {
            return;
        }

        auto s = cacheManager.getCacheStats(t);
        if (s.memoryUsage <= 0) {
            return;
        }

        cacheManager.requestCacheEviction(
            t, static_cast<qint64>(static_cast<double>(s.memoryUsage) * 0.5));
    };
    req(CacheType::SearchResultCache);
    req(CacheType::PageTextCache);
    req(CacheType::SearchHighlightCache);
    req(CacheType::PdfRenderCache);
    req(CacheType::ThumbnailCache);

    qint64 memoryAfter = d->calculateTotalCacheMemory();
    qint64 memoryFreed = memoryBefore - memoryAfter;

    emit emergencyCleanupTriggered(memoryFreed);
}

void MemoryManager::enablePredictiveOptimization(bool enabled) {
    QMutexLocker locker(&d->mutex);
    d->predictiveOptimizationEnabled = enabled;
}

bool MemoryManager::isPredictiveOptimizationEnabled() const {
    QMutexLocker locker(&d->mutex);
    return d->predictiveOptimizationEnabled;
}

void MemoryManager::analyzeMemoryUsagePatterns() {
    // Analyze memory usage patterns and provide recommendations
    QMutexLocker locker(&d->mutex);

    double efficiency = getMemoryEfficiency();
    if (efficiency < 0.5) {
        emit optimizationRecommendation(
            "Consider increasing cache sizes for better hit rates");
    } else if (efficiency > 0.9 && d->currentStats.pressureLevel == Critical) {
        emit optimizationRecommendation(
            "High cache efficiency but memory pressure detected - consider "
            "reducing cache sizes");
    }
}

void MemoryManager::predictMemoryNeeds() {
    // Implement predictive memory management based on usage patterns
    if (!d->predictiveOptimizationEnabled) {
        return;
    }

    // This would analyze historical usage patterns and predict future memory
    // needs For now, just trigger optimization if patterns suggest high memory
    // usage
    analyzeMemoryUsagePatterns();
}

void MemoryManager::registerSearchEngine(SearchEngine* engine) {
    if (engine && !d->searchEngines.contains(engine)) {
        QMutexLocker locker(&d->mutex);
        d->searchEngines.append(engine);
    }
}

void MemoryManager::registerTextExtractor(TextExtractor* extractor) {
    if (extractor && !d->textExtractors.contains(extractor)) {
        QMutexLocker locker(&d->mutex);
        d->textExtractors.append(extractor);
    }
}

void MemoryManager::unregisterSearchEngine(SearchEngine* engine) {
    QMutexLocker locker(&d->mutex);
    d->searchEngines.removeAll(engine);
}

void MemoryManager::unregisterTextExtractor(TextExtractor* extractor) {
    QMutexLocker locker(&d->mutex);
    d->textExtractors.removeAll(extractor);
}

void MemoryManager::onMemoryPressureDetected(double pressure) {
    MemoryPressureLevel level =
        Implementation::calculatePressureLevel(pressure);

    if (level != d->currentStats.pressureLevel) {
        d->currentStats.pressureLevel = level;
        emit memoryPressureChanged(level);

        // Trigger optimization based on pressure level
        if (level >= Warning && d->autoOptimizationEnabled) {
            optimizeMemoryUsage();
        }
    }
}

void MemoryManager::onSystemMemoryPressure(double systemPressure) {
    // Handle system-wide memory pressure
    if (systemPressure > 0.9 && d->autoOptimizationEnabled) {
        performEmergencyCleanup();
    }
}

void MemoryManager::onCacheMemoryExceeded(qint64 /*usage*/, qint64 /*limit*/) {
    // Handle cache memory limit exceeded
    if (d->autoOptimizationEnabled) {
        optimizeMemoryUsage();
    }
}

void MemoryManager::performPeriodicOptimization() {
    if (d->autoOptimizationEnabled) {
        QElapsedTimer timer;
        timer.start();

        optimizeMemoryUsage();

        bool ranPredictive = false;
        if (d->predictiveOptimizationEnabled) {
            predictMemoryNeeds();
            ranPredictive = true;
        }

        qint64 elapsedMs = timer.elapsed();
        SLOG_DEBUG_F(
            "MemoryManager::performPeriodicOptimization completed in {} ms "
            "(predictive={})",
            elapsedMs, ranPredictive);
    }
}

void MemoryManager::updateMemoryStats() {
    QMutexLocker locker(&d->mutex);

    CacheManager& cacheManager = CacheManager::instance();

    d->currentStats.totalMemoryUsage = cacheManager.getTotalMemoryUsage();
    d->currentStats.systemMemoryUsage = CacheManager::getSystemMemoryUsage();
    d->currentStats.systemMemoryTotal = CacheManager::getSystemMemoryTotal();
    d->currentStats.memoryPressure = cacheManager.getSystemMemoryPressure();
    d->currentStats.pressureLevel =
        Implementation::calculatePressureLevel(d->currentStats.memoryPressure);

    // Get individual cache memory usage
    auto searchStats = cacheManager.getCacheStats(CacheType::SearchResultCache);
    auto textStats = cacheManager.getCacheStats(CacheType::PageTextCache);
    auto highlightStats =
        cacheManager.getCacheStats(CacheType::SearchHighlightCache);

    d->currentStats.searchCacheMemory = searchStats.memoryUsage;
    d->currentStats.textCacheMemory = textStats.memoryUsage;
    d->currentStats.highlightCacheMemory = highlightStats.memoryUsage;

    emit memoryStatsUpdated(d->currentStats);
}

void MemoryManager::checkMemoryPressure() {
    updateMemoryStats();

    if (d->currentStats.pressureLevel >= Warning &&
        d->autoOptimizationEnabled) {
        optimizeMemoryUsage();
    }
}

// Complete implementations for MemoryAwareSearchResults and SmartEvictionPolicy

// Implementation classes
class MemoryAwareSearchResults::ResultsImplementation {
public:
    explicit ResultsImplementation(QObject* parent = nullptr) {
        // Initialize memory monitoring timer with proper parent
        if (parent != nullptr) {
            memoryTimer = new QTimer(parent);
            memoryTimer->setInterval(5000);  // Check every 5 seconds
            memoryTimer->setSingleShot(false);
        }
    }

    ~ResultsImplementation() {
        if (memoryTimer != nullptr) {
            memoryTimer->stop();
            // Avoid double-delete: if the timer has a QObject parent (created
            // with parent), Qt's parent-child system will clean it up in
            // QObject::~QObject().
            if (memoryTimer->parent() == nullptr) {
                delete memoryTimer;
            }
            memoryTimer = nullptr;
        }
    }

    // Delete copy and move operations (Rule of Five)
    ResultsImplementation(const ResultsImplementation&) = delete;
    ResultsImplementation& operator=(const ResultsImplementation&) = delete;
    ResultsImplementation(ResultsImplementation&&) = delete;
    ResultsImplementation& operator=(ResultsImplementation&&) = delete;

    // Data members
    QList<SearchResult> results;
    mutable QHash<int, bool> loadedPages;  // Track which result pages are
                                           // loaded (mutable for lazy loading)
    mutable QMutex mutex;

    // Memory management
    qint64 maxMemoryUsage{static_cast<qint64>(50) * 1024 *
                          1024};  // 50MB default
    qint64 currentMemoryUsage{0};
    bool lazyLoadingEnabled{false};
    QTimer* memoryTimer{nullptr};

    // Lazy loading support
    struct LazyPage {
        int startIndex{0};
        int count{0};
        bool isLoaded{false};
        qint64 memorySize{0};
        QDateTime lastAccess;
    };
    mutable QHash<int, LazyPage> lazyPages;  // Mutable for lazy loading state

    // Helper methods
    qint64 calculateResultMemoryUsage(const SearchResult& result) const {
        qint64 size = 0;
        size += static_cast<qint64>(result.matchedText.size() * sizeof(QChar));
        size += static_cast<qint64>(result.contextText.size() * sizeof(QChar));
        size += static_cast<qint64>(sizeof(SearchResult));  // Base object size
        return size;
    }

    LazyPage initializeLazyPage(int pageIndex, int pageSize, int totalResults) {
        LazyPage page;
        page.startIndex = pageIndex * pageSize;
        page.count = qMin(pageSize, totalResults - page.startIndex);
        page.isLoaded = true;  // Currently loaded results are marked as loaded
        page.memorySize = 0;
        page.lastAccess = QDateTime::currentDateTime();

        // Calculate memory usage for this page
        for (int j = page.startIndex; j < page.startIndex + page.count; ++j) {
            if (j < results.size()) {
                page.memorySize += calculateResultMemoryUsage(results[j]);
            }
        }

        return page;
    }

    qint64 calculateTotalMemoryUsage() const {
        qint64 total = 0;
        for (const auto& result : results) {
            total += calculateResultMemoryUsage(result);
        }
        return total;
    }

    void updateMemoryUsage() {
        currentMemoryUsage = calculateTotalMemoryUsage();
    }

    void evictOldestResults(qint64 targetMemoryReduction) {
        if (results.isEmpty()) {
            return;
        }

        // Sort results by access time (if we had access tracking)
        // For now, remove from the end (LIFO approach)
        qint64 memoryFreed = 0;
        while (memoryFreed < targetMemoryReduction && !results.isEmpty()) {
            SearchResult removed = results.takeLast();
            memoryFreed += calculateResultMemoryUsage(removed);
        }

        updateMemoryUsage();
    }
};

class SmartEvictionPolicy::PolicyImplementation {
public:
    explicit PolicyImplementation(QObject* parent = nullptr)
        : parentObject(parent) {
        // Create timer with proper parent to avoid Qt meta-object system issues
        if (parent != nullptr) {
            analysisTimer = new QTimer(parent);
            analysisTimer->setInterval(30000);  // Analyze every 30 seconds
            analysisTimer->setSingleShot(false);
        }
    }

    ~PolicyImplementation() {
        if (analysisTimer != nullptr && parentObject == nullptr) {
            // Only delete if we created it without a parent
            analysisTimer->stop();
            delete analysisTimer;
        }
        // If analysisTimer has a parent, Qt will handle cleanup automatically
    }

    // Delete copy and move operations (Rule of Five)
    PolicyImplementation(const PolicyImplementation&) = delete;
    PolicyImplementation& operator=(const PolicyImplementation&) = delete;
    PolicyImplementation(PolicyImplementation&&) = delete;
    PolicyImplementation& operator=(PolicyImplementation&&) = delete;

    // Data members
    SmartEvictionPolicy::EvictionStrategy currentStrategy{
        SmartEvictionPolicy::LRU};
    double adaptiveThreshold{0.75};
    mutable QMutex mutex;
    QTimer* analysisTimer{nullptr};
    QObject* parentObject;

    // Access tracking
    struct AccessInfo {
        qint64 lastAccess{0};
        int accessCount{0};
        qint64 firstAccess{0};
        double averageInterval{0.0};
        bool isFrequent{false};
    };
    QHash<QString, AccessInfo> accessHistory;

    // Pattern analysis
    struct AccessPattern {
        QString patternType;  // "sequential", "random", "burst", "periodic"
        double confidence{0.0};
        QDateTime detectedAt;
        QVariantMap parameters;
    };
    QList<AccessPattern> detectedPatterns;

    // Strategy performance tracking
    struct StrategyStats {
        int evictionsPerformed;
        int correctPredictions;
        int totalPredictions;
        double averageAccuracy;
        qint64 memoryFreed;
    };
    QHash<SmartEvictionPolicy::EvictionStrategy, StrategyStats> strategyStats;

    // Helper methods
    double calculateLRUScore(const QString& itemId) const {
        auto it = accessHistory.find(itemId);
        if (it == accessHistory.end()) {
            return 0.0;  // Never accessed, highest eviction priority
        }

        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        qint64 timeSinceLastAccess =
            std::max(qint64{0}, currentTime - it->lastAccess);

        // Higher score = more recently used = lower eviction priority
        double denominator =
            1.0 + (static_cast<double>(timeSinceLastAccess) / 1000.0);
        return 1.0 / denominator;
    }

    double calculateLFUScore(const QString& itemId) const {
        auto iter = accessHistory.find(itemId);
        if (iter == accessHistory.end()) {
            return 0.0;  // Never accessed
        }

        // Higher access count = lower eviction priority
        return static_cast<double>(iter->accessCount);
    }

    double calculateAdaptiveScore(const QString& itemId) const {
        auto iter = accessHistory.find(itemId);
        if (iter == accessHistory.end()) {
            return 0.0;
        }

        // Cache the access info to avoid multiple lookups
        const AccessInfo& info = iter.value();
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

        // Calculate LRU score directly
        qint64 timeSinceLastAccess =
            std::max(qint64{0}, currentTime - info.lastAccess);
        double lruScore =
            1.0 / (1.0 + (static_cast<double>(timeSinceLastAccess) / 1000.0));

        // Calculate LFU score directly
        auto lfuScore = static_cast<double>(info.accessCount);

        // Calculate pattern score directly
        double patternScore = 0.5;  // Default neutral score
        if (info.isFrequent && info.averageInterval > 0) {
            if (static_cast<double>(timeSinceLastAccess) <
                info.averageInterval * 1.5) {
                patternScore = 1.0;
            }
        }

        // Weighted combination
        return (0.4 * lruScore) + (0.3 * lfuScore) + (0.3 * patternScore);
    }

    double calculatePatternScore(const QString& itemId) const {
        auto iter = accessHistory.find(itemId);
        if (iter == accessHistory.end()) {
            return 0.0;
        }

        // Analyze access patterns for this item
        if (iter->isFrequent && iter->averageInterval > 0) {
            qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
            qint64 timeSinceLastAccess = currentTime - iter->lastAccess;

            // If item is accessed regularly and we're within expected interval
            if (static_cast<double>(timeSinceLastAccess) <
                iter->averageInterval * 1.5) {
                return 1.0;  // High score, don't evict
            }
        }

        return 0.5;  // Neutral score
    }

    double calculatePredictiveScore(const QString& itemId) const {
        // Use machine learning-like approach to predict future access
        auto iter = accessHistory.find(itemId);
        if (iter == accessHistory.end()) {
            return 0.0;
        }

        // Cache the access info to avoid multiple lookups
        const AccessInfo& info = iter.value();
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

        // Calculate factors directly without recursive calls
        qint64 timeSinceLastAccess =
            std::max(qint64{0}, currentTime - info.lastAccess);
        double recencyFactor =
            1.0 / (1.0 + (static_cast<double>(timeSinceLastAccess) / 1000.0));
        double frequencyFactor =
            static_cast<double>(info.accessCount) / 10.0;  // Normalize

        double patternFactor = 0.5;  // Default neutral score
        if (info.isFrequent && info.averageInterval > 0) {
            if (static_cast<double>(timeSinceLastAccess) <
                info.averageInterval * 1.5) {
                patternFactor = 1.0;
            }
        }

        // Predict probability of future access
        return (recencyFactor + frequencyFactor + patternFactor) / 3.0;
    }

    void updateAccessInfo(const QString& itemId) {
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

        auto it = accessHistory.find(itemId);
        if (it == accessHistory.end()) {
            // First access
            AccessInfo info;
            info.firstAccess = currentTime;
            info.lastAccess = currentTime;
            info.accessCount = 1;
            info.averageInterval = 0;
            info.isFrequent = false;
            accessHistory[itemId] = info;
        } else {
            // Update existing access info
            qint64 interval = currentTime - it->lastAccess;
            it->accessCount++;

            // Update average interval
            if (it->averageInterval == 0) {
                it->averageInterval = static_cast<double>(interval);
            } else {
                it->averageInterval =
                    (it->averageInterval + static_cast<double>(interval)) / 2.0;
            }

            it->lastAccess = currentTime;

            // Determine if item is frequently accessed
            it->isFrequent = (it->accessCount >= 5 &&
                              it->averageInterval < 60000);  // < 1 minute
        }
    }
};

// Constructors (create the PIMPL)
MemoryAwareSearchResults::MemoryAwareSearchResults(QObject* parent)
    : QObject(parent), d(std::make_unique<ResultsImplementation>(this)) {}

SmartEvictionPolicy::SmartEvictionPolicy(QObject* parent)
    : QObject(parent), d(std::make_unique<PolicyImplementation>(this)) {}

// MemoryAwareSearchResults method implementations
void MemoryAwareSearchResults::addResults(const QList<SearchResult>& results) {
    QMutexLocker locker(&d->mutex);

    // Calculate memory impact
    qint64 additionalMemory = 0;
    for (const auto& result : results) {
        additionalMemory += d->calculateResultMemoryUsage(result);
    }

    // Check if we need to free memory first
    if (d->currentMemoryUsage + additionalMemory > d->maxMemoryUsage) {
        qint64 memoryToFree =
            (d->currentMemoryUsage + additionalMemory) - d->maxMemoryUsage;
        d->evictOldestResults(memoryToFree);
        emit memoryOptimized(memoryToFree);
    }

    // Add new results
    d->results.append(results);
    d->updateMemoryUsage();

    emit resultsAdded(static_cast<int>(results.size()));
}

void MemoryAwareSearchResults::clearResults() {
    QMutexLocker locker(&d->mutex);

    d->results.clear();
    d->loadedPages.clear();
    d->lazyPages.clear();
    d->currentMemoryUsage = 0;

    emit resultsCleared();
}

QList<SearchResult> MemoryAwareSearchResults::getResults(int start,
                                                         int count) const {
    QMutexLocker locker(&d->mutex);

    if (start < 0 || start >= static_cast<int>(d->results.size())) {
        return QList<SearchResult>();
    }

    int actualCount = count;
    if (count < 0 || start + count > static_cast<int>(d->results.size())) {
        actualCount = static_cast<int>(d->results.size()) - start;
    }

    // Handle lazy loading
    if (d->lazyLoadingEnabled) {
        // Check if requested range is loaded
        int pageId = start / 100;  // Assume 100 results per page
        if (!d->loadedPages.value(pageId, false)) {
            // Request lazy loading (const-safe due to mutable members)
            preloadResults(start, actualCount);
        }
    }

    return d->results.mid(start, actualCount);
}

int MemoryAwareSearchResults::getResultCount() const {
    QMutexLocker locker(&d->mutex);
    return static_cast<int>(d->results.size());
}

void MemoryAwareSearchResults::setMaxMemoryUsage(qint64 maxBytes) {
    QMutexLocker locker(&d->mutex);

    qint64 oldMax = d->maxMemoryUsage;
    d->maxMemoryUsage = maxBytes;

    // If new limit is lower and we're over it, optimize immediately
    if (maxBytes < oldMax && d->currentMemoryUsage > maxBytes) {
        qint64 memoryToFree = d->currentMemoryUsage - maxBytes;
        d->evictOldestResults(memoryToFree);
        emit memoryOptimized(memoryToFree);
    }
}

qint64 MemoryAwareSearchResults::getMaxMemoryUsage() const {
    QMutexLocker locker(&d->mutex);
    return d->maxMemoryUsage;
}

qint64 MemoryAwareSearchResults::getCurrentMemoryUsage() const {
    QMutexLocker locker(&d->mutex);
    return d->currentMemoryUsage;
}

void MemoryAwareSearchResults::optimizeMemoryUsage() {
    QMutexLocker locker(&d->mutex);

    if (d->currentMemoryUsage <= d->maxMemoryUsage) {
        return;  // No optimization needed
    }

    qint64 memoryToFree =
        d->currentMemoryUsage -
        static_cast<qint64>(static_cast<double>(d->maxMemoryUsage) *
                            0.8);  // Target 80% of max
    qint64 initialMemory = d->currentMemoryUsage;

    d->evictOldestResults(memoryToFree);

    qint64 memoryFreed = initialMemory - d->currentMemoryUsage;
    emit memoryOptimized(memoryFreed);
}

void MemoryAwareSearchResults::enableLazyLoading(bool enabled) {
    QMutexLocker locker(&d->mutex);
    d->lazyLoadingEnabled = enabled;

    if (enabled) {
        // Initialize lazy loading pages
        int totalResults = static_cast<int>(d->results.size());
        constexpr int pageSize = 100;
        int pageCount = (totalResults + pageSize - 1) / pageSize;

        for (int i = 0; i < pageCount; ++i) {
            d->lazyPages[i] = d->initializeLazyPage(i, pageSize, totalResults);
            d->loadedPages[i] = true;
        }
    }
}

bool MemoryAwareSearchResults::isLazyLoadingEnabled() const {
    QMutexLocker locker(&d->mutex);
    return d->lazyLoadingEnabled;
}

void MemoryAwareSearchResults::preloadResults(int start, int count) const {
    QMutexLocker locker(&d->mutex);

    if (!d->lazyLoadingEnabled) {
        return;
    }

    int pageSize = 100;
    int startPage = start / pageSize;
    int endPage = (start + count - 1) / pageSize;

    for (int pageId = startPage; pageId <= endPage; ++pageId) {
        if (!d->loadedPages.value(pageId, false)) {
            // Simulate loading (in real implementation, this would load from
            // storage)
            d->loadedPages[pageId] = true;

            if (d->lazyPages.contains(pageId)) {
                d->lazyPages[pageId].isLoaded = true;
                d->lazyPages[pageId].lastAccess = QDateTime::currentDateTime();
            }

            // Qt signals are logically const (don't modify observable state)
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            emit const_cast<MemoryAwareSearchResults*>(this)->lazyLoadRequested(
                pageId * pageSize, pageSize);
        }
    }
}

// SmartEvictionPolicy method implementations
void SmartEvictionPolicy::setEvictionStrategy(EvictionStrategy strategy) {
    QMutexLocker locker(&d->mutex);

    if (d->currentStrategy != strategy) {
        d->currentStrategy = strategy;
        emit evictionStrategyChanged(strategy);
    }
}

SmartEvictionPolicy::EvictionStrategy SmartEvictionPolicy::getEvictionStrategy()
    const {
    QMutexLocker locker(&d->mutex);
    return d->currentStrategy;
}

void SmartEvictionPolicy::setAdaptiveThreshold(double threshold) {
    QMutexLocker locker(&d->mutex);
    d->adaptiveThreshold = qBound(0.0, threshold, 1.0);
}

double SmartEvictionPolicy::getAdaptiveThreshold() const {
    QMutexLocker locker(&d->mutex);
    return d->adaptiveThreshold;
}

QStringList SmartEvictionPolicy::selectItemsForEviction(
    const QStringList& candidates, int targetCount) {
    QMutexLocker locker(&d->mutex);

    if (candidates.isEmpty() || targetCount <= 0) {
        return QStringList();
    }

    // Calculate scores for all candidates based on current strategy
    QList<QPair<QString, double>> scoredItems;

    for (const QString& itemId : candidates) {
        double score = 0.0;

        switch (d->currentStrategy) {
            case LRU:
                score = d->calculateLRUScore(itemId);
                break;
            case LFU:
                score = d->calculateLFUScore(itemId);
                break;
            case Adaptive:
                score = d->calculateAdaptiveScore(itemId);
                break;
            case Predictive:
                score = d->calculatePredictiveScore(itemId);
                break;
        }

        scoredItems.append(qMakePair(itemId, score));
    }

    // Sort by score (lower score = higher eviction priority)
    std::ranges::sort(scoredItems, [](const QPair<QString, double>& a,
                                      const QPair<QString, double>& b) {
        return a.second < b.second;
    });

    // Select items for eviction
    QStringList itemsToEvict;
    int count = qMin(targetCount, static_cast<int>(scoredItems.size()));

    for (int i = 0; i < count; ++i) {
        itemsToEvict.append(scoredItems[i].first);
    }

    // Update statistics
    auto& stats = d->strategyStats[d->currentStrategy];
    stats.evictionsPerformed += count;
    stats.totalPredictions += count;

    return itemsToEvict;
}

bool SmartEvictionPolicy::shouldEvictItem(const QString& itemId,
                                          qint64 lastAccess, int accessCount) {
    QMutexLocker locker(&d->mutex);

    // Update our internal access tracking with provided data
    if (d->accessHistory.contains(itemId)) {
        auto& info = d->accessHistory[itemId];
        info.lastAccess = lastAccess;
        info.accessCount = accessCount;
    } else {
        PolicyImplementation::AccessInfo info;
        info.lastAccess = lastAccess;
        info.accessCount = accessCount;
        info.firstAccess = lastAccess;
        info.averageInterval = 0;
        info.isFrequent = accessCount > 5;
        d->accessHistory[itemId] = info;
    }

    // Calculate eviction score
    double score = 0.0;
    switch (d->currentStrategy) {
        case LRU:
            score = d->calculateLRUScore(itemId);
            break;
        case LFU:
            score = d->calculateLFUScore(itemId);
            break;
        case Adaptive:
            score = d->calculateAdaptiveScore(itemId);
            break;
        case Predictive:
            score = d->calculatePredictiveScore(itemId);
            break;
    }

    // Lower score means higher eviction priority
    return score < d->adaptiveThreshold;
}

void SmartEvictionPolicy::recordAccess(const QString& itemId) {
    QMutexLocker locker(&d->mutex);
    d->updateAccessInfo(itemId);
}

void SmartEvictionPolicy::recordEviction(const QString& itemId) {
    QMutexLocker locker(&d->mutex);

    // Remove from access history
    d->accessHistory.remove(itemId);

    // Update statistics
    auto& stats = d->strategyStats[d->currentStrategy];
    stats.evictionsPerformed++;
}

void SmartEvictionPolicy::analyzeAccessPatterns() {
    QMutexLocker locker(&d->mutex);

    d->detectedPatterns.clear();

    if (d->accessHistory.isEmpty()) {
        return;
    }

    // Analyze temporal patterns
    QList<qint64> accessTimes;
    for (const auto& accessInfo : d->accessHistory) {
        accessTimes.append(accessInfo.lastAccess);
    }

    std::ranges::sort(accessTimes);

    // Detect sequential access pattern
    if (accessTimes.size() > 1) {
        qint64 averageInterval = 0;
        for (int i = 1; i < static_cast<int>(accessTimes.size()); ++i) {
            averageInterval += accessTimes[i] - accessTimes[i - 1];
        }
        averageInterval /= static_cast<qint64>(accessTimes.size() - 1);

        // Check if intervals are relatively consistent
        int consistentIntervals = 0;
        for (int i = 1; i < static_cast<int>(accessTimes.size()); ++i) {
            qint64 interval = accessTimes[i] - accessTimes[i - 1];
            if (qAbs(interval - averageInterval) <
                static_cast<qint64>(static_cast<double>(averageInterval) *
                                    0.3)) {
                consistentIntervals++;
            }
        }

        if (static_cast<double>(consistentIntervals) >=
            static_cast<double>(accessTimes.size()) * 0.7) {
            PolicyImplementation::AccessPattern pattern;
            pattern.patternType = "sequential";
            pattern.confidence = static_cast<double>(consistentIntervals) /
                                 static_cast<double>(accessTimes.size());
            pattern.detectedAt = QDateTime::currentDateTime();
            pattern.parameters["averageInterval"] = averageInterval;
            d->detectedPatterns.append(pattern);
        }
    }

    // Detect burst pattern (many accesses in short time)
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    int recentAccesses = 0;
    qint64 burstWindow = 10000;  // 10 seconds

    for (const auto& accessInfo : d->accessHistory) {
        if (currentTime - accessInfo.lastAccess < burstWindow) {
            recentAccesses++;
        }
    }

    if (static_cast<double>(recentAccesses) >=
        static_cast<double>(d->accessHistory.size()) * 0.5) {
        PolicyImplementation::AccessPattern pattern;
        pattern.patternType = "burst";
        pattern.confidence = static_cast<double>(recentAccesses) /
                             static_cast<double>(d->accessHistory.size());
        pattern.detectedAt = QDateTime::currentDateTime();
        pattern.parameters["burstWindow"] = burstWindow;
        pattern.parameters["burstCount"] = recentAccesses;
        d->detectedPatterns.append(pattern);
    }

    // Emit analysis results
    for (const auto& pattern : d->detectedPatterns) {
        emit accessPatternAnalyzed(QString("%1 (confidence: %2)")
                                       .arg(pattern.patternType)
                                       .arg(pattern.confidence, 0, 'f', 2));
    }
}

void SmartEvictionPolicy::updateEvictionStrategy() {
    QMutexLocker locker(&d->mutex);

    // Analyze current strategy performance
    auto& currentStats = d->strategyStats[d->currentStrategy];
    double currentAccuracy = 0.0;
    if (currentStats.totalPredictions > 0) {
        currentAccuracy = static_cast<double>(currentStats.correctPredictions) /
                          currentStats.totalPredictions;
    }

    // Find best performing strategy
    EvictionStrategy bestStrategy = d->currentStrategy;
    double bestAccuracy = currentAccuracy;

    for (auto it = d->strategyStats.begin(); it != d->strategyStats.end();
         ++it) {
        if (it->totalPredictions > 10) {  // Minimum sample size
            double accuracy = static_cast<double>(it->correctPredictions) /
                              it->totalPredictions;
            if (accuracy > bestAccuracy) {
                bestAccuracy = accuracy;
                bestStrategy = it.key();
            }
        }
    }

    // Switch strategy if we found a better one
    if (bestStrategy != d->currentStrategy &&
        bestAccuracy > currentAccuracy + 0.1) {
        setEvictionStrategy(bestStrategy);
        emit evictionRecommendation(
            QString("Switched to %1 strategy (accuracy: %2)")
                .arg(bestStrategy)
                .arg(bestAccuracy, 0, 'f', 2));
    }
}

QString SmartEvictionPolicy::getRecommendedStrategy() const {
    QMutexLocker locker(&d->mutex);

    // Analyze detected patterns and recommend strategy
    if (d->detectedPatterns.isEmpty()) {
        return "LRU";  // Default recommendation
    }

    // Look for dominant pattern
    QString dominantPattern;
    double maxConfidence = 0.0;

    for (const auto& pattern : d->detectedPatterns) {
        if (pattern.confidence > maxConfidence) {
            maxConfidence = pattern.confidence;
            dominantPattern = pattern.patternType;
        }
    }

    // Recommend strategy based on pattern
    if (dominantPattern == "sequential") {
        return "Predictive";
    }
    if (dominantPattern == "burst") {
        return "LFU";
    }
    if (dominantPattern == "random") {
        return "LRU";
    }
    return "Adaptive";
}
