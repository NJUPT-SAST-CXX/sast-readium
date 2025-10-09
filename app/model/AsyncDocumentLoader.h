#pragma once

#include <QFileInfo>
#include <QMutex>
#include <QObject>
#include <QStringList>
#include <QThread>
#include <QTimer>
#include <atomic>

#include <poppler/qt6/poppler-qt6.h>

// 前向声明
class AsyncDocumentLoaderWorker;

/**
 * 异步文档加载器
 * 在后台线程中加载PDF文档，避免阻塞UI线程
 * 提供加载进度回调和取消功能
 *
 * ARCHITECTURE: Uses a separate worker thread (AsyncDocumentLoaderWorker) to
 * perform the actual document loading via Poppler::Document::load(). The worker
 * is moved to a dedicated thread to prevent UI blocking. Timeout handling is
 * implemented with proper thread affinity to ensure reliable operation.
 *
 * THREAD SAFETY: The timeout timer is created in the worker thread context to
 * ensure Qt's thread affinity rules are respected, fixing previous freeze
 * issues where documents would hang indefinitely during loading.
 */
class AsyncDocumentLoader : public QObject {
    Q_OBJECT

public:
    enum class LoadingState {
        Idle,       // 空闲状态
        Loading,    // 正在加载
        Completed,  // 加载完成
        Failed,     // 加载失败
        Cancelled   // 加载取消
    };

    explicit AsyncDocumentLoader(QObject* parent = nullptr);
    ~AsyncDocumentLoader();

    // 开始异步加载文档
    void loadDocument(const QString& filePath);

    // 将多个文档加入加载队列
    void queueDocuments(const QStringList& filePaths);

    // 取消当前加载
    void cancelLoading();

    // 获取当前状态
    LoadingState currentState() const;

    // 获取当前加载的文件路径
    QString currentFilePath() const;

    // 获取队列中剩余文档数量
    int queueSize() const;

    // 配置超时设置
    void setTimeoutConfiguration(int defaultTimeoutMs, int minTimeoutMs,
                                 int maxTimeoutMs);
    void resetTimeoutConfiguration();

signals:
    // 加载进度更新 (0-100)
    void loadingProgressChanged(int progress);

    // 加载状态消息更新
    void loadingMessageChanged(const QString& message);

    // 加载完成
    void documentLoaded(Poppler::Document* document, const QString& filePath);

    // 加载失败
    void loadingFailed(const QString& error, const QString& filePath);

    // 加载取消
    void loadingCancelled(const QString& filePath);

private slots:
    void onProgressTimerTimeout();

private:
    void startProgressSimulation();
    void stopProgressSimulation();
    void resetState();
    void processNextInQueue();
    int calculateExpectedLoadTime(qint64 fileSize) const;

    // 状态管理
    LoadingState m_state;
    QString m_currentFilePath;
    mutable QMutex m_stateMutex;

    // 文档加载队列
    QStringList m_documentQueue;
    mutable QMutex m_queueMutex;

    // 进度模拟
    QTimer* m_progressTimer;
    int m_currentProgress;
    int m_expectedLoadTime;  // 预期加载时间(ms)
    qint64 m_startTime;      // 开始加载时间

    // 工作线程
    QThread* m_workerThread;
    AsyncDocumentLoaderWorker* m_worker;

    // 超时配置
    int m_configuredDefaultTimeout;
    int m_configuredMinTimeout;
    int m_configuredMaxTimeout;
    bool m_useCustomTimeoutConfig;

    // 常量
    static constexpr int PROGRESS_UPDATE_INTERVAL = 50;  // 50ms更新一次进度
    static constexpr int MIN_LOAD_TIME = 200;   // 最小加载时间200ms
    static constexpr int MAX_LOAD_TIME = 5000;  // 最大加载时间5s
    static constexpr qint64 SIZE_THRESHOLD_FAST =
        1024 * 1024;  // 1MB以下快速加载
    static constexpr qint64 SIZE_THRESHOLD_MEDIUM =
        10 * 1024 * 1024;  // 10MB以下中等加载
};

/**
 * 文档加载工作线程类
 *
 * THREAD SAFETY NOTE: This class is designed to be moved to a worker thread via
 * moveToThread(). The timeout timer is created in doLoad() when the worker is
 * already in the target thread, ensuring proper Qt thread affinity. This fixes
 * the previous issue where the timer was created in the main thread but the
 * worker was moved to a different thread, causing timeout mechanisms to fail
 * and document loading to freeze indefinitely.
 */
class AsyncDocumentLoaderWorker : public QObject {
    Q_OBJECT

public:
    AsyncDocumentLoaderWorker(const QString& filePath);
    ~AsyncDocumentLoaderWorker();

public slots:
    void doLoad();
    void retryLoad(int extendedTimeoutMs = 0);

signals:
    void loadCompleted(Poppler::Document* document);
    void loadFailed(const QString& error);

private slots:
    void onLoadTimeout();

private:
    QString m_filePath;

    // Timeout mechanism - Timer is created in worker thread to ensure proper
    // thread affinity This fixes the issue where timer created in main thread
    // couldn't properly timeout operations running in worker thread due to Qt's
    // thread affinity rules
    QTimer* m_timeoutTimer;
    QMutex m_stateMutex;
    std::atomic<bool> m_cancelled;
    std::atomic<bool> m_loadingInProgress;

    // Retry mechanism
    int m_retryCount;
    int m_maxRetries;
    int m_customTimeoutMs;

    // Timeout constants
    static constexpr int DEFAULT_TIMEOUT_MS = 30000;  // 30 seconds default
    static constexpr int MIN_TIMEOUT_MS = 5000;       // 5 seconds minimum
    static constexpr int MAX_TIMEOUT_MS = 120000;     // 2 minutes maximum

    // Retry constants
    static constexpr int DEFAULT_MAX_RETRIES = 2;  // Maximum retry attempts
    static constexpr int EXTENDED_TIMEOUT_MULTIPLIER =
        2;  // Multiply timeout by this for retries

    // Helper methods
    int calculateTimeoutForFile(qint64 fileSize) const;
    void cleanup();
};
