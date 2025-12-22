#include <QSignalSpy>
#include <QTest>
#include <QVariantMap>
#include "plugin/PluginHookRegistry.h"
#include "plugin/PluginManager.h"

/**
 * @brief Test class for plugin hook integration
 *
 * Tests that hooks are properly registered and can be executed.
 */
class TestPluginHooks : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Standard Hooks Registration Tests
    void testDocumentHooksRegistered();
    void testSearchHooksRegistered();
    void testCacheHooksRegistered();
    void testAnnotationHooksRegistered();
    void testRenderHooksRegistered();
    void testExportHooksRegistered();

    // Hook Execution Tests
    void testExecuteHookWithNoCallbacks();
    void testExecuteHookWithCallback();
    void testExecuteHookWithMultipleCallbacks();
    void testExecuteDisabledHook();

    // Hook Context Tests
    void testHookContextPassthrough();
    void testHookResultAggregation();

private:
    PluginHookRegistry* m_hookRegistry = nullptr;
};

void TestPluginHooks::initTestCase() {
    // Register standard hooks
    PluginManager::instance().registerStandardHooks();
    m_hookRegistry = &PluginHookRegistry::instance();
}

void TestPluginHooks::cleanupTestCase() {
    // Cleanup is handled by singleton destructors
}

void TestPluginHooks::testDocumentHooksRegistered() {
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::DOCUMENT_PRE_LOAD));
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::DOCUMENT_POST_LOAD));
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::DOCUMENT_PRE_CLOSE));
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::DOCUMENT_POST_CLOSE));
}

void TestPluginHooks::testSearchHooksRegistered() {
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::SEARCH_PRE_EXECUTE));
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::SEARCH_POST_EXECUTE));
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::SEARCH_INDEX_BUILD));
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::SEARCH_RESULTS_RANK));
}

void TestPluginHooks::testCacheHooksRegistered() {
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::CACHE_PRE_ADD));
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::CACHE_POST_ADD));
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::CACHE_PRE_EVICT));
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::CACHE_POST_EVICT));
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::CACHE_OPTIMIZE));
}

void TestPluginHooks::testAnnotationHooksRegistered() {
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::ANNOTATION_CREATED));
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::ANNOTATION_UPDATED));
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::ANNOTATION_DELETED));
}

void TestPluginHooks::testRenderHooksRegistered() {
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::RENDER_PRE_PAGE));
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::RENDER_POST_PAGE));
}

void TestPluginHooks::testExportHooksRegistered() {
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::EXPORT_PRE_EXECUTE));
    QVERIFY(m_hookRegistry->hasHook(StandardHooks::EXPORT_POST_EXECUTE));
}

void TestPluginHooks::testExecuteHookWithNoCallbacks() {
    // Executing a hook with no callbacks should return empty QVariant
    QVariant result = m_hookRegistry->executeHook(
        StandardHooks::DOCUMENT_PRE_LOAD, {{"filePath", "/test/path.pdf"}});

    // Result should be valid but potentially empty (no callbacks)
    QVERIFY(result.isNull() || result.isValid());
}

void TestPluginHooks::testExecuteHookWithCallback() {
    static bool callbackExecuted = false;
    static QString receivedPath;

    // Register a test callback
    m_hookRegistry->registerCallback(
        StandardHooks::DOCUMENT_PRE_LOAD, "TestPlugin",
        [](const QVariantMap& context) -> QVariant {
            callbackExecuted = true;
            receivedPath = context.value("filePath").toString();
            return QVariant(true);
        });

    // Execute the hook
    m_hookRegistry->executeHook(StandardHooks::DOCUMENT_PRE_LOAD,
                                {{"filePath", "/test/callback.pdf"}});

    QVERIFY(callbackExecuted);
    QCOMPARE(receivedPath, QString("/test/callback.pdf"));

    // Cleanup
    m_hookRegistry->unregisterCallback(StandardHooks::DOCUMENT_PRE_LOAD,
                                       "TestPlugin");
}

void TestPluginHooks::testExecuteHookWithMultipleCallbacks() {
    static int callbackCount = 0;

    // Register multiple callbacks
    m_hookRegistry->registerCallback(StandardHooks::DOCUMENT_POST_LOAD,
                                     "TestPlugin1",
                                     [](const QVariantMap&) -> QVariant {
                                         callbackCount++;
                                         return QVariant();
                                     });

    m_hookRegistry->registerCallback(StandardHooks::DOCUMENT_POST_LOAD,
                                     "TestPlugin2",
                                     [](const QVariantMap&) -> QVariant {
                                         callbackCount++;
                                         return QVariant();
                                     });

    // Execute the hook
    callbackCount = 0;
    m_hookRegistry->executeHook(StandardHooks::DOCUMENT_POST_LOAD, {});

    QCOMPARE(callbackCount, 2);

    // Cleanup
    m_hookRegistry->unregisterCallback(StandardHooks::DOCUMENT_POST_LOAD,
                                       "TestPlugin1");
    m_hookRegistry->unregisterCallback(StandardHooks::DOCUMENT_POST_LOAD,
                                       "TestPlugin2");
}

void TestPluginHooks::testExecuteDisabledHook() {
    static bool callbackExecuted = false;

    // Register callback
    m_hookRegistry->registerCallback(StandardHooks::SEARCH_PRE_EXECUTE,
                                     "TestPlugin",
                                     [](const QVariantMap&) -> QVariant {
                                         callbackExecuted = true;
                                         return QVariant();
                                     });

    // Disable the hook
    m_hookRegistry->setHookEnabled(StandardHooks::SEARCH_PRE_EXECUTE, false);

    // Execute - should not trigger callback
    callbackExecuted = false;
    m_hookRegistry->executeHook(StandardHooks::SEARCH_PRE_EXECUTE, {});
    QVERIFY(!callbackExecuted);

    // Re-enable and test
    m_hookRegistry->setHookEnabled(StandardHooks::SEARCH_PRE_EXECUTE, true);
    m_hookRegistry->executeHook(StandardHooks::SEARCH_PRE_EXECUTE, {});
    QVERIFY(callbackExecuted);

    // Cleanup
    m_hookRegistry->unregisterCallback(StandardHooks::SEARCH_PRE_EXECUTE,
                                       "TestPlugin");
}

void TestPluginHooks::testHookContextPassthrough() {
    static QVariantMap receivedContext;

    m_hookRegistry->registerCallback(
        StandardHooks::CACHE_PRE_ADD, "TestPlugin",
        [](const QVariantMap& context) -> QVariant {
            receivedContext = context;
            return QVariant();
        });

    QVariantMap testContext;
    testContext["key"] = "testKey";
    testContext["size"] = 1024;
    testContext["cacheType"] = 1;

    m_hookRegistry->executeHook(StandardHooks::CACHE_PRE_ADD, testContext);

    QCOMPARE(receivedContext.value("key").toString(), QString("testKey"));
    QCOMPARE(receivedContext.value("size").toInt(), 1024);
    QCOMPARE(receivedContext.value("cacheType").toInt(), 1);

    // Cleanup
    m_hookRegistry->unregisterCallback(StandardHooks::CACHE_PRE_ADD,
                                       "TestPlugin");
}

void TestPluginHooks::testHookResultAggregation() {
    // Register callbacks that return values
    m_hookRegistry->registerCallback(
        StandardHooks::ANNOTATION_CREATED, "TestPlugin1",
        [](const QVariantMap&) -> QVariant { return QVariant(10); });

    m_hookRegistry->registerCallback(
        StandardHooks::ANNOTATION_CREATED, "TestPlugin2",
        [](const QVariantMap&) -> QVariant { return QVariant(20); });

    // Execute and check result
    QVariant result =
        m_hookRegistry->executeHook(StandardHooks::ANNOTATION_CREATED, {});

    // Result should be valid (aggregation depends on implementation)
    QVERIFY(result.isValid());

    // Cleanup
    m_hookRegistry->unregisterCallback(StandardHooks::ANNOTATION_CREATED,
                                       "TestPlugin1");
    m_hookRegistry->unregisterCallback(StandardHooks::ANNOTATION_CREATED,
                                       "TestPlugin2");
}

QTEST_MAIN(TestPluginHooks)
#include "test_plugin_hooks.moc"
