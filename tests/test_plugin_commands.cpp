#include <QApplication>
#include <QDebug>
#include <QSignalSpy>
#include <QtTest/QtTest>

#include "command/PluginCommands.h"
#include "plugin/PluginManager.h"

/**
 * Test suite for Plugin Commands
 */
class PluginCommandsTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Command creation tests
    void testLoadPluginCommand();
    void testUnloadPluginCommand();
    void testEnablePluginCommand();
    void testDisablePluginCommand();
    void testScanPluginsCommand();

    // Command factory tests
    void testCommandFactory();

    // Command execution tests
    void testCanExecute();
    void testErrorHandling();

private:
    PluginManager* m_pluginManager;
};

void PluginCommandsTest::initTestCase() {
    qDebug() << "=== Plugin Commands Test Suite ===";
    qDebug() << "Qt version:" << QT_VERSION_STR;

    m_pluginManager = &PluginManager::instance();
    QVERIFY(m_pluginManager != nullptr);
}

void PluginCommandsTest::cleanupTestCase() {
    qDebug() << "=== Plugin Commands Tests Completed ===";
}

void PluginCommandsTest::testLoadPluginCommand() {
    qDebug() << "Testing LoadPluginCommand...";

    QString testPluginName = "TestPlugin";
    LoadPluginCommand command(m_pluginManager, testPluginName);

    // Check command metadata
    QCOMPARE(command.name(), QString("LoadPlugin"));
    QVERIFY(!command.description().isEmpty());

    // Plugin doesn't exist, so execution should fail
    bool result = command.execute();
    QVERIFY(!result);
    QVERIFY(command.hasError());

    qDebug() << "✓ LoadPluginCommand works";
}

void PluginCommandsTest::testUnloadPluginCommand() {
    qDebug() << "Testing UnloadPluginCommand...";

    QString testPluginName = "TestPlugin";
    UnloadPluginCommand command(m_pluginManager, testPluginName);

    // Check command metadata
    QCOMPARE(command.name(), QString("UnloadPlugin"));

    // Plugin is not loaded, so can't unload
    QVERIFY(!command.canExecute());

    // Execute anyway (should succeed since plugin isn't loaded)
    bool result = command.execute();
    QVERIFY(result);

    qDebug() << "✓ UnloadPluginCommand works";
}

void PluginCommandsTest::testEnablePluginCommand() {
    qDebug() << "Testing EnablePluginCommand...";

    QString testPluginName = "TestPlugin";
    EnablePluginCommand command(m_pluginManager, testPluginName);

    // Check command metadata
    QCOMPARE(command.name(), QString("EnablePlugin"));

    // Can enable non-existent plugin (will fail to load but won't error)
    bool result = command.execute();
    QVERIFY(result);

    qDebug() << "✓ EnablePluginCommand works";
}

void PluginCommandsTest::testDisablePluginCommand() {
    qDebug() << "Testing DisablePluginCommand...";

    QString testPluginName = "TestPlugin";
    DisablePluginCommand command(m_pluginManager, testPluginName);

    // Check command metadata
    QCOMPARE(command.name(), QString("DisablePlugin"));

    // Can execute if plugin is not loaded
    bool result = command.execute();
    QVERIFY(result);

    qDebug() << "✓ DisablePluginCommand works";
}

void PluginCommandsTest::testScanPluginsCommand() {
    qDebug() << "Testing ScanPluginsCommand...";

    ScanPluginsCommand command(m_pluginManager);

    // Check command metadata
    QCOMPARE(command.name(), QString("ScanPlugins"));

    // Should always be able to execute
    QVERIFY(command.canExecute());

    // Execute the command
    QSignalSpy executedSpy(&command, &PluginCommand::executed);
    QSignalSpy statusSpy(&command, &PluginCommand::statusMessage);

    bool result = command.execute();
    QVERIFY(result);

    // Check that signals were emitted
    QCOMPARE(executedSpy.count(), 1);
    QVERIFY(statusSpy.count() >= 1);

    // Check signal parameters
    QList<QVariant> arguments = executedSpy.takeFirst();
    QVERIFY(arguments.at(0).toBool());

    qDebug() << "✓ ScanPluginsCommand works";
}

void PluginCommandsTest::testCommandFactory() {
    qDebug() << "Testing PluginCommandFactory...";

    // Test creating different command types
    auto loadCmd =
        PluginCommandFactory::createLoadCommand(m_pluginManager, "Test");
    QVERIFY(loadCmd != nullptr);
    QCOMPARE(loadCmd->name(), QString("LoadPlugin"));

    auto unloadCmd =
        PluginCommandFactory::createUnloadCommand(m_pluginManager, "Test");
    QVERIFY(unloadCmd != nullptr);
    QCOMPARE(unloadCmd->name(), QString("UnloadPlugin"));

    auto enableCmd =
        PluginCommandFactory::createEnableCommand(m_pluginManager, "Test");
    QVERIFY(enableCmd != nullptr);
    QCOMPARE(enableCmd->name(), QString("EnablePlugin"));

    auto disableCmd =
        PluginCommandFactory::createDisableCommand(m_pluginManager, "Test");
    QVERIFY(disableCmd != nullptr);
    QCOMPARE(disableCmd->name(), QString("DisablePlugin"));

    auto scanCmd = PluginCommandFactory::createScanCommand(m_pluginManager);
    QVERIFY(scanCmd != nullptr);
    QCOMPARE(scanCmd->name(), QString("ScanPlugins"));

    // Test createCommandFromType
    auto cmdFromType =
        PluginCommandFactory::createCommandFromType("scan", m_pluginManager);
    QVERIFY(cmdFromType != nullptr);
    QCOMPARE(cmdFromType->name(), QString("ScanPlugins"));

    auto invalidCmd =
        PluginCommandFactory::createCommandFromType("invalid", m_pluginManager);
    QVERIFY(invalidCmd == nullptr);

    qDebug() << "✓ PluginCommandFactory works";
}

void PluginCommandsTest::testCanExecute() {
    qDebug() << "Testing canExecute()...";

    // Test with null manager
    LoadPluginCommand nullManagerCmd(nullptr, "Test");
    QVERIFY(!nullManagerCmd.canExecute());

    // Test with empty plugin name
    LoadPluginCommand emptyNameCmd(m_pluginManager, "");
    QVERIFY(!emptyNameCmd.canExecute());

    // Test with valid parameters
    ScanPluginsCommand validCmd(m_pluginManager);
    QVERIFY(validCmd.canExecute());

    qDebug() << "✓ canExecute() validation works";
}

void PluginCommandsTest::testErrorHandling() {
    qDebug() << "Testing error handling...";

    LoadPluginCommand command(m_pluginManager, "NonExistentPlugin");

    // Initially no error
    QVERIFY(!command.hasError());
    QVERIFY(command.errorMessage().isEmpty());

    // Execute (will fail)
    command.execute();

    // Now should have error
    QVERIFY(command.hasError());
    QVERIFY(!command.errorMessage().isEmpty());

    qDebug() << "Error message:" << command.errorMessage();
    qDebug() << "✓ Error handling works";
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Set up for headless testing
    app.setAttribute(Qt::AA_Use96Dpi, true);

    PluginCommandsTest test;
    int result = QTest::qExec(&test, argc, argv);

    if (result == 0) {
        qDebug() << "SUCCESS: All Plugin Commands tests passed";
    } else {
        qDebug() << "FAILURE: Some Plugin Commands tests failed";
    }

    return result;
}

#include "test_plugin_commands.moc"
