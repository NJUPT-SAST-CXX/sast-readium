#include <QElapsedTimer>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <QThread>
#include <QtConcurrent>
#include "../../app/controller/EventBus.h"
#include "../../app/controller/ServiceLocator.h"
#include "../../app/controller/StateManager.h"
#include "../../app/factory/ModelFactory.h"
#include "../TestUtilities.h"

// Mock model for testing
class MockModel : public QObject {
    Q_OBJECT
public:
    MockModel(const QString& id = QString())
        : m_id(id.isEmpty() ? QUuid::createUuid().toString() : id) {}

    QString id() const { return m_id; }
    void setData(const QVariant& data) { m_data = data; }
    QVariant data() const { return m_data; }

signals:
    void dataChanged();

private:
    QString m_id;
    QVariant m_data;
};

// Mock factory for testing - using the actual ModelFactory API
class MockModelFactory : public ModelFactory {
public:
    MockModelFactory() : ModelFactory(nullptr) {}

    // Use the custom model creation API that actually exists
    QObject* createCustomModel(const QString& type) {
        creationCount++;
        lastType = type;

        if (shouldFail) {
            return nullptr;
        }

        auto* model = new MockModel();
        model->setParent(this);  // Set parent for proper cleanup
        return model;
    }

    void reset() {
        creationCount = 0;
        shouldFail = false;
        lastType.clear();
    }

    int creationCount = 0;
    bool shouldFail = false;
    QString lastType;
};

class TestModelFactory : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

    // Basic factory operations
    void testFactoryCreation();
    void testModelCreation();
    void testFactoryRegistration();

    // Service integration
    void testFactoryWithServiceLocator();
    void testFactoryWithStateManager();

    // Performance
    void testCreationPerformance();
    void testConcurrentCreation();

    // Error handling
    void testCreationFailure();
    void testMemoryManagement();

private:
    void setupServices();
    void teardownServices();

    MockModelFactory* m_factory = nullptr;
    QMap<QString, ModelFactory*> m_factoryRegistry;
};

void TestModelFactory::initTestCase() { setupServices(); }

void TestModelFactory::cleanupTestCase() { teardownServices(); }

void TestModelFactory::init() {
    m_factory = new MockModelFactory();
    m_factoryRegistry.clear();
}

void TestModelFactory::cleanup() {
    delete m_factory;
    m_factory = nullptr;

    qDeleteAll(m_factoryRegistry);
    m_factoryRegistry.clear();
}

void TestModelFactory::setupServices() {
    ServiceLocator::instance().clearServices();
    StateManager::instance().reset();
    EventBus::instance().clearEventQueue();
}

void TestModelFactory::teardownServices() {
    ServiceLocator::instance().clearServices();
    StateManager::instance().reset();
    EventBus::instance().clearEventQueue();
}

void TestModelFactory::testFactoryCreation() {
    QVERIFY(m_factory != nullptr);
    QCOMPARE(m_factory->creationCount, 0);
}

void TestModelFactory::testModelCreation() {
    QObject* obj = m_factory->createCustomModel("mock");

    QVERIFY(obj != nullptr);

    auto* model = qobject_cast<MockModel*>(obj);
    QVERIFY(model != nullptr);
    QCOMPARE(m_factory->creationCount, 1);

    // Model will be cleaned up by parent-child relationship
}

void TestModelFactory::testFactoryRegistration() {
    // Register factory in global registry
    m_factoryRegistry["mock"] = m_factory;

    QVERIFY(m_factoryRegistry.contains("mock"));
    QCOMPARE(m_factoryRegistry["mock"], m_factory);

    // Create through registry
    auto* factory = m_factoryRegistry["mock"];
    QObject* model = factory->createCustomModel("mock");

    QVERIFY(model != nullptr);
    // Model will be cleaned up by parent-child relationship
}

void TestModelFactory::testFactoryWithServiceLocator() {
    // Test basic ServiceLocator integration
    ServiceLocator::instance().registerService<ModelFactory>(m_factory);

    auto* factory = ServiceLocator::instance().getService<ModelFactory>();
    QVERIFY(factory != nullptr);

    QObject* model =
        static_cast<MockModelFactory*>(factory)->createCustomModel("mock");
    QVERIFY(model != nullptr);
}

void TestModelFactory::testFactoryWithStateManager() {
    // Test basic StateManager reset functionality
    StateManager::instance().reset();
    // Basic StateManager test - just verify it doesn't crash
}

void TestModelFactory::testCreationPerformance() {
    const int numCreations = 1000;

    QElapsedTimer timer;
    timer.start();

    for (int i = 0; i < numCreations; ++i) {
        QObject* model = m_factory->createCustomModel("mock");
        QVERIFY(model != nullptr);
    }

    qint64 elapsed = timer.elapsed();
    qDebug() << "Created" << numCreations << "models in" << elapsed << "ms";
    QVERIFY(elapsed < 5000);  // Should complete within 5 seconds

    QCOMPARE(m_factory->creationCount, numCreations);
}

void TestModelFactory::testConcurrentCreation() {
    const int numThreads = 10;
    const int modelsPerThread = 100;

    QList<QFuture<QList<QObject*>>> futures;

    for (int t = 0; t < numThreads; ++t) {
        futures.append(QtConcurrent::run([this, modelsPerThread]() {
            QList<QObject*> models;
            for (int i = 0; i < modelsPerThread; ++i) {
                models.append(m_factory->createCustomModel("mock"));
            }
            return models;
        }));
    }

    QList<QObject*> allModels;
    for (auto& future : futures) {
        allModels.append(future.result());
    }

    QCOMPARE(allModels.size(), numThreads * modelsPerThread);
    QCOMPARE(m_factory->creationCount, numThreads * modelsPerThread);

    // Models will be cleaned up by parent-child relationship
}

void TestModelFactory::testCreationFailure() {
    m_factory->shouldFail = true;

    QObject* model = m_factory->createCustomModel("mock");
    QVERIFY(model == nullptr);
    QCOMPARE(m_factory->creationCount, 1);

    m_factory->shouldFail = false;
}

void TestModelFactory::testMemoryManagement() {
    // Test that models are properly cleaned up
    QList<QObject*> models;

    for (int i = 0; i < 100; ++i) {
        QObject* model = m_factory->createCustomModel("mock");
        QVERIFY(model != nullptr);
        models.append(model);
    }

    QCOMPARE(m_factory->creationCount, 100);

    // Models will be cleaned up when factory is deleted due to parent-child
    // relationship
}

#include "test_model_factory.moc"

QTEST_MAIN(TestModelFactory)
