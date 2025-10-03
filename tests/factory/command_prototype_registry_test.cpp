#include <QTest>
#include <QSignalSpy>
#include <QJsonObject>
#include <QMetaObject>
#include <memory>
#include "../TestUtilities.h"
#include "../../app/factory/CommandFactory.h"
#include "../../app/factory/CommandPrototypeRegistry.h"

class CommandPrototypeRegistryTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic registry functionality
    void testRegistryCreation();
    void testStandardPrototypes();
    void testCustomPrototypeRegistration();
    void testPrototypeRetrieval();
    void testPrototypeCloning();

    // Prototype management
    void testPrototypeUnregistration();
    void testPrototypeClear();
    void testPrototypeInfo();

    // Error handling and edge cases
    void testInvalidPrototypes();
    void testDuplicateRegistration();
    void testNullPrototypeHandling();

    // Performance tests
    void testClonePerformance();
    void testLargeRegistryPerformance();

private:
    std::unique_ptr<CommandFactory> m_factory;
};

void CommandPrototypeRegistryTest::initTestCase() {
    qDebug() << "Initializing CommandPrototypeRegistry tests";
}

void CommandPrototypeRegistryTest::cleanupTestCase() {
    qDebug() << "Cleaning up CommandPrototypeRegistry tests";
}

void CommandPrototypeRegistryTest::init() {
    TestBase::init();
    m_factory = std::make_unique<CommandFactory>();
}

void CommandPrototypeRegistryTest::cleanup() {
    m_factory.reset();
    TestBase::cleanup();
}

void CommandPrototypeRegistryTest::testRegistryCreation() {
    CommandPrototypeRegistry registry(m_factory.get());

    // Test initial state
    QVERIFY(registry.prototypeCount() > 0); // Should have standard prototypes
    QVERIFY(registry.availablePrototypes().contains("open"));
    QVERIFY(registry.availablePrototypes().contains("gotoPage"));
    QVERIFY(registry.availablePrototypes().contains("zoom"));
}

void CommandPrototypeRegistryTest::testStandardPrototypes() {
    CommandPrototypeRegistry registry(m_factory.get());

    QStringList prototypes = registry.availablePrototypes();

    // Verify expected standard prototypes are registered
    QVERIFY(prototypes.contains("open"));
    QVERIFY(prototypes.contains("save"));
    QVERIFY(prototypes.contains("close"));
    QVERIFY(prototypes.contains("print"));
    QVERIFY(prototypes.contains("gotoPage"));
    QVERIFY(prototypes.contains("nextPage"));
    QVERIFY(prototypes.contains("previousPage"));
    QVERIFY(prototypes.contains("zoom"));
    QVERIFY(prototypes.contains("fitToWidth"));
    QVERIFY(prototypes.contains("fitToPage"));
    QVERIFY(prototypes.contains("rotateClockwise"));
    QVERIFY(prototypes.contains("rotateCounterClockwise"));
    QVERIFY(prototypes.contains("toggleFullscreen"));

    // Verify prototype existence checks
    QVERIFY(registry.hasPrototype("open"));
    QVERIFY(registry.hasPrototype("zoom"));
    QVERIFY(!registry.hasPrototype("nonexistent"));
}

void CommandPrototypeRegistryTest::testCustomPrototypeRegistration() {
    CommandPrototypeRegistry registry(m_factory.get());

    // Create a custom prototype
    QObject* customPrototype = new QObject();
    customPrototype->setObjectName("CustomCommand");

    // Register custom prototype
    QVERIFY(registry.registerCustomPrototype("custom", customPrototype));
    QVERIFY(registry.hasPrototype("custom"));
    QVERIFY(registry.availablePrototypes().contains("custom"));

    // Verify prototype count increased
    int initialCount = registry.prototypeCount();
    QObject* anotherCustom = new QObject();
    QVERIFY(registry.registerCustomPrototype("another", anotherCustom));
    QCOMPARE(registry.prototypeCount(), initialCount + 1);
}

void CommandPrototypeRegistryTest::testPrototypeRetrieval() {
    CommandPrototypeRegistry registry(m_factory.get());

    // Test retrieving existing prototypes
    QStringList prototypes = registry.availablePrototypes();
    for (const QString& name : prototypes) {
        QObject* prototype = registry.getPrototype(name);
        QVERIFY(prototype != nullptr);
        QCOMPARE(prototype->objectName(), name);
    }

    // Test retrieving non-existent prototype
    QObject* nonexistent = registry.getPrototype("nonexistent");
    QVERIFY(nonexistent == nullptr);
}

void CommandPrototypeRegistryTest::testPrototypeCloning() {
    CommandPrototypeRegistry registry(m_factory.get());

    // Test cloning cloneable prototypes
    QObject* openClone = registry.cloneCommand("open");
    QVERIFY(openClone != nullptr);
    QVERIFY(openClone != registry.getPrototype("open")); // Should be different object
    delete openClone;

    QObject* gotoPageClone = registry.cloneCommand("gotoPage");
    QVERIFY(gotoPageClone != nullptr);
    QVERIFY(gotoPageClone != registry.getPrototype("gotoPage"));
    delete gotoPageClone;

    QObject* zoomClone = registry.cloneCommand("zoom");
    QVERIFY(zoomClone != nullptr);
    QVERIFY(zoomClone != registry.getPrototype("zoom"));
    delete zoomClone;

    // Test cloning non-existent prototype
    QObject* nonexistentClone = registry.cloneCommand("nonexistent");
    QVERIFY(nonexistentClone == nullptr);
}

void CommandPrototypeRegistryTest::testPrototypeUnregistration() {
    CommandPrototypeRegistry registry(m_factory.get());

    // Add a custom prototype
    QObject* customPrototype = new QObject();
    registry.registerCustomPrototype("temporary", customPrototype);
    QVERIFY(registry.hasPrototype("temporary"));

    // Unregister the prototype
    registry.unregisterPrototype("temporary");
    QVERIFY(!registry.hasPrototype("temporary"));
    QVERIFY(!registry.availablePrototypes().contains("temporary"));

    // Test unregistering non-existent prototype (should not crash)
    registry.unregisterPrototype("nonexistent");
}

void CommandPrototypeRegistryTest::testPrototypeClear() {
    CommandPrototypeRegistry registry(m_factory.get());

    int initialCount = registry.prototypeCount();
    QVERIFY(initialCount > 0);

    // Add some custom prototypes
    registry.registerCustomPrototype("temp1", new QObject());
    registry.registerCustomPrototype("temp2", new QObject());
    QVERIFY(registry.prototypeCount() > initialCount);

    // Clear all prototypes
    registry.clearPrototypes();
    QCOMPARE(registry.prototypeCount(), 0);
    QVERIFY(registry.availablePrototypes().isEmpty());

    // Verify no prototypes exist
    QVERIFY(!registry.hasPrototype("open"));
    QVERIFY(!registry.hasPrototype("zoom"));
}

void CommandPrototypeRegistryTest::testPrototypeInfo() {
    CommandPrototypeRegistry registry(m_factory.get());

    // Test info for existing prototype
    QJsonObject openInfo = registry.getPrototypeInfo("open");
    QVERIFY(!openInfo.isEmpty());
    QCOMPARE(openInfo["name"].toString(), "open");
    QVERIFY(openInfo.contains("className"));
    QVERIFY(openInfo.contains("hasCloneInterface"));
    QVERIFY(openInfo.contains("methods"));

    // Test info for non-existent prototype
    QJsonObject nonexistentInfo = registry.getPrototypeInfo("nonexistent");
    QVERIFY(nonexistentInfo.contains("error"));
    QCOMPARE(nonexistentInfo["error"].toString(), "Prototype not found");
}

void CommandPrototypeRegistryTest::testInvalidPrototypes() {
    CommandPrototypeRegistry registry(m_factory.get());

    // Test registering null prototype
    QVERIFY(!registry.registerCustomPrototype("null", nullptr));
    QVERIFY(!registry.hasPrototype("null"));

    // Test registering with empty name
    QObject* prototype = new QObject();
    QVERIFY(!registry.registerCustomPrototype("", prototype));
    delete prototype;
}

void CommandPrototypeRegistryTest::testDuplicateRegistration() {
    CommandPrototypeRegistry registry(m_factory.get());

    // Register first prototype
    QObject* prototype1 = new QObject();
    QVERIFY(registry.registerCustomPrototype("duplicate", prototype1));
    QVERIFY(registry.hasPrototype("duplicate"));

    // Try to register with same name
    QObject* prototype2 = new QObject();
    // This might succeed and replace the existing prototype, depending on implementation
    // The important thing is that it doesn't crash and the registry remains consistent
    bool result = registry.registerCustomPrototype("duplicate", prototype2);
    Q_UNUSED(result); // We accept either behavior
    QVERIFY(registry.hasPrototype("duplicate"));
}

void CommandPrototypeRegistryTest::testNullPrototypeHandling() {
    CommandPrototypeRegistry registry(m_factory.get());

    // Test operations with null parameters
    QCOMPARE(registry.cloneCommand(nullptr), nullptr);
    QCOMPARE(registry.getPrototype(nullptr), nullptr);
    QVERIFY(!registry.hasPrototype(nullptr));
    QVERIFY(!registry.unregisterPrototype(nullptr));

    // Test empty string handling
    QObject* emptyClone = registry.cloneCommand("");
    QVERIFY(emptyClone == nullptr);
}

void CommandPrototypeRegistryTest::testClonePerformance() {
    CommandPrototypeRegistry registry(m_factory.get());

    const int iterations = 1000;
    QElapsedTimer timer;

    // Measure cloning performance
    timer.start();
    for (int i = 0; i < iterations; ++i) {
        QObject* clone = registry.cloneCommand("open");
        QVERIFY(clone != nullptr);
        delete clone;
    }
    qint64 elapsed = timer.elapsed();

    // Cloning should be reasonably fast (< 100ms for 1000 operations)
    QVERIFY2(elapsed < 100, QString("Cloning performance test failed: %1ms for %2 operations")
                         .arg(elapsed).arg(iterations).toLocal8Bit());
}

void CommandPrototypeRegistryTest::testLargeRegistryPerformance() {
    CommandPrototypeRegistry registry(m_factory.get());

    // Add many custom prototypes
    const int prototypeCount = 1000;
    timer.start();

    for (int i = 0; i < prototypeCount; ++i) {
        QObject* prototype = new QObject();
        registry.registerCustomPrototype(QString("prototype_%1").arg(i), prototype);
    }
    qint64 addTime = timer.elapsed();

    // Adding should be reasonably fast
    QVERIFY2(addTime < 200, QString("Adding prototypes too slow: %1ms").arg(addTime).toLocal8Bit());

    // Test lookup performance
    timer.start();
    for (int i = 0; i < 100; ++i) {
        QString name = QString("prototype_%1").arg(i * 10);
        QVERIFY(registry.hasPrototype(name));
        QObject* prototype = registry.getPrototype(name);
        QVERIFY(prototype != nullptr);
    }
    qint64 lookupTime = timer.elapsed();

    // Lookups should be very fast
    QVERIFY2(lookupTime < 10, QString("Prototype lookup too slow: %1ms").arg(lookupTime).toLocal8Bit());

    // Test enumeration performance
    timer.start();
    QStringList allPrototypes = registry.availablePrototypes();
    qint64 enumerationTime = timer.elapsed();

    // Enumeration should be fast
    QVERIFY2(enumerationTime < 50, QString("Prototype enumeration too slow: %1ms").arg(enumerationTime).toLocal8Bit());
    QVERIFY(allPrototypes.size() >= prototypeCount);

    // Clean up
    registry.clearPrototypes();
}

QTEST_MAIN(CommandPrototypeRegistryTest)
#include "command_prototype_registry_test.moc"