#include <QSignalSpy>
#include <QSystemTrayIcon>
#include <QTest>
#include "../../app/managers/SystemTrayManager.h"
#include "../TestUtilities.h"

class TestSystemTrayManager : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void testSingleton() {
        SystemTrayManager& instance1 = SystemTrayManager::instance();
        SystemTrayManager& instance2 = SystemTrayManager::instance();
        QVERIFY(&instance1 == &instance2);
    }

    void testIsSystemTrayAvailable() {
        bool available = SystemTrayManager::isSystemTrayAvailable();
    }

    void testIsEnabled() {
        SystemTrayManager& manager = SystemTrayManager::instance();
        bool enabled = manager.isEnabled();
    }

    void testSetEnabled() {
        SystemTrayManager& manager = SystemTrayManager::instance();
        manager.setEnabled(true);
        QVERIFY(manager.isEnabled());

        manager.setEnabled(false);
        QVERIFY(!manager.isEnabled());
    }

    void testMinimizeToTrayEnabled() {
        SystemTrayManager& manager = SystemTrayManager::instance();

        manager.setMinimizeToTrayEnabled(true);
        QVERIFY(manager.isMinimizeToTrayEnabled());

        manager.setMinimizeToTrayEnabled(false);
        QVERIFY(!manager.isMinimizeToTrayEnabled());
    }

    void testShowNotification() {
        SystemTrayManager& manager = SystemTrayManager::instance();

        if (SystemTrayManager::isSystemTrayAvailable() && manager.isEnabled()) {
            manager.showNotification("Test", "Test message", "info", 1000);
        }
    }

    void testSetApplicationStatus() {
        SystemTrayManager& manager = SystemTrayManager::instance();
        manager.setApplicationStatus("idle", "Ready");
        manager.setApplicationStatus("processing", "Working...");
    }

    void testCheckSystemTrayAvailability() {
        SystemTrayManager& manager = SystemTrayManager::instance();
        manager.checkSystemTrayAvailability();
    }
};

QTEST_MAIN(TestSystemTrayManager)
#include "test_system_tray_manager.moc"
