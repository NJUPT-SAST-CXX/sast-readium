#include "EventBus.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QMetaObject>

// Event implementation
Event::Event(QString type, QObject* parent)
    : QObject(parent),
      m_type(std::move(type)),
      m_timestamp(QDateTime::currentMSecsSinceEpoch()) {}

Event* Event::clone() const {
    Event* cloned = new Event(m_type);
    cloned->m_timestamp = m_timestamp;
    cloned->m_source = m_source;
    cloned->m_data = m_data;
    cloned->m_handled = m_handled;
    cloned->m_propagationStopped = m_propagationStopped;
    return cloned;
}

// EventBus implementation
EventBus::EventBus(QObject* parent)
    : QObject(parent),
      m_processTimer(new QTimer(this)),
      m_overflowTimer(new QTimer(this)),
      m_logger("EventBus") {
    m_processTimer->setSingleShot(true);
    connect(m_processTimer, &QTimer::timeout, this,
            &EventBus::processNextEvent);

    m_overflowTimer->setSingleShot(true);
    connect(m_overflowTimer, &QTimer::timeout, this, [this]() {
        if (m_totalDropped > 0) {
            emit queueOverflow(m_totalDropped);
            m_logger.warning(QString("Event queue overflow, dropped %1 events")
                                 .arg(m_totalDropped));
            m_totalDropped = 0;
        }
        m_overflowEmitted = false;
    });
}

EventBus::~EventBus() {
    // During static destruction, QCoreApplication might already be destroyed
    // In that case, we can't safely do anything, so just return
    if (!QCoreApplication::instance()) {
        return;
    }

    clearEventQueue();
}

EventBus& EventBus::instance() {
    static EventBus instance;
    return instance;
}

void EventBus::subscribe(const QString& eventType, QObject* subscriber,
                         EventHandler handler) {
    QMutexLocker locker(&m_mutex);

    if (!subscriber || !handler) {
        m_logger.warning("Invalid subscriber or handler for event type: " +
                         eventType);
        return;
    }

    Subscription subscription;
    subscription.subscriber = subscriber;
    subscription.handler = handler;
    subscription.connectionType = Qt::AutoConnection;

    m_subscriptions[eventType].append(subscription);

    // Connect to subscriber's destroyed signal for cleanup
    connect(subscriber, &QObject::destroyed, this,
            &EventBus::onSubscriberDestroyed);

    emit subscriberAdded(eventType, subscriber);
    m_logger.debug("Subscriber added for event type: " + eventType);
}

void EventBus::subscribe(const QString& eventType, QObject* subscriber,
                         const char* slot, Qt::ConnectionType type) {
    if (!subscriber || !slot) {
        m_logger.warning("Invalid subscriber or slot for event type: " +
                         eventType);
        return;
    }

    // Create a handler that invokes the slot
    EventHandler handler = [subscriber, slot](Event* event) {
        QMetaObject::invokeMethod(subscriber, slot, Qt::AutoConnection,
                                  Q_ARG(Event*, event));
    };

    subscribe(eventType, subscriber, handler);
}

void EventBus::unsubscribe(const QString& eventType, QObject* subscriber) {
    QMutexLocker locker(&m_mutex);

    if (!m_subscriptions.contains(eventType)) {
        return;
    }

    auto& subscriptions = m_subscriptions[eventType];
    subscriptions.removeIf([subscriber](const Subscription& sub) {
        return sub.subscriber == subscriber;
    });

    if (subscriptions.isEmpty()) {
        m_subscriptions.remove(eventType);
    }

    emit subscriberRemoved(eventType, subscriber);
    m_logger.debug("Subscriber removed for event type: " + eventType);
}

void EventBus::unsubscribeAll(QObject* subscriber) {
    QMutexLocker locker(&m_mutex);

    QStringList eventTypes = m_subscriptions.keys();
    for (const QString& eventType : eventTypes) {
        auto& subscriptions = m_subscriptions[eventType];
        subscriptions.removeIf([subscriber](const Subscription& sub) {
            return sub.subscriber == subscriber;
        });

        if (subscriptions.isEmpty()) {
            m_subscriptions.remove(eventType);
        }
    }

    m_logger.debug("All subscriptions removed for subscriber");
}

void EventBus::publish(Event* event) {
    if (!event) {
        m_logger.warning("Attempted to publish null event");
        return;
    }

    QString eventType;
    bool shouldEmitSignal = false;

    // DEADLOCK FIX: Minimize critical section and emit signal outside lock
    {
        QMutexLocker locker(&m_mutex);

        // Apply filters
        if (!applyFilters(event)) {
            m_logger.debug("Event filtered out: " + event->type());
            return;
        }

        m_totalEventsPublished++;
        eventType = event->type();
        shouldEmitSignal = true;

        if (m_asyncProcessingEnabled) {
            // Add to queue for async processing
            m_eventQueue.append(event);

            if (!m_processTimer->isActive()) {
                m_processTimer->start(0);
            }
        } else {
            // Process immediately (still under lock for now)
            deliverEvent(event);
            delete event;
        }
    }

    // Emit signal outside mutex to prevent deadlock from re-entrant calls
    if (shouldEmitSignal) {
        emit eventPublished(eventType);
    }
}

void EventBus::publish(const QString& eventType, const QVariant& data) {
    Event* event = new Event(eventType);
    event->setData(data);
    publish(event);
}

void EventBus::publishAsync(Event* event, int delayMs) {
    if (!event) {
        m_logger.warning("Attempted to publish null event async");
        return;
    }

    QMutexLocker locker(&m_mutex);

    // Add event to queue
    m_eventQueue.append(event);
    m_totalEventsPublished++;

    // Check if queue exceeds capacity and emit overflow only once per batch
    if (m_eventQueue.size() > m_maxQueueSize) {
        int eventsToDrop = m_eventQueue.size() - m_maxQueueSize;
        for (int i = 0; i < eventsToDrop; ++i) {
            delete m_eventQueue.takeFirst();
        }
        m_totalDropped += eventsToDrop;

        if (!m_overflowEmitted) {
            // Use a timer to emit overflow after a short delay to batch
            // multiple calls
            m_overflowTimer->start(1);
            m_overflowEmitted = true;
        }
    }

    locker.unlock();

    // Schedule processing
    if (delayMs > 0) {
        QTimer::singleShot(delayMs, [this]() {
            if (!m_eventQueue.isEmpty()) {
                m_processTimer->start(0);
            }
        });
    } else {
        m_processTimer->start(0);
    }

    emit eventPublished(event->type());
}

void EventBus::publishAsync(const QString& eventType, const QVariant& data,
                            int delayMs) {
    Event* event = new Event(eventType);
    event->setData(data);
    publishAsync(event, delayMs);
}

void EventBus::addFilter(const QString& eventType, EventFilter filter) {
    QMutexLocker locker(&m_mutex);
    m_filters[eventType] = filter;
    m_logger.debug("Filter added for event type: " + eventType);
}

void EventBus::removeFilter(const QString& eventType) {
    QMutexLocker locker(&m_mutex);
    m_filters.remove(eventType);
    m_logger.debug("Filter removed for event type: " + eventType);
}

void EventBus::processEventQueue() {
    QList<Event*> eventsToProcess;

    // DEADLOCK FIX: Take events from queue under lock, process outside lock
    {
        QMutexLocker locker(&m_mutex);
        eventsToProcess = m_eventQueue;
        m_eventQueue.clear();
    }

    // Process events outside mutex to prevent deadlock from re-entrant calls
    for (Event* event : eventsToProcess) {
        deliverEvent(event);
        delete event;
    }
}

void EventBus::clearEventQueue() {
    QMutexLocker locker(&m_mutex);

    qDeleteAll(m_eventQueue);
    m_eventQueue.clear();
    m_logger.debug("Event queue cleared");
}

int EventBus::subscriberCount(const QString& eventType) const {
    QMutexLocker locker(&m_mutex);
    return m_subscriptions.value(eventType).size();
}

QStringList EventBus::subscribedEvents() const {
    QMutexLocker locker(&m_mutex);
    return m_subscriptions.keys();
}

void EventBus::resetStatistics() {
    QMutexLocker locker(&m_mutex);
    m_totalEventsPublished = 0;
    m_totalEventsHandled = 0;
}

void EventBus::processNextEvent() {
    QMutexLocker locker(&m_mutex);

    if (m_eventQueue.isEmpty() || m_isProcessing) {
        return;
    }

    m_isProcessing = true;
    Event* event = m_eventQueue.takeFirst();

    locker.unlock();

    deliverEvent(event);
    delete event;

    locker.relock();
    m_isProcessing = false;

    // Schedule next event if queue is not empty
    if (!m_eventQueue.isEmpty()) {
        m_processTimer->start(0);
    }
}

void EventBus::onSubscriberDestroyed(QObject* obj) { unsubscribeAll(obj); }

void EventBus::deliverEvent(Event* event) {
    if (!event) {
        return;
    }

    const QString& eventType = event->type();

    QMutexLocker locker(&m_mutex);
    if (!m_subscriptions.contains(eventType)) {
        return;
    }

    auto subscriptions =
        m_subscriptions[eventType];  // Copy to avoid issues with modifications
    locker.unlock();

    bool eventWasHandled = false;
    for (const Subscription& subscription : subscriptions) {
        if (subscription.subscriber && subscription.handler) {
            try {
                subscription.handler(event);
                eventWasHandled = true;

                if (event->isPropagationStopped()) {
                    break;
                }
            } catch (const std::exception& e) {
                m_logger.error("Exception in event handler: " +
                               QString::fromStdString(e.what()));
            } catch (...) {
                m_logger.error("Unknown exception in event handler");
            }
        }
    }

    if (eventWasHandled) {
        m_totalEventsHandled++;
    }

    emit eventHandled(eventType);
}

bool EventBus::applyFilters(Event* event) {
    if (!event) {
        return false;
    }

    const QString& eventType = event->type();
    if (!m_filters.contains(eventType)) {
        return true;  // No filter means allow
    }

    try {
        return m_filters[eventType](event);
    } catch (const std::exception& e) {
        m_logger.error("Exception in event filter: " +
                       QString::fromStdString(e.what()));
        return false;
    } catch (...) {
        m_logger.error("Unknown exception in event filter");
        return false;
    }
}

void EventBus::cleanupSubscriptions() {
    QMutexLocker locker(&m_mutex);

    QStringList eventTypes = m_subscriptions.keys();
    for (const QString& eventType : eventTypes) {
        auto& subscriptions = m_subscriptions[eventType];
        subscriptions.removeIf(
            [](const Subscription& sub) { return !sub.subscriber; });

        if (subscriptions.isEmpty()) {
            m_subscriptions.remove(eventType);
        }
    }
}

// EventSubscriber implementation
EventSubscriber::EventSubscriber(QObject* parent) : QObject(parent) {}

EventSubscriber::~EventSubscriber() { unsubscribeFromAll(); }

void EventSubscriber::subscribeTo(const QString& eventType,
                                  EventBus::EventHandler handler) {
    EventBus::instance().subscribe(eventType, this, handler);
    if (!m_subscribedEvents.contains(eventType)) {
        m_subscribedEvents.append(eventType);
    }
}

void EventSubscriber::subscribeTo(const QString& eventType, const char* slot) {
    EventBus::instance().subscribe(eventType, this, slot);
    if (!m_subscribedEvents.contains(eventType)) {
        m_subscribedEvents.append(eventType);
    }
}

void EventSubscriber::unsubscribeFrom(const QString& eventType) {
    EventBus::instance().unsubscribe(eventType, this);
    m_subscribedEvents.removeAll(eventType);
}

void EventSubscriber::unsubscribeFromAll() {
    for (const QString& eventType : m_subscribedEvents) {
        EventBus::instance().unsubscribe(eventType, this);
    }
    m_subscribedEvents.clear();
}

void EventSubscriber::handleEvent(Event* event) {
    // Default implementation - can be overridden by subclasses
    Q_UNUSED(event)
}

// EventAggregator implementation
EventAggregator::EventAggregator(const QStringList& eventTypes,
                                 int timeWindowMs, QObject* parent)
    : QObject(parent),
      m_eventTypes(eventTypes),
      m_timeWindowMs(timeWindowMs),
      m_windowTimer(new QTimer(this)) {
    m_windowTimer->setSingleShot(true);
    connect(m_windowTimer, &QTimer::timeout, this,
            &EventAggregator::onTimeWindowExpired);

    // Default aggregation function - just collect all events
    m_aggregationFunction = [](const QList<Event*>& events) -> QVariant {
        QVariantList result;
        for (Event* event : events) {
            result.append(event->data());
        }
        return result;
    };
}

EventAggregator::~EventAggregator() {
    stop();
    qDeleteAll(m_bufferedEvents);
}

void EventAggregator::start() {
    if (m_isRunning) {
        return;
    }

    m_isRunning = true;

    // Subscribe to all specified event types
    for (const QString& eventType : m_eventTypes) {
        EventBus::instance().subscribe(
            eventType, this, [this](Event* event) { onEventReceived(event); });
    }
}

void EventAggregator::stop() {
    if (!m_isRunning) {
        return;
    }

    m_isRunning = false;
    m_windowTimer->stop();

    // Unsubscribe from all events
    for (const QString& eventType : m_eventTypes) {
        EventBus::instance().unsubscribe(eventType, this);
    }

    // Clear buffered events
    qDeleteAll(m_bufferedEvents);
    m_bufferedEvents.clear();
}

void EventAggregator::onEventReceived(Event* event) {
    if (!m_isRunning) {
        return;
    }

    // Clone the event to avoid ownership issues
    Event* clonedEvent = event->clone();
    m_bufferedEvents.append(clonedEvent);

    // Start or restart the time window
    m_windowTimer->start(m_timeWindowMs);
}

void EventAggregator::onTimeWindowExpired() {
    if (m_bufferedEvents.isEmpty()) {
        return;
    }

    // Apply aggregation function
    QVariant aggregatedData = m_aggregationFunction(m_bufferedEvents);

    // Emit aggregated event
    QString aggregatedType = "aggregated." + m_eventTypes.join(".");
    emit aggregatedEvent(aggregatedType, aggregatedData);

    // Clear buffered events
    qDeleteAll(m_bufferedEvents);
    m_bufferedEvents.clear();
}
