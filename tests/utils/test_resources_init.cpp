#include <QTest>
#include "../../app/utils/ResourcesInit.h"
#include "../TestUtilities.h"

class TestResourcesInit : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void testInitializeResources() {
        bool result = ResourcesInit::initialize();
        QVERIFY(result);
    }

    void testIsInitialized() {
        ResourcesInit::initialize();
        QVERIFY(ResourcesInit::isInitialized());
    }

    void testMultipleInitializations() {
        bool result1 = ResourcesInit::initialize();
        bool result2 = ResourcesInit::initialize();

        QVERIFY(result1);
        QVERIFY(result2);
        QVERIFY(ResourcesInit::isInitialized());
    }

    void testGetResourcePath() {
        ResourcesInit::initialize();

        QString path = ResourcesInit::getResourcePath("icons");
        QVERIFY(!path.isEmpty());
    }

    void testGetStylePath() {
        ResourcesInit::initialize();

        QString path = ResourcesInit::getStylePath();
        QVERIFY(!path.isEmpty());
    }

    void testGetTranslationPath() {
        ResourcesInit::initialize();

        QString path = ResourcesInit::getTranslationPath();
        QVERIFY(!path.isEmpty());
    }

    void testGetFontPath() {
        ResourcesInit::initialize();

        QString path = ResourcesInit::getFontPath();
        QVERIFY(!path.isEmpty());
    }

    void testGetConfigPath() {
        ResourcesInit::initialize();

        QString path = ResourcesInit::getConfigPath();
        QVERIFY(!path.isEmpty());
    }

    void testGetDataPath() {
        ResourcesInit::initialize();

        QString path = ResourcesInit::getDataPath();
        QVERIFY(!path.isEmpty());
    }

    void testGetCachePath() {
        ResourcesInit::initialize();

        QString path = ResourcesInit::getCachePath();
        QVERIFY(!path.isEmpty());
    }

    void testGetLogPath() {
        ResourcesInit::initialize();

        QString path = ResourcesInit::getLogPath();
        QVERIFY(!path.isEmpty());
    }

    void testGetTempPath() {
        ResourcesInit::initialize();

        QString path = ResourcesInit::getTempPath();
        QVERIFY(!path.isEmpty());
    }

    void testGetPluginPath() {
        ResourcesInit::initialize();

        QString path = ResourcesInit::getPluginPath();
        QVERIFY(!path.isEmpty());
    }

    void testEnsureDirectoriesExist() {
        ResourcesInit::initialize();
        bool result = ResourcesInit::ensureDirectoriesExist();
        QVERIFY(result);
    }

    void testCleanup() {
        ResourcesInit::initialize();
        ResourcesInit::cleanup();
    }

    void testGetVersion() {
        QString version = ResourcesInit::getVersion();
        QVERIFY(!version.isEmpty());
    }

    void testGetApplicationName() {
        QString name = ResourcesInit::getApplicationName();
        QVERIFY(!name.isEmpty());
    }

    void testGetOrganizationName() {
        QString name = ResourcesInit::getOrganizationName();
        QVERIFY(!name.isEmpty());
    }
};

QTEST_MAIN(TestResourcesInit)
#include "test_resources_init.moc"
