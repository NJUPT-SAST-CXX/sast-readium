#pragma once

#include <QDebug>
#include <QException>
#include <QString>
#include <functional>
#include <memory>
#include <optional>
#include <variant>
#include "../logging/Logger.h"

namespace ErrorHandling {

/**
 * @brief Standard error categories for the application
 */
enum class ErrorCategory {
    FileSystem,     // File I/O, path operations
    Document,       // PDF document operations
    Rendering,      // Page rendering, graphics
    Search,         // Search operations
    Cache,          // Cache operations
    Network,        // Network operations
    Threading,      // Thread safety, async operations
    UI,             // User interface operations
    Plugin,         // Plugin system
    Configuration,  // Settings, configuration
    Memory,         // Memory allocation, management
    Unknown         // Fallback category
};

/**
 * @brief Standard error severity levels
 */
enum class ErrorSeverity {
    Info,      // Informational, no action needed
    Warning,   // Warning, operation can continue
    Error,     // Error, operation failed but recoverable
    Critical,  // Critical error, application state compromised
    Fatal      // Fatal error, application must terminate
};

/**
 * @brief Structured error information
 */
struct ErrorInfo {
    ErrorCategory category;
    ErrorSeverity severity;
    QString message;
    QString details;
    QString context;
    int errorCode;

    ErrorInfo(ErrorCategory cat = ErrorCategory::Unknown,
              ErrorSeverity sev = ErrorSeverity::Error,
              const QString& msg = QString(), const QString& det = QString(),
              const QString& ctx = QString(), int code = 0)
        : category(cat),
          severity(sev),
          message(msg),
          details(det),
          context(ctx),
          errorCode(code) {}
};

/**
 * @brief Standard application exception with structured error information
 */
class ApplicationException : public QException {
public:
    explicit ApplicationException(const ErrorInfo& error)
        : m_errorInfo(error) {}

    ApplicationException(ErrorCategory category, ErrorSeverity severity,
                         const QString& message,
                         const QString& details = QString(),
                         const QString& context = QString(), int errorCode = 0)
        : m_errorInfo(category, severity, message, details, context,
                      errorCode) {}

    const ErrorInfo& errorInfo() const { return m_errorInfo; }
    const char* what() const noexcept override {
        static QByteArray msg = m_errorInfo.message.toUtf8();
        return msg.constData();
    }

    ApplicationException* clone() const override {
        return new ApplicationException(*this);
    }

    void raise() const override { throw *this; }

private:
    ErrorInfo m_errorInfo;
};

/**
 * @brief Result type for operations that can fail
 */
template <typename T>
using Result = std::variant<T, ErrorInfo>;

/**
 * @brief Check if result contains a value
 */
template <typename T>
bool isSuccess(const Result<T>& result) {
    return std::holds_alternative<T>(result);
}

/**
 * @brief Check if result contains an error
 */
template <typename T>
bool isError(const Result<T>& result) {
    return std::holds_alternative<ErrorInfo>(result);
}

/**
 * @brief Get value from successful result
 */
template <typename T>
const T& getValue(const Result<T>& result) {
    return std::get<T>(result);
}

/**
 * @brief Get error from failed result
 */
template <typename T>
const ErrorInfo& getError(const Result<T>& result) {
    return std::get<ErrorInfo>(result);
}

/**
 * @brief Create successful result
 */
template <typename T>
Result<std::decay_t<T>> success(T&& value) {
    return Result<std::decay_t<T>>(std::forward<T>(value));
}

/**
 * @brief Create error result
 */
template <typename T>
Result<T> error(const ErrorInfo& errorInfo) {
    return Result<T>(errorInfo);
}

/**
 * @brief Create error result with parameters
 */
template <typename T>
Result<T> error(ErrorCategory category, ErrorSeverity severity,
                const QString& message, const QString& details = QString(),
                const QString& context = QString(), int errorCode = 0) {
    return Result<T>(
        ErrorInfo(category, severity, message, details, context, errorCode));
}

// Forward declaration of logError function
void logError(const ErrorInfo& errorInfo);

/**
 * @brief Safe execution wrapper with automatic error handling
 */
template <typename Func>
auto safeExecute(Func&& func, ErrorCategory category = ErrorCategory::Unknown,
                 const QString& context = QString()) {
    using ReturnType = decltype(func());

    if constexpr (std::is_void_v<ReturnType>) {
        // For void functions, use bool as success indicator
        using ResultType = Result<bool>;
        try {
            func();
            return success<bool>(true);
        } catch (const ApplicationException& e) {
            logError(e.errorInfo());
            return error<bool>(e.errorInfo());
        } catch (const std::exception& e) {
            ErrorInfo errorInfo(category, ErrorSeverity::Error,
                                QString("Standard exception: %1").arg(e.what()),
                                QString(), context);
            logError(errorInfo);
            return error<bool>(errorInfo);
        } catch (...) {
            ErrorInfo errorInfo(category, ErrorSeverity::Error,
                                "Unknown exception occurred", QString(),
                                context);
            logError(errorInfo);
            return error<bool>(errorInfo);
        }
    } else {
        // For non-void functions, use actual return type
        using ResultType = Result<ReturnType>;
        try {
            return success<ReturnType>(func());
        } catch (const ApplicationException& e) {
            logError(e.errorInfo());
            return error<ReturnType>(e.errorInfo());
        } catch (const std::exception& e) {
            ErrorInfo errorInfo(category, ErrorSeverity::Error,
                                QString("Standard exception: %1").arg(e.what()),
                                QString(), context);
            logError(errorInfo);
            return error<ReturnType>(errorInfo);
        } catch (...) {
            ErrorInfo errorInfo(category, ErrorSeverity::Error,
                                "Unknown exception occurred", QString(),
                                context);
            logError(errorInfo);
            return error<ReturnType>(errorInfo);
        }
    }
}

/**
 * @brief Convert error category to string
 */
QString categoryToString(ErrorCategory category);

/**
 * @brief Convert error severity to string
 */
QString severityToString(ErrorSeverity severity);

/**
 * @brief Create error info for file system operations
 */
ErrorInfo createFileSystemError(const QString& operation, const QString& path,
                                const QString& details = QString());

/**
 * @brief Create error info for document operations
 */
ErrorInfo createDocumentError(const QString& operation,
                              const QString& details = QString());

/**
 * @brief Create error info for rendering operations
 */
ErrorInfo createRenderingError(const QString& operation,
                               const QString& details = QString());

/**
 * @brief Create error info for search operations
 */
ErrorInfo createSearchError(const QString& operation,
                            const QString& details = QString());

/**
 * @brief Create error info for cache operations
 */
ErrorInfo createCacheError(const QString& operation,
                           const QString& details = QString());

/**
 * @brief Create error info for threading operations
 */
ErrorInfo createThreadingError(const QString& operation,
                               const QString& details = QString());

}  // namespace ErrorHandling

/**
 * @brief Convenience macros for error handling
 */
#define SAFE_EXECUTE(func, category, context) \
    ErrorHandling::safeExecute([&]() { return func; }, category, context)

#define SAFE_EXECUTE_VOID(func, category, context) \
    ErrorHandling::safeExecute([&]() { func; }, category, context)

#define CHECK_RESULT(result)                                          \
    if (ErrorHandling::isError(result)) {                             \
        ErrorHandling::logError(ErrorHandling::getError(result));     \
        return ErrorHandling::error<decltype(ErrorHandling::getValue( \
            result))>(ErrorHandling::getError(result));               \
    }

#define RETURN_IF_ERROR(result)                 \
    if (ErrorHandling::isError(result)) {       \
        return ErrorHandling::getError(result); \
    }

#define LOG_AND_RETURN_ERROR(category, severity, message, details, context)    \
    do {                                                                       \
        auto errorInfo = ErrorHandling::ErrorInfo(category, severity, message, \
                                                  details, context);           \
        ErrorHandling::logError(errorInfo);                                    \
        return ErrorHandling::error<decltype(ErrorHandling::getValue(          \
            result))>(errorInfo);                                              \
    } while (0)
