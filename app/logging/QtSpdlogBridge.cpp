#include "QtSpdlogBridge.h"
#include "Logger.h"
#include <QCoreApplication>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>
#include <QLoggingCategory>

QtSpdlogBridge& QtSpdlogBridge::instance()
{
    static QtSpdlogBridge instance;
    return instance;
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
    if (m_handlerInstalled) {
        return;
    }
    
    m_previousHandler = qInstallMessageHandler(qtMessageHandler);
    m_handlerInstalled = true;
}

void QtSpdlogBridge::restoreDefaultMessageHandler()
{
    if (!m_handlerInstalled) {
        return;
    }
    
    qInstallMessageHandler(m_previousHandler);
    m_handlerInstalled = false;
    m_previousHandler = nullptr;
}

void QtSpdlogBridge::qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    QtSpdlogBridge::instance().handleQtMessage(type, context, message);
}

void QtSpdlogBridge::handleQtMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    Logger::LogLevel level = qtMsgTypeToLogLevel(type);
    QString formattedMessage = formatQtMessage(type, context, message);
    
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

Logger::LogLevel QtSpdlogBridge::qtMsgTypeToLogLevel(QtMsgType type) const
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

QString QtSpdlogBridge::formatQtMessage(QtMsgType type, const QMessageLogContext& context, const QString& message) const
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
    m_categoryFilteringEnabled = enabled;
}

void QtSpdlogBridge::addCategoryMapping(const QString& category, const QString& spdlogLogger)
{
    m_categoryMappings[category] = spdlogLogger.isEmpty() ? category : spdlogLogger;
}

void QtSpdlogBridge::removeCategoryMapping(const QString& category)
{
    m_categoryMappings.remove(category);
}

// SpdlogQDebug implementation
SpdlogQDebug::SpdlogQDebug(Logger::LogLevel level)
    : m_level(level), m_stream(&m_buffer)
{
}

SpdlogQDebug::SpdlogQDebug(const SpdlogQDebug& other)
    : m_level(other.m_level), m_stream(&m_buffer), m_messageOutput(false)
{
    m_buffer = other.m_buffer;
}

SpdlogQDebug::~SpdlogQDebug()
{
    if (m_messageOutput && !m_buffer.isEmpty()) {
        Logger& logger = Logger::instance();
        
        switch (m_level) {
            case Logger::LogLevel::Debug:
                logger.debug(m_buffer);
                break;
            case Logger::LogLevel::Info:
                logger.info(m_buffer);
                break;
            case Logger::LogLevel::Warning:
                logger.warning(m_buffer);
                break;
            case Logger::LogLevel::Error:
                logger.error(m_buffer);
                break;
            case Logger::LogLevel::Critical:
                logger.critical(m_buffer);
                break;
            default:
                logger.info(m_buffer);
                break;
        }
    }
}

SpdlogQDebug& SpdlogQDebug::operator<<(const QString& string)
{
    m_stream << string;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(const char* string)
{
    m_stream << string;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(const QByteArray& array)
{
    m_stream << array;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(int number)
{
    m_stream << number;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(long number)
{
    m_stream << number;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(long long number)
{
    m_stream << number;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(unsigned int number)
{
    m_stream << number;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(unsigned long number)
{
    m_stream << number;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(unsigned long long number)
{
    m_stream << number;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(float number)
{
    m_stream << number;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(double number)
{
    m_stream << number;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(bool boolean)
{
    m_stream << (boolean ? "true" : "false");
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(const void* pointer)
{
    m_stream << pointer;
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(const QObject* object)
{
    if (object) {
        m_stream << object->objectName() << "(" << object->metaObject()->className() << ")";
    } else {
        m_stream << "QObject(nullptr)";
    }
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(const QRect& rect)
{
    m_stream << "QRect(" << rect.x() << "," << rect.y() << " " << rect.width() << "x" << rect.height() << ")";
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(const QSize& size)
{
    m_stream << "QSize(" << size.width() << ", " << size.height() << ")";
    return *this;
}

SpdlogQDebug& SpdlogQDebug::operator<<(const QPoint& point)
{
    m_stream << "QPoint(" << point.x() << "," << point.y() << ")";
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
