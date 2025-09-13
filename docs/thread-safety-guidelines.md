# Thread Safety Guidelines for SAST-Readium

## Overview

This document outlines the thread safety guidelines and best practices for the SAST-Readium PDF viewer application. Following these guidelines is crucial to prevent deadlocks, race conditions, and other multithreading issues.

## Key Principles

### 1. Consistent Lock Ordering

**Rule**: Always acquire mutexes in the same order across all code paths to prevent circular dependencies.

**Example of WRONG approach:**
```cpp
// Thread A
QMutexLocker locker1(&mutex1);
QMutexLocker locker2(&mutex2);  // Potential deadlock

// Thread B  
QMutexLocker locker2(&mutex2);
QMutexLocker locker1(&mutex1);  // Potential deadlock
```

**Example of CORRECT approach:**
```cpp
// Always acquire mutex1 before mutex2
QMutexLocker locker1(&mutex1);
QMutexLocker locker2(&mutex2);
```

### 2. Minimize Mutex Scope

**Rule**: Hold mutexes for the shortest time possible. Avoid calling other functions while holding locks.

**Example of WRONG approach:**
```cpp
QMutexLocker locker(&mutex);
expensiveOperation();  // Long operation while holding lock
anotherMutexOperation();  // Potential nested lock
```

**Example of CORRECT approach:**
```cpp
{
    QMutexLocker locker(&mutex);
    // Copy data needed for operation
    auto dataCopy = sharedData;
}
// Perform expensive operation outside of lock
expensiveOperation(dataCopy);
```

### 3. Avoid Nested Mutex Calls

**Rule**: Never call methods that acquire mutexes while already holding a mutex.

**Fixed in ThumbnailGenerator:**
```cpp
// OLD (deadlock-prone):
QMutexLocker locker(&m_queueMutex);
if (isGenerating(pageNumber)) {  // This acquires m_jobsMutex!
    return;
}

// NEW (deadlock-safe):
bool alreadyGenerating = false;
{
    QMutexLocker jobsLocker(&m_jobsMutex);
    alreadyGenerating = m_activeJobs.contains(pageNumber);
}
if (alreadyGenerating) {
    return;
}
QMutexLocker queueLocker(&m_queueMutex);
// ... rest of operation
```

### 4. Thread Cleanup Best Practices

**Rule**: Always cleanup threads outside of mutex scope to prevent deadlocks during thread termination.

**Example from AsyncDocumentLoader fix:**
```cpp
// Take ownership outside mutex
QThread* threadToCleanup = nullptr;
{
    QMutexLocker locker(&m_stateMutex);
    threadToCleanup = m_workerThread;
    m_workerThread = nullptr;
}

// Cleanup outside mutex
if (threadToCleanup) {
    threadToCleanup->quit();
    threadToCleanup->wait(3000);
    threadToCleanup->deleteLater();
}
```

## Component-Specific Guidelines

### ThumbnailGenerator

**Mutex Hierarchy (acquire in this order):**
1. `m_documentMutex` (document access)
2. `m_queueMutex` (request queue)
3. `m_jobsMutex` (active jobs)
4. `m_dpiCacheMutex` (DPI cache)

**Key Fixes Applied:**
- Separated mutex scopes in `generateThumbnail()`
- Eliminated busy-wait loop in `setMaxConcurrentJobs()`
- Moved thread cleanup outside mutex in `cleanupJobs()`

### AsyncDocumentLoader

**Thread Safety Measures:**
- Separate mutexes for state (`m_stateMutex`) and queue (`m_queueMutex`)
- Thread cleanup performed outside mutex scope
- Timeout handling with proper thread affinity

**Key Fixes Applied:**
- Minimized mutex scope in `cancelLoading()`
- Fixed thread cleanup deadlocks
- Improved timeout error handling

### PDFPrerenderer

**Thread Coordination:**
- Main thread manages cache and requests
- Worker threads handle rendering
- Proper shutdown sequence with timeouts

**Key Fixes Applied:**
- Document updates outside queue mutex
- Improved worker thread shutdown
- Added timeout for thread termination

## Testing Guidelines

### Stress Testing

Run the thread safety tests to verify deadlock fixes:

```bash
cd build
make test_thread_safety
./tests/unit/test_thread_safety
```

### Manual Testing Scenarios

1. **Rapid Document Switching**: Load different documents quickly while thumbnails are generating
2. **Concurrent Operations**: Multiple thumbnail requests while changing settings
3. **Shutdown Under Load**: Close application while background operations are running

## Common Deadlock Patterns to Avoid

### 1. Lock Inversion
```cpp
// Thread A: mutex1 -> mutex2
// Thread B: mutex2 -> mutex1
// SOLUTION: Always use same order
```

### 2. Waiting While Holding Locks
```cpp
// WRONG:
QMutexLocker locker(&mutex);
thread.wait();  // Can deadlock if thread needs mutex

// CORRECT:
locker.unlock();
thread.wait();
```

### 3. Signal Emission Under Lock
```cpp
// WRONG:
QMutexLocker locker(&mutex);
emit signal();  // Connected slots might acquire locks

// CORRECT:
{
    QMutexLocker locker(&mutex);
    // Prepare data
}
emit signal();  // Emit outside lock
```

## Performance Considerations

### 1. Read-Write Locks
For data that's read frequently but written rarely, consider `QReadWriteLock`:

```cpp
QReadWriteLock rwLock;

// Multiple readers can access simultaneously
QReadLocker readLocker(&rwLock);
auto value = sharedData;

// Exclusive write access
QWriteLocker writeLocker(&rwLock);
sharedData = newValue;
```

### 2. Atomic Operations
For simple counters and flags, use `std::atomic`:

```cpp
std::atomic<int> counter{0};
std::atomic<bool> isRunning{false};

// Thread-safe without explicit locking
counter++;
if (isRunning.load()) {
    // ...
}
```

### 3. Lock-Free Data Structures
Consider Qt's concurrent containers for high-performance scenarios:
- `QConcurrentQueue` for producer-consumer patterns
- `QFuture` and `QtConcurrent` for parallel processing

## Debugging Tools

### 1. Thread Sanitizer
Compile with ThreadSanitizer to detect race conditions:
```bash
cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread" ..
```

### 2. Helgrind (Valgrind)
Use Helgrind to detect potential deadlocks:
```bash
valgrind --tool=helgrind ./your_application
```

### 3. Qt Debug Output
Enable Qt's thread debugging:
```cpp
QLoggingCategory::setFilterRules("qt.core.thread.debug=true");
```

## Code Review Checklist

When reviewing multithreaded code, check for:

- [ ] Consistent lock ordering across all code paths
- [ ] Minimal mutex scope (no long operations under lock)
- [ ] No nested mutex calls
- [ ] Proper thread cleanup outside mutex scope
- [ ] Timeout handling for thread operations
- [ ] Signal emission outside of locks
- [ ] Use of appropriate synchronization primitives

## Conclusion

Following these guidelines will help maintain thread safety and prevent deadlocks in the SAST-Readium application. Regular testing with the provided stress tests and careful code review are essential for maintaining robust multithreaded code.

For questions or clarifications, refer to the fixed implementations in:
- `app/ui/thumbnail/ThumbnailGenerator.cpp`
- `app/model/AsyncDocumentLoader.cpp`
- `app/ui/viewer/PDFPrerenderer.cpp`
