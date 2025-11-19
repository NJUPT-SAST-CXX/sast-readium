/**
 * @file CrashHandler.cpp
 * @brief Comprehensive crash handling implementation
 * @author SAST Readium Project
 * @version 1.0
 * @date 2025-10-31
 */

#include "CrashHandler.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QStringConverter>
#include <QSysInfo>
#include <QTextStream>
#include <csignal>
#include <exception>
#include "CrashReporter.h"
#include "SimpleLogging.h"
#include "StackTrace.h"

#ifdef Q_OS_WIN
#include <psapi.h>
#include <windows.h>
#elif defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
#include <signal.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

// Static instance
CrashHandler* CrashHandler::s_instance = nullptr;

// Implementation class
class CrashHandler::Implementation {
public:
    bool initialized = false;
    bool showDialog = true;
    QString crashLogDir;
    QString lastOperation;
    QMap<QString, QString> contextData;
    std::vector<CrashCallback> callbacks;
    QMutex mutex;

    // Previous handlers
    std::terminate_handler previousTerminateHandler = nullptr;

#ifdef Q_OS_WIN
    LPTOP_LEVEL_EXCEPTION_FILTER previousExceptionFilter = nullptr;
#else
    // Signal handlers storage (Unix only)
    struct sigaction previousHandlers[32];
#endif
};

CrashHandler::CrashHandler() : d(std::make_unique<Implementation>()) {
    s_instance = this;
}

CrashHandler::~CrashHandler() {
    shutdown();
    s_instance = nullptr;
}

CrashHandler& CrashHandler::instance() {
    static CrashHandler instance;
    return instance;
}

bool CrashHandler::initialize(bool enableDialog) {
    QMutexLocker locker(&d->mutex);

    if (d->initialized) {
        return true;
    }

    // Initialize stack trace system
    if (!StackTraceUtils::initialize()) {
        return false;
    }

    d->showDialog = enableDialog;

    // Set default crash log directory
    if (d->crashLogDir.isEmpty()) {
        QString appDataDir =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (appDataDir.isEmpty()) {
            appDataDir = "./logs";
        }
        d->crashLogDir = appDataDir + "/crashes";
    }

    // Create crash log directory
    QDir().mkpath(d->crashLogDir);

    // Install handlers
    installSignalHandlers();
    installExceptionHandlers();

    d->initialized = true;

    // Log initialization
    if (SastLogging::isInitialized()) {
        SastLogging::info(QString("Crash handler initialized. Crash logs: %1")
                              .arg(d->crashLogDir));
    }

    return true;
}

void CrashHandler::shutdown() {
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        // Still clear transient state to be safe between tests
        d->callbacks.clear();
        d->contextData.clear();
        d->lastOperation.clear();
        return;
    }

    uninstallSignalHandlers();
    uninstallExceptionHandlers();
    StackTraceUtils::cleanup();

    // Clear transient state and user-provided callbacks to avoid dangling refs
    d->callbacks.clear();
    d->contextData.clear();
    d->lastOperation.clear();

    d->initialized = false;
}

bool CrashHandler::isInitialized() const { return d->initialized; }

void CrashHandler::setCrashLogDirectory(const QString& directory) {
    QMutexLocker locker(&d->mutex);

    if (directory.isEmpty()) {
        QString appDataDir =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        d->crashLogDir = appDataDir + "/crashes";
    } else {
        d->crashLogDir = directory;
    }

    QDir().mkpath(d->crashLogDir);
}

QString CrashHandler::getCrashLogDirectory() const { return d->crashLogDir; }

void CrashHandler::setShowErrorDialog(bool show) { d->showDialog = show; }

bool CrashHandler::getShowErrorDialog() const { return d->showDialog; }

void CrashHandler::registerCrashCallback(CrashCallback callback) {
    QMutexLocker locker(&d->mutex);
    d->callbacks.push_back(callback);
}

void CrashHandler::clearCrashCallbacks() {
    QMutexLocker locker(&d->mutex);
    d->callbacks.clear();
}

void CrashHandler::setContextData(const QString& key, const QString& value) {
    QMutexLocker locker(&d->mutex);
    d->contextData[key] = value;
}

void CrashHandler::clearContextData() {
    QMutexLocker locker(&d->mutex);
    d->contextData.clear();
}

void CrashHandler::setLastOperation(const QString& operation) {
    QMutexLocker locker(&d->mutex);
    d->lastOperation = operation;
}

void CrashHandler::triggerTestCrash(const QString& message) {
    handleCrash("Test Crash", message,
                StackTraceUtils::captureAndFormatStackTrace());
}

QStringList CrashHandler::getCrashLogFiles() const {
    QDir dir(d->crashLogDir);
    QStringList filters;
    filters << "crash_*.log";

    QFileInfoList files =
        dir.entryInfoList(filters, QDir::Files, QDir::Time | QDir::Reversed);

    QStringList result;
    for (const QFileInfo& fileInfo : files) {
        result.append(fileInfo.absoluteFilePath());
    }

    return result;
}

QString CrashHandler::getMostRecentCrashLog() const {
    QStringList logs = getCrashLogFiles();
    return logs.isEmpty() ? QString() : logs.first();
}

void CrashHandler::cleanupOldCrashLogs(int keepCount) {
    QStringList logs = getCrashLogFiles();

    // Remove old logs, keeping only the most recent ones
    for (int i = keepCount; i < logs.size(); ++i) {
        QFile::remove(logs[i]);
    }
}

void CrashHandler::handleCrash(const QString& exceptionType,
                               const QString& exceptionMessage,
                               const QString& stackTrace) {
    // Create crash info
    CrashInfo info;
    info.timestamp = QDateTime::currentDateTime();
    info.exceptionType = exceptionType;
    info.exceptionMessage = exceptionMessage;
    info.stackTrace = stackTrace;
    info.threadInfo = StackTraceUtils::getThreadInfo();
    info.lastOperation = d->lastOperation;
    info.customData = d->contextData;

    // Collect system information
    collectSystemInfo(info);

    // Call registered callbacks
    bool continueHandling = true;
    for (const auto& callback : d->callbacks) {
        if (!callback(info)) {
            continueHandling = false;
            break;
        }
    }

    if (!continueHandling) {
        return;
    }

    // Write crash log
    info.logFilePath = writeCrashLog(info);

    // Emit signal
    emit crashDetected(info);

    // Show error dialog if enabled
    if (d->showDialog) {
        showErrorDialog(info);
    }

    // Flush logs
    if (SastLogging::isInitialized()) {
        SastLogging::flush();
    }
}

QString CrashHandler::writeCrashLog(const CrashInfo& info) {
    // Include milliseconds to avoid filename collisions in rapid successive
    // crashes
    QString timestamp = info.timestamp.toString("yyyy-MM-dd_HH-mm-ss_zzz");
    QString filename = QString("crash_%1.log").arg(timestamp);
    QString filepath = d->crashLogDir + "/" + filename;

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Write crash report header
    out << "==================================================================="
           "=============\n";
    out << "                        SAST READIUM CRASH REPORT\n";
    out << "==================================================================="
           "=============\n\n";

    // Crash information
    out << "Crash Time: " << info.timestamp.toString("yyyy-MM-dd HH:mm:ss")
        << "\n";
    out << "Exception Type: " << info.exceptionType << "\n";
    out << "Exception Message: " << info.exceptionMessage << "\n\n";

    // Application information
    out << "-------------------------------------------------------------------"
           "-------------\n";
    out << "Application Information\n";
    out << "-------------------------------------------------------------------"
           "-------------\n";
    out << "Version: " << info.applicationVersion << "\n";
    out << "Qt Version: " << info.qtVersion << "\n";
    out << "Platform: " << info.platform << "\n";
    out << "Architecture: " << info.architecture << "\n";
    out << "Memory Usage: "
        << QString::number(info.memoryUsage / (1024.0 * 1024.0), 'f', 2)
        << " MB\n\n";

    // Thread information
    out << "-------------------------------------------------------------------"
           "-------------\n";
    out << "Thread Information\n";
    out << "-------------------------------------------------------------------"
           "-------------\n";
    out << info.threadInfo << "\n\n";

    // Last operation
    if (!info.lastOperation.isEmpty()) {
        out << "---------------------------------------------------------------"
               "-----------------\n";
        out << "Last Operation\n";
        out << "---------------------------------------------------------------"
               "-----------------\n";
        out << info.lastOperation << "\n\n";
    }

    // Custom context data
    if (!info.customData.isEmpty()) {
        out << "---------------------------------------------------------------"
               "-----------------\n";
        out << "Context Data\n";
        out << "---------------------------------------------------------------"
               "-----------------\n";
        for (auto it = info.customData.constBegin();
             it != info.customData.constEnd(); ++it) {
            out << it.key() << ": " << it.value() << "\n";
        }
        out << "\n";
    }

    // Stack trace
    out << "-------------------------------------------------------------------"
           "-------------\n";
    out << "Stack Trace\n";
    out << "-------------------------------------------------------------------"
           "-------------\n";
    out << info.stackTrace << "\n";

    out << "==================================================================="
           "=============\n";
    out << "                           END OF CRASH REPORT\n";
    out << "==================================================================="
           "=============\n";

    file.close();

    return filepath;
}

void CrashHandler::showErrorDialog(const CrashInfo& info) {
    // Use the custom crash reporter dialog
    CrashReporter::showCrashReport(info, nullptr);
}

void CrashHandler::collectSystemInfo(CrashInfo& info) {
    info.applicationVersion = QCoreApplication::applicationVersion();
    if (info.applicationVersion.isEmpty()) {
        info.applicationVersion = "Unknown";
    }

    info.qtVersion = qVersion();
    info.platform = QSysInfo::prettyProductName();
    info.architecture = QSysInfo::currentCpuArchitecture();
    info.memoryUsage = getMemoryUsage();
}

qint64 CrashHandler::getMemoryUsage() {
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (K32GetProcessMemoryInfo(GetCurrentProcess(),
                                (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return static_cast<qint64>(pmc.WorkingSetSize);
    }
#elif defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
#ifdef Q_OS_LINUX
        return usage.ru_maxrss * 1024;  // Linux reports in KB
#else
        return usage.ru_maxrss;  // macOS reports in bytes
#endif
    }
#endif
    return -1;
}

void CrashHandler::installSignalHandlers() {
#ifdef Q_OS_WIN
    // Windows uses structured exception handling
    d->previousExceptionFilter =
        SetUnhandledExceptionFilter(windowsExceptionHandler);
#else
    // Unix-like systems use signal handlers
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    // Install handlers for common crash signals
    sigaction(SIGSEGV, &sa,
              &d->previousHandlers[SIGSEGV]);  // Segmentation fault
    sigaction(SIGABRT, &sa, &d->previousHandlers[SIGABRT]);  // Abort
    sigaction(SIGFPE, &sa,
              &d->previousHandlers[SIGFPE]);  // Floating point exception
    sigaction(SIGILL, &sa,
              &d->previousHandlers[SIGILL]);  // Illegal instruction
    sigaction(SIGBUS, &sa, &d->previousHandlers[SIGBUS]);  // Bus error
#endif
}

void CrashHandler::uninstallSignalHandlers() {
#ifdef Q_OS_WIN
    if (d->previousExceptionFilter != nullptr) {
        SetUnhandledExceptionFilter(d->previousExceptionFilter);
        d->previousExceptionFilter = nullptr;
    }
#else
    // Restore previous signal handlers
    sigaction(SIGSEGV, &d->previousHandlers[SIGSEGV], nullptr);
    sigaction(SIGABRT, &d->previousHandlers[SIGABRT], nullptr);
    sigaction(SIGFPE, &d->previousHandlers[SIGFPE], nullptr);
    sigaction(SIGILL, &d->previousHandlers[SIGILL], nullptr);
    sigaction(SIGBUS, &d->previousHandlers[SIGBUS], nullptr);
#endif
}

void CrashHandler::installExceptionHandlers() {
    // Install C++ exception handlers
    d->previousTerminateHandler = std::set_terminate(terminateHandler);
}

void CrashHandler::uninstallExceptionHandlers() {
    if (d->previousTerminateHandler != nullptr) {
        std::set_terminate(d->previousTerminateHandler);
        d->previousTerminateHandler = nullptr;
    }
}

void CrashHandler::signalHandler(int signal) {
    if (s_instance == nullptr) {
        return;
    }

    QString signalName;
    QString signalDescription;

    switch (signal) {
        case SIGSEGV:
            signalName = "SIGSEGV";
            signalDescription = "Segmentation fault (invalid memory access)";
            break;
        case SIGABRT:
            signalName = "SIGABRT";
            signalDescription = "Abort signal (abnormal termination)";
            break;
        case SIGFPE:
            signalName = "SIGFPE";
            signalDescription = "Floating point exception";
            break;
        case SIGILL:
            signalName = "SIGILL";
            signalDescription = "Illegal instruction";
            break;
#ifndef Q_OS_WIN
        case SIGBUS:
            signalName = "SIGBUS";
            signalDescription = "Bus error (invalid memory alignment)";
            break;
#endif
        default:
            signalName = QString("Signal %1").arg(signal);
            signalDescription = "Unknown signal";
            break;
    }

    QString stackTrace = StackTraceUtils::captureAndFormatStackTrace(64, 2);
    s_instance->handleCrash(signalName, signalDescription, stackTrace);

    // Re-raise the signal to allow default handling
    std::signal(signal, SIG_DFL);
    std::raise(signal);
}

void CrashHandler::terminateHandler() {
    if (s_instance == nullptr) {
        std::abort();
    }

    QString exceptionType = "std::terminate";
    QString exceptionMessage = "Unhandled exception";

    // Try to get exception information
    try {
        auto currentException = std::current_exception();
        if (currentException) {
            try {
                std::rethrow_exception(currentException);
            } catch (const std::exception& e) {
                exceptionType = "std::exception";
                exceptionMessage = QString::fromUtf8(e.what());
            } catch (const QString& e) {
                exceptionType = "QString exception";
                exceptionMessage = e;
            } catch (...) {
                exceptionType = "Unknown exception";
                exceptionMessage = "Non-standard exception thrown";
            }
        }
    } catch (...) {
        // Failed to get exception info
    }

    QString stackTrace = StackTraceUtils::captureAndFormatStackTrace(64, 2);
    s_instance->handleCrash(exceptionType, exceptionMessage, stackTrace);

    std::abort();
}

#ifdef Q_OS_WIN
LONG WINAPI
CrashHandler::windowsExceptionHandler(EXCEPTION_POINTERS* exceptionInfo) {
    if (s_instance == nullptr) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    QString exceptionType;
    QString exceptionMessage;

    DWORD code = exceptionInfo->ExceptionRecord->ExceptionCode;

    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:
            exceptionType = "EXCEPTION_ACCESS_VIOLATION";
            exceptionMessage = "Access violation (invalid memory access)";
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            exceptionType = "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
            exceptionMessage = "Array bounds exceeded";
            break;
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            exceptionType = "EXCEPTION_DATATYPE_MISALIGNMENT";
            exceptionMessage = "Datatype misalignment";
            break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            exceptionType = "EXCEPTION_FLT_DIVIDE_BY_ZERO";
            exceptionMessage = "Floating point divide by zero";
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            exceptionType = "EXCEPTION_INT_DIVIDE_BY_ZERO";
            exceptionMessage = "Integer divide by zero";
            break;
        case EXCEPTION_STACK_OVERFLOW:
            exceptionType = "EXCEPTION_STACK_OVERFLOW";
            exceptionMessage = "Stack overflow";
            break;
        default:
            exceptionType =
                QString("Windows Exception 0x%1").arg(code, 8, 16, QChar('0'));
            exceptionMessage = "Unhandled Windows exception";
            break;
    }

    QString stackTrace = StackTraceUtils::captureAndFormatStackTrace(64, 2);
    s_instance->handleCrash(exceptionType, exceptionMessage, stackTrace);

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

// CrashContextGuard implementation
CrashContextGuard::CrashContextGuard(const QString& operation) {
    CrashHandler& handler = CrashHandler::instance();
    m_previousOperation = handler.d->lastOperation;
    handler.setLastOperation(operation);
}

CrashContextGuard::~CrashContextGuard() {
    CrashHandler::instance().setLastOperation(m_previousOperation);
}
