#include <QTest>
#include <QSignalSpy>
#include <QJsonObject>
#include <QJsonDocument>
#include <memory>
#include "../TestUtilities.h"
#include "../../app/command/CommandManager.h"
#include "../../app/command/CommandInterface.h"

// Mock command for testing
class MockCommand : public QObject, public CommandInterface {
    Q_OBJECT

public:
    Q_INVOKABLE explicit MockCommand(const QString& name = "mock", QObject* parent = nullptr)
        : QObject(parent), m_name(name), m_executed(false), m_canExecute(true) {}

    QString name() const override { return m_name; }
    QString description() const override { return QString("Mock command: %1").arg(m_name); }
    bool canExecute() const override { return m_canExecute; }
    bool execute() override {
        if (!canExecute()) return false;
        m_executed = true;
        Q_EMIT executed();
        return true;
    }
    bool undo() override {
        if (!m_executed) return false;
        m_executed = false;
        Q_EMIT undone();
        return true;
    }
    void reset() override {
        m_executed = false;
        Q_EMIT reset();
    }

    QJsonObject serialize() const override {
        QJsonObject obj;
        obj["name"] = m_name;
        obj["executed"] = m_executed;
        obj["canExecute"] = m_canExecute;
        return obj;
    }

    void deserialize(const QJsonObject& data) override {
        m_name = data["name"].toString();
        m_executed = data["executed"].toBool();
        m_canExecute = data["canExecute"].toBool();
    }

    // Test helper methods
    void setCanExecute(bool canExecute) { m_canExecute = canExecute; }
    bool wasExecuted() const { return m_executed; }

signals:
    void executed();
    void undone();
    void reset();

private:
    QString m_name;
    bool m_executed;
    bool m_canExecute;
};

class CommandManagerTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // CommandManager functionality tests
    void testBasicFunctionality();
    void testCommandRegistration();
    void testCommandExecution();
    void testUndoRedo();
    void testCommandValidation();
    void testCommandSerialization();
    void testSignalEmission();
    void testErrorHandling();
    void testCommandHistory();
    void testBatchExecution();
};

void CommandManagerTest::initTestCase() {
    // Initialize test case
    qDebug() << "Initializing CommandManager tests";
}

void CommandManagerTest::cleanupTestCase() {
    // Clean up test case
    qDebug() << "Cleaning up CommandManager tests";
}

void CommandManagerTest::init() {
    // Initialize each test
    TestBase::init();
}

void CommandManagerTest::cleanup() {
    // Clean up after each test
    TestBase::cleanup();
}

void CommandManagerTest::testBasicFunctionality() {
    // Test basic Qt functionality (legacy test)
    QString testString = "Hello World";
    QCOMPARE(testString, "Hello World");
    QVERIFY(!testString.isEmpty());

    QStringList testList;
    testList << "item1" << "item2";
    QCOMPARE(testList.size(), 2);
    QVERIFY(testList.contains("item1"));
}

void CommandManagerTest::testCommandRegistration() {
    CommandManager manager;
    MockCommand* mockCommand = new MockCommand("testCommand");

    // Test command registration
    QVERIFY(manager.registerCommand("test", mockCommand));
    QVERIFY(manager.hasCommand("test"));
    QCOMPARE(manager.commandCount(), 1);

    // Test duplicate registration
    MockCommand* duplicateCommand = new MockCommand("duplicate");
    QVERIFY(!manager.registerCommand("test", duplicateCommand));

    // Test getting command
    CommandInterface* retrievedCommand = manager.getCommand("test");
    QVERIFY(retrievedCommand != nullptr);
    QCOMPARE(retrievedCommand->name(), "testCommand");

    // Test getting non-existent command
    QVERIFY(manager.getCommand("nonexistent") == nullptr);

    // Test unregistering command
    QVERIFY(manager.unregisterCommand("test"));
    QVERIFY(!manager.hasCommand("test"));
    QCOMPARE(manager.commandCount(), 0);
}

void CommandManagerTest::testCommandExecution() {
    CommandManager manager;
    MockCommand* mockCommand = new MockCommand("executable");
    manager.registerCommand("exec", mockCommand);

    // Test successful execution
    QSignalSpy executedSpy(mockCommand, &MockCommand::executed);
    QVERIFY(manager.executeCommand("exec"));
    QVERIFY(mockCommand->wasExecuted());
    QCOMPARE(executedSpy.count(), 1);

    // Reset command
    mockCommand->reset();
    QVERIFY(!mockCommand->wasExecuted());

    // Test execution of non-existent command
    QVERIFY(!manager.executeCommand("nonexistent"));

    // Test execution of command that cannot execute
    mockCommand->setCanExecute(false);
    QVERIFY(!manager.executeCommand("exec"));
    QVERIFY(!mockCommand->wasExecuted());
}

void CommandManagerTest::testUndoRedo() {
    CommandManager manager;
    MockCommand* mockCommand = new MockCommand("undoTest");
    manager.registerCommand("undo", mockCommand);

    // Execute command first
    QVERIFY(manager.executeCommand("undo"));
    QVERIFY(mockCommand->wasExecuted());

    // Test undo
    QSignalSpy undoneSpy(mockCommand, &MockCommand::undone);
    QVERIFY(manager.undo());
    QVERIFY(!mockCommand->wasExecuted());
    QCOMPARE(undoneSpy.count(), 1);

    // Test redo
    QVERIFY(manager.redo());
    QVERIFY(mockCommand->wasExecuted());

    // Test undo/redo with no history
    QVERIFY(manager.undo()); // Should succeed (undo the redo)
    QVERIFY(manager.undo()); // Should fail (no more to undo)
    QVERIFY(!manager.undo()); // Should fail
}

void CommandManagerTest::testCommandValidation() {
    CommandManager manager;
    MockCommand* validCommand = new MockCommand("valid");
    MockCommand* invalidCommand = new MockCommand("invalid");
    invalidCommand->setCanExecute(false);

    manager.registerCommand("valid", validCommand);
    manager.registerCommand("invalid", invalidCommand);

    // Test validation
    QVERIFY(manager.canExecute("valid"));
    QVERIFY(!manager.canExecute("invalid"));
    QVERIFY(!manager.canExecute("nonexistent"));
}

void CommandManagerTest::testCommandSerialization() {
    CommandManager manager;
    MockCommand* mockCommand = new MockCommand("serializable");
    manager.registerCommand("serial", mockCommand);

    // Execute command to create history
    manager.executeCommand("serial");

    // Serialize manager state
    QJsonObject serialized = manager.serialize();
    QVERIFY(!serialized.isEmpty());
    QVERIFY(serialized.contains("commands"));
    QVERIFY(serialized.contains("history"));

    // Create new manager and deserialize
    CommandManager newManager;
    QVERIFY(newManager.deserialize(serialized));
    QVERIFY(newManager.hasCommand("serial"));
}

void CommandManagerTest::testSignalEmission() {
    CommandManager manager;
    MockCommand* mockCommand = new MockCommand("signalTest");
    manager.registerCommand("signal", mockCommand);

    // Test command execution signal
    QSignalSpy commandExecutedSpy(&manager, &CommandManager::commandExecuted);
    QSignalSpy commandFailedSpy(&manager, &CommandManager::commandFailed);

    // Successful execution
    QVERIFY(manager.executeCommand("signal"));
    QCOMPARE(commandExecutedSpy.count(), 1);
    QCOMPARE(commandFailedSpy.count(), 0);

    // Failed execution
    mockCommand->setCanExecute(false);
    QVERIFY(!manager.executeCommand("signal"));
    QCOMPARE(commandExecutedSpy.count(), 1); // No change
    QCOMPARE(commandFailedSpy.count(), 1);
}

void CommandManagerTest::testErrorHandling() {
    CommandManager manager;

    // Test error handling for various scenarios
    QVERIFY(!manager.executeCommand(""));
    QVERIFY(!manager.executeCommand(nullptr));

    // Test null command registration
    QVERIFY(!manager.registerCommand("null", nullptr));

    // Test operations on empty manager
    QVERIFY(!manager.undo());
    QVERIFY(!manager.redo());
    QCOMPARE(manager.commandCount(), 0);
}

void CommandManagerTest::testCommandHistory() {
    CommandManager manager;
    MockCommand* cmd1 = new MockCommand("cmd1");
    MockCommand* cmd2 = new MockCommand("cmd2");
    MockCommand* cmd3 = new MockCommand("cmd3");

    manager.registerCommand("cmd1", cmd1);
    manager.registerCommand("cmd2", cmd2);
    manager.registerCommand("cmd3", cmd3);

    // Execute commands in sequence
    manager.executeCommand("cmd1");
    manager.executeCommand("cmd2");
    manager.executeCommand("cmd3");

    // Test history size
    QCOMPARE(manager.historySize(), 3);

    // Test current index
    QCOMPARE(manager.currentIndex(), 2); // 0-based index

    // Clear history
    manager.clearHistory();
    QCOMPARE(manager.historySize(), 0);
    QCOMPARE(manager.currentIndex(), -1);
}

void CommandManagerTest::testBatchExecution() {
    CommandManager manager;
    MockCommand* cmd1 = new MockCommand("batch1");
    MockCommand* cmd2 = new MockCommand("batch2");
    MockCommand* cmd3 = new MockCommand("batch3");

    manager.registerCommand("batch1", cmd1);
    manager.registerCommand("batch2", cmd2);
    manager.registerCommand("batch3", cmd3);

    QStringList commands = {"batch1", "batch2", "batch3"};

    // Test batch execution
    QVERIFY(manager.executeBatch(commands));
    QVERIFY(cmd1->wasExecuted());
    QVERIFY(cmd2->wasExecuted());
    QVERIFY(cmd3->wasExecuted());

    // Test batch execution with failing command
    MockCommand* failingCmd = new MockCommand("failing");
    failingCmd->setCanExecute(false);
    manager.registerCommand("failing", failingCmd);

    QStringList failingBatch = {"batch1", "failing", "batch3"};
    QVERIFY(!manager.executeBatch(failingBatch));
}

QTEST_MAIN(CommandManagerTest)
#include "command_manager_test.moc"
