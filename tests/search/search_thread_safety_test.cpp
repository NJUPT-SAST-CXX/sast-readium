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
        while (consumedCount.loadAcquire() <
               THREAD_COUNT * ITERATIONS_PER_THREAD) {
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

QTEST_MAIN(SearchThreadSafetyTest)
#include "search_thread_safety_test.moc"
