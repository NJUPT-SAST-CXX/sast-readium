#include <QFile>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "../../app/plugin/PluginManager.h"
#include "../TestUtilities.h"

class PluginManagerCoreTest : public TestBase {
    Q_OBJECT

private slots:
    void test_directories_and_scan_empty();
    void test_hot_reloading_toggle();
    void test_settings_persistence_no_plugins();
    void test_validation_and_reporting_and_backup_restore();
};

void PluginManagerCoreTest::test_directories_and_scan_empty() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    PluginManager* mgr = &PluginManager::instance();
    QSignalSpy scannedSpy(mgr, &PluginManager::pluginsScanned);

    mgr->setPluginDirectories(QStringList{tmp.path()});
    mgr->scanForPlugins();

    if (scannedSpy.count() == 0) {
        QVERIFY(scannedSpy.wait(500));
    } else {
        QTest::qWait(0);
    }
    QCOMPARE(scannedSpy.takeFirst().at(0).toInt(), 0);
}

void PluginManagerCoreTest::test_hot_reloading_toggle() {
    PluginManager* mgr = &PluginManager::instance();
    QVERIFY(!mgr->isHotReloadingEnabled());
    mgr->enableHotReloading(true);
    QVERIFY(mgr->isHotReloadingEnabled());
    mgr->enableHotReloading(false);
    QVERIFY(!mgr->isHotReloadingEnabled());
}

void PluginManagerCoreTest::test_settings_persistence_no_plugins() {
    PluginManager* mgr = &PluginManager::instance();
    // With no plugins, load/save should not crash and simply be no-ops
    mgr->saveSettings();
    mgr->loadSettings();
}

void PluginManagerCoreTest::test_validation_and_reporting_and_backup_restore() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    PluginManager* mgr = &PluginManager::instance();
    // validatePlugin should return false for nonexistent path
    QVERIFY(!PluginManager::validatePlugin(tmp.filePath("does_not_exist")));

    // getPluginInfo for unknown plugin -> empty object
    QJsonObject info = mgr->getPluginInfo("unknown");
    QVERIFY(info.isEmpty());

    // exportPluginList writes a file
    const QString listPath = tmp.filePath("plugins.json");
    mgr->exportPluginList(listPath);
    QVERIFY(QFile::exists(listPath));

    // backup/restore round-trip on empty state
    const QString backupPath = tmp.filePath("backup.json");
    QVERIFY(mgr->backupPluginConfiguration(backupPath));
    QVERIFY(QFile::exists(backupPath));

    // Backup should be valid JSON
    QFile f(backupPath);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const auto doc = QJsonDocument::fromJson(f.readAll());
    QVERIFY(!doc.isNull());

    QVERIFY(mgr->restorePluginConfiguration(backupPath));

    // createPluginReport should not crash
    mgr->createPluginReport();
}

QTEST_MAIN(PluginManagerCoreTest)
#include "test_plugin_manager_core.moc"
