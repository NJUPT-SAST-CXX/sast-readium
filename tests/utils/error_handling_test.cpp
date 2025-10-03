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
        "Info");
    QCOMPARE(
        ErrorHandling::severityToString(ErrorHandling::ErrorSeverity::Warning),
        "Warning");
    QCOMPARE(
        ErrorHandling::severityToString(ErrorHandling::ErrorSeverity::Error),
        "Error");
    QCOMPARE(
        ErrorHandling::severityToString(ErrorHandling::ErrorSeverity::Critical),
        "Critical");
    QCOMPARE(
        ErrorHandling::severityToString(ErrorHandling::ErrorSeverity::Fatal),
        "Fatal");
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

QTEST_MAIN(ErrorHandlingTest)
#include "error_handling_test.moc"
