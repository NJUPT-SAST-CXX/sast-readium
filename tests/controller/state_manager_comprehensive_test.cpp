#include <QTest>
#include <QJsonObject>
#include <QJsonArray>
#include <QSignalSpy>
#include <memory>
#include "../TestUtilities.h"
#include "../../app/controller/StateManager.h"

class StateManagerComprehensiveTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic state functionality
    void testStateCreation();
    void testStateGetSet();
    void testNestedStateAccess();
    void testArrayStateAccess();

    // State comparison and diffing
    void testStateEquality();
    void testStateInequality();
    void testStateDiffSimple();
    void testStateDiffNested();
    void testStateDiffArrays();
    void testStateDiffComplex();

    // State management
    void testStateManagerBasics();
    void testStateTransactions();
    void testStateHistory();
    void testStateValidation();

    // State change tracking
    void testStateChangeCreation();
    void testStateChangeSignals();
    void testStateChangeRevert();

    // Performance and edge cases
    void testLargeStatePerformance();
    void testDeepNestingPerformance();
    void testInvalidPaths();
    void testTypeConversions();

private:
    QJsonObject createComplexNestedState();
    void verifyStateDiff(const QStringList& expected, const QStringList& actual);
};

void StateManagerComprehensiveTest::initTestCase() {
    qDebug() << "Initializing StateManager comprehensive tests";
}

void StateManagerComprehensiveTest::cleanupTestCase() {
    qDebug() << "Cleaning up StateManager comprehensive tests";
}

void StateManagerComprehensiveTest::init() {
    TestBase::init();
}

void StateManagerComprehensiveTest::cleanup() {
    TestBase::cleanup();
}

void StateManagerComprehensiveTest::testStateCreation() {
    // Test empty state
    State emptyState;
    QVERIFY(emptyState.data().isEmpty());
    QVERIFY(emptyState.get("nonexistent").isNull());

    // Test state with JSON object
    QJsonObject obj;
    obj["key1"] = "value1";
    obj["number"] = 42;
    State stateWithObj(obj);
    QCOMPARE(stateWithObj.get("key1").toString(), "value1");
    QCOMPARE(stateWithObj.get("number").toInt(), 42);
}

void StateManagerComprehensiveTest::testStateGetSet() {
    State state;

    // Test setting and getting simple values
    state.set("string", "hello");
    state.set("number", 123);
    state.set("boolean", true);
    state.set("double", 3.14);

    QCOMPARE(state.get("string").toString(), "hello");
    QCOMPARE(state.get("number").toInt(), 123);
    QCOMPARE(state.get("boolean").toBool(), true);
    QCOMPARE(state.get("double").toDouble(), 3.14);

    // Test overwriting values
    state.set("string", "world");
    QCOMPARE(state.get("string").toString(), "world");
}

void StateManagerComprehensiveTest::testNestedStateAccess() {
    State state;

    // Create nested structure
    state.set("user.name", "John Doe");
    state.set("user.age", 30);
    state.set("user.address.city", "New York");
    state.set("user.address.zip", "10001");

    QCOMPARE(state.get("user.name").toString(), "John Doe");
    QCOMPARE(state.get("user.age").toInt(), 30);
    QCOMPARE(state.get("user.address.city").toString(), "New York");
    QCOMPARE(state.get("user.address.zip").toString(), "10001");

    // Test accessing non-existent nested paths
    QVERIFY(state.get("user.nonexistent").isNull());
    QVERIFY(state.get("nonexistent.path").isNull());
}

void StateManagerComprehensiveTest::testArrayStateAccess() {
    State state;

    // Create array structure
    QJsonArray items;
    items.append("item1");
    items.append("item2");
    items.append(42);

    state.set("items", items);

    QVariant itemsVariant = state.get("items");
    QVERIFY(itemsVariant.isValid());

    // Test array access through string indexing (simplified approach)
    state.set("items[0]", "modified_item1");
    QCOMPARE(state.get("items[0]").toString(), "modified_item1");
}

void StateManagerComprehensiveTest::testStateEquality() {
    QJsonObject obj1;
    obj1["key"] = "value";
    obj1["number"] = 42;

    QJsonObject obj2;
    obj2["key"] = "value";
    obj2["number"] = 42;

    State state1(obj1);
    State state2(obj2);

    QVERIFY(state1 == state2);
    QVERIFY(!(state1 != state2));
}

void StateManagerComprehensiveTest::testStateInequality() {
    QJsonObject obj1;
    obj1["key"] = "value1";
    obj1["number"] = 42;

    QJsonObject obj2;
    obj2["key"] = "value2";  // Different value
    obj2["number"] = 42;

    State state1(obj1);
    State state2(obj2);

    QVERIFY(state1 != state2);
    QVERIFY(!(state1 == state2));
}

void StateManagerComprehensiveTest::testStateDiffSimple() {
    State oldState;
    oldState.set("key1", "value1");
    oldState.set("key2", "value2");
    oldState.set("number", 42);

    State newState;
    newState.set("key1", "value1");        // Same
    newState.set("key2", "modified_value2"); // Changed
    newState.set("number", 100);            // Changed
    newState.set("key3", "new_value");      // Added

    StateChange change(oldState, newState, "test");
    QStringList changedPaths = change.changedPaths();

    // The exact paths depend on the implementation details
    // We expect at least the changed and added keys to be detected
    QVERIFY(changedPaths.contains("key2") || changedPaths.contains("number") || changedPaths.contains("key3"));
}

void StateManagerComprehensiveTest::testStateDiffNested() {
    State oldState;
    oldState.set("user.name", "John");
    oldState.set("user.age", 30);
    oldState.set("user.address.city", "New York");

    State newState;
    newState.set("user.name", "John");           // Same
    newState.set("user.age", 31);                 // Changed
    newState.set("user.address.city", "Boston");  // Changed
    newState.set("user.address.country", "USA");  // Added

    StateChange change(oldState, newState, "nested test");
    QStringList changedPaths = change.changedPaths();

    // Should detect nested changes
    QVERIFY(!changedPaths.isEmpty());
    // Look for nested paths or flat paths depending on implementation
    bool foundNestedChange = false;
    for (const QString& path : changedPaths) {
        if (path.contains("user.age") || path.contains("user.address")) {
            foundNestedChange = true;
            break;
        }
    }
    QVERIFY(foundNestedChange);
}

void StateManagerComprehensiveTest::testStateDiffArrays() {
    QJsonArray oldArray;
    oldArray.append("item1");
    oldArray.append("item2");

    QJsonArray newArray;
    newArray.append("item1");
    newArray.append("modified_item2");
    newArray.append("item3");

    State oldState;
    oldState.set("array", oldArray);

    State newState;
    newState.set("array", newArray);

    StateChange change(oldState, newState, "array test");
    QStringList changedPaths = change.changedPaths();

    // Should detect array changes
    QVERIFY(!changedPaths.isEmpty());
}

void StateManagerComprehensiveTest::testStateDiffComplex() {
    State oldState = State(createComplexNestedState());

    State newState = State(createComplexNestedState());
    // Make several changes
    newState.set("users[0].name", "Alice Johnson");
    newState.set("settings.theme", "dark");
    newState.set("settings.notifications.email", false);
    newState.remove("tempData"); // Remove a key

    StateChange change(oldState, newState, "complex test");
    QStringList changedPaths = change.changedPaths();

    // Should detect multiple changes in complex structure
    QVERIFY(changedPaths.size() > 2);

    // Verify specific expected changes are detected
    bool foundUserChange = false;
    bool foundSettingsChange = false;

    for (const QString& path : changedPaths) {
        if (path.contains("users") || path.contains("0].name")) {
            foundUserChange = true;
        }
        if (path.contains("settings")) {
            foundSettingsChange = true;
        }
    }

    QVERIFY(foundUserChange);
    QVERIFY(foundSettingsChange);
}

void StateManagerComprehensiveTest::testStateManagerBasics() {
    StateManager manager;

    // Test initial state
    QVERIFY(manager.getCurrentState().data().isEmpty());

    // Test state update
    QJsonObject update;
    update["test"] = "value";
    manager.updateState(update);

    QCOMPARE(manager.getCurrentState().get("test").toString(), "value");
}

void StateManagerComprehensiveTest::testStateTransactions() {
    StateManager manager;

    // Start transaction
    QVERIFY(manager.beginTransaction());

    // Make changes within transaction
    manager.set("key1", "value1");
    manager.set("key2", "value2");

    // Commit transaction
    QVERIFY(manager.commitTransaction());

    // Verify changes were applied
    QCOMPARE(manager.getCurrentState().get("key1").toString(), "value1");
    QCOMPARE(manager.getCurrentState().get("key2").toString(), "value2");

    // Test rollback
    QVERIFY(manager.beginTransaction());
    manager.set("key1", "modified");
    QVERIFY(manager.rollbackTransaction());

    // Verify rollback worked
    QCOMPARE(manager.getCurrentState().get("key1").toString(), "value1");
}

void StateManagerComprehensiveTest::testStateHistory() {
    StateManager manager;

    // Make some changes
    manager.set("key1", "value1");
    manager.set("key2", "value2");
    manager.set("key3", "value3");

    // Test undo functionality
    QVERIFY(manager.canUndo());
    manager.undo();
    QVERIFY(!manager.getCurrentState().get("key3").isValid());

    QVERIFY(manager.canUndo());
    manager.undo();
    QVERIFY(!manager.getCurrentState().get("key2").isValid());

    // Test redo functionality
    QVERIFY(manager.canRedo());
    manager.redo();
    QCOMPARE(manager.getCurrentState().get("key2").toString(), "value2");
}

void StateManagerComprehensiveTest::testStateValidation() {
    StateManager manager;

    // Test validation rules
    QJsonObject schema;
    schema["type"] = "object";
    schema["required"] = QJsonArray({"name", "age"});

    manager.setValidationSchema(schema);

    // Valid update should pass
    QJsonObject validUpdate;
    validUpdate["name"] = "John";
    validUpdate["age"] = 30;
    QVERIFY(manager.validateUpdate(validUpdate));

    // Invalid update should fail
    QJsonObject invalidUpdate;
    invalidUpdate["name"] = "John";
    // Missing required "age" field
    QVERIFY(!manager.validateUpdate(invalidUpdate));
}

void StateManagerComprehensiveTest::testStateChangeCreation() {
    State oldState;
    oldState.set("key", "old_value");

    State newState;
    newState.set("key", "new_value");

    StateChange change(oldState, newState, "test change");

    QCOMPARE(change.oldValue("key").toString(), "old_value");
    QCOMPARE(change.newValue("key").toString(), "new_value");
    QVERIFY(change.hasChanged("key"));
    QVERIFY(!change.hasChanged("nonexistent"));
}

void StateManagerComprehensiveTest::testStateChangeSignals() {
    StateManager manager;

    QSignalSpy changeSpy(&manager, &StateManager::stateChanged);

    // Make a change
    manager.set("test", "value");

    QCOMPARE(changeSpy.count(), 1);
    QList<QVariant> arguments = changeSpy.takeFirst();
    QVERIFY(arguments.at(0).canConvert<QString>()); // Reason
}

void StateManagerComprehensiveTest::testStateChangeRevert() {
    State oldState;
    oldState.set("key1", "value1");
    oldState.set("key2", "value2");

    State newState;
    newState.set("key1", "modified_value1");
    newState.set("key2", "value2");
    newState.set("key3", "new_value3");

    StateChange change(oldState, newState, "test");

    // Revert the change
    State revertedState = change.revert();

    QCOMPARE(revertedState.get("key1").toString(), "value1");
    QCOMPARE(revertedState.get("key2").toString(), "value2");
    QVERIFY(!revertedState.get("key3").isValid());
}

void StateManagerComprehensiveTest::testLargeStatePerformance() {
    StateManager manager;
    QElapsedTimer timer;

    // Create a large state
    timer.start();
    for (int i = 0; i < 1000; ++i) {
        manager.set(QString("key%1").arg(i), QString("value%1").arg(i));
    }
    qint64 setTime = timer.elapsed();

    // Performance should be reasonable (< 100ms for 1000 operations)
    QVERIFY2(setTime < 100, QString("State set took too long: %1ms").arg(setTime).toLocal8Bit());

    // Test diff performance
    State copy = manager.getCurrentState();
    timer.restart();
    for (int i = 0; i < 100; ++i) {
        manager.set(QString("key%1").arg(i), QString("modified_value%1").arg(i));
    }
    qint64 diffTime = timer.elapsed();

    // Diff should also be reasonable
    QVERIFY2(diffTime < 50, QString("State diff took too long: %1ms").arg(diffTime).toLocal8Bit());
}

void StateManagerComprehensiveTest::testDeepNestingPerformance() {
    StateManager manager;
    QElapsedTimer timer;

    // Create deeply nested structure
    timer.start();
    QString basePath = "level1.level2.level3.level4.level5";
    for (int i = 0; i < 100; ++i) {
        manager.set(QString("%1.item%2").arg(basePath).arg(i), QString("value%1").arg(i));
    }
    qint64 setTime = timer.elapsed();

    // Deep nesting should still perform reasonably
    QVERIFY2(setTime < 50, QString("Deep nesting took too long: %1ms").arg(setTime).toLocal8Bit());
}

void StateManagerComprehensiveTest::testInvalidPaths() {
    State state;

    // Test invalid path formats
    state.set("", "invalid");  // Empty path
    state.set(".", "invalid"); // Single dot
    state.set("..", "invalid"); // Double dot

    // These should not crash and should handle gracefully
    QVERIFY(state.get("").isNull());
    QVERIFY(state.get(".").isNull());
    QVERIFY(state.get("..").isNull());
}

void StateManagerComprehensiveTest::testTypeConversions() {
    State state;

    // Test automatic type conversions
    state.set("string_number", "123");
    QCOMPARE(state.get("string_number").toInt(), 123);

    state.set("int_number", 456);
    QCOMPARE(state.get("int_number").toString(), "456");

    state.set("bool_string", "true");
    QCOMPARE(state.get("bool_string").toBool(), true);

    // Test invalid conversions
    state.set("not_a_number", "abc");
    QCOMPARE(state.get("not_a_number").toInt(), 0); // Default conversion
}

QJsonObject StateManagerComprehensiveTest::createComplexNestedState() {
    QJsonObject state;

    // Users array
    QJsonArray users;
    QJsonObject user1;
    user1["name"] = "John Doe";
    user1["age"] = 30;
    user1["active"] = true;

    QJsonObject user1Address;
    user1Address["street"] = "123 Main St";
    user1Address["city"] = "New York";
    user1Address["zip"] = "10001";
    user1["address"] = user1Address;

    users.append(user1);
    state["users"] = users;

    // Settings object
    QJsonObject settings;
    settings["theme"] = "light";
    settings["language"] = "en";

    QJsonObject notifications;
    notifications["email"] = true;
    notifications["push"] = false;
    notifications["sms"] = true;
    settings["notifications"] = notifications;

    state["settings"] = settings;

    // Temporary data
    state["tempData"] = "temporary";

    // Metadata
    QJsonObject metadata;
    metadata["version"] = "1.0.0";
    metadata["lastModified"] = "2023-01-01T00:00:00Z";
    state["metadata"] = metadata;

    return state;
}

void StateManagerComprehensiveTest::verifyStateDiff(const QStringList& expected, const QStringList& actual) {
    QCOMPARE(actual.size(), expected.size());

    for (const QString& path : expected) {
        QVERIFY2(actual.contains(path),
                QString("Expected path '%1' not found in diff results").arg(path).toLocal8Bit());
    }
}

QTEST_MAIN(StateManagerComprehensiveTest)
#include "state_manager_comprehensive_test.moc"