#pragma once

#include <QTest>
#include <QSignalSpy>
#include <QTimer>
#include <QRandomGenerator>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QVariantMap>
#include <QDateTime>
#include <QCoreApplication>
#include <QEventLoop>
#include <memory>
#include <functional>

/**
 * @brief TestBase - Base class for all test cases
 * 
 * Provides common functionality for test setup and teardown
 */
class TestBase : public QObject {
    Q_OBJECT
    
protected:
    // Setup and teardown
    virtual void initTestCase() {}
    virtual void cleanupTestCase() {}
    virtual void init() {}
    virtual void cleanup() {}
    
    // Helper methods
    bool waitFor(std::function<bool()> condition, int timeout = 5000) {
        QTimer timer;
        timer.setSingleShot(true);
        timer.start(timeout);
        
        while (timer.isActive() && !condition()) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        }
        
        return condition();
    }
    
    void waitMs(int ms) {
        QTimer timer;
        timer.setSingleShot(true);
        timer.start(ms);
        
        while (timer.isActive()) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        }
    }
    
    template<typename T>
    bool waitForSignal(T* object, const char* signal, int timeout = 5000) {
        QSignalSpy spy(object, signal);
        return spy.wait(timeout);
    }
};

/**
 * @brief MockObject - Base class for mock objects
 */
class MockObject : public QObject {
    Q_OBJECT
    
public:
    struct Call {
        QString method;
        QVariantList args;
        QDateTime timestamp;
    };
    
    void recordCall(const QString& method, const QVariantList& args = QVariantList()) {
        Call call;
        call.method = method;
        call.args = args;
        call.timestamp = QDateTime::currentDateTime();
        m_calls.append(call);
    }
    
    bool wasMethodCalled(const QString& method) const {
        for (const auto& call : m_calls) {
            if (call.method == method) {
                return true;
            }
        }
        return false;
    }
    
    int methodCallCount(const QString& method) const {
        int count = 0;
        for (const auto& call : m_calls) {
            if (call.method == method) {
                count++;
            }
        }
        return count;
    }
    
    QVariantList lastCallArgs(const QString& method) const {
        for (auto it = m_calls.rbegin(); it != m_calls.rend(); ++it) {
            if (it->method == method) {
                return it->args;
            }
        }
        return QVariantList();
    }
    
    void clearCalls() {
        m_calls.clear();
    }
    
    QList<Call> calls() const { return m_calls; }
    
private:
    QList<Call> m_calls;
};

/**
 * @brief Test macros for common assertions
 */
#define QVERIFY_TIMEOUT(condition, timeout) \
    QVERIFY(waitFor([&](){ return (condition); }, timeout))

#define QVERIFY_SIGNAL(object, signal, timeout) \
    QVERIFY(waitForSignal(object, signal, timeout))

#define QCOMPARE_TIMEOUT(actual, expected, timeout) \
    QVERIFY(waitFor([&](){ return (actual) == (expected); }, timeout))

#define QVERIFY_EXCEPTION(expression, exceptionType) \
    do { \
        bool caughtExpected = false; \
        try { \
            expression; \
        } catch (const exceptionType&) { \
            caughtExpected = true; \
        } catch (...) { \
        } \
        QVERIFY(caughtExpected); \
    } while(0)

#define QVERIFY_NO_EXCEPTION(expression) \
    do { \
        bool caughtException = false; \
        try { \
            expression; \
        } catch (...) { \
            caughtException = true; \
        } \
        QVERIFY(!caughtException); \
    } while(0)

/**
 * @brief ScopedCleanup - RAII helper for test cleanup
 */
class ScopedCleanup {
public:
    explicit ScopedCleanup(std::function<void()> cleanup)
        : m_cleanup(cleanup) {}
    
    ~ScopedCleanup() {
        if (m_cleanup) {
            m_cleanup();
        }
    }
    
    void cancel() {
        m_cleanup = nullptr;
    }
    
private:
    std::function<void()> m_cleanup;
};

/**
 * @brief TestDataGenerator - Helper for generating test data
 */
class TestDataGenerator {
public:
    static QString randomString(int length = 10) {
        const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        QString result;
        for (int i = 0; i < length; ++i) {
            result += chars[QRandomGenerator::global()->bounded(chars.length())];
        }
        return result;
    }
    
    static int randomInt(int min = 0, int max = 100) {
        return QRandomGenerator::global()->bounded(min, max);
    }
    
    static QVariantMap randomMap(int size = 5) {
        QVariantMap map;
        for (int i = 0; i < size; ++i) {
            map[randomString(5)] = QVariant(randomString(10));
        }
        return map;
    }
    
    static QJsonObject randomJsonObject(int size = 5) {
        QJsonObject obj;
        for (int i = 0; i < size; ++i) {
            obj[randomString(5)] = randomString(10);
        }
        return obj;
    }
};
