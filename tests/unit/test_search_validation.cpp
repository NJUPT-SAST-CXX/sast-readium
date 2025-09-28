#include <QtTest>
#include <QObject>
#include "../../app/search/SearchValidator.h"
#include "../../app/search/SearchConfiguration.h"

class TestSearchValidation : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Basic validation tests
    void testQueryValidation();
    void testEmptyQueryValidation();
    void testQueryLengthValidation();
    void testQueryCharacterValidation();
    
    // Security validation tests
    void testSecurityThreats();
    void testScriptInjectionDetection();
    void testPathTraversalDetection();
    void testResourceExhaustionDetection();
    
    // Options validation tests
    void testSearchOptionsValidation();
    void testPageRangeValidation();
    void testResultLimitsValidation();
    
    // Regex validation tests
    void testRegexPatternValidation();
    void testInvalidRegexDetection();
    void testDangerousRegexDetection();
    
    // Sanitization tests
    void testQuerySanitization();
    void testControlCharacterRemoval();
    void testWhitespaceNormalization();
    
    // Performance validation tests
    void testTimeoutValidation();
    void testMemoryLimitValidation();
    void testThreadCountValidation();
    
    // Batch validation tests
    void testSearchRequestValidation();
    void testMultipleQueryValidation();
    
    // Custom validation rules tests
    void testCustomValidationRules();
    
    // Statistics tests
    void testValidationStatistics();

private:
    SearchValidator* validator;
    ValidationConfig config;
};

void TestSearchValidation::initTestCase()
{
    // Initialize with standard validation config
    config.level = Standard;
    config.enableSanitization = true;
    config.preventResourceExhaustion = true;
    validator = new SearchValidator(config);
}

void TestSearchValidation::cleanupTestCase()
{
    delete validator;
}

void TestSearchValidation::testQueryValidation()
{
    // Test valid queries
    auto result = validator->validateQuery("test query");
    QVERIFY(result.isValid);
    QVERIFY(result.errorMessages.isEmpty());
    
    result = validator->validateQuery("search with numbers 123");
    QVERIFY(result.isValid);
    
    result = validator->validateQuery("unicode test: café");
    QVERIFY(result.isValid);
}

void TestSearchValidation::testEmptyQueryValidation()
{
    // Test empty and null queries
    auto result = validator->validateQuery("");
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(EmptyInput));
    
    result = validator->validateQuery(QString());
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(EmptyInput));
}

void TestSearchValidation::testQueryLengthValidation()
{
    // Test query length limits
    ValidationConfig strictConfig;
    strictConfig.level = Strict;
    strictConfig.maxQueryLength = 10;
    SearchValidator strictValidator(strictConfig);
    
    auto result = strictValidator.validateQuery("short");
    QVERIFY(result.isValid);
    
    result = strictValidator.validateQuery("this query is too long for the limit");
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(InvalidLength));
}

void TestSearchValidation::testQueryCharacterValidation()
{
    // Test character validation with strict mode
    ValidationConfig strictConfig;
    strictConfig.level = Strict;
    strictConfig.allowSpecialCharacters = false;
    SearchValidator strictValidator(strictConfig);
    
    auto result = strictValidator.validateQuery("normal text");
    QVERIFY(result.isValid);
    
    result = strictValidator.validateQuery("text with @#$% symbols");
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(InvalidCharacters));
    
    // Test control characters
    result = validator->validateQuery("text with\x00control");
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(InvalidCharacters));
}

void TestSearchValidation::testSecurityThreats()
{
    // Test various security threats
    auto result = validator->validateQuery("javascript:alert('xss')");
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(SecurityViolation));
    
    result = validator->validateQuery("<script>alert('xss')</script>");
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(SecurityViolation));
    
    result = validator->validateQuery("eval(malicious_code)");
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(SecurityViolation));
}

void TestSearchValidation::testScriptInjectionDetection()
{
    QVERIFY(validator->containsSuspiciousPatterns("javascript:"));
    QVERIFY(validator->containsSuspiciousPatterns("vbscript:"));
    QVERIFY(validator->containsSuspiciousPatterns("<script"));
    QVERIFY(validator->containsSuspiciousPatterns("eval("));
    QVERIFY(!validator->containsSuspiciousPatterns("normal search text"));
}

void TestSearchValidation::testPathTraversalDetection()
{
    auto result = validator->validateQuery("../../../etc/passwd");
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(SecurityViolation));
    
    result = validator->validateQuery("..\\..\\windows\\system32");
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(SecurityViolation));
}

void TestSearchValidation::testResourceExhaustionDetection()
{
    // Test extremely long input
    QString longQuery(20000, 'a');
    auto result = validator->validateQuery(longQuery);
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(SecurityViolation) || result.hasError(InvalidLength));
    
    // Test excessive repetition
    QString repetitiveQuery = QString("a").repeated(1000);
    result = validator->validateQuery(repetitiveQuery);
    QVERIFY(!result.isValid);
}

void TestSearchValidation::testSearchOptionsValidation()
{
    SearchOptions options;
    options.maxResults = 1000;
    options.contextLength = 50;
    options.searchTimeout = 30000;
    
    auto result = validator->validateSearchOptions(options);
    QVERIFY(result.isValid);
    
    // Test invalid options
    options.maxResults = -1;
    result = validator->validateSearchOptions(options);
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(InvalidRange));
    
    options.maxResults = 1000;
    options.contextLength = -10;
    result = validator->validateSearchOptions(options);
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(InvalidRange));
}

void TestSearchValidation::testPageRangeValidation()
{
    auto result = validator->validatePageRange(0, 10, 20);
    QVERIFY(result.isValid);
    
    result = validator->validatePageRange(10, 5, 20); // start > end
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(InvalidRange));
    
    result = validator->validatePageRange(0, 25, 20); // end > total
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(InvalidRange));
    
    result = validator->validatePageRange(-5, 10, 20); // negative start
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(InvalidRange));
}

void TestSearchValidation::testResultLimitsValidation()
{
    auto result = validator->validateResultLimits(100, 50);
    QVERIFY(result.isValid);
    
    result = validator->validateResultLimits(0, 50); // zero results
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(InvalidRange));
    
    result = validator->validateResultLimits(100, -10); // negative context
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(InvalidRange));
}

void TestSearchValidation::testRegexPatternValidation()
{
    SearchOptions options;
    options.useRegex = true;
    
    auto result = validator->validateQueryWithOptions(".*test.*", options);
    QVERIFY(result.isValid);
    
    result = validator->validateQueryWithOptions("[invalid", options); // Invalid regex
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(InvalidFormat));
}

void TestSearchValidation::testInvalidRegexDetection()
{
    auto result = validator->validateQuery("[unclosed bracket");
    // Should be valid as plain text
    QVERIFY(result.isValid);
    
    // But invalid when used as regex
    SearchOptions regexOptions;
    regexOptions.useRegex = true;
    result = validator->validateQueryWithOptions("[unclosed bracket", regexOptions);
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(InvalidFormat));
}

void TestSearchValidation::testDangerousRegexDetection()
{
    SearchOptions regexOptions;
    regexOptions.useRegex = true;
    
    // Test catastrophic backtracking patterns
    auto result = validator->validateQueryWithOptions(".*.*.*", regexOptions);
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(SecurityViolation));
    
    result = validator->validateQueryWithOptions(".+.+.+", regexOptions);
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(SecurityViolation));
}

void TestSearchValidation::testQuerySanitization()
{
    QString query = QString("test%1with%2control%3chars").arg(QChar(0x00)).arg(QChar(0x01)).arg(QChar(0x1f));
    auto result = validator->validateQuery(query);

    if (result.isValid) {
        QVERIFY(!result.sanitizedInput.contains(QChar(0x00)));
        QVERIFY(!result.sanitizedInput.contains(QChar(0x01)));
        QVERIFY(!result.sanitizedInput.contains(QChar(0x1f)));
    }
}

void TestSearchValidation::testControlCharacterRemoval()
{
    QString input = QString("normal%1text%2with%3control").arg(QChar(0x00)).arg(QChar(0x01)).arg(QChar(0x1f));
    QString sanitized = validator->sanitizeQuery(input);

    QVERIFY(!sanitized.contains(QChar(0x00)));
    QVERIFY(!sanitized.contains(QChar(0x01)));
    QVERIFY(!sanitized.contains(QChar(0x1f)));
    QVERIFY(sanitized.contains("normal"));
    QVERIFY(sanitized.contains("text"));
}

void TestSearchValidation::testWhitespaceNormalization()
{
    QString input = "  multiple   spaces   here  ";
    QString sanitized = validator->sanitizeQuery(input);
    
    QCOMPARE(sanitized, "multiple spaces here");
}

void TestSearchValidation::testTimeoutValidation()
{
    auto result = validator->validateTimeout(30000); // 30 seconds
    QVERIFY(result.isValid);
    
    result = validator->validateTimeout(-1000); // Negative timeout
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(InvalidRange));
    
    result = validator->validateTimeout(1000000); // Very long timeout
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(ResourceLimit));
}

void TestSearchValidation::testMemoryLimitValidation()
{
    auto result = validator->validateMemoryLimit(100 * 1024 * 1024); // 100MB
    QVERIFY(result.isValid);
    
    result = validator->validateMemoryLimit(-1); // Negative memory
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(InvalidRange));
    
    result = validator->validateMemoryLimit(500); // Too small
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(InvalidRange));
}

void TestSearchValidation::testThreadCountValidation()
{
    auto result = validator->validateThreadCount(4);
    QVERIFY(result.isValid);
    
    result = validator->validateThreadCount(0); // Zero threads
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(InvalidRange));
    
    result = validator->validateThreadCount(1000); // Too many threads
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(ResourceLimit));
}

void TestSearchValidation::testSearchRequestValidation()
{
    // This would require a mock Poppler::Document
    // For now, just test that the method exists and handles null document
    SearchOptions options;
    auto result = validator->validateSearchRequest("test", options, nullptr);
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(EmptyInput));
}

void TestSearchValidation::testMultipleQueryValidation()
{
    QStringList queries = {"valid query", "", "another valid", "javascript:alert()"};
    auto results = validator->validateMultipleQueries(queries);
    
    QCOMPARE(results.size(), 4);
    QVERIFY(results[0].isValid);
    QVERIFY(!results[1].isValid); // Empty query
    QVERIFY(results[2].isValid);
    QVERIFY(!results[3].isValid); // Security threat
}

void TestSearchValidation::testCustomValidationRules()
{
    // Add a custom rule
    validator->addCustomRule("test_rule", [](const QVariant& value) {
        SearchValidator::ValidationResult result;
        if (value.toString().contains("forbidden")) {
            result.addError(BusinessLogicViolation, "Contains forbidden word");
        }
        return result;
    });
    
    auto result = validator->applyCustomRules("test_rule", QVariant("normal text"));
    QVERIFY(result.isValid);
    
    result = validator->applyCustomRules("test_rule", QVariant("forbidden word"));
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(BusinessLogicViolation));
    
    // Remove the rule
    validator->removeCustomRule("test_rule");
    result = validator->applyCustomRules("test_rule", QVariant("anything"));
    QVERIFY(!result.isValid);
    QVERIFY(result.hasError(InvalidFormat)); // Rule not found
}

void TestSearchValidation::testValidationStatistics()
{
    validator->resetValidationStats();
    
    // Perform some validations
    validator->validateQuery("valid");
    validator->validateQuery("");
    validator->validateQuery("another valid");
    validator->validateQuery("javascript:");
    
    auto stats = validator->getValidationStats();
    QCOMPARE(stats.totalValidations, 4);
    QCOMPARE(stats.successfulValidations, 2);
    QCOMPARE(stats.failedValidations, 2);
    QVERIFY(stats.errorCounts.contains(EmptyInput));
    QVERIFY(stats.errorCounts.contains(SecurityViolation));
}

QTEST_APPLESS_MAIN(TestSearchValidation)

#include "test_search_validation.moc"
