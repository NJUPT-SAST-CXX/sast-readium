# Enhanced PDF Search Engine Test Suite

This comprehensive test suite validates the enhanced PDF search engine functionality, covering all aspects from basic search operations to advanced features and performance optimizations.

## Test Structure

### 📁 Test Categories

#### 1. **Unit Tests** (`tests/unit/`)
- **`test_search_engine_core.cpp`** - Core search functionality tests
- **`test_search_advanced_features.cpp`** - Advanced search features tests  
- **`test_search_performance_caching.cpp`** - Performance and caching tests
- **`test_search_edge_cases.cpp`** - Edge cases and error handling tests

#### 2. **Integration Tests** (`tests/integration/`)
- **`test_search_integration.cpp`** - Component integration tests

#### 3. **Performance Tests** (`tests/performance/`)
- **`test_search_optimizations.cpp`** - Search optimization benchmarks

## 🧪 Test Coverage

### Core Search Functionality Tests
Tests basic search operations, regex patterns, case sensitivity, and word matching:

- ✅ **Basic Text Search**: Simple word searches with multiple results
- ✅ **Case Sensitivity**: Case-sensitive vs case-insensitive search modes
- ✅ **Whole Word Matching**: Exact word boundaries vs partial matches
- ✅ **Regex Patterns**: Basic and complex regular expression searches
- ✅ **Search Result Validation**: Accuracy, completeness, coordinates, and context

**Key Test Methods:**
```cpp
void testBasicTextSearch();
void testCaseSensitiveSearch();
void testWholeWordMatching();
void testBasicRegexPatterns();
void testSearchResultAccuracy();
```

### Advanced Search Features Tests
Tests fuzzy search, Levenshtein distance, page range search, and search history:

- ✅ **Fuzzy Search**: Approximate text matching with configurable thresholds
- ✅ **Levenshtein Distance**: Edit distance calculations for fuzzy matching
- ✅ **Page Range Search**: Searching within specific page ranges
- ✅ **Search History**: History management with LRU eviction

**Key Test Methods:**
```cpp
void testFuzzySearchWithDifferentThresholds();
void testLevenshteinDistanceCalculation();
void testPageRangeSearchValid();
void testSearchHistoryManagement();
```

### Performance and Caching Tests
Tests search result caching, incremental search, background operations, and memory usage:

- ✅ **Search Result Caching**: Cache hit/miss scenarios and eviction policies
- ✅ **Incremental Search**: Progressive query refinement
- ✅ **Background Search**: Asynchronous search operations
- ✅ **Memory Management**: Memory usage monitoring and cleanup
- ✅ **Performance Benchmarks**: Speed comparisons and optimizations

**Key Test Methods:**
```cpp
void testSearchResultCaching();
void testIncrementalSearchPerformance();
void testBackgroundSearchOperations();
void benchmarkCachedVsUncachedSearch();
```

### Integration Tests
Tests integration between SearchModel, SearchWidget, OptimizedSearchEngine, and PDFViewer:

- ✅ **Component Integration**: Cross-component communication
- ✅ **Search Highlighting**: Color customization and visual feedback
- ✅ **Virtual Scrolling Integration**: Compatibility with existing optimizations
- ✅ **End-to-End Workflows**: Complete user interaction flows

**Key Test Methods:**
```cpp
void testSearchModelEngineIntegration();
void testSearchHighlightingColorCustomization();
void testEndToEndSearchFlow();
void testSearchWithExistingOptimizations();
```

### Edge Cases and Error Handling Tests
Tests with empty documents, malformed PDFs, invalid parameters, and error scenarios:

- ✅ **Empty/Malformed Documents**: Graceful handling of problematic PDFs
- ✅ **Invalid Queries**: Empty, whitespace, and special character queries
- ✅ **Parameter Validation**: Invalid page ranges, thresholds, and options
- ✅ **Timeout/Cancellation**: Search interruption and cleanup
- ✅ **Resource Limits**: Large documents and memory constraints

**Key Test Methods:**
```cpp
void testEmptyDocumentSearch();
void testInvalidRegexPatterns();
void testSearchTimeout();
void testLargeDocumentHandling();
```

## 🚀 Running the Tests

### Prerequisites
1. **Build the project** with CMake:
   ```bash
   mkdir build && cd build
   cmake .. -DENABLE_QGRAPHICS_PDF_SUPPORT=ON
   cmake --build . --target all
   ```

2. **Build the tests**:
   ```bash
   cmake --build . --target run_all_tests
   ```

### Running Individual Tests

#### Core Search Tests
```bash
# Windows
./build/Debug-MSYS2/tests/unit/test_search_engine_core.exe

# Linux/macOS
./build/tests/unit/test_search_engine_core
```

#### Advanced Features Tests
```bash
./build/Debug-MSYS2/tests/unit/test_search_advanced_features.exe
```

#### Performance Tests
```bash
./build/Debug-MSYS2/tests/unit/test_search_performance_caching.exe
```

#### Integration Tests
```bash
./build/Debug-MSYS2/tests/integration/test_search_integration.exe
```

#### Edge Cases Tests
```bash
./build/Debug-MSYS2/tests/unit/test_search_edge_cases.exe
```

### Running All Search Tests

#### Using Python Test Runner (Recommended)
```bash
cd tests
python run_search_tests.py --build-dir ../build/Debug-MSYS2
```

#### Using CTest
```bash
cd build
ctest -R "test_search" --output-on-failure
```

### Performance Benchmarks Only
```bash
python tests/run_search_tests.py --benchmarks-only
```

## 📊 Test Reports

The test runner generates comprehensive reports:

### JSON Report (`search_test_report.json`)
```json
{
  "total_tests": 25,
  "passed_tests": 24,
  "failed_tests": 1,
  "success_rate": 96.0,
  "total_duration": 45.2,
  "suites": {
    "Core Search Functionality": {
      "passed": 8,
      "total": 8,
      "success_rate": 100.0
    }
  }
}
```

### Console Output
```
===============================================================================
ENHANCED PDF SEARCH ENGINE TEST SUMMARY
===============================================================================
Total Tests: 25
Passed: 24
Failed: 1
Success Rate: 96.0%
Total Duration: 45.20 seconds

Test Suite Results:           Passed   Total    Rate    
------------------------------------------------------------
Core Search Functionality    8        8        100.0%
Advanced Search Features     6        6        100.0%
Performance and Caching      5        5        100.0%
Integration Tests           4        4        100.0%
Edge Cases and Error        1        2        50.0%
```

## 🔧 Test Configuration

### Environment Variables
- `QT_QPA_PLATFORM=offscreen` - Headless testing
- `SEARCH_TEST_TIMEOUT=300` - Test timeout in seconds
- `SEARCH_TEST_VERBOSE=1` - Verbose output

### CMake Options
- `ENABLE_QGRAPHICS_PDF_SUPPORT=ON` - Enable PDF support
- `BUILD_TESTING=ON` - Enable test building

## 📈 Performance Benchmarks

The test suite includes performance benchmarks that measure:

### Search Speed Metrics
- **Basic Search**: < 50ms average per search
- **Fuzzy Search**: < 200ms average per search  
- **Large Document**: < 10 seconds for 50+ pages
- **Cache Performance**: 5-10x speedup for repeated searches

### Memory Usage Metrics
- **Cache Memory**: Monitored and limited
- **Search Results**: Memory cleanup verification
- **Large Documents**: Memory efficiency validation

### Example Benchmark Output
```
Basic search benchmark: 100 searches in 2341ms, average: 23.4ms per search
Fuzzy search benchmark: 50 searches in 4567ms, average: 91.3ms per search
Cache performance benchmark - Uncached: 1234ms, Cached: 156ms, Speedup: 7.9x
Large document search: found 847 results in 3456ms for 50 pages
```

## 🐛 Troubleshooting

### Common Issues

#### Test Executable Not Found
```bash
Error: Test executable not found: build/tests/unit/test_search_engine_core.exe
```
**Solution**: Build the tests first with `cmake --build . --target all`

#### Qt Platform Plugin Error
```bash
qt.qpa.plugin: Could not find the Qt platform plugin "windows"
```
**Solution**: Set `QT_QPA_PLATFORM=offscreen` environment variable

#### Test Timeout
```bash
Test timed out after 5 minutes
```
**Solution**: Increase timeout or check for infinite loops in search logic

### Debug Mode
Run tests with verbose output:
```bash
python run_search_tests.py --verbose
```

## 📝 Adding New Tests

### Creating a New Test File
1. Create test file in appropriate directory (`unit/`, `integration/`, `performance/`)
2. Follow Qt Test framework conventions
3. Add to `CMakeLists.txt`
4. Update test runner script

### Test Template
```cpp
#include <QtTest/QtTest>
#include "../../app/search/OptimizedSearchEngine.h"

class TestNewFeature : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testNewFeature();
    void cleanupTestCase();

private:
    OptimizedSearchEngine* m_engine;
};

void TestNewFeature::testNewFeature()
{
    // Test implementation
    QVERIFY(true);
}

QTEST_MAIN(TestNewFeature)
#include "test_new_feature.moc"
```

## 🎯 Test Quality Metrics

- **Code Coverage**: Aims for >90% coverage of search components
- **Test Reliability**: All tests should pass consistently
- **Performance Regression**: Benchmarks detect performance degradation
- **Error Handling**: Comprehensive edge case coverage

## 📚 Related Documentation

- [PDF Rendering Optimizations](../docs/pdf_optimizations.md)
- [Search Engine Architecture](../docs/search_architecture.md)
- [Performance Tuning Guide](../docs/performance_tuning.md)
