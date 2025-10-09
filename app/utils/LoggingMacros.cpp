#include "LoggingMacros.h"
#include <mach/mach.h>
#include <mach/task.h>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QProcess>
#include <QThread>
#include <chrono>

#ifdef Q_OS_WIN
#include <psapi.h>
#include <windows.h>
#endif

// Static member initialization
QHash<QString, qint64> MemoryLogger::s_memoryBaselines;

// ============================================================================
// PerformanceLogger Implementation
// ============================================================================

PerformanceLogger::PerformanceLogger(const QString& name, const char* file,
                                     int line)
    : m_name(name), m_startTime(std::chrono::high_resolution_clock::now()) {
    if (file && line > 0) {
        QFileInfo fileInfo(file);
        m_location = QString("%1:%2").arg(fileInfo.fileName()).arg(line);
    }

    LOG_TRACE("Performance tracking started: {}{}", m_name.toStdString(),
              m_location.isEmpty() ? "" : " at " + m_location.toStdString());
}

PerformanceLogger::~PerformanceLogger() {
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        endTime - m_startTime)
                        .count();

    // Only log if duration exceeds threshold (if set)
    if (m_thresholdMs == 0 || duration >= m_thresholdMs) {
        QString message =
            QString("Performance [%1]: %2ms").arg(m_name).arg(duration);

        if (!m_location.isEmpty()) {
            message += QString(" at %1").arg(m_location);
        }

        // Use different log levels based on duration
        if (duration > 1000) {  // > 1 second
            LOG_WARNING("{}", message.toStdString());
        } else if (duration > 100) {  // > 100ms
            LOG_INFO("{}", message.toStdString());
        } else {
            LOG_DEBUG("{}", message.toStdString());
        }
    }
}

void PerformanceLogger::checkpoint(const QString& description) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        currentTime - m_startTime)
                        .count();

    QString message =
        QString("Performance checkpoint [%1]: %2ms").arg(m_name).arg(duration);
    if (!description.isEmpty()) {
        message += QString(" - %1").arg(description);
    }

    LOG_DEBUG("{}", message.toStdString());
}

// ============================================================================
// ScopedLogLevel Implementation
// ============================================================================

ScopedLogLevel::ScopedLogLevel(Logger::LogLevel tempLevel)
    : m_originalLevel(
          LoggingManager::instance().getConfiguration().globalLogLevel) {
    LoggingManager::instance().setGlobalLogLevel(tempLevel);
}

ScopedLogLevel::~ScopedLogLevel() {
    LoggingManager::instance().setGlobalLogLevel(m_originalLevel);
}

// ============================================================================
// MemoryLogger Implementation
// ============================================================================

void MemoryLogger::logCurrentUsage(const QString& context) {
    qint64 currentUsage = getCurrentMemoryUsage();
    if (currentUsage > 0) {
        QString message =
            QString("Memory usage: %1 MB").arg(currentUsage / (1024 * 1024));
        if (!context.isEmpty()) {
            message = QString("[%1] %2").arg(context, message);
        }
        LOG_INFO("{}", message.toStdString());
    } else {
        LOG_WARNING("Failed to retrieve memory usage information");
    }
}

void MemoryLogger::logMemoryDelta(const QString& context) {
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    qint64 currentUsage = getCurrentMemoryUsage();
    if (currentUsage <= 0) {
        LOG_WARNING("Failed to retrieve memory usage for delta calculation");
        return;
    }

    QString key = context.isEmpty() ? "default" : context;

    if (s_memoryBaselines.contains(key)) {
        qint64 baseline = s_memoryBaselines[key];
        qint64 delta = currentUsage - baseline;

        QString message =
            QString("Memory delta: %1%2 MB (current: %3 MB, baseline: %4 MB)")
                .arg(delta >= 0 ? "+" : "")
                .arg(delta / (1024 * 1024))
                .arg(currentUsage / (1024 * 1024))
                .arg(baseline / (1024 * 1024));

        if (!context.isEmpty()) {
            message = QString("[%1] %2").arg(context, message);
        }

        if (delta > 10 * 1024 * 1024) {  // > 10MB increase
            LOG_WARNING("{}", message.toStdString());
        } else if (delta > 1024 * 1024) {  // > 1MB increase
            LOG_INFO("{}", message.toStdString());
        } else {
            LOG_DEBUG("{}", message.toStdString());
        }
    } else {
        LOG_DEBUG(
            "No baseline found for memory delta calculation in context: {}",
            context.isEmpty() ? "default" : context.toStdString());
    }
}

void MemoryLogger::startMemoryTracking(const QString& context) {
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    qint64 currentUsage = getCurrentMemoryUsage();
    if (currentUsage > 0) {
        QString key = context.isEmpty() ? "default" : context;
        s_memoryBaselines[key] = currentUsage;

        LOG_DEBUG("Memory tracking started for context '{}': baseline {} MB",
                  key.toStdString(), currentUsage / (1024 * 1024));
    } else {
        LOG_WARNING(
            "Failed to start memory tracking - could not retrieve current "
            "usage");
    }
}

void MemoryLogger::endMemoryTracking(const QString& context) {
    logMemoryDelta(context);

    static QMutex mutex;
    QMutexLocker locker(&mutex);

    QString key = context.isEmpty() ? "default" : context;
    if (s_memoryBaselines.remove(key)) {
        LOG_DEBUG("Memory tracking ended for context '{}'", key.toStdString());
    } else {
        LOG_WARNING("Attempted to end memory tracking for unknown context '{}'",
                    key.toStdString());
    }
}

qint64 MemoryLogger::getCurrentMemoryUsage() {
#ifdef Q_OS_WIN
    // Windows implementation using GetProcessMemoryInfo
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return static_cast<qint64>(pmc.WorkingSetSize);
    }
#elif defined(Q_OS_LINUX)
    // Linux implementation using /proc/self/status
    QFile file("/proc/self/status");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        QString line;
        while (stream.readLineInto(&line)) {
            if (line.startsWith("VmRSS:")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() >= 2) {
                    bool ok;
                    qint64 kb = parts[1].toLongLong(&ok);
                    if (ok) {
                        return kb * 1024;  // Convert KB to bytes
                    }
                }
                break;
            }
        }
    }
#elif defined(Q_OS_MACOS)
    // macOS implementation using task_info
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info,
                  &infoCount) == KERN_SUCCESS) {
        return static_cast<qint64>(info.resident_size);
    }
#endif

    // Fallback: try to use QProcess to get memory info
    QProcess process;
    process.start("ps",
                  QStringList()
                      << "-o"
                      << "rss="
                      << "-p"
                      << QString::number(QCoreApplication::applicationPid()));
    if (process.waitForFinished(1000)) {
        QString output = process.readAllStandardOutput().trimmed();
        bool ok;
        qint64 kb = output.toLongLong(&ok);
        if (ok) {
            return kb * 1024;  // Convert KB to bytes
        }
    }

    return -1;  // Failed to get memory usage
}

// ============================================================================
// Additional Utility Functions
// ============================================================================

namespace LoggingUtils {

/**
 * @brief Format Qt objects for logging
 */
QString formatQtObject(const QObject* obj) {
    if (!obj) {
        return "QObject(nullptr)";
    }

    QString name = obj->objectName();
    QString className = obj->metaObject()->className();

    if (name.isEmpty()) {
        return QString("%1(unnamed)").arg(className);
    } else {
        return QString("%1(\"%2\")").arg(className, name);
    }
}

/**
 * @brief Format QRect for logging
 */
QString formatQRect(const QRect& rect) {
    return QString("QRect(%1,%2 %3x%4)")
        .arg(rect.x())
        .arg(rect.y())
        .arg(rect.width())
        .arg(rect.height());
}

/**
 * @brief Format QSize for logging
 */
QString formatQSize(const QSize& size) {
    return QString("QSize(%1x%2)").arg(size.width()).arg(size.height());
}

/**
 * @brief Format QPoint for logging
 */
QString formatQPoint(const QPoint& point) {
    return QString("QPoint(%1,%2)").arg(point.x()).arg(point.y());
}

/**
 * @brief Get current thread information for logging
 */
QString getCurrentThreadInfo() {
    QThread* currentThread = QThread::currentThread();
    QString threadName = currentThread->objectName();

    if (threadName.isEmpty()) {
        return QString("Thread(0x%1)")
            .arg(reinterpret_cast<quintptr>(currentThread), 0, 16);
    } else {
        return QString("Thread(\"%1\")").arg(threadName);
    }
}

/**
 * @brief Create a separator line for log readability
 */
void logSeparator(const QString& title, char separator) {
    QString line(60, separator);
    if (!title.isEmpty()) {
        QString centeredTitle = QString(" %1 ").arg(title);
        int pos = (line.length() - centeredTitle.length()) / 2;
        if (pos > 0) {
            line.replace(pos, centeredTitle.length(), centeredTitle);
        }
    }
    LOG_INFO("{}", line.toStdString());
}

}  // namespace LoggingUtils

// ============================================================================
// Platform-specific includes for memory usage
// ============================================================================

#ifdef Q_OS_WIN
#include <psapi.h>
#include <windows.h>
#elif defined(Q_OS_MACOS)
#include <mach/mach.h>
#include <mach/mach_init.h>
#include <mach/task.h>
#elif defined(Q_OS_LINUX)
#include <QRegularExpression>
#include <QTextStream>
#endif
