#include "AsyncDocumentLoader.h"
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include "../logging/SimpleLogging.h"

AsyncDocumentLoader::AsyncDocumentLoader(QObject* parent)
    : QObject(parent),
      m_state(LoadingState::Idle),
      m_currentProgress(0),
      m_expectedLoadTime(0),
      m_startTime(0),
      m_workerThread(nullptr),
      m_worker(nullptr),
      m_configuredDefaultTimeout(0),
      m_configuredMinTimeout(0),
      m_configuredMaxTimeout(0),
      m_useCustomTimeoutConfig(false) {
    // 初始化进度定时器
    m_progressTimer = new QTimer(this);
    m_progressTimer->setInterval(PROGRESS_UPDATE_INTERVAL);
    connect(m_progressTimer, &QTimer::timeout, this,
            &AsyncDocumentLoader::onProgressTimerTimeout);
}

AsyncDocumentLoader::~AsyncDocumentLoader() { cancelLoading(); }

void AsyncDocumentLoader::loadDocument(const QString& filePath) {
    // Validate file path before acquiring mutex
    if (filePath.isEmpty()) {
        emit loadingFailed("文件路径为空", filePath);
        return;
    }

    if (!QFile::exists(filePath)) {
        emit loadingFailed("文件不存在", filePath);
        return;
    }

    QMutexLocker locker(&m_stateMutex);

    // 如果正在加载，先取消 (unlock mutex before calling cancelLoading)
    if (m_state == LoadingState::Loading) {
        locker.unlock();
        cancelLoading();
        locker.relock();
    }

    // 重置状态
    resetState();
    m_state = LoadingState::Loading;
    m_currentFilePath = filePath;

    locker.unlock();

    // 计算预期加载时间
    QFileInfo fileInfo(filePath);
    qint64 fileSize = fileInfo.size();
    m_expectedLoadTime = calculateExpectedLoadTime(fileSize);

    emit loadingMessageChanged(
        QString("正在加载 %1...").arg(fileInfo.fileName()));
    emit loadingProgressChanged(0);

    // 创建工作线程
    m_workerThread = new QThread(this);
    m_worker = new AsyncDocumentLoaderWorker(filePath);
    m_worker->moveToThread(m_workerThread);

    // 连接信号
    connect(m_workerThread, &QThread::started, m_worker,
            &AsyncDocumentLoaderWorker::doLoad);
    connect(m_worker, &AsyncDocumentLoaderWorker::loadCompleted, this,
            [this](Poppler::Document* document) {
                QString filePath;
                QThread* threadToCleanup = nullptr;
                AsyncDocumentLoaderWorker* workerToCleanup = nullptr;

                // DEADLOCK FIX: Minimize mutex scope for thread cleanup
                {
                    QMutexLocker locker(&m_stateMutex);
                    if (m_state == LoadingState::Loading) {
                        m_state = LoadingState::Completed;
                        filePath = m_currentFilePath;

                        // Take ownership for cleanup outside mutex
                        threadToCleanup = m_workerThread;
                        workerToCleanup = m_worker;
                        m_workerThread = nullptr;
                        m_worker = nullptr;
                    } else {
                        // State changed, cleanup document and return
                        delete document;
                        return;
                    }
                }

                stopProgressSimulation();
                emit loadingProgressChanged(100);
                emit loadingMessageChanged("加载完成");
                emit documentLoaded(document, filePath);

                // Cleanup worker and thread asynchronously to avoid blocking
                // main thread DEADLOCK FIX: Use deleteLater instead of blocking
                // wait()
                if (threadToCleanup != nullptr) {
                    // Disconnect all signals to prevent issues during cleanup
                    if (workerToCleanup != nullptr) {
                        workerToCleanup->disconnect();
                        workerToCleanup->deleteLater();
                    }

                    // Delete thread after it finishes
                    connect(threadToCleanup, &QThread::finished,
                            threadToCleanup, &QThread::deleteLater);

                    // Request thread to quit gracefully
                    threadToCleanup->quit();

                    // Log if thread doesn't quit within reasonable time
                    QTimer::singleShot(5000, this, [threadToCleanup]() {
                        if (threadToCleanup && threadToCleanup->isRunning()) {
                            SLOG_WARNING(
                                "AsyncDocumentLoader: Thread still running 5s "
                                "after quit request");
                        }
                    });
                }

                // 检查队列中是否还有待加载的文档
                processNextInQueue();
            });
    connect(m_worker, &AsyncDocumentLoaderWorker::loadFailed, this,
            [this](const QString& error) {
                QString filePath;
                QThread* threadToCleanup = nullptr;
                AsyncDocumentLoaderWorker* workerToCleanup = nullptr;

                // DEADLOCK FIX: Minimize mutex scope for thread cleanup
                {
                    QMutexLocker locker(&m_stateMutex);
                    if (m_state == LoadingState::Loading) {
                        m_state = LoadingState::Failed;
                        filePath = m_currentFilePath;

                        // Take ownership for cleanup outside mutex
                        threadToCleanup = m_workerThread;
                        workerToCleanup = m_worker;
                        m_workerThread = nullptr;
                        m_worker = nullptr;
                    } else {
                        return;
                    }
                }

                stopProgressSimulation();
                emit loadingFailed(error, filePath);

                // Cleanup worker and thread asynchronously to avoid blocking
                // main thread DEADLOCK FIX: Use deleteLater instead of blocking
                // wait()
                if (threadToCleanup != nullptr) {
                    // Disconnect all signals to prevent issues during cleanup
                    if (workerToCleanup != nullptr) {
                        workerToCleanup->disconnect();
                        workerToCleanup->deleteLater();
                    }

                    // Delete thread after it finishes
                    connect(threadToCleanup, &QThread::finished,
                            threadToCleanup, &QThread::deleteLater);

                    // Request thread to quit gracefully
                    threadToCleanup->quit();

                    // Log if thread doesn't quit within reasonable time
                    QTimer::singleShot(5000, this, [threadToCleanup]() {
                        if (threadToCleanup && threadToCleanup->isRunning()) {
                            SLOG_WARNING(
                                "AsyncDocumentLoader: Thread still running 5s "
                                "after quit request");
                        }
                    });
                }

                // Process next document in queue after failure
                processNextInQueue();
            });

    // 开始进度模拟和加载
    startProgressSimulation();
    m_workerThread->start();
}

void AsyncDocumentLoader::cancelLoading() {
    QString filePath;
    QThread* threadToCleanup = nullptr;
    AsyncDocumentLoaderWorker* workerToCleanup = nullptr;
    bool emitCancelled = false;

    // DEADLOCK FIX: Minimize mutex scope and avoid holding mutex during thread
    // operations
    {
        QMutexLocker locker(&m_stateMutex);

        if (m_workerThread != nullptr || m_worker != nullptr) {
            if (m_state == LoadingState::Loading) {
                m_state = LoadingState::Cancelled;
                filePath = m_currentFilePath;
                emitCancelled = true;
            }
            // Take ownership of thread and worker for cleanup outside mutex
            threadToCleanup = m_workerThread;
            workerToCleanup = m_worker;
            m_workerThread = nullptr;
            m_worker = nullptr;
        } else {
            return;
        }
    }

    stopProgressSimulation();

    // DEADLOCK FIX: Cleanup thread and worker outside of mutex to prevent
    // deadlocks
    if (threadToCleanup != nullptr) {
        threadToCleanup->quit();
        if (!threadToCleanup->wait(3000)) {
            SLOG_WARNING(
                "AsyncDocumentLoader: Thread cleanup timeout, terminating");
            threadToCleanup->terminate();
            threadToCleanup->wait(1000);
        }
    }

    if (workerToCleanup != nullptr) {
        // Ensure worker is in current thread for deletion after its thread
        // stops
        if (workerToCleanup->thread() != nullptr &&
            workerToCleanup->thread()->isRunning()) {
            workerToCleanup->thread()->quit();
            workerToCleanup->thread()->wait(1000);
        }
    }

    delete workerToCleanup;
    delete threadToCleanup;

    if (emitCancelled) {
        emit loadingCancelled(filePath);
    }
}

AsyncDocumentLoader::LoadingState AsyncDocumentLoader::currentState() const {
    QMutexLocker locker(&m_stateMutex);
    return m_state;
}

QString AsyncDocumentLoader::currentFilePath() const {
    QMutexLocker locker(&m_stateMutex);
    return m_currentFilePath;
}

void AsyncDocumentLoader::queueDocuments(const QStringList& filePaths) {
    bool shouldStartLoading = false;

    {
        QMutexLocker locker(&m_queueMutex);

        // 将文档添加到队列中
        for (const QString& filePath : filePaths) {
            if (!filePath.isEmpty() && QFile::exists(filePath) &&
                !m_documentQueue.contains(filePath)) {
                m_documentQueue.append(filePath);
            }
        }

        SLOG_DEBUG_F("Added {} documents to queue. Queue size: {}",
                     filePaths.size(), m_documentQueue.size());

        // Check if we should start loading
        shouldStartLoading = !m_documentQueue.isEmpty();
    }

    // Start loading first document if idle
    if (shouldStartLoading && currentState() == LoadingState::Idle) {
        processNextInQueue();
    }
}

int AsyncDocumentLoader::queueSize() const {
    QMutexLocker locker(&m_queueMutex);
    return m_documentQueue.size();
}

void AsyncDocumentLoader::processNextInQueue() {
    // Check if we can start loading (must be idle or
    // completed/failed/cancelled)
    {
        QMutexLocker locker(&m_stateMutex);
        if (m_state == LoadingState::Loading) {
            SLOG_DEBUG(
                "AsyncDocumentLoader: Cannot process queue while loading");
            return;  // Already loading, don't start another
        }
    }

    QString nextFilePath;
    {
        QMutexLocker queueLocker(&m_queueMutex);

        if (m_documentQueue.isEmpty()) {
            return;  // 队列为空，无需处理
        }

        nextFilePath = m_documentQueue.takeFirst();
    }

    // 加载下一个文档
    SLOG_DEBUG_F("Loading next document from queue: {}", nextFilePath);
    loadDocument(nextFilePath);
}

void AsyncDocumentLoader::onProgressTimerTimeout() {
    QMutexLocker locker(&m_stateMutex);

    if (m_state != LoadingState::Loading) {
        return;
    }

    locker.unlock();

    // 计算当前进度
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsed = currentTime - m_startTime;

    // 使用非线性进度计算，前80%较快，后20%较慢
    int newProgress;
    if (elapsed < m_expectedLoadTime * 0.8) {
        newProgress =
            static_cast<int>((elapsed * 80.0) / (m_expectedLoadTime * 0.8));
    } else {
        int baseProgress = 80;
        qint64 remainingTime =
            elapsed - static_cast<qint64>(m_expectedLoadTime * 0.8);
        qint64 slowPhaseTime = static_cast<qint64>(m_expectedLoadTime * 0.2);
        int additionalProgress = static_cast<int>((remainingTime * 15.0) /
                                                  slowPhaseTime);  // 最多到95%
        newProgress = qMin(95, baseProgress + additionalProgress);
    }

    if (newProgress != m_currentProgress) {
        m_currentProgress = newProgress;
        emit loadingProgressChanged(m_currentProgress);
    }
}

void AsyncDocumentLoader::startProgressSimulation() {
    m_currentProgress = 0;
    m_startTime = QDateTime::currentMSecsSinceEpoch();
    m_progressTimer->start();
}

void AsyncDocumentLoader::stopProgressSimulation() { m_progressTimer->stop(); }

void AsyncDocumentLoader::resetState() {
    m_currentProgress = 0;
    m_expectedLoadTime = 0;
    m_startTime = 0;
    m_currentFilePath.clear();
}

int AsyncDocumentLoader::calculateExpectedLoadTime(qint64 fileSize) const {
    if (fileSize < SIZE_THRESHOLD_FAST) {
        return MIN_LOAD_TIME;
    }
    if (fileSize < SIZE_THRESHOLD_MEDIUM) {
        // 1MB到10MB之间线性增长
        double ratio = static_cast<double>(fileSize - SIZE_THRESHOLD_FAST) /
                       (SIZE_THRESHOLD_MEDIUM - SIZE_THRESHOLD_FAST);
        return MIN_LOAD_TIME +
               static_cast<int>(ratio * (MAX_LOAD_TIME - MIN_LOAD_TIME) * 0.6);
    }  // 大于10MB的文件
    return static_cast<int>(MAX_LOAD_TIME * 0.8);
}

void AsyncDocumentLoader::setTimeoutConfiguration(int defaultTimeoutMs,
                                                  int minTimeoutMs,
                                                  int maxTimeoutMs) {
    m_configuredDefaultTimeout = defaultTimeoutMs;
    m_configuredMinTimeout = minTimeoutMs;
    m_configuredMaxTimeout = maxTimeoutMs;
    m_useCustomTimeoutConfig = true;

    SLOG_DEBUG_F(
        "AsyncDocumentLoader: Timeout configuration set - Default: {} Min: {} "
        "Max: {}",
        defaultTimeoutMs, minTimeoutMs, maxTimeoutMs);
}

void AsyncDocumentLoader::resetTimeoutConfiguration() {
    m_useCustomTimeoutConfig = false;
    m_configuredDefaultTimeout = 0;
    m_configuredMinTimeout = 0;
    m_configuredMaxTimeout = 0;

    SLOG_DEBUG("AsyncDocumentLoader: Timeout configuration reset to defaults");
}

// AsyncDocumentLoaderWorker 实现
AsyncDocumentLoaderWorker::AsyncDocumentLoaderWorker(const QString& filePath)
    : m_filePath(filePath),
      m_timeoutTimer(nullptr),
      m_cancelled(false),
      m_loadingInProgress(false),
      m_retryCount(0),
      m_maxRetries(DEFAULT_MAX_RETRIES),
      m_customTimeoutMs(0) {
    // Timer will be created in doLoad() when we're in the worker thread
    // This fixes the thread affinity issue where timer was created in main
    // thread but worker was moved to different thread via moveToThread()
}

AsyncDocumentLoaderWorker::~AsyncDocumentLoaderWorker() { cleanup(); }

void AsyncDocumentLoaderWorker::doLoad() {
    // Initialize loading state
    {
        QMutexLocker locker(&m_stateMutex);
        if (m_cancelled) {
            return;  // Already cancelled
        }
        m_loadingInProgress = true;
    }

    // Create timeout timer in worker thread (fixes thread affinity issue)
    if (m_timeoutTimer == nullptr) {
        m_timeoutTimer = new QTimer();  // No parent = current thread affinity
        m_timeoutTimer->setSingleShot(true);
        connect(m_timeoutTimer, &QTimer::timeout, this,
                &AsyncDocumentLoaderWorker::onLoadTimeout);

        SLOG_DEBUG_F(
            "AsyncDocumentLoaderWorker: Timer created in worker thread: {}",
            static_cast<void*>(QThread::currentThread()));
    }

    // Calculate timeout based on file size
    QFileInfo fileInfo(m_filePath);
    int timeoutMs = calculateTimeoutForFile(fileInfo.size());

    // Start timeout timer (now works correctly in same thread)
    m_timeoutTimer->start(timeoutMs);

    SLOG_DEBUG_F(
        "AsyncDocumentLoaderWorker: Starting load with timeout: {} ms for "
        "file: {}",
        timeoutMs, m_filePath);
    SLOG_DEBUG_F(
        "AsyncDocumentLoaderWorker: Timer and worker both in thread: {}",
        static_cast<void*>(QThread::currentThread()));

    try {
        // Check for cancellation before loading
        {
            QMutexLocker locker(&m_stateMutex);
            if (m_cancelled) {
                return;
            }
        }

        // 实际加载文档
        auto document = Poppler::Document::load(m_filePath);

        // Check for cancellation after loading
        {
            QMutexLocker locker(&m_stateMutex);
            if (m_cancelled) {
                SLOG_DEBUG(
                    "AsyncDocumentLoaderWorker: Loading cancelled after "
                    "Poppler::Document::load()");
                // Document will be automatically cleaned up when unique_ptr
                // goes out of scope
                return;  // Loading was cancelled during
                         // Poppler::Document::load()
            }
        }

        // Stop timeout timer - loading completed successfully
        if (m_timeoutTimer != nullptr) {
            m_timeoutTimer->stop();
            SLOG_DEBUG(
                "AsyncDocumentLoaderWorker: Timer stopped - loading completed "
                "successfully");
        }

        if (!document) {
            QMutexLocker locker(&m_stateMutex);
            m_loadingInProgress = false;
            emit loadFailed("无法加载PDF文档");
            return;
        }

        // 配置文档渲染设置
        document->setRenderHint(Poppler::Document::Antialiasing, true);
        document->setRenderHint(Poppler::Document::TextAntialiasing, true);
        document->setRenderHint(Poppler::Document::TextHinting, true);
        document->setRenderHint(Poppler::Document::TextSlightHinting, true);
        document->setRenderHint(Poppler::Document::ThinLineShape, true);
        document->setRenderHint(Poppler::Document::OverprintPreview, true);

        // 验证文档
        if (document->numPages() <= 0) {
            QMutexLocker locker(&m_stateMutex);
            m_loadingInProgress = false;
            emit loadFailed("文档没有有效页面");
            return;
        }

        // 测试第一页
        std::unique_ptr<Poppler::Page> testPage(document->page(0));
        if (!testPage) {
            QMutexLocker locker(&m_stateMutex);
            m_loadingInProgress = false;
            emit loadFailed("无法访问文档页面");
            return;
        }

        // Mark loading as completed
        {
            QMutexLocker locker(&m_stateMutex);
            m_loadingInProgress = false;
        }

        emit loadCompleted(document.release());

    } catch (const std::exception& e) {
        // Stop timeout timer on exception
        if (m_timeoutTimer != nullptr) {
            m_timeoutTimer->stop();
        }

        QMutexLocker locker(&m_stateMutex);
        m_loadingInProgress = false;
        emit loadFailed(QString("加载异常: %1").arg(e.what()));
    } catch (...) {
        // Stop timeout timer on exception
        if (m_timeoutTimer != nullptr) {
            m_timeoutTimer->stop();
        }

        QMutexLocker locker(&m_stateMutex);
        m_loadingInProgress = false;
        emit loadFailed("未知加载错误");
    }
}

void AsyncDocumentLoaderWorker::retryLoad(int extendedTimeoutMs) {
    QMutexLocker locker(&m_stateMutex);

    // Reset state for retry
    m_cancelled = false;
    m_loadingInProgress = false;
    m_customTimeoutMs = extendedTimeoutMs;

    locker.unlock();

    SLOG_DEBUG_F(
        "AsyncDocumentLoaderWorker: Retrying load for file: {} with extended "
        "timeout: {} ms",
        m_filePath, extendedTimeoutMs);

    // Call doLoad to retry
    doLoad();
}

void AsyncDocumentLoaderWorker::onLoadTimeout() {
    bool shouldEmitError = false;
    QString timeoutMessage;

    // DEADLOCK FIX: Minimize mutex scope and prepare error message outside
    // mutex
    {
        QMutexLocker locker(&m_stateMutex);

        if (!m_loadingInProgress || m_cancelled) {
            SLOG_DEBUG(
                "AsyncDocumentLoaderWorker: Timeout ignored - already finished "
                "or cancelled");
            return;  // Already finished or cancelled
        }

        SLOG_DEBUG_F(
            "AsyncDocumentLoaderWorker: Load timeout for file: {} in thread: "
            "{}",
            m_filePath, static_cast<void*>(QThread::currentThread()));

        // Set cancellation flag - this will be checked by the loading operation
        m_cancelled = true;
        m_loadingInProgress = false;
        shouldEmitError = true;

        // Stop the timeout timer to prevent multiple timeouts
        if (m_timeoutTimer != nullptr) {
            m_timeoutTimer->stop();
        }
    }

    if (shouldEmitError) {
        // DEADLOCK FIX: Prepare error message outside of mutex
        QFileInfo fileInfo(m_filePath);
        timeoutMessage =
            QString("文档加载超时: %1 (文件大小: %2 MB，超时时间: %3 秒)")
                .arg(fileInfo.fileName())
                .arg(QString::number(fileInfo.size() / (1024.0 * 1024.0), 'f',
                                     1))
                .arg(calculateTimeoutForFile(fileInfo.size()) / 1000);

        SLOG_DEBUG_F("AsyncDocumentLoaderWorker: Emitting timeout error: {}",
                     timeoutMessage);
        emit loadFailed(timeoutMessage);

        // Perform cleanup - this is now thread-safe
        cleanup();
    }
}

int AsyncDocumentLoaderWorker::calculateTimeoutForFile(qint64 fileSize) const {
    // Use custom timeout if specified (for retries)
    if (m_customTimeoutMs > 0) {
        return qBound(MIN_TIMEOUT_MS, m_customTimeoutMs,
                      MAX_TIMEOUT_MS * 2);  // Allow longer timeout for retries
    }

    // Base timeout calculation on file size
    if (fileSize <= 0) {
        return DEFAULT_TIMEOUT_MS;
    }

    // Apply retry multiplier if this is a retry attempt
    int baseTimeout =
        MIN_TIMEOUT_MS + static_cast<int>((fileSize / (1024 * 1024)) *
                                          2000);  // 2 seconds per MB
    if (m_retryCount > 0) {
        baseTimeout *= EXTENDED_TIMEOUT_MULTIPLIER;
    }

    return qBound(MIN_TIMEOUT_MS, baseTimeout, MAX_TIMEOUT_MS);
}

void AsyncDocumentLoaderWorker::cleanup() {
    QMutexLocker locker(&m_stateMutex);

    if (m_timeoutTimer != nullptr) {
        m_timeoutTimer->stop();
        // Prefer direct delete when target thread has no event loop
        QThread* timerThread = m_timeoutTimer->thread();
        if (timerThread == QThread::currentThread() ||
            (timerThread != nullptr && !timerThread->isRunning())) {
            delete m_timeoutTimer;
        } else {
            m_timeoutTimer->deleteLater();
        }
        m_timeoutTimer = nullptr;
    }

    m_cancelled = true;
    m_loadingInProgress = false;
}
