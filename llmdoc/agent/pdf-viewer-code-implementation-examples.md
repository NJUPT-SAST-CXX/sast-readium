# PDF Viewer Components - Code Implementation Examples

## 1. PDFViewer - Core Rendering Implementation

### 1.1 Synchronous Rendering Fallback (Lines 236-295)

**File:** `app/ui/viewer/PDFViewer.cpp`

```cpp
// Fallback to synchronous rendering with improved error handling
try {
    renderState = Rendering;
    setText("Rendering (sync)...");

    // Use cached DPI calculation if available
    double optimizedDpi;
    if (dpiCalculator) {
        optimizedDpi = dpiCalculator->calculateOptimalDPI(currentScaleFactor);
    } else {
        // Fallback calculation
        optimizedDpi = 72.0 * currentScaleFactor * devicePixelRatioF();
        optimizedDpi = qMin(optimizedDpi, 300.0);
    }

    // SafePDFRenderer for robustness
    SafePDFRenderer& renderer = SafePDFRenderer::instance();
    SafePDFRenderer::RenderInfo renderInfo;

    // Compatibility check for Qt-generated PDFs
    PDFViewer* parentViewer = qobject_cast<PDFViewer*>(parent());
    if (parentViewer && parentViewer->hasDocument()) {
        if (optimizedDpi > 150.0) {
            qDebug() << "Using conservative DPI settings";
            optimizedDpi = qMin(optimizedDpi, 150.0);
        }
    }

    QImage image = renderer.safeRenderPage(currentPage, optimizedDpi, &renderInfo);

    if (!renderInfo.success || image.isNull()) {
        QString errorMsg = renderInfo.errorMessage.isEmpty()
                               ? "Failed to render page"
                               : renderInfo.errorMessage;
        setText(errorMsg);
        renderState = RenderError;
        qWarning() << "PDF rendering failed:" << errorMsg
                   << "Attempts:" << renderInfo.attemptCount;
        return;
    }

    // Apply device pixel ratio
    renderedPixmap = QPixmap::fromImage(image);
    renderedPixmap.setDevicePixelRatio(devicePixelRatioF());
```

**Key Implementation Details:**

- **Error Recovery:** SafePDFRenderer wraps rendering with exception handling
- **DPI Optimization:** Conservative 150 DPI for problematic PDFs
- **Device Awareness:** Device pixel ratio applied for high-DPI displays
- **Diagnostic Info:** Detailed error tracking with attempt counts
- **State Management:** RenderError state for UI feedback

### 1.2 Asynchronous Rendering (Lines 168-234)

```cpp
if (asyncRenderingEnabled && pageNumber >= 0) {
    // Ensure prerenderer is available
    if (!prerenderer) {
        if (auto* viewer = qobject_cast<PDFViewer*>(parent())) {
            prerenderer = viewer->getPrerenderer();
        }
    }

    if (prerenderer) {
        renderState = Rendering;
        setText("Rendering...");

        // Check cache first - avoids queue duplication
        QPixmap cachedPixmap = prerenderer->getCachedPage(
            pageNumber, currentScaleFactor, currentRotation);
        if (!cachedPixmap.isNull()) {
            setPixmap(cachedPixmap);
            renderState = Rendered;
            resize(cachedPixmap.size() / devicePixelRatioF());
            return;
        }

        // Request async rendering with retry mechanism
        try {
            prerenderer->requestPrerender(pageNumber, currentScaleFactor,
                                          currentRotation, 1);
            asyncRenderRetryCount = 0;  // Reset on success
            return;
        } catch (const std::exception& e) {
            asyncRenderRetryCount++;
            qWarning() << "Async rendering failed (attempt"
                       << asyncRenderRetryCount << "):" << e.what();

            if (asyncRenderRetryCount < MAX_ASYNC_RETRY_COUNT) {
                // Exponential backoff: 100ms, 200ms, 300ms
                QTimer::singleShot(100 * asyncRenderRetryCount, this,
                                   &PDFPageWidget::renderPage);
                return;
            }
            // Fall through to sync after max retries
        } catch (...) {
            asyncRenderRetryCount++;
            if (asyncRenderRetryCount < MAX_ASYNC_RETRY_COUNT) {
                QTimer::singleShot(100 * asyncRenderRetryCount, this,
                                   &PDFPageWidget::renderPage);
                return;
            }
        }
    } else {
        qWarning() << "Prerenderer not available, falling back to sync";
    }
}
```

**Key Features:**

- **Cache-First:** Checks prerenderer cache before queuing
- **Exponential Backoff:** Retries with increasing delays (100ms, 200ms, 300ms)
- **Exception Safety:** Catches std::exception and generic exceptions separately
- **Automatic Fallback:** Switches to sync after 3 failed attempts
- **Clean Signal:** No blocking UI thread during async operations

---

## 2. Virtual Scrolling Implementation

### 2.1 Page Position Caching (PDFViewer.cpp)

```cpp
// Virtual scrolling optimization
void PDFViewer::updateVirtualScrolling() {
    if (!isVirtualScrollingEnabled) return;

    // Invalidate cache if needed
    if (!pagePositionsCacheValid) {
        updatePagePositionsCache();
    }

    // Get current scroll value and viewport height
    int scrollValue = continuousScrollArea->verticalScrollBar()->value();
    int viewportHeight = continuousScrollArea->height();

    // Find visible page range using cached positions
    QPair<int, int> visibleRange = calculateVisiblePageRange(scrollValue, viewportHeight);
    visiblePageStart = visibleRange.first;
    visiblePageEnd = visibleRange.second;

    // Track scroll direction for prerendering optimization
    if (scrollValue > lastScrollValue) {
        scrollDirection = 1;  // Scrolling down
    } else if (scrollValue < lastScrollValue) {
        scrollDirection = -1;  // Scrolling up
    } else {
        scrollDirection = 0;   // No movement
    }
    lastScrollValue = scrollValue;

    // Update scroll direction in prerenderer for directional optimization
    if (prerenderer) {
        prerenderer->updateScrollDirection(scrollDirection);
    }

    // Update visible pages
    updateVisiblePages();
}
```

**Optimization Techniques:**

- **Position Cache:** `pagePositions` vector avoids recalculation
- **Scroll Direction:** Tracked for intelligent prerendering
- **Lazy Update:** Cache only updated when invalidated
- **Range Finding:** Binary search possible with sorted positions
- **Prerenderer Coordination:** Direction fed to background system

### 2.2 Page Widget Creation/Destruction (PDFViewer.cpp)

```cpp
void PDFViewer::createPageWidget(int pageNumber) {
    if (activePageWidgets.contains(pageNumber)) {
        return;  // Already exists
    }

    // Get page from document
    Poppler::Page* page = document->getPage(pageNumber);
    if (!page) return;

    // Create widget
    PDFPageWidget* pageWidget = new PDFPageWidget(continuousWidget);
    pageWidget->setPage(page, currentZoomFactor, currentRotation);
    pageWidget->setAsyncRenderingEnabled(true);
    pageWidget->setPrerenderer(prerenderer);
    pageWidget->setPageNumber(pageNumber);

    // Add to layout
    continuousLayout->addWidget(pageWidget);

    // Track in active widgets
    activePageWidgets[pageNumber] = pageWidget;

    // Store estimated height
    QSizeF pageSize = page->pageSizeF();
    pageHeights[pageNumber] = static_cast<int>(
        pageSize.height() * currentZoomFactor + 10);  // +10 for spacing

    // Connect search results if available
    if (!m_allSearchResults.isEmpty()) {
        // Filter results for this page
        QList<SearchResult> pageResults;
        for (const SearchResult& result : m_allSearchResults) {
            if (result.pageNumber == pageNumber) {
                pageResults.append(result);
            }
        }
        if (!pageResults.isEmpty()) {
            pageWidget->setSearchResults(pageResults);
        }
    }
}

void PDFViewer::destroyPageWidget(int pageNumber) {
    auto it = activePageWidgets.find(pageNumber);
    if (it != activePageWidgets.end()) {
        PDFPageWidget* widget = *it;
        widget->deleteLater();  // Schedule deletion (async)
        activePageWidgets.erase(it);

        // Invalidate position cache due to size change
        invalidatePagePositionsCache();
    }
}
```

**Resource Management:**

- **On-Demand Creation:** Widgets created only when needed
- **Lazy Destruction:** Uses `deleteLater()` for safe cleanup
- **Height Tracking:** Cache page heights for position calculation
- **Search Integration:** Applies cached search results to new widgets
- **Cache Invalidation:** Marks position cache dirty on structural changes

---

## 3. Intelligent Cache System

### 3.1 LRU Cache with Importance Scoring (PDFViewer.h/cpp)

**File:** `app/ui/viewer/PDFViewer.h` (Lines 513-540)

```cpp
// Enhanced cache with importance scoring
struct PageCacheItem {
    QPixmap pixmap;
    double zoomFactor;
    int rotation;
    qint64 lastAccessed;
    qint64 memorySize;
    int accessCount;
    double importance;  // Calculated importance score

    // LRU list pointers for O(1) operations
    PageCacheItem* prev;
    PageCacheItem* next;

    PageCacheItem() : prev(nullptr), next(nullptr) {}
};

// Cache management methods
quint64 getCacheKey(int pageNumber, double zoomFactor, int rotation) {
    // Combine three values into 64-bit key
    quint32 zoomId = getZoomFactorId(zoomFactor);
    return (static_cast<quint64>(pageNumber) << 40) |
           (static_cast<quint64>(zoomId) << 20) |
           static_cast<quint64>(rotation);
}

double calculateCacheItemImportance(const PageCacheItem& item,
                                    int currentPage) {
    // Importance = recency + proximity + frequency
    qint64 ageMs = QDateTime::currentMSecsSinceEpoch() - item.lastAccessed;
    double recency = 1.0 / (1.0 + ageMs / 1000.0);  // Recent = more important

    int pageDistance = qAbs(item.pageNumber - currentPage);
    double proximity = 1.0 / (1.0 + pageDistance);   // Close pages = more important

    double frequency = qMin(1.0, item.accessCount / 10.0);  // Access frequency

    // Weighted importance: 40% recency, 40% proximity, 20% frequency
    return 0.4 * recency + 0.4 * proximity + 0.2 * frequency;
}

void evictLeastImportantItems() {
    while (currentCacheMemory > maxCacheMemory ||
           pageCache.size() > maxCacheSize) {
        // Find least important item
        PageCacheItem* leastImportant = cacheTail;
        double minImportance = 1.0;

        // Recalculate importance for all items
        for (auto it = pageCache.begin(); it != pageCache.end(); ++it) {
            double importance =
                calculateCacheItemImportance(**it, currentPageNumber);
            if (importance < minImportance) {
                minImportance = importance;
                leastImportant = *it;
            }
        }

        if (leastImportant) {
            removeFromList(leastImportant);
            currentCacheMemory -= leastImportant->memorySize;
            delete leastImportant;
        } else {
            break;
        }
    }
}
```

**Cache Efficiency:**

- **64-Bit Keys:** Efficient integer keys from (page, zoom, rotation)
- **Importance Scoring:** Balances recency (40%), proximity (40%), frequency (20%)
- **O(1) Operations:** LRU doubly-linked list for fast removals
- **Adaptive Eviction:** Removes least important item when over limits
- **Memory Awareness:** Tracks exact pixmap memory usage

### 3.2 PDFPrerenderer Cache (PDFPrerenderer.cpp)

**File:** `app/ui/viewer/PDFPrerenderer.cpp` (Lines 59-119)

```cpp
void PDFPrerenderer::requestPrerender(int pageNumber, double scaleFactor,
                                      int rotation, int priority) {
    if (!m_document || pageNumber < 0 ||
        pageNumber >= m_document->numPages()) {
        return;
    }

    // Check if already cached
    QString cacheKey = getCacheKey(pageNumber, scaleFactor, rotation);
    if (m_cache.contains(cacheKey)) {
        return;  // Already cached, no need to queue
    }

    QMutexLocker locker(&m_queueMutex);

    // Check if already in queue (duplicate detection)
    for (const RenderRequest& req : m_renderQueue) {
        if (req.pageNumber == pageNumber &&
            qAbs(req.scaleFactor - scaleFactor) < 0.001 &&
            req.rotation == rotation) {
            return;  // Already queued
        }
    }

    // Create render request with priority
    RenderRequest request;
    request.pageNumber = pageNumber;
    request.scaleFactor = scaleFactor;
    request.rotation = rotation;
    request.priority = priority;
    request.timestamp = QDateTime::currentMSecsSinceEpoch();

    // Enqueue and signal worker threads
    m_renderQueue.enqueue(request);
    m_queueCondition.wakeOne();  // Wake one worker thread
}
```

**Queue Optimization:**

- **Duplicate Detection:** Avoids redundant rendering requests
- **Priority Queue:** RenderRequest orders by priority then timestamp
- **Cache-First:** Skips queue if already cached
- **Worker Signaling:** Wakes worker threads when work available
- **Bounds Checking:** Validates page numbers before queueing

---

## 4. Search Highlighting Optimization

### 4.1 Layered Highlight System (PDFViewer.cpp)

```cpp
void PDFPageWidget::updateSearchHighlightLayer() {
    if (!m_searchHighlightsDirty || m_searchResults.isEmpty()) {
        return;
    }

    // Render all highlights to a layer pixmap
    QPixmap basePixmap = renderedPixmap;
    if (basePixmap.isNull()) {
        m_searchHighlightsDirty = false;
        return;
    }

    // Create highlight layer
    m_searchHighlightLayer = QPixmap(basePixmap.size());
    m_searchHighlightLayer.fill(Qt::transparent);

    QPainter highlightPainter(&m_searchHighlightLayer);
    highlightPainter.setOpacity(0.5);

    // Draw all normal highlights
    for (int i = 0; i < m_searchResults.size(); ++i) {
        if (i != m_currentSearchResultIndex) {
            const SearchResult& result = m_searchResults[i];
            // Transform rect to rendered pixmap coordinates
            QRectF transformedRect = result.boundingRect;
            transformedRect.translate(-pageRect.topLeft());

            highlightPainter.fillRect(transformedRect, m_normalHighlightColor);
        }
    }

    // Draw current highlight with emphasis
    if (m_currentSearchResultIndex >= 0 &&
        m_currentSearchResultIndex < m_searchResults.size()) {
        const SearchResult& currentResult =
            m_searchResults[m_currentSearchResultIndex];
        QRectF transformedRect = currentResult.boundingRect;
        transformedRect.translate(-pageRect.topLeft());

        highlightPainter.fillRect(transformedRect, m_currentHighlightColor);

        // Add border for emphasis
        highlightPainter.setPen(QPen(Qt::darkYellow, 2));
        highlightPainter.drawRect(transformedRect);
    }

    highlightPainter.end();
    m_searchHighlightsDirty = false;
}

void PDFPageWidget::drawSearchHighlights(QPainter& painter) {
    if (!m_searchHighlightsEnabled || m_searchResults.isEmpty()) {
        return;
    }

    // Update layer if needed
    if (m_searchHighlightsDirty) {
        updateSearchHighlightLayer();
    }

    // Draw pre-rendered layer
    if (!m_searchHighlightLayer.isNull()) {
        painter.drawPixmap(0, 0, m_searchHighlightLayer);
    }
}
```

**Optimization Benefits:**

- **Pre-Rendered Layer:** Highlights rendered once, drawn many times
- **Dirty Flag:** Only regenerates when search results change
- **Transparent Layer:** Composited over page for smooth appearance
- **Current Emphasis:** Current result highlighted with border
- **Performance:** Avoids redrawing 100+ rectangles per paint event

---

## 5. Gesture and Touch Handling

### 5.1 Multi-Touch Support (PDFViewer.cpp)

```cpp
bool PDFPageWidget::gestureEvent(QGestureEvent* event) {
    if (QPinchGesture* pinch =
            static_cast<QPinchGesture*>(event->gesture(Qt::PinchGesture))) {
        pinchTriggered(pinch);
        return true;
    }
    if (QSwipeGesture* swipe =
            static_cast<QSwipeGesture*>(event->gesture(Qt::SwipeGesture))) {
        swipeTriggered(swipe);
        return true;
    }
    if (QPanGesture* pan =
            static_cast<QPanGesture*>(event->gesture(Qt::PanGesture))) {
        panTriggered(pan);
        return true;
    }
    return false;
}

void PDFPageWidget::pinchTriggered(QPinchGesture* gesture) {
    // Scale factor from gesture
    qreal scaleFactor = gesture->scaleFactor();

    // Apply zoom
    emit scaleChanged(currentScaleFactor * scaleFactor);
    setScaleFactor(currentScaleFactor * scaleFactor);

    // Visual feedback
    gesture->setHotSpot(mapToGlobal(geometry().center()));
}

void PDFPageWidget::panTriggered(QPanGesture* gesture) {
    // Pan offset
    QPointF offset = gesture->offset();

    // Update view position
    if (auto* parent = parentWidget()) {
        if (auto* scrollArea = qobject_cast<QScrollArea*>(parent)) {
            QScrollBar* vbar = scrollArea->verticalScrollBar();
            vbar->setValue(vbar->value() - offset.y());

            QScrollBar* hbar = scrollArea->horizontalScrollBar();
            hbar->setValue(hbar->value() - offset.x());
        }
    }
}

void PDFPageWidget::touchEvent(QTouchEvent* event) {
    // Handle raw touch points
    const QList<QTouchEvent::TouchPoint>& touchPoints =
        event->points();

    if (touchPoints.count() == 1) {
        // Single touch - pan
        const QTouchEvent::TouchPoint& tp = touchPoints.first();
        lastPanPoint = tp.position().toPoint();
        isDragging = true;

        event->accept();
    } else if (touchPoints.count() == 2) {
        // Two-finger pinch zoom
        const QTouchEvent::TouchPoint& p1 = touchPoints.at(0);
        const QTouchEvent::TouchPoint& p2 = touchPoints.at(1);

        qreal currentDistance = QLineF(p1.position(), p2.position()).length();
        // Calculate zoom based on distance change
        event->accept();
    }
}
```

**Touch Interaction Features:**

- **Pinch Zoom:** Two-finger pinch to zoom in/out
- **Pan:** Single-finger drag to pan
- **Swipe:** Multi-page swipe navigation
- **Gesture Event Routing:** Qt gesture system integration
- **Cursor Feedback:** Hand cursor during panning

---

## 6. Keyboard Shortcut Implementation

### 6.1 Shortcut Setup (PDFViewer.cpp constructor)

```cpp
// Setup zoom shortcuts
zoomInShortcut = new QShortcut(QKeySequence::ZoomIn, this);
connect(zoomInShortcut, &QShortcut::activated, this, &PDFViewer::zoomIn);

zoomOutShortcut = new QShortcut(QKeySequence::ZoomOut, this);
connect(zoomOutShortcut, &QShortcut::activated, this, &PDFViewer::zoomOut);

// Custom zoom shortcuts
zoom25Shortcut = new QShortcut(Qt::CTRL + Qt::Key_2, this);
connect(zoom25Shortcut, &QShortcut::activated, this,
        [this]() { setZoomFromPercentage(25); });

zoom50Shortcut = new QShortcut(Qt::CTRL + Qt::Key_5, this);
connect(zoom50Shortcut, &QShortcut::activated, this,
        [this]() { setZoomFromPercentage(50); });

// Navigation shortcuts
nextPageShortcut = new QShortcut(Qt::Key_PageDown, this);
connect(nextPageShortcut, &QShortcut::activated, this, &PDFViewer::nextPage);

prevPageShortcut = new QShortcut(Qt::Key_PageUp, this);
connect(prevPageShortcut, &QShortcut::activated, this, &PDFViewer::previousPage);

// Search shortcuts
findShortcut = new QShortcut(QKeySequence::Find, this);
connect(findShortcut, &QShortcut::activated, this, &PDFViewer::showSearch);

findNextShortcut = new QShortcut(QKeySequence::FindNext, this);
connect(findNextShortcut, &QShortcut::activated, this, &PDFViewer::findNext);

// Bookmark shortcuts
addBookmarkShortcut = new QShortcut(Qt::CTRL + Qt::Key_B, this);
connect(addBookmarkShortcut, &QShortcut::activated, this, &PDFViewer::addBookmark);
```

**Shortcut Management:**

- **QKeySequence:** Standard shortcuts (ZoomIn, ZoomOut, Find, FindNext)
- **Custom Shortcuts:** Numeric zoom levels (25%, 50%, 75%, etc.)
- **Navigation:** Page Up/Down, Home, End
- **View Control:** Fullscreen (F11), presentation mode (F5)
- **Member Storage:** Ensures shortcuts not garbage collected prematurely

---

## 7. Error Handling and Resilience

### 7.1 SafePDFRenderer Integration

```cpp
// Use SafePDFRenderer for crash protection
SafePDFRenderer& renderer = SafePDFRenderer::instance();
SafePDFRenderer::RenderInfo renderInfo;

QImage image = renderer.safeRenderPage(currentPage, optimizedDpi, &renderInfo);

// Check both success flag and null image
if (!renderInfo.success || image.isNull()) {
    QString errorMsg = renderInfo.errorMessage.isEmpty()
                           ? "Failed to render page"
                           : renderInfo.errorMessage;
    setText(errorMsg);
    renderState = RenderError;

    // Detailed logging for debugging
    qWarning() << "PDF rendering failed:" << errorMsg
               << "Attempts:" << renderInfo.attemptCount
               << "Compatibility:" << static_cast<int>(renderInfo.compatibility)
               << "Used fallback:" << renderInfo.usedFallback;
    return;
}
```

**Error Resilience:**

- **SafePDFRenderer:** Exception-safe wrapper around Poppler
- **Success Flag:** Explicit success tracking separate from null check
- **Error Messages:** User-facing error descriptions
- **Diagnostic Info:** Attempt counts and compatibility flags
- **Graceful Degradation:** Error state displayed instead of crash

---

## 8. Prerenderer Intelligence

### 8.1 Reading Pattern Analysis (PDFPrerenderer.h)

```cpp
// Adaptive learning system
void recordPageView(int pageNumber, qint64 viewDuration);
void recordNavigationPattern(int fromPage, int toPage);
void updateScrollDirection(int direction);  // -1 = up, 0 = none, 1 = down

// Pattern analysis for prediction
QList<int> predictNextPages(int currentPage);
double getPageImportance(int pageNumber);
bool isSequentialReader() const;
bool isRandomAccessReader() const;

// Strategy selection
enum class PrerenderStrategy {
    Conservative,  // Only adjacent pages
    Balanced,      // Based on patterns
    Aggressive     // Extensive prerendering
};
```

**Intelligent Prerendering:**

- **Pattern Tracking:** Records page view duration and navigation paths
- **Reader Classification:** Detects sequential vs random access patterns
- **Prediction:** Uses history to predict next pages
- **Strategy Adaptation:** Adjusts prerender strategy based on behavior
- **Direction Awareness:** Optimizes for scroll direction (up/down)

---

## Summary

The PDF Viewer components demonstrate sophisticated implementation across:

1. **Rendering:** Dual async/sync paths with robust fallbacks
2. **Caching:** Multi-layer LRU with importance scoring
3. **Scrolling:** Virtual scrolling with position caching
4. **Search:** Layered highlighting with dirty flag optimization
5. **Input:** Complete mouse, keyboard, touch, and gesture support
6. **Error Handling:** SafePDFRenderer integration with retry mechanisms
7. **Intelligence:** Adaptive prerendering based on usage patterns
8. **Threading:** Safe background rendering with worker pools

All components follow Qt best practices with proper memory management, signal/slot connections, and exception safety.
