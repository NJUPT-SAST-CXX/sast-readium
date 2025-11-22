#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "plugin/PluginInterface.h"
#include "plugin/PluginManager.h"

/**
 * Test suite for PluginManager
 */
class PluginManagerTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // PluginManager basic tests
    void testSingleton();
    void testPluginDirectories();
    void testScanForPlugins();
    void testPluginMetadata();

    // Plugin state management
    void testEnableDisablePlugin();
    void testLoadUnloadPlugin();

    // Plugin settings
    void testSaveLoadSettings();

    // Plugin dependency resolution
    void testDependencyResolution();
    void testCyclicDependencyDetection();

    // Plugin queries
    void testPluginQueries();
    void testFeatureQueries();

private:
    PluginManager* m_pluginManager;
    QTemporaryDir m_tempDir;
};

void PluginManagerTest::initTestCase() {
    qDebug() << "=== PluginManager Test Suite ===";
    qDebug() << "Qt version:" << QT_VERSION_STR;

    // Get PluginManager instance
    m_pluginManager = &PluginManager::instance();
    QVERIFY(m_pluginManager != nullptr);

    // Create temporary directory for test plugins
    QVERIFY(m_tempDir.isValid());
    qDebug() << "Temporary plugin directory:" << m_tempDir.path();

    // Set test plugin directory
    QStringList dirs;
    dirs << m_tempDir.path();
    m_pluginManager->setPluginDirectories(dirs);
}

void PluginManagerTest::cleanupTestCase() {
    // Unload all plugins
    if (m_pluginManager) {
        m_pluginManager->unloadAllPlugins();
    }

    qDebug() << "=== PluginManager Tests Completed ===";
}

void PluginManagerTest::testSingleton() {
    qDebug() << "Testing PluginManager singleton...";

    PluginManager& instance1 = PluginManager::instance();
    PluginManager& instance2 = PluginManager::instance();

    // Both references should point to the same object
    QCOMPARE(&instance1, &instance2);

    qDebug() << "✓ Singleton pattern verified";
}

void PluginManagerTest::testPluginDirectories() {
    qDebug() << "Testing plugin directories...";

    QStringList testDirs;
    testDirs << "/test/path1" << "/test/path2";

    m_pluginManager->setPluginDirectories(testDirs);
    QStringList retrievedDirs = m_pluginManager->getPluginDirectories();

    QCOMPARE(retrievedDirs.size(), testDirs.size());
    for (const QString& dir : testDirs) {
        QVERIFY(retrievedDirs.contains(dir));
    }

    // Restore temp directory
    QStringList dirs;
    dirs << m_tempDir.path();
    m_pluginManager->setPluginDirectories(dirs);

    qDebug() << "✓ Plugin directories management works";
}

void PluginManagerTest::testScanForPlugins() {
    qDebug() << "Testing plugin scanning...";

    // Scan for plugins (should find none in empty temp directory)
    m_pluginManager->scanForPlugins();

    QStringList availablePlugins = m_pluginManager->getAvailablePlugins();

    // Temp directory is empty, so no plugins should be found
    QCOMPARE(availablePlugins.size(), 0);

    qDebug() << "✓ Plugin scanning works (found" << availablePlugins.size()
             << "plugins)";
}

void PluginManagerTest::testPluginMetadata() {
    qDebug() << "Testing plugin metadata...";

    // Get all plugin metadata
    QHash<QString, PluginMetadata> allMetadata =
        m_pluginManager->getAllPluginMetadata();

    // Should be empty since we haven't loaded any plugins
    QCOMPARE(allMetadata.size(), 0);

    // Test getting metadata for non-existent plugin
    PluginMetadata metadata = m_pluginManager->getPluginMetadata("NonExistent");
    QVERIFY(metadata.name.isEmpty());

    qDebug() << "✓ Plugin metadata retrieval works";
}

void PluginManagerTest::testEnableDisablePlugin() {
    qDebug() << "Testing enable/disable plugin...";

    // Since we have no real plugins, we'll test the API interface
    QString testPluginName = "TestPlugin";

    // Try to enable a non-existent plugin
    m_pluginManager->setPluginEnabled(testPluginName, true);

    // The plugin doesn't exist, so isPluginEnabled should return false
    bool isEnabled = m_pluginManager->isPluginEnabled(testPluginName);
    QVERIFY(!isEnabled);

    qDebug() << "✓ Enable/disable plugin API works";
}

void PluginManagerTest::testLoadUnloadPlugin() {
    qDebug() << "Testing load/unload plugin...";

    QString testPluginName = "TestPlugin";

    // Try to load a non-existent plugin
    bool loadResult = m_pluginManager->loadPlugin(testPluginName);
    QVERIFY(!loadResult);

    // Try to unload a non-existent plugin
    bool unloadResult = m_pluginManager->unloadPlugin(testPluginName);
    QVERIFY(unloadResult);  // Unload returns true if plugin wasn't loaded

    // Check that plugin is not loaded
    QVERIFY(!m_pluginManager->isPluginLoaded(testPluginName));

    qDebug() << "✓ Load/unload plugin API works";
}

void PluginManagerTest::testSaveLoadSettings() {
    qDebug() << "Testing save/load settings...";

    // Save settings
    m_pluginManager->saveSettings();

    // Load settings
    m_pluginManager->loadSettings();

    qDebug() << "✓ Save/load settings works";
}

void PluginManagerTest::testDependencyResolution() {
    qDebug() << "Testing dependency resolution...";

    // Create test metadata with dependencies
    QHash<QString, PluginMetadata> testPlugins;

    PluginMetadata plugin1;
    plugin1.name = "Plugin1";
    plugin1.dependencies = QStringList();
    testPlugins["Plugin1"] = plugin1;

    PluginMetadata plugin2;
    plugin2.name = "Plugin2";
    plugin2.dependencies = QStringList() << "Plugin1";
    testPlugins["Plugin2"] = plugin2;

    PluginMetadata plugin3;
    plugin3.name = "Plugin3";
    plugin3.dependencies = QStringList() << "Plugin2";
    testPlugins["Plugin3"] = plugin3;

    // Resolve dependencies
    QStringList loadOrder =
        PluginDependencyResolver::resolveDependencies(testPlugins);

    // Plugin1 should come before Plugin2, and Plugin2 before Plugin3
    int idx1 = loadOrder.indexOf("Plugin1");
    int idx2 = loadOrder.indexOf("Plugin2");
    int idx3 = loadOrder.indexOf("Plugin3");

    QVERIFY(idx1 < idx2);
    QVERIFY(idx2 < idx3);

    qDebug() << "✓ Dependency resolution works. Load order:" << loadOrder;
}

void PluginManagerTest::testCyclicDependencyDetection() {
    qDebug() << "Testing cyclic dependency detection...";

    // Create test metadata with cyclic dependencies
    QHash<QString, PluginMetadata> cyclicPlugins;

    PluginMetadata pluginA;
    pluginA.name = "PluginA";
    pluginA.dependencies = QStringList() << "PluginB";
    cyclicPlugins["PluginA"] = pluginA;

    PluginMetadata pluginB;
    pluginB.name = "PluginB";
    pluginB.dependencies = QStringList() << "PluginC";
    cyclicPlugins["PluginB"] = pluginB;

    PluginMetadata pluginC;
    pluginC.name = "PluginC";
    pluginC.dependencies = QStringList() << "PluginA";  // Creates cycle
    cyclicPlugins["PluginC"] = pluginC;

    // Check for cyclic dependencies
    bool hasCycle =
        PluginDependencyResolver::hasCyclicDependencies(cyclicPlugins);
    QVERIFY(hasCycle);

    qDebug() << "✓ Cyclic dependency detection works";
}

void PluginManagerTest::testPluginQueries() {
    qDebug() << "Testing plugin queries...";

    // Test various query methods
    QStringList availablePlugins = m_pluginManager->getAvailablePlugins();
    QStringList loadedPlugins = m_pluginManager->getLoadedPlugins();
    QStringList enabledPlugins = m_pluginManager->getEnabledPlugins();

    // All should be empty since we have no plugins
    QCOMPARE(availablePlugins.size(), 0);
    QCOMPARE(loadedPlugins.size(), 0);
    QCOMPARE(enabledPlugins.size(), 0);

    qDebug() << "✓ Plugin query methods work";
}

void PluginManagerTest::testFeatureQueries() {
    qDebug() << "Testing feature queries...";

    QString testFeature = "TestFeature";

    // Query for plugins with a feature
    QStringList pluginsWithFeature =
        m_pluginManager->getPluginsWithFeature(testFeature);
    QCOMPARE(pluginsWithFeature.size(), 0);

    // Check if feature is available
    bool featureAvailable = m_pluginManager->isFeatureAvailable(testFeature);
    QVERIFY(!featureAvailable);

    qDebug() << "✓ Feature query methods work";
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Set up for headless testing
    app.setAttribute(Qt::AA_Use96Dpi, true);

    PluginManagerTest test;
    int result = QTest::qExec(&test, argc, argv);

    if (result == 0) {
        qDebug() << "SUCCESS: All PluginManager tests passed";
    } else {
        qDebug() << "FAILURE: Some PluginManager tests failed";
    }

    return result;
}

#include "test_plugin_manager.moc"
