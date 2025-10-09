#include "AsyncDocumentLoader.h"
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QMutexLocker>

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
    QMutexLocker locker(&m_stateMutex);

    // 检查文件是否存在
    if (!QFile::exists(filePath)) {
        emit loadingFailed("文件不存在", filePath);
        return;
    }

    // 如果正在加载，先取消
    if (m_state == LoadingState::Loading) {
        cancelLoading();
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
                QMutexLocker locker(&m_stateMutex);
                if (m_state == LoadingState::Loading) {
                    m_state = LoadingState::Completed;
                    QString filePath = m_currentFilePath;
                    locker.unlock();

                    stopProgressSimulation();
                    emit loadingProgressChanged(100);
                    emit loadingMessageChanged("加载完成");
                    emit documentLoaded(document, filePath);

                    // 清理工作线程
                    if (m_workerThread) {
                        m_workerThread->quit();
                        m_workerThread->wait();
                        m_workerThread->deleteLater();
                        m_workerThread = nullptr;
                    }
                    if (m_worker) {
                        m_worker->deleteLater();
                        m_worker = nullptr;
                    }

                    // 检查队列中是否还有待加载的文档
                    processNextInQueue();
                }
            });
    connect(m_worker, &AsyncDocumentLoaderWorker::loadFailed, this,
            [this](const QString& error) {
                QMutexLocker locker(&m_stateMutex);
                if (m_state == LoadingState::Loading) {
                    m_state = LoadingState::Failed;
                    locker.unlock();
                    stopProgressSimulation();
                    emit loadingFailed(error, m_currentFilePath);
                }
            });

    // 开始进度模拟和加载
    startProgressSimulation();
    m_workerThread->start();
}

void AsyncDocumentLoader::cancelLoading() {
    QMutexLocker locker(&m_stateMutex);

    if (m_state != LoadingState::Loading) {
        return;
    }

    m_state = LoadingState::Cancelled;
    QString filePath = m_currentFilePath;

    locker.unlock();

    stopProgressSimulation();

    // 清理工作线程
    if (m_workerThread) {
        m_workerThread->quit();
        if (!m_workerThread->wait(3000)) {
            m_workerThread->terminate();
            m_workerThread->wait(1000);
        }
        m_workerThread->deleteLater();
        m_workerThread = nullptr;
    }

    if (m_worker) {
        m_worker->deleteLater();
        m_worker = nullptr;
    }

    emit loadingCancelled(filePath);
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
    QMutexLocker locker(&m_queueMutex);

    // 将文档添加到队列中
    for (const QString& filePath : filePaths) {
        if (!filePath.isEmpty() && QFile::exists(filePath) &&
            !m_documentQueue.contains(filePath)) {
            m_documentQueue.append(filePath);
        }
    }

    qDebug() << "Added" << filePaths.size()
             << "documents to queue. Queue size:" << m_documentQueue.size();
}

int AsyncDocumentLoader::queueSize() const {
    QMutexLocker locker(&m_queueMutex);
    return m_documentQueue.size();
}

void AsyncDocumentLoader::processNextInQueue() {
    QMutexLocker queueLocker(&m_queueMutex);

    if (m_documentQueue.isEmpty()) {
        return;  // 队列为空，无需处理
    }

    QString nextFilePath = m_documentQueue.takeFirst();
    queueLocker.unlock();

    // 加载下一个文档
    qDebug() << "Loading next document from queue:" << nextFilePath;
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
    } else if (fileSize < SIZE_THRESHOLD_MEDIUM) {
        // 1MB到10MB之间线性增长
        double ratio = static_cast<double>(fileSize - SIZE_THRESHOLD_FAST) /
                       (SIZE_THRESHOLD_MEDIUM - SIZE_THRESHOLD_FAST);
        return MIN_LOAD_TIME +
               static_cast<int>(ratio * (MAX_LOAD_TIME - MIN_LOAD_TIME) * 0.6);
    } else {
        // 大于10MB的文件
        return static_cast<int>(MAX_LOAD_TIME * 0.8);
    }
}

void AsyncDocumentLoader::setTimeoutConfiguration(int defaultTimeoutMs,
                                                  int minTimeoutMs,
                                                  int maxTimeoutMs) {
    m_configuredDefaultTimeout = defaultTimeoutMs;
    m_configuredMinTimeout = minTimeoutMs;
    m_configuredMaxTimeout = maxTimeoutMs;
    m_useCustomTimeoutConfig = true;

    qDebug() << "AsyncDocumentLoader: Timeout configuration set - Default:"
             << defaultTimeoutMs << "Min:" << minTimeoutMs
             << "Max:" << maxTimeoutMs;
}

void AsyncDocumentLoader::resetTimeoutConfiguration() {
    m_useCustomTimeoutConfig = false;
    m_configuredDefaultTimeout = 0;
    m_configuredMinTimeout = 0;
    m_configuredMaxTimeout = 0;

    qDebug() << "AsyncDocumentLoader: Timeout configuration reset to defaults";
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
    if (!m_timeoutTimer) {
        m_timeoutTimer = new QTimer();  // No parent = current thread affinity
        m_timeoutTimer->setSingleShot(true);
        connect(m_timeoutTimer, &QTimer::timeout, this,
                &AsyncDocumentLoaderWorker::onLoadTimeout);

        qDebug() << "AsyncDocumentLoaderWorker: Timer created in worker thread:"
                 << QThread::currentThread();
    }

    // Calculate timeout based on file size
    QFileInfo fileInfo(m_filePath);
    int timeoutMs = calculateTimeoutForFile(fileInfo.size());

    // Start timeout timer (now works correctly in same thread)
    m_timeoutTimer->start(timeoutMs);

    qDebug() << "AsyncDocumentLoaderWorker: Starting load with timeout:"
             << timeoutMs << "ms for file:" << m_filePath;
    qDebug() << "AsyncDocumentLoaderWorker: Timer and worker both in thread:"
             << QThread::currentThread();

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
                qDebug() << "AsyncDocumentLoaderWorker: Loading cancelled "
                            "after Poppler::Document::load()";
                // Document will be automatically cleaned up when unique_ptr
                // goes out of scope
                return;  // Loading was cancelled during
                         // Poppler::Document::load()
            }
        }

        // Stop timeout timer - loading completed successfully
        if (m_timeoutTimer) {
            m_timeoutTimer->stop();
            qDebug() << "AsyncDocumentLoaderWorker: Timer stopped - loading "
                        "completed successfully";
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
        if (m_timeoutTimer) {
            m_timeoutTimer->stop();
        }

        QMutexLocker locker(&m_stateMutex);
        m_loadingInProgress = false;
        emit loadFailed(QString("加载异常: %1").arg(e.what()));
    } catch (...) {
        // Stop timeout timer on exception
        if (m_timeoutTimer) {
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

    qDebug() << "AsyncDocumentLoaderWorker: Retrying load for file:"
             << m_filePath << "with extended timeout:" << extendedTimeoutMs
             << "ms";

    // Call doLoad to retry
    doLoad();
}

void AsyncDocumentLoaderWorker::onLoadTimeout() {
    QMutexLocker locker(&m_stateMutex);

    if (!m_loadingInProgress || m_cancelled) {
        qDebug() << "AsyncDocumentLoaderWorker: Timeout ignored - already "
                    "finished or cancelled";
        return;  // Already finished or cancelled
    }

    qDebug() << "AsyncDocumentLoaderWorker: Load timeout for file:"
             << m_filePath << "in thread:" << QThread::currentThread();

    // Set cancellation flag - this will be checked by the loading operation
    m_cancelled = true;
    m_loadingInProgress = false;

    // Stop the timeout timer to prevent multiple timeouts
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
    }

    locker.unlock();

    // Emit timeout error with detailed message
    QFileInfo fileInfo(m_filePath);
    QString timeoutMessage =
        QString("文档加载超时: %1 (文件大小: %2 MB，超时时间: %3 秒)")
            .arg(fileInfo.fileName())
            .arg(QString::number(fileInfo.size() / (1024.0 * 1024.0), 'f', 1))
            .arg(calculateTimeoutForFile(fileInfo.size()) / 1000);

    qDebug() << "AsyncDocumentLoaderWorker: Emitting timeout error:"
             << timeoutMessage;
    emit loadFailed(timeoutMessage);

    // Perform cleanup - this is now thread-safe
    cleanup();
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
        MIN_TIMEOUT_MS + (fileSize / (1024 * 1024)) * 2000;  // 2 seconds per MB
    if (m_retryCount > 0) {
        baseTimeout *= EXTENDED_TIMEOUT_MULTIPLIER;
    }

    return qBound(MIN_TIMEOUT_MS, baseTimeout, MAX_TIMEOUT_MS);
}

void AsyncDocumentLoaderWorker::cleanup() {
    QMutexLocker locker(&m_stateMutex);

    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
        // Timer will be automatically deleted when worker is destroyed
        // since it's created without parent in the worker thread
        m_timeoutTimer->deleteLater();
        m_timeoutTimer = nullptr;
    }

    m_cancelled = true;
    m_loadingInProgress = false;
}
