#include "PDFViewer.h"

// Business logic
#include "PDFPrerenderer.h"
#include "model/DocumentModel.h"
#include "model/PageModel.h"
#include "model/RenderModel.h"

// Logging
#include "logging/SimpleLogging.h"

// Poppler
#include <poppler/qt6/poppler-qt6.h>

// Qt
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>
#include "ElaText.h"

// ============================================================================
// PageWidget - 单个页面的显示组件
// ============================================================================

class PageWidget : public QWidget {
    Q_OBJECT

public:
    explicit PageWidget(int pageNumber, QWidget* parent = nullptr)
        : QWidget(parent), m_pageNumber(pageNumber), m_rotation(0) {
        setMinimumSize(100, 100);
    }

    void setImage(const QImage& image) {
        m_image = image;
        updateGeometry();
        update();
    }

    void setRotation(int rotation) {
        m_rotation = rotation;
        updateGeometry();
        update();
    }

    void setSearchHighlights(const QList<QRectF>& highlights) {
        m_highlights = highlights;
        update();
    }

    void clearSearchHighlights() {
        m_highlights.clear();
        update();
    }

    int pageNumber() const { return m_pageNumber; }

    QSize sizeHint() const override {
        if (m_image.isNull()) {
            return QSize(100, 100);
        }

        if (m_rotation == 90 || m_rotation == 270) {
            return QSize(m_image.height(), m_image.width());
        }
        return m_image.size();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        // 绘制背景
        painter.fillRect(rect(), Qt::white);

        if (m_image.isNull()) {
            // 绘制加载中提示
            painter.setPen(Qt::gray);
            painter.drawText(rect(), Qt::AlignCenter, tr("Loading..."));
            return;
        }

        // 应用旋转
        if (m_rotation != 0) {
            painter.translate(width() / 2, height() / 2);
            painter.rotate(m_rotation);
            painter.translate(-m_image.width() / 2, -m_image.height() / 2);
        }

        // 绘制页面图像
        painter.drawImage(0, 0, m_image);

        // 绘制搜索高亮
        if (!m_highlights.isEmpty()) {
            painter.setPen(QPen(QColor(255, 255, 0, 100), 2));
            painter.setBrush(QColor(255, 255, 0, 50));

            for (const QRectF& highlight : m_highlights) {
                QRectF scaledRect = highlight;
                // 缩放到图像坐标
                scaledRect.setX(scaledRect.x() * m_image.width());
                scaledRect.setY(scaledRect.y() * m_image.height());
                scaledRect.setWidth(scaledRect.width() * m_image.width());
                scaledRect.setHeight(scaledRect.height() * m_image.height());
                painter.drawRect(scaledRect);
            }
        }

        // 绘制边框
        painter.setPen(QPen(Qt::lightGray, 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
    }

private:
    int m_pageNumber;
    QImage m_image;
    int m_rotation;
    QList<QRectF> m_highlights;
};

// ============================================================================
// PDFViewer::Implementation - Pimpl 实现
// ============================================================================

class PDFViewer::Implementation {
public:
    // 文档相关
    std::shared_ptr<Poppler::Document> document;
    RenderModel* renderModel = nullptr;
    PageModel* pageModel = nullptr;
    bool qgraphicsRenderingEnabled = false;
    bool qgraphicsHighQualityEnabled = false;
    PDFPrerenderer* prerenderer = nullptr;

    // 视图状态
    int currentPage = 1;
    int totalPages = 0;
    double zoomFactor = 1.0;
    int rotation = 0;
    ViewMode viewMode = ViewMode::Continuous;

    // UI 组件
    QWidget* contentWidget = nullptr;
    QVBoxLayout* mainLayout = nullptr;
    QList<PageWidget*> pageWidgets;
    QWidget* emptyStateWidget = nullptr;

    // 搜索高亮
    QMap<int, QList<QRectF>> searchHighlights;

    // 渲染缓存
    QMap<int, QImage> renderCache;
    int maxCacheSize = 10;

    // 辅助方法
    void clearCache() { renderCache.clear(); }

    void addToCache(int pageNumber, const QImage& image) {
        if (renderCache.size() >= maxCacheSize) {
            // 移除最远的页面
            int farthest = -1;
            int maxDistance = -1;
            for (auto it = renderCache.begin(); it != renderCache.end(); ++it) {
                int distance = qAbs(it.key() - currentPage);
                if (distance > maxDistance) {
                    maxDistance = distance;
                    farthest = it.key();
                }
            }
            if (farthest != -1) {
                renderCache.remove(farthest);
            }
        }
        renderCache[pageNumber] = image;
    }

    QImage getFromCache(int pageNumber) const {
        return renderCache.value(pageNumber, QImage());
    }
};

// ============================================================================
// PDFViewer - 构造和析构
// ============================================================================

PDFViewer::PDFViewer(QWidget* parent)
    : ElaScrollArea(parent), m_impl(std::make_unique<Implementation>()) {
    SLOG_INFO("PDFViewer: Constructor started");

    // 设置滚动区域属性
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 创建内容容器
    m_impl->contentWidget = new QWidget(this);
    m_impl->mainLayout = new QVBoxLayout(m_impl->contentWidget);
    m_impl->mainLayout->setContentsMargins(10, 10, 10, 10);
    m_impl->mainLayout->setSpacing(10);
    m_impl->mainLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    // 创建空状态占位符
    createEmptyStateWidget();

    setWidget(m_impl->contentWidget);

    m_impl->prerenderer = new PDFPrerenderer(this);

    SLOG_INFO("PDFViewer: Constructor completed");
}

// Overloaded constructor used by tests to disable styling overhead when desired
PDFViewer::PDFViewer(QWidget* parent, bool enableStyling) : PDFViewer(parent) {
    if (!enableStyling) {
        // Apply minimal styling: remove frame and clear stylesheets to reduce
        // overhead
        setFrameShape(QFrame::NoFrame);
        setStyleSheet("");
    }
}

PDFViewer::~PDFViewer() {
    SLOG_INFO("PDFViewer: Destructor called");
    clearDocument();
}

// ============================================================================
// 文档操作
// ============================================================================

bool PDFViewer::setDocument(std::shared_ptr<Poppler::Document> document) {
    SLOG_INFO("PDFViewer: Setting document");

    if (!document) {
        SLOG_ERROR("PDFViewer: Null document provided");
        return false;
    }

    // 清除旧文档
    clearDocument();

    // 隐藏空状态占位符
    hideEmptyState();

    // 设置新文档
    m_impl->document = document;
    m_impl->totalPages = document->numPages();
    m_impl->currentPage = 1;

    SLOG_INFO_F("PDFViewer: Document loaded with {} pages", m_impl->totalPages);

    if (m_impl->prerenderer) {
        m_impl->prerenderer->setDocument(m_impl->document.get());
        m_impl->prerenderer->startPrerendering();
    }

    // 渲染当前页面
    renderCurrentPages();

    emit documentLoaded(m_impl->totalPages);
    emit pageChanged(m_impl->currentPage, m_impl->totalPages);

    return true;
}

void PDFViewer::clearDocument() {
    SLOG_INFO("PDFViewer: Clearing document");

    if (m_impl->prerenderer) {
        m_impl->prerenderer->stopPrerendering();
        m_impl->prerenderer->setDocument(nullptr);
    }

    // 清除布局
    clearLayout();

    // 清除状态
    m_impl->document.reset();
    m_impl->totalPages = 0;
    m_impl->currentPage = 1;
    m_impl->clearCache();
    m_impl->searchHighlights.clear();

    // 显示空状态占位符
    showEmptyState();

    emit documentClosed();
}

std::shared_ptr<Poppler::Document> PDFViewer::document() const {
    return m_impl->document;
}

bool PDFViewer::hasDocument() const { return m_impl->document != nullptr; }

// ============================================================================
// 页面导航
// ============================================================================

void PDFViewer::goToPage(int pageNumber) {
    if (!hasDocument() || pageNumber < 1 || pageNumber > m_impl->totalPages) {
        SLOG_WARNING_F("PDFViewer: Invalid page number: {}", pageNumber);
        return;
    }

    if (m_impl->currentPage == pageNumber) {
        return;
    }

    SLOG_INFO_F("PDFViewer: Going to page {}", pageNumber);
    m_impl->currentPage = pageNumber;

    // 根据视图模式处理
    if (m_impl->viewMode == ViewMode::SinglePage) {
        renderCurrentPages();
    } else {
        // 连续模式：滚动到对应页面
        if (pageNumber - 1 < m_impl->pageWidgets.size()) {
            PageWidget* widget = m_impl->pageWidgets[pageNumber - 1];
            ensureWidgetVisible(widget, 0, 0);
        }
    }

    emit pageChanged(m_impl->currentPage, m_impl->totalPages);
}

void PDFViewer::goToNextPage() { goToPage(m_impl->currentPage + 1); }

void PDFViewer::goToPreviousPage() { goToPage(m_impl->currentPage - 1); }

void PDFViewer::goToFirstPage() { goToPage(1); }

void PDFViewer::goToLastPage() { goToPage(m_impl->totalPages); }

int PDFViewer::currentPage() const { return m_impl->currentPage; }

int PDFViewer::pageCount() const { return m_impl->totalPages; }

// ============================================================================
// 缩放控制
// ============================================================================

void PDFViewer::setZoom(double zoomFactor) {
    if (zoomFactor < 0.1 || zoomFactor > 5.0) {
        SLOG_WARNING_F("PDFViewer: Invalid zoom factor: {}", zoomFactor);
        return;
    }

    if (qAbs(m_impl->zoomFactor - zoomFactor) < 0.01) {
        return;
    }

    SLOG_INFO_F("PDFViewer: Setting zoom to {}", zoomFactor);
    m_impl->zoomFactor = zoomFactor;
    m_impl->clearCache();
    renderCurrentPages();

    emit zoomChanged(m_impl->zoomFactor);
}

void PDFViewer::zoomIn() {
    double newZoom = m_impl->zoomFactor * 1.25;
    if (newZoom > 5.0) {
        newZoom = 5.0;
    }
    setZoom(newZoom);
}

void PDFViewer::zoomOut() {
    double newZoom = m_impl->zoomFactor / 1.25;
    if (newZoom < 0.1) {
        newZoom = 0.1;
    }
    setZoom(newZoom);
}

void PDFViewer::fitToWidth() {
    double zoom = calculateFitWidthZoom();
    setZoom(zoom);
}

void PDFViewer::fitToPage() {
    double zoom = calculateFitPageZoom();
    setZoom(zoom);
}

void PDFViewer::fitToHeight() {
    double zoom = calculateFitHeightZoom();
    setZoom(zoom);
}

double PDFViewer::zoom() const { return m_impl->zoomFactor; }

// ============================================================================
// 滚动控制
// ============================================================================

QPoint PDFViewer::scrollPosition() const {
    auto* h = horizontalScrollBar();
    auto* v = verticalScrollBar();
    return QPoint(h ? h->value() : 0, v ? v->value() : 0);
}

void PDFViewer::setScrollPosition(const QPoint& position) {
    if (auto* h = horizontalScrollBar()) {
        h->setValue(position.x());
    }
    if (auto* v = verticalScrollBar()) {
        v->setValue(position.y());
    }
}

void PDFViewer::scrollToTop() {
    if (auto* v = verticalScrollBar()) {
        v->setValue(v->minimum());
    }
}

void PDFViewer::scrollToBottom() {
    if (auto* v = verticalScrollBar()) {
        v->setValue(v->maximum());
    }
}

// ============================================================================
// 旋转控制
// ============================================================================

void PDFViewer::rotateLeft() {
    m_impl->rotation = (m_impl->rotation - 90 + 360) % 360;
    SLOG_INFO_F("PDFViewer: Rotated left to {} degrees", m_impl->rotation);

    // 更新所有页面的旋转
    for (PageWidget* widget : m_impl->pageWidgets) {
        widget->setRotation(m_impl->rotation);
    }

    updateLayout();
    emit rotationChanged(m_impl->rotation);
}

void PDFViewer::rotateRight() {
    m_impl->rotation = (m_impl->rotation + 90) % 360;
    SLOG_INFO_F("PDFViewer: Rotated right to {} degrees", m_impl->rotation);

    // NULL CHECK FIX: Validate widget pointer before accessing
    for (PageWidget* widget : m_impl->pageWidgets) {
        if (widget) {
            widget->setRotation(m_impl->rotation);
        }
    }

    updateLayout();
    emit rotationChanged(m_impl->rotation);
}

void PDFViewer::resetRotation() {
    if (m_impl->rotation == 0) {
        return;
    }

    m_impl->rotation = 0;
    SLOG_INFO("PDFViewer: Reset rotation");

    // NULL CHECK FIX: Validate widget pointer before accessing
    for (PageWidget* widget : m_impl->pageWidgets) {
        if (widget) {
            widget->setRotation(0);
        }
    }

    updateLayout();
    emit rotationChanged(0);
}

int PDFViewer::rotation() const { return m_impl->rotation; }

// ============================================================================
// 视图模式
// ============================================================================

void PDFViewer::setViewMode(ViewMode mode) {
    if (m_impl->viewMode == mode) {
        return;
    }

    SLOG_INFO_F("PDFViewer: Setting view mode to {}", static_cast<int>(mode));
    m_impl->viewMode = mode;

    renderCurrentPages();
    emit viewModeChanged(mode);
}

PDFViewer::ViewMode PDFViewer::viewMode() const { return m_impl->viewMode; }

// -----------------------------------------------------------------------------
// Backward-compatibility API for tests
// -----------------------------------------------------------------------------

bool PDFViewer::setDocument(Poppler::Document* document) {
    if (!document) {
        clearDocument();
        return false;
    }
    // Wrap raw pointer without taking ownership to avoid double free in tests
    auto shim =
        std::shared_ptr<Poppler::Document>(document, [](Poppler::Document*) {});
    return setDocument(std::move(shim));
}

int PDFViewer::getCurrentPage() const { return qMax(0, currentPage() - 1); }

int PDFViewer::getPageCount() const { return pageCount(); }

double PDFViewer::getCurrentZoom() const { return zoom(); }

void PDFViewer::nextPage() { goToNextPage(); }

void PDFViewer::previousPage() { goToPreviousPage(); }

void PDFViewer::zoomToWidth() { fitToWidth(); }

void PDFViewer::zoomToFit() { fitToPage(); }

void PDFViewer::setViewMode(PDFViewMode mode) {
    switch (mode) {
        case PDFViewMode::SinglePage:
            setViewMode(ViewMode::SinglePage);
            break;
        case PDFViewMode::ContinuousScroll:
            setViewMode(ViewMode::Continuous);
            break;
    }
}

PDFViewer::PDFViewMode PDFViewer::getViewMode() const {
    return (viewMode() == ViewMode::SinglePage) ? PDFViewMode::SinglePage
                                                : PDFViewMode::ContinuousScroll;
}

void PDFViewer::showSearch() {}
void PDFViewer::hideSearch() {}
void PDFViewer::toggleSearch() {}
void PDFViewer::findNext() {}
void PDFViewer::findPrevious() {}
void PDFViewer::clearSearch() {}

// ============================================================================
// 搜索高亮
// ============================================================================

void PDFViewer::highlightSearchResults(int pageNumber,
                                       const QList<QRectF>& results) {
    SLOG_INFO_F("PDFViewer: Highlighting {} results on page {}", results.size(),
                pageNumber);

    // VALIDATION FIX: Validate page number before accessing
    if (pageNumber < 1 || pageNumber > m_impl->totalPages) {
        SLOG_WARNING_F(
            "PDFViewer: Invalid page number {} for highlighting (total pages: "
            "{})",
            pageNumber, m_impl->totalPages);
        return;
    }

    m_impl->searchHighlights[pageNumber] = results;

    // BOUNDS CHECK FIX: Properly validate widget index
    // pageNumber is 1-based, pageWidgets is 0-based
    int widgetIndex = pageNumber - 1;
    if (widgetIndex >= 0 && widgetIndex < m_impl->pageWidgets.size()) {
        PageWidget* widget = m_impl->pageWidgets[widgetIndex];
        if (widget) {
            widget->setSearchHighlights(results);
        } else {
            SLOG_WARNING_F("PDFViewer: Null widget at index {} for page {}",
                           widgetIndex, pageNumber);
        }
    } else {
        SLOG_DEBUG_F(
            "PDFViewer: Page {} widget not currently visible (widget count: "
            "{})",
            pageNumber, m_impl->pageWidgets.size());
    }
}

void PDFViewer::clearSearchHighlights() {
    SLOG_INFO("PDFViewer: Clearing search highlights");

    m_impl->searchHighlights.clear();

    // NULL CHECK FIX: Validate widget pointer before accessing
    for (PageWidget* widget : m_impl->pageWidgets) {
        if (widget) {
            widget->clearSearchHighlights();
        }
    }
}

// ============================================================================
// 业务逻辑集成
// ============================================================================

void PDFViewer::setRenderModel(RenderModel* model) {
    m_impl->renderModel = model;
    SLOG_INFO("PDFViewer: RenderModel set");
}

void PDFViewer::setPageModel(PageModel* model) {
    m_impl->pageModel = model;
    SLOG_INFO("PDFViewer: PageModel set");
}

void PDFViewer::setQGraphicsRenderingEnabled(bool enabled) {
    if (m_impl->qgraphicsRenderingEnabled == enabled) {
        return;
    }

    m_impl->qgraphicsRenderingEnabled = enabled;
    SLOG_INFO_F("PDFViewer: QGraphics rendering %s",
                enabled ? "enabled" : "disabled");
}

bool PDFViewer::isQGraphicsRenderingEnabled() const {
    return m_impl->qgraphicsRenderingEnabled;
}

void PDFViewer::setQGraphicsHighQualityRendering(bool enabled) {
    if (m_impl->qgraphicsHighQualityEnabled == enabled) {
        return;
    }

    m_impl->qgraphicsHighQualityEnabled = enabled;
    SLOG_INFO_F("PDFViewer: QGraphics high-quality rendering %s",
                enabled ? "enabled" : "disabled");
}

// ============================================================================
// 事件处理
// ============================================================================

void PDFViewer::resizeEvent(QResizeEvent* event) {
    ElaScrollArea::resizeEvent(event);

    // 如果是适应宽度/页面模式，需要重新计算缩放
    // 这里可以添加自动调整逻辑
}

void PDFViewer::wheelEvent(QWheelEvent* event) {
    // Ctrl + 滚轮 = 缩放
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
    } else {
        ElaScrollArea::wheelEvent(event);
    }
}

void PDFViewer::mousePressEvent(QMouseEvent* event) {
    // 这里可以添加文本选择等功能
    ElaScrollArea::mousePressEvent(event);
}

void PDFViewer::mouseMoveEvent(QMouseEvent* event) {
    // 这里可以添加文本选择等功能
    ElaScrollArea::mouseMoveEvent(event);
}

void PDFViewer::mouseReleaseEvent(QMouseEvent* event) {
    // 这里可以添加文本选择等功能
    ElaScrollArea::mouseReleaseEvent(event);
}

// ============================================================================
// 渲染相关私有方法
// ============================================================================

void PDFViewer::renderCurrentPages() {
    if (!hasDocument()) {
        return;
    }

    SLOG_INFO_F("PDFViewer: Rendering current pages in mode {}",
                static_cast<int>(m_impl->viewMode));

    switch (m_impl->viewMode) {
        case ViewMode::SinglePage:
            applySinglePageMode();
            break;
        case ViewMode::Continuous:
            applyContinuousMode();
            break;
        case ViewMode::TwoPage:
            applyTwoPageMode();
            break;
        case ViewMode::BookMode:
            applyBookMode();
            break;
    }
}

void PDFViewer::renderPage(int pageNumber) {
    if (!hasDocument() || pageNumber < 1 || pageNumber > m_impl->totalPages) {
        return;
    }

    SLOG_DEBUG_F("PDFViewer: Rendering page {}", pageNumber);

    // 检查本地缓存
    QImage cachedImage = m_impl->getFromCache(pageNumber);
    if (!cachedImage.isNull()) {
        SLOG_DEBUG_F("PDFViewer: Using cached image for page {}", pageNumber);

        if (pageNumber - 1 < m_impl->pageWidgets.size()) {
            PageWidget* widget = m_impl->pageWidgets[pageNumber - 1];
            if (widget) {
                widget->setImage(cachedImage);
                widget->setRotation(m_impl->rotation);

                if (m_impl->searchHighlights.contains(pageNumber)) {
                    widget->setSearchHighlights(
                        m_impl->searchHighlights.value(pageNumber));
                }
            }
        }

        emit pageRendered(pageNumber);
        return;
    }

    if (m_impl->prerenderer) {
        int zeroBasedIndex = pageNumber - 1;
        double scaleFactor = m_impl->zoomFactor;
        int rotation = m_impl->rotation;
        QPixmap prerendered = m_impl->prerenderer->getCachedPage(
            zeroBasedIndex, scaleFactor, rotation);
        if (!prerendered.isNull()) {
            QImage imageFromPrerender = prerendered.toImage();
            m_impl->addToCache(pageNumber, imageFromPrerender);

            if (pageNumber - 1 < m_impl->pageWidgets.size()) {
                PageWidget* widget = m_impl->pageWidgets[pageNumber - 1];
                if (widget) {
                    widget->setImage(imageFromPrerender);
                    widget->setRotation(m_impl->rotation);

                    if (m_impl->searchHighlights.contains(pageNumber)) {
                        widget->setSearchHighlights(
                            m_impl->searchHighlights.value(pageNumber));
                    }
                }
            }

            emit pageRendered(pageNumber);
            return;
        }
    }

    QImage image;

    // 优先使用 RenderModel 进行渲染
    if (m_impl->renderModel && m_impl->renderModel->isDocumentValid()) {
        const double devicePixelRatio = devicePixelRatioF();
        const double effectiveDpiX = m_impl->renderModel->getEffectiveDpiX(
            m_impl->zoomFactor, devicePixelRatio);
        const double effectiveDpiY = m_impl->renderModel->getEffectiveDpiY(
            m_impl->zoomFactor, devicePixelRatio);

        SLOG_DEBUG_F(
            "PDFViewer: Rendering via RenderModel page {} at dpiX={} dpiY={}",
            pageNumber, effectiveDpiX, effectiveDpiY);

        // RenderModel 使用 0-based 页码
        try {
            image = m_impl->renderModel->renderPage(
                pageNumber - 1, effectiveDpiX, effectiveDpiY);
        } catch (const std::exception& e) {
            SLOG_ERROR_F(
                "PDFViewer: Exception while rendering page {} via RenderModel: "
                "{}",
                pageNumber, e.what());
        }
    }

    // 如果 RenderModel 不可用或渲染失败，退回到直接 Poppler 渲染
    if (image.isNull()) {
        // SAFETY CHECK: Verify document is still valid before accessing
        if (!m_impl->document) {
            SLOG_WARNING("PDFViewer: Document became null during renderPage");
            return;
        }

        std::unique_ptr<Poppler::Page> page(
            m_impl->document->page(pageNumber - 1));
        if (!page) {
            SLOG_ERROR_F("PDFViewer: Failed to get page {}", pageNumber);
            emit renderError(tr("Failed to render page %1").arg(pageNumber));
            return;
        }

        double dpi = 72.0 * m_impl->zoomFactor;

        image = page->renderToImage(dpi, dpi);

        if (image.isNull()) {
            SLOG_ERROR_F("PDFViewer: Failed to render page {} to image",
                         pageNumber);
            emit renderError(tr("Failed to render page %1").arg(pageNumber));
            return;
        }
    }

    // 更新本地缓存
    m_impl->addToCache(pageNumber, image);

    // 更新对应的 PageWidget
    if (pageNumber - 1 < m_impl->pageWidgets.size()) {
        PageWidget* widget = m_impl->pageWidgets[pageNumber - 1];
        if (widget) {
            widget->setImage(image);
            widget->setRotation(m_impl->rotation);

            // 应用搜索高亮
            if (m_impl->searchHighlights.contains(pageNumber)) {
                widget->setSearchHighlights(
                    m_impl->searchHighlights.value(pageNumber));
            }
        }
    }

    if (m_impl->prerenderer) {
        int rotation = m_impl->rotation;
        double scaleFactor = m_impl->zoomFactor;
        auto requestNeighbor = [this, pageNumber, rotation,
                                scaleFactor](int logicalPage) {
            if (logicalPage < 1 || logicalPage > m_impl->totalPages) {
                return;
            }
            int zeroBasedIndex = logicalPage - 1;
            if (!m_impl->prerenderer->hasPrerenderedPage(
                    zeroBasedIndex, scaleFactor, rotation)) {
                int priority = qAbs(logicalPage - pageNumber);
                m_impl->prerenderer->requestPrerender(
                    zeroBasedIndex, scaleFactor, rotation, priority);
            }
        };

        requestNeighbor(pageNumber - 1);
        requestNeighbor(pageNumber + 1);
    }

    emit pageRendered(pageNumber);
}

void PDFViewer::updateLayout() {
    if (m_impl->contentWidget) {
        m_impl->contentWidget->updateGeometry();
        m_impl->contentWidget->update();
    }
}

void PDFViewer::createEmptyStateWidget() {
    // PERFORMANCE FIX: Defer empty state widget creation to avoid blocking
    // constructor The emoji character was causing 5-second font rendering
    // delays on Windows Use QTimer::singleShot to create the widget
    // asynchronously
    QTimer::singleShot(0, this, [this]() {
        // 创建空状态占位符
        m_impl->emptyStateWidget = new QWidget(m_impl->contentWidget);
        auto* emptyLayout = new QVBoxLayout(m_impl->emptyStateWidget);
        emptyLayout->setAlignment(Qt::AlignCenter);
        emptyLayout->setSpacing(20);

        // 图标/图片（使用简单的SVG图标或文本，避免emoji导致的字体加载延迟）
        auto* iconLabel = new ElaText(m_impl->emptyStateWidget);
        iconLabel->setText("PDF");
        QFont iconFont = iconLabel->font();
        iconFont.setPointSize(48);
        iconFont.setBold(true);
        iconFont.setFamily("Arial");
        iconLabel->setFont(iconFont);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet(
            "ElaText {"
            "  color: #CCCCCC;"
            "  background-color: #F5F5F5;"
            "  border: 3px dashed #DDDDDD;"
            "  border-radius: 10px;"
            "  padding: 30px 50px;"
            "}");
        emptyLayout->addWidget(iconLabel);

        // 主标题
        auto* titleLabel =
            new ElaText(tr("No Document Loaded"), m_impl->emptyStateWidget);
        QFont titleFont = titleLabel->font();
        titleFont.setPointSize(18);
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setStyleSheet("color: #555555;");
        emptyLayout->addWidget(titleLabel);

        // 副标题
        auto* subtitleLabel = new ElaText(tr("Open a PDF file to get started"),
                                          m_impl->emptyStateWidget);
        QFont subtitleFont = subtitleLabel->font();
        subtitleFont.setPointSize(12);
        subtitleLabel->setFont(subtitleFont);
        subtitleLabel->setAlignment(Qt::AlignCenter);
        subtitleLabel->setStyleSheet("color: #888888;");
        emptyLayout->addWidget(subtitleLabel);

        // 添加到主布局
        m_impl->mainLayout->addWidget(m_impl->emptyStateWidget, 1,
                                      Qt::AlignCenter);
    });
}

void PDFViewer::showEmptyState() {
    if (m_impl->emptyStateWidget) {
        m_impl->emptyStateWidget->show();
    }
}

void PDFViewer::hideEmptyState() {
    if (m_impl->emptyStateWidget) {
        m_impl->emptyStateWidget->hide();
    }
}

void PDFViewer::clearLayout() {
    SLOG_INFO("PDFViewer: Clearing layout");

    // NULL CHECK FIX: Validate widget and layout pointers before accessing
    if (!m_impl->mainLayout) {
        SLOG_WARNING("PDFViewer: mainLayout is null in clearLayout");
        m_impl->pageWidgets.clear();
        return;
    }

    // 删除所有页面组件
    for (PageWidget* widget : m_impl->pageWidgets) {
        if (widget) {
            m_impl->mainLayout->removeWidget(widget);
            delete widget;  // direct delete to avoid deferred deletion at
                            // teardown
        }
    }
    m_impl->pageWidgets.clear();
}

// ============================================================================
// 缩放辅助方法
// ============================================================================

double PDFViewer::calculateFitWidthZoom() {
    if (!hasDocument()) {
        return 1.0;
    }

    // SAFETY CHECK: Verify document is still valid
    if (!m_impl->document) {
        SLOG_WARNING(
            "PDFViewer: Document became null during calculateFitWidthZoom");
        return 1.0;
    }

    // 获取第一页的尺寸
    std::unique_ptr<Poppler::Page> page(m_impl->document->page(0));
    if (!page) {
        return 1.0;
    }

    QSizeF pageSize = page->pageSizeF();
    double pageWidth = pageSize.width();

    // 考虑旋转
    if (m_impl->rotation == 90 || m_impl->rotation == 270) {
        pageWidth = pageSize.height();
    }

    // 计算可用宽度（减去边距和滚动条）
    int availableWidth = viewport()->width() - 40;  // 20px 边距 * 2

    // 计算缩放比例
    double zoom = (availableWidth / pageWidth) * (72.0 / 72.0);

    // 限制范围
    if (zoom < 0.1) {
        zoom = 0.1;
    }
    if (zoom > 5.0) {
        zoom = 5.0;
    }

    return zoom;
}

double PDFViewer::calculateFitPageZoom() {
    if (!hasDocument()) {
        return 1.0;
    }

    // SAFETY CHECK: Verify document is still valid
    if (!m_impl->document) {
        SLOG_WARNING(
            "PDFViewer: Document became null during calculateFitPageZoom");
        return 1.0;
    }

    // 获取第一页的尺寸
    std::unique_ptr<Poppler::Page> page(m_impl->document->page(0));
    if (!page) {
        return 1.0;
    }

    QSizeF pageSize = page->pageSizeF();
    double pageWidth = pageSize.width();
    double pageHeight = pageSize.height();

    // 考虑旋转
    if (m_impl->rotation == 90 || m_impl->rotation == 270) {
        std::swap(pageWidth, pageHeight);
    }

    // 计算可用空间
    int availableWidth = viewport()->width() - 40;
    int availableHeight = viewport()->height() - 40;

    // 计算两个方向的缩放比例，取较小值
    double zoomWidth = (availableWidth / pageWidth) * (72.0 / 72.0);
    double zoomHeight = (availableHeight / pageHeight) * (72.0 / 72.0);
    double zoom = qMin(zoomWidth, zoomHeight);

    // 限制范围
    if (zoom < 0.1) {
        zoom = 0.1;
    }
    if (zoom > 5.0) {
        zoom = 5.0;
    }

    return zoom;
}

double PDFViewer::calculateFitHeightZoom() {
    if (!hasDocument()) {
        return 1.0;
    }

    // SAFETY CHECK: Verify document is still valid
    if (!m_impl->document) {
        SLOG_WARNING(
            "PDFViewer: Document became null during calculateFitHeightZoom");
        return 1.0;
    }

    // 获取第一页的尺寸
    std::unique_ptr<Poppler::Page> page(m_impl->document->page(0));
    if (!page) {
        return 1.0;
    }

    QSizeF pageSize = page->pageSizeF();
    double pageHeight = pageSize.height();

    // 考虑旋转
    if (m_impl->rotation == 90 || m_impl->rotation == 270) {
        pageHeight = pageSize.width();
    }

    // 计算可用高度
    int availableHeight = viewport()->height() - 40;

    // 计算缩放比例
    double zoom = (availableHeight / pageHeight) * (72.0 / 72.0);

    // 限制范围
    if (zoom < 0.1) {
        zoom = 0.1;
    }
    if (zoom > 5.0) {
        zoom = 5.0;
    }

    return zoom;
}

// ============================================================================
// 视图模式实现
// ============================================================================

void PDFViewer::applySinglePageMode() {
    SLOG_INFO("PDFViewer: Applying single page mode");

    clearLayout();

    // 只显示当前页
    PageWidget* pageWidget =
        new PageWidget(m_impl->currentPage, m_impl->contentWidget);
    m_impl->pageWidgets.append(pageWidget);
    m_impl->mainLayout->addWidget(pageWidget, 0, Qt::AlignCenter);

    // 渲染当前页
    renderPage(m_impl->currentPage);

    updateLayout();
}

void PDFViewer::applyContinuousMode() {
    SLOG_INFO("PDFViewer: Applying continuous mode");

    clearLayout();

    // 显示所有页面
    for (int i = 1; i <= m_impl->totalPages; ++i) {
        PageWidget* pageWidget = new PageWidget(i, m_impl->contentWidget);
        m_impl->pageWidgets.append(pageWidget);
        m_impl->mainLayout->addWidget(pageWidget, 0, Qt::AlignCenter);
    }

    // 渲染可见页面（当前页及前后几页）
    int startPage = qMax(1, m_impl->currentPage - 2);
    int endPage = qMin(m_impl->totalPages, m_impl->currentPage + 2);

    for (int i = startPage; i <= endPage; ++i) {
        renderPage(i);
    }

    updateLayout();

    // 滚动到当前页
    if (m_impl->currentPage - 1 < m_impl->pageWidgets.size()) {
        QTimer::singleShot(100, this, [this]() {
            PageWidget* widget = m_impl->pageWidgets[m_impl->currentPage - 1];
            ensureWidgetVisible(widget, 0, 0);
        });
    }
}

void PDFViewer::applyTwoPageMode() {
    SLOG_INFO("PDFViewer: Applying two page mode");

    clearLayout();

    // 两页一组显示
    for (int i = 1; i <= m_impl->totalPages; i += 2) {
        QWidget* rowWidget = new QWidget(m_impl->contentWidget);
        QHBoxLayout* rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(10);

        // 左页
        PageWidget* leftPage = new PageWidget(i, rowWidget);
        m_impl->pageWidgets.append(leftPage);
        rowLayout->addWidget(leftPage);

        // 右页（如果存在）
        if (i + 1 <= m_impl->totalPages) {
            PageWidget* rightPage = new PageWidget(i + 1, rowWidget);
            m_impl->pageWidgets.append(rightPage);
            rowLayout->addWidget(rightPage);
        } else {
            // 添加占位符保持对称
            rowLayout->addStretch();
        }

        m_impl->mainLayout->addWidget(rowWidget, 0, Qt::AlignCenter);
    }

    // 渲染当前页及相邻页
    int startPage = ((m_impl->currentPage - 1) / 2) * 2 + 1;
    int endPage = qMin(m_impl->totalPages, startPage + 3);

    for (int i = startPage; i <= endPage; ++i) {
        renderPage(i);
    }

    updateLayout();
}

void PDFViewer::applyBookMode() {
    SLOG_INFO("PDFViewer: Applying book mode");

    clearLayout();

    // 第一页单独显示
    if (m_impl->totalPages >= 1) {
        PageWidget* firstPage = new PageWidget(1, m_impl->contentWidget);
        m_impl->pageWidgets.append(firstPage);
        m_impl->mainLayout->addWidget(firstPage, 0, Qt::AlignCenter);
    }

    // 后续页面两页一组
    for (int i = 2; i <= m_impl->totalPages; i += 2) {
        QWidget* rowWidget = new QWidget(m_impl->contentWidget);
        QHBoxLayout* rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(10);

        // 左页
        PageWidget* leftPage = new PageWidget(i, rowWidget);
        m_impl->pageWidgets.append(leftPage);
        rowLayout->addWidget(leftPage);

        // 右页（如果存在）
        if (i + 1 <= m_impl->totalPages) {
            PageWidget* rightPage = new PageWidget(i + 1, rowWidget);
            m_impl->pageWidgets.append(rightPage);
            rowLayout->addWidget(rightPage);
        } else {
            rowLayout->addStretch();
        }

        m_impl->mainLayout->addWidget(rowWidget, 0, Qt::AlignCenter);
    }

    // 渲染可见页面
    int startPage = qMax(1, m_impl->currentPage - 2);
    int endPage = qMin(m_impl->totalPages, m_impl->currentPage + 2);

    for (int i = startPage; i <= endPage; ++i) {
        renderPage(i);
    }

    updateLayout();
}

// 包含 MOC 生成的代码
#include "PDFViewer.moc"
