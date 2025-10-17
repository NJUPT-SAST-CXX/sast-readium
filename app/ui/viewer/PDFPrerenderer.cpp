#include "PDFPrerenderer.h"
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QMutexLocker>
#include <QThread>
#include <QTimer>
#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <algorithm>
#include <cmath>
#include "../../model/RenderModel.h"
#include "../../utils/SafePDFRenderer.h"

// PDFPrerenderer Implementation
PDFPrerenderer::PDFPrerenderer(QObject* parent)
    : QObject(parent),
      m_document(nullptr),
      m_workerThreads(),
      m_workers(),
      m_strategy(PrerenderStrategy::Balanced),
      m_maxWorkerThreads(QThread::idealThreadCount()),
      m_maxCacheSize(100),
      m_maxMemoryUsage(512 * 1024 * 1024),  // 512MB
      m_renderQueue(),
      m_queueMutex(),
      m_queueCondition(),
      m_isRunning(false),
      m_isPaused(false),
      m_cache(),
      m_currentMemoryUsage(0),
      m_cacheHits(0),
      m_cacheMisses(0),
      m_pageViewTimes(),
      m_navigationPatterns(),
      m_adaptiveTimer(nullptr),
      m_accessHistory(),
      m_prerenderRange(3),
      m_currentScrollDirection(0),
      m_lastScrollTime() {
    // Setup adaptive analysis timer
    m_adaptiveTimer = new QTimer(this);
    m_adaptiveTimer->setInterval(30000);  // 30 seconds
    connect(m_adaptiveTimer, &QTimer::timeout, this,
            &PDFPrerenderer::onAdaptiveAnalysis);

    // Initialize scroll direction tracking
    m_lastScrollTime = QTime::currentTime();

    setupWorkerThreads();
}

PDFPrerenderer::~PDFPrerenderer() {
    stopPrerendering();
    cleanupWorkerThreads();
}

void PDFPrerenderer::setDocument(Poppler::Document* document) {
    // DEADLOCK FIX: Update workers outside of queue mutex to prevent deadlocks
    // since worker->setDocument() also acquires mutexes

    // First, update workers with new document
    for (PDFRenderWorker* worker : m_workers) {
        worker->setDocument(document);
    }

    // Then update our document and clear cache under mutex
    QMutexLocker locker(&m_queueMutex);

    m_document = document;

    // Configure document for optimal rendering using centralized method
    if (m_document) {
        RenderModel::configureDocumentRenderHints(m_document);
    }

    // Clear cache when document changes
    m_cache.clear();
    m_currentMemoryUsage = 0;
}

void PDFPrerenderer::setStrategy(PrerenderStrategy strategy) {
    m_strategy = strategy;
}

void PDFPrerenderer::requestPrerender(int pageNumber, double scaleFactor,
                                      int rotation, int priority) {
    if (!m_document || pageNumber < 0 || pageNumber >= m_document->numPages()) {
        return;
    }

    // Check if already cached
    QString cacheKey = getCacheKey(pageNumber, scaleFactor, rotation);
    if (m_cache.contains(cacheKey)) {
        return;
    }

    QMutexLocker locker(&m_queueMutex);

    // Check if already in queue
    for (const RenderRequest& req : m_renderQueue) {
        if (req.pageNumber == pageNumber &&
            qAbs(req.scaleFactor - scaleFactor) < 0.001 &&
            req.rotation == rotation) {
            return;  // Already queued
        }
    }

    RenderRequest request;
    request.pageNumber = pageNumber;
    request.scaleFactor = scaleFactor;
    request.rotation = rotation;
    request.priority = priority;
    request.timestamp = QDateTime::currentMSecsSinceEpoch();

    m_renderQueue.enqueue(request);
    m_queueCondition.wakeOne();
}

QPixmap PDFPrerenderer::getCachedPage(int pageNumber, double scaleFactor,
                                      int rotation) {
    QString cacheKey = getCacheKey(pageNumber, scaleFactor, rotation);

    if (m_cache.contains(cacheKey)) {
        CacheItem& item = m_cache[cacheKey];
        item.timestamp = QDateTime::currentMSecsSinceEpoch();
        item.accessCount++;
        m_cacheHits++;
        return item.pixmap;
    }

    m_cacheMisses++;
    return QPixmap();
}

bool PDFPrerenderer::hasPrerenderedPage(int pageNumber, double scaleFactor,
                                        int rotation) {
    QString cacheKey = getCacheKey(pageNumber, scaleFactor, rotation);
    return m_cache.contains(cacheKey);
}

void PDFPrerenderer::startPrerendering() {
    if (m_isRunning) {
        return;
    }

    m_isRunning = true;
    m_isPaused = false;

    // Start worker threads
    for (QThread* thread : m_workerThreads) {
        if (!thread->isRunning()) {
            thread->start();
        }
    }

    // Start adaptive analysis
    m_adaptiveTimer->start();

    emit prerenderingStarted();
}

void PDFPrerenderer::stopPrerendering() {
    if (!m_isRunning) {
        return;
    }

    m_isRunning = false;
    m_adaptiveTimer->stop();

    // DEADLOCK FIX: Stop all workers first, then wait for threads
    // This prevents potential deadlocks during shutdown
    for (PDFRenderWorker* worker : m_workers) {
        worker->stop();
    }

    // Wake up any waiting workers to ensure they can exit
    m_queueCondition.wakeAll();

    // Wait for threads to finish with timeout
    for (QThread* thread : m_workerThreads) {
        if (thread->isRunning()) {
            thread->quit();
            if (!thread->wait(3000)) {
                qWarning() << "PDFPrerenderer: Worker thread cleanup timeout, "
                              "terminating";
                thread->terminate();
                thread->wait(1000);
            }
        }
    }

    emit prerenderingStopped();
}

void PDFPrerenderer::recordPageView(int pageNumber, qint64 viewDuration) {
    if (!m_pageViewTimes.contains(pageNumber)) {
        m_pageViewTimes[pageNumber] = QList<qint64>();
    }

    QList<qint64>& times = m_pageViewTimes[pageNumber];
    times.append(viewDuration);

    // Keep only recent history (last 20 views)
    while (times.size() > 20) {
        times.removeFirst();
    }
}

void PDFPrerenderer::recordNavigationPattern(int fromPage, int toPage) {
    if (!m_navigationPatterns.contains(fromPage)) {
        m_navigationPatterns[fromPage] = QHash<int, int>();
    }

    m_navigationPatterns[fromPage][toPage]++;
}

void PDFPrerenderer::updateScrollDirection(int direction) {
    m_currentScrollDirection = direction;
    m_lastScrollTime = QTime::currentTime();

    // Trigger adaptive prerendering based on new scroll direction
    // This helps predict which pages are likely to be viewed next
    if (direction != 0) {
        // Schedule adaptive prerendering with scroll direction consideration
        QTimer::singleShot(100, this, [this]() {
            if (m_document && !m_accessHistory.isEmpty()) {
                int currentPage = m_accessHistory.last();
                scheduleAdaptivePrerendering(currentPage);
            }
        });
    }
}

void PDFPrerenderer::scheduleAdaptivePrerendering(int currentPage) {
    if (!m_document) {
        return;
    }

    QList<int> pagesToPrerender = predictNextPages(currentPage);

    for (int i = 0; i < pagesToPrerender.size(); ++i) {
        int pageNum = pagesToPrerender[i];
        int priority = calculatePriority(pageNum, currentPage);

        // Use current zoom and rotation settings
        requestPrerender(pageNum, 1.0, 0, priority);
    }
}

QList<int> PDFPrerenderer::predictNextPages(int currentPage) {
    QList<int> predictions;

    switch (m_strategy) {
        case PrerenderStrategy::Conservative:
            // Only adjacent pages, prioritize based on scroll direction
            if (m_currentScrollDirection <= 0 && currentPage > 0) {
                predictions.append(
                    currentPage - 1);  // Scrolling up, prioritize previous page
            }
            if (m_currentScrollDirection >= 0 &&
                currentPage < m_document->numPages() - 1) {
                predictions.append(currentPage +
                                   1);  // Scrolling down, prioritize next page
            }
            // Add the other direction with lower priority
            if (m_currentScrollDirection > 0 && currentPage > 0) {
                predictions.append(currentPage - 1);
            }
            if (m_currentScrollDirection < 0 &&
                currentPage < m_document->numPages() - 1) {
                predictions.append(currentPage + 1);
            }
            break;

        case PrerenderStrategy::Balanced:
            // Adjacent pages + navigation patterns, optimized for scroll
            // direction
            if (m_currentScrollDirection > 0) {
                // Scrolling down - prioritize pages ahead
                for (int offset = 1; offset <= 2; ++offset) {
                    int pageNum = currentPage + offset;
                    if (pageNum < m_document->numPages()) {
                        predictions.append(pageNum);
                    }
                }
                for (int offset = -1; offset >= -2; --offset) {
                    int pageNum = currentPage + offset;
                    if (pageNum >= 0) {
                        predictions.append(pageNum);
                    }
                }
            } else if (m_currentScrollDirection < 0) {
                // Scrolling up - prioritize pages behind
                for (int offset = -1; offset >= -2; --offset) {
                    int pageNum = currentPage + offset;
                    if (pageNum >= 0) {
                        predictions.append(pageNum);
                    }
                }
                for (int offset = 1; offset <= 2; ++offset) {
                    int pageNum = currentPage + offset;
                    if (pageNum < m_document->numPages()) {
                        predictions.append(pageNum);
                    }
                }
            } else {
                // No clear direction - use balanced approach
                for (int offset = -2; offset <= 2; ++offset) {
                    int pageNum = currentPage + offset;
                    if (pageNum >= 0 && pageNum < m_document->numPages() &&
                        pageNum != currentPage) {
                        predictions.append(pageNum);
                    }
                }
            }

            // Add pages based on navigation patterns
            if (m_navigationPatterns.contains(currentPage)) {
                const QHash<int, int>& patterns =
                    m_navigationPatterns[currentPage];
                QList<int> sortedPages;

                for (auto it = patterns.begin(); it != patterns.end(); ++it) {
                    sortedPages.append(it.key());
                }

                // Sort by frequency
                std::sort(sortedPages.begin(), sortedPages.end(),
                          [&patterns](int a, int b) {
                              return patterns[a] > patterns[b];
                          });

                // Add top 3 most likely pages
                for (int i = 0; i < qMin(3, sortedPages.size()); ++i) {
                    if (!predictions.contains(sortedPages[i])) {
                        predictions.append(sortedPages[i]);
                    }
                }
            }
            break;

        case PrerenderStrategy::Aggressive:
            // Wider range + patterns + sequential prediction, heavily optimized
            // for scroll direction
            if (m_currentScrollDirection > 0) {
                // Scrolling down - heavily prioritize pages ahead
                for (int offset = 1; offset <= 5; ++offset) {
                    int pageNum = currentPage + offset;
                    if (pageNum < m_document->numPages()) {
                        predictions.append(pageNum);
                    }
                }
                for (int offset = -1; offset >= -3; --offset) {
                    int pageNum = currentPage + offset;
                    if (pageNum >= 0) {
                        predictions.append(pageNum);
                    }
                }
            } else if (m_currentScrollDirection < 0) {
                // Scrolling up - heavily prioritize pages behind
                for (int offset = -1; offset >= -5; --offset) {
                    int pageNum = currentPage + offset;
                    if (pageNum >= 0) {
                        predictions.append(pageNum);
                    }
                }
                for (int offset = 1; offset <= 3; ++offset) {
                    int pageNum = currentPage + offset;
                    if (pageNum < m_document->numPages()) {
                        predictions.append(pageNum);
                    }
                }
            } else {
                // No clear direction - use full range
                for (int offset = -5; offset <= 5; ++offset) {
                    int pageNum = currentPage + offset;
                    if (pageNum >= 0 && pageNum < m_document->numPages() &&
                        pageNum != currentPage) {
                        predictions.append(pageNum);
                    }
                }
            }
            break;
    }

    return predictions;
}

int PDFPrerenderer::calculatePriority(int pageNumber, int currentPage) const {
    int distance = qAbs(pageNumber - currentPage);

    // Base priority on distance (closer = higher priority = lower number)
    int priority = distance;

    // Adjust priority based on scroll direction
    if (m_currentScrollDirection != 0) {
        int direction = (pageNumber > currentPage) ? 1 : -1;
        if (direction == m_currentScrollDirection) {
            // Page is in the scroll direction - higher priority
            priority = qMax(1, priority - 2);
        } else {
            // Page is against scroll direction - lower priority
            priority += 1;
        }
    }

    // Adjust based on navigation patterns
    if (m_navigationPatterns.contains(currentPage)) {
        const QHash<int, int>& patterns = m_navigationPatterns[currentPage];
        if (patterns.contains(pageNumber)) {
            int frequency = patterns[pageNumber];
            priority -= frequency;  // More frequent = higher priority
        }
    }

    // Ensure priority is positive
    return qMax(1, priority);
}

void PDFPrerenderer::setupWorkerThreads() {
    for (int i = 0; i < m_maxWorkerThreads; ++i) {
        auto* thread = new QThread(this);
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) - Qt manages lifetime
        // via deleteLater
        auto* worker = new PDFRenderWorker();

        worker->moveToThread(thread);
        worker->setDocument(m_document);

        connect(thread, &QThread::started, worker,
                &PDFRenderWorker::processRenderQueue);
        connect(worker, &PDFRenderWorker::pageRendered, this,
                &PDFPrerenderer::onRenderCompleted);

        m_workerThreads.append(thread);
        m_workers.append(worker);
    }
}

void PDFPrerenderer::cleanupWorkerThreads() {
    for (PDFRenderWorker* worker : m_workers) {
        worker->deleteLater();
    }
    m_workers.clear();

    for (QThread* thread : m_workerThreads) {
        thread->deleteLater();
    }
    m_workerThreads.clear();
}

void PDFPrerenderer::onRenderCompleted(int pageNumber, const QPixmap& pixmap,
                                       double scaleFactor, int rotation) {
    if (pixmap.isNull()) {
        return;
    }

    QString cacheKey = getCacheKey(pageNumber, scaleFactor, rotation);
    qint64 pixmapSize = getPixmapMemorySize(pixmap);

    // Evict items if necessary
    while (m_currentMemoryUsage + pixmapSize > m_maxMemoryUsage &&
           !m_cache.isEmpty()) {
        evictLRUItems();
    }

    // Add to cache
    CacheItem item;
    item.pixmap = pixmap;
    item.timestamp = QDateTime::currentMSecsSinceEpoch();
    item.memorySize = pixmapSize;
    item.accessCount = 0;

    m_cache[cacheKey] = item;
    m_currentMemoryUsage += pixmapSize;

    emit pagePrerendered(pageNumber, scaleFactor, rotation);
    emit cacheUpdated();
    emit memoryUsageChanged(m_currentMemoryUsage);
}

void PDFPrerenderer::onAdaptiveAnalysis() { analyzeReadingPatterns(); }

QString PDFPrerenderer::getCacheKey(int pageNumber, double scaleFactor,
                                    int rotation) {
    return QString("%1_%2_%3")
        .arg(pageNumber)
        .arg(scaleFactor, 0, 'f', 3)
        .arg(rotation);
}

void PDFPrerenderer::pausePrerendering() { m_isPaused = true; }

void PDFPrerenderer::resumePrerendering() {
    m_isPaused = false;
    // Wake up worker threads to resume processing
    m_queueCondition.wakeAll();
}

void PDFPrerenderer::setMaxWorkerThreads(int maxThreads) {
    m_maxWorkerThreads = qBound(1, maxThreads, QThread::idealThreadCount());
}

void PDFPrerenderer::setMaxCacheSize(int maxItems) {
    m_maxCacheSize = qMax(1, maxItems);

    // Evict items if current cache size exceeds new limit
    while (m_cache.size() > m_maxCacheSize) {
        evictLRUItems();
    }
}

void PDFPrerenderer::analyzeReadingPatterns() {
    // Simple reading pattern analysis
    // This could be expanded to track user behavior and optimize prerendering

    // For now, just adjust prerender range based on recent access patterns
    if (m_accessHistory.size() > 10) {
        // Calculate average jump distance
        int totalJumps = 0;
        int jumpCount = 0;

        for (int i = 1; i < m_accessHistory.size(); ++i) {
            int jump = qAbs(m_accessHistory[i] - m_accessHistory[i - 1]);
            if (jump > 0) {
                totalJumps += jump;
                jumpCount++;
            }
        }

        if (jumpCount > 0) {
            int avgJump = totalJumps / jumpCount;
            // Adjust prerender range based on jump patterns
            if (avgJump > 5) {
                m_prerenderRange = qMin(m_prerenderRange + 1, 10);
            } else if (avgJump < 2) {
                m_prerenderRange = qMax(m_prerenderRange - 1, 2);
            }
        }
    }
}

void PDFPrerenderer::evictLRUItems() {
    if (m_cache.isEmpty()) {
        return;
    }

    // Find least recently used item
    QString oldestKey;
    qint64 oldestTime = QDateTime::currentMSecsSinceEpoch();

    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it->timestamp < oldestTime) {
            oldestTime = it->timestamp;
            oldestKey = it.key();
        }
    }

    if (!oldestKey.isEmpty()) {
        m_currentMemoryUsage -= m_cache[oldestKey].memorySize;
        m_cache.remove(oldestKey);
    }
}

qint64 PDFPrerenderer::getPixmapMemorySize(const QPixmap& pixmap) {
    return pixmap.width() * pixmap.height() * 4;  // 32-bit ARGB
}

double PDFPrerenderer::cacheHitRatio() const {
    int total = m_cacheHits + m_cacheMisses;
    return total > 0 ? static_cast<double>(m_cacheHits) / total : 0.0;
}

// PDFRenderWorker Implementation
PDFRenderWorker::PDFRenderWorker(QObject* parent)
    : QObject(parent),
      m_document(nullptr),
      m_localQueue(),
      m_queueMutex(),
      m_queueCondition(),
      m_shouldStop(false) {}

void PDFRenderWorker::setDocument(Poppler::Document* document) {
    QMutexLocker locker(&m_queueMutex);
    m_document = document;
}

void PDFRenderWorker::addRenderRequest(
    const PDFPrerenderer::RenderRequest& request) {
    QMutexLocker locker(&m_queueMutex);
    m_localQueue.enqueue(request);
    m_queueCondition.wakeOne();
}

void PDFRenderWorker::clearQueue() {
    QMutexLocker locker(&m_queueMutex);
    m_localQueue.clear();
}

void PDFRenderWorker::stop() {
    QMutexLocker locker(&m_queueMutex);
    m_shouldStop = true;
    m_queueCondition.wakeOne();
}

void PDFRenderWorker::processRenderQueue() {
    while (!m_shouldStop) {
        PDFPrerenderer::RenderRequest request{};

        {
            QMutexLocker locker(&m_queueMutex);

            while (m_localQueue.isEmpty() && !m_shouldStop) {
                m_queueCondition.wait(&m_queueMutex);
            }

            if (m_shouldStop) {
                break;
            }

            request = m_localQueue.dequeue();
        }

        try {
            QPixmap pixmap = renderPage(request);
            if (!pixmap.isNull()) {
                emit pageRendered(request.pageNumber, pixmap,
                                  request.scaleFactor, request.rotation);
            }
        } catch (const std::exception& e) {
            emit renderError(request.pageNumber,
                             QString::fromStdString(e.what()));
        }
    }
}

QPixmap PDFRenderWorker::renderPage(
    const PDFPrerenderer::RenderRequest& request) {
    if (!m_document) {
        return QPixmap();
    }

    std::unique_ptr<Poppler::Page> page(m_document->page(request.pageNumber));
    if (!page) {
        return QPixmap();
    }

    // Calculate DPI including device pixel ratio for high-DPI displays
    double dpi = calculateOptimalDPI(request.scaleFactor);
    double deviceRatio = qApp != nullptr ? qApp->devicePixelRatio() : 1.0;

    // Use SafePDFRenderer for Qt PDF compatibility
    SafePDFRenderer& renderer = SafePDFRenderer::instance();
    SafePDFRenderer::RenderInfo renderInfo;

    QImage image = renderer.safeRenderPage(page.get(), dpi, &renderInfo);
    if (!renderInfo.success || image.isNull()) {
        // For prerendering, we can safely fail silently
        qDebug() << "PDFPrerenderer: Safe rendering failed for page" << request.pageNumber
                 << "Error:" << renderInfo.errorMessage;
        return QPixmap();
    }

    // Convert to pixmap and set device pixel ratio for proper high-DPI scaling
    QPixmap pixmap = QPixmap::fromImage(image);
    if (QApplication::instance()) {
        pixmap.setDevicePixelRatio(deviceRatio);
    }

    return pixmap;
}

double PDFRenderWorker::calculateOptimalDPI(double scaleFactor) {
    double baseDpi = 72.0;
    double deviceRatio = qApp->devicePixelRatio();
    return baseDpi * scaleFactor * deviceRatio;
}
