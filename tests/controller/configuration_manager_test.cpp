#include <QApplication>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTest>
#include "../../app/controller/ConfigurationManager.h"
#include "../TestUtilities.h"

class ConfigurationManagerTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override {
        // Ensure QApplication exists for QSettings
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            new QApplication(argc, argv);
        }
    }

    void init() override {
        // Reset configuration before each test
        ConfigurationManager::instance().resetToDefaults();
    }

    void cleanup() override {
        // Clean up after each test
        ConfigurationManager::instance().clearRuntimeValues();
    }

    // Singleton tests
    void testSingletonInstance() {
        ConfigurationManager& instance1 = ConfigurationManager::instance();
        ConfigurationManager& instance2 = ConfigurationManager::instance();

        // Should be the same instance
        QVERIFY(&instance1 == &instance2);
    }

    // Basic value operations
    void testSetAndGetValue() {
        ConfigurationManager& config = ConfigurationManager::instance();

        QSignalSpy configChangedSpy(
            &config, static_cast<void (ConfigurationManager::*)(
                         const QString&, const QVariant&)>(
                         &ConfigurationManager::configurationChanged));

        // Test basic value operations
        config.setValue("test.key", "test_value");
        QCOMPARE(config.getValue("test.key").toString(), QString("test_value"));

        // Should emit signal
        QCOMPARE(configChangedSpy.count(), 1);
        QList<QVariant> arguments = configChangedSpy.takeFirst();
        QCOMPARE(arguments.at(0).toString(), QString("test.key"));
        QCOMPARE(arguments.at(1).toString(), QString("test_value"));
    }

    void testDefaultValues() {
        ConfigurationManager& config = ConfigurationManager::instance();

        // Test default value return
        QCOMPARE(config.getValue("nonexistent.key", "default").toString(),
                 QString("default"));
        QCOMPARE(config.getValue("nonexistent.key").toString(), QString());
    }

    // Group-based operations
    void testGroupBasedOperations() {
        ConfigurationManager& config = ConfigurationManager::instance();

        QSignalSpy groupConfigChangedSpy(
            &config, QOverload<ConfigurationManager::ConfigGroup,
                               const QString&, const QVariant&>::
                         of(&ConfigurationManager::configurationChanged));

        // Test group-based operations
        config.setValue(ConfigurationManager::UI, "theme", "dark");
        QCOMPARE(config.getValue(ConfigurationManager::UI, "theme").toString(),
                 QString("dark"));

        // Should emit group-based signal
        QCOMPARE(groupConfigChangedSpy.count(), 1);
        QList<QVariant> arguments = groupConfigChangedSpy.takeFirst();
        QCOMPARE(arguments.at(0).toInt(),
                 static_cast<int>(ConfigurationManager::UI));
        QCOMPARE(arguments.at(1).toString(), QString("theme"));
        QCOMPARE(arguments.at(2).toString(), QString("dark"));
    }

    // Type-safe accessors
    void testTypeSafeAccessors() {
        ConfigurationManager& config = ConfigurationManager::instance();

        // Test boolean accessor
        config.setValue("test.bool", true);
        QVERIFY(config.getBool("test.bool"));
        QVERIFY(!config.getBool("nonexistent.bool", false));

        // Test integer accessor
        config.setValue("test.int", 42);
        QCOMPARE(config.getInt("test.int"), 42);
        QCOMPARE(config.getInt("nonexistent.int", 100), 100);

        // Test double accessor
        config.setValue("test.double", 3.14);
        QCOMPARE(config.getDouble("test.double"), 3.14);
        QCOMPARE(config.getDouble("nonexistent.double", 2.71), 2.71);

        // Test string accessor
        config.setValue("test.string", "hello");
        QCOMPARE(config.getString("test.string"), QString("hello"));
        QCOMPARE(config.getString("nonexistent.string", "world"),
                 QString("world"));

        // Test string list accessor
        QStringList testList = {"item1", "item2", "item3"};
        config.setValue("test.stringlist", testList);
        QCOMPARE(config.getStringList("test.stringlist"), testList);

        QStringList defaultList = {"default1", "default2"};
        QCOMPARE(config.getStringList("nonexistent.stringlist", defaultList),
                 defaultList);
    }

    // Configuration management
    void testSaveAndLoadConfiguration() {
        ConfigurationManager& config = ConfigurationManager::instance();

        QSignalSpy savedSpy(&config, &ConfigurationManager::configurationSaved);
        QSignalSpy loadedSpy(&config,
                             &ConfigurationManager::configurationLoaded);

        // Set some values
        config.setValue("test.save", "saved_value");
        config.setValue(ConfigurationManager::General, "app.version", "1.0.0");

        // Save configuration
        config.saveConfiguration();
        QCOMPARE(savedSpy.count(), 1);

        // Reset and load
        config.resetToDefaults();
        config.loadConfiguration();
        QCOMPARE(loadedSpy.count(), 1);

        // Values should be restored
        QCOMPARE(config.getValue("test.save").toString(),
                 QString("saved_value"));
        QCOMPARE(config.getValue(ConfigurationManager::General, "app.version")
                     .toString(),
                 QString("1.0.0"));
    }

    void testResetToDefaults() {
        ConfigurationManager& config = ConfigurationManager::instance();

        QSignalSpy resetSpy(&config, &ConfigurationManager::configurationReset);

        // Set some values
        config.setValue("test.reset", "will_be_reset");
        QCOMPARE(config.getValue("test.reset").toString(),
                 QString("will_be_reset"));

        // Reset to defaults
        config.resetToDefaults();
        QCOMPARE(resetSpy.count(), 1);

        // Value should be gone (return default)
        QCOMPARE(config.getValue("test.reset", "default").toString(),
                 QString("default"));
    }

    void testResetGroup() {
        ConfigurationManager& config = ConfigurationManager::instance();

        // Set values in different groups
        config.setValue(ConfigurationManager::UI, "theme", "dark");
        config.setValue(ConfigurationManager::UI, "font_size", 12);
        config.setValue(ConfigurationManager::General, "language", "en");

        // Reset UI group
        config.resetGroup(ConfigurationManager::UI);

        // UI values should be reset, General should remain
        QCOMPARE(config.getValue(ConfigurationManager::UI, "theme", "light")
                     .toString(),
                 QString("light"));
        QCOMPARE(
            config.getValue(ConfigurationManager::UI, "font_size", 10).toInt(),
            10);
        QCOMPARE(config.getValue(ConfigurationManager::General, "language")
                     .toString(),
                 QString("en"));
    }

    // Runtime configuration
    void testRuntimeValues() {
        ConfigurationManager& config = ConfigurationManager::instance();

        // Set runtime values
        config.setRuntimeValue("runtime.test", "runtime_value");
        QCOMPARE(config.getRuntimeValue("runtime.test").toString(),
                 QString("runtime_value"));
        QCOMPARE(
            config.getRuntimeValue("nonexistent.runtime", "default").toString(),
            QString("default"));

        // Runtime values should not affect persistent configuration
        QCOMPARE(config.getValue("runtime.test", "not_found").toString(),
                 QString("not_found"));

        // Clear runtime values
        config.clearRuntimeValues();
        QCOMPARE(config.getRuntimeValue("runtime.test", "cleared").toString(),
                 QString("cleared"));
    }

    // Configuration monitoring
    void testConfigurationWatching() {
        ConfigurationManager& config = ConfigurationManager::instance();

        // Watch a key
        config.watchKey("watched.key");
        QVERIFY(config.isWatching("watched.key"));
        QVERIFY(!config.isWatching("unwatched.key"));

        // Unwatch the key
        config.unwatchKey("watched.key");
        QVERIFY(!config.isWatching("watched.key"));
    }

    // Import/Export
    void testImportExportConfiguration() {
        ConfigurationManager& config = ConfigurationManager::instance();

        // Set some test values
        config.setValue("export.test1", "value1");
        config.setValue("export.test2", 42);
        config.setValue(ConfigurationManager::UI, "export.theme", "dark");

        // Create temporary file for export
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        QString exportPath = tempFile.fileName();
        tempFile.close();

        // Export configuration
        bool exportResult = config.exportConfiguration(exportPath);
        QVERIFY(exportResult);

        // Reset configuration
        config.resetToDefaults();
        QCOMPARE(config.getValue("export.test1", "not_found").toString(),
                 QString("not_found"));

        // Import configuration
        bool importResult = config.importConfiguration(exportPath);
        QVERIFY(importResult);

        // Values should be restored
        QCOMPARE(config.getValue("export.test1").toString(), QString("value1"));
        QCOMPARE(config.getInt("export.test2"), 42);
        QCOMPARE(config.getValue(ConfigurationManager::UI, "export.theme")
                     .toString(),
                 QString("dark"));
    }

    // Validation
    void testConfigurationValidation() {
        ConfigurationManager& config = ConfigurationManager::instance();

        QSignalSpy validationFailedSpy(&config,
                                       &ConfigurationManager::validationFailed);

        // Basic validation should pass for normal values
        config.setValue("valid.string", "test");
        config.setValue("valid.number", 100);
        bool validationResult = config.validateConfiguration();

        // Should pass basic validation
        QVERIFY(validationResult);
        QCOMPARE(validationFailedSpy.count(), 0);
    }

    // Configuration groups enum
    void testConfigurationGroups() {
        // Test that all enum values are valid
        QVERIFY(static_cast<int>(ConfigurationManager::General) >= 0);
        QVERIFY(static_cast<int>(ConfigurationManager::UI) >= 0);
        QVERIFY(static_cast<int>(ConfigurationManager::Document) >= 0);
        QVERIFY(static_cast<int>(ConfigurationManager::View) >= 0);
        QVERIFY(static_cast<int>(ConfigurationManager::Navigation) >= 0);
        QVERIFY(static_cast<int>(ConfigurationManager::Performance) >= 0);
        QVERIFY(static_cast<int>(ConfigurationManager::Network) >= 0);
        QVERIFY(static_cast<int>(ConfigurationManager::Advanced) >= 0);
    }

    // Convenience macros
    void testConvenienceMacros() {
        // Test that macros work
        CONFIG_SET("macro.test", "macro_value");
        QCOMPARE(CONFIG_GET("macro.test", "not_found").toString(),
                 QString("macro_value"));

        CONFIG_SET("macro.bool", true);
        QVERIFY(CONFIG_BOOL("macro.bool", false));

        CONFIG_SET("macro.int", 123);
        QCOMPARE(CONFIG_INT("macro.int", 0), 123);

        CONFIG_SET("macro.string", "hello");
        QCOMPARE(CONFIG_STRING("macro.string", "world"), QString("hello"));
    }
};

QTEST_MAIN(ConfigurationManagerTest)
#include "configuration_manager_test.moc"
