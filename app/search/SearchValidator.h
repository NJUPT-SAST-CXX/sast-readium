#pragma once

#include <QHash>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <functional>
#include <memory>
#include "SearchConfiguration.h"

// Forward declarations
namespace Poppler {
class Document;
}

enum ValidationLevel {
    Basic,     // Basic null/empty checks
    Standard,  // Standard validation with bounds checking
    Strict,    // Strict validation with security checks
    Paranoid   // Maximum validation with all checks enabled
};

enum ValidationError {
    NoError = 0,
    EmptyInput = 1,
    InvalidLength = 2,
    InvalidCharacters = 4,
    InvalidFormat = 8,
    InvalidRange = 16,
    SecurityViolation = 32,
    ResourceLimit = 64,
    BusinessLogicViolation = 128
};
Q_DECLARE_FLAGS(ValidationErrors, ValidationError)

struct ValidationConfig {
    ValidationLevel level = Standard;

    // Query validation
    int minQueryLength = 1;
    int maxQueryLength = 1000;
    bool allowSpecialCharacters = true;
    bool allowUnicodeCharacters = true;
    bool allowRegexPatterns = true;
    QStringList forbiddenPatterns;

    // Page validation
    int maxPageNumber = 10000;
    int maxPageRange = 1000;

    // Results validation
    int maxResults = 10000;
    int maxContextLength = 500;

    // Performance limits
    int maxSearchTimeout = 300000;  // 5 minutes
    int maxConcurrentSearches = 10;

    // Security settings
    bool enableSanitization = true;
    bool preventResourceExhaustion = true;
    bool logValidationFailures = true;
};

/**
 * Comprehensive input validation for search operations
 * Provides security, boundary, and business logic validation
 */
class SearchValidator {
public:
    struct ValidationResult {
        bool isValid = true;
        ValidationErrors errors = NoError;
        QStringList errorMessages;
        QString sanitizedInput;

        void addError(ValidationError error, const QString& message) {
            isValid = false;
            errors |= error;
            errorMessages.append(message);
        }

        bool hasError(ValidationError error) const {
            return errors.testFlag(error);
        }
    };

    explicit SearchValidator(
        const ValidationConfig& config = ValidationConfig());
    ~SearchValidator();

    // Configuration
    void setValidationLevel(ValidationLevel level);
    ValidationLevel getValidationLevel() const;
    void setValidationConfig(const ValidationConfig& config);
    ValidationConfig getValidationConfig() const;

    // Query validation
    ValidationResult validateQuery(const QString& query) const;
    ValidationResult validateQueryWithOptions(
        const QString& query, const SearchOptions& options) const;
    QString sanitizeQuery(const QString& query) const;
    bool isQuerySafe(const QString& query) const;

    // Search options validation
    ValidationResult validateSearchOptions(const SearchOptions& options) const;
    ValidationResult validatePageRange(int startPage, int endPage,
                                       int totalPages) const;
    ValidationResult validateResultLimits(int maxResults,
                                          int contextLength) const;

    // Document validation
    ValidationResult validateDocument(class Poppler::Document* document) const;
    ValidationResult validatePageNumber(int pageNumber, int totalPages) const;
    ValidationResult validatePageNumbers(const QList<int>& pageNumbers,
                                         int totalPages) const;

    // Cache validation
    ValidationResult validateCacheKey(const QString& key) const;
    ValidationResult validateCacheSize(qint64 size, qint64 maxSize) const;

    // Performance validation
    ValidationResult validateTimeout(int timeout) const;
    ValidationResult validateMemoryLimit(qint64 memoryLimit) const;
    ValidationResult validateThreadCount(int threadCount) const;

    // Security validation
    ValidationResult validateForSecurityThreats(const QString& input) const;
    ValidationResult validateResourceUsage(qint64 memoryUsage,
                                           int cpuUsage) const;
    bool containsSuspiciousPatterns(const QString& input) const;

    // Batch validation
    ValidationResult validateSearchRequest(
        const QString& query, const SearchOptions& options,
        class Poppler::Document* document) const;
    QList<ValidationResult> validateMultipleQueries(
        const QStringList& queries) const;

    // Custom validation rules
    using ValidationRule = std::function<ValidationResult(const QVariant&)>;
    void addCustomRule(const QString& name, const ValidationRule& rule);
    void removeCustomRule(const QString& name);
    ValidationResult applyCustomRules(const QString& ruleName,
                                      const QVariant& value) const;

    // Validation statistics
    struct ValidationStats {
        int totalValidations = 0;
        int successfulValidations = 0;
        int failedValidations = 0;
        QHash<ValidationError, int> errorCounts;
        QStringList recentErrors;
    };

    ValidationStats getValidationStats() const;
    void resetValidationStats();

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ValidationErrors)

/**
 * Validation exception for critical validation failures
 */
class ValidationException : public std::exception {
public:
    explicit ValidationException(const QString& message)
        : m_message(message.toStdString()) {}
    const char* what() const noexcept override { return m_message.c_str(); }

private:
    std::string m_message;
};

/**
 * RAII validation scope for automatic validation
 */
class ValidationScope {
public:
    explicit ValidationScope(SearchValidator* validator,
                             const QString& operation);
    ~ValidationScope();

    void addValidation(const SearchValidator::ValidationResult& result);
    bool isValid() const;
    QStringList getErrors() const;

private:
    SearchValidator* m_validator;
    QString m_operation;
    QList<SearchValidator::ValidationResult> m_results;
    bool m_valid;
};

/**
 * Validation helper macros for common validation patterns
 */
#define VALIDATE_QUERY(validator, query)                                \
    do {                                                                \
        auto result = validator->validateQuery(query);                  \
        if (!result.isValid) {                                          \
            throw ValidationException(result.errorMessages.join("; ")); \
        }                                                               \
    } while (0)

#define VALIDATE_SEARCH_REQUEST(validator, query, options, document)    \
    do {                                                                \
        auto result =                                                   \
            validator->validateSearchRequest(query, options, document); \
        if (!result.isValid) {                                          \
            throw ValidationException(result.errorMessages.join("; ")); \
        }                                                               \
    } while (0)

#define VALIDATE_OR_RETURN(validator, input, returnValue) \
    do {                                                  \
        auto result = validator->validateQuery(input);    \
        if (!result.isValid) {                            \
            return returnValue;                           \
        }                                                 \
    } while (0)
