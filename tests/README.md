# SAST-Readium Test Framework Documentation

## Overview

The SAST-Readium project uses a comprehensive test framework built on Qt Test with custom extensions for modular architecture testing. This framework provides standardized testing patterns, mock objects, performance benchmarking, and integration with the new service-oriented architecture.

## Table of Contents

1. [Test Framework Architecture](#test-framework-architecture)
2. [Test Categories](#test-categories)
3. [Writing Tests](#writing-tests)
4. [Running Tests](#running-tests)
5. [Test Utilities](#test-utilities)
6. [Mock Objects](#mock-objects)
7. [Performance Testing](#performance-testing)
8. [Coverage Analysis](#coverage-analysis)
9. [Best Practices](#best-practices)

## Test Framework Architecture

### Core Components

- **TestBase**: Base class for all tests providing common functionality
- **TestUtilities**: Helper functions and macros for testing
- **MockObject**: Framework for creating mock objects
- **Service Integration**: Testing with ServiceLocator, StateManager, and EventBus

### Directory Structure

```
tests/
├── TestUtilities.h/cpp      # Base test framework
├── MockObject.h/cpp          # Mock object framework
├── unit/                     # Unit tests
│   ├── test_search_*.cpp
│   ├── test_error_*.cpp
│   └── test_qgraphics_*.cpp
├── integration/              # Integration tests
│   ├── test_search_integration.cpp
│   └── test_rendering_mode_switch.cpp
├── command/                  # Command pattern tests
│   └── InitializationCommandTest.cpp
├── controller/               # Controller tests
│   ├── ServiceLocatorTest.cpp
│   └── StateManagerTest.cpp
├── factory/                  # Factory pattern tests
│   └── ModelFactoryTest.cpp
├── performance/              # Performance tests
│   ├── test_rendering_performance.cpp
│   └── test_search_optimizations.cpp
├── real_world/              # Real-world scenario tests
│   └── test_real_pdf_documents.cpp
├── scripts/                  # Test scripts
│   ├── run_tests.ps1
│   └── migrate_tests.py
└── CMakeLists.txt           # Build configuration
```

## Test Categories

### 1. Unit Tests
Tests individual components in isolation.

```cpp
class TestSearchEngine : public TestBase {
    Q_OBJECT
private slots:
    void testBasicSearch();
    void testCaseSensitiveSearch();
    void testRegexSearch();
};
```

### 2. Integration Tests
Tests interaction between multiple components.

```cpp
class TestSearchIntegration : public TestBase {
    Q_OBJECT
private slots:
    void testSearchWithServiceLocator();
    void testSearchWithStateManager();
    void testSearchWithEventBus();
};
```

### 3. Performance Tests
Benchmarks and performance measurements.

```cpp
class TestRenderingPerformance : public TestBase {
    Q_OBJECT
private slots:
    void benchmarkRenderingPipeline();
    void testMemoryUsageComparison();
    void testConcurrentRendering();
};
```

### 4. Controller Tests
Tests for service-oriented architecture components.

```cpp
class TestStateManager : public TestBase {
    Q_OBJECT
private slots:
    void testStateOperations();
    void testSubscriptions();
    void testTransactions();
};
```

## Writing Tests

### Basic Test Structure

```cpp
#include "../TestUtilities.h"
#include "../../app/ComponentToTest.h"

class TestComponent : public TestBase {
    Q_OBJECT

private slots:
    // Qt Test lifecycle methods
    void initTestCase() override;    // Run once before all tests
    void cleanupTestCase() override; // Run once after all tests
    void init() override;             // Run before each test
    void cleanup() override;          // Run after each test
    
    // Test methods
    void testFeatureOne();
    void testFeatureTwo();
    
private:
    // Test data and helpers
    ComponentToTest* m_component;
};

void TestComponent::initTestCase() {
    // Global setup
    setupServices();
}

void TestComponent::init() {
    // Per-test setup
    m_component = new ComponentToTest();
}

void TestComponent::cleanup() {
    // Per-test cleanup
    delete m_component;
}

void TestComponent::testFeatureOne() {
    // Arrange
    m_component->setValue(42);
    
    // Act
    int result = m_component->getValue();
    
    // Assert
    QCOMPARE(result, 42);
}

QTEST_MAIN(TestComponent)
#include "test_component.moc"
```

### Using Test Utilities

```cpp
void TestComponent::testAsyncOperation() {
    QSignalSpy spy(m_component, &Component::finished);
    
    m_component->startAsync();
    
    // Wait for completion with timeout
    QVERIFY_TIMEOUT(spy.count() > 0, 5000);
    
    // Process events
    processEvents();
    
    // Wait specific time
    waitMs(100);
}
```

### Service Integration

```cpp
void TestComponent::testWithServices() {
    // Use ServiceLocator
    auto* service = ServiceLocator::instance().getService<MyService>();
    QVERIFY(service != nullptr);
    
    // Use StateManager
    StateManager::instance().set("test.value", 42);
    QCOMPARE(StateManager::instance().get("test.value").toInt(), 42);
    
    // Use EventBus
    bool received = false;
    EventBus::instance().subscribe("test.event", this, [&received](Event* e) {
        received = true;
    });
    
    EventBus::instance().publish("test.event", QVariant());
    waitMs(10);
    QVERIFY(received);
}
```

## Running Tests

### Using PowerShell Script

```powershell
# Run all tests
.\scripts\run_tests.ps1

# Run specific test category
.\scripts\run_tests.ps1 -TestType Unit

# Run with coverage
.\scripts\run_tests.ps1 -Coverage

# Run with HTML report
.\scripts\run_tests.ps1 -Report

# Run tests in parallel
.\scripts\run_tests.ps1 -Parallel

# Run with all options
.\scripts\run_tests.ps1 -TestType All -Coverage -Report -Parallel -Verbose
```

### Using CMake/CTest

```bash
# Build tests
cmake -B build -DBUILD_TESTS=ON
cmake --build build

# Run all tests
ctest --test-dir build --output-on-failure

# Run specific test
ctest --test-dir build -R TestSearchEngine

# Run with verbose output
ctest --test-dir build -V

# Run in parallel
ctest --test-dir build -j4
```

### Using Test Migration Script

```bash
# Migrate existing tests to new framework
python scripts/migrate_tests.py tests/

# Dry run to see what would change
python scripts/migrate_tests.py tests/ --dry-run

# Update CMake files too
python scripts/migrate_tests.py tests/ --update-cmake
```

## Test Utilities

### Macros and Functions

```cpp
// Timeout assertions
QVERIFY_TIMEOUT(condition, timeoutMs)

// Wait functions
waitMs(100);                    // Wait 100ms
processEvents();                 // Process Qt event loop
waitForSignal(obj, signal);     // Wait for specific signal

// Performance measurement
QElapsedTimer timer;
timer.start();
// ... operation ...
qint64 elapsed = timer.elapsed();

// Memory tracking
size_t startMem = getCurrentMemoryUsage();
// ... operation ...
size_t memUsed = getCurrentMemoryUsage() - startMem;
```

## Mock Objects

### Creating Mocks

```cpp
class MockService : public MockObject, public IService {
public:
    MockService() : MockObject("MockService") {}
    
    // Mock method with recording
    void doSomething(int value) override {
        recordCall("doSomething", QVariantList{value});
        if (shouldFail) {
            throw std::runtime_error("Mock failure");
        }
    }
    
    // Mock with return value
    int getValue() override {
        recordCall("getValue");
        return mockReturnValue;
    }
    
    // Test configuration
    bool shouldFail = false;
    int mockReturnValue = 42;
};

// Using in tests
void TestComponent::testWithMock() {
    MockService mock;
    
    // Configure mock
    mock.mockReturnValue = 100;
    
    // Use mock
    m_component->setService(&mock);
    m_component->execute();
    
    // Verify interactions
    QCOMPARE(mock.callCount("getValue"), 1);
    QVERIFY(mock.wasCalledWith("doSomething", QVariantList{42}));
}
```

## Performance Testing

### Benchmarking

```cpp
void TestPerformance::benchmarkOperation() {
    const int iterations = 1000;
    
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < iterations; ++i) {
        performOperation();
    }
    
    qint64 totalTime = timer.elapsed();
    double avgTime = (double)totalTime / iterations;
    
    qDebug() << "Total time:" << totalTime << "ms";
    qDebug() << "Average:" << avgTime << "ms";
    qDebug() << "Throughput:" << 1000.0 / avgTime << "ops/sec";
    
    // Performance assertion
    QVERIFY(avgTime < 10); // Less than 10ms average
}
```

### Memory Profiling

```cpp
void TestPerformance::testMemoryUsage() {
    // Measure memory for operation
    size_t baseline = getCurrentMemoryUsage();
    
    performOperation();
    
    size_t peak = getCurrentMemoryUsage();
    size_t used = peak - baseline;
    
    qDebug() << "Memory used:" << used / (1024 * 1024) << "MB";
    
    // Memory assertion
    QVERIFY(used < 100 * 1024 * 1024); // Less than 100MB
}
```

### Concurrent Testing

```cpp
void TestPerformance::testConcurrency() {
    const int numThreads = 10;
    
    QList<QFuture<void>> futures;
    
    for (int i = 0; i < numThreads; ++i) {
        futures.append(QtConcurrent::run([this]() {
            performThreadSafeOperation();
        }));
    }
    
    for (auto& future : futures) {
        future.waitForFinished();
    }
    
    // Verify thread safety
    QVERIFY(isConsistent());
}
```

## Coverage Analysis

### Enabling Coverage

```cmake
# In CMakeLists.txt
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(--coverage)
    add_link_options(--coverage)
endif()
```

### Running with Coverage

```powershell
# Build with coverage
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
cmake --build build

# Run tests with coverage
.\scripts\run_tests.ps1 -Coverage

# Generate HTML report
lcov --capture --directory build --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

## Best Practices

### 1. Test Organization
- One test class per component
- Group related tests in same file
- Use descriptive test names
- Follow AAA pattern (Arrange, Act, Assert)

### 2. Test Independence
- Each test should be independent
- Use init() and cleanup() for setup/teardown
- Don't rely on test execution order
- Clean up resources properly

### 3. Performance Tests
- Use warm-up iterations
- Measure multiple runs
- Report statistics (min, max, avg, stddev)
- Set reasonable performance targets

### 4. Mock Usage
- Mock external dependencies
- Verify mock interactions
- Reset mocks between tests
- Use dependency injection

### 5. Async Testing
- Use QSignalSpy for signals
- Set reasonable timeouts
- Process events when needed
- Handle race conditions

### 6. Error Testing
- Test error conditions
- Verify error messages
- Test recovery paths
- Check resource cleanup

### 7. Documentation
- Document test purpose
- Explain complex test logic
- Note any special requirements
- Update when code changes

## Troubleshooting

### Common Issues

1. **Tests not found**
   ```bash
   # Ensure tests are built
   cmake --build build --target test_name
   ```

2. **Timeout failures**
   ```cpp
   // Increase timeout for slow operations
   QVERIFY_TIMEOUT(condition, 10000); // 10 seconds
   ```

3. **Flaky tests**
   ```cpp
   // Add retry logic for network/async operations
   for (int i = 0; i < 3; ++i) {
       if (tryOperation()) break;
       waitMs(100);
   }
   ```

4. **Memory leaks**
   ```cpp
   // Use QPointer to track object lifetime
   QPointer<QObject> ptr = object;
   // ... operations ...
   QVERIFY(ptr.isNull()); // Verify deleted
   ```

## Contributing

When adding new tests:

1. Follow existing patterns
2. Add to appropriate category
3. Update CMakeLists.txt
4. Document special requirements
5. Ensure tests pass locally
6. Check coverage metrics

## Support

For questions or issues with the test framework:

1. Check this documentation
2. Review existing test examples
3. Run tests with verbose output
4. Check test logs in build/Testing/

---

*Last updated: 2024*
*Test Framework Version: 2.0*
