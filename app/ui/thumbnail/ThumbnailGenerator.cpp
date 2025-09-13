#include "ThumbnailGenerator.h"
#include <QApplication>
#include <QDebug>
#include <QDateTime>
#include <QMutexLocker>
#include <QtConcurrent/QtConcurrent>
#include <QThread>
#include <QHash>
#include <QQueue>
#include <QTimer>
#include <QImageWriter>
#include <QBuffer>
#include <poppler-qt6.h>
#include <cmath>
#include <algorithm>
#include "../../utils/LoggingMacros.h"

ThumbnailGenerator::ThumbnailGenerator(QObject* parent)
    : QObject(parent)
    , m_defaultSize(DEFAULT_THUMBNAIL_WIDTH, DEFAULT_THUMBNAIL_HEIGHT)
    , m_defaultQuality(DEFAULT_QUALITY)
    , m_maxConcurrentJobs(DEFAULT_MAX_CONCURRENT_JOBS)
    , m_maxRetries(DEFAULT_MAX_RETRIES)
    , m_renderMode(RenderMode::HYBRID)
    , m_cacheStrategy(CacheStrategy::ADAPTIVE)
    , m_gpuAccelerationEnabled(true)
    , m_gpuAccelerationAvailable(false)
    , m_memoryPoolSize(DEFAULT_MEMORY_POOL_SIZE)
    , m_compressionEnabled(true)
    , m_compressionQuality(DEFAULT_COMPRESSION_QUALITY)
    , m_running(false)
    , m_paused(false)
    , m_batchSize(DEFAULT_BATCH_SIZE)
    , m_batchInterval(DEFAULT_BATCH_INTERVAL)
    , m_totalGenerated(0)
    , m_totalErrors(0)
    , m_totalTime(0)
{
    // 初始化压缩缓存
    m_compressedCache.setMaxCost(COMPRESSED_CACHE_SIZE);

    initializeGenerator();

    // 检查GPU加速可用性
    if (m_gpuAccelerationEnabled) {
        m_gpuAccelerationAvailable = initializeGpuContext();
        if (!m_gpuAccelerationAvailable) {
            LOG_WARNING("GPU acceleration not available, falling back to CPU rendering");
        }
    }
}

ThumbnailGenerator::~ThumbnailGenerator()
{
    stop();
    cleanupJobs();
    cleanupGpuContext();
    cleanupMemoryPool();
}

void ThumbnailGenerator::initializeGenerator()
{
    // 创建批处理定时器
    m_batchTimer = new QTimer(this);
    m_batchTimer->setInterval(m_batchInterval);
    m_batchTimer->setSingleShot(false);
    connect(m_batchTimer, &QTimer::timeout, this, &ThumbnailGenerator::onBatchTimer);
    
    // 创建队列处理定时器
    QTimer* queueTimer = new QTimer(this);
    queueTimer->setInterval(QUEUE_PROCESS_INTERVAL);
    queueTimer->setSingleShot(false);
    connect(queueTimer, &QTimer::timeout, this, &ThumbnailGenerator::processQueue);
    queueTimer->start();
}

void ThumbnailGenerator::setDocument(std::shared_ptr<Poppler::Document> document)
{
    // DEADLOCK FIX: Avoid nested mutex calls by clearing queues/jobs before acquiring document mutex
    // This prevents the lock ordering: documentMutex -> queueMutex -> jobsMutex

    // First, stop all operations without holding document mutex
    clearQueue();
    cleanupJobs();

    // Now safely acquire document mutex and set the new document
    QMutexLocker locker(&m_documentMutex);
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

std::shared_ptr<Poppler::Document> ThumbnailGenerator::document() const
{
    QMutexLocker locker(&m_documentMutex);
    return m_document;
}

void ThumbnailGenerator::setThumbnailSize(const QSize& size)
{
    if (size.isValid() && m_defaultSize != size) {
        m_defaultSize = size;
        
        // 清除队列中使用默认尺寸的请求
        QMutexLocker locker(&m_queueMutex);
        QQueue<GenerationRequest> newQueue;
        
        while (!m_requestQueue.isEmpty()) {
            GenerationRequest req = m_requestQueue.dequeue();
            if (req.size != QSize(DEFAULT_THUMBNAIL_WIDTH, DEFAULT_THUMBNAIL_HEIGHT)) {
                newQueue.enqueue(req);
            }
        }
        
        m_requestQueue = newQueue;
        emit queueSizeChanged(m_requestQueue.size());
    }
}

void ThumbnailGenerator::setQuality(double quality)
{
    m_defaultQuality = qBound(0.1, quality, 3.0);
}

void ThumbnailGenerator::setMaxConcurrentJobs(int maxJobs)
{
    m_maxConcurrentJobs = qBound(1, maxJobs, 8);

    // DEADLOCK FIX: Replace busy-wait loop with proper signaling
    // The old busy-wait loop could cause deadlocks if called from main thread while holding locks
    // Instead, we'll let the natural job completion process handle the reduction

    // If we're reducing concurrent jobs, the processQueue() method will naturally
    // respect the new limit when starting new jobs. No need to forcefully wait here.
    // This prevents potential deadlocks and improves responsiveness.
}

void ThumbnailGenerator::setMaxRetries(int maxRetries)
{
    m_maxRetries = qBound(0, maxRetries, 5);
}

void ThumbnailGenerator::generateThumbnail(int pageNumber, const QSize& size,
                                          double quality, int priority)
{
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
        return; // Already being processed
    }

    QMutexLocker queueLocker(&m_queueMutex);

    // 检查是否已经在队列中
    for (const auto& existing : m_requestQueue) {
        if (existing.pageNumber == pageNumber &&
            existing.size == actualSize &&
            qAbs(existing.quality - actualQuality) < 0.001) {
            return; // 已存在相同请求
        }
    }

    m_requestQueue.enqueue(request);
    std::sort(m_requestQueue.begin(), m_requestQueue.end());

    emit queueSizeChanged(m_requestQueue.size());
    m_queueCondition.wakeOne();
}

void ThumbnailGenerator::generateThumbnailRange(int startPage, int endPage, 
                                               const QSize& size, double quality)
{
    if (!m_document) return;
    
    int numPages = m_document->numPages();
    startPage = qBound(0, startPage, numPages - 1);
    endPage = qBound(startPage, endPage, numPages - 1);
    
    for (int i = startPage; i <= endPage; ++i) {
        generateThumbnail(i, size, quality, i - startPage); // 按顺序设置优先级
    }
}

void ThumbnailGenerator::clearQueue()
{
    QMutexLocker locker(&m_queueMutex);
    m_requestQueue.clear();
    emit queueSizeChanged(0);
}

void ThumbnailGenerator::cancelRequest(int pageNumber)
{
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
        delete job; // 手动删除
        m_activeJobs.erase(it);
        emit activeJobsChanged(m_activeJobs.size());
    }
}

void ThumbnailGenerator::setPriority(int pageNumber, int priority)
{
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

bool ThumbnailGenerator::isGenerating(int pageNumber) const
{
    QMutexLocker locker(&m_jobsMutex);
    return m_activeJobs.contains(pageNumber);
}

int ThumbnailGenerator::queueSize() const
{
    QMutexLocker locker(&m_queueMutex);
    return m_requestQueue.size();
}

int ThumbnailGenerator::activeJobCount() const
{
    QMutexLocker locker(&m_jobsMutex);
    return m_activeJobs.size();
}

void ThumbnailGenerator::pause()
{
    m_paused = true;
}

void ThumbnailGenerator::resume()
{
    m_paused = false;
    m_queueCondition.wakeAll();
}

void ThumbnailGenerator::stop()
{
    m_running = false;
    m_paused = false;

    // DEADLOCK FIX: Stop timer first to prevent new operations
    if (m_batchTimer) {
        m_batchTimer->stop();
    }

    // Clear operations in safe order
    clearQueue();
    cleanupJobs();
}

void ThumbnailGenerator::start()
{
    m_running = true;
    m_paused = false;
    
    if (m_batchTimer) {
        m_batchTimer->start();
    }
}

void ThumbnailGenerator::processQueue()
{
    if (!m_running || m_paused || !m_document) {
        return;
    }
    
    // 启动新任务直到达到并发限制
    while (activeJobCount() < m_maxConcurrentJobs && queueSize() > 0) {
        startNextJob();
    }
}

void ThumbnailGenerator::startNextJob()
{
    GenerationRequest request;
    bool hasRequest = false;

    // DEADLOCK FIX: Minimize mutex scope and avoid nested mutex calls
    {
        QMutexLocker queueLocker(&m_queueMutex);
        if (!m_requestQueue.isEmpty()) {
            request = m_requestQueue.dequeue();
            hasRequest = true;
            emit queueSizeChanged(m_requestQueue.size());
        }
    }

    if (!hasRequest) {
        return;
    }

    // Check if already generating (separate mutex scope)
    {
        QMutexLocker jobsLocker(&m_jobsMutex);
        if (m_activeJobs.contains(request.pageNumber)) {
            return; // Already being processed
        }
    }

    // 创建新任务
    auto job = std::make_unique<GenerationJob>();
    job->request = request;
    job->watcher = new QFutureWatcher<QPixmap>();

    connect(job->watcher, &QFutureWatcher<QPixmap>::finished,
            this, &ThumbnailGenerator::onGenerationFinished);

    // 启动异步生成
    job->future = QtConcurrent::run([this, request]() {
        return generatePixmap(request);
    });

    job->watcher->setFuture(job->future);

    // 添加到活动任务 (separate mutex scope)
    {
        QMutexLocker jobsLocker(&m_jobsMutex);
        m_activeJobs[request.pageNumber] = job.release(); // 转移所有权
        emit activeJobsChanged(m_activeJobs.size());
    }
}

void ThumbnailGenerator::onGenerationFinished()
{
    QFutureWatcher<QPixmap>* watcher = static_cast<QFutureWatcher<QPixmap>*>(sender());
    if (!watcher) return;

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
        delete m_activeJobs.take(pageNumber); // 删除并移除
        emit activeJobsChanged(m_activeJobs.size());
    }

    watcher->deleteLater();
}

void ThumbnailGenerator::onBatchTimer()
{
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

void ThumbnailGenerator::cleanupJobs()
{
    QHash<int, GenerationJob*> jobsToCleanup;

    // DEADLOCK FIX: Copy jobs to cleanup outside of mutex to minimize lock time
    {
        QMutexLocker locker(&m_jobsMutex);
        jobsToCleanup = m_activeJobs;
        m_activeJobs.clear();
        emit activeJobsChanged(0);
    }

    // Cleanup jobs outside of mutex to prevent deadlocks during waitForFinished()
    for (auto it = jobsToCleanup.begin(); it != jobsToCleanup.end(); ++it) {
        GenerationJob* job = it.value();
        if (job && job->watcher) {
            job->watcher->cancel();
            job->watcher->waitForFinished(); // Wait for completion
        }
        delete job; // 手动删除
    }
}

void ThumbnailGenerator::handleJobCompletion(GenerationJob* job)
{
    qint64 duration = QDateTime::currentMSecsSinceEpoch() - job->request.timestamp;
    logPerformance(job->request, duration);
    m_totalTime += duration;
}

void ThumbnailGenerator::handleJobError(GenerationJob* job, const QString& error)
{
    m_totalErrors++;

    // 检查是否需要重试
    if (job->request.retryCount < m_maxRetries) {
        job->request.retryCount++;
        job->request.timestamp = QDateTime::currentMSecsSinceEpoch();
        job->request.priority += 10; // 降低优先级

        // 重新加入队列
        QMutexLocker locker(&m_queueMutex);
        m_requestQueue.enqueue(job->request);
        std::sort(m_requestQueue.begin(), m_requestQueue.end());
        emit queueSizeChanged(m_requestQueue.size());

        qDebug() << "Retrying thumbnail generation for page" << job->request.pageNumber
                 << "attempt" << job->request.retryCount;
    } else {
        emit thumbnailError(job->request.pageNumber, error);
        qWarning() << "Failed to generate thumbnail for page" << job->request.pageNumber
                   << "after" << m_maxRetries << "retries:" << error;
    }
}

QPixmap ThumbnailGenerator::generatePixmap(const GenerationRequest& request)
{
    QMutexLocker locker(&m_documentMutex);

    if (!m_document) {
        return QPixmap();
    }

    try {
        std::unique_ptr<Poppler::Page> page(m_document->page(request.pageNumber));
        if (!page) {
            return QPixmap();
        }

        // 检查压缩缓存
        if (m_compressionEnabled) {
            QByteArray* cachedData = m_compressedCache.object(request.cacheKey);
            if (cachedData) {
                QPixmap cachedPixmap = decompressPixmap(*cachedData);
                if (!cachedPixmap.isNull()) {
                    return cachedPixmap;
                }
            }
        }

        QPixmap result;

        // 根据渲染模式选择渲染方法
        switch (request.preferredMode) {
            case RenderMode::GPU_ACCELERATED:
                if (m_gpuAccelerationAvailable) {
                    result = renderPageToPixmapGpu(page.get(), request.size, request.quality);
                    if (!result.isNull()) break;
                }
                // 如果GPU渲染失败，回退到CPU渲染
                [[fallthrough]];

            case RenderMode::CPU_ONLY:
            case RenderMode::HYBRID:
            default:
                result = renderPageToPixmap(page.get(), request.size, request.quality);
                break;
        }

        // 如果启用压缩，缓存压缩后的数据
        if (m_compressionEnabled && !result.isNull()) {
            QByteArray compressedData = compressPixmap(result);
            if (!compressedData.isEmpty()) {
                m_compressedCache.insert(request.cacheKey, new QByteArray(compressedData));
            }
        }

        return result;

    } catch (const std::exception& e) {
        qWarning() << "Exception in generatePixmap:" << e.what();
        return QPixmap();
    } catch (...) {
        qWarning() << "Unknown exception in generatePixmap";
        return QPixmap();
    }
}

QPixmap ThumbnailGenerator::renderPageToPixmap(Poppler::Page* page, const QSize& size, double quality)
{
    // 使用优化版本
    return renderPageToPixmapOptimized(page, size, quality);
}

QPixmap ThumbnailGenerator::renderPageToPixmapOptimized(Poppler::Page* page, const QSize& size, double quality)
{
    if (!page) {
        return QPixmap();
    }

    try {
        QSizeF pageSize = page->pageSizeF();
        double dpi = getCachedDPI(size, pageSize, quality);

        // 渲染页面 - 直接渲染到目标尺寸附近以减少缩放
        QImage image = page->renderToImage(dpi, dpi, -1, -1, -1, -1, Poppler::Page::Rotate0);

        if (image.isNull()) {
            return QPixmap();
        }

        // 优化缩放操作
        if (image.size() != size) {
            Qt::TransformationMode mode = getOptimalTransformationMode(image.size(), size);
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

double ThumbnailGenerator::calculateOptimalDPI(const QSize& targetSize, const QSizeF& pageSize, double quality)
{
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

double ThumbnailGenerator::getCachedDPI(const QSize& targetSize, const QSizeF& pageSize, double quality)
{
    QString cacheKey = QString("%1x%2_%3x%4_%5")
                      .arg(targetSize.width()).arg(targetSize.height())
                      .arg(static_cast<int>(pageSize.width())).arg(static_cast<int>(pageSize.height()))
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

void ThumbnailGenerator::cacheDPI(const QSize& targetSize, const QSizeF& pageSize, double quality, double dpi)
{
    QString cacheKey = QString("%1x%2_%3x%4_%5")
                      .arg(targetSize.width()).arg(targetSize.height())
                      .arg(static_cast<int>(pageSize.width())).arg(static_cast<int>(pageSize.height()))
                      .arg(static_cast<int>(quality * 100));

    QMutexLocker locker(&m_dpiCacheMutex);
    m_dpiCache[cacheKey] = dpi;

    // 限制缓存大小
    if (m_dpiCache.size() > 100) {
        auto it = m_dpiCache.begin();
        m_dpiCache.erase(it);
    }
}

Qt::TransformationMode ThumbnailGenerator::getOptimalTransformationMode(const QSize& sourceSize, const QSize& targetSize)
{
    // 如果缩放比例较小或目标尺寸较小，使用快速变换
    double scaleRatio = qMin(static_cast<double>(targetSize.width()) / sourceSize.width(),
                            static_cast<double>(targetSize.height()) / sourceSize.height());

    if (scaleRatio > 0.8 || targetSize.width() <= 150) {
        return Qt::FastTransformation;
    }

    return Qt::SmoothTransformation;
}

void ThumbnailGenerator::updateStatistics()
{
    int totalRequests = m_totalGenerated + m_totalErrors;
    if (totalRequests > 0) {
        double successRate = (double)m_totalGenerated / totalRequests * 100.0;
        double avgTime = totalRequests > 0 ? (double)m_totalTime / totalRequests : 0.0;

        emit generationProgress(m_totalGenerated, totalRequests);

        // 定期输出统计信息
        static int logCounter = 0;
        if (++logCounter % 50 == 0) {
            qDebug() << "Thumbnail generation stats:"
                     << "Success rate:" << QString::number(successRate, 'f', 1) << "%"
                     << "Avg time:" << QString::number(avgTime, 'f', 1) << "ms"
                     << "Queue size:" << queueSize()
                     << "Active jobs:" << activeJobCount();
        }
    }
}

void ThumbnailGenerator::logPerformance(const GenerationRequest& request, qint64 duration)
{
    Q_UNUSED(request)

    // 记录性能数据用于优化
    if (duration > 1000) { // 超过1秒的任务
        qDebug() << "Slow thumbnail generation:"
                 << "Page" << request.pageNumber
                 << "Size" << request.size
                 << "Quality" << request.quality
                 << "Duration" << duration << "ms";
    }
}

// ============================================================================
// 新增的优化方法实现
// ============================================================================

void ThumbnailGenerator::setRenderMode(RenderMode mode)
{
    if (m_renderMode != mode) {
        m_renderMode = mode;

        // 如果切换到GPU模式但GPU不可用，回退到CPU模式
        if (mode == RenderMode::GPU_ACCELERATED && !m_gpuAccelerationAvailable) {
            m_renderMode = RenderMode::CPU_ONLY;
            qWarning() << "GPU acceleration not available, falling back to CPU rendering";
        }
    }
}

void ThumbnailGenerator::setCacheStrategy(CacheStrategy strategy)
{
    m_cacheStrategy = strategy;
}

void ThumbnailGenerator::setGpuAccelerationEnabled(bool enabled)
{
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

bool ThumbnailGenerator::isGpuAccelerationAvailable() const
{
    return m_gpuAccelerationAvailable;
}

void ThumbnailGenerator::setMemoryPoolSize(qint64 size)
{
    if (m_memoryPoolSize != size) {
        m_memoryPoolSize = qBound(static_cast<qint64>(16 * 1024 * 1024), // 最小16MB
                                  size,
                                  static_cast<qint64>(512 * 1024 * 1024)); // 最大512MB

        // 如果新大小小于当前使用量，清理内存池
        if (m_memoryPoolUsage > m_memoryPoolSize) {
            cleanupMemoryPool();
        }
    }
}

qint64 ThumbnailGenerator::memoryPoolUsage() const
{
    return m_memoryPoolUsage.load();
}

void ThumbnailGenerator::setCompressionEnabled(bool enabled)
{
    m_compressionEnabled = enabled;
}

void ThumbnailGenerator::setCompressionQuality(int quality)
{
    m_compressionQuality = qBound(1, quality, 100);
}

void ThumbnailGenerator::generateThumbnailBatch(const QList<int>& pageNumbers,
                                               const QSize& size, double quality)
{
    if (!m_document || pageNumbers.isEmpty()) return;

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
    }
}

bool ThumbnailGenerator::initializeGpuContext()
{
    // 简化的GPU初始化 - 在实际项目中需要完整的OpenGL上下文设置
    // 这里只是检查OpenGL是否可用
    try {
        // 检查OpenGL支持
        // 在实际实现中，这里会创建OpenGL上下文
        return false; // 暂时禁用GPU加速，直到完整实现
    } catch (...) {
        return false;
    }
}

void ThumbnailGenerator::cleanupGpuContext()
{
    if (m_gpuContext) {
        m_gpuContext->cleanup();
        m_gpuContext.reset();
    }
}

QPixmap ThumbnailGenerator::renderPageToPixmapGpu(Poppler::Page* page, const QSize& size, double quality)
{
    Q_UNUSED(page)
    Q_UNUSED(size)
    Q_UNUSED(quality)

    // GPU渲染实现 - 暂时返回空，需要完整的OpenGL实现
    return QPixmap();
}

ThumbnailGenerator::MemoryPoolEntry* ThumbnailGenerator::acquireMemoryPoolEntry(const QSize& size)
{
    QMutexLocker locker(&m_memoryPoolMutex);

    qint64 requiredSize = size.width() * size.height() * 4; // RGBA

    // 查找可用的内存池条目
    for (MemoryPoolEntry* entry : m_memoryPool) {
        if (!entry->inUse && entry->data.size() >= requiredSize) {
            entry->inUse = true;
            entry->lastUsed = QDateTime::currentMSecsSinceEpoch();
            entry->size = size;
            return entry;
        }
    }

    // 如果没有找到合适的条目，创建新的
    if (m_memoryPoolUsage + requiredSize <= m_memoryPoolSize) {
        MemoryPoolEntry* entry = new MemoryPoolEntry();
        entry->data.resize(requiredSize);
        entry->inUse = true;
        entry->lastUsed = QDateTime::currentMSecsSinceEpoch();
        entry->size = size;

        m_memoryPool.append(entry);
        m_memoryPoolUsage += requiredSize;

        return entry;
    }

    return nullptr; // 内存池已满
}

void ThumbnailGenerator::releaseMemoryPoolEntry(MemoryPoolEntry* entry)
{
    if (entry) {
        QMutexLocker locker(&m_memoryPoolMutex);
        entry->inUse = false;
    }
}

void ThumbnailGenerator::cleanupMemoryPool()
{
    QMutexLocker locker(&m_memoryPoolMutex);

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    const qint64 maxAge = 300000; // 5分钟

    auto it = m_memoryPool.begin();
    while (it != m_memoryPool.end()) {
        MemoryPoolEntry* entry = *it;

        // 清理未使用且过期的条目
        if (!entry->inUse && (currentTime - entry->lastUsed) > maxAge) {
            m_memoryPoolUsage -= entry->data.size();
            delete entry;
            it = m_memoryPool.erase(it);
        } else {
            ++it;
        }
    }
}

QByteArray ThumbnailGenerator::compressPixmap(const QPixmap& pixmap)
{
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

QPixmap ThumbnailGenerator::decompressPixmap(const QByteArray& data)
{
    if (data.isEmpty()) {
        return QPixmap();
    }

    QPixmap pixmap;
    if (pixmap.loadFromData(data, "JPEG")) {
        return pixmap;
    }

    return QPixmap();
}

void ThumbnailGenerator::processBatchRequest(const QList<GenerationRequest>& requests)
{
    if (requests.isEmpty()) return;

    QList<GenerationRequest> optimizedRequests = requests;
    optimizeBatchOrder(optimizedRequests);

    // 将批处理请求添加到队列
    QMutexLocker locker(&m_queueMutex);
    for (const GenerationRequest& request : optimizedRequests) {
        m_requestQueue.enqueue(request);
    }

    emit queueSizeChanged(m_requestQueue.size());
    m_queueCondition.wakeAll(); // 唤醒所有等待的线程
}

void ThumbnailGenerator::optimizeBatchOrder(QList<GenerationRequest>& requests)
{
    // 按页码排序以优化磁盘访问模式
    std::sort(requests.begin(), requests.end(),
              [](const GenerationRequest& a, const GenerationRequest& b) {
                  return a.pageNumber < b.pageNumber;
              });

    // 根据缓存策略调整优先级
    if (m_cacheStrategy == CacheStrategy::MEMORY_AWARE) {
        // 内存感知策略：优先处理小尺寸的缩略图
        std::stable_sort(requests.begin(), requests.end(),
                        [](const GenerationRequest& a, const GenerationRequest& b) {
                            qint64 sizeA = a.size.width() * a.size.height();
                            qint64 sizeB = b.size.width() * b.size.height();
                            return sizeA < sizeB;
                        });
    }
}
