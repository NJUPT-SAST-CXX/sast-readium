# SAST Readium Logging System

## Overview

SAST Readium uses a modern logging system based on [spdlog](https://github.com/gabime/spdlog) with seamless Qt integration. This system provides high-performance, thread-safe logging with multiple output destinations and runtime configuration capabilities.

## Key Features

- **High Performance**: Built on spdlog, one of the fastest C++ logging libraries
- **Qt Integration**: Seamless integration with Qt's logging system (qDebug, qWarning, etc.)
- **Multiple Sinks**: Console, file, rotating file, and Qt widget output
- **Runtime Configuration**: Change log levels and settings without restarting
- **Thread Safety**: Safe to use from multiple threads
- **Structured Logging**: Support for formatted messages and structured data
- **Performance Monitoring**: Built-in performance and memory tracking

## Quick Start

### Basic Usage

```cpp
#include "utils/LoggingMacros.h"

// Simple logging
LOG_INFO("Application started");
LOG_WARNING("This is a warning message");
LOG_ERROR("An error occurred");

// Formatted logging
LOG_INFO("User {} logged in with ID {}", username, userId);
LOG_DEBUG("Processing {} items in {} seconds", count, duration);
```

### Initialization

```cpp
#include "utils/LoggingManager.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Initialize logging system
    LoggingManager::instance().initialize();

    // Your application code here

    // Shutdown logging system
    LoggingManager::instance().shutdown();
    return app.exec();
}
```

## Logging Levels

The system supports six logging levels:

1. **TRACE**: Very detailed information, typically only of interest when diagnosing problems
2. **DEBUG**: Detailed information, typically only of interest when diagnosing problems
3. **INFO**: General information about program execution
4. **WARNING**: Something unexpected happened, but the application can continue
5. **ERROR**: A serious problem occurred, but the application can continue
6. **CRITICAL**: A very serious error occurred, application may not be able to continue

### Usage Examples

```cpp
LOG_TRACE("Entering function: {}", __FUNCTION__);
LOG_DEBUG("Variable value: x = {}", x);
LOG_INFO("File loaded successfully: {}", filename);
LOG_WARNING("Deprecated function used: {}", __FUNCTION__);
LOG_ERROR("Failed to open file: {}", filename);
LOG_CRITICAL("Out of memory, cannot continue");
```

## Configuration

### Development Configuration

```cpp
LoggingConfigBuilder configBuilder;
configBuilder.useDevelopmentPreset()
             .setGlobalLevel(Logger::LogLevel::Debug)
             .addConsoleSink("console", Logger::LogLevel::Debug)
             .addFileSink("file", "logs/debug.log", Logger::LogLevel::Info);

LoggingManager::instance().initialize(configBuilder.build());
```

### Production Configuration

```cpp
LoggingConfigBuilder configBuilder;
configBuilder.useProductionPreset()
             .setGlobalLevel(Logger::LogLevel::Warning)
             .addFileSink("file", "logs/app.log", Logger::LogLevel::Warning);

LoggingManager::instance().initialize(configBuilder.build());
```

### Runtime Configuration Changes

```cpp
// Change global log level
LoggingManager::instance().setGlobalLogLevel(Logger::LogLevel::Debug);

// Enable/disable specific sinks
LoggingManager::instance().enableConsoleLogging(Logger::LogLevel::Info);
LoggingManager::instance().enableFileLogging("logs/runtime.log", Logger::LogLevel::Warning);
```

## Qt Integration

The logging system automatically redirects Qt's logging functions to spdlog:

```cpp
// These Qt logging calls are automatically redirected to spdlog
qDebug() << "Qt debug message";
qInfo() << "Qt info message";
qWarning() << "Qt warning message";
qCritical() << "Qt critical message";
```

### Qt Logging Categories

For compatibility with existing Qt logging categories:

```cpp
// Old Qt way
Q_DECLARE_LOGGING_CATEGORY(lcPdfRender)
Q_LOGGING_CATEGORY(lcPdfRender, "pdf.render")

// New spdlog way
Q_DECLARE_SPDLOG_CATEGORY(lcPdfRender)
Q_SPDLOG_CATEGORY(lcPdfRender, "pdf.render")

// Usage remains the same
lcPdfRender().debug() << "PDF rendering started";
```

## Advanced Features

### Performance Monitoring

```cpp
// Automatic performance logging
{
    LOG_PERFORMANCE_SCOPE("expensive_operation");
    // Your expensive operation here
} // Automatically logs execution time

// Manual performance tracking
LOG_PERFORMANCE_START(my_operation);
// Your operation here
LOG_PERFORMANCE_END(my_operation, "Operation completed");
```

### Memory Tracking

```cpp
// Start memory tracking
MemoryLogger::startMemoryTracking("my_context");

// Your memory-intensive operation
std::vector<int> data(1000000);

// Log memory delta
MemoryLogger::logMemoryDelta("my_context");
MemoryLogger::endMemoryTracking("my_context");
```

### Conditional Logging

```cpp
LOG_DEBUG_IF(condition, "Debug message only if condition is true");
LOG_ERROR_IF(!success, "Operation failed with error code: {}", errorCode);
```

### Thread-Safe Logging

```cpp
// Log with thread information
LOG_WITH_THREAD(INFO, "Message from thread");
LOG_THREAD_ID(); // Log current thread ID
```

## Best Practices

### 1. Use Appropriate Log Levels

- Use **TRACE** for very detailed debugging information
- Use **DEBUG** for general debugging information
- Use **INFO** for important application events
- Use **WARNING** for unexpected but recoverable situations
- Use **ERROR** for errors that don't stop the application
- Use **CRITICAL** for fatal errors

### 2. Use Structured Logging

```cpp
// Good: Structured and searchable
LOG_INFO("User login: user={}, ip={}, success={}", username, ipAddress, success);

// Avoid: Unstructured text
LOG_INFO("User " + username + " from " + ipAddress + " login " + (success ? "successful" : "failed"));
```

### 3. Avoid Expensive Operations in Log Statements

```cpp
// Good: Only evaluate if logging level is enabled
if (Logger::instance().getSpdlogLogger()->should_log(spdlog::level::debug)) {
    LOG_DEBUG("Expensive operation result: {}", expensiveFunction());
}

// Better: Use conditional logging macros
LOG_DEBUG_IF(condition, "Message: {}", expensiveFunction());
```

### 4. Use Categories for Different Components

```cpp
// Define categories for different parts of your application
DEFINE_LOG_CATEGORY(UI, "ui");
DEFINE_LOG_CATEGORY(NETWORK, "network");
DEFINE_LOG_CATEGORY(DATABASE, "database");

// Use categories in logging
LOG_CATEGORY_INFO(UI, "Window opened: {}", windowTitle);
LOG_CATEGORY_ERROR(NETWORK, "Connection failed: {}", errorMessage);
```

## Configuration Files

### JSON Configuration

```json
{
  "global": {
    "globalLevel": "info",
    "globalPattern": "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v",
    "asyncLogging": false,
    "redirectQtMessages": true
  },
  "sinks": [
    {
      "name": "console",
      "type": "console",
      "level": "debug",
      "enabled": true,
      "colorEnabled": true
    },
    {
      "name": "file",
      "type": "rotating_file",
      "level": "info",
      "enabled": true,
      "filename": "logs/sast-readium.log",
      "maxFileSize": 10485760,
      "maxFiles": 5
    }
  ]
}
```

### Environment Variables

You can override logging configuration using environment variables:

```bash
export SAST_READIUM_LOG_LEVEL=debug
export SAST_READIUM_LOG_CONSOLE=true
export SAST_READIUM_LOG_FILE_PATH=/custom/path/app.log
```

## Troubleshooting

### Common Issues

**Q: Logging macros not recognized**
A: Make sure to include `"utils/LoggingMacros.h"` in your source files.

**Q: Qt logging not redirected**
A: Ensure `redirectQtMessages` is enabled in your configuration and the QtSpdlogBridge is initialized.

**Q: Log files not created**
A: Check file permissions and ensure the log directory exists.

**Q: Performance issues with logging**
A: Enable async logging for high-throughput scenarios or increase the log level to reduce output.

### Debug Information

To enable verbose logging for the logging system itself:

```cpp
LoggingManager::instance().setGlobalLogLevel(Logger::LogLevel::Trace);
```

## Migration from Qt Logging

If you're migrating from Qt's built-in logging system:

1. Replace `#include <QDebug>` with `#include "utils/LoggingMacros.h"`
2. Replace `qDebug()` with `LOG_DEBUG()`
3. Replace `qWarning()` with `LOG_WARNING()`
4. Replace `qCritical()` with `LOG_CRITICAL()`
5. Use formatted logging instead of stream operators

### Before (Qt logging)
```cpp
qDebug() << "Processing file:" << filename << "size:" << size;
```

### After (spdlog)
```cpp
LOG_DEBUG("Processing file: {} size: {}", filename, size);
```

## Performance Considerations

- spdlog is significantly faster than Qt's logging system
- Async logging can improve performance in high-throughput scenarios
- Use appropriate log levels to avoid unnecessary overhead
- Consider using conditional logging for expensive operations

## Thread Safety

The logging system is fully thread-safe. You can safely log from multiple threads without additional synchronization.
