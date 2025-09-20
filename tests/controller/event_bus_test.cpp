#include <QTest>
#include <QSignalSpy>
#include <QTimer>
#include <QEventLoop>
#include <QApplication>
#include "../../app/controller/EventBus.h"
#include "../TestUtilities.h"

// Test event classes
class TestEvent : public Event {
    Q_OBJECT
public:
    explicit TestEvent(const QString& message, QObject* parent = nullptr)
        : Event("test.event", parent), m_message(message) {
        setData(QVariant(message));
    }
    
    QString message() const { return m_message; }
    
    Event* clone() const override {
        return new TestEvent(m_message);
    }
    
private:
    QString m_message;
};

class TestSubscriber : public EventSubscriber {
    Q_OBJECT
public:
    explicit TestSubscriber(QObject* parent = nullptr) : EventSubscriber(parent) {}
    
    void subscribeToTestEvents() {
        subscribeTo("test.event", [this](Event* event) {
            m_receivedEvents.append(event->clone());
        });
    }
    
    QList<Event*> receivedEvents() const { return m_receivedEvents; }
    void clearReceivedEvents() { 
        qDeleteAll(m_receivedEvents);
        m_receivedEvents.clear(); 
    }
    
protected:
    void handleEvent(Event* event) override {
        m_handledEvents.append(event->clone());
    }
    
public:
    QList<Event*> m_receivedEvents;
    QList<Event*> m_handledEvents;
};

class EventBusTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override {
        // Ensure QApplication exists
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            new QApplication(argc, argv);
        }
    }
    
    void init() override {
        // Clear event bus state before each test
        EventBus::instance().clearEventQueue();
        m_testSubscriber = new TestSubscriber(this);
    }
    
    void cleanup() override {
        // Clean up after each test
        EventBus::instance().unsubscribeAll(m_testSubscriber);
        EventBus::instance().clearEventQueue();
        delete m_testSubscriber;
        m_testSubscriber = nullptr;
    }
    
    // Singleton tests
    void testSingletonInstance() {
        EventBus& instance1 = EventBus::instance();
        EventBus& instance2 = EventBus::instance();
        
        // Should be the same instance
        QVERIFY(&instance1 == &instance2);
    }
    
    // Basic event publishing and subscription
    void testBasicEventPublishing() {
        EventBus& eventBus = EventBus::instance();
        
        QSignalSpy publishedSpy(&eventBus, &EventBus::eventPublished);
        QSignalSpy handledSpy(&eventBus, &EventBus::eventHandled);
        
        bool eventReceived = false;
        QString receivedMessage;
        
        // Subscribe to test events
        eventBus.subscribe("test.event", this, [&](Event* event) {
            eventReceived = true;
            receivedMessage = event->data().toString();
        });
        
        // Publish an event
        TestEvent* testEvent = new TestEvent("Hello World");
        eventBus.publish(testEvent);
        
        // Process events
        QCoreApplication::processEvents();
        
        // Verify event was received
        QVERIFY(eventReceived);
        QCOMPARE(receivedMessage, QString("Hello World"));
        QCOMPARE(publishedSpy.count(), 1);
        QCOMPARE(handledSpy.count(), 1);
    }
    
    void testEventPublishingWithData() {
        EventBus& eventBus = EventBus::instance();
        
        bool eventReceived = false;
        QVariant receivedData;
        
        // Subscribe to test events
        eventBus.subscribe("data.event", this, [&](Event* event) {
            eventReceived = true;
            receivedData = event->data();
        });
        
        // Publish event with data
        eventBus.publish("data.event", QVariant("test_data"));
        
        // Process events
        QCoreApplication::processEvents();
        
        // Verify event was received
        QVERIFY(eventReceived);
        QCOMPARE(receivedData.toString(), QString("test_data"));
    }
    
    // Subscription management
    void testSubscriptionManagement() {
        EventBus& eventBus = EventBus::instance();
        
        QSignalSpy subscriberAddedSpy(&eventBus, &EventBus::subscriberAdded);
        QSignalSpy subscriberRemovedSpy(&eventBus, &EventBus::subscriberRemoved);
        
        // Subscribe to events
        eventBus.subscribe("manage.event", this, [](Event*) {});
        QCOMPARE(subscriberAddedSpy.count(), 1);
        QCOMPARE(eventBus.subscriberCount("manage.event"), 1);
        
        // Unsubscribe from events
        eventBus.unsubscribe("manage.event", this);
        QCOMPARE(subscriberRemovedSpy.count(), 1);
        QCOMPARE(eventBus.subscriberCount("manage.event"), 0);
    }
    
    void testMultipleSubscribers() {
        EventBus& eventBus = EventBus::instance();
        
        int subscriber1Count = 0;
        int subscriber2Count = 0;
        
        // Multiple subscribers to same event
        eventBus.subscribe("multi.event", this, [&](Event*) { subscriber1Count++; });
        
        TestSubscriber* subscriber2 = new TestSubscriber(this);
        eventBus.subscribe("multi.event", subscriber2, [&](Event*) { subscriber2Count++; });
        
        QCOMPARE(eventBus.subscriberCount("multi.event"), 2);
        
        // Publish event
        eventBus.publish("multi.event");
        QCoreApplication::processEvents();
        
        // Both subscribers should receive the event
        QCOMPARE(subscriber1Count, 1);
        QCOMPARE(subscriber2Count, 1);
        
        // Unsubscribe one
        eventBus.unsubscribe("multi.event", this);
        QCOMPARE(eventBus.subscriberCount("multi.event"), 1);
        
        // Publish again
        eventBus.publish("multi.event");
        QCoreApplication::processEvents();
        
        // Only subscriber2 should receive the event
        QCOMPARE(subscriber1Count, 1);
        QCOMPARE(subscriber2Count, 2);
    }
    
    void testUnsubscribeAll() {
        EventBus& eventBus = EventBus::instance();
        
        // Subscribe to multiple events
        eventBus.subscribe("event1", this, [](Event*) {});
        eventBus.subscribe("event2", this, [](Event*) {});
        eventBus.subscribe("event3", this, [](Event*) {});
        
        QCOMPARE(eventBus.subscriberCount("event1"), 1);
        QCOMPARE(eventBus.subscriberCount("event2"), 1);
        QCOMPARE(eventBus.subscriberCount("event3"), 1);
        
        // Unsubscribe from all
        eventBus.unsubscribeAll(this);
        
        QCOMPARE(eventBus.subscriberCount("event1"), 0);
        QCOMPARE(eventBus.subscriberCount("event2"), 0);
        QCOMPARE(eventBus.subscriberCount("event3"), 0);
    }
    
    // Event filtering
    void testEventFiltering() {
        EventBus& eventBus = EventBus::instance();
        
        bool eventReceived = false;
        
        // Add filter that blocks events containing "blocked"
        eventBus.addFilter("filter.event", [](Event* event) {
            return !event->data().toString().contains("blocked");
        });
        
        // Subscribe to events
        eventBus.subscribe("filter.event", this, [&](Event*) {
            eventReceived = true;
        });
        
        // Publish blocked event
        eventBus.publish("filter.event", QVariant("blocked_message"));
        QCoreApplication::processEvents();
        QVERIFY(!eventReceived);
        
        // Publish allowed event
        eventBus.publish("filter.event", QVariant("allowed_message"));
        QCoreApplication::processEvents();
        QVERIFY(eventReceived);
        
        // Remove filter
        eventBus.removeFilter("filter.event");
        eventReceived = false;
        
        // Publish blocked event again (should now pass)
        eventBus.publish("filter.event", QVariant("blocked_message"));
        QCoreApplication::processEvents();
        QVERIFY(eventReceived);
    }
    
    // Async event processing
    void testAsyncEventPublishing() {
        EventBus& eventBus = EventBus::instance();
        
        bool eventReceived = false;
        
        // Subscribe to events
        eventBus.subscribe("async.event", this, [&](Event*) {
            eventReceived = true;
        });
        
        // Publish async event
        eventBus.publishAsync("async.event", QVariant("async_data"), 10);
        
        // Event should not be received immediately
        QCoreApplication::processEvents();
        QVERIFY(!eventReceived);
        
        // Wait for async processing
        QEventLoop loop;
        QTimer::singleShot(50, &loop, &QEventLoop::quit);
        loop.exec();
        
        // Event should now be received
        QVERIFY(eventReceived);
    }
    
    // Event queue management
    void testEventQueueManagement() {
        EventBus& eventBus = EventBus::instance();
        
        // Set small queue size for testing
        int originalMaxSize = eventBus.maxQueueSize();
        eventBus.setMaxQueueSize(3);
        
        QSignalSpy overflowSpy(&eventBus, &EventBus::queueOverflow);
        
        // Fill queue beyond capacity
        for (int i = 0; i < 5; ++i) {
            eventBus.publishAsync("queue.event", QVariant(i));
        }

        // Wait for overflow signal to be emitted
        QCoreApplication::processEvents();
        QThread::msleep(10);
        QCoreApplication::processEvents();

        // Should have overflow
        QVERIFY(eventBus.queueSize() <= 3);
        QCOMPARE(overflowSpy.count(), 1);
        
        // Clear queue
        eventBus.clearEventQueue();
        QCOMPARE(eventBus.queueSize(), 0);
        
        // Restore original max size
        eventBus.setMaxQueueSize(originalMaxSize);
    }
    
    // Event statistics
    void testEventStatistics() {
        EventBus& eventBus = EventBus::instance();

        // Reset statistics for clean test
        eventBus.resetStatistics();

        qint64 initialPublished = eventBus.totalEventsPublished();
        qint64 initialHandled = eventBus.totalEventsHandled();

        // Subscribe to events
        eventBus.subscribe("stats.event", this, [](Event*) {});

        // Publish some events
        eventBus.publish("stats.event");
        eventBus.publish("stats.event");
        eventBus.publish("stats.event");

        // Process events to ensure async events are handled
        for (int i = 0; i < 10 && eventBus.queueSize() > 0; ++i) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }

        // Check statistics
        QCOMPARE(eventBus.totalEventsPublished(), initialPublished + 3);
        QCOMPARE(eventBus.totalEventsHandled(), initialHandled + 3);
    }
    
    // Event properties
    void testEventProperties() {
        TestEvent* event = new TestEvent("test_message");
        
        // Test basic properties
        QCOMPARE(event->type(), QString("test.event"));
        QCOMPARE(event->message(), QString("test_message"));
        QVERIFY(event->timestamp() > 0);
        QVERIFY(!event->isHandled());
        QVERIFY(!event->isPropagationStopped());
        
        // Test property modification
        event->setSource("test_source");
        QCOMPARE(event->source(), QString("test_source"));
        
        event->setHandled(true);
        QVERIFY(event->isHandled());
        
        event->stopPropagation();
        QVERIFY(event->isPropagationStopped());
        
        // Test cloning
        Event* clonedEvent = event->clone();
        QCOMPARE(clonedEvent->type(), event->type());
        QCOMPARE(clonedEvent->data(), event->data());
        
        delete event;
        delete clonedEvent;
    }
    
    // TypedEvent tests
    void testTypedEvent() {
        struct TestData {
            int value;
            QString name;
        };
        
        TestData testData{42, "test"};
        TypedEvent<TestData>* typedEvent = new TypedEvent<TestData>("typed.event", testData);
        
        QCOMPARE(typedEvent->type(), QString("typed.event"));
        QCOMPARE(typedEvent->payload().value, 42);
        QCOMPARE(typedEvent->payload().name, QString("test"));
        
        // Test cloning
        Event* clonedEvent = typedEvent->clone();
        TypedEvent<TestData>* clonedTypedEvent = dynamic_cast<TypedEvent<TestData>*>(clonedEvent);
        QVERIFY(clonedTypedEvent != nullptr);
        QCOMPARE(clonedTypedEvent->payload().value, 42);
        
        delete typedEvent;
        delete clonedEvent;
    }
    
    // EventSubscriber tests
    void testEventSubscriber() {
        EventBus& eventBus = EventBus::instance();
        
        m_testSubscriber->subscribeToTestEvents();
        
        // Publish test event
        TestEvent* testEvent = new TestEvent("subscriber_test");
        eventBus.publish(testEvent);
        
        QCoreApplication::processEvents();
        
        // Check that subscriber received the event
        QCOMPARE(m_testSubscriber->receivedEvents().size(), 1);
        QCOMPARE(m_testSubscriber->receivedEvents().first()->data().toString(), QString("subscriber_test"));
    }
    
    // Common application events
    void testCommonApplicationEvents() {
        // Test that common event constants are defined
        QVERIFY(!AppEvents::DOCUMENT_OPENED.isEmpty());
        QVERIFY(!AppEvents::DOCUMENT_CLOSED.isEmpty());
        QVERIFY(!AppEvents::DOCUMENT_SAVED.isEmpty());
        QVERIFY(!AppEvents::DOCUMENT_MODIFIED.isEmpty());
        
        QVERIFY(!AppEvents::PAGE_CHANGED.isEmpty());
        QVERIFY(!AppEvents::ZOOM_CHANGED.isEmpty());
        QVERIFY(!AppEvents::VIEW_MODE_CHANGED.isEmpty());
        
        QVERIFY(!AppEvents::THEME_CHANGED.isEmpty());
        QVERIFY(!AppEvents::LAYOUT_CHANGED.isEmpty());
        QVERIFY(!AppEvents::SIDEBAR_TOGGLED.isEmpty());
        
        QVERIFY(!AppEvents::APPLICATION_READY.isEmpty());
        QVERIFY(!AppEvents::SHUTDOWN_REQUESTED.isEmpty());
        QVERIFY(!AppEvents::ERROR_OCCURRED.isEmpty());
    }
    
    // Configuration tests
    void testEventBusConfiguration() {
        EventBus& eventBus = EventBus::instance();
        
        // Test async processing configuration
        bool originalAsync = eventBus.isAsyncProcessingEnabled();
        eventBus.setAsyncProcessingEnabled(!originalAsync);
        QCOMPARE(eventBus.isAsyncProcessingEnabled(), !originalAsync);
        eventBus.setAsyncProcessingEnabled(originalAsync);
        
        // Test max queue size configuration
        int originalMaxSize = eventBus.maxQueueSize();
        eventBus.setMaxQueueSize(500);
        QCOMPARE(eventBus.maxQueueSize(), 500);
        eventBus.setMaxQueueSize(originalMaxSize);
    }

private:
    TestSubscriber* m_testSubscriber = nullptr;
};

QTEST_MAIN(EventBusTest)
#include "event_bus_test.moc"
