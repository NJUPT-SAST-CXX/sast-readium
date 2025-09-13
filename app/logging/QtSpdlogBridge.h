#pragma once

#include "Logger.h"
#include <QLoggingCategory>
#include <QDebug>
#include <QString>
#include <QTextStream>
#include <functional>

/**
 * @brief Bridge class that integrates Qt's logging system with spdlog
 * 
 * This class provides seamless integration between Qt's logging categories
 * and spdlog, allowing existing Qt logging code to work with the new
 * spdlog-based logging system.
 */
class QtSpdlogBridge : public QObject
{
    Q_OBJECT

public:
    static QtSpdlogBridge& instance();
    ~QtSpdlogBridge() = default;

    /**
     * @brief Initialize the Qt-spdlog bridge
     * 
     * This sets up Qt's message handler to redirect all Qt logging
     * through spdlog while maintaining compatibility with existing code.
     */
    void initialize();

    /**
     * @brief Install custom Qt message handler
     * 
     * Redirects qDebug(), qWarning(), qCritical(), etc. to spdlog
     */
    void installMessageHandler();

    /**
     * @brief Restore Qt's default message handler
     */
    void restoreDefaultMessageHandler();

    /**
     * @brief Enable/disable Qt logging category filtering
     */
    void setQtCategoryFilteringEnabled(bool enabled);

    /**
     * @brief Add Qt logging category mapping to spdlog logger
     */
    void addCategoryMapping(const QString& category, const QString& spdlogLogger = "");

    /**
     * @brief Remove Qt logging category mapping
     */
    void removeCategoryMapping(const QString& category);

    /**
     * @brief Check if Qt message handler is installed
     */
    bool isMessageHandlerInstalled() const { return m_handlerInstalled; }

private slots:
    void handleQtMessage(QtMsgType type, const QMessageLogContext& context, const QString& message);

private:
    QtSpdlogBridge() = default;
    QtSpdlogBridge(const QtSpdlogBridge&) = delete;
    QtSpdlogBridge& operator=(const QtSpdlogBridge&) = delete;

    static void qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message);
    Logger::LogLevel qtMsgTypeToLogLevel(QtMsgType type) const;
    QString formatQtMessage(QtMsgType type, const QMessageLogContext& context, const QString& message) const;

    bool m_handlerInstalled = false;
    bool m_categoryFilteringEnabled = true;
    QtMessageHandler m_previousHandler = nullptr;
    QHash<QString, QString> m_categoryMappings;
};

/**
 * @brief Custom QDebug-like class that works with spdlog
 * 
 * Provides Qt-style streaming interface while using spdlog backend
 */
class SpdlogQDebug
{
public:
    explicit SpdlogQDebug(Logger::LogLevel level);
    SpdlogQDebug(const SpdlogQDebug& other);
    ~SpdlogQDebug();

    SpdlogQDebug& operator=(const SpdlogQDebug& other) = delete;

    // Stream operators for various types
    SpdlogQDebug& operator<<(const QString& string);
    SpdlogQDebug& operator<<(const char* string);
    SpdlogQDebug& operator<<(const QByteArray& array);
    SpdlogQDebug& operator<<(int number);
    SpdlogQDebug& operator<<(long number);
    SpdlogQDebug& operator<<(long long number);
    SpdlogQDebug& operator<<(unsigned int number);
    SpdlogQDebug& operator<<(unsigned long number);
    SpdlogQDebug& operator<<(unsigned long long number);
    SpdlogQDebug& operator<<(float number);
    SpdlogQDebug& operator<<(double number);
    SpdlogQDebug& operator<<(bool boolean);
    SpdlogQDebug& operator<<(const void* pointer);

    // Qt-specific types
    SpdlogQDebug& operator<<(const QObject* object);
    SpdlogQDebug& operator<<(const QRect& rect);
    SpdlogQDebug& operator<<(const QSize& size);
    SpdlogQDebug& operator<<(const QPoint& point);

    // Function call operator for printf-style formatting
    template<typename... Args>
    void operator()(const QString& format, Args&&... args) {
        m_stream << QString::asprintf(format.toLocal8Bit().constData(), args...);
    }

private:
    Logger::LogLevel m_level;
    QTextStream m_stream;
    QString m_buffer;
    bool m_messageOutput = true;
};

// Convenience functions that return SpdlogQDebug instances
SpdlogQDebug spdlogDebug();
SpdlogQDebug spdlogInfo();
SpdlogQDebug spdlogWarning();
SpdlogQDebug spdlogCritical();

/**
 * @brief Compatibility macros for seamless Qt logging migration
 * 
 * These macros can be used to gradually migrate from Qt logging
 * to spdlog while maintaining the same API.
 */

// Option 1: Direct replacement macros (uncomment to use)
/*
#undef qDebug
#undef qInfo  
#undef qWarning
#undef qCritical

#define qDebug() spdlogDebug()
#define qInfo() spdlogInfo()
#define qWarning() spdlogWarning()
#define qCritical() spdlogCritical()
*/

// Option 2: Gradual migration macros (default)
// Note: These macros are commented out to avoid conflicts with spdlog's own macros
// Use the functions directly: spdlogDebug(), spdlogInfo(), etc.
/*
#define SPDLOG_DEBUG() spdlogDebug()
#define SPDLOG_INFO() spdlogInfo()
#define SPDLOG_WARNING() spdlogWarning()
#define SPDLOG_CRITICAL() spdlogCritical()
*/

/**
 * @brief QLoggingCategory compatibility for spdlog
 * 
 * Provides a way to use QLoggingCategory-style filtering with spdlog
 */
class SpdlogLoggingCategory
{
public:
    explicit SpdlogLoggingCategory(const char* category);
    
    bool isDebugEnabled() const;
    bool isInfoEnabled() const;
    bool isWarningEnabled() const;
    bool isCriticalEnabled() const;
    
    SpdlogQDebug debug() const;
    SpdlogQDebug info() const;
    SpdlogQDebug warning() const;
    SpdlogQDebug critical() const;
    
    const QString& categoryName() const { return m_categoryName; }

private:
    QString m_categoryName;
    Logger::LogLevel m_enabledLevel;
};

// Macro for declaring spdlog-compatible logging categories
#define Q_DECLARE_SPDLOG_CATEGORY(name) \
    extern const SpdlogLoggingCategory& name();

#define Q_SPDLOG_CATEGORY(name, string) \
    const SpdlogLoggingCategory& name() \
    { \
        static const SpdlogLoggingCategory category(string); \
        return category; \
    }

// Usage example:
// Q_DECLARE_SPDLOG_CATEGORY(lcPdfRender)
// Q_SPDLOG_CATEGORY(lcPdfRender, "pdf.render")
// 
// Then use: lcPdfRender().debug() << "PDF rendering started";
