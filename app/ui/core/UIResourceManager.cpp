#include "UIResourceManager.h"
#include <QApplication>
#include <QPixmapCache>
#include <QStyleFactory>
#include <QThread>
#include <QTimer>
#include <QWidget>
#include "../../logging/LoggingMacros.h"

UIResourceManager::UIResourceManager()
    : QObject(nullptr),
      m_cleanupTimer(new QTimer(this)),
      m_autoCleanupEnabled(true),
      m_memoryThreshold(100 * 1024 * 1024),  // 100MB default
      m_cleanupIntervalMs(60000),            // 1 minute
      m_logger("UIResourceManager") {
    setupCleanupTimer();
    m_logger.info("UIResourceManager initialized");
}

UIResourceManager::~UIResourceManager() {
    cleanupAllResources();
    m_logger.debug("UIResourceManager destroyed");
}

UIResourceManager& UIResourceManager::instance() {
    static UIResourceManager instance;
    return instance;
}

void UIResourceManager::registerResource(QObject* object, ResourceType type,
                                         const QString& description,
                                         qint64 memoryUsage, bool autoCleanup) {
    if (!object) {
        m_logger.warning("Cannot register null resource");
        return;
    }

    // Calculate memory usage if not provided
    if (memoryUsage == 0 && type == ResourceType::Widget) {
        QWidget* widget = qobject_cast<QWidget*>(object);
        if (widget) {
            memoryUsage = calculateWidgetMemoryUsage(widget);
        }
    }

    ResourceInfo info(type, object, description, memoryUsage, autoCleanup);
    m_resources[object] = info;

    // Connect to destroyed signal for automatic cleanup
    connect(object, &QObject::destroyed, this,
            &UIResourceManager::onResourceDestroyed);

    emit resourceRegistered(object, type);

    m_logger.debug(
        QString("Resource registered: %1 (%2, %3 bytes)")
            .arg(description.isEmpty() ? object->objectName() : description)
            .arg(static_cast<int>(type))
            .arg(memoryUsage));

    // Check memory threshold
    checkMemoryUsage();
}

void UIResourceManager::unregisterResource(QObject* object) {
    if (!object)
        return;

    auto it = m_resources.find(object);
    if (it != m_resources.end()) {
        ResourceType type = it->type;
        QString description = it->description;

        disconnect(object, &QObject::destroyed, this,
                   &UIResourceManager::onResourceDestroyed);
        m_resources.erase(it);

        emit resourceUnregistered(object, type);

        m_logger.debug(QString("Resource unregistered: %1")
                           .arg(description.isEmpty() ? object->objectName()
                                                      : description));
    }
}

void UIResourceManager::registerWidget(QWidget* widget,
                                       const QString& description) {
    if (!widget)
        return;

    registerResource(widget, ResourceType::Widget, description);
}

void UIResourceManager::scheduleWidgetCleanup(QWidget* widget, int delayMs) {
    if (!widget)
        return;

    if (delayMs <= 0) {
        cleanupWidget(widget);
    } else {
        QTimer::singleShot(delayMs, this,
                           [this, widget]() { cleanupWidget(widget); });
    }
}

void UIResourceManager::cleanupWidget(QWidget* widget) {
    if (!widget)
        return;

    m_logger.debug(QString("Cleaning up widget: %1").arg(widget->objectName()));

    // Unregister first
    unregisterResource(widget);

    // Hide widget to prevent visual artifacts
    widget->hide();

    // Clear any references
    widget->setParent(nullptr);

    // Schedule deletion
    widget->deleteLater();
}

QTimer* UIResourceManager::createManagedTimer(QObject* parent,
                                              const QString& description) {
    QTimer* timer = new QTimer(parent);
    registerResource(timer, ResourceType::Timer, description);
    return timer;
}

void UIResourceManager::cleanupTimer(QTimer* timer) {
    if (!timer)
        return;

    timer->stop();
    unregisterResource(timer);
    timer->deleteLater();
}

void UIResourceManager::optimizeMemoryUsage() {
    m_logger.info("Optimizing memory usage...");

    // Clear various caches
    clearPixmapCache();
    clearStyleSheetCache();

    // Cleanup expired resources
    cleanupExpiredResources();

    // Force garbage collection if possible
    if (QThread::currentThread() == QApplication::instance()->thread()) {
        QApplication::processEvents();
    }

    qint64 currentUsage = getTotalMemoryUsage();
    m_logger.info(QString("Memory optimization completed. Current usage: %1 MB")
                      .arg(currentUsage / (1024 * 1024)));
}

void UIResourceManager::clearPixmapCache() {
    QPixmapCache::clear();
    m_logger.debug("Pixmap cache cleared");
}

void UIResourceManager::clearStyleSheetCache() {
    // Clear application stylesheet to force recompilation
    QApplication* app = qobject_cast<QApplication*>(QApplication::instance());
    if (app) {
        QString currentStyle = app->styleSheet();
        app->setStyleSheet("");
        app->setStyleSheet(currentStyle);
    }
    m_logger.debug("StyleSheet cache cleared");
}

qint64 UIResourceManager::getTotalMemoryUsage() const {
    qint64 total = 0;
    for (const auto& info : m_resources) {
        total += info.memoryUsage;
    }
    return total;
}

qint64 UIResourceManager::getResourceMemoryUsage(ResourceType type) const {
    qint64 total = 0;
    for (const auto& info : m_resources) {
        if (info.type == type) {
            total += info.memoryUsage;
        }
    }
    return total;
}

int UIResourceManager::getResourceCount(ResourceType type) const {
    int count = 0;
    for (const auto& info : m_resources) {
        if (info.type == type) {
            count++;
        }
    }
    return count;
}

void UIResourceManager::cleanupExpiredResources() {
    QDateTime cutoff =
        QDateTime::currentDateTime().addSecs(-3600);  // 1 hour ago

    auto it = m_resources.begin();
    int cleanedCount = 0;

    while (it != m_resources.end()) {
        if (it->autoCleanup && it->created < cutoff &&
            it->type != ResourceType::Widget) {  // Don't auto-cleanup widgets

            QObject* obj = it->object;
            it = m_resources.erase(it);

            if (obj) {
                obj->deleteLater();
            }
            cleanedCount++;
        } else {
            ++it;
        }
    }

    if (cleanedCount > 0) {
        m_logger.info(
            QString("Cleaned up %1 expired resources").arg(cleanedCount));
    }
}

void UIResourceManager::cleanupAllResources() {
    m_logger.info("Cleaning up all resources...");

    int totalCount = m_resources.size();

    // Cleanup in reverse order of creation
    QList<QObject*> objects;
    for (auto it = m_resources.begin(); it != m_resources.end(); ++it) {
        if (it->object && it->autoCleanup) {
            objects.append(it->object);
        }
    }

    // Sort by creation time (newest first)
    std::sort(objects.begin(), objects.end(), [this](QObject* a, QObject* b) {
        auto itA = m_resources.find(a);
        auto itB = m_resources.find(b);
        if (itA != m_resources.end() && itB != m_resources.end()) {
            return itA->created > itB->created;
        }
        return false;
    });

    // Cleanup objects
    for (QObject* obj : objects) {
        if (obj) {
            unregisterResource(obj);
            obj->deleteLater();
        }
    }

    m_resources.clear();

    m_logger.info(QString("Cleaned up %1 resources").arg(totalCount));
    emit cleanupCompleted(ResourceType::Other, totalCount);
}

// Private helper methods

void UIResourceManager::setupCleanupTimer() {
    m_cleanupTimer->setSingleShot(false);
    m_cleanupTimer->setInterval(m_cleanupIntervalMs);
    connect(m_cleanupTimer, &QTimer::timeout, this,
            &UIResourceManager::onCleanupTimer);

    if (m_autoCleanupEnabled) {
        m_cleanupTimer->start();
    }
}

void UIResourceManager::checkMemoryUsage() {
    qint64 currentUsage = getTotalMemoryUsage();
    if (currentUsage > m_memoryThreshold) {
        emit memoryThresholdExceeded(currentUsage, m_memoryThreshold);

        if (m_autoCleanupEnabled) {
            QTimer::singleShot(0, this, &UIResourceManager::onMemoryPressure);
        }
    }
}

qint64 UIResourceManager::calculateWidgetMemoryUsage(QWidget* widget) {
    if (!widget)
        return 0;

    // Rough estimation based on widget properties
    qint64 baseSize = sizeof(QWidget);

    // Add size for geometry
    baseSize += widget->size().width() * widget->size().height() *
                4;  // Assume 32-bit pixels

    // Add size for children
    const auto children =
        widget->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    baseSize += children.size() * sizeof(QWidget);

    // Add size for stylesheet
    if (!widget->styleSheet().isEmpty()) {
        baseSize += widget->styleSheet().size() * sizeof(QChar);
    }

    return baseSize;
}

// Slots

void UIResourceManager::onResourceDestroyed(QObject* object) {
    unregisterResource(object);
}

void UIResourceManager::onCleanupTimer() {
    if (m_autoCleanupEnabled) {
        cleanupExpiredResources();
        checkMemoryUsage();
    }
}

void UIResourceManager::onMemoryPressure() {
    m_logger.warning("Memory pressure detected, optimizing...");
    optimizeMemoryUsage();
}

// ResourceGuard Implementation

ResourceGuard::ResourceGuard(QObject* resource,
                             UIResourceManager::ResourceType type,
                             const QString& description)
    : m_resource(resource), m_type(type), m_released(false) {
    if (m_resource) {
        UIResourceManager::instance().registerResource(m_resource, m_type,
                                                       description);
    }
}

ResourceGuard::~ResourceGuard() {
    if (m_resource && !m_released) {
        UIResourceManager::instance().unregisterResource(m_resource);
        m_resource->deleteLater();
    }
}

void ResourceGuard::release() { m_released = true; }

// UIResourceManager configuration methods

void UIResourceManager::setCleanupInterval(int ms) {
    if (ms <= 0) {
        m_logger.warning(
            QString("Invalid cleanup interval: %1 ms (must be > 0)").arg(ms));
        return;
    }

    m_cleanupIntervalMs = ms;

    // Update timer if it's already running
    if (m_cleanupTimer->isActive()) {
        m_cleanupTimer->setInterval(ms);
        m_logger.info(QString("Cleanup interval updated to %1 ms").arg(ms));
    } else {
        m_logger.debug(
            QString("Cleanup interval set to %1 ms (timer not active)")
                .arg(ms));
    }
}
