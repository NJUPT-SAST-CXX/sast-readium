#include "ThumbnailGenerator.h"
#include <poppler/qt6/poppler-qt6.h>
#include <QApplication>
#include <QBuffer>
#include <QDateTime>
#include <QDebug>
#include <QHash>
#include <QImageWriter>
#include <QMutexLocker>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QQueue>
#include <QThread>
#include <QTimer>
#include <QtConcurrent/QtConcurrent>
#include <algorithm>
#include <limits>
#include "../../logging/LoggingMacros.h"
#include "../../model/RenderModel.h"
#include "../../utils/SafePDFRenderer.h"

// Constants for magic numbers
namespace {
constexpr double MIN_QUALITY_BOUND = 0.1;
constexpr double MAX_QUALITY_BOUND = 3.0;
constexpr int DEFAULT_MAX_JOBS_BOUND = 8;
constexpr int DEFAULT_MAX_RETRIES_BOUND = 5;
constexpr double QUALITY_TOLERANCE = 0.001;
constexpr qint64 MEMORY_POOL_ENTRY_AGE_MS = 300000;  // 5 minutes
constexpr int BYTES_PER_PIXEL = 4;                   // RGBA
}  // namespace

ThumbnailGenerator::ThumbnailGenerator(QObject* parent)
    : QObject(parent),
      m_document(nullptr),
      m_documentMutex(),
      m_dpiCache(),
      m_dpiCacheMutex(),
      m_requestQueue(),
      m_queueMutex(),
      m_queueCondition(),
      m_activeJobs(),
      m_jobsMutex(),
      m_defaultSize(DEFAULT_THUMBNAIL_WIDTH, DEFAULT_THUMBNAIL_HEIGHT),
      m_defaultQuality(THUMBNAIL_DEFAULT_QUALITY),
      m_maxConcurrentJobs(DEFAULT_MAX_CONCURRENT_JOBS),
      m_maxRetries(DEFAULT_MAX_RETRIES),
      m_renderMode(RenderMode::Hybrid),
      m_cacheStrategy(CacheStrategy::Adaptive),
      m_gpuAccelerationEnabled(true),
      m_gpuAccelerationAvailable(false),
      m_gpuContext(nullptr),
      m_memoryPoolSize(DEFAULT_MEMORY_POOL_SIZE),
      m_memoryPool(),
      m_memoryPoolMutex(),
      m_compressionEnabled(true),
      m_compressionQuality(DEFAULT_COMPRESSION_QUALITY),
      m_compressedCache(),
      m_cacheMetadata(),
      m_cacheMetadataMutex(),
      m_maxCacheSize(DEFAULT_MAX_CACHE_SIZE),
      m_running(false),
      m_paused(false),
      m_batchTimer(nullptr),
      m_batchSize(DEFAULT_BATCH_SIZE),
      m_batchInterval(DEFAULT_BATCH_INTERVAL),
      m_totalGenerated(0),
      m_totalErrors(0),
      m_totalTime(0) {
    // 初始化压缩缓存
    m_compressedCache.setMaxCost(COMPRESSED_CACHE_SIZE);

    initializeGenerator();

    // 检查GPU加速可用性
    if (m_gpuAccelerationEnabled) {
        m_gpuAccelerationAvailable = initializeGpuContext();
        if (!m_gpuAccelerationAvailable) {
            LOG_WARNING(
                "GPU acceleration not available, falling back to CPU "
                "rendering");
        }
    }
}

ThumbnailGenerator::~ThumbnailGenerator() {
    stop();
    cleanupJobs();
    cleanupGpuContext();
    cleanupMemoryPool();
}

void ThumbnailGenerator::initializeGenerator() {
    // 创建批处理定时器
    m_batchTimer = new QTimer(this);
    m_batchTimer->setInterval(m_batchInterval);
    m_batchTimer->setSingleShot(false);
    connect(m_batchTimer, &QTimer::timeout, this,
            &ThumbnailGenerator::onBatchTimer);

    // 创建队列处理定时器
    auto* queueTimer = new QTimer(this);
    queueTimer->setInterval(QUEUE_PROCESS_INTERVAL);
    queueTimer->setSingleShot(false);
    connect(queueTimer, &QTimer::timeout, this,
            &ThumbnailGenerator::processQueue);
    queueTimer->start();
}

void ThumbnailGenerator::setDocument(
    std::shared_ptr<Poppler::Document> document) {
    // DEADLOCK FIX: Avoid nested mutex calls by clearing queues/jobs before
    // acquiring document mutex This prevents the lock ordering: documentMutex
    // -> queueMutex -> jobsMutex

    // First, stop all operations without holding document mutex
    clearQueue();
    cleanupJobs();

    // Now safely acquire document mutex and set the new document
    QMutexLocker locker(&m_documentMutex);
    m_document = std::move(document);

    // 配置文档渲染设置 - use centralized configuration
    if (m_document) {
        // ArthurBackend可能不可用，注释掉
        // m_document->setRenderBackend(Poppler::Document::ArthurBackend);
        RenderModel::configureDocumentRenderHints(m_document.get());
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
        emit queueSizeChanged(static_cast<int>(m_requestQueue.size()));
    }
}

void ThumbnailGenerator::setQuality(double quality) {
    m_defaultQuality = qBound(MIN_QUALITY_BOUND, quality, MAX_QUALITY_BOUND);
}

void ThumbnailGenerator::setMaxConcurrentJobs(int maxJobs) {
    m_maxConcurrentJobs = qBound(1, maxJobs, DEFAULT_MAX_JOBS_BOUND);

    // DEADLOCK FIX: Replace busy-wait loop with proper signaling
    // The old busy-wait loop could cause deadlocks if called from main thread
    // while holding locks Instead, we'll let the natural job completion process
    // handle the reduction

    // If we're reducing concurrent jobs, the processQueue() method will
    // naturally respect the new limit when starting new jobs. No need to
    // forcefully wait here. This prevents potential deadlocks and improves
    // responsiveness.
}

void ThumbnailGenerator::setMaxRetries(int maxRetries) {
    m_maxRetries = qBound(0, maxRetries, DEFAULT_MAX_RETRIES_BOUND);
}

void ThumbnailGenerator::generateThumbnail(int pageNumber, const QSize& size,
                                           double quality, int priority,
                                           RenderMode mode) {
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

    // DEADLOCK FIX: Check if already generating BEFORE acquiring queue mutex
    // This prevents nested mutex calls (queueMutex -> jobsMutex)
    bool alreadyGenerating = false;
    {
        QMutexLocker jobsLocker(&m_jobsMutex);
        alreadyGenerating = m_activeJobs.contains(pageNumber);
    }

    if (alreadyGenerating) {
        return;  // Already being processed
    }

    QMutexLocker queueLocker(&m_queueMutex);

    // 检查是否已经在队列中
    for (const auto& existing : m_requestQueue) {
        if (existing.pageNumber == pageNumber && existing.size == actualSize &&
            qAbs(existing.quality - actualQuality) < QUALITY_TOLERANCE) {
            return;  // 已存在相同请求
        }
    }

    m_requestQueue.enqueue(request);
    std::sort(m_requestQueue.begin(), m_requestQueue.end());

    emit queueSizeChanged(static_cast<int>(m_requestQueue.size()));
    m_queueCondition.wakeOne();

    // Ensure processing starts promptly in on-demand usage (tests/integration)
    if (!m_running) {
        start();
    }
    // Kick the queue processor immediately for responsiveness
    processQueue();
}

void ThumbnailGenerator::generateThumbnailRange(int startPage, int endPage,
                                                const QSize& size,
                                                double quality) {
    if (!m_document) {
        return;
    }

    int numPages = m_document->numPages();
    startPage = qBound(0, startPage, numPages - 1);
    endPage = qBound(startPage, endPage, numPages - 1);

    for (int i = startPage; i <= endPage; ++i) {
        generateThumbnail(i, size, quality, i - startPage);  // 按顺序设置优先级
    }

    if (!m_running) {
        start();
    }
    processQueue();
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
    emit queueSizeChanged(static_cast<int>(m_requestQueue.size()));

    // 也尝试取消正在进行的任务
    QMutexLocker jobsLocker(&m_jobsMutex);
    auto jobIter = m_activeJobs.find(pageNumber);
    if (jobIter != m_activeJobs.end()) {
        GenerationJob* job = jobIter.value();
        if (job != nullptr && job->watcher != nullptr) {
            job->watcher->cancel();
        }
        delete job;  // 手动删除
        m_activeJobs.erase(jobIter);
        emit activeJobsChanged(static_cast<int>(m_activeJobs.size()));
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

    // DEADLOCK FIX: Stop timer first to prevent new operations
    if (m_batchTimer != nullptr) {
        m_batchTimer->stop();
    }

    // Clear operations in safe order
    clearQueue();
    cleanupJobs();
}

void ThumbnailGenerator::start() {
    m_running = true;
    m_paused = false;

    if (m_batchTimer != nullptr) {
        m_batchTimer->start();
    }
}

void ThumbnailGenerator::processQueue() {
    if (!m_running || m_paused || m_document == nullptr) {
        return;
    }

    // 启动新任务直到达到并发限制
    while (activeJobCount() < m_maxConcurrentJobs && queueSize() > 0) {
        startNextJob();
    }
}

void ThumbnailGenerator::startNextJob() {
    GenerationRequest request;
    bool hasRequest = false;

    // DEADLOCK FIX: Minimize mutex scope and avoid nested mutex calls
    {
        QMutexLocker queueLocker(&m_queueMutex);
        if (!m_requestQueue.isEmpty()) {
            request = m_requestQueue.dequeue();
            hasRequest = true;
            emit queueSizeChanged(static_cast<int>(m_requestQueue.size()));
        }
    }

    if (!hasRequest) {
        return;
    }

    // Check if already generating (separate mutex scope)
    {
        QMutexLocker jobsLocker(&m_jobsMutex);
        if (m_activeJobs.contains(request.pageNumber)) {
            return;  // Already being processed
        }
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

    qInfo() << "ThumbnailGenerator: started job for page" << request.pageNumber
            << "size" << request.size;

    // 添加到活动任务 (separate mutex scope)
    {
        QMutexLocker jobsLocker(&m_jobsMutex);
        m_activeJobs[request.pageNumber] = job.release();  // 转移所有权
        emit activeJobsChanged(static_cast<int>(m_activeJobs.size()));
    }
}

void ThumbnailGenerator::onGenerationFinished() {
    auto* watcher = static_cast<QFutureWatcher<QPixmap>*>(sender());
    if (watcher == nullptr) {
        return;
    }

    // 找到对应的任务
    GenerationJob* job = nullptr;
    int pageNumber = -1;

    {
        QMutexLocker locker(&m_jobsMutex);
        for (auto jobsIter = m_activeJobs.begin();
             jobsIter != m_activeJobs.end(); ++jobsIter) {
            if (jobsIter.value()->watcher == watcher) {
                job = jobsIter.value();
                pageNumber = jobsIter.key();
                break;
            }
        }
    }

    if (job == nullptr) {
        watcher->deleteLater();
        return;
    }

    try {
        QPixmap pixmap = watcher->result();

        if (!pixmap.isNull()) {
            qInfo() << "ThumbnailGenerator: job completed for page"
                    << pageNumber;
            handleJobCompletion(job);
            emit thumbnailGenerated(pageNumber, pixmap);
            m_totalGenerated++;
        } else {
            qWarning() << "ThumbnailGenerator: job returned null for page"
                       << pageNumber;
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
        emit activeJobsChanged(static_cast<int>(m_activeJobs.size()));
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
    QHash<int, GenerationJob*> jobsToCleanup;

    // DEADLOCK FIX: Copy jobs to cleanup outside of mutex to minimize lock time
    {
        QMutexLocker locker(&m_jobsMutex);
        jobsToCleanup = m_activeJobs;
        m_activeJobs.clear();
        emit activeJobsChanged(0);
    }

    // Cleanup jobs outside of mutex to prevent deadlocks during
    // waitForFinished()
    for (auto it = jobsToCleanup.begin(); it != jobsToCleanup.end(); ++it) {
        GenerationJob* job = it.value();
        if (job && job->watcher) {
            job->watcher->cancel();
            job->watcher->waitForFinished();  // Wait for completion
        }
        delete job;  // 手动删除
    }
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
        emit queueSizeChanged(static_cast<int>(m_requestQueue.size()));

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

    if (m_document == nullptr) {
        return {};
    }

    try {
        std::unique_ptr<Poppler::Page> page(
            m_document->page(request.pageNumber));
        if (page == nullptr) {
            return {};
        }

        // 检查压缩缓存
        if (m_compressionEnabled) {
            QByteArray* cachedData = m_compressedCache.object(request.cacheKey);
            if (cachedData != nullptr) {
                QPixmap cachedPixmap = decompressPixmap(*cachedData);
                if (!cachedPixmap.isNull()) {
                    // 记录缓存访问以更新LRU/LFU统计
                    recordCacheAccess(request.cacheKey);
                    return cachedPixmap;
                }
            }
        }

        QPixmap result;

        // 根据渲染模式选择渲染方法
        switch (request.preferredMode) {
            case RenderMode::GpuAccelerated:
                if (m_gpuAccelerationAvailable) {
                    result = renderPageToPixmapGpu(page.get(), request.size,
                                                   request.quality);
                    if (!result.isNull()) {
                        break;
                    }
                }
                // 如果GPU渲染失败，回退到CPU渲染
                [[fallthrough]];

            case RenderMode::CpuOnly:
            case RenderMode::Hybrid:
            default:
                result = renderPageToPixmap(page.get(), request.size,
                                            request.quality);
                break;
        }

        // 如果启用压缩，缓存压缩后的数据
        if (m_compressionEnabled && !result.isNull()) {
            QByteArray compressedData = compressPixmap(result);
            if (!compressedData.isEmpty()) {
                // 更新缓存元数据用于LRU/LFU/Adaptive策略
                updateCacheMetadata(request.cacheKey, compressedData.size());
                m_compressedCache.insert(request.cacheKey,
                                         new QByteArray(compressedData));
            }
        }

        return result;

    } catch (const std::exception& e) {
        qWarning() << "Exception in generatePixmap:" << e.what();
        return {};
    } catch (...) {
        qWarning() << "Unknown exception in generatePixmap";
        return {};
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
    if (page == nullptr) {
        return {};
    }

    try {
        QSizeF pageSize = page->pageSizeF();
        double dpi = getCachedDPI(size, pageSize, quality);

        // 尝试从内存池获取缓冲区以减少内存分配开销
        MemoryPoolEntry* poolEntry = acquireMemoryPoolEntry(size);

        // 渲染页面 - 直接渲染到目标尺寸附近以减少缩放
        // Use SafePDFRenderer
        QImage image = SafePDFRendering::renderPage(page, dpi);

        if (image.isNull()) {
            if (poolEntry) {
                releaseMemoryPoolEntry(poolEntry);
            }
            return {};
        }

        // 优化缩放操作
        if (image.size() != size) {
            Qt::TransformationMode mode =
                getOptimalTransformationMode(image.size(), size);

            // 如果有内存池条目，使用预分配的缓冲区进行缩放
            if (poolEntry &&
                poolEntry->data.size() >= static_cast<qint64>(size.width()) *
                                              size.height() * BYTES_PER_PIXEL) {
                // 使用内存池缓冲区创建目标图像
                QImage scaledImage(
                    reinterpret_cast<uchar*>(poolEntry->data.data()),
                    size.width(), size.height(), size.width() * BYTES_PER_PIXEL,
                    QImage::Format_ARGB32_Premultiplied);

                // 绘制缩放后的图像
                QPainter painter(&scaledImage);
                painter.setRenderHint(QPainter::SmoothPixmapTransform,
                                      mode == Qt::SmoothTransformation);
                QImage scaledSrc =
                    image.scaled(size, Qt::KeepAspectRatio, mode);

                // 居中绘制
                int xOffset = (size.width() - scaledSrc.width()) / 2;
                int yOffset = (size.height() - scaledSrc.height()) / 2;
                scaledImage.fill(Qt::transparent);
                painter.drawImage(xOffset, yOffset, scaledSrc);
                painter.end();

                // 复制结果（因为内存池缓冲区会被复用）
                QPixmap result = QPixmap::fromImage(scaledImage.copy());
                releaseMemoryPoolEntry(poolEntry);
                return result;
            }

            image = image.scaled(size, Qt::KeepAspectRatio, mode);
        }

        if (poolEntry) {
            releaseMemoryPoolEntry(poolEntry);
        }

        return QPixmap::fromImage(image);

    } catch (const std::exception& e) {
        qWarning() << "Exception in renderPageToPixmapOptimized:" << e.what();
        return {};
    } catch (...) {
        qWarning() << "Unknown exception in renderPageToPixmapOptimized";
        return {};
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
    auto cacheIter = m_dpiCache.find(cacheKey);
    if (cacheIter != m_dpiCache.end()) {
        return cacheIter.value();
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
        auto firstIter = m_dpiCache.begin();
        m_dpiCache.erase(firstIter);
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
            qDebug() << "Thumbnail generation stats:" << "Success rate:"
                     << QString::number(successRate, 'f', 1) << "%"
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
        qDebug() << "Slow thumbnail generation:" << "Page" << request.pageNumber
                 << "Size" << request.size << "Quality" << request.quality
                 << "Duration" << duration << "ms";
    }
}

// ============================================================================
// 新增的优化方法实现
// ============================================================================

void ThumbnailGenerator::setRenderMode(RenderMode mode) {
    if (m_renderMode != mode) {
        m_renderMode = mode;

        // 如果切换到GPU模式但GPU不可用，回退到CPU模式
        if (mode == RenderMode::GpuAccelerated && !m_gpuAccelerationAvailable) {
            m_renderMode = RenderMode::CpuOnly;
            qWarning() << "GPU acceleration not available, falling back to CPU "
                          "rendering";
        }
    }
}

void ThumbnailGenerator::setCacheStrategy(CacheStrategy strategy) {
    m_cacheStrategy = strategy;
}

void ThumbnailGenerator::setGpuAccelerationEnabled(bool enabled) {
    if (m_gpuAccelerationEnabled != enabled) {
        m_gpuAccelerationEnabled = enabled;

        if (enabled && !m_gpuAccelerationAvailable) {
            m_gpuAccelerationAvailable = initializeGpuContext();
        } else if (!enabled) {
            cleanupGpuContext();
            m_gpuAccelerationAvailable = false;
        }
    }
}

bool ThumbnailGenerator::isGpuAccelerationAvailable() const {
    return m_gpuAccelerationAvailable;
}

void ThumbnailGenerator::setMemoryPoolSize(qint64 size) {
    if (m_memoryPoolSize != size) {
        m_memoryPoolSize =
            qBound(static_cast<qint64>(16 * 1024 * 1024),  // 最小16MB
                   size,
                   static_cast<qint64>(512 * 1024 * 1024));  // 最大512MB

        // 如果新大小小于当前使用量，清理内存池
        if (m_memoryPoolUsage > m_memoryPoolSize) {
            cleanupMemoryPool();
        }
    }
}

qint64 ThumbnailGenerator::memoryPoolUsage() const {
    return m_memoryPoolUsage.load();
}

void ThumbnailGenerator::setCompressionEnabled(bool enabled) {
    m_compressionEnabled = enabled;
}

void ThumbnailGenerator::setCompressionQuality(int quality) {
    m_compressionQuality = qBound(1, quality, 100);
}

void ThumbnailGenerator::generateThumbnailBatch(const QList<int>& pageNumbers,
                                                const QSize& size,
                                                double quality) {
    if (!m_document || pageNumbers.isEmpty()) {
        return;
    }

    QList<GenerationRequest> requests;
    requests.reserve(pageNumbers.size());

    QSize actualSize = size.isValid() ? size : m_defaultSize;
    double actualQuality = (quality > 0) ? quality : m_defaultQuality;

    // 创建批处理请求
    for (int i = 0; i < pageNumbers.size(); ++i) {
        int pageNumber = pageNumbers[i];
        if (pageNumber >= 0 && pageNumber < m_document->numPages()) {
            GenerationRequest request(pageNumber, actualSize, actualQuality, i);
            requests.append(request);
        }
    }

    if (!requests.isEmpty()) {
        processBatchRequest(requests);
        if (!m_running) {
            start();
        }
        processQueue();
    }
}

bool ThumbnailGenerator::initializeGpuContext() {
    try {
        // 创建GPU渲染上下文
        m_gpuContext = std::make_unique<GpuRenderContext>();

        // 设置OpenGL表面格式
        QSurfaceFormat format;
        format.setVersion(3, 3);
        format.setProfile(QSurfaceFormat::CoreProfile);
        format.setRenderableType(QSurfaceFormat::OpenGL);
        format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);

        // 创建离屏表面
        m_gpuContext->surface = new QOffscreenSurface();
        m_gpuContext->surface->setFormat(format);
        m_gpuContext->surface->create();

        if (!m_gpuContext->surface->isValid()) {
            LOG_WARNING("Failed to create offscreen surface for GPU rendering");
            cleanupGpuContext();
            return false;
        }

        // 创建OpenGL上下文
        m_gpuContext->context = new QOpenGLContext();
        m_gpuContext->context->setFormat(format);

        if (!m_gpuContext->context->create()) {
            LOG_WARNING("Failed to create OpenGL context for GPU rendering");
            cleanupGpuContext();
            return false;
        }

        // 绑定上下文到表面
        if (!m_gpuContext->context->makeCurrent(m_gpuContext->surface)) {
            LOG_WARNING("Failed to make OpenGL context current");
            cleanupGpuContext();
            return false;
        }

        // 检查OpenGL版本
        QOpenGLFunctions* functions = m_gpuContext->context->functions();
        if (functions == nullptr) {
            LOG_WARNING("Failed to get OpenGL functions");
            cleanupGpuContext();
            return false;
        }

        // 初始化OpenGL函数
        functions->initializeOpenGLFunctions();

        // 创建帧缓冲对象用于渲染
        QOpenGLFramebufferObjectFormat fboFormat;
        fboFormat.setSamples(4);  // 4x MSAA
        fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);

        m_gpuContext->fbo = new QOpenGLFramebufferObject(
            DEFAULT_THUMBNAIL_WIDTH, DEFAULT_THUMBNAIL_HEIGHT, fboFormat);

        if (!m_gpuContext->fbo->isValid()) {
            LOG_WARNING("Failed to create framebuffer object");
            cleanupGpuContext();
            return false;
        }

        m_gpuContext->context->doneCurrent();
        m_gpuContext->isValid = true;

        LOG_INFO("GPU acceleration initialized successfully");
        return true;

    } catch (const std::exception& e) {
        LOG_WARNING("Exception during GPU context initialization: %s",
                    e.what());
        cleanupGpuContext();
        return false;
    } catch (...) {
        LOG_WARNING("Unknown exception during GPU context initialization");
        cleanupGpuContext();
        return false;
    }
}

void ThumbnailGenerator::cleanupGpuContext() {
    if (m_gpuContext != nullptr) {
        m_gpuContext->cleanup();
        m_gpuContext.reset();
    }
}

QPixmap ThumbnailGenerator::renderPageToPixmapGpu(Poppler::Page* page,
                                                  const QSize& size,
                                                  double quality) {
    if (page == nullptr) {
        return {};
    }

    // 检查GPU上下文是否可用
    if (!m_gpuContext || !m_gpuContext->isValid) {
        // 回退到CPU渲染
        return renderPageToPixmapOptimized(page, size, quality);
    }

    try {
        // 绑定OpenGL上下文
        if (!m_gpuContext->context->makeCurrent(m_gpuContext->surface)) {
            LOG_WARNING(
                "Failed to make GPU context current, falling back to CPU");
            return renderPageToPixmapOptimized(page, size, quality);
        }

        // 确保FBO大小匹配
        if (m_gpuContext->fbo->size() != size) {
            delete m_gpuContext->fbo;
            QOpenGLFramebufferObjectFormat fboFormat;
            fboFormat.setSamples(4);
            fboFormat.setAttachment(
                QOpenGLFramebufferObject::CombinedDepthStencil);
            m_gpuContext->fbo = new QOpenGLFramebufferObject(size, fboFormat);

            if (!m_gpuContext->fbo->isValid()) {
                m_gpuContext->context->doneCurrent();
                return renderPageToPixmapOptimized(page, size, quality);
            }
        }

        // 绑定FBO
        m_gpuContext->fbo->bind();

        // 使用Poppler渲染到QImage
        double dpi = calculateOptimalDPI(size, page->pageSizeF(), quality);
        QImage cpuImage = SafePDFRendering::renderPage(page, dpi);

        if (cpuImage.isNull()) {
            m_gpuContext->fbo->release();
            m_gpuContext->context->doneCurrent();
            return {};
        }

        // 使用QPainter在GPU加速的FBO上绘制
        QOpenGLPaintDevice paintDevice(size);
        QPainter painter(&paintDevice);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // 缩放并绘制
        QImage scaledImage = cpuImage.scaled(size, Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation);
        int xOffset = (size.width() - scaledImage.width()) / 2;
        int yOffset = (size.height() - scaledImage.height()) / 2;

        painter.fillRect(QRect(QPoint(0, 0), size), Qt::white);
        painter.drawImage(xOffset, yOffset, scaledImage);
        painter.end();

        // 从FBO读取结果
        QImage result = m_gpuContext->fbo->toImage();

        m_gpuContext->fbo->release();
        m_gpuContext->context->doneCurrent();

        return QPixmap::fromImage(result);

    } catch (const std::exception& e) {
        LOG_WARNING("GPU rendering failed: %s, falling back to CPU", e.what());
        if (m_gpuContext && m_gpuContext->context) {
            m_gpuContext->context->doneCurrent();
        }
        return renderPageToPixmapOptimized(page, size, quality);
    } catch (...) {
        LOG_WARNING(
            "GPU rendering failed with unknown error, falling back to CPU");
        if (m_gpuContext && m_gpuContext->context) {
            m_gpuContext->context->doneCurrent();
        }
        return renderPageToPixmapOptimized(page, size, quality);
    }
}

ThumbnailGenerator::MemoryPoolEntry* ThumbnailGenerator::acquireMemoryPoolEntry(
    const QSize& size) {
    QMutexLocker locker(&m_memoryPoolMutex);

    qint64 requiredSize = static_cast<qint64>(size.width()) * size.height() *
                          BYTES_PER_PIXEL;  // RGBA

    // 查找可用的内存池条目
    for (MemoryPoolEntry* entry : m_memoryPool) {
        if (entry != nullptr && !entry->inUse &&
            entry->data.size() >= requiredSize) {
            entry->inUse = true;
            entry->lastUsed = QDateTime::currentMSecsSinceEpoch();
            entry->size = size;
            return entry;
        }
    }

    // 如果没有找到合适的条目，创建新的
    if (m_memoryPoolUsage + requiredSize <= m_memoryPoolSize) {
        auto* entry = new MemoryPoolEntry();
        entry->data.resize(requiredSize);
        entry->inUse = true;
        entry->lastUsed = QDateTime::currentMSecsSinceEpoch();
        entry->size = size;

        m_memoryPool.append(entry);
        m_memoryPoolUsage += requiredSize;

        return entry;
    }

    return nullptr;  // 内存池已满
}

void ThumbnailGenerator::releaseMemoryPoolEntry(MemoryPoolEntry* entry) {
    if (entry) {
        QMutexLocker locker(&m_memoryPoolMutex);
        entry->inUse = false;
    }
}

void ThumbnailGenerator::cleanupMemoryPool() {
    QMutexLocker locker(&m_memoryPoolMutex);

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    const qint64 MAX_AGE = MEMORY_POOL_ENTRY_AGE_MS;  // 5分钟

    auto poolIter = m_memoryPool.begin();
    while (poolIter != m_memoryPool.end()) {
        MemoryPoolEntry* entry = *poolIter;

        // 清理未使用且过期的条目
        if (!entry->inUse && (currentTime - entry->lastUsed) > MAX_AGE) {
            m_memoryPoolUsage -= entry->data.size();
            delete entry;
            poolIter = m_memoryPool.erase(poolIter);
        } else {
            ++poolIter;
        }
    }
}

QByteArray ThumbnailGenerator::compressPixmap(const QPixmap& pixmap) {
    if (!m_compressionEnabled || pixmap.isNull()) {
        return QByteArray();
    }

    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);

    QImageWriter writer(&buffer, "JPEG");
    writer.setQuality(m_compressionQuality);

    if (writer.write(pixmap.toImage())) {
        return data;
    }

    return QByteArray();
}

QPixmap ThumbnailGenerator::decompressPixmap(const QByteArray& data) {
    if (data.isEmpty()) {
        return {};
    }

    QPixmap pixmap;
    if (pixmap.loadFromData(data, "JPEG")) {
        return pixmap;
    }

    return {};
}

void ThumbnailGenerator::processBatchRequest(
    const QList<GenerationRequest>& requests) {
    if (requests.isEmpty()) {
        return;
    }

    QList<GenerationRequest> optimizedRequests = requests;
    optimizeBatchOrder(optimizedRequests);

    // 将批处理请求添加到队列
    QMutexLocker locker(&m_queueMutex);
    for (const GenerationRequest& request : optimizedRequests) {
        m_requestQueue.enqueue(request);
    }

    emit queueSizeChanged(static_cast<int>(m_requestQueue.size()));
    m_queueCondition.wakeAll();  // 唤醒所有等待的线程
}

void ThumbnailGenerator::optimizeBatchOrder(
    QList<GenerationRequest>& requests) {
    // 按页码排序以优化磁盘访问模式
    std::sort(requests.begin(), requests.end(),
              [](const GenerationRequest& firstRequest,
                 const GenerationRequest& secondRequest) {
                  return firstRequest.pageNumber < secondRequest.pageNumber;
              });

    // 根据缓存策略调整优先级
    if (m_cacheStrategy == CacheStrategy::MemoryAware) {
        // 内存感知策略：优先处理小尺寸的缩略图
        std::stable_sort(
            requests.begin(), requests.end(),
            [](const GenerationRequest& firstRequest,
               const GenerationRequest& secondRequest) {
                qint64 sizeA = static_cast<qint64>(firstRequest.size.width()) *
                               firstRequest.size.height();
                qint64 sizeB = static_cast<qint64>(secondRequest.size.width()) *
                               secondRequest.size.height();
                return sizeA < sizeB;
            });
    }
}

// ============================================================================
// 缓存策略实现
// ============================================================================

void ThumbnailGenerator::updateCacheMetadata(const QString& key, qint64 size) {
    QMutexLocker locker(&m_cacheMetadataMutex);

    if (m_cacheMetadata.contains(key)) {
        // 更新已存在的条目
        CacheEntryMetadata& metadata = m_cacheMetadata[key];
        metadata.lastAccessTime = QDateTime::currentMSecsSinceEpoch();
        metadata.accessCount++;
        metadata.priority = calculateAdaptivePriority(metadata);
    } else {
        // 创建新条目
        CacheEntryMetadata metadata(key, size);
        metadata.priority = calculateAdaptivePriority(metadata);
        m_cacheMetadata[key] = metadata;
        m_currentCacheSize += size;

        // 检查是否需要驱逐
        if (m_currentCacheSize > m_maxCacheSize) {
            locker.unlock();
            evictCacheEntries(m_currentCacheSize - m_maxCacheSize);
        }
    }
}

void ThumbnailGenerator::recordCacheAccess(const QString& key) {
    QMutexLocker locker(&m_cacheMetadataMutex);

    auto it = m_cacheMetadata.find(key);
    if (it != m_cacheMetadata.end()) {
        it->lastAccessTime = QDateTime::currentMSecsSinceEpoch();
        it->accessCount++;
        it->priority = calculateAdaptivePriority(*it);
    }
}

QString ThumbnailGenerator::selectEvictionCandidate() const {
    QMutexLocker locker(&m_cacheMetadataMutex);

    if (m_cacheMetadata.isEmpty()) {
        return QString();
    }

    QString candidate;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    switch (m_cacheStrategy) {
        case CacheStrategy::Lru: {
            // LRU: 选择最久未访问的条目
            qint64 oldestTime = std::numeric_limits<qint64>::max();
            for (auto it = m_cacheMetadata.constBegin();
                 it != m_cacheMetadata.constEnd(); ++it) {
                if (it->lastAccessTime < oldestTime) {
                    oldestTime = it->lastAccessTime;
                    candidate = it->key;
                }
            }
            break;
        }

        case CacheStrategy::Lfu: {
            // LFU: 选择访问次数最少的条目
            int minCount = std::numeric_limits<int>::max();
            for (auto it = m_cacheMetadata.constBegin();
                 it != m_cacheMetadata.constEnd(); ++it) {
                if (it->accessCount < minCount) {
                    minCount = it->accessCount;
                    candidate = it->key;
                }
            }
            break;
        }

        case CacheStrategy::Adaptive: {
            // Adaptive: 基于综合优先级选择
            int lowestPriority = std::numeric_limits<int>::max();
            for (auto it = m_cacheMetadata.constBegin();
                 it != m_cacheMetadata.constEnd(); ++it) {
                if (it->priority < lowestPriority) {
                    lowestPriority = it->priority;
                    candidate = it->key;
                }
            }
            break;
        }

        case CacheStrategy::MemoryAware: {
            // MemoryAware: 优先驱逐大的、不常用的条目
            qint64 worstScore = std::numeric_limits<qint64>::max();
            for (auto it = m_cacheMetadata.constBegin();
                 it != m_cacheMetadata.constEnd(); ++it) {
                // 计算得分：(size * age) / accessCount
                // 更高的得分意味着更应该被驱逐
                qint64 age = currentTime - it->lastAccessTime;
                qint64 score = (it->size * age) / qMax(1, it->accessCount);
                // 我们寻找最高得分来驱逐，但这里用worstScore表示
                // 反转逻辑：寻找得分最高（最差）的条目
                if (worstScore == std::numeric_limits<qint64>::max() ||
                    score > worstScore) {
                    worstScore = score;
                    candidate = it->key;
                }
            }
            break;
        }
    }

    return candidate;
}

void ThumbnailGenerator::evictCacheEntries(qint64 requiredSpace) {
    qint64 freedSpace = 0;

    while (freedSpace < requiredSpace) {
        QString candidate = selectEvictionCandidate();
        if (candidate.isEmpty()) {
            break;
        }

        // 从压缩缓存中移除
        m_compressedCache.remove(candidate);

        // 更新元数据
        QMutexLocker locker(&m_cacheMetadataMutex);
        auto it = m_cacheMetadata.find(candidate);
        if (it != m_cacheMetadata.end()) {
            freedSpace += it->size;
            m_currentCacheSize -= it->size;
            m_cacheMetadata.erase(it);
        }
    }

    LOG_DEBUG("ThumbnailGenerator: Evicted cache entries, freed %lld bytes",
              freedSpace);
}

int ThumbnailGenerator::calculateAdaptivePriority(
    const CacheEntryMetadata& metadata) const {
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 age = currentTime - metadata.lastAccessTime;

    // 综合考虑访问频率和新近度
    // 公式: priority = accessCount * recencyWeight - sizeWeight
    // 高优先级 = 更多访问 + 更近的访问 - 更小的尺寸

    // 新近度权重：最近1分钟内的访问获得高分
    int recencyScore = 0;
    if (age < 60000) {  // 1分钟
        recencyScore = 100;
    } else if (age < 300000) {  // 5分钟
        recencyScore = 50;
    } else if (age < 600000) {  // 10分钟
        recencyScore = 20;
    }

    // 访问频率得分
    int frequencyScore = qMin(metadata.accessCount * 10, 100);

    // 尺寸惩罚：较大的条目优先级降低
    int sizePenalty =
        static_cast<int>(metadata.size / (1024 * 10));  // 每10KB扣1分
    sizePenalty = qMin(sizePenalty, 50);

    return recencyScore + frequencyScore - sizePenalty;
}
