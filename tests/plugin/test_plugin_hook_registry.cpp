#include <QSignalSpy>
#include <QTest>
#include <QVariantMap>

#include "../../app/plugin/PluginHookPoint.h"
#include "../../app/plugin/PluginHookRegistry.h"
#include "../TestUtilities.h"

/**
 * @brief Test fixture for PluginHookRegistry and PluginHookPoint
 */
class PluginHookRegistryTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

    // PluginHookPoint tests
    void test_hook_point_construction();
    void test_hook_point_register_callback();
    void test_hook_point_unregister_callback();
    void test_hook_point_execute_single_callback();
    void test_hook_point_execute_multiple_callbacks();
    void test_hook_point_callback_count();

    // PluginHookRegistry - Hook Registration tests
    void test_registry_register_hook();
    void test_registry_register_hook_with_description();
    void test_registry_register_hook_empty_name();
    void test_registry_register_hook_duplicate();
    void test_registry_unregister_hook();
    void test_registry_unregister_nonexistent_hook();
    void test_registry_has_hook();
    void test_registry_get_hook_names();
    void test_registry_get_hook_description();

    // PluginHookRegistry - Callback Management tests
    void test_registry_register_callback();
    void test_registry_register_callback_nonexistent_hook();
    void test_registry_register_callback_empty_plugin_name();
    void test_registry_register_callback_null_callback();
    void test_registry_unregister_callback();
    void test_registry_unregister_all_callbacks();
    void test_registry_get_callback_count();

    // PluginHookRegistry - Hook Execution tests
    void test_registry_execute_hook();
    void test_registry_execute_hook_with_context();
    void test_registry_execute_nonexistent_hook();
    void test_registry_execute_disabled_hook();
    void test_registry_execute_hook_no_callbacks();

    // PluginHookRegistry - Enable/Disable tests
    void test_registry_set_hook_enabled();
    void test_registry_is_hook_enabled();
    void test_registry_enable_nonexistent_hook();

    // PluginHookRegistry - Signal tests
    void test_registry_hook_registered_signal();
    void test_registry_hook_unregistered_signal();
    void test_registry_callback_registered_signal();
    void test_registry_callback_unregistered_signal();
    void test_registry_hook_executed_signal();

    // StandardHooks namespace tests
    void test_standard_hooks_constants();

private:
    PluginHookRegistry* m_registry = nullptr;
};

void PluginHookRegistryTest::initTestCase() { TestBase::initTestCase(); }

void PluginHookRegistryTest::cleanupTestCase() { TestBase::cleanupTestCase(); }

void PluginHookRegistryTest::init() {
    m_registry = &PluginHookRegistry::instance();
    // Clean up any existing hooks from previous tests
    for (const QString& hookName : m_registry->getHookNames()) {
        m_registry->unregisterHook(hookName);
    }
}

void PluginHookRegistryTest::cleanup() {
    // Clean up hooks after each test
    if (m_registry) {
        for (const QString& hookName : m_registry->getHookNames()) {
            m_registry->unregisterHook(hookName);
        }
    }
}

// ============================================================================
// PluginHookPoint Tests
// ============================================================================

void PluginHookRegistryTest::test_hook_point_construction() {
    PluginHookPoint hookPoint("test.hook");
    QCOMPARE(hookPoint.name(), QString("test.hook"));
    QCOMPARE(hookPoint.callbackCount(), 0);
}

void PluginHookRegistryTest::test_hook_point_register_callback() {
    PluginHookPoint hookPoint("test.hook");

    auto callback = [](const QVariantMap&) -> QVariant {
        return QVariant("result");
    };

    hookPoint.registerCallback("TestPlugin", callback);
    QCOMPARE(hookPoint.callbackCount(), 1);
}

void PluginHookRegistryTest::test_hook_point_unregister_callback() {
    PluginHookPoint hookPoint("test.hook");

    auto callback = [](const QVariantMap&) -> QVariant {
        return QVariant("result");
    };

    hookPoint.registerCallback("TestPlugin", callback);
    QCOMPARE(hookPoint.callbackCount(), 1);

    hookPoint.unregisterCallback("TestPlugin");
    QCOMPARE(hookPoint.callbackCount(), 0);
}

void PluginHookRegistryTest::test_hook_point_execute_single_callback() {
    PluginHookPoint hookPoint("test.hook");

    bool callbackExecuted = false;
    auto callback =
        [&callbackExecuted](const QVariantMap& context) -> QVariant {
        callbackExecuted = true;
        return context.value("input").toString() + "_processed";
    };

    hookPoint.registerCallback("TestPlugin", callback);

    QVariantMap context;
    context["input"] = "test";

    QVariant result = hookPoint.execute(context);
    QVERIFY(callbackExecuted);
    QVERIFY(result.isValid());

    QVariantList results = result.toList();
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.first().toString(), QString("test_processed"));
}

void PluginHookRegistryTest::test_hook_point_execute_multiple_callbacks() {
    PluginHookPoint hookPoint("test.hook");

    auto callback1 = [](const QVariantMap&) -> QVariant {
        return QVariant("result1");
    };
    auto callback2 = [](const QVariantMap&) -> QVariant {
        return QVariant("result2");
    };

    hookPoint.registerCallback("Plugin1", callback1);
    hookPoint.registerCallback("Plugin2", callback2);

    QVariant result = hookPoint.execute(QVariantMap());
    QVariantList results = result.toList();

    QCOMPARE(results.size(), 2);
    QVERIFY(results.contains(QVariant("result1")));
    QVERIFY(results.contains(QVariant("result2")));
}

void PluginHookRegistryTest::test_hook_point_callback_count() {
    PluginHookPoint hookPoint("test.hook");

    QCOMPARE(hookPoint.callbackCount(), 0);

    auto callback = [](const QVariantMap&) -> QVariant { return QVariant(); };

    hookPoint.registerCallback("Plugin1", callback);
    QCOMPARE(hookPoint.callbackCount(), 1);

    hookPoint.registerCallback("Plugin2", callback);
    QCOMPARE(hookPoint.callbackCount(), 2);

    hookPoint.unregisterCallback("Plugin1");
    QCOMPARE(hookPoint.callbackCount(), 1);
}

// ============================================================================
// PluginHookRegistry - Hook Registration Tests
// ============================================================================

void PluginHookRegistryTest::test_registry_register_hook() {
    QVERIFY(m_registry->registerHook("test.hook"));
    QVERIFY(m_registry->hasHook("test.hook"));
}

void PluginHookRegistryTest::test_registry_register_hook_with_description() {
    QVERIFY(m_registry->registerHook("test.hook", "Test hook description"));
    QCOMPARE(m_registry->getHookDescription("test.hook"),
             QString("Test hook description"));
}

void PluginHookRegistryTest::test_registry_register_hook_empty_name() {
    QVERIFY(!m_registry->registerHook(""));
}

void PluginHookRegistryTest::test_registry_register_hook_duplicate() {
    QVERIFY(m_registry->registerHook("test.hook"));
    QVERIFY(!m_registry->registerHook("test.hook"));
}

void PluginHookRegistryTest::test_registry_unregister_hook() {
    m_registry->registerHook("test.hook");
    QVERIFY(m_registry->hasHook("test.hook"));

    m_registry->unregisterHook("test.hook");
    QVERIFY(!m_registry->hasHook("test.hook"));
}

void PluginHookRegistryTest::test_registry_unregister_nonexistent_hook() {
    // Should not crash when unregistering non-existent hook
    m_registry->unregisterHook("nonexistent.hook");
    QVERIFY(!m_registry->hasHook("nonexistent.hook"));
}

void PluginHookRegistryTest::test_registry_has_hook() {
    QVERIFY(!m_registry->hasHook("test.hook"));
    m_registry->registerHook("test.hook");
    QVERIFY(m_registry->hasHook("test.hook"));
}

void PluginHookRegistryTest::test_registry_get_hook_names() {
    m_registry->registerHook("hook1");
    m_registry->registerHook("hook2");
    m_registry->registerHook("hook3");

    QStringList names = m_registry->getHookNames();
    QCOMPARE(names.size(), 3);
    QVERIFY(names.contains("hook1"));
    QVERIFY(names.contains("hook2"));
    QVERIFY(names.contains("hook3"));
}

void PluginHookRegistryTest::test_registry_get_hook_description() {
    m_registry->registerHook("test.hook", "My description");
    QCOMPARE(m_registry->getHookDescription("test.hook"),
             QString("My description"));

    // Non-existent hook returns empty string
    QVERIFY(m_registry->getHookDescription("nonexistent").isEmpty());

    // Hook without description gets default description
    m_registry->registerHook("no_desc_hook");
    QVERIFY(!m_registry->getHookDescription("no_desc_hook").isEmpty());
}

// ============================================================================
// PluginHookRegistry - Callback Management Tests
// ============================================================================

void PluginHookRegistryTest::test_registry_register_callback() {
    m_registry->registerHook("test.hook");

    auto callback = [](const QVariantMap&) -> QVariant {
        return QVariant("result");
    };

    QVERIFY(m_registry->registerCallback("test.hook", "TestPlugin", callback));
    QCOMPARE(m_registry->getCallbackCount("test.hook"), 1);
}

void PluginHookRegistryTest::
    test_registry_register_callback_nonexistent_hook() {
    auto callback = [](const QVariantMap&) -> QVariant {
        return QVariant("result");
    };

    QVERIFY(!m_registry->registerCallback("nonexistent.hook", "TestPlugin",
                                          callback));
}

void PluginHookRegistryTest::
    test_registry_register_callback_empty_plugin_name() {
    m_registry->registerHook("test.hook");

    auto callback = [](const QVariantMap&) -> QVariant {
        return QVariant("result");
    };

    QVERIFY(!m_registry->registerCallback("test.hook", "", callback));
}

void PluginHookRegistryTest::test_registry_register_callback_null_callback() {
    m_registry->registerHook("test.hook");
    QVERIFY(!m_registry->registerCallback("test.hook", "TestPlugin", nullptr));
}

void PluginHookRegistryTest::test_registry_unregister_callback() {
    m_registry->registerHook("test.hook");

    auto callback = [](const QVariantMap&) -> QVariant {
        return QVariant("result");
    };

    m_registry->registerCallback("test.hook", "TestPlugin", callback);
    QCOMPARE(m_registry->getCallbackCount("test.hook"), 1);

    m_registry->unregisterCallback("test.hook", "TestPlugin");
    QCOMPARE(m_registry->getCallbackCount("test.hook"), 0);
}

void PluginHookRegistryTest::test_registry_unregister_all_callbacks() {
    m_registry->registerHook("hook1");
    m_registry->registerHook("hook2");

    auto callback = [](const QVariantMap&) -> QVariant {
        return QVariant("result");
    };

    m_registry->registerCallback("hook1", "TestPlugin", callback);
    m_registry->registerCallback("hook2", "TestPlugin", callback);

    QCOMPARE(m_registry->getCallbackCount("hook1"), 1);
    QCOMPARE(m_registry->getCallbackCount("hook2"), 1);

    m_registry->unregisterAllCallbacks("TestPlugin");

    QCOMPARE(m_registry->getCallbackCount("hook1"), 0);
    QCOMPARE(m_registry->getCallbackCount("hook2"), 0);
}

void PluginHookRegistryTest::test_registry_get_callback_count() {
    m_registry->registerHook("test.hook");
    QCOMPARE(m_registry->getCallbackCount("test.hook"), 0);

    auto callback = [](const QVariantMap&) -> QVariant {
        return QVariant("result");
    };

    m_registry->registerCallback("test.hook", "Plugin1", callback);
    QCOMPARE(m_registry->getCallbackCount("test.hook"), 1);

    m_registry->registerCallback("test.hook", "Plugin2", callback);
    QCOMPARE(m_registry->getCallbackCount("test.hook"), 2);

    // Non-existent hook returns 0
    QCOMPARE(m_registry->getCallbackCount("nonexistent"), 0);
}

// ============================================================================
// PluginHookRegistry - Hook Execution Tests
// ============================================================================

void PluginHookRegistryTest::test_registry_execute_hook() {
    m_registry->registerHook("test.hook");

    bool executed = false;
    auto callback = [&executed](const QVariantMap&) -> QVariant {
        executed = true;
        return QVariant("success");
    };

    m_registry->registerCallback("test.hook", "TestPlugin", callback);

    QVariant result = m_registry->executeHook("test.hook");
    QVERIFY(executed);
    QVERIFY(result.isValid());
}

void PluginHookRegistryTest::test_registry_execute_hook_with_context() {
    m_registry->registerHook("test.hook");

    auto callback = [](const QVariantMap& context) -> QVariant {
        int value = context.value("value").toInt();
        return QVariant(value * 2);
    };

    m_registry->registerCallback("test.hook", "TestPlugin", callback);

    QVariantMap context;
    context["value"] = 21;

    QVariant result = m_registry->executeHook("test.hook", context);
    QVariantList results = result.toList();
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.first().toInt(), 42);
}

void PluginHookRegistryTest::test_registry_execute_nonexistent_hook() {
    QVariant result = m_registry->executeHook("nonexistent.hook");
    QVERIFY(!result.isValid());
}

void PluginHookRegistryTest::test_registry_execute_disabled_hook() {
    m_registry->registerHook("test.hook");

    bool executed = false;
    auto callback = [&executed](const QVariantMap&) -> QVariant {
        executed = true;
        return QVariant("result");
    };

    m_registry->registerCallback("test.hook", "TestPlugin", callback);
    m_registry->setHookEnabled("test.hook", false);

    QVariant result = m_registry->executeHook("test.hook");
    QVERIFY(!executed);
    QVERIFY(!result.isValid());
}

void PluginHookRegistryTest::test_registry_execute_hook_no_callbacks() {
    m_registry->registerHook("test.hook");
    QVariant result = m_registry->executeHook("test.hook");
    // Empty result when no callbacks registered
    QVERIFY(!result.isValid() || result.toList().isEmpty());
}

// ============================================================================
// PluginHookRegistry - Enable/Disable Tests
// ============================================================================

void PluginHookRegistryTest::test_registry_set_hook_enabled() {
    m_registry->registerHook("test.hook");

    QVERIFY(m_registry->isHookEnabled("test.hook"));

    m_registry->setHookEnabled("test.hook", false);
    QVERIFY(!m_registry->isHookEnabled("test.hook"));

    m_registry->setHookEnabled("test.hook", true);
    QVERIFY(m_registry->isHookEnabled("test.hook"));
}

void PluginHookRegistryTest::test_registry_is_hook_enabled() {
    // Non-existent hook returns false
    QVERIFY(!m_registry->isHookEnabled("nonexistent.hook"));

    m_registry->registerHook("test.hook");
    // New hooks are enabled by default
    QVERIFY(m_registry->isHookEnabled("test.hook"));
}

void PluginHookRegistryTest::test_registry_enable_nonexistent_hook() {
    // Should not crash
    m_registry->setHookEnabled("nonexistent.hook", true);
    QVERIFY(!m_registry->isHookEnabled("nonexistent.hook"));
}

// ============================================================================
// PluginHookRegistry - Signal Tests
// ============================================================================

void PluginHookRegistryTest::test_registry_hook_registered_signal() {
    QSignalSpy spy(m_registry, &PluginHookRegistry::hookRegistered);

    m_registry->registerHook("test.hook");

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString("test.hook"));
}

void PluginHookRegistryTest::test_registry_hook_unregistered_signal() {
    m_registry->registerHook("test.hook");

    QSignalSpy spy(m_registry, &PluginHookRegistry::hookUnregistered);

    m_registry->unregisterHook("test.hook");

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString("test.hook"));
}

void PluginHookRegistryTest::test_registry_callback_registered_signal() {
    m_registry->registerHook("test.hook");

    QSignalSpy spy(m_registry, &PluginHookRegistry::callbackRegistered);

    auto callback = [](const QVariantMap&) -> QVariant { return QVariant(); };
    m_registry->registerCallback("test.hook", "TestPlugin", callback);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toString(), QString("test.hook"));
    QCOMPARE(args.at(1).toString(), QString("TestPlugin"));
}

void PluginHookRegistryTest::test_registry_callback_unregistered_signal() {
    m_registry->registerHook("test.hook");

    auto callback = [](const QVariantMap&) -> QVariant { return QVariant(); };
    m_registry->registerCallback("test.hook", "TestPlugin", callback);

    QSignalSpy spy(m_registry, &PluginHookRegistry::callbackUnregistered);

    m_registry->unregisterCallback("test.hook", "TestPlugin");

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toString(), QString("test.hook"));
    QCOMPARE(args.at(1).toString(), QString("TestPlugin"));
}

void PluginHookRegistryTest::test_registry_hook_executed_signal() {
    m_registry->registerHook("test.hook");

    auto callback = [](const QVariantMap&) -> QVariant {
        return QVariant("result");
    };
    m_registry->registerCallback("test.hook", "TestPlugin", callback);

    QSignalSpy spy(m_registry, &PluginHookRegistry::hookExecuted);

    m_registry->executeHook("test.hook");

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toString(), QString("test.hook"));
    QCOMPARE(args.at(1).toInt(), 1);  // callback count
}

// ============================================================================
// StandardHooks Namespace Tests
// ============================================================================

void PluginHookRegistryTest::test_standard_hooks_constants() {
    // Verify all standard hook constants are defined and non-empty
    QVERIFY(QString(StandardHooks::DOCUMENT_PRE_LOAD).length() > 0);
    QVERIFY(QString(StandardHooks::DOCUMENT_POST_LOAD).length() > 0);
    QVERIFY(QString(StandardHooks::DOCUMENT_PRE_CLOSE).length() > 0);
    QVERIFY(QString(StandardHooks::DOCUMENT_POST_CLOSE).length() > 0);
    QVERIFY(QString(StandardHooks::DOCUMENT_METADATA_EXTRACTED).length() > 0);

    QVERIFY(QString(StandardHooks::RENDER_PRE_PAGE).length() > 0);
    QVERIFY(QString(StandardHooks::RENDER_POST_PAGE).length() > 0);
    QVERIFY(QString(StandardHooks::RENDER_APPLY_FILTER).length() > 0);
    QVERIFY(QString(StandardHooks::RENDER_OVERLAY).length() > 0);

    QVERIFY(QString(StandardHooks::SEARCH_PRE_EXECUTE).length() > 0);
    QVERIFY(QString(StandardHooks::SEARCH_POST_EXECUTE).length() > 0);
    QVERIFY(QString(StandardHooks::SEARCH_INDEX_BUILD).length() > 0);
    QVERIFY(QString(StandardHooks::SEARCH_RESULTS_RANK).length() > 0);

    QVERIFY(QString(StandardHooks::CACHE_PRE_ADD).length() > 0);
    QVERIFY(QString(StandardHooks::CACHE_POST_ADD).length() > 0);
    QVERIFY(QString(StandardHooks::CACHE_PRE_EVICT).length() > 0);
    QVERIFY(QString(StandardHooks::CACHE_POST_EVICT).length() > 0);
    QVERIFY(QString(StandardHooks::CACHE_OPTIMIZE).length() > 0);

    QVERIFY(QString(StandardHooks::ANNOTATION_CREATED).length() > 0);
    QVERIFY(QString(StandardHooks::ANNOTATION_UPDATED).length() > 0);
    QVERIFY(QString(StandardHooks::ANNOTATION_DELETED).length() > 0);
    QVERIFY(QString(StandardHooks::ANNOTATION_RENDER).length() > 0);

    QVERIFY(QString(StandardHooks::EXPORT_PRE_EXECUTE).length() > 0);
    QVERIFY(QString(StandardHooks::EXPORT_POST_EXECUTE).length() > 0);

    // Verify hook names follow naming convention
    QVERIFY(QString(StandardHooks::DOCUMENT_PRE_LOAD).contains("."));
    QVERIFY(QString(StandardHooks::RENDER_PRE_PAGE).contains("."));
    QVERIFY(QString(StandardHooks::SEARCH_PRE_EXECUTE).contains("."));
}

QTEST_MAIN(PluginHookRegistryTest)
#include "test_plugin_hook_registry.moc"
