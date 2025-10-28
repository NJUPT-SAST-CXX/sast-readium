#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <functional>
#include "../../logging/SimpleLogging.h"
#include "../../utils/ErrorHandling.h"
#include "../../utils/ErrorRecovery.h"

/**
 * @brief Specialized recovery manager for UI-specific error scenarios
 *
 * Extends the general ErrorRecovery system with UI-specific recovery
 * strategies and user interaction patterns.
 */
class UIRecoveryManager : public QObject {
    Q_OBJECT

public:
    enum class RecoveryStrategy {
        AutomaticRetry,       // Retry operation automatically
        UserPrompt,           // Ask user what to do
        FallbackMethod,       // Use alternative approach
        GracefulDegradation,  // Reduce functionality
        ResetToDefault,       // Reset to safe state
        UserGuidance          // Show user how to fix
    };

    enum class UIErrorType {
        WidgetCreationFailed,    // Widget creation/initialization failed
        LayoutError,             // Layout management error
        StyleApplicationFailed,  // Theme/style application failed
        EventHandlingError,      // Event processing error
        DataBindingError,        // Model-view binding error
        ValidationError,         // Input validation error
        ResourceLoadError,       // UI resource loading error
        PermissionError,         // UI permission/access error
        StateCorruption,         // UI state corruption
        MemoryPressure           // UI memory pressure
    };

    struct RecoveryAction {
        RecoveryStrategy strategy;
        QString description;
        std::function<bool(QWidget*, const ErrorHandling::ErrorInfo&)> action;
        int priority;  // Higher priority actions tried first

        RecoveryAction(
            RecoveryStrategy s, const QString& desc,
            std::function<bool(QWidget*, const ErrorHandling::ErrorInfo&)> act,
            int prio = 0)
            : strategy(s), description(desc), action(act), priority(prio) {}
    };

    static UIRecoveryManager& instance();

    // Recovery registration
    void registerRecoveryAction(UIErrorType errorType,
                                const RecoveryAction& action);
    void registerFallbackWidget(const QString& widgetType,
                                std::function<QWidget*(QWidget*)> factory);

    // Recovery execution
    bool attemptRecovery(UIErrorType errorType, QWidget* context,
                         const ErrorHandling::ErrorInfo& error);
    bool recoverWidgetCreation(QWidget* parent, const QString& widgetType,
                               const ErrorHandling::ErrorInfo& error);
    bool recoverLayoutError(QWidget* widget,
                            const ErrorHandling::ErrorInfo& error);
    bool recoverStyleError(QWidget* widget,
                           const ErrorHandling::ErrorInfo& error);
    bool recoverDataBinding(QWidget* widget,
                            const ErrorHandling::ErrorInfo& error);

    // State management
    void saveWidgetState(QWidget* widget);
    bool restoreWidgetState(QWidget* widget);
    void clearSavedState(QWidget* widget);

    // User interaction
    bool promptUserForRecovery(QWidget* parent,
                               const ErrorHandling::ErrorInfo& error,
                               const QStringList& options);
    void showRecoveryGuidance(QWidget* parent, const QString& problem,
                              const QString& solution);

    // Configuration
    void setAutoRecoveryEnabled(bool enabled) {
        m_autoRecoveryEnabled = enabled;
    }
    void setMaxRetryAttempts(int attempts) { m_maxRetryAttempts = attempts; }
    void setRecoveryTimeout(int ms) { m_recoveryTimeout = ms; }

signals:
    void recoveryAttempted(UIErrorType errorType, bool success,
                           const QString& method);
    void recoveryFailed(UIErrorType errorType, const QString& reason);
    void userGuidanceShown(const QString& problem, const QString& solution);

private:
    UIRecoveryManager();
    ~UIRecoveryManager() override = default;
    UIRecoveryManager(const UIRecoveryManager&) = delete;
    UIRecoveryManager& operator=(const UIRecoveryManager&) = delete;

    // Recovery implementations
    bool tryAutomaticRetry(QWidget* context,
                           const ErrorHandling::ErrorInfo& error);
    bool tryFallbackMethod(QWidget* context,
                           const ErrorHandling::ErrorInfo& error);
    bool tryGracefulDegradation(QWidget* context,
                                const ErrorHandling::ErrorInfo& error);
    bool tryResetToDefault(QWidget* context,
                           const ErrorHandling::ErrorInfo& error);

    // Widget state management
    struct WidgetState {
        QString objectName;
        QVariantMap properties;
        QString styleSheet;
        bool enabled;
        bool visible;
        QRect geometry;
        QVariant data;
    };

    WidgetState captureWidgetState(QWidget* widget);
    void applyWidgetState(QWidget* widget, const WidgetState& state);

    // Internal helpers
    QStringList getRecoveryOptions(UIErrorType errorType);
    QString getErrorTypeString(UIErrorType errorType);
    void logRecoveryAttempt(UIErrorType errorType, const QString& method,
                            bool success);

    // Data members
    QHash<UIErrorType, QList<RecoveryAction>> m_recoveryActions;
    QHash<QString, std::function<QWidget*(QWidget*)>> m_fallbackWidgets;
    QHash<QWidget*, WidgetState> m_savedStates;

    bool m_autoRecoveryEnabled = true;
    int m_maxRetryAttempts = 3;
    int m_recoveryTimeout = 5000;

    SastLogging::CategoryLogger m_logger;
};

/**
 * @brief RAII helper for UI operation recovery
 */
class UIOperationGuard {
public:
    UIOperationGuard(QWidget* widget, const QString& operation);
    ~UIOperationGuard();

    void setErrorType(UIRecoveryManager::UIErrorType type) {
        m_errorType = type;
    }
    void commit();                    // Operation succeeded
    void fail(const QString& error);  // Operation failed

private:
    QWidget* m_widget;
    QString m_operation;
    UIRecoveryManager::UIErrorType m_errorType;
    bool m_committed;
    bool m_failed;
};

// Convenience macros
#define UI_RECOVERY_GUARD(widget, operation) \
    UIOperationGuard guard(widget, operation)

#define UI_RECOVERY_COMMIT() guard.commit()

#define UI_RECOVERY_FAIL(error) guard.fail(error)

#define REGISTER_UI_RECOVERY(errorType, strategy, description, action,      \
                             priority)                                      \
    UIRecoveryManager::instance().registerRecoveryAction(                   \
        errorType, UIRecoveryManager::RecoveryAction(strategy, description, \
                                                     action, priority))
