#include "ValidationUtils.h"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSpinBox>
#include <QTextEdit>
#include <QTimer>
#include <QToolTip>
#include "../../logging/LoggingMacros.h"
#include "../../managers/StyleManager.h"

// ValidationUtils implementation

bool ValidationUtils::validateAndShowFeedback(
    QWidget* widget, const UIErrorHandler::ValidationInfo& validation,
    bool showSuccess) {
    if (!widget) {
        LOG_WARNING(
            "ValidationUtils::validateAndShowFeedback - widget is null");
        return false;
    }

    // Apply visual feedback
    UIErrorHandler::instance().showValidationFeedback(widget, validation);

    // Show success feedback if requested and validation passed
    if (showSuccess &&
        validation.result == UIErrorHandler::ValidationResult::Valid) {
        UIErrorHandler::instance().showFeedback(
            widget->parentWidget(), QObject::tr("Input valid"),
            UIErrorHandler::FeedbackType::Success, 1500);
    }

    return validation.result == UIErrorHandler::ValidationResult::Valid ||
           validation.canProceed;
}

bool ValidationUtils::validatePageInput(QWidget* widget, int page,
                                        int totalPages) {
    auto validation =
        UIErrorHandler::instance().validatePageNumber(page, totalPages);
    return validateAndShowFeedback(widget, validation);
}

bool ValidationUtils::validateZoomInput(QWidget* widget, double zoom) {
    auto validation = UIErrorHandler::instance().validateZoomLevel(zoom);
    return validateAndShowFeedback(widget, validation);
}

bool ValidationUtils::validateFileInput(QWidget* widget,
                                        const QString& filePath,
                                        bool mustExist) {
    auto validation = InputValidator::validateFilePath(filePath, mustExist);
    return validateAndShowFeedback(widget, validation);
}

bool ValidationUtils::validateSearchInput(QWidget* widget, const QString& query,
                                          bool allowEmpty) {
    auto validation = InputValidator::validateSearchQuery(query, allowEmpty);
    return validateAndShowFeedback(widget, validation);
}

bool ValidationUtils::validateNumericRange(QWidget* widget, double value,
                                           double min, double max,
                                           const QString& fieldName) {
    auto validation = UIErrorHandler::instance().validateNumericInput(
        value, min, max, fieldName);
    return validateAndShowFeedback(widget, validation);
}

bool ValidationUtils::validateForm(const QList<ValidationRule>& rules,
                                   QWidget* parent) {
    bool allValid = true;
    QStringList errors;

    ValidationStateGuard guard(parent);

    for (const auto& rule : rules) {
        if (!rule.widget) {
            LOG_WARNING(
                "ValidationUtils::validateForm - null widget in rule for "
                "field: {}",
                rule.fieldName.toStdString());
            continue;
        }

        guard.addWidget(rule.widget);

        auto validation = rule.validator();
        bool isValid = validateAndShowFeedback(rule.widget, validation);

        if (!isValid) {
            allValid = false;
            errors.append(
                QObject::tr("%1: %2").arg(rule.fieldName, validation.message));

            if (rule.required) {
                // Focus first invalid required field
                if (allValid) {  // This was the first error
                    rule.widget->setFocus();
                }
            }
        }
    }

    if (allValid) {
        guard.commit();
        if (parent) {
            UIErrorHandler::instance().showFeedback(
                parent, QObject::tr("Form validation successful"),
                UIErrorHandler::FeedbackType::Success, 2000);
        }
    } else {
        // Show summary of validation errors
        if (parent && !errors.isEmpty()) {
            QString errorSummary =
                QObject::tr("Please correct the following errors:\n• %1")
                    .arg(errors.join("\n• "));
            UIErrorHandler::instance().showFeedback(
                parent, errorSummary, UIErrorHandler::FeedbackType::Error,
                8000);
        }
    }

    return allValid;
}

// Input sanitization methods

QString ValidationUtils::sanitizeTextInput(const QString& input,
                                           int maxLength) {
    QString sanitized = input.trimmed();

    // Remove control characters except newlines and tabs
    sanitized.remove(
        QRegularExpression("[\\x00-\\x08\\x0B\\x0C\\x0E-\\x1F\\x7F]"));

    // Limit length if specified
    if (maxLength > 0 && sanitized.length() > maxLength) {
        sanitized = sanitized.left(maxLength);
    }

    return sanitized;
}

QString ValidationUtils::sanitizeFilePath(const QString& path) {
    QString sanitized = path.trimmed();

    // Remove invalid file path characters on Windows
    sanitized.remove(QRegularExpression("[<>:\"|?*]"));

    // Normalize path separators
    sanitized.replace('\\', '/');

    // Remove double slashes
    sanitized.replace(QRegularExpression("/+"), "/");

    return sanitized;
}

double ValidationUtils::clampNumericInput(double value, double min,
                                          double max) {
    return qBound(min, value, max);
}

// Visual feedback helpers

void ValidationUtils::highlightValidationError(QWidget* widget,
                                               const QString& message) {
    if (!widget)
        return;

    UIErrorHandler::ValidationInfo validation(
        UIErrorHandler::ValidationResult::Invalid, message, QString(), false);
    UIErrorHandler::instance().showValidationFeedback(widget, validation);
}

void ValidationUtils::clearValidationHighlight(QWidget* widget) {
    if (!widget)
        return;

    UIErrorHandler::instance().clearWidgetValidationState(widget);
}

void ValidationUtils::showValidationTooltip(QWidget* widget,
                                            const QString& message,
                                            int duration) {
    if (!widget || message.isEmpty())
        return;

    UIErrorHandler::instance().showWidgetTooltip(widget, message, duration);
}

// Validation state management

void ValidationUtils::setValidationState(QWidget* widget, bool isValid,
                                         const QString& message) {
    if (!widget)
        return;

    UIErrorHandler::ValidationResult result =
        isValid ? UIErrorHandler::ValidationResult::Valid
                : UIErrorHandler::ValidationResult::Invalid;

    UIErrorHandler::ValidationInfo validation(result, message, QString(),
                                              isValid);
    UIErrorHandler::instance().showValidationFeedback(widget, validation);

    // Store state in widget property
    widget->setProperty("validationState", isValid);
    widget->setProperty("validationMessage", message);
}

bool ValidationUtils::getValidationState(QWidget* widget) {
    if (!widget) {
        return false;
    }

    return widget->property("validationState").toBool();
}

void ValidationUtils::clearAllValidationStates(QWidget* parent) {
    if (!parent) {
        return;
    }

    // Find all child widgets and clear their validation states
    auto children = parent->findChildren<QWidget*>();
    for (auto* child : children) {
        clearValidationHighlight(child);
        child->setProperty("validationState", QVariant());
        child->setProperty("validationMessage", QVariant());
    }
}

// Internal helpers

void ValidationUtils::applyValidationStyling(
    QWidget* widget, UIErrorHandler::ValidationResult result) {
    if (!widget) {
        return;
    }

    QString styleClass;
    switch (result) {
        case UIErrorHandler::ValidationResult::Valid:
            styleClass = "valid-input";
            break;
        case UIErrorHandler::ValidationResult::Invalid:
            styleClass = "invalid-input";
            break;
        case UIErrorHandler::ValidationResult::Warning:
            styleClass = "warning-input";
            break;
        case UIErrorHandler::ValidationResult::Critical:
            styleClass = "critical-input";
            break;
    }

    // Apply style class (requires CSS styling to be defined)
    widget->setProperty("validationClass", styleClass);
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
}

QString ValidationUtils::getValidationMessage(
    const UIErrorHandler::ValidationInfo& validation) {
    QString message = validation.message;
    if (!validation.suggestion.isEmpty()) {
        message += QObject::tr(" (%1)").arg(validation.suggestion);
    }
    return message;
}

// ValidationStateGuard implementation

ValidationStateGuard::ValidationStateGuard(QWidget* parent)
    : m_parent(parent), m_committed(false) {}

ValidationStateGuard::~ValidationStateGuard() {
    if (!m_committed) {
        rollback();
    }
}

void ValidationStateGuard::addWidget(QWidget* widget) {
    if (!widget) {
        return;
    }

    m_widgets.append(widget);
    m_originalStates[widget] = ValidationUtils::getValidationState(widget);
}

void ValidationStateGuard::commit() { m_committed = true; }

void ValidationStateGuard::rollback() {
    for (auto* widget : m_widgets) {
        if (widget && m_originalStates.contains(widget)) {
            bool originalState = m_originalStates[widget];
            QString originalMessage =
                widget->property("validationMessage").toString();
            ValidationUtils::setValidationState(widget, originalState,
                                                originalMessage);
        }
    }
    m_committed = true;
}
