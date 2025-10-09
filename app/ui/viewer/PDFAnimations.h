#pragma once

#include <QColor>
#include <QEasingCurve>
#include <QGraphicsBlurEffect>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QList>
#include <QObject>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPixmap>
#include <QPoint>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QTimer>
#include <QWidget>
#include <QtCore>
#include <QtGui>
#include <QtWidgets>

/**
 * Animation manager for PDF viewer with smooth transitions and effects
 */
class PDFAnimationManager : public QObject {
    Q_OBJECT

public:
    enum class AnimationType {
        ZoomIn,
        ZoomOut,
        PageTransition,
        FadeIn,
        FadeOut,
        SlideLeft,
        SlideRight,
        Bounce,
        Elastic
    };

    explicit PDFAnimationManager(QObject* parent = nullptr);
    ~PDFAnimationManager();

    // Zoom animations
    void animateZoom(QWidget* target, double fromScale, double toScale,
                     int duration = 300);
    void animateZoomWithCenter(QWidget* target, double fromScale,
                               double toScale, const QPoint& center,
                               int duration = 300);

    // Page transition animations
    void animatePageTransition(QWidget* fromPage, QWidget* toPage,
                               AnimationType type = AnimationType::SlideLeft,
                               int duration = 400);

    // Fade animations
    void animateFadeIn(QWidget* target, int duration = 250);
    void animateFadeOut(QWidget* target, int duration = 250);
    void animateFadeTransition(QWidget* fromWidget, QWidget* toWidget,
                               int duration = 300);

    // UI feedback animations
    void animateButtonPress(QWidget* button);
    void animateHighlight(QWidget* target,
                          const QColor& color = QColor(255, 255, 0, 100));
    void animateShake(QWidget* target);
    void animatePulse(QWidget* target);

    // Loading animations
    void startLoadingAnimation(QWidget* target);
    void stopLoadingAnimation(QWidget* target);

    // Utility functions
    void setDefaultDuration(int duration) { m_defaultDuration = duration; }
    void setDefaultEasing(QEasingCurve::Type easing) {
        m_defaultEasing = easing;
    }

    bool isAnimating() const { return m_activeAnimations > 0; }
    void stopAllAnimations();

public slots:
    void onAnimationFinished();

private:
    void setupAnimation(QPropertyAnimation* animation, int duration = -1);
    QGraphicsOpacityEffect* ensureOpacityEffect(QWidget* widget);
    void cleanupEffects(QWidget* widget);

    int m_defaultDuration;
    QEasingCurve::Type m_defaultEasing;
    int m_activeAnimations;
    QList<QPropertyAnimation*> m_runningAnimations;

signals:
    void animationStarted(AnimationType type);
    void animationFinished(AnimationType type);
    void allAnimationsFinished();
};

/**
 * Smooth zoom animation widget that handles scaling with proper center point
 */
class SmoothZoomWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double scaleFactor READ scaleFactor WRITE setScaleFactor)

public:
    explicit SmoothZoomWidget(QWidget* parent = nullptr);

    double scaleFactor() const { return m_scaleFactor; }
    void setScaleFactor(double factor);

    void animateToScale(double targetScale, const QPoint& center = QPoint(),
                        int duration = 300);
    void setContent(QWidget* content);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onScaleAnimationFinished();

private:
    void updateContentTransform();

    double m_scaleFactor;
    QPoint m_scaleCenter;
    QWidget* m_content;
    QPropertyAnimation* m_scaleAnimation;

signals:
    void scaleChanged(double scale);
    void scaleAnimationFinished();
};

/**
 * Page transition widget with various animation effects
 */
class PageTransitionWidget : public QWidget {
    Q_OBJECT

public:
    enum class TransitionType {
        None,
        Fade,
        SlideLeft,
        SlideRight,
        SlideUp,
        SlideDown,
        Zoom,
        Flip,
        Cube
    };

    explicit PageTransitionWidget(QWidget* parent = nullptr);

    void setCurrentWidget(QWidget* widget);
    void transitionTo(QWidget* newWidget,
                      TransitionType type = TransitionType::SlideLeft,
                      int duration = 400);

    TransitionType currentTransition() const { return m_currentTransition; }
    bool isTransitioning() const { return m_isTransitioning; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onTransitionFinished();

private:
    void setupTransition(TransitionType type, int duration);
    void paintSlideTransition(QPainter& painter);
    void paintFadeTransition(QPainter& painter);
    void paintZoomTransition(QPainter& painter);

    QWidget* m_currentWidget;
    QWidget* m_nextWidget;
    TransitionType m_currentTransition;
    bool m_isTransitioning;

    QPropertyAnimation* m_transitionAnimation;
    double m_transitionProgress;

    QPixmap m_currentPixmap;
    QPixmap m_nextPixmap;

signals:
    void transitionStarted(TransitionType type);
    void transitionFinished();
};

/**
 * Loading animation widget with various spinner types
 */
class LoadingAnimationWidget : public QWidget {
    Q_OBJECT

public:
    enum class LoadingType { Spinner, Dots, Bars, Ring, Pulse };

    explicit LoadingAnimationWidget(QWidget* parent = nullptr);

    void setLoadingType(LoadingType type) { m_loadingType = type; }
    void setColor(const QColor& color) { m_color = color; }
    void setSize(int size) { m_size = size; }

    void startAnimation();
    void stopAnimation();
    bool isAnimating() const { return m_timer && m_timer->isActive(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    void timerEvent(QTimerEvent* event) override;

private:
    void paintSpinner(QPainter& painter);
    void paintDots(QPainter& painter);
    void paintBars(QPainter& painter);
    void paintRing(QPainter& painter);
    void paintPulse(QPainter& painter);

    LoadingType m_loadingType;
    QColor m_color;
    int m_size;
    int m_angle;
    int m_frame;
    QTimer* m_timer;
};

/**
 * Utility class for creating common animation effects
 */
class AnimationUtils {
public:
    // Easing curve presets
    static QEasingCurve smoothEasing() { return QEasingCurve::OutCubic; }
    static QEasingCurve bounceEasing() { return QEasingCurve::OutBounce; }
    static QEasingCurve elasticEasing() { return QEasingCurve::OutElastic; }
    static QEasingCurve backEasing() { return QEasingCurve::OutBack; }

    // Animation duration presets
    static constexpr int FAST_DURATION = 150;
    static constexpr int NORMAL_DURATION = 300;
    static constexpr int SLOW_DURATION = 500;

    // Create common animations
    static QPropertyAnimation* createFadeAnimation(
        QWidget* target, double from, double to,
        int duration = NORMAL_DURATION);
    static QPropertyAnimation* createMoveAnimation(
        QWidget* target, const QPoint& from, const QPoint& to,
        int duration = NORMAL_DURATION);
    static QPropertyAnimation* createScaleAnimation(
        QObject* target, const QByteArray& property, double from, double to,
        int duration = NORMAL_DURATION);

    // Utility functions
    static QPixmap grabWidget(QWidget* widget);
    static void applyDropShadow(QWidget* widget,
                                const QColor& color = QColor(0, 0, 0, 80));
    static void removeEffects(QWidget* widget);
};
