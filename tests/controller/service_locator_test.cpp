#include <QSignalSpy>
#include <QTest>
#include "../../app/controller/ServiceLocator.h"
#include "../TestUtilities.h"

// Test service interfaces
class ITestService : public QObject {
    Q_OBJECT
public:
    virtual ~ITestService() = default;
    virtual QString getName() const = 0;
    virtual void doWork() = 0;
};

class TestServiceImpl : public ITestService {
    Q_OBJECT
public:
    QString getName() const override { return "TestService"; }
    void doWork() override { m_workDone = true; }
    bool wasWorkDone() const { return m_workDone; }

private:
    bool m_workDone = false;
};

class ServiceLocatorTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override {
        // Clear any existing services before tests
        ServiceLocator::instance().clearServices();
    }

    void cleanup() override {
        // Clear services after each test
        ServiceLocator::instance().clearServices();
    }

    void testSingletonInstance() {
        // Test that instance() always returns the same object
        ServiceLocator& instance1 = ServiceLocator::instance();
        ServiceLocator& instance2 = ServiceLocator::instance();
        QCOMPARE(&instance1, &instance2);
    }

    void testRegisterAndRetrieveService() {
        // Register a service instance
        auto* service = new TestServiceImpl();
        ServiceLocator::instance().registerService<ITestService>(service);

        // Retrieve the service
        auto* retrieved = ServiceLocator::instance().getService<ITestService>();
        QVERIFY(retrieved != nullptr);
        QCOMPARE(retrieved, service);
        QCOMPARE(retrieved->getName(), QString("TestService"));
    }

    void testRegisterServiceFactory() {
        // Register a service factory
        ServiceLocator::instance()
            .registerService<ITestService, TestServiceImpl>();

        // Service should be created on first access (lazy loading)
        auto* service1 = ServiceLocator::instance().getService<ITestService>();
        QVERIFY(service1 != nullptr);

        // Should return the same instance on subsequent calls
        auto* service2 = ServiceLocator::instance().getService<ITestService>();
        QCOMPARE(service1, service2);
    }

    void testRegisterSharedService() {
        // Register a shared_ptr service
        auto sharedService = std::make_shared<TestServiceImpl>();
        ServiceLocator::instance().registerService<ITestService>(sharedService);

        // Retrieve as regular pointer
        auto* service = ServiceLocator::instance().getService<ITestService>();
        QVERIFY(service != nullptr);
        QCOMPARE(service, sharedService.get());

        // Retrieve as shared_ptr
        auto retrieved =
            ServiceLocator::instance().getSharedService<ITestService>();
        QVERIFY(retrieved != nullptr);
        QCOMPARE(retrieved, sharedService);
    }

    void testHasService() {
        QVERIFY(!ServiceLocator::instance().hasService("ITestService"));

        ServiceLocator::instance()
            .registerService<ITestService, TestServiceImpl>();

        // Note: hasService uses type name string
        QString typeName = QString::fromStdString(typeid(ITestService).name());
        QVERIFY(ServiceLocator::instance().hasService(typeName));
    }

    void testRemoveService() {
        auto* service = new TestServiceImpl();
        ServiceLocator::instance().registerService<ITestService>(service);

        QString typeName = QString::fromStdString(typeid(ITestService).name());
        QVERIFY(ServiceLocator::instance().hasService(typeName));

        ServiceLocator::instance().removeService(typeName);
        QVERIFY(!ServiceLocator::instance().hasService(typeName));

        auto* retrieved = ServiceLocator::instance().getService<ITestService>();
        QVERIFY(retrieved == nullptr);
    }

    void testClearServices() {
        // Register multiple services
        ServiceLocator::instance().registerService<ITestService>(
            new TestServiceImpl());

        QStringList services = ServiceLocator::instance().registeredServices();
        QVERIFY(!services.isEmpty());

        ServiceLocator::instance().clearServices();

        services = ServiceLocator::instance().registeredServices();
        QVERIFY(services.isEmpty());
    }

    void testLazyLoading() {
        ServiceLocator& locator = ServiceLocator::instance();

        // Enable lazy loading (default)
        locator.setLazyLoading(true);
        QVERIFY(locator.isLazyLoading());

        // Register factory but don't create yet
        locator.registerService<ITestService, TestServiceImpl>();

        // Service should be created on first access
        auto* service = locator.getService<ITestService>();
        QVERIFY(service != nullptr);

        // Disable lazy loading
        locator.setLazyLoading(false);
        QVERIFY(!locator.isLazyLoading());
    }

    void testServiceRegisteredSignal() {
        ServiceLocator& locator = ServiceLocator::instance();
        QSignalSpy spy(&locator, &ServiceLocator::serviceRegistered);

        locator.registerService<ITestService>(new TestServiceImpl());

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QString typeName = QString::fromStdString(typeid(ITestService).name());
        QCOMPARE(arguments.at(0).toString(), typeName);
    }

    void testServiceRemovedSignal() {
        ServiceLocator& locator = ServiceLocator::instance();
        locator.registerService<ITestService>(new TestServiceImpl());

        QSignalSpy spy(&locator, &ServiceLocator::serviceRemoved);

        QString typeName = QString::fromStdString(typeid(ITestService).name());
        locator.removeService(typeName);

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.at(0).toString(), typeName);
    }

    void testServiceRequestedSignal() {
        ServiceLocator& locator = ServiceLocator::instance();
        locator.registerService<ITestService>(new TestServiceImpl());

        QSignalSpy spy(&locator, &ServiceLocator::serviceRequested);

        auto* service = locator.getService<ITestService>();
        Q_UNUSED(service)

        QCOMPARE(spy.count(), 1);
    }

    void testServiceCreatedSignal() {
        ServiceLocator& locator = ServiceLocator::instance();
        locator.setLazyLoading(true);

        QSignalSpy spy(&locator, &ServiceLocator::serviceCreated);

        locator.registerService<ITestService, TestServiceImpl>();
        auto* service = locator.getService<ITestService>();
        Q_UNUSED(service)

        QCOMPARE(spy.count(), 1);
    }

    void testServiceRegistry() {
        ServiceRegistry registry;

        // Test registering services through registry
        registry.registerService<ITestService, TestServiceImpl>();

        auto* service = ServiceLocator::instance().getService<ITestService>();
        QVERIFY(service != nullptr);
    }

    void testServiceScope() {
        QString typeName = QString::fromStdString(typeid(ITestService).name());

        {
            ServiceScope scope;
            scope.registerScoped<ITestService, TestServiceImpl>();

            QVERIFY(ServiceLocator::instance().hasService(typeName));
            auto* service =
                ServiceLocator::instance().getService<ITestService>();
            QVERIFY(service != nullptr);
        }

        // Service should be removed when scope ends
        QVERIFY(!ServiceLocator::instance().hasService(typeName));
    }

    void testDependencyInjector() {
        // This is a simplified test since full dependency injection
        // would require property metadata
        class TestTarget : public QObject {
            // Q_OBJECT cannot be used in local classes
        public:
            void setTestService(ITestService* service) { m_service = service; }
            ITestService* testService() const { return m_service; }

        private:
            ITestService* m_service = nullptr;
        };

        ServiceLocator::instance().registerService<ITestService>(
            new TestServiceImpl());

        TestTarget target;
        DependencyInjector::inject(&target);

        // Manual injection test
        DependencyInjector::injectService(&target, &TestTarget::setTestService);
        QVERIFY(target.testService() != nullptr);
    }
};

QTEST_MAIN(ServiceLocatorTest)
#include "service_locator_test.moc"
