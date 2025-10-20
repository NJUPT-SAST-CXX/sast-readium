#include "EnhancedFocusIndicator.h"
#include <QApplication>
#include <QGraphicsBlurEffect>
#include <QPainterPath>
#include <QTimer>
#include "../../managers/StyleManager.h"

// EnhancedFocusIndicator Implementation
EnhancedFocusIndicator::EnhancedFocusIndicator(QWidget* parent)
    : QWidget(parent),
      m_targetWidget(nullptr),
      m_style(Style::Glow),
      m_focusColor(STYLE.primaryColor()),
      m_borderThickness(3),
      m_animationDuration(STYLE.animationNormal()),
      m_showAnimation(nullptr),
      m_hideAnimation(nullptr),
      m_pulseAnimation(nullptr),
      m_borderOpacity(0.0),
      m_borderWidth(0),
      m_animationPhase(0.0),
      m_isVisible(false) {
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint |
                   Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_ShowWithoutActivating);

    setupAnimations();
}

EnhancedFocusIndicator::~EnhancedFocusIndicator() {
    if (m_targetWidget) {
        m_targetWidget->removeEventFilter(this);
    }
}

void EnhancedFocusIndicator::setupAnimations() {
    // Show animation (fade in + expand)
    m_showAnimation = new QPropertyAnimation(this, "borderOpacity", this);
    m_showAnimation->setDuration(m_animationDuration);
    m_showAnimation->setStartValue(0.0);
    m_showAnimation->setEndValue(1.0);
    m_showAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // Hide animation (fade out + contract)
    m_hideAnimation = new QPropertyAnimation(this, "borderOpacity", this);
    m_hideAnimation->setDuration(m_animationDuration);
    m_hideAnimation->setStartValue(1.0);
    m_hideAnimation->setEndValue(0.0);
    m_hideAnimation->setEasingCurve(QEasingCurve::InCubic);

    connect(m_hideAnimation, &QPropertyAnimation::finished, this,
            &QWidget::hide);

    // Pulse animation (for glow effect)
    m_pulseAnimation = new QPropertyAnimation(this, "borderWidth", this);
    m_pulseAnimation->setDuration(1000);
    m_pulseAnimation->setStartValue(m_borderThickness);
    m_pulseAnimation->setEndValue(m_borderThickness + 2);
    m_pulseAnimation->setEasingCurve(QEasingCurve::InOutSine);
    m_pulseAnimation->setLoopCount(-1);  // Infinite
}

void EnhancedFocusIndicator::setTargetWidget(QWidget* widget) {
    if (m_targetWidget == widget) {
        return;
    }

    // Remove event filter from old target
    if (m_targetWidget) {
        m_targetWidget->removeEventFilter(this);
    }

    m_targetWidget = widget;

    // Install event filter on new target
    if (m_targetWidget) {
        m_targetWidget->installEventFilter(this);
        updatePosition();
    }
}

void EnhancedFocusIndicator::setStyle(Style style) {
    if (m_style != style) {
        m_style = style;
        update();
    }
}

void EnhancedFocusIndicator::setFocusColor(const QColor& color) {
    if (m_focusColor != color) {
        m_focusColor = color;
        update();
    }
}

void EnhancedFocusIndicator::setBorderThickness(int thickness) {
    if (m_borderThickness != thickness) {
        m_borderThickness = thickness;
        update();
    }
}

void EnhancedFocusIndicator::setAnimationDuration(int ms) {
    m_animationDuration = ms;
    if (m_showAnimation) {
        m_showAnimation->setDuration(ms);
    }
    if (m_hideAnimation) {
        m_hideAnimation->setDuration(ms);
    }
}

void EnhancedFocusIndicator::setBorderOpacity(qreal opacity) {
    if (m_borderOpacity != opacity) {
        m_borderOpacity = opacity;
        update();
    }
}

void EnhancedFocusIndicator::setBorderWidth(int width) {
    if (m_borderWidth != width) {
        m_borderWidth = width;
        update();
    }
}

void EnhancedFocusIndicator::showIndicator() {
    if (m_isVisible || !m_targetWidget) {
        return;
    }

    m_isVisible = true;
    updatePosition();
    show();
    raise();

    animateShow();
}

void EnhancedFocusIndicator::hideIndicator() {
    if (!m_isVisible) {
        return;
    }

    m_isVisible = false;
    animateHide();
}

void EnhancedFocusIndicator::updatePosition() {
    if (!m_targetWidget || !m_targetWidget->isVisible()) {
        return;
    }

    // Get target widget's global geometry
    QRect targetRect = m_targetWidget->rect();
    QPoint globalPos = m_targetWidget->mapToGlobal(targetRect.topLeft());

    // Add padding around the target
    int padding = m_borderThickness + 2;
    QRect indicatorRect =
        targetRect.adjusted(-padding, -padding, padding, padding);

    setGeometry(globalPos.x() - padding, globalPos.y() - padding,
                indicatorRect.width(), indicatorRect.height());
}

void EnhancedFocusIndicator::animateShow() {
    if (m_hideAnimation &&
        m_hideAnimation->state() == QPropertyAnimation::Running) {
        m_hideAnimation->stop();
    }

    m_borderWidth = m_borderThickness;
    m_showAnimation->start();

    // Start pulse animation for glow style
    if (m_style == Style::Glow && m_pulseAnimation) {
        m_pulseAnimation->start();
    }
}

void EnhancedFocusIndicator::animateHide() {
    if (m_showAnimation &&
        m_showAnimation->state() == QPropertyAnimation::Running) {
        m_showAnimation->stop();
    }

    if (m_pulseAnimation &&
        m_pulseAnimation->state() == QPropertyAnimation::Running) {
        m_pulseAnimation->stop();
    }

    m_hideAnimation->start();
}

void EnhancedFocusIndicator::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    if (!m_targetWidget || m_borderOpacity <= 0.0) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    switch (m_style) {
        case Style::Solid:
            drawSolidBorder(painter);
            break;
        case Style::Dashed:
            drawDashedBorder(painter);
            break;
        case Style::Glow:
            drawGlowBorder(painter);
            break;
        case Style::Animated:
            drawAnimatedBorder(painter);
            break;
    }
}

void EnhancedFocusIndicator::drawSolidBorder(QPainter& painter) {
    QColor color = m_focusColor;
    color.setAlphaF(static_cast<float>(m_borderOpacity));

    QPen pen(color, m_borderThickness);
    pen.setJoinStyle(Qt::MiterJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    QRect rect =
        this->rect().adjusted(m_borderThickness / 2, m_borderThickness / 2,
                              -m_borderThickness / 2, -m_borderThickness / 2);
    painter.drawRoundedRect(rect, STYLE.radiusSM(), STYLE.radiusSM());
}

void EnhancedFocusIndicator::drawDashedBorder(QPainter& painter) {
    QColor color = m_focusColor;
    color.setAlphaF(static_cast<float>(m_borderOpacity));

    QPen pen(color, m_borderThickness);
    pen.setStyle(Qt::DashLine);
    pen.setDashPattern({4, 4});
    pen.setJoinStyle(Qt::MiterJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    QRect rect =
        this->rect().adjusted(m_borderThickness / 2, m_borderThickness / 2,
                              -m_borderThickness / 2, -m_borderThickness / 2);
    painter.drawRoundedRect(rect, STYLE.radiusSM(), STYLE.radiusSM());
}

void EnhancedFocusIndicator::drawGlowBorder(QPainter& painter) {
    QColor color = m_focusColor;

    // Draw multiple layers for glow effect
    int layers = 3;
    for (int i = layers; i > 0; --i) {
        qreal layerOpacity = m_borderOpacity * (1.0 - (i - 1) * 0.3);
        color.setAlphaF(layerOpacity);

        int layerWidth = m_borderWidth + (i - 1) * 2;
        QPen pen(color, layerWidth);
        pen.setJoinStyle(Qt::MiterJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        int offset = layerWidth / 2;
        QRect rect = this->rect().adjusted(offset, offset, -offset, -offset);
        painter.drawRoundedRect(rect, STYLE.radiusSM(), STYLE.radiusSM());
    }
}

void EnhancedFocusIndicator::drawAnimatedBorder(QPainter& painter) {
    QColor color = m_focusColor;
    color.setAlphaF(static_cast<float>(m_borderOpacity));

    // Animated dashed pattern
    QPen pen(color, m_borderThickness);
    pen.setStyle(Qt::CustomDashLine);
    pen.setDashPattern({6, 4});
    pen.setDashOffset(m_animationPhase * 10);  // Animate dash offset
    pen.setJoinStyle(Qt::MiterJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    QRect rect =
        this->rect().adjusted(m_borderThickness / 2, m_borderThickness / 2,
                              -m_borderThickness / 2, -m_borderThickness / 2);
    painter.drawRoundedRect(rect, STYLE.radiusSM(), STYLE.radiusSM());

    // Update animation phase
    m_animationPhase += 0.1;
    if (m_animationPhase > 1.0) {
        m_animationPhase = 0.0;
    }

    // Schedule repaint for animation
    if (m_isVisible) {
        QTimer::singleShot(16, this, [this]() { update(); });  // ~60 FPS
    }
}

bool EnhancedFocusIndicator::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_targetWidget) {
        switch (event->type()) {
            case QEvent::Move:
            case QEvent::Resize:
            case QEvent::Show:
                updatePosition();
                break;
            case QEvent::Hide:
                hideIndicator();
                break;
            default:
                break;
        }
    }
    return QWidget::eventFilter(obj, event);
}

// FocusManager Implementation
FocusManager& FocusManager::instance() {
    static FocusManager instance;
    return instance;
}

FocusManager::FocusManager()
    : QObject(nullptr),
      m_indicator(nullptr),
      m_currentFocusWidget(nullptr),
      m_enabled(true),
      m_installed(false),
      m_style(EnhancedFocusIndicator::Style::Glow),
      m_color(STYLE.primaryColor()),
      m_thickness(3) {}

FocusManager::~FocusManager() {
    uninstallFromApplication();
    if (m_indicator) {
        delete m_indicator;
    }
}

void FocusManager::setEnabled(bool enabled) {
    if (m_enabled != enabled) {
        m_enabled = enabled;
        if (!enabled && m_indicator) {
            m_indicator->hideIndicator();
        }
    }
}

void FocusManager::setIndicatorStyle(EnhancedFocusIndicator::Style style) {
    m_style = style;
    if (m_indicator) {
        m_indicator->setStyle(style);
    }
}

void FocusManager::setIndicatorColor(const QColor& color) {
    m_color = color;
    if (m_indicator) {
        m_indicator->setFocusColor(color);
    }
}

void FocusManager::setIndicatorThickness(int thickness) {
    m_thickness = thickness;
    if (m_indicator) {
        m_indicator->setBorderThickness(thickness);
    }
}

void FocusManager::installOnApplication() {
    if (m_installed) {
        return;
    }

    QApplication* app = qobject_cast<QApplication*>(QApplication::instance());
    if (app) {
        app->installEventFilter(this);
        connect(app, &QApplication::focusChanged, this,
                &FocusManager::onFocusChanged);
        m_installed = true;
    }
}

void FocusManager::uninstallFromApplication() {
    if (!m_installed) {
        return;
    }

    QApplication* app = qobject_cast<QApplication*>(QApplication::instance());
    if (app) {
        app->removeEventFilter(this);
        disconnect(app, &QApplication::focusChanged, this,
                   &FocusManager::onFocusChanged);
        m_installed = false;
    }
}

void FocusManager::onFocusChanged(QWidget* old, QWidget* now) {
    Q_UNUSED(old);

    if (!m_enabled) {
        return;
    }

    // Hide indicator if no focus or focus widget shouldn't show indicator
    if (!now || !shouldShowIndicatorFor(now)) {
        if (m_indicator) {
            m_indicator->hideIndicator();
        }
        m_currentFocusWidget = nullptr;
        return;
    }

    // Create indicator if it doesn't exist
    if (!m_indicator) {
        // Find top-level widget to use as parent
        QWidget* topLevel = now->window();
        m_indicator = new EnhancedFocusIndicator(topLevel);
        m_indicator->setStyle(m_style);
        m_indicator->setFocusColor(m_color);
        m_indicator->setBorderThickness(m_thickness);
    }

    // Update indicator target and show
    m_currentFocusWidget = now;
    m_indicator->setTargetWidget(now);
    m_indicator->showIndicator();
}

bool FocusManager::shouldShowIndicatorFor(QWidget* widget) const {
    if (!widget) {
        return false;
    }

    // Don't show for certain widget types
    QString className = widget->metaObject()->className();

    // Skip popup widgets, tooltips, etc.
    if (widget->windowFlags() & (Qt::Popup | Qt::ToolTip | Qt::SplashScreen)) {
        return false;
    }

    // Skip if widget doesn't accept focus
    if (!widget->focusPolicy() || widget->focusPolicy() == Qt::NoFocus) {
        return false;
    }

    // Skip invisible widgets
    if (!widget->isVisible()) {
        return false;
    }

    return true;
}

bool FocusManager::eventFilter(QObject* obj, QEvent* event) {
    // Handle keyboard events to ensure indicator is visible during keyboard
    // navigation
    if (event->type() == QEvent::KeyPress && m_enabled) {
        QWidget* focusWidget = QApplication::focusWidget();
        if (focusWidget && shouldShowIndicatorFor(focusWidget)) {
            if (!m_indicator || m_indicator->targetWidget() != focusWidget) {
                onFocusChanged(nullptr, focusWidget);
            }
        }
    }

    return QObject::eventFilter(obj, event);
}
