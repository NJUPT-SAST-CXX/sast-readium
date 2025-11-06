# GDB script for debugging application freeze/deadlock
# Usage: gdb -x scripts/debug-freeze.gdb build/Debug-MSYS2/app.exe

# Disable pagination for automated output
set pagination off

# Disable thread event messages for cleaner output
set print thread-events off

# Enable logging
set logging file freeze-debug-output.txt
set logging overwrite on
set logging on

# Print startup message
echo \n
echo ========================================\n
echo  SAST Readium Freeze Debugging Session\n
echo ========================================\n
echo \n
echo Instructions:\n
echo 1. Application will start automatically\n
echo 2. Reproduce the freeze condition\n
echo 3. Press Ctrl+C when application freezes\n
echo 4. GDB will capture thread information\n
echo \n
echo Known freeze scenarios:\n
echo - Document Comparison (Tools -> Document Comparison)\n
echo - Large PDF loading (>100MB files)\n
echo - Intensive search operations\n
echo \n
echo ========================================\n
echo \n

# Define custom commands for freeze analysis

define freeze-analyze
    echo \n
    echo ========================================\n
    echo  Freeze Analysis Started\n
    echo ========================================\n
    echo \n

    echo [1/5] Capturing thread list...\n
    info threads
    echo \n

    echo [2/5] Capturing all thread backtraces...\n
    thread apply all bt
    echo \n

    echo [3/5] Capturing detailed backtraces with locals...\n
    thread apply all bt full
    echo \n

    echo [4/5] Analyzing main thread (Thread 1)...\n
    thread 1
    bt full
    echo \n

    echo [5/5] Checking for common freeze patterns...\n
    freeze-check-patterns
    echo \n

    echo ========================================\n
    echo  Freeze Analysis Complete\n
    echo  Output saved to: freeze-debug-output.txt\n
    echo ========================================\n
    echo \n
end

define freeze-check-patterns
    echo Checking for known freeze patterns:\n
    echo \n

    # Check for DocumentComparison freeze
    echo Pattern 1: DocumentComparison synchronous operation\n
    thread apply all bt | grep -i "DocumentComparison::compareDocuments"
    echo \n

    # Check for mutex deadlock
    echo Pattern 2: Mutex deadlock (threads waiting on locks)\n
    thread apply all bt | grep -i "QMutex::lock\|pthread_mutex_lock"
    echo \n

    # Check for event loop blockage
    echo Pattern 3: Event loop blockage (main thread not in event loop)\n
    thread 1
    bt | grep -i "QEventLoop::exec\|QCoreApplication::exec"
    echo \n

    # Check for AsyncDocumentLoader issues
    echo Pattern 4: AsyncDocumentLoader timeout/hang\n
    thread apply all bt | grep -i "AsyncDocumentLoader\|Poppler::Document::load"
    echo \n

    # Check for search operations
    echo Pattern 5: Search operation freeze\n
    thread apply all bt | grep -i "SearchEngine\|BackgroundProcessor"
    echo \n
end

define freeze-quick
    echo Quick freeze analysis (threads only):\n
    info threads
    echo \n
    thread apply all bt
end

# Set breakpoints for known problematic areas (optional)
# Uncomment these if you want to catch the freeze before it happens

# break DocumentComparison::startComparison
# commands
#     echo WARNING: Entering DocumentComparison::startComparison (known freeze point)\n
#     continue
# end

# break AsyncDocumentLoaderWorker::doLoad
# commands
#     echo INFO: Starting document load in worker thread\n
#     continue
# end

# Handle SIGINT (Ctrl+C) gracefully
handle SIGINT stop print

# Print help message
define freeze-help
    echo \n
    echo Available custom commands:\n
    echo   freeze-analyze        - Full freeze analysis (recommended)\n
    echo   freeze-quick          - Quick thread analysis\n
    echo   freeze-check-patterns - Check for known freeze patterns\n
    echo   freeze-help           - Show this help message\n
    echo \n
    echo After pressing Ctrl+C when frozen, run:\n
    echo   (gdb) freeze-analyze\n
    echo \n
end

# Print help on startup
freeze-help

# Start the application
echo Starting application...\n
echo \n
run
