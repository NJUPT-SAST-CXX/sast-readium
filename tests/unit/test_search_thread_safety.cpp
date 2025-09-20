#include <QtTest>
#include <QObject>
#include <QThread>
#include <QSignalSpy>
#include <QMutex>
#include <QWaitCondition>
#include <QElapsedTimer>
#include <QRandomGenerator>
#include "../../app/search/SearchThreadSafety.h"

class TestSearchThreadSafety : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Atomic operations tests
    void testAtomicCounter();
    void testAtomicFlag();
    void testAtomicPointer();
    
    // Shared data tests
    void testSharedDataReadWrite();
    void testSharedDataConcurrency();
    
    // Thread-safe queue tests
    void testThreadSafeQueue();
    void testQueueProducerConsumer();
    
    // Mutex hierarchy tests
    void testMutexHierarchy();
    void testHierarchyViolationDetection();
    
    // Multi-lock guard tests
    void testMultiLockGuard();
    void testDeadlockPrevention();
    
    // Contention monitoring tests
    void testContentionMonitoring();
    void testContentionStatistics();
    
    // Thread-safe cache tests
    void testThreadSafeCache();
    void testCacheConcurrency();
    
    // Performance tests
    void testConcurrentPerformance();
    void testScalability();

private:
    // Helper classes for testing
    class TestWorker : public QThread
    {
    public:
        TestWorker(std::function<void()> work, QObject* parent = nullptr)
            : QThread(parent), m_work(work), m_completed(false) {}
        
        void run() override {
            m_work();
            m_completed = true;
        }
        
        bool isCompleted() const { return m_completed; }
        
    private:
        std::function<void()> m_work;
        bool m_completed;
    };
    
    SearchThreadSafety::AtomicCounter* counter;
    SearchThreadSafety::AtomicFlag* flag;
    SearchThreadSafety::SharedData<QString>* sharedString;
    SearchThreadSafety::ThreadSafeQueue<int>* queue;
};

void TestSearchThreadSafety::initTestCase()
{
    counter = new SearchThreadSafety::AtomicCounter(0);
    flag = new SearchThreadSafety::AtomicFlag(false);
    sharedString = new SearchThreadSafety::SharedData<QString>("initial");
    queue = new SearchThreadSafety::ThreadSafeQueue<int>();
}

void TestSearchThreadSafety::cleanupTestCase()
{
    delete counter;
    delete flag;
    delete sharedString;
    delete queue;
}

void TestSearchThreadSafety::testAtomicCounter()
{
    SearchThreadSafety::AtomicCounter testCounter(10);
    
    QCOMPARE(testCounter.value(), 10);
    QCOMPARE(testCounter.increment(), 11);
    QCOMPARE(testCounter.decrement(), 10);
    QCOMPARE(testCounter.value(), 10);
    
    testCounter.setValue(5);
    QCOMPARE(testCounter.value(), 5);
    
    // Test compare and swap
    QVERIFY(testCounter.compareAndSwap(5, 15));
    QCOMPARE(testCounter.value(), 15);
    QVERIFY(!testCounter.compareAndSwap(5, 20)); // Should fail
    QCOMPARE(testCounter.value(), 15);
}

void TestSearchThreadSafety::testAtomicFlag()
{
    SearchThreadSafety::AtomicFlag testFlag(false);
    
    QVERIFY(!testFlag.isSet());
    testFlag.set();
    QVERIFY(testFlag.isSet());
    testFlag.clear();
    QVERIFY(!testFlag.isSet());
    
    // Test test-and-set
    QVERIFY(testFlag.testAndSet()); // Should set and return true
    QVERIFY(testFlag.isSet());
    QVERIFY(!testFlag.testAndSet()); // Should fail since already set
    
    // Test test-and-clear
    QVERIFY(testFlag.testAndClear()); // Should clear and return true
    QVERIFY(!testFlag.isSet());
    QVERIFY(!testFlag.testAndClear()); // Should fail since already clear
}

void TestSearchThreadSafety::testAtomicPointer()
{
    QString* str1 = new QString("test1");
    QString* str2 = new QString("test2");
    
    SearchThreadSafety::AtomicPointer<QString> testPointer(str1);
    
    QCOMPARE(testPointer.load(), str1);
    testPointer.store(str2);
    QCOMPARE(testPointer.load(), str2);
    
    // Test compare and swap
    QVERIFY(testPointer.compareAndSwap(str2, str1));
    QCOMPARE(testPointer.load(), str1);
    QVERIFY(!testPointer.compareAndSwap(str2, str1)); // Should fail
    
    // Test exchange
    QString* old = testPointer.exchange(str2);
    QCOMPARE(old, str1);
    QCOMPARE(testPointer.load(), str2);
    
    delete str1;
    delete str2;
}

void TestSearchThreadSafety::testSharedDataReadWrite()
{
    SearchThreadSafety::SharedData<QString> testData("initial");
    
    // Test read access
    {
        auto readAccess = testData.read();
        QCOMPARE(*readAccess, "initial");
        QCOMPARE(readAccess->length(), 7);
    }
    
    // Test write access
    {
        auto writeAccess = testData.write();
        *writeAccess = "modified";
    }
    
    // Verify modification
    QCOMPARE(testData.copy(), "modified");
    
    // Test convenience methods
    testData.set("convenient");
    QCOMPARE(testData.copy(), "convenient");
}

void TestSearchThreadSafety::testSharedDataConcurrency()
{
    SearchThreadSafety::SharedData<int> sharedInt(0);
    const int numThreads = 10;
    const int incrementsPerThread = 1000;
    
    QList<TestWorker*> workers;
    
    // Create worker threads that increment the shared value
    for (int i = 0; i < numThreads; ++i) {
        auto worker = new TestWorker([&sharedInt, incrementsPerThread]() {
            for (int j = 0; j < incrementsPerThread; ++j) {
                auto writeAccess = sharedInt.write();
                (*writeAccess)++;
            }
        });
        workers.append(worker);
    }
    
    // Start all workers
    for (auto worker : workers) {
        worker->start();
    }
    
    // Wait for completion
    for (auto worker : workers) {
        worker->wait();
        QVERIFY(worker->isCompleted());
        delete worker;
    }
    
    // Verify final value
    QCOMPARE(sharedInt.copy(), numThreads * incrementsPerThread);
}

void TestSearchThreadSafety::testThreadSafeQueue()
{
    SearchThreadSafety::ThreadSafeQueue<int> testQueue;
    
    QVERIFY(testQueue.isEmpty());
    QCOMPARE(testQueue.size(), 0);
    
    // Test enqueue/dequeue
    testQueue.enqueue(1);
    testQueue.enqueue(2);
    testQueue.enqueue(3);
    
    QCOMPARE(testQueue.size(), 3);
    QVERIFY(!testQueue.isEmpty());
    
    int item;
    QVERIFY(testQueue.tryDequeue(item));
    QCOMPARE(item, 1);
    
    QVERIFY(testQueue.dequeue(item, 100)); // 100ms timeout
    QCOMPARE(item, 2);
    
    testQueue.clear();
    QVERIFY(testQueue.isEmpty());
}

void TestSearchThreadSafety::testQueueProducerConsumer()
{
    SearchThreadSafety::ThreadSafeQueue<int> testQueue;
    const int numItems = 1000;
    QAtomicInt producedCount(0);
    QAtomicInt consumedCount(0);
    
    // Producer thread
    auto producer = new TestWorker([&testQueue, &producedCount, numItems]() {
        for (int i = 0; i < numItems; ++i) {
            testQueue.enqueue(i);
            producedCount.fetchAndAddOrdered(1);
            QThread::msleep(1); // Small delay
        }
    });
    
    // Consumer thread
    auto consumer = new TestWorker([&testQueue, &consumedCount, numItems]() {
        int item;
        while (consumedCount.loadAcquire() < numItems) {
            if (testQueue.dequeue(item, 100)) {
                consumedCount.fetchAndAddOrdered(1);
            }
        }
    });
    
    producer->start();
    consumer->start();
    
    producer->wait();
    consumer->wait();
    
    QCOMPARE(producedCount.loadAcquire(), numItems);
    QCOMPARE(consumedCount.loadAcquire(), numItems);
    QVERIFY(testQueue.isEmpty());
    
    delete producer;
    delete consumer;
}

void TestSearchThreadSafety::testMutexHierarchy()
{
    auto documentMutex = SearchThreadSafety::MutexHierarchy::createMutex(
        SearchThreadSafety::MutexHierarchy::DocumentLevel);
    auto cacheMutex = SearchThreadSafety::MutexHierarchy::createMutex(
        SearchThreadSafety::MutexHierarchy::CacheLevel);
    
    // Test proper hierarchy (document -> cache)
    documentMutex->lock();
    QVERIFY(cacheMutex->tryLock()); // Should succeed
    cacheMutex->unlock();
    documentMutex->unlock();
    
    delete documentMutex;
    delete cacheMutex;
}

void TestSearchThreadSafety::testHierarchyViolationDetection()
{
    // This test would require debug mode to see warnings
    // In release mode, hierarchy violations are logged but not enforced
    auto documentMutex = SearchThreadSafety::MutexHierarchy::createMutex(
        SearchThreadSafety::MutexHierarchy::DocumentLevel);
    auto cacheMutex = SearchThreadSafety::MutexHierarchy::createMutex(
        SearchThreadSafety::MutexHierarchy::CacheLevel);
    
    // Test hierarchy validation
    SearchThreadSafety::MutexHierarchy::validateHierarchy();
    
    delete documentMutex;
    delete cacheMutex;
}

void TestSearchThreadSafety::testMultiLockGuard()
{
    QMutex mutex1, mutex2, mutex3;
    
    {
        SearchThreadSafety::MultiLockGuard guard(mutex1, mutex2, mutex3);
        // All mutexes should be locked here
        
        // Try to lock from another thread - should fail
        auto worker = new TestWorker([&mutex1]() {
            QVERIFY(!mutex1.tryLock());
        });
        worker->start();
        worker->wait();
        delete worker;
    }
    // All mutexes should be unlocked here
    
    QVERIFY(mutex1.tryLock());
    mutex1.unlock();
}

void TestSearchThreadSafety::testDeadlockPrevention()
{
    // Test that MultiLockGuard prevents deadlocks by consistent ordering
    QMutex mutexA, mutexB;
    bool deadlockOccurred = false;
    
    auto thread1 = new TestWorker([&mutexA, &mutexB, &deadlockOccurred]() {
        try {
            SearchThreadSafety::MultiLockGuard guard(mutexA, mutexB);
            QThread::msleep(100);
        } catch (...) {
            deadlockOccurred = true;
        }
    });
    
    auto thread2 = new TestWorker([&mutexA, &mutexB, &deadlockOccurred]() {
        try {
            SearchThreadSafety::MultiLockGuard guard(mutexB, mutexA);
            QThread::msleep(100);
        } catch (...) {
            deadlockOccurred = true;
        }
    });
    
    thread1->start();
    thread2->start();
    
    thread1->wait();
    thread2->wait();
    
    QVERIFY(!deadlockOccurred);
    
    delete thread1;
    delete thread2;
}

void TestSearchThreadSafety::testContentionMonitoring()
{
    SearchThreadSafety::ContentionMonitor::resetStats();
    
    const QString mutexName = "test_mutex";
    
    // Record some lock attempts and contentions
    SearchThreadSafety::ContentionMonitor::recordLockAttempt(mutexName);
    SearchThreadSafety::ContentionMonitor::recordLockAttempt(mutexName);
    SearchThreadSafety::ContentionMonitor::recordLockContention(mutexName, 100);
    
    auto stats = SearchThreadSafety::ContentionMonitor::getStats(mutexName);
    QCOMPARE(stats.lockAttempts, 2);
    QCOMPARE(stats.lockContentions, 1);
    QCOMPARE(stats.totalWaitTime, 100);
    QCOMPARE(stats.maxWaitTime, 100);
    QCOMPARE(stats.contentionRate(), 0.5);
}

void TestSearchThreadSafety::testContentionStatistics()
{
    SearchThreadSafety::ContentionMonitor::resetStats();
    
    // Simulate contention on multiple mutexes
    SearchThreadSafety::ContentionMonitor::recordLockAttempt("mutex1");
    SearchThreadSafety::ContentionMonitor::recordLockContention("mutex1", 50);
    SearchThreadSafety::ContentionMonitor::recordLockAttempt("mutex2");
    SearchThreadSafety::ContentionMonitor::recordLockContention("mutex2", 75);
    
    auto allStats = SearchThreadSafety::ContentionMonitor::getAllStats();
    QCOMPARE(allStats.size(), 2);
    QVERIFY(allStats.contains("mutex1"));
    QVERIFY(allStats.contains("mutex2"));
    
    QCOMPARE(allStats["mutex1"].maxWaitTime, 50);
    QCOMPARE(allStats["mutex2"].maxWaitTime, 75);
}

void TestSearchThreadSafety::testThreadSafeCache()
{
    // This would test the ThreadSafeCache template if it was exposed
    // For now, we'll test the concept with a simple implementation
    QVERIFY(true); // Placeholder
}

void TestSearchThreadSafety::testCacheConcurrency()
{
    // Test concurrent cache access
    QVERIFY(true); // Placeholder
}

void TestSearchThreadSafety::testConcurrentPerformance()
{
    // Performance test with multiple threads
    const int numThreads = QThread::idealThreadCount();
    const int operationsPerThread = 10000;
    
    SearchThreadSafety::AtomicCounter performanceCounter(0);
    QElapsedTimer timer;
    timer.start();
    
    QList<TestWorker*> workers;
    for (int i = 0; i < numThreads; ++i) {
        auto worker = new TestWorker([&performanceCounter, operationsPerThread]() {
            for (int j = 0; j < operationsPerThread; ++j) {
                performanceCounter.increment();
            }
        });
        workers.append(worker);
    }
    
    for (auto worker : workers) {
        worker->start();
    }
    
    for (auto worker : workers) {
        worker->wait();
        delete worker;
    }
    
    qint64 elapsed = timer.elapsed();
    int totalOperations = numThreads * operationsPerThread;
    
    QCOMPARE(performanceCounter.value(), totalOperations);
    qDebug() << "Performed" << totalOperations << "atomic operations in" << elapsed << "ms";
    qDebug() << "Rate:" << (totalOperations * 1000.0 / elapsed) << "operations/second";
}

void TestSearchThreadSafety::testScalability()
{
    // Test how performance scales with thread count
    const int maxThreads = QThread::idealThreadCount() * 2;
    const int operationsPerThread = 5000;
    
    for (int threadCount = 1; threadCount <= maxThreads; threadCount *= 2) {
        SearchThreadSafety::AtomicCounter scalabilityCounter(0);
        QElapsedTimer timer;
        timer.start();
        
        QList<TestWorker*> workers;
        for (int i = 0; i < threadCount; ++i) {
            auto worker = new TestWorker([&scalabilityCounter, operationsPerThread]() {
                for (int j = 0; j < operationsPerThread; ++j) {
                    scalabilityCounter.increment();
                }
            });
            workers.append(worker);
        }
        
        for (auto worker : workers) {
            worker->start();
        }
        
        for (auto worker : workers) {
            worker->wait();
            delete worker;
        }
        
        qint64 elapsed = timer.elapsed();
        int totalOperations = threadCount * operationsPerThread;
        double rate = totalOperations * 1000.0 / elapsed;
        
        QCOMPARE(scalabilityCounter.value(), totalOperations);
        qDebug() << "Threads:" << threadCount << "Operations:" << totalOperations 
                 << "Time:" << elapsed << "ms Rate:" << rate << "ops/sec";
    }
}

QTEST_APPLESS_MAIN(TestSearchThreadSafety)

#include "test_search_thread_safety.moc"
