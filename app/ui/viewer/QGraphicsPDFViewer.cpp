#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT

#include "QGraphicsPDFViewer.h"
#include <QApplication>
#include <QDebug>
#include <QFuture>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStyleOptionGraphicsItem>
#include <QtConcurrent/QtConcurrent>
#include <cmath>

// QGraphicsPDFPageItem Implementation
QGraphicsPDFPageItem::QGraphicsPDFPageItem(QGraphicsItem* parent)
    : QObject(nullptr),
      QGraphicsPixmapItem(parent),
      m_page(nullptr),
      m_scaleFactor(1.0),
      m_rotation(0),
      m_pageNumber(-1),
      m_highQualityEnabled(true),
      m_isRendering(false),
      m_currentSearchResultIndex(-1),
      m_normalHighlightColor(255, 255, 0, 100),
      m_currentHighlightColor(255, 165, 0, 150) {
    setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
    setTransformationMode(Qt::SmoothTransformation);

    // Setup render timer for debouncing
    m_renderTimer = new QTimer();
    m_renderTimer->setSingleShot(true);
    m_renderTimer->setInterval(100);
    QObject::connect(m_renderTimer, &QTimer::timeout,
                     [this]() { renderPage(); });

    // Setup render watcher
    m_renderWatcher = new QFutureWatcher<QPixmap>();
    QObject::connect(m_renderWatcher, &QFutureWatcher<QPixmap>::finished,
                     [this]() { onRenderCompleted(); });
}

void QGraphicsPDFPageItem::setPage(Poppler::Page* page, double scaleFactor,
                                   int rotation) {
    m_page = page;
    m_scaleFactor = qBound(0.1, scaleFactor, 10.0);
    m_rotation = ((rotation % 360) + 360) % 360;

    if (page) {
        m_pageNumber = page->index();
        renderPageAsync();
    } else {
        setPixmap(QPixmap());
    }
}

void QGraphicsPDFPageItem::setScaleFactor(double factor) {
    double newFactor = qBound(0.1, factor, 10.0);
    if (qAbs(newFactor - m_scaleFactor) > 0.01) {
        m_scaleFactor = newFactor;
        renderPageAsync();
    }
}

void QGraphicsPDFPageItem::setRotation(int degrees) {
    int newRotation = ((degrees % 360) + 360) % 360;
    if (newRotation != m_rotation) {
        m_rotation = newRotation;
        renderPageAsync();
    }
}

void QGraphicsPDFPageItem::renderPageAsync() {
    if (!m_page || m_isRendering)
        return;

    // Cancel any pending render
    if (m_renderWatcher && m_renderWatcher->isRunning()) {
        m_renderWatcher->cancel();
    }

    m_renderTimer->start();
}

void QGraphicsPDFPageItem::renderPageSync() {
    if (!m_page)
        return;

    // Render synchronously for testing
    double dpi = 72.0 * m_scaleFactor * qApp->devicePixelRatio();

    QImage image = m_page->renderToImage(
        dpi, dpi, -1, -1, -1, -1,
        static_cast<Poppler::Page::Rotation>(m_rotation / 90));

    if (!image.isNull()) {
        QPixmap pixmap = QPixmap::fromImage(image);
        pixmap.setDevicePixelRatio(qApp->devicePixelRatio());
        setPixmap(pixmap);
        update();
    }
}

void QGraphicsPDFPageItem::setHighQualityRendering(bool enabled) {
    if (m_highQualityEnabled != enabled) {
        m_highQualityEnabled = enabled;
        setTransformationMode(enabled ? Qt::SmoothTransformation
                                      : Qt::FastTransformation);
        renderPageAsync();
    }
}

void QGraphicsPDFPageItem::renderPage() {
    if (!m_page || m_isRendering)
        return;

    m_isRendering = true;

    // Render in background thread
    QFuture<QPixmap> future = QtConcurrent::run([this]() -> QPixmap {
        double dpi = 72.0 * m_scaleFactor * qApp->devicePixelRatio();

        QImage image = m_page->renderToImage(
            dpi, dpi, -1, -1, -1, -1,
            static_cast<Poppler::Page::Rotation>(m_rotation / 90));

        if (image.isNull()) {
            return QPixmap();
        }

        QPixmap pixmap = QPixmap::fromImage(image);
        pixmap.setDevicePixelRatio(qApp->devicePixelRatio());

        return pixmap;
    });

    m_renderWatcher->setFuture(future);
}

void QGraphicsPDFPageItem::onRenderCompleted() {
    m_isRendering = false;

    if (m_renderWatcher->isCanceled()) {
        return;
    }

    QPixmap pixmap = m_renderWatcher->result();
    if (!pixmap.isNull()) {
        setPixmap(pixmap);
        update();
    }
}

void QGraphicsPDFPageItem::setSearchResults(const QList<QRectF>& results) {
    m_searchResults = results;
    m_currentSearchResultIndex = -1;
    update();
}

void QGraphicsPDFPageItem::clearSearchHighlights() {
    m_searchResults.clear();
    m_currentSearchResultIndex = -1;
    update();
}

void QGraphicsPDFPageItem::setCurrentSearchResult(int index) {
    if (index >= 0 && index < m_searchResults.size()) {
        m_currentSearchResultIndex = index;
        update();
    }
}

void QGraphicsPDFPageItem::paint(QPainter* painter,
                                 const QStyleOptionGraphicsItem* option,
                                 QWidget* widget) {
    // Draw the page pixmap
    QGraphicsPixmapItem::paint(painter, option, widget);

    // Draw search highlights
    if (!m_searchResults.isEmpty()) {
        drawSearchHighlights(painter);
    }
}

QRectF QGraphicsPDFPageItem::boundingRect() const {
    return QGraphicsPixmapItem::boundingRect();
}

void QGraphicsPDFPageItem::drawSearchHighlights(QPainter* painter) {
    painter->save();

    for (int i = 0; i < m_searchResults.size(); ++i) {
        QColor color = (i == m_currentSearchResultIndex)
                           ? m_currentHighlightColor
                           : m_normalHighlightColor;
        painter->fillRect(m_searchResults[i], color);
    }

    painter->restore();
}

// QGraphicsPDFScene Implementation
QGraphicsPDFScene::QGraphicsPDFScene(QObject* parent)
    : QGraphicsScene(parent),
      m_document(nullptr),
      m_pageSpacing(20),
      m_pageMargin(50),
      m_scaleFactor(1.0),
      m_rotation(0),
      m_highQualityEnabled(true) {
    setBackgroundBrush(QBrush(QColor(128, 128, 128)));
}

void QGraphicsPDFScene::setDocument(Poppler::Document* document) {
    clearDocument();
    m_document = document;

    if (document) {
        // Add all pages
        for (int i = 0; i < document->numPages(); ++i) {
            addPage(i);
        }
        updateLayout();
    }
}

void QGraphicsPDFScene::clearDocument() {
    removeAllPages();
    m_document = nullptr;
}

void QGraphicsPDFScene::addPage(int pageNumber) {
    if (!m_document || pageNumber < 0 || pageNumber >= m_document->numPages()) {
        return;
    }

    if (m_pageItems.contains(pageNumber)) {
        return;  // Page already added
    }

    std::unique_ptr<Poppler::Page> page(m_document->page(pageNumber));
    if (!page) {
        return;
    }

    QGraphicsPDFPageItem* pageItem = new QGraphicsPDFPageItem();
    pageItem->setPage(page.get(), m_scaleFactor, m_rotation);
    pageItem->setHighQualityRendering(m_highQualityEnabled);

    addItem(pageItem);
    m_pageItems[pageNumber] = pageItem;
}

void QGraphicsPDFScene::removePage(int pageNumber) {
    if (m_pageItems.contains(pageNumber)) {
        QGraphicsPDFPageItem* item = m_pageItems.take(pageNumber);
        removeItem(item);
        delete item;
    }
}

void QGraphicsPDFScene::removeAllPages() {
    for (auto it = m_pageItems.begin(); it != m_pageItems.end(); ++it) {
        removeItem(it.value());
        delete it.value();
    }
    m_pageItems.clear();
}

QGraphicsPDFPageItem* QGraphicsPDFScene::getPageItem(int pageNumber) const {
    return m_pageItems.value(pageNumber, nullptr);
}

int QGraphicsPDFScene::getPageCount() const {
    return m_document ? m_document->numPages() : 0;
}

void QGraphicsPDFScene::setPageSpacing(int spacing) {
    if (m_pageSpacing != spacing) {
        m_pageSpacing = spacing;
        updateLayout();
    }
}

void QGraphicsPDFScene::setPageMargin(int margin) {
    if (m_pageMargin != margin) {
        m_pageMargin = margin;
        updateLayout();
    }
}

void QGraphicsPDFScene::updateLayout() { layoutPages(); }

void QGraphicsPDFScene::setHighQualityRendering(bool enabled) {
    if (m_highQualityEnabled != enabled) {
        m_highQualityEnabled = enabled;
        for (auto* item : m_pageItems) {
            item->setHighQualityRendering(enabled);
        }
    }
}

void QGraphicsPDFScene::setScaleFactor(double factor) {
    double newFactor = qBound(0.1, factor, 10.0);
    if (qAbs(newFactor - m_scaleFactor) > 0.01) {
        m_scaleFactor = newFactor;
        for (auto* item : m_pageItems) {
            item->setScaleFactor(newFactor);
        }
        updateLayout();
        emit scaleChanged(m_scaleFactor);
    }
}

void QGraphicsPDFScene::setRotation(int degrees) {
    int newRotation = ((degrees % 360) + 360) % 360;
    if (newRotation != m_rotation) {
        m_rotation = newRotation;
        for (auto* item : m_pageItems) {
            item->setRotation(newRotation);
        }
        updateLayout();
    }
}

void QGraphicsPDFScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsScene::mousePressEvent(event);

    QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
    if (QGraphicsPDFPageItem* pageItem =
            qgraphicsitem_cast<QGraphicsPDFPageItem*>(item)) {
        QPointF localPos = pageItem->mapFromScene(event->scenePos());
        emit pageClicked(pageItem->getPageNumber(), localPos);
    }
}

void QGraphicsPDFScene::layoutPages() {
    if (m_pageItems.isEmpty()) {
        return;
    }

    qreal yOffset = m_pageMargin;

    // Layout pages vertically
    for (int i = 0; i < getPageCount(); ++i) {
        QGraphicsPDFPageItem* item = getPageItem(i);
        if (!item)
            continue;

        QRectF boundingRect = item->boundingRect();
        qreal xOffset = (sceneRect().width() - boundingRect.width()) / 2.0;

        item->setPos(xOffset, yOffset);
        yOffset += boundingRect.height() + m_pageSpacing;
    }

    // Update scene rect
    if (!m_pageItems.isEmpty()) {
        QRectF totalRect;
        for (auto* item : m_pageItems) {
            totalRect = totalRect.united(item->sceneBoundingRect());
        }
        totalRect.adjust(-m_pageMargin, -m_pageMargin, m_pageMargin,
                         m_pageMargin);
        setSceneRect(totalRect);
    }
}

// QGraphicsPDFViewer Implementation
QGraphicsPDFViewer::QGraphicsPDFViewer(QWidget* parent)
    : QGraphicsView(parent),
      m_scene(nullptr),
      m_document(nullptr),
      m_viewMode(SinglePage),
      m_currentPage(0),
      m_zoomFactor(1.0),
      m_rotation(0),
      m_highQualityEnabled(true),
      m_smoothScrollingEnabled(true),
      m_pageSpacing(20),
      m_pageMargin(50),
      m_isPanning(false),
      m_rubberBand(nullptr) {
    setupView();

    // Setup timers
    m_updateTimer = new QTimer(this);
    m_updateTimer->setSingleShot(true);
    m_updateTimer->setInterval(100);
    connect(m_updateTimer, &QTimer::timeout, this,
            &QGraphicsPDFViewer::updateCurrentPage);

    m_renderTimer = new QTimer(this);
    m_renderTimer->setSingleShot(true);
    m_renderTimer->setInterval(200);
}

QGraphicsPDFViewer::~QGraphicsPDFViewer() { clearDocument(); }

void QGraphicsPDFViewer::setupView() {
    // Create scene
    m_scene = new QGraphicsPDFScene(this);
    setScene(m_scene);

    // Connect scene signals
    connect(m_scene, &QGraphicsPDFScene::pageClicked, this,
            &QGraphicsPDFViewer::onScenePageClicked);
    connect(m_scene, &QGraphicsPDFScene::scaleChanged, this,
            &QGraphicsPDFViewer::onSceneScaleChanged);

    // Configure view
    setDragMode(QGraphicsView::NoDrag);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setOptimizationFlags(QGraphicsView::DontSavePainterState |
                         QGraphicsView::DontAdjustForAntialiasing);

    // Enable smooth scrolling
    if (m_smoothScrollingEnabled) {
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }

    // Set background
    setBackgroundBrush(QBrush(QColor(128, 128, 128)));
}

void QGraphicsPDFViewer::setDocument(Poppler::Document* document) {
    clearDocument();
    m_document = document;

    if (document) {
        m_scene->setDocument(document);
        m_currentPage = 0;

        // Configure document for optimal rendering
        document->setRenderHint(Poppler::Document::Antialiasing,
                                m_highQualityEnabled);
        document->setRenderHint(Poppler::Document::TextAntialiasing,
                                m_highQualityEnabled);

        updateViewTransform();
        centerOnPage(0);

        emit documentChanged(true);
        emit currentPageChanged(0);
    } else {
        emit documentChanged(false);
    }
}

void QGraphicsPDFViewer::clearDocument() {
    if (m_scene) {
        m_scene->clearDocument();
    }
    m_document = nullptr;
    m_currentPage = 0;
}

void QGraphicsPDFViewer::goToPage(int pageNumber) {
    if (!m_document || pageNumber < 0 || pageNumber >= getPageCount()) {
        return;
    }

    m_currentPage = pageNumber;
    centerOnPage(pageNumber);
    emit currentPageChanged(pageNumber);
}

void QGraphicsPDFViewer::nextPage() {
    if (m_currentPage < getPageCount() - 1) {
        goToPage(m_currentPage + 1);
    }
}

void QGraphicsPDFViewer::previousPage() {
    if (m_currentPage > 0) {
        goToPage(m_currentPage - 1);
    }
}

void QGraphicsPDFViewer::firstPage() { goToPage(0); }

void QGraphicsPDFViewer::lastPage() { goToPage(getPageCount() - 1); }

void QGraphicsPDFViewer::zoomIn() { setZoom(m_zoomFactor * 1.25); }

void QGraphicsPDFViewer::zoomOut() { setZoom(m_zoomFactor / 1.25); }

void QGraphicsPDFViewer::zoomToFit() { fitToView(); }

void QGraphicsPDFViewer::zoomToWidth() { fitToWidth(); }

void QGraphicsPDFViewer::zoomToHeight() { fitToHeight(); }

void QGraphicsPDFViewer::setZoom(double factor) {
    double newFactor = qBound(0.1, factor, 10.0);
    if (qAbs(newFactor - m_zoomFactor) > 0.01) {
        m_zoomFactor = newFactor;
        m_scene->setScaleFactor(newFactor);
        updateViewTransform();
        emit zoomChanged(newFactor);
    }
}

void QGraphicsPDFViewer::resetZoom() { setZoom(1.0); }

void QGraphicsPDFViewer::rotateLeft() { setRotation(m_rotation - 90); }

void QGraphicsPDFViewer::rotateRight() { setRotation(m_rotation + 90); }

void QGraphicsPDFViewer::resetRotation() { setRotation(0); }

void QGraphicsPDFViewer::setRotation(int degrees) {
    int newRotation = ((degrees % 360) + 360) % 360;
    if (newRotation != m_rotation) {
        m_rotation = newRotation;
        m_scene->setRotation(newRotation);
        updateViewTransform();
        emit rotationChanged(newRotation);
    }
}

void QGraphicsPDFViewer::setViewMode(ViewMode mode) {
    if (m_viewMode != mode) {
        m_viewMode = mode;
        updateViewTransform();
    }
}

void QGraphicsPDFViewer::setHighQualityRendering(bool enabled) {
    if (m_highQualityEnabled != enabled) {
        m_highQualityEnabled = enabled;
        m_scene->setHighQualityRendering(enabled);

        if (m_document) {
            m_document->setRenderHint(Poppler::Document::Antialiasing, enabled);
            m_document->setRenderHint(Poppler::Document::TextAntialiasing,
                                      enabled);
        }

        setRenderHints(
            enabled ? (QPainter::Antialiasing | QPainter::SmoothPixmapTransform)
                    : QPainter::RenderHints());
    }
}

void QGraphicsPDFViewer::setPageSpacing(int spacing) {
    if (m_pageSpacing != spacing) {
        m_pageSpacing = spacing;
        m_scene->setPageSpacing(spacing);
    }
}

void QGraphicsPDFViewer::setPageMargin(int margin) {
    if (m_pageMargin != margin) {
        m_pageMargin = margin;
        m_scene->setPageMargin(margin);
    }
}

void QGraphicsPDFViewer::setSmoothScrolling(bool enabled) {
    m_smoothScrollingEnabled = enabled;
}

int QGraphicsPDFViewer::getPageCount() const {
    return m_document ? m_document->numPages() : 0;
}

void QGraphicsPDFViewer::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom with Ctrl+Wheel
        const double scaleFactor = 1.15;
        if (event->angleDelta().y() > 0) {
            setZoom(m_zoomFactor * scaleFactor);
        } else {
            setZoom(m_zoomFactor / scaleFactor);
        }
        event->accept();
    } else {
        // Normal scrolling
        QGraphicsView::wheelEvent(event);
        m_updateTimer->start();
    }
}

void QGraphicsPDFViewer::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        // Start panning
        m_isPanning = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else if (event->button() == Qt::LeftButton &&
               event->modifiers() & Qt::ControlModifier) {
        // Start rubber band selection
        m_rubberBandOrigin = event->pos();
        if (!m_rubberBand) {
            m_rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
        }
        m_rubberBand->setGeometry(QRect(m_rubberBandOrigin, QSize()));
        m_rubberBand->show();
        event->accept();
    } else {
        QGraphicsView::mousePressEvent(event);
    }
}

void QGraphicsPDFViewer::mouseMoveEvent(QMouseEvent* event) {
    if (m_isPanning && (event->buttons() & Qt::MiddleButton)) {
        // Pan the view
        QPoint delta = event->pos() - m_lastPanPoint;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() -
                                        delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        m_lastPanPoint = event->pos();
        event->accept();
    } else if (m_rubberBand && m_rubberBand->isVisible()) {
        // Update rubber band
        m_rubberBand->setGeometry(
            QRect(m_rubberBandOrigin, event->pos()).normalized());
        event->accept();
    } else {
        QGraphicsView::mouseMoveEvent(event);
    }
}

void QGraphicsPDFViewer::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton && m_isPanning) {
        // End panning
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else if (m_rubberBand && m_rubberBand->isVisible()) {
        // End rubber band selection
        m_rubberBand->hide();

        // Zoom to selected area
        QRect selection = m_rubberBand->geometry();
        if (selection.width() > 10 && selection.height() > 10) {
            QRectF sceneRect = mapToScene(selection).boundingRect();
            fitInView(sceneRect, Qt::KeepAspectRatio);
            m_zoomFactor = transform().m11();
            emit zoomChanged(m_zoomFactor);
        }
        event->accept();
    } else {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

void QGraphicsPDFViewer::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_PageUp:
            previousPage();
            event->accept();
            break;
        case Qt::Key_PageDown:
            nextPage();
            event->accept();
            break;
        case Qt::Key_Home:
            firstPage();
            event->accept();
            break;
        case Qt::Key_End:
            lastPage();
            event->accept();
            break;
        case Qt::Key_Plus:
        case Qt::Key_Equal:
            if (event->modifiers() & Qt::ControlModifier) {
                zoomIn();
                event->accept();
            }
            break;
        case Qt::Key_Minus:
            if (event->modifiers() & Qt::ControlModifier) {
                zoomOut();
                event->accept();
            }
            break;
        case Qt::Key_0:
            if (event->modifiers() & Qt::ControlModifier) {
                resetZoom();
                event->accept();
            }
            break;
        default:
            QGraphicsView::keyPressEvent(event);
            break;
    }
}

void QGraphicsPDFViewer::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);

    // Update view if needed
    if (m_viewMode == SinglePage) {
        m_renderTimer->start();
    }
}

void QGraphicsPDFViewer::onScenePageClicked(int pageNumber, QPointF position) {
    if (pageNumber != m_currentPage) {
        m_currentPage = pageNumber;
        emit currentPageChanged(pageNumber);
    }
    emit pageClicked(pageNumber, position);
}

void QGraphicsPDFViewer::onSceneScaleChanged(double scale) {
    m_zoomFactor = scale;
    emit zoomChanged(scale);
}

void QGraphicsPDFViewer::updateCurrentPage() {
    if (!m_document)
        return;

    // Find the page that's most visible in the viewport
    QRectF viewportRect = mapToScene(viewport()->rect()).boundingRect();

    int bestPage = m_currentPage;
    double bestOverlap = 0.0;

    for (int i = 0; i < getPageCount(); ++i) {
        QGraphicsPDFPageItem* pageItem = m_scene->getPageItem(i);
        if (!pageItem)
            continue;

        QRectF pageRect = pageItem->sceneBoundingRect();
        QRectF intersection = viewportRect.intersected(pageRect);

        if (!intersection.isEmpty()) {
            double overlap = intersection.width() * intersection.height();
            if (overlap > bestOverlap) {
                bestOverlap = overlap;
                bestPage = i;
            }
        }
    }

    if (bestPage != m_currentPage) {
        m_currentPage = bestPage;
        emit currentPageChanged(bestPage);
    }
}

void QGraphicsPDFViewer::updateViewTransform() {
    if (!m_scene)
        return;

    // Update scene layout
    m_scene->updateLayout();
}

void QGraphicsPDFViewer::centerOnPage(int pageNumber) {
    QGraphicsPDFPageItem* pageItem = m_scene->getPageItem(pageNumber);
    if (pageItem) {
        centerOn(pageItem);
    }
}

void QGraphicsPDFViewer::fitToView() {
    if (!m_document || getPageCount() == 0)
        return;

    QGraphicsPDFPageItem* pageItem = m_scene->getPageItem(m_currentPage);
    if (pageItem) {
        fitInView(pageItem, Qt::KeepAspectRatio);
        m_zoomFactor = transform().m11();
        emit zoomChanged(m_zoomFactor);
    }
}

void QGraphicsPDFViewer::fitToWidth() {
    if (!m_document || getPageCount() == 0)
        return;

    QGraphicsPDFPageItem* pageItem = m_scene->getPageItem(m_currentPage);
    if (pageItem) {
        QRectF pageRect = pageItem->boundingRect();
        QRectF viewRect = viewport()->rect();

        double scale = viewRect.width() / pageRect.width();
        setZoom(scale);
        centerOnPage(m_currentPage);
    }
}

void QGraphicsPDFViewer::fitToHeight() {
    if (!m_document || getPageCount() == 0)
        return;

    QGraphicsPDFPageItem* pageItem = m_scene->getPageItem(m_currentPage);
    if (pageItem) {
        QRectF pageRect = pageItem->boundingRect();
        QRectF viewRect = viewport()->rect();

        double scale = viewRect.height() / pageRect.height();
        setZoom(scale);
        centerOnPage(m_currentPage);
    }
}

#endif  // ENABLE_QGRAPHICS_PDF_SUPPORT
