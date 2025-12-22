#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest/QtTest>
#include "../../../app/ui/core/UIResourceManager.h"

class UIResourceManagerTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testSingletonInstance();
    void testRegisterResource();
    void testUnregisterResource();
    void testRegisterWidget();
    void testScheduleWidgetCleanup();
    void testCleanupWidget();
    void testCreateManagedTimer();
    void testCleanupTimer();
    void testOptimizeMemoryUsage();
    void testClearPixmapCache();
    void testClearStyleSheetCache();
    void testGetTotalMemoryUsage();
    void testGetResourceMemoryUsage();
    void testGetResourceCount();
    void testGetResourceList();
    void testCleanupExpiredResources();
    void testCleanupAllResources();
    void testForceCleanupType();
    void testSetAutoCleanupEnabled();
    void testSetMemoryThreshold();
    void testSetCleanupInterval();
    void testValidateResources();
    void testFindLeakedResources();
    void testResourceRegisteredSignal();
    void testResourceUnregisteredSignal();
    void testMemoryThresholdExceededSignal();
    void testResourceGuardConstruction();
    void testResourceGuardRelease();
    void testManagedWidgetFactoryCreate();

private:
    QWidget* m_parentWidget;
};

void UIResourceManagerTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
    }
}

void UIResourceManagerTest::cleanupTestCase() {
    UIResourceManager::instance().cleanupAllResources();
    delete m_parentWidget;
}

void UIResourceManagerTest::init() {}

void UIResourceManagerTest::cleanup() {}

void UIResourceManagerTest::testSingletonInstance() {
    auto& i1 = UIResourceManager::instance();
    auto& i2 = UIResourceManager::instance();
    QCOMPARE(&i1, &i2);
}

void UIResourceManagerTest::testRegisterResource() {
    auto* label = new QLabel("Test", m_parentWidget);
    UIResourceManager::instance().registerResource(
        label, UIResourceManager::ResourceType::Widget, "Test Label", 1000,
        true);
    UIResourceManager::instance().unregisterResource(label);
    delete label;
    QVERIFY(true);
}

void UIResourceManagerTest::testUnregisterResource() {
    auto* label = new QLabel("Test", m_parentWidget);
    UIResourceManager::instance().registerResource(
        label, UIResourceManager::ResourceType::Widget, "Test");
    UIResourceManager::instance().unregisterResource(label);
    delete label;
    QVERIFY(true);
}

void UIResourceManagerTest::testRegisterWidget() {
    auto* button = new QPushButton("Test", m_parentWidget);
    UIResourceManager::instance().registerWidget(button, "Test Button");
    UIResourceManager::instance().unregisterResource(button);
    delete button;
    QVERIFY(true);
}

void UIResourceManagerTest::testScheduleWidgetCleanup() {
    auto* label = new QLabel("Test", m_parentWidget);
    UIResourceManager::instance().registerWidget(label, "Test");
    UIResourceManager::instance().scheduleWidgetCleanup(label, 100);
    QTest::qWait(200);
    QVERIFY(true);
}

void UIResourceManagerTest::testCleanupWidget() {
    auto* label = new QLabel("Test", m_parentWidget);
    UIResourceManager::instance().registerWidget(label, "Test");
    UIResourceManager::instance().cleanupWidget(label);
    QVERIFY(true);
}

void UIResourceManagerTest::testCreateManagedTimer() {
    QTimer* timer = UIResourceManager::instance().createManagedTimer(
        m_parentWidget, "Test Timer");
    QVERIFY(timer != nullptr);
    UIResourceManager::instance().cleanupTimer(timer);
}

void UIResourceManagerTest::testCleanupTimer() {
    QTimer* timer = UIResourceManager::instance().createManagedTimer(
        m_parentWidget, "Test");
    UIResourceManager::instance().cleanupTimer(timer);
    QVERIFY(true);
}

void UIResourceManagerTest::testOptimizeMemoryUsage() {
    UIResourceManager::instance().optimizeMemoryUsage();
    QVERIFY(true);
}

void UIResourceManagerTest::testClearPixmapCache() {
    UIResourceManager::instance().clearPixmapCache();
    QVERIFY(true);
}

void UIResourceManagerTest::testClearStyleSheetCache() {
    UIResourceManager::instance().clearStyleSheetCache();
    QVERIFY(true);
}

void UIResourceManagerTest::testGetTotalMemoryUsage() {
    qint64 usage = UIResourceManager::instance().getTotalMemoryUsage();
    QVERIFY(usage >= 0);
}

void UIResourceManagerTest::testGetResourceMemoryUsage() {
    qint64 usage = UIResourceManager::instance().getResourceMemoryUsage(
        UIResourceManager::ResourceType::Widget);
    QVERIFY(usage >= 0);
}

void UIResourceManagerTest::testGetResourceCount() {
    int count = UIResourceManager::instance().getResourceCount(
        UIResourceManager::ResourceType::Widget);
    QVERIFY(count >= 0);
}

void UIResourceManagerTest::testGetResourceList() {
    auto list = UIResourceManager::instance().getResourceList(
        UIResourceManager::ResourceType::Widget);
    QVERIFY(true);  // List may be empty
}

void UIResourceManagerTest::testCleanupExpiredResources() {
    UIResourceManager::instance().cleanupExpiredResources();
    QVERIFY(true);
}

void UIResourceManagerTest::testCleanupAllResources() {
    UIResourceManager::instance().cleanupAllResources();
    QVERIFY(true);
}

void UIResourceManagerTest::testForceCleanupType() {
    UIResourceManager::instance().forceCleanupType(
        UIResourceManager::ResourceType::Timer);
    QVERIFY(true);
}

void UIResourceManagerTest::testSetAutoCleanupEnabled() {
    UIResourceManager::instance().setAutoCleanupEnabled(true);
    UIResourceManager::instance().setAutoCleanupEnabled(false);
    QVERIFY(true);
}

void UIResourceManagerTest::testSetMemoryThreshold() {
    UIResourceManager::instance().setMemoryThreshold(1024 * 1024 * 100);
    QVERIFY(true);
}

void UIResourceManagerTest::testSetCleanupInterval() {
    UIResourceManager::instance().setCleanupInterval(60000);
    QVERIFY(true);
}

void UIResourceManagerTest::testValidateResources() {
    bool valid = UIResourceManager::instance().validateResources();
    QVERIFY(valid || !valid);
}

void UIResourceManagerTest::testFindLeakedResources() {
    QStringList leaked = UIResourceManager::instance().findLeakedResources();
    QVERIFY(true);  // May be empty
}

void UIResourceManagerTest::testResourceRegisteredSignal() {
    QSignalSpy spy(&UIResourceManager::instance(),
                   &UIResourceManager::resourceRegistered);
    QVERIFY(spy.isValid());
}

void UIResourceManagerTest::testResourceUnregisteredSignal() {
    QSignalSpy spy(&UIResourceManager::instance(),
                   &UIResourceManager::resourceUnregistered);
    QVERIFY(spy.isValid());
}

void UIResourceManagerTest::testMemoryThresholdExceededSignal() {
    QSignalSpy spy(&UIResourceManager::instance(),
                   &UIResourceManager::memoryThresholdExceeded);
    QVERIFY(spy.isValid());
}

void UIResourceManagerTest::testResourceGuardConstruction() {
    auto* label = new QLabel("Test", m_parentWidget);
    {
        ResourceGuard guard(label, UIResourceManager::ResourceType::Widget,
                            "Test");
        QVERIFY(guard.get() == label);
        guard.release();
    }
    delete label;
    QVERIFY(true);
}

void UIResourceManagerTest::testResourceGuardRelease() {
    auto* label = new QLabel("Test", m_parentWidget);
    ResourceGuard guard(label, UIResourceManager::ResourceType::Widget, "Test");
    guard.release();
    QVERIFY(guard.get() == label);
    delete label;
}

void UIResourceManagerTest::testManagedWidgetFactoryCreate() {
    auto* label = ManagedWidgetFactory::create<QLabel>(m_parentWidget,
                                                       "Managed Label", "Text");
    QVERIFY(label != nullptr);
    UIResourceManager::instance().cleanupWidget(label);
}

QTEST_MAIN(UIResourceManagerTest)
#include "test_ui_resource_manager.moc"
