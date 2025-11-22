# Application Freeze Fixes

## 1. Purpose

This document describes the critical fixes applied to resolve application freezes caused by blocking operations, infinite waits, deadlock scenarios, and timer callback overlap. These issues were systematically identified through timer and threading analysis and have been addressed through architectural improvements in thread management, mutex lock scoping, and timer callback coordination.

## 2. How it Works

### 2.1 AsyncDocumentLoader - Thread Cleanup Redesign

**Problem**: Blocking `wait(3000)` calls on worker threads were freezing the UI thread for 3 seconds during document load completion.

**Solution**: Replaced synchronous blocking cleanup with asynchronous `deleteLater()` pattern and proper signal connections.

**Implementation Details**:

- Lines 104-127 (Success handler): When document loading completes, instead of blocking the main thread with `wait()`, the worker and thread are scheduled for deletion using Qt's event loop.
- Lines 158-181 (Failure handler): Same pattern applied when document loading fails.
- **Key Pattern**:
  1. Disconnect all signals from worker (`workerToCleanup->disconnect()`)
  2. Schedule worker deletion (`workerToCleanup->deleteLater()`)
  3. Connect thread's `finished` signal to its own `deleteLater()` to schedule cleanup
  4. Request graceful quit (`threadToCleanup->quit()`)
  5. Set 5-second timeout warning to detect threads that hang

**Impact**: Eliminates 3-4 second UI freezes during document load completion. The main thread continues responding to user input while cleanup happens asynchronously.

### 2.2 SearchEngine - Infinite Wait Prevention

**Problem**: `SearchEngine::cancelCurrentSearch()` used infinite wait (`waitForDone(-1)`) with no timeout, causing potential indefinite UI freezes if background tasks don't complete.

**Solution**: Added explicit 5-second timeout to the wait call with warning logging for timeout scenarios.

**Implementation Details** (Lines 457-467):

- Changed from `backgroundProcessor->waitForDone(-1)` to `waitForDone(CANCEL_TIMEOUT_MS)` where `CANCEL_TIMEOUT_MS = 5000`
- Captures elapsed time and logs warning if timeout is reached
- Proceeds with cleanup regardless of timeout to prevent indefinite hang
- **Key Pattern**: `const int CANCEL_TIMEOUT_MS = 5000; backgroundProcessor->waitForDone(CANCEL_TIMEOUT_MS);`

**Impact**: Prevents indefinite waits. Even if background search tasks fail to complete gracefully, the application continues after 5 seconds instead of hanging indefinitely.

### 2.3 EventBus - Deadlock Prevention via Lock Minimization

**Problem**: Signal emission under mutex lock could cause deadlock from re-entrant calls. If an event handler publishes another event synchronously, a deadlock occurs when trying to acquire the same mutex.

**Solution**: Minimized critical sections by extracting necessary data under lock and emitting signals outside the lock.

**Implementation Details**:

**publish() method** (Lines 140-181):

- Extract event type and decision flags inside mutex-protected block
- Exit mutex scope before emitting signal
- **Pattern**:

  ```
  bool shouldEmitSignal = false;
  {
      QMutexLocker locker(&m_mutex);
      // ... work under lock ...
      shouldEmitSignal = true;
  }
  if (shouldEmitSignal) {
      emit eventPublished(eventType);  // Outside lock!
  }
  ```

**processEventQueue() method** (Lines 252-267):

- Take snapshot of event queue under lock
- Clear queue under lock
- Exit lock scope before processing events
- Prevents deadlock if event handlers re-enter event bus methods

**Impact**: Eliminates deadlock scenarios from re-entrant event bus calls and signal handlers. Improves concurrency by reducing lock contention.

### 2.4 CacheManager - Timer Callback Overlap Prevention

**Problem**: Four overlapping QTimer instances (5s cleanup timer, 10s memory pressure timer, statistics timer, system memory timer) could fire simultaneously or overlap, causing event loop starvation and excessive processing overhead.

**Solution**: Added atomic flag to prevent concurrent timer callback execution across all four timer-driven methods.

**Implementation Details**:

**Flag Declaration** (Line 116):

- `std::atomic<bool> timerCallbackActive{false}` - Thread-safe atomic flag in private data

**Applied to all timer callbacks**:

1. **performPeriodicCleanup()** (Lines 461-487):
   - Lines 463-467: Check and set flag atomically
   - If another callback is active, skip this execution and log
   - Line 486: Clear flag after completion

2. **onMemoryPressureTimer()** (Lines 489-499):
   - Lines 491-495: Same atomic check-and-set pattern
   - Line 498: Clear flag after completion

3. **updateCacheStatistics()** (Lines 501-539):
   - Lines 503-507: Check and set flag
   - Line 538: Clear flag after completion

4. **handleSystemMemoryPressure()** (Lines 808-866):
   - Lines 810-814: Check and set flag
   - Line 817: Handle early return (disabled monitoring) with flag reset
   - Line 865: Clear flag after completion

**Pattern Used**: `std::atomic<bool>::compare_exchange_strong(expected, desired)` provides lock-free synchronization:

```cpp
bool expected = false;
if (!m_d->timerCallbackActive.compare_exchange_strong(expected, true)) {
    // Another callback is running - skip this execution
    return;
}
// Do work...
m_d->timerCallbackActive.store(false);  // Release flag
```

**Impact**: Prevents timer callback overlap and event loop starvation. If a callback is slow (e.g., memory pressure handling), other timers skip their scheduled execution rather than queuing up behind it. Improves application responsiveness during heavy memory operations.

## 3. Relevant Code Modules

- `/app/model/AsyncDocumentLoader.cpp` (Lines 104-127, 158-181): Thread cleanup pattern
- `/app/search/SearchEngine.cpp` (Lines 457-467): Wait timeout implementation
- `/app/controller/EventBus.cpp` (Lines 140-181, 252-267): Lock minimization pattern
- `/app/cache/CacheManager.cpp` (Lines 116, 461-486, 489-499, 501-539, 808-866): Atomic timer flag and callback coordination

## 4. Attention

1. **Atomic Flag Pattern**: The `compare_exchange_strong` pattern in CacheManager is lock-free and requires careful understanding. Do not modify the atomics operations without understanding memory ordering semantics.

2. **Thread Cleanup Timing**: AsyncDocumentLoader's 5-second timeout warning is non-blocking but will log if threads don't cleanup gracefully. Monitor logs for these warnings in performance testing.

3. **Timeout Values**: Both AsyncDocumentLoader (5s) and SearchEngine (5s) use matching timeout values. If these timeout frequently, investigate why background tasks are slow.

4. **Signal Emission Safety**: The EventBus pattern of emitting signals outside locks must be maintained. Any future modifications must preserve this pattern to avoid reintroducing deadlocks.

5. **Timer Callback Atomicity**: The timerCallbackActive flag prevents overlap but means skipped executions. This is acceptable because timers are periodic (5-10s intervals) and missing one execution has minimal impact.
