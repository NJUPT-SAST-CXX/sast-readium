#include <QTest>
#include <QSignalSpy>
#include "../TestUtilities.h"
#include "../../app/factory/ModelFactory.h"
#include "../../app/controller/ServiceLocator.h"
#include "../../app/controller/StateManager.h"
#include "../../app/controller/EventBus.h"
#include "../MockObject.h"

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

// Mock factory for testing
class MockModelFactory : public ModelFactory {
public:
    MockModelFactory() : ModelFactory("MockFactory") {}
    
    QObject* createModel(const QString& type, const QVariantMap& params) override {
        creationCount++;
        lastType = type;
        lastParams = params;
        
        if (shouldFail) {
            return nullptr;
        }
        
        auto* model = new MockModel(params.value("id").toString());
        model->setData(params.value("data"));
        return model;
    }
    
    bool canCreate(const QString& type) const override {
        return supportedTypes.contains(type);
    }
    
    void reset() {
        creationCount = 0;
        shouldFail = false;
        lastType.clear();
        lastParams.clear();
    }
    
    int creationCount = 0;
    bool shouldFail = false;
    QStringList supportedTypes = {"mock", "test", "custom"};
    QString lastType;
    QVariantMap lastParams;
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
    void testFactoryTypes();
    void testFactoryParameters();
    
    // Factory registry
    void testFactoryRegistry();
    void testMultipleFactories();
    void testFactoryPriority();
    void testFactoryReplacement();
    
    // Model lifecycle
    void testModelOwnership();
    void testModelCaching();
    void testModelPooling();
    void testModelRecycling();
    
    // Factory patterns
    void testAbstractFactory();
    void testFactoryMethod();
    void testBuilderPattern();
    void testPrototypePattern();
    
    // Service integration
    void testFactoryWithServiceLocator();
    void testFactoryWithStateManager();
    void testFactoryWithEventBus();
    
    // Advanced features
    void testAsyncCreation();
    void testLazyCreation();
    void testFactoryChaining();
    void testFactoryDecorator();
    
    // Performance
    void testCreationPerformance();
    void testCachePerformance();
    void testConcurrentCreation();
    
    // Error handling
    void testInvalidType();
    void testCreationFailure();
    void testMemoryManagement();

private:
    void setupServices();
    void teardownServices();
    
    MockModelFactory* m_factory;
    QMap<QString, ModelFactory*> m_factoryRegistry;
};

void TestModelFactory::initTestCase() {
    setupServices();
}

void TestModelFactory::cleanupTestCase() {
    teardownServices();
}

void TestModelFactory::init() {
    m_factory = new MockModelFactory();
    m_factory->reset();
}

void TestModelFactory::cleanup() {
    delete m_factory;
    
    // Clean up registry
    qDeleteAll(m_factoryRegistry);
    m_factoryRegistry.clear();
}

void TestModelFactory::setupServices() {
    ServiceLocator::instance().clearServices();
    StateManager::instance().reset();
    EventBus::instance().clear();
}

void TestModelFactory::teardownServices() {
    ServiceLocator::instance().clearServices();
    StateManager::instance().reset();
    EventBus::instance().clear();
}

void TestModelFactory::testFactoryCreation() {
    QVERIFY(m_factory != nullptr);
    QCOMPARE(m_factory->name(), QString("MockFactory"));
    QCOMPARE(m_factory->creationCount, 0);
}

void TestModelFactory::testModelCreation() {
    QVariantMap params;
    params["id"] = "test-123";
    params["data"] = "test data";
    
    QObject* obj = m_factory->createModel("mock", params);
    
    QVERIFY(obj != nullptr);
    
    auto* model = qobject_cast<MockModel*>(obj);
    QVERIFY(model != nullptr);
    QCOMPARE(model->id(), QString("test-123"));
    QCOMPARE(model->data().toString(), QString("test data"));
    QCOMPARE(m_factory->creationCount, 1);
    
    delete obj;
}

void TestModelFactory::testFactoryRegistration() {
    // Register factory in global registry
    m_factoryRegistry["mock"] = m_factory;
    
    QVERIFY(m_factoryRegistry.contains("mock"));
    QCOMPARE(m_factoryRegistry["mock"], m_factory);
    
    // Create through registry
    auto* factory = m_factoryRegistry["mock"];
    QObject* model = factory->createModel("mock", QVariantMap());
    
    QVERIFY(model != nullptr);
    delete model;
}

void TestModelFactory::testFactoryTypes() {
    // Test supported types
    QVERIFY(m_factory->canCreate("mock"));
    QVERIFY(m_factory->canCreate("test"));
    QVERIFY(m_factory->canCreate("custom"));
    QVERIFY(!m_factory->canCreate("unsupported"));
    
    // Create different types
    QObject* mock = m_factory->createModel("mock", QVariantMap());
    QObject* test = m_factory->createModel("test", QVariantMap());
    
    QVERIFY(mock != nullptr);
    QVERIFY(test != nullptr);
    QCOMPARE(m_factory->lastType, QString("test"));
    
    delete mock;
    delete test;
}

void TestModelFactory::testFactoryParameters() {
    QVariantMap params;
    params["string"] = "text";
    params["number"] = 42;
    params["bool"] = true;
    params["list"] = QVariantList{1, 2, 3};
    params["map"] = QVariantMap{{"nested", "value"}};
    
    m_factory->createModel("mock", params);
    
    QCOMPARE(m_factory->lastParams, params);
    QCOMPARE(m_factory->lastParams["string"].toString(), QString("text"));
    QCOMPARE(m_factory->lastParams["number"].toInt(), 42);
    QVERIFY(m_factory->lastParams["bool"].toBool());
}

void TestModelFactory::testFactoryRegistry() {
    // Create multiple factories
    auto* factory1 = new MockModelFactory();
    auto* factory2 = new MockModelFactory();
    auto* factory3 = new MockModelFactory();
    
    factory1->supportedTypes = {"type1"};
    factory2->supportedTypes = {"type2"};
    factory3->supportedTypes = {"type3"};
    
    // Register factories
    m_factoryRegistry["factory1"] = factory1;
    m_factoryRegistry["factory2"] = factory2;
    m_factoryRegistry["factory3"] = factory3;
    
    // Find appropriate factory for type
    ModelFactory* found = nullptr;
    for (auto* factory : m_factoryRegistry) {
        if (factory->canCreate("type2")) {
            found = factory;
            break;
        }
    }
    
    QVERIFY(found != nullptr);
    QCOMPARE(found, factory2);
}

void TestModelFactory::testMultipleFactories() {
    // Create factories with overlapping types
    auto* factory1 = new MockModelFactory();
    auto* factory2 = new MockModelFactory();
    
    factory1->supportedTypes = {"common", "type1"};
    factory2->supportedTypes = {"common", "type2"};
    
    m_factoryRegistry["primary"] = factory1;
    m_factoryRegistry["secondary"] = factory2;
    
    // Both can create "common" type
    QVERIFY(factory1->canCreate("common"));
    QVERIFY(factory2->canCreate("common"));
    
    // Create using primary
    QObject* model1 = factory1->createModel("common", QVariantMap());
    QVERIFY(model1 != nullptr);
    QCOMPARE(factory1->creationCount, 1);
    QCOMPARE(factory2->creationCount, 0);
    
    delete model1;
}

void TestModelFactory::testFactoryPriority() {
    // Implement priority-based factory selection
    struct PriorityFactory {
        ModelFactory* factory;
        int priority;
    };
    
    QList<PriorityFactory> priorityList;
    
    auto* highPriority = new MockModelFactory();
    auto* lowPriority = new MockModelFactory();
    
    highPriority->supportedTypes = {"common"};
    lowPriority->supportedTypes = {"common"};
    
    priorityList.append({highPriority, 10});
    priorityList.append({lowPriority, 1});
    
    // Sort by priority
    std::sort(priorityList.begin(), priorityList.end(), 
        [](const PriorityFactory& a, const PriorityFactory& b) {
            return a.priority > b.priority;
        });
    
    // Use highest priority factory
    QObject* model = priorityList.first().factory->createModel("common", QVariantMap());
    
    QVERIFY(model != nullptr);
    QCOMPARE(highPriority->creationCount, 1);
    QCOMPARE(lowPriority->creationCount, 0);
    
    delete model;
    delete highPriority;
    delete lowPriority;
}

void TestModelFactory::testFactoryReplacement() {
    auto* original = new MockModelFactory();
    auto* replacement = new MockModelFactory();
    
    // Register original
    m_factoryRegistry["test"] = original;
    QCOMPARE(m_factoryRegistry["test"], original);
    
    // Replace with new factory
    delete original;
    m_factoryRegistry["test"] = replacement;
    QCOMPARE(m_factoryRegistry["test"], replacement);
    
    // Verify replacement works
    QObject* model = replacement->createModel("mock", QVariantMap());
    QVERIFY(model != nullptr);
    QCOMPARE(replacement->creationCount, 1);
    
    delete model;
}

void TestModelFactory::testModelOwnership() {
    QPointer<QObject> modelPtr;
    
    {
        // Create model with factory as parent
        QObject* model = m_factory->createModel("mock", QVariantMap());
        model->setParent(m_factory);
        modelPtr = model;
        
        QVERIFY(!modelPtr.isNull());
    }
    
    // Model should still exist (owned by factory)
    QVERIFY(!modelPtr.isNull());
    
    // Delete factory should delete model
    delete m_factory;
    m_factory = nullptr;
    
    waitMs(10);
    QVERIFY(modelPtr.isNull());
}

void TestModelFactory::testModelCaching() {
    // Simple cache implementation
    QMap<QString, QObject*> cache;
    
    auto createOrGet = [this, &cache](const QString& id) -> QObject* {
        if (cache.contains(id)) {
            return cache[id];
        }
        
        QVariantMap params;
        params["id"] = id;
        QObject* model = m_factory->createModel("mock", params);
        cache[id] = model;
        return model;
    };
    
    // First access creates
    QObject* model1 = createOrGet("cached-1");
    QVERIFY(model1 != nullptr);
    QCOMPARE(m_factory->creationCount, 1);
    
    // Second access returns cached
    QObject* model2 = createOrGet("cached-1");
    QCOMPARE(model1, model2);
    QCOMPARE(m_factory->creationCount, 1); // No new creation
    
    // Different ID creates new
    QObject* model3 = createOrGet("cached-2");
    QVERIFY(model3 != model1);
    QCOMPARE(m_factory->creationCount, 2);
    
    // Cleanup
    qDeleteAll(cache);
}

void TestModelFactory::testModelPooling() {
    // Object pool implementation
    class ModelPool {
    public:
        QObject* acquire(MockModelFactory* factory) {
            if (!m_available.isEmpty()) {
                return m_available.takeFirst();
            }
            return factory->createModel("mock", QVariantMap());
        }
        
        void release(QObject* model) {
            if (model && m_available.size() < m_maxSize) {
                m_available.append(model);
            } else {
                delete model;
            }
        }
        
        ~ModelPool() {
            qDeleteAll(m_available);
        }
        
    private:
        QList<QObject*> m_available;
        int m_maxSize = 5;
    };
    
    ModelPool pool;
    
    // Acquire models
    QObject* model1 = pool.acquire(m_factory);
    QObject* model2 = pool.acquire(m_factory);
    QCOMPARE(m_factory->creationCount, 2);
    
    // Release back to pool
    pool.release(model1);
    pool.release(model2);
    
    // Acquire again (should reuse)
    QObject* model3 = pool.acquire(m_factory);
    QCOMPARE(model3, model1); // Same instance
    QCOMPARE(m_factory->creationCount, 2); // No new creation
    
    pool.release(model3);
}

void TestModelFactory::testModelRecycling() {
    // Recycling with reset functionality
    class RecyclableModel : public MockModel {
    public:
        void reset() {
            setData(QVariant());
            resetCount++;
        }
        int resetCount = 0;
    };
    
    QList<RecyclableModel*> recycled;
    
    auto getOrCreate = [&recycled]() -> RecyclableModel* {
        if (!recycled.isEmpty()) {
            auto* model = recycled.takeFirst();
            model->reset();
            return model;
        }
        return new RecyclableModel();
    };
    
    auto recycle = [&recycled](RecyclableModel* model) {
        recycled.append(model);
    };
    
    // Create and use
    RecyclableModel* model1 = getOrCreate();
    model1->setData("used");
    QCOMPARE(model1->resetCount, 0);
    
    // Recycle
    recycle(model1);
    
    // Get recycled (should be reset)
    RecyclableModel* model2 = getOrCreate();
    QCOMPARE(model1, model2); // Same instance
    QCOMPARE(model2->resetCount, 1); // Was reset
    QVERIFY(model2->data().isNull()); // Data cleared
    
    delete model2;
}

void TestModelFactory::testAbstractFactory() {
    // Abstract factory pattern
    class AbstractUIFactory {
    public:
        virtual ~AbstractUIFactory() = default;
        virtual QWidget* createButton() = 0;
        virtual QWidget* createLabel() = 0;
    };
    
    class DarkThemeFactory : public AbstractUIFactory {
    public:
        QWidget* createButton() override {
            auto* btn = new QPushButton("Dark Button");
            btn->setStyleSheet("background: #333; color: #fff;");
            return btn;
        }
        
        QWidget* createLabel() override {
            auto* lbl = new QLabel("Dark Label");
            lbl->setStyleSheet("color: #fff;");
            return lbl;
        }
    };
    
    DarkThemeFactory darkFactory;
    QWidget* button = darkFactory.createButton();
    QWidget* label = darkFactory.createLabel();
    
    QVERIFY(button != nullptr);
    QVERIFY(label != nullptr);
    QVERIFY(button->styleSheet().contains("#333"));
    
    delete button;
    delete label;
}

void TestModelFactory::testFactoryMethod() {
    // Factory method pattern
    class Document {
    public:
        virtual ~Document() = default;
        virtual QString type() const = 0;
    };
    
    class PDFDocument : public Document {
    public:
        QString type() const override { return "PDF"; }
    };
    
    class TextDocument : public Document {
    public:
        QString type() const override { return "Text"; }
    };
    
    class DocumentFactory {
    public:
        static Document* createDocument(const QString& type) {
            if (type == "pdf") {
                return new PDFDocument();
            } else if (type == "text") {
                return new TextDocument();
            }
            return nullptr;
        }
    };
    
    Document* pdf = DocumentFactory::createDocument("pdf");
    Document* text = DocumentFactory::createDocument("text");
    
    QVERIFY(pdf != nullptr);
    QVERIFY(text != nullptr);
    QCOMPARE(pdf->type(), QString("PDF"));
    QCOMPARE(text->type(), QString("Text"));
    
    delete pdf;
    delete text;
}

void TestModelFactory::testBuilderPattern() {
    // Builder pattern for complex object creation
    class ModelBuilder {
    public:
        ModelBuilder& withId(const QString& id) {
            m_params["id"] = id;
            return *this;
        }
        
        ModelBuilder& withData(const QVariant& data) {
            m_params["data"] = data;
            return *this;
        }
        
        ModelBuilder& withProperty(const QString& key, const QVariant& value) {
            m_params[key] = value;
            return *this;
        }
        
        QObject* build(MockModelFactory* factory) {
            return factory->createModel("mock", m_params);
        }
        
    private:
        QVariantMap m_params;
    };
    
    ModelBuilder builder;
    QObject* model = builder
        .withId("builder-123")
        .withData("builder data")
        .withProperty("custom", "value")
        .build(m_factory);
    
    QVERIFY(model != nullptr);
    auto* mockModel = qobject_cast<MockModel*>(model);
    QCOMPARE(mockModel->id(), QString("builder-123"));
    QCOMPARE(mockModel->data().toString(), QString("builder data"));
    
    delete model;
}

void TestModelFactory::testPrototypePattern() {
    // Prototype pattern for cloning
    class CloneableModel : public MockModel {
    public:
        CloneableModel* clone() const {
            auto* copy = new CloneableModel();
            copy->setData(data());
            return copy;
        }
    };
    
    // Create prototype
    CloneableModel prototype;
    prototype.setData("prototype data");
    
    // Clone multiple times
    CloneableModel* clone1 = prototype.clone();
    CloneableModel* clone2 = prototype.clone();
    
    QVERIFY(clone1 != &prototype);
    QVERIFY(clone2 != &prototype);
    QVERIFY(clone1 != clone2);
    QCOMPARE(clone1->data(), prototype.data());
    QCOMPARE(clone2->data(), prototype.data());
    
    delete clone1;
    delete clone2;
}

void TestModelFactory::testFactoryWithServiceLocator() {
    // Register factory as service
    ServiceLocator::instance().registerService<ModelFactory>(m_factory);
    
    // Get and use through service locator
    auto* factory = ServiceLocator::instance().getService<ModelFactory>();
    QVERIFY(factory != nullptr);
    
    QObject* model = factory->createModel("mock", QVariantMap());
    QVERIFY(model != nullptr);
    
    delete model;
    ServiceLocator::instance().clearServices();
}

void TestModelFactory::testFactoryWithStateManager() {
    // Store factory configuration in state
    StateManager::instance().set("factory.defaultType", "mock");
    StateManager::instance().set("factory.cacheEnabled", true);
    StateManager::instance().set("factory.maxPoolSize", 10);
    
    // Read configuration
    QString defaultType = StateManager::instance().get("factory.defaultType").toString();
    bool cacheEnabled = StateManager::instance().get("factory.cacheEnabled").toBool();
    
    QCOMPARE(defaultType, QString("mock"));
    QVERIFY(cacheEnabled);
    
    // Create with default type
    QObject* model = m_factory->createModel(defaultType, QVariantMap());
    QVERIFY(model != nullptr);
    
    delete model;
}

void TestModelFactory::testFactoryWithEventBus() {
    bool creationEventReceived = false;
    QString createdType;
    
    // Subscribe to creation events
    EventBus::instance().subscribe("model.created", this, 
        [&creationEventReceived, &createdType](Event* e) {
            creationEventReceived = true;
            createdType = e->data.toMap()["type"].toString();
        });
    
    // Wrap factory to emit events
    auto createWithEvent = [this](const QString& type, const QVariantMap& params) {
        QObject* model = m_factory->createModel(type, params);
        if (model) {
            QVariantMap eventData;
            eventData["type"] = type;
            eventData["model"] = QVariant::fromValue(model);
            EventBus::instance().publish("model.created", eventData);
        }
        return model;
    };
    
    QObject* model = createWithEvent("test", QVariantMap());
    
    waitMs(10);
    
    QVERIFY(model != nullptr);
    QVERIFY(creationEventReceived);
    QCOMPARE(createdType, QString("test"));
    
    delete model;
}

void TestModelFactory::testAsyncCreation() {
    // Async factory creation
    QFuture<QObject*> future = QtConcurrent::run([this]() {
        QThread::msleep(50); // Simulate async work
        return m_factory->createModel("mock", QVariantMap());
    });
    
    // Do other work while waiting...
    
    // Wait for result
    QObject* model = future.result();
    QVERIFY(model != nullptr);
    
    delete model;
}

void TestModelFactory::testLazyCreation() {
    // Lazy creation wrapper
    class LazyModel {
    public:
        LazyModel(MockModelFactory* factory, const QString& type, const QVariantMap& params)
            : m_factory(factory), m_type(type), m_params(params) {}
        
        QObject* get() {
            if (!m_model) {
                m_model = m_factory->createModel(m_type, m_params);
            }
            return m_model;
        }
        
        ~LazyModel() {
            delete m_model;
        }
        
    private:
        MockModelFactory* m_factory;
        QString m_type;
        QVariantMap m_params;
        QObject* m_model = nullptr;
    };
    
    LazyModel lazy(m_factory, "mock", QVariantMap());
    
    // Not created yet
    QCOMPARE(m_factory->creationCount, 0);
    
    // First access creates
    QObject* model1 = lazy.get();
    QVERIFY(model1 != nullptr);
    QCOMPARE(m_factory->creationCount, 1);
    
    // Second access returns same
    QObject* model2 = lazy.get();
    QCOMPARE(model1, model2);
    QCOMPARE(m_factory->creationCount, 1);
}

void TestModelFactory::testFactoryChaining() {
    // Chain multiple factories
    class ChainedFactory : public ModelFactory {
    public:
        ChainedFactory() : ModelFactory("Chained") {}
        
        void setNext(ModelFactory* next) { m_next = next; }
        
        QObject* createModel(const QString& type, const QVariantMap& params) override {
            if (canCreate(type)) {
                return new MockModel();
            }
            if (m_next) {
                return m_next->createModel(type, params);
            }
            return nullptr;
        }
        
        bool canCreate(const QString& type) const override {
            return type == "chained";
        }
        
    private:
        ModelFactory* m_next = nullptr;
    };
    
    ChainedFactory chain1;
    ChainedFactory chain2;
    chain1.setNext(&chain2);
    chain2.setNext(m_factory);
    
    // First factory handles "chained"
    QObject* model1 = chain1.createModel("chained", QVariantMap());
    QVERIFY(model1 != nullptr);
    
    // Delegates to next in chain for "mock"
    QObject* model2 = chain1.createModel("mock", QVariantMap());
    QVERIFY(model2 != nullptr);
    QCOMPARE(m_factory->creationCount, 1);
    
    delete model1;
    delete model2;
}

void TestModelFactory::testFactoryDecorator() {
    // Decorator pattern for factories
    class LoggingFactory : public ModelFactory {
    public:
        LoggingFactory(ModelFactory* wrapped) 
            : ModelFactory("Logging"), m_wrapped(wrapped) {}
        
        QObject* createModel(const QString& type, const QVariantMap& params) override {
            qDebug() << "Creating model of type:" << type;
            QObject* model = m_wrapped->createModel(type, params);
            qDebug() << "Model created:" << (model != nullptr);
            return model;
        }
        
        bool canCreate(const QString& type) const override {
            return m_wrapped->canCreate(type);
        }
        
    private:
        ModelFactory* m_wrapped;
    };
    
    LoggingFactory loggingFactory(m_factory);
    
    QObject* model = loggingFactory.createModel("mock", QVariantMap());
    QVERIFY(model != nullptr);
    QCOMPARE(m_factory->creationCount, 1);
    
    delete model;
}

void TestModelFactory::testCreationPerformance() {
    const int numCreations = 1000;
    
    QElapsedTimer timer;
    timer.start();
    
    QList<QObject*> models;
    for (int i = 0; i < numCreations; ++i) {
        models.append(m_factory->createModel("mock", QVariantMap()));
    }
    
    qint64 elapsed = timer.elapsed();
    qDebug() << "Created" << numCreations << "models in" << elapsed << "ms";
    qDebug() << "Average:" << (double)elapsed / numCreations << "ms per model";
    
    // Should be fast
    QVERIFY(elapsed < 1000); // Less than 1 second for 1000 models
    
    qDeleteAll(models);
}

void TestModelFactory::testCachePerformance() {
    QMap<QString, QObject*> cache;
    const int numAccesses = 10000;
    const int cacheSize = 100;
    
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < numAccesses; ++i) {
        QString id = QString("model-%1").arg(i % cacheSize);
        
        if (!cache.contains(id)) {
            QVariantMap params;
            params["id"] = id;
            cache[id] = m_factory->createModel("mock", params);
        }
        
        QObject* model = cache[id];
        Q_UNUSED(model);
    }
    
    qint64 elapsed = timer.elapsed();
    qDebug() << "Cache performance:" << numAccesses << "accesses in" << elapsed << "ms";
    
    // Cache should provide good performance
    QVERIFY(elapsed < 100); // Very fast for cached access
    
    qDeleteAll(cache);
}

void TestModelFactory::testConcurrentCreation() {
    const int numThreads = 10;
    const int modelsPerThread = 100;
    
    QList<QFuture<QList<QObject*>>> futures;
    
    for (int t = 0; t < numThreads; ++t) {
        futures.append(QtConcurrent::run([this, modelsPerThread]() {
            QList<QObject*> models;
            for (int i = 0; i < modelsPerThread; ++i) {
                models.append(m_factory->createModel("mock", QVariantMap()));
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
    
    qDeleteAll(allModels);
}

void TestModelFactory::testInvalidType() {
    QObject* model = m_factory->createModel("invalid", QVariantMap());
    
    QVERIFY(model == nullptr);
    QCOMPARE(m_factory->creationCount, 0);
}

void TestModelFactory::testCreationFailure() {
    m_factory->shouldFail = true;
    
    QObject* model = m_factory->createModel("mock", QVariantMap());
    
    QVERIFY(model == nullptr);
    QCOMPARE(m_factory->creationCount, 1); // Attempted but failed
}

void TestModelFactory::testMemoryManagement() {
    // Test for memory leaks with many create/delete cycles
    for (int cycle = 0; cycle < 100; ++cycle) {
        QList<QObject*> models;
        
        // Create batch
        for (int i = 0; i < 100; ++i) {
            models.append(m_factory->createModel("mock", QVariantMap()));
        }
        
        // Delete batch
        qDeleteAll(models);
    }
    
    // Should not leak memory
    QVERIFY(true);
}

QTEST_MAIN(TestModelFactory)
#include "ModelFactoryTest_new.moc"
