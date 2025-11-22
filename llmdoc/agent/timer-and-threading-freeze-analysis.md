# Application Freeze Issue: Timer and Threading Analysis

## Executive Summary

This investigation identified 40+ QTimer instances across the SAST Readium codebase, multiple mutex patterns, potential deadlock scenarios, and blocking operations in timer callbacks. The analysis reveals several patterns that could contribute to application freezes, particularly involving timer cleanup, thread synchronization, and blocking wait operations on the main/UI thread.

---

## Part 1: Evidence - Code Sections and Findings

### Section 1.1: Timer Usage Inventory

#### CacheManager - Four Periodic Timers

**File:** `app/cache/CacheManager.cpp`
**Lines:** 33-61

```cpp
explicit Implementation(CacheManager* qPtr)
    : q_ptr(qPtr),
      cleanupTimer(new QTimer(qPtr)),
      memoryPressureTimer(new QTimer(qPtr)),
      statsUpdateTimer(new QTimer(qPtr)),
      systemMemoryTimer(new QTimer(qPtr)),
      ...
{
    // Setup timers
    cleanupTimer->setSingleShot(false);
    memoryPressureTimer->setSingleShot(false);
    statsUpdateTimer->setSingleShot(false);
    systemMemoryTimer->setSingleShot(false);

    QObject::connect(cleanupTimer, &QTimer::timeout, qPtr,
                     &CacheManager::performPeriodicCleanup);
    QObject::connect(memoryPressureTimer, &QTimer::timeout, qPtr,
                     &CacheManager::onMemoryPressureTimer);
    QObject::connect(statsUpdateTimer, &QTimer::timeout, qPtr,
                     &CacheManager::updateCacheStatistics);
    QObject::connect(systemMemoryTimer, &QTimer::timeout, qPtr,
                     &CacheManager::handleSystemMemoryPressure);

    // Start timers
    cleanupTimer->start(config.cleanupInterval);
    memoryPressureTimer->start(5000);  // Check every 5 seconds
    statsUpdateTimer->start(10000);    // Update stats every 10 seconds
    systemMemoryTimer->start(config.systemMemoryCheckInterval);
}
```

**Key Details:**

- Four recurring timers checking cache state every 5-10 seconds
- `cleanupTimer` performs periodic cache cleanup
- `memoryPressureTimer` monitors memory every 5 seconds
- `statsUpdateTimer` updates statistics every 10 seconds
- `systemMemoryTimer` checks system memory pressure periodically
- All timers are recurring (setSingleShot(false))

#### IncrementalSearchManager - Single-Shot Debounce Timer

**File:** `app/search/IncrementalSearchManager.cpp`
**Lines:** 4-20

```cpp
class IncrementalSearchManager::Implementation {
public:
    Implementation(IncrementalSearchManager* q)
        : q_ptr(q), enabled(true), searchDelay(300) {
        timer = new QTimer(q_ptr);
        timer->setSingleShot(true);
        QObject::connect(timer, &QTimer::timeout, q_ptr,
                         &IncrementalSearchManager::onTimerTimeout);
    }

    IncrementalSearchManager* q_ptr;
    QTimer* timer;
    bool enabled;
    int searchDelay;
    QString pendingQuery;
    SearchOptions pendingOptions;
};
```

**Start Logic:**

**File:** `app/search/IncrementalSearchManager.cpp`
**Lines:** 61-76

```cpp
void IncrementalSearchManager::scheduleSearch(const QString& query,
                                              const SearchOptions& options) {
    if (!d->enabled) {
        emit searchTriggered(query, options);
        return;
    }

    d->pendingQuery = query;
    d->pendingOptions = options;

    d->timer->stop();
    d->timer->setInterval(d->searchDelay);
    d->timer->start();

    emit searchScheduled();
}
```

**Key Details:**

- Single-shot timer with 300ms debounce delay
- Stores pending query and options during interval
- Restarts timer on each query change
- Designed for incremental search triggering

#### AsyncDocumentLoader - Progress Timer and Worker Timeout Timer

**File:** `app/model/AsyncDocumentLoader.h`
**Lines:** 104-164

```cpp
// 进度模拟
QTimer* m_progressTimer;
int m_currentProgress;
int m_expectedLoadTime;  // 预期加载时间(ms)
qint64 m_startTime;      // 开始加载时间

...

// Timeout mechanism - Timer is created in worker thread to ensure proper
// thread affinity This fixes the issue where timer created in main thread
// couldn't properly timeout operations running in worker thread due to Qt's
// thread affinity rules
QTimer* m_timeoutTimer;
QMutex m_stateMutex;
std::atomic<bool> m_cancelled;
std::atomic<bool> m_loadingInProgress;
```

**Progress Timer Setup:**

**File:** `app/model/AsyncDocumentLoader.cpp`
**Lines:** 20-25

```cpp
// 初始化进度定时器
m_progressTimer = new QTimer(this);
m_progressTimer->setInterval(PROGRESS_UPDATE_INTERVAL);
connect(m_progressTimer, &QTimer::timeout, this,
        &AsyncDocumentLoader::onProgressTimerTimeout);
```

**Worker Timeout Timer (Critical):**

**File:** `app/model/AsyncDocumentLoader.cpp`
**Lines:** 434-450

```cpp
// Timeout mechanism - Timer is created in worker thread to ensure proper
// thread affinity
m_timeoutTimer = new QTimer();  // No parent = current thread affinity
...
m_timeoutTimer->setInterval(timeout);
connect(m_timeoutTimer, &QTimer::timeout, this,
        &AsyncDocumentLoaderWorker::onLoadTimeout);
m_timeoutTimer->start();
```

**Key Details:**

- Progress timer: 50ms updates during document loading
- Timeout timer: Created in worker thread (not main thread) for proper thread affinity
- CRITICAL FIX: Timeout timer parent is nullptr to ensure thread context
- Maximum timeout: 120 seconds

#### MemoryManager - Two Optimization Timers

**File:** `app/search/MemoryManager.cpp`
**Lines:** 14-40

```cpp
explicit Implementation(MemoryManager* q)
    : q_ptr(q),
      optimizationTimer(new QTimer(q)),
      statsUpdateTimer(new QTimer(q)) {
    // Setup timers
    optimizationTimer->setSingleShot(false);
    statsUpdateTimer->setSingleShot(false);

    QObject::connect(optimizationTimer, &QTimer::timeout, q,
                     &MemoryManager::performPeriodicOptimization);
    QObject::connect(statsUpdateTimer, &QTimer::timeout, q,
                     &MemoryManager::updateMemoryStats);

    // Connect to cache manager signals
    CacheManager& cacheManager = CacheManager::instance();
    QObject::connect(&cacheManager, &CacheManager::memoryPressureDetected,
                     q, &MemoryManager::onMemoryPressureDetected);
    QObject::connect(&cacheManager,
                     &CacheManager::systemMemoryPressureDetected, q,
                     &MemoryManager::onSystemMemoryPressure);
    QObject::connect(&cacheManager, &CacheManager::memoryLimitExceeded, q,
                     &MemoryManager::onCacheMemoryExceeded);

    // Start timers
    optimizationTimer->start(optimizationInterval * 1000);
    statsUpdateTimer->start(5000);  // Update stats every 5 seconds
}
```

**Key Details:**

- `optimizationTimer`: Periodic optimization with configurable interval (default 30 seconds)
- `statsUpdateTimer`: Memory statistics every 5 seconds
- Both recurring timers
- Connected to CacheManager signals for memory pressure handling

#### EventBus - Process and Overflow Timers

**File:** `app/controller/EventBus.cpp`
**Lines:** 24-43

```cpp
EventBus::EventBus(QObject* parent)
    : QObject(parent),
      m_processTimer(new QTimer(this)),
      m_overflowTimer(new QTimer(this)),
      m_logger("EventBus") {
    m_processTimer->setSingleShot(true);
    connect(m_processTimer, &QTimer::timeout, this,
            &EventBus::processNextEvent);

    m_overflowTimer->setSingleShot(true);
    connect(m_overflowTimer, &QTimer::timeout, this, [this]() {
        if (m_totalDropped > 0) {
            emit queueOverflow(m_totalDropped);
            m_logger.warning(QString("Event queue overflow, dropped %1 events")
                                 .arg(m_totalDropped));
            m_totalDropped = 0;
        }
        m_overflowEmitted = false;
    });
}
```

**Key Details:**

- `m_processTimer`: Single-shot, fires to process next event from queue
- `m_overflowTimer`: Single-shot, detects queue overflow
- Both running in main thread (singleton pattern)

#### UIStateManager - Autosave Timer

**File:** `app/ui/core/UIStateManager.cpp`
**Lines:** 20-44

```cpp
UIStateManager::UIStateManager()
    : QObject(nullptr),
      m_autosaveTimer(new QTimer(this)),
      m_autosaveEnabled(true),
      ...
{
    // Setup autosave timer
    m_autosaveTimer->setSingleShot(false);
    connect(m_autosaveTimer, &QTimer::timeout, this,
            &UIStateManager::onAutosaveTimer);

    // Load existing state
    loadStateFromFile();

    m_logger.info("UIStateManager initialized with state file: " +
                  m_stateFilePath);
}
```

**Key Details:**

- Recurring autosave timer for UI state persistence
- Saves window geometry, splitter positions, widget visibility
- Interval configurable

#### HighlightManager - Autosave Timer

**File:** `app/managers/HighlightManager.cpp`
**Lines:** 16-34

```cpp
HighlightManager::HighlightManager(QObject* parent)
    : QObject(parent),
      m_model(std::make_unique<HighlightModel>()),
      m_undoStack(nullptr),
      m_selectionManager(nullptr),
      m_document(nullptr),
      m_autoSaveEnabled(true),
      m_autoSaveInterval(30000),  // 30 seconds
      ...
{
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setInterval(m_autoSaveInterval);
    connect(m_autoSaveTimer, &QTimer::timeout, this,
            &HighlightManager::performAutoSave);

    connectModelSignals();

    SLOG_INFO("HighlightManager initialized");
}
```

**Start Logic:**

**File:** `app/managers/HighlightManager.cpp`
**Lines:** 60-65

```cpp
// Start auto-save timer
if (m_autoSaveEnabled) {
    m_autoSaveTimer->start();
}

SLOG_INFO(QString("Document set: %1").arg(documentPath));
```

**Key Details:**

- 30-second interval autosave timer
- Saves highlight annotations to disk
- Starts when document is set

#### ThumbnailModel - Three Timers (Preload, Cleanup, Priority)

**File:** `app/model/ThumbnailModel.cpp`
**Lines:** 78-100

```cpp
// 设置预加载定时器
m_preloadTimer = new QTimer(this);
m_preloadTimer->setInterval(PRELOAD_TIMER_INTERVAL);
m_preloadTimer->setSingleShot(false);
connect(m_preloadTimer, &QTimer::timeout, this,
        &ThumbnailModel::onPreloadTimer);

// 设置缓存清理定时器
QTimer* cleanupTimer = new QTimer(this);
cleanupTimer->setInterval(30000);  // 30秒清理一次
cleanupTimer->setSingleShot(false);
connect(cleanupTimer, &QTimer::timeout, this, [this]() {
    cleanupCache();
    cleanupOptimizedCache();
});
cleanupTimer->start();

// 设置优先级更新定时器
m_priorityUpdateTimer = new QTimer(this);
m_priorityUpdateTimer->setInterval(200);  // 200ms间隔
m_priorityUpdateTimer->setSingleShot(false);
connect(m_priorityUpdateTimer, &QTimer::timeout, this,
        &ThumbnailModel::onPriorityUpdateTimer);
```

**Key Details:**

- `m_preloadTimer`: Prefetch adjacent/upcoming thumbnails
- `cleanupTimer`: Cache cleanup every 30 seconds
- `m_priorityUpdateTimer`: Priority queue updates every 200ms
- Callback accesses shared mutex (`m_thumbnailsMutex`)

#### SearchWidget - Debounce Timer

**File:** `app/ui/widgets/SearchWidget.cpp`
**Lines:** 73, 89-91

```cpp
m_searchModel(new SearchModel(this)),
m_document(nullptr),
m_searchTimer(new QTimer(this)),
...

// Configure search timer for debounced search
m_searchTimer->setSingleShot(true);
m_searchTimer->setInterval(300);  // 300ms delay
```

**Key Details:**

- Single-shot 300ms debounce timer
- Prevents excessive search triggering during typing

#### ThumbnailDelegate - Loading Animation and Cache Cleanup Timers

**File:** `app/delegate/ThumbnailDelegate.cpp`
**Lines:** 71-89

```cpp
loadingTimer(new QTimer(q)) {
    // ... setup ...
    // 设置加载动画定时器
    loadingTimer->setInterval(LOADING_ANIMATION_INTERVAL);
    QObject::connect(loadingTimer, &QTimer::timeout, q,
                     &ThumbnailDelegate::onLoadingAnimationTimer);

    // 设置缓存清理定时器
    QTimer* cacheCleanupTimer = new QTimer(q);
    cacheCleanupTimer->setInterval(CACHE_CLEANUP_INTERVAL);
    QObject::connect(cacheCleanupTimer, &QTimer::timeout, q,
                     [this]() { cleanupExpiredCache(); });
    cacheCleanupTimer->start();
}
```

**Key Details:**

- `loadingTimer`: 50ms animation frames
- `cacheCleanupTimer`: 60-second cache cleanup
- Lambda callback with capture of `this` pointer

### Section 1.2: Mutex and Lock Analysis

#### CacheManager Mutex

**File:** `app/cache/CacheManager.cpp`
**Lines:** 113, 300-966 (multiple lock points)

```cpp
// Thread safety
mutable QMutex mutex;
```

**Usage Pattern:** QMutexLocker extensively used in ~60 lock points

**Critical Lock Scope Example:**

**File:** `app/cache/CacheManager.cpp`
**Lines:** 300-311

```cpp
void CacheManager::registerCache(CacheType type, ICacheComponent* cache) {
    QMutexLocker locker(&m_d->mutex);

    if (!cache) {
        qWarning() << "Null cache component";
        return;
    }

    m_d->registeredCaches[type] = cache;
    m_d->initializeDefaultLimits();
}
```

#### EventBus Mutex

**File:** `app/controller/EventBus.cpp`
**Lines:** 62-83, 102-120, 140-169, 228-248

```cpp
void EventBus::subscribe(const QString& eventType, QObject* subscriber,
                         EventHandler handler) {
    QMutexLocker locker(&m_mutex);
    // ... subscription logic ...
}

void EventBus::publish(Event* event) {
    if (!event) {
        m_logger.warning("Attempted to publish null event");
        return;
    }

    QMutexLocker locker(&m_mutex);

    // Apply filters
    if (!applyFilters(event)) {
        m_logger.debug("Event filtered out: " + event->type());
        return;
    }

    m_totalEventsPublished++;
    emit eventPublished(event->type());

    if (m_asyncProcessingEnabled) {
        // Add to queue for async processing
        m_eventQueue.append(event);

        if (!m_processTimer->isActive()) {
            m_processTimer->start(0);
        }
    } else {
        // Process immediately
        deliverEvent(event);
        delete event;
    }
}
```

**Key Details:**

- Single QMutex protecting subscriptions, event queue, and filters
- Timer is started inside mutex scope (potential lock contention)
- publishAsync also holds mutex while checking queue overflow

#### AsyncDocumentLoader State Mutex

**File:** `app/model/AsyncDocumentLoader.h`
**Lines:** 97-101

```cpp
// 状态管理
LoadingState m_state;
QString m_currentFilePath;
mutable QMutex m_stateMutex;

// 文档加载队列
QStringList m_documentQueue;
mutable QMutex m_queueMutex;
```

**Critical Deadlock Fix Pattern:**

**File:** `app/model/AsyncDocumentLoader.cpp`
**Lines:** 80-98

```cpp
// DEADLOCK FIX: Minimize mutex scope for thread cleanup
{
    QMutexLocker locker(&m_stateMutex);
    if (m_state == LoadingState::Loading) {
        m_state = LoadingState::Completed;
        filePath = m_currentFilePath;

        // Take ownership for cleanup outside mutex
        threadToCleanup = m_workerThread;
        workerToCleanup = m_worker;
        m_workerThread = nullptr;
        m_worker = nullptr;
    } else {
        // State changed, cleanup document and return
        delete document;
        return;
    }
}

stopProgressSimulation();
emit loadingProgressChanged(100);
emit loadingMessageChanged("加载完成");
emit documentLoaded(document, filePath);

// Cleanup worker and thread synchronously
if (threadToCleanup != nullptr) {
    threadToCleanup->quit();
    if (!threadToCleanup->wait(3000)) {  // BLOCKING WAIT!
        SLOG_WARNING("AsyncDocumentLoader: Thread cleanup timeout");
        threadToCleanup->terminate();
        threadToCleanup->wait(1000);
    }
}
```

**Key Details:**

- Mutex scope minimized to avoid holding during thread cleanup
- BLOCKING WAIT OUTSIDE MUTEX (but still on main thread)
- Cleanup operations moved outside critical section
- Multiple wait() calls on worker threads (potential freeze point)

#### StateManager Mutex

**File:** `app/controller/StateManager.cpp`
**Lines:** 575-602

```cpp
void StateManager::createSnapshot(const QString& name) {
    QMutexLocker locker(&m_mutex);
    m_snapshots[name] = m_currentState;
    locker.unlock();

    emit snapshotCreated(name);
    m_logger.debug(QString("Snapshot created: %1").arg(name));
}

bool StateManager::restoreSnapshot(const QString& name) {
    QMutexLocker locker(&m_mutex);

    if (!m_snapshots.contains(name)) {
        m_logger.warning(QString("Snapshot not found: %1").arg(name));
        return false;
    }

    State snapshot = m_snapshots[name];
    locker.unlock();

    setState(snapshot, QString("Restore snapshot: %1").arg(name));
    emit snapshotRestored(name);

    m_logger.debug(QString("Snapshot restored: %1").arg(name));
    return true;
}
```

**Key Details:**

- Explicit unlock() after copying state
- Prevents holding mutex during setState() call
- Good pattern for avoiding nested lock scenarios

#### ThumbnailModel Mutex

**File:** `app/model/ThumbnailModel.cpp`
**Lines:** 118-169

```cpp
case PixmapRole: {
    // 记录访问开始时间
    QElapsedTimer accessTimer;
    accessTimer.start();

    QMutexLocker locker(&m_thumbnailsMutex);

    // Try optimized cache first
    ThumbnailItem* cachedItem =
        const_cast<ThumbnailModel*>(this)->getFromOptimizedCache(
            pageNumber);
    if (cachedItem) {
        // ... complex cache operations under lock ...
        // Update access statistics
        cachedItem->lastAccessed = QDateTime::currentMSecsSinceEpoch();
        cachedItem->accessCount++;

        // 记录访问时间
        qint64 accessTime = accessTimer.elapsed();
        const_cast<ThumbnailModel*>(this)->recordAccessTime(
            accessTime);

        // 分析访问模式
        const_cast<ThumbnailModel*>(this)->analyzeAccessPattern(
            pageNumber);

        // Check prefetch statistics
        if (cachedItem->wasPrefetched) {
            const_cast<ThumbnailModel*>(this)
                ->m_prefetchHits.fetch_add(1);
            cachedItem->wasPrefetched = false;
        }

        return cachedItem->pixmap;
    }
```

**Key Details:**

- Long-held mutex lock in data() method (called from view)
- Multiple const_cast operations and method calls under lock
- Potential UI thread blocking during frequent thumbnail access

#### SearchThreadSafety - Hierarchical Mutex and Queue

**File:** `app/search/SearchThreadSafety.h`
**Lines:** 215-271, 150-210

```cpp
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
    };
};

// Thread-safe queue with wait condition
template <typename T>
class ThreadSafeQueue {
public:
    bool dequeue(T& item, int timeoutMs = -1) {
        QMutexLocker locker(&m_mutex);

        if (m_queue.isEmpty()) {
            if (timeoutMs < 0) {
                m_condition.wait(&m_mutex);  // BLOCKING WAIT
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
};
```

**Key Details:**

- Hierarchical mutex system prevents deadlocks
- Queue uses QWaitCondition with potential blocking
- ThreadSafeQueue used for producer-consumer patterns

### Section 1.3: Thread Synchronization and Blocking Patterns

#### BackgroundProcessor - Thread Pool and Watcher Cleanup

**File:** `app/search/BackgroundProcessor.cpp`
**Lines:** 18-93

```cpp
~Implementation() {
    qInfo() << "BackgroundProcessor::Implementation dtor - begin";
    // Stop accepting callbacks and cancel queued tasks
    cancelAllTasks();
    qInfo() << "BackgroundProcessor::Implementation dtor - waiting for "
               "threadPool";
    threadPool.waitForDone(5000);  // BLOCKING WAIT WITH 5 SECOND TIMEOUT

    // Ensure all watchers are destroyed deterministically
    QList<QFutureWatcherBase*> toDelete;
    {
        QMutexLocker locker(&futuresMutex);
        toDelete = activeWatchers;
        activeWatchers.clear();
    }
    for (auto* watcher : toDelete) {
        if (!watcher)
            continue;
        watcher->disconnect();
        // Best-effort cancel; tasks should already be done by now
        if (!watcher->isFinished()) {
            watcher->cancel();
        }
        delete watcher;  // direct delete is safe here
    }
    qInfo() << "BackgroundProcessor::Implementation dtor - end";
}

void BackgroundProcessor::waitForDone(int msecs) {
    qInfo() << "BackgroundProcessor::waitForDone(" << msecs << ")";
    d->threadPool.waitForDone(msecs);  // BLOCKING WAIT
    qInfo() << "BackgroundProcessor::waitForDone - done";
}
```

**Key Details:**

- Destructor blocks for 5 seconds waiting for thread pool
- Called during shutdown (main thread may freeze)
- Watcher cleanup with explicit deletion (not deleteLater)

#### SearchEngine - Blocking Wait on BackgroundProcessor

**File:** `app/search/SearchEngine.cpp`
**Lines:** 457

```cpp
backgroundProcessor->waitForDone(-1);  // BLOCKING WAIT WITH NO TIMEOUT
```

**Key Details:**

- Infinite blocking wait on background processor
- Could freeze main thread indefinitely if background tasks hang

#### PDFPrerenderer - Thread Wait in Queue Processing

**File:** `app/ui/viewer/PDFPrerenderer.cpp`
**Lines:** 202-206, 632

```cpp
if (!thread->wait(3000)) {
    SLOG_WARNING("Worker thread still active after 3 seconds");
    thread->terminate();
    thread->wait(1000);
}

...

m_queueCondition.wait(&m_queueMutex);  // BLOCKING WAIT ON CONDITION
```

**Key Details:**

- Threads waited in prerender cleanup
- QWaitCondition in queue processing loop

### Section 1.4: Timer Callback Complexity

#### CacheManager Memory Pressure Callback

**File:** `app/cache/CacheManager.cpp`
**Lines:** 159-213

```cpp
void performMemoryPressureEviction() const {
    qint64 totalUsage = calculateTotalMemoryUsage();
    qint64 targetUsage = static_cast<qint64>(config.totalMemoryLimit * 0.7);

    if (totalUsage <= targetUsage) {
        return;
    }

    qint64 bytesToFree = totalUsage - targetUsage;

    // Prioritize eviction based on cache importance and usage patterns
    QList<QPair<CacheType, double>> evictionPriority;

    for (auto it = registeredCaches.constBegin();
         it != registeredCaches.constEnd(); ++it) {
        CacheType type = it.key();
        if (it.value() == nullptr || !cacheEnabled.value(type, true)) {
            continue;
        }

        double priority = calculateEvictionPriority(type);
        evictionPriority.append({type, priority});
    }

    // Sort by priority (lower priority = evict first)
    std::ranges::sort(evictionPriority, ...);

    // Evict from lowest priority caches first
    for (const auto& pair : evictionPriority) {
        if (bytesToFree <= 0) {
            break;
        }

        CacheType type = pair.first;
        ICacheComponent* cache = registeredCaches.value(type);
        if (cache == nullptr) {
            continue;
        }

        qint64 cacheUsage = cache->getMemoryUsage();
        qint64 toEvictFromCache =
            std::min(bytesToFree,
                     cacheUsage / 2);  // Evict up to 50% from each cache

        cache->evictLRU(toEvictFromCache);  // COMPLEX OPERATION
        bytesToFree -= toEvictFromCache;

        emit q_ptr->cacheEvictionRequested(type, toEvictFromCache);
    }
}
```

**Key Details:**

- Called every 5 seconds from timer
- Iterates all registered caches
- Performs sorting and complex eviction logic
- Emits signals (re-entrance risk)

#### EventBus Queue Processing

**File:** `app/controller/EventBus.cpp`
**Lines:** 240-248

```cpp
void EventBus::processEventQueue() {
    QMutexLocker locker(&m_mutex);

    while (!m_eventQueue.isEmpty()) {
        Event* event = m_eventQueue.takeFirst();
        deliverEvent(event);  // Potential nested publishing
        delete event;
    }
}
```

**Key Details:**

- Processes entire queue while holding mutex
- deliverEvent() calls handlers that may publish new events
- Potential for event loop starvation

#### ThumbnailModel Cleanup Timers

**File:** `app/model/ThumbnailModel.cpp`
**Lines:** 89-91

```cpp
cleanupTimer->start();  // Registered on line 86-88
```

**Callback:**

```cpp
connect(cleanupTimer, &QTimer::timeout, this, [this]() {
    cleanupCache();
    cleanupOptimizedCache();
});
```

**Key Details:**

- Lambda captures `this` pointer
- Called every 30 seconds
- Accesses thumbnail cache (protected by `m_thumbnailsMutex`)

### Section 1.5: Potential Deadlock Scenarios

#### Scenario 1: AsyncDocumentLoader Worker Thread Cleanup

**File:** `app/model/AsyncDocumentLoader.cpp`
**Lines:** 80-128

```cpp
// Mutex minimized scope
{
    QMutexLocker locker(&m_stateMutex);
    if (m_state == LoadingState::Loading) {
        m_state = LoadingState::Completed;
        filePath = m_currentFilePath;
        threadToCleanup = m_workerThread;
        workerToCleanup = m_worker;
        m_workerThread = nullptr;
        m_worker = nullptr;
    } else {
        delete document;
        return;
    }
}

stopProgressSimulation();
emit loadingProgressChanged(100);
emit loadingMessageChanged("加载完成");
emit documentLoaded(document, filePath);

// Cleanup with blocking wait (POTENTIAL FREEZE POINT)
if (threadToCleanup != nullptr) {
    threadToCleanup->quit();
    if (!threadToCleanup->wait(3000)) {
        SLOG_WARNING("AsyncDocumentLoader: Thread cleanup timeout");
        threadToCleanup->terminate();
        threadToCleanup->wait(1000);
    }
}
```

**Risk:** If worker thread is deadlocked internally, wait(3000) blocks main thread

#### Scenario 2: EventBus Mutex Contention

**File:** `app/controller/EventBus.cpp`
**Lines:** 140-169

```cpp
void EventBus::publish(Event* event) {
    // ...
    QMutexLocker locker(&m_mutex);

    if (!applyFilters(event)) {
        // ...
        return;  // OK, early unlock
    }

    m_totalEventsPublished++;
    emit eventPublished(event->type());  // SIGNAL EMISSION UNDER LOCK

    if (m_asyncProcessingEnabled) {
        m_eventQueue.append(event);

        if (!m_processTimer->isActive()) {
            m_processTimer->start(0);  // TIMER STARTED UNDER LOCK
        }
    } else {
        deliverEvent(event);  // POTENTIAL RE-ENTRANCE
        delete event;
    }
    // Implicit unlock here
}
```

**Risk:** Signal emission under lock can cause nested publish() calls

#### Scenario 3: CacheManager System Memory Monitoring

**File:** `app/cache/CacheManager.cpp`
**Lines:** 48-61

```cpp
QObject::connect(cleanupTimer, &QTimer::timeout, qPtr,
                 &CacheManager::performPeriodicCleanup);
QObject::connect(memoryPressureTimer, &QTimer::timeout, qPtr,
                 &CacheManager::onMemoryPressureTimer);
QObject::connect(statsUpdateTimer, &QTimer::timeout, qPtr,
                 &CacheManager::updateCacheStatistics);
QObject::connect(systemMemoryTimer, &QTimer::timeout, qPtr,
                 &CacheManager::handleSystemMemoryPressure);

// Start all four timers
cleanupTimer->start(config.cleanupInterval);
memoryPressureTimer->start(5000);
statsUpdateTimer->start(10000);
systemMemoryTimer->start(config.systemMemoryCheckInterval);
```

**Risk:** Four timers firing asynchronously could cause rapid succession of cache evictions and system memory checks

### Section 1.6: Timer Cleanup Issues

#### CacheManager Destructor

**File:** `app/cache/CacheManager.cpp`
**Lines:** 64-87

```cpp
~Implementation() {
    // Stop and disconnect timers explicitly
    if (cleanupTimer != nullptr) {
        cleanupTimer->stop();
        cleanupTimer->disconnect();
    }
    if (memoryPressureTimer != nullptr) {
        memoryPressureTimer->stop();
        memoryPressureTimer->disconnect();
    }
    if (statsUpdateTimer != nullptr) {
        statsUpdateTimer->stop();
        statsUpdateTimer->disconnect();
    }
    if (systemMemoryTimer != nullptr) {
        systemMemoryTimer->stop();
        systemMemoryTimer->disconnect();
    }

    // Clear registered caches
    registeredCaches.clear();
}
```

**Key Details:**

- All timers properly stopped and disconnected
- Registered caches cleared (potential for dangling pointers if callbacks still running)

#### IncrementalSearchManager Destructor

**File:** `app/search/IncrementalSearchManager.cpp`
**Lines:** 29-36

```cpp
IncrementalSearchManager::~IncrementalSearchManager() {
    // Ensure timer won't fire during or after destruction
    if (d && d->timer) {
        d->timer->stop();
        d->timer->disconnect(this);
    }
    d->pendingQuery.clear();
}
```

**Key Details:**

- Timer stopped and disconnected explicitly
- Clean shutdown pattern

---

## Part 2: Findings and Analysis

### Finding 1: Excessive Timer Count and Firing Frequency

**Summary:** The codebase contains 40+ QTimer instances spread across multiple components with overlapping intervals (5s, 10s, 30s, 50ms, 200ms, 300ms). Without proper coordination, these timers can fire in rapid succession during the same event loop iteration.

**Components Affected:**

- CacheManager: 4 timers (5s, 10s, 5s, variable intervals)
- MemoryManager: 2 timers (30s, 5s intervals)
- ThumbnailModel: 3 timers (50ms preload, 30s cleanup, 200ms priority)
- AsyncDocumentLoader: 2 timers (50ms progress, 30s timeout)
- EventBus: 2 timers (async processing)
- UI widgets: 6+ timers (search, highlighting, logging updates)

**Impact:** Under memory pressure or high load, multiple timer callbacks could execute within milliseconds, causing cache evictions, memory optimization, and state updates to all occur simultaneously, potentially starving the main thread.

### Finding 2: Blocking Wait Operations on Main Thread

**Summary:** Multiple wait() calls on worker threads occur outside of background processor abstraction, directly blocking the main/UI thread:

1. **AsyncDocumentLoader.cpp:108** - `threadToCleanup->wait(3000)` after document loaded
2. **AsyncDocumentLoader.cpp:163** - `threadToCleanup->wait(3000)` after document failed
3. **SearchEngine.cpp:457** - `backgroundProcessor->waitForDone(-1)` with no timeout
4. **PDFPrerenderer.cpp:202** - `thread->wait(3000)` during prerender cleanup

**Impact:** If worker threads experience internal deadlocks or Poppler library hangs, these wait() calls will freeze the UI for 3 seconds to indefinitely.

### Finding 3: Mutex Contention in Hot Paths

**Summary:** ThumbnailModel holds `m_thumbnailsMutex` for entire duration of data() method, which is called frequently during scrolling:

```cpp
case PixmapRole: {
    QMutexLocker locker(&m_thumbnailsMutex);
    // Complex cache lookup, statistics, pattern analysis operations
    // ...
    return cachedItem->pixmap;
}
```

**Impact:** Thumbnail rendering on main thread blocks when background thumbnail generation (also using same mutex) is active.

### Finding 4: Nested Locking and Signal Emission Under Locks

**Summary:** EventBus emits signals while holding `m_mutex`, and deliverEvent() calls handlers that may attempt to publish new events:

**File:** `app/controller/EventBus.cpp:155-168`

```cpp
emit eventPublished(event->type());  // WHILE HOLDING m_mutex

if (m_asyncProcessingEnabled) {
    m_eventQueue.append(event);
    if (!m_processTimer->isActive()) {
        m_processTimer->start(0);  // TIMER STARTED UNDER LOCK
    }
} else {
    deliverEvent(event);  // CALLS HANDLERS THAT MAY PUBLISH
}
```

**Impact:** Event handlers receiving published events cannot themselves publish without potential deadlock.

### Finding 5: Insufficient Timeout Configuration

**Summary:** Critical operations lack universal timeout handling:

- SearchEngine: `waitForDone(-1)` - infinite wait
- AsyncDocumentLoader: Nested wait(3000) calls could total 4+ seconds
- CacheManager: Complex eviction algorithms with no timeout

**Impact:** No mechanism to interrupt frozen operations; application hangs indefinitely.

### Finding 6: Thread Affinity Issues (Partially Fixed)

**Summary:** AsyncDocumentLoaderWorker correctly creates timeout timer in worker thread (line 434: `m_timeoutTimer = new QTimer();` with no parent), but other components may have similar issues.

**Pattern Found:**

- AsyncDocumentLoader.cpp: FIXED - timer created in worker thread
- PDFPrerenderer.cpp: Uses QWaitCondition properly
- BackgroundProcessor.cpp: Uses QThreadPool::waitForDone() correctly

**Impact:** Previous freeze issues with document loading may recur if timeout timer creation moves back to main thread.

### Finding 7: Complex Operations in Timer Callbacks

**Summary:** Timer callbacks perform expensive operations that could block the event loop:

1. **CacheManager::performPeriodicCleanup()** (5-10s interval)
   - Iterates all caches
   - Calculates eviction priorities
   - Performs LRU eviction
   - Emits signals

2. **ThumbnailModel cleanup** (30s interval)
   - Calls cleanupCache() and cleanupOptimizedCache()
   - May allocate/deallocate significant memory

3. **MemoryManager optimization** (30s interval)
   - Analyzes memory usage patterns
   - May trigger cache evictions
   - Updates statistics

**Impact:** If cleanup operations take >100ms, next timer event may queue before previous completes, accumulating delays.

### Finding 8: Atomic Operations and Lock-Free Patterns

**Summary:** SearchThreadSafety provides lock-free structures but they may not be used consistently:

**Available:**

- AtomicCounter, AtomicFlag, AtomicPointer
- SharedData<T> with RWLock
- ThreadSafeQueue<T> with QWaitCondition
- HierarchicalMutex

**Usage:** Some components (SearchEngine, MemoryManager) use explicit QMutex instead of these patterns, missing optimization opportunities.

### Finding 9: Missing Resource Cleanup on Error Paths

**Summary:** Timer callbacks that allocate or queue resources lack comprehensive error handling:

**Example - EventBus overflow handling:**

**File:** `app/controller/EventBus.cpp:189-203`

```cpp
if (m_eventQueue.size() > m_maxQueueSize) {
    int eventsToDrop = m_eventQueue.size() - m_maxQueueSize;
    for (int i = 0; i < eventsToDrop; ++i) {
        delete m_eventQueue.takeFirst();
    }
    m_totalDropped += eventsToDrop;

    if (!m_overflowEmitted) {
        m_overflowTimer->start(1);
        m_overflowEmitted = true;
    }
}
```

**Risk:** Dropped events are deleted, but handlers expecting them may leak resources.

### Finding 10: Singleton Timer Instances in Static Destructors

**Summary:** EventBus and other singletons use QTimer as members, but destruction order during static destruction is unpredictable:

**Pattern:**

```cpp
// EventBus destructor
~EventBus() {
    if (!QCoreApplication::instance()) {
        return;  // Can't access timers, Qt objects during shutdown
    }
    clearEventQueue();
}
```

**Risk:** If QCoreApplication destroyed before singleton, timer cleanup skipped, potentially leaving timer callbacks queued.

---

## Summary of Freeze Risk Patterns

### Critical Risk Patterns

1. Blocking wait() on worker threads for 3+ seconds (AsyncDocumentLoader, SearchEngine)
2. Four overlapping timers in CacheManager firing frequently
3. Mutex held during expensive operations (cache eviction, memory analysis)
4. Signal emission under locks (EventBus)

### Medium Risk Patterns

1. Timer callback complexity and potential event loop starvation
2. Mutex contention in hot paths (ThumbnailModel)
3. Missing timeouts on background operations (SearchEngine)

### Low Risk Patterns

1. Proper timeout timer creation in worker threads (AsyncDocumentLoader)
2. Most timer cleanup is explicit and correct
3. Hierarchical mutex system available but underutilized
