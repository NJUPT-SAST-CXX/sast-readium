#include "SkeletonWidget.h"
#include <QHBoxLayout>
#include <QLinearGradient>
#include <QPainterPath>
#include <QVBoxLayout>
#include "../../managers/StyleManager.h"

// SkeletonWidget Implementation
SkeletonWidget::SkeletonWidget(Shape shape, QWidget* parent)
    : QWidget(parent),
      m_shape(shape),
      m_shimmerAnimation(nullptr),
      m_shimmerPosition(0.0),
      m_animationDuration(1500),
      m_cornerRadius(STYLE.radiusMD()),
      m_isAnimating(false) {
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumSize(50, 20);

    setupAnimation();
}

SkeletonWidget::~SkeletonWidget() {
    if (m_shimmerAnimation) {
        m_shimmerAnimation->stop();
    }
}

void SkeletonWidget::setupAnimation() {
    m_shimmerAnimation = new QPropertyAnimation(this, "shimmerPosition", this);
    m_shimmerAnimation->setDuration(m_animationDuration);
    m_shimmerAnimation->setStartValue(0.0);
    m_shimmerAnimation->setEndValue(1.0);
    m_shimmerAnimation->setEasingCurve(QEasingCurve::Linear);
    m_shimmerAnimation->setLoopCount(-1);  // Infinite loop
}

void SkeletonWidget::startAnimation() {
    if (!m_isAnimating && m_shimmerAnimation) {
        m_isAnimating = true;
        m_shimmerAnimation->start();
    }
}

void SkeletonWidget::stopAnimation() {
    if (m_isAnimating && m_shimmerAnimation) {
        m_isAnimating = false;
        m_shimmerAnimation->stop();
        m_shimmerPosition = 0.0;
        update();
    }
}

void SkeletonWidget::setShape(Shape shape) {
    if (m_shape != shape) {
        m_shape = shape;
        update();
    }
}

void SkeletonWidget::setAnimationDuration(int ms) {
    m_animationDuration = ms;
    if (m_shimmerAnimation) {
        bool wasRunning = m_isAnimating;
        if (wasRunning) {
            stopAnimation();
        }
        m_shimmerAnimation->setDuration(ms);
        if (wasRunning) {
            startAnimation();
        }
    }
}

void SkeletonWidget::setCornerRadius(int radius) {
    if (m_cornerRadius != radius) {
        m_cornerRadius = radius;
        update();
    }
}

void SkeletonWidget::setShimmerPosition(qreal position) {
    if (m_shimmerPosition != position) {
        m_shimmerPosition = position;
        update();
    }
}

void SkeletonWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    switch (m_shape) {
        case Shape::Rectangle:
            drawRectangle(painter);
            break;
        case Shape::Circle:
            drawCircle(painter);
            break;
        case Shape::TextLine:
            drawTextLine(painter);
            break;
        case Shape::Custom:
            // Override paintEvent in derived class
            break;
    }
}

void SkeletonWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    startAnimation();
}

void SkeletonWidget::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    stopAnimation();
}

void SkeletonWidget::drawRectangle(QPainter& painter) {
    QRect rect = this->rect();

    // Draw base color
    painter.setBrush(getBaseColor());
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect, m_cornerRadius, m_cornerRadius);

    // Draw shimmer effect
    if (m_isAnimating) {
        drawShimmer(painter, rect);
    }
}

void SkeletonWidget::drawCircle(QPainter& painter) {
    QRect rect = this->rect();
    int size = qMin(rect.width(), rect.height());
    QRect circleRect((rect.width() - size) / 2, (rect.height() - size) / 2,
                     size, size);

    // Draw base color
    painter.setBrush(getBaseColor());
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(circleRect);

    // Draw shimmer effect
    if (m_isAnimating) {
        QPainterPath clipPath;
        clipPath.addEllipse(circleRect);
        painter.setClipPath(clipPath);
        drawShimmer(painter, circleRect);
    }
}

void SkeletonWidget::drawTextLine(QPainter& painter) {
    QRect rect = this->rect();
    int lineHeight = qMin(rect.height(), 16);  // Max 16px height for text lines
    QRect lineRect(rect.x(), rect.y() + (rect.height() - lineHeight) / 2,
                   rect.width(), lineHeight);

    // Draw base color
    painter.setBrush(getBaseColor());
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(lineRect, STYLE.radiusSM(), STYLE.radiusSM());

    // Draw shimmer effect
    if (m_isAnimating) {
        drawShimmer(painter, lineRect);
    }
}

void SkeletonWidget::drawShimmer(QPainter& painter, const QRect& rect) {
    // Create shimmer gradient
    QLinearGradient gradient;
    gradient.setStart(rect.left(), rect.center().y());
    gradient.setFinalStop(rect.right(), rect.center().y());

    // Calculate shimmer position
    qreal shimmerWidth = 0.3;  // 30% of width
    qreal shimmerStart = m_shimmerPosition - shimmerWidth / 2;
    qreal shimmerEnd = m_shimmerPosition + shimmerWidth / 2;

    QColor baseColor = getBaseColor();
    QColor shimmerColor = getShimmerColor();

    // Create gradient stops
    if (shimmerStart > 0) {
        gradient.setColorAt(0, Qt::transparent);
    }
    if (shimmerStart > 0 && shimmerStart < 1) {
        gradient.setColorAt(qMax(0.0, shimmerStart), Qt::transparent);
    }
    gradient.setColorAt(qBound(0.0, m_shimmerPosition, 1.0), shimmerColor);
    if (shimmerEnd < 1 && shimmerEnd > 0) {
        gradient.setColorAt(qMin(1.0, shimmerEnd), Qt::transparent);
    }
    if (shimmerEnd < 1) {
        gradient.setColorAt(1, Qt::transparent);
    }

    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    if (m_shape == Shape::Circle) {
        painter.drawEllipse(rect);
    } else {
        painter.drawRoundedRect(rect, m_cornerRadius, m_cornerRadius);
    }
}

QColor SkeletonWidget::getBaseColor() const {
    if (STYLE.currentTheme() == Theme::Light) {
        return QColor(240, 240, 240);  // Light gray
    } else {
        return QColor(60, 60, 60);  // Dark gray
    }
}

QColor SkeletonWidget::getShimmerColor() const {
    if (STYLE.currentTheme() == Theme::Light) {
        return QColor(255, 255, 255, 180);  // White with transparency
    } else {
        return QColor(100, 100, 100, 180);  // Lighter gray with transparency
    }
}

// DocumentSkeletonWidget Implementation
DocumentSkeletonWidget::DocumentSkeletonWidget(QWidget* parent)
    : QWidget(parent),
      m_headerSkeleton(nullptr),
      m_contentSkeleton1(nullptr),
      m_contentSkeleton2(nullptr),
      m_contentSkeleton3(nullptr) {
    setupLayout();
}

DocumentSkeletonWidget::~DocumentSkeletonWidget() = default;

void DocumentSkeletonWidget::setupLayout() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(STYLE.spacingMD(), STYLE.spacingMD(),
                               STYLE.spacingMD(), STYLE.spacingMD());
    layout->setSpacing(STYLE.spacingSM());

    // Header skeleton (title area)
    m_headerSkeleton = new SkeletonWidget(SkeletonWidget::Shape::Rectangle);
    m_headerSkeleton->setFixedHeight(40);
    layout->addWidget(m_headerSkeleton);

    layout->addSpacing(STYLE.spacingMD());

    // Content skeletons (text lines)
    m_contentSkeleton1 = new SkeletonWidget(SkeletonWidget::Shape::TextLine);
    m_contentSkeleton1->setFixedHeight(16);
    layout->addWidget(m_contentSkeleton1);

    m_contentSkeleton2 = new SkeletonWidget(SkeletonWidget::Shape::TextLine);
    m_contentSkeleton2->setFixedHeight(16);
    layout->addWidget(m_contentSkeleton2);

    m_contentSkeleton3 = new SkeletonWidget(SkeletonWidget::Shape::TextLine);
    m_contentSkeleton3->setFixedHeight(16);
    // Make last line shorter (80% width)
    m_contentSkeleton3->setMaximumWidth(INT_MAX * 0.8);
    layout->addWidget(m_contentSkeleton3);

    layout->addStretch();
}

void DocumentSkeletonWidget::startAnimation() {
    if (m_headerSkeleton)
        m_headerSkeleton->startAnimation();
    if (m_contentSkeleton1)
        m_contentSkeleton1->startAnimation();
    if (m_contentSkeleton2)
        m_contentSkeleton2->startAnimation();
    if (m_contentSkeleton3)
        m_contentSkeleton3->startAnimation();
}

void DocumentSkeletonWidget::stopAnimation() {
    if (m_headerSkeleton)
        m_headerSkeleton->stopAnimation();
    if (m_contentSkeleton1)
        m_contentSkeleton1->stopAnimation();
    if (m_contentSkeleton2)
        m_contentSkeleton2->stopAnimation();
    if (m_contentSkeleton3)
        m_contentSkeleton3->stopAnimation();
}

void DocumentSkeletonWidget::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
}

// ThumbnailSkeletonWidget Implementation
ThumbnailSkeletonWidget::ThumbnailSkeletonWidget(QWidget* parent)
    : QWidget(parent),
      m_thumbnailSkeleton(nullptr),
      m_pageNumberSkeleton(nullptr),
      m_thumbnailSize(120, 160) {
    setupLayout();
}

ThumbnailSkeletonWidget::~ThumbnailSkeletonWidget() = default;

void ThumbnailSkeletonWidget::setupLayout() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(STYLE.spacingXS(), STYLE.spacingXS(),
                               STYLE.spacingXS(), STYLE.spacingXS());
    layout->setSpacing(STYLE.spacingXS());

    // Thumbnail skeleton
    m_thumbnailSkeleton = new SkeletonWidget(SkeletonWidget::Shape::Rectangle);
    m_thumbnailSkeleton->setFixedSize(m_thumbnailSize);
    m_thumbnailSkeleton->setCornerRadius(STYLE.radiusSM());
    layout->addWidget(m_thumbnailSkeleton, 0, Qt::AlignCenter);

    // Page number skeleton
    m_pageNumberSkeleton = new SkeletonWidget(SkeletonWidget::Shape::TextLine);
    m_pageNumberSkeleton->setFixedSize(40, 12);
    layout->addWidget(m_pageNumberSkeleton, 0, Qt::AlignCenter);
}

void ThumbnailSkeletonWidget::startAnimation() {
    if (m_thumbnailSkeleton)
        m_thumbnailSkeleton->startAnimation();
    if (m_pageNumberSkeleton)
        m_pageNumberSkeleton->startAnimation();
}

void ThumbnailSkeletonWidget::stopAnimation() {
    if (m_thumbnailSkeleton)
        m_thumbnailSkeleton->stopAnimation();
    if (m_pageNumberSkeleton)
        m_pageNumberSkeleton->stopAnimation();
}

void ThumbnailSkeletonWidget::setThumbnailSize(const QSize& size) {
    m_thumbnailSize = size;
    if (m_thumbnailSkeleton) {
        m_thumbnailSkeleton->setFixedSize(size);
    }
}

void ThumbnailSkeletonWidget::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
}
