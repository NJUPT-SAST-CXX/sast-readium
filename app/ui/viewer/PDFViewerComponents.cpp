#include "PDFViewerComponents.h"
#include <QApplication>
#include <QMutexLocker>
#include <QPainter>
#include <QPixmap>
#include "../../model/RenderModel.h"

// PDFRenderCache Implementation
bool PDFRenderCache::CacheKey::operator==(const CacheKey& other) const {
    return pageNumber == other.pageNumber &&
           qAbs(scaleFactor - other.scaleFactor) < 0.01 &&
           rotation == other.rotation && highQuality == other.highQuality;
}

bool PDFRenderCache::CacheKey::operator<(const CacheKey& other) const {
    if (pageNumber != other.pageNumber)
        return pageNumber < other.pageNumber;
    if (qAbs(scaleFactor - other.scaleFactor) >= 0.01)
        return scaleFactor < other.scaleFactor;
    if (rotation != other.rotation)
        return rotation < other.rotation;
    return highQuality < other.highQuality;
}

size_t qHash(const PDFRenderCache::CacheKey& key, size_t seed) {
    return qHashMulti(seed, key.pageNumber,
                      static_cast<int>(key.scaleFactor * 100), key.rotation,
                      key.highQuality);
}

PDFRenderCache& PDFRenderCache::instance() {
    static PDFRenderCache instance;
    return instance;
}

PDFRenderCache::PDFRenderCache()
    : m_cache(100)  // Default max cost
{}

void PDFRenderCache::insert(const CacheKey& key, const QPixmap& pixmap) {
    QMutexLocker locker(&m_mutex);
    int cost = pixmap.width() * pixmap.height() * 4;  // Estimate memory usage
    m_cache.insert(key, new QPixmap(pixmap), cost);
}

QPixmap PDFRenderCache::get(const CacheKey& key) {
    QMutexLocker locker(&m_mutex);
    QPixmap* pixmap = m_cache.object(key);
    return pixmap ? *pixmap : QPixmap();
}

bool PDFRenderCache::contains(const CacheKey& key) const {
    QMutexLocker locker(&m_mutex);
    return m_cache.contains(key);
}

void PDFRenderCache::clear() {
    QMutexLocker locker(&m_mutex);
    m_cache.clear();
}

void PDFRenderCache::setMaxCost(int maxCost) {
    QMutexLocker locker(&m_mutex);
    m_cache.setMaxCost(maxCost);
}

// PDFPerformanceMonitor Implementation
PDFPerformanceMonitor& PDFPerformanceMonitor::instance() {
    static PDFPerformanceMonitor instance;
    return instance;
}

void PDFPerformanceMonitor::recordRenderTime(int pageNumber,
                                             qint64 milliseconds) {
    QMutexLocker locker(&m_mutex);
    m_renderTimes.append(milliseconds);

    // Keep only last 100 measurements
    if (m_renderTimes.size() > 100) {
        m_renderTimes.removeFirst();
    }
}

void PDFPerformanceMonitor::recordCacheHit(int pageNumber) {
    QMutexLocker locker(&m_mutex);
    m_cacheHits++;
}

void PDFPerformanceMonitor::recordCacheMiss(int pageNumber) {
    QMutexLocker locker(&m_mutex);
    m_cacheMisses++;
}

double PDFPerformanceMonitor::getAverageRenderTime() const {
    QMutexLocker locker(&m_mutex);
    if (m_renderTimes.isEmpty())
        return 0.0;

    qint64 total = 0;
    for (qint64 time : m_renderTimes) {
        total += time;
    }
    return static_cast<double>(total) / m_renderTimes.size();
}

double PDFPerformanceMonitor::getCacheHitRate() const {
    QMutexLocker locker(&m_mutex);
    int total = m_cacheHits + m_cacheMisses;
    return total > 0 ? static_cast<double>(m_cacheHits) / total : 0.0;
}

void PDFPerformanceMonitor::reset() {
    QMutexLocker locker(&m_mutex);
    m_renderTimes.clear();
    m_cacheHits = 0;
    m_cacheMisses = 0;
}

// PDFRenderUtils Implementation
namespace PDFRenderUtils {
void configureRenderHints(QPainter& painter, bool highQuality) {
    if (highQuality) {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    }
}

QPixmap renderPageHighQuality(Poppler::Page* page, double scaleFactor,
                              int rotation) {
    if (!page)
        return QPixmap();

    double dpi = calculateOptimalDPI(scaleFactor, true);
    QImage image = page->renderToImage(
        dpi, dpi, -1, -1, -1, -1,
        static_cast<Poppler::Page::Rotation>(rotation / 90));

    QPixmap pixmap = QPixmap::fromImage(image);
    if (QApplication::instance()) {
        pixmap.setDevicePixelRatio(qApp->devicePixelRatio());
    }
    return pixmap;
}

QPixmap renderPageFast(Poppler::Page* page, double scaleFactor, int rotation) {
    if (!page)
        return QPixmap();

    double dpi = calculateOptimalDPI(scaleFactor, false);
    QImage image = page->renderToImage(
        dpi, dpi, -1, -1, -1, -1,
        static_cast<Poppler::Page::Rotation>(rotation / 90));

    QPixmap pixmap = QPixmap::fromImage(image);
    if (QApplication::instance()) {
        pixmap.setDevicePixelRatio(qApp->devicePixelRatio());
    }
    return pixmap;
}

double calculateOptimalDPI(double scaleFactor, bool highQuality) {
    double baseDPI = highQuality ? 150.0 : 72.0;
    double devicePixelRatio =
        QApplication::instance() ? qApp->devicePixelRatio() : 1.0;
    return baseDPI * scaleFactor * devicePixelRatio;
}

void optimizeDocument(Poppler::Document* document) {
    // Use centralized render hint configuration from RenderModel
    RenderModel::configureDocumentRenderHints(document);
}
}  // namespace PDFRenderUtils
