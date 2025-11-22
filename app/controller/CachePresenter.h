/**
 * @file CachePresenter.h
 * @brief Main presenter for cache coordination in MVP architecture
 * @author SAST Team
 * @version 2.0
 * @date 2024
 *
 * This file contains the CachePresenter class which coordinates between
 * cache models and views, handling business logic for cache operations,
 * eviction policies, and memory management.
 *
 * @copyright Copyright (c) 2024 SAST Team
 */

#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <memory>
#include "cache/CacheTypes.h"
#include "model/CacheConfigModel.h"
#include "model/CacheDataModel.h"
#include "model/CacheStatsModel.h"
#include "view/ICacheView.h"

/**
 * @brief Main presenter for cache coordination
 *
 * Coordinates between cache models (data, config, stats) and views,
 * implementing business logic for cache operations, eviction policies,
 * and memory management. This is the Presenter layer in the MVP architecture.
 */
class CachePresenter : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructs a new CachePresenter
     * @param parent Parent QObject
     */
    explicit CachePresenter(QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~CachePresenter() override;

    // Disable copy and move
    CachePresenter(const CachePresenter&) = delete;
    CachePresenter& operator=(const CachePresenter&) = delete;
    CachePresenter(CachePresenter&&) = delete;
    CachePresenter& operator=(CachePresenter&&) = delete;

    // View registration
    void registerView(ICacheView* view);
    void registerStatsView(ICacheStatsView* view);
    void registerConfigView(ICacheConfigView* view);
    void registerMemoryView(ICacheMemoryView* view);

    void unregisterView(ICacheView* view);
    void unregisterStatsView(ICacheStatsView* view);
    void unregisterConfigView(ICacheConfigView* view);
    void unregisterMemoryView(ICacheMemoryView* view);

    // Model access
    [[nodiscard]] CacheDataModel* getDataModel() const;
    [[nodiscard]] CacheConfigModel* getConfigModel() const;
    [[nodiscard]] CacheStatsModel* getStatsModel() const;

    // Cache operations
    bool insert(const QString& key, const QVariant& data, CacheType type,
                int priority = 1);
    [[nodiscard]] QVariant get(const QString& key, CacheType type);
    bool contains(const QString& key, CacheType type) const;
    bool remove(const QString& key, CacheType type);
    void clear(CacheType type);
    void clearAll();

    // Memory management
    void enforceMemoryLimits();
    void handleMemoryPressure();
    [[nodiscard]] qint64 getTotalMemoryUsage() const;
    [[nodiscard]] double getMemoryUsageRatio() const;

    // Statistics
    [[nodiscard]] CacheStats getStats(CacheType type) const;
    [[nodiscard]] QHash<CacheType, CacheStats> getAllStats() const;
    [[nodiscard]] double getGlobalHitRatio() const;

    // Configuration
    void setGlobalConfig(const GlobalCacheConfig& config);
    [[nodiscard]] GlobalCacheConfig getGlobalConfig() const;
    void setCacheLimit(CacheType type, qint64 limit);
    [[nodiscard]] qint64 getCacheLimit(CacheType type) const;

    // Eviction policies
    void evictLRU(CacheType type, qint64 bytesToFree);
    void evictExpired(qint64 maxAge);
    void performAdaptiveEviction();

signals:
    // Emitted for EventBus integration
    void cacheHit(CacheType type, const QString& key);
    void cacheMiss(CacheType type, const QString& key);
    void cacheEvictionOccurred(CacheType type, qint64 bytesFreed);
    void memoryPressureWarning(double usageRatio);
    void memoryPressureCritical(double usageRatio);

private:
    class Implementation;
    std::unique_ptr<Implementation> m_impl;

    // Internal methods
    void notifyViews(CacheType type, const QString& key);
    void notifyStatsViews();
    void notifyConfigViews(CacheType type);
    void notifyMemoryViews(qint64 usage, qint64 limit);
    void checkMemoryPressure();
};
