#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QAbstractListModel>
#include <QByteArray>
#include <QDateTime>
#include <QFuture>
#include <QFutureWatcher>
#include <QHash>
#include <QModelIndex>
#include <QMutex>
#include <QObject>
#include <QPixmap>
#include <QSet>
#include <QSize>
#include <QTimer>
#include <QVariant>
#include <memory>

// 前向声明
namespace Poppler {
class Document;
class Page;
}  // namespace Poppler

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
class ThumbnailModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum ThumbnailRole {
        PageNumberRole = Qt::UserRole + 1,
        PixmapRole,
        LoadingRole,
        ErrorRole,
        ErrorMessageRole,
        PageSizeRole
    };

    explicit ThumbnailModel(QObject* parent = nullptr);
    ~ThumbnailModel() override;

    // QAbstractListModel接口
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override;
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

    // 统计信息
    int cacheHitCount() const { return m_cacheHits; }
    int cacheMissCount() const { return m_cacheMisses; }
    qint64 currentMemoryUsage() const { return m_currentMemory; }

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

        ThumbnailItem() = default;
        ThumbnailItem(const ThumbnailItem&) = default;
        ThumbnailItem& operator=(const ThumbnailItem&) = default;
    };

    void initializeModel();
    void updateThumbnailItem(int pageNumber, const ThumbnailItem& item);
    void evictLeastRecentlyUsed();
    void evictLeastFrequentlyUsed();
    void evictByAdaptivePolicy();
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

    // 数据成员
    std::shared_ptr<Poppler::Document> m_document;
    std::unique_ptr<ThumbnailGenerator> m_generator;

    mutable QHash<int, ThumbnailItem> m_thumbnails;
    mutable QMutex m_thumbnailsMutex;

    QSize m_thumbnailSize;
    double m_thumbnailQuality;

    // 缓存管理 - 优化版本
    int m_maxCacheSize;
    qint64 m_maxMemory;
    qint64 m_currentMemory;
    int m_cacheHits;
    int m_cacheMisses;

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

    // 常量
    static constexpr int DEFAULT_THUMBNAIL_WIDTH = 120;
    static constexpr int DEFAULT_THUMBNAIL_HEIGHT = 160;
    static constexpr double DEFAULT_QUALITY = 1.0;
    static constexpr int DEFAULT_CACHE_SIZE = 100;
    static constexpr qint64 DEFAULT_MEMORY_LIMIT = 128 * 1024 * 1024;  // 128MB
    static constexpr int DEFAULT_PRELOAD_RANGE = 5;
    static constexpr int PRELOAD_TIMER_INTERVAL = 100;  // ms
};
