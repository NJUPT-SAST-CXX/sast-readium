/**
 * @file test_crash_handler.cpp
 * @brief Tests for crash handler functionality
 * @author SAST Readium Project
 * @version 1.0
 * @date 2025-10-31
 */

#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include "logging/CrashHandler.h"
#include "logging/StackTrace.h"

class TestCrashHandler : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Stack trace tests
    void testStackTraceCapture();
    void testStackTraceFormatting();
    void testThreadInfo();

    // Crash handler tests
    void testCrashHandlerInitialization();
    void testCrashLogDirectory();
    void testCrashCallback();
    void testContextData();
    void testCrashLogCreation();
    void testCrashLogCleanup();
    void testTestCrash();

private:
    QTemporaryDir* m_tempDir;
};

void TestCrashHandler::initTestCase() {
    // Initialize stack trace system
    StackTraceUtils::initialize();
}

void TestCrashHandler::cleanupTestCase() {
    // Cleanup stack trace system
    StackTraceUtils::cleanup();
}

void TestCrashHandler::init() {
    // Create temporary directory for crash logs
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
}

void TestCrashHandler::cleanup() {
    // Shutdown crash handler
    if (CrashHandler::instance().isInitialized()) {
        CrashHandler::instance().shutdown();
    }

    // Clean up temporary directory
    delete m_tempDir;
    m_tempDir = nullptr;
}

void TestCrashHandler::testStackTraceCapture() {
    // Test that we can capture a stack trace
    auto frames = StackTraceUtils::captureStackTrace();

    // Should have at least one frame (this function)
    QVERIFY(!frames.empty());

    // Check that frames have valid addresses
    for (const auto& frame : frames) {
        QVERIFY(frame.address != 0);
    }
}

void TestCrashHandler::testStackTraceFormatting() {
    // Test stack trace formatting
    QString stackTrace = StackTraceUtils::captureAndFormatStackTrace();

    // Should not be empty
    QVERIFY(!stackTrace.isEmpty());

    // Should contain frame information
    QVERIFY(stackTrace.contains("Frame") || stackTrace.contains("#"));
}

void TestCrashHandler::testThreadInfo() {
    // Test thread information capture
    QString threadInfo = StackTraceUtils::getThreadInfo();

    // Should not be empty
    QVERIFY(!threadInfo.isEmpty());

    // Should contain thread ID
    QVERIFY(threadInfo.contains("Thread") || threadInfo.contains("ID"));
}

void TestCrashHandler::testCrashHandlerInitialization() {
    // Test initialization
    QVERIFY(!CrashHandler::instance().isInitialized());

    bool result = CrashHandler::instance().initialize(false);
    QVERIFY(result);
    QVERIFY(CrashHandler::instance().isInitialized());

    // Test double initialization (should succeed)
    result = CrashHandler::instance().initialize(false);
    QVERIFY(result);
}

void TestCrashHandler::testCrashLogDirectory() {
    // Initialize crash handler
    CrashHandler::instance().initialize(false);

    // Set custom crash log directory
    QString customDir = m_tempDir->path() + "/crashes";
    CrashHandler::instance().setCrashLogDirectory(customDir);

    // Verify directory was set
    QCOMPARE(CrashHandler::instance().getCrashLogDirectory(), customDir);

    // Verify directory was created
    QVERIFY(QFileInfo::exists(customDir));
}

void TestCrashHandler::testCrashCallback() {
    // Initialize crash handler
    CrashHandler::instance().initialize(false);
    CrashHandler::instance().setCrashLogDirectory(m_tempDir->path());

    // Register callback
    bool callbackCalled = false;
    QString receivedType;
    QString receivedMessage;

    CrashHandler::instance().registerCrashCallback([&](const CrashInfo& info) {
        callbackCalled = true;
        receivedType = info.exceptionType;
        receivedMessage = info.exceptionMessage;
        return true;  // Continue handling
    });

    // Trigger test crash
    CrashHandler::instance().triggerTestCrash("Test crash message");

    // Verify callback was called
    QVERIFY(callbackCalled);
    QCOMPARE(receivedType, QString("Test Crash"));
    QCOMPARE(receivedMessage, QString("Test crash message"));
}

void TestCrashHandler::testContextData() {
    // Initialize crash handler
    CrashHandler::instance().initialize(false);
    CrashHandler::instance().setCrashLogDirectory(m_tempDir->path());

    // Set context data
    CrashHandler::instance().setContextData("operation", "test_operation");
    CrashHandler::instance().setContextData("user", "test_user");

    // Register callback to verify context data
    bool callbackCalled = false;
    QMap<QString, QString> receivedData;

    CrashHandler::instance().registerCrashCallback([&](const CrashInfo& info) {
        callbackCalled = true;
        receivedData = info.customData;
        return true;
    });

    // Trigger test crash
    CrashHandler::instance().triggerTestCrash("Test");

    // Verify context data was captured
    QVERIFY(callbackCalled);
    QCOMPARE(receivedData["operation"], QString("test_operation"));
    QCOMPARE(receivedData["user"], QString("test_user"));
}

void TestCrashHandler::testCrashLogCreation() {
    // Initialize crash handler
    CrashHandler::instance().initialize(false);
    CrashHandler::instance().setCrashLogDirectory(m_tempDir->path());

    // Trigger test crash
    CrashHandler::instance().triggerTestCrash("Test crash for log creation");

    // Verify crash log was created
    QStringList logs = CrashHandler::instance().getCrashLogFiles();
    QVERIFY(!logs.isEmpty());

    // Verify log file exists
    QString logFile = logs.first();
    QVERIFY(QFileInfo::exists(logFile));

    // Verify log file contains expected information
    QFile file(logFile);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = QString::fromUtf8(file.readAll());
    file.close();

    QVERIFY(content.contains("CRASH REPORT"));
    QVERIFY(content.contains("Test Crash"));
    QVERIFY(content.contains("Test crash for log creation"));
    QVERIFY(content.contains("Stack Trace"));
}

void TestCrashHandler::testCrashLogCleanup() {
    // Initialize crash handler
    CrashHandler::instance().initialize(false);
    CrashHandler::instance().setCrashLogDirectory(m_tempDir->path());

    // Create multiple crash logs
    for (int i = 0; i < 5; ++i) {
        CrashHandler::instance().triggerTestCrash(
            QString("Test crash %1").arg(i));
        QTest::qWait(100);  // Small delay to ensure different timestamps
    }

    // Verify all logs were created
    QStringList logs = CrashHandler::instance().getCrashLogFiles();
    QCOMPARE(logs.size(), 5);

    // Clean up old logs, keeping only 2
    CrashHandler::instance().cleanupOldCrashLogs(2);

    // Verify only 2 logs remain
    logs = CrashHandler::instance().getCrashLogFiles();
    QCOMPARE(logs.size(), 2);
}

void TestCrashHandler::testTestCrash() {
    // Initialize crash handler
    CrashHandler::instance().initialize(false);
    CrashHandler::instance().setCrashLogDirectory(m_tempDir->path());

    // Connect to crash detected signal
    QSignalSpy spy(&CrashHandler::instance(), &CrashHandler::crashDetected);

    // Trigger test crash
    CrashHandler::instance().triggerTestCrash("Signal test");

    // Verify signal was emitted
    QCOMPARE(spy.count(), 1);

    // Verify signal parameters
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.size() > 0);
}

QTEST_MAIN(TestCrashHandler)
#include "test_crash_handler.moc"
