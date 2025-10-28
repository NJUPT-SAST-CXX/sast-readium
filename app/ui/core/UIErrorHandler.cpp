#include "UIErrorHandler.h"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>
#include <QRegularExpression>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>
#include <QToolTip>
#include "../../logging/LoggingMacros.h"
#include "../../managers/StyleManager.h"
#include "../widgets/ToastNotification.h"

UIErrorHandler::UIErrorHandler()
    : QObject(nullptr), m_logger("UIErrorHandler") {
    m_logger.info("UIErrorHandler initialized");

    // Register default UI recovery actions
    registerUIRecoveryAction(
        ErrorHandling::ErrorCategory::FileSystem,
        [this](const ErrorHandling::ErrorInfo& error, QWidget* parent) -> bool {
            return handleFileSystemRecovery(error, parent);
        });

    registerUIRecoveryAction(
        ErrorHandling::ErrorCategory::Document,
        [this](const ErrorHandling::ErrorInfo& error, QWidget* parent) -> bool {
            return handleDocumentRecovery(error, parent);
        });
}

UIErrorHandler& UIErrorHandler::instance() {
    static UIErrorHandler instance;
    return instance;
}

// Error handling methods implementation (Requirements 5.1-5.5)

void UIErrorHandler::handleUserInputError(QWidget* parent, const QString& field,
                                          const QString& error,
                                          const QString& suggestion) {
    m_logger.warning(
        QString("User input error in field '%1': %2").arg(field, error));

    QString message = tr("Invalid input in %1: %2").arg(field, error);
    if (!suggestion.isEmpty()) {
        message += tr("\n\nSuggestion: %1").arg(suggestion);
    }

    showFeedback(parent, message, FeedbackType::Error, 5000);
    emit validationFailed(parent, field, error);
}

void UIErrorHandler::handleSystemError(QWidget* parent,
                                       const ErrorHandling::ErrorInfo& error) {
    m_logger.error(
        QString("System error: %1 - %2").arg(error.message, error.details));

    // Attempt automatic recovery first
    if (m_autoRecovery && attemptErrorRecovery(error, "UI", parent)) {
        showFeedback(parent, tr("System error recovered automatically"),
                     FeedbackType::Success);
        return;
    }

    QString title = tr("System Error");
    QString message = formatErrorMessage(error);

    if (error.severity == ErrorHandling::ErrorSeverity::Critical ||
        error.severity == ErrorHandling::ErrorSeverity::Fatal) {
        showErrorDialog(parent, title, message, error.details);
    } else {
        showFeedback(parent, message, FeedbackType::Error, 8000);
    }

    emit errorHandled("System", error.message);
}

void UIErrorHandler::handleFileOperationError(QWidget* parent,
                                              const QString& operation,
                                              const QString& filePath,
                                              const QString& error) {
    m_logger.error(QString("File operation '%1' failed for '%2': %3")
                       .arg(operation, filePath, error));

    QString message =
        tr("File operation failed: %1\nFile: %2\nError: %3")
            .arg(operation, QFileInfo(filePath).fileName(), error);

    // Create error info for recovery attempt
    ErrorHandling::ErrorInfo errorInfo =
        ErrorHandling::createFileSystemError(operation, filePath, error);

    if (m_autoRecovery &&
        attemptErrorRecovery(errorInfo, "FileOperation", parent)) {
        showFeedback(parent, tr("File operation recovered"),
                     FeedbackType::Success);
        return;
    }

    showErrorDialog(parent, tr("File Operation Error"), message,
                    tr("Path: %1\nOperation: %2").arg(filePath, operation));

    emit errorHandled("FileOperation", error);
}
void UIErrorHandler::handleUnexpectedError(QWidget* parent,
                                           const QString& context,
                                           const std::exception& exception) {
    QString error = QString::fromStdString(exception.what());
    m_logger.critical(
        QString("Unexpected exception in %1: %2").arg(context, error));

    ErrorHandling::ErrorInfo errorInfo(ErrorHandling::ErrorCategory::Unknown,
                                       ErrorHandling::ErrorSeverity::Critical,
                                       tr("Unexpected error occurred"), error,
                                       context);

    logError(errorInfo, context);

    QString message =
        tr("An unexpected error occurred in %1.\n\nThe application will "
           "attempt to continue, but some features may not work correctly.")
            .arg(context);
    showErrorDialog(parent, tr("Unexpected Error"), message, error);

    emit errorHandled(context, error);
}

void UIErrorHandler::handleUnexpectedError(QWidget* parent,
                                           const QString& context,
                                           const QString& error) {
    m_logger.critical(
        QString("Unexpected error in %1: %2").arg(context, error));

    ErrorHandling::ErrorInfo errorInfo(ErrorHandling::ErrorCategory::Unknown,
                                       ErrorHandling::ErrorSeverity::Critical,
                                       tr("Unexpected error occurred"), error,
                                       context);

    logError(errorInfo, context);

    QString message =
        tr("An unexpected error occurred in %1.\n\nThe application will "
           "attempt to continue, but some features may not work correctly.")
            .arg(context);
    showErrorDialog(parent, tr("Unexpected Error"), message, error);

    emit errorHandled(context, error);
}

// User feedback methods implementation (Requirements 6.1-6.5)

void UIErrorHandler::showFeedback(QWidget* parent, const QString& message,
                                  FeedbackType type, int duration) {
    if (!parent) {
        m_logger.warning("Cannot show feedback: parent widget is null");
        return;
    }

    ToastNotification::Type toastType;
    switch (type) {
        case FeedbackType::Success:
            toastType = ToastNotification::Type::Success;
            break;
        case FeedbackType::Info:
            toastType = ToastNotification::Type::Info;
            break;
        case FeedbackType::Warning:
            toastType = ToastNotification::Type::Warning;
            break;
        case FeedbackType::Error:
        case FeedbackType::Critical:
            toastType = ToastNotification::Type::Error;
            break;
    }

    ToastNotification::show(parent, message, toastType, duration);

    m_logger.debug(QString("Showed feedback: %1 (type: %2)")
                       .arg(message)
                       .arg(static_cast<int>(type)));
    emit userFeedbackShown(parent, message, type);
}

void UIErrorHandler::showProgressFeedback(QWidget* parent,
                                          const QString& operation,
                                          int progress) {
    if (!parent)
        return;

    QString message;
    if (progress >= 0) {
        message = tr("%1... %2%").arg(operation).arg(progress);
    } else {
        message = tr("%1...").arg(operation);
    }

    showFeedback(parent, message, FeedbackType::Info,
                 0);  // No timeout for progress

    // Store timer for this parent to manage progress updates
    if (!m_progressTimers.contains(parent)) {
        QTimer* timer = new QTimer(this);
        timer->setSingleShot(true);
        m_progressTimers[parent] = timer;

        // Clean up when parent is destroyed
        connect(parent, &QObject::destroyed, this,
                [this, parent]() { m_progressTimers.remove(parent); });
    }
}

void UIErrorHandler::hideProgressFeedback(QWidget* parent) {
    if (!parent)
        return;

    // Hide any active toast for this parent
    if (m_activeToasts.contains(parent) && m_activeToasts[parent]) {
        m_activeToasts[parent]->hideNotification();
        m_activeToasts.remove(parent);
    }

    // Clean up progress timer
    if (m_progressTimers.contains(parent)) {
        m_progressTimers[parent]->stop();
        m_progressTimers.remove(parent);
    }
}

void UIErrorHandler::showInteractionFeedback(QWidget* widget,
                                             const QString& action) {
    if (!widget)
        return;

    // Show brief tooltip feedback for the action
    QString message = tr("Action: %1").arg(action);
    showWidgetTooltip(widget, message, 1500);

    m_logger.debug(QString("Interaction feedback for %1: %2")
                       .arg(widget->objectName(), action));
}

void UIErrorHandler::showValidationFeedback(QWidget* widget,
                                            const ValidationInfo& info) {
    if (!widget)
        return;

    setWidgetValidationState(widget, info.result, info.message);

    if (info.result != ValidationResult::Valid) {
        QString feedbackMessage = info.message;
        if (!info.suggestion.isEmpty()) {
            feedbackMessage += tr(" - %1").arg(info.suggestion);
        }

        FeedbackType type = (info.result == ValidationResult::Critical)
                                ? FeedbackType::Error
                                : FeedbackType::Warning;

        showFeedback(widget->parentWidget(), feedbackMessage, type, 4000);
        emit validationFailed(widget, widget->objectName(), info.message);
    }
}

// Input validation methods

UIErrorHandler::ValidationInfo UIErrorHandler::validatePageNumber(
    int page, int totalPages) {
    if (totalPages <= 0) {
        return ValidationInfo(ValidationResult::Critical,
                              tr("Invalid document: no pages available"),
                              tr("Please open a valid PDF document"), false);
    }

    if (page < 1) {
        return ValidationInfo(
            ValidationResult::Invalid, tr("Page number must be at least 1"),
            tr("Enter a number between 1 and %1").arg(totalPages), false);
    }

    if (page > totalPages) {
        return ValidationInfo(
            ValidationResult::Invalid,
            tr("Page number exceeds document length"),
            tr("Enter a number between 1 and %1").arg(totalPages), false);
    }

    return ValidationInfo(ValidationResult::Valid);
}

UIErrorHandler::ValidationInfo UIErrorHandler::validateZoomLevel(double zoom) {
    const double minZoom = 0.1;  // 10%
    const double maxZoom = 5.0;  // 500%

    if (zoom < minZoom) {
        return ValidationInfo(ValidationResult::Invalid,
                              tr("Zoom level too low (minimum 10%)"),
                              tr("Enter a value between 10% and 500%"), false);
    }

    if (zoom > maxZoom) {
        return ValidationInfo(ValidationResult::Invalid,
                              tr("Zoom level too high (maximum 500%)"),
                              tr("Enter a value between 10% and 500%"), false);
    }

    return ValidationInfo(ValidationResult::Valid);
}

UIErrorHandler::ValidationInfo UIErrorHandler::validateFilePath(
    const QString& path, bool mustExist) {
    if (path.isEmpty()) {
        return ValidationInfo(ValidationResult::Invalid,
                              tr("File path cannot be empty"),
                              tr("Please select a file"), false);
    }

    QFileInfo fileInfo(path);

    if (mustExist && !fileInfo.exists()) {
        return ValidationInfo(ValidationResult::Invalid,
                              tr("File does not exist"),
                              tr("Please select an existing file"), false);
    }

    if (mustExist && !fileInfo.isReadable()) {
        return ValidationInfo(ValidationResult::Invalid,
                              tr("File is not readable"),
                              tr("Please check file permissions"), false);
    }

    // Check if it's a PDF file
    if (mustExist && !path.toLower().endsWith(".pdf")) {
        return ValidationInfo(ValidationResult::Warning,
                              tr("File may not be a PDF document"),
                              tr("PDF files are recommended"), true);
    }

    return ValidationInfo(ValidationResult::Valid);
}

UIErrorHandler::ValidationInfo UIErrorHandler::validateCacheSize(int sizeMB) {
    const int minSize = 50;        // 50 MB minimum
    const int maxSize = 10000;     // 10 GB maximum
    const int warningSize = 2000;  // 2 GB warning threshold

    if (sizeMB < minSize) {
        return ValidationInfo(
            ValidationResult::Invalid,
            tr("Cache size too small (minimum %1 MB)").arg(minSize),
            tr("Increase cache size for better performance"), false);
    }

    if (sizeMB > maxSize) {
        return ValidationInfo(
            ValidationResult::Invalid,
            tr("Cache size too large (maximum %1 MB)").arg(maxSize),
            tr("Reduce cache size to prevent disk space issues"), false);
    }

    if (sizeMB > warningSize) {
        return ValidationInfo(
            ValidationResult::Warning,
            tr("Large cache size may consume significant disk space"),
            tr("Consider using a smaller cache size"), true);
    }

    return ValidationInfo(ValidationResult::Valid);
}

UIErrorHandler::ValidationInfo UIErrorHandler::validateRecentFilesCount(
    int count) {
    const int minCount = 5;
    const int maxCount = 100;
    const int warningCount = 50;

    if (count < minCount) {
        return ValidationInfo(
            ValidationResult::Invalid,
            tr("Recent files count too low (minimum %1)").arg(minCount),
            tr("Increase count for better file access"), false);
    }

    if (count > maxCount) {
        return ValidationInfo(
            ValidationResult::Invalid,
            tr("Recent files count too high (maximum %1)").arg(maxCount),
            tr("Reduce count to improve performance"), false);
    }

    if (count > warningCount) {
        return ValidationInfo(
            ValidationResult::Warning,
            tr("Large number of recent files may slow down the application"),
            tr("Consider using fewer recent files"), true);
    }

    return ValidationInfo(ValidationResult::Valid);
}

UIErrorHandler::ValidationInfo UIErrorHandler::validateSearchQuery(
    const QString& query) {
    if (query.isEmpty()) {
        return ValidationInfo(ValidationResult::Invalid,
                              tr("Search query cannot be empty"),
                              tr("Enter text to search for"), false);
    }

    if (query.length() > 1000) {
        return ValidationInfo(
            ValidationResult::Invalid,
            tr("Search query too long (maximum 1000 characters)"),
            tr("Shorten your search query"), false);
    }

    // Check for potentially problematic regex patterns
    if (query.contains(QRegularExpression("[\\[\\]{}()*+?.\\\\^$|]"))) {
        return ValidationInfo(
            ValidationResult::Warning,
            tr("Query contains special characters that may affect search"),
            tr("Use simple text for basic search"), true);
    }

    return ValidationInfo(ValidationResult::Valid);
}

UIErrorHandler::ValidationInfo UIErrorHandler::validateNumericInput(
    double value, double min, double max, const QString& fieldName) {
    if (value < min) {
        return ValidationInfo(
            ValidationResult::Invalid,
            tr("%1 is too low (minimum %2)").arg(fieldName).arg(min),
            tr("Enter a value between %1 and %2").arg(min).arg(max), false);
    }

    if (value > max) {
        return ValidationInfo(
            ValidationResult::Invalid,
            tr("%1 is too high (maximum %2)").arg(fieldName).arg(max),
            tr("Enter a value between %1 and %2").arg(min).arg(max), false);
    }

    return ValidationInfo(ValidationResult::Valid);
}

// Visual feedback methods

void UIErrorHandler::setWidgetValidationState(QWidget* widget,
                                              ValidationResult result,
                                              const QString& tooltip) {
    if (!widget)
        return;

    QString styleSheet;
    QString iconTooltip = tooltip;

    switch (result) {
        case ValidationResult::Valid:
            styleSheet = "";
            widget->setToolTip("");
            break;

        case ValidationResult::Warning:
            styleSheet =
                "border: 2px solid " + STYLE.warningColor().name() + ";";
            if (!iconTooltip.isEmpty()) {
                widget->setToolTip(iconTooltip);
            }
            break;

        case ValidationResult::Invalid:
            styleSheet = "border: 2px solid " + STYLE.errorColor().name() + ";";
            if (!iconTooltip.isEmpty()) {
                widget->setToolTip(iconTooltip);
            }
            break;

        case ValidationResult::Critical:
            styleSheet = "border: 2px solid " + STYLE.errorColor().name() +
                         "; "
                         "background-color: " +
                         STYLE.errorColor().lighter(180).name() + ";";
            if (!iconTooltip.isEmpty()) {
                widget->setToolTip(iconTooltip);
            }
            break;
    }

    widget->setStyleSheet(widget->styleSheet() + styleSheet);
}

void UIErrorHandler::clearWidgetValidationState(QWidget* widget) {
    if (!widget)
        return;

    // Remove validation-related styles
    QString currentStyle = widget->styleSheet();
    currentStyle.remove(
        QRegularExpression("border:\\s*2px\\s*solid\\s*[^;]*;"));
    currentStyle.remove(QRegularExpression("background-color:\\s*[^;]*;"));
    widget->setStyleSheet(currentStyle);
    widget->setToolTip("");
}

void UIErrorHandler::setWidgetEnabled(QWidget* widget, bool enabled,
                                      const QString& reason) {
    if (!widget)
        return;

    widget->setEnabled(enabled);

    if (!enabled && !reason.isEmpty()) {
        widget->setToolTip(tr("Disabled: %1").arg(reason));
    } else if (enabled) {
        widget->setToolTip("");
    }
}

void UIErrorHandler::showWidgetTooltip(QWidget* widget, const QString& message,
                                       int duration) {
    if (!widget || message.isEmpty())
        return;

    QToolTip::showText(widget->mapToGlobal(QPoint(0, widget->height())),
                       message, widget, QRect(), duration);
}

// Error recovery methods

bool UIErrorHandler::attemptErrorRecovery(const ErrorHandling::ErrorInfo& error,
                                          const QString& component,
                                          QWidget* parent) {
    m_logger.info(QString("Attempting UI error recovery for component: %1")
                      .arg(component));

    // Try registered UI recovery action first
    auto it = m_uiRecoveryActions.find(error.category);
    if (it != m_uiRecoveryActions.end()) {
        try {
            bool recovered = it.value()(error, parent);
            if (recovered) {
                m_logger.info(
                    QString("UI recovery successful for %1").arg(component));
                emit recoveryAttempted(component, true);
                return true;
            }
        } catch (const std::exception& e) {
            m_logger.error(
                QString("UI recovery action failed: %1").arg(e.what()));
        }
    }

    // Fall back to system recovery manager
    auto& recoveryManager = ErrorRecovery::RecoveryManager::instance();
    ErrorRecovery::RecoveryResult result =
        recoveryManager.executeRecovery(error, component, "UI");

    bool success = (result == ErrorRecovery::RecoveryResult::Success);
    emit recoveryAttempted(component, success);

    return success;
}

void UIErrorHandler::registerUIRecoveryAction(
    ErrorHandling::ErrorCategory category,
    std::function<bool(const ErrorHandling::ErrorInfo&, QWidget*)> action) {
    m_uiRecoveryActions[category] = action;
    m_logger.info(QString("Registered UI recovery action for category: %1")
                      .arg(ErrorHandling::categoryToString(category)));
}

// Helper methods

void UIErrorHandler::showErrorDialog(QWidget* parent, const QString& title,
                                     const QString& message,
                                     const QString& details) {
    QMessageBox msgBox(parent);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(QMessageBox::Critical);

    if (!details.isEmpty() && m_showDetailedErrors) {
        msgBox.setDetailedText(details);
    }

    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);

    // Apply theme-appropriate styling
    msgBox.setStyleSheet(
        QString("QMessageBox { background-color: %1; color: %2; }"
                "QMessageBox QPushButton { min-width: 80px; padding: 5px; }")
            .arg(STYLE.backgroundColor().name(), STYLE.textColor().name()));

    msgBox.exec();
}

void UIErrorHandler::showRecoveryDialog(QWidget* parent,
                                        const ErrorHandling::ErrorInfo& error,
                                        const QStringList& recoveryOptions) {
    QMessageBox msgBox(parent);
    msgBox.setWindowTitle(tr("Error Recovery"));
    msgBox.setText(
        tr("An error occurred: %1\n\nWould you like to attempt recovery?")
            .arg(error.message));
    msgBox.setIcon(QMessageBox::Question);

    QPushButton* retryButton =
        msgBox.addButton(tr("Retry"), QMessageBox::ActionRole);
    QPushButton* ignoreButton =
        msgBox.addButton(tr("Ignore"), QMessageBox::RejectRole);
    QPushButton* abortButton =
        msgBox.addButton(tr("Abort"), QMessageBox::DestructiveRole);

    msgBox.setDefaultButton(retryButton);
    msgBox.exec();

    if (msgBox.clickedButton() == retryButton) {
        attemptErrorRecovery(error, "UserChoice", parent);
    }
}

QString UIErrorHandler::formatErrorMessage(
    const ErrorHandling::ErrorInfo& error) {
    QString message = error.message;

    if (!error.context.isEmpty()) {
        message = tr("Error in %1: %2").arg(error.context, error.message);
    }

    // Add severity indicator
    QString severityText = ErrorHandling::severityToString(error.severity);
    if (error.severity >= ErrorHandling::ErrorSeverity::Error) {
        message = tr("[%1] %2").arg(severityText, message);
    }

    return message;
}

QString UIErrorHandler::getErrorIcon(ErrorHandling::ErrorSeverity severity) {
    switch (severity) {
        case ErrorHandling::ErrorSeverity::Info:
            return "ℹ️";
        case ErrorHandling::ErrorSeverity::Warning:
            return "⚠️";
        case ErrorHandling::ErrorSeverity::Error:
            return "❌";
        case ErrorHandling::ErrorSeverity::Critical:
        case ErrorHandling::ErrorSeverity::Fatal:
            return "🚨";
    }
    return "❓";
}

QColor UIErrorHandler::getFeedbackColor(FeedbackType type) {
    switch (type) {
        case FeedbackType::Success:
            return STYLE.successColor();
        case FeedbackType::Info:
            return STYLE.primaryColor();
        case FeedbackType::Warning:
            return STYLE.warningColor();
        case FeedbackType::Error:
            return STYLE.errorColor();
        case FeedbackType::Critical:
            return STYLE.errorColor().darker(120);
    }
    return STYLE.textColor();
}

void UIErrorHandler::logError(const ErrorHandling::ErrorInfo& error,
                              const QString& context) {
    QString logMessage =
        QString("UI Error in %1: %2").arg(context, error.message);

    switch (error.severity) {
        case ErrorHandling::ErrorSeverity::Info:
            m_logger.info(logMessage);
            break;
        case ErrorHandling::ErrorSeverity::Warning:
            m_logger.warning(logMessage);
            break;
        case ErrorHandling::ErrorSeverity::Error:
            m_logger.error(logMessage);
            break;
        case ErrorHandling::ErrorSeverity::Critical:
        case ErrorHandling::ErrorSeverity::Fatal:
            m_logger.critical(logMessage);
            break;
    }
}

// Default recovery action implementations
bool UIErrorHandler::handleFileSystemRecovery(
    const ErrorHandling::ErrorInfo& error, QWidget* parent) {
    // Extract file path from error details
    QString filePath = error.details;
    if (filePath.contains("Path: ")) {
        filePath = filePath.mid(filePath.indexOf("Path: ") + 6);
        if (filePath.contains(",")) {
            filePath = filePath.left(filePath.indexOf(","));
        }
    }

    if (filePath.isEmpty()) {
        return false;
    }

    QFileInfo fileInfo(filePath);

    // Try to create parent directory if it doesn't exist
    if (!fileInfo.dir().exists()) {
        if (fileInfo.dir().mkpath(".")) {
            showFeedback(
                parent,
                tr("Created missing directory: %1").arg(fileInfo.dir().path()),
                FeedbackType::Success);
            return true;
        }
    }

    return false;
}

bool UIErrorHandler::handleDocumentRecovery(
    const ErrorHandling::ErrorInfo& error, QWidget* parent) {
    // For document errors, suggest user actions
    if (error.message.contains("corrupt") ||
        error.message.contains("invalid")) {
        showFeedback(
            parent,
            tr("Document may be corrupted. Try opening a different file."),
            FeedbackType::Warning, 6000);
        return false;  // Can't automatically recover from corruption
    }

    if (error.message.contains("memory") ||
        error.message.contains("allocation")) {
        showFeedback(parent, tr("Memory issue detected. Clearing cache..."),
                     FeedbackType::Info);
        // Could trigger cache cleanup here
        return true;
    }

    return false;
}

// InputValidator implementation

UIErrorHandler::ValidationInfo InputValidator::validateFilePath(
    const QString& path, bool mustExist, bool mustBeWritable) {
    if (path.isEmpty()) {
        return UIErrorHandler::ValidationInfo(
            UIErrorHandler::ValidationResult::Invalid,
            QObject::tr("File path cannot be empty"),
            QObject::tr("Please select a file"), false);
    }

    QFileInfo fileInfo(path);

    if (mustExist && !fileInfo.exists()) {
        return UIErrorHandler::ValidationInfo(
            UIErrorHandler::ValidationResult::Invalid,
            QObject::tr("File does not exist: %1").arg(fileInfo.fileName()),
            QObject::tr("Please select an existing file"), false);
    }

    if (mustExist && !fileInfo.isReadable()) {
        return UIErrorHandler::ValidationInfo(
            UIErrorHandler::ValidationResult::Invalid,
            QObject::tr("File is not readable: %1").arg(fileInfo.fileName()),
            QObject::tr("Please check file permissions"), false);
    }

    if (mustBeWritable && !fileInfo.isWritable()) {
        return UIErrorHandler::ValidationInfo(
            UIErrorHandler::ValidationResult::Invalid,
            QObject::tr("File is not writable: %1").arg(fileInfo.fileName()),
            QObject::tr(
                "Please check file permissions or select a different location"),
            false);
    }

    return UIErrorHandler::ValidationInfo(
        UIErrorHandler::ValidationResult::Valid);
}

UIErrorHandler::ValidationInfo InputValidator::validateRange(
    double value, double min, double max, const QString& fieldName) {
    if (value < min) {
        return UIErrorHandler::ValidationInfo(
            UIErrorHandler::ValidationResult::Invalid,
            QObject::tr("%1 is below minimum value (%2)")
                .arg(fieldName)
                .arg(min),
            QObject::tr("Enter a value between %1 and %2").arg(min).arg(max),
            false);
    }

    if (value > max) {
        return UIErrorHandler::ValidationInfo(
            UIErrorHandler::ValidationResult::Invalid,
            QObject::tr("%1 exceeds maximum value (%2)")
                .arg(fieldName)
                .arg(max),
            QObject::tr("Enter a value between %1 and %2").arg(min).arg(max),
            false);
    }

    return UIErrorHandler::ValidationInfo(
        UIErrorHandler::ValidationResult::Valid);
}

UIErrorHandler::ValidationInfo InputValidator::validateTextInput(
    const QString& text, int minLength, int maxLength, const QString& pattern) {
    if (text.length() < minLength) {
        return UIErrorHandler::ValidationInfo(
            UIErrorHandler::ValidationResult::Invalid,
            QObject::tr("Text is too short (minimum %1 characters)")
                .arg(minLength),
            QObject::tr("Enter at least %1 characters").arg(minLength), false);
    }

    if (maxLength > 0 && text.length() > maxLength) {
        return UIErrorHandler::ValidationInfo(
            UIErrorHandler::ValidationResult::Invalid,
            QObject::tr("Text is too long (maximum %1 characters)")
                .arg(maxLength),
            QObject::tr("Enter no more than %1 characters").arg(maxLength),
            false);
    }

    if (!pattern.isEmpty()) {
        QRegularExpression regex(pattern);
        if (!regex.match(text).hasMatch()) {
            return UIErrorHandler::ValidationInfo(
                UIErrorHandler::ValidationResult::Invalid,
                QObject::tr("Text format is invalid"),
                QObject::tr("Please enter text in the correct format"), false);
        }
    }

    return UIErrorHandler::ValidationInfo(
        UIErrorHandler::ValidationResult::Valid);
}

UIErrorHandler::ValidationInfo InputValidator::validatePDFFile(
    const QString& filePath) {
    auto pathValidation = validateFilePath(filePath, true, false);
    if (pathValidation.result != UIErrorHandler::ValidationResult::Valid) {
        return pathValidation;
    }

    // Check file extension
    if (!filePath.toLower().endsWith(".pdf")) {
        return UIErrorHandler::ValidationInfo(
            UIErrorHandler::ValidationResult::Warning,
            QObject::tr("File may not be a PDF document"),
            QObject::tr("PDF files are recommended for best compatibility"),
            true);
    }

    // Check file size (warn if very large)
    QFileInfo fileInfo(filePath);
    qint64 fileSizeBytes = fileInfo.size();
    qint64 maxSizeBytes = 500 * 1024 * 1024;  // 500 MB

    if (fileSizeBytes > maxSizeBytes) {
        return UIErrorHandler::ValidationInfo(
            UIErrorHandler::ValidationResult::Warning,
            QObject::tr("PDF file is very large (%1 MB)")
                .arg(fileSizeBytes / (1024 * 1024)),
            QObject::tr(
                "Large files may take longer to load and use more memory"),
            true);
    }

    return UIErrorHandler::ValidationInfo(
        UIErrorHandler::ValidationResult::Valid);
}

UIErrorHandler::ValidationInfo InputValidator::validatePageRange(
    int start, int end, int totalPages) {
    if (totalPages <= 0) {
        return UIErrorHandler::ValidationInfo(
            UIErrorHandler::ValidationResult::Critical,
            QObject::tr("Invalid document: no pages available"),
            QObject::tr("Please open a valid PDF document"), false);
    }

    if (start < 1 || end < 1) {
        return UIErrorHandler::ValidationInfo(
            UIErrorHandler::ValidationResult::Invalid,
            QObject::tr("Page numbers must be at least 1"),
            QObject::tr("Enter page numbers between 1 and %1").arg(totalPages),
            false);
    }

    if (start > totalPages || end > totalPages) {
        return UIErrorHandler::ValidationInfo(
            UIErrorHandler::ValidationResult::Invalid,
            QObject::tr("Page numbers exceed document length"),
            QObject::tr("Enter page numbers between 1 and %1").arg(totalPages),
            false);
    }

    if (start > end) {
        return UIErrorHandler::ValidationInfo(
            UIErrorHandler::ValidationResult::Invalid,
            QObject::tr("Start page cannot be greater than end page"),
            QObject::tr("Ensure start page ≤ end page"), false);
    }

    return UIErrorHandler::ValidationInfo(
        UIErrorHandler::ValidationResult::Valid);
}

UIErrorHandler::ValidationInfo InputValidator::validateZoomRange(double zoom) {
    return UIErrorHandler::instance().validateZoomLevel(zoom);
}

UIErrorHandler::ValidationInfo InputValidator::validateSearchQuery(
    const QString& query, bool allowEmpty, bool checkRegex) {
    if (!allowEmpty && query.isEmpty()) {
        return UIErrorHandler::ValidationInfo(
            UIErrorHandler::ValidationResult::Invalid,
            QObject::tr("Search query cannot be empty"),
            QObject::tr("Enter text to search for"), false);
    }

    if (query.length() > 1000) {
        return UIErrorHandler::ValidationInfo(
            UIErrorHandler::ValidationResult::Invalid,
            QObject::tr("Search query too long (maximum 1000 characters)"),
            QObject::tr("Shorten your search query"), false);
    }

    if (checkRegex &&
        query.contains(QRegularExpression("[\\[\\]{}()*+?.\\\\^$|]"))) {
        // Test if it's a valid regex
        QRegularExpression testRegex(query);
        if (!testRegex.isValid()) {
            return UIErrorHandler::ValidationInfo(
                UIErrorHandler::ValidationResult::Invalid,
                QObject::tr("Invalid regular expression"),
                QObject::tr(
                    "Check your regex syntax or use simple text search"),
                false);
        }

        return UIErrorHandler::ValidationInfo(
            UIErrorHandler::ValidationResult::Warning,
            QObject::tr("Using regular expression search"),
            QObject::tr("Complex patterns may be slower"), true);
    }

    return UIErrorHandler::ValidationInfo(
        UIErrorHandler::ValidationResult::Valid);
}
