#pragma once

#include <poppler-qt6.h>
#include <QCache>
#include <QDateTime>
#include <QHash>
#include <QMutex>
#include <QObject>
#include <QPixmap>
#include <QQueue>
#include <QThread>
#include <QTimer>
#include <QWaitCondition>

/**
 * Intelligent PDF page prerendering system with predictive loading
 * Uses background workers to prerender likely-to-be-viewed pages
 */
class PDFPrerenderer : public QObject {
    Q_OBJECT

public:
    struct RenderRequest {
        int pageNumber;
        double scaleFactor;
        int rotation;
        int priority;  // Lower number = higher priority
        qint64 timestamp;

        bool operator<(const RenderRequest& other) const {
            if (priority != other.priority) {
                return priority < other.priority;
            }
            return timestamp < other.timestamp;
        }
    };

    enum class PrerenderStrategy {
        Conservative,  // Only prerender adjacent pages
        Balanced,      // Prerender based on reading patterns
        Aggressive     // Prerender extensively for smooth experience
    };

    explicit PDFPrerenderer(QObject* parent = nullptr);
    ~PDFPrerenderer();

    // Configuration
    void setDocument(Poppler::Document* document);
    void setStrategy(PrerenderStrategy strategy);
    void setMaxWorkerThreads(int threads);
    void setMaxCacheSize(int maxItems);
    void setMaxMemoryUsage(qint64 bytes);

    // Prerendering control
    void requestPrerender(int pageNumber, double scaleFactor, int rotation,
                          int priority = 5);
    void prioritizePages(const QList<int>& pageNumbers);
    void cancelPrerenderingForPage(int pageNumber);
    void clearPrerenderQueue();

    // Cache access
    QPixmap getCachedPage(int pageNumber, double scaleFactor, int rotation);
    bool hasPrerenderedPage(int pageNumber, double scaleFactor, int rotation);

    // Statistics and monitoring
    int queueSize() const;
    int cacheSize() const;
    qint64 memoryUsage() const;
    double cacheHitRatio() const;

    // Adaptive learning
    void recordPageView(int pageNumber, qint64 viewDuration);
    void recordNavigationPattern(int fromPage, int toPage);

public slots:
    void startPrerendering();
    void stopPrerendering();
    void pausePrerendering();
    void resumePrerendering();

private slots:
    void onRenderCompleted(int pageNumber, const QPixmap& pixmap,
                           double scaleFactor, int rotation);
    void onAdaptiveAnalysis();

private:
    void setupWorkerThreads();
    void cleanupWorkerThreads();
    void scheduleAdaptivePrerendering(int currentPage);
    void analyzeReadingPatterns();
    QList<int> predictNextPages(int currentPage);
    int calculatePriority(int pageNumber, int currentPage);

    // Core components
    Poppler::Document* m_document;
    QList<QThread*> m_workerThreads;
    QList<class PDFRenderWorker*> m_workers;

    // Configuration
    PrerenderStrategy m_strategy;
    int m_maxWorkerThreads;
    int m_maxCacheSize;
    qint64 m_maxMemoryUsage;

    // Request management
    QQueue<RenderRequest> m_renderQueue;
    QMutex m_queueMutex;
    QWaitCondition m_queueCondition;
    bool m_isRunning;
    bool m_isPaused;

    // Cache management
    struct CacheItem {
        QPixmap pixmap;
        qint64 timestamp;
        qint64 memorySize;
        int accessCount;
    };
    QHash<QString, CacheItem> m_cache;
    qint64 m_currentMemoryUsage;

    // Statistics
    int m_cacheHits;
    int m_cacheMisses;

    // Adaptive learning
    QHash<int, QList<qint64>> m_pageViewTimes;
    QHash<int, QHash<int, int>> m_navigationPatterns;  // from -> to -> count
    QTimer* m_adaptiveTimer;

    // Reading pattern analysis
    QList<int> m_accessHistory;
    int m_prerenderRange;

    // Helper methods
    QString getCacheKey(int pageNumber, double scaleFactor, int rotation);
    void evictLRUItems();
    qint64 getPixmapMemorySize(const QPixmap& pixmap);

signals:
    void pagePrerendered(int pageNumber, double scaleFactor, int rotation);
    void prerenderingStarted();
    void prerenderingStopped();
    void cacheUpdated();
    void memoryUsageChanged(qint64 bytes);
};

/**
 * Background worker thread for PDF page rendering
 */
class PDFRenderWorker : public QObject {
    Q_OBJECT

public:
    explicit PDFRenderWorker(QObject* parent = nullptr);

    void setDocument(Poppler::Document* document);
    void addRenderRequest(const PDFPrerenderer::RenderRequest& request);
    void clearQueue();
    void stop();

public slots:
    void processRenderQueue();

private:
    QPixmap renderPage(const PDFPrerenderer::RenderRequest& request);
    double calculateOptimalDPI(double scaleFactor);

    Poppler::Document* m_document;
    QQueue<PDFPrerenderer::RenderRequest> m_localQueue;
    QMutex m_queueMutex;
    QWaitCondition m_queueCondition;
    bool m_shouldStop;

signals:
    void pageRendered(int pageNumber, const QPixmap& pixmap, double scaleFactor,
                      int rotation);
    void renderError(int pageNumber, const QString& error);
};

/**
 * Reading pattern analyzer for intelligent prerendering
 */
class ReadingPatternAnalyzer {
public:
    struct ReadingSession {
        QDateTime startTime;
        QDateTime endTime;
        QList<int> pagesViewed;
        QHash<int, qint64> pageViewDurations;
    };

    void recordPageView(int pageNumber, qint64 duration);
    void recordNavigation(int fromPage, int toPage);
    void startNewSession();
    void endCurrentSession();

    // Pattern analysis
    QList<int> predictNextPages(int currentPage, int count = 5);
    double getPageImportance(int pageNumber);
    bool isSequentialReader() const;
    bool isRandomAccessReader() const;

    // Statistics
    double getAverageViewTime(int pageNumber) const;
    QList<int> getMostViewedPages(int count = 10) const;
    QHash<int, int> getNavigationFrequency(int fromPage) const;

private:
    QList<ReadingSession> m_sessions;
    ReadingSession m_currentSession;
    QHash<int, QList<qint64>> m_pageViewTimes;
    QHash<int, QHash<int, int>> m_navigationPatterns;

    void updatePatterns();
    double calculateTransitionProbability(int fromPage, int toPage) const;
};

/**
 * Memory-aware cache with intelligent eviction
 */
class IntelligentCache {
public:
    struct CacheEntry {
        QPixmap pixmap;
        qint64 timestamp;
        qint64 memorySize;
        int accessCount;
        double importance;
    };

    explicit IntelligentCache(qint64 maxMemory = 256 * 1024 * 1024);

    void insert(const QString& key, const QPixmap& pixmap,
                double importance = 1.0);
    QPixmap get(const QString& key);
    bool contains(const QString& key) const;
    void remove(const QString& key);
    void clear();

    // Configuration
    void setMaxMemoryUsage(qint64 bytes);
    void setMaxItems(int items);

    // Statistics
    qint64 currentMemoryUsage() const { return m_currentMemoryUsage; }
    int size() const { return m_cache.size(); }
    double hitRatio() const;

private:
    void evictItems();
    double calculateEvictionScore(const CacheEntry& entry) const;
    qint64 getPixmapMemorySize(const QPixmap& pixmap) const;

    QHash<QString, CacheEntry> m_cache;
    qint64 m_maxMemoryUsage;
    int m_maxItems;
    qint64 m_currentMemoryUsage;
    int m_hits;
    int m_misses;
};
