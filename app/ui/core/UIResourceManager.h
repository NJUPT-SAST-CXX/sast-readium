#pragma once

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QSharedPointer>
#include <QTimer>
#include <QWeakPointer>
#include <QWidget>
#include <functional>
#include "../../logging/SimpleLogging.h"

/**
 * @brief Comprehensive UI resource management and cleanup system
 *
 * Manages UI resources including widgets, timers, animations, and memory
 * to prevent leaks and ensure proper cleanup. Provides automatic resource
 * tracking and cleanup on application shutdown.
 */
class UIResourceManager : public QObject {
    Q_OBJECT

public:
    enum class ResourceType {
        Widget,
        Timer,
        Animation,
        PixmapCache,
        StyleSheet,
        Connection,
        EventFilter,
        Other
    };

    struct ResourceInfo {
        ResourceType type;
        QObject* object;
        QString description;
        QDateTime created;
        qint64 memoryUsage;
        bool autoCleanup;

        ResourceInfo() = default;
        ResourceInfo(ResourceType t, QObject* obj, const QString& desc,
                     qint64 memory = 0, bool autoClean = true)
            : type(t),
              object(obj),
              description(desc),
              created(QDateTime::currentDateTime()),
              memoryUsage(memory),
              autoCleanup(autoClean) {}
    };

    static UIResourceManager& instance();

    // Resource registration
    void registerResource(QObject* object, ResourceType type,
                          const QString& description = QString(),
                          qint64 memoryUsage = 0, bool autoCleanup = true);
    void unregisterResource(QObject* object);

    // Widget lifecycle management
    void registerWidget(QWidget* widget,
                        const QString& description = QString());
    void scheduleWidgetCleanup(QWidget* widget, int delayMs = 0);
    void cleanupWidget(QWidget* widget);

    // Timer management
    QTimer* createManagedTimer(QObject* parent,
                               const QString& description = QString());
    void cleanupTimer(QTimer* timer);

    // Memory management
    void optimizeMemoryUsage();
    void clearPixmapCache();
    void clearStyleSheetCache();
    qint64 getTotalMemoryUsage() const;
    qint64 getResourceMemoryUsage(ResourceType type) const;

    // Resource monitoring
    int getResourceCount(ResourceType type = ResourceType::Widget) const;
    QList<ResourceInfo> getResourceList(
        ResourceType type = ResourceType::Widget) const;
    void dumpResourceInfo() const;

    // Cleanup operations
    void cleanupExpiredResources();
    void cleanupAllResources();
    void forceCleanupType(ResourceType type);

    // Configuration
    void setAutoCleanupEnabled(bool enabled) { m_autoCleanupEnabled = enabled; }
    void setMemoryThreshold(qint64 bytes) { m_memoryThreshold = bytes; }
    void setCleanupInterval(int ms);

    // Resource validation
    bool validateResources();
    QStringList findLeakedResources();
    void repairResourceReferences();

signals:
    void resourceRegistered(QObject* object, ResourceType type);
    void resourceUnregistered(QObject* object, ResourceType type);
    void memoryThresholdExceeded(qint64 currentUsage, qint64 threshold);
    void resourceLeakDetected(const QString& description);
    void cleanupCompleted(ResourceType type, int cleanedCount);

private slots:
    void onResourceDestroyed(QObject* object);
    void onCleanupTimer();
    void onMemoryPressure();

private:
    UIResourceManager();
    ~UIResourceManager() override;
    UIResourceManager(const UIResourceManager&) = delete;
    UIResourceManager& operator=(const UIResourceManager&) = delete;

    // Internal helpers
    void setupCleanupTimer();
    void checkMemoryUsage();
    qint64 calculateWidgetMemoryUsage(QWidget* widget);
    void cleanupResourceType(ResourceType type);

    // Data members
    QHash<QObject*, ResourceInfo> m_resources;
    QTimer* m_cleanupTimer;
    bool m_autoCleanupEnabled;
    qint64 m_memoryThreshold;
    int m_cleanupIntervalMs;

    SastLogging::CategoryLogger m_logger;
};

/**
 * @brief RAII helper for automatic resource cleanup
 */
class ResourceGuard {
public:
    explicit ResourceGuard(QObject* resource,
                           UIResourceManager::ResourceType type,
                           const QString& description = QString());
    ~ResourceGuard();

    void release();  // Release ownership without cleanup
    QObject* get() const { return m_resource; }

private:
    QObject* m_resource;
    UIResourceManager::ResourceType m_type;
    bool m_released;
};

/**
 * @brief Memory-aware widget factory with automatic cleanup
 */
class ManagedWidgetFactory {
public:
    template <typename T, typename... Args>
    static T* create(QWidget* parent, const QString& description,
                     Args&&... args) {
        T* widget = new T(std::forward<Args>(args)..., parent);
        UIResourceManager::instance().registerWidget(widget, description);
        return widget;
    }

    template <typename T>
    static void scheduleDestroy(T* widget, int delayMs = 0) {
        UIResourceManager::instance().scheduleWidgetCleanup(widget, delayMs);
    }
};

// Convenience macros
#define UI_RESOURCE_MANAGER UIResourceManager::instance()

#define REGISTER_UI_RESOURCE(object, type, description) \
    UI_RESOURCE_MANAGER.registerResource(               \
        object, UIResourceManager::ResourceType::type, description)

#define CREATE_MANAGED_WIDGET(Type, parent, description, ...) \
    ManagedWidgetFactory::create<Type>(parent, description, ##__VA_ARGS__)

#define RESOURCE_GUARD(object, type, description)                      \
    ResourceGuard guard(object, UIResourceManager::ResourceType::type, \
                        description)

#define CLEANUP_WIDGET(widget) UI_RESOURCE_MANAGER.cleanupWidget(widget)
