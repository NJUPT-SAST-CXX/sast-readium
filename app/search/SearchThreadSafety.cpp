#include "SearchThreadSafety.h"
#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <stdexcept>

// Static member definitions
thread_local SearchThreadSafety::MutexHierarchy::Level
    SearchThreadSafety::MutexHierarchy::s_currentLevel =
        SearchThreadSafety::MutexHierarchy::MetricsLevel;
QHash<QThread*, SearchThreadSafety::MutexHierarchy::Level>
    SearchThreadSafety::MutexHierarchy::s_threadLevels;
QMutex SearchThreadSafety::MutexHierarchy::s_hierarchyMutex;

QHash<QString, SearchThreadSafety::ContentionMonitor::ContentionStats>
    SearchThreadSafety::ContentionMonitor::s_stats;
QMutex SearchThreadSafety::ContentionMonitor::s_statsMutex;

// MutexHierarchy implementation
void SearchThreadSafety::MutexHierarchy::HierarchicalMutex::checkHierarchy() {
    QMutexLocker locker(&s_hierarchyMutex);
    QThread* currentThread = QThread::currentThread();

    if (s_threadLevels.contains(currentThread)) {
        Level currentLevel = s_threadLevels[currentThread];
        if (m_level >= currentLevel) {
            qWarning() << "Potential deadlock detected: Attempting to acquire "
                          "mutex at level"
                       << m_level << "while holding level" << currentLevel;

// In debug builds, we could assert here
#ifdef QT_DEBUG
            Q_ASSERT_X(m_level < currentLevel, "MutexHierarchy",
                       "Lock ordering violation detected");
#endif
        }
    }
}

bool SearchThreadSafety::MutexHierarchy::HierarchicalMutex::canAcquire() const {
    QMutexLocker locker(&s_hierarchyMutex);
    QThread* currentThread = QThread::currentThread();

    if (s_threadLevels.contains(currentThread)) {
        Level currentLevel = s_threadLevels[currentThread];
        // Can acquire if this lock has lower priority (higher number) than
        // current DocumentLevel=1 (highest priority) -> CacheLevel=2 (lower
        // priority) is allowed
        return m_level > currentLevel;
    }

    return true;
}

void SearchThreadSafety::MutexHierarchy::HierarchicalMutex::setCurrentLevel(
    Level level) {
    QMutexLocker locker(&s_hierarchyMutex);
    s_threadLevels[QThread::currentThread()] = level;
}

void SearchThreadSafety::MutexHierarchy::HierarchicalMutex::
    clearCurrentLevel() {
    QMutexLocker locker(&s_hierarchyMutex);
    QThread* currentThread = QThread::currentThread();

    if (s_threadLevels.contains(currentThread)) {
        // Restore previous level or remove if this was the only lock
        s_threadLevels.remove(currentThread);
    }
}

SearchThreadSafety::MutexHierarchy::HierarchicalMutex*
SearchThreadSafety::MutexHierarchy::createMutex(Level level) {
    return new HierarchicalMutex(level);
}

void SearchThreadSafety::MutexHierarchy::validateHierarchy() {
    QMutexLocker locker(&s_hierarchyMutex);

    // Check for any potential deadlock situations
    for (auto it = s_threadLevels.begin(); it != s_threadLevels.end(); ++it) {
        QThread* thread = it.key();
        Level level = it.value();

        qDebug() << "Thread" << thread << "currently holds lock at level"
                 << level;
    }
}

// ContentionMonitor implementation
void SearchThreadSafety::ContentionMonitor::recordLockAttempt(
    const QString& mutexName) {
    QMutexLocker locker(&s_statsMutex);
    s_stats[mutexName].lockAttempts++;
}

void SearchThreadSafety::ContentionMonitor::recordLockContention(
    const QString& mutexName, qint64 waitTime) {
    QMutexLocker locker(&s_statsMutex);
    ContentionStats& stats = s_stats[mutexName];
    stats.lockContentions++;
    stats.totalWaitTime += waitTime;
    stats.maxWaitTime = qMax(stats.maxWaitTime, waitTime);
}

SearchThreadSafety::ContentionMonitor::ContentionStats
SearchThreadSafety::ContentionMonitor::getStats(const QString& mutexName) {
    QMutexLocker locker(&s_statsMutex);
    return s_stats.value(mutexName, ContentionStats());
}

QHash<QString, SearchThreadSafety::ContentionMonitor::ContentionStats>
SearchThreadSafety::ContentionMonitor::getAllStats() {
    QMutexLocker locker(&s_statsMutex);
    return s_stats;
}

void SearchThreadSafety::ContentionMonitor::resetStats() {
    QMutexLocker locker(&s_statsMutex);
    s_stats.clear();
}

/**
 * Monitored mutex wrapper that tracks contention
 */
class MonitoredMutex {
public:
    explicit MonitoredMutex(const QString& name) : m_name(name) {}

    void lock() {
        SearchThreadSafety::ContentionMonitor::recordLockAttempt(m_name);

        QElapsedTimer timer;
        timer.start();

        if (!m_mutex.tryLock()) {
            // Lock contention detected
            m_mutex.lock();
            qint64 waitTime = timer.elapsed();
            SearchThreadSafety::ContentionMonitor::recordLockContention(
                m_name, waitTime);
        }
    }

    void unlock() { m_mutex.unlock(); }

    bool tryLock() {
        SearchThreadSafety::ContentionMonitor::recordLockAttempt(m_name);
        return m_mutex.tryLock();
    }

    QMutex& mutex() { return m_mutex; }

private:
    QMutex m_mutex;
    QString m_name;
};

/**
 * Thread-safe search state manager
 */
class SearchStateManager {
public:
    enum SearchState { Idle, Searching, Cancelling, Error };

    SearchStateManager() : m_state(Idle), m_searchId(0) {}

    bool startSearch() {
        QMutexLocker locker(&m_mutex);
        if (m_state != Idle) {
            return false;
        }
        m_state = Searching;
        m_searchId++;
        return true;
    }

    void finishSearch() {
        QMutexLocker locker(&m_mutex);
        if (m_state == Searching) {
            m_state = Idle;
        }
    }

    bool cancelSearch() {
        QMutexLocker locker(&m_mutex);
        if (m_state == Searching) {
            m_state = Cancelling;
            return true;
        }
        return false;
    }

    void setError() {
        QMutexLocker locker(&m_mutex);
        m_state = Error;
    }

    void reset() {
        QMutexLocker locker(&m_mutex);
        m_state = Idle;
    }

    SearchState state() const {
        QMutexLocker locker(&m_mutex);
        return m_state;
    }

    int currentSearchId() const {
        QMutexLocker locker(&m_mutex);
        return m_searchId;
    }

    bool isSearching() const {
        QMutexLocker locker(&m_mutex);
        return m_state == Searching;
    }

    bool isCancelling() const {
        QMutexLocker locker(&m_mutex);
        return m_state == Cancelling;
    }

private:
    mutable QMutex m_mutex;
    SearchState m_state;
    int m_searchId;
};

/**
 * Thread-safe result accumulator
 */
template <typename T>
class ThreadSafeAccumulator {
public:
    void addResult(const T& result) {
        QMutexLocker locker(&m_mutex);
        m_results.append(result);
        m_condition.wakeAll();
    }

    void addResults(const QList<T>& results) {
        QMutexLocker locker(&m_mutex);
        m_results.append(results);
        m_condition.wakeAll();
    }

    QList<T> getResults() const {
        QMutexLocker locker(&m_mutex);
        return m_results;
    }

    QList<T> takeResults() {
        QMutexLocker locker(&m_mutex);
        QList<T> results = m_results;
        m_results.clear();
        return results;
    }

    int count() const {
        QMutexLocker locker(&m_mutex);
        return m_results.size();
    }

    bool isEmpty() const {
        QMutexLocker locker(&m_mutex);
        return m_results.isEmpty();
    }

    void clear() {
        QMutexLocker locker(&m_mutex);
        m_results.clear();
        m_condition.wakeAll();
    }

    bool waitForResults(int count, int timeoutMs = -1) {
        QMutexLocker locker(&m_mutex);

        while (m_results.size() < count) {
            if (timeoutMs < 0) {
                m_condition.wait(&m_mutex);
            } else {
                if (!m_condition.wait(&m_mutex, timeoutMs)) {
                    return false;  // Timeout
                }
            }
        }

        return true;
    }

private:
    mutable QMutex m_mutex;
    QWaitCondition m_condition;
    QList<T> m_results;
};

/**
 * Thread-safe progress tracker
 */
class ProgressTracker {
public:
    ProgressTracker() : m_current(0), m_total(0), m_percentage(0) {}

    void setTotal(int total) {
        QMutexLocker locker(&m_mutex);
        m_total = total;
        updatePercentage();
    }

    void setCurrent(int current) {
        QMutexLocker locker(&m_mutex);
        m_current = current;
        updatePercentage();
    }

    void increment() {
        QMutexLocker locker(&m_mutex);
        m_current++;
        updatePercentage();
    }

    void reset() {
        QMutexLocker locker(&m_mutex);
        m_current = 0;
        m_total = 0;
        m_percentage = 0;
    }

    int current() const {
        QMutexLocker locker(&m_mutex);
        return m_current;
    }

    int total() const {
        QMutexLocker locker(&m_mutex);
        return m_total;
    }

    double percentage() const {
        QMutexLocker locker(&m_mutex);
        return m_percentage;
    }

    bool isComplete() const {
        QMutexLocker locker(&m_mutex);
        return m_total > 0 && m_current >= m_total;
    }

private:
    mutable QMutex m_mutex;
    int m_current;
    int m_total;
    double m_percentage;

    void updatePercentage() {
        if (m_total > 0) {
            m_percentage = (double)m_current / m_total * 100.0;
        } else {
            m_percentage = 0.0;
        }
    }
};

/**
 * Thread-safe cache with LRU eviction
 */
template <typename Key, typename Value>
class ThreadSafeCache {
public:
    explicit ThreadSafeCache(int maxSize = 1000) : m_maxSize(maxSize) {}

    void insert(const Key& key, const Value& value) {
        QMutexLocker locker(&m_mutex);

        // Remove if already exists
        if (m_cache.contains(key)) {
            m_accessOrder.removeOne(key);
        }

        // Add to cache and access order
        m_cache[key] = value;
        m_accessOrder.append(key);

        // Evict if necessary
        while (m_cache.size() > m_maxSize) {
            Key oldestKey = m_accessOrder.takeFirst();
            m_cache.remove(oldestKey);
        }
    }

    bool contains(const Key& key) const {
        QMutexLocker locker(&m_mutex);
        return m_cache.contains(key);
    }

    Value value(const Key& key, const Value& defaultValue = Value()) {
        QMutexLocker locker(&m_mutex);

        if (m_cache.contains(key)) {
            // Update access order
            m_accessOrder.removeOne(key);
            m_accessOrder.append(key);
            return m_cache[key];
        }

        return defaultValue;
    }

    void remove(const Key& key) {
        QMutexLocker locker(&m_mutex);
        m_cache.remove(key);
        m_accessOrder.removeOne(key);
    }

    void clear() {
        QMutexLocker locker(&m_mutex);
        m_cache.clear();
        m_accessOrder.clear();
    }

    int size() const {
        QMutexLocker locker(&m_mutex);
        return m_cache.size();
    }

    QList<Key> keys() const {
        QMutexLocker locker(&m_mutex);
        return m_cache.keys();
    }

private:
    mutable QMutex m_mutex;
    QHash<Key, Value> m_cache;
    QList<Key> m_accessOrder;
    int m_maxSize;
};
