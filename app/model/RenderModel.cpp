#include "RenderModel.h"
#include <QApplication>
#include <QtConcurrent/QtConcurrent>
#include <cmath>
#include "../logging/LoggingMacros.h"
#include "qimage.h"
#include "qlogging.h"

RenderModel::RenderModel(double dpiX, double dpiY, Poppler::Document* _document,
                         QObject* parent)
    : QObject(parent),
      document(_document),
      dpiX(dpiX),
      dpiY(dpiY),
      renderQuality(RenderQuality::High),  // Default to High for
                                           // high-performance rendering
      maxCacheSize(50) {
    // Initialize cache with reasonable size (in MB)
    pageCache.setMaxCost(maxCacheSize * 1024 * 1024);

    // Setup cache cleanup timer
    cacheCleanupTimer = new QTimer(this);
    cacheCleanupTimer->setSingleShot(false);
    cacheCleanupTimer->setInterval(60000);  // Clean every minute
    connect(cacheCleanupTimer, &QTimer::timeout, this, [this]() {
        // Periodic cache maintenance if needed
        if (pageCache.totalCost() > pageCache.maxCost() * 0.8) {
            LOG_DEBUG(
                "RenderModel: Cache approaching limit, triggering cleanup");
        }
    });
    cacheCleanupTimer->start();

    LOG_INFO(
        "RenderModel: Initialized with DPI {}x{}, Quality: High (1.5x "
        "multiplier)",
        dpiX, dpiY);
}

RenderModel::~RenderModel() {
    // Cancel all pending async renders
    for (auto it = asyncRenders.begin(); it != asyncRenders.end(); ++it) {
        if (it.value()) {
            it.value()->cancel();
            it.value()->deleteLater();
        }
    }
    asyncRenders.clear();
}

// Page Information Methods
QSizeF RenderModel::getPageSize(int pageNum) const {
    if (!document) {
        LOG_WARNING("RenderModel: Document not loaded");
        lastError = "Document not loaded";
        return QSizeF();
    }

    if (pageNum < 0 || pageNum >= document->numPages()) {
        LOG_WARNING(
            "RenderModel: Invalid page number {} (document has {} pages)",
            pageNum, document->numPages());
        lastError = QString("Invalid page number %1").arg(pageNum);
        return QSizeF();
    }

    try {
        std::unique_ptr<Poppler::Page> pdfPage(document->page(pageNum));
        if (!pdfPage) {
            LOG_WARNING("RenderModel: Failed to get page {}", pageNum);
            lastError = QString("Failed to get page %1").arg(pageNum);
            return QSizeF();
        }

        QSizeF size = pdfPage->pageSizeF();
        LOG_DEBUG("RenderModel: Page {} size: {}x{}", pageNum, size.width(),
                  size.height());
        return size;

    } catch (const std::exception& e) {
        LOG_ERROR(
            "RenderModel: Exception while getting page size for page {}: {}",
            pageNum, e.what());
        lastError = QString("Exception: %1").arg(e.what());
        return QSizeF();
    } catch (...) {
        LOG_ERROR(
            "RenderModel: Unknown exception while getting page size for page "
            "{}",
            pageNum);
        lastError = "Unknown exception occurred";
        return QSizeF();
    }
}

QRectF RenderModel::getPageBoundingBox(int pageNum) const {
    QSizeF size = getPageSize(pageNum);
    if (size.isEmpty()) {
        return QRectF();
    }
    return QRectF(0, 0, size.width(), size.height());
}

double RenderModel::getPageRotation(int pageNum) const {
    if (!document) {
        LOG_WARNING("RenderModel: Document not loaded");
        lastError = "Document not loaded";
        return 0.0;
    }

    if (pageNum < 0 || pageNum >= document->numPages()) {
        LOG_WARNING(
            "RenderModel: Invalid page number {} (document has {} pages)",
            pageNum, document->numPages());
        lastError = QString("Invalid page number %1").arg(pageNum);
        return 0.0;
    }

    try {
        std::unique_ptr<Poppler::Page> pdfPage(document->page(pageNum));
        if (!pdfPage) {
            LOG_WARNING("RenderModel: Failed to get page {}", pageNum);
            lastError = QString("Failed to get page %1").arg(pageNum);
            return 0.0;
        }

        // Convert Poppler rotation enum to degrees
        Poppler::Page::Rotation rotation =
            static_cast<Poppler::Page::Rotation>(pdfPage->orientation());
        double degrees = 0.0;
        switch (rotation) {
            case Poppler::Page::Rotate0:
                degrees = 0.0;
                break;
            case Poppler::Page::Rotate90:
                degrees = 90.0;
                break;
            case Poppler::Page::Rotate180:
                degrees = 180.0;
                break;
            case Poppler::Page::Rotate270:
                degrees = 270.0;
                break;
        }

        LOG_DEBUG("RenderModel: Page {} rotation: {} degrees", pageNum,
                  degrees);
        return degrees;

    } catch (const std::exception& e) {
        LOG_ERROR(
            "RenderModel: Exception while getting page rotation for page {}: "
            "{}",
            pageNum, e.what());
        lastError = QString("Exception: %1").arg(e.what());
        return 0.0;
    } catch (...) {
        LOG_ERROR(
            "RenderModel: Unknown exception while getting page rotation for "
            "page {}",
            pageNum);
        lastError = "Unknown exception occurred";
        return 0.0;
    }
}

// Document Information Methods
QString RenderModel::getDocumentTitle() const {
    if (!document) {
        LOG_WARNING("RenderModel: Document not loaded");
        lastError = "Document not loaded";
        return QString();
    }

    try {
        QString title = document->info("Title");
        LOG_DEBUG("RenderModel: Document title: {}", title.toStdString());
        return title;
    } catch (const std::exception& e) {
        LOG_ERROR("RenderModel: Exception while getting document title: {}",
                  e.what());
        lastError = QString("Exception: %1").arg(e.what());
        return QString();
    } catch (...) {
        LOG_ERROR(
            "RenderModel: Unknown exception while getting document title");
        lastError = "Unknown exception occurred";
        return QString();
    }
}

QString RenderModel::getDocumentAuthor() const {
    if (!document) {
        LOG_WARNING("RenderModel: Document not loaded");
        lastError = "Document not loaded";
        return QString();
    }

    try {
        QString author = document->info("Author");
        LOG_DEBUG("RenderModel: Document author: {}", author.toStdString());
        return author;
    } catch (const std::exception& e) {
        LOG_ERROR("RenderModel: Exception while getting document author: {}",
                  e.what());
        lastError = QString("Exception: %1").arg(e.what());
        return QString();
    } catch (...) {
        LOG_ERROR(
            "RenderModel: Unknown exception while getting document author");
        lastError = "Unknown exception occurred";
        return QString();
    }
}

QString RenderModel::getDocumentSubject() const {
    if (!document) {
        LOG_WARNING("RenderModel: Document not loaded");
        lastError = "Document not loaded";
        return QString();
    }

    try {
        QString subject = document->info("Subject");
        LOG_DEBUG("RenderModel: Document subject: {}", subject.toStdString());
        return subject;
    } catch (const std::exception& e) {
        LOG_ERROR("RenderModel: Exception while getting document subject: {}",
                  e.what());
        lastError = QString("Exception: %1").arg(e.what());
        return QString();
    } catch (...) {
        LOG_ERROR(
            "RenderModel: Unknown exception while getting document subject");
        lastError = "Unknown exception occurred";
        return QString();
    }
}

QString RenderModel::getDocumentCreator() const {
    if (!document) {
        LOG_WARNING("RenderModel: Document not loaded");
        lastError = "Document not loaded";
        return QString();
    }

    try {
        QString creator = document->info("Creator");
        LOG_DEBUG("RenderModel: Document creator: {}", creator.toStdString());
        return creator;
    } catch (const std::exception& e) {
        LOG_ERROR("RenderModel: Exception while getting document creator: {}",
                  e.what());
        lastError = QString("Exception: %1").arg(e.what());
        return QString();
    } catch (...) {
        LOG_ERROR(
            "RenderModel: Unknown exception while getting document creator");
        lastError = "Unknown exception occurred";
        return QString();
    }
}

QDateTime RenderModel::getDocumentCreationDate() const {
    if (!document) {
        LOG_WARNING("RenderModel: Document not loaded");
        lastError = "Document not loaded";
        return QDateTime();
    }

    try {
        QDateTime creationDate = document->date("CreationDate");
        LOG_DEBUG("RenderModel: Document creation date: {}",
                  creationDate.toString().toStdString());
        return creationDate;
    } catch (const std::exception& e) {
        LOG_ERROR(
            "RenderModel: Exception while getting document creation date: {}",
            e.what());
        lastError = QString("Exception: %1").arg(e.what());
        return QDateTime();
    } catch (...) {
        LOG_ERROR(
            "RenderModel: Unknown exception while getting document creation "
            "date");
        lastError = "Unknown exception occurred";
        return QDateTime();
    }
}

QDateTime RenderModel::getDocumentModificationDate() const {
    if (!document) {
        LOG_WARNING("RenderModel: Document not loaded");
        lastError = "Document not loaded";
        return QDateTime();
    }

    try {
        QDateTime modDate = document->date("ModDate");
        LOG_DEBUG("RenderModel: Document modification date: {}",
                  modDate.toString().toStdString());
        return modDate;
    } catch (const std::exception& e) {
        LOG_ERROR(
            "RenderModel: Exception while getting document modification date: "
            "{}",
            e.what());
        lastError = QString("Exception: %1").arg(e.what());
        return QDateTime();
    } catch (...) {
        LOG_ERROR(
            "RenderModel: Unknown exception while getting document "
            "modification date");
        lastError = "Unknown exception occurred";
        return QDateTime();
    }
}

QMap<QString, QString> RenderModel::getDocumentInfo() const {
    QMap<QString, QString> info;

    if (!document) {
        LOG_WARNING("RenderModel: Document not loaded");
        lastError = "Document not loaded";
        return info;
    }

    try {
        // Get all standard metadata fields
        info["Title"] = getDocumentTitle();
        info["Author"] = getDocumentAuthor();
        info["Subject"] = getDocumentSubject();
        info["Creator"] = getDocumentCreator();
        info["Producer"] = document->info("Producer");
        info["Keywords"] = document->info("Keywords");

        // Get dates as strings
        QDateTime creationDate = getDocumentCreationDate();
        QDateTime modDate = getDocumentModificationDate();

        if (creationDate.isValid()) {
            info["CreationDate"] = creationDate.toString(Qt::ISODate);
        }
        if (modDate.isValid()) {
            info["ModificationDate"] = modDate.toString(Qt::ISODate);
        }

        // Additional document properties
        info["PageCount"] = QString::number(document->numPages());
        // Try to get PDF version - method name may vary by Poppler version
        try {
            info["Version"] =
                QString("PDF");  // Fallback if version method not available
        } catch (...) {
            info["Version"] = QString("PDF");
        }
        info["Encrypted"] = document->isEncrypted() ? "Yes" : "No";
        info["Linearized"] = document->isLinearized() ? "Yes" : "No";

        LOG_DEBUG("RenderModel: Retrieved document info with {} fields",
                  info.size());
        return info;

    } catch (const std::exception& e) {
        LOG_ERROR("RenderModel: Exception while getting document info: {}",
                  e.what());
        lastError = QString("Exception: %1").arg(e.what());
        return info;
    } catch (...) {
        LOG_ERROR("RenderModel: Unknown exception while getting document info");
        lastError = "Unknown exception occurred";
        return info;
    }
}

// DPI Management Methods
double RenderModel::getDpiX() const { return dpiX; }

double RenderModel::getDpiY() const { return dpiY; }

void RenderModel::setDpiX(double dpi) {
    if (dpi <= 0) {
        LOG_WARNING("RenderModel: Invalid DPI X value: {}, using default 72.0",
                    dpi);
        dpi = 72.0;
    }

    if (qAbs(dpiX - dpi) > 0.001) {
        double oldDpiX = dpiX;
        dpiX = dpi;
        LOG_INFO("RenderModel: DPI X changed from {} to {}", oldDpiX, dpiX);

        // Clear cache since DPI affects rendering
        clearCache();
        emit dpiChanged(dpiX, dpiY);
    }
}

void RenderModel::setDpiY(double dpi) {
    if (dpi <= 0) {
        LOG_WARNING("RenderModel: Invalid DPI Y value: {}, using default 72.0",
                    dpi);
        dpi = 72.0;
    }

    if (qAbs(dpiY - dpi) > 0.001) {
        double oldDpiY = dpiY;
        dpiY = dpi;
        LOG_INFO("RenderModel: DPI Y changed from {} to {}", oldDpiY, dpiY);

        // Clear cache since DPI affects rendering
        clearCache();
        emit dpiChanged(dpiX, dpiY);
    }
}

void RenderModel::setDpi(double dpiX, double dpiY) {
    if (dpiX <= 0) {
        LOG_WARNING("RenderModel: Invalid DPI X value: {}, using default 72.0",
                    dpiX);
        dpiX = 72.0;
    }
    if (dpiY <= 0) {
        LOG_WARNING("RenderModel: Invalid DPI Y value: {}, using default 72.0",
                    dpiY);
        dpiY = 72.0;
    }

    bool changed = false;
    if (qAbs(this->dpiX - dpiX) > 0.001) {
        this->dpiX = dpiX;
        changed = true;
    }
    if (qAbs(this->dpiY - dpiY) > 0.001) {
        this->dpiY = dpiY;
        changed = true;
    }

    if (changed) {
        LOG_INFO("RenderModel: DPI changed to {}x{}", this->dpiX, this->dpiY);

        // Clear cache since DPI affects rendering
        clearCache();
        emit dpiChanged(this->dpiX, this->dpiY);
    }
}

double RenderModel::getEffectiveDpiX(double scaleFactor,
                                     double devicePixelRatio) const {
    // Validate device pixel ratio - fallback to 1.0 if invalid
    if (devicePixelRatio <= 0.0 || !std::isfinite(devicePixelRatio)) {
        LOG_WARNING(
            "RenderModel: Invalid device pixel ratio: {}, using default 1.0",
            devicePixelRatio);
        devicePixelRatio = 1.0;
    }

    // Calculate effective DPI: base DPI * scale factor * quality multiplier *
    // device pixel ratio This ensures consistent high-quality rendering across
    // all zoom levels and display types
    // - baseDpi: 72.0 (standard PDF DPI)
    // - scaleFactor: User zoom level (0.1 to 10.0)
    // - qualityMultiplier: Based on RenderQuality setting (1.0 to 2.0)
    // - devicePixelRatio: Display scaling (1.0 for standard, 2.0 for Retina,
    // etc.)
    double effectiveDpi = dpiX * qMax(0.1, scaleFactor) *
                          getQualityMultiplier() * devicePixelRatio;

    // Cap at reasonable maximum to prevent excessive memory usage
    // 600 DPI is sufficient for even 4K displays at high zoom with 2x scaling
    double maxDpi = 600.0;
    double clampedDpi = qMin(effectiveDpi, maxDpi);

    if (effectiveDpi > maxDpi) {
        LOG_DEBUG(
            "RenderModel: Clamping effective DPI X from {:.2f} to {:.2f} "
            "(scale: {:.2f}, devicePixelRatio: {:.2f})",
            effectiveDpi, clampedDpi, scaleFactor, devicePixelRatio);
    }

    return clampedDpi;
}

double RenderModel::getEffectiveDpiY(double scaleFactor,
                                     double devicePixelRatio) const {
    // Validate device pixel ratio - fallback to 1.0 if invalid
    if (devicePixelRatio <= 0.0 || !std::isfinite(devicePixelRatio)) {
        LOG_WARNING(
            "RenderModel: Invalid device pixel ratio: {}, using default 1.0",
            devicePixelRatio);
        devicePixelRatio = 1.0;
    }

    // Calculate effective DPI: base DPI * scale factor * quality multiplier *
    // device pixel ratio This ensures consistent high-quality rendering across
    // all zoom levels and display types
    double effectiveDpi = dpiY * qMax(0.1, scaleFactor) *
                          getQualityMultiplier() * devicePixelRatio;

    // Cap at reasonable maximum to prevent excessive memory usage
    // 600 DPI is sufficient for even 4K displays at high zoom with 2x scaling
    double maxDpi = 600.0;
    double clampedDpi = qMin(effectiveDpi, maxDpi);

    if (effectiveDpi > maxDpi) {
        LOG_DEBUG(
            "RenderModel: Clamping effective DPI Y from {:.2f} to {:.2f} "
            "(scale: {:.2f}, devicePixelRatio: {:.2f})",
            effectiveDpi, clampedDpi, scaleFactor, devicePixelRatio);
    }

    return clampedDpi;
}

// Rendering Quality Methods
void RenderModel::setRenderQuality(RenderQuality quality) {
    if (renderQuality != quality) {
        renderQuality = quality;

        QString qualityStr;
        switch (quality) {
            case RenderQuality::Draft:
                qualityStr = "Draft";
                break;
            case RenderQuality::Normal:
                qualityStr = "Normal";
                break;
            case RenderQuality::High:
                qualityStr = "High";
                break;
            case RenderQuality::Ultra:
                qualityStr = "Ultra";
                break;
        }

        LOG_INFO("RenderModel: Render quality changed to {}",
                 qualityStr.toStdString());

        // Clear cache since quality affects rendering
        clearCache();
        emit renderQualityChanged(renderQuality);
    }
}

RenderModel::RenderQuality RenderModel::getRenderQuality() const {
    return renderQuality;
}

// Helper method to get DPI multiplier based on quality
double RenderModel::getQualityMultiplier() const {
    switch (renderQuality) {
        case RenderQuality::Draft:
            return 0.5;
        case RenderQuality::Normal:
            return 1.0;
        case RenderQuality::High:
            return 1.5;
        case RenderQuality::Ultra:
            return 2.0;
        default:
            return 1.0;
    }
}

// Cache Management Methods
void RenderModel::clearCache() {
    int oldSize = pageCache.size();
    pageCache.clear();

    if (oldSize > 0) {
        LOG_INFO("RenderModel: Cache cleared, removed {} items", oldSize);
        emit cacheSizeChanged(0);
    }
}

void RenderModel::clearPageFromCache(int pageNum) {
    // Find and remove all cache entries for this page
    QStringList keysToRemove;

    // Since QCache doesn't provide key iteration, we'll use a simple approach
    // Generate common cache keys for this page and try to remove them
    QList<double> commonDpis = {72.0, 96.0, 144.0, 150.0, 300.0};
    QList<QPair<int, int>> commonSizes = {{-1, -1}, {0, 0}};

    for (double dpi : commonDpis) {
        for (auto size : commonSizes) {
            QString key = generateCacheKey(pageNum, dpi, dpi, 0, 0, size.first,
                                           size.second);
            if (pageCache.contains(key)) {
                pageCache.remove(key);
                keysToRemove.append(key);
            }
        }
    }

    if (!keysToRemove.isEmpty()) {
        LOG_DEBUG("RenderModel: Removed {} cache entries for page {}",
                  keysToRemove.size(), pageNum);
        emit cacheUpdated(pageNum);
        emit cacheSizeChanged(pageCache.size());
    }
}

int RenderModel::getCacheSize() const { return pageCache.size(); }

int RenderModel::getMaxCacheSize() const { return maxCacheSize; }

void RenderModel::setMaxCacheSize(int size) {
    if (size <= 0) {
        LOG_WARNING("RenderModel: Invalid cache size: {}, using default 50",
                    size);
        size = 50;
    }

    if (maxCacheSize != size) {
        int oldSize = maxCacheSize;
        maxCacheSize = size;

        // Update cache max cost (in MB)
        pageCache.setMaxCost(maxCacheSize * 1024 * 1024);

        LOG_INFO("RenderModel: Cache max size changed from {} to {} MB",
                 oldSize, maxCacheSize);
        emit cacheSizeChanged(pageCache.size());
    }
}

bool RenderModel::isPageCached(int pageNum, double xres, double yres) const {
    QString key = generateCacheKey(pageNum, xres, yres, 0, 0, -1, -1);
    return pageCache.contains(key);
}

// Helper method to generate cache keys
QString RenderModel::generateCacheKey(int pageNum, double xres, double yres,
                                      int x, int y, int w, int h) const {
    // Include quality in cache key since it affects rendering
    QString qualityStr;
    switch (renderQuality) {
        case RenderQuality::Draft:
            qualityStr = "D";
            break;
        case RenderQuality::Normal:
            qualityStr = "N";
            break;
        case RenderQuality::High:
            qualityStr = "H";
            break;
        case RenderQuality::Ultra:
            qualityStr = "U";
            break;
    }

    return QString("p%1_x%2_y%3_q%4_%5_%6_%7_%8")
        .arg(pageNum)
        .arg(xres, 0, 'f', 1)
        .arg(yres, 0, 'f', 1)
        .arg(qualityStr)
        .arg(x)
        .arg(y)
        .arg(w)
        .arg(h);
}

// Async Rendering Methods
void RenderModel::renderPageAsync(int pageNum, double xres, double yres, int x,
                                  int y, int w, int h) {
    // Input validation
    if (!document) {
        LOG_WARNING("RenderModel: Document not loaded for async render");
        emit asyncRenderFailed(pageNum, "Document not loaded");
        return;
    }

    if (pageNum < 0 || pageNum >= document->numPages()) {
        LOG_WARNING(
            "RenderModel: Invalid page number {} for async render (document "
            "has {} pages)",
            pageNum, document->numPages());
        emit asyncRenderFailed(pageNum,
                               QString("Invalid page number %1").arg(pageNum));
        return;
    }

    // Check if already rendering this page
    if (asyncRenders.contains(pageNum)) {
        LOG_DEBUG("RenderModel: Page {} already being rendered asynchronously",
                  pageNum);
        return;
    }

    // Check cache first
    QString cacheKey = generateCacheKey(pageNum, xres, yres, x, y, w, h);
    if (pageCache.contains(cacheKey)) {
        QImage* cachedImage = pageCache.object(cacheKey);
        if (cachedImage && !cachedImage->isNull()) {
            LOG_DEBUG("RenderModel: Page {} found in cache for async request",
                      pageNum);
            emit asyncRenderCompleted(pageNum, *cachedImage);
            return;
        }
    }

    // Validate DPI values
    double actualDpiX = (xres > 0) ? xres : dpiX;
    double actualDpiY = (yres > 0) ? yres : dpiY;

    if (actualDpiX <= 0 || actualDpiY <= 0) {
        LOG_WARNING(
            "RenderModel: Invalid DPI values for async render: x={}, y={}",
            actualDpiX, actualDpiY);
        actualDpiX = actualDpiY = 72.0;  // Fallback to default
    }

    // Apply quality multiplier
    double qualityMultiplier = getQualityMultiplier();
    actualDpiX *= qualityMultiplier;
    actualDpiY *= qualityMultiplier;

    LOG_INFO(
        "RenderModel: Starting async render for page {} (DPI: {}x{}, Quality: "
        "{})",
        pageNum, actualDpiX, actualDpiY, static_cast<int>(renderQuality));

    // Create future watcher
    QFutureWatcher<QImage>* watcher = new QFutureWatcher<QImage>(this);
    asyncRenders[pageNum] = watcher;

    // Connect completion signal
    connect(watcher, &QFutureWatcher<QImage>::finished, this,
            [this, pageNum, cacheKey]() { onAsyncRenderCompleted(); });

    // Start async rendering
    QFuture<QImage> future = QtConcurrent::run([this, pageNum, actualDpiX,
                                                actualDpiY, x, y, w,
                                                h]() -> QImage {
        try {
            // Create a copy of the document pointer for thread safety
            Poppler::Document* doc = document;
            if (!doc) {
                return QImage();
            }

            std::unique_ptr<Poppler::Page> pdfPage(doc->page(pageNum));
            if (!pdfPage) {
                return QImage();
            }

            // Render the page
            QImage image =
                pdfPage->renderToImage(actualDpiX, actualDpiY, x, y, w, h);
            return image;

        } catch (const std::exception& e) {
            LOG_ERROR(
                "RenderModel: Exception in async render thread for page {}: {}",
                pageNum, e.what());
            return QImage();
        } catch (...) {
            LOG_ERROR(
                "RenderModel: Unknown exception in async render thread for "
                "page {}",
                pageNum);
            return QImage();
        }
    });

    watcher->setFuture(future);
}

void RenderModel::cancelAsyncRender(int pageNum) {
    if (asyncRenders.contains(pageNum)) {
        QFutureWatcher<QImage>* watcher = asyncRenders[pageNum];
        if (watcher) {
            watcher->cancel();
            watcher->deleteLater();
            asyncRenders.remove(pageNum);
            LOG_INFO("RenderModel: Cancelled async render for page {}",
                     pageNum);
        }
    }
}

bool RenderModel::isRenderingAsync(int pageNum) const {
    return asyncRenders.contains(pageNum) && asyncRenders[pageNum] &&
           asyncRenders[pageNum]->isRunning();
}

void RenderModel::onAsyncRenderCompleted() {
    QObject* s = sender();
    auto watcher = static_cast<QFutureWatcher<QImage>*>(s);
    if (!watcher) {
        return;
    }

    // Find which page this watcher belongs to
    int pageNum = -1;
    for (auto it = asyncRenders.begin(); it != asyncRenders.end(); ++it) {
        if (it.value() == watcher) {
            pageNum = it.key();
            break;
        }
    }

    if (pageNum == -1) {
        LOG_WARNING(
            "RenderModel: Async render completed but page number not found");
        watcher->deleteLater();
        return;
    }

    // Remove from active renders
    asyncRenders.remove(pageNum);

    try {
        QImage result = watcher->result();

        if (result.isNull()) {
            LOG_ERROR("RenderModel: Async render failed for page {}", pageNum);
            emit asyncRenderFailed(pageNum, "Rendering failed");
        } else {
            LOG_DEBUG(
                "RenderModel: Async render completed for page {} (size: {}x{})",
                pageNum, result.width(), result.height());

            // Cache the result
            QString cacheKey =
                generateCacheKey(pageNum, dpiX, dpiY, 0, 0, -1, -1);
            int imageCost = result.sizeInBytes();
            pageCache.insert(cacheKey, new QImage(result), imageCost);

            emit cacheUpdated(pageNum);
            emit asyncRenderCompleted(pageNum, result);
        }
    } catch (const std::exception& e) {
        LOG_ERROR(
            "RenderModel: Exception retrieving async render result for page "
            "{}: {}",
            pageNum, e.what());
        emit asyncRenderFailed(pageNum, QString("Exception: %1").arg(e.what()));
    } catch (...) {
        LOG_ERROR(
            "RenderModel: Unknown exception retrieving async render result for "
            "page {}",
            pageNum);
        emit asyncRenderFailed(pageNum, "Unknown exception occurred");
    }

    watcher->deleteLater();
}

// Validation Methods
bool RenderModel::isDocumentValid() const {
    if (!document) {
        lastError = "Document not loaded";
        return false;
    }

    try {
        // Basic validation - check if we can get page count
        int pageCount = document->numPages();
        if (pageCount <= 0) {
            lastError = "Document has no pages";
            return false;
        }

        // Try to access the first page to ensure document is readable
        std::unique_ptr<Poppler::Page> firstPage(document->page(0));
        if (!firstPage) {
            lastError = "Cannot access document pages";
            return false;
        }

        LOG_DEBUG("RenderModel: Document validation passed ({} pages)",
                  pageCount);
        lastError.clear();
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("RenderModel: Exception during document validation: {}",
                  e.what());
        lastError = QString("Validation exception: %1").arg(e.what());
        return false;
    } catch (...) {
        LOG_ERROR("RenderModel: Unknown exception during document validation");
        lastError = "Unknown validation exception";
        return false;
    }
}

bool RenderModel::hasPage(int pageNum) const {
    if (!document) {
        lastError = "Document not loaded";
        return false;
    }

    try {
        int pageCount = document->numPages();
        bool hasPage = (pageNum >= 0 && pageNum < pageCount);

        if (!hasPage) {
            lastError =
                QString("Page %1 does not exist (document has %2 pages)")
                    .arg(pageNum)
                    .arg(pageCount);
        } else {
            lastError.clear();
        }

        return hasPage;

    } catch (const std::exception& e) {
        LOG_ERROR(
            "RenderModel: Exception checking page existence for page {}: {}",
            pageNum, e.what());
        lastError = QString("Exception: %1").arg(e.what());
        return false;
    } catch (...) {
        LOG_ERROR(
            "RenderModel: Unknown exception checking page existence for page "
            "{}",
            pageNum);
        lastError = "Unknown exception occurred";
        return false;
    }
}

bool RenderModel::isPageValid(int pageNum) const {
    if (!hasPage(pageNum)) {
        return false;
    }

    try {
        std::unique_ptr<Poppler::Page> pdfPage(document->page(pageNum));
        if (!pdfPage) {
            lastError = QString("Cannot access page %1").arg(pageNum);
            return false;
        }

        // Check if page has valid dimensions
        QSizeF size = pdfPage->pageSizeF();
        if (size.width() <= 0 || size.height() <= 0) {
            lastError = QString("Page %1 has invalid dimensions").arg(pageNum);
            return false;
        }

        LOG_DEBUG("RenderModel: Page {} validation passed (size: {}x{})",
                  pageNum, size.width(), size.height());
        lastError.clear();
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("RenderModel: Exception validating page {}: {}", pageNum,
                  e.what());
        lastError = QString("Page validation exception: %1").arg(e.what());
        return false;
    } catch (...) {
        LOG_ERROR("RenderModel: Unknown exception validating page {}", pageNum);
        lastError = "Unknown page validation exception";
        return false;
    }
}

QString RenderModel::getLastError() const { return lastError; }

QImage RenderModel::renderPage(int pageNum, double xres, double yres, int x,
                               int y, int w, int h) {
    // Input validation
    if (!document) {
        LOG_WARNING("RenderModel: Document not loaded");
        lastError = "Document not loaded";
        return QImage();
    }

    if (pageNum < 0 || pageNum >= document->numPages()) {
        LOG_WARNING(
            "RenderModel: Invalid page number {} (document has {} pages)",
            pageNum, document->numPages());
        lastError = QString("Invalid page number %1").arg(pageNum);
        return QImage();
    }

    // Check cache first
    QString cacheKey = generateCacheKey(pageNum, xres, yres, x, y, w, h);
    if (pageCache.contains(cacheKey)) {
        QImage* cachedImage = pageCache.object(cacheKey);
        if (cachedImage && !cachedImage->isNull()) {
            LOG_DEBUG("RenderModel: Page {} found in cache", pageNum);
            emit renderPageDone(*cachedImage);
            return *cachedImage;
        }
    }

    // Validate DPI values
    double actualDpiX = (xres > 0) ? xres : dpiX;
    double actualDpiY = (yres > 0) ? yres : dpiY;

    if (actualDpiX <= 0 || actualDpiY <= 0) {
        LOG_WARNING("RenderModel: Invalid DPI values: x={}, y={}", actualDpiX,
                    actualDpiY);
        actualDpiX = actualDpiY = 72.0;  // Fallback to default
    }

    // Apply quality multiplier
    double qualityMultiplier = getQualityMultiplier();
    actualDpiX *= qualityMultiplier;
    actualDpiY *= qualityMultiplier;

    try {
        std::unique_ptr<Poppler::Page> pdfPage(document->page(pageNum));
        if (!pdfPage) {
            LOG_WARNING("RenderModel: Failed to get page {}", pageNum);
            return QImage();
        }

        // Render with quality-adjusted DPI
        QImage image =
            pdfPage->renderToImage(actualDpiX, actualDpiY, x, y, w, h);

        if (image.isNull()) {
            LOG_ERROR(
                "RenderModel: Failed to render page {} (DPI: {}x{}, Quality: "
                "{})",
                pageNum, actualDpiX, actualDpiY,
                static_cast<int>(renderQuality));
            lastError = QString("Failed to render page %1").arg(pageNum);
            return QImage();
        }

        LOG_DEBUG(
            "RenderModel: Successfully rendered page {} (size: {}x{}, Quality: "
            "{})",
            pageNum, image.width(), image.height(),
            static_cast<int>(renderQuality));

        // Cache the result
        int imageCost = image.sizeInBytes();
        pageCache.insert(cacheKey, new QImage(image), imageCost);
        emit cacheUpdated(pageNum);

        lastError.clear();
        emit renderPageDone(image);
        return image;

    } catch (const std::exception& e) {
        LOG_ERROR("RenderModel: Exception while rendering page {}: {}", pageNum,
                  e.what());
        return QImage();
    } catch (...) {
        LOG_ERROR("RenderModel: Unknown exception while rendering page {}",
                  pageNum);
        return QImage();
    }
}

int RenderModel::getPageCount() {
    if (!document) {
        LOG_DEBUG("RenderModel: No document loaded, returning 0 pages");
        return 0;
    }

    try {
        int pageCount = document->numPages();
        LOG_DEBUG("RenderModel: Document has {} pages", pageCount);
        return pageCount;
    } catch (const std::exception& e) {
        LOG_ERROR("RenderModel: Exception while getting page count: {}",
                  e.what());
        return 0;
    } catch (...) {
        LOG_ERROR("RenderModel: Unknown exception while getting page count");
        return 0;
    }
}

// Static method to configure document render hints for high-quality rendering
void RenderModel::configureDocumentRenderHints(Poppler::Document* doc) {
    if (!doc) {
        return;
    }

    // Enable all high-quality rendering hints for optimal output
    // These settings ensure crisp rendering on all displays, especially
    // high-DPI
    doc->setRenderHint(Poppler::Document::Antialiasing, true);
    doc->setRenderHint(Poppler::Document::TextAntialiasing, true);
    doc->setRenderHint(Poppler::Document::TextHinting, true);
    doc->setRenderHint(Poppler::Document::TextSlightHinting, true);
    doc->setRenderHint(Poppler::Document::ThinLineShape, true);
    doc->setRenderHint(Poppler::Document::OverprintPreview, true);

    LOG_DEBUG(
        "RenderModel: Configured document with high-quality render hints");
}

void RenderModel::setDocument(Poppler::Document* _document) {
    if (document == _document) {
        LOG_DEBUG("RenderModel: Document already set, ignoring");
        return;
    }

    // Cancel any pending async renders
    for (auto it = asyncRenders.begin(); it != asyncRenders.end(); ++it) {
        if (it.value()) {
            it.value()->cancel();
            it.value()->deleteLater();
        }
    }
    asyncRenders.clear();

    // Clear cache when document changes
    clearCache();

    if (!_document) {
        LOG_INFO("RenderModel: Setting document to null");
        document = nullptr;
        emit documentChanged(document);
        emit documentValidationChanged(false);
        return;
    }

    try {
        // Validate the document before setting it
        int pageCount = _document->numPages();
        if (pageCount <= 0) {
            LOG_WARNING("RenderModel: Document has no pages, rejecting");
            return;
        }

        LOG_INFO("RenderModel: Setting new document with {} pages", pageCount);

        // Configure document for high-quality rendering
        configureDocumentRenderHints(_document);

        // document.reset(_document);       //
        // 这里不能用reset，因为_document是外部传入的智能指针，
        //  app\model\DocumentModel.cpp已经reset()过了
        document = _document;  //  直接赋值防止重复reset导致崩溃

        emit documentChanged(document);
        emit documentValidationChanged(isDocumentValid());

    } catch (const std::exception& e) {
        LOG_ERROR("RenderModel: Exception while setting document: {}",
                  e.what());
    } catch (...) {
        LOG_ERROR("RenderModel: Unknown exception while setting document");
    }
}
