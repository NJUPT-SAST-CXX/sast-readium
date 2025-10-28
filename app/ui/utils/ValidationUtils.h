#pragma once

#include <QString>
#include <QWidget>
#include <functional>
#include "../core/UIErrorHandler.h"

/**
 * @brief Utility class for common UI validation patterns
 *
 * Provides convenient validation methods that integrate with UIErrorHandler
 * for consistent validation across the application.
 */
class ValidationUtils {
public:
    // Validation with automatic UI feedback
    static bool validateAndShowFeedback(
        QWidget* widget, const UIErrorHandler::ValidationInfo& validation,
        bool showSuccess = false);

    // Common validation patterns with UI feedback
    static bool validatePageInput(QWidget* widget, int page, int totalPages);
    static bool validateZoomInput(QWidget* widget, double zoom);
    static bool validateFileInput(QWidget* widget, const QString& filePath,
                                  bool mustExist = true);
    static bool validateSearchInput(QWidget* widget, const QString& query,
                                    bool allowEmpty = false);
    static bool validateNumericRange(QWidget* widget, double value, double min,
                                     double max, const QString& fieldName);

    // Batch validation for forms
    struct ValidationRule {
        QWidget* widget;
        std::function<UIErrorHandler::ValidationInfo()> validator;
        QString fieldName;
        bool required;

        ValidationRule(QWidget* w,
                       std::function<UIErrorHandler::ValidationInfo()> v,
                       const QString& name, bool req = true)
            : widget(w), validator(v), fieldName(name), required(req) {}
    };

    static bool validateForm(const QList<ValidationRule>& rules,
                             QWidget* parent = nullptr);

    // Input sanitization
    static QString sanitizeTextInput(const QString& input, int maxLength = -1);
    static QString sanitizeFilePath(const QString& path);
    static double clampNumericInput(double value, double min, double max);

    // Visual feedback helpers
    static void highlightValidationError(QWidget* widget,
                                         const QString& message);
    static void clearValidationHighlight(QWidget* widget);
    static void showValidationTooltip(QWidget* widget, const QString& message,
                                      int duration = 3000);

    // Validation state management
    static void setValidationState(QWidget* widget, bool isValid,
                                   const QString& message = QString());
    static bool getValidationState(QWidget* widget);
    static void clearAllValidationStates(QWidget* parent);

private:
    ValidationUtils() = delete;  // Static utility class

    // Internal helpers
    static void applyValidationStyling(QWidget* widget,
                                       UIErrorHandler::ValidationResult result);
    static QString getValidationMessage(
        const UIErrorHandler::ValidationInfo& validation);
};

/**
 * @brief RAII class for managing validation state during form operations
 */
class ValidationStateGuard {
public:
    explicit ValidationStateGuard(QWidget* parent);
    ~ValidationStateGuard();

    void addWidget(QWidget* widget);
    void commit();    // Keep current validation states
    void rollback();  // Restore original states

private:
    QWidget* m_parent;
    QList<QWidget*> m_widgets;
    QHash<QWidget*, bool> m_originalStates;
    bool m_committed;
};

// Convenience macros for validation
#define VALIDATE_AND_RETURN(widget, validation)                          \
    if (!ValidationUtils::validateAndShowFeedback(widget, validation)) { \
        return false;                                                    \
    }

#define VALIDATE_FORM_AND_RETURN(rules)          \
    if (!ValidationUtils::validateForm(rules)) { \
        return false;                            \
    }

#define SANITIZE_INPUT(input, maxLen) \
    ValidationUtils::sanitizeTextInput(input, maxLen)

#define CLAMP_INPUT(value, min, max) \
    ValidationUtils::clampNumericInput(value, min, max)
