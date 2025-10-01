#include <QTest>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QTemporaryFile>
#include "../../app/controller/StateManager.h"
#include "../TestUtilities.h"

class StateManagerTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override {
        // Reset state manager before tests
        StateManager::instance().reset();
    }
    
    void cleanup() override {
        // Clear state after each test
        StateManager::instance().reset();
        StateManager::instance().clearHistory();
    }
    
    // State class tests
    void testStateConstruction() {
        State state;
        QVERIFY(!state.has("anykey"));
        
        QJsonObject data;
        data["key1"] = "value1";
        data["key2"] = 42;
        
        State state2(data);
        QVERIFY(state2.has("key1"));
        QCOMPARE(state2.get("key1").toString(), QString("value1"));
        QCOMPARE(state2.get("key2").toInt(), 42);
    }
    
    void testStateGet() {
        QJsonObject data;
        data["user"] = QJsonObject{{"name", "John"}, {"age", 30}};
        State state(data);
        
        // Test simple get
        QVERIFY(state.has("user"));
        
        // Test nested get with path
        QCOMPARE(state.get("user.name").toString(), QString("John"));
        QCOMPARE(state.get("user.age").toInt(), 30);
        
        // Test non-existent path
        QVERIFY(!state.has("user.email"));
        QVERIFY(state.get("user.email").isNull());
    }
    
    void testStateSet() {
        State state;
        
        // Test simple set
        State newState = state.set("key", "value");
        QVERIFY(!state.has("key")); // Original unchanged
        QVERIFY(newState.has("key"));
        QCOMPARE(newState.get("key").toString(), QString("value"));
        
        // Test nested set
        State state2 = newState.set("user.name", "Alice");
        QVERIFY(state2.has("user.name"));
        QCOMPARE(state2.get("user.name").toString(), QString("Alice"));
    }
    
    void testStateMerge() {
        QJsonObject initial;
        initial["key1"] = "value1";
        initial["key2"] = "value2";
        State state(initial);
        
        QJsonObject toMerge;
        toMerge["key2"] = "updated";
        toMerge["key3"] = "new";
        
        State merged = state.merge(toMerge);
        QCOMPARE(merged.get("key1").toString(), QString("value1"));
        QCOMPARE(merged.get("key2").toString(), QString("updated"));
        QCOMPARE(merged.get("key3").toString(), QString("new"));
    }
    
    void testStateRemove() {
        QJsonObject data;
        data["key1"] = "value1";
        data["key2"] = "value2";
        State state(data);
        
        State newState = state.remove("key1");
        QVERIFY(state.has("key1")); // Original unchanged
        QVERIFY(!newState.has("key1"));
        QVERIFY(newState.has("key2"));
    }
    
    void testStateEquality() {
        QJsonObject data;
        data["key"] = "value";
        
        State state1(data);
        State state2(data);
        State state3;
        
        QVERIFY(state1 == state2);
        QVERIFY(state1 != state3);
    }
    
    // StateManager tests
    void testStateManagerSingleton() {
        StateManager& instance1 = StateManager::instance();
        StateManager& instance2 = StateManager::instance();
        QCOMPARE(&instance1, &instance2);
    }
    
    void testStateManagerSet() {
        StateManager& manager = StateManager::instance();
        QSignalSpy spy(&manager, SIGNAL(stateChanged(const StateChange&)));
        
        manager.set("test.value", 42);
        
        QCOMPARE(manager.get("test.value").toInt(), 42);
        QVERIFY(manager.has("test.value"));
        QCOMPARE(spy.count(), 1);
    }
    
    void testStateManagerMerge() {
        StateManager& manager = StateManager::instance();
        
        QJsonObject data;
        data["app"] = QJsonObject{{"version", "1.0"}, {"name", "Test"}};
        
        manager.merge(data);
        
        QCOMPARE(manager.get("app.version").toString(), QString("1.0"));
        QCOMPARE(manager.get("app.name").toString(), QString("Test"));
    }
    
    void testStateManagerRemove() {
        StateManager& manager = StateManager::instance();
        
        manager.set("temp.data", "value");
        QVERIFY(manager.has("temp.data"));
        
        manager.remove("temp.data");
        QVERIFY(!manager.has("temp.data"));
    }
    
    void testStateManagerSubscribe() {
        StateManager& manager = StateManager::instance();
        
        bool callbackCalled = false;
        QString observedPath;
        QVariant oldValue, newValue;
        
        manager.subscribe("test.path", this, [&](const StateChange& change) {
            callbackCalled = true;
            observedPath = "test.path";
            oldValue = change.oldValue("test.path");
            newValue = change.newValue("test.path");
        });
        
        manager.set("test.path", "new value");
        
        QVERIFY(callbackCalled);
        QCOMPARE(observedPath, QString("test.path"));
        QVERIFY(oldValue.isNull());
        QCOMPARE(newValue.toString(), QString("new value"));
    }
    
    void testStateManagerUnsubscribe() {
        StateManager& manager = StateManager::instance();
        
        int callCount = 0;
        manager.subscribe("test", this, [&](const StateChange&) {
            callCount++;
        });
        
        manager.set("test", 1);
        QCOMPARE(callCount, 1);
        
        manager.unsubscribe("test", this);
        manager.set("test", 2);
        QCOMPARE(callCount, 1); // Should not increase
    }
    
    void testStateManagerHistory() {
        StateManager& manager = StateManager::instance();
        manager.enableHistory(10);
        
        QVERIFY(!manager.canUndo());
        QVERIFY(!manager.canRedo());
        
        manager.set("value", 1);
        manager.set("value", 2);
        manager.set("value", 3);
        
        QVERIFY(manager.canUndo());
        QVERIFY(!manager.canRedo());
        
        manager.undo();
        QCOMPARE(manager.get("value").toInt(), 2);
        QVERIFY(manager.canUndo());
        QVERIFY(manager.canRedo());
        
        manager.undo();
        QCOMPARE(manager.get("value").toInt(), 1);
        
        manager.redo();
        QCOMPARE(manager.get("value").toInt(), 2);
        
        manager.redo();
        QCOMPARE(manager.get("value").toInt(), 3);
        QVERIFY(!manager.canRedo());
    }
    
    void testStateManagerHistoryLimit() {
        StateManager& manager = StateManager::instance();
        manager.enableHistory(3);
        
        for (int i = 0; i < 5; i++) {
            manager.set("value", i);
        }
        
        // Should only keep last 3 changes
        auto history = manager.history();
        QVERIFY(history.size() <= 3);
    }
    
    void testStateManagerSnapshots() {
        StateManager& manager = StateManager::instance();
        
        manager.set("data", "original");
        manager.createSnapshot("snapshot1");
        
        manager.set("data", "modified");
        QCOMPARE(manager.get("data").toString(), QString("modified"));
        
        QVERIFY(manager.restoreSnapshot("snapshot1"));
        QCOMPARE(manager.get("data").toString(), QString("original"));
        
        QStringList snapshots = manager.snapshots();
        QVERIFY(snapshots.contains("snapshot1"));
        
        manager.deleteSnapshot("snapshot1");
        snapshots = manager.snapshots();
        QVERIFY(!snapshots.contains("snapshot1"));
    }
    
    void testStateManagerPersistence() {
        StateManager& manager = StateManager::instance();
        
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        QString filePath = tempFile.fileName();
        tempFile.close();
        
        // Set some state and save
        manager.set("persistent.value", "test data");
        manager.set("persistent.number", 123);
        QVERIFY(manager.saveState(filePath));
        
        // Reset and verify state is cleared
        manager.reset();
        QVERIFY(!manager.has("persistent.value"));
        
        // Load state back
        QVERIFY(manager.loadState(filePath));
        QCOMPARE(manager.get("persistent.value").toString(), QString("test data"));
        QCOMPARE(manager.get("persistent.number").toInt(), 123);
    }
    
    void testStateChange() {
        State oldState;
        State newState = oldState.set("key", "value");
        
        StateChange change(oldState, newState, "test change");
        
        QCOMPARE(change.reason(), QString("test change"));
        QVERIFY(change.hasChanged("key"));
        QVERIFY(!change.hasChanged("other"));
        
        QStringList changed = change.changedPaths();
        QVERIFY(changed.contains("key"));
        
        QVERIFY(change.oldValue("key").isNull());
        QCOMPARE(change.newValue("key").toString(), QString("value"));
    }
    
    void testStateStore() {
        StateStore store;
        QSignalSpy spy(&store, &StateStore::stateChanged);
        
        // Add a reducer
        store.addReducer("counter", [](const State& state, const StateStore::Action& action) {
            if (action.type == "INCREMENT") {
                int current = state.get("counter").toInt();
                return state.set("counter", current + 1);
            }
            if (action.type == "DECREMENT") {
                int current = state.get("counter").toInt();
                return state.set("counter", current - 1);
            }
            return state;
        });
        
        // Initialize counter
        store.dispatch("INIT", 0);
        store.dispatch(StateStore::Action{"SET", QVariant(), {{"path", "counter"}, {"value", 0}}});
        
        // Dispatch actions
        store.dispatch("INCREMENT");
        QCOMPARE(store.get("counter").toInt(), 1);
        
        store.dispatch("INCREMENT");
        QCOMPARE(store.get("counter").toInt(), 2);
        
        store.dispatch("DECREMENT");
        QCOMPARE(store.get("counter").toInt(), 1);
        
        QVERIFY(spy.count() >= 3);
    }
    
    void testStateSelector() {
        State state;
        state = state.set("user.name", "John");
        state = state.set("user.age", 30);
        
        StateSelector<QString> nameSelector([](const State& s) {
            return s.get("user.name").toString();
        });
        
        QString name = nameSelector.select(state);
        QCOMPARE(name, QString("John"));
        
        // Should use cached value on same state
        QString name2 = nameSelector.select(state);
        QCOMPARE(name2, QString("John"));
        
        // Should recalculate on different state
        State newState = state.set("user.name", "Jane");
        QString name3 = nameSelector.select(newState);
        QCOMPARE(name3, QString("Jane"));
    }
    
    void testStateMiddleware() {
        StateManager& manager = StateManager::instance();
        
        // Add logging middleware
        QStringList log;
        manager.addMiddleware([&log](const State& oldState, const State& newState) {
            log.append("State changed");
            return newState;
        });
        
        // Add validation middleware
        manager.addMiddleware([](const State& oldState, const State& newState) {
            // Prevent negative values
            if (newState.has("value")) {
                int val = newState.get("value").toInt();
                if (val < 0) {
                    return oldState; // Reject change
                }
            }
            return newState;
        });
        
        manager.set("value", 10);
        QCOMPARE(manager.get("value").toInt(), 10);
        QCOMPARE(log.size(), 1);
        
        manager.set("value", -5);
        QCOMPARE(manager.get("value").toInt(), 10); // Should be rejected
        QCOMPARE(log.size(), 2); // Middleware still called
    }
    
    void testDebugMode() {
        StateManager& manager = StateManager::instance();
        
        QVERIFY(!manager.isDebugMode());
        manager.enableDebugMode(true);
        QVERIFY(manager.isDebugMode());
        
        manager.set("debug.test", "value");
        QString report = manager.stateReport();
        QVERIFY(!report.isEmpty());
        
        manager.enableDebugMode(false);
        QVERIFY(!manager.isDebugMode());
    }
};

QTEST_MAIN(StateManagerTest)
#include "state_manager_test.moc"
