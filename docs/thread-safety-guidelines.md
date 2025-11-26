# Thread Safety Guidelines

This document outlines the thread safety principles and best practices for SAST Readium development.

## Overview

SAST Readium is a multi-threaded application. Ensuring thread safety is critical for stability and correctness. We follow specific patterns to manage concurrency effectively.

## Core Principles

1. **UI Thread Affinity**: All UI operations must occur on the main thread (UI thread).
2. **Worker Threads**: Long-running operations (IO, heavy computation, network requests) must run on background threads to keep the UI responsive.
3. **Message Passing**: Prefer message passing (Signals & Slots, Events) over shared state.
4. **Explicit Synchronization**: When shared state is unavoidable, protect it with explicit synchronization primitives (Mutexes, Atomics).

## Architecture Patterns

### 1. Event Bus

The `EventBus` is the primary mechanism for decoupled, thread-safe communication between components.

- **Publishing**: Can be done from any thread.
- **Subscribing**: Handlers are invoked on the thread where the subscriber lives (if using `Qt::AutoConnection` or `Qt::QueuedConnection`).

**Best Practice**:
Use `EventBus::publishAsync()` for cross-thread events to ensure the event loop handles the dispatch.

```cpp
// Safe to call from a background thread
EventBus::instance().publishAsync("document_loaded", filePath);
```

### 2. AsyncDocumentLoader

Document loading happens in a dedicated worker thread (`AsyncDocumentLoaderWorker`).

- **Thread Affinity**: The worker object is moved to a `QThread`.
- **Timer Safety**: Timers used within the worker must be created **after** the worker is moved to the thread, or explicitly parented to the worker, to ensure correct thread affinity.

### 3. Logging

The logging system (`app/logging/`) is thread-safe by design (via `spdlog`). You can log from any thread without external locking.

```cpp
// Safe from any thread
LOG_INFO("Processing page {} on thread {}", pageNum, QThread::currentThreadId());
```

## Qt Specific Guidelines

### Signals and Slots

Qt's Signals and Slots mechanism is thread-safe and is the preferred way to communicate across threads.

- **Auto Connection** (Default): If sender and receiver are in different threads, the signal is queued (asynchronous). If in the same thread, it's a direct call (synchronous).
- **Queued Connection**: Forces asynchronous delivery. Useful when you want to hand off work to the receiver's event loop.
- **Direct Connection**: **Avoid** across threads unless you know exactly what you are doing (requires locking in the slot).

### QObject Parent-Child Relationship

- **Thread Affinity**: All children of a `QObject` must live in the same thread as the parent.
- **Moving Threads**: Use `moveToThread()` to change affinity. Note that you cannot move an object if it has a parent.

## Common Pitfalls

### 1. Blocking the UI Thread

**Don't:**

```cpp
void MainWindow::onOpenDocument() {
    // Heavy operation on UI thread - FREEZES UI
    auto doc = Poppler::Document::load(path);
    renderPage(doc, 0);
}
```

**Do:**

Use `AsyncDocumentLoader` or `QtConcurrent`.

### 2. Accessing UI from Worker Thread

**Don't:**

```cpp
void Worker::process() {
    // Crash or undefined behavior
    mainWindow->statusLabel->setText("Done");
}
```

**Do:**

Emit a signal or use `QMetaObject::invokeMethod`.

```cpp
void Worker::process() {
    emit workDone("Done");
}

// In MainWindow
connect(worker, &Worker::workDone, this, [this](const QString& msg) {
    statusLabel->setText(msg);
});
```

### 3. Shared Mutable State without Locking

**Don't:**

```cpp
// Global or shared cache
QHash<int, QImage> cache;

// Thread 1 writes, Thread 2 reads -> RACE CONDITION
```

**Do:**

Use `QMutex` or `QReadWriteLock`.

```cpp
QMutex cacheMutex;
QHash<int, QImage> cache;

void updateCache(int key, const QImage& img) {
    QMutexLocker locker(&cacheMutex);
    cache.insert(key, img);
}
```

## Debugging Thread Issues

- **TSan (ThreadSanitizer)**: Use Clang/GCC ThreadSanitizer to detect data races.
- **Qt Assertions**: Qt will often warn you in the console if you perform illegal thread operations (e.g., `QTimer` can only be used with threads started with `QThread`).
- **Logging**: Log thread IDs (`LOG_THREAD_ID()`) to verify where code is executing.

## Checklist for New Features

- [ ] Is long-running work offloaded to a background thread?
- [ ] Are UI updates dispatched to the main thread?
- [ ] Is shared state protected by mutexes?
- [ ] Are QObjects created in the correct thread context?
- [ ] Are signals/slots used for cross-thread communication?
