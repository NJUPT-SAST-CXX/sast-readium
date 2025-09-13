#pragma once

#include <QObject>
#include <QThreadPool>
#include <QFuture>
#include <QFutureWatcher>
#include <functional>
#include <memory>

/**
 * Background processing component
 * Manages asynchronous tasks and thread pool operations
 */
class BackgroundProcessor : public QObject
{
    Q_OBJECT

public:
    explicit BackgroundProcessor(QObject* parent = nullptr);
    ~BackgroundProcessor();

    // Configuration
    void setMaxThreadCount(int count);
    int maxThreadCount() const;
    void setThreadPriority(QThread::Priority priority);

    // Task management
    template<typename Result>
    QFuture<Result> execute(std::function<Result()> task);
    
    void executeAsync(std::function<void()> task);
    void executeBatch(const QList<std::function<void()>>& tasks);
    
    // Control
    void cancelAll();
    void waitForDone(int msecs = -1);
    bool isIdle() const;
    int activeThreadCount() const;

signals:
    void taskStarted();
    void taskFinished();
    void allTasksFinished();
    void progressUpdate(int completed, int total);

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};

// Template implementation
template<typename Result>
QFuture<Result> BackgroundProcessor::execute(std::function<Result()> task)
{
    emit taskStarted();
    return QtConcurrent::run(task);
}
