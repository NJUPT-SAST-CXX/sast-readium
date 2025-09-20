#include <QtTest/QtTest>
#include <QObject>
#include "../../app/search/MemoryManager.h"
#include "../TestUtilities.h"

/**
 * Minimal test for SmartEvictionPolicy to isolate crash issues
 */
class SmartEvictionPolicyTestMinimal : public TestBase
{
    Q_OBJECT

private slots:
    void testBasicInstantiation();
};

void SmartEvictionPolicyTestMinimal::testBasicInstantiation()
{
    // Test that we can create and destroy the policy without crashing
    SmartEvictionPolicy* policy = new SmartEvictionPolicy();
    QVERIFY(policy != nullptr);
    
    // Test basic method calls
    SmartEvictionPolicy::EvictionStrategy strategy = policy->getEvictionStrategy();
    QVERIFY(strategy == SmartEvictionPolicy::LRU ||
            strategy == SmartEvictionPolicy::LFU ||
            strategy == SmartEvictionPolicy::Adaptive ||
            strategy == SmartEvictionPolicy::Predictive);
    
    // Test setting strategy
    policy->setEvictionStrategy(SmartEvictionPolicy::LFU);
    QCOMPARE(policy->getEvictionStrategy(), SmartEvictionPolicy::LFU);
    
    // Test threshold methods
    double threshold = policy->getAdaptiveThreshold();
    QVERIFY(threshold >= 0.0 && threshold <= 1.0);
    
    policy->setAdaptiveThreshold(0.5);
    QCOMPARE(policy->getAdaptiveThreshold(), 0.5);
    
    // Clean up
    delete policy;
}

QTEST_MAIN(SmartEvictionPolicyTestMinimal)
#include "SmartEvictionPolicyTestMinimal.moc"
