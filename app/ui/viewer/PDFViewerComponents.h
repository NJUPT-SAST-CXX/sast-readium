#pragma once

#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QTimer>
#include <QCache>
#include <QMutex>
#include <QFutureWatcher>
#include <QPainter>
#include <poppler-qt6.h>

/**
 * High-quality PDF page widget with improved rendering quality and performance optimizations
 */
class HighQualityPDFPageWidget : public QLabel
{
    Q_OBJECT

public:
    explicit HighQualityPDFPageWidget(QWidget* parent = nullptr);
    ~HighQualityPDFPageWidget();

    // Same interface as original PDFPageWidget for drop-in replacement
    void setPage(Poppler::Page* page, Poppler::Document* document = nullptr, double scaleFactor = 1.0, int rotation = 0);
    void setScaleFactor(double factor);
    void setRotation(int degrees);
    
    double getScaleFactor() const { return m_currentScaleFactor; }
    int getRotation() const { return m_currentRotation; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onRenderCompleted();
    void onRenderTimeout();

private:
    void setupWidget();
    void renderPageAsync();
    void updateDisplay();
    void showPlaceholder(const QString& text = "Loading...");
    
    // Page state
    Poppler::Page* m_currentPage;
    Poppler::Document* m_document; // Keep reference to document
    double m_currentScaleFactor;
    int m_currentRotation;
    
    // Rendering
    QFutureWatcher<QPixmap>* m_renderWatcher;
    QTimer* m_renderTimer;
    QPixmap m_renderedPixmap;
    bool m_isRendering;
    
    // Interaction
    bool m_isDragging;
    QPoint m_lastPanPoint;
    
    // Performance settings
    static constexpr int RENDER_DELAY_MS = 100;
    static constexpr double MIN_SCALE = 0.1;
    static constexpr double MAX_SCALE = 5.0;

signals:
    void scaleChanged(double factor);
};

/**
 * High-performance render task for PDF pages
 */
struct HighQualityRenderTask
{
    Poppler::Page* page;
    Poppler::Document* document;
    double scaleFactor;
    int rotation;
    bool highQuality;

    QPixmap render() const;
    
private:
    void configureDocument(Poppler::Document* doc) const;
    double calculateDPI(double scale, bool highQuality) const;
};

/**
 * Thread-safe cache for rendered PDF pages
 */
class PDFRenderCache
{
public:
    struct CacheKey
    {
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
class PDFPerformanceMonitor
{
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
namespace PDFRenderUtils
{
    void configureRenderHints(QPainter& painter, bool highQuality = true);
    QPixmap renderPageHighQuality(Poppler::Page* page, double scaleFactor, int rotation = 0);
    QPixmap renderPageFast(Poppler::Page* page, double scaleFactor, int rotation = 0);
    double calculateOptimalDPI(double scaleFactor, bool highQuality = true);
    void optimizeDocument(Poppler::Document* document);
}

/**
 * Advanced PDF viewer with performance enhancements
 */
class AdvancedPDFViewer : public QWidget
{
    Q_OBJECT

public:
    explicit AdvancedPDFViewer(QWidget* parent = nullptr);
    
    // Same interface as original for compatibility
    void setDocument(Poppler::Document* document);
    void setCurrentPage(int pageNumber);
    void setZoomFactor(double factor);
    void setRotation(int degrees);
    
    // Enhanced features
    void enableHighQualityRendering(bool enable);
    void setRenderingThreads(int threads);
    void setCacheSize(int maxItems);
    
    // Performance monitoring
    PDFPerformanceMonitor& performanceMonitor() { return PDFPerformanceMonitor::instance(); }

private:
    void setupUI();
    void updateCurrentPage();
    
    HighQualityPDFPageWidget* m_pageWidget;
    Poppler::Document* m_document;
    int m_currentPage;
    double m_zoomFactor;
    int m_rotation;
    bool m_highQualityEnabled;

signals:
    void currentPageChanged(int pageNumber);
    void zoomChanged(double factor);
    void rotationChanged(int degrees);
};
