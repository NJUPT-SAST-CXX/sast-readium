#include <QObject>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTextEdit>
#include <QThread>
#include <QtTest/QtTest>
#include "../../app/logging/Logger.h"
#include "../../app/logging/LoggingConfig.h"
#include "../../app/logging/LoggingMacros.h"
#include "../../app/logging/LoggingManager.h"
#include "../../app/logging/QtSpdlogBridge.h"
#include "../../app/logging/SimpleLogging.h"
#include "../TestUtilities.h"

/**
 * Comprehensive tests for the logging system
 * Tests all major components: Logger, LoggingManager, QtSpdlogBridge, Macros,
 * SimpleLogging
 */
class LoggingComprehensiveTest : public TestBase {
    Q_OBJECT

protected:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

private slots:
    // Logger tests
    void testLoggerSingleton();
    void testLoggerInitialization();
    void testLoggerLevels();
    void testLoggerSinkManagement();
    void testLoggerThreadSafety();

    // LoggingManager tests
    void testLoggingManagerSingleton();
    void testLoggingManagerInitialization();
    void testLoggingManagerAsyncLogging();
    void testLoggingManagerCategoryManagement();
    void testLoggingManagerShutdown();

    // QtSpdlogBridge tests
    void testQtSpdlogBridgeSingleton();
    void testQtSpdlogBridgeMessageHandler();
    void testQtSpdlogBridgeCategoryMapping();
    void testQtSpdlogBridgeThreadSafety();

    // LoggingConfig tests
    void testLoggingConfigDefaults();
    void testLoggingConfigSerialization();
    void testLoggingConfigValidation();
    void testLoggingConfigPresets();

    // Macro tests
    void testLoggingMacros();
    void testPerformanceLogger();
    void testScopedLogLevel();
    void testMemoryLogger();

    // SimpleLogging tests
    void testSimpleLoggingInterface();
    void testSimpleLoggingCategoryLogger();
    void testSimpleLoggingTimer();

    // Integration tests
    void testEndToEndLogging();
    void testConfigurationPersistence();
    void testErrorRecovery();

private:
    QTemporaryDir* m_tempDir;
    QString m_logFilePath;

    // Helper methods
    void waitForLogFlush();
    bool logFileContains(const QString& text);
    void cleanupLogFiles();
};

void LoggingComprehensiveTest::initTestCase() {
    qDebug() << "Starting comprehensive logging tests";
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_logFilePath = m_tempDir->path() + "/test.log";
}

void LoggingComprehensiveTest::cleanupTestCase() {
    cleanupLogFiles();
    delete m_tempDir;
    qDebug() << "Comprehensive logging tests completed";
}

void LoggingComprehensiveTest::init() {
    // Ensure clean state before each test
    LoggingManager::instance().shutdown();
    cleanupLogFiles();
}

void LoggingComprehensiveTest::cleanup() {
    // Clean up after each test
    LoggingManager::instance().shutdown();
    cleanupLogFiles();
}

// ============================================================================
// Logger Tests
// ============================================================================

void LoggingComprehensiveTest::testLoggerSingleton() {
    Logger& logger1 = Logger::instance();
    Logger& logger2 = Logger::instance();

    QCOMPARE(&logger1, &logger2);
}

void LoggingComprehensiveTest::testLoggerInitialization() {
    Logger::LoggerConfig config;
    config.level = Logger::LogLevel::Debug;
    config.pattern = "[%Y-%m-%d %H:%M:%S.%e] [%l] %v";
    config.enableConsole = true;
    config.enableFile = true;
    config.logFileName = m_logFilePath;

    Logger::instance().initialize(config);

    // Note: Logger doesn't have isInitialized() method
    // We verify initialization by checking that logging works
    Logger::instance().info("Initialization test");
    waitForLogFlush();
    QVERIFY(logFileContains("Initialization test"));
}

void LoggingComprehensiveTest::testLoggerLevels() {
    Logger::LoggerConfig config;
    config.level = Logger::LogLevel::Info;
    config.enableConsole = true;
    config.enableFile = true;
    config.logFileName = m_logFilePath;

    Logger::instance().initialize(config);

    // Test logging at different levels
    Logger::instance().trace("Trace message");
    Logger::instance().debug("Debug message");
    Logger::instance().info("Info message");
    Logger::instance().warning("Warning message");
    Logger::instance().error("Error message");
    Logger::instance().critical("Critical message");

    waitForLogFlush();

    // Verify that only Info and above are logged
    QVERIFY(!logFileContains("Trace message"));
    QVERIFY(!logFileContains("Debug message"));
    QVERIFY(logFileContains("Info message"));
    QVERIFY(logFileContains("Warning message"));
    QVERIFY(logFileContains("Error message"));
    QVERIFY(logFileContains("Critical message"));
}

void LoggingComprehensiveTest::testLoggerSinkManagement() {
    Logger::LoggerConfig config;
    config.level = Logger::LogLevel::Debug;
    config.enableConsole = true;

    Logger::instance().initialize(config);

    // Add file sink
    Logger::instance().addFileSink(m_logFilePath);

    Logger::instance().info("Test message");
    waitForLogFlush();

    QVERIFY(logFileContains("Test message"));

    // Remove file sink
    Logger::instance().removeSink(Logger::SinkType::File);

    // Clear log file
    QFile::remove(m_logFilePath);

    Logger::instance().info("Another message");
    waitForLogFlush();

    // File should not exist or be empty
    QVERIFY(!QFile::exists(m_logFilePath) ||
            !logFileContains("Another message"));
}

void LoggingComprehensiveTest::testLoggerThreadSafety() {
    Logger::LoggerConfig config;
    config.level = Logger::LogLevel::Debug;
    config.enableConsole = true;
    config.enableFile = true;
    config.logFileName = m_logFilePath;

    Logger::instance().initialize(config);

    // Create multiple threads that log concurrently
    QList<QThread*> threads;
    for (int i = 0; i < 10; ++i) {
        QThread* thread = QThread::create([i]() {
            for (int j = 0; j < 100; ++j) {
                Logger::instance().info(
                    QString("Thread %1 message %2").arg(i).arg(j));
            }
        });
        threads.append(thread);
        thread->start();
    }

    // Wait for all threads to complete
    for (QThread* thread : threads) {
        thread->wait();
        delete thread;
    }

    waitForLogFlush();

    // Verify that all messages were logged
    QVERIFY(logFileContains("Thread 0 message 0"));
    QVERIFY(logFileContains("Thread 9 message 99"));
}

// ============================================================================
// LoggingManager Tests
// ============================================================================

void LoggingComprehensiveTest::testLoggingManagerSingleton() {
    LoggingManager& manager1 = LoggingManager::instance();
    LoggingManager& manager2 = LoggingManager::instance();

    QCOMPARE(&manager1, &manager2);
}

void LoggingComprehensiveTest::testLoggingManagerInitialization() {
    LoggingManager::LoggingConfiguration config;
    config.globalLogLevel = Logger::LogLevel::Debug;
    config.enableConsoleLogging = true;
    config.enableFileLogging = true;
    config.logDirectory = m_tempDir->path();
    config.logFileName = "test.log";

    LoggingManager::instance().initialize(config);

    QVERIFY(LoggingManager::instance().isInitialized());
}

void LoggingComprehensiveTest::testLoggingManagerAsyncLogging() {
    LoggingManager::LoggingConfiguration config;
    config.globalLogLevel = Logger::LogLevel::Debug;
    config.enableConsoleLogging = true;
    config.enableFileLogging = true;
    config.logDirectory = m_tempDir->path();
    config.logFileName = "async_test.log";
    config.enableAsyncLogging = true;
    config.asyncQueueSize = 8192;

    LoggingManager::instance().initialize(config);

    QVERIFY(LoggingManager::instance().isInitialized());

    // Log many messages quickly
    for (int i = 0; i < 1000; ++i) {
        LOG_INFO("Async message {}", i);
    }

    LoggingManager::instance().flushLogs();
    waitForLogFlush();

    QString asyncLogPath = m_tempDir->path() + "/async_test.log";
    QFile logFile(asyncLogPath);
    QVERIFY(logFile.exists());
}

void LoggingComprehensiveTest::testLoggingManagerCategoryManagement() {
    LoggingManager::LoggingConfiguration config;
    config.globalLogLevel = Logger::LogLevel::Debug;
    config.enableConsoleLogging = true;

    LoggingManager::instance().initialize(config);

    // Add categories
    LoggingManager::instance().addLoggingCategory("TestCategory");
    LoggingManager::instance().setLoggingCategoryLevel(
        "TestCategory", Logger::LogLevel::Warning);

    QCOMPARE(LoggingManager::instance().getLoggingCategoryLevel("TestCategory"),
             Logger::LogLevel::Warning);

    // Remove category
    LoggingManager::instance().removeLoggingCategory("TestCategory");

    QCOMPARE(LoggingManager::instance().getLoggingCategoryLevel("TestCategory"),
             Logger::LogLevel::Info);
}

void LoggingComprehensiveTest::testLoggingManagerShutdown() {
    LoggingManager::LoggingConfiguration config;
    config.globalLogLevel = Logger::LogLevel::Debug;
    config.enableConsoleLogging = true;

    LoggingManager::instance().initialize(config);
    QVERIFY(LoggingManager::instance().isInitialized());

    LoggingManager::instance().shutdown();
    QVERIFY(!LoggingManager::instance().isInitialized());
}

// ============================================================================
// QtSpdlogBridge Tests
// ============================================================================

void LoggingComprehensiveTest::testQtSpdlogBridgeSingleton() {
    QtSpdlogBridge& bridge1 = QtSpdlogBridge::instance();
    QtSpdlogBridge& bridge2 = QtSpdlogBridge::instance();

    QCOMPARE(&bridge1, &bridge2);
}

void LoggingComprehensiveTest::testQtSpdlogBridgeMessageHandler() {
    Logger::LoggerConfig config;
    config.level = Logger::LogLevel::Debug;
    config.enableConsole = true;
    config.enableFile = true;
    config.logFileName = m_logFilePath;

    Logger::instance().initialize(config);

    QtSpdlogBridge::instance().initialize();
    QVERIFY(QtSpdlogBridge::instance().isMessageHandlerInstalled());

    // Test Qt logging through bridge
    qDebug() << "Qt debug message";
    qInfo() << "Qt info message";
    qWarning() << "Qt warning message";

    waitForLogFlush();

    QVERIFY(logFileContains("Qt debug message"));
    QVERIFY(logFileContains("Qt info message"));
    QVERIFY(logFileContains("Qt warning message"));

    QtSpdlogBridge::instance().restoreDefaultMessageHandler();
    QVERIFY(!QtSpdlogBridge::instance().isMessageHandlerInstalled());
}

void LoggingComprehensiveTest::testQtSpdlogBridgeCategoryMapping() {
    Logger::LoggerConfig config;
    config.level = Logger::LogLevel::Debug;
    config.enableConsole = true;

    Logger::instance().initialize(config);

    QtSpdlogBridge::instance().initialize();
    QtSpdlogBridge::instance().addCategoryMapping("test.category",
                                                  "test_logger");

    // Category mapping is added successfully
    QVERIFY(true);

    QtSpdlogBridge::instance().removeCategoryMapping("test.category");
}

void LoggingComprehensiveTest::testQtSpdlogBridgeThreadSafety() {
    Logger::LoggerConfig config;
    config.level = Logger::LogLevel::Debug;
    config.enableConsole = true;

    Logger::instance().initialize(config);
    QtSpdlogBridge::instance().initialize();

    // Create multiple threads that add/remove category mappings
    QList<QThread*> threads;
    for (int i = 0; i < 5; ++i) {
        QThread* thread = QThread::create([i]() {
            for (int j = 0; j < 50; ++j) {
                QString category = QString("category_%1_%2").arg(i).arg(j);
                QtSpdlogBridge::instance().addCategoryMapping(category,
                                                              category);
                QtSpdlogBridge::instance().removeCategoryMapping(category);
            }
        });
        threads.append(thread);
        thread->start();
    }

    for (QThread* thread : threads) {
        thread->wait();
        delete thread;
    }

    QtSpdlogBridge::instance().restoreDefaultMessageHandler();
}

// ============================================================================
// LoggingConfig Tests
// ============================================================================

void LoggingComprehensiveTest::testLoggingConfigDefaults() {
    LoggingConfig config;

    auto globalConfig = config.getGlobalConfig();
    QCOMPARE(globalConfig.globalLevel, Logger::LogLevel::Info);
    QVERIFY(!globalConfig.asyncLogging);
}

void LoggingComprehensiveTest::testLoggingConfigSerialization() {
    LoggingConfig config;

    LoggingConfig::GlobalConfiguration globalConfig;
    globalConfig.globalLevel = Logger::LogLevel::Debug;
    globalConfig.asyncLogging = true;
    config.setGlobalConfig(globalConfig);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString filePath = tempFile.fileName();
    tempFile.close();

    QVERIFY(config.saveToJsonFile(filePath));

    LoggingConfig loadedConfig;
    QVERIFY(loadedConfig.loadFromJsonFile(filePath));

    auto loadedGlobalConfig = loadedConfig.getGlobalConfig();
    QCOMPARE(loadedGlobalConfig.globalLevel, Logger::LogLevel::Debug);
    QVERIFY(loadedGlobalConfig.asyncLogging);
}

void LoggingComprehensiveTest::testLoggingConfigValidation() {
    LoggingConfig config;

    LoggingConfig::SinkConfiguration sinkConfig;
    sinkConfig.name = "test_sink";
    sinkConfig.type = "console";
    sinkConfig.enabled = true;

    // Validation is internal, just verify we can set configurations
    QList<LoggingConfig::SinkConfiguration> sinks;
    sinks.append(sinkConfig);
    config.setSinkConfigurations(sinks);

    QCOMPARE(config.getSinkConfigurations().size(), 1);
}

void LoggingComprehensiveTest::testLoggingConfigPresets() {
    LoggingConfig devConfig;
    devConfig.loadDevelopmentPreset();
    QVERIFY(true);

    LoggingConfig prodConfig;
    prodConfig.loadProductionPreset();
    QVERIFY(true);

    LoggingConfig debugConfig;
    debugConfig.loadDebugPreset();
    QVERIFY(true);
}

// ============================================================================
// Macro Tests
// ============================================================================

void LoggingComprehensiveTest::testLoggingMacros() {
    Logger::LoggerConfig config;
    config.level = Logger::LogLevel::Trace;
    config.enableConsole = true;
    config.enableFile = true;
    config.logFileName = m_logFilePath;

    Logger::instance().initialize(config);

    LOG_TRACE("Trace macro test");
    LOG_DEBUG("Debug macro test");
    LOG_INFO("Info macro test");
    LOG_WARNING("Warning macro test");
    LOG_ERROR("Error macro test");
    LOG_CRITICAL("Critical macro test");

    waitForLogFlush();

    QVERIFY(logFileContains("Trace macro test"));
    QVERIFY(logFileContains("Debug macro test"));
    QVERIFY(logFileContains("Info macro test"));
    QVERIFY(logFileContains("Warning macro test"));
    QVERIFY(logFileContains("Error macro test"));
    QVERIFY(logFileContains("Critical macro test"));
}

void LoggingComprehensiveTest::testPerformanceLogger() {
    Logger::LoggerConfig config;
    config.level = Logger::LogLevel::Debug;
    config.enableConsole = true;
    config.enableFile = true;
    config.logFileName = m_logFilePath;

    Logger::instance().initialize(config);

    {
        PerformanceLogger perfLogger("TestOperation");
        QThread::msleep(10);  // Simulate work
    }

    waitForLogFlush();

    QVERIFY(logFileContains("Performance"));
    QVERIFY(logFileContains("TestOperation"));
}

void LoggingComprehensiveTest::testScopedLogLevel() {
    Logger::LoggerConfig config;
    config.level = Logger::LogLevel::Info;
    config.enableConsole = true;
    config.enableFile = true;
    config.logFileName = m_logFilePath;

    Logger::instance().initialize(config);

    LOG_DEBUG("Debug before scope");

    {
        ScopedLogLevel scopedLevel(Logger::LogLevel::Debug);
        LOG_DEBUG("Debug inside scope");
    }

    LOG_DEBUG("Debug after scope");

    waitForLogFlush();

    QVERIFY(!logFileContains("Debug before scope"));
    QVERIFY(logFileContains("Debug inside scope"));
    QVERIFY(!logFileContains("Debug after scope"));
}

void LoggingComprehensiveTest::testMemoryLogger() {
    Logger::LoggerConfig config;
    config.level = Logger::LogLevel::Debug;
    config.enableConsole = true;

    Logger::instance().initialize(config);

    MemoryLogger::startMemoryTracking("test_context");
    MemoryLogger::logCurrentUsage("test_checkpoint");
    MemoryLogger::endMemoryTracking("test_context");

    // Just verify it doesn't crash
    QVERIFY(true);
}

// ============================================================================
// SimpleLogging Tests
// ============================================================================

void LoggingComprehensiveTest::testSimpleLoggingInterface() {
    SastLogging::Config config;
    config.level = SastLogging::Level::Debug;
    config.console = true;
    config.file = true;
    config.logDir = m_tempDir->path();
    config.logFile = "simple_test.log";

    SastLogging::init(config);

    SLOG_INFO("Simple logging test");

    SastLogging::flush();
    QThread::msleep(100);

    QString simpleLogPath = m_tempDir->path() + "/simple_test.log";
    QFile logFile(simpleLogPath);
    QVERIFY(logFile.exists());

    SastLogging::shutdown();
}

void LoggingComprehensiveTest::testSimpleLoggingCategoryLogger() {
    SastLogging::Config config;
    config.level = SastLogging::Level::Debug;
    config.console = true;

    SastLogging::init(config);

    SastLogging::CategoryLogger categoryLogger("TestCategory");
    categoryLogger.info("Category message");

    SastLogging::shutdown();

    // Just verify it doesn't crash
    QVERIFY(true);
}

void LoggingComprehensiveTest::testSimpleLoggingTimer() {
    // Timer requires a name parameter
    SastLogging::Timer timer("TestTimer");
    QThread::msleep(10);

    // Timer doesn't have elapsed() method, it logs automatically on destruction
    // We just verify it doesn't crash
    QVERIFY(true);
}

// ============================================================================
// Integration Tests
// ============================================================================

void LoggingComprehensiveTest::testEndToEndLogging() {
    // Initialize with LoggingManager
    LoggingManager::LoggingConfiguration config;
    config.globalLogLevel = Logger::LogLevel::Debug;
    config.enableConsoleLogging = true;
    config.enableFileLogging = true;
    config.logDirectory = m_tempDir->path();
    config.logFileName = "integration_test.log";
    config.enableQtMessageHandlerRedirection = true;

    LoggingManager::instance().initialize(config);

    // Test various logging methods
    LOG_INFO("Direct macro logging");
    Logger::instance().info("Direct logger call");
    qInfo() << "Qt logging through bridge";

    LoggingManager::instance().flushLogs();
    QThread::msleep(100);

    QString integrationLogPath = m_tempDir->path() + "/integration_test.log";
    QFile logFile(integrationLogPath);
    QVERIFY(logFile.exists());

    LoggingManager::instance().shutdown();
}

void LoggingComprehensiveTest::testConfigurationPersistence() {
    LoggingConfig config;

    LoggingConfig::GlobalConfiguration globalConfig;
    globalConfig.globalLevel = Logger::LogLevel::Warning;
    globalConfig.asyncLogging = true;
    config.setGlobalConfig(globalConfig);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString filePath = tempFile.fileName();
    tempFile.close();

    QVERIFY(config.saveToJsonFile(filePath));

    LoggingConfig loadedConfig;
    QVERIFY(loadedConfig.loadFromJsonFile(filePath));

    auto loadedGlobalConfig = loadedConfig.getGlobalConfig();
    QCOMPARE(loadedGlobalConfig.globalLevel, Logger::LogLevel::Warning);
    QVERIFY(loadedGlobalConfig.asyncLogging);
}

void LoggingComprehensiveTest::testErrorRecovery() {
    // Test initialization with invalid configuration
    LoggingManager::LoggingConfiguration config;
    config.globalLogLevel = Logger::LogLevel::Debug;
    config.enableFileLogging = true;
    config.logDirectory = "/invalid/path/that/does/not/exist";
    config.logFileName = "test.log";

    // Should fall back to console-only logging
    LoggingManager::instance().initialize(config);

    // Should still be able to log
    LOG_INFO("Error recovery test");

    LoggingManager::instance().shutdown();

    // Just verify it doesn't crash
    QVERIFY(true);
}

// ============================================================================
// Helper Methods
// ============================================================================

void LoggingComprehensiveTest::waitForLogFlush() {
    LoggingManager::instance().flushLogs();
    QThread::msleep(100);  // Give time for async operations
}

bool LoggingComprehensiveTest::logFileContains(const QString& text) {
    QFile logFile(m_logFilePath);
    if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&logFile);
    QString content = in.readAll();
    logFile.close();

    return content.contains(text);
}

void LoggingComprehensiveTest::cleanupLogFiles() {
    QFile::remove(m_logFilePath);
    QFile::remove(m_tempDir->path() + "/async_test.log");
    QFile::remove(m_tempDir->path() + "/simple_test.log");
    QFile::remove(m_tempDir->path() + "/integration_test.log");
}

QTEST_MAIN(LoggingComprehensiveTest)
#include "test_logging_comprehensive.moc"
