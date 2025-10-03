#pragma once

#include <QPointer>
#include <QWidget>
#include <memory>

class QLabel;
class QPushButton;
class QPropertyAnimation;
class QGraphicsOpacityEffect;
class QTimer;
class OnboardingManager;

enum class OnboardingStep;

/**
 * OnboardingWidget
 *
 * An overlay widget that provides interactive guided tours for first-time
 * users. Features tooltips, highlights, and step-by-step navigation through app
 * features.
 */
class OnboardingWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal overlayOpacity READ overlayOpacity WRITE setOverlayOpacity)
    Q_PROPERTY(
        QPoint tooltipPosition READ tooltipPosition WRITE setTooltipPosition)

public:
    explicit OnboardingWidget(QWidget* parent = nullptr);
    ~OnboardingWidget();

    // Manager
    void setOnboardingManager(OnboardingManager* manager);

    // Display control
    void showStep(OnboardingStep step);
    void hideStep();
    void updateStepContent();

    // Highlighting
    void highlightWidget(QWidget* widget);
    void highlightArea(const QRect& area);
    void clearHighlight();

    // Tooltip management
    void showTooltip(const QString& title, const QString& description,
                     const QPoint& position,
                     Qt::Alignment alignment = Qt::AlignCenter);
    void hideTooltip();
    void updateTooltipPosition(const QPoint& position);

    // Animation control
    void startAnimation();
    void stopAnimation();
    bool isAnimating() const;

    // Properties
    qreal overlayOpacity() const;
    void setOverlayOpacity(qreal opacity);

    QPoint tooltipPosition() const;
    void setTooltipPosition(const QPoint& position);

    // Theme support
    void applyTheme();

signals:
    void nextClicked();
    void previousClicked();
    void skipClicked();
    void closeClicked();
    void stepCompleted(OnboardingStep step);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onNextClicked();
    void onPreviousClicked();
    void onSkipClicked();
    void onCloseClicked();
    void onAnimationFinished();
    void onPulseTimer();
    void updateLayout();

private:
    void initializeUI();
    void setupTooltip();
    void setupNavigation();
    void setupAnimations();
    void setupConnections();

    void positionTooltip();
    void updateNavigationButtons();
    QRect calculateHighlightRect(QWidget* widget) const;
    void drawOverlay(QPainter& painter);
    void drawHighlight(QPainter& painter);
    void drawSpotlight(QPainter& painter, const QRect& rect);

    // Manager
    OnboardingManager* m_manager;

    // UI Components
    QWidget* m_tooltipWidget;
    QLabel* m_titleLabel;
    QLabel* m_descriptionLabel;
    QLabel* m_stepIndicator;
    QPushButton* m_nextButton;
    QPushButton* m_previousButton;
    QPushButton* m_skipButton;
    QPushButton* m_closeButton;

    // Highlight
    QPointer<QWidget> m_highlightedWidget;
    QRect m_highlightArea;
    bool m_hasHighlight;

    // Animation
    std::unique_ptr<QPropertyAnimation> m_fadeAnimation;
    std::unique_ptr<QPropertyAnimation> m_moveAnimation;
    std::unique_ptr<QTimer> m_pulseTimer;
    qreal m_overlayOpacity;
    QPoint m_tooltipPosition;
    qreal m_pulsePhase;
    bool m_isAnimating;

    // Layout
    Qt::Alignment m_tooltipAlignment;
    QPoint m_tooltipOffset;

    // Constants
    static const int TOOLTIP_WIDTH = 320;
    static const int TOOLTIP_MARGIN = 20;
    static const int HIGHLIGHT_PADDING = 10;
    static const qreal MAX_OVERLAY_OPACITY;
    static const int ANIMATION_DURATION = 300;
    static const int PULSE_INTERVAL = 50;
};
