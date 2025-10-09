#include "PDFAnimations.h"
#include <QApplication>
#include <QDebug>
#include <QEasingCurve>
#include <QGraphicsDropShadowEffect>
#include <QObject>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPixmap>
#include <QPoint>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QTimer>
#include <QWidget>
#include <cmath>

// PDFAnimationManager Implementation
PDFAnimationManager::PDFAnimationManager(QObject* parent)
    : QObject(parent),
      m_defaultDuration(300),
      m_defaultEasing(QEasingCurve::OutCubic),
      m_activeAnimations(0) {}

PDFAnimationManager::~PDFAnimationManager() { stopAllAnimations(); }

void PDFAnimationManager::animateZoom(QWidget* target, double fromScale,
                                      double toScale, int duration) {
    if (!target)
        return;

    QPropertyAnimation* animation =
        new QPropertyAnimation(target, "scaleFactor");
    setupAnimation(animation, duration);

    animation->setStartValue(fromScale);
    animation->setEndValue(toScale);

    connect(animation, &QPropertyAnimation::finished, this,
            &PDFAnimationManager::onAnimationFinished);

    m_runningAnimations.append(animation);
    m_activeAnimations++;

    emit animationStarted(AnimationType::ZoomIn);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void PDFAnimationManager::animatePageTransition(QWidget* fromPage,
                                                QWidget* toPage,
                                                AnimationType type,
                                                int duration) {
    if (!fromPage || !toPage)
        return;

    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);

    // Fade out current page
    QGraphicsOpacityEffect* fromEffect = ensureOpacityEffect(fromPage);
    QPropertyAnimation* fadeOut = new QPropertyAnimation(fromEffect, "opacity");
    setupAnimation(fadeOut, duration / 2);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);

    // Fade in new page
    QGraphicsOpacityEffect* toEffect = ensureOpacityEffect(toPage);
    toEffect->setOpacity(0.0);
    toPage->show();

    QPropertyAnimation* fadeIn = new QPropertyAnimation(toEffect, "opacity");
    setupAnimation(fadeIn, duration / 2);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    // Note: Start time will be handled by sequential animation group

    group->addAnimation(fadeOut);
    group->addAnimation(fadeIn);

    connect(group, &QAnimationGroup::finished, this,
            [this, fromPage, toPage]() {
                cleanupEffects(fromPage);
                cleanupEffects(toPage);
                fromPage->hide();
                onAnimationFinished();
            });

    m_activeAnimations++;
    emit animationStarted(type);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void PDFAnimationManager::animateFadeIn(QWidget* target, int duration) {
    if (!target)
        return;

    QGraphicsOpacityEffect* effect = ensureOpacityEffect(target);
    effect->setOpacity(0.0);
    target->show();

    QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity");
    setupAnimation(animation, duration);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);

    connect(animation, &QPropertyAnimation::finished, this, [this, target]() {
        cleanupEffects(target);
        onAnimationFinished();
    });

    m_runningAnimations.append(animation);
    m_activeAnimations++;

    emit animationStarted(AnimationType::FadeIn);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void PDFAnimationManager::animateFadeOut(QWidget* target, int duration) {
    if (!target)
        return;

    QGraphicsOpacityEffect* effect = ensureOpacityEffect(target);

    QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity");
    setupAnimation(animation, duration);
    animation->setStartValue(1.0);
    animation->setEndValue(0.0);

    connect(animation, &QPropertyAnimation::finished, this, [this, target]() {
        target->hide();
        cleanupEffects(target);
        onAnimationFinished();
    });

    m_runningAnimations.append(animation);
    m_activeAnimations++;

    emit animationStarted(AnimationType::FadeOut);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void PDFAnimationManager::animateButtonPress(QWidget* button) {
    if (!button)
        return;

    QSequentialAnimationGroup* group = new QSequentialAnimationGroup(this);

    // Scale down
    QPropertyAnimation* scaleDown =
        AnimationUtils::createScaleAnimation(button, "scale", 1.0, 0.95, 100);
    scaleDown->setEasingCurve(QEasingCurve::OutCubic);

    // Scale back up
    QPropertyAnimation* scaleUp =
        AnimationUtils::createScaleAnimation(button, "scale", 0.95, 1.0, 100);
    scaleUp->setEasingCurve(QEasingCurve::OutBounce);

    group->addAnimation(scaleDown);
    group->addAnimation(scaleUp);

    connect(group, &QAnimationGroup::finished, this,
            &PDFAnimationManager::onAnimationFinished);

    m_activeAnimations++;
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void PDFAnimationManager::animateHighlight(QWidget* target,
                                           const QColor& color) {
    if (!target)
        return;

    // Create highlight effect using background color animation
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(target);
    target->setGraphicsEffect(effect);

    QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity");
    animation->setDuration(500);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::InOutSine);
    animation->setLoopCount(2);

    connect(animation, &QPropertyAnimation::finished, this, [this, target]() {
        cleanupEffects(target);
        onAnimationFinished();
    });

    m_runningAnimations.append(animation);
    m_activeAnimations++;
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void PDFAnimationManager::animateShake(QWidget* target) {
    if (!target)
        return;

    QSequentialAnimationGroup* group = new QSequentialAnimationGroup(this);
    QPoint originalPos = target->pos();

    // Create shake sequence
    for (int i = 0; i < 4; ++i) {
        int offset = (i % 2 == 0) ? 5 : -5;
        QPropertyAnimation* shake = AnimationUtils::createMoveAnimation(
            target, target->pos(), originalPos + QPoint(offset, 0), 50);
        group->addAnimation(shake);
    }

    // Return to original position
    QPropertyAnimation* returnAnim = AnimationUtils::createMoveAnimation(
        target, target->pos(), originalPos, 50);
    group->addAnimation(returnAnim);

    connect(group, &QAnimationGroup::finished, this,
            &PDFAnimationManager::onAnimationFinished);

    m_activeAnimations++;
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void PDFAnimationManager::stopAllAnimations() {
    for (QPropertyAnimation* animation : m_runningAnimations) {
        if (animation && animation->state() == QAbstractAnimation::Running) {
            animation->stop();
        }
    }
    m_runningAnimations.clear();
    m_activeAnimations = 0;
}

void PDFAnimationManager::onAnimationFinished() {
    m_activeAnimations--;
    if (m_activeAnimations <= 0) {
        m_activeAnimations = 0;
        emit allAnimationsFinished();
    }
}

void PDFAnimationManager::setupAnimation(QPropertyAnimation* animation,
                                         int duration) {
    if (!animation)
        return;

    animation->setDuration(duration > 0 ? duration : m_defaultDuration);
    animation->setEasingCurve(m_defaultEasing);
}

QGraphicsOpacityEffect* PDFAnimationManager::ensureOpacityEffect(
    QWidget* widget) {
    if (!widget)
        return nullptr;

    QGraphicsOpacityEffect* effect =
        qobject_cast<QGraphicsOpacityEffect*>(widget->graphicsEffect());
    if (!effect) {
        effect = new QGraphicsOpacityEffect(widget);
        widget->setGraphicsEffect(effect);
    }
    return effect;
}

void PDFAnimationManager::cleanupEffects(QWidget* widget) {
    if (widget && widget->graphicsEffect()) {
        widget->setGraphicsEffect(nullptr);
    }
}

// SmoothZoomWidget Implementation
SmoothZoomWidget::SmoothZoomWidget(QWidget* parent)
    : QWidget(parent),
      m_scaleFactor(1.0),
      m_content(nullptr),
      m_scaleAnimation(nullptr) {
    m_scaleAnimation = new QPropertyAnimation(this, "scaleFactor");
    m_scaleAnimation->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_scaleAnimation, &QPropertyAnimation::finished, this,
            &SmoothZoomWidget::onScaleAnimationFinished);
}

void SmoothZoomWidget::setScaleFactor(double factor) {
    if (qAbs(factor - m_scaleFactor) < 0.001)
        return;

    m_scaleFactor = factor;
    updateContentTransform();
    emit scaleChanged(factor);
    update();
}

void SmoothZoomWidget::animateToScale(double targetScale, const QPoint& center,
                                      int duration) {
    if (m_scaleAnimation->state() == QAbstractAnimation::Running) {
        m_scaleAnimation->stop();
    }

    m_scaleCenter = center.isNull() ? rect().center() : center;

    m_scaleAnimation->setDuration(duration);
    m_scaleAnimation->setStartValue(m_scaleFactor);
    m_scaleAnimation->setEndValue(targetScale);
    m_scaleAnimation->start();
}

void SmoothZoomWidget::setContent(QWidget* content) {
    if (m_content) {
        m_content->setParent(nullptr);
    }

    m_content = content;
    if (m_content) {
        m_content->setParent(this);
        updateContentTransform();
    }
}

void SmoothZoomWidget::updateContentTransform() {
    if (!m_content)
        return;

    // Apply scaling transformation
    QSize scaledSize = m_content->sizeHint() * m_scaleFactor;
    m_content->resize(scaledSize);

    // Center the content
    QPoint centerPos = rect().center() -
                       QPoint(scaledSize.width() / 2, scaledSize.height() / 2);
    m_content->move(centerPos);
}

void SmoothZoomWidget::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);

    if (m_content) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        // Draw content with current scale
        painter.save();
        painter.translate(m_scaleCenter);
        painter.scale(m_scaleFactor, m_scaleFactor);
        painter.translate(-m_scaleCenter);

        m_content->render(&painter, m_content->pos());
        painter.restore();
    }
}

void SmoothZoomWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateContentTransform();
}

void SmoothZoomWidget::onScaleAnimationFinished() {
    emit scaleAnimationFinished();
}

// AnimationUtils Implementation
QPropertyAnimation* AnimationUtils::createFadeAnimation(QWidget* target,
                                                        double from, double to,
                                                        int duration) {
    if (!target)
        return nullptr;

    QGraphicsOpacityEffect* effect =
        qobject_cast<QGraphicsOpacityEffect*>(target->graphicsEffect());
    if (!effect) {
        effect = new QGraphicsOpacityEffect(target);
        target->setGraphicsEffect(effect);
    }

    QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity");
    animation->setDuration(duration);
    animation->setStartValue(from);
    animation->setEndValue(to);
    animation->setEasingCurve(smoothEasing());

    return animation;
}

QPropertyAnimation* AnimationUtils::createMoveAnimation(QWidget* target,
                                                        const QPoint& from,
                                                        const QPoint& to,
                                                        int duration) {
    if (!target)
        return nullptr;

    QPropertyAnimation* animation = new QPropertyAnimation(target, "pos");
    animation->setDuration(duration);
    animation->setStartValue(from);
    animation->setEndValue(to);
    animation->setEasingCurve(smoothEasing());

    return animation;
}

QPropertyAnimation* AnimationUtils::createScaleAnimation(
    QObject* target, const QByteArray& property, double from, double to,
    int duration) {
    if (!target)
        return nullptr;

    QPropertyAnimation* animation = new QPropertyAnimation(target, property);
    animation->setDuration(duration);
    animation->setStartValue(from);
    animation->setEndValue(to);
    animation->setEasingCurve(smoothEasing());

    return animation;
}

QPixmap AnimationUtils::grabWidget(QWidget* widget) {
    if (!widget)
        return QPixmap();

    return widget->grab();
}

void AnimationUtils::applyDropShadow(QWidget* widget, const QColor& color) {
    if (!widget)
        return;

    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setColor(color);
    shadow->setBlurRadius(10);
    shadow->setOffset(2, 2);
    widget->setGraphicsEffect(shadow);
}

void AnimationUtils::removeEffects(QWidget* widget) {
    if (widget && widget->graphicsEffect()) {
        widget->setGraphicsEffect(nullptr);
    }
}

// PageTransitionWidget Implementation
PageTransitionWidget::PageTransitionWidget(QWidget* parent)
    : QWidget(parent),
      m_currentWidget(nullptr),
      m_nextWidget(nullptr),
      m_currentTransition(TransitionType::None),
      m_isTransitioning(false),
      m_transitionAnimation(nullptr),
      m_transitionProgress(0.0) {
    m_transitionAnimation = new QPropertyAnimation(this, "transitionProgress");
    connect(m_transitionAnimation, &QPropertyAnimation::finished, this,
            &PageTransitionWidget::onTransitionFinished);
}

void PageTransitionWidget::setCurrentWidget(QWidget* widget) {
    m_currentWidget = widget;
    if (widget) {
        widget->setParent(this);
        widget->resize(size());
    }
    update();
}

void PageTransitionWidget::transitionTo(QWidget* newWidget, TransitionType type,
                                        int duration) {
    if (m_isTransitioning || !newWidget)
        return;

    m_nextWidget = newWidget;
    m_currentTransition = type;
    m_isTransitioning = true;

    // Capture current and next widget as pixmaps
    if (m_currentWidget) {
        m_currentPixmap = m_currentWidget->grab();
    }

    newWidget->setParent(this);
    newWidget->resize(size());
    m_nextPixmap = newWidget->grab();

    setupTransition(type, duration);
    emit transitionStarted(type);
}

void PageTransitionWidget::setupTransition(TransitionType type, int duration) {
    m_transitionAnimation->setDuration(duration);
    m_transitionAnimation->setStartValue(0.0);
    m_transitionAnimation->setEndValue(1.0);

    switch (type) {
        case TransitionType::Fade:
            m_transitionAnimation->setEasingCurve(QEasingCurve::InOutQuad);
            break;
        case TransitionType::SlideLeft:
        case TransitionType::SlideRight:
            m_transitionAnimation->setEasingCurve(QEasingCurve::OutCubic);
            break;
        default:
            m_transitionAnimation->setEasingCurve(QEasingCurve::Linear);
            break;
    }

    m_transitionAnimation->start();
}

void PageTransitionWidget::onTransitionFinished() {
    m_isTransitioning = false;
    m_currentWidget = m_nextWidget;
    m_nextWidget = nullptr;
    m_currentTransition = TransitionType::None;
    m_transitionProgress = 0.0;

    update();
    emit transitionFinished();
}

void PageTransitionWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);

    if (!m_isTransitioning) {
        if (!m_currentPixmap.isNull()) {
            painter.drawPixmap(rect(), m_currentPixmap);
        }
        return;
    }

    switch (m_currentTransition) {
        case TransitionType::Fade:
            paintFadeTransition(painter);
            break;
        case TransitionType::SlideLeft:
        case TransitionType::SlideRight:
            paintSlideTransition(painter);
            break;
        default:
            paintFadeTransition(painter);
            break;
    }
}

void PageTransitionWidget::paintFadeTransition(QPainter& painter) {
    if (!m_currentPixmap.isNull()) {
        painter.setOpacity(1.0 - m_transitionProgress);
        painter.drawPixmap(rect(), m_currentPixmap);
    }

    if (!m_nextPixmap.isNull()) {
        painter.setOpacity(m_transitionProgress);
        painter.drawPixmap(rect(), m_nextPixmap);
    }
}

void PageTransitionWidget::paintSlideTransition(QPainter& painter) {
    int offset = static_cast<int>(width() * m_transitionProgress);

    if (m_currentTransition == TransitionType::SlideLeft) {
        if (!m_currentPixmap.isNull()) {
            painter.drawPixmap(-offset, 0, m_currentPixmap);
        }
        if (!m_nextPixmap.isNull()) {
            painter.drawPixmap(width() - offset, 0, m_nextPixmap);
        }
    } else {  // SlideRight
        if (!m_currentPixmap.isNull()) {
            painter.drawPixmap(offset, 0, m_currentPixmap);
        }
        if (!m_nextPixmap.isNull()) {
            painter.drawPixmap(-width() + offset, 0, m_nextPixmap);
        }
    }
}

void PageTransitionWidget::paintZoomTransition(QPainter& painter) {
    // Simple zoom transition implementation
    if (!m_nextPixmap.isNull()) {
        double scale = m_transitionProgress;
        int scaledWidth = static_cast<int>(width() * scale);
        int scaledHeight = static_cast<int>(height() * scale);
        int x = (width() - scaledWidth) / 2;
        int y = (height() - scaledHeight) / 2;

        painter.drawPixmap(x, y, scaledWidth, scaledHeight, m_nextPixmap);
    }
}

void PageTransitionWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_currentWidget) {
        m_currentWidget->resize(size());
    }
    if (m_nextWidget) {
        m_nextWidget->resize(size());
    }
}

// LoadingAnimationWidget Implementation
LoadingAnimationWidget::LoadingAnimationWidget(QWidget* parent)
    : QWidget(parent),
      m_loadingType(LoadingType::Spinner),
      m_color(Qt::blue),
      m_size(32),
      m_angle(0),
      m_frame(0),
      m_timer(nullptr) {
    setFixedSize(m_size, m_size);
}

void LoadingAnimationWidget::startAnimation() {
    if (!m_timer) {
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, [this]() {
            m_angle = (m_angle + 30) % 360;
            m_frame = (m_frame + 1) % 8;
            update();
        });
    }

    m_timer->start(100);  // 100ms interval
}

void LoadingAnimationWidget::stopAnimation() {
    if (m_timer) {
        m_timer->stop();
    }
    m_angle = 0;
    m_frame = 0;
    update();
}

void LoadingAnimationWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    switch (m_loadingType) {
        case LoadingType::Spinner:
            paintSpinner(painter);
            break;
        case LoadingType::Dots:
            paintDots(painter);
            break;
        case LoadingType::Bars:
            paintBars(painter);
            break;
        case LoadingType::Ring:
            paintRing(painter);
            break;
        case LoadingType::Pulse:
            paintPulse(painter);
            break;
    }
}

void LoadingAnimationWidget::paintSpinner(QPainter& painter) {
    painter.translate(width() / 2, height() / 2);
    painter.rotate(m_angle);

    painter.setPen(QPen(m_color, 3));
    painter.drawArc(-m_size / 4, -m_size / 4, m_size / 2, m_size / 2, 0,
                    270 * 16);
}

void LoadingAnimationWidget::paintDots(QPainter& painter) {
    painter.setBrush(m_color);

    int dotSize = m_size / 8;
    int spacing = m_size / 4;

    for (int i = 0; i < 3; ++i) {
        double opacity = (i == m_frame % 3) ? 1.0 : 0.3;
        painter.setOpacity(opacity);

        int x = width() / 2 - spacing + i * spacing;
        int y = height() / 2;
        painter.drawEllipse(x - dotSize / 2, y - dotSize / 2, dotSize, dotSize);
    }
}

void LoadingAnimationWidget::paintBars(QPainter& painter) {
    painter.setBrush(m_color);

    int barWidth = m_size / 8;
    int barSpacing = m_size / 6;

    for (int i = 0; i < 4; ++i) {
        double scale = (i == m_frame % 4) ? 1.0 : 0.5;
        int barHeight = static_cast<int>(m_size / 2 * scale);

        int x = width() / 2 - 2 * barSpacing + i * barSpacing;
        int y = height() / 2 - barHeight / 2;
        painter.drawRect(x, y, barWidth, barHeight);
    }
}

void LoadingAnimationWidget::paintRing(QPainter& painter) {
    painter.translate(width() / 2, height() / 2);
    painter.rotate(m_angle);

    painter.setPen(QPen(m_color, 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(-m_size / 4, -m_size / 4, m_size / 2, m_size / 2);

    painter.setPen(QPen(m_color, 4));
    painter.drawArc(-m_size / 4, -m_size / 4, m_size / 2, m_size / 2, 0,
                    90 * 16);
}

void LoadingAnimationWidget::paintPulse(QPainter& painter) {
    double scale = 0.5 + 0.5 * std::sin(m_frame * M_PI / 4);
    int size = static_cast<int>(m_size / 2 * scale);

    painter.setBrush(m_color);
    painter.setOpacity(1.0 - scale * 0.5);

    int x = width() / 2 - size / 2;
    int y = height() / 2 - size / 2;
    painter.drawEllipse(x, y, size, size);
}

void LoadingAnimationWidget::timerEvent(QTimerEvent* event) {
    Q_UNUSED(event);
    // Timer events are handled by QTimer::timeout signal
}

// Note: MOC file will be generated automatically by CMake
