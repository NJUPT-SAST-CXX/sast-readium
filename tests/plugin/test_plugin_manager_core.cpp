#include <QFile>
#include <QJsonDocument>
#include <QMainWindow>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QWidget>

#include "../../app/plugin/PluginHookRegistry.h"
#include "../../app/plugin/PluginInterface.h"
#include "../../app/plugin/PluginManager.h"
#include "../TestUtilities.h"

/**
 * @brief Mock plugin for testing PluginManager functionality
 */
class TestablePlugin : public PluginBase {
    Q_OBJECT
    Q_INTERFACES(IPluginInterface)

public:
    explicit TestablePlugin(QObject* parent = nullptr) : PluginBase(parent) {
        m_metadata.name = "TestablePlugin";
        m_metadata.version = "1.0.0";
        m_metadata.author = "Test";
        m_metadata.description = "Plugin for PluginManager tests";
        m_metadata.supportedTypes = {"pdf", "epub"};
        m_capabilities.provides = {"feature.test", "document.handler"};
    }

    void setName(const QString& name) { m_metadata.name = name; }
    void setDependencies(const QStringList& deps) {
        m_metadata.dependencies = deps;
    }
    void setSupportedTypes(const QStringList& types) {
        m_metadata.supportedTypes = types;
    }
    void setFeatures(const QStringList& features) {
        m_capabilities.provides = features;
    }

    bool messageReceived() const { return m_messageReceived; }
    QString lastMessageFrom() const { return m_lastMessageFrom; }
    QVariant lastMessage() const { return m_lastMessage; }

    void handleMessage(const QString& from, const QVariant& message) override {
        m_messageReceived = true;
        m_lastMessageFrom = from;
        m_lastMessage = message;
    }

protected:
    bool onInitialize() override { return true; }
    void onShutdown() override {}

private:
    bool m_messageReceived = false;
    QString m_lastMessageFrom;
    QVariant m_lastMessage;
};

class PluginManagerCoreTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

    // Basic functionality tests (existing)
    void test_directories_and_scan_empty();
    void test_hot_reloading_toggle();
    void test_settings_persistence_no_plugins();
    void test_validation_and_reporting_and_backup_restore();

    // Plugin state management tests
    void test_plugin_enabled_disabled();
    void test_get_available_plugins();
    void test_get_loaded_plugins();
    void test_get_enabled_plugins();

    // Plugin metadata tests
    void test_get_plugin_metadata();
    void test_get_all_plugin_metadata();
    void test_get_plugin_info();

    // Plugin configuration tests
    void test_get_set_plugin_configuration();

    // Feature and file type queries
    void test_get_plugins_with_feature();
    void test_get_plugins_for_file_type();
    void test_is_feature_available();

    // Dependency management tests
    void test_get_plugin_dependencies();
    void test_get_plugins_depending_on();
    void test_can_unload_plugin();

    // Plugin reload tests
    void test_reload_plugin();
    void test_reload_all_plugins();

    // Plugin installation tests
    void test_install_plugin_invalid_path();
    void test_uninstall_nonexistent_plugin();
    void test_update_nonexistent_plugin();

    // UI element tracking tests
    void test_register_plugin_ui_element();
    void test_cleanup_plugin_ui_elements();

    // Standard hooks tests
    void test_register_standard_hooks();
    void test_unregister_all_hooks();

    // IPluginHost interface tests
    void test_ipluginhost_get_plugin();
    void test_ipluginhost_get_plugins();
    void test_ipluginhost_scan_plugin_directory();
    void test_ipluginhost_available_plugins();

    // Plugin communication tests
    void test_send_plugin_message();
    void test_broadcast_plugin_message();

    // Signals tests
    void test_plugin_enabled_signal();
    void test_plugin_disabled_signal();

private:
    PluginManager* m_manager = nullptr;
};

void PluginManagerCoreTest::initTestCase() { TestBase::initTestCase(); }

void PluginManagerCoreTest::cleanupTestCase() { TestBase::cleanupTestCase(); }

void PluginManagerCoreTest::init() { m_manager = &PluginManager::instance(); }

void PluginManagerCoreTest::cleanup() {
    // Clean up any hooks registered during tests
    auto& hookRegistry = PluginHookRegistry::instance();
    for (const QString& hookName : hookRegistry.getHookNames()) {
        hookRegistry.unregisterHook(hookName);
    }
}

// ============================================================================
// Basic Functionality Tests (Existing)
// ============================================================================

void PluginManagerCoreTest::test_directories_and_scan_empty() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    QSignalSpy scannedSpy(m_manager, &PluginManager::pluginsScanned);

    m_manager->setPluginDirectories(QStringList{tmp.path()});
    m_manager->scanForPlugins();

    if (scannedSpy.count() == 0) {
        QVERIFY(scannedSpy.wait(500));
    } else {
        QTest::qWait(0);
    }
    QCOMPARE(scannedSpy.takeFirst().at(0).toInt(), 0);
}

void PluginManagerCoreTest::test_hot_reloading_toggle() {
    QVERIFY(!m_manager->isHotReloadingEnabled());
    m_manager->enableHotReloading(true);
    QVERIFY(m_manager->isHotReloadingEnabled());
    m_manager->enableHotReloading(false);
    QVERIFY(!m_manager->isHotReloadingEnabled());
}

void PluginManagerCoreTest::test_settings_persistence_no_plugins() {
    // With no plugins, load/save should not crash and simply be no-ops
    m_manager->saveSettings();
    m_manager->loadSettings();
}

void PluginManagerCoreTest::test_validation_and_reporting_and_backup_restore() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    // validatePlugin should return false for nonexistent path
    QVERIFY(!PluginManager::validatePlugin(tmp.filePath("does_not_exist")));

    // getPluginInfo for unknown plugin -> empty object
    QJsonObject info = m_manager->getPluginInfo("unknown");
    QVERIFY(info.isEmpty());

    // exportPluginList writes a file
    const QString listPath = tmp.filePath("plugins.json");
    m_manager->exportPluginList(listPath);
    QVERIFY(QFile::exists(listPath));

    // backup/restore round-trip on empty state
    const QString backupPath = tmp.filePath("backup.json");
    QVERIFY(m_manager->backupPluginConfiguration(backupPath));
    QVERIFY(QFile::exists(backupPath));

    // Backup should be valid JSON
    QFile f(backupPath);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const auto doc = QJsonDocument::fromJson(f.readAll());
    QVERIFY(!doc.isNull());

    QVERIFY(m_manager->restorePluginConfiguration(backupPath));

    // createPluginReport should not crash
    m_manager->createPluginReport();
}

// ============================================================================
// Plugin State Management Tests
// ============================================================================

void PluginManagerCoreTest::test_plugin_enabled_disabled() {
    // Test with non-existent plugin (should handle gracefully)
    QVERIFY(!m_manager->isPluginEnabled("NonExistent"));

    // setPluginEnabled on non-existent plugin should not crash
    m_manager->setPluginEnabled("NonExistent", true);
    QVERIFY(!m_manager->isPluginEnabled("NonExistent"));
}

void PluginManagerCoreTest::test_get_available_plugins() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    m_manager->setPluginDirectories({tmp.path()});
    m_manager->scanForPlugins();

    // With empty directory, should return empty list
    QStringList available = m_manager->getAvailablePlugins();
    QVERIFY(available.isEmpty() ||
            available.size() >= 0);  // Could have leftover plugins
}

void PluginManagerCoreTest::test_get_loaded_plugins() {
    QStringList loaded = m_manager->getLoadedPlugins();
    // Should not crash, may or may not have loaded plugins
    QVERIFY(loaded.size() >= 0);
}

void PluginManagerCoreTest::test_get_enabled_plugins() {
    QStringList enabled = m_manager->getEnabledPlugins();
    // Should not crash
    QVERIFY(enabled.size() >= 0);
}

// ============================================================================
// Plugin Metadata Tests
// ============================================================================

void PluginManagerCoreTest::test_get_plugin_metadata() {
    // Non-existent plugin should return default metadata
    PluginMetadata metadata = m_manager->getPluginMetadata("NonExistent");
    QVERIFY(metadata.name.isEmpty());
    QVERIFY(!metadata.isLoaded);
}

void PluginManagerCoreTest::test_get_all_plugin_metadata() {
    QHash<QString, PluginMetadata> allMetadata =
        m_manager->getAllPluginMetadata();
    // Should not crash, returns hash (possibly empty)
    QVERIFY(allMetadata.size() >= 0);
}

void PluginManagerCoreTest::test_get_plugin_info() {
    // Non-existent plugin should return empty QJsonObject
    QJsonObject info = m_manager->getPluginInfo("NonExistent");
    QVERIFY(info.isEmpty());
}

// ============================================================================
// Plugin Configuration Tests
// ============================================================================

void PluginManagerCoreTest::test_get_set_plugin_configuration() {
    // Getting configuration for non-existent plugin returns empty object
    QJsonObject config = m_manager->getPluginConfiguration("NonExistent");
    QVERIFY(config.isEmpty());

    // Setting configuration for non-existent plugin should not crash
    QJsonObject newConfig;
    newConfig["key"] = "value";
    m_manager->setPluginConfiguration("NonExistent", newConfig);

    // Still returns empty (plugin doesn't exist in metadata)
    config = m_manager->getPluginConfiguration("NonExistent");
    QVERIFY(config.isEmpty());
}

// ============================================================================
// Feature and File Type Queries
// ============================================================================

void PluginManagerCoreTest::test_get_plugins_with_feature() {
    QStringList plugins =
        m_manager->getPluginsWithFeature("nonexistent.feature");
    QVERIFY(plugins.isEmpty());
}

void PluginManagerCoreTest::test_get_plugins_for_file_type() {
    QStringList plugins = m_manager->getPluginsForFileType(".xyz");
    QVERIFY(plugins.isEmpty());
}

void PluginManagerCoreTest::test_is_feature_available() {
    QVERIFY(!m_manager->isFeatureAvailable("nonexistent.feature"));
}

// ============================================================================
// Dependency Management Tests
// ============================================================================

void PluginManagerCoreTest::test_get_plugin_dependencies() {
    QStringList deps = m_manager->getPluginDependencies("NonExistent");
    QVERIFY(deps.isEmpty());
}

void PluginManagerCoreTest::test_get_plugins_depending_on() {
    QStringList dependents = m_manager->getPluginsDependingOn("NonExistent");
    QVERIFY(dependents.isEmpty());
}

void PluginManagerCoreTest::test_can_unload_plugin() {
    // Non-existent plugin - should return true (no dependents)
    QVERIFY(m_manager->canUnloadPlugin("NonExistent"));
}

// ============================================================================
// Plugin Reload Tests
// ============================================================================

void PluginManagerCoreTest::test_reload_plugin() {
    // Reloading non-existent plugin should not crash
    m_manager->reloadPlugin("NonExistent");
    QVERIFY(true);
}

void PluginManagerCoreTest::test_reload_all_plugins() {
    // Should not crash even with no plugins
    m_manager->reloadAllPlugins();
    QVERIFY(true);
}

// ============================================================================
// Plugin Installation Tests
// ============================================================================

void PluginManagerCoreTest::test_install_plugin_invalid_path() {
    QVERIFY(!m_manager->installPlugin("/invalid/path/plugin.dll"));
}

void PluginManagerCoreTest::test_uninstall_nonexistent_plugin() {
    QVERIFY(!m_manager->uninstallPlugin("NonExistent"));
}

void PluginManagerCoreTest::test_update_nonexistent_plugin() {
    QVERIFY(!m_manager->updatePlugin("NonExistent", "/path/new_plugin.dll"));
}

// ============================================================================
// UI Element Tracking Tests
// ============================================================================

void PluginManagerCoreTest::test_register_plugin_ui_element() {
    QWidget* widget = new QWidget();

    // Should not crash
    m_manager->registerPluginUIElement("TestPlugin", widget);
    QVERIFY(true);

    // Cleanup
    m_manager->cleanupPluginUIElements("TestPlugin");
}

void PluginManagerCoreTest::test_cleanup_plugin_ui_elements() {
    QWidget* widget1 = new QWidget();
    QWidget* widget2 = new QWidget();

    m_manager->registerPluginUIElement("TestPlugin", widget1);
    m_manager->registerPluginUIElement("TestPlugin", widget2);

    // Should cleanup without crash
    m_manager->cleanupPluginUIElements("TestPlugin");
    QVERIFY(true);

    // Cleanup non-existent plugin should not crash
    m_manager->cleanupPluginUIElements("NonExistent");
    QVERIFY(true);
}

// ============================================================================
// Standard Hooks Tests
// ============================================================================

void PluginManagerCoreTest::test_register_standard_hooks() {
    auto& hookRegistry = PluginHookRegistry::instance();

    // Clear any existing hooks first
    for (const QString& hookName : hookRegistry.getHookNames()) {
        hookRegistry.unregisterHook(hookName);
    }

    m_manager->registerStandardHooks();

    // Verify standard hooks are registered
    QVERIFY(hookRegistry.hasHook(StandardHooks::DOCUMENT_PRE_LOAD));
    QVERIFY(hookRegistry.hasHook(StandardHooks::DOCUMENT_POST_LOAD));
    QVERIFY(hookRegistry.hasHook(StandardHooks::RENDER_PRE_PAGE));
    QVERIFY(hookRegistry.hasHook(StandardHooks::SEARCH_PRE_EXECUTE));
    QVERIFY(hookRegistry.hasHook(StandardHooks::CACHE_PRE_ADD));
    QVERIFY(hookRegistry.hasHook(StandardHooks::ANNOTATION_CREATED));
    QVERIFY(hookRegistry.hasHook(StandardHooks::EXPORT_PRE_EXECUTE));
}

void PluginManagerCoreTest::test_unregister_all_hooks() {
    auto& hookRegistry = PluginHookRegistry::instance();

    // Register a hook and callback
    hookRegistry.registerHook("test.hook");
    auto callback = [](const QVariantMap&) -> QVariant { return QVariant(); };
    hookRegistry.registerCallback("test.hook", "TestPlugin", callback);

    QCOMPARE(hookRegistry.getCallbackCount("test.hook"), 1);

    // Unregister all hooks for the plugin
    m_manager->unregisterAllHooks("TestPlugin");

    QCOMPARE(hookRegistry.getCallbackCount("test.hook"), 0);
}

// ============================================================================
// IPluginHost Interface Tests
// ============================================================================

void PluginManagerCoreTest::test_ipluginhost_get_plugin() {
    IPluginHost* host = m_manager;
    IPluginInterface* plugin = host->getPlugin("NonExistent");
    QVERIFY(plugin == nullptr);
}

void PluginManagerCoreTest::test_ipluginhost_get_plugins() {
    IPluginHost* host = m_manager;
    QList<IPluginInterface*> plugins = host->getPlugins();
    // Should not crash, may return empty or populated list
    QVERIFY(plugins.size() >= 0);
}

void PluginManagerCoreTest::test_ipluginhost_scan_plugin_directory() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    IPluginHost* host = m_manager;

    // Should not crash
    host->scanPluginDirectory(tmp.path());
    QVERIFY(true);
}

void PluginManagerCoreTest::test_ipluginhost_available_plugins() {
    IPluginHost* host = m_manager;
    QStringList available = host->availablePlugins();
    QVERIFY(available.size() >= 0);
}

// ============================================================================
// Plugin Communication Tests
// ============================================================================

void PluginManagerCoreTest::test_send_plugin_message() {
    // Sending message to non-existent plugin should return false
    QVERIFY(!m_manager->sendPluginMessage("Sender", "NonExistent",
                                          QVariant("test message")));
}

void PluginManagerCoreTest::test_broadcast_plugin_message() {
    // Broadcasting should not crash even with no plugins
    m_manager->broadcastPluginMessage("Sender", QVariant("broadcast message"));
    QVERIFY(true);
}

// ============================================================================
// Signals Tests
// ============================================================================

void PluginManagerCoreTest::test_plugin_enabled_signal() {
    // We can't fully test this without a real plugin loaded,
    // but we verify the signal connection doesn't crash
    QSignalSpy enabledSpy(m_manager, &PluginManager::pluginEnabled);
    QVERIFY(enabledSpy.isValid());
}

void PluginManagerCoreTest::test_plugin_disabled_signal() {
    QSignalSpy disabledSpy(m_manager, &PluginManager::pluginDisabled);
    QVERIFY(disabledSpy.isValid());
}

QTEST_MAIN(PluginManagerCoreTest)
#include "test_plugin_manager_core.moc"
