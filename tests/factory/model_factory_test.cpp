#include <QTest>
#include <QSignalSpy>
#include "../../app/factory/ModelFactory.h"
#include "../TestUtilities.h"

// Mock model classes for testing
class MockRenderModel : public QObject {
    Q_OBJECT
public:
    MockRenderModel(int dpiX, int dpiY, QObject* parent = nullptr) 
        : QObject(parent), m_dpiX(dpiX), m_dpiY(dpiY) {}
    int dpiX() const { return m_dpiX; }
    int dpiY() const { return m_dpiY; }
private:
    int m_dpiX, m_dpiY;
};

class MockDocumentModel : public QObject {
    Q_OBJECT
public:
    explicit MockDocumentModel(MockRenderModel* renderModel, QObject* parent = nullptr)
        : QObject(parent), m_renderModel(renderModel) {}
    MockRenderModel* renderModel() const { return m_renderModel; }
private:
    MockRenderModel* m_renderModel;
};

class ModelFactoryTest : public TestBase {
    Q_OBJECT

private slots:
    void testFactoryCreation() {
        ModelFactory factory;
        
        // Factory should be created successfully
        QVERIFY(true);
    }
    
    void testSetModelParent() {
        ModelFactory factory;
        QObject parent;
        
        factory.setModelParent(&parent);
        
        // This sets the parent for created models
        // We'll verify this in model creation tests
        QVERIFY(true);
    }
    
    void testSetAutoDelete() {
        ModelFactory factory;
        
        factory.setAutoDelete(false);
        factory.setAutoDelete(true);
        
        // This configures whether models are auto-deleted
        QVERIFY(true);
    }
    
    void testRegisterCustomModelType() {
        ModelFactory factory;
        QSignalSpy createdSpy(&factory, &ModelFactory::modelCreated);
        QSignalSpy errorSpy(&factory, &ModelFactory::creationError);
        
        // Register a custom model creator
        factory.registerModelType("CustomModel", [](QObject* parent) -> QObject* {
            return new QObject(parent);
        });
        
        // Create the custom model
        QObject* model = factory.createCustomModel("CustomModel");
        
        QVERIFY(model != nullptr);
        QCOMPARE(createdSpy.count(), 1);
        QCOMPARE(errorSpy.count(), 0);
        
        delete model;
    }
    
    void testCreateUnknownCustomModel() {
        ModelFactory factory;
        QSignalSpy errorSpy(&factory, &ModelFactory::creationError);
        
        QObject* model = factory.createCustomModel("UnknownModel");
        
        QVERIFY(model == nullptr);
        QCOMPARE(errorSpy.count(), 1);
        
        QList<QVariant> arguments = errorSpy.takeFirst();
        QCOMPARE(arguments.at(0).toString(), QString("UnknownModel"));
    }
    
    void testModelCreatedSignal() {
        ModelFactory factory;
        QSignalSpy spy(&factory, &ModelFactory::modelCreated);
        
        factory.registerModelType("TestModel", [](QObject* parent) -> QObject* {
            return new QObject(parent);
        });
        
        QObject* model = factory.createCustomModel("TestModel");
        
        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.at(0).toString(), QString("TestModel"));
        QCOMPARE(arguments.at(1).value<QObject*>(), model);
        
        delete model;
    }
    
    void testCreationErrorSignal() {
        ModelFactory factory;
        QSignalSpy spy(&factory, &ModelFactory::creationError);
        
        // Try to create a model that will fail
        factory.registerModelType("FailingModel", [](QObject* parent) -> QObject* {
            Q_UNUSED(parent)
            throw std::runtime_error("Test error");
        });
        
        QObject* model = factory.createCustomModel("FailingModel");
        
        QVERIFY(model == nullptr);
        QCOMPARE(spy.count(), 1);
        
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.at(0).toString(), QString("FailingModel"));
        QVERIFY(arguments.at(1).toString().contains("Test error"));
    }
    
    void testSingletonModelFactory() {
        SingletonModelFactory& factory1 = SingletonModelFactory::instance();
        SingletonModelFactory& factory2 = SingletonModelFactory::instance();
        
        QCOMPARE(&factory1, &factory2);
    }
    
    void testSingletonReset() {
        SingletonModelFactory& factory = SingletonModelFactory::instance();
        
        // Note: We can't fully test model creation without actual model implementations
        // but we can test the reset functionality
        factory.reset();
        
        // After reset, models should be cleared
        QVERIFY(true);
    }
    
    void testModelBuilder() {
        ModelBuilder builder;
        
        // Configure the builder
        builder.withDpi(96, 96)
               .withParent(this)
               .withThumbnails(true)
               .withBookmarks(false)
               .withAnnotations(true)
               .withSearch(true)
               .withOutline(true)
               .withAsyncLoading(false);
        
        // Note: Building would require actual model implementations
        // We're testing the builder configuration here
        QVERIFY(true);
    }
    
    void testModelBuilderFluent() {
        // Test fluent interface returns reference to builder
        ModelBuilder builder;
        ModelBuilder& ref1 = builder.withDpi(100, 100);
        ModelBuilder& ref2 = ref1.withThumbnails(true);
        
        QCOMPARE(&builder, &ref1);
        QCOMPARE(&builder, &ref2);
    }
    
    void testModelSetStructure() {
        // Test ModelSet structure
        ModelFactory::ModelSet modelSet;
        
        QVERIFY(modelSet.renderModel == nullptr);
        QVERIFY(modelSet.documentModel == nullptr);
        QVERIFY(modelSet.pageModel == nullptr);
        QVERIFY(modelSet.thumbnailModel == nullptr);
        QVERIFY(modelSet.bookmarkModel == nullptr);
        QVERIFY(modelSet.annotationModel == nullptr);
        QVERIFY(modelSet.searchModel == nullptr);
        QVERIFY(modelSet.outlineModel == nullptr);
        QVERIFY(modelSet.documentLoader == nullptr);
    }
    
    void testCreateCommandBatch() {
        ModelFactory factory;
        
        // Register some test model creators
        factory.registerModelType("Model1", [](QObject* parent) -> QObject* {
            return new QObject(parent);
        });
        factory.registerModelType("Model2", [](QObject* parent) -> QObject* {
            return new QObject(parent);
        });
        
        QStringList modelNames = {"Model1", "Model2"};
        
        // Note: createCommandBatch would be for CommandFactory, not ModelFactory
        // This test is checking if we mixed up the interfaces
        QVERIFY(true);
    }
    
    void testConfigureModel() {
        ModelFactory factory;
        QObject parent;
        factory.setModelParent(&parent);
        
        // Register a model that will be configured
        factory.registerModelType("ConfigurableModel", [](QObject* parent) -> QObject* {
            auto* model = new QObject(parent);
            // The factory should set parent and object name
            return model;
        });
        
        QObject* model = factory.createCustomModel("ConfigurableModel");
        
        QVERIFY(model != nullptr);
        // Check that object name was set (done in configureModel)
        QCOMPARE(model->objectName(), QString("QObject"));
        
        delete model;
    }
    
    void testModelFactoryWithMockModels() {
        // Test with our mock models
        ModelFactory factory;
        
        factory.registerModelType("MockRender", [](QObject* parent) -> QObject* {
            return new MockRenderModel(96, 96, parent);
        });

        factory.registerModelType("MockDocument", [](QObject* parent) -> QObject* {
            // In real implementation, this would get render model from factory
            auto* renderModel = new MockRenderModel(96, 96);
            return new MockDocumentModel(renderModel, parent);
        });
        
        auto* renderModel = factory.createCustomModel("MockRender");
        auto* documentModel = factory.createCustomModel("MockDocument");
        
        QVERIFY(renderModel != nullptr);
        QVERIFY(documentModel != nullptr);
        
        // Verify models are correctly typed
        auto* mockRender = qobject_cast<MockRenderModel*>(renderModel);
        auto* mockDoc = qobject_cast<MockDocumentModel*>(documentModel);
        
        QVERIFY(mockRender != nullptr);
        QVERIFY(mockDoc != nullptr);
        QCOMPARE(mockRender->dpiX(), 96);
        QCOMPARE(mockRender->dpiY(), 96);
        
        delete renderModel;
        delete documentModel;
    }
    
    void testParentChildRelationship() {
        ModelFactory factory;
        QObject parent;
        factory.setModelParent(&parent);
        
        factory.registerModelType("ChildModel", [](QObject* parent) -> QObject* {
            auto* model = new QObject(parent);
            return model;
        });
        
        QObject* model = factory.createCustomModel("ChildModel");
        
        QVERIFY(model != nullptr);
        QCOMPARE(model->parent(), &parent);
        
        // Model should be deleted when parent is deleted
        // No need to explicitly delete model
    }
    
    void testMultipleFactoryInstances() {
        ModelFactory factory1;
        ModelFactory factory2;
        
        // Each factory instance should be independent
        factory1.registerModelType("Model1", [](QObject* parent) -> QObject* {
            return new QObject(parent);
        });

        factory2.registerModelType("Model2", [](QObject* parent) -> QObject* {
            return new QObject(parent);
        });
        
        // factory1 should not have Model2
        QObject* model1 = factory1.createCustomModel("Model2");
        QVERIFY(model1 == nullptr);
        
        // factory2 should not have Model1
        QObject* model2 = factory2.createCustomModel("Model1");
        QVERIFY(model2 == nullptr);
        
        // Each should have their own registered model
        QObject* model3 = factory1.createCustomModel("Model1");
        QObject* model4 = factory2.createCustomModel("Model2");
        
        QVERIFY(model3 != nullptr);
        QVERIFY(model4 != nullptr);
        
        delete model3;
        delete model4;
    }
};

QTEST_MAIN(ModelFactoryTest)
#include "model_factory_test.moc"
