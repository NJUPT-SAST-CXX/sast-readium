#include "QtSpdlogBridge.h"
#include "Logger.h"
#include <QObject>
#include <QCoreApplication>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>
#include <QLoggingCategory>
#include <QTextStream>
#include <QRect>
#include <QSize>
#include <QPoint>

// Note: Implementation classes are defined in the header for PIMPL pattern

QtSpdlogBridge& QtSpdlogBridge::instance()
{
    static QtSpdlogBridge instance;
    return instance;
}

// Constructor implementation for PIMPL
QtSpdlogBridge::QtSpdlogBridge() : d(std::make_unique<Implementation>(this))
{
}

bool QtSpdlogBridge::isMessageHandlerInstalled() const
{
    return d->handlerInstalled;
}

void QtSpdlogBridge::initialize()
{
    // Install our custom message handler to redirect Qt logging to spdlog
    installMessageHandler();
    
    // Add some default category mappings
    addCategoryMapping("qt", "qt");
    addCategoryMapping("default", "qt.default");
}

void QtSpdlogBridge::installMessageHandler()
{
    if (d->handlerInstalled) {
        return;
    }

    d->previousHandler = qInstallMessageHandler(qtMessageHandler);
    d->handlerInstalled = true;
}

void QtSpdlogBridge::restoreDefaultMessageHandler()
{
    if (!d->handlerInstalled) {
        return;
    }

    qInstallMessageHandler(d->previousHandler);
    d->handlerInstalled = false;
    d->previousHandler = nullptr;
}

void QtSpdlogBridge::qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    QtSpdlogBridge::instance().handleQtMessage(type, context, message);
}

void QtSpdlogBridge::handleQtMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    Logger::LogLevel level = d->qtMsgTypeToLogLevel(type);
    QString formattedMessage = d->formatQtMessage(type, context, message);

    // Get the logger instance and log the message
    Logger& logger = Logger::instance();
    
    switch (level) {
        case Logger::LogLevel::Debug:
            logger.debug(formattedMessage);
            break;
        case Logger::LogLevel::Info:
            logger.info(formattedMessage);
            break;
        case Logger::LogLevel::Warning:
            logger.warning(formattedMessage);
            break;
        case Logger::LogLevel::Error:
        case Logger::LogLevel::Critical:
            logger.error(formattedMessage);
            break;
        default:
            logger.info(formattedMessage);
            break;
    }
}

Logger::LogLevel QtSpdlogBridge::Implementation::qtMsgTypeToLogLevel(QtMsgType type) const
{
    switch (type) {
        case QtDebugMsg:
            return Logger::LogLevel::Debug;
        case QtInfoMsg:
            return Logger::LogLevel::Info;
        case QtWarningMsg:
            return Logger::LogLevel::Warning;
        case QtCriticalMsg:
            return Logger::LogLevel::Error;
        case QtFatalMsg:
            return Logger::LogLevel::Critical;
        default:
            return Logger::LogLevel::Info;
    }
}

QString QtSpdlogBridge::Implementation::formatQtMessage(QtMsgType type, const QMessageLogContext& context, const QString& message) const
{
    QString formatted = message;
    
    // Add context information if available
    if (context.category && strlen(context.category) > 0 && strcmp(context.category, "default") != 0) {
        formatted = QString("[%1] %2").arg(context.category, message);
    }
    
    // Add file/line information in debug builds
#ifdef QT_DEBUG
    if (context.file && context.line > 0) {
        QString filename = QString(context.file).split('/').last().split('\\').last();
        formatted += QString(" (%1:%2)").arg(filename).arg(context.line);
    }
#endif
    
    return formatted;
}

void QtSpdlogBridge::setQtCategoryFilteringEnabled(bool enabled)
{
    d->categoryFilteringEnabled = enabled;
}

void QtSpdlogBridge::addCategoryMapping(const QString& category, const QString& spdlogLogger)
{
    d->categoryMappings[category] = spdlogLogger.isEmpty() ? category : spdlogLogger;
}

void QtSpdlogBridge::removeCategoryMapping(const QString& category)
{
    d->categoryMappings.remove(category);
}

// SpdlogQDebug implementation
SpdlogQDebug::SpdlogQDebug(Logger::LogLevel level)
    : d(std::make_unique<Implementation>(level))
{
}

SpdlogQDebug::SpdlogQDebug(const SpdlogQDebug& other)
    : d(std::make_unique<Implementation>(other.d->level))
{
    d->buffer = other.d->buffer;
    d->messageOutput = false;
}

SpdlogQDebug::~SpdlogQDebug()
{
    if (d->messageOutput && !d->buffer.isEmpty()) {
        Logger& logger = Logger::instance();

        switch (d->level) {
            case Logger::LogLevel::Debug:
                logger.debug(d->buffer);
                break;
            case Logger::LogLevel::Info:
                logger.info(d->buffer);
                break;
            case Logger::LogLevel::Warning:
                logger.warning(d->buffer);
                break;
            case Logger::LogLevel::Error:
                logger.error(d->buffer);
                break;
            case Logger::LogLevel::Critical:
                logger.critical(d->buffer);
                break;
            default:
                logger.info(d->buffer);
                break;
        }
    }
}

SpdlogQDebug& SpdlogQDebug::operator<<(const QString& string)
{
    d->stream << string;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(const char* string)
{
    d->stream << string;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(const QByteArray& array)
{
    d->stream << array;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(int number)
{
    d->stream << number;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(long number)
{
    d->stream << number;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(long long number)
{
    d->stream << number;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(unsigned int number)
{
    d->stream << number;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(unsigned long number)
{
    d->stream << number;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(unsigned long long number)
{
    d->stream << number;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(float number)
{
    d->stream << number;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(double number)
{
    d->stream << number;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(bool boolean)
{
    d->stream << (boolean ? "true" : "false");
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(const void* pointer)
{
    d->stream << pointer;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(const QObject* object)
{
    if (object) {
        d->stream << object->objectName() << "(" << object->metaObject()->className() << ")";
    } else {
        d->stream << "QObject(nullptr)";
    }
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(const QRect& rect)
{
    d->stream << "QRect(" << rect.x() << "," << rect.y() << " " << rect.width() << "x" << rect.height() << ")";
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(const QSize& size)
{
    d->stream << "QSize(" << size.width() << ", " << size.height() << ")";
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(const QPoint& point)
{
    d->stream << "QPoint(" << point.x() << "," << point.y() << ")";
    return *this;
}

// Convenience functions
SpdlogQDebug spdlogDebug()
{
    return SpdlogQDebug(Logger::LogLevel::Debug);
}

SpdlogQDebug spdlogInfo()
{
    return SpdlogQDebug(Logger::LogLevel::Info);
}

SpdlogQDebug spdlogWarning()
{
    return SpdlogQDebug(Logger::LogLevel::Warning);
}

SpdlogQDebug spdlogCritical()
{
    return SpdlogQDebug(Logger::LogLevel::Critical);
}

// SpdlogLoggingCategory implementation
SpdlogLoggingCategory::SpdlogLoggingCategory(const char* category)
    : m_categoryName(QString::fromLatin1(category)), m_enabledLevel(Logger::LogLevel::Debug)
{
}

bool SpdlogLoggingCategory::isDebugEnabled() const
{
    return m_enabledLevel <= Logger::LogLevel::Debug;
}

bool SpdlogLoggingCategory::isInfoEnabled() const
{
    return m_enabledLevel <= Logger::LogLevel::Info;
}

bool SpdlogLoggingCategory::isWarningEnabled() const
{
    return m_enabledLevel <= Logger::LogLevel::Warning;
}

bool SpdlogLoggingCategory::isCriticalEnabled() const
{
    return m_enabledLevel <= Logger::LogLevel::Critical;
}

SpdlogQDebug SpdlogLoggingCategory::debug() const
{
    SpdlogQDebug debug(Logger::LogLevel::Debug);
    debug << "[" << m_categoryName << "] ";
    return debug;
}

SpdlogQDebug SpdlogLoggingCategory::info() const
{
    SpdlogQDebug info(Logger::LogLevel::Info);
    info << "[" << m_categoryName << "] ";
    return info;
}

SpdlogQDebug SpdlogLoggingCategory::warning() const
{
    SpdlogQDebug warning(Logger::LogLevel::Warning);
    warning << "[" << m_categoryName << "] ";
    return warning;
}

SpdlogQDebug SpdlogLoggingCategory::critical() const
{
    SpdlogQDebug critical(Logger::LogLevel::Critical);
    critical << "[" << m_categoryName << "] ";
    return critical;
}
