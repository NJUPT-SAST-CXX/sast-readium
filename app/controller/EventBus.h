#pragma once

#include <QHash>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <functional>
#include <memory>
#include "../logging/SimpleLogging.h"

/**
 * @brief Event - Base class for all events in the system
 *
 * Events are immutable data carriers that represent something
 * that has happened in the system.
 */
class Event : public QObject {
    Q_OBJECT

public:
    explicit Event(QString type, QObject* parent = nullptr);
    ~Event() override = default;

    // Special member functions
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;
    Event(Event&&) = delete;
    Event& operator=(Event&&) = delete;

    // Event metadata
    [[nodiscard]] QString type() const { return m_type; }
    [[nodiscard]] qint64 timestamp() const { return m_timestamp; }
    [[nodiscard]] QString source() const { return m_source; }
    void setSource(const QString& source) { m_source = source; }

    // Event data
    [[nodiscard]] QVariant data() const { return m_data; }
    void setData(const QVariant& data) { m_data = data; }

    // Event properties
    [[nodiscard]] bool isHandled() const { return m_handled; }
    void setHandled(bool handled = true) { m_handled = handled; }

    [[nodiscard]] bool isPropagationStopped() const {
        return m_propagationStopped;
    }
    void stopPropagation() { m_propagationStopped = true; }

    // Clone event
    [[nodiscard]] virtual Event* clone() const;

protected:
    QString m_type;
    qint64 m_timestamp;
    QString m_source;
    QVariant m_data;
    bool m_handled = false;
    bool m_propagationStopped = false;
};

/**
 * @brief EventBus - Centralized event bus for publish-subscribe communication
 *
 * The EventBus provides a decoupled way for components to communicate
 * through events without direct dependencies.
 */
class EventBus : public QObject {
    Q_OBJECT

public:
    ~EventBus() override;

    // Singleton access
    static EventBus& instance();

    // Event subscription
    using EventHandler = std::function<void(Event*)>;

    void subscribe(const QString& eventType, QObject* subscriber,
                   EventHandler handler);
    void subscribe(const QString& eventType, QObject* subscriber,
                   const char* slot,
                   Qt::ConnectionType type = Qt::AutoConnection);

    void unsubscribe(const QString& eventType, QObject* subscriber);
    void unsubscribeAll(QObject* subscriber);

    // Event publishing
    void publish(Event* event);
    void publish(const QString& eventType, const QVariant& data = QVariant());
    void publishAsync(Event* event, int delayMs = 0);
    void publishAsync(const QString& eventType,
                      const QVariant& data = QVariant(), int delayMs = 0);

    // Event filtering
    using EventFilter = std::function<bool(Event*)>;
    void addFilter(const QString& eventType, EventFilter filter);
    void removeFilter(const QString& eventType);

    // Event queue management
    void processEventQueue();
    void clearEventQueue();
    int queueSize() const { return static_cast<int>(m_eventQueue.size()); }
    bool isProcessing() const { return m_isProcessing; }

    // Statistics
    int subscriberCount(const QString& eventType) const;
    QStringList subscribedEvents() const;
    qint64 totalEventsPublished() const { return m_totalEventsPublished; }
    qint64 totalEventsHandled() const { return m_totalEventsHandled; }
    void resetStatistics();

    // Configuration
    void setMaxQueueSize(int size) { m_maxQueueSize = size; }
    int maxQueueSize() const { return m_maxQueueSize; }
    void setAsyncProcessingEnabled(bool enabled) {
        m_asyncProcessingEnabled = enabled;
    }
    bool isAsyncProcessingEnabled() const { return m_asyncProcessingEnabled; }

signals:
    void eventPublished(const QString& eventType);
    void eventHandled(const QString& eventType);
    void subscriberAdded(const QString& eventType, QObject* subscriber);
    void subscriberRemoved(const QString& eventType, QObject* subscriber);
    void queueOverflow(int droppedEvents);

private slots:
    void processNextEvent();
    void onSubscriberDestroyed(QObject* obj);

private:
    explicit EventBus(QObject* parent = nullptr);

public:
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    EventBus(EventBus&&) = delete;
    EventBus& operator=(EventBus&&) = delete;

private:
    struct Subscription {
        QObject* subscriber;
        EventHandler handler;
        Qt::ConnectionType connectionType;
    };

    void deliverEvent(Event* event);
    bool applyFilters(Event* event);
    void cleanupSubscriptions();

    // Subscriptions
    QHash<QString, QList<Subscription>> m_subscriptions;
    QHash<QString, EventFilter> m_filters;

    // Event queue
    QList<Event*> m_eventQueue;
    QTimer* m_processTimer;
    int m_maxQueueSize = 1000;
    bool m_asyncProcessingEnabled = true;
    bool m_isProcessing = false;

    // Statistics
    qint64 m_totalEventsPublished = 0;
    qint64 m_totalEventsHandled = 0;

    // Overflow batching
    bool m_overflowEmitted = false;
    int m_totalDropped = 0;
    QTimer* m_overflowTimer;

    // Thread safety
    mutable QMutex m_mutex;

    // Logging
    SastLogging::CategoryLogger m_logger{"EventBus"};
};

/**
 * @brief TypedEvent - Template class for type-safe events
 */
template <typename T>
class TypedEvent : public Event {
public:
    explicit TypedEvent(const QString& type, const T& payload,
                        QObject* parent = nullptr)
        : Event(type, parent), m_payload(payload) {
        setData(QVariant::fromValue(payload));
    }

    T payload() const { return m_payload; }

    [[nodiscard]] Event* clone() const override {
        return new TypedEvent<T>(type(), m_payload);
    }

private:
    T m_payload;
};

/**
 * @brief EventEmitter - Mixin class for objects that emit events
 */
class EventEmitter {
public:
    virtual ~EventEmitter() = default;

    // Special member functions
    EventEmitter(const EventEmitter&) = delete;
    EventEmitter& operator=(const EventEmitter&) = delete;
    EventEmitter(EventEmitter&&) = delete;
    EventEmitter& operator=(EventEmitter&&) = delete;

protected:
    void emitEvent(Event* event) {
        event->setSource(objectName());
        EventBus::instance().publish(event);
    }

    void emitEvent(const QString& eventType,
                   const QVariant& data = QVariant()) {
        EventBus::instance().publish(eventType, data);
    }

    void emitAsync(Event* event, int delayMs = 0) {
        event->setSource(objectName());
        EventBus::instance().publishAsync(event, delayMs);
    }

    [[nodiscard]] virtual QString objectName() const = 0;
};

/**
 * @brief EventSubscriber - Base class for objects that subscribe to events
 */
class EventSubscriber : public QObject {
    Q_OBJECT

public:
    explicit EventSubscriber(QObject* parent = nullptr);
    ~EventSubscriber() override;

    // Special member functions
    EventSubscriber(const EventSubscriber&) = delete;
    EventSubscriber& operator=(const EventSubscriber&) = delete;
    EventSubscriber(EventSubscriber&&) = delete;
    EventSubscriber& operator=(EventSubscriber&&) = delete;

protected:
    void subscribeTo(const QString& eventType, EventBus::EventHandler handler);
    void subscribeTo(const QString& eventType, const char* slot);
    void unsubscribeFrom(const QString& eventType);
    void unsubscribeFromAll();

    virtual void handleEvent(Event* event);

private:
    QStringList m_subscribedEvents;
};

/**
 * @brief EventAggregator - Aggregates multiple events into one
 */
class EventAggregator : public QObject {
    Q_OBJECT

public:
    explicit EventAggregator(const QStringList& eventTypes,
                             int timeWindowMs = 1000,
                             QObject* parent = nullptr);
    ~EventAggregator() override;

    // Special member functions
    EventAggregator(const EventAggregator&) = delete;
    EventAggregator& operator=(const EventAggregator&) = delete;
    EventAggregator(EventAggregator&&) = delete;
    EventAggregator& operator=(EventAggregator&&) = delete;

    void start();
    void stop();
    [[nodiscard]] bool isRunning() const { return m_isRunning; }

    // Configuration
    void setTimeWindow(int ms) { m_timeWindowMs = ms; }
    [[nodiscard]] int timeWindow() const { return m_timeWindowMs; }
    void setAggregationFunction(
        std::function<QVariant(const QList<Event*>&)> func) {
        m_aggregationFunction = std::move(func);
    }

signals:
    void aggregatedEvent(const QString& type, const QVariant& data);

private slots:
    void onEventReceived(Event* event);
    void onTimeWindowExpired();

private:
    QStringList m_eventTypes;
    int m_timeWindowMs;
    bool m_isRunning = false;
    QList<Event*> m_bufferedEvents;
    QTimer* m_windowTimer;
    std::function<QVariant(const QList<Event*>&)> m_aggregationFunction;
};

/**
 * @brief Common application events
 */
namespace AppEvents {
// Document events
inline const QString&
DOCUMENT_OPENED() {  // NOLINT(readability-identifier-naming)
    static const QString kValue =
        "document.opened";  // NOLINT(readability-identifier-naming)
    return kValue;
}
inline const QString&
DOCUMENT_CLOSED() {  // NOLINT(readability-identifier-naming)
    static const QString kValue =
        "document.closed";  // NOLINT(readability-identifier-naming)
    return kValue;
}
inline const QString&
DOCUMENT_SAVED() {  // NOLINT(readability-identifier-naming)
    static const QString kValue =
        "document.saved";  // NOLINT(readability-identifier-naming)
    return kValue;
}
inline const QString&
DOCUMENT_MODIFIED() {  // NOLINT(readability-identifier-naming)
    static const QString kValue =
        "document.modified";  // NOLINT(readability-identifier-naming)
    return kValue;
}

// Navigation events
inline const QString& PAGE_CHANGED() {  // NOLINT(readability-identifier-naming)
    static const QString kValue =
        "navigation.page_changed";  // NOLINT(readability-identifier-naming)
    return kValue;
}
inline const QString& ZOOM_CHANGED() {  // NOLINT(readability-identifier-naming)
    static const QString kValue =
        "navigation.zoom_changed";  // NOLINT(readability-identifier-naming)
    return kValue;
}
inline const QString&
VIEW_MODE_CHANGED() {  // NOLINT(readability-identifier-naming)
    static const QString kValue =
        "navigation.view_mode_changed";  // NOLINT(readability-identifier-naming)
    return kValue;
}

// UI events
inline const QString&
THEME_CHANGED() {  // NOLINT(readability-identifier-naming)
    static const QString kValue =
        "ui.theme_changed";  // NOLINT(readability-identifier-naming)
    return kValue;
}
inline const QString&
LAYOUT_CHANGED() {  // NOLINT(readability-identifier-naming)
    static const QString kValue =
        "ui.layout_changed";  // NOLINT(readability-identifier-naming)
    return kValue;
}
inline const QString&
SIDEBAR_TOGGLED() {  // NOLINT(readability-identifier-naming)
    static const QString kValue =
        "ui.sidebar_toggled";  // NOLINT(readability-identifier-naming)
    return kValue;
}

// System events
inline const QString&
APPLICATION_READY() {  // NOLINT(readability-identifier-naming)
    static const QString kValue =
        "system.application_ready";  // NOLINT(readability-identifier-naming)
    return kValue;
}
inline const QString&
SHUTDOWN_REQUESTED() {  // NOLINT(readability-identifier-naming)
    static const QString kValue =
        "system.shutdown_requested";  // NOLINT(readability-identifier-naming)
    return kValue;
}
inline const QString&
ERROR_OCCURRED() {  // NOLINT(readability-identifier-naming)
    static const QString kValue =
        "system.error_occurred";  // NOLINT(readability-identifier-naming)
    return kValue;
}
}  // namespace AppEvents

// Convenience macros
#define PUBLISH_EVENT(type, data) EventBus::instance().publish(type, data)
#define SUBSCRIBE_EVENT(type, handler) \
    EventBus::instance().subscribe(type, this, handler)
#define UNSUBSCRIBE_EVENT(type) EventBus::instance().unsubscribe(type, this)
