#include "PDFViewer.h"
#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QDebug>
#include <QGesture>
#include <QGestureEvent>
#include <QGraphicsOpacityEffect>
#include <QGroupBox>
#include <QLabel>
#include <QLayoutItem>
#include <QMouseEvent>
#include <QPainter>
#include <QPanGesture>
#include <QPen>
#include <QPinchGesture>
#include <QPropertyAnimation>
#include <QRect>
#include <QScrollBar>
#include <QSettings>
#include <QSize>
#include <QSizeF>
#include <QSplitter>
#include <QStackedWidget>
#include <QSwipeGesture>
#include <QTouchEvent>
#include <QWheelEvent>
#include <QtCore>
#include <QtGlobal>
#include <QtWidgets>
#include <cmath>
#include <memory>
#include <stdexcept>
#include "../../managers/StyleManager.h"

// PDFPageWidget Implementation
PDFPageWidget::PDFPageWidget(QWidget* parent)
    : QLabel(parent),
      currentPage(nullptr),
      currentScaleFactor(1.0),
      currentRotation(0),
      isDragging(false),
      m_currentSearchResultIndex(-1),
      m_normalHighlightColor(QColor(255, 255, 0, 100)),
      m_currentHighlightColor(QColor(255, 165, 0, 150)) {
    setAlignment(Qt::AlignCenter);
    setMinimumSize(200, 200);
    setObjectName("pdfPage");

    // Enable gesture support
    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::SwipeGesture);
    grabGesture(Qt::PanGesture);

    // Enable touch events
    setAttribute(Qt::WA_AcceptTouchEvents, true);

    // 设置现代化的页面样式 (仅在非测试环境中)
    try {
        setStyleSheet(QString(R"(
            QLabel#pdfPage {
                background-color: white;
                border: 1px solid %1;
                border-radius: 8px;
                margin: 12px;
                padding: 8px;
            }
        )")
                          .arg(STYLE.borderColor().name()));
    } catch (...) {
        // 在测试环境中忽略样式错误
        setStyleSheet(
            "QLabel#pdfPage { background-color: white; border: 1px solid gray; "
            "}");
    }

    setText("No PDF loaded");

    // 添加阴影效果
    QGraphicsDropShadowEffect* shadowEffect =
        new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(15);
    shadowEffect->setColor(QColor(0, 0, 0, 50));
    shadowEffect->setOffset(0, 4);
    setGraphicsEffect(shadowEffect);
}

void PDFPageWidget::setPage(Poppler::Page* page, double scaleFactor,
                            int rotation) {
    currentPage = page;
    currentScaleFactor = scaleFactor;
    currentRotation = rotation;
    renderPage();
}

void PDFPageWidget::setScaleFactor(double factor) {
    if (factor != currentScaleFactor) {
        currentScaleFactor = factor;
        renderPage();
        emit scaleChanged(factor);
    }
}

void PDFPageWidget::setRotation(int degrees) {
    // 确保旋转角度是90度的倍数
    degrees = ((degrees % 360) + 360) % 360;
    if (degrees != currentRotation) {
        currentRotation = degrees;
        renderPage();
    }
}

void PDFPageWidget::renderPage() {
    if (!currentPage) {
        setText("No page to render");
        return;
    }

    try {
        // Enhanced rendering with optimized DPI calculation
        double devicePixelRatio = devicePixelRatioF();
        double baseDpi = 72.0 * currentScaleFactor;
        double optimizedDpi = baseDpi * devicePixelRatio;

        // Limit DPI to prevent excessive memory usage
        optimizedDpi = qMin(optimizedDpi, 300.0);

        QSizeF pageSize = currentPage->pageSizeF();

        // Note: Document configuration would be done at document level
        // High-quality rendering is achieved through optimized DPI and render
        // hints

        // 渲染页面为图像，包含旋转和优化设置
        QImage image = currentPage->renderToImage(
            optimizedDpi, optimizedDpi, -1, -1, -1, -1,
            static_cast<Poppler::Page::Rotation>(currentRotation / 90));
        if (image.isNull()) {
            setText("Failed to render page");
            return;
        }

        // Apply device pixel ratio for high-DPI displays
        renderedPixmap = QPixmap::fromImage(image);
        renderedPixmap.setDevicePixelRatio(devicePixelRatio);

        setPixmap(renderedPixmap);
        resize(renderedPixmap.size() / devicePixelRatio);

    } catch (const std::exception& e) {
        setText(QString("渲染错误: %1").arg(e.what()));
        qDebug() << "Page render error:" << e.what();
    } catch (...) {
        setText("未知渲染错误");
        qDebug() << "Unknown page render error";
    }
}

void PDFPageWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);

    // Enable high-quality rendering hints
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::SmoothPixmapTransform |
                           QPainter::TextAntialiasing);

    // Call parent implementation with enhanced rendering
    QLabel::paintEvent(event);

    // Draw search highlights
    if (!m_searchResults.isEmpty()) {
        drawSearchHighlights(painter);
    }

    // Add subtle shadow effect for better visual appearance
    if (!renderedPixmap.isNull()) {
        QRect pixmapRect = rect();
        painter.setPen(QPen(QColor(0, 0, 0, 30), 1));
        painter.drawRect(pixmapRect.adjusted(0, 0, -1, -1));
    }
}

bool PDFPageWidget::event(QEvent* event) {
    if (event->type() == QEvent::Gesture) {
        return gestureEvent(static_cast<QGestureEvent*>(event));
    } else if (event->type() == QEvent::TouchBegin ||
               event->type() == QEvent::TouchUpdate ||
               event->type() == QEvent::TouchEnd) {
        touchEvent(static_cast<QTouchEvent*>(event));
        return true;
    }
    return QLabel::event(event);
}

bool PDFPageWidget::gestureEvent(QGestureEvent* event) {
    if (QGesture* swipe = event->gesture(Qt::SwipeGesture)) {
        swipeTriggered(static_cast<QSwipeGesture*>(swipe));
    }
    if (QGesture* pan = event->gesture(Qt::PanGesture)) {
        panTriggered(static_cast<QPanGesture*>(pan));
    }
    if (QGesture* pinch = event->gesture(Qt::PinchGesture)) {
        pinchTriggered(static_cast<QPinchGesture*>(pinch));
    }
    return true;
}

void PDFPageWidget::pinchTriggered(QPinchGesture* gesture) {
    QPinchGesture::ChangeFlags changeFlags = gesture->changeFlags();

    if (changeFlags & QPinchGesture::ScaleFactorChanged) {
        qreal scaleFactor = gesture->totalScaleFactor();

        // Apply pinch zoom
        double newScale = currentScaleFactor * scaleFactor;
        newScale = qBound(0.1, newScale, 5.0);  // Limit zoom range

        if (qAbs(newScale - currentScaleFactor) > 0.01) {
            setScaleFactor(newScale);
            emit scaleChanged(newScale);
        }
    }

    if (gesture->state() == Qt::GestureFinished) {
        // Gesture completed
        update();
    }
}

void PDFPageWidget::swipeTriggered(QSwipeGesture* gesture) {
    if (gesture->state() == Qt::GestureFinished) {
        if (gesture->horizontalDirection() == QSwipeGesture::Left) {
            // Swipe left - next page
            emit pageClicked(
                QPoint(-1, 0));  // Use special coordinates to indicate swipe
        } else if (gesture->horizontalDirection() == QSwipeGesture::Right) {
            // Swipe right - previous page
            emit pageClicked(
                QPoint(-2, 0));  // Use special coordinates to indicate swipe
        }
    }
}

void PDFPageWidget::panTriggered(QPanGesture* gesture) {
    QPointF delta = gesture->delta();

    if (gesture->state() == Qt::GestureStarted) {
        setCursor(Qt::ClosedHandCursor);
    } else if (gesture->state() == Qt::GestureUpdated) {
        // Handle panning - this would typically scroll the parent scroll area
        // For now, we'll emit a signal that the parent can handle
        emit pageClicked(
            QPoint(static_cast<int>(delta.x()), static_cast<int>(delta.y())));
    } else if (gesture->state() == Qt::GestureFinished ||
               gesture->state() == Qt::GestureCanceled) {
        setCursor(Qt::ArrowCursor);
    }
}

void PDFPageWidget::touchEvent(QTouchEvent* event) {
    // Handle multi-touch events
    const QList<QEventPoint>& touchPoints = event->points();

    switch (event->type()) {
        case QEvent::TouchBegin:
            if (touchPoints.count() == 1) {
                // Single touch - potential tap
                lastPanPoint = touchPoints.first().position().toPoint();
            }
            break;

        case QEvent::TouchUpdate:
            if (touchPoints.count() == 1) {
                // Single touch drag
                QPoint currentPoint = touchPoints.first().position().toPoint();
                QPoint delta = currentPoint - lastPanPoint;
                lastPanPoint = currentPoint;

                // Emit pan signal
                emit pageClicked(delta);
            }
            break;

        case QEvent::TouchEnd:
            if (touchPoints.count() == 1) {
                // Single touch end - potential tap
                QPoint tapPoint = touchPoints.first().position().toPoint();
                emit pageClicked(tapPoint);
            }
            break;

        default:
            break;
    }

    event->accept();
}

// Drag and Drop Implementation
void PDFPageWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString fileName = urls.first().toLocalFile();
            if (fileName.toLower().endsWith(".pdf")) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void PDFPageWidget::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void PDFPageWidget::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString fileName = urls.first().toLocalFile();
            if (fileName.toLower().endsWith(".pdf")) {
                // Emit signal to parent to handle file opening
                emit pageClicked(
                    QPoint(-100, -100));  // Special coordinates for file drop
                // Store the file path for retrieval
                setProperty("droppedFile", fileName);
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void PDFPageWidget::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        // Ctrl + 滚轮进行缩放
        int delta = event->angleDelta().y();
        if (delta != 0) {
            // 使用更平滑的缩放步长
            double scaleDelta = delta > 0 ? 1.15 : (1.0 / 1.15);
            double newScale = currentScaleFactor * scaleDelta;

            // 限制缩放范围
            newScale = qBound(0.1, newScale, 5.0);

            // 应用缩放
            setScaleFactor(newScale);
        }
        event->accept();
    } else {
        QLabel::wheelEvent(event);
    }
}

void PDFPageWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    QLabel::mousePressEvent(event);
}

void PDFPageWidget::mouseMoveEvent(QMouseEvent* event) {
    if (isDragging && (event->buttons() & Qt::LeftButton)) {
        // 实现拖拽平移功能
        QPoint delta = event->pos() - lastPanPoint;
        lastPanPoint = event->pos();

        // 这里可以实现滚动区域的平移
        // 由于我们在ScrollArea中，这个功能由ScrollArea处理
    }
    QLabel::mouseMoveEvent(event);
}

void PDFPageWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
        setCursor(Qt::ArrowCursor);
    }
    QLabel::mouseReleaseEvent(event);
}

// Search highlighting implementation
void PDFPageWidget::setSearchResults(const QList<SearchResult>& results) {
    m_searchResults = results;
    m_currentSearchResultIndex = -1;

    // Transform coordinates for all results
    updateSearchResultCoordinates();

    // Trigger repaint to show highlights
    update();
}

void PDFPageWidget::clearSearchHighlights() {
    m_searchResults.clear();
    m_currentSearchResultIndex = -1;
    update();
}

void PDFPageWidget::setCurrentSearchResult(int index) {
    if (index >= 0 && index < m_searchResults.size()) {
        // Clear previous current result
        if (m_currentSearchResultIndex >= 0 &&
            m_currentSearchResultIndex < m_searchResults.size()) {
            m_searchResults[m_currentSearchResultIndex].isCurrentResult = false;
        }

        // Set new current result
        m_currentSearchResultIndex = index;
        m_searchResults[index].isCurrentResult = true;

        update();
    }
}

void PDFPageWidget::updateHighlightColors(const QColor& normalColor,
                                          const QColor& currentColor) {
    m_normalHighlightColor = normalColor;
    m_currentHighlightColor = currentColor;
    update();
}

void PDFPageWidget::updateSearchResultCoordinates() {
    if (!currentPage || m_searchResults.isEmpty()) {
        return;
    }

    QSizeF pageSize = currentPage->pageSizeF();
    QSize widgetSize = size();

    for (SearchResult& result : m_searchResults) {
        result.transformToWidgetCoordinates(currentScaleFactor, currentRotation,
                                            pageSize, widgetSize);
    }
}

void PDFPageWidget::drawSearchHighlights(QPainter& painter) {
    if (m_searchResults.isEmpty()) {
        return;
    }

    painter.save();

    for (const SearchResult& result : m_searchResults) {
        if (!result.isValidForHighlight() || result.widgetRect.isEmpty()) {
            continue;
        }

        // Choose color based on whether this is the current result
        QColor highlightColor = result.isCurrentResult ? m_currentHighlightColor
                                                       : m_normalHighlightColor;

        // Draw highlight rectangle
        painter.fillRect(result.widgetRect, highlightColor);

        // Draw border for current result
        if (result.isCurrentResult) {
            painter.setPen(QPen(highlightColor.darker(150), 2));
            painter.drawRect(result.widgetRect);
        }
    }

    painter.restore();
}

// PDFViewer Implementation
PDFViewer::PDFViewer(QWidget* parent, bool enableStyling)
    : QWidget(parent),
      document(nullptr),
      currentPageNumber(0),
      currentZoomFactor(DEFAULT_ZOOM),
      currentViewMode(PDFViewMode::SinglePage),
      currentZoomType(ZoomType::FixedValue),
      currentRotation(0),
      pendingZoomFactor(DEFAULT_ZOOM),
      isZoomPending(false),
      m_currentSearchResultIndex(-1),
      m_enableStyling(enableStyling) {
    // 初始化动画管理器
    animationManager = new PDFAnimationManager(this);

    // 初始化预渲染器
    prerenderer = new PDFPrerenderer(this);
    prerenderer->setStrategy(PDFPrerenderer::PrerenderStrategy::Balanced);
    prerenderer->setMaxWorkerThreads(2);  // Use 2 background threads

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    // 初始化QGraphics PDF查看器
    qgraphicsViewer = nullptr;
    useQGraphicsViewer = false;  // 默认使用传统渲染
#endif

    // 启用拖放功能
    setAcceptDrops(true);

    // 初始化防抖定时器
    zoomTimer = new QTimer(this);
    zoomTimer->setSingleShot(true);
    zoomTimer->setInterval(150);  // 150ms防抖延迟

    // 初始化虚拟化渲染
    visiblePageStart = 0;
    visiblePageEnd = 0;
    renderBuffer = 2;  // 预渲染前后2页

    scrollTimer = new QTimer(this);
    scrollTimer->setSingleShot(true);
    scrollTimer->setInterval(100);  // 100ms滚动防抖

    // 初始化页面缓存
    maxCacheSize = 20;  // 最多缓存20页

    // 初始化动画效果
    opacityEffect = new QGraphicsOpacityEffect(this);
    fadeAnimation = new QPropertyAnimation(opacityEffect, "opacity", this);
    fadeAnimation->setDuration(300);  // 300ms动画时间

    setupUI();
    setupConnections();
    setupShortcuts();
    loadZoomSettings();
    updateNavigationButtons();
    updateZoomControls();
}

void PDFViewer::setupUI() {
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 应用样式 (仅在非测试环境中)
    if (m_enableStyling) {
        setStyleSheet(STYLE.getApplicationStyleSheet());
    }

    // 创建工具栏
    QWidget* toolbar = new QWidget(this);
    toolbar->setObjectName("toolbar");
    if (m_enableStyling) {
        toolbar->setStyleSheet(STYLE.getToolbarStyleSheet());
        toolbarLayout = new QHBoxLayout(toolbar);
        toolbarLayout->setContentsMargins(STYLE.margin(), STYLE.spacing(),
                                          STYLE.margin(), STYLE.spacing());
        toolbarLayout->setSpacing(STYLE.spacing());
    } else {
        toolbarLayout = new QHBoxLayout(toolbar);
        toolbarLayout->setContentsMargins(8, 8, 8, 8);
        toolbarLayout->setSpacing(8);
    }

    // 页面导航控件
    QGroupBox* navGroup = new QGroupBox("页面导航", toolbar);
    QHBoxLayout* navLayout = new QHBoxLayout(navGroup);

    // 使用现代化图标
    firstPageBtn = new QPushButton("⏮", navGroup);
    prevPageBtn = new QPushButton("◀", navGroup);
    pageNumberSpinBox = new QSpinBox(navGroup);
    pageCountLabel = new QLabel("/ 0", navGroup);
    nextPageBtn = new QPushButton("▶", navGroup);
    lastPageBtn = new QPushButton("⏭", navGroup);

    // 设置按钮样式和尺寸
    QString buttonStyle = STYLE.getButtonStyleSheet();
    firstPageBtn->setStyleSheet(buttonStyle);
    prevPageBtn->setStyleSheet(buttonStyle);
    nextPageBtn->setStyleSheet(buttonStyle);
    lastPageBtn->setStyleSheet(buttonStyle);

    firstPageBtn->setFixedSize(STYLE.buttonHeight(), STYLE.buttonHeight());
    prevPageBtn->setFixedSize(STYLE.buttonHeight(), STYLE.buttonHeight());
    nextPageBtn->setFixedSize(STYLE.buttonHeight(), STYLE.buttonHeight());
    lastPageBtn->setFixedSize(STYLE.buttonHeight(), STYLE.buttonHeight());
    pageNumberSpinBox->setMaximumWidth(60);

    // 设置工具提示
    firstPageBtn->setToolTip("第一页 (Ctrl+Home)");
    prevPageBtn->setToolTip("上一页 (Page Up)");
    nextPageBtn->setToolTip("下一页 (Page Down)");
    lastPageBtn->setToolTip("最后一页 (Ctrl+End)");

    navLayout->addWidget(firstPageBtn);
    navLayout->addWidget(prevPageBtn);
    navLayout->addWidget(pageNumberSpinBox);
    navLayout->addWidget(pageCountLabel);
    navLayout->addWidget(nextPageBtn);
    navLayout->addWidget(lastPageBtn);

    // 缩放控件
    QGroupBox* zoomGroup = new QGroupBox("缩放", toolbar);
    QHBoxLayout* zoomLayout = new QHBoxLayout(zoomGroup);

    // 使用现代化图标
    zoomOutBtn = new QPushButton("🔍-", zoomGroup);
    zoomInBtn = new QPushButton("🔍+", zoomGroup);
    zoomSlider = new QSlider(Qt::Horizontal, zoomGroup);
    zoomPercentageSpinBox = new QSpinBox(zoomGroup);
    fitWidthBtn = new QPushButton("📏", zoomGroup);
    fitHeightBtn = new QPushButton("📐", zoomGroup);
    fitPageBtn = new QPushButton("🗎", zoomGroup);

    // 设置按钮样式
    zoomOutBtn->setStyleSheet(buttonStyle);
    zoomInBtn->setStyleSheet(buttonStyle);
    fitWidthBtn->setStyleSheet(buttonStyle);
    fitHeightBtn->setStyleSheet(buttonStyle);
    fitPageBtn->setStyleSheet(buttonStyle);

    zoomOutBtn->setFixedSize(STYLE.buttonHeight(), STYLE.buttonHeight());
    zoomInBtn->setFixedSize(STYLE.buttonHeight(), STYLE.buttonHeight());
    fitWidthBtn->setFixedSize(STYLE.buttonHeight(), STYLE.buttonHeight());
    fitHeightBtn->setFixedSize(STYLE.buttonHeight(), STYLE.buttonHeight());
    fitPageBtn->setFixedSize(STYLE.buttonHeight(), STYLE.buttonHeight());

    zoomSlider->setRange(10, 500);  // 10% to 500%
    zoomSlider->setValue(100);
    zoomSlider->setMinimumWidth(120);

    // 配置百分比输入框
    zoomPercentageSpinBox->setRange(10, 500);
    zoomPercentageSpinBox->setValue(100);
    zoomPercentageSpinBox->setSuffix("%");
    zoomPercentageSpinBox->setMinimumWidth(80);
    zoomPercentageSpinBox->setMaximumWidth(80);

    // 设置工具提示
    zoomOutBtn->setToolTip("缩小 (Ctrl+-)");
    zoomInBtn->setToolTip("放大 (Ctrl++)");
    fitWidthBtn->setToolTip("适合宽度 (Ctrl+1)");
    fitHeightBtn->setToolTip("适合高度 (Ctrl+2)");
    fitPageBtn->setToolTip("适合页面 (Ctrl+0)");

    zoomLayout->addWidget(zoomOutBtn);
    zoomLayout->addWidget(zoomInBtn);
    zoomLayout->addWidget(zoomSlider);
    zoomLayout->addWidget(zoomPercentageSpinBox);
    zoomLayout->addWidget(fitWidthBtn);
    zoomLayout->addWidget(fitHeightBtn);
    zoomLayout->addWidget(fitPageBtn);

    // 旋转控件
    QGroupBox* rotateGroup = new QGroupBox("旋转", toolbar);
    QHBoxLayout* rotateLayout = new QHBoxLayout(rotateGroup);

    rotateLeftBtn = new QPushButton("↺", rotateGroup);
    rotateRightBtn = new QPushButton("↻", rotateGroup);

    // 设置旋转按钮样式
    rotateLeftBtn->setStyleSheet(buttonStyle);
    rotateRightBtn->setStyleSheet(buttonStyle);

    rotateLeftBtn->setFixedSize(STYLE.buttonHeight(), STYLE.buttonHeight());
    rotateRightBtn->setFixedSize(STYLE.buttonHeight(), STYLE.buttonHeight());
    rotateLeftBtn->setToolTip("向左旋转90度 (Ctrl+L)");
    rotateRightBtn->setToolTip("向右旋转90度 (Ctrl+R)");

    rotateLayout->addWidget(rotateLeftBtn);
    rotateLayout->addWidget(rotateRightBtn);

    // 主题切换控件
    QGroupBox* themeGroup = new QGroupBox("主题", toolbar);
    QHBoxLayout* themeLayout = new QHBoxLayout(themeGroup);

    themeToggleBtn = new QPushButton("🌙", themeGroup);
    themeToggleBtn->setStyleSheet(buttonStyle);
    themeToggleBtn->setFixedSize(STYLE.buttonHeight(), STYLE.buttonHeight());
    themeToggleBtn->setToolTip("切换主题 (Ctrl+T)");

    themeLayout->addWidget(themeToggleBtn);

    // 查看模式控件
    QGroupBox* viewGroup = new QGroupBox("查看模式", toolbar);
    QHBoxLayout* viewLayout = new QHBoxLayout(viewGroup);

    viewModeComboBox = new QComboBox(viewGroup);
    viewModeComboBox->addItem("单页视图",
                              static_cast<int>(PDFViewMode::SinglePage));
    viewModeComboBox->addItem("连续滚动",
                              static_cast<int>(PDFViewMode::ContinuousScroll));
    viewModeComboBox->setCurrentIndex(0);  // 默认单页视图

    viewLayout->addWidget(viewModeComboBox);

    toolbarLayout->addWidget(navGroup);
    toolbarLayout->addWidget(zoomGroup);
    toolbarLayout->addWidget(rotateGroup);
    toolbarLayout->addWidget(themeGroup);
    toolbarLayout->addWidget(viewGroup);
    toolbarLayout->addStretch();

    // 创建视图堆叠组件
    viewStack = new QStackedWidget(this);

    setupViewModes();

    // 创建搜索组件
    searchWidget = new SearchWidget(this);
    searchWidget->setVisible(false);  // 默认隐藏

    mainLayout->addWidget(toolbar);
    mainLayout->addWidget(searchWidget);
    mainLayout->addWidget(viewStack, 1);
}

void PDFViewer::setupViewModes() {
    // 创建单页视图
    singlePageScrollArea = new QScrollArea(this);
    singlePageWidget = new PDFPageWidget(singlePageScrollArea);
    singlePageScrollArea->setWidget(singlePageWidget);
    singlePageScrollArea->setWidgetResizable(true);
    singlePageScrollArea->setAlignment(Qt::AlignCenter);

    // 应用样式
    if (m_enableStyling) {
        singlePageScrollArea->setStyleSheet(STYLE.getPDFViewerStyleSheet() +
                                            STYLE.getScrollBarStyleSheet());
    }

    // 创建连续滚动视图
    continuousScrollArea = new QScrollArea(this);
    continuousWidget = new QWidget(continuousScrollArea);
    continuousLayout = new QVBoxLayout(continuousWidget);
    if (m_enableStyling) {
        continuousLayout->setContentsMargins(STYLE.margin(), STYLE.margin(),
                                             STYLE.margin(), STYLE.margin());
        continuousLayout->setSpacing(STYLE.spacing() * 2);
    } else {
        continuousLayout->setContentsMargins(12, 12, 12, 12);
        continuousLayout->setSpacing(16);
    }
    continuousScrollArea->setWidget(continuousWidget);
    continuousScrollArea->setWidgetResizable(true);

    // 应用样式
    if (m_enableStyling) {
        continuousScrollArea->setStyleSheet(STYLE.getPDFViewerStyleSheet() +
                                            STYLE.getScrollBarStyleSheet());
    }

    // 添加到堆叠组件
    viewStack->addWidget(singlePageScrollArea);  // index 0
    viewStack->addWidget(continuousScrollArea);  // index 1

    // 为连续滚动区域安装事件过滤器以处理Ctrl+滚轮缩放
    continuousScrollArea->installEventFilter(this);

    // 默认显示单页视图
    viewStack->setCurrentIndex(0);
}

void PDFViewer::setupConnections() {
    // 页面导航
    connect(firstPageBtn, &QPushButton::clicked, this, &PDFViewer::firstPage);
    connect(prevPageBtn, &QPushButton::clicked, this, &PDFViewer::previousPage);
    connect(nextPageBtn, &QPushButton::clicked, this, &PDFViewer::nextPage);
    connect(lastPageBtn, &QPushButton::clicked, this, &PDFViewer::lastPage);
    connect(pageNumberSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PDFViewer::onPageNumberChanged);

    // 缩放控制
    connect(zoomInBtn, &QPushButton::clicked, this, &PDFViewer::zoomIn);
    connect(zoomOutBtn, &QPushButton::clicked, this, &PDFViewer::zoomOut);
    connect(zoomSlider, &QSlider::valueChanged, this,
            &PDFViewer::onZoomSliderChanged);
    connect(zoomPercentageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PDFViewer::onZoomPercentageChanged);
    connect(fitWidthBtn, &QPushButton::clicked, this, &PDFViewer::zoomToWidth);
    connect(fitHeightBtn, &QPushButton::clicked, this,
            &PDFViewer::zoomToHeight);
    connect(fitPageBtn, &QPushButton::clicked, this, &PDFViewer::zoomToFit);

    // 旋转控制
    connect(rotateLeftBtn, &QPushButton::clicked, this, &PDFViewer::rotateLeft);
    connect(rotateRightBtn, &QPushButton::clicked, this,
            &PDFViewer::rotateRight);

    // 主题切换
    connect(themeToggleBtn, &QPushButton::clicked, this,
            &PDFViewer::toggleTheme);

    // 搜索组件连接
    if (searchWidget) {
        connect(searchWidget, &SearchWidget::searchRequested, this,
                &PDFViewer::onSearchRequested);
        connect(searchWidget, &SearchWidget::resultSelected, this,
                &PDFViewer::onSearchResultSelected);
        connect(searchWidget, &SearchWidget::navigateToResult, this,
                &PDFViewer::onNavigateToSearchResult);
        connect(searchWidget, &SearchWidget::searchClosed, this,
                &PDFViewer::hideSearch);
        connect(searchWidget, &SearchWidget::searchCleared, this,
                &PDFViewer::clearSearchHighlights);

        // Connect search model signals for real-time highlighting
        if (searchWidget->getSearchModel()) {
            connect(searchWidget->getSearchModel(),
                    &SearchModel::realTimeResultsUpdated, this,
                    &PDFViewer::setSearchResults);
        }
    }

    // 防抖定时器
    connect(zoomTimer, &QTimer::timeout, this, &PDFViewer::onZoomTimerTimeout);
    connect(scrollTimer, &QTimer::timeout, this, &PDFViewer::onScrollChanged);

    // 查看模式控制
    connect(viewModeComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &PDFViewer::onViewModeChanged);

    // 页面组件信号
    connect(singlePageWidget, &PDFPageWidget::scaleChanged, this,
            &PDFViewer::onScaleChanged);
}

void PDFViewer::setupShortcuts() {
    // 缩放快捷键
    zoomInShortcut = new QShortcut(QKeySequence("Ctrl++"), this);
    zoomOutShortcut = new QShortcut(QKeySequence("Ctrl+-"), this);
    fitPageShortcut = new QShortcut(QKeySequence("Ctrl+0"), this);
    fitWidthShortcut = new QShortcut(QKeySequence("Ctrl+1"), this);
    fitHeightShortcut = new QShortcut(QKeySequence("Ctrl+2"), this);

    // 额外缩放快捷键
    QShortcut* zoomIn2 = new QShortcut(QKeySequence("Ctrl+="), this);
    QShortcut* zoomActualSize = new QShortcut(QKeySequence("Ctrl+Alt+0"), this);
    QShortcut* zoom25 = new QShortcut(QKeySequence("Ctrl+Alt+1"), this);
    QShortcut* zoom50 = new QShortcut(QKeySequence("Ctrl+Alt+2"), this);
    QShortcut* zoom75 = new QShortcut(QKeySequence("Ctrl+Alt+3"), this);
    QShortcut* zoom100 = new QShortcut(QKeySequence("Ctrl+Alt+4"), this);
    QShortcut* zoom150 = new QShortcut(QKeySequence("Ctrl+Alt+5"), this);
    QShortcut* zoom200 = new QShortcut(QKeySequence("Ctrl+Alt+6"), this);

    // 旋转快捷键
    rotateLeftShortcut = new QShortcut(QKeySequence("Ctrl+L"), this);
    rotateRightShortcut = new QShortcut(QKeySequence("Ctrl+R"), this);
    QShortcut* rotate180 = new QShortcut(QKeySequence("Ctrl+Shift+R"), this);

    // 主题切换快捷键
    QShortcut* themeToggleShortcut =
        new QShortcut(QKeySequence("Ctrl+T"), this);

    // 导航快捷键 - 基本
    firstPageShortcut = new QShortcut(QKeySequence("Ctrl+Home"), this);
    lastPageShortcut = new QShortcut(QKeySequence("Ctrl+End"), this);
    nextPageShortcut = new QShortcut(QKeySequence("Page Down"), this);
    prevPageShortcut = new QShortcut(QKeySequence("Page Up"), this);

    // 导航快捷键 - 高级
    QShortcut* nextPage2 = new QShortcut(QKeySequence("Space"), this);
    QShortcut* prevPage2 = new QShortcut(QKeySequence("Shift+Space"), this);
    QShortcut* nextPage3 = new QShortcut(QKeySequence("Right"), this);
    QShortcut* prevPage3 = new QShortcut(QKeySequence("Left"), this);
    QShortcut* nextPage4 = new QShortcut(QKeySequence("Down"), this);
    QShortcut* prevPage4 = new QShortcut(QKeySequence("Up"), this);
    QShortcut* jump10Forward = new QShortcut(QKeySequence("Ctrl+Right"), this);
    QShortcut* jump10Backward = new QShortcut(QKeySequence("Ctrl+Left"), this);
    QShortcut* gotoPage = new QShortcut(QKeySequence("Ctrl+G"), this);

    // 视图模式快捷键
    QShortcut* toggleFullscreen = new QShortcut(QKeySequence("F11"), this);
    QShortcut* toggleSidebar = new QShortcut(QKeySequence("F9"), this);
    QShortcut* presentationMode = new QShortcut(QKeySequence("F5"), this);
    QShortcut* readingMode = new QShortcut(QKeySequence("F6"), this);

    // 搜索快捷键
    QShortcut* findShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);
    QShortcut* findNext = new QShortcut(QKeySequence("F3"), this);
    QShortcut* findPrev = new QShortcut(QKeySequence("Shift+F3"), this);

    // 书签快捷键
    QShortcut* addBookmark = new QShortcut(QKeySequence("Ctrl+D"), this);
    QShortcut* showBookmarks = new QShortcut(QKeySequence("Ctrl+B"), this);

    // 文档操作快捷键
    QShortcut* refresh = new QShortcut(QKeySequence("F5"), this);
    QShortcut* properties = new QShortcut(QKeySequence("Alt+Enter"), this);
    QShortcut* selectAll = new QShortcut(QKeySequence("Ctrl+A"), this);
    QShortcut* copyText = new QShortcut(QKeySequence("Ctrl+C"), this);

    // 连接快捷键信号 - 基本缩放
    connect(zoomInShortcut, &QShortcut::activated, this, &PDFViewer::zoomIn);
    connect(zoomOutShortcut, &QShortcut::activated, this, &PDFViewer::zoomOut);
    connect(zoomIn2, &QShortcut::activated, this, &PDFViewer::zoomIn);
    connect(fitPageShortcut, &QShortcut::activated, this,
            &PDFViewer::zoomToFit);
    connect(fitWidthShortcut, &QShortcut::activated, this,
            &PDFViewer::zoomToWidth);
    connect(fitHeightShortcut, &QShortcut::activated, this,
            &PDFViewer::zoomToHeight);

    // 连接预设缩放级别
    connect(zoomActualSize, &QShortcut::activated, this,
            [this]() { setZoom(1.0); });
    connect(zoom25, &QShortcut::activated, this, [this]() { setZoom(0.25); });
    connect(zoom50, &QShortcut::activated, this, [this]() { setZoom(0.5); });
    connect(zoom75, &QShortcut::activated, this, [this]() { setZoom(0.75); });
    connect(zoom100, &QShortcut::activated, this, [this]() { setZoom(1.0); });
    connect(zoom150, &QShortcut::activated, this, [this]() { setZoom(1.5); });
    connect(zoom200, &QShortcut::activated, this, [this]() { setZoom(2.0); });

    // 连接旋转快捷键
    connect(rotateLeftShortcut, &QShortcut::activated, this,
            &PDFViewer::rotateLeft);
    connect(rotateRightShortcut, &QShortcut::activated, this,
            &PDFViewer::rotateRight);
    connect(rotate180, &QShortcut::activated, this,
            [this]() { setRotation(currentRotation + 180); });

    // 连接主题快捷键
    connect(themeToggleShortcut, &QShortcut::activated, this,
            &PDFViewer::toggleTheme);

    // 连接基本导航快捷键
    connect(firstPageShortcut, &QShortcut::activated, this,
            &PDFViewer::firstPage);
    connect(lastPageShortcut, &QShortcut::activated, this,
            &PDFViewer::lastPage);
    connect(nextPageShortcut, &QShortcut::activated, this,
            &PDFViewer::nextPage);
    connect(prevPageShortcut, &QShortcut::activated, this,
            &PDFViewer::previousPage);

    // 连接高级导航快捷键
    connect(nextPage2, &QShortcut::activated, this, &PDFViewer::nextPage);
    connect(prevPage2, &QShortcut::activated, this, &PDFViewer::previousPage);
    connect(nextPage3, &QShortcut::activated, this, &PDFViewer::nextPage);
    connect(prevPage3, &QShortcut::activated, this, &PDFViewer::previousPage);
    connect(nextPage4, &QShortcut::activated, this, &PDFViewer::nextPage);
    connect(prevPage4, &QShortcut::activated, this, &PDFViewer::previousPage);

    // 连接跳转快捷键
    connect(jump10Forward, &QShortcut::activated, this,
            [this]() { goToPage(currentPageNumber + 10); });
    connect(jump10Backward, &QShortcut::activated, this,
            [this]() { goToPage(currentPageNumber - 10); });
    connect(gotoPage, &QShortcut::activated, this, [this]() {
        // Focus on page number input
        if (pageNumberSpinBox) {
            pageNumberSpinBox->setFocus();
            pageNumberSpinBox->selectAll();
        }
    });

    // 连接视图模式快捷键
    connect(toggleFullscreen, &QShortcut::activated, this, [this]() {
        // Toggle fullscreen mode
        if (window()->isFullScreen()) {
            window()->showNormal();
        } else {
            window()->showFullScreen();
        }
    });

    connect(toggleSidebar, &QShortcut::activated, this, [this]() {
        // Emit signal to toggle sidebar
        emit sidebarToggleRequested();
    });

    // 连接搜索快捷键
    connect(findShortcut, &QShortcut::activated, this, &PDFViewer::showSearch);

    // 连接书签快捷键
    connect(addBookmark, &QShortcut::activated, this, [this]() {
        if (document && currentPageNumber >= 0) {
            emit bookmarkRequested(currentPageNumber);
        }
    });

    // 连接文档操作快捷键
    connect(refresh, &QShortcut::activated, this, [this]() {
        // Refresh current page
        if (singlePageWidget) {
            singlePageWidget->renderPage();
        }
    });
}

void PDFViewer::setDocument(Poppler::Document* doc) {
    try {
        // 清理旧文档
        if (document) {
            clearPageCache();  // 清理缓存
        }

        document = doc;
        currentPageNumber = 0;
        currentRotation = 0;  // 重置旋转

        if (document) {
            // Configure document for high-quality rendering
            document->setRenderHint(Poppler::Document::Antialiasing, true);
            document->setRenderHint(Poppler::Document::TextAntialiasing, true);
            document->setRenderHint(Poppler::Document::TextHinting, true);
            document->setRenderHint(Poppler::Document::TextSlightHinting, true);
            document->setRenderHint(Poppler::Document::ThinLineShape, true);
            document->setRenderHint(Poppler::Document::OverprintPreview, true);
            // 验证文档有效性
            int numPages = document->numPages();
            if (numPages <= 0) {
                throw std::runtime_error("文档没有有效页面");
            }

            // 测试第一页是否可以访问
            std::unique_ptr<Poppler::Page> testPage(document->page(0));
            if (!testPage) {
                throw std::runtime_error("无法访问文档页面");
            }

            pageNumberSpinBox->setRange(1, numPages);
            pageNumberSpinBox->setValue(1);
            pageCountLabel->setText(QString("/ %1").arg(numPages));
            updatePageDisplay();

            // 如果是连续模式，创建所有页面
            if (currentViewMode == PDFViewMode::ContinuousScroll) {
                createContinuousPages();
            }

            setMessage(QString("文档加载成功，共 %1 页").arg(numPages));

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
            // 如果启用了QGraphics渲染，也为其设置文档
            if (useQGraphicsViewer && qgraphicsViewer) {
                qgraphicsViewer->setDocument(document);
                qgraphicsViewer->goToPage(currentPageNumber);
            }
#endif

        } else {
            pageNumberSpinBox->setRange(0, 0);
            pageCountLabel->setText("/ 0");
            singlePageWidget->setPage(nullptr);

            // 清空连续视图
            QLayoutItem* item;
            while ((item = continuousLayout->takeAt(0)) != nullptr) {
                delete item->widget();
                delete item;
            }

            setMessage("文档已关闭");

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
            // 清理QGraphics查看器
            if (qgraphicsViewer) {
                qgraphicsViewer->clearDocument();
            }
#endif
        }

        updateNavigationButtons();
        emit documentChanged(document != nullptr);

    } catch (const std::exception& e) {
        // 文档加载失败，清理状态
        document = nullptr;
        pageNumberSpinBox->setRange(0, 0);
        pageCountLabel->setText("/ 0");
        singlePageWidget->setPage(nullptr);

        setMessage(QString("文档加载失败: %1").arg(e.what()));
        qDebug() << "Document loading failed:" << e.what();

        updateNavigationButtons();
        emit documentChanged(false);
    }
}

void PDFViewer::clearDocument() { setDocument(nullptr); }

void PDFViewer::goToPage(int pageNumber) {
    goToPageWithValidation(pageNumber, false);
}

bool PDFViewer::goToPageWithValidation(int pageNumber, bool showMessage) {
    if (!document) {
        if (showMessage) {
            setMessage("没有打开的文档");
        }
        return false;
    }

    if (pageNumber < 0 || pageNumber >= document->numPages()) {
        if (showMessage) {
            setMessage(
                QString("页码超出范围 (1-%1)").arg(document->numPages()));
        }
        return false;
    }

    currentPageNumber = pageNumber;
    pageNumberSpinBox->setValue(pageNumber + 1);

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    // 如果使用QGraphics渲染，也更新QGraphics查看器
    if (useQGraphicsViewer && qgraphicsViewer) {
        qgraphicsViewer->goToPage(pageNumber);
    } else {
        updatePageDisplay();
    }
#else
    updatePageDisplay();
#endif

    updateNavigationButtons();

    // Update search highlights for the new page
    updateSearchHighlightsForCurrentPage();

    emit pageChanged(pageNumber);

    if (showMessage) {
        setMessage(QString("跳转到第 %1 页").arg(pageNumber + 1));
    }

    return true;
}

void PDFViewer::nextPage() {
    if (document && currentPageNumber < document->numPages() - 1) {
        goToPage(currentPageNumber + 1);
    }
}

void PDFViewer::previousPage() {
    if (document && currentPageNumber > 0) {
        goToPage(currentPageNumber - 1);
    }
}

void PDFViewer::firstPage() {
    if (document) {
        goToPage(0);
    }
}

void PDFViewer::lastPage() {
    if (document) {
        goToPage(document->numPages() - 1);
    }
}

void PDFViewer::zoomIn() {
    double newZoom = currentZoomFactor + ZOOM_STEP;
    setZoomWithType(newZoom, ZoomType::FixedValue);
}

void PDFViewer::zoomOut() {
    double newZoom = currentZoomFactor - ZOOM_STEP;
    setZoomWithType(newZoom, ZoomType::FixedValue);
}

void PDFViewer::zoomToFit() {
    if (!document)
        return;

    // 获取当前视图的viewport大小
    QScrollArea* currentScrollArea =
        (currentViewMode == PDFViewMode::SinglePage) ? singlePageScrollArea
                                                     : continuousScrollArea;
    QSize viewportSize = currentScrollArea->viewport()->size();

    if (document->numPages() > 0) {
        std::unique_ptr<Poppler::Page> page(document->page(currentPageNumber));
        if (page) {
            QSizeF pageSize = page->pageSizeF();
            double scaleX = viewportSize.width() / pageSize.width();
            double scaleY = viewportSize.height() / pageSize.height();
            setZoomWithType(qMin(scaleX, scaleY) * 0.9,
                            ZoomType::FitPage);  // 留一些边距
        }
    }
}

void PDFViewer::zoomToWidth() {
    if (!document)
        return;

    // 获取当前视图的viewport大小
    QScrollArea* currentScrollArea =
        (currentViewMode == PDFViewMode::SinglePage) ? singlePageScrollArea
                                                     : continuousScrollArea;
    QSize viewportSize = currentScrollArea->viewport()->size();

    if (document->numPages() > 0) {
        std::unique_ptr<Poppler::Page> page(document->page(currentPageNumber));
        if (page) {
            QSizeF pageSize = page->pageSizeF();
            double scale = viewportSize.width() / pageSize.width();
            setZoomWithType(scale * 0.95, ZoomType::FitWidth);  // 留一些边距
        }
    }
}

void PDFViewer::setZoom(double factor) {
    setZoomWithType(factor, ZoomType::FixedValue);
}

int PDFViewer::getPageCount() const {
    return document ? document->numPages() : 0;
}

double PDFViewer::getCurrentZoom() const { return currentZoomFactor; }

void PDFViewer::updatePageDisplay() {
    if (!document || currentPageNumber < 0 ||
        currentPageNumber >= document->numPages()) {
        if (currentViewMode == PDFViewMode::SinglePage) {
            singlePageWidget->setPage(nullptr, currentZoomFactor,
                                      currentRotation);
        }
        return;
    }

    if (currentViewMode == PDFViewMode::SinglePage) {
        // 添加淡入淡出动画
        if (fadeAnimation->state() != QAbstractAnimation::Running) {
            singlePageWidget->setGraphicsEffect(opacityEffect);
            fadeAnimation->setStartValue(0.3);
            fadeAnimation->setEndValue(1.0);
            fadeAnimation->start();
        }

        std::unique_ptr<Poppler::Page> page(document->page(currentPageNumber));
        if (page) {
            singlePageWidget->setPage(page.get(), currentZoomFactor,
                                      currentRotation);
        }
    }
    // 连续模式下不需要更新单个页面，因为所有页面都已经渲染
}

void PDFViewer::updateContinuousView() {
    if (!document || currentViewMode != PDFViewMode::ContinuousScroll) {
        return;
    }

    // 优化：只更新缩放因子，避免重新渲染所有页面
    for (int i = 0; i < continuousLayout->count() - 1;
         ++i) {  // -1 因为最后一个是stretch
        QLayoutItem* item = continuousLayout->itemAt(i);
        if (item && item->widget()) {
            PDFPageWidget* pageWidget =
                qobject_cast<PDFPageWidget*>(item->widget());
            if (pageWidget) {
                // 阻止信号发出，避免循环
                pageWidget->blockSignals(true);
                pageWidget->setScaleFactor(currentZoomFactor);
                pageWidget->blockSignals(false);
            }
        }
    }
}

void PDFViewer::updateNavigationButtons() {
    bool hasDoc = (document != nullptr);
    bool hasPages = hasDoc && document->numPages() > 0;
    bool notFirst = hasPages && currentPageNumber > 0;
    bool notLast = hasPages && currentPageNumber < document->numPages() - 1;

    // 导航按钮状态
    firstPageBtn->setEnabled(notFirst);
    prevPageBtn->setEnabled(notFirst);
    nextPageBtn->setEnabled(notLast);
    lastPageBtn->setEnabled(notLast);
    pageNumberSpinBox->setEnabled(hasPages);

    // 缩放按钮状态
    zoomInBtn->setEnabled(hasPages && currentZoomFactor < MAX_ZOOM);
    zoomOutBtn->setEnabled(hasPages && currentZoomFactor > MIN_ZOOM);
    zoomSlider->setEnabled(hasPages);
    zoomPercentageSpinBox->setEnabled(hasPages);
    fitWidthBtn->setEnabled(hasPages);
    fitHeightBtn->setEnabled(hasPages);
    fitPageBtn->setEnabled(hasPages);

    // 旋转按钮状态
    rotateLeftBtn->setEnabled(hasPages);
    rotateRightBtn->setEnabled(hasPages);

    // 查看模式选择状态
    viewModeComboBox->setEnabled(hasPages);

    // 更新按钮工具提示
    if (!hasPages) {
        firstPageBtn->setToolTip("需要先打开文档");
        prevPageBtn->setToolTip("需要先打开文档");
        nextPageBtn->setToolTip("需要先打开文档");
        lastPageBtn->setToolTip("需要先打开文档");
        rotateLeftBtn->setToolTip("需要先打开文档");
        rotateRightBtn->setToolTip("需要先打开文档");
    } else {
        firstPageBtn->setToolTip("第一页");
        prevPageBtn->setToolTip("上一页");
        nextPageBtn->setToolTip("下一页");
        lastPageBtn->setToolTip("最后一页");
        rotateLeftBtn->setToolTip("向左旋转90度");
        rotateRightBtn->setToolTip("向右旋转90度");
    }
}

void PDFViewer::updateZoomControls() {
    int percentageValue = static_cast<int>(currentZoomFactor * 100);

    // 更新滑块和百分比输入框（阻止信号避免循环）
    zoomSlider->blockSignals(true);
    zoomPercentageSpinBox->blockSignals(true);

    zoomSlider->setValue(percentageValue);
    zoomPercentageSpinBox->setValue(percentageValue);

    zoomSlider->blockSignals(false);
    zoomPercentageSpinBox->blockSignals(false);

    // 更新按钮状态
    zoomInBtn->setEnabled(currentZoomFactor < MAX_ZOOM);
    zoomOutBtn->setEnabled(currentZoomFactor > MIN_ZOOM);
}

void PDFViewer::onPageNumberChanged(int pageNumber) {
    goToPage(pageNumber - 1);  // SpinBox is 1-based, internal is 0-based
}

void PDFViewer::onZoomSliderChanged(int value) {
    double factor = value / 100.0;
    setZoom(factor);
}

void PDFViewer::onScaleChanged(double scale) {
    // 防止信号循环：只有当缩放来自用户交互（如Ctrl+滚轮）时才处理
    if (scale != currentZoomFactor && !isZoomPending) {
        currentZoomFactor = scale;
        updateZoomControls();
        saveZoomSettings();
        emit zoomChanged(scale);
    }
}

void PDFViewer::setViewMode(PDFViewMode mode) {
    if (mode == currentViewMode) {
        return;
    }

    // 保存当前状态
    int savedPageNumber = currentPageNumber;
    double savedZoomFactor = currentZoomFactor;
    int savedRotation = currentRotation;

    PDFViewMode oldMode = currentViewMode;
    currentViewMode = mode;

    try {
        // 更新UI
        viewModeComboBox->blockSignals(true);
        viewModeComboBox->setCurrentIndex(static_cast<int>(mode));
        viewModeComboBox->blockSignals(false);

        // 切换视图
        if (mode == PDFViewMode::SinglePage) {
            switchToSinglePageMode();
        } else {
            switchToContinuousMode();
        }

        // 恢复状态
        currentPageNumber = savedPageNumber;
        currentZoomFactor = savedZoomFactor;
        currentRotation = savedRotation;

        // 更新显示
        updatePageDisplay();
        updateNavigationButtons();
        updateZoomControls();

        emit viewModeChanged(mode);
        setMessage(
            QString("切换到%1模式")
                .arg(mode == PDFViewMode::SinglePage ? "单页" : "连续滚动"));

    } catch (const std::exception& e) {
        // 恢复到原来的模式
        currentViewMode = oldMode;
        viewModeComboBox->blockSignals(true);
        viewModeComboBox->setCurrentIndex(static_cast<int>(oldMode));
        viewModeComboBox->blockSignals(false);

        setMessage(QString("切换视图模式失败: %1").arg(e.what()));
        qDebug() << "View mode switch failed:" << e.what();
    }
}

void PDFViewer::switchToSinglePageMode() {
    viewStack->setCurrentIndex(0);
    updatePageDisplay();
}

void PDFViewer::switchToContinuousMode() {
    viewStack->setCurrentIndex(1);
    if (document) {
        createContinuousPages();
    }
}

void PDFViewer::createContinuousPages() {
    if (!document)
        return;

    // 清空现有页面
    QLayoutItem* item;
    while ((item = continuousLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    // 创建所有页面
    for (int i = 0; i < document->numPages(); ++i) {
        PDFPageWidget* pageWidget = new PDFPageWidget(continuousWidget);

        std::unique_ptr<Poppler::Page> page(document->page(i));
        if (page) {
            // 阻止信号发出，避免在初始化时触发缩放循环
            pageWidget->blockSignals(true);
            pageWidget->setPage(page.get(), currentZoomFactor, currentRotation);
            pageWidget->blockSignals(false);
        }

        continuousLayout->addWidget(pageWidget);

        // 连接信号
        connect(pageWidget, &PDFPageWidget::scaleChanged, this,
                &PDFViewer::onScaleChanged);
    }

    continuousLayout->addStretch();

    // 连接滚动区域的滚动信号以实现虚拟化渲染
    if (continuousScrollArea->verticalScrollBar()) {
        connect(continuousScrollArea->verticalScrollBar(),
                &QScrollBar::valueChanged, this, [this]() {
                    scrollTimer->start();  // 使用防抖
                });
    }
}

void PDFViewer::updateVisiblePages() {
    if (!document || currentViewMode != PDFViewMode::ContinuousScroll) {
        return;
    }

    QScrollBar* scrollBar = continuousScrollArea->verticalScrollBar();
    if (!scrollBar)
        return;

    int viewportHeight = continuousScrollArea->viewport()->height();
    int scrollValue = scrollBar->value();

    // 估算可见页面范围
    int totalPages = document->numPages();
    if (totalPages == 0)
        return;

    // 简化计算：假设所有页面高度相似
    int estimatedPageHeight = viewportHeight;  // 粗略估算
    if (continuousLayout->count() > 1) {
        QLayoutItem* firstItem = continuousLayout->itemAt(0);
        if (firstItem && firstItem->widget()) {
            estimatedPageHeight = firstItem->widget()->height();
        }
    }

    if (estimatedPageHeight <= 0)
        estimatedPageHeight = viewportHeight;

    int newVisibleStart =
        qMax(0, (scrollValue / estimatedPageHeight) - renderBuffer);
    int newVisibleEnd = qMin(
        totalPages - 1,
        ((scrollValue + viewportHeight) / estimatedPageHeight) + renderBuffer);

    // 如果可见范围发生变化，更新渲染
    if (newVisibleStart != visiblePageStart ||
        newVisibleEnd != visiblePageEnd) {
        visiblePageStart = newVisibleStart;
        visiblePageEnd = newVisibleEnd;
        renderVisiblePages();
    }
}

void PDFViewer::renderVisiblePages() {
    if (!document || currentViewMode != PDFViewMode::ContinuousScroll) {
        return;
    }

    // 这里可以实现更复杂的虚拟化逻辑
    // 目前保持简单的实现，只在需要时更新页面
    for (int i = 0; i < continuousLayout->count() - 1; ++i) {
        QLayoutItem* item = continuousLayout->itemAt(i);
        if (item && item->widget()) {
            PDFPageWidget* pageWidget =
                qobject_cast<PDFPageWidget*>(item->widget());
            if (pageWidget) {
                // 只渲染可见范围内的页面
                bool shouldRender =
                    (i >= visiblePageStart && i <= visiblePageEnd);
                pageWidget->setVisible(shouldRender);
            }
        }
    }
}

void PDFViewer::onScrollChanged() {
    if (currentViewMode == PDFViewMode::ContinuousScroll) {
        updateVisiblePages();
    }
}

QPixmap PDFViewer::getCachedPage(int pageNumber, double zoomFactor,
                                 int rotation) {
    auto it = pageCache.find(pageNumber);
    if (it != pageCache.end()) {
        const PageCacheItem& item = it.value();
        // 检查缓存是否匹配当前参数
        if (qAbs(item.zoomFactor - zoomFactor) < 0.001 &&
            item.rotation == rotation) {
            // 更新访问时间
            const_cast<PageCacheItem&>(item).lastAccessed =
                QDateTime::currentMSecsSinceEpoch();
            return item.pixmap;
        }
    }
    return QPixmap();  // 返回空的QPixmap表示缓存未命中
}

void PDFViewer::setCachedPage(int pageNumber, const QPixmap& pixmap,
                              double zoomFactor, int rotation) {
    // 如果缓存已满，清理最旧的条目
    if (pageCache.size() >= maxCacheSize) {
        cleanupCache();
    }

    PageCacheItem item;
    item.pixmap = pixmap;
    item.zoomFactor = zoomFactor;
    item.rotation = rotation;
    item.lastAccessed = QDateTime::currentMSecsSinceEpoch();

    pageCache[pageNumber] = item;
}

void PDFViewer::clearPageCache() { pageCache.clear(); }

void PDFViewer::cleanupCache() {
    if (pageCache.size() <= maxCacheSize / 2) {
        return;  // 不需要清理
    }

    // 找到最旧的条目并删除
    QList<int> keysToRemove;
    qint64 oldestTime = QDateTime::currentMSecsSinceEpoch();

    for (auto it = pageCache.begin(); it != pageCache.end(); ++it) {
        if (it.value().lastAccessed < oldestTime) {
            oldestTime = it.value().lastAccessed;
        }
    }

    // 删除超过一半的最旧条目
    int removeCount = pageCache.size() - maxCacheSize / 2;
    for (auto it = pageCache.begin(); it != pageCache.end() && removeCount > 0;
         ++it) {
        if (it.value().lastAccessed <= oldestTime + 1000) {  // 1秒容差
            keysToRemove.append(it.key());
            removeCount--;
        }
    }

    for (int key : keysToRemove) {
        pageCache.remove(key);
    }
}

void PDFViewer::toggleTheme() {
    Theme currentTheme = STYLE.currentTheme();
    Theme newTheme =
        (currentTheme == Theme::Light) ? Theme::Dark : Theme::Light;

    STYLE.setTheme(newTheme);

    // 更新主题按钮图标
    if (newTheme == Theme::Dark) {
        themeToggleBtn->setText("☀");
        themeToggleBtn->setToolTip("切换到亮色主题 (Ctrl+T)");
    } else {
        themeToggleBtn->setText("🌙");
        themeToggleBtn->setToolTip("切换到暗色主题 (Ctrl+T)");
    }

    // 重新应用样式
    setStyleSheet(STYLE.getApplicationStyleSheet());

    // 更新所有子组件的样式
    QString buttonStyle = STYLE.getButtonStyleSheet();
    firstPageBtn->setStyleSheet(buttonStyle);
    prevPageBtn->setStyleSheet(buttonStyle);
    nextPageBtn->setStyleSheet(buttonStyle);
    lastPageBtn->setStyleSheet(buttonStyle);
    zoomOutBtn->setStyleSheet(buttonStyle);
    zoomInBtn->setStyleSheet(buttonStyle);
    fitWidthBtn->setStyleSheet(buttonStyle);
    fitHeightBtn->setStyleSheet(buttonStyle);
    fitPageBtn->setStyleSheet(buttonStyle);
    rotateLeftBtn->setStyleSheet(buttonStyle);
    rotateRightBtn->setStyleSheet(buttonStyle);
    themeToggleBtn->setStyleSheet(buttonStyle);

    // 更新滚动区域样式
    QString scrollStyle =
        STYLE.getPDFViewerStyleSheet() + STYLE.getScrollBarStyleSheet();
    singlePageScrollArea->setStyleSheet(scrollStyle);
    continuousScrollArea->setStyleSheet(scrollStyle);

    setMessage(QString("已切换到%1主题")
                   .arg(newTheme == Theme::Dark ? "暗色" : "亮色"));
}

void PDFViewer::onViewModeChanged(int index) {
    PDFViewMode mode = static_cast<PDFViewMode>(index);
    setViewMode(mode);
}

void PDFViewer::onZoomPercentageChanged() {
    int percentage = zoomPercentageSpinBox->value();
    setZoomFromPercentage(percentage);
}

void PDFViewer::onZoomTimerTimeout() {
    if (isZoomPending) {
        double factor = pendingZoomFactor;
        isZoomPending = false;
        applyZoom(factor);
    }
}

void PDFViewer::setZoomFromPercentage(int percentage) {
    double factor = percentage / 100.0;
    setZoomWithType(factor, ZoomType::FixedValue);
}

void PDFViewer::setZoomWithType(double factor, ZoomType type) {
    // 检查文档有效性
    if (!document || document->numPages() == 0) {
        qDebug() << "Cannot zoom: no valid document";
        return;
    }

    currentZoomType = type;

    // 限制缩放范围
    factor = qBound(MIN_ZOOM, factor, MAX_ZOOM);

    // 如果值没有变化，直接返回
    if (qAbs(factor - currentZoomFactor) < 0.001) {
        return;
    }

    // 改进的防抖机制
    bool shouldUseDebounce = false;

    // 根据缩放类型和变化幅度决定是否使用防抖
    if (type == ZoomType::FixedValue) {
        double changeMagnitude = qAbs(factor - currentZoomFactor);
        // 小幅度变化且不是第一次缩放时使用防抖
        shouldUseDebounce = (changeMagnitude < 0.15 && zoomTimer->isActive());
    }

    if (shouldUseDebounce) {
        // 使用防抖机制
        pendingZoomFactor = factor;
        isZoomPending = true;
        zoomTimer->start();  // 重新启动定时器
    } else {
        // 立即应用缩放
        if (zoomTimer->isActive()) {
            zoomTimer->stop();
        }

        // 如果有待处理的缩放，先清除
        if (isZoomPending) {
            isZoomPending = false;
        }

        applyZoom(factor);
    }
}

void PDFViewer::zoomToHeight() {
    if (!document)
        return;

    // 获取当前视图的viewport大小
    QScrollArea* currentScrollArea =
        (currentViewMode == PDFViewMode::SinglePage) ? singlePageScrollArea
                                                     : continuousScrollArea;
    QSize viewportSize = currentScrollArea->viewport()->size();

    if (document->numPages() > 0) {
        std::unique_ptr<Poppler::Page> page(document->page(currentPageNumber));
        if (page) {
            QSizeF pageSize = page->pageSizeF();
            double scale = viewportSize.height() / pageSize.height();
            setZoomWithType(scale * 0.95, ZoomType::FitHeight);  // 留一些边距
        }
    }
}

void PDFViewer::applyZoom(double factor) {
    factor = qBound(MIN_ZOOM, factor, MAX_ZOOM);
    if (factor != currentZoomFactor) {
        // 设置标志，防止信号循环
        bool wasZoomPending = isZoomPending;
        isZoomPending = true;

        currentZoomFactor = factor;

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
        // 如果使用QGraphics渲染，更新QGraphics查看器
        if (useQGraphicsViewer && qgraphicsViewer) {
            qgraphicsViewer->setZoom(factor);
        } else {
#endif
            if (currentViewMode == PDFViewMode::SinglePage) {
                // 阻止信号发出，避免循环
                singlePageWidget->blockSignals(true);
                singlePageWidget->setScaleFactor(factor);
                singlePageWidget->blockSignals(false);
            } else {
                updateContinuousView();
            }
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
        }
#endif

        updateZoomControls();
        saveZoomSettings();  // 保存缩放设置
        emit zoomChanged(factor);

        // 恢复标志状态
        isZoomPending = wasZoomPending;
    }
}

bool PDFViewer::eventFilter(QObject* object, QEvent* event) {
    // 处理连续滚动区域的Ctrl+滚轮缩放
    if (object == continuousScrollArea && event->type() == QEvent::Wheel) {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        if (wheelEvent->modifiers() & Qt::ControlModifier) {
            int delta = wheelEvent->angleDelta().y();
            if (delta != 0) {
                // 使用与PDFPageWidget相同的缩放逻辑
                double scaleDelta = delta > 0 ? 1.15 : (1.0 / 1.15);
                double newZoom = currentZoomFactor * scaleDelta;
                setZoomWithType(newZoom, ZoomType::FixedValue);
            }
            return true;  // 事件已处理
        }
    }

    return QWidget::eventFilter(object, event);
}

void PDFViewer::saveZoomSettings() {
    QSettings settings;
    settings.beginGroup("PDFViewer");
    settings.setValue("defaultZoom", currentZoomFactor);
    settings.setValue("zoomType", static_cast<int>(currentZoomType));
    settings.endGroup();
}

void PDFViewer::loadZoomSettings() {
    QSettings settings;
    settings.beginGroup("PDFViewer");

    // 加载默认缩放值
    double savedZoom = settings.value("defaultZoom", DEFAULT_ZOOM).toDouble();
    int savedZoomType =
        settings.value("zoomType", static_cast<int>(ZoomType::FixedValue))
            .toInt();

    settings.endGroup();

    // 应用保存的设置
    currentZoomFactor = qBound(MIN_ZOOM, savedZoom, MAX_ZOOM);
    currentZoomType = static_cast<ZoomType>(savedZoomType);
}

void PDFViewer::keyPressEvent(QKeyEvent* event) {
    // 处理页码输入框的回车键
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (pageNumberSpinBox->hasFocus()) {
            // 如果页码输入框有焦点，应用当前值并跳转
            int pageNumber = pageNumberSpinBox->value();
            if (goToPageWithValidation(
                    pageNumber - 1,
                    true)) {  // SpinBox is 1-based, internal is 0-based
                pageNumberSpinBox->clearFocus();  // 清除焦点
            }
            event->accept();
            return;
        }
    }

    QWidget::keyPressEvent(event);
}

void PDFViewer::setMessage(const QString& message) {
    // 发出信号让主窗口显示消息
    // 这里可以通过信号传递给StatusBar或者其他消息显示组件
    qDebug() << "PDFViewer Message:" << message;
}

void PDFViewer::rotateLeft() {
    if (!document || document->numPages() == 0) {
        setMessage("没有可旋转的文档");
        return;
    }
    setRotation(currentRotation - 90);
}

void PDFViewer::rotateRight() {
    if (!document || document->numPages() == 0) {
        setMessage("没有可旋转的文档");
        return;
    }
    setRotation(currentRotation + 90);
}

void PDFViewer::resetRotation() {
    if (!document || document->numPages() == 0) {
        setMessage("没有可重置的文档");
        return;
    }
    setRotation(0);
}

void PDFViewer::setRotation(int degrees) {
    // 检查文档有效性
    if (!document || document->numPages() == 0) {
        qDebug() << "Cannot rotate: no valid document";
        return;
    }

    // 确保旋转角度是90度的倍数
    degrees = ((degrees % 360) + 360) % 360;

    if (degrees != currentRotation) {
        int oldRotation = currentRotation;
        currentRotation = degrees;

        try {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
            // 如果使用QGraphics渲染，更新QGraphics查看器
            if (useQGraphicsViewer && qgraphicsViewer) {
                qgraphicsViewer->setRotation(currentRotation);
            } else {
#endif
                // 更新当前视图
                if (currentViewMode == PDFViewMode::SinglePage) {
                    if (currentPageNumber >= 0 &&
                        currentPageNumber < document->numPages()) {
                        std::unique_ptr<Poppler::Page> page(
                            document->page(currentPageNumber));
                        if (page) {
                            singlePageWidget->setPage(
                                page.get(), currentZoomFactor, currentRotation);
                        } else {
                            throw std::runtime_error(
                                "Failed to get page for rotation");
                        }
                    }
                } else {
                    // 更新连续视图中的所有页面
                    updateContinuousViewRotation();
                }
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
            }
#endif

            emit rotationChanged(currentRotation);
            setMessage(QString("页面已旋转到 %1 度").arg(currentRotation));
        } catch (const std::exception& e) {
            // 恢复旧的旋转状态
            currentRotation = oldRotation;
            setMessage(QString("旋转失败: %1").arg(e.what()));
            qDebug() << "Rotation failed:" << e.what();
        }
    }
}

void PDFViewer::updateContinuousViewRotation() {
    if (!document || currentViewMode != PDFViewMode::ContinuousScroll) {
        return;
    }

    int successCount = 0;
    int totalPages = continuousLayout->count() - 1;  // -1 因为最后一个是stretch

    // 更新连续视图中所有页面的旋转
    for (int i = 0; i < totalPages; ++i) {
        try {
            QLayoutItem* item = continuousLayout->itemAt(i);
            if (item && item->widget()) {
                PDFPageWidget* pageWidget =
                    qobject_cast<PDFPageWidget*>(item->widget());
                if (pageWidget && i < document->numPages()) {
                    std::unique_ptr<Poppler::Page> page(document->page(i));
                    if (page) {
                        // 阻止信号发出，避免循环
                        pageWidget->blockSignals(true);
                        pageWidget->setPage(page.get(), currentZoomFactor,
                                            currentRotation);
                        pageWidget->blockSignals(false);
                        successCount++;
                    } else {
                        qDebug() << "Failed to get page" << i << "for rotation";
                    }
                }
            }
        } catch (const std::exception& e) {
            qDebug() << "Error rotating page" << i << ":" << e.what();
        }
    }

    if (successCount < totalPages) {
        setMessage(QString("部分页面旋转失败 (%1/%2)")
                       .arg(successCount)
                       .arg(totalPages));
    }
}

// 搜索功能实现
void PDFViewer::showSearch() {
    if (searchWidget) {
        searchWidget->setVisible(true);
        searchWidget->focusSearchInput();
        searchWidget->setDocument(document);
    }
}

void PDFViewer::hideSearch() {
    if (searchWidget) {
        searchWidget->setVisible(false);
        searchWidget->clearSearch();
    }
}

void PDFViewer::toggleSearch() {
    if (searchWidget) {
        if (searchWidget->isVisible()) {
            hideSearch();
        } else {
            showSearch();
        }
    }
}

void PDFViewer::findNext() {
    if (searchWidget && searchWidget->isVisible()) {
        searchWidget->nextResult();
    }
}

void PDFViewer::findPrevious() {
    if (searchWidget && searchWidget->isVisible()) {
        searchWidget->previousResult();
    }
}

void PDFViewer::clearSearch() {
    if (searchWidget) {
        searchWidget->clearSearch();
    }
}

// 搜索相关槽函数
void PDFViewer::onSearchRequested(const QString& query,
                                  const SearchOptions& options) {
    // Clear previous search highlights
    clearSearchHighlights();

    if (!query.isEmpty() && document) {
        // The search is handled by SearchWidget, but we prepare for
        // highlighting
        setMessage(QString("搜索: %1").arg(query));
    }
}

void PDFViewer::onSearchResultSelected(const SearchResult& result) {
    // 当搜索结果被选中时，导航到对应页面并高亮
    if (result.pageNumber >= 0) {
        goToPage(result.pageNumber);
        highlightCurrentSearchResult(result);
    }
}

void PDFViewer::onNavigateToSearchResult(int pageNumber, const QRectF& rect) {
    // 导航到搜索结果位置并应用高亮
    if (pageNumber >= 0 && pageNumber < (document ? document->numPages() : 0)) {
        goToPage(pageNumber);

        // Apply highlighting to the current page
        updateSearchHighlightsForCurrentPage();

        setMessage(QString("已导航到第 %1 页的搜索结果").arg(pageNumber + 1));
    }
}

// Search highlighting implementation
void PDFViewer::setSearchResults(const QList<SearchResult>& results) {
    m_allSearchResults = results;
    updateSearchHighlightsForCurrentPage();
}

void PDFViewer::clearSearchHighlights() {
    m_allSearchResults.clear();
    m_currentSearchResultIndex = -1;

    // Clear highlights from current page widget
    if (currentViewMode == PDFViewMode::SinglePage && singlePageWidget) {
        singlePageWidget->clearSearchHighlights();
    } else if (currentViewMode == PDFViewMode::ContinuousScroll) {
        // Clear highlights from all visible page widgets in continuous mode
        for (int i = 0; i < continuousLayout->count() - 1; ++i) {
            QLayoutItem* item = continuousLayout->itemAt(i);
            if (item && item->widget()) {
                PDFPageWidget* pageWidget =
                    qobject_cast<PDFPageWidget*>(item->widget());
                if (pageWidget) {
                    pageWidget->clearSearchHighlights();
                }
            }
        }
    }
}

void PDFViewer::highlightCurrentSearchResult(const SearchResult& result) {
    m_currentSearchResultIndex = findSearchResultIndex(result);
    updateSearchHighlightsForCurrentPage();
}

void PDFViewer::updateSearchHighlightsForCurrentPage() {
    if (m_allSearchResults.isEmpty()) {
        return;
    }

    if (currentViewMode == PDFViewMode::SinglePage && singlePageWidget) {
        // Filter results for current page in single-page mode
        QList<SearchResult> currentPageResults;
        for (int i = 0; i < m_allSearchResults.size(); ++i) {
            SearchResult result = m_allSearchResults[i];
            if (result.pageNumber == currentPageNumber) {
                // Mark current result if this is the selected one
                result.isCurrentResult = (i == m_currentSearchResultIndex);
                currentPageResults.append(result);
            }
        }
        singlePageWidget->setSearchResults(currentPageResults);

    } else if (currentViewMode == PDFViewMode::ContinuousScroll) {
        // Apply highlights to all visible pages in continuous mode
        updateAllPagesSearchHighlights();
    }
}

void PDFViewer::updateAllPagesSearchHighlights() {
    if (m_allSearchResults.isEmpty() ||
        currentViewMode != PDFViewMode::ContinuousScroll) {
        return;
    }

    // Group results by page number
    QHash<int, QList<SearchResult>> resultsByPage;
    for (int i = 0; i < m_allSearchResults.size(); ++i) {
        SearchResult result = m_allSearchResults[i];
        result.isCurrentResult = (i == m_currentSearchResultIndex);
        resultsByPage[result.pageNumber].append(result);
    }

    // Apply highlights to each page widget
    for (int pageNum = 0; pageNum < continuousLayout->count() - 1; ++pageNum) {
        QLayoutItem* item = continuousLayout->itemAt(pageNum);
        if (item && item->widget()) {
            PDFPageWidget* pageWidget =
                qobject_cast<PDFPageWidget*>(item->widget());
            if (pageWidget) {
                if (resultsByPage.contains(pageNum)) {
                    pageWidget->setSearchResults(resultsByPage[pageNum]);
                } else {
                    pageWidget->clearSearchHighlights();
                }
            }
        }
    }
}

int PDFViewer::findSearchResultIndex(const SearchResult& target) {
    for (int i = 0; i < m_allSearchResults.size(); ++i) {
        const SearchResult& result = m_allSearchResults[i];
        if (result.pageNumber == target.pageNumber &&
            result.startIndex == target.startIndex &&
            result.length == target.length) {
            return i;
        }
    }
    return -1;
}

// 书签功能实现
void PDFViewer::addBookmark() {
    if (document && currentPageNumber >= 0) {
        addBookmarkForPage(currentPageNumber);
    }
}

void PDFViewer::addBookmarkForPage(int pageNumber) {
    if (!document || pageNumber < 0 || pageNumber >= document->numPages()) {
        setMessage("无法添加书签：页面无效");
        return;
    }

    // 发出书签请求信号，让上层组件处理
    emit bookmarkRequested(pageNumber);
    setMessage(QString("已为第 %1 页添加书签").arg(pageNumber + 1));
}

void PDFViewer::removeBookmark() {
    if (document && currentPageNumber >= 0) {
        // 这里需要与BookmarkWidget集成来实际删除书签
        // 目前只是发出信号
        setMessage(QString("已移除第 %1 页的书签").arg(currentPageNumber + 1));
    }
}

void PDFViewer::toggleBookmark() {
    if (hasBookmarkForCurrentPage()) {
        removeBookmark();
    } else {
        addBookmark();
    }
}

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
void PDFViewer::setQGraphicsRenderingEnabled(bool enabled) {
    if (useQGraphicsViewer == enabled) {
        return;  // No change needed
    }

    useQGraphicsViewer = enabled;

    if (enabled) {
        // Create QGraphics viewer if not exists
        if (!qgraphicsViewer) {
            try {
                qgraphicsViewer = new QGraphicsPDFViewer(this);
            } catch (...) {
                // If QGraphics viewer creation fails, fall back to traditional
                // mode
                useQGraphicsViewer = false;
                return;
            }

            // Connect signals
            connect(qgraphicsViewer, &QGraphicsPDFViewer::currentPageChanged,
                    this, [this](int page) {
                        currentPageNumber = page;
                        emit pageChanged(page);
                    });

            connect(qgraphicsViewer, &QGraphicsPDFViewer::zoomChanged, this,
                    [this](double zoom) {
                        currentZoomFactor = zoom;
                        emit zoomChanged(zoom);
                    });

            connect(qgraphicsViewer, &QGraphicsPDFViewer::rotationChanged, this,
                    [this](int rotation) {
                        currentRotation = rotation;
                        emit rotationChanged(rotation);
                    });

            connect(qgraphicsViewer, &QGraphicsPDFViewer::documentChanged, this,
                    &PDFViewer::documentChanged);
        }

        // Hide traditional viewer components and show QGraphics viewer
        if (singlePageWidget)
            singlePageWidget->hide();
        if (continuousScrollArea)
            continuousScrollArea->hide();

        if (qgraphicsViewer) {
            qgraphicsViewer->show();
            if (document) {
                qgraphicsViewer->setDocument(document);
                qgraphicsViewer->goToPage(currentPageNumber);
                qgraphicsViewer->setZoom(currentZoomFactor);
                qgraphicsViewer->setRotation(currentRotation);
            }
        }

        // Update layout to include QGraphics viewer
        if (layout() && qgraphicsViewer->parent() != this) {
            // Only add to layout if not already added
            QVBoxLayout* vLayout = qobject_cast<QVBoxLayout*>(layout());
            if (vLayout) {
                vLayout->addWidget(qgraphicsViewer,
                                   1);  // Give it stretch factor
            }
        }

    } else {
        // Hide QGraphics viewer and show traditional components
        if (qgraphicsViewer) {
            qgraphicsViewer->hide();
        }

        if (singlePageWidget)
            singlePageWidget->show();
        if (continuousScrollArea &&
            currentViewMode == PDFViewMode::ContinuousScroll) {
            continuousScrollArea->show();
        }

        // Update traditional viewer with current state
        updatePageDisplay();
    }
}

bool PDFViewer::isQGraphicsRenderingEnabled() const {
    return useQGraphicsViewer;
}

void PDFViewer::setQGraphicsHighQualityRendering(bool enabled) {
    if (qgraphicsViewer) {
        qgraphicsViewer->setHighQualityRendering(enabled);
    }
}

void PDFViewer::setQGraphicsViewMode(int mode) {
    if (qgraphicsViewer) {
        QGraphicsPDFViewer::ViewMode viewMode =
            static_cast<QGraphicsPDFViewer::ViewMode>(mode);
        qgraphicsViewer->setViewMode(viewMode);
    }
}
#endif

bool PDFViewer::hasBookmarkForCurrentPage() const {
    // 这里需要与BookmarkWidget集成来检查书签状态
    // 目前返回false作为占位符
    return false;
}
