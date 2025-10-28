#include <QSignalSpy>
#include <QTest>
#include <stdexcept>
#include "../../app/utils/ErrorHandling.h"
#include "../TestUtilities.h"

class ErrorHandlingTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

    // ErrorInfo tests
    void testErrorInfoConstructor();
    void testErrorInfoDefaultValues();
    void testErrorInfoWithAllParameters();

    // ErrorCategory tests
    void testErrorCategoryEnum();
    void testCategoryToString();

    // ErrorSeverity tests
    void testErrorSeverityEnum();
    void testSeverityToString();

    // ApplicationException tests
    void testApplicationExceptionConstructor();
    void testApplicationExceptionWithErrorInfo();
    void testApplicationExceptionWithParameters();
    void testApplicationExceptionWhat();
    void testApplicationExceptionClone();
    void testApplicationExceptionRaise();

    // Result type tests
    void testResultTypeSuccess();
    void testResultTypeError();
    void testIsSuccess();
    void testIsError();
    void testGetValue();
    void testGetError();
    void testSuccessFunction();
    void testErrorFunction();
    void testErrorFunctionWithParameters();

    // Safe execution tests
    void testSafeExecuteVoidSuccess();
    void testSafeExecuteVoidException();
    void testSafeExecuteVoidApplicationException();
    void testSafeExecuteVoidStandardException();
    void testSafeExecuteVoidUnknownException();
    void testSafeExecuteReturnValueSuccess();
    void testSafeExecuteReturnValueException();

    // Utility function tests
    void testCreateFileSystemError();
    void testCreateDocumentError();
    void testCreateRenderingError();
    void testCreateSearchError();
    void testCreateCacheError();
    void testCreateThreadingError();

    // Macro tests
    void testSafeExecuteMacro();
    void testSafeExecuteVoidMacro();

    // Error logging tests
    void testLogError();
    void testLogErrorWithDifferentSeverities();

    // Edge cases and error handling
    void testEmptyErrorMessage();
    void testLongErrorMessage();
    void testSpecialCharactersInError();
    void testNullPointerHandling();

private:
    // Helper methods
    void throwApplicationException();
    void throwStandardException();
    void throwUnknownException();
    int returnValue();
    void voidFunction();
    ErrorHandling::ErrorInfo createTestErrorInfo();
};

void ErrorHandlingTest::initTestCase() {
    // Setup test environment
}

void ErrorHandlingTest::cleanupTestCase() {
    // Cleanup test environment
}

void ErrorHandlingTest::init() {
    // Per-test setup
}

void ErrorHandlingTest::cleanup() {
    // Per-test cleanup
}

void ErrorHandlingTest::testErrorInfoConstructor() {
    // Test default constructor
    ErrorHandling::ErrorInfo defaultError;
    QCOMPARE(defaultError.category, ErrorHandling::ErrorCategory::Unknown);
    QCOMPARE(defaultError.severity, ErrorHandling::ErrorSeverity::Error);
    QVERIFY(defaultError.message.isEmpty());
    QVERIFY(defaultError.details.isEmpty());
    QVERIFY(defaultError.context.isEmpty());
    QCOMPARE(defaultError.errorCode, 0);
}

void ErrorHandlingTest::testErrorInfoDefaultValues() {
    ErrorHandling::ErrorInfo error(ErrorHandling::ErrorCategory::FileSystem);

    QCOMPARE(error.category, ErrorHandling::ErrorCategory::FileSystem);
    QCOMPARE(error.severity, ErrorHandling::ErrorSeverity::Error);
    QVERIFY(error.message.isEmpty());
    QVERIFY(error.details.isEmpty());
    QVERIFY(error.context.isEmpty());
    QCOMPARE(error.errorCode, 0);
}

void ErrorHandlingTest::testErrorInfoWithAllParameters() {
    ErrorHandling::ErrorInfo error(ErrorHandling::ErrorCategory::Document,
                                   ErrorHandling::ErrorSeverity::Critical,
                                   "Test message", "Test details",
                                   "Test context", 42);

    QCOMPARE(error.category, ErrorHandling::ErrorCategory::Document);
    QCOMPARE(error.severity, ErrorHandling::ErrorSeverity::Critical);
    QCOMPARE(error.message, "Test message");
    QCOMPARE(error.details, "Test details");
    QCOMPARE(error.context, "Test context");
    QCOMPARE(error.errorCode, 42);
}

void ErrorHandlingTest::testErrorCategoryEnum() {
    // Test that all enum values are distinct
    QVERIFY(ErrorHandling::ErrorCategory::FileSystem !=
            ErrorHandling::ErrorCategory::Document);
    QVERIFY(ErrorHandling::ErrorCategory::Document !=
            ErrorHandling::ErrorCategory::Rendering);
    QVERIFY(ErrorHandling::ErrorCategory::Rendering !=
            ErrorHandling::ErrorCategory::Search);
    QVERIFY(ErrorHandling::ErrorCategory::Search !=
            ErrorHandling::ErrorCategory::Cache);
    QVERIFY(ErrorHandling::ErrorCategory::Cache !=
            ErrorHandling::ErrorCategory::Network);
    QVERIFY(ErrorHandling::ErrorCategory::Network !=
            ErrorHandling::ErrorCategory::Threading);
    QVERIFY(ErrorHandling::ErrorCategory::Threading !=
            ErrorHandling::ErrorCategory::UI);
    QVERIFY(ErrorHandling::ErrorCategory::UI !=
            ErrorHandling::ErrorCategory::Plugin);
    QVERIFY(ErrorHandling::ErrorCategory::Plugin !=
            ErrorHandling::ErrorCategory::Configuration);
    QVERIFY(ErrorHandling::ErrorCategory::Configuration !=
            ErrorHandling::ErrorCategory::Memory);
    QVERIFY(ErrorHandling::ErrorCategory::Memory !=
            ErrorHandling::ErrorCategory::Unknown);
}

void ErrorHandlingTest::testCategoryToString() {
    QCOMPARE(ErrorHandling::categoryToString(
                 ErrorHandling::ErrorCategory::FileSystem),
             "FileSystem");
    QCOMPARE(
        ErrorHandling::categoryToString(ErrorHandling::ErrorCategory::Document),
        "Document");
    QCOMPARE(ErrorHandling::categoryToString(
                 ErrorHandling::ErrorCategory::Rendering),
             "Rendering");
    QCOMPARE(
        ErrorHandling::categoryToString(ErrorHandling::ErrorCategory::Search),
        "Search");
    QCOMPARE(
        ErrorHandling::categoryToString(ErrorHandling::ErrorCategory::Cache),
        "Cache");
    QCOMPARE(
        ErrorHandling::categoryToString(ErrorHandling::ErrorCategory::Network),
        "Network");
    QCOMPARE(ErrorHandling::categoryToString(
                 ErrorHandling::ErrorCategory::Threading),
             "Threading");
    QCOMPARE(ErrorHandling::categoryToString(ErrorHandling::ErrorCategory::UI),
             "UI");
    QCOMPARE(
        ErrorHandling::categoryToString(ErrorHandling::ErrorCategory::Plugin),
        "Plugin");
    QCOMPARE(ErrorHandling::categoryToString(
                 ErrorHandling::ErrorCategory::Configuration),
             "Configuration");
    QCOMPARE(
        ErrorHandling::categoryToString(ErrorHandling::ErrorCategory::Memory),
        "Memory");
    QCOMPARE(
        ErrorHandling::categoryToString(ErrorHandling::ErrorCategory::Unknown),
        "Unknown");
}

void ErrorHandlingTest::testErrorSeverityEnum() {
    // Test that all enum values are distinct
    QVERIFY(ErrorHandling::ErrorSeverity::Info !=
            ErrorHandling::ErrorSeverity::Warning);
    QVERIFY(ErrorHandling::ErrorSeverity::Warning !=
            ErrorHandling::ErrorSeverity::Error);
    QVERIFY(ErrorHandling::ErrorSeverity::Error !=
            ErrorHandling::ErrorSeverity::Critical);
    QVERIFY(ErrorHandling::ErrorSeverity::Critical !=
            ErrorHandling::ErrorSeverity::Fatal);
}

void ErrorHandlingTest::testSeverityToString() {
    QCOMPARE(
        ErrorHandling::severityToString(ErrorHandling::ErrorSeverity::Info),
        "INFO");
    QCOMPARE(
        ErrorHandling::severityToString(ErrorHandling::ErrorSeverity::Warning),
        "WARNING");
    QCOMPARE(
        ErrorHandling::severityToString(ErrorHandling::ErrorSeverity::Error),
        "ERROR");
    QCOMPARE(
        ErrorHandling::severityToString(ErrorHandling::ErrorSeverity::Critical),
        "CRITICAL");
    QCOMPARE(
        ErrorHandling::severityToString(ErrorHandling::ErrorSeverity::Fatal),
        "FATAL");
}

void ErrorHandlingTest::testApplicationExceptionConstructor() {
    ErrorHandling::ErrorInfo errorInfo(ErrorHandling::ErrorCategory::FileSystem,
                                       ErrorHandling::ErrorSeverity::Error,
                                       "Test error");

    ErrorHandling::ApplicationException exception(errorInfo);

    QCOMPARE(exception.errorInfo().category,
             ErrorHandling::ErrorCategory::FileSystem);
    QCOMPARE(exception.errorInfo().severity,
             ErrorHandling::ErrorSeverity::Error);
    QCOMPARE(exception.errorInfo().message, "Test error");
}

void ErrorHandlingTest::testApplicationExceptionWithErrorInfo() {
    ErrorHandling::ErrorInfo errorInfo = createTestErrorInfo();
    ErrorHandling::ApplicationException exception(errorInfo);

    const ErrorHandling::ErrorInfo& retrievedInfo = exception.errorInfo();
    QCOMPARE(retrievedInfo.category, errorInfo.category);
    QCOMPARE(retrievedInfo.severity, errorInfo.severity);
    QCOMPARE(retrievedInfo.message, errorInfo.message);
    QCOMPARE(retrievedInfo.details, errorInfo.details);
    QCOMPARE(retrievedInfo.context, errorInfo.context);
    QCOMPARE(retrievedInfo.errorCode, errorInfo.errorCode);
}

void ErrorHandlingTest::testApplicationExceptionWithParameters() {
    ErrorHandling::ApplicationException exception(
        ErrorHandling::ErrorCategory::Document,
        ErrorHandling::ErrorSeverity::Critical, "Direct construction",
        "Test details", "Test context", 123);

    QCOMPARE(exception.errorInfo().category,
             ErrorHandling::ErrorCategory::Document);
    QCOMPARE(exception.errorInfo().severity,
             ErrorHandling::ErrorSeverity::Critical);
    QCOMPARE(exception.errorInfo().message, "Direct construction");
    QCOMPARE(exception.errorInfo().details, "Test details");
    QCOMPARE(exception.errorInfo().context, "Test context");
    QCOMPARE(exception.errorInfo().errorCode, 123);
}

void ErrorHandlingTest::testApplicationExceptionWhat() {
    ErrorHandling::ApplicationException exception(
        ErrorHandling::ErrorCategory::FileSystem,
        ErrorHandling::ErrorSeverity::Error, "Test message");

    QString what = exception.what();
    QCOMPARE(what, "Test message");
}

void ErrorHandlingTest::testApplicationExceptionClone() {
    ErrorHandling::ApplicationException original(
        ErrorHandling::ErrorCategory::Search,
        ErrorHandling::ErrorSeverity::Warning, "Original message");

    std::unique_ptr<ErrorHandling::ApplicationException> cloned(
        original.clone());

    QVERIFY(cloned != nullptr);
    QCOMPARE(cloned->errorInfo().category, original.errorInfo().category);
    QCOMPARE(cloned->errorInfo().severity, original.errorInfo().severity);
    QCOMPARE(cloned->errorInfo().message, original.errorInfo().message);
}

void ErrorHandlingTest::testApplicationExceptionRaise() {
    ErrorHandling::ApplicationException exception(
        ErrorHandling::ErrorCategory::Cache,
        ErrorHandling::ErrorSeverity::Error, "Raise test");

    bool exceptionCaught = false;
    try {
        exception.raise();
    } catch (const ErrorHandling::ApplicationException& e) {
        exceptionCaught = true;
        QCOMPARE(e.errorInfo().message, "Raise test");
    }

    QVERIFY(exceptionCaught);
}

// Helper method implementations
void ErrorHandlingTest::throwApplicationException() {
    throw ErrorHandling::ApplicationException(
        ErrorHandling::ErrorCategory::FileSystem,
        ErrorHandling::ErrorSeverity::Error, "Application exception test");
}

void ErrorHandlingTest::throwStandardException() {
    throw std::runtime_error("Standard exception test");
}

void ErrorHandlingTest::throwUnknownException() {
    throw 42;  // Unknown exception type
}

int ErrorHandlingTest::returnValue() { return 42; }

void ErrorHandlingTest::voidFunction() {
    // Do nothing
}

ErrorHandling::ErrorInfo ErrorHandlingTest::createTestErrorInfo() {
    return ErrorHandling::ErrorInfo(ErrorHandling::ErrorCategory::Document,
                                    ErrorHandling::ErrorSeverity::Warning,
                                    "Test message", "Test details",
                                    "Test context", 100);
}

void ErrorHandlingTest::testResultTypeSuccess() {
    ErrorHandling::Result<int> result = ErrorHandling::success(42);

    QVERIFY(ErrorHandling::isSuccess(result));
    QVERIFY(!ErrorHandling::isError(result));
    QCOMPARE(ErrorHandling::getValue(result), 42);
}

void ErrorHandlingTest::testResultTypeError() {
    ErrorHandling::ErrorInfo errorInfo = createTestErrorInfo();
    ErrorHandling::Result<int> result = ErrorHandling::error<int>(errorInfo);

    QVERIFY(!ErrorHandling::isSuccess(result));
    QVERIFY(ErrorHandling::isError(result));

    const ErrorHandling::ErrorInfo& retrievedError =
        ErrorHandling::getError(result);
    QCOMPARE(retrievedError.category, errorInfo.category);
    QCOMPARE(retrievedError.message, errorInfo.message);
}

void ErrorHandlingTest::testIsSuccess() {
    ErrorHandling::Result<QString> successResult =
        ErrorHandling::success(QString("test"));
    ErrorHandling::Result<QString> errorResult =
        ErrorHandling::error<QString>(createTestErrorInfo());

    QVERIFY(ErrorHandling::isSuccess(successResult));
    QVERIFY(!ErrorHandling::isSuccess(errorResult));
}

void ErrorHandlingTest::testIsError() {
    ErrorHandling::Result<QString> successResult =
        ErrorHandling::success(QString("test"));
    ErrorHandling::Result<QString> errorResult =
        ErrorHandling::error<QString>(createTestErrorInfo());

    QVERIFY(!ErrorHandling::isError(successResult));
    QVERIFY(ErrorHandling::isError(errorResult));
}

void ErrorHandlingTest::testGetValue() {
    QString testValue = "test value";
    ErrorHandling::Result<QString> result = ErrorHandling::success(testValue);

    QCOMPARE(ErrorHandling::getValue(result), testValue);
}

void ErrorHandlingTest::testGetError() {
    ErrorHandling::ErrorInfo errorInfo = createTestErrorInfo();
    ErrorHandling::Result<int> result = ErrorHandling::error<int>(errorInfo);

    const ErrorHandling::ErrorInfo& retrievedError =
        ErrorHandling::getError(result);
    QCOMPARE(retrievedError.category, errorInfo.category);
    QCOMPARE(retrievedError.severity, errorInfo.severity);
    QCOMPARE(retrievedError.message, errorInfo.message);
}

void ErrorHandlingTest::testSuccessFunction() {
    int value = 123;
    ErrorHandling::Result<int> result = ErrorHandling::success(value);

    QVERIFY(ErrorHandling::isSuccess(result));
    QCOMPARE(ErrorHandling::getValue(result), value);
}

void ErrorHandlingTest::testErrorFunction() {
    ErrorHandling::ErrorInfo errorInfo = createTestErrorInfo();
    ErrorHandling::Result<double> result =
        ErrorHandling::error<double>(errorInfo);

    QVERIFY(ErrorHandling::isError(result));
    QCOMPARE(ErrorHandling::getError(result).message, errorInfo.message);
}

void ErrorHandlingTest::testErrorFunctionWithParameters() {
    ErrorHandling::Result<bool> result = ErrorHandling::error<bool>(
        ErrorHandling::ErrorCategory::Network,
        ErrorHandling::ErrorSeverity::Critical, "Network error",
        "Connection timeout", "HTTP request", 404);

    QVERIFY(ErrorHandling::isError(result));

    const ErrorHandling::ErrorInfo& error = ErrorHandling::getError(result);
    QCOMPARE(error.category, ErrorHandling::ErrorCategory::Network);
    QCOMPARE(error.severity, ErrorHandling::ErrorSeverity::Critical);
    QCOMPARE(error.message, "Network error");
    QCOMPARE(error.details, "Connection timeout");
    QCOMPARE(error.context, "HTTP request");
    QCOMPARE(error.errorCode, 404);
}

void ErrorHandlingTest::testSafeExecuteVoidSuccess() {
    auto result = ErrorHandling::safeExecute(
        [this]() { voidFunction(); }, ErrorHandling::ErrorCategory::Unknown,
        "Test context");

    QVERIFY(ErrorHandling::isSuccess(result));
    QCOMPARE(ErrorHandling::getValue(result), true);
}

void ErrorHandlingTest::testSafeExecuteVoidException() {
    auto result = ErrorHandling::safeExecute(
        [this]() { throwApplicationException(); },
        ErrorHandling::ErrorCategory::FileSystem, "Exception test");

    QVERIFY(ErrorHandling::isError(result));

    const ErrorHandling::ErrorInfo& error = ErrorHandling::getError(result);
    QCOMPARE(error.message, "Application exception test");
}

void ErrorHandlingTest::testSafeExecuteVoidApplicationException() {
    auto result = ErrorHandling::safeExecute(
        [this]() { throwApplicationException(); },
        ErrorHandling::ErrorCategory::Document, "App exception test");

    QVERIFY(ErrorHandling::isError(result));

    const ErrorHandling::ErrorInfo& error = ErrorHandling::getError(result);
    QCOMPARE(
        error.category,
        ErrorHandling::ErrorCategory::FileSystem);  // From the thrown exception
    QCOMPARE(error.message, "Application exception test");
}

void ErrorHandlingTest::testSafeExecuteVoidStandardException() {
    auto result = ErrorHandling::safeExecute(
        [this]() { throwStandardException(); },
        ErrorHandling::ErrorCategory::Memory, "Std exception test");

    QVERIFY(ErrorHandling::isError(result));

    const ErrorHandling::ErrorInfo& error = ErrorHandling::getError(result);
    QCOMPARE(error.category, ErrorHandling::ErrorCategory::Memory);
    QVERIFY(error.message.contains("Standard exception"));
    QVERIFY(error.message.contains("Standard exception test"));
}

void ErrorHandlingTest::testSafeExecuteVoidUnknownException() {
    auto result = ErrorHandling::safeExecute(
        [this]() { throwUnknownException(); },
        ErrorHandling::ErrorCategory::Threading, "Unknown exception test");

    QVERIFY(ErrorHandling::isError(result));

    const ErrorHandling::ErrorInfo& error = ErrorHandling::getError(result);
    QCOMPARE(error.category, ErrorHandling::ErrorCategory::Threading);
    QCOMPARE(error.message, "Unknown exception occurred");
    QCOMPARE(error.context, "Unknown exception test");
}

void ErrorHandlingTest::testSafeExecuteReturnValueSuccess() {
    auto result = ErrorHandling::safeExecute(
        [this]() { return returnValue(); },
        ErrorHandling::ErrorCategory::Unknown, "Return value test");

    QVERIFY(ErrorHandling::isSuccess(result));
    QCOMPARE(ErrorHandling::getValue(result), 42);
}

void ErrorHandlingTest::testSafeExecuteReturnValueException() {
    auto result = ErrorHandling::safeExecute(
        [this]() -> int {
            throwStandardException();
            return 0;
        },
        ErrorHandling::ErrorCategory::Cache, "Return exception test");

    QVERIFY(ErrorHandling::isError(result));

    const ErrorHandling::ErrorInfo& error = ErrorHandling::getError(result);
    QCOMPARE(error.category, ErrorHandling::ErrorCategory::Cache);
    QVERIFY(error.message.contains("Standard exception"));
}

void ErrorHandlingTest::testCreateFileSystemError() {
    ErrorHandling::ErrorInfo error =
        ErrorHandling::createFileSystemError("read", "/path/to/file.pdf");

    QCOMPARE(error.category, ErrorHandling::ErrorCategory::FileSystem);
    QCOMPARE(error.severity, ErrorHandling::ErrorSeverity::Error);
    QVERIFY(error.message.contains("File system operation failed"));
    QVERIFY(error.message.contains("read"));
    QVERIFY(error.details.contains("/path/to/file.pdf"));
    QCOMPARE(error.context, "read");
}

void ErrorHandlingTest::testCreateDocumentError() {
    ErrorHandling::ErrorInfo error =
        ErrorHandling::createDocumentError("parse", "Invalid PDF structure");

    QCOMPARE(error.category, ErrorHandling::ErrorCategory::Document);
    QCOMPARE(error.severity, ErrorHandling::ErrorSeverity::Error);
    QVERIFY(error.message.contains("Document operation failed"));
    QVERIFY(error.message.contains("parse"));
    QCOMPARE(error.details, "Invalid PDF structure");
    QCOMPARE(error.context, "parse");
}

void ErrorHandlingTest::testCreateRenderingError() {
    ErrorHandling::ErrorInfo error =
        ErrorHandling::createRenderingError("render page", "Out of memory");

    QCOMPARE(error.category, ErrorHandling::ErrorCategory::Rendering);
    QCOMPARE(error.severity, ErrorHandling::ErrorSeverity::Error);
    QVERIFY(error.message.contains("Rendering operation failed"));
    QVERIFY(error.message.contains("render page"));
    QCOMPARE(error.details, "Out of memory");
    QCOMPARE(error.context, "render page");
}

void ErrorHandlingTest::testCreateSearchError() {
    ErrorHandling::ErrorInfo error =
        ErrorHandling::createSearchError("regex search", "Invalid pattern");

    QCOMPARE(error.category, ErrorHandling::ErrorCategory::Search);
    QCOMPARE(error.severity, ErrorHandling::ErrorSeverity::Error);
    QVERIFY(error.message.contains("Search operation failed"));
    QVERIFY(error.message.contains("regex search"));
    QCOMPARE(error.details, "Invalid pattern");
    QCOMPARE(error.context, "regex search");
}

void ErrorHandlingTest::testCreateCacheError() {
    ErrorHandling::ErrorInfo error =
        ErrorHandling::createCacheError("cache write", "Disk full");

    QCOMPARE(error.category, ErrorHandling::ErrorCategory::Cache);
    QCOMPARE(
        error.severity,
        ErrorHandling::ErrorSeverity::Warning);  // Cache errors are warnings
    QVERIFY(error.message.contains("Cache operation failed"));
    QVERIFY(error.message.contains("cache write"));
    QCOMPARE(error.details, "Disk full");
    QCOMPARE(error.context, "cache write");
}

void ErrorHandlingTest::testCreateThreadingError() {
    ErrorHandling::ErrorInfo error =
        ErrorHandling::createThreadingError("mutex lock", "Deadlock detected");

    QCOMPARE(error.category, ErrorHandling::ErrorCategory::Threading);
    QCOMPARE(error.severity,
             ErrorHandling::ErrorSeverity::Critical);  // Threading
                                                       // errors are
                                                       // critical
    QVERIFY(error.message.contains("Threading operation failed"));
    QVERIFY(error.message.contains("mutex lock"));
    QCOMPARE(error.details, "Deadlock detected");
    QCOMPARE(error.context, "mutex lock");
}

void ErrorHandlingTest::testSafeExecuteMacro() {
    // Test the SAFE_EXECUTE macro with a function that returns a value
    int testValue = 42;
    auto result = SAFE_EXECUTE(testValue, ErrorHandling::ErrorCategory::Unknown,
                               "Macro test");

    QVERIFY(ErrorHandling::isSuccess(result));
    QCOMPARE(ErrorHandling::getValue(result), 42);

    // Test with a function that throws
    auto errorResult = SAFE_EXECUTE(
        [this]() -> int {
            throwStandardException();
            return 0;
        }(),
        ErrorHandling::ErrorCategory::FileSystem, "Macro error test");

    QVERIFY(ErrorHandling::isError(errorResult));
}

void ErrorHandlingTest::testSafeExecuteVoidMacro() {
    // Test the SAFE_EXECUTE_VOID macro with a void function
    bool executed = false;
    auto result = SAFE_EXECUTE_VOID(executed = true,
                                    ErrorHandling::ErrorCategory::Unknown,
                                    "Void macro test");

    QVERIFY(ErrorHandling::isSuccess(result));
    QVERIFY(executed);

    // Test with a function that throws
    auto errorResult = SAFE_EXECUTE_VOID(throwStandardException(),
                                         ErrorHandling::ErrorCategory::Document,
                                         "Void error test");

    QVERIFY(ErrorHandling::isError(errorResult));
}

void ErrorHandlingTest::testLogError() {
    // Test that logError doesn't crash with various error types
    ErrorHandling::ErrorInfo error(ErrorHandling::ErrorCategory::FileSystem,
                                   ErrorHandling::ErrorSeverity::Error,
                                   "Test error message", "Test details",
                                   "Test context", 404);

    // This should not throw or crash
    ErrorHandling::logError(error);

    // Test with minimal error info
    ErrorHandling::ErrorInfo minimalError;
    ErrorHandling::logError(minimalError);
}

void ErrorHandlingTest::testLogErrorWithDifferentSeverities() {
    // Test logging with each severity level
    ErrorHandling::ErrorInfo infoError(ErrorHandling::ErrorCategory::UI,
                                       ErrorHandling::ErrorSeverity::Info,
                                       "Info message");
    ErrorHandling::logError(infoError);

    ErrorHandling::ErrorInfo warningError(ErrorHandling::ErrorCategory::Cache,
                                          ErrorHandling::ErrorSeverity::Warning,
                                          "Warning message");
    ErrorHandling::logError(warningError);

    ErrorHandling::ErrorInfo errorError(ErrorHandling::ErrorCategory::Document,
                                        ErrorHandling::ErrorSeverity::Error,
                                        "Error message");
    ErrorHandling::logError(errorError);

    ErrorHandling::ErrorInfo criticalError(
        ErrorHandling::ErrorCategory::Threading,
        ErrorHandling::ErrorSeverity::Critical, "Critical message");
    ErrorHandling::logError(criticalError);

    ErrorHandling::ErrorInfo fatalError(ErrorHandling::ErrorCategory::Memory,
                                        ErrorHandling::ErrorSeverity::Fatal,
                                        "Fatal message");
    ErrorHandling::logError(fatalError);

    // All should complete without crashing
    QVERIFY(true);
}

void ErrorHandlingTest::testEmptyErrorMessage() {
    ErrorHandling::ErrorInfo error(ErrorHandling::ErrorCategory::Unknown,
                                   ErrorHandling::ErrorSeverity::Error, "");

    QVERIFY(error.message.isEmpty());
    QCOMPARE(error.category, ErrorHandling::ErrorCategory::Unknown);

    // Should handle empty message gracefully
    ErrorHandling::logError(error);
}

void ErrorHandlingTest::testLongErrorMessage() {
    QString longMessage = QString("Error: ").repeated(1000);
    ErrorHandling::ErrorInfo error(ErrorHandling::ErrorCategory::Document,
                                   ErrorHandling::ErrorSeverity::Error,
                                   longMessage);

    QCOMPARE(error.message, longMessage);
    QVERIFY(error.message.length() > 5000);

    // Should handle long messages without issues
    ErrorHandling::logError(error);
}

void ErrorHandlingTest::testSpecialCharactersInError() {
    QString specialMessage = "Error with special chars: \n\t\r\"'<>&";
    QString specialDetails = "Details: 日本�?中文 한국�?العربية";

    ErrorHandling::ErrorInfo error(ErrorHandling::ErrorCategory::FileSystem,
                                   ErrorHandling::ErrorSeverity::Error,
                                   specialMessage, specialDetails);

    QCOMPARE(error.message, specialMessage);
    QCOMPARE(error.details, specialDetails);

    // Should handle special characters gracefully
    ErrorHandling::logError(error);
}

void ErrorHandlingTest::testNullPointerHandling() {
    // Test that error handling works with null/empty strings
    ErrorHandling::ErrorInfo error(ErrorHandling::ErrorCategory::Memory,
                                   ErrorHandling::ErrorSeverity::Critical,
                                   QString(), QString(), QString(), 0);

    QVERIFY(error.message.isEmpty());
    QVERIFY(error.details.isEmpty());
    QVERIFY(error.context.isEmpty());
    QCOMPARE(error.errorCode, 0);

    // Should not crash with null/empty values
    ErrorHandling::logError(error);

    // Test categoryToString and severityToString with all values
    for (int i = 0;
         i <= static_cast<int>(ErrorHandling::ErrorCategory::Unknown); ++i) {
        QString cat = ErrorHandling::categoryToString(
            static_cast<ErrorHandling::ErrorCategory>(i));
        QVERIFY(!cat.isEmpty());
    }

    for (int i = 0; i <= static_cast<int>(ErrorHandling::ErrorSeverity::Fatal);
         ++i) {
        QString sev = ErrorHandling::severityToString(
            static_cast<ErrorHandling::ErrorSeverity>(i));
        QVERIFY(!sev.isEmpty());
    }
}

QTEST_MAIN(ErrorHandlingTest)
#include "test_error_handling.moc"
