#include "ThumbnailWidget.h"
#include <QApplication>
#include <QColor>
#include <QContextMenuEvent>
#include <QDebug>
#include <QEnterEvent>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QRect>
#include <QSize>
#include <QStyleOption>
#include <cmath>
#include "../../managers/StyleManager.h"

// 常量定义
namespace {
constexpr double DEFAULT_SHADOW_OPACITY = 0.3;
constexpr int HOVER_ANIMATION_DURATION = 200;
constexpr int SELECTION_ANIMATION_DURATION = 300;
constexpr int LOADING_TIMER_INTERVAL = 50;  // 20 FPS
}  // namespace

ThumbnailWidget::ThumbnailWidget(int pageNumber, QWidget* parent)
    : QWidget(parent),
      m_pageNumber(pageNumber),
      m_state(State::Normal),
      m_thumbnailSize(DEFAULT_THUMBNAIL_WIDTH, DEFAULT_THUMBNAIL_HEIGHT),
      m_shadowOpacity(DEFAULT_SHADOW_OPACITY),
      m_borderOpacity(0.0),
      m_loadingAngle(0),
      m_hoverAnimation(nullptr),
      m_selectionAnimation(nullptr),
      m_shadowAnimation(nullptr),
      m_borderAnimation(nullptr),
      m_loadingTimer(nullptr),
      m_shadowEffect(nullptr) {
    setupUI();
    setupAnimations();
    setMouseTracking(true);
}

ThumbnailWidget::~ThumbnailWidget() {
    if (m_loadingTimer) {
        m_loadingTimer->stop();
    }
}

void ThumbnailWidget::setupUI() {
    setFixedSize(m_thumbnailSize.width() + 2 * MARGIN,
                 m_thumbnailSize.height() + PAGE_NUMBER_HEIGHT + 2 * MARGIN);

    // 设置阴影效果
    m_shadowEffect = new QGraphicsDropShadowEffect(this);
    m_shadowEffect->setBlurRadius(SHADOW_BLUR_RADIUS);
    m_shadowEffect->setOffset(SHADOW_OFFSET, SHADOW_OFFSET);
    m_shadowEffect->setColor(getShadowColor());
    setGraphicsEffect(m_shadowEffect);

    updateShadowEffect();

    // Connect to theme changes
    connect(&STYLE, &StyleManager::themeChanged, this,
            &ThumbnailWidget::applyTheme);
}

void ThumbnailWidget::setupAnimations() {
    // 悬停动画
    m_hoverAnimation = new QPropertyAnimation(this, "borderOpacity", this);
    m_hoverAnimation->setDuration(HOVER_ANIMATION_DURATION);
    m_hoverAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_hoverAnimation, &QPropertyAnimation::finished, this,
            &ThumbnailWidget::onHoverAnimationFinished);

    // 选中动画
    m_selectionAnimation = new QPropertyAnimation(this, "shadowOpacity", this);
    m_selectionAnimation->setDuration(SELECTION_ANIMATION_DURATION);
    m_selectionAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_selectionAnimation, &QPropertyAnimation::finished, this,
            &ThumbnailWidget::onSelectionAnimationFinished);

    // 加载动画定时器
    m_loadingTimer = new QTimer(this);
    m_loadingTimer->setInterval(LOADING_TIMER_INTERVAL);  // 20 FPS
    connect(m_loadingTimer, &QTimer::timeout, this,
            &ThumbnailWidget::updateLoadingAnimation);
}

void ThumbnailWidget::setPageNumber(int pageNumber) {
    if (m_pageNumber != pageNumber) {
        m_pageNumber = pageNumber;
        update();
    }
}

void ThumbnailWidget::setPixmap(const QPixmap& pixmap) {
    m_pixmap = pixmap;
    if (!pixmap.isNull() && m_state == State::Loading) {
        setState(State::Normal);
    }
    update();
}

void ThumbnailWidget::setState(State state) {
    if (m_state == state) {
        return;
    }

    State oldState = m_state;
    m_state = state;

    // 处理状态变化
    switch (state) {
        case State::Normal:
            if (m_loadingTimer->isActive()) {
                m_loadingTimer->stop();
            }
            if (oldState == State::Selected) {
                m_selectionAnimation->setStartValue(m_shadowOpacity);
                m_selectionAnimation->setEndValue(DEFAULT_SHADOW_OPACITY);
                m_selectionAnimation->start();
            }
            break;

        case State::Hovered:
            m_hoverAnimation->setStartValue(m_borderOpacity);
            m_hoverAnimation->setEndValue(1.0);
            m_hoverAnimation->start();
            break;

        case State::Selected:
            m_selectionAnimation->setStartValue(m_shadowOpacity);
            m_selectionAnimation->setEndValue(0.8);
            m_selectionAnimation->start();
            break;

        case State::Loading:
            m_loadingAngle = 0;
            m_loadingTimer->start();
            break;

        case State::Error:
            if (m_loadingTimer->isActive()) {
                m_loadingTimer->stop();
            }
            break;
    }

    update();
}

void ThumbnailWidget::setThumbnailSize(const QSize& size) {
    if (m_thumbnailSize != size) {
        m_thumbnailSize = size;
        setFixedSize(size.width() + 2 * MARGIN,
                     size.height() + PAGE_NUMBER_HEIGHT + 2 * MARGIN);
        update();
    }
}

void ThumbnailWidget::setShadowOpacity(qreal opacity) {
    if (qAbs(m_shadowOpacity - opacity) > 0.001) {
        m_shadowOpacity = opacity;
        updateShadowEffect();
        update();
    }
}

void ThumbnailWidget::setBorderOpacity(qreal opacity) {
    if (qAbs(m_borderOpacity - opacity) > 0.001) {
        m_borderOpacity = opacity;
        update();
    }
}

void ThumbnailWidget::setLoading(bool loading) {
    setState(loading ? State::Loading : State::Normal);
}

void ThumbnailWidget::setError(const QString& errorMessage) {
    m_errorMessage = errorMessage;
    setState(State::Error);
}

void ThumbnailWidget::updateShadowEffect() {
    if (m_shadowEffect) {
        QColor shadowColor = getShadowColor();
        shadowColor.setAlphaF(m_shadowOpacity);
        m_shadowEffect->setColor(shadowColor);
    }
}

QSize ThumbnailWidget::sizeHint() const {
    return QSize(m_thumbnailSize.width() + 2 * MARGIN,
                 m_thumbnailSize.height() + PAGE_NUMBER_HEIGHT + 2 * MARGIN);
}

QSize ThumbnailWidget::minimumSizeHint() const { return sizeHint(); }

QRect ThumbnailWidget::getThumbnailRect() const {
    return QRect(MARGIN, MARGIN, m_thumbnailSize.width(),
                 m_thumbnailSize.height());
}

QRect ThumbnailWidget::getPageNumberRect() const {
    QRect thumbRect = getThumbnailRect();
    return QRect(thumbRect.left(), thumbRect.bottom() + 4, thumbRect.width(),
                 PAGE_NUMBER_HEIGHT - 4);
}

void ThumbnailWidget::onHoverAnimationFinished() {
    // 悬停动画完成处理
}

void ThumbnailWidget::onSelectionAnimationFinished() {
    // 选中动画完成处理
}

void ThumbnailWidget::updateLoadingAnimation() {
    m_loadingAngle = (m_loadingAngle + 15) % 360;
    update();
}

void ThumbnailWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect thumbRect = getThumbnailRect();
    QRect pageNumRect = getPageNumberRect();

    // 绘制缩略图内容
    drawThumbnail(painter, thumbRect);

    // 绘制边框
    drawBorder(painter, thumbRect);

    // 绘制页码
    drawPageNumber(painter, pageNumRect);

    // 根据状态绘制额外内容
    if (m_state == State::Loading) {
        drawLoadingIndicator(painter, thumbRect);
    } else if (m_state == State::Error) {
        drawErrorIndicator(painter, thumbRect);
    }
}

void ThumbnailWidget::drawThumbnail(QPainter& painter, const QRect& rect) {
    // 创建圆角路径
    QPainterPath path;
    path.addRoundedRect(rect, BORDER_RADIUS, BORDER_RADIUS);
    painter.setClipPath(path);

    if (!m_pixmap.isNull()) {
        // 绘制缩略图
        QPixmap scaledPixmap = m_pixmap.scaled(rect.size(), Qt::KeepAspectRatio,
                                               Qt::SmoothTransformation);

        // 居中绘制
        QRect targetRect = rect;
        if (scaledPixmap.size() != rect.size()) {
            int x = rect.x() + (rect.width() - scaledPixmap.width()) / 2;
            int y = rect.y() + (rect.height() - scaledPixmap.height()) / 2;
            targetRect =
                QRect(x, y, scaledPixmap.width(), scaledPixmap.height());
        }

        painter.drawPixmap(targetRect, scaledPixmap);
    } else {
        // 绘制占位符
        painter.fillRect(rect, STYLE.surfaceAltColor());

        // 绘制占位符图标
        painter.setPen(STYLE.textSecondaryColor());
        QFont font = painter.font();
        font.setPixelSize(24);
        painter.setFont(font);
        painter.drawText(rect, Qt::AlignCenter, "📄");
    }

    painter.setClipping(false);
}

void ThumbnailWidget::drawBorder(QPainter& painter, const QRect& rect) {
    if (m_borderOpacity > 0.001) {
        QColor borderColor;
        switch (m_state) {
            case State::Hovered:
                borderColor = getBorderColorHovered();
                break;
            case State::Selected:
                borderColor = getBorderColorSelected();
                break;
            default:
                borderColor = getBorderColorNormal();
                break;
        }

        borderColor.setAlphaF(m_borderOpacity);
        painter.setPen(QPen(borderColor, BORDER_WIDTH));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(
            rect.adjusted(BORDER_WIDTH / 2, BORDER_WIDTH / 2, -BORDER_WIDTH / 2,
                          -BORDER_WIDTH / 2),
            BORDER_RADIUS, BORDER_RADIUS);
    }
}

void ThumbnailWidget::drawPageNumber(QPainter& painter, const QRect& rect) {
    if (rect.height() <= 0) {
        return;
    }

    // 绘制页码背景
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect, 4, 4);
    painter.fillPath(bgPath, getPageNumberBgColor());

    // 绘制页码文字
    painter.setPen(getPageNumberTextColor());
    QFont font = painter.font();
    font.setPixelSize(11);
    font.setBold(true);
    painter.setFont(font);

    QString pageText = QString::number(m_pageNumber + 1);  // 页码从1开始显示
    painter.drawText(rect, Qt::AlignCenter, pageText);
}

void ThumbnailWidget::drawLoadingIndicator(QPainter& painter,
                                           const QRect& rect) {
    // 绘制半透明遮罩
    QColor overlayColor = STYLE.backgroundColor();
    overlayColor.setAlpha(200);
    painter.fillRect(rect, overlayColor);

    // 绘制旋转的加载指示器
    QRect spinnerRect(rect.center().x() - LOADING_SPINNER_SIZE / 2,
                      rect.center().y() - LOADING_SPINNER_SIZE / 2,
                      LOADING_SPINNER_SIZE, LOADING_SPINNER_SIZE);

    painter.save();
    painter.translate(spinnerRect.center());
    painter.rotate(m_loadingAngle);

    painter.setPen(QPen(getLoadingColor(), 3, Qt::SolidLine, Qt::RoundCap));
    painter.drawArc(-LOADING_SPINNER_SIZE / 2, -LOADING_SPINNER_SIZE / 2,
                    LOADING_SPINNER_SIZE, LOADING_SPINNER_SIZE, 0,
                    270 * 16);  // 3/4 圆弧

    painter.restore();
}

void ThumbnailWidget::drawErrorIndicator(QPainter& painter, const QRect& rect) {
    // 绘制半透明遮罩
    QColor overlayColor = STYLE.backgroundColor();
    overlayColor.setAlpha(200);
    painter.fillRect(rect, overlayColor);

    // 绘制错误图标
    painter.setPen(QPen(getErrorColor(), 2));
    painter.setBrush(Qt::NoBrush);

    QRect iconRect(rect.center().x() - 12, rect.center().y() - 12, 24, 24);
    painter.drawEllipse(iconRect);

    // 绘制感叹号
    painter.setPen(QPen(getErrorColor(), 3, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(iconRect.center().x(), iconRect.top() + 6,
                     iconRect.center().x(), iconRect.center().y() + 2);
    painter.drawPoint(iconRect.center().x(), iconRect.bottom() - 4);
}

void ThumbnailWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_pageNumber);
    }
    QWidget::mousePressEvent(event);
}

void ThumbnailWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked(m_pageNumber);
    }
    QWidget::mouseDoubleClickEvent(event);
}

void ThumbnailWidget::enterEvent(QEnterEvent* event) {
    if (m_state == State::Normal) {
        setState(State::Hovered);
        emit hoverEntered(m_pageNumber);
    }
    QWidget::enterEvent(event);
}

void ThumbnailWidget::leaveEvent(QEvent* event) {
    if (m_state == State::Hovered) {
        setState(State::Normal);
        m_hoverAnimation->setStartValue(m_borderOpacity);
        m_hoverAnimation->setEndValue(0.0);
        m_hoverAnimation->start();
        emit hoverLeft(m_pageNumber);
    }
    QWidget::leaveEvent(event);
}

void ThumbnailWidget::contextMenuEvent(QContextMenuEvent* event) {
    emit rightClicked(m_pageNumber, event->globalPos());
    QWidget::contextMenuEvent(event);
}

void ThumbnailWidget::applyTheme() {
    // Update shadow effect with new theme colors
    updateShadowEffect();
    // Trigger repaint to update all colors
    update();
}

// Theme-aware color getters
QColor ThumbnailWidget::getBorderColorNormal() const {
    return STYLE.borderColor();
}

QColor ThumbnailWidget::getBorderColorHovered() const {
    return STYLE.accentColor();
}

QColor ThumbnailWidget::getBorderColorSelected() const {
    return STYLE.primaryColor();
}

QColor ThumbnailWidget::getShadowColor() const {
    // Semi-transparent shadow based on theme
    if (STYLE.currentTheme() == Theme::Dark) {
        return QColor(0, 0, 0, 60);  // Darker shadow for dark theme
    }
    return QColor(0, 0, 0, 40);  // Lighter shadow for light theme
}

QColor ThumbnailWidget::getPageNumberBgColor() const {
    // Semi-transparent overlay
    if (STYLE.currentTheme() == Theme::Dark) {
        return QColor(0, 0, 0, 200);
    }
    return QColor(0, 0, 0, 180);
}

QColor ThumbnailWidget::getPageNumberTextColor() const {
    // Always white for good contrast on dark background
    return QColor(255, 255, 255);
}

QColor ThumbnailWidget::getLoadingColor() const { return STYLE.primaryColor(); }

QColor ThumbnailWidget::getErrorColor() const { return STYLE.errorColor(); }
