# GDB script to verify DocumentComparison freeze fix
# Usage: gdb -x scripts/verify-freeze-fix.gdb build/Debug-MSYS2/app.exe

set pagination off
set print thread-events off

# Enable logging
set logging file freeze-fix-verification.txt
set logging overwrite on
set logging on

echo \n
echo ========================================\n
echo  Freeze Fix Verification\n
echo ========================================\n
echo \n
echo This script will help verify that the\n
echo DocumentComparison freeze fix works.\n
echo \n
echo Instructions:\n
echo 1. Application will start\n
echo 2. Open two PDF documents\n
echo 3. Go to Tools -> Document Comparison\n
echo 4. Click Compare button\n
echo 5. While comparison is running, press Ctrl+C\n
echo 6. Run: verify-threads\n
echo \n
echo Expected Result:\n
echo - Thread 1 (main) in QEventLoop::exec()\n
echo - Worker thread in compareDocuments()\n
echo - UI should be responsive\n
echo \n
echo ========================================\n
echo \n

# Define verification command
define verify-threads
    echo \n
    echo ========================================\n
    echo  Thread Verification\n
    echo ========================================\n
    echo \n

    echo [1/4] Listing all threads...\n
    info threads
    echo \n

    echo [2/4] Checking main thread (Thread 1)...\n
    thread 1
    bt 10
    echo \n

    echo [3/4] Analyzing main thread state...\n
    set $in_event_loop = 0
    set $in_compare = 0

    # Check if main thread is in event loop (GOOD)
    thread 1
    if $_thread == 1
        # This is a simplified check - in real GDB we'd parse backtrace
        echo Main thread backtrace shown above.\n
        echo \n
        echo EXPECTED (GOOD): Main thread should be in QEventLoop::exec()\n
        echo PROBLEM (BAD):  Main thread in DocumentComparison::compareDocuments()\n
        echo \n
    end

    echo [4/4] Checking for worker thread...\n
    thread apply all bt | grep -i "DocumentComparisonWorker\|compareDocuments"
    echo \n

    echo ========================================\n
    echo  Verification Complete\n
    echo ========================================\n
    echo \n
    echo Review the output above:\n
    echo \n
    echo ✓ PASS if Thread 1 shows QEventLoop::exec()\n
    echo ✓ PASS if worker thread shows compareDocuments()\n
    echo ✗ FAIL if Thread 1 shows compareDocuments()\n
    echo \n
    echo Output saved to: freeze-fix-verification.txt\n
    echo \n
end

# Define quick check command
define quick-check
    echo Quick thread check:\n
    info threads
    echo \n
    echo Main thread backtrace:\n
    thread 1
    bt 5
    echo \n
end

# Set breakpoint at worker start (optional)
# Uncomment to catch when worker starts
# break DocumentComparisonWorker::doComparison
# commands
#     echo \n
#     echo *** Worker thread started comparison! ***\n
#     echo Thread ID:
#     info thread
#     echo \n
#     continue
# end

# Set breakpoint at comparison start (optional)
# Uncomment to catch when comparison is initiated
# break DocumentComparison::startComparison
# commands
#     echo \n
#     echo *** Comparison starting... ***\n
#     echo This should create a worker thread.\n
#     echo \n
#     continue
# end

# Handle Ctrl+C gracefully
handle SIGINT stop print

# Print help
define verify-help
    echo \n
    echo Available commands:\n
    echo   verify-threads  - Full thread verification (recommended)\n
    echo   quick-check     - Quick thread status check\n
    echo   verify-help     - Show this help\n
    echo \n
    echo Usage:\n
    echo 1. Start comparison in the application\n
    echo 2. Press Ctrl+C in GDB while comparison is running\n
    echo 3. Run: (gdb) verify-threads\n
    echo 4. Check if main thread is in event loop\n
    echo \n
end

verify-help

echo Starting application...\n
echo \n
run
