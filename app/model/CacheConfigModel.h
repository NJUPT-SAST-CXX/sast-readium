/**
 * @file CacheConfigModel.h
 * @brief Configuration model for cache settings
 * @author SAST Team
 * @version 2.0
 * @date 2024
 *
 * This file contains the CacheConfigModel class which manages cache
 * configuration settings including limits, thresholds, and policies.
 * Part of the MVP architecture refactoring.
 *
 * @copyright Copyright (c) 2024 SAST Team
 */

#pragma once

#include <QHash>
#include <QMutex>
#include <QString>
#include "cache/CacheTypes.h"

/**
 * @brief Configuration model for cache settings
 *
 * Manages all cache configuration including memory limits, entry limits,
 * eviction policies, and memory pressure thresholds. This is part of the
 * Model layer in the MVP architecture.
 */
class CacheConfigModel {
public:
    /**
     * @brief Constructs a new CacheConfigModel with default settings
     */
    CacheConfigModel();

    /**
     * @brief Destructor
     */
    ~CacheConfigModel() = default;

    // Global settings
    [[nodiscard]] qint64 getTotalMemoryLimit() const;
    void setTotalMemoryLimit(qint64 limit);

    [[nodiscard]] qint64 getCleanupInterval() const;
    void setCleanupInterval(qint64 interval);

    [[nodiscard]] double getMemoryPressureThreshold() const;
    void setMemoryPressureThreshold(double threshold);

    [[nodiscard]] double getMemoryPressureWarningThreshold() const;
    void setMemoryPressureWarningThreshold(double threshold);

    [[nodiscard]] double getMemoryPressureCriticalThreshold() const;
    void setMemoryPressureCriticalThreshold(double threshold);

    // Per-cache-type settings
    [[nodiscard]] qint64 getCacheLimit(CacheType type) const;
    void setCacheLimit(CacheType type, qint64 limit);

    [[nodiscard]] QString getEvictionStrategy(CacheType type) const;
    void setEvictionStrategy(CacheType type, const QString& strategy);

    [[nodiscard]] bool isCacheEnabled(CacheType type) const;
    void setCacheEnabled(CacheType type, bool enabled);

    // Feature flags
    [[nodiscard]] bool isLRUEvictionEnabled() const;
    void setLRUEvictionEnabled(bool enabled);

    [[nodiscard]] bool isMemoryPressureEvictionEnabled() const;
    void setMemoryPressureEvictionEnabled(bool enabled);

    [[nodiscard]] bool isCacheCoordinationEnabled() const;
    void setCacheCoordinationEnabled(bool enabled);

    [[nodiscard]] bool isAdaptiveMemoryManagementEnabled() const;
    void setAdaptiveMemoryManagementEnabled(bool enabled);

    [[nodiscard]] bool isCachePreloadingEnabled() const;
    void setCachePreloadingEnabled(bool enabled);

    [[nodiscard]] bool isSystemMemoryMonitoringEnabled() const;
    void setSystemMemoryMonitoringEnabled(bool enabled);

    [[nodiscard]] bool isPredictiveEvictionEnabled() const;
    void setPredictiveEvictionEnabled(bool enabled);

    [[nodiscard]] bool isMemoryCompressionEnabled() const;
    void setMemoryCompressionEnabled(bool enabled);

    [[nodiscard]] bool isEmergencyEvictionEnabled() const;
    void setEmergencyEvictionEnabled(bool enabled);

    // System memory settings
    [[nodiscard]] qint64 getSystemMemoryCheckInterval() const;
    void setSystemMemoryCheckInterval(qint64 interval);

    [[nodiscard]] double getSystemMemoryPressureThreshold() const;
    void setSystemMemoryPressureThreshold(double threshold);

    // Conversion to GlobalCacheConfig
    [[nodiscard]] GlobalCacheConfig toGlobalCacheConfig() const;
    void fromGlobalCacheConfig(const GlobalCacheConfig& config);

private:
    mutable QMutex m_mutex;

    // Global limits
    qint64 m_totalMemoryLimit;
    qint64 m_cleanupInterval;
    double m_memoryPressureThreshold;
    double m_memoryPressureWarningThreshold;
    double m_memoryPressureCriticalThreshold;

    // Per-type limits
    QHash<CacheType, qint64> m_cacheLimits;
    QHash<CacheType, QString> m_evictionStrategies;
    QHash<CacheType, bool> m_cacheEnabled;

    // Feature flags
    bool m_lruEvictionEnabled;
    bool m_memoryPressureEvictionEnabled;
    bool m_cacheCoordinationEnabled;
    bool m_adaptiveMemoryManagementEnabled;
    bool m_cachePreloadingEnabled;
    bool m_systemMemoryMonitoringEnabled;
    bool m_predictiveEvictionEnabled;
    bool m_memoryCompressionEnabled;
    bool m_emergencyEvictionEnabled;

    // System memory settings
    qint64 m_systemMemoryCheckInterval;
    double m_systemMemoryPressureThreshold;

    void initializeDefaults();
};
