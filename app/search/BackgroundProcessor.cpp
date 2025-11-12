#include "BackgroundProcessor.h"
#include <QDebug>
#include <QMutexLocker>
#include <QtConcurrent>
#include <atomic>
#include <memory>

class BackgroundProcessor::Implementation {
public:
    Implementation(BackgroundProcessor* q) : q_ptr(q) {
        threadPool.setMaxThreadCount(QThread::idealThreadCount());
        // Note: we intentionally avoid connecting to QThreadPool::destroyed
        // here, as emitting signals during object destruction can cause
        // reentrancy issues in tests. We instead emit allTasksFinished based on
        // active watchers/state.
    }

    ~Implementation() {
        qInfo() << "BackgroundProcessor::Implementation dtor - begin";
        // Stop accepting callbacks and cancel queued tasks
        cancelAllTasks();
        qInfo() << "BackgroundProcessor::Implementation dtor - waiting for "
                   "threadPool";
        threadPool.waitForDone(5000);

        // Ensure all watchers are destroyed deterministically to avoid
        // deleteLater() during shutdown when the event loop is gone
        QList<QFutureWatcherBase*> toDelete;
        {
            QMutexLocker locker(&futuresMutex);
            toDelete = activeWatchers;
            activeWatchers.clear();
        }
        for (auto* watcher : toDelete) {
            if (!watcher)
                continue;
            watcher->disconnect();
            // Best-effort cancel; tasks should already be done by now
            if (!watcher->isFinished()) {
                watcher->cancel();
            }
            delete watcher;  // direct delete is safe here
        }
        qInfo() << "BackgroundProcessor::Implementation dtor - end";
    }

    void cancelAllTasks() {
        QMutexLocker locker(&futuresMutex);
        for (auto& watcher : activeWatchers) {
            if (watcher && !watcher->isFinished()) {
                watcher->cancel();
            }
        }
        activeWatchers.clear();
    }

    void addWatcher(QFutureWatcherBase* watcher) {
        QMutexLocker locker(&futuresMutex);
        activeWatchers.append(watcher);

        connect(watcher, &QFutureWatcherBase::finished, [this, watcher]() {
            qInfo() << "BackgroundProcessor: watcher finished";
            qInfo() << "BackgroundProcessor: before removeWatcher";
            removeWatcher(watcher);
            qInfo() << "BackgroundProcessor: after removeWatcher";
            emit q_ptr->taskFinished();
            qInfo() << "BackgroundProcessor: after emit taskFinished";
            checkAllTasksFinished();
            qInfo() << "BackgroundProcessor: after checkAllTasksFinished";
        });
    }

    void removeWatcher(QFutureWatcherBase* watcher) {
        qInfo() << "removeWatcher: begin" << watcher;
        QMutexLocker locker(&futuresMutex);
        qInfo() << "removeWatcher: locked";
        activeWatchers.removeAll(watcher);
        qInfo() << "removeWatcher: removed from list, closingDown="
                << QCoreApplication::closingDown();
        // Avoid deleteLater() which can crash if the event loop is shutting
        // down
        if (QCoreApplication::closingDown()) {
            locker.unlock();
            watcher->disconnect();
            qInfo() << "removeWatcher: deleting watcher directly";
            delete watcher;
        } else {
            locker.unlock();
            qInfo() << "removeWatcher: scheduling deleteLater";
            watcher->deleteLater();
        }
        qInfo() << "removeWatcher: end";
    }

    void checkAllTasksFinished() {
        qInfo() << "checkAllTasksFinished: begin";
        QMutexLocker locker(&futuresMutex);
        bool none =
            activeWatchers.isEmpty() && threadPool.activeThreadCount() == 0;
        qInfo() << "checkAllTasksFinished: none=" << none
                << ", activeWatchers=" << activeWatchers.size()
                << ", activeThreads=" << threadPool.activeThreadCount();
        if (none) {
            emit q_ptr->allTasksFinished();
        }
        qInfo() << "checkAllTasksFinished: end";
    }

    BackgroundProcessor* q_ptr;
    QThreadPool threadPool;
    QList<QFutureWatcherBase*> activeWatchers;
    QMutex futuresMutex;
    QThread::Priority threadPriority = QThread::NormalPriority;
};

BackgroundProcessor::BackgroundProcessor(QObject* parent)
    : QObject(parent), d(std::make_unique<Implementation>(this)) {}

BackgroundProcessor::~BackgroundProcessor() = default;

QThreadPool& BackgroundProcessor::threadPool() const { return d->threadPool; }

void BackgroundProcessor::setMaxThreadCount(int count) {
    d->threadPool.setMaxThreadCount(count);
}

int BackgroundProcessor::maxThreadCount() const {
    return d->threadPool.maxThreadCount();
}

void BackgroundProcessor::setThreadPriority(QThread::Priority priority) {
    d->threadPriority = priority;
}

void BackgroundProcessor::executeAsync(std::function<void()> task) {
    qInfo() << "BackgroundProcessor::executeAsync: scheduling task";
    emit taskStarted();

    // Create watcher without parent - will be managed manually via
    // deleteLater()
    auto* watcher = new QFutureWatcher<void>();
    d->addWatcher(watcher);

    QFuture<void> future = QtConcurrent::run(&d->threadPool, std::move(task));
    qInfo() << "BackgroundProcessor::executeAsync: future started?"
            << future.isStarted();
    watcher->setFuture(future);
    qInfo() << "BackgroundProcessor::executeAsync: future set";
}

void BackgroundProcessor::executeBatch(
    const QList<std::function<void()>>& tasks) {
    int total = tasks.size();
    // Use shared_ptr to safely share counter across async tasks
    auto completed = std::make_shared<std::atomic<int>>(0);

    for (const auto& task : tasks) {
        executeAsync([this, task, completed, total]() {
            task();
            int current = ++(*completed);
            emit progressUpdate(current, total);
        });
    }
}

void BackgroundProcessor::cancelAll() { d->cancelAllTasks(); }

void BackgroundProcessor::waitForDone(int msecs) {
    qInfo() << "BackgroundProcessor::waitForDone(" << msecs << ")";
    d->threadPool.waitForDone(msecs);
    qInfo() << "BackgroundProcessor::waitForDone - done";
}

bool BackgroundProcessor::isIdle() const {
    return d->threadPool.activeThreadCount() == 0 &&
           d->activeWatchers.isEmpty();
}

int BackgroundProcessor::activeThreadCount() const {
    return d->threadPool.activeThreadCount();
}
