#pragma once

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QPixmap>
#include <QSize>
#include <QTimer>
#include <QFuture>
#include <QFutureWatcher>
#include <QDateTime>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QThreadPool>
#include <QRunnable>
#include <QAtomicInt>
#include <QElapsedTimer>
#include <QCache>
#include <QBuffer>
#include <QImageWriter>
#include <poppler/qt6/poppler-qt6.h>
#include <memory>

/**
 * @brief 异步PDF缩略图生成器
 * 
 * 特性：
 * - 多线程异步生成
 * - 优先级队列管理
 * - 智能批处理
 * - 内存使用优化
 * - 错误处理和重试机制
 * - 与现有PDF渲染系统集成
 */
class ThumbnailGenerator : public QObject
{
    Q_OBJECT

public:
    enum class RenderMode {
        CPU_ONLY,           // 仅使用CPU渲染
        GPU_ACCELERATED,    // GPU加速渲染
        HYBRID             // 混合模式：根据任务选择
    };

    enum class CacheStrategy {
        LRU,               // 最近最少使用
        LFU,               // 最少使用频率
        ADAPTIVE,          // 自适应策略
        MEMORY_AWARE       // 内存感知策略
    };

    struct GenerationRequest {
        int pageNumber;
        QSize size;
        double quality;
        int priority; // 数值越小优先级越高
        qint64 timestamp;
        int retryCount;
        RenderMode preferredMode;
        bool useCompression;
        QString cacheKey;

        GenerationRequest()
            : pageNumber(-1), quality(1.0), priority(0), timestamp(0), retryCount(0),
              preferredMode(RenderMode::HYBRID), useCompression(true) {}

        GenerationRequest(int page, const QSize& sz, double qual, int prio = 0)
            : pageNumber(page), size(sz), quality(qual), priority(prio),
              timestamp(QDateTime::currentMSecsSinceEpoch()), retryCount(0),
              preferredMode(RenderMode::HYBRID), useCompression(true) {
            cacheKey = QString("%1_%2x%3_q%4").arg(page).arg(sz.width()).arg(sz.height()).arg(qual);
        }

        bool operator<(const GenerationRequest& other) const {
            // 优先级队列：优先级数值小的优先，时间戳早的优先
            if (priority != other.priority) {
                return priority > other.priority; // 注意：这里是>，因为QQueue是最小堆
            }
            return timestamp > other.timestamp;
        }
    };

    explicit ThumbnailGenerator(QObject* parent = nullptr);
    ~ThumbnailGenerator() override;

    // 文档管理
    void setDocument(std::shared_ptr<Poppler::Document> document);
    std::shared_ptr<Poppler::Document> document() const;
    
    // 生成设置
    void setThumbnailSize(const QSize& size);
    QSize thumbnailSize() const { return m_defaultSize; }
    
    void setQuality(double quality);
    double quality() const { return m_defaultQuality; }
    
    void setMaxConcurrentJobs(int maxJobs);
    int maxConcurrentJobs() const { return m_maxConcurrentJobs; }
    
    void setMaxRetries(int maxRetries);
    int maxRetries() const { return m_maxRetries; }

    // 渲染模式控制
    void setRenderMode(RenderMode mode);
    RenderMode renderMode() const { return m_renderMode; }

    void setCacheStrategy(CacheStrategy strategy);
    CacheStrategy cacheStrategy() const { return m_cacheStrategy; }

    // GPU加速控制
    void setGpuAccelerationEnabled(bool enabled);
    bool isGpuAccelerationEnabled() const { return m_gpuAccelerationEnabled; }
    bool isGpuAccelerationAvailable() const;

    // 内存池管理
    void setMemoryPoolSize(qint64 size);
    qint64 memoryPoolSize() const { return m_memoryPoolSize; }
    qint64 memoryPoolUsage() const;

    // 压缩设置
    void setCompressionEnabled(bool enabled);
    bool isCompressionEnabled() const { return m_compressionEnabled; }
    void setCompressionQuality(int quality);
    int compressionQuality() const { return m_compressionQuality; }

    // 生成请求
    void generateThumbnail(int pageNumber, const QSize& size = QSize(),
                          double quality = -1.0, int priority = 0,
                          RenderMode mode = RenderMode::HYBRID);
    void generateThumbnailRange(int startPage, int endPage,
                               const QSize& size = QSize(), double quality = -1.0);
    void generateThumbnailBatch(const QList<int>& pageNumbers,
                               const QSize& size = QSize(), double quality = -1.0);
    
    // 队列管理
    void clearQueue();
    void cancelRequest(int pageNumber);
    void setPriority(int pageNumber, int priority);
    
    // 状态查询
    bool isGenerating(int pageNumber) const;
    int queueSize() const;
    int activeJobCount() const;
    
    // 控制
    void pause();
    void resume();
    bool isPaused() const { return m_paused; }
    
    void stop();
    void start();
    bool isRunning() const { return m_running; }

signals:
    void thumbnailGenerated(int pageNumber, const QPixmap& pixmap);
    void thumbnailError(int pageNumber, const QString& error);
    void queueSizeChanged(int size);
    void activeJobsChanged(int count);
    void generationProgress(int completed, int total);

private slots:
    void processQueue();
    void onGenerationFinished();
    void onBatchTimer();

private:
    struct GenerationJob {
        GenerationRequest request;
        QFuture<QPixmap> future;
        QFutureWatcher<QPixmap>* watcher;
        QElapsedTimer timer;

        GenerationJob() : watcher(nullptr) {}
        ~GenerationJob() { delete watcher; }
    };

    // GPU渲染上下文
    struct GpuRenderContext {
        QOpenGLContext* context;
        QOffscreenSurface* surface;
        QOpenGLFramebufferObject* fbo;
        bool isValid;

        GpuRenderContext() : context(nullptr), surface(nullptr), fbo(nullptr), isValid(false) {}
        ~GpuRenderContext() {
            cleanup();
        }

        void cleanup() {
            delete fbo;
            delete surface;
            delete context;
            fbo = nullptr;
            surface = nullptr;
            context = nullptr;
            isValid = false;
        }
    };

    // 内存池条目
    struct MemoryPoolEntry {
        QByteArray data;
        QSize size;
        qint64 lastUsed;
        bool inUse;

        MemoryPoolEntry() : lastUsed(0), inUse(false) {}
    };

    void initializeGenerator();
    void cleanupJobs();
    void startNextJob();
    void handleJobCompletion(GenerationJob* job);
    void handleJobError(GenerationJob* job, const QString& error);
    
    QPixmap generatePixmap(const GenerationRequest& request);
    QPixmap renderPageToPixmap(Poppler::Page* page, const QSize& size, double quality);
    double calculateOptimalDPI(const QSize& targetSize, const QSizeF& pageSize, double quality);
    
    void updateStatistics();
    void logPerformance(const GenerationRequest& request, qint64 duration);

    // 优化方法
    QPixmap renderPageToPixmapOptimized(Poppler::Page* page, const QSize& size, double quality);
    double getCachedDPI(const QSize& targetSize, const QSizeF& pageSize, double quality);
    void cacheDPI(const QSize& targetSize, const QSizeF& pageSize, double quality, double dpi);
    Qt::TransformationMode getOptimalTransformationMode(const QSize& sourceSize, const QSize& targetSize);

    // GPU渲染方法
    QPixmap renderPageToPixmapGpu(Poppler::Page* page, const QSize& size, double quality);
    bool initializeGpuContext();
    void cleanupGpuContext();

    // 内存池方法
    MemoryPoolEntry* acquireMemoryPoolEntry(const QSize& size);
    void releaseMemoryPoolEntry(MemoryPoolEntry* entry);
    void cleanupMemoryPool();

    // 压缩方法
    QByteArray compressPixmap(const QPixmap& pixmap);
    QPixmap decompressPixmap(const QByteArray& data);

    // 批处理方法
    void processBatchRequest(const QList<GenerationRequest>& requests);
    void optimizeBatchOrder(QList<GenerationRequest>& requests);

    // 数据成员
    std::shared_ptr<Poppler::Document> m_document;
    mutable QMutex m_documentMutex;

    // DPI缓存优化
    mutable QHash<QString, double> m_dpiCache;
    mutable QMutex m_dpiCacheMutex;
    
    // 队列管理
    QQueue<GenerationRequest> m_requestQueue;
    mutable QMutex m_queueMutex;
    QWaitCondition m_queueCondition;
    
    // 活动任务
    QHash<int, GenerationJob*> m_activeJobs;
    mutable QMutex m_jobsMutex;
    
    // 设置
    QSize m_defaultSize;
    double m_defaultQuality;
    int m_maxConcurrentJobs;
    int m_maxRetries;

    // 渲染模式和策略
    RenderMode m_renderMode;
    CacheStrategy m_cacheStrategy;

    // GPU加速设置
    bool m_gpuAccelerationEnabled;
    bool m_gpuAccelerationAvailable;
    std::unique_ptr<GpuRenderContext> m_gpuContext;

    // 内存池设置
    qint64 m_memoryPoolSize;
    QList<MemoryPoolEntry*> m_memoryPool;
    mutable QMutex m_memoryPoolMutex;
    std::atomic<qint64> m_memoryPoolUsage{0};

    // 压缩设置
    bool m_compressionEnabled;
    int m_compressionQuality;
    QCache<QString, QByteArray> m_compressedCache;

    // 状态
    bool m_running;
    bool m_paused;
    
    // 批处理
    QTimer* m_batchTimer;
    int m_batchSize;
    int m_batchInterval;
    
    // 统计
    int m_totalGenerated;
    int m_totalErrors;
    qint64 m_totalTime;
    
    // 常量 - 优化后的默认值
    static constexpr int DEFAULT_THUMBNAIL_WIDTH = 120;
    static constexpr int DEFAULT_THUMBNAIL_HEIGHT = 160;
    static constexpr double DEFAULT_QUALITY = 1.0;
    static constexpr int DEFAULT_MAX_CONCURRENT_JOBS = 6; // 增加并发数
    static constexpr int DEFAULT_MAX_RETRIES = 2;
    static constexpr int DEFAULT_BATCH_SIZE = 8; // 增加批处理大小
    static constexpr int DEFAULT_BATCH_INTERVAL = 50; // 减少批处理间隔
    static constexpr int QUEUE_PROCESS_INTERVAL = 25; // 减少队列处理间隔
    static constexpr double MIN_DPI = 72.0;
    static constexpr double MAX_DPI = 200.0; // 降低最大DPI以提升性能

    // GPU和内存池常量
    static constexpr qint64 DEFAULT_MEMORY_POOL_SIZE = 64 * 1024 * 1024; // 64MB
    static constexpr int DEFAULT_COMPRESSION_QUALITY = 85;
    static constexpr int MAX_MEMORY_POOL_ENTRIES = 100;
    static constexpr qint64 MEMORY_POOL_CLEANUP_THRESHOLD = 80 * 1024 * 1024; // 80MB
    static constexpr int GPU_CONTEXT_TIMEOUT = 5000; // 5秒
    static constexpr int COMPRESSED_CACHE_SIZE = 200; // 缓存条目数
};
