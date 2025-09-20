#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QMutex>
#include <QHash>
#include <QDateTime>
#include <functional>
#include <exception>
#include "../utils/ErrorHandling.h"
#include "../utils/ErrorRecovery.h"

/**
 * Comprehensive error recovery framework for search operations
 * Provides graceful error handling, recovery strategies, and fallback mechanisms
 */
class SearchErrorRecovery : public QObject
{
    Q_OBJECT

public:
    enum ErrorType {
        ValidationError,
        DocumentError,
        SearchError,
        CacheError,
        MemoryError,
        TimeoutError,
        NetworkError,
        UnknownError
    };

    enum RecoveryStrategy {
        NoRecovery,         // Fail immediately
        Retry,              // Retry the operation
        Fallback,           // Use fallback mechanism
        Degrade,            // Degrade functionality
        Skip,               // Skip and continue
        Reset               // Reset and restart
    };

    struct ErrorContext {
        ErrorType type = UnknownError;
        QString operation;
        QString component;
        QString details;
        QDateTime timestamp;
        int attemptCount = 0;
        QVariantMap metadata;
        
        ErrorContext() : timestamp(QDateTime::currentDateTime()) {}
        ErrorContext(ErrorType t, const QString& op, const QString& comp, const QString& det = QString())
            : type(t), operation(op), component(comp), details(det), timestamp(QDateTime::currentDateTime()) {}
    };

    struct RecoveryConfig {
        RecoveryStrategy strategy = Retry;
        int maxRetries = 3;
        int retryDelayMs = 1000;
        bool exponentialBackoff = true;
        bool enableFallback = true;
        bool enableDegradation = true;
        int timeoutMs = 30000;
        bool logRecoveryAttempts = true;
    };

    struct RecoveryResult {
        bool success = false;
        RecoveryStrategy usedStrategy = NoRecovery;
        int attemptsUsed = 0;
        QString message;
        QVariantMap recoveryData;
        
        RecoveryResult() = default;
        RecoveryResult(bool s, RecoveryStrategy rs, int attempts, const QString& msg = QString())
            : success(s), usedStrategy(rs), attemptsUsed(attempts), message(msg) {}
    };

    explicit SearchErrorRecovery(QObject* parent = nullptr);
    ~SearchErrorRecovery();

    // Configuration
    void setRecoveryConfig(ErrorType type, const RecoveryConfig& config);
    RecoveryConfig getRecoveryConfig(ErrorType type) const;
    void setGlobalRecoveryEnabled(bool enabled);
    bool isGlobalRecoveryEnabled() const;

    // Error handling and recovery
    template<typename T>
    T executeWithRecovery(std::function<T()> operation, const ErrorContext& context);
    
    RecoveryResult handleError(const std::exception& e, const ErrorContext& context);
    RecoveryResult handleError(const QString& errorMessage, const ErrorContext& context);
    RecoveryResult recoverFromError(const ErrorContext& context);

    // Specific recovery strategies
    RecoveryResult retryOperation(std::function<bool()> operation, const ErrorContext& context);
    RecoveryResult fallbackOperation(const ErrorContext& context);
    RecoveryResult degradeOperation(const ErrorContext& context);
    RecoveryResult skipOperation(const ErrorContext& context);
    RecoveryResult resetOperation(const ErrorContext& context);

    // Circuit breaker pattern
    void enableCircuitBreaker(const QString& operationName, int failureThreshold = 5, int timeoutMs = 60000);
    void disableCircuitBreaker(const QString& operationName);
    bool isCircuitBreakerOpen(const QString& operationName) const;
    void recordOperationSuccess(const QString& operationName);
    void recordOperationFailure(const QString& operationName);

    // State management
    void saveOperationState(const QString& operationId, const QVariantMap& state);
    QVariantMap restoreOperationState(const QString& operationId);
    void clearOperationState(const QString& operationId);

    // Error statistics and monitoring
    struct ErrorStats {
        int totalErrors = 0;
        int recoveredErrors = 0;
        int failedRecoveries = 0;
        QHash<ErrorType, int> errorCounts;
        QHash<RecoveryStrategy, int> strategyCounts;
        QDateTime lastError;
        QStringList recentErrors;
    };
    
    ErrorStats getErrorStats() const;
    void resetErrorStats();
    QStringList getRecentErrors(int maxCount = 10) const;

    // Fallback mechanisms
    using FallbackFunction = std::function<QVariant(const ErrorContext&)>;
    void registerFallback(ErrorType type, const QString& operation, FallbackFunction fallback);
    void unregisterFallback(ErrorType type, const QString& operation);
    QVariant executeFallback(ErrorType type, const QString& operation, const ErrorContext& context);

    // Health monitoring
    bool isComponentHealthy(const QString& component) const;
    void reportComponentHealth(const QString& component, bool healthy);
    QStringList getUnhealthyComponents() const;

signals:
    void errorOccurred(const ErrorContext& context);
    void recoveryAttempted(const ErrorContext& context, RecoveryStrategy strategy);
    void recoverySucceeded(const ErrorContext& context, const RecoveryResult& result);
    void recoveryFailed(const ErrorContext& context, const RecoveryResult& result);
    void circuitBreakerOpened(const QString& operationName);
    void circuitBreakerClosed(const QString& operationName);
    void componentHealthChanged(const QString& component, bool healthy);

private slots:
    void onRecoveryTimer();
    void onCircuitBreakerTimer();

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
    
    // Helper methods
    ErrorType classifyError(const std::exception& e) const;
    ErrorType classifyError(const QString& errorMessage) const;
    int calculateRetryDelay(int attempt, const RecoveryConfig& config) const;
    bool shouldRetry(const ErrorContext& context, const RecoveryConfig& config) const;
    void logRecoveryAttempt(const ErrorContext& context, RecoveryStrategy strategy) const;
    void updateErrorStats(const ErrorContext& context, const RecoveryResult& result) const;
};

/**
 * RAII class for automatic error recovery scope
 */
class SearchErrorScope
{
public:
    explicit SearchErrorScope(SearchErrorRecovery* recovery, const SearchErrorRecovery::ErrorContext& context);
    ~SearchErrorScope();
    
    void setSuccessful(bool success = true);
    void addMetadata(const QString& key, const QVariant& value);
    void updateDetails(const QString& details);
    
private:
    SearchErrorRecovery* m_recovery;
    SearchErrorRecovery::ErrorContext m_context;
    bool m_successful;
    bool m_committed;
};

/**
 * Exception classes for search-specific errors
 */
class SearchException : public std::exception
{
public:
    explicit SearchException(const QString& message, SearchErrorRecovery::ErrorType type = SearchErrorRecovery::SearchError)
        : m_message(message.toStdString()), m_type(type) {}
    
    const char* what() const noexcept override { return m_message.c_str(); }
    SearchErrorRecovery::ErrorType type() const { return m_type; }
    
private:
    std::string m_message;
    SearchErrorRecovery::ErrorType m_type;
};

// ValidationException is defined in SearchValidator.h

class DocumentException : public SearchException
{
public:
    explicit DocumentException(const QString& message)
        : SearchException(message, SearchErrorRecovery::DocumentError) {}
};

class CacheException : public SearchException
{
public:
    explicit CacheException(const QString& message)
        : SearchException(message, SearchErrorRecovery::CacheError) {}
};

class TimeoutException : public SearchException
{
public:
    explicit TimeoutException(const QString& message)
        : SearchException(message, SearchErrorRecovery::TimeoutError) {}
};

/**
 * Convenience macros for error recovery
 */
#define SEARCH_ERROR_SCOPE(recovery, type, operation, component) \
    SearchErrorScope _scope(recovery, SearchErrorRecovery::ErrorContext(type, operation, component))

#define SEARCH_TRY_RECOVER(recovery, operation, context) \
    recovery->executeWithRecovery<bool>([&]() -> bool { \
        operation; \
        return true; \
    }, context)

#define SEARCH_HANDLE_ERROR(recovery, error, context) \
    recovery->handleError(error, context)

#define SEARCH_FALLBACK(recovery, type, operation, context) \
    recovery->executeFallback(type, operation, context)

// Template implementation
template<typename T>
T SearchErrorRecovery::executeWithRecovery(std::function<T()> operation, const ErrorContext& context)
{
    RecoveryConfig config = getRecoveryConfig(context.type);
    ErrorContext mutableContext = context;
    
    for (int attempt = 0; attempt < config.maxRetries + 1; ++attempt) {
        try {
            mutableContext.attemptCount = attempt + 1;
            
            if (attempt > 0) {
                int delay = calculateRetryDelay(attempt, config);
                if (delay > 0) {
                    QThread::msleep(delay);
                }
                logRecoveryAttempt(mutableContext, RecoveryStrategy::Retry);
            }
            
            T result = operation();
            
            if (attempt > 0) {
                RecoveryResult recoveryResult(true, RecoveryStrategy::Retry, attempt + 1);
                emit recoverySucceeded(mutableContext, recoveryResult);
                updateErrorStats(mutableContext, recoveryResult);
            }
            
            return result;
            
        } catch (const std::exception& e) {
            mutableContext.details = QString::fromStdString(e.what());
            
            if (attempt >= config.maxRetries) {
                RecoveryResult recoveryResult(false, RecoveryStrategy::Retry, attempt + 1, mutableContext.details);
                emit recoveryFailed(mutableContext, recoveryResult);
                updateErrorStats(mutableContext, recoveryResult);
                throw;
            }
            
            emit recoveryAttempted(mutableContext, RecoveryStrategy::Retry);
        }
    }
    
    // Should never reach here, but just in case
    throw SearchException("Maximum retry attempts exceeded");
}
