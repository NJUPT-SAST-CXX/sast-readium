# Debugging Application Freeze/Deadlock with GDB

This guide provides systematic steps to debug application freeze/deadlock issues using GDB.

## Prerequisites

- MSYS2 environment with GDB installed
- Debug build of the application (`build/Debug-MSYS2/app.exe`)
- Basic familiarity with GDB commands

## Quick Start

### 1. Launch Application Under GDB

```bash
# In MSYS2 terminal
cd /d/Project/sast-readium
gdb build/Debug-MSYS2/app.exe
```

### 2. Set Up GDB for Qt Debugging

```gdb
# In GDB prompt
(gdb) set print thread-events off
(gdb) set pagination off
(gdb) set logging file gdb-freeze-debug.log
(gdb) set logging on
(gdb) run
```

### 3. Reproduce the Freeze

Once the application starts:

1. Perform the actions that cause the freeze
2. When the application becomes unresponsive, switch to the GDB terminal
3. Press `Ctrl+C` to interrupt the application

### 4. Capture Thread Information

```gdb
# After interrupting with Ctrl+C
(gdb) info threads
(gdb) thread apply all bt
(gdb) thread apply all bt full
```

## Known Freeze Scenarios

Based on code analysis, the following scenarios are known to cause freezes:

### Scenario 1: Document Comparison Freeze

**Location:** `app/ui/dialogs/DocumentComparison.cpp:294-298`

**Issue:** Document comparison runs synchronously on the main thread, blocking the UI.

**Reproduction Steps:**

1. Open the application
2. Load a PDF document
3. Open Tools → Document Comparison
4. Select two documents to compare
5. Click "Compare" button
6. **Expected:** Application freezes during comparison

**GDB Investigation:**

```gdb
# When frozen, check main thread
(gdb) info threads
(gdb) thread 1
(gdb) bt
# Look for: DocumentComparison::startComparison() or DocumentComparison::compareDocuments()
```

### Scenario 2: Large Document Loading

**Location:** `app/model/AsyncDocumentLoader.cpp`

**Issue:** Although mostly fixed, large documents might still timeout or hang.

**Reproduction Steps:**

1. Open a very large PDF (>100MB)
2. Wait for loading
3. If it hangs for >30 seconds, it's likely a timeout issue

**GDB Investigation:**

```gdb
# Check worker thread
(gdb) info threads
# Look for threads in AsyncDocumentLoaderWorker::doLoad()
(gdb) thread <worker_thread_id>
(gdb) bt
```

## Detailed GDB Commands

### Thread Analysis

```gdb
# List all threads
(gdb) info threads

# Switch to specific thread
(gdb) thread <thread_id>

# Get backtrace of current thread
(gdb) bt

# Get backtrace with local variables
(gdb) bt full

# Get backtrace of all threads
(gdb) thread apply all bt

# Get detailed backtrace of all threads
(gdb) thread apply all bt full
```

### Mutex/Lock Analysis

```gdb
# Print mutex state (if you have the mutex variable)
(gdb) print <mutex_variable>

# Check if thread is waiting on mutex
(gdb) info threads
# Look for threads in state "waiting" or "blocked"

# Examine call stack for mutex operations
(gdb) bt
# Look for: QMutex::lock(), QMutexLocker, pthread_mutex_lock
```

### Qt-Specific Debugging

```gdb
# Print QString
(gdb) print <qstring_variable>.d->data()

# Print QObject name
(gdb) print <qobject_variable>->objectName().d->data()

# Check Qt event loop
(gdb) bt
# Look for: QEventLoop::exec(), QCoreApplication::exec()
```

## Analyzing Deadlock Patterns

### Pattern 1: Circular Wait

**Symptoms:**

- Thread A waiting for mutex held by Thread B
- Thread B waiting for mutex held by Thread A

**GDB Analysis:**

```gdb
(gdb) thread apply all bt
# Look for multiple threads in QMutex::lock() or similar
# Check which mutexes each thread is trying to acquire
```

### Pattern 2: Event Loop Blockage

**Symptoms:**

- Main thread stuck in long-running operation
- UI unresponsive but application not crashed

**GDB Analysis:**

```gdb
(gdb) thread 1
(gdb) bt
# Main thread should be in QEventLoop::exec()
# If it's in a different function, that's the blocking operation
```

### Pattern 3: Infinite Loop

**Symptoms:**

- High CPU usage
- Application unresponsive

**GDB Analysis:**

```gdb
(gdb) interrupt
(gdb) bt
# Check if same function appears multiple times in stack
# Or if thread is stuck in a loop (same PC address)
```

## Common Freeze Locations

Based on code analysis, check these locations:

1. **DocumentComparison::compareDocuments()** - Synchronous comparison
2. **AsyncDocumentLoader::doLoad()** - Document loading
3. **SearchEngine** operations - Search in large documents
4. **PDFRenderCache** operations - Rendering cache locks
5. **BackgroundProcessor** - Background task processing

## Generating Debug Report

After capturing the freeze:

```gdb
# Save all thread backtraces
(gdb) set logging file freeze-report.txt
(gdb) set logging on
(gdb) info threads
(gdb) thread apply all bt full
(gdb) set logging off
(gdb) quit
```

## Example Debug Session

```gdb
$ gdb build/Debug-MSYS2/app.exe
(gdb) set pagination off
(gdb) set logging file freeze-debug.log
(gdb) set logging on
(gdb) run

# ... Application starts, reproduce freeze ...
# Press Ctrl+C when frozen

^C
Program received signal SIGINT, Interrupt.
[Switching to Thread 12345.0x6789]
0x00007ff8... in ntdll!ZwWaitForSingleObject ()

(gdb) info threads
  Id   Target Id                                   Frame
* 1    Thread 12345.0x6789 "app.exe"              0x00007ff8... in ntdll!ZwWaitForSingleObject ()
  2    Thread 12345.0x1234 "QThread"              0x00007ff8... in ntdll!ZwWaitForMultipleObjects ()
  3    Thread 12345.0x5678 "QThread"              0x00007ff8... in ntdll!ZwWaitForSingleObject ()

(gdb) thread apply all bt

Thread 1 (Thread 12345.0x6789 "app.exe"):
#0  0x00007ff8... in ntdll!ZwWaitForSingleObject ()
#1  0x00007ff8... in QMutex::lock() ()
#2  0x00000001... in DocumentComparison::compareDocuments() at DocumentComparison.cpp:296
#3  0x00000001... in DocumentComparison::startComparison() at DocumentComparison.cpp:295
...

# This shows the main thread is blocked in DocumentComparison::compareDocuments()
```

## Next Steps After Identifying the Issue

1. **Document the freeze location** - Note the exact function and line number
2. **Analyze the root cause** - Is it a deadlock, blocking operation, or infinite loop?
3. **Implement the fix** - Based on the root cause:
   - Deadlock: Reorder lock acquisition or use try_lock
   - Blocking operation: Move to background thread
   - Infinite loop: Fix loop condition
4. **Test the fix** - Rebuild and verify the freeze is resolved
5. **Add regression test** - Ensure the issue doesn't reoccur

## References

- [GDB Documentation](https://sourceware.org/gdb/documentation/)
- [Qt Debugging Techniques](https://doc.qt.io/qt-6/debug.html)
- [Thread Safety Guidelines](../architecture.md#thread-safety)
