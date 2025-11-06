/**
 * @file CrashHandler.h
 * @brief Comprehensive crash handling and logging system
 * @author SAST Readium Project
 * @version 1.0
 * @date 2025-10-31
 *
 * This file provides automatic crash detection, stack trace capture,
 * and detailed crash log generation for debugging purposes.
 */

#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <functional>
#include <memory>

// Platform-specific includes
#ifdef Q_OS_WIN
#include <windows.h>
#endif

/**
 * @brief Crash information structure
 */
struct CrashInfo {
    QDateTime timestamp;         ///< When the crash occurred
    QString exceptionType;       ///< Type of exception/signal
    QString exceptionMessage;    ///< Exception message
    QString stackTrace;          ///< Formatted stack trace
    QString threadInfo;          ///< Thread information
    QString applicationVersion;  ///< Application version
    QString qtVersion;           ///< Qt version
    QString platform;            ///< Operating system
    QString architecture;        ///< CPU architecture
    qint64 memoryUsage;          ///< Memory usage at crash time
    QString logFilePath;         ///< Path to crash log file

    // Additional context
    QString lastOperation;              ///< Last known operation
    QMap<QString, QString> customData;  ///< Custom context data
};

/**
 * @brief Crash handler callback type
 * @param info Crash information
 * @return true to continue with default handling, false to suppress
 */
using CrashCallback = std::function<bool(const CrashInfo& info)>;

/**
 * @brief Comprehensive crash handler for automatic crash logging
 *
 * This class provides:
 * - Automatic crash detection (signals, exceptions)
 * - Stack trace capture
 * - Detailed crash log generation
 * - User-friendly error reporting
 * - Cross-platform support (Windows, Linux, macOS)
 */
class CrashHandler : public QObject {
    Q_OBJECT
    friend class CrashContextGuard;

public:
    /**
     * @brief Get singleton instance
     */
    static CrashHandler& instance();

    /**
     * @brief Initialize crash handler
     * @param enableDialog Show error dialog on crash
     * @return true if successful
     */
    bool initialize(bool enableDialog = true);

    /**
     * @brief Shutdown crash handler
     */
    void shutdown();

    /**
     * @brief Check if crash handler is initialized
     */
    bool isInitialized() const;

    /**
     * @brief Set crash log directory
     * @param directory Directory path (empty for default)
     */
    void setCrashLogDirectory(const QString& directory);

    /**
     * @brief Get crash log directory
     */
    QString getCrashLogDirectory() const;

    /**
     * @brief Set whether to show error dialog on crash
     */
    void setShowErrorDialog(bool show);

    /**
     * @brief Get whether error dialog is shown on crash
     */
    bool getShowErrorDialog() const;

    /**
     * @brief Register a callback to be called on crash
     * @param callback Callback function
     */
    void registerCrashCallback(CrashCallback callback);

    /**
     * @brief Clear all crash callbacks
     */
    void clearCrashCallbacks();

    /**
     * @brief Set custom context data
     * @param key Context key
     * @param value Context value
     */
    void setContextData(const QString& key, const QString& value);

    /**
     * @brief Clear custom context data
     */
    void clearContextData();

    /**
     * @brief Set last operation (for context)
     * @param operation Operation description
     */
    void setLastOperation(const QString& operation);

    /**
     * @brief Manually trigger crash report (for testing)
     * @param message Test message
     */
    void triggerTestCrash(const QString& message = "Test crash");

    /**
     * @brief Get list of crash log files
     */
    QStringList getCrashLogFiles() const;

    /**
     * @brief Get most recent crash log file
     */
    QString getMostRecentCrashLog() const;

    /**
     * @brief Clean up old crash logs
     * @param keepCount Number of recent logs to keep (default: 10)
     */
    void cleanupOldCrashLogs(int keepCount = 10);

signals:
    /**
     * @brief Emitted when a crash is detected
     * @param info Crash information
     */
    void crashDetected(const CrashInfo& info);

private:
    CrashHandler();
    ~CrashHandler() override;
    CrashHandler(const CrashHandler&) = delete;
    CrashHandler& operator=(const CrashHandler&) = delete;

    /**
     * @brief Handle crash internally
     * @param exceptionType Exception/signal type
     * @param exceptionMessage Exception message
     * @param stackTrace Stack trace
     */
    void handleCrash(const QString& exceptionType,
                     const QString& exceptionMessage,
                     const QString& stackTrace);

    /**
     * @brief Write crash log to file
     * @param info Crash information
     * @return Path to crash log file
     */
    QString writeCrashLog(const CrashInfo& info);

    /**
     * @brief Show error dialog
     * @param info Crash information
     */
    void showErrorDialog(const CrashInfo& info);

    /**
     * @brief Collect system information
     * @param info Crash information to fill
     */
    void collectSystemInfo(CrashInfo& info);

    /**
     * @brief Get memory usage
     * @return Memory usage in bytes
     */
    qint64 getMemoryUsage();

    /**
     * @brief Install signal handlers
     */
    void installSignalHandlers();

    /**
     * @brief Uninstall signal handlers
     */
    void uninstallSignalHandlers();

    /**
     * @brief Install exception handlers
     */
    void installExceptionHandlers();

    /**
     * @brief Uninstall exception handlers
     */
    void uninstallExceptionHandlers();

    // Platform-specific handlers
    static void signalHandler(int signal);
    static void terminateHandler();

#ifdef Q_OS_WIN
    static LONG WINAPI
    windowsExceptionHandler(EXCEPTION_POINTERS* exceptionInfo);
#endif

    class Implementation;
    std::unique_ptr<Implementation> d;

    // Static instance for signal handlers
    static CrashHandler* s_instance;
};

/**
 * @brief RAII helper for setting operation context
 */
class CrashContextGuard {
public:
    explicit CrashContextGuard(const QString& operation);
    ~CrashContextGuard();

private:
    QString m_previousOperation;
};

// Convenience macro for crash context
#define CRASH_CONTEXT(operation) CrashContextGuard _crashContext(operation)
