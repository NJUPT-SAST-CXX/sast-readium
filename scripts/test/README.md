# Testing Scripts

Scripts for running tests and generating test utilities.

## Scripts

### run-tests.ps1 (Windows PowerShell)

Full-featured test runner with coverage support.

```powershell
# Run all tests
.\scripts\test\run-tests.ps1

# Run specific test type
.\scripts\test\run-tests.ps1 -TestType Unit

# Run with coverage
.\scripts\test\run-tests.ps1 -Coverage

# Generate HTML report
.\scripts\test\run-tests.ps1 -HtmlReport
```

### run-tests.sh (MSYS2)

Bash test runner for MSYS2 environment.

```bash
./scripts/test/run-tests.sh
```

### run-tests-unix.sh (Linux/macOS)

Full-featured test runner for Unix systems with coverage support.

```bash
# Run all tests
./scripts/test/run-tests-unix.sh

# Run unit tests with coverage
./scripts/test/run-tests-unix.sh -T Unit -c --html

# Run with verbose output
./scripts/test/run-tests-unix.sh -v

# Stop on first failure
./scripts/test/run-tests-unix.sh -s
```

**Options:**

- `-t, --type`: Build type (Debug, Release)
- `-T, --test-type`: Test type (All, Unit, Integration, Smoke)
- `-c, --coverage`: Generate code coverage report
- `-j, --jobs`: Number of parallel test jobs
- `-s, --stop-on-fail`: Stop on first failure
- `--html`: Generate HTML test report

### run-tests.bat / run-tests-debug.bat (Windows Batch)

Batch test runners for Windows without PowerShell.

```batch
REM Run all tests
scripts\test\run-tests.bat

REM Run Debug-Windows tests
scripts\test\run-tests-debug.bat --phase unit
```

### run-qgraphics-tests.py

Python script for comprehensive QGraphics PDF testing.

```bash
# Run all configurations
python scripts/test/run-qgraphics-tests.py

# Run with specific build system
python scripts/test/run-qgraphics-tests.py --build-system cmake

# Run with specific configuration
python scripts/test/run-qgraphics-tests.py --config Debug-Unix
```

### generate-test-stubs.py

Generate stub implementations for missing test methods.

```bash
python scripts/test/generate-test-stubs.py tests/cache/test_cache_manager.cpp
```

### ui-test-helper.ps1

Helper script for manual UI testing.

```powershell
# Setup screenshot directories
.\scripts\test\ui-test-helper.ps1 -SetupScreenshots

# Launch application for testing
.\scripts\test\ui-test-helper.ps1 -LaunchApp

# Generate test report template
.\scripts\test\ui-test-helper.ps1 -GenerateReport
```
