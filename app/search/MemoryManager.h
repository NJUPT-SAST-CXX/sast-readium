#pragma once

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QHash>
#include <QDateTime>
#include "../cache/CacheManager.h"

/**
 * Memory management utility for search components
 * Provides intelligent memory management and optimization strategies
 */
class MemoryManager : public QObject
{
    Q_OBJECT

public:
    enum OptimizationLevel {
        Conservative,   // Minimal optimization, preserve performance
        Balanced,       // Balance between memory and performance
        Aggressive      // Maximum memory optimization
    };

    enum MemoryPressureLevel {
        Normal,         // < 70% memory usage
        Warning,        // 70-85% memory usage
        Critical        // > 85% memory usage
    };

    struct MemoryStats {
        qint64 totalMemoryUsage = 0;
        qint64 searchCacheMemory = 0;
        qint64 textCacheMemory = 0;
        qint64 highlightCacheMemory = 0;
        qint64 systemMemoryUsage = 0;
        qint64 systemMemoryTotal = 0;
        double memoryPressure = 0.0;
        MemoryPressureLevel pressureLevel = Normal;
        QDateTime lastOptimization;
        int optimizationCount = 0;
    };

    explicit MemoryManager(QObject* parent = nullptr);
    ~MemoryManager();

    // Configuration
    void setOptimizationLevel(OptimizationLevel level);
    OptimizationLevel getOptimizationLevel() const;
    void setAutoOptimizationEnabled(bool enabled);
    bool isAutoOptimizationEnabled() const;
    void setOptimizationInterval(int seconds);
    int getOptimizationInterval() const;

    // Memory monitoring
    MemoryStats getMemoryStats() const;
    MemoryPressureLevel getCurrentPressureLevel() const;
    qint64 getAvailableMemory() const;
    double getMemoryEfficiency() const;

    // Manual optimization
    void optimizeMemoryUsage();
    void optimizeSearchCaches();
    void optimizeTextCaches();
    void optimizeHighlightCaches();
    void performEmergencyCleanup();

    // Predictive optimization
    void enablePredictiveOptimization(bool enabled);
    bool isPredictiveOptimizationEnabled() const;
    void analyzeMemoryUsagePatterns();
    void predictMemoryNeeds();

    // Integration with search components
    void registerSearchEngine(class SearchEngine* engine);
    void registerTextExtractor(class TextExtractor* extractor);
    void unregisterSearchEngine(class SearchEngine* engine);
    void unregisterTextExtractor(class TextExtractor* extractor);

public slots:
    void onMemoryPressureDetected(double pressure);
    void onSystemMemoryPressure(double systemPressure);
    void onCacheMemoryExceeded(qint64 usage, qint64 limit);
    void performPeriodicOptimization();

signals:
    void memoryOptimizationStarted(OptimizationLevel level);
    void memoryOptimizationCompleted(qint64 memoryFreed);
    void memoryPressureChanged(MemoryPressureLevel level);
    void memoryStatsUpdated(const MemoryStats& stats);
    void emergencyCleanupTriggered(qint64 memoryFreed);
    void optimizationRecommendation(const QString& recommendation);

private slots:
    void updateMemoryStats();
    void checkMemoryPressure();

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};

/**
 * Memory-aware search result container
 * Automatically manages memory usage based on system pressure
 */
class MemoryAwareSearchResults : public QObject
{
    Q_OBJECT

public:
    explicit MemoryAwareSearchResults(QObject* parent = nullptr);
    ~MemoryAwareSearchResults();

    // Result management
    void addResults(const QList<class SearchResult>& results);
    void clearResults();
    QList<class SearchResult> getResults(int start = 0, int count = -1) const;
    int getResultCount() const;

    // Memory management
    void setMaxMemoryUsage(qint64 maxBytes);
    qint64 getMaxMemoryUsage() const;
    qint64 getCurrentMemoryUsage() const;
    void optimizeMemoryUsage();

    // Lazy loading
    void enableLazyLoading(bool enabled);
    bool isLazyLoadingEnabled() const;
    void preloadResults(int start, int count);

signals:
    void resultsAdded(int count);
    void resultsCleared();
    void memoryOptimized(qint64 memoryFreed);
    void lazyLoadRequested(int start, int count);

private:
    class ResultsImplementation;
    std::unique_ptr<ResultsImplementation> d;
};

/**
 * Smart cache eviction policy
 * Implements intelligent eviction based on usage patterns and memory pressure
 */
class SmartEvictionPolicy : public QObject
{
    Q_OBJECT

public:
    enum EvictionStrategy {
        LRU,            // Least Recently Used
        LFU,            // Least Frequently Used
        Adaptive,       // Adaptive based on access patterns
        Predictive      // Predictive based on usage patterns
    };

    explicit SmartEvictionPolicy(QObject* parent = nullptr);
    ~SmartEvictionPolicy();

    // Strategy configuration
    void setEvictionStrategy(EvictionStrategy strategy);
    EvictionStrategy getEvictionStrategy() const;
    void setAdaptiveThreshold(double threshold);
    double getAdaptiveThreshold() const;

    // Eviction decisions
    QStringList selectItemsForEviction(const QStringList& candidates, int targetCount);
    bool shouldEvictItem(const QString& itemId, qint64 lastAccess, int accessCount);
    void recordAccess(const QString& itemId);
    void recordEviction(const QString& itemId);

    // Pattern analysis
    void analyzeAccessPatterns();
    void updateEvictionStrategy();
    QString getRecommendedStrategy() const;

signals:
    void evictionStrategyChanged(EvictionStrategy strategy);
    void accessPatternAnalyzed(const QString& pattern);
    void evictionRecommendation(const QString& recommendation);

private:
    class PolicyImplementation;
    std::unique_ptr<PolicyImplementation> d;
};
