#include "BackgroundProcessor.h"
#include <QtConcurrent>
#include <QDebug>
#include <QMutexLocker>

class BackgroundProcessor::Implementation
{
public:
    Implementation(BackgroundProcessor* q)
        : q_ptr(q)
    {
        threadPool.setMaxThreadCount(QThread::idealThreadCount());
        
        // Connect to monitor when all tasks finish
        connect(&threadPool, &QThreadPool::destroyed, [this]() {
            emit q_ptr->allTasksFinished();
        });
    }

    ~Implementation()
    {
        cancelAllTasks();
        threadPool.waitForDone(5000);
    }

    void cancelAllTasks()
    {
        QMutexLocker locker(&futuresMutex);
        for (auto& watcher : activeWatchers) {
            if (watcher && !watcher->isFinished()) {
                watcher->cancel();
            }
        }
        activeWatchers.clear();
    }

    void addWatcher(QFutureWatcherBase* watcher)
    {
        QMutexLocker locker(&futuresMutex);
        activeWatchers.append(watcher);
        
        connect(watcher, &QFutureWatcherBase::finished, [this, watcher]() {
            removeWatcher(watcher);
            emit q_ptr->taskFinished();
            checkAllTasksFinished();
        });
    }

    void removeWatcher(QFutureWatcherBase* watcher)
    {
        QMutexLocker locker(&futuresMutex);
        activeWatchers.removeAll(watcher);
        watcher->deleteLater();
    }

    void checkAllTasksFinished()
    {
        QMutexLocker locker(&futuresMutex);
        if (activeWatchers.isEmpty() && threadPool.activeThreadCount() == 0) {
            emit q_ptr->allTasksFinished();
        }
    }

    BackgroundProcessor* q_ptr;
    QThreadPool threadPool;
    QList<QFutureWatcherBase*> activeWatchers;
    QMutex futuresMutex;
    QThread::Priority threadPriority = QThread::NormalPriority;
};

BackgroundProcessor::BackgroundProcessor(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Implementation>(this))
{
}

BackgroundProcessor::~BackgroundProcessor() = default;

void BackgroundProcessor::setMaxThreadCount(int count)
{
    d->threadPool.setMaxThreadCount(count);
}

int BackgroundProcessor::maxThreadCount() const
{
    return d->threadPool.maxThreadCount();
}

void BackgroundProcessor::setThreadPriority(QThread::Priority priority)
{
    d->threadPriority = priority;
}

void BackgroundProcessor::executeAsync(std::function<void()> task)
{
    emit taskStarted();
    
    auto* watcher = new QFutureWatcher<void>(this);
    d->addWatcher(watcher);
    
    QFuture<void> future = QtConcurrent::run(&d->threadPool, task);
    watcher->setFuture(future);
}

void BackgroundProcessor::executeBatch(const QList<std::function<void()>>& tasks)
{
    int total = tasks.size();
    int completed = 0;
    
    for (const auto& task : tasks) {
        executeAsync([this, task, &completed, total]() {
            task();
            completed++;
            emit progressUpdate(completed, total);
        });
    }
}

void BackgroundProcessor::cancelAll()
{
    d->cancelAllTasks();
}

void BackgroundProcessor::waitForDone(int msecs)
{
    d->threadPool.waitForDone(msecs);
}

bool BackgroundProcessor::isIdle() const
{
    return d->threadPool.activeThreadCount() == 0 && d->activeWatchers.isEmpty();
}

int BackgroundProcessor::activeThreadCount() const
{
    return d->threadPool.activeThreadCount();
}
