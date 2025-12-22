# Debugging Scripts

GDB scripts for debugging application freezes and deadlocks.

## Scripts

### debug-freeze.gdb

GDB script for diagnosing application freezes.

```bash
gdb -x scripts/debug/debug-freeze.gdb build/Debug-MSYS2/app/sast-readium.exe
```

**Commands available in GDB:**

- `freeze-analyze` - Full thread analysis with backtraces
- `freeze-quick` - Quick thread status check
- `freeze-check-patterns` - Check for common freeze patterns

### verify-freeze-fix.gdb

GDB script for verifying DocumentComparison freeze fix.

```bash
gdb -x scripts/debug/verify-freeze-fix.gdb build/Debug-MSYS2/app/sast-readium.exe
```

**Commands available in GDB:**

- `verify-threads` - Full thread verification
- `quick-check` - Quick thread status
- `verify-help` - Show available commands

## Usage

1. Start the application under GDB
2. Reproduce the freeze condition
3. Press Ctrl+C to interrupt
4. Run the diagnostic commands
5. Review the output in the log file
