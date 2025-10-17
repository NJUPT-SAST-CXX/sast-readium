#include "OnboardingWidget.h"
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>
#include "../../managers/OnboardingManager.h"

// Static constants
const qreal OnboardingWidget::MAX_OVERLAY_OPACITY = 0.8;

OnboardingWidget::OnboardingWidget(QWidget* parent)
    : QWidget(parent),
      m_manager(nullptr),
      m_tooltipWidget(nullptr),
      m_titleLabel(nullptr),
      m_descriptionLabel(nullptr),
      m_stepIndicator(nullptr),
      m_nextButton(nullptr),
      m_previousButton(nullptr),
      m_skipButton(nullptr),
      m_closeButton(nullptr),
      m_highlightedWidget(nullptr),
      m_hasHighlight(false),
      m_overlayOpacity(0.0),
      m_tooltipPosition(0, 0),
      m_pulsePhase(0.0),
      m_isAnimating(false),
      m_tooltipAlignment(Qt::AlignCenter),
      m_tooltipOffset(0, 0) {
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    initializeUI();
    setupTooltip();

    // Initialize animations
    m_fadeAnimation =
        std::make_unique<QPropertyAnimation>(this, "overlayOpacity");
    m_fadeAnimation->setDuration(ANIMATION_DURATION);

    m_moveAnimation =
        std::make_unique<QPropertyAnimation>(this, "tooltipPosition");
    m_moveAnimation->setDuration(ANIMATION_DURATION);

    m_pulseTimer = std::make_unique<QTimer>(this);
    m_pulseTimer->setInterval(PULSE_INTERVAL);
    connect(m_pulseTimer.get(), &QTimer::timeout, this,
            &OnboardingWidget::onPulseTimer);

    connect(m_fadeAnimation.get(), &QPropertyAnimation::finished, this,
            &OnboardingWidget::onAnimationFinished);
    connect(m_moveAnimation.get(), &QPropertyAnimation::finished, this,
            &OnboardingWidget::onAnimationFinished);
}

OnboardingWidget::~OnboardingWidget() = default;

void OnboardingWidget::setOnboardingManager(OnboardingManager* manager) {
    m_manager = manager;
}

void OnboardingWidget::showStep(OnboardingStep step) {
    // Update step content based on the step
    updateStepContent();

    // Show the widget with fade in animation
    show();
    m_fadeAnimation->setStartValue(0.0);
    m_fadeAnimation->setEndValue(MAX_OVERLAY_OPACITY);
    m_fadeAnimation->start();

    m_pulseTimer->start();
}

void OnboardingWidget::hideStep() {
    m_pulseTimer->stop();

    // Hide with fade out animation
    m_fadeAnimation->setStartValue(m_overlayOpacity);
    m_fadeAnimation->setEndValue(0.0);
    m_fadeAnimation->start();
}

void OnboardingWidget::updateStepContent() {
    if (!m_manager)
        return;

    OnboardingStep currentStep = m_manager->currentStep();

    // Update content based on current step
    switch (currentStep) {
        case OnboardingStep::Welcome:
            m_titleLabel->setText("Welcome to SAST Readium!");
            m_descriptionLabel->setText(
                "Let's take a quick tour to get you started with our PDF "
                "reader.");
            break;
        case OnboardingStep::OpenFile:
            m_titleLabel->setText("Opening Files");
            m_descriptionLabel->setText(
                "Click 'File > Open' or use Ctrl+O to open a PDF document.");
            break;
        case OnboardingStep::Navigation:
            m_titleLabel->setText("Navigation");
            m_descriptionLabel->setText(
                "Use the navigation buttons or arrow keys to move between "
                "pages.");
            break;
        case OnboardingStep::Search:
            m_titleLabel->setText("Search");
            m_descriptionLabel->setText(
                "Use Ctrl+F to search for text within your document.");
            break;
        case OnboardingStep::Bookmarks:
            m_titleLabel->setText("Bookmarks");
            m_descriptionLabel->setText(
                "Create bookmarks to quickly navigate to important sections.");
            break;
        case OnboardingStep::Annotations:
            m_titleLabel->setText("Annotations");
            m_descriptionLabel->setText(
                "Add notes and highlights to your documents.");
            break;
        case OnboardingStep::ViewModes:
            m_titleLabel->setText("View Modes");
            m_descriptionLabel->setText(
                "Switch between different view modes for optimal reading.");
            break;
        case OnboardingStep::Settings:
            m_titleLabel->setText("Settings");
            m_descriptionLabel->setText(
                "Customize the application to your preferences.");
            break;
        case OnboardingStep::KeyboardShortcuts:
            m_titleLabel->setText("Keyboard Shortcuts");
            m_descriptionLabel->setText(
                "Learn useful keyboard shortcuts to work more efficiently.");
            break;
        case OnboardingStep::Complete:
            m_titleLabel->setText("Tour Complete!");
            m_descriptionLabel->setText(
                "You're all set! Enjoy using SAST Readium.");
            break;
    }

    // Update step indicator
    int currentStepNum = static_cast<int>(currentStep) + 1;
    int totalSteps = m_manager->getTotalStepsCount();
    m_stepIndicator->setText(
        QString("Step %1 of %2").arg(currentStepNum).arg(totalSteps));

    // Update button states
    m_previousButton->setEnabled(currentStepNum > 1);
    m_nextButton->setText(currentStep == OnboardingStep::Complete ? "Finish"
                                                                  : "Next");
}

void OnboardingWidget::highlightWidget(QWidget* widget) {
    m_highlightedWidget = widget;
    m_hasHighlight = true;

    if (widget) {
        m_highlightArea = widget->geometry();
        if (widget->parentWidget()) {
            QPoint globalTopLeft =
                widget->parentWidget()->mapToGlobal(m_highlightArea.topLeft());
            QPoint localTopLeft = mapFromGlobal(globalTopLeft);
            m_highlightArea.moveTopLeft(localTopLeft);
        }
    }

    update();
}

void OnboardingWidget::highlightArea(const QRect& area) {
    m_highlightArea = area;
    m_hasHighlight = true;
    m_highlightedWidget = nullptr;
    update();
}

void OnboardingWidget::clearHighlight() {
    m_hasHighlight = false;
    m_highlightedWidget = nullptr;
    update();
}

void OnboardingWidget::showTooltip(const QString& title,
                                   const QString& description,
                                   const QPoint& position,
                                   Qt::Alignment alignment) {
    m_titleLabel->setText(title);
    m_descriptionLabel->setText(description);
    m_tooltipAlignment = alignment;
    updateTooltipPosition(position);

    if (!isVisible()) {
        show();
    }
}

void OnboardingWidget::hideTooltip() { hideStep(); }

void OnboardingWidget::updateTooltipPosition(const QPoint& position) {
    setTooltipPosition(position);
    updateLayout();
}

qreal OnboardingWidget::overlayOpacity() const { return m_overlayOpacity; }

void OnboardingWidget::setOverlayOpacity(qreal opacity) {
    m_overlayOpacity = opacity;
    update();
}

QPoint OnboardingWidget::tooltipPosition() const { return m_tooltipPosition; }

void OnboardingWidget::setTooltipPosition(const QPoint& position) {
    m_tooltipPosition = position;
    updateLayout();
}

void OnboardingWidget::applyTheme() {
    // Apply current theme to the widget
    QString tooltipStyle =
        "QWidget {"
        "    background-color: rgba(255, 255, 255, 240);"
        "    border: 1px solid #ccc;"
        "    border-radius: 8px;"
        "    padding: 16px;"
        "}"
        "QLabel {"
        "    background: transparent;"
        "    color: #333;"
        "}"
        "QPushButton {"
        "    background-color: #2196F3;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 8px 16px;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #0D47A1;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #ccc;"
        "    color: #999;"
        "}";

    if (m_tooltipWidget) {
        m_tooltipWidget->setStyleSheet(tooltipStyle);
    }
}

void OnboardingWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw overlay
    painter.fillRect(rect(),
                     QColor(0, 0, 0, int(255 * m_overlayOpacity * 0.6)));

    // Draw highlight if present
    if (m_hasHighlight) {
        drawSpotlight(painter, m_highlightArea);
    }
}

void OnboardingWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateLayout();
}

void OnboardingWidget::mousePressEvent(QMouseEvent* event) {
    // Allow clicking through to highlighted widget
    if (m_hasHighlight && m_highlightArea.contains(event->pos())) {
        event->ignore();
        return;
    }

    QWidget::mousePressEvent(event);
}

void OnboardingWidget::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Escape:
            onCloseClicked();
            break;
        case Qt::Key_Right:
        case Qt::Key_Space:
            onNextClicked();
            break;
        case Qt::Key_Left:
            onPreviousClicked();
            break;
        default:
            QWidget::keyPressEvent(event);
    }
}

bool OnboardingWidget::eventFilter(QObject* watched, QEvent* event) {
    // Track highlighted widget movements and updates
    if (watched == m_highlightedWidget && m_hasHighlight) {
        switch (event->type()) {
            case QEvent::Move:
            case QEvent::Resize:
            case QEvent::Show:
            case QEvent::Hide:
                // Update highlight area when widget changes
                if (m_highlightedWidget) {
                    m_highlightArea = m_highlightedWidget->geometry();
                    if (m_highlightedWidget->parentWidget()) {
                        QPoint globalTopLeft = m_highlightedWidget->parentWidget()
                                                   ->mapToGlobal(m_highlightArea.topLeft());
                        QPoint localTopLeft = mapFromGlobal(globalTopLeft);
                        m_highlightArea.moveTopLeft(localTopLeft);
                    }
                    update();
                }
                break;
            default:
                break;
        }
    }
    return QWidget::eventFilter(watched, event);
}

// Public animation control methods
void OnboardingWidget::startAnimation() {
    if (!m_isAnimating) {
        m_isAnimating = true;
        if (m_pulseTimer) {
            m_pulseTimer->start();
        }
    }
}

void OnboardingWidget::stopAnimation() {
    if (m_isAnimating) {
        m_isAnimating = false;
        if (m_pulseTimer) {
            m_pulseTimer->stop();
        }
        m_pulsePhase = 0.0;
        update();
    }
}

bool OnboardingWidget::isAnimating() const {
    return m_isAnimating;
}

// Private slots
void OnboardingWidget::onNextClicked() { emit nextClicked(); }

void OnboardingWidget::onPreviousClicked() { emit previousClicked(); }

void OnboardingWidget::onSkipClicked() { emit skipClicked(); }

void OnboardingWidget::onCloseClicked() { emit closeClicked(); }

void OnboardingWidget::onAnimationFinished() {
    m_isAnimating = false;

    // Hide widget if opacity is 0
    if (m_overlayOpacity <= 0.0) {
        hide();
    }
}

void OnboardingWidget::onPulseTimer() {
    m_pulsePhase += 0.1;
    if (m_pulsePhase > 2.0 * M_PI) {
        m_pulsePhase = 0.0;
    }

    if (m_hasHighlight) {
        update();
    }
}

void OnboardingWidget::updateLayout() {
    if (!m_tooltipWidget)
        return;

    // Calculate tooltip position
    QPoint pos = m_tooltipPosition;
    QSize tooltipSize = m_tooltipWidget->sizeHint();

    // Adjust position to keep tooltip within bounds
    if (pos.x() + tooltipSize.width() > width()) {
        pos.setX(width() - tooltipSize.width() - TOOLTIP_MARGIN);
    }
    if (pos.x() < TOOLTIP_MARGIN) {
        pos.setX(TOOLTIP_MARGIN);
    }
    if (pos.y() + tooltipSize.height() > height()) {
        pos.setY(height() - tooltipSize.height() - TOOLTIP_MARGIN);
    }
    if (pos.y() < TOOLTIP_MARGIN) {
        pos.setY(TOOLTIP_MARGIN);
    }

    m_tooltipWidget->move(pos);
    m_tooltipWidget->resize(tooltipSize);
}

// Private methods
void OnboardingWidget::initializeUI() {
    // Create tooltip widget
    m_tooltipWidget = new QWidget(this);
    m_tooltipWidget->setFixedWidth(TOOLTIP_WIDTH);

    // Create labels
    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet(
        "font-weight: bold; font-size: 16px; color: #333;");
    m_titleLabel->setWordWrap(true);

    m_descriptionLabel = new QLabel(this);
    m_descriptionLabel->setStyleSheet("font-size: 14px; color: #666;");
    m_descriptionLabel->setWordWrap(true);

    m_stepIndicator = new QLabel(this);
    m_stepIndicator->setStyleSheet("font-size: 12px; color: #888;");
    m_stepIndicator->setAlignment(Qt::AlignCenter);

    // Create buttons
    m_nextButton = new QPushButton("Next", this);
    m_previousButton = new QPushButton("Previous", this);
    m_skipButton = new QPushButton("Skip Tour", this);
    m_closeButton = new QPushButton("×", this);

    m_closeButton->setFixedSize(24, 24);
    m_closeButton->setStyleSheet(
        "QPushButton { font-size: 16px; font-weight: bold; }");

    // Connect signals
    connect(m_nextButton, &QPushButton::clicked, this,
            &OnboardingWidget::onNextClicked);
    connect(m_previousButton, &QPushButton::clicked, this,
            &OnboardingWidget::onPreviousClicked);
    connect(m_skipButton, &QPushButton::clicked, this,
            &OnboardingWidget::onSkipClicked);
    connect(m_closeButton, &QPushButton::clicked, this,
            &OnboardingWidget::onCloseClicked);
}

void OnboardingWidget::setupTooltip() {
    if (!m_tooltipWidget)
        return;

    auto* layout = new QVBoxLayout(m_tooltipWidget);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    // Header with close button
    auto* headerLayout = new QHBoxLayout();
    headerLayout->addWidget(m_stepIndicator);
    headerLayout->addStretch();
    headerLayout->addWidget(m_closeButton);

    // Content
    layout->addLayout(headerLayout);
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_descriptionLabel);

    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_skipButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_previousButton);
    buttonLayout->addWidget(m_nextButton);

    layout->addLayout(buttonLayout);

    applyTheme();
}

void OnboardingWidget::drawSpotlight(QPainter& painter, const QRect& rect) {
    // Create spotlight effect around highlighted area
    QRect expandedRect = rect.adjusted(-HIGHLIGHT_PADDING, -HIGHLIGHT_PADDING,
                                       HIGHLIGHT_PADDING, HIGHLIGHT_PADDING);

    // Draw pulsing border
    qreal pulseIntensity = 0.5 + 0.5 * sin(m_pulsePhase);
    QColor borderColor(255, 255, 255, int(255 * pulseIntensity));

    painter.setPen(QPen(borderColor, 3));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(expandedRect, 8, 8);

    // Clear the highlighted area
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.fillRect(rect, Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
}

void OnboardingWidget::setupNavigation() {
    // Navigation is set up in initializeUI() where buttons are created
    // and connected. This method is kept for potential future enhancements.
}

void OnboardingWidget::setupAnimations() {
    // Animations are already set up in the constructor
    // This method is kept for potential future enhancements.
}

void OnboardingWidget::setupConnections() {
    // Connections are already set up in initializeUI()
    // This method is kept for potential future enhancements.
}

void OnboardingWidget::positionTooltip() {
    // Tooltip positioning is handled by updateLayout()
    // This is an alias for consistency with the interface
    updateLayout();
}

void OnboardingWidget::updateNavigationButtons() {
    if (!m_manager || !m_previousButton || !m_nextButton)
        return;

    int currentStepNum = static_cast<int>(m_manager->currentStep()) + 1;
    int totalSteps = m_manager->getTotalStepsCount();

    // Enable/disable previous button based on current step
    m_previousButton->setEnabled(currentStepNum > 1);

    // Update next button text based on whether this is the last step
    OnboardingStep currentStep = m_manager->currentStep();
    m_nextButton->setText(currentStep == OnboardingStep::Complete ? "Finish" : "Next");
}

QRect OnboardingWidget::calculateHighlightRect(QWidget* widget) const {
    if (!widget) {
        return QRect();
    }

    QRect rect = widget->geometry();

    // Convert to global coordinates if widget has a parent
    if (widget->parentWidget()) {
        QPoint globalTopLeft = widget->parentWidget()->mapToGlobal(rect.topLeft());
        QPoint localTopLeft = mapFromGlobal(globalTopLeft);
        rect.moveTopLeft(localTopLeft);
    }

    return rect;
}

void OnboardingWidget::drawOverlay(QPainter& painter) {
    // Draw semi-transparent overlay over entire widget
    painter.fillRect(rect(), QColor(0, 0, 0, int(255 * m_overlayOpacity * 0.6)));
}

void OnboardingWidget::drawHighlight(QPainter& painter) {
    if (!m_hasHighlight) {
        return;
    }

    // Draw spotlight effect on highlighted area
    drawSpotlight(painter, m_highlightArea);
}
