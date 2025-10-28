#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <functional>
#include "../../logging/SimpleLogging.h"
#include "../../utils/ErrorHandling.h"
#include "../../utils/ErrorRecovery.h"

class QMessageBox;
class ToastNotification;

/**
 * @brief Comprehensive UI error handling and user feedback manager
 *
 * This class provides centralized error handling for all UI components,
 * implementing the requirements for comprehensive error handling and
 * user feedback systems (Requirements 5.1-5.5, 6.1-6.5).
 *
 * Features:
 * - User input validation with clear error messages
 * - System error handling with appropriate user messages
 * - File operation error handling with specific error information
 * - Unexpected error handling with proper logging and user notification
 * - Visual feedback systems for all user interactions
 */
class UIErrorHandler : public QObject {
    Q_OBJECT

public:
    enum class FeedbackType {
        Success,  // Green - successful operations
        Info,     // Blue - informational messages
        Warning,  // Orange - warnings that don't prevent operation
        Error,    // Red - errors that prevent operation
        Critical  // Dark red - critical errors requiring immediate attention
    };

    enum class ValidationResult {
        Valid,    // Input is valid
        Warning,  // Input is valid but has warnings
        Invalid,  // Input is invalid but recoverable
        Critical  // Input is invalid and may cause system issues
    };

    struct ValidationInfo {
        ValidationResult result;
        QString message;
        QString suggestion;
        bool canProceed;

        ValidationInfo(ValidationResult r = ValidationResult::Valid,
                       const QString& msg = QString(),
                       const QString& sug = QString(), bool proceed = true)
            : result(r), message(msg), suggestion(sug), canProceed(proceed) {}
    };

    static UIErrorHandler& instance();

    // Error handling methods (Requirement 5.1-5.5)
    void handleUserInputError(QWidget* parent, const QString& field,
                              const QString& error,
                              const QString& suggestion = QString());
    void handleSystemError(QWidget* parent,
                           const ErrorHandling::ErrorInfo& error);
    void handleFileOperationError(QWidget* parent, const QString& operation,
                                  const QString& filePath,
                                  const QString& error);
    void handleUnexpectedError(QWidget* parent, const QString& context,
                               const std::exception& exception);
    void handleUnexpectedError(QWidget* parent, const QString& context,
                               const QString& error);

    // User feedback methods (Requirement 6.1-6.5)
    void showFeedback(QWidget* parent, const QString& message,
                      FeedbackType type, int duration = 3000);
    void showProgressFeedback(QWidget* parent, const QString& operation,
                              int progress = -1);
    void hideProgressFeedback(QWidget* parent);
    void showInteractionFeedback(QWidget* widget, const QString& action);
    void showValidationFeedback(QWidget* widget, const ValidationInfo& info);

    // Input validation methods
    ValidationInfo validatePageNumber(int page, int totalPages);
    ValidationInfo validateZoomLevel(double zoom);
    ValidationInfo validateFilePath(const QString& path, bool mustExist = true);
    ValidationInfo validateCacheSize(int sizeMB);
    ValidationInfo validateRecentFilesCount(int count);
    ValidationInfo validateSearchQuery(const QString& query);
    ValidationInfo validateNumericInput(double value, double min, double max,
                                        const QString& fieldName);

    // Visual feedback for widgets
    void setWidgetValidationState(QWidget* widget, ValidationResult result,
                                  const QString& tooltip = QString());
    void clearWidgetValidationState(QWidget* widget);
    void setWidgetEnabled(QWidget* widget, bool enabled,
                          const QString& reason = QString());
    void showWidgetTooltip(QWidget* widget, const QString& message,
                           int duration = 2000);

    // Error recovery integration
    bool attemptErrorRecovery(const ErrorHandling::ErrorInfo& error,
                              const QString& component,
                              QWidget* parent = nullptr);
    void registerUIRecoveryAction(
        ErrorHandling::ErrorCategory category,
        std::function<bool(const ErrorHandling::ErrorInfo&, QWidget*)> action);

    // Configuration
    void setShowDetailedErrors(bool show) { m_showDetailedErrors = show; }
    void setAutoRecovery(bool enabled) { m_autoRecovery = enabled; }
    void setFeedbackDuration(int ms) { m_defaultFeedbackDuration = ms; }

signals:
    void errorHandled(const QString& context, const QString& error);
    void validationFailed(QWidget* widget, const QString& field,
                          const QString& error);
    void recoveryAttempted(const QString& component, bool success);
    void userFeedbackShown(QWidget* parent, const QString& message,
                           FeedbackType type);

private:
    UIErrorHandler();
    ~UIErrorHandler() override = default;
    UIErrorHandler(const UIErrorHandler&) = delete;
    UIErrorHandler& operator=(const UIErrorHandler&) = delete;

    // Helper methods
    void showErrorDialog(QWidget* parent, const QString& title,
                         const QString& message,
                         const QString& details = QString());
    void showRecoveryDialog(QWidget* parent,
                            const ErrorHandling::ErrorInfo& error,
                            const QStringList& recoveryOptions);
    QString formatErrorMessage(const ErrorHandling::ErrorInfo& error);
    QString getErrorIcon(ErrorHandling::ErrorSeverity severity);
    QColor getFeedbackColor(FeedbackType type);
    void logError(const ErrorHandling::ErrorInfo& error,
                  const QString& context);

    // Default recovery implementations
    bool handleFileSystemRecovery(const ErrorHandling::ErrorInfo& error,
                                  QWidget* parent);
    bool handleDocumentRecovery(const ErrorHandling::ErrorInfo& error,
                                QWidget* parent);

    // State management
    QHash<QWidget*, QTimer*> m_progressTimers;
    QHash<QWidget*, QPointer<ToastNotification>> m_activeToasts;
    QHash<ErrorHandling::ErrorCategory,
          std::function<bool(const ErrorHandling::ErrorInfo&, QWidget*)>>
        m_uiRecoveryActions;

    // Configuration
    bool m_showDetailedErrors = true;
    bool m_autoRecovery = true;
    int m_defaultFeedbackDuration = 3000;

    // Logging
    SastLogging::CategoryLogger m_logger;
};

/**
 * @brief Input validator for common UI validation patterns
 */
class InputValidator {
public:
    // File path validation
    static UIErrorHandler::ValidationInfo validateFilePath(
        const QString& path, bool mustExist = true,
        bool mustBeWritable = false);

    // Numeric range validation
    static UIErrorHandler::ValidationInfo validateRange(
        double value, double min, double max, const QString& fieldName);

    // Text input validation
    static UIErrorHandler::ValidationInfo validateTextInput(
        const QString& text, int minLength = 0, int maxLength = -1,
        const QString& pattern = QString());

    // PDF-specific validation
    static UIErrorHandler::ValidationInfo validatePDFFile(
        const QString& filePath);
    static UIErrorHandler::ValidationInfo validatePageRange(int start, int end,
                                                            int totalPages);
    static UIErrorHandler::ValidationInfo validateZoomRange(double zoom);

    // Search validation
    static UIErrorHandler::ValidationInfo validateSearchQuery(
        const QString& query, bool allowEmpty = false, bool checkRegex = false);
};

// Convenience macros for error handling
#define UI_HANDLE_INPUT_ERROR(parent, field, error) \
    UIErrorHandler::instance().handleUserInputError(parent, field, error)

#define UI_HANDLE_SYSTEM_ERROR(parent, error) \
    UIErrorHandler::instance().handleSystemError(parent, error)

#define UI_HANDLE_FILE_ERROR(parent, operation, path, error)               \
    UIErrorHandler::instance().handleFileOperationError(parent, operation, \
                                                        path, error)

#define UI_SHOW_FEEDBACK(parent, message, type) \
    UIErrorHandler::instance().showFeedback(    \
        parent, message, UIErrorHandler::FeedbackType::type)

#define UI_VALIDATE_AND_SHOW(widget, validation)                           \
    do {                                                                   \
        auto result = validation;                                          \
        UIErrorHandler::instance().showValidationFeedback(widget, result); \
        if (result.result != UIErrorHandler::ValidationResult::Valid) {    \
            return false;                                                  \
        }                                                                  \
    } while (0)
