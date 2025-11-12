/**
 * @file StackTrace.cpp
 * @brief Cross-platform stack trace capture implementation
 * @author SAST Readium Project
 * @version 1.0
 * @date 2025-10-31
 */

#include "StackTrace.h"
#include <QCoreApplication>
#include <QDateTime>
#include <sstream>

// Platform-specific includes
#ifdef Q_OS_WIN
#include <dbghelp.h>
#include <psapi.h>
#include <windows.h>
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "psapi.lib")
#elif defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <pthread.h>
#include <unistd.h>
#endif

namespace StackTraceUtils {

// Static initialization flag
static bool s_initialized = false;

#ifdef Q_OS_WIN
// Windows-specific initialization
static HANDLE s_process = nullptr;
static bool s_symInitialized = false;
#endif

QString StackFrame::toString() const {
    QString result;

    if (!functionName.isEmpty()) {
        result += functionName;
    } else {
        result += "<unknown function>";
    }

    if (!fileName.isEmpty() && lineNumber >= 0) {
        result += QString(" at %1:%2").arg(fileName).arg(lineNumber);
    } else if (!moduleName.isEmpty()) {
        result += QString(" in %1").arg(moduleName);
    }

    if (address != nullptr) {
        result +=
            QString(" [0x%1]").arg(reinterpret_cast<quintptr>(address), 0, 16);
    }

    return result;
}

bool initialize() {
    if (s_initialized) {
        return true;
    }

#ifdef Q_OS_WIN
    s_process = GetCurrentProcess();

    // Initialize symbol handler
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);

    if (SymInitialize(s_process, nullptr, TRUE)) {
        s_symInitialized = true;
        s_initialized = true;
        return true;
    }
    return false;
#else
    // Unix-like systems don't need special initialization
    s_initialized = true;
    return true;
#endif
}

void cleanup() {
#ifdef Q_OS_WIN
    if (s_symInitialized) {
        SymCleanup(s_process);
        s_symInitialized = false;
    }
#endif
    s_initialized = false;
}

bool isAvailable() {
#ifdef Q_OS_WIN
    return true;  // DbgHelp is available on Windows
#elif defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    return true;  // backtrace is available on Unix-like systems
#else
    return false;
#endif
}

QString demangleSymbol(const QString& mangledName) {
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    int status = 0;
    char* demangled = abi::__cxa_demangle(mangledName.toUtf8().constData(),
                                          nullptr, nullptr, &status);

    if (status == 0 && demangled) {
        QString result = QString::fromUtf8(demangled);
        free(demangled);
        return result;
    }
#endif
    return mangledName;
}

QString getCurrentThreadId() {
#ifdef Q_OS_WIN
    return QString::number(GetCurrentThreadId());
#elif defined(Q_OS_LINUX)
    return QString::number(static_cast<qulonglong>(pthread_self()));
#elif defined(Q_OS_MACOS)
    uint64_t tid;
    pthread_threadid_np(nullptr, &tid);
    return QString::number(tid);
#else
    return QString::number(
        reinterpret_cast<qulonglong>(QThread::currentThreadId()));
#endif
}

QString getCurrentThreadName() {
    QThread* thread = QThread::currentThread();
    if (thread) {
        QString name = thread->objectName();
        if (!name.isEmpty()) {
            return name;
        }
    }

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    char threadName[256] = {0};
    if (pthread_getname_np(pthread_self(), threadName, sizeof(threadName)) ==
        0) {
        if (threadName[0] != '\0') {
            return QString::fromUtf8(threadName);
        }
    }
#endif

    return QString();
}

QString getThreadInfo() {
    QString threadId = getCurrentThreadId();
    QString threadName = getCurrentThreadName();

    if (!threadName.isEmpty()) {
        return QString("Thread %1 (%2)").arg(threadId).arg(threadName);
    } else {
        return QString("Thread %1").arg(threadId);
    }
}

std::vector<StackFrame> captureStackTrace(int maxFrames, int skipFrames) {
    std::vector<StackFrame> frames;

    if (!s_initialized) {
        initialize();
    }

#ifdef Q_OS_WIN
    // Windows implementation using CaptureStackBackTrace and SymFromAddr
    std::vector<void*> stack(maxFrames + skipFrames);
    USHORT capturedFrames =
        CaptureStackBackTrace(skipFrames, maxFrames, stack.data(), nullptr);

    if (!s_symInitialized) {
        // Fallback: just return addresses
        for (USHORT i = 0; i < capturedFrames; ++i) {
            StackFrame frame;
            frame.address = stack[i];
            frame.addressString = QString("0x%1").arg(
                reinterpret_cast<quintptr>(stack[i]), 0, 16);
            frames.push_back(frame);
        }
        return frames;
    }

    // Symbol buffer (allocate safely on the heap to avoid stack overrun)
    constexpr DWORD kMaxNameLen = MAX_SYM_NAME;  // per DbgHelp docs
    std::vector<char> symStorage(sizeof(SYMBOL_INFO) + kMaxNameLen);
    SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(symStorage.data());
    symbol->MaxNameLen = kMaxNameLen;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    // Line info
    IMAGEHLP_LINE64 line;
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    for (USHORT i = 0; i < capturedFrames; ++i) {
        StackFrame frame;
        frame.address = stack[i];
        frame.addressString =
            QString("0x%1").arg(reinterpret_cast<quintptr>(stack[i]), 0, 16);

        // Get symbol information
        DWORD64 displacement = 0;
        if (SymFromAddr(s_process, reinterpret_cast<DWORD64>(stack[i]),
                        &displacement, symbol)) {
            frame.functionName = QString::fromUtf8(symbol->Name);
        }

        // Get line information
        DWORD lineDisplacement = 0;
        if (SymGetLineFromAddr64(s_process, reinterpret_cast<DWORD64>(stack[i]),
                                 &lineDisplacement, &line)) {
            frame.fileName = QString::fromUtf8(line.FileName);
            frame.lineNumber = line.LineNumber;
        }

        // Get module information
        HMODULE hModule = nullptr;
        if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                  GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              reinterpret_cast<LPCTSTR>(stack[i]), &hModule)) {
            char moduleName[MAX_PATH];
            if (GetModuleFileNameA(hModule, moduleName, MAX_PATH)) {
                frame.moduleName = QString::fromUtf8(moduleName);
                // Extract just the filename
                int lastSlash = frame.moduleName.lastIndexOf('\\');
                if (lastSlash >= 0) {
                    frame.moduleName = frame.moduleName.mid(lastSlash + 1);
                }
            }
        }

        frames.push_back(frame);
    }

#elif defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    // Unix implementation using backtrace
    std::vector<void*> stack(maxFrames + skipFrames);
    int capturedFrames = backtrace(stack.data(), maxFrames + skipFrames);

    char** symbols = backtrace_symbols(stack.data(), capturedFrames);

    for (int i = skipFrames; i < capturedFrames; ++i) {
        StackFrame frame;
        frame.address = stack[i];
        frame.addressString =
            QString("0x%1").arg(reinterpret_cast<quintptr>(stack[i]), 0, 16);

        if (symbols && symbols[i]) {
            QString symbolStr = QString::fromUtf8(symbols[i]);

            // Parse symbol string (format varies by platform)
            // Linux: module(function+offset) [address]
            // macOS: index module address function + offset

            Dl_info info;
            if (dladdr(stack[i], &info)) {
                if (info.dli_sname) {
                    frame.functionName =
                        demangleSymbol(QString::fromUtf8(info.dli_sname));
                }
                if (info.dli_fname) {
                    frame.moduleName = QString::fromUtf8(info.dli_fname);
                    int lastSlash = frame.moduleName.lastIndexOf('/');
                    if (lastSlash >= 0) {
                        frame.moduleName = frame.moduleName.mid(lastSlash + 1);
                    }
                }
            }

            // If we couldn't get function name from dladdr, try parsing the
            // symbol string
            if (frame.functionName.isEmpty()) {
                frame.functionName = symbolStr;
            }
        }

        frames.push_back(frame);
    }

    if (symbols) {
        free(symbols);
    }
#endif

    return frames;
}

QString formatStackTrace(const std::vector<StackFrame>& frames,
                         bool includeAddresses) {
    QString result;

    for (size_t i = 0; i < frames.size(); ++i) {
        result += QString("#%1  ").arg(i, 2, 10, QChar('0'));
        result += frames[i].toString();
        result += "\n";
    }

    return result;
}

QString captureAndFormatStackTrace(int maxFrames, int skipFrames,
                                   bool includeAddresses) {
    auto frames = captureStackTrace(
        maxFrames, skipFrames + 1);  // +1 to skip this function
    return formatStackTrace(frames, includeAddresses);
}

}  // namespace StackTraceUtils
