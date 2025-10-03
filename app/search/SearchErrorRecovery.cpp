#include "SearchErrorRecovery.h"
#include <QCoreApplication>
#include <QDebug>
#include <QMutexLocker>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QThread>
#include <cmath>

class SearchErrorRecovery::Implementation {
public:
    Implementation(SearchErrorRecovery* q)
        : q_ptr(q),
          globalRecoveryEnabled(true),
          recoveryTimer(new QTimer(q)),
          circuitBreakerTimer(new QTimer(q)) {
        // Initialize default recovery configurations
        initializeDefaultConfigs();

        // Setup timers
        recoveryTimer->setSingleShot(false);
        circuitBreakerTimer->setSingleShot(false);

        QObject::connect(recoveryTimer, &QTimer::timeout, q,
                         &SearchErrorRecovery::onRecoveryTimer);
        QObject::connect(circuitBreakerTimer, &QTimer::timeout, q,
                         &SearchErrorRecovery::onCircuitBreakerTimer);

        // Start monitoring timers
        recoveryTimer->start(5000);  // Check every 5 seconds
        circuitBreakerTimer->start(
            10000);  // Check circuit breakers every 10 seconds
    }

    void initializeDefaultConfigs() {
        // Default configuration for each error type
        RecoveryConfig defaultConfig;
        defaultConfig.strategy = Retry;
        defaultConfig.maxRetries = 3;
        defaultConfig.retryDelayMs = 1000;
        defaultConfig.exponentialBackoff = true;
        defaultConfig.enableFallback = true;
        defaultConfig.enableDegradation = true;
        defaultConfig.timeoutMs = 30000;
        defaultConfig.logRecoveryAttempts = true;

        // Customize for specific error types
        recoveryConfigs[ValidationError] = defaultConfig;

        RecoveryConfig documentConfig = defaultConfig;
        documentConfig.maxRetries = 2;
        documentConfig.retryDelayMs = 2000;
        recoveryConfigs[DocumentError] = documentConfig;

        RecoveryConfig searchConfig = defaultConfig;
        searchConfig.strategy = Fallback;
        searchConfig.maxRetries = 2;
        recoveryConfigs[SearchError] = searchConfig;

        RecoveryConfig cacheConfig = defaultConfig;
        cacheConfig.strategy = Skip;
        cacheConfig.maxRetries = 1;
        recoveryConfigs[CacheError] = cacheConfig;

        RecoveryConfig memoryConfig = defaultConfig;
        memoryConfig.strategy = Degrade;
        memoryConfig.maxRetries = 1;
        memoryConfig.retryDelayMs = 5000;
        recoveryConfigs[MemoryError] = memoryConfig;

        RecoveryConfig timeoutConfig = defaultConfig;
        timeoutConfig.strategy = Retry;
        timeoutConfig.maxRetries = 2;
        timeoutConfig.retryDelayMs = 3000;
        recoveryConfigs[TimeoutError] = timeoutConfig;
    }

    SearchErrorRecovery* q_ptr;
    bool globalRecoveryEnabled;

    // Configuration
    QHash<ErrorType, RecoveryConfig> recoveryConfigs;

    // Circuit breaker state
    struct CircuitBreakerState {
        int failureCount = 0;
        int failureThreshold = 5;
        QDateTime lastFailure;
        QDateTime openedAt;
        int timeoutMs = 60000;
        bool isOpen = false;
    };
    QHash<QString, CircuitBreakerState> circuitBreakers;

    // Operation state management
    QHash<QString, QVariantMap> operationStates;

    // Fallback functions
    QHash<QPair<ErrorType, QString>, FallbackFunction> fallbackFunctions;

    // Component health tracking
    QHash<QString, bool> componentHealth;
    QHash<QString, QDateTime> lastHealthCheck;

    // Statistics
    mutable ErrorStats stats;

    // Thread safety
    mutable QMutex mutex;

    // Timers
    QTimer* recoveryTimer;
    QTimer* circuitBreakerTimer;
};

SearchErrorRecovery::SearchErrorRecovery(QObject* parent)
    : QObject(parent), d(std::make_unique<Implementation>(this)) {}

SearchErrorRecovery::~SearchErrorRecovery() = default;

void SearchErrorRecovery::setRecoveryConfig(ErrorType type,
                                            const RecoveryConfig& config) {
    QMutexLocker locker(&d->mutex);
    d->recoveryConfigs[type] = config;
}

SearchErrorRecovery::RecoveryConfig SearchErrorRecovery::getRecoveryConfig(
    ErrorType type) const {
    QMutexLocker locker(&d->mutex);
    return d->recoveryConfigs.value(type, RecoveryConfig());
}

void SearchErrorRecovery::setGlobalRecoveryEnabled(bool enabled) {
    QMutexLocker locker(&d->mutex);
    d->globalRecoveryEnabled = enabled;
}

bool SearchErrorRecovery::isGlobalRecoveryEnabled() const {
    QMutexLocker locker(&d->mutex);
    return d->globalRecoveryEnabled;
}

SearchErrorRecovery::RecoveryResult SearchErrorRecovery::handleError(
    const std::exception& e, const ErrorContext& context) {
    ErrorContext mutableContext = context;
    mutableContext.type = classifyError(e);
    mutableContext.details = QString::fromStdString(e.what());

    return recoverFromError(mutableContext);
}

SearchErrorRecovery::RecoveryResult SearchErrorRecovery::handleError(
    const QString& errorMessage, const ErrorContext& context) {
    ErrorContext mutableContext = context;
    mutableContext.type = classifyError(errorMessage);
    mutableContext.details = errorMessage;

    return recoverFromError(mutableContext);
}

SearchErrorRecovery::RecoveryResult SearchErrorRecovery::recoverFromError(
    const ErrorContext& context) {
    QMutexLocker locker(&d->mutex);

    if (!d->globalRecoveryEnabled) {
        return RecoveryResult(false, NoRecovery, 0, "Global recovery disabled");
    }

    // Update statistics
    d->stats.totalErrors++;
    d->stats.errorCounts[context.type]++;
    d->stats.lastError = context.timestamp;
    d->stats.recentErrors.append(
        QString("%1: %2").arg(context.operation, context.details));
    if (d->stats.recentErrors.size() > 100) {
        d->stats.recentErrors.removeFirst();
    }

    emit errorOccurred(context);

    RecoveryConfig config =
        d->recoveryConfigs.value(context.type, RecoveryConfig());
    RecoveryResult result;

    switch (config.strategy) {
        case Retry:
            result = retryOperation([]() { return true; }, context);
            break;
        case Fallback:
            result = fallbackOperation(context);
            break;
        case Degrade:
            result = degradeOperation(context);
            break;
        case Skip:
            result = skipOperation(context);
            break;
        case Reset:
            result = resetOperation(context);
            break;
        default:
            result = RecoveryResult(false, NoRecovery, 0,
                                    "No recovery strategy configured");
            break;
    }

    // Update statistics
    d->stats.strategyCounts[result.usedStrategy]++;
    if (result.success) {
        d->stats.recoveredErrors++;
        emit recoverySucceeded(context, result);
    } else {
        d->stats.failedRecoveries++;
        emit recoveryFailed(context, result);
    }

    return result;
}

SearchErrorRecovery::RecoveryResult SearchErrorRecovery::retryOperation(
    std::function<bool()> operation, const ErrorContext& context) {
    RecoveryConfig config = getRecoveryConfig(context.type);

    for (int attempt = 1; attempt <= config.maxRetries; ++attempt) {
        try {
            if (attempt > 1) {
                int delay = calculateRetryDelay(attempt - 1, config);
                if (delay > 0) {
                    QThread::msleep(delay);
                }
            }

            logRecoveryAttempt(context, Retry);
            emit recoveryAttempted(context, Retry);

            if (operation()) {
                return RecoveryResult(true, Retry, attempt, "Retry successful");
            }

        } catch (const std::exception& e) {
            if (attempt >= config.maxRetries) {
                return RecoveryResult(
                    false, Retry, attempt,
                    QString("Retry failed: %1").arg(e.what()));
            }
        }
    }

    return RecoveryResult(false, Retry, config.maxRetries,
                          "Maximum retries exceeded");
}

SearchErrorRecovery::RecoveryResult SearchErrorRecovery::fallbackOperation(
    const ErrorContext& context) {
    QMutexLocker locker(&d->mutex);

    QPair<ErrorType, QString> key(context.type, context.operation);
    if (d->fallbackFunctions.contains(key)) {
        try {
            logRecoveryAttempt(context, Fallback);
            emit recoveryAttempted(context, Fallback);

            QVariant result = d->fallbackFunctions[key](context);
            return RecoveryResult(true, Fallback, 1,
                                  "Fallback executed successfully");

        } catch (const std::exception& e) {
            return RecoveryResult(false, Fallback, 1,
                                  QString("Fallback failed: %1").arg(e.what()));
        }
    }

    return RecoveryResult(false, Fallback, 0,
                          "No fallback function registered");
}

SearchErrorRecovery::RecoveryResult SearchErrorRecovery::degradeOperation(
    const ErrorContext& context) {
    logRecoveryAttempt(context, Degrade);
    emit recoveryAttempted(context, Degrade);

    // Mark component as degraded
    d->componentHealth[context.component] = false;
    emit componentHealthChanged(context.component, false);

    return RecoveryResult(true, Degrade, 1, "Operation degraded");
}

SearchErrorRecovery::RecoveryResult SearchErrorRecovery::skipOperation(
    const ErrorContext& context) {
    logRecoveryAttempt(context, Skip);
    emit recoveryAttempted(context, Skip);

    return RecoveryResult(true, Skip, 1, "Operation skipped");
}

SearchErrorRecovery::RecoveryResult SearchErrorRecovery::resetOperation(
    const ErrorContext& context) {
    logRecoveryAttempt(context, Reset);
    emit recoveryAttempted(context, Reset);

    // Clear operation state
    clearOperationState(context.operation);

    // Reset component health
    d->componentHealth[context.component] = true;
    emit componentHealthChanged(context.component, true);

    return RecoveryResult(true, Reset, 1, "Operation reset");
}

void SearchErrorRecovery::enableCircuitBreaker(const QString& operationName,
                                               int failureThreshold,
                                               int timeoutMs) {
    QMutexLocker locker(&d->mutex);

    Implementation::CircuitBreakerState& state =
        d->circuitBreakers[operationName];
    state.failureThreshold = failureThreshold;
    state.timeoutMs = timeoutMs;
    state.isOpen = false;
    state.failureCount = 0;
}

void SearchErrorRecovery::disableCircuitBreaker(const QString& operationName) {
    QMutexLocker locker(&d->mutex);
    d->circuitBreakers.remove(operationName);
}

bool SearchErrorRecovery::isCircuitBreakerOpen(
    const QString& operationName) const {
    QMutexLocker locker(&d->mutex);

    if (!d->circuitBreakers.contains(operationName)) {
        return false;
    }

    const auto& state = d->circuitBreakers[operationName];
    if (!state.isOpen) {
        return false;
    }

    // Check if timeout has elapsed
    QDateTime now = QDateTime::currentDateTime();
    if (state.openedAt.msecsTo(now) >= state.timeoutMs) {
        // Circuit breaker should be closed
        const_cast<Implementation::CircuitBreakerState&>(state).isOpen = false;
        emit const_cast<SearchErrorRecovery*>(this)->circuitBreakerClosed(
            operationName);
        return false;
    }

    return true;
}

void SearchErrorRecovery::recordOperationSuccess(const QString& operationName) {
    QMutexLocker locker(&d->mutex);

    if (d->circuitBreakers.contains(operationName)) {
        auto& state = d->circuitBreakers[operationName];
        state.failureCount = 0;
        if (state.isOpen) {
            state.isOpen = false;
            emit circuitBreakerClosed(operationName);
        }
    }
}

void SearchErrorRecovery::recordOperationFailure(const QString& operationName) {
    QMutexLocker locker(&d->mutex);

    if (d->circuitBreakers.contains(operationName)) {
        auto& state = d->circuitBreakers[operationName];
        state.failureCount++;
        state.lastFailure = QDateTime::currentDateTime();

        if (!state.isOpen && state.failureCount >= state.failureThreshold) {
            state.isOpen = true;
            state.openedAt = QDateTime::currentDateTime();
            emit circuitBreakerOpened(operationName);
        }
    }
}

void SearchErrorRecovery::saveOperationState(const QString& operationId,
                                             const QVariantMap& state) {
    QMutexLocker locker(&d->mutex);
    d->operationStates[operationId] = state;
}

QVariantMap SearchErrorRecovery::restoreOperationState(
    const QString& operationId) {
    QMutexLocker locker(&d->mutex);
    return d->operationStates.value(operationId, QVariantMap());
}

void SearchErrorRecovery::clearOperationState(const QString& operationId) {
    QMutexLocker locker(&d->mutex);
    d->operationStates.remove(operationId);
}

SearchErrorRecovery::ErrorStats SearchErrorRecovery::getErrorStats() const {
    QMutexLocker locker(&d->mutex);
    return d->stats;
}

void SearchErrorRecovery::resetErrorStats() {
    QMutexLocker locker(&d->mutex);
    d->stats = ErrorStats();
}

QStringList SearchErrorRecovery::getRecentErrors(int maxCount) const {
    QMutexLocker locker(&d->mutex);

    QStringList recent = d->stats.recentErrors;
    if (recent.size() > maxCount) {
        recent = recent.mid(recent.size() - maxCount);
    }

    return recent;
}

void SearchErrorRecovery::registerFallback(ErrorType type,
                                           const QString& operation,
                                           FallbackFunction fallback) {
    QMutexLocker locker(&d->mutex);
    d->fallbackFunctions[QPair<ErrorType, QString>(type, operation)] = fallback;
}

void SearchErrorRecovery::unregisterFallback(ErrorType type,
                                             const QString& operation) {
    QMutexLocker locker(&d->mutex);
    d->fallbackFunctions.remove(QPair<ErrorType, QString>(type, operation));
}

QVariant SearchErrorRecovery::executeFallback(ErrorType type,
                                              const QString& operation,
                                              const ErrorContext& context) {
    QMutexLocker locker(&d->mutex);

    QPair<ErrorType, QString> key(type, operation);
    if (d->fallbackFunctions.contains(key)) {
        return d->fallbackFunctions[key](context);
    }

    return QVariant();
}

bool SearchErrorRecovery::isComponentHealthy(const QString& component) const {
    QMutexLocker locker(&d->mutex);
    return d->componentHealth.value(component, true);
}

void SearchErrorRecovery::reportComponentHealth(const QString& component,
                                                bool healthy) {
    QMutexLocker locker(&d->mutex);

    bool wasHealthy = d->componentHealth.value(component, true);
    d->componentHealth[component] = healthy;
    d->lastHealthCheck[component] = QDateTime::currentDateTime();

    if (wasHealthy != healthy) {
        emit componentHealthChanged(component, healthy);
    }
}

QStringList SearchErrorRecovery::getUnhealthyComponents() const {
    QMutexLocker locker(&d->mutex);

    QStringList unhealthy;
    for (auto it = d->componentHealth.constBegin();
         it != d->componentHealth.constEnd(); ++it) {
        if (!it.value()) {
            unhealthy.append(it.key());
        }
    }

    return unhealthy;
}

// Helper methods implementation
SearchErrorRecovery::ErrorType SearchErrorRecovery::classifyError(
    const std::exception& e) const {
    QString errorMsg = QString::fromStdString(e.what()).toLower();
    return classifyError(errorMsg);
}

SearchErrorRecovery::ErrorType SearchErrorRecovery::classifyError(
    const QString& errorMessage) const {
    QString msg = errorMessage.toLower();

    if (msg.contains("validation") || msg.contains("invalid") ||
        msg.contains("malformed")) {
        return ValidationError;
    }
    if (msg.contains("document") || msg.contains("pdf") ||
        msg.contains("page") || msg.contains("load")) {
        return DocumentError;
    }
    if (msg.contains("search") || msg.contains("query") ||
        msg.contains("pattern") || msg.contains("regex")) {
        return SearchError;
    }
    if (msg.contains("cache") || msg.contains("storage")) {
        return CacheError;
    }
    if (msg.contains("memory") || msg.contains("allocation") ||
        msg.contains("out of")) {
        return MemoryError;
    }
    if (msg.contains("timeout") || msg.contains("time") ||
        msg.contains("deadline")) {
        return TimeoutError;
    }
    if (msg.contains("network") || msg.contains("connection") ||
        msg.contains("socket")) {
        return NetworkError;
    }

    return UnknownError;
}

int SearchErrorRecovery::calculateRetryDelay(
    int attempt, const RecoveryConfig& config) const {
    if (!config.exponentialBackoff) {
        return config.retryDelayMs;
    }

    // Exponential backoff with jitter
    int baseDelay = config.retryDelayMs;
    int exponentialDelay =
        static_cast<int>(baseDelay * std::pow(2, attempt - 1));

    // Add jitter (±25%)
    int jitter = exponentialDelay / 4;
    int randomJitter =
        (QRandomGenerator::global()->bounded(2 * jitter + 1)) - jitter;

    return qMax(baseDelay, exponentialDelay + randomJitter);
}

bool SearchErrorRecovery::shouldRetry(const ErrorContext& context,
                                      const RecoveryConfig& config) const {
    if (context.attemptCount >= config.maxRetries) {
        return false;
    }

    // Don't retry validation errors
    if (context.type == ValidationError) {
        return false;
    }

    // Check circuit breaker
    if (isCircuitBreakerOpen(context.operation)) {
        return false;
    }

    return true;
}

void SearchErrorRecovery::logRecoveryAttempt(const ErrorContext& context,
                                             RecoveryStrategy strategy) const {
    if (!getRecoveryConfig(context.type).logRecoveryAttempts) {
        return;
    }

    QString strategyName;
    switch (strategy) {
        case Retry:
            strategyName = "Retry";
            break;
        case Fallback:
            strategyName = "Fallback";
            break;
        case Degrade:
            strategyName = "Degrade";
            break;
        case Skip:
            strategyName = "Skip";
            break;
        case Reset:
            strategyName = "Reset";
            break;
        default:
            strategyName = "Unknown";
            break;
    }

    qDebug() << QString(
                    "SearchErrorRecovery: Attempting %1 for %2 in %3 (attempt "
                    "%4): %5")
                    .arg(strategyName, context.operation, context.component)
                    .arg(context.attemptCount)
                    .arg(context.details);
}

void SearchErrorRecovery::updateErrorStats(const ErrorContext& context,
                                           const RecoveryResult& result) const {
    // Statistics are updated in recoverFromError method
    Q_UNUSED(context)
    Q_UNUSED(result)
}

void SearchErrorRecovery::onRecoveryTimer() {
    // Periodic recovery maintenance
    QMutexLocker locker(&d->mutex);

    // Clean up old operation states (older than 1 hour)
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-3600);

    auto it = d->operationStates.begin();
    while (it != d->operationStates.end()) {
        QVariantMap state = it.value();
        QDateTime timestamp =
            state.value("timestamp", QDateTime::currentDateTime()).toDateTime();

        if (timestamp < cutoff) {
            it = d->operationStates.erase(it);
        } else {
            ++it;
        }
    }

    // Check component health timeouts
    QDateTime healthCutoff =
        QDateTime::currentDateTime().addSecs(-300);  // 5 minutes
    for (auto it = d->lastHealthCheck.begin(); it != d->lastHealthCheck.end();
         ++it) {
        if (it.value() < healthCutoff) {
            QString component = it.key();
            if (d->componentHealth.value(component, true)) {
                d->componentHealth[component] = false;
                emit componentHealthChanged(component, false);
            }
        }
    }
}

void SearchErrorRecovery::onCircuitBreakerTimer() {
    // Check circuit breaker timeouts
    QMutexLocker locker(&d->mutex);
    QDateTime now = QDateTime::currentDateTime();

    for (auto it = d->circuitBreakers.begin(); it != d->circuitBreakers.end();
         ++it) {
        auto& state = it.value();
        if (state.isOpen && state.openedAt.msecsTo(now) >= state.timeoutMs) {
            state.isOpen = false;
            emit circuitBreakerClosed(it.key());
        }
    }
}

// SearchErrorScope implementation
SearchErrorScope::SearchErrorScope(
    SearchErrorRecovery* recovery,
    const SearchErrorRecovery::ErrorContext& context)
    : m_recovery(recovery),
      m_context(context),
      m_successful(false),
      m_committed(false) {
    m_context.timestamp = QDateTime::currentDateTime();
}

SearchErrorScope::~SearchErrorScope() {
    if (!m_committed && !m_successful && m_recovery) {
        // Automatic error handling on scope exit
        m_recovery->handleError(m_context.details, m_context);
    }
}

void SearchErrorScope::setSuccessful(bool success) {
    m_successful = success;
    m_committed = true;
}

void SearchErrorScope::addMetadata(const QString& key, const QVariant& value) {
    m_context.metadata[key] = value;
}

void SearchErrorScope::updateDetails(const QString& details) {
    m_context.details = details;
}
