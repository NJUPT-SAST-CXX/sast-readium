#pragma once

#include <QAtomicInt>
#include <QAtomicPointer>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QQueue>
#include <QReadLocker>
#include <QReadWriteLock>
#include <QThread>
#include <QTimer>
#include <QWaitCondition>
#include <QWriteLocker>
#include <atomic>
#include <memory>

/**
 * Comprehensive thread safety framework for search operations
 * Provides thread-safe access patterns, atomic operations, and synchronization
 * primitives
 */
class SearchThreadSafety {
public:
    /**
     * Thread-safe counter with atomic operations
     */
    class AtomicCounter {
    public:
        AtomicCounter(int initialValue = 0) : m_value(initialValue) {}

        int increment() { return m_value.fetchAndAddOrdered(1) + 1; }
        int decrement() { return m_value.fetchAndSubOrdered(1) - 1; }
        int value() const { return m_value.loadAcquire(); }
        void setValue(int value) { m_value.storeRelease(value); }

        bool compareAndSwap(int expected, int newValue) {
            return m_value.testAndSetOrdered(expected, newValue);
        }

    private:
        QAtomicInt m_value;
    };

    /**
     * Thread-safe flag with atomic operations
     */
    class AtomicFlag {
    public:
        AtomicFlag(bool initialValue = false) : m_value(initialValue ? 1 : 0) {}

        bool isSet() const { return m_value.loadAcquire() != 0; }
        void set() { m_value.storeRelease(1); }
        void clear() { m_value.storeRelease(0); }

        bool testAndSet() { return m_value.testAndSetOrdered(0, 1); }
        bool testAndClear() { return m_value.testAndSetOrdered(1, 0); }

    private:
        QAtomicInt m_value;
    };

    /**
     * Thread-safe pointer with atomic operations
     */
    template <typename T>
    class AtomicPointer {
    public:
        AtomicPointer(T* initialValue = nullptr) : m_pointer(initialValue) {}

        T* load() const { return m_pointer.loadAcquire(); }
        void store(T* value) { m_pointer.storeRelease(value); }

        bool compareAndSwap(T* expected, T* newValue) {
            return m_pointer.testAndSetOrdered(expected, newValue);
        }

        T* exchange(T* newValue) {
            T* old = m_pointer.loadAcquire();
            while (!m_pointer.testAndSetOrdered(old, newValue)) {
                old = m_pointer.loadAcquire();
            }
            return old;
        }

    private:
        QAtomicPointer<T> m_pointer;
    };

    /**
     * Read-Write lock wrapper for shared data access
     */
    template <typename T>
    class SharedData {
    public:
        explicit SharedData(const T& initialValue = T())
            : m_data(initialValue) {}

        // Read access (multiple readers allowed)
        class ReadAccess {
        public:
            explicit ReadAccess(SharedData* parent)
                : m_locker(&parent->m_lock), m_data(&parent->m_data) {}
            const T& operator*() const { return *m_data; }
            const T* operator->() const { return m_data; }
            const T* get() const { return m_data; }

        private:
            QReadLocker m_locker;
            const T* m_data;
        };

        // Write access (exclusive access)
        class WriteAccess {
        public:
            explicit WriteAccess(SharedData* parent)
                : m_locker(&parent->m_lock), m_data(&parent->m_data) {}
            T& operator*() { return *m_data; }
            T* operator->() { return m_data; }
            T* get() { return m_data; }

        private:
            QWriteLocker m_locker;
            T* m_data;
        };

        ReadAccess read() { return ReadAccess(this); }
        WriteAccess write() { return WriteAccess(this); }

        // Convenience methods for simple operations
        T copy() const {
            QReadLocker locker(&m_lock);
            return m_data;
        }

        void set(const T& value) {
            QWriteLocker locker(&m_lock);
            m_data = value;
        }

    private:
        mutable QReadWriteLock m_lock;
        T m_data;
    };

    /**
     * Thread-safe queue for producer-consumer patterns
     */
    template <typename T>
    class ThreadSafeQueue {
    public:
        void enqueue(const T& item) {
            QMutexLocker locker(&m_mutex);
            m_queue.enqueue(item);
            m_condition.wakeOne();
        }

        bool dequeue(T& item, int timeoutMs = -1) {
            QMutexLocker locker(&m_mutex);

            if (m_queue.isEmpty()) {
                if (timeoutMs < 0) {
                    m_condition.wait(&m_mutex);
                } else {
                    if (!m_condition.wait(&m_mutex, timeoutMs)) {
                        return false;  // Timeout
                    }
                }
            }

            if (!m_queue.isEmpty()) {
                item = m_queue.dequeue();
                return true;
            }

            return false;
        }

        bool tryDequeue(T& item) {
            QMutexLocker locker(&m_mutex);
            if (!m_queue.isEmpty()) {
                item = m_queue.dequeue();
                return true;
            }
            return false;
        }

        int size() const {
            QMutexLocker locker(&m_mutex);
            return m_queue.size();
        }

        bool isEmpty() const {
            QMutexLocker locker(&m_mutex);
            return m_queue.isEmpty();
        }

        void clear() {
            QMutexLocker locker(&m_mutex);
            m_queue.clear();
            m_condition.wakeAll();
        }

    private:
        mutable QMutex m_mutex;
        QWaitCondition m_condition;
        QQueue<T> m_queue;
    };

    /**
     * Hierarchical mutex system for consistent lock ordering
     */
    class MutexHierarchy {
    public:
        enum Level {
            DocumentLevel = 1,  // Highest priority - document access
            CacheLevel = 2,     // Cache operations
            SearchLevel = 3,    // Search operations
            UILevel = 4,        // UI updates
            MetricsLevel = 5    // Lowest priority - metrics and logging
        };

        class HierarchicalMutex {
        public:
            explicit HierarchicalMutex(Level level) : m_level(level) {}

            void lock() {
                checkHierarchy();
                m_mutex.lock();
                setCurrentLevel(m_level);
            }

            void unlock() {
                clearCurrentLevel();
                m_mutex.unlock();
            }

            bool tryLock() {
                if (!canAcquire())
                    return false;
                if (m_mutex.tryLock()) {
                    setCurrentLevel(m_level);
                    return true;
                }
                return false;
            }

            QMutex& mutex() { return m_mutex; }
            Level level() const { return m_level; }

        private:
            QMutex m_mutex;
            Level m_level;

            void checkHierarchy();
            bool canAcquire() const;
            void setCurrentLevel(Level level);
            void clearCurrentLevel();
        };

        static HierarchicalMutex* createMutex(Level level);
        static void validateHierarchy();

    private:
        static thread_local Level s_currentLevel;
        static QHash<QThread*, Level> s_threadLevels;
        static QMutex s_hierarchyMutex;
    };

    /**
     * RAII lock guard for multiple mutexes with deadlock prevention
     */
    class MultiLockGuard {
    public:
        template <typename... Mutexes>
        explicit MultiLockGuard(Mutexes&... mutexes) {
            lockAll(mutexes...);
        }

        ~MultiLockGuard() {
            // Unlock in reverse order
            for (auto it = m_lockers.rbegin(); it != m_lockers.rend(); ++it) {
                (*it)->unlock();
            }
        }

    private:
        std::vector<std::unique_ptr<QMutexLocker<QMutex>>> m_lockers;

        template <typename Mutex>
        void lockAll(Mutex& mutex) {
            m_lockers.emplace_back(
                std::make_unique<QMutexLocker<QMutex>>(&mutex));
        }

        template <typename Mutex, typename... Rest>
        void lockAll(Mutex& mutex, Rest&... rest) {
            m_lockers.emplace_back(
                std::make_unique<QMutexLocker<QMutex>>(&mutex));
            lockAll(rest...);
        }
    };

    /**
     * Thread-safe singleton pattern
     */
    template <typename T>
    class ThreadSafeSingleton {
    public:
        static T& instance() {
            static std::once_flag flag;
            std::call_once(flag, []() { s_instance.reset(new T()); });
            return *s_instance;
        }

        static void destroy() { s_instance.reset(); }

    private:
        static std::unique_ptr<T> s_instance;
    };

    /**
     * Performance monitoring for thread contention
     */
    class ContentionMonitor {
    public:
        struct ContentionStats {
            int lockAttempts = 0;
            int lockContentions = 0;
            qint64 totalWaitTime = 0;
            qint64 maxWaitTime = 0;
            double contentionRate() const {
                return lockAttempts > 0 ? (double)lockContentions / lockAttempts
                                        : 0.0;
            }
        };

        static void recordLockAttempt(const QString& mutexName);
        static void recordLockContention(const QString& mutexName,
                                         qint64 waitTime);
        static ContentionStats getStats(const QString& mutexName);
        static QHash<QString, ContentionStats> getAllStats();
        static void resetStats();

    private:
        static QHash<QString, ContentionStats> s_stats;
        static QMutex s_statsMutex;
    };
};

/**
 * Convenience macros for thread safety
 */
#define SEARCH_ATOMIC_COUNTER(name, initial) \
    SearchThreadSafety::AtomicCounter name(initial)

#define SEARCH_ATOMIC_FLAG(name, initial) \
    SearchThreadSafety::AtomicFlag name(initial)

#define SEARCH_SHARED_DATA(type, name, initial) \
    SearchThreadSafety::SharedData<type> name(initial)

#define SEARCH_THREAD_SAFE_QUEUE(type, name) \
    SearchThreadSafety::ThreadSafeQueue<type> name

#define SEARCH_HIERARCHICAL_MUTEX(name, level)                  \
    SearchThreadSafety::MutexHierarchy::HierarchicalMutex name( \
        SearchThreadSafety::MutexHierarchy::level)

#define SEARCH_MULTI_LOCK(...) \
    SearchThreadSafety::MultiLockGuard _guard(__VA_ARGS__)

#define SEARCH_READ_LOCK(sharedData) auto _readLock = sharedData.read()

#define SEARCH_WRITE_LOCK(sharedData) auto _writeLock = sharedData.write()

// Template implementation
template <typename T>
std::unique_ptr<T> SearchThreadSafety::ThreadSafeSingleton<T>::s_instance;
