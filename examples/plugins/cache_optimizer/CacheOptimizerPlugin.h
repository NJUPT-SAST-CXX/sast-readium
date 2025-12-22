#pragma once

#include <QHash>
#include <QObject>
#include "plugin/ICacheStrategyPlugin.h"
#include "plugin/PluginInterface.h"

/**
 * @brief CacheOptimizerPlugin - Example cache strategy plugin
 *
 * This plugin demonstrates the ICacheStrategyPlugin interface by providing:
 * - **LFU Strategy**: Least Frequently Used eviction algorithm
 * - **Priority Calculation**: Smart priority based on access patterns
 * - **Cache Persistence**: Save/load cache state to disk
 * - **Optimization**: Periodic cache optimization and defragmentation
 */
class CacheOptimizerPlugin : public PluginBase, public ICacheStrategyPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE
                          "cache_optimizer.json")
    Q_INTERFACES(IPluginInterface ICacheStrategyPlugin)

public:
    explicit CacheOptimizerPlugin(QObject* parent = nullptr);
    ~CacheOptimizerPlugin() override;

    void handleMessage(const QString& from, const QVariant& message) override;

protected:
    bool onInitialize() override;
    void onShutdown() override;

    // ICacheStrategyPlugin interface
    QString strategyName() const override;
    CacheEvictionStrategy evictionStrategy() const override;
    bool shouldCache(const QString& key, qint64 size,
                     const QVariantMap& metadata) const override;
    QString selectEvictionCandidate(const QList<CacheEntryMetadata>& entries,
                                    qint64 newEntrySize) const override;
    int calculatePriority(const CacheEntryMetadata& metadata) const override;
    int optimizeCache(qint64 currentSize, qint64 maxSize) override;
    bool persistCache(const QString& cachePath,
                      const QList<CacheEntryMetadata>& entries) override;
    QList<CacheEntryMetadata> loadCache(const QString& cachePath) override;

private:
    void registerHooks();
    void unregisterHooks();

    QVariant onCachePreAdd(const QVariantMap& context);
    QVariant onCachePreEvict(const QVariantMap& context);
    QVariant onCacheOptimize(const QVariantMap& context);

    // Configuration
    qint64 m_minSizeToCache;
    qint64 m_maxSizeToCache;
    int m_priorityBoostForRecent;
    int m_priorityDecayRate;

    // Statistics
    int m_cacheHits;
    int m_cacheMisses;
    int m_evictions;
    int m_optimizations;
};
