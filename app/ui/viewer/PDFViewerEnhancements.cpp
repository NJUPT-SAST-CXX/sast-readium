#include "PDFViewerEnhancements.h"
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QFuture>
#include <QImage>
#include <QMouseEvent>
#include <QMutexLocker>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QStyleOption>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QtConcurrent/QtConcurrent>
#include <QtGlobal>
#include <cmath>

// HighQualityPDFPageWidget Implementation
HighQualityPDFPageWidget::HighQualityPDFPageWidget(QWidget* parent)
    : QLabel(parent),
      m_currentPage(nullptr),
      m_currentScaleFactor(1.0),
      m_currentRotation(0),
      m_renderWatcher(nullptr),
      m_renderTimer(nullptr),
      m_isRendering(false),
      m_isDragging(false) {
    setupWidget();

    // Setup render timer for debouncing
    m_renderTimer = new QTimer(this);
    m_renderTimer->setSingleShot(true);
    m_renderTimer->setInterval(RENDER_DELAY_MS);
    connect(m_renderTimer, &QTimer::timeout, this,
            &HighQualityPDFPageWidget::onRenderTimeout);

    // Setup render watcher
    m_renderWatcher = new QFutureWatcher<QPixmap>(this);
    connect(m_renderWatcher, &QFutureWatcher<QPixmap>::finished, this,
            &HighQualityPDFPageWidget::onRenderCompleted);

    showPlaceholder("No PDF loaded");
}

HighQualityPDFPageWidget::~HighQualityPDFPageWidget() {
    if (m_renderWatcher && m_renderWatcher->isRunning()) {
        m_renderWatcher->cancel();
        m_renderWatcher->waitForFinished();
    }
}

void HighQualityPDFPageWidget::setupWidget() {
    setAlignment(Qt::AlignCenter);
    setMinimumSize(200, 200);
    setStyleSheet(
        "QLabel { background-color: white; border: 1px solid #ccc; }");

    // Enable mouse tracking for pan operations
    setMouseTracking(true);

    // Accept focus for keyboard events
    setFocusPolicy(Qt::StrongFocus);
}

void HighQualityPDFPageWidget::setPage(Poppler::Page* page,
                                       Poppler::Document* document,
                                       double scaleFactor, int rotation) {
    m_currentPage = page;
    m_document = document;
    m_currentScaleFactor = qBound(MIN_SCALE, scaleFactor, MAX_SCALE);
    m_currentRotation = ((rotation % 360) + 360) % 360;

    if (page) {
        // Cancel any pending render
        if (m_renderWatcher && m_renderWatcher->isRunning()) {
            m_renderWatcher->cancel();
        }

        // Start render timer
        m_renderTimer->start();
        showPlaceholder("Rendering...");
    } else {
        showPlaceholder("No page to display");
    }
}

void HighQualityPDFPageWidget::setScaleFactor(double factor) {
    double newFactor = qBound(MIN_SCALE, factor, MAX_SCALE);
    if (qAbs(newFactor - m_currentScaleFactor) > 0.01) {
        m_currentScaleFactor = newFactor;

        if (m_currentPage) {
            m_renderTimer->start();
            showPlaceholder("Scaling...");
        }

        emit scaleChanged(m_currentScaleFactor);
    }
}

void HighQualityPDFPageWidget::setRotation(int degrees) {
    int newRotation = ((degrees % 360) + 360) % 360;
    if (newRotation != m_currentRotation) {
        m_currentRotation = newRotation;

        if (m_currentPage) {
            m_renderTimer->start();
            showPlaceholder("Rotating...");
        }
    }
}

void HighQualityPDFPageWidget::onRenderTimeout() {
    if (!m_currentPage || m_isRendering) {
        return;
    }

    renderPageAsync();
}

void HighQualityPDFPageWidget::renderPageAsync() {
    if (!m_currentPage || m_isRendering) {
        return;
    }

    m_isRendering = true;

    // Create render task
    HighQualityRenderTask task;
    task.page = m_currentPage;
    task.document = m_document;
    task.scaleFactor = m_currentScaleFactor;
    task.rotation = m_currentRotation;
    task.highQuality = true;

    // Start async rendering
    QFuture<QPixmap> future =
        QtConcurrent::run([task]() { return task.render(); });

    m_renderWatcher->setFuture(future);
}

void HighQualityPDFPageWidget::onRenderCompleted() {
    m_isRendering = false;

    if (m_renderWatcher->isCanceled()) {
        return;
    }

    QPixmap result = m_renderWatcher->result();
    if (!result.isNull()) {
        m_renderedPixmap = result;
        setPixmap(result);

        // Record performance metrics
        PDFPerformanceMonitor::instance().recordRenderTime(
            m_currentPage ? m_currentPage->index() : -1,
            QDateTime::currentMSecsSinceEpoch());
    } else {
        showPlaceholder("Render failed");
    }
}

void HighQualityPDFPageWidget::showPlaceholder(const QString& text) {
    QPixmap placeholder(400, 300);
    placeholder.fill(Qt::white);

    QPainter painter(&placeholder);
    painter.setPen(Qt::gray);
    painter.setFont(QFont("Arial", 12));
    painter.drawText(placeholder.rect(), Qt::AlignCenter, text);

    setPixmap(placeholder);
}

void HighQualityPDFPageWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);

    // Enable high-quality rendering
    PDFRenderUtils::configureRenderHints(painter, true);

    // Call parent implementation
    QLabel::paintEvent(event);

    // Add subtle shadow effect
    if (!m_renderedPixmap.isNull()) {
        QRect pixmapRect = rect();
        painter.setPen(QPen(QColor(0, 0, 0, 30), 1));
        painter.drawRect(pixmapRect.adjusted(0, 0, -1, -1));
    }
}

void HighQualityPDFPageWidget::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom with Ctrl+Wheel
        const double scaleFactor =
            event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
        setScaleFactor(m_currentScaleFactor * scaleFactor);
        event->accept();
    } else {
        QLabel::wheelEvent(event);
    }
}

void HighQualityPDFPageWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else {
        QLabel::mousePressEvent(event);
    }
}

void HighQualityPDFPageWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        // Handle panning (would need scroll area integration)
        m_lastPanPoint = event->pos();
        event->accept();
    } else {
        QLabel::mouseMoveEvent(event);
    }
}

void HighQualityPDFPageWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_isDragging) {
        m_isDragging = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else {
        QLabel::mouseReleaseEvent(event);
    }
}

// HighQualityRenderTask Implementation
QPixmap HighQualityRenderTask::render() const {
    if (!page) {
        return QPixmap();
    }

    try {
        // Configure document for high quality
        if (document) {
            configureDocument(document);
        }

        // Calculate DPI
        double dpi = calculateDPI(scaleFactor, highQuality);

        // Render the page
        QImage image = page->renderToImage(
            dpi, dpi, -1, -1, -1, -1,
            static_cast<Poppler::Page::Rotation>(rotation / 90));

        if (image.isNull()) {
            qWarning() << "HighQualityRenderTask: Failed to render page";
            return QPixmap();
        }

        return QPixmap::fromImage(image);

    } catch (const std::exception& e) {
        qWarning() << "HighQualityRenderTask: Exception during rendering:"
                   << e.what();
        return QPixmap();
    } catch (...) {
        qWarning()
            << "HighQualityRenderTask: Unknown exception during rendering";
        return QPixmap();
    }
}

void HighQualityRenderTask::configureDocument(Poppler::Document* doc) const {
    if (!doc)
        return;

    // Enable high-quality rendering hints
    doc->setRenderHint(Poppler::Document::Antialiasing, true);
    doc->setRenderHint(Poppler::Document::TextAntialiasing, true);
    doc->setRenderHint(Poppler::Document::TextHinting, true);
    doc->setRenderHint(Poppler::Document::TextSlightHinting, true);
    doc->setRenderHint(Poppler::Document::ThinLineShape, true);
}

double HighQualityRenderTask::calculateDPI(double scale,
                                           bool highQuality) const {
    double baseDPI = highQuality ? 150.0 : 72.0;
    return baseDPI * scale;
}

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
    return QPixmap::fromImage(image);
}

QPixmap renderPageFast(Poppler::Page* page, double scaleFactor, int rotation) {
    if (!page)
        return QPixmap();

    double dpi = calculateOptimalDPI(scaleFactor, false);
    QImage image = page->renderToImage(
        dpi, dpi, -1, -1, -1, -1,
        static_cast<Poppler::Page::Rotation>(rotation / 90));
    return QPixmap::fromImage(image);
}

double calculateOptimalDPI(double scaleFactor, bool highQuality) {
    double baseDPI = highQuality ? 150.0 : 72.0;
    return baseDPI * scaleFactor;
}

void optimizeDocument(Poppler::Document* document) {
    if (!document)
        return;

    // Enable high-quality rendering hints
    document->setRenderHint(Poppler::Document::Antialiasing, true);
    document->setRenderHint(Poppler::Document::TextAntialiasing, true);
    document->setRenderHint(Poppler::Document::TextHinting, true);
    document->setRenderHint(Poppler::Document::TextSlightHinting, true);
    document->setRenderHint(Poppler::Document::ThinLineShape, true);
}
}  // namespace PDFRenderUtils

// AdvancedPDFViewer Implementation
AdvancedPDFViewer::AdvancedPDFViewer(QWidget* parent)
    : QWidget(parent),
      m_pageWidget(nullptr),
      m_document(nullptr),
      m_currentPage(0),
      m_zoomFactor(1.0),
      m_rotation(0),
      m_highQualityEnabled(true) {
    setupUI();
}

void AdvancedPDFViewer::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_pageWidget = new HighQualityPDFPageWidget(this);
    layout->addWidget(m_pageWidget);

    // Connect signals
    connect(m_pageWidget, &HighQualityPDFPageWidget::scaleChanged, this,
            &AdvancedPDFViewer::zoomChanged);
}

void AdvancedPDFViewer::setDocument(Poppler::Document* document) {
    m_document = document;

    if (m_document) {
        PDFRenderUtils::optimizeDocument(m_document);
        updateCurrentPage();
    } else {
        m_pageWidget->setPage(nullptr, nullptr);
    }
}

void AdvancedPDFViewer::setCurrentPage(int pageNumber) {
    if (!m_document || pageNumber < 0 || pageNumber >= m_document->numPages()) {
        return;
    }

    m_currentPage = pageNumber;
    updateCurrentPage();
    emit currentPageChanged(pageNumber);
}

void AdvancedPDFViewer::setZoomFactor(double factor) {
    m_zoomFactor = factor;
    updateCurrentPage();
    emit zoomChanged(factor);
}

void AdvancedPDFViewer::setRotation(int degrees) {
    m_rotation = degrees;
    updateCurrentPage();
    emit rotationChanged(degrees);
}

void AdvancedPDFViewer::enableHighQualityRendering(bool enable) {
    m_highQualityEnabled = enable;
    updateCurrentPage();
}

void AdvancedPDFViewer::setRenderingThreads(int threads) {
    // Configure thread pool if needed
    Q_UNUSED(threads)
}

void AdvancedPDFViewer::setCacheSize(int maxItems) {
    PDFRenderCache::instance().setMaxCost(maxItems * 1024 *
                                          1024);  // Convert to bytes
}

void AdvancedPDFViewer::updateCurrentPage() {
    if (!m_document || m_currentPage < 0 ||
        m_currentPage >= m_document->numPages()) {
        m_pageWidget->setPage(nullptr, nullptr);
        return;
    }

    std::unique_ptr<Poppler::Page> page(m_document->page(m_currentPage));
    m_pageWidget->setPage(page.get(), m_document, m_zoomFactor, m_rotation);
}
