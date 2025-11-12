#include <QPointer>
#include <QSignalSpy>
#include <QTest>

#include "../../app/plugin/PluginInterface.h"
#include "../../app/plugin/PluginManager.h"
#include "../TestUtilities.h"

class DummyPlugin : public PluginBase {
    Q_OBJECT
public:
    explicit DummyPlugin(QObject* parent = nullptr) : PluginBase(parent) {
        m_metadata.name = "Dummy";
        m_metadata.version = "1.0";
        m_metadata.author = "Test";
        m_metadata.description = "Dummy plugin for tests";
        m_capabilities.provides = {"feature.test"};
    }

protected:
    bool onInitialize() override { return true; }
    void onShutdown() override {}
};

class PluginBaseAndContextTest : public TestBase {
    Q_OBJECT

private slots:
    void test_pluginbase_lifecycle_and_signals();
    void test_plugincontext_paths_and_messaging();
    void test_dependency_resolver_toposort_and_cycles();
};

void PluginBaseAndContextTest::test_pluginbase_lifecycle_and_signals() {
    DummyPlugin plugin;
    QSignalSpy initSpy(&plugin, &PluginBase::initialized);
    QSignalSpy shutdownSpy(&plugin, &PluginBase::shutdownCompleted);

    QVERIFY(plugin.initialize());
    QVERIFY(initSpy.count() == 1);
    QVERIFY(plugin.isInitialized());

    plugin.shutdown();
    QVERIFY(shutdownSpy.count() == 1);
    QVERIFY(!plugin.isInitialized());
}

void PluginBaseAndContextTest::test_plugincontext_paths_and_messaging() {
    PluginContext ctx;
    const QString dataPath = ctx.pluginDataPath("Dummy");
    const QString cfgPath = ctx.pluginConfigPath("Dummy");
    QVERIFY(!dataPath.isEmpty());
    QVERIFY(!cfgPath.isEmpty());

    QSignalSpy msgSpy(&ctx, &PluginContext::messageReceived);
    QVERIFY(ctx.sendMessage("Target", 123));
    if (msgSpy.count() == 0) {
        QVERIFY(msgSpy.wait(200));
    }
    QCOMPARE(msgSpy.takeFirst().at(0).toString(), QString("Target"));

    ctx.broadcastMessage("hello");
    if (msgSpy.count() == 0) {
        QVERIFY(msgSpy.wait(200));
    }
    QCOMPARE(msgSpy.takeFirst().at(0).toString(), QString("*"));
}

void PluginBaseAndContextTest::test_dependency_resolver_toposort_and_cycles() {
    QHash<QString, PluginMetadata> plugins;
    PluginMetadata A;
    A.name = "A";
    plugins.insert("A", A);
    PluginMetadata B;
    B.name = "B";
    B.dependencies = {"A"};
    plugins.insert("B", B);
    PluginMetadata C;
    C.name = "C";
    C.dependencies = {"B"};
    plugins.insert("C", C);

    const QStringList order = PluginDependencyResolver::getLoadOrder(plugins);
    QVERIFY(order.contains("A") && order.contains("B") && order.contains("C"));
    QVERIFY(order.indexOf("A") < order.indexOf("B"));
    QVERIFY(order.indexOf("B") < order.indexOf("C"));
    QVERIFY(!PluginDependencyResolver::hasCyclicDependencies(plugins));

    // Introduce a cycle: A depends on C
    plugins["A"].dependencies = {"C"};
    QVERIFY(PluginDependencyResolver::hasCyclicDependencies(plugins));
}

QTEST_MAIN(PluginBaseAndContextTest)
#include "test_plugin_base_and_context.moc"
