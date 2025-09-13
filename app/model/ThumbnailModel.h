#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QPixmap>
#include <QSize>
#include <QHash>
#include <QTimer>
#include <QMutex>
#include <QFuture>
#include <QFutureWatcher>
#include <QDateTime>
#include <QSet>
#include <QModelIndex>
#include <QVariant>
#include <QByteArray>
#include <QCache>
#include <QMultiHash>
#include <QQueue>
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QThread>
#include <QThreadPool>
#include <atomic>
#include <memory>
#include <poppler/qt6/poppler-qt6.h>
#include "../utils/ErrorHandling.h"

// 前向声明
namespace Poppler {
    class Document;
    class Page;
}

class ThumbnailGenerator;

/**
 * @brief 高性能的PDF缩略图数据模型
 * 
 * 特性：
 * - 基于QAbstractListModel，支持虚拟滚动
 * - 异步缩略图生成和加载
 * - 智能缓存管理
 * - 懒加载机制
 * - 内存使用优化
 */
class ThumbnailModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum ThumbnailRole {
        PageNumberRole = Qt::UserRole + 1,
        PixmapRole,
        LoadingRole,
        ErrorRole,
        ErrorMessageRole,
        PageSizeRole,
        CacheHitRole,
        CompressionRatioRole,
        LastAccessTimeRole
    };

    enum class PrefetchStrategy {
        NONE,              // 不预取
        LINEAR,            // 线性预取
        ADAPTIVE,          // 自适应预取
        PREDICTIVE,        // 预测性预取
        MACHINE_LEARNING   // 机器学习预取
    };

    enum class CompressionMode {
        NONE,              // 不压缩
        LOSSLESS,          // 无损压缩
        LOSSY,             // 有损压缩
        ADAPTIVE           // 自适应压缩
    };

    enum class MemoryStrategy {
        CONSERVATIVE,      // 保守策略
        BALANCED,          // 平衡策略
        AGGRESSIVE,        // 激进策略
        ADAPTIVE           // 自适应策略
    };

    explicit ThumbnailModel(QObject* parent = nullptr);
    ~ThumbnailModel() override;

    // QAbstractListModel接口
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QHash<int, QByteArray> roleNames() const override;

    // 文档管理
    void setDocument(std::shared_ptr<Poppler::Document> document);
    std::shared_ptr<Poppler::Document> document() const { return m_document; }
    
    // 缩略图设置
    void setThumbnailSize(const QSize& size);
    QSize thumbnailSize() const { return m_thumbnailSize; }
    
    void setThumbnailQuality(double quality);
    double thumbnailQuality() const { return m_thumbnailQuality; }
    
    // 缓存管理
    void setCacheSize(int maxItems);
    int cacheSize() const { return m_maxCacheSize; }
    
    void setMemoryLimit(qint64 maxMemory);
    qint64 memoryLimit() const { return m_maxMemory; }
    
    void clearCache();
    
    // 预加载控制
    void setPreloadRange(int range);
    int preloadRange() const { return m_preloadRange; }
    
    void requestThumbnail(int pageNumber);
    void requestThumbnailRange(int startPage, int endPage);
    
    // 状态查询
    bool isLoading(int pageNumber) const;
    bool hasError(int pageNumber) const;
    QString errorMessage(int pageNumber) const;
    
    // 预取策略控制
    void setPrefetchStrategy(PrefetchStrategy strategy);
    PrefetchStrategy prefetchStrategy() const { return m_prefetchStrategy; }

    void setPrefetchDistance(int distance);
    int prefetchDistance() const { return m_prefetchDistance; }

    // 压缩控制
    void setCompressionMode(CompressionMode mode);
    CompressionMode compressionMode() const { return m_compressionMode; }

    void setCompressionQuality(int quality);
    int compressionQuality() const { return m_compressionQuality; }

    // 内存策略控制
    void setMemoryStrategy(MemoryStrategy strategy);
    MemoryStrategy memoryStrategy() const { return m_memoryStrategy; }

    void setMemoryPressureThreshold(double threshold);
    double memoryPressureThreshold() const { return m_memoryPressureThreshold; }

    // 高级缓存控制
    void enableIntelligentPrefetch(bool enabled);
    bool isIntelligentPrefetchEnabled() const { return m_intelligentPrefetchEnabled; }

    void enableMemoryCompression(bool enabled);
    bool isMemoryCompressionEnabled() const { return m_memoryCompressionEnabled; }

    void enablePredictiveLoading(bool enabled);
    bool isPredictiveLoadingEnabled() const { return m_predictiveLoadingEnabled; }

    // 统计信息
    int cacheHitCount() const { return m_cacheHits; }
    int cacheMissCount() const { return m_cacheMisses; }
    qint64 currentMemoryUsage() const { return m_currentMemory; }
    double compressionRatio() const;
    double averageAccessTime() const;
    int prefetchHitRate() const;
    
public slots:
    void refreshThumbnail(int pageNumber);
    void refreshAllThumbnails();
    void preloadVisibleRange(int firstVisible, int lastVisible);

    // 懒加载和视口管理
    void setLazyLoadingEnabled(bool enabled);
    void setViewportRange(int start, int end, int margin = 2);
    void updateViewportPriorities();

signals:
    void thumbnailLoaded(int pageNumber);
    void thumbnailError(int pageNumber, const QString& error);
    void cacheUpdated();
    void memoryUsageChanged(qint64 usage);
    void loadingStateChanged(int pageNumber, bool loading);

private slots:
    void onThumbnailGenerated(int pageNumber, const QPixmap& pixmap);
    void onThumbnailError(int pageNumber, const QString& error);
    void onPreloadTimer();
    void cleanupCache();
    void onPriorityUpdateTimer();

private:
    struct ThumbnailItem {
        QPixmap pixmap;
        bool isLoading = false;
        bool hasError = false;
        QString errorMessage;
        qint64 lastAccessed = 0;
        qint64 memorySize = 0;
        QSize pageSize;
        int accessCount = 0;  // For LFU tracking
        QByteArray compressedData; // 压缩数据
        double compressionRatio = 1.0; // 压缩比
        bool isCompressed = false; // 是否已压缩
        qint64 loadTime = 0; // 加载时间
        bool wasPrefetched = false; // 是否通过预取获得

        ThumbnailItem() = default;
        ThumbnailItem(const ThumbnailItem&) = default;
        ThumbnailItem& operator=(const ThumbnailItem&) = default;
    };

    // 预取条目
    struct PrefetchEntry {
        int pageNumber;
        int priority;
        qint64 timestamp;
        PrefetchStrategy strategy;

        PrefetchEntry(int page, int prio, PrefetchStrategy strat)
            : pageNumber(page), priority(prio),
              timestamp(QDateTime::currentMSecsSinceEpoch()), strategy(strat) {}
    };

    // 访问模式分析
    struct AccessPattern {
        QList<int> recentAccesses;
        QHash<int, int> accessFrequency;
        QElapsedTimer sessionTimer;
        double averageInterval = 0.0;
        int sequentialCount = 0;
        int randomCount = 0;

        AccessPattern() {
            sessionTimer.start();
        }
    };

    // Optimized cache entry for QCache
    struct CacheEntry {
        ThumbnailItem item;
        int pageNumber;

        CacheEntry(const ThumbnailItem& thumbnailItem, int page)
            : item(thumbnailItem), pageNumber(page) {}
    };

    void initializeModel();
    void updateThumbnailItem(int pageNumber, const ThumbnailItem& item);
    // Legacy eviction methods (kept for compatibility)
    void evictLeastRecentlyUsed();
    void evictLeastFrequentlyUsed();
    void evictByAdaptivePolicy();

    // Optimized cache methods
    void insertIntoOptimizedCache(int pageNumber, const ThumbnailItem& item);
    ThumbnailItem* getFromOptimizedCache(int pageNumber);
    void evictFromOptimizedCache(int count = 1);
    void updateAccessFrequencyOptimized(int pageNumber);
    void cleanupOptimizedCache();
    qint64 calculatePixmapMemory(const QPixmap& pixmap) const;
    void updateMemoryUsage();
    bool shouldPreload(int pageNumber) const;

    // 高级缓存管理
    void updateAccessFrequency(int pageNumber);
    double calculateCacheEfficiency() const;
    void adaptCacheSize();

    // 懒加载优化方法
    bool shouldGenerateThumbnail(int pageNumber) const;
    int calculatePriority(int pageNumber) const;
    bool isInViewport(int pageNumber) const;

    // 新增的高级方法
    void initializeAdvancedFeatures();
    void cleanupAdvancedFeatures();

    // 预取方法
    void startIntelligentPrefetch();
    void stopIntelligentPrefetch();
    void processPrefetchQueue();
    void addToPrefetchQueue(int pageNumber, PrefetchStrategy strategy, int priority = 0);
    void predictNextPages(const QList<int>& recentAccesses, QList<int>& predictions);

    // 压缩方法
    QByteArray compressThumbnail(const QPixmap& pixmap);
    QPixmap decompressThumbnail(const QByteArray& data);
    void updateCompressionStats(qint64 originalSize, qint64 compressedSize);

    // 访问模式分析
    void analyzeAccessPattern(int pageNumber);
    void updateAccessPattern();
    PrefetchStrategy determineBestStrategy() const;

    // 内存管理优化
    void optimizeMemoryUsage();
    bool isMemoryPressureHigh() const;
    void handleMemoryPressure();
    void compressOldEntries();

    // 性能监控
    void recordAccessTime(qint64 time);
    void updatePerformanceMetrics();
    
    // 数据成员
    std::shared_ptr<Poppler::Document> m_document;
    std::unique_ptr<ThumbnailGenerator> m_generator;

    // Optimized cache implementation
    mutable QCache<int, CacheEntry> m_optimizedCache;  // Main LRU cache
    mutable QHash<int, ThumbnailItem> m_thumbnails;    // Fallback for compatibility
    mutable QMultiHash<int, int> m_accessFrequencyIndex;  // frequency -> pageNumber mapping
    mutable QSet<int> m_loadingPages;  // Track pages currently being loaded
    mutable QMutex m_thumbnailsMutex;
    
    QSize m_thumbnailSize;
    double m_thumbnailQuality;
    
    // 缓存管理 - 优化版本
    int m_maxCacheSize;
    qint64 m_maxMemory;
    std::atomic<qint64> m_currentMemory{0};  // Thread-safe memory tracking
    std::atomic<int> m_cacheHits{0};         // Thread-safe hit counter
    std::atomic<int> m_cacheMisses{0};       // Thread-safe miss counter

    // 高级缓存策略
    double m_cacheCompressionRatio;
    bool m_adaptiveCaching;
    QHash<int, int> m_accessFrequency;
    qint64 m_lastCleanupTime;
    
    // 预加载和懒加载优化
    int m_preloadRange;
    QTimer* m_preloadTimer;
    QSet<int> m_preloadQueue;

    // 视口管理
    int m_visibleStart;
    int m_visibleEnd;
    int m_viewportMargin;
    bool m_lazyLoadingEnabled;

    // 优先级管理
    QHash<int, int> m_pagePriorities;
    QTimer* m_priorityUpdateTimer;

    // 新增的高级功能成员变量
    PrefetchStrategy m_prefetchStrategy;
    int m_prefetchDistance;
    CompressionMode m_compressionMode;
    int m_compressionQuality;
    MemoryStrategy m_memoryStrategy;
    double m_memoryPressureThreshold;

    // 功能开关
    bool m_intelligentPrefetchEnabled;
    bool m_memoryCompressionEnabled;
    bool m_predictiveLoadingEnabled;

    // 预取管理
    QQueue<PrefetchEntry> m_prefetchQueue;
    QSet<int> m_prefetchedPages;
    QTimer* m_prefetchTimer;
    QThreadPool* m_prefetchThreadPool;
    std::atomic<int> m_prefetchHits{0};
    std::atomic<int> m_prefetchMisses{0};

    // 访问模式分析
    AccessPattern m_accessPattern;
    QTimer* m_patternAnalysisTimer;

    // 压缩管理
    QHash<int, QByteArray> m_compressedCache;
    mutable QMutex m_compressionMutex;
    std::atomic<qint64> m_originalSize{0};
    std::atomic<qint64> m_compressedSize{0};

    // 性能监控
    QElapsedTimer m_performanceTimer;
    QList<qint64> m_accessTimes;
    mutable QMutex m_performanceMutex;

    // 常量
    static constexpr int DEFAULT_THUMBNAIL_WIDTH = 120;
    static constexpr int DEFAULT_THUMBNAIL_HEIGHT = 160;
    static constexpr double DEFAULT_QUALITY = 1.0;
    static constexpr int DEFAULT_CACHE_SIZE = 100;
    static constexpr qint64 DEFAULT_MEMORY_LIMIT = 128 * 1024 * 1024; // 128MB
    static constexpr int DEFAULT_PRELOAD_RANGE = 5;
    static constexpr int PRELOAD_TIMER_INTERVAL = 100; // ms
    static constexpr int DEFAULT_PREFETCH_DISTANCE = 3;
    static constexpr int DEFAULT_COMPRESSION_QUALITY = 85;
    static constexpr double DEFAULT_MEMORY_PRESSURE_THRESHOLD = 0.8;
    static constexpr int PATTERN_ANALYSIS_INTERVAL = 5000; // 5秒
    static constexpr int MAX_ACCESS_HISTORY = 100;
};
