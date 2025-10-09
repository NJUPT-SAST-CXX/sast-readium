#include "ThumbnailGenerator.h"
#include <poppler-qt6.h>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QHash>
#include <QMutexLocker>
#include <QQueue>
#include <QThread>
#include <QtConcurrent/QtConcurrent>
#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <algorithm>
#include <cmath>
#include "utils/LoggingMacros.h"

ThumbnailGenerator::ThumbnailGenerator(QObject* parent)
    : QObject(parent),
      m_defaultSize(DEFAULT_THUMBNAIL_WIDTH, DEFAULT_THUMBNAIL_HEIGHT),
      m_defaultQuality(DEFAULT_QUALITY),
      m_maxConcurrentJobs(DEFAULT_MAX_CONCURRENT_JOBS),
      m_maxRetries(DEFAULT_MAX_RETRIES),
      m_running(false),
      m_paused(false),
      m_batchSize(DEFAULT_BATCH_SIZE),
      m_batchInterval(DEFAULT_BATCH_INTERVAL),
      m_totalGenerated(0),
      m_totalErrors(0),
      m_totalTime(0) {
    initializeGenerator();
}

ThumbnailGenerator::~ThumbnailGenerator() {
    stop();
    cleanupJobs();
}

void ThumbnailGenerator::initializeGenerator() {
    // 创建批处理定时器
    m_batchTimer = new QTimer(this);
    m_batchTimer->setInterval(m_batchInterval);
    m_batchTimer->setSingleShot(false);
    connect(m_batchTimer, &QTimer::timeout, this,
            &ThumbnailGenerator::onBatchTimer);

    // 创建队列处理定时器
    QTimer* queueTimer = new QTimer(this);
    queueTimer->setInterval(QUEUE_PROCESS_INTERVAL);
    queueTimer->setSingleShot(false);
    connect(queueTimer, &QTimer::timeout, this,
            &ThumbnailGenerator::processQueue);
    queueTimer->start();
}

void ThumbnailGenerator::setDocument(
    std::shared_ptr<Poppler::Document> document) {
    QMutexLocker locker(&m_documentMutex);

    // 停止当前所有任务
    clearQueue();
    cleanupJobs();

    m_document = document;

    // 配置文档渲染设置
    if (m_document) {
        // ArthurBackend可能不可用，注释掉
        // m_document->setRenderBackend(Poppler::Document::ArthurBackend);
        m_document->setRenderHint(Poppler::Document::Antialiasing, true);
        m_document->setRenderHint(Poppler::Document::TextAntialiasing, true);
        m_document->setRenderHint(Poppler::Document::TextHinting, true);
        m_document->setRenderHint(Poppler::Document::TextSlightHinting, true);
    }
}

std::shared_ptr<Poppler::Document> ThumbnailGenerator::document() const {
    QMutexLocker locker(&m_documentMutex);
    return m_document;
}

void ThumbnailGenerator::setThumbnailSize(const QSize& size) {
    if (size.isValid() && m_defaultSize != size) {
        m_defaultSize = size;

        // 清除队列中使用默认尺寸的请求
        QMutexLocker locker(&m_queueMutex);
        QQueue<GenerationRequest> newQueue;

        while (!m_requestQueue.isEmpty()) {
            GenerationRequest req = m_requestQueue.dequeue();
            if (req.size !=
                QSize(DEFAULT_THUMBNAIL_WIDTH, DEFAULT_THUMBNAIL_HEIGHT)) {
                newQueue.enqueue(req);
            }
        }

        m_requestQueue = newQueue;
        emit queueSizeChanged(m_requestQueue.size());
    }
}

void ThumbnailGenerator::setQuality(double quality) {
    m_defaultQuality = qBound(0.1, quality, 3.0);
}

void ThumbnailGenerator::setMaxConcurrentJobs(int maxJobs) {
    m_maxConcurrentJobs = qBound(1, maxJobs, 8);

    // 如果当前活动任务超过限制，等待一些完成
    while (activeJobCount() > m_maxConcurrentJobs) {
        QThread::msleep(10);
        QApplication::processEvents();
    }
}

void ThumbnailGenerator::setMaxRetries(int maxRetries) {
    m_maxRetries = qBound(0, maxRetries, 5);
}

void ThumbnailGenerator::generateThumbnail(int pageNumber, const QSize& size,
                                           double quality, int priority) {
    if (!m_document) {
        emit thumbnailError(pageNumber, "No document loaded");
        return;
    }

    if (pageNumber < 0 || pageNumber >= m_document->numPages()) {
        emit thumbnailError(pageNumber, "Invalid page number");
        return;
    }

    // 使用默认值填充参数
    QSize actualSize = size.isValid() ? size : m_defaultSize;
    double actualQuality = (quality > 0) ? quality : m_defaultQuality;

    GenerationRequest request(pageNumber, actualSize, actualQuality, priority);

    QMutexLocker locker(&m_queueMutex);

    // 检查是否已经在队列中或正在处理
    for (const auto& existing : m_requestQueue) {
        if (existing.pageNumber == pageNumber && existing.size == actualSize &&
            qAbs(existing.quality - actualQuality) < 0.001) {
            return;  // 已存在相同请求
        }
    }

    // 检查是否正在处理
    if (isGenerating(pageNumber)) {
        return;
    }

    m_requestQueue.enqueue(request);
    std::sort(m_requestQueue.begin(), m_requestQueue.end());

    emit queueSizeChanged(m_requestQueue.size());
    m_queueCondition.wakeOne();
}

void ThumbnailGenerator::generateThumbnailRange(int startPage, int endPage,
                                                const QSize& size,
                                                double quality) {
    if (!m_document)
        return;

    int numPages = m_document->numPages();
    startPage = qBound(0, startPage, numPages - 1);
    endPage = qBound(startPage, endPage, numPages - 1);

    for (int i = startPage; i <= endPage; ++i) {
        generateThumbnail(i, size, quality, i - startPage);  // 按顺序设置优先级
    }
}

void ThumbnailGenerator::clearQueue() {
    QMutexLocker locker(&m_queueMutex);
    m_requestQueue.clear();
    emit queueSizeChanged(0);
}

void ThumbnailGenerator::cancelRequest(int pageNumber) {
    QMutexLocker locker(&m_queueMutex);

    QQueue<GenerationRequest> newQueue;
    while (!m_requestQueue.isEmpty()) {
        GenerationRequest req = m_requestQueue.dequeue();
        if (req.pageNumber != pageNumber) {
            newQueue.enqueue(req);
        }
    }

    m_requestQueue = newQueue;
    emit queueSizeChanged(m_requestQueue.size());

    // 也尝试取消正在进行的任务
    QMutexLocker jobsLocker(&m_jobsMutex);
    auto it = m_activeJobs.find(pageNumber);
    if (it != m_activeJobs.end()) {
        GenerationJob* job = it.value();
        if (job && job->watcher) {
            job->watcher->cancel();
        }
        delete job;  // 手动删除
        m_activeJobs.erase(it);
        emit activeJobsChanged(m_activeJobs.size());
    }
}

void ThumbnailGenerator::setPriority(int pageNumber, int priority) {
    QMutexLocker locker(&m_queueMutex);

    for (auto& req : m_requestQueue) {
        if (req.pageNumber == pageNumber) {
            req.priority = priority;
            break;
        }
    }

    // 重新排序队列
    std::sort(m_requestQueue.begin(), m_requestQueue.end());
}

bool ThumbnailGenerator::isGenerating(int pageNumber) const {
    QMutexLocker locker(&m_jobsMutex);
    return m_activeJobs.contains(pageNumber);
}

int ThumbnailGenerator::queueSize() const {
    QMutexLocker locker(&m_queueMutex);
    return m_requestQueue.size();
}

int ThumbnailGenerator::activeJobCount() const {
    QMutexLocker locker(&m_jobsMutex);
    return m_activeJobs.size();
}

void ThumbnailGenerator::pause() { m_paused = true; }

void ThumbnailGenerator::resume() {
    m_paused = false;
    m_queueCondition.wakeAll();
}

void ThumbnailGenerator::stop() {
    m_running = false;
    m_paused = false;

    clearQueue();
    cleanupJobs();

    if (m_batchTimer) {
        m_batchTimer->stop();
    }
}

void ThumbnailGenerator::start() {
    m_running = true;
    m_paused = false;

    if (m_batchTimer) {
        m_batchTimer->start();
    }
}

void ThumbnailGenerator::processQueue() {
    if (!m_running || m_paused || !m_document) {
        return;
    }

    // 启动新任务直到达到并发限制
    while (activeJobCount() < m_maxConcurrentJobs && queueSize() > 0) {
        startNextJob();
    }
}

void ThumbnailGenerator::startNextJob() {
    QMutexLocker queueLocker(&m_queueMutex);

    if (m_requestQueue.isEmpty()) {
        return;
    }

    GenerationRequest request = m_requestQueue.dequeue();
    emit queueSizeChanged(m_requestQueue.size());

    queueLocker.unlock();

    // 检查是否已经在处理
    if (isGenerating(request.pageNumber)) {
        return;
    }

    // 创建新任务
    auto job = std::make_unique<GenerationJob>();
    job->request = request;
    job->watcher = new QFutureWatcher<QPixmap>();

    connect(job->watcher, &QFutureWatcher<QPixmap>::finished, this,
            &ThumbnailGenerator::onGenerationFinished);

    // 启动异步生成
    job->future = QtConcurrent::run(
        [this, request]() { return generatePixmap(request); });

    job->watcher->setFuture(job->future);

    // 添加到活动任务
    QMutexLocker jobsLocker(&m_jobsMutex);
    m_activeJobs[request.pageNumber] = job.release();  // 转移所有权
    emit activeJobsChanged(m_activeJobs.size());
}

void ThumbnailGenerator::onGenerationFinished() {
    QFutureWatcher<QPixmap>* watcher =
        static_cast<QFutureWatcher<QPixmap>*>(sender());
    if (!watcher)
        return;

    // 找到对应的任务
    GenerationJob* job = nullptr;
    int pageNumber = -1;

    {
        QMutexLocker locker(&m_jobsMutex);
        for (auto it = m_activeJobs.begin(); it != m_activeJobs.end(); ++it) {
            if (it.value()->watcher == watcher) {
                job = it.value();
                pageNumber = it.key();
                break;
            }
        }
    }

    if (!job) {
        watcher->deleteLater();
        return;
    }

    try {
        QPixmap pixmap = watcher->result();

        if (!pixmap.isNull()) {
            handleJobCompletion(job);
            emit thumbnailGenerated(pageNumber, pixmap);
            m_totalGenerated++;
        } else {
            handleJobError(job, "Failed to generate pixmap");
        }
    } catch (const std::exception& e) {
        handleJobError(job, QString("Generation error: %1").arg(e.what()));
    } catch (...) {
        handleJobError(job, "Unknown generation error");
    }

    // 清理任务
    {
        QMutexLocker locker(&m_jobsMutex);
        delete m_activeJobs.take(pageNumber);  // 删除并移除
        emit activeJobsChanged(m_activeJobs.size());
    }

    watcher->deleteLater();
}

void ThumbnailGenerator::onBatchTimer() {
    // 批处理逻辑：定期检查队列状态
    updateStatistics();

    // 如果队列积压太多，增加并发数
    if (queueSize() > m_batchSize * 2 && m_maxConcurrentJobs < 6) {
        setMaxConcurrentJobs(m_maxConcurrentJobs + 1);
    }
    // 如果队列很少，减少并发数以节省资源
    else if (queueSize() < m_batchSize && m_maxConcurrentJobs > 2) {
        setMaxConcurrentJobs(m_maxConcurrentJobs - 1);
    }
}

void ThumbnailGenerator::cleanupJobs() {
    QMutexLocker locker(&m_jobsMutex);

    for (auto it = m_activeJobs.begin(); it != m_activeJobs.end(); ++it) {
        GenerationJob* job = it.value();
        if (job && job->watcher) {
            job->watcher->cancel();
            job->watcher->waitForFinished();
        }
        delete job;  // 手动删除
    }

    m_activeJobs.clear();
    emit activeJobsChanged(0);
}

void ThumbnailGenerator::handleJobCompletion(GenerationJob* job) {
    qint64 duration =
        QDateTime::currentMSecsSinceEpoch() - job->request.timestamp;
    logPerformance(job->request, duration);
    m_totalTime += duration;
}

void ThumbnailGenerator::handleJobError(GenerationJob* job,
                                        const QString& error) {
    m_totalErrors++;

    // 检查是否需要重试
    if (job->request.retryCount < m_maxRetries) {
        job->request.retryCount++;
        job->request.timestamp = QDateTime::currentMSecsSinceEpoch();
        job->request.priority += 10;  // 降低优先级

        // 重新加入队列
        QMutexLocker locker(&m_queueMutex);
        m_requestQueue.enqueue(job->request);
        std::sort(m_requestQueue.begin(), m_requestQueue.end());
        emit queueSizeChanged(m_requestQueue.size());

        qDebug() << "Retrying thumbnail generation for page"
                 << job->request.pageNumber << "attempt"
                 << job->request.retryCount;
    } else {
        emit thumbnailError(job->request.pageNumber, error);
        qWarning() << "Failed to generate thumbnail for page"
                   << job->request.pageNumber << "after" << m_maxRetries
                   << "retries:" << error;
    }
}

QPixmap ThumbnailGenerator::generatePixmap(const GenerationRequest& request) {
    QMutexLocker locker(&m_documentMutex);

    if (!m_document) {
        return QPixmap();
    }

    try {
        std::unique_ptr<Poppler::Page> page(
            m_document->page(request.pageNumber));
        if (!page) {
            return QPixmap();
        }

        return renderPageToPixmap(page.get(), request.size, request.quality);

    } catch (const std::exception& e) {
        LOG_WARNING("Exception in generatePixmap: {}", std::string(e.what()));
        return QPixmap();
    } catch (...) {
        LOG_WARNING("Unknown exception in generatePixmap");
        return QPixmap();
    }
}

QPixmap ThumbnailGenerator::renderPageToPixmap(Poppler::Page* page,
                                               const QSize& size,
                                               double quality) {
    // 使用优化版本
    return renderPageToPixmapOptimized(page, size, quality);
}

QPixmap ThumbnailGenerator::renderPageToPixmapOptimized(Poppler::Page* page,
                                                        const QSize& size,
                                                        double quality) {
    if (!page) {
        return QPixmap();
    }

    try {
        QSizeF pageSize = page->pageSizeF();
        double dpi = getCachedDPI(size, pageSize, quality);

        // 渲染页面 - 直接渲染到目标尺寸附近以减少缩放
        QImage image = page->renderToImage(dpi, dpi, -1, -1, -1, -1,
                                           Poppler::Page::Rotate0);

        if (image.isNull()) {
            return QPixmap();
        }

        // 优化缩放操作
        if (image.size() != size) {
            Qt::TransformationMode mode =
                getOptimalTransformationMode(image.size(), size);
            image = image.scaled(size, Qt::KeepAspectRatio, mode);
        }

        return QPixmap::fromImage(image);

    } catch (const std::exception& e) {
        qWarning() << "Exception in renderPageToPixmapOptimized:" << e.what();
        return QPixmap();
    } catch (...) {
        qWarning() << "Unknown exception in renderPageToPixmapOptimized";
        return QPixmap();
    }
}

double ThumbnailGenerator::calculateOptimalDPI(const QSize& targetSize,
                                               const QSizeF& pageSize,
                                               double quality) {
    if (pageSize.isEmpty() || targetSize.isEmpty()) {
        return MIN_DPI;
    }

    // 计算缩放比例
    double scaleX = targetSize.width() / pageSize.width();
    double scaleY = targetSize.height() / pageSize.height();
    double scale = qMin(scaleX, scaleY);

    // 基础DPI - 优化：根据目标尺寸调整基础DPI
    double baseDPI = (targetSize.width() <= 150) ? 72.0 : 96.0;

    // 根据质量和缩放比例计算DPI
    double dpi = baseDPI * scale * quality;

    // 考虑设备像素比
    double deviceRatio = qApp->devicePixelRatio();
    dpi *= deviceRatio;

    // 限制DPI范围
    return qBound(MIN_DPI, dpi, MAX_DPI);
}

double ThumbnailGenerator::getCachedDPI(const QSize& targetSize,
                                        const QSizeF& pageSize,
                                        double quality) {
    QString cacheKey = QString("%1x%2_%3x%4_%5")
                           .arg(targetSize.width())
                           .arg(targetSize.height())
                           .arg(static_cast<int>(pageSize.width()))
                           .arg(static_cast<int>(pageSize.height()))
                           .arg(static_cast<int>(quality * 100));

    QMutexLocker locker(&m_dpiCacheMutex);
    auto it = m_dpiCache.find(cacheKey);
    if (it != m_dpiCache.end()) {
        return it.value();
    }

    locker.unlock();

    double dpi = calculateOptimalDPI(targetSize, pageSize, quality);
    cacheDPI(targetSize, pageSize, quality, dpi);
    return dpi;
}

void ThumbnailGenerator::cacheDPI(const QSize& targetSize,
                                  const QSizeF& pageSize, double quality,
                                  double dpi) {
    QString cacheKey = QString("%1x%2_%3x%4_%5")
                           .arg(targetSize.width())
                           .arg(targetSize.height())
                           .arg(static_cast<int>(pageSize.width()))
                           .arg(static_cast<int>(pageSize.height()))
                           .arg(static_cast<int>(quality * 100));

    QMutexLocker locker(&m_dpiCacheMutex);
    m_dpiCache[cacheKey] = dpi;

    // 限制缓存大小
    if (m_dpiCache.size() > 100) {
        auto it = m_dpiCache.begin();
        m_dpiCache.erase(it);
    }
}

Qt::TransformationMode ThumbnailGenerator::getOptimalTransformationMode(
    const QSize& sourceSize, const QSize& targetSize) {
    // 如果缩放比例较小或目标尺寸较小，使用快速变换
    double scaleRatio =
        qMin(static_cast<double>(targetSize.width()) / sourceSize.width(),
             static_cast<double>(targetSize.height()) / sourceSize.height());

    if (scaleRatio > 0.8 || targetSize.width() <= 150) {
        return Qt::FastTransformation;
    }

    return Qt::SmoothTransformation;
}

void ThumbnailGenerator::updateStatistics() {
    int totalRequests = m_totalGenerated + m_totalErrors;
    if (totalRequests > 0) {
        double successRate = (double)m_totalGenerated / totalRequests * 100.0;
        double avgTime =
            totalRequests > 0 ? (double)m_totalTime / totalRequests : 0.0;

        emit generationProgress(m_totalGenerated, totalRequests);

        // 定期输出统计信息
        static int logCounter = 0;
        if (++logCounter % 50 == 0) {
            qDebug() << "Thumbnail generation stats:"
                     << "Success rate:" << QString::number(successRate, 'f', 1)
                     << "%"
                     << "Avg time:" << QString::number(avgTime, 'f', 1) << "ms"
                     << "Queue size:" << queueSize()
                     << "Active jobs:" << activeJobCount();
        }
    }
}

void ThumbnailGenerator::logPerformance(const GenerationRequest& request,
                                        qint64 duration) {
    Q_UNUSED(request)

    // 记录性能数据用于优化
    if (duration > 1000) {  // 超过1秒的任务
        qDebug() << "Slow thumbnail generation:"
                 << "Page" << request.pageNumber << "Size" << request.size
                 << "Quality" << request.quality << "Duration" << duration
                 << "ms";
    }
}
