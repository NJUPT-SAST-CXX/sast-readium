#include "ToastNotification.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QVBoxLayout>
#include "../../managers/StyleManager.h"

// ToastNotification Implementation
ToastNotification::ToastNotification(QWidget* parent)
    : QWidget(parent),
      m_iconLabel(nullptr),
      m_messageLabel(nullptr),
      m_actionButton(nullptr),
      m_closeButton(nullptr),
      m_fadeInAnimation(nullptr),
      m_fadeOutAnimation(nullptr),
      m_opacityEffect(nullptr),
      m_dismissTimer(nullptr),
      m_type(Type::Info),
      m_position(Position::BottomCenter),
      m_duration(3000),
      m_actionCallback(nullptr),
      m_isShowing(false) {
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint |
                   Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    setupUI();
    setupAnimations();

    // Install event filter on parent to handle resize
    if (parent) {
        parent->installEventFilter(this);
    }
}

ToastNotification::~ToastNotification() {
    if (m_dismissTimer) {
        m_dismissTimer->stop();
    }
}

void ToastNotification::setupUI() {
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(STYLE.spacingMD(), STYLE.spacingSM(),
                                   STYLE.spacingMD(), STYLE.spacingSM());
    mainLayout->setSpacing(STYLE.spacingSM());

    // Icon label
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(24, 24);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_iconLabel);

    // Message label
    m_messageLabel = new QLabel(this);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setFont(STYLE.defaultFont());
    mainLayout->addWidget(m_messageLabel, 1);

    // Action button (hidden by default)
    m_actionButton = new QPushButton(this);
    m_actionButton->setFont(STYLE.buttonFont());
    m_actionButton->setVisible(false);
    m_actionButton->setCursor(Qt::PointingHandCursor);
    connect(m_actionButton, &QPushButton::clicked, this, [this]() {
        if (m_actionCallback) {
            m_actionCallback();
            emit actionTriggered();
        }
        hideNotification();
    });
    mainLayout->addWidget(m_actionButton);

    // Close button
    m_closeButton = new QPushButton("×", this);
    m_closeButton->setFixedSize(24, 24);
    m_closeButton->setCursor(Qt::PointingHandCursor);
    m_closeButton->setFlat(true);
    connect(m_closeButton, &QPushButton::clicked, this,
            &ToastNotification::hideNotification);
    mainLayout->addWidget(m_closeButton);

    // Set minimum and maximum size
    setMinimumWidth(300);
    setMaximumWidth(600);
    setMinimumHeight(48);

    updateStyle();
}

void ToastNotification::setupAnimations() {
    // Opacity effect
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(m_opacityEffect);

    // Fade in animation
    m_fadeInAnimation =
        new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_fadeInAnimation->setDuration(STYLE.animationNormal());
    m_fadeInAnimation->setStartValue(0.0);
    m_fadeInAnimation->setEndValue(1.0);
    m_fadeInAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // Fade out animation
    m_fadeOutAnimation =
        new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_fadeOutAnimation->setDuration(STYLE.animationNormal());
    m_fadeOutAnimation->setStartValue(1.0);
    m_fadeOutAnimation->setEndValue(0.0);
    m_fadeOutAnimation->setEasingCurve(QEasingCurve::InCubic);

    connect(m_fadeOutAnimation, &QPropertyAnimation::finished, this, [this]() {
        hide();
        emit dismissed();
        deleteLater();
    });

    // Auto-dismiss timer
    m_dismissTimer = new QTimer(this);
    m_dismissTimer->setSingleShot(true);
    connect(m_dismissTimer, &QTimer::timeout, this,
            &ToastNotification::hideNotification);
}

void ToastNotification::setMessage(const QString& message) {
    if (m_messageLabel) {
        m_messageLabel->setText(message);
        adjustSize();
        updatePosition();
    }
}

void ToastNotification::setType(Type type) {
    if (m_type != type) {
        m_type = type;
        updateStyle();
    }
}

void ToastNotification::setDuration(int ms) { m_duration = ms; }

void ToastNotification::setPosition(Position position) {
    if (m_position != position) {
        m_position = position;
        updatePosition();
    }
}

void ToastNotification::setActionButton(const QString& text,
                                        std::function<void()> callback) {
    if (m_actionButton) {
        m_actionButton->setText(text);
        m_actionButton->setVisible(!text.isEmpty());
        m_actionCallback = callback;
        adjustSize();
        updatePosition();
    }
}

void ToastNotification::showNotification() {
    if (m_isShowing) {
        return;
    }

    m_isShowing = true;
    updatePosition();
    QWidget::show();
    raise();

    // Start fade-in animation
    m_fadeInAnimation->start();

    // Start auto-dismiss timer
    if (m_duration > 0) {
        m_dismissTimer->start(m_duration);
    }
}

void ToastNotification::hideNotification() {
    if (!m_isShowing) {
        return;
    }

    m_isShowing = false;

    // Stop dismiss timer
    if (m_dismissTimer) {
        m_dismissTimer->stop();
    }

    // Start fade-out animation
    m_fadeOutAnimation->start();
}

qreal ToastNotification::opacity() const {
    return m_opacityEffect ? m_opacityEffect->opacity() : 1.0;
}

void ToastNotification::setOpacity(qreal opacity) {
    if (m_opacityEffect) {
        m_opacityEffect->setOpacity(opacity);
    }
}

void ToastNotification::updatePosition() {
    if (!parentWidget()) {
        return;
    }

    QWidget* parent = parentWidget();
    QRect parentRect = parent->rect();
    QSize toastSize = sizeHint();

    int x = 0, y = 0;
    int margin = STYLE.spacingMD();

    switch (m_position) {
        case Position::BottomCenter:
            x = (parentRect.width() - toastSize.width()) / 2;
            y = parentRect.height() - toastSize.height() - margin;
            break;
        case Position::BottomLeft:
            x = margin;
            y = parentRect.height() - toastSize.height() - margin;
            break;
        case Position::BottomRight:
            x = parentRect.width() - toastSize.width() - margin;
            y = parentRect.height() - toastSize.height() - margin;
            break;
        case Position::TopCenter:
            x = (parentRect.width() - toastSize.width()) / 2;
            y = margin;
            break;
        case Position::TopLeft:
            x = margin;
            y = margin;
            break;
        case Position::TopRight:
            x = parentRect.width() - toastSize.width() - margin;
            y = margin;
            break;
    }

    // Convert to global coordinates
    QPoint globalPos = parent->mapToGlobal(QPoint(x, y));
    move(globalPos);
}

void ToastNotification::updateStyle() {
    QColor bgColor = getBackgroundColor();
    QColor textColor = getTextColor();
    QString icon = getIcon();

    // Update icon
    if (m_iconLabel) {
        m_iconLabel->setText(icon);
        m_iconLabel->setStyleSheet(
            QString("QLabel { color: %1; font-size: 18px; font-weight: bold; }")
                .arg(textColor.name()));
    }

    // Update message label
    if (m_messageLabel) {
        m_messageLabel->setStyleSheet(
            QString("QLabel { color: %1; }").arg(textColor.name()));
    }

    // Update action button
    if (m_actionButton) {
        m_actionButton->setStyleSheet(
            QString("QPushButton {"
                    "   background-color: transparent;"
                    "   color: %1;"
                    "   border: 1px solid %1;"
                    "   border-radius: %2px;"
                    "   padding: 4px 12px;"
                    "   font-weight: bold;"
                    "}"
                    "QPushButton:hover {"
                    "   background-color: rgba(255, 255, 255, 0.1);"
                    "}"
                    "QPushButton:pressed {"
                    "   background-color: rgba(255, 255, 255, 0.2);"
                    "}")
                .arg(textColor.name())
                .arg(STYLE.radiusSM()));
    }

    // Update close button
    if (m_closeButton) {
        m_closeButton->setStyleSheet(
            QString("QPushButton {"
                    "   background-color: transparent;"
                    "   color: %1;"
                    "   border: none;"
                    "   font-size: 20px;"
                    "   font-weight: bold;"
                    "}"
                    "QPushButton:hover {"
                    "   background-color: rgba(255, 255, 255, 0.1);"
                    "}")
                .arg(textColor.name()));
    }

    update();
}

QColor ToastNotification::getBackgroundColor() const {
    switch (m_type) {
        case Type::Success:
            return STYLE.successColor();
        case Type::Warning:
            return STYLE.warningColor();
        case Type::Error:
            return STYLE.errorColor();
        case Type::Info:
        default:
            return STYLE.infoColor();
    }
}

QColor ToastNotification::getTextColor() const {
    return QColor(255, 255, 255);  // White text for all types
}

QString ToastNotification::getIcon() const {
    switch (m_type) {
        case Type::Success:
            return "✓";
        case Type::Warning:
            return "⚠";
        case Type::Error:
            return "✕";
        case Type::Info:
        default:
            return "ℹ";
    }
}

void ToastNotification::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw rounded rectangle background
    QPainterPath path;
    path.addRoundedRect(rect(), STYLE.radiusLG(), STYLE.radiusLG());

    painter.fillPath(path, getBackgroundColor());

    // Draw subtle shadow
    painter.setPen(QPen(QColor(0, 0, 0, 30), 1));
    painter.drawPath(path);
}

void ToastNotification::mousePressEvent(QMouseEvent* event) {
    // Dismiss on click (except on buttons)
    if (event->button() == Qt::LeftButton) {
        QWidget* widget = childAt(event->pos());
        if (widget != m_actionButton && widget != m_closeButton) {
            hideNotification();
        }
    }
    QWidget::mousePressEvent(event);
}

bool ToastNotification::eventFilter(QObject* obj, QEvent* event) {
    if (obj == parentWidget() && event->type() == QEvent::Resize) {
        updatePosition();
    }
    return QWidget::eventFilter(obj, event);
}

// Static convenience methods
void ToastNotification::show(QWidget* parent, const QString& message, Type type,
                             int duration) {
    ToastManager::instance().showToast(parent, message, type, duration);
}

void ToastNotification::show(QWidget* parent, const QString& message, Type type,
                             int duration, const QString& actionText,
                             std::function<void()> actionCallback) {
    ToastManager::instance().showToast(parent, message, type, duration,
                                       actionText, actionCallback);
}

// ToastManager Implementation
ToastManager& ToastManager::instance() {
    static ToastManager instance;
    return instance;
}

ToastManager::ToastManager()
    : QObject(nullptr), m_currentToast(nullptr), m_isProcessing(false) {}

ToastManager::~ToastManager() { clearQueue(); }

void ToastManager::showToast(QWidget* parent, const QString& message,
                             ToastNotification::Type type, int duration) {
    ToastRequest request;
    request.parent = parent;
    request.message = message;
    request.type = type;
    request.duration = duration;
    request.hasAction = false;

    m_queue.append(request);
    processQueue();
}

void ToastManager::showToast(QWidget* parent, const QString& message,
                             ToastNotification::Type type, int duration,
                             const QString& actionText,
                             std::function<void()> actionCallback) {
    ToastRequest request;
    request.parent = parent;
    request.message = message;
    request.type = type;
    request.duration = duration;
    request.actionText = actionText;
    request.actionCallback = actionCallback;
    request.hasAction = true;

    m_queue.append(request);
    processQueue();
}

void ToastManager::clearQueue() {
    m_queue.clear();
    if (m_currentToast) {
        m_currentToast->hideNotification();
        m_currentToast = nullptr;
    }
}

int ToastManager::queueSize() const { return m_queue.size(); }

void ToastManager::processQueue() {
    // If already processing or queue is empty, return
    if (m_isProcessing || m_queue.isEmpty()) {
        return;
    }

    // If there's a current toast showing, wait for it to finish
    if (m_currentToast) {
        return;
    }

    m_isProcessing = true;

    // Get next request from queue
    ToastRequest request = m_queue.takeFirst();

    // Create and show toast
    m_currentToast = new ToastNotification(request.parent);
    m_currentToast->setMessage(request.message);
    m_currentToast->setType(request.type);
    m_currentToast->setDuration(request.duration);

    if (request.hasAction) {
        m_currentToast->setActionButton(request.actionText,
                                        request.actionCallback);
    }

    connect(m_currentToast, &ToastNotification::dismissed, this,
            &ToastManager::onToastDismissed);

    m_currentToast->showNotification();

    m_isProcessing = false;
}

void ToastManager::onToastDismissed() {
    m_currentToast = nullptr;

    // Process next toast in queue
    if (!m_queue.isEmpty()) {
        processQueue();
    }
}
