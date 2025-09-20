#include <QTest>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include "../TestUtilities.h"
#include "../../app/controller/StateManager.h"
#include "../../app/controller/ServiceLocator.h"
#include "../../app/controller/EventBus.h"

class TestStateManager : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;
    
    // Basic state operations
    void testSetAndGet();
    void testHasState();
    void testRemoveState();
    void testClearState();
    void testStateTypes();
    
    // Nested state paths
    void testNestedPaths();
    void testDeepNesting();
    void testPathValidation();
    
    // State change notifications
    void testStateChangeSignals();
    void testSubscriptions();
    void testUnsubscribe();
    void testWildcardSubscriptions();
    
    // State persistence
    void testSaveState();
    void testLoadState();
    void testSerializeDeserialize();
    
    // Transaction support
    void testTransaction();
    void testTransactionRollback();
    void testNestedTransactions();
    
    // Integration tests
    void testStateManagerWithEventBus();
    void testStateManagerWithServiceLocator();
    void testConcurrentAccess();
    
    // Performance tests
    void testLargeStateTree();
    void testManySubscribers();
    
    // Error handling
    void testInvalidPaths();
    void testCircularReferences();
    void testMemoryManagement();

private:
    StateManager* m_stateManager;
    QString m_testFilePath;
};

void TestStateManager::initTestCase() {
    m_testFilePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) 
                    + "/test_state.json";
}

void TestStateManager::cleanupTestCase() {
    QFile::remove(m_testFilePath);
}

void TestStateManager::init() {
    m_stateManager = &StateManager::instance();
    m_stateManager->reset();
}

void TestStateManager::cleanup() {
    m_stateManager->reset();
}

void TestStateManager::testSetAndGet() {
    // Test basic set and get
    m_stateManager->set("test.value", 42);
    QCOMPARE(m_stateManager->get("test.value").toInt(), 42);
    
    // Test overwrite
    m_stateManager->set("test.value", 100);
    QCOMPARE(m_stateManager->get("test.value").toInt(), 100);
    
    // Test different types
    m_stateManager->set("test.string", "hello");
    QCOMPARE(m_stateManager->get("test.string").toString(), QString("hello"));
    
    m_stateManager->set("test.bool", true);
    QVERIFY(m_stateManager->get("test.bool").toBool());
    
    m_stateManager->set("test.double", 3.14);
    QCOMPARE(m_stateManager->get("test.double").toDouble(), 3.14);
}

void TestStateManager::testHasState() {
    QVERIFY(!m_stateManager->has("nonexistent"));
    
    m_stateManager->set("existing", "value");
    QVERIFY(m_stateManager->has("existing"));
    
    // Test nested paths
    m_stateManager->set("parent.child", "value");
    QVERIFY(m_stateManager->has("parent.child"));
    QVERIFY(m_stateManager->has("parent")); // Parent should exist too
}

void TestStateManager::testRemoveState() {
    m_stateManager->set("temp", "value");
    QVERIFY(m_stateManager->has("temp"));
    
    m_stateManager->remove("temp");
    QVERIFY(!m_stateManager->has("temp"));
    
    // Test removing nested state
    m_stateManager->set("parent.child1", "value1");
    m_stateManager->set("parent.child2", "value2");
    
    m_stateManager->remove("parent.child1");
    QVERIFY(!m_stateManager->has("parent.child1"));
    QVERIFY(m_stateManager->has("parent.child2")); // Other child should remain
}

void TestStateManager::testClearState() {
    m_stateManager->set("key1", "value1");
    m_stateManager->set("key2", "value2");
    m_stateManager->set("nested.key", "value3");
    
    m_stateManager->reset();
    
    QVERIFY(!m_stateManager->has("key1"));
    QVERIFY(!m_stateManager->has("key2"));
    QVERIFY(!m_stateManager->has("nested.key"));
}

void TestStateManager::testStateTypes() {
    // Test various Qt types
    QStringList list = {"one", "two", "three"};
    m_stateManager->set("list", list);
    QCOMPARE(m_stateManager->get("list").toStringList(), list);
    
    QVariantMap map;
    map["key1"] = "value1";
    map["key2"] = 42;
    m_stateManager->set("map", map);
    QCOMPARE(m_stateManager->get("map").toMap(), map);
    
    QDateTime now = QDateTime::currentDateTime();
    m_stateManager->set("datetime", now);
    QCOMPARE(m_stateManager->get("datetime").toDateTime(), now);
}

void TestStateManager::testNestedPaths() {
    // Test setting nested paths
    m_stateManager->set("app.window.width", 800);
    m_stateManager->set("app.window.height", 600);
    m_stateManager->set("app.window.maximized", false);
    
    QCOMPARE(m_stateManager->get("app.window.width").toInt(), 800);
    QCOMPARE(m_stateManager->get("app.window.height").toInt(), 600);
    QVERIFY(!m_stateManager->get("app.window.maximized").toBool());
    
    // Get parent object
    QVariantMap window = m_stateManager->get("app.window").toMap();
    QCOMPARE(window["width"].toInt(), 800);
    QCOMPARE(window["height"].toInt(), 600);
}

void TestStateManager::testDeepNesting() {
    // Test deeply nested paths
    QString deepPath = "level1.level2.level3.level4.level5.value";
    m_stateManager->set(deepPath, "deep");
    
    QCOMPARE(m_stateManager->get(deepPath).toString(), QString("deep"));
    
    // Verify intermediate levels exist
    QVERIFY(m_stateManager->has("level1"));
    QVERIFY(m_stateManager->has("level1.level2"));
    QVERIFY(m_stateManager->has("level1.level2.level3"));
}

void TestStateManager::testPathValidation() {
    // Test invalid paths
    m_stateManager->set("", "empty");
    QVERIFY(!m_stateManager->has("")); // Empty path should be rejected
    
    m_stateManager->set(".", "dot");
    QVERIFY(!m_stateManager->has(".")); // Single dot should be rejected
    
    // Valid paths
    m_stateManager->set("valid_path", "value");
    QVERIFY(m_stateManager->has("valid_path"));
    
    m_stateManager->set("path-with-dash", "value");
    QVERIFY(m_stateManager->has("path-with-dash"));
}

void TestStateManager::testStateChangeSignals() {
    QSignalSpy changeSpy(m_stateManager, &StateManager::stateChanged);
    
    m_stateManager->set("test", "value");
    
    QCOMPARE(changeSpy.count(), 1);
    
    QList<QVariant> arguments = changeSpy.takeFirst();
    StateChange change = arguments.at(0).value<StateChange>();
    
    QCOMPARE(change.path, QString("test"));
    QCOMPARE(change.newValue.toString(), QString("value"));
}

void TestStateManager::testSubscriptions() {
    bool callbackCalled = false;
    QString receivedPath;
    QVariant receivedValue;
    
    m_stateManager->subscribe("test.path", this, 
        [&callbackCalled, &receivedPath, &receivedValue](const StateChange& change) {
            callbackCalled = true;
            receivedPath = change.path;
            receivedValue = change.newValue;
        });
    
    m_stateManager->set("test.path", "new value");
    
    QVERIFY(callbackCalled);
    QCOMPARE(receivedPath, QString("test.path"));
    QCOMPARE(receivedValue.toString(), QString("new value"));
}

void TestStateManager::testUnsubscribe() {
    int callCount = 0;
    
    auto callback = [&callCount](const StateChange&) {
        callCount++;
    };
    
    m_stateManager->subscribe("test", this, callback);
    
    m_stateManager->set("test", "value1");
    QCOMPARE(callCount, 1);
    
    m_stateManager->unsubscribe("test", this);
    
    m_stateManager->set("test", "value2");
    QCOMPARE(callCount, 1); // Should not increase
}

void TestStateManager::testWildcardSubscriptions() {
    QStringList changedPaths;
    
    // Subscribe to all changes under "app"
    m_stateManager->subscribe("app.*", this, 
        [&changedPaths](const StateChange& change) {
            changedPaths.append(change.path);
        });
    
    m_stateManager->set("app.setting1", "value1");
    m_stateManager->set("app.setting2", "value2");
    m_stateManager->set("other.setting", "value3"); // Should not trigger
    
    waitMs(10);
    
    QCOMPARE(changedPaths.size(), 2);
    QVERIFY(changedPaths.contains("app.setting1"));
    QVERIFY(changedPaths.contains("app.setting2"));
    QVERIFY(!changedPaths.contains("other.setting"));
}

void TestStateManager::testSaveState() {
    // Set some state
    m_stateManager->set("app.version", "1.0.0");
    m_stateManager->set("app.settings.theme", "dark");
    m_stateManager->set("app.settings.fontSize", 12);
    
    // Save to file
    bool saved = m_stateManager->saveToFile(m_testFilePath);
    QVERIFY(saved);
    
    // Verify file exists
    QVERIFY(QFile::exists(m_testFilePath));
}

void TestStateManager::testLoadState() {
    // Save state first
    m_stateManager->set("saved.value1", "test");
    m_stateManager->set("saved.value2", 42);
    m_stateManager->saveToFile(m_testFilePath);
    
    // Clear and reload
    m_stateManager->reset();
    QVERIFY(!m_stateManager->has("saved.value1"));
    
    bool loaded = m_stateManager->loadFromFile(m_testFilePath);
    QVERIFY(loaded);
    
    // Verify state was restored
    QCOMPARE(m_stateManager->get("saved.value1").toString(), QString("test"));
    QCOMPARE(m_stateManager->get("saved.value2").toInt(), 42);
}

void TestStateManager::testSerializeDeserialize() {
    // Set complex state
    QVariantMap complexData;
    complexData["nested"] = QVariantMap{{"deep", "value"}};
    complexData["array"] = QVariantList{1, 2, 3};
    
    m_stateManager->set("complex", complexData);
    
    // Serialize to JSON
    QJsonObject json = m_stateManager->toJson();
    
    // Clear and deserialize
    m_stateManager->reset();
    m_stateManager->fromJson(json);
    
    // Verify restoration
    QVariantMap restored = m_stateManager->get("complex").toMap();
    QCOMPARE(restored["nested"].toMap()["deep"].toString(), QString("value"));
    QCOMPARE(restored["array"].toList().size(), 3);
}

void TestStateManager::testTransaction() {
    m_stateManager->set("initial", "value");
    
    // Start transaction
    m_stateManager->beginTransaction();
    
    m_stateManager->set("initial", "modified");
    m_stateManager->set("new", "value");
    
    // Commit transaction
    m_stateManager->commitTransaction();
    
    QCOMPARE(m_stateManager->get("initial").toString(), QString("modified"));
    QCOMPARE(m_stateManager->get("new").toString(), QString("value"));
}

void TestStateManager::testTransactionRollback() {
    m_stateManager->set("initial", "value");
    
    // Start transaction
    m_stateManager->beginTransaction();
    
    m_stateManager->set("initial", "modified");
    m_stateManager->set("new", "value");
    
    // Rollback transaction
    m_stateManager->rollbackTransaction();
    
    QCOMPARE(m_stateManager->get("initial").toString(), QString("value"));
    QVERIFY(!m_stateManager->has("new"));
}

void TestStateManager::testNestedTransactions() {
    m_stateManager->set("value", 1);
    
    // Outer transaction
    m_stateManager->beginTransaction();
    m_stateManager->set("value", 2);
    
    // Inner transaction
    m_stateManager->beginTransaction();
    m_stateManager->set("value", 3);
    
    // Rollback inner
    m_stateManager->rollbackTransaction();
    QCOMPARE(m_stateManager->get("value").toInt(), 2);
    
    // Commit outer
    m_stateManager->commitTransaction();
    QCOMPARE(m_stateManager->get("value").toInt(), 2);
}

void TestStateManager::testStateManagerWithEventBus() {
    EventBus& eventBus = EventBus::instance();
    
    bool eventReceived = false;
    QString eventPath;
    
    // Subscribe to state change events via EventBus
    eventBus.subscribe("state.changed", this, [&eventReceived, &eventPath](Event* e) {
        eventReceived = true;
        eventPath = e->data.toMap()["path"].toString();
    });
    
    // Connect StateManager to EventBus
    QObject::connect(m_stateManager, &StateManager::stateChanged, 
        [&eventBus](const StateChange& change) {
            QVariantMap data;
            data["path"] = change.path;
            data["value"] = change.newValue;
            eventBus.publish("state.changed", data);
        });
    
    m_stateManager->set("test.event", "value");
    
    waitMs(10);
    
    QVERIFY(eventReceived);
    QCOMPARE(eventPath, QString("test.event"));
}

void TestStateManager::testStateManagerWithServiceLocator() {
    // Register StateManager as a service
    ServiceLocator::instance().registerService<StateManager>(m_stateManager);
    
    // Get from service locator
    auto* service = ServiceLocator::instance().getService<StateManager>();
    QVERIFY(service != nullptr);
    QCOMPARE(service, m_stateManager);
    
    // Use through service
    service->set("service.test", "value");
    QCOMPARE(m_stateManager->get("service.test").toString(), QString("value"));
    
    ServiceLocator::instance().clearServices();
}

void TestStateManager::testConcurrentAccess() {
    const int numThreads = 10;
    const int numOperations = 100;
    
    QList<QFuture<void>> futures;
    
    // Launch multiple threads performing state operations
    for (int t = 0; t < numThreads; ++t) {
        futures.append(QtConcurrent::run([this, t, numOperations]() {
            for (int i = 0; i < numOperations; ++i) {
                QString key = QString("thread%1.value%2").arg(t).arg(i);
                m_stateManager->set(key, i);
                
                // Random read
                if (i % 2 == 0) {
                    m_stateManager->get(key);
                }
                
                // Random remove
                if (i % 5 == 0) {
                    m_stateManager->remove(key);
                }
            }
        }));
    }
    
    // Wait for all threads
    for (auto& future : futures) {
        future.waitForFinished();
    }
    
    // Should not crash
    QVERIFY(true);
}

void TestStateManager::testLargeStateTree() {
    // Create a large state tree
    const int numKeys = 1000;
    
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < numKeys; ++i) {
        QString path = QString("level1.level2.level3.key%1").arg(i);
        m_stateManager->set(path, i);
    }
    
    qint64 writeTime = timer.elapsed();
    qDebug() << "Write time for" << numKeys << "keys:" << writeTime << "ms";
    
    // Read performance
    timer.restart();
    
    for (int i = 0; i < numKeys; ++i) {
        QString path = QString("level1.level2.level3.key%1").arg(i);
        int value = m_stateManager->get(path).toInt();
        QCOMPARE(value, i);
    }
    
    qint64 readTime = timer.elapsed();
    qDebug() << "Read time for" << numKeys << "keys:" << readTime << "ms";
    
    // Performance should be reasonable
    QVERIFY(writeTime < 5000); // Less than 5 seconds
    QVERIFY(readTime < 5000);
}

void TestStateManager::testManySubscribers() {
    const int numSubscribers = 100;
    QList<int> callCounts(numSubscribers, 0);
    
    // Add many subscribers
    for (int i = 0; i < numSubscribers; ++i) {
        m_stateManager->subscribe("test.path", this, 
            [&callCounts, i](const StateChange&) {
                callCounts[i]++;
            });
    }
    
    // Trigger state change
    m_stateManager->set("test.path", "value");
    
    waitMs(100);
    
    // All subscribers should be called
    for (int count : callCounts) {
        QCOMPARE(count, 1);
    }
}

void TestStateManager::testInvalidPaths() {
    // Test various invalid paths
    QStringList invalidPaths = {
        "",           // Empty
        ".",          // Just dot
        "..",         // Double dot
        "..test",     // Starting with dots
        "test..",     // Ending with dots
        "test..path", // Double dots in middle
    };
    
    for (const QString& path : invalidPaths) {
        m_stateManager->set(path, "value");
        // Should handle gracefully, either reject or accept
        // Just verify no crash
    }
    
    QVERIFY(true);
}

void TestStateManager::testCircularReferences() {
    // Create a potential circular reference
    QVariantMap map1;
    QVariantMap map2;
    
    map1["ref"] = QVariant::fromValue(&map2);
    map2["ref"] = QVariant::fromValue(&map1);
    
    // Try to set (should handle gracefully)
    m_stateManager->set("circular", map1);
    
    // Should not crash
    QVERIFY(true);
}

void TestStateManager::testMemoryManagement() {
    // Test memory cleanup
    for (int iteration = 0; iteration < 10; ++iteration) {
        // Add many keys
        for (int i = 0; i < 1000; ++i) {
            QString key = QString("temp.key%1").arg(i);
            m_stateManager->set(key, QString("value%1").arg(i));
        }
        
        // Remove them all
        m_stateManager->remove("temp");
        
        // Memory should be freed
    }
    
    // Final cleanup
    m_stateManager->reset();
    
    // Should not have memory leaks
    QVERIFY(true);
}

QTEST_MAIN(TestStateManager)
#include "StateManagerTest_new.moc"
