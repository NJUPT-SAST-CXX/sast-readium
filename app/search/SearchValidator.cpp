#include "SearchValidator.h"
#include <QDebug>
#include <QRegularExpression>
#include <QTextCodec>
#include <QCoreApplication>
#include <QThread>
#include <poppler-qt6.h>
#include <algorithm>
#include <memory>

// SearchValidator Implementation class
class SearchValidator::Implementation
{
public:
    explicit Implementation(const ValidationConfig& config)
        : config(config)
    {
        // Initialize forbidden patterns for security
        if (config.forbiddenPatterns.isEmpty()) {
            this->config.forbiddenPatterns = {
                "javascript:",
                "vbscript:",
                "data:",
                "file:",
                "<script",
                "</script>",
                "eval\\(",
                "setTimeout\\(",
                "setInterval\\(",
                "Function\\(",
                "\\x00",  // Null bytes
                "\\x1f",  // Control characters
                "\\.\\./", // Path traversal
                "\\\\\\.\\.\\\\", // Windows path traversal
            };
        }
    }

    ValidationConfig config;
    mutable ValidationStats stats;
    QHash<QString, ValidationRule> customRules;

    // Internal validation methods
    ValidationResult validateQueryLength(const QString& query) const;
    ValidationResult validateQueryCharacters(const QString& query) const;
    ValidationResult validateRegexPattern(const QString& pattern) const;
    ValidationResult validateUnicodeHandling(const QString& input) const;
    ValidationResult validateAgainstForbiddenPatterns(const QString& input) const;

    // Security helpers
    bool containsScriptInjection(const QString& input) const;
    bool containsPathTraversal(const QString& input) const;
    bool containsResourceExhaustion(const QString& input) const;

    // Sanitization helpers
    QString removeControlCharacters(const QString& input) const;
    QString normalizeWhitespace(const QString& input) const;
    QString escapeSpecialCharacters(const QString& input) const;

    // Statistics helpers
    void recordValidation(const ValidationResult& result) const;
    void updateErrorStats(ValidationError error) const;
};

SearchValidator::SearchValidator(const ValidationConfig& config)
    : d(std::make_unique<Implementation>(config))
{
}

SearchValidator::~SearchValidator() = default;

void SearchValidator::setValidationLevel(ValidationLevel level)
{
    d->config.level = level;

    // Adjust validation strictness based on level
    switch (level) {
    case Basic:
        d->config.allowSpecialCharacters = true;
        d->config.enableSanitization = false;
        d->config.preventResourceExhaustion = false;
        break;
    case Standard:
        d->config.allowSpecialCharacters = true;
        d->config.enableSanitization = true;
        d->config.preventResourceExhaustion = true;
        break;
    case Strict:
        d->config.allowSpecialCharacters = false;
        d->config.enableSanitization = true;
        d->config.preventResourceExhaustion = true;
        d->config.maxQueryLength = 500;
        break;
    case Paranoid:
        d->config.allowSpecialCharacters = false;
        d->config.allowRegexPatterns = false;
        d->config.enableSanitization = true;
        d->config.preventResourceExhaustion = true;
        d->config.maxQueryLength = 200;
        d->config.maxResults = 1000;
        break;
    }
}

ValidationLevel SearchValidator::getValidationLevel() const
{
    return d->config.level;
}

void SearchValidator::setValidationConfig(const ValidationConfig& config)
{
    d->config = config;
}

ValidationConfig SearchValidator::getValidationConfig() const
{
    return d->config;
}

SearchValidator::ValidationResult SearchValidator::validateQuery(const QString& query) const
{
    ValidationResult result;
    result.sanitizedInput = query;
    
    // Record validation attempt
    d->stats.totalValidations++;

    // Basic validation
    if (query.isNull()) {
        result.addError(EmptyInput, "Query cannot be null");
        d->recordValidation(result);
        return result;
    }

    if (query.isEmpty()) {
        result.addError(EmptyInput, "Query cannot be empty");
        d->recordValidation(result);
        return result;
    }

    // Length validation
    auto lengthResult = d->validateQueryLength(query);
    if (!lengthResult.isValid) {
        result.errors |= lengthResult.errors;
        result.errorMessages.append(lengthResult.errorMessages);
        result.isValid = false;
    }

    // Character validation
    auto charResult = d->validateQueryCharacters(query);
    if (!charResult.isValid) {
        result.errors |= charResult.errors;
        result.errorMessages.append(charResult.errorMessages);
        result.isValid = false;
    }

    // Unicode validation
    auto unicodeResult = d->validateUnicodeHandling(query);
    if (!unicodeResult.isValid) {
        result.errors |= unicodeResult.errors;
        result.errorMessages.append(unicodeResult.errorMessages);
        result.isValid = false;
    }

    // Security validation
    if (d->config.level >= Standard) {
        auto securityResult = validateForSecurityThreats(query);
        if (!securityResult.isValid) {
            result.errors |= securityResult.errors;
            result.errorMessages.append(securityResult.errorMessages);
            result.isValid = false;
        }

        auto forbiddenResult = d->validateAgainstForbiddenPatterns(query);
        if (!forbiddenResult.isValid) {
            result.errors |= forbiddenResult.errors;
            result.errorMessages.append(forbiddenResult.errorMessages);
            result.isValid = false;
        }
    }

    // Sanitization
    if (d->config.enableSanitization && result.isValid) {
        result.sanitizedInput = sanitizeQuery(query);
    }

    d->recordValidation(result);
    return result;
}

SearchValidator::ValidationResult SearchValidator::validateQueryWithOptions(const QString& query, const SearchOptions& options) const
{
    ValidationResult result = validateQuery(query);
    
    if (!result.isValid) {
        return result;
    }
    
    // Validate regex pattern if regex search is enabled
    if (options.useRegex) {
        if (!d->config.allowRegexPatterns) {
            result.addError(SecurityViolation, "Regular expression patterns are not allowed");
            d->recordValidation(result);
            return result;
        }

        auto regexResult = d->validateRegexPattern(query);
        if (!regexResult.isValid) {
            result.errors |= regexResult.errors;
            result.errorMessages.append(regexResult.errorMessages);
            result.isValid = false;
        }
    }

    // Validate search options
    auto optionsResult = validateSearchOptions(options);
    if (!optionsResult.isValid) {
        result.errors |= optionsResult.errors;
        result.errorMessages.append(optionsResult.errorMessages);
        result.isValid = false;
    }

    d->recordValidation(result);
    return result;
}

QString SearchValidator::sanitizeQuery(const QString& query) const
{
    if (!d->config.enableSanitization) {
        return query;
    }

    QString sanitized = query;

    // Remove control characters
    sanitized = d->removeControlCharacters(sanitized);

    // Normalize whitespace
    sanitized = d->normalizeWhitespace(sanitized);

    // Escape special characters if needed
    if (d->config.level >= Strict) {
        sanitized = d->escapeSpecialCharacters(sanitized);
    }

    return sanitized;
}

bool SearchValidator::isQuerySafe(const QString& query) const
{
    auto result = validateQuery(query);
    return result.isValid && !result.hasError(SecurityViolation);
}

SearchValidator::ValidationResult SearchValidator::validateSearchOptions(const SearchOptions& options) const
{
    ValidationResult result;
    
    // Validate result limits
    auto limitsResult = validateResultLimits(options.maxResults, options.contextLength);
    if (!limitsResult.isValid) {
        result.errors |= limitsResult.errors;
        result.errorMessages.append(limitsResult.errorMessages);
        result.isValid = false;
    }
    
    // Validate page range
    if (options.startPage >= 0 || options.endPage >= 0) {
        auto pageResult = validatePageRange(options.startPage, options.endPage, INT_MAX);
        if (!pageResult.isValid) {
            result.errors |= pageResult.errors;
            result.errorMessages.append(pageResult.errorMessages);
            result.isValid = false;
        }
    }
    
    // Validate timeout
    auto timeoutResult = validateTimeout(options.searchTimeout);
    if (!timeoutResult.isValid) {
        result.errors |= timeoutResult.errors;
        result.errorMessages.append(timeoutResult.errorMessages);
        result.isValid = false;
    }
    
    // Validate fuzzy search parameters
    if (options.fuzzySearch) {
        if (options.fuzzyThreshold < 0 || options.fuzzyThreshold > 10) {
            result.addError(InvalidRange, QString("Fuzzy threshold must be between 0 and 10, got %1").arg(options.fuzzyThreshold));
        }
    }
    
    d->recordValidation(result);
    return result;
}

SearchValidator::ValidationResult SearchValidator::validatePageRange(int startPage, int endPage, int totalPages) const
{
    ValidationResult result;
    
    if (startPage < -1) {
        result.addError(InvalidRange, QString("Start page cannot be less than -1, got %1").arg(startPage));
    }
    
    if (endPage < -1) {
        result.addError(InvalidRange, QString("End page cannot be less than -1, got %1").arg(endPage));
    }
    
    if (startPage >= 0 && endPage >= 0 && startPage > endPage) {
        result.addError(InvalidRange, QString("Start page (%1) cannot be greater than end page (%2)").arg(startPage).arg(endPage));
    }
    
    if (totalPages > 0) {
        if (startPage >= totalPages) {
            result.addError(InvalidRange, QString("Start page (%1) exceeds total pages (%2)").arg(startPage).arg(totalPages));
        }
        
        if (endPage >= totalPages) {
            result.addError(InvalidRange, QString("End page (%1) exceeds total pages (%2)").arg(endPage).arg(totalPages));
        }
    }
    
    // Check for excessive page range
    if (startPage >= 0 && endPage >= 0) {
        int rangeSize = endPage - startPage + 1;
        if (rangeSize > d->config.maxPageRange) {
            result.addError(ResourceLimit, QString("Page range too large: %1 pages (max: %2)").arg(rangeSize).arg(d->config.maxPageRange));
        }
    }
    
    d->recordValidation(result);
    return result;
}

SearchValidator::ValidationResult SearchValidator::validateResultLimits(int maxResults, int contextLength) const
{
    ValidationResult result;
    
    if (maxResults <= 0) {
        result.addError(InvalidRange, QString("Max results must be positive, got %1").arg(maxResults));
    } else if (maxResults > d->config.maxResults) {
        result.addError(ResourceLimit, QString("Max results (%1) exceeds limit (%2)").arg(maxResults).arg(d->config.maxResults));
    }

    if (contextLength < 0) {
        result.addError(InvalidRange, QString("Context length cannot be negative, got %1").arg(contextLength));
    } else if (contextLength > d->config.maxContextLength) {
        result.addError(ResourceLimit, QString("Context length (%1) exceeds limit (%2)").arg(contextLength).arg(d->config.maxContextLength));
    }
    
    d->recordValidation(result);
    return result;
}

SearchValidator::ValidationResult SearchValidator::validateDocument(Poppler::Document* document) const
{
    ValidationResult result;
    
    if (!document) {
        result.addError(EmptyInput, "Document cannot be null");
        d->recordValidation(result);
        return result;
    }
    
    if (document->isLocked()) {
        result.addError(SecurityViolation, "Document is password protected");
    }
    
    int pageCount = document->numPages();
    if (pageCount <= 0) {
        result.addError(InvalidFormat, "Document has no pages");
    } else if (pageCount > d->config.maxPageNumber) {
        result.addError(ResourceLimit, QString("Document too large: %1 pages (max: %2)").arg(pageCount).arg(d->config.maxPageNumber));
    }
    
    d->recordValidation(result);
    return result;
}



SearchValidator::ValidationStats SearchValidator::getValidationStats() const
{
    return d->stats;
}

void SearchValidator::resetValidationStats()
{
    d->stats = ValidationStats();
}

// Batch validation methods
SearchValidator::ValidationResult SearchValidator::validateSearchRequest(const QString& query, const SearchOptions& options, Poppler::Document* document) const
{
    ValidationResult result;

    // Validate document first
    auto docResult = validateDocument(document);
    if (!docResult.isValid) {
        result.errors |= docResult.errors;
        result.errorMessages.append(docResult.errorMessages);
        result.isValid = false;
        d->recordValidation(result);
        return result;
    }

    // Validate query with options
    auto queryResult = validateQueryWithOptions(query, options);
    if (!queryResult.isValid) {
        result.errors |= queryResult.errors;
        result.errorMessages.append(queryResult.errorMessages);
        result.isValid = false;
    } else {
        result.sanitizedInput = queryResult.sanitizedInput;
    }

    // Validate page range against actual document
    if (options.startPage >= 0 || options.endPage >= 0) {
        auto pageResult = validatePageRange(options.startPage, options.endPage, document->numPages());
        if (!pageResult.isValid) {
            result.errors |= pageResult.errors;
            result.errorMessages.append(pageResult.errorMessages);
            result.isValid = false;
        }
    }

    d->recordValidation(result);
    return result;
}

QList<SearchValidator::ValidationResult> SearchValidator::validateMultipleQueries(const QStringList& queries) const
{
    QList<ValidationResult> results;

    for (const QString& query : queries) {
        results.append(validateQuery(query));
    }

    return results;
}

// Custom validation rules
void SearchValidator::addCustomRule(const QString& name, const ValidationRule& rule)
{
    d->customRules[name] = rule;
}

void SearchValidator::removeCustomRule(const QString& name)
{
    d->customRules.remove(name);
}

SearchValidator::ValidationResult SearchValidator::applyCustomRules(const QString& ruleName, const QVariant& value) const
{
    ValidationResult result;

    if (d->customRules.contains(ruleName)) {
        result = d->customRules[ruleName](value);
    } else {
        result.addError(InvalidFormat, QString("Custom rule '%1' not found").arg(ruleName));
    }

    d->recordValidation(result);
    return result;
}

// ValidationScope implementation
ValidationScope::ValidationScope(SearchValidator* validator, const QString& operation)
    : m_validator(validator), m_operation(operation), m_valid(true)
{
}

ValidationScope::~ValidationScope()
{
    if (!m_valid && m_validator) {
        qWarning() << "Validation scope" << m_operation << "completed with errors:" << getErrors();
    }
}

void ValidationScope::addValidation(const SearchValidator::ValidationResult& result)
{
    m_results.append(result);
    if (!result.isValid) {
        m_valid = false;
    }
}

bool ValidationScope::isValid() const
{
    return m_valid;
}

QStringList ValidationScope::getErrors() const
{
    QStringList allErrors;
    for (const auto& result : m_results) {
        allErrors.append(result.errorMessages);
    }
    return allErrors;
}

// Performance validation methods
SearchValidator::ValidationResult SearchValidator::validateMemoryLimit(qint64 memoryLimit) const
{
    ValidationResult result;

    if (memoryLimit < 0) {
        result.addError(ResourceLimit, "Memory limit cannot be negative");
        return result;
    }

    // Check against reasonable limits (e.g., 1GB max)
    const qint64 maxMemoryLimit = 1024 * 1024 * 1024; // 1GB
    if (memoryLimit > maxMemoryLimit) {
        result.addError(ResourceLimit, QString("Memory limit %1 exceeds maximum allowed %2").arg(memoryLimit).arg(maxMemoryLimit));
    }

    // Check against minimum reasonable limit (e.g., 1MB)
    const qint64 minMemoryLimit = 1024 * 1024; // 1MB
    if (memoryLimit > 0 && memoryLimit < minMemoryLimit) {
        result.addError(ResourceLimit, QString("Memory limit %1 is below minimum recommended %2").arg(memoryLimit).arg(minMemoryLimit));
    }

    return result;
}

SearchValidator::ValidationResult SearchValidator::validateThreadCount(int threadCount) const
{
    ValidationResult result;

    if (threadCount < 0) {
        result.addError(ResourceLimit, "Thread count cannot be negative");
        return result;
    }

    // Check against reasonable limits
    const int maxThreads = QThread::idealThreadCount() * 2; // Allow up to 2x ideal thread count
    if (threadCount > maxThreads) {
        result.addError(ResourceLimit, QString("Thread count %1 exceeds maximum recommended %2").arg(threadCount).arg(maxThreads));
    }

    // Zero threads is valid (means use default)
    if (threadCount == 0) {
        // This is valid - means use default thread count
    }

    return result;
}

SearchValidator::ValidationResult SearchValidator::validateTimeout(int timeout) const
{
    ValidationResult result;
    // Simple validation - timeout should be positive and reasonable (e.g., max 5 minutes)
    const int MAX_TIMEOUT = 300000; // 5 minutes in milliseconds
    result.isValid = (timeout > 0 && timeout <= MAX_TIMEOUT);
    if (!result.isValid) {
        result.addError(InvalidRange, QString("Invalid timeout %1 (must be between 1 and %2 ms)").arg(timeout).arg(MAX_TIMEOUT));
    }
    d->recordValidation(result);
    return result;
}

SearchValidator::ValidationResult SearchValidator::validatePageNumber(int pageNumber, int totalPages) const
{
    ValidationResult result;
    result.isValid = (pageNumber >= 0 && pageNumber < totalPages);
    if (!result.isValid) {
        result.addError(InvalidRange, QString("Invalid page number %1 (total pages: %2)").arg(pageNumber).arg(totalPages));
    }
    d->recordValidation(result);
    return result;
}

SearchValidator::ValidationResult SearchValidator::validatePageNumbers(const QList<int>& pageNumbers, int totalPages) const
{
    ValidationResult result;
    for (int pageNumber : pageNumbers) {
        if (pageNumber < 0 || pageNumber >= totalPages) {
            result.addError(InvalidRange, QString("Invalid page number %1 (total pages: %2)").arg(pageNumber).arg(totalPages));
        }
    }
    d->recordValidation(result);
    return result;
}

SearchValidator::ValidationResult SearchValidator::validateForSecurityThreats(const QString& input) const
{
    ValidationResult result;
    result.isValid = true;
    result.sanitizedInput = input;
    
    // Check for SQL injection patterns
    static const QRegularExpression sqlPattern("(;|--|'|\"|\\b(DROP|DELETE|INSERT|UPDATE|SELECT|UNION|ALTER|CREATE|EXEC|EXECUTE)\\b)", QRegularExpression::CaseInsensitiveOption);
    if (sqlPattern.match(input).hasMatch()) {
        result.addError(SecurityViolation, "Potential SQL injection detected");
    }
    
    // Check for script injection patterns
    static const QRegularExpression scriptPattern("<script|javascript:|onerror=|onload=", QRegularExpression::CaseInsensitiveOption);
    if (scriptPattern.match(input).hasMatch()) {
        result.addError(SecurityViolation, "Potential script injection detected");
    }
    
    // Check for path traversal patterns
    if (input.contains("..") || input.contains("../")) {
        result.addError(SecurityViolation, "Potential path traversal detected");
    }
    
    d->recordValidation(result);
    return result;
}

SearchValidator::ValidationResult SearchValidator::validateResourceUsage(qint64 memoryUsage, int cpuUsage) const
{
    ValidationResult result;
    
    // Check memory usage (e.g., max 2GB)
    const qint64 maxMemory = 2LL * 1024 * 1024 * 1024; // 2GB
    if (memoryUsage > maxMemory) {
        result.addError(ResourceLimit, QString("Memory usage %1 exceeds limit %2").arg(memoryUsage).arg(maxMemory));
    }
    
    // Check CPU usage (percentage)
    if (cpuUsage > 100 || cpuUsage < 0) {
        result.addError(InvalidRange, QString("Invalid CPU usage percentage: %1").arg(cpuUsage));
    } else if (cpuUsage > 90) {
        result.addError(ResourceLimit, QString("CPU usage %1%% is too high").arg(cpuUsage));
    }
    
    d->recordValidation(result);
    return result;
}

bool SearchValidator::containsSuspiciousPatterns(const QString& input) const
{
    auto result = validateForSecurityThreats(input);
    return result.hasError(SecurityViolation);
}

SearchValidator::ValidationResult SearchValidator::validateCacheKey(const QString& key) const
{
    ValidationResult result;
    
    if (key.isEmpty()) {
        result.addError(EmptyInput, "Cache key cannot be empty");
    } else if (key.length() > 255) {
        result.addError(InvalidLength, QString("Cache key too long: %1 characters (max: 255)").arg(key.length()));
    }
    
    // Check for invalid characters in cache key
    static const QRegularExpression invalidChars("[<>:\"|?*]");
    if (invalidChars.match(key).hasMatch()) {
        result.addError(InvalidCharacters, "Cache key contains invalid characters");
    }
    
    d->recordValidation(result);
    return result;
}

SearchValidator::ValidationResult SearchValidator::validateCacheSize(qint64 size, qint64 maxSize) const
{
    ValidationResult result;
    
    if (size < 0) {
        result.addError(InvalidRange, "Cache size cannot be negative");
    } else if (size > maxSize) {
        result.addError(ResourceLimit, QString("Cache size %1 exceeds maximum %2").arg(size).arg(maxSize));
    }
    
    d->recordValidation(result);
    return result;
}

// Implementation method definitions
SearchValidator::ValidationResult SearchValidator::Implementation::validateQueryLength(const QString& query) const
{
    ValidationResult result;

    if (query.length() < config.minQueryLength) {
        result.addError(InvalidLength, QString("Query too short: %1 characters (min: %2)").arg(query.length()).arg(config.minQueryLength));
    } else if (query.length() > config.maxQueryLength) {
        result.addError(InvalidLength, QString("Query too long: %1 characters (max: %2)").arg(query.length()).arg(config.maxQueryLength));
    }

    return result;
}

SearchValidator::ValidationResult SearchValidator::Implementation::validateQueryCharacters(const QString& query) const
{
    ValidationResult result;

    if (!config.allowSpecialCharacters) {
        static const QRegularExpression specialChars("[^a-zA-Z0-9\\s]");
        if (query.contains(specialChars)) {
            result.addError(InvalidCharacters, "Query contains special characters which are not allowed");
        }
    }

    return result;
}

SearchValidator::ValidationResult SearchValidator::Implementation::validateRegexPattern(const QString& pattern) const
{
    ValidationResult result;

    if (!config.allowRegexPatterns) {
        result.addError(SecurityViolation, "Regular expression patterns are not allowed");
        return result;
    }

    try {
        QRegularExpression regex(pattern);
        if (!regex.isValid()) {
            result.addError(InvalidFormat, QString("Invalid regular expression: %1").arg(regex.errorString()));
        }
    } catch (...) {
        result.addError(InvalidFormat, "Failed to validate regular expression pattern");
    }

    return result;
}

SearchValidator::ValidationResult SearchValidator::Implementation::validateUnicodeHandling(const QString& input) const
{
    ValidationResult result;

    if (!config.allowUnicodeCharacters) {
        for (const QChar& ch : input) {
            if (ch.unicode() > 127) {
                result.addError(InvalidCharacters, "Unicode characters are not allowed");
                break;
            }
        }
    }

    return result;
}

SearchValidator::ValidationResult SearchValidator::Implementation::validateAgainstForbiddenPatterns(const QString& input) const
{
    ValidationResult result;

    for (const QString& pattern : config.forbiddenPatterns) {
        QRegularExpression regex(pattern, QRegularExpression::CaseInsensitiveOption);
        if (input.contains(regex)) {
            result.addError(SecurityViolation, QString("Input contains forbidden pattern: %1").arg(pattern));
        }
    }

    return result;
}

bool SearchValidator::Implementation::containsScriptInjection(const QString& input) const
{
    for (const QString& pattern : config.forbiddenPatterns) {
        QRegularExpression regex(pattern, QRegularExpression::CaseInsensitiveOption);
        if (input.contains(regex)) {
            return true;
        }
    }
    return false;
}

bool SearchValidator::Implementation::containsPathTraversal(const QString& input) const
{
    static const QStringList pathTraversalPatterns = {
        "../", "..\\", "%2e%2e%2f", "%2e%2e%5c", "..%2f", "..%5c"
    };

    for (const QString& pattern : pathTraversalPatterns) {
        if (input.contains(pattern, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool SearchValidator::Implementation::containsResourceExhaustion(const QString& input) const
{
    // Check for patterns that could cause resource exhaustion
    if (input.length() > config.maxQueryLength * 2) {
        return true;
    }

    // Check for excessive repetition
    static const QRegularExpression repetition("(.)\\1{50,}");
    if (input.contains(repetition)) {
        return true;
    }

    return false;
}

QString SearchValidator::Implementation::removeControlCharacters(const QString& input) const
{
    QString result;
    for (const QChar& ch : input) {
        if (!ch.isNonCharacter() && ch.category() != QChar::Other_Control) {
            result.append(ch);
        }
    }
    return result;
}

QString SearchValidator::Implementation::normalizeWhitespace(const QString& input) const
{
    return input.simplified();
}

QString SearchValidator::Implementation::escapeSpecialCharacters(const QString& input) const
{
    QString result = input;
    result.replace("\\", "\\\\");
    result.replace("\"", "\\\"");
    result.replace("'", "\\'");
    return result;
}

void SearchValidator::Implementation::recordValidation(const ValidationResult& result) const
{
    if (result.isValid) {
        stats.successfulValidations++;
    } else {
        stats.failedValidations++;

        // Update error counts
        for (int i = 0; i < 32; ++i) {
            ValidationError error = static_cast<ValidationError>(1 << i);
            if (result.hasError(error)) {
                stats.errorCounts[error]++;
            }
        }

        // Update recent errors
        for (const QString& errorMsg : result.errorMessages) {
            stats.recentErrors.append(errorMsg);
            if (stats.recentErrors.size() > 100) {
                stats.recentErrors.removeFirst();
            }
        }
    }
}

void SearchValidator::Implementation::updateErrorStats(ValidationError error) const
{
    stats.errorCounts[error]++;
}
