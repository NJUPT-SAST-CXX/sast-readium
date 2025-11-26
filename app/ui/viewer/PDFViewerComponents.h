#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QCache>
#include <QMutex>
#include <QPainter>
#include <QPixmap>

/**
 * Thread-safe cache for rendered PDF pages
 */
class PDFRenderCache {
public:
    struct CacheKey {
        int pageNumber;
        double scaleFactor;
        int rotation;
        bool highQuality;

        bool operator==(const CacheKey& other) const;
        bool operator<(const CacheKey& other) const;
    };

    friend size_t qHash(const CacheKey& key, size_t seed);

    static PDFRenderCache& instance();

    void insert(const CacheKey& key, const QPixmap& pixmap);
    QPixmap get(const CacheKey& key);
    bool contains(const CacheKey& key) const;
    void clear();
    void setMaxCost(int maxCost);

private:
    PDFRenderCache();
    mutable QMutex m_mutex;
    QCache<CacheKey, QPixmap> m_cache;
};

/**
 * Performance monitoring for PDF rendering
 */
class PDFPerformanceMonitor {
public:
    static PDFPerformanceMonitor& instance();

    void recordRenderTime(int pageNumber, qint64 milliseconds);
    void recordCacheHit(int pageNumber);
    void recordCacheMiss(int pageNumber);

    double getAverageRenderTime() const;
    double getCacheHitRate() const;
    void reset();

private:
    PDFPerformanceMonitor() = default;
    mutable QMutex m_mutex;
    QList<qint64> m_renderTimes;
    int m_cacheHits = 0;
    int m_cacheMisses = 0;
};

/**
 * Utility functions for PDF rendering
 */
namespace PDFRenderUtils {
void configureRenderHints(QPainter& painter, bool highQuality = true);
QPixmap renderPageHighQuality(Poppler::Page* page, double scaleFactor,
                              int rotation = 0);
QPixmap renderPageFast(Poppler::Page* page, double scaleFactor,
                       int rotation = 0);
double calculateOptimalDPI(double scaleFactor, bool highQuality = true);
void optimizeDocument(Poppler::Document* document);
}  // namespace PDFRenderUtils
