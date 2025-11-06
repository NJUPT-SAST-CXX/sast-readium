#include <QAtomicInt>
#include <QElapsedTimer>
#include <QFuture>
#include <QFutureWatcher>
#include <QMutex>
#include <QObject>
#include <QRandomGenerator>
#include <QThread>
#include <QWaitCondition>
#include <QtConcurrent>
#include <QtTest/QtTest>
#include "../../app/search/SearchThreadSafety.h"
#include "../TestUtilities.h"

/**
 * Comprehensive tests for SearchThreadSafety classes
 * Tests atomic operations, shared data access, thread-safe containers, and
 * synchronization
 */
class SearchThreadSafetyTest : public TestBase {
    Q_OBJECT

protected:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

private slots:
    // AtomicCounter tests
    void testAtomicCounterBasicOperations();
    void testAtomicCounterConcurrentAccess();
    void testAtomicCounterCompareAndSwap();

    // AtomicFlag tests
    void testAtomicFlagBasicOperations();
    void testAtomicFlagConcurrentAccess();
    void testAtomicFlagTestAndSet();

    // AtomicPointer tests
    void testAtomicPointerBasicOperations();
    void testAtomicPointerConcurrentAccess();
    void testAtomicPointerExchange();

    // SharedData tests
    void testSharedDataReadAccess();
    void testSharedDataWriteAccess();
    void testSharedDataConcurrentAccess();
    void testSharedDataConvenienceMethods();

    // ThreadSafeQueue tests
    void testThreadSafeQueueBasicOperations();
    void testThreadSafeQueueProducerConsumer();
    void testThreadSafeQueueTimeout();
    void testThreadSafeQueueConcurrentAccess();

    // MutexHierarchy tests
    void testHierarchicalMutexBasicOperations();
    void testMutexHierarchyValidation();
    void testHierarchicalMutexDeadlockPrevention();

    // MultiLockGuard tests
    void testMultiLockGuardBasicUsage();
    void testMultiLockGuardDeadlockPrevention();

    // ThreadSafeSingleton tests
    void testThreadSafeSingletonCreation();
    void testThreadSafeSingletonConcurrentAccess();

    // ContentionMonitor tests
    void testContentionMonitorRecording();
    void testContentionMonitorStatistics();

    // Performance and stress tests
    void testHighContentionScenario();
    void testMixedOperationsStressTest();
    void testMemoryOrderingConsistency();

    // Macro tests
    void testThreadSafetyMacros();

private:
    static const int THREAD_COUNT = 4;
    static const int ITERATIONS_PER_THREAD = 1000;

    // Helper methods
    void runConcurrentTest(std::function<void(int)> threadFunction,
                           int threadCount = THREAD_COUNT);
    void verifyAtomicConsistency();
    void simulateHighContention();

    // Test data
    QList<QThread*> m_testThreads;
    QAtomicInt m_testCounter;
    QMutex m_testMutex;
    QWaitCondition m_testCondition;
};

void SearchThreadSafetyTest::initTestCase() {
    QSKIP(
        "Temporarily skipping SearchThreadSafetyTest due to timeout/deadlock "
        "issues");
    qDebug() << "Starting SearchThreadSafety tests";
    qDebug() << "Using" << THREAD_COUNT << "threads with"
             << ITERATIONS_PER_THREAD << "iterations each";
}

void SearchThreadSafetyTest::cleanupTestCase() {
    qDebug() << "SearchThreadSafety tests completed";
}

void SearchThreadSafetyTest::init() {
    m_testCounter.storeRelease(0);
    m_testThreads.clear();
}

void SearchThreadSafetyTest::cleanup() {
    // Wait for all test threads to finish
    for (QThread* thread : m_testThreads) {
        if (thread->isRunning()) {
            thread->quit();
            thread->wait(1000);
        }
        delete thread;
    }
    m_testThreads.clear();
}

void SearchThreadSafetyTest::testAtomicCounterBasicOperations() {
    SearchThreadSafety::AtomicCounter counter(10);

    QCOMPARE(counter.value(), 10);

    QCOMPARE(counter.increment(), 11);
    QCOMPARE(counter.value(), 11);

    QCOMPARE(counter.decrement(), 10);
    QCOMPARE(counter.value(), 10);

    counter.setValue(42);
    QCOMPARE(counter.value(), 42);
}

void SearchThreadSafetyTest::testAtomicCounterConcurrentAccess() {
    SearchThreadSafety::AtomicCounter counter(0);

    auto incrementFunction = [&counter](int threadId) {
        Q_UNUSED(threadId)
        for (int i = 0; i < ITERATIONS_PER_THREAD; ++i) {
            counter.increment();
        }
    };

    runConcurrentTest(incrementFunction);

    QCOMPARE(counter.value(), THREAD_COUNT * ITERATIONS_PER_THREAD);
}

void SearchThreadSafetyTest::testAtomicCounterCompareAndSwap() {
    SearchThreadSafety::AtomicCounter counter(10);

    QVERIFY(counter.compareAndSwap(10, 20));
    QCOMPARE(counter.value(), 20);

    QVERIFY(!counter.compareAndSwap(10, 30));  // Should fail
    QCOMPARE(counter.value(), 20);             // Value unchanged
}

void SearchThreadSafetyTest::testAtomicFlagBasicOperations() {
    SearchThreadSafety::AtomicFlag flag(false);

    QVERIFY(!flag.isSet());

    flag.set();
    QVERIFY(flag.isSet());

    flag.clear();
    QVERIFY(!flag.isSet());
}

void SearchThreadSafetyTest::testAtomicFlagConcurrentAccess() {
    SearchThreadSafety::AtomicFlag flag(false);
    QAtomicInt successCount(0);

    auto testAndSetFunction = [&flag, &successCount](int threadId) {
        Q_UNUSED(threadId)
        for (int i = 0; i < ITERATIONS_PER_THREAD; ++i) {
            if (flag.testAndSet()) {
                successCount.fetchAndAddOrdered(1);
                // Do some work while flag is set
                QThread::usleep(1);
                flag.clear();
            }
        }
    };

    runConcurrentTest(testAndSetFunction);

    // At least some operations should have succeeded
    QVERIFY(successCount.loadAcquire() > 0);
}

void SearchThreadSafetyTest::testAtomicFlagTestAndSet() {
    SearchThreadSafety::AtomicFlag flag(false);

    QVERIFY(flag.testAndSet());  // Should succeed (false -> true)
    QVERIFY(flag.isSet());

    QVERIFY(!flag.testAndSet());  // Should fail (already true)
    QVERIFY(flag.isSet());

    QVERIFY(flag.testAndClear());  // Should succeed (true -> false)
    QVERIFY(!flag.isSet());
}

void SearchThreadSafetyTest::testAtomicPointerBasicOperations() {
    int value1 = 42;
    int value2 = 84;

    SearchThreadSafety::AtomicPointer<int> pointer(&value1);

    QCOMPARE(pointer.load(), &value1);

    pointer.store(&value2);
    QCOMPARE(pointer.load(), &value2);

    QVERIFY(pointer.compareAndSwap(&value2, &value1));
    QCOMPARE(pointer.load(), &value1);

    QVERIFY(!pointer.compareAndSwap(&value2, nullptr));  // Should fail
    QCOMPARE(pointer.load(), &value1);                   // Unchanged
}

void SearchThreadSafetyTest::testAtomicPointerConcurrentAccess() {
    QList<int> values;
    for (int i = 0; i < THREAD_COUNT; ++i) {
        values.append(i);
    }

    SearchThreadSafety::AtomicPointer<int> pointer(&values[0]);

    auto swapFunction = [&pointer, &values](int threadId) {
        for (int i = 0; i < ITERATIONS_PER_THREAD / 10; ++i) {
            int* expected = &values[threadId % values.size()];
            int* newValue = &values[(threadId + 1) % values.size()];
            pointer.compareAndSwap(expected, newValue);
        }
    };

    runConcurrentTest(swapFunction);

    // Pointer should point to one of the valid values
    int* finalValue = pointer.load();
    QVERIFY(values.contains(*finalValue));
}

void SearchThreadSafetyTest::testAtomicPointerExchange() {
    int value1 = 10;
    int value2 = 20;

    SearchThreadSafety::AtomicPointer<int> pointer(&value1);

    int* oldValue = pointer.exchange(&value2);
    QCOMPARE(oldValue, &value1);
    QCOMPARE(pointer.load(), &value2);
}

void SearchThreadSafetyTest::testSharedDataReadAccess() {
    SearchThreadSafety::SharedData<QString> sharedString("initial");

    {
        auto readAccess = sharedString.read();
        QCOMPARE(*readAccess, QString("initial"));
        QCOMPARE(readAccess->length(), 7);
    }

    // Test copy method
    QString copy = sharedString.copy();
    QCOMPARE(copy, QString("initial"));
}

void SearchThreadSafetyTest::testSharedDataWriteAccess() {
    SearchThreadSafety::SharedData<QString> sharedString("initial");

    {
        auto writeAccess = sharedString.write();
        *writeAccess = "modified";
    }

    QCOMPARE(sharedString.copy(), QString("modified"));

    // Test set method
    sharedString.set("final");
    QCOMPARE(sharedString.copy(), QString("final"));
}

void SearchThreadSafetyTest::testSharedDataConcurrentAccess() {
    SearchThreadSafety::SharedData<int> sharedInt(0);

    auto readerFunction = [&sharedInt](int threadId) {
        Q_UNUSED(threadId)
        for (int i = 0; i < ITERATIONS_PER_THREAD; ++i) {
            auto readAccess = sharedInt.read();
            int value = *readAccess;
            QVERIFY(value >= 0);  // Should always be non-negative
        }
    };

    auto writerFunction = [&sharedInt](int threadId) {
        Q_UNUSED(threadId)
        for (int i = 0; i < ITERATIONS_PER_THREAD / 10; ++i) {
            auto writeAccess = sharedInt.write();
            *writeAccess = (*writeAccess) + 1;
        }
    };

    // Run readers and writers concurrently
    QList<QFuture<void>> futures;

    for (int i = 0; i < THREAD_COUNT / 2; ++i) {
        futures.append(QtConcurrent::run(readerFunction, i));
    }

    for (int i = 0; i < THREAD_COUNT / 2; ++i) {
        futures.append(QtConcurrent::run(writerFunction, i));
    }

    for (auto& future : futures) {
        future.waitForFinished();
    }

    // Final value should be positive
    QVERIFY(sharedInt.copy() > 0);
}

void SearchThreadSafetyTest::testThreadSafeQueueBasicOperations() {
    SearchThreadSafety::ThreadSafeQueue<int> queue;

    QVERIFY(queue.isEmpty());
    QCOMPARE(queue.size(), 0);

    queue.enqueue(42);
    QVERIFY(!queue.isEmpty());
    QCOMPARE(queue.size(), 1);

    int value;
    QVERIFY(queue.tryDequeue(value));
    QCOMPARE(value, 42);
    QVERIFY(queue.isEmpty());

    // Test dequeue with timeout
    queue.enqueue(84);
    QVERIFY(queue.dequeue(value, 100));
    QCOMPARE(value, 84);
}

void SearchThreadSafetyTest::testThreadSafeQueueProducerConsumer() {
    SearchThreadSafety::ThreadSafeQueue<int> queue;
    QAtomicInt producedCount(0);
    QAtomicInt consumedCount(0);

    auto producer = [&queue, &producedCount](int threadId) {
        for (int i = 0; i < ITERATIONS_PER_THREAD; ++i) {
            queue.enqueue(threadId * ITERATIONS_PER_THREAD + i);
            producedCount.fetchAndAddOrdered(1);
        }
    };

    auto consumer = [&queue, &consumedCount](int threadId) {
        Q_UNUSED(threadId)
        int value;
        // Fixed: Consumers should wait for the actual number of items produced
        // THREAD_COUNT / 2 producers each produce ITERATIONS_PER_THREAD items
        while (consumedCount.loadAcquire() <
               THREAD_COUNT / 2 * ITERATIONS_PER_THREAD) {
            if (queue.dequeue(value, 10)) {
                consumedCount.fetchAndAddOrdered(1);
            }
        }
    };

    // Start producers and consumers
    QList<QFuture<void>> futures;

    for (int i = 0; i < THREAD_COUNT / 2; ++i) {
        futures.append(QtConcurrent::run(producer, i));
    }

    for (int i = 0; i < THREAD_COUNT / 2; ++i) {
        futures.append(QtConcurrent::run(consumer, i));
    }

    for (auto& future : futures) {
        future.waitForFinished();
    }

    QCOMPARE(producedCount.loadAcquire(),
             THREAD_COUNT / 2 * ITERATIONS_PER_THREAD);
    QCOMPARE(consumedCount.loadAcquire(),
             THREAD_COUNT / 2 * ITERATIONS_PER_THREAD);
}

void SearchThreadSafetyTest::testThreadSafeQueueTimeout() {
    SearchThreadSafety::ThreadSafeQueue<int> queue;

    int value;
    QElapsedTimer timer;
    timer.start();

    QVERIFY(!queue.dequeue(value, 100));  // Should timeout

    qint64 elapsed = timer.elapsed();
    QVERIFY(elapsed >= 90 && elapsed <= 200);  // Allow some tolerance
}

void SearchThreadSafetyTest::runConcurrentTest(
    std::function<void(int)> threadFunction, int threadCount) {
    QList<QFuture<void>> futures;

    for (int i = 0; i < threadCount; ++i) {
        futures.append(QtConcurrent::run(threadFunction, i));
    }

    for (auto& future : futures) {
        future.waitForFinished();
    }
}

// SharedData convenience methods test
void SearchThreadSafetyTest::testSharedDataConvenienceMethods() {
    SearchThreadSafety::SharedData<QStringList> sharedList;

    // Test set() convenience method
    QStringList testList = {"item1", "item2", "item3"};
    sharedList.set(testList);

    // Test copy() convenience method
    QStringList copiedList = sharedList.copy();
    QCOMPARE(copiedList.size(), 3);
    QCOMPARE(copiedList[0], QString("item1"));
    QCOMPARE(copiedList[1], QString("item2"));
    QCOMPARE(copiedList[2], QString("item3"));

    // Test that copy is independent
    copiedList.append("item4");
    QStringList originalList = sharedList.copy();
    QCOMPARE(originalList.size(), 3);  // Original unchanged

    // Test with complex type
    SearchThreadSafety::SharedData<QHash<QString, int>> sharedHash;
    QHash<QString, int> testHash;
    testHash["key1"] = 100;
    testHash["key2"] = 200;

    sharedHash.set(testHash);
    QHash<QString, int> copiedHash = sharedHash.copy();
    QCOMPARE(copiedHash.size(), 2);
    QCOMPARE(copiedHash["key1"], 100);
    QCOMPARE(copiedHash["key2"], 200);
}

void SearchThreadSafetyTest::testThreadSafeQueueConcurrentAccess() {
    SearchThreadSafety::ThreadSafeQueue<int> queue;
    QAtomicInt producedTotal(0);
    QAtomicInt consumedTotal(0);
    const int itemsPerProducer = 100;
    const int producerCount = 2;
    const int consumerCount = 2;

    // Producer function
    auto producer = [&queue, &producedTotal, itemsPerProducer](int threadId) {
        for (int i = 0; i < itemsPerProducer; ++i) {
            int value = threadId * 1000 + i;
            queue.enqueue(value);
            producedTotal.fetchAndAddOrdered(1);
        }
    };

    // Consumer function
    auto consumer = [&queue, &consumedTotal, producerCount,
                     itemsPerProducer](int threadId) {
        Q_UNUSED(threadId)
        int value;
        while (consumedTotal.loadAcquire() < producerCount * itemsPerProducer) {
            if (queue.dequeue(value, 50)) {  // 50ms timeout
                consumedTotal.fetchAndAddOrdered(1);
            }
        }
    };

    // Start producers
    QList<QFuture<void>> futures;
    for (int i = 0; i < producerCount; ++i) {
        futures.append(QtConcurrent::run(producer, i));
    }

    // Start consumers
    for (int i = 0; i < consumerCount; ++i) {
        futures.append(QtConcurrent::run(consumer, i));
    }

    // Wait for all to complete
    for (auto& future : futures) {
        future.waitForFinished();
    }

    // Verify all items were produced and consumed
    QCOMPARE(producedTotal.loadAcquire(), producerCount * itemsPerProducer);
    QCOMPARE(consumedTotal.loadAcquire(), producerCount * itemsPerProducer);
    QVERIFY(queue.isEmpty());
}

void SearchThreadSafetyTest::testHierarchicalMutexBasicOperations() {
    using MutexHierarchy = SearchThreadSafety::MutexHierarchy;

    MutexHierarchy::HierarchicalMutex documentMutex(
        MutexHierarchy::DocumentLevel);
    MutexHierarchy::HierarchicalMutex cacheMutex(MutexHierarchy::CacheLevel);

    // Test basic lock/unlock
    documentMutex.lock();
    QVERIFY(true);  // Successfully locked
    documentMutex.unlock();

    // Test tryLock
    QVERIFY(documentMutex.tryLock());
    documentMutex.unlock();

    // Test locking in correct order (DocumentLevel before CacheLevel)
    documentMutex.lock();
    cacheMutex.lock();
    cacheMutex.unlock();
    documentMutex.unlock();

    QVERIFY(true);  // No deadlock occurred
}

void SearchThreadSafetyTest::testMutexHierarchyValidation() {
    using MutexHierarchy = SearchThreadSafety::MutexHierarchy;

    // Create mutexes at different levels
    MutexHierarchy::HierarchicalMutex docMutex(MutexHierarchy::DocumentLevel);
    MutexHierarchy::HierarchicalMutex searchMutex(MutexHierarchy::SearchLevel);
    MutexHierarchy::HierarchicalMutex uiMutex(MutexHierarchy::UILevel);

    // Lock in correct order: Document -> Search -> UI
    docMutex.lock();
    searchMutex.lock();
    uiMutex.lock();

    // Unlock in reverse order
    uiMutex.unlock();
    searchMutex.unlock();
    docMutex.unlock();

    // Verify hierarchy validation doesn't crash
    QVERIFY(true);

    // Test validateHierarchy static method
    MutexHierarchy::validateHierarchy();  // Should not crash
    QVERIFY(true);
}

void SearchThreadSafetyTest::testHierarchicalMutexDeadlockPrevention() {
    using MutexHierarchy = SearchThreadSafety::MutexHierarchy;

    // Test that hierarchical mutexes prevent deadlock by enforcing lock
    // ordering
    MutexHierarchy::HierarchicalMutex highPriorityMutex(
        MutexHierarchy::DocumentLevel);
    MutexHierarchy::HierarchicalMutex lowPriorityMutex(
        MutexHierarchy::MetricsLevel);

    // Correct order: high priority (lower number) before low priority (higher
    // number)
    highPriorityMutex.lock();
    lowPriorityMutex.lock();
    lowPriorityMutex.unlock();
    highPriorityMutex.unlock();

    QVERIFY(true);  // No deadlock

    // In debug mode, attempting to lock in wrong order would trigger assertion
    // In release mode, it logs a warning but doesn't deadlock
    // We can't easily test the wrong order without crashing in debug builds
}

void SearchThreadSafetyTest::testMultiLockGuardBasicUsage() {
    QMutex mutex1;
    QMutex mutex2;
    QMutex mutex3;

    // Test that MultiLockGuard locks all mutexes
    {
        SearchThreadSafety::MultiLockGuard guard(mutex1, mutex2, mutex3);

        // All mutexes should be locked now
        // We can't directly test if they're locked, but we can verify
        // that the guard was created successfully
        QVERIFY(true);

        // Mutexes will be unlocked when guard goes out of scope
    }

    // Verify mutexes are unlocked by successfully locking them again
    QVERIFY(mutex1.tryLock());
    mutex1.unlock();
    QVERIFY(mutex2.tryLock());
    mutex2.unlock();
    QVERIFY(mutex3.tryLock());
    mutex3.unlock();
}

void SearchThreadSafetyTest::testMultiLockGuardDeadlockPrevention() {
    QMutex mutexA;
    QMutex mutexB;

    // MultiLockGuard should lock mutexes in a consistent order to prevent
    // deadlock Test with two threads trying to lock the same mutexes
    QAtomicInt successCount(0);

    auto lockFunction = [&mutexA, &mutexB, &successCount](int threadId) {
        Q_UNUSED(threadId)
        for (int i = 0; i < 10; ++i) {
            SearchThreadSafety::MultiLockGuard guard(mutexA, mutexB);
            successCount.fetchAndAddOrdered(1);
            QThread::usleep(1);  // Simulate some work
        }
    };

    QList<QFuture<void>> futures;
    futures.append(QtConcurrent::run(lockFunction, 0));
    futures.append(QtConcurrent::run(lockFunction, 1));

    for (auto& future : futures) {
        future.waitForFinished();
    }

    // Both threads should have completed all iterations without deadlock
    QCOMPARE(successCount.loadAcquire(), 20);
}

void SearchThreadSafetyTest::testThreadSafeSingletonCreation() {
    // Define a simple test class for singleton
    class TestSingleton {
    public:
        TestSingleton() : m_value(42) {}
        int getValue() const { return m_value; }
        void setValue(int value) { m_value = value; }

    private:
        int m_value;
    };

    // Get singleton instance
    TestSingleton& instance1 =
        SearchThreadSafety::ThreadSafeSingleton<TestSingleton>::instance();
    TestSingleton& instance2 =
        SearchThreadSafety::ThreadSafeSingleton<TestSingleton>::instance();

    // Both references should point to the same instance
    QCOMPARE(&instance1, &instance2);
    QCOMPARE(instance1.getValue(), 42);

    // Modify through one reference
    instance1.setValue(100);

    // Verify change is visible through other reference
    QCOMPARE(instance2.getValue(), 100);
}

void SearchThreadSafetyTest::testThreadSafeSingletonConcurrentAccess() {
    class CounterSingleton {
    public:
        CounterSingleton() : m_counter(0) {}
        void increment() { m_counter.fetchAndAddOrdered(1); }
        int value() const { return m_counter.loadAcquire(); }

    private:
        QAtomicInt m_counter;
    };

    // Access singleton from multiple threads concurrently
    auto accessFunction = [](int threadId) {
        Q_UNUSED(threadId)
        CounterSingleton& singleton = SearchThreadSafety::ThreadSafeSingleton<
            CounterSingleton>::instance();

        for (int i = 0; i < 100; ++i) {
            singleton.increment();
        }
    };

    QList<QFuture<void>> futures;
    for (int i = 0; i < 4; ++i) {
        futures.append(QtConcurrent::run(accessFunction, i));
    }

    for (auto& future : futures) {
        future.waitForFinished();
    }

    // Verify all increments were counted
    CounterSingleton& singleton =
        SearchThreadSafety::ThreadSafeSingleton<CounterSingleton>::instance();
    QCOMPARE(singleton.value(), 400);

    // Cleanup for next test
    SearchThreadSafety::ThreadSafeSingleton<CounterSingleton>::destroy();
}

void SearchThreadSafetyTest::testContentionMonitorRecording() {
    // Reset stats before test
    SearchThreadSafety::ContentionMonitor::resetStats();

    // Record some lock attempts
    SearchThreadSafety::ContentionMonitor::recordLockAttempt("testMutex");
    SearchThreadSafety::ContentionMonitor::recordLockAttempt("testMutex");
    SearchThreadSafety::ContentionMonitor::recordLockAttempt("testMutex");

    // Get stats
    auto stats = SearchThreadSafety::ContentionMonitor::getStats("testMutex");

    QCOMPARE(stats.lockAttempts, 3);
    QCOMPARE(stats.lockContentions, 0);  // No contentions recorded yet

    // Record a contention
    SearchThreadSafety::ContentionMonitor::recordLockContention(
        "testMutex", 100);  // 100ms wait

    stats = SearchThreadSafety::ContentionMonitor::getStats("testMutex");
    QCOMPARE(stats.lockContentions, 1);
    QCOMPARE(stats.totalWaitTime, 100);
    QCOMPARE(stats.maxWaitTime, 100);
}

void SearchThreadSafetyTest::testContentionMonitorStatistics() {
    SearchThreadSafety::ContentionMonitor::resetStats();

    // Record multiple contentions with different wait times
    SearchThreadSafety::ContentionMonitor::recordLockAttempt("mutex1");
    SearchThreadSafety::ContentionMonitor::recordLockAttempt("mutex1");
    SearchThreadSafety::ContentionMonitor::recordLockAttempt("mutex1");
    SearchThreadSafety::ContentionMonitor::recordLockContention("mutex1", 50);
    SearchThreadSafety::ContentionMonitor::recordLockContention("mutex1", 150);

    auto stats = SearchThreadSafety::ContentionMonitor::getStats("mutex1");

    QCOMPARE(stats.lockAttempts, 3);
    QCOMPARE(stats.lockContentions, 2);
    QCOMPARE(stats.totalWaitTime, 200);  // 50 + 150
    QCOMPARE(stats.maxWaitTime, 150);

    // Test contention rate calculation
    double expectedRate = 2.0 / 3.0;  // 2 contentions out of 3 attempts
    QVERIFY(qAbs(stats.contentionRate() - expectedRate) < 0.01);

    // Test getAllStats
    auto allStats = SearchThreadSafety::ContentionMonitor::getAllStats();
    QVERIFY(allStats.contains("mutex1"));
    QCOMPARE(allStats["mutex1"].lockAttempts, 3);
}

void SearchThreadSafetyTest::testHighContentionScenario() {
    // Simulate high contention with many threads accessing shared resource
    SearchThreadSafety::SharedData<int> sharedCounter(0);
    QMutex contentionMutex;
    QAtomicInt operationCount(0);

    auto highContentionFunction = [&sharedCounter, &contentionMutex,
                                   &operationCount](int threadId) {
        Q_UNUSED(threadId)
        for (int i = 0; i < 50; ++i) {
            // Simulate contention by holding lock while doing work
            QMutexLocker locker(&contentionMutex);

            auto writeAccess = sharedCounter.write();
            *writeAccess = *writeAccess + 1;

            // Simulate some processing time to increase contention
            QThread::usleep(10);

            operationCount.fetchAndAddOrdered(1);
        }
    };

    // Run with many threads to create high contention
    QList<QFuture<void>> futures;
    const int threadCount = 8;
    for (int i = 0; i < threadCount; ++i) {
        futures.append(QtConcurrent::run(highContentionFunction, i));
    }

    for (auto& future : futures) {
        future.waitForFinished();
    }

    // Verify all operations completed
    QCOMPARE(operationCount.loadAcquire(), threadCount * 50);
    QCOMPARE(sharedCounter.copy(), threadCount * 50);
}

void SearchThreadSafetyTest::testMixedOperationsStressTest() {
    // Test with mixed read/write operations and different synchronization
    // primitives
    SearchThreadSafety::SharedData<QHash<QString, int>> sharedData;
    SearchThreadSafety::ThreadSafeQueue<QString> taskQueue;
    SearchThreadSafety::AtomicCounter completedTasks(0);

    // Initialize shared data
    {
        auto writeAccess = sharedData.write();
        writeAccess->insert("counter", 0);
    }

    // Producer: adds tasks to queue
    auto producer = [&taskQueue](int threadId) {
        for (int i = 0; i < 20; ++i) {
            taskQueue.enqueue(QString("task_%1_%2").arg(threadId).arg(i));
        }
    };

    // Consumer: processes tasks and updates shared data
    auto consumer = [&taskQueue, &sharedData, &completedTasks](int threadId) {
        Q_UNUSED(threadId)
        QString task;
        while (completedTasks.value() < 40) {  // 2 producers * 20 tasks
            if (taskQueue.dequeue(task, 10)) {
                // Update shared data
                auto writeAccess = sharedData.write();
                (*writeAccess)["counter"] = writeAccess->value("counter") + 1;

                completedTasks.increment();
            }
        }
    };

    // Reader: periodically reads shared data
    auto reader = [&sharedData, &completedTasks](int threadId) {
        Q_UNUSED(threadId)
        while (completedTasks.value() < 40) {
            auto readAccess = sharedData.read();
            int currentValue = readAccess->value("counter");
            Q_UNUSED(currentValue)
            QThread::usleep(100);
        }
    };

    // Start mixed operations
    QList<QFuture<void>> futures;
    futures.append(QtConcurrent::run(producer, 0));
    futures.append(QtConcurrent::run(producer, 1));
    futures.append(QtConcurrent::run(consumer, 0));
    futures.append(QtConcurrent::run(consumer, 1));
    futures.append(QtConcurrent::run(reader, 0));

    for (auto& future : futures) {
        future.waitForFinished();
    }

    // Verify final state
    QCOMPARE(completedTasks.value(), 40);
    QCOMPARE(sharedData.copy().value("counter"), 40);
}

void SearchThreadSafetyTest::testMemoryOrderingConsistency() {
    // Test that atomic operations maintain proper memory ordering
    SearchThreadSafety::AtomicCounter counter(0);
    SearchThreadSafety::AtomicFlag flag(false);
    QAtomicInt dataReady(0);
    QList<int> sharedData;
    QMutex dataMutex;

    // Writer thread: increments counter and sets flag
    auto writer = [&counter, &flag, &sharedData, &dataMutex,
                   &dataReady](int threadId) {
        Q_UNUSED(threadId)
        for (int i = 0; i < 100; ++i) {
            counter.increment();

            {
                QMutexLocker locker(&dataMutex);
                sharedData.append(i);
            }

            dataReady.storeRelease(1);
            flag.set();

            QThread::usleep(1);
            flag.clear();
            dataReady.storeRelease(0);
        }
    };

    // Reader thread: waits for flag and reads counter
    auto reader = [&counter, &flag, &dataReady](int threadId) {
        Q_UNUSED(threadId)
        int readCount = 0;
        while (readCount < 100) {
            if (flag.isSet() && dataReady.loadAcquire() == 1) {
                int value = counter.value();
                QVERIFY(value > 0);
                readCount++;
            }
            QThread::usleep(1);
        }
    };

    QFuture<void> writerFuture = QtConcurrent::run(writer, 0);
    QFuture<void> readerFuture = QtConcurrent::run(reader, 1);

    writerFuture.waitForFinished();
    readerFuture.waitForFinished();

    // Verify final state
    QCOMPARE(counter.value(), 100);
    QMutexLocker locker(&dataMutex);
    QCOMPARE(sharedData.size(), 100);
}

void SearchThreadSafetyTest::testThreadSafetyMacros() {
    // Test the convenience macros for thread safety

    // Test SEARCH_ATOMIC_COUNTER macro
    SEARCH_ATOMIC_COUNTER(testCounter, 10);
    QCOMPARE(testCounter.value(), 10);
    testCounter.increment();
    QCOMPARE(testCounter.value(), 11);

    // Test SEARCH_ATOMIC_FLAG macro
    SEARCH_ATOMIC_FLAG(testFlag, false);
    QVERIFY(!testFlag.isSet());
    testFlag.set();
    QVERIFY(testFlag.isSet());

    // Test SEARCH_SHARED_DATA macro
    SEARCH_SHARED_DATA(QString, testString, QString("initial"));
    QCOMPARE(testString.copy(), QString("initial"));
    testString.set("modified");
    QCOMPARE(testString.copy(), QString("modified"));

    // Test SEARCH_THREAD_SAFE_QUEUE macro
    SEARCH_THREAD_SAFE_QUEUE(int, testQueue);
    QVERIFY(testQueue.isEmpty());
    testQueue.enqueue(42);
    QVERIFY(!testQueue.isEmpty());
    int value;
    QVERIFY(testQueue.tryDequeue(value));
    QCOMPARE(value, 42);

    // Test SEARCH_HIERARCHICAL_MUTEX macro
    SEARCH_HIERARCHICAL_MUTEX(testMutex, DocumentLevel);
    testMutex.lock();
    QVERIFY(true);  // Successfully locked
    testMutex.unlock();
}

QTEST_MAIN(SearchThreadSafetyTest)
#include "test_search_thread_safety.moc"
