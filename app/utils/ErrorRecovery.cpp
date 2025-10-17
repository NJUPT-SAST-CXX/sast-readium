#include "ErrorRecovery.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QThread>
#include <algorithm>
#include "../logging/Logger.h"

namespace ErrorRecovery {

// CircuitBreaker Implementation
CircuitBreaker::CircuitBreaker(int failureThreshold,
                               std::chrono::milliseconds timeout)
    : m_failureThreshold(failureThreshold), m_timeout(timeout) {}

bool CircuitBreaker::canExecute() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_state == CircuitState::Closed) {
        return true;
    }

    if (m_state == CircuitState::Open) {
        auto now = std::chrono::steady_clock::now();
        if (now - m_lastFailureTime >= m_timeout) {
            const_cast<CircuitBreaker*>(this)->transitionToHalfOpen();
            return true;
        }
        return false;
    }

    // HalfOpen state - allow one test request
    return true;
}

void CircuitBreaker::recordSuccess() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_successCount++;

    if (m_state == CircuitState::HalfOpen) {
        transitionToClosed();
    }
}

void CircuitBreaker::recordFailure() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_failureCount++;
    m_lastFailureTime = std::chrono::steady_clock::now();

    if (m_state == CircuitState::Closed &&
        m_failureCount >= m_failureThreshold) {
        transitionToOpen();
    } else if (m_state == CircuitState::HalfOpen) {
        transitionToOpen();
    }
}

void CircuitBreaker::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_failureCount = 0;
    m_successCount = 0;
    transitionToClosed();
}

void CircuitBreaker::transitionToOpen() {
    auto oldState = m_state.load();
    m_state = CircuitState::Open;
    Logger::instance().warning(
        "Circuit breaker transitioned to OPEN state (failures: {})",
        m_failureCount.load());
}

void CircuitBreaker::transitionToHalfOpen() {
    auto oldState = m_state.load();
    m_state = CircuitState::HalfOpen;
    Logger::instance().info("Circuit breaker transitioned to HALF-OPEN state");
}

void CircuitBreaker::transitionToClosed() {
    auto oldState = m_state.load();
    m_state = CircuitState::Closed;
    m_failureCount = 0;
    Logger::instance().info("Circuit breaker transitioned to CLOSED state");
}

// RecoveryManager Implementation
RecoveryManager& RecoveryManager::instance() {
    static RecoveryManager instance;
    return instance;
}

void RecoveryManager::registerRecoveryAction(
    ErrorHandling::ErrorCategory category,
    std::shared_ptr<IRecoveryAction> action) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_recoveryActions[category] = action;
    Logger::instance().info(
        "Registered recovery action for category: {}",
        ErrorHandling::categoryToString(category).toStdString());
}

RecoveryResult RecoveryManager::executeRecovery(
    const ErrorHandling::ErrorInfo& error, const QString& componentName,
    const QString& operationName) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_recoveryActions.find(error.category);
    if (it == m_recoveryActions.end()) {
        Logger::instance().warning(
            "No recovery action registered for category: {}",
            ErrorHandling::categoryToString(error.category).toStdString());
        updateStats(componentName, RecoveryResult::Failed);
        return RecoveryResult::Failed;
    }

    try {
        Logger::instance().info(
            "Executing recovery for {}.{}: {}", componentName.toStdString(),
            operationName.toStdString(), error.message.toStdString());

        RecoveryResult result = it->second->execute(error);
        updateStats(componentName, result);

        emit recoveryAttempted(componentName, operationName, result);
        return result;

    } catch (const std::exception& e) {
        Logger::instance().error("Recovery action failed with exception: {}",
                                 e.what());
        updateStats(componentName, RecoveryResult::Failed);
        return RecoveryResult::Failed;
    }
}

CircuitBreaker& RecoveryManager::getCircuitBreaker(const QString& name) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_circuitBreakers.find(name);
    if (it == m_circuitBreakers.end()) {
        m_circuitBreakers[name] = std::make_unique<CircuitBreaker>();
        Logger::instance().info("Created circuit breaker: {}",
                                name.toStdString());
    }

    return *m_circuitBreakers[name];
}

void RecoveryManager::resetCircuitBreaker(const QString& name) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_circuitBreakers.find(name);
    if (it != m_circuitBreakers.end()) {
        it->second->reset();
        Logger::instance().info("Reset circuit breaker: {}",
                                name.toStdString());
    }
}

std::chrono::milliseconds RecoveryManager::calculateDelay(
    const RetryConfig& config, int attempt) const {
    switch (config.policy) {
        case RetryPolicy::None:
        case RetryPolicy::Immediate:
            return std::chrono::milliseconds(0);

        case RetryPolicy::FixedDelay:
            return config.initialDelay;

        case RetryPolicy::ExponentialBackoff: {
            auto delay = config.initialDelay *
                         static_cast<long long>(
                             std::pow(config.backoffMultiplier, attempt - 1));
            return std::min(delay, config.maxDelay);
        }

        case RetryPolicy::LinearBackoff: {
            auto delay = config.initialDelay * attempt;
            return std::min(delay, config.maxDelay);
        }
    }

    return config.initialDelay;
}

void RecoveryManager::updateStats(const QString& componentName,
                                  RecoveryResult result) {
    auto& stats = m_stats[componentName];
    stats.totalAttempts++;
    stats.lastRecovery = QDateTime::currentDateTime();

    if (result == RecoveryResult::Success) {
        stats.successfulRecoveries++;
    } else {
        stats.failedRecoveries++;
    }
}

RecoveryManager::RecoveryStats RecoveryManager::getStats(
    const QString& componentName) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (componentName.isEmpty()) {
        // Return aggregated stats
        RecoveryStats total;
        for (const auto& pair : m_stats) {
            total.totalAttempts += pair.second.totalAttempts;
            total.successfulRecoveries += pair.second.successfulRecoveries;
            total.failedRecoveries += pair.second.failedRecoveries;
            if (pair.second.lastRecovery > total.lastRecovery) {
                total.lastRecovery = pair.second.lastRecovery;
            }
        }
        return total;
    }

    auto it = m_stats.find(componentName);
    return (it != m_stats.end()) ? it->second : RecoveryStats{};
}

void RecoveryManager::resetStats() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats.clear();
    Logger::instance().info("Recovery statistics reset");
}

// Recovery Action Implementations
RecoveryResult FileSystemRecoveryAction::execute(
    const ErrorHandling::ErrorInfo& error) {
    Logger::instance().info("Attempting file system recovery for: {}",
                            error.message.toStdString());

    // Extract file path from error details
    QString filePath = error.details;
    if (filePath.contains("Path: ")) {
        filePath = filePath.mid(filePath.indexOf("Path: ") + 6);
        if (filePath.contains(",")) {
            filePath = filePath.left(filePath.indexOf(","));
        }
    }

    if (filePath.isEmpty()) {
        return RecoveryResult::Failed;
    }

    QFileInfo fileInfo(filePath);

    // Try to create parent directory if it doesn't exist
    if (!fileInfo.dir().exists()) {
        if (fileInfo.dir().mkpath(".")) {
            Logger::instance().info("Created missing directory: {}",
                                    fileInfo.dir().path().toStdString());
            return RecoveryResult::Retry;
        }
    }

    // Check if file is locked and suggest retry
    if (error.message.contains("locked") ||
        error.message.contains("access denied")) {
        QThread::msleep(100);  // Brief wait
        return RecoveryResult::Retry;
    }

    // For missing files, suggest fallback
    if (!fileInfo.exists()) {
        return RecoveryResult::Fallback;
    }

    return RecoveryResult::Failed;
}

RecoveryResult DocumentRecoveryAction::execute(
    const ErrorHandling::ErrorInfo& error) {
    Logger::instance().info("Attempting document recovery for: {}",
                            error.message.toStdString());

    // For document parsing errors, try fallback rendering
    if (error.message.contains("parse") || error.message.contains("invalid")) {
        return RecoveryResult::Fallback;
    }

    // For memory issues, suggest cleanup and retry
    if (error.message.contains("memory") ||
        error.message.contains("allocation")) {
        // Trigger garbage collection hint
        return RecoveryResult::Retry;
    }

    return RecoveryResult::Failed;
}

RecoveryResult RenderingRecoveryAction::execute(
    const ErrorHandling::ErrorInfo& error) {
    Logger::instance().info("Attempting rendering recovery for: {}",
                            error.message.toStdString());

    // Check both message and details for error patterns
    QString fullErrorText = error.message + " " + error.details;

    // For DPI or resolution issues, try with lower quality
    if (fullErrorText.contains("DPI", Qt::CaseInsensitive) ||
        fullErrorText.contains("resolution", Qt::CaseInsensitive) ||
        fullErrorText.contains("high", Qt::CaseInsensitive)) {
        Logger::instance().info(
            "DPI/resolution issue detected, suggesting fallback");
        return RecoveryResult::Fallback;
    }

    // For memory issues during rendering
    if (fullErrorText.contains("memory", Qt::CaseInsensitive) ||
        fullErrorText.contains("allocation", Qt::CaseInsensitive)) {
        Logger::instance().info("Memory issue detected, suggesting fallback");
        return RecoveryResult::Fallback;
    }

    // For timeout issues
    if (fullErrorText.contains("timeout", Qt::CaseInsensitive)) {
        Logger::instance().info("Timeout detected, suggesting retry");
        return RecoveryResult::Retry;
    }

    Logger::instance().warning(
        "No specific recovery strategy for rendering error: {}",
        error.message.toStdString());
    return RecoveryResult::Failed;
}

RecoveryResult SearchRecoveryAction::execute(
    const ErrorHandling::ErrorInfo& error) {
    Logger::instance().info("Attempting search recovery for: {}",
                            error.message.toStdString());

    QString fullErrorText = error.message + " " + error.details;

    // For search timeout, try with simpler query
    if (fullErrorText.contains("timeout", Qt::CaseInsensitive)) {
        return RecoveryResult::Fallback;
    }

    // For complex regex errors, fallback to simple text search
    if (fullErrorText.contains("regex", Qt::CaseInsensitive) ||
        fullErrorText.contains("pattern", Qt::CaseInsensitive)) {
        return RecoveryResult::Fallback;
    }

    return RecoveryResult::Retry;
}

RecoveryResult CacheRecoveryAction::execute(
    const ErrorHandling::ErrorInfo& error) {
    Logger::instance().info("Attempting cache recovery for: {}",
                            error.message.toStdString());

    // Cache errors are usually non-critical, continue without cache
    return RecoveryResult::Fallback;
}

// Utility Functions
namespace Utils {
RetryConfig createQuickRetry() {
    return RetryConfig(RetryPolicy::Immediate, 2, std::chrono::milliseconds(0));
}

RetryConfig createStandardRetry() {
    return RetryConfig(RetryPolicy::ExponentialBackoff, 3,
                       std::chrono::milliseconds(100));
}

RetryConfig createPatientRetry() {
    return RetryConfig(RetryPolicy::ExponentialBackoff, 5,
                       std::chrono::milliseconds(500));
}

RetryConfig createNetworkRetry() {
    return RetryConfig(RetryPolicy::ExponentialBackoff, 4,
                       std::chrono::milliseconds(1000));
}
}  // namespace Utils

}  // namespace ErrorRecovery

