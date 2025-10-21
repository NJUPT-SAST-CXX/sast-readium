#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QThread>
#include <QTimer>
#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include "../logging/Logger.h"
#include "ErrorHandling.h"

namespace ErrorRecovery {

/**
 * @brief Retry policy types
 */
enum class RetryPolicy {
    None,                // No retry
    Immediate,           // Immediate retry
    FixedDelay,          // Fixed delay between retries
    ExponentialBackoff,  // Exponential backoff
    LinearBackoff        // Linear increase in delay
};

/**
 * @brief Fallback strategy types
 */
enum class FallbackStrategy {
    None,                 // No fallback
    DefaultValue,         // Return default value
    CachedValue,          // Use cached value
    AlternativeMethod,    // Try alternative method
    GracefulDegradation,  // Reduce functionality
    UserPrompt            // Ask user for action
};

/**
 * @brief Recovery action result
 */
enum class RecoveryResult {
    Success,   // Recovery successful
    Failed,    // Recovery failed
    Retry,     // Should retry original operation
    Fallback,  // Should use fallback
    Abort      // Abort operation
};

/**
 * @brief Configuration for retry behavior
 */
struct RetryConfig {
    RetryPolicy policy = RetryPolicy::ExponentialBackoff;
    // Backward-compatible fields: both maxAttempts and maxRetries are supported
    int maxAttempts = 3;
    int maxRetries = 3;  // alias for legacy code paths
    std::chrono::milliseconds initialDelay{100};
    std::chrono::milliseconds maxDelay{5000};
    double backoffMultiplier = 2.0;

    RetryConfig() = default;
    RetryConfig(RetryPolicy p, int attempts, std::chrono::milliseconds delay)
        : policy(p),
          maxAttempts(attempts),
          maxRetries(attempts),
          initialDelay(delay) {}

    // Effective attempts count considering both fields
    int attempts() const { return (maxRetries > 0 ? maxRetries : maxAttempts); }
};
enum class CircuitState {
    Closed,   // Normal operation
    Open,     // Circuit open, failing fast
    HalfOpen  // Testing if service recovered
};

/**
 * @brief Circuit breaker for preventing cascading failures
 */
class CircuitBreaker {
public:
    CircuitBreaker(
        int failureThreshold = 5,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(60000));

    bool canExecute() const;
    void recordSuccess();
    void recordFailure();
    void reset();

    CircuitState getState() const { return m_state; }
    int getFailureCount() const { return m_failureCount; }

private:
    void transitionToOpen();
    void transitionToHalfOpen();
    void transitionToClosed();

    mutable std::atomic<CircuitState> m_state{CircuitState::Closed};
    std::atomic<int> m_failureCount{0};
    std::atomic<int> m_successCount{0};
    std::chrono::steady_clock::time_point m_lastFailureTime;

    const int m_failureThreshold;
    const std::chrono::milliseconds m_timeout;
    mutable std::mutex m_mutex;
};

/**
 * @brief Recovery action interface
 */
class IRecoveryAction {
public:
    virtual ~IRecoveryAction() = default;
    virtual RecoveryResult execute(const ErrorHandling::ErrorInfo& error) = 0;
    virtual QString getDescription() const = 0;
};

/**
 * @brief Recovery context containing error and recovery state
 */
struct RecoveryContext {
    ErrorHandling::ErrorInfo error;
    int attemptCount = 0;
    QDateTime firstAttempt;
    QDateTime lastAttempt;
    QString componentName;
    QString operationName;
    QVariantMap metadata;

    RecoveryContext(const ErrorHandling::ErrorInfo& err,
                    const QString& component, const QString& operation)
        : error(err),
          componentName(component),
          operationName(operation),
          firstAttempt(QDateTime::currentDateTime()) {}
};

/**
 * @brief Main recovery manager
 */
class RecoveryManager : public QObject {
    Q_OBJECT

public:
    static RecoveryManager& instance();

    // Register recovery actions for specific error categories
    void registerRecoveryAction(ErrorHandling::ErrorCategory category,
                                std::shared_ptr<IRecoveryAction> action);

    // Execute recovery for an error
    RecoveryResult executeRecovery(const ErrorHandling::ErrorInfo& error,
                                   const QString& componentName,
                                   const QString& operationName);

    // Retry with policy
    template <typename Func>
    auto retryWithPolicy(Func&& func, const RetryConfig& config,
                         const QString& context = QString())
        -> ErrorHandling::Result<decltype(func())> {
        using ReturnType = decltype(func());
        using ResultType = ErrorHandling::Result<ReturnType>;

        if (config.policy == RetryPolicy::None || config.attempts() <= 0) {
            return ErrorHandling::safeExecute(
                std::forward<Func>(func), ErrorHandling::ErrorCategory::Unknown,
                context);
        }

        ErrorHandling::ErrorInfo lastError;

        for (int attempt = 1; attempt <= config.attempts(); ++attempt) {
            auto result = ErrorHandling::safeExecute(
                std::forward<Func>(func), ErrorHandling::ErrorCategory::Unknown,
                context);

            if (ErrorHandling::isSuccess(result)) {
                if (attempt > 1) {
                    Logger::instance().info(
                        "Retry succeeded on attempt {} for: {}", attempt,
                        context.toStdString());
                }
                return result;
            }

            lastError = ErrorHandling::getError(result);

            if (attempt < config.attempts()) {
                auto delay = calculateDelay(config, attempt);
                if (delay.count() > 0) {
                    Logger::instance().info(
                        "Retrying in {}ms (attempt {}/{}) for: {}",
                        delay.count(), attempt, config.attempts(),
                        context.toStdString());
                    QThread::msleep(static_cast<unsigned long>(delay.count()));
                }
            }
        }

        Logger::instance().warning("All retry attempts failed for: {}",
                                   context.toStdString());
        return ErrorHandling::error<ReturnType>(lastError);
    }

    // Circuit breaker operations
    CircuitBreaker& getCircuitBreaker(const QString& name);
    void resetCircuitBreaker(const QString& name);

    // Configuration
    void setDefaultRetryConfig(const RetryConfig& config) {
        m_defaultRetryConfig = config;
    }
    const RetryConfig& getDefaultRetryConfig() const {
        return m_defaultRetryConfig;
    }

    // Statistics
    struct RecoveryStats {
        int totalAttempts = 0;
        int successfulRecoveries = 0;
        int failedRecoveries = 0;
        QDateTime lastRecovery;
    };

    RecoveryStats getStats(const QString& componentName = QString()) const;
    void resetStats();

signals:
    void recoveryAttempted(const QString& component, const QString& operation,
                           RecoveryResult result);
    void circuitBreakerStateChanged(const QString& name, CircuitState oldState,
                                    CircuitState newState);

private:
    RecoveryManager() = default;

    std::chrono::milliseconds calculateDelay(const RetryConfig& config,
                                             int attempt) const;
    void updateStats(const QString& componentName, RecoveryResult result);

    std::map<ErrorHandling::ErrorCategory, std::shared_ptr<IRecoveryAction>>
        m_recoveryActions;
    std::map<QString, std::unique_ptr<CircuitBreaker>> m_circuitBreakers;
    std::map<QString, RecoveryStats> m_stats;
    RetryConfig m_defaultRetryConfig;
    mutable std::mutex m_mutex;
};

/**
 * @brief Common recovery actions
 */
class FileSystemRecoveryAction : public IRecoveryAction {
public:
    RecoveryResult execute(const ErrorHandling::ErrorInfo& error) override;
    QString getDescription() const override {
        return "File system error recovery";
    }
};

class DocumentRecoveryAction : public IRecoveryAction {
public:
    RecoveryResult execute(const ErrorHandling::ErrorInfo& error) override;
    QString getDescription() const override {
        return "Document error recovery";
    }
};

class RenderingRecoveryAction : public IRecoveryAction {
public:
    RecoveryResult execute(const ErrorHandling::ErrorInfo& error) override;
    QString getDescription() const override {
        return "Rendering error recovery";
    }
};

class SearchRecoveryAction : public IRecoveryAction {
public:
    RecoveryResult execute(const ErrorHandling::ErrorInfo& error) override;
    QString getDescription() const override { return "Search error recovery"; }
};

class CacheRecoveryAction : public IRecoveryAction {
public:
    RecoveryResult execute(const ErrorHandling::ErrorInfo& error) override;
    QString getDescription() const override { return "Cache error recovery"; }
};

/**
 * @brief Utility functions for common recovery patterns
 */
namespace Utils {
// Create retry config for common scenarios
RetryConfig createQuickRetry();
RetryConfig createStandardRetry();
RetryConfig createPatientRetry();
RetryConfig createNetworkRetry();

// Helper for safe resource cleanup
template <typename Resource, typename Cleanup>
void safeCleanup(Resource& resource, Cleanup&& cleanup,
                 const QString& context = QString());

// Helper for state rollback
template <typename State>
class StateGuard {
public:
    StateGuard(State& state) : m_state(state), m_originalState(state) {}
    ~StateGuard() {
        if (!m_committed)
            m_state = m_originalState;
    }
    void commit() { m_committed = true; }
    void rollback() {
        m_state = m_originalState;
        m_committed = true;
    }

private:
    State& m_state;
    State m_originalState;
    bool m_committed = false;
};
}  // namespace Utils

}  // namespace ErrorRecovery

/**
 * @brief Convenience macros for error recovery
 */
#define RETRY_ON_ERROR(func, config)                            \
    ErrorRecovery::RecoveryManager::instance().retryWithPolicy( \
        [&]() { return func; }, config, #func)

#define WITH_CIRCUIT_BREAKER(name, func)                                      \
    [&]() {                                                                   \
        auto& breaker =                                                       \
            ErrorRecovery::RecoveryManager::instance().getCircuitBreaker(     \
                name);                                                        \
        if (!breaker.canExecute()) {                                          \
            throw ErrorHandling::ApplicationException(                        \
                ErrorHandling::ErrorCategory::Unknown,                        \
                ErrorHandling::ErrorSeverity::Error, "Circuit breaker open"); \
        }                                                                     \
        try {                                                                 \
            auto result = func;                                               \
            breaker.recordSuccess();                                          \
            return result;                                                    \
        } catch (...) {                                                       \
            breaker.recordFailure();                                          \
            throw;                                                            \
        }                                                                     \
    }()

#define WITH_STATE_GUARD(state) \
    auto stateGuard = ErrorRecovery::Utils::StateGuard(state)
