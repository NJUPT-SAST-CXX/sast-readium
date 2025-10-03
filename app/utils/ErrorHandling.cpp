#include "ErrorHandling.h"
#include "../logging/Logger.h"

namespace ErrorHandling {

void logError(const ErrorInfo& errorInfo) {
    QString logMessage = QString("[%1] %2: %3")
                             .arg(categoryToString(errorInfo.category))
                             .arg(severityToString(errorInfo.severity))
                             .arg(errorInfo.message);

    if (!errorInfo.details.isEmpty()) {
        logMessage += QString(" - Details: %1").arg(errorInfo.details);
    }

    if (!errorInfo.context.isEmpty()) {
        logMessage += QString(" - Context: %1").arg(errorInfo.context);
    }

    if (errorInfo.errorCode != 0) {
        logMessage += QString(" - Code: %1").arg(errorInfo.errorCode);
    }

    // Log based on severity level
    switch (errorInfo.severity) {
        case ErrorSeverity::Info:
            Logger::instance().info(logMessage);
            break;
        case ErrorSeverity::Warning:
            Logger::instance().warning(logMessage);
            break;
        case ErrorSeverity::Error:
            Logger::instance().error(logMessage);
            break;
        case ErrorSeverity::Critical:
        case ErrorSeverity::Fatal:
            Logger::instance().critical(logMessage);
            break;
    }
}

QString categoryToString(ErrorCategory category) {
    switch (category) {
        case ErrorCategory::FileSystem:
            return "FileSystem";
        case ErrorCategory::Document:
            return "Document";
        case ErrorCategory::Rendering:
            return "Rendering";
        case ErrorCategory::Search:
            return "Search";
        case ErrorCategory::Cache:
            return "Cache";
        case ErrorCategory::Network:
            return "Network";
        case ErrorCategory::Threading:
            return "Threading";
        case ErrorCategory::UI:
            return "UI";
        case ErrorCategory::Plugin:
            return "Plugin";
        case ErrorCategory::Configuration:
            return "Configuration";
        case ErrorCategory::Memory:
            return "Memory";
        case ErrorCategory::Unknown:
            return "Unknown";
    }
    return "Unknown";
}

QString severityToString(ErrorSeverity severity) {
    switch (severity) {
        case ErrorSeverity::Info:
            return "INFO";
        case ErrorSeverity::Warning:
            return "WARNING";
        case ErrorSeverity::Error:
            return "ERROR";
        case ErrorSeverity::Critical:
            return "CRITICAL";
        case ErrorSeverity::Fatal:
            return "FATAL";
    }
    return "UNKNOWN";
}

ErrorInfo createFileSystemError(const QString& operation, const QString& path,
                                const QString& details) {
    return ErrorInfo(ErrorCategory::FileSystem, ErrorSeverity::Error,
                     QString("File system operation failed: %1").arg(operation),
                     details.isEmpty()
                         ? QString("Path: %1").arg(path)
                         : QString("Path: %1, %2").arg(path, details),
                     operation);
}

ErrorInfo createDocumentError(const QString& operation,
                              const QString& details) {
    return ErrorInfo(ErrorCategory::Document, ErrorSeverity::Error,
                     QString("Document operation failed: %1").arg(operation),
                     details, operation);
}

ErrorInfo createRenderingError(const QString& operation,
                               const QString& details) {
    return ErrorInfo(ErrorCategory::Rendering, ErrorSeverity::Error,
                     QString("Rendering operation failed: %1").arg(operation),
                     details, operation);
}

ErrorInfo createSearchError(const QString& operation, const QString& details) {
    return ErrorInfo(ErrorCategory::Search, ErrorSeverity::Error,
                     QString("Search operation failed: %1").arg(operation),
                     details, operation);
}

ErrorInfo createCacheError(const QString& operation, const QString& details) {
    return ErrorInfo(
        ErrorCategory::Cache,
        ErrorSeverity::Warning,  // Cache errors are usually non-critical
        QString("Cache operation failed: %1").arg(operation), details,
        operation);
}

ErrorInfo createThreadingError(const QString& operation,
                               const QString& details) {
    return ErrorInfo(
        ErrorCategory::Threading,
        ErrorSeverity::Critical,  // Threading errors are usually serious
        QString("Threading operation failed: %1").arg(operation), details,
        operation);
}

}  // namespace ErrorHandling
