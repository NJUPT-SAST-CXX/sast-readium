#include "TutorialCard.h"
#include <QApplication>
#include <QEnterEvent>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QStyleOption>
#include <QVBoxLayout>

// ElaWidgetTools
#include "ElaPushButton.h"
#include "ElaText.h"

TutorialCard::TutorialCard(const QString& id, const QString& title,
                           const QString& description, const QIcon& icon,
                           QWidget* parent)
    : QWidget(parent),
      m_tutorialId(id),
      m_title(title),
      m_description(description),
      m_icon(icon),
      m_isCompleted(false),
      m_hoverOpacity(1.0),
      m_isHovered(false),
      m_isPressed(false) {
    setFixedSize(CARD_WIDTH, CARD_HEIGHT);
    initializeUI();
    setupLayout();
    setupAnimations();
    updateCompletedState();
}

TutorialCard::~TutorialCard() = default;

void TutorialCard::setCompleted(bool completed) {
    if (m_isCompleted != completed) {
        m_isCompleted = completed;
        updateCompletedState();
    }
}

void TutorialCard::setDuration(const QString& duration) {
    m_duration = duration;
    if (m_durationLabel) {
        m_durationLabel->setText(duration);
    }
}

void TutorialCard::setDifficulty(const QString& difficulty) {
    m_difficulty = difficulty;
    if (m_difficultyLabel) {
        m_difficultyLabel->setText(difficulty);
    }
}

void TutorialCard::setHoverOpacity(qreal opacity) {
    m_hoverOpacity = opacity;
    update();
}

void TutorialCard::enterEvent(QEnterEvent* event) {
    Q_UNUSED(event)
    m_isHovered = true;
    if (m_hoverAnimation) {
        m_hoverAnimation->setDirection(QAbstractAnimation::Forward);
        m_hoverAnimation->start();
    }
}

void TutorialCard::leaveEvent(QEvent* event) {
    Q_UNUSED(event)
    m_isHovered = false;
    if (m_hoverAnimation) {
        m_hoverAnimation->setDirection(QAbstractAnimation::Backward);
        m_hoverAnimation->start();
    }
}

void TutorialCard::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isPressed = true;
        update();
    }
    QWidget::mousePressEvent(event);
}

void TutorialCard::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_isPressed) {
        m_isPressed = false;
        update();

        if (rect().contains(event->pos())) {
            emit clicked(m_tutorialId);
        }
    }
    QWidget::mouseReleaseEvent(event);
}

void TutorialCard::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw card background
    QRect cardRect = rect().adjusted(2, 2, -2, -2);
    painter.setBrush(QColor(255, 255, 255, int(255 * m_hoverOpacity)));
    painter.setPen(QPen(QColor(200, 200, 200), 1));
    painter.drawRoundedRect(cardRect, BORDER_RADIUS, BORDER_RADIUS);

    // Draw pressed effect
    if (m_isPressed) {
        painter.setBrush(QColor(0, 0, 0, 20));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(cardRect, BORDER_RADIUS, BORDER_RADIUS);
    }
}

void TutorialCard::initializeUI() {
    // Create UI components
    m_iconLabel = new QLabel(this);
    m_titleLabel = new ElaText(m_title, this);
    m_descriptionLabel = new ElaText(m_description, this);
    m_durationLabel = new ElaText(m_duration, this);
    m_difficultyLabel = new ElaText(m_difficulty, this);
    m_completedLabel = new ElaText(this);
    m_startButton = new ElaPushButton("Start Tutorial", this);

    // Configure icon
    if (!m_icon.isNull()) {
        m_iconLabel->setPixmap(m_icon.pixmap(ICON_SIZE, ICON_SIZE));
    }
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setFixedSize(ICON_SIZE, ICON_SIZE);

    // Configure title
    m_titleLabel->setStyleSheet(
        "font-weight: bold; font-size: 14px; color: #333;");
    m_titleLabel->setWordWrap(true);

    // Configure description
    m_descriptionLabel->setStyleSheet("font-size: 12px; color: #666;");
    m_descriptionLabel->setWordWrap(true);

    // Configure metadata labels
    m_durationLabel->setStyleSheet("font-size: 10px; color: #888;");
    m_difficultyLabel->setStyleSheet("font-size: 10px; color: #888;");
    m_completedLabel->setStyleSheet(
        "font-size: 10px; color: #4CAF50; font-weight: bold;");

    // Configure button
    m_startButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #2196F3;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 6px 12px;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #0D47A1;"
        "}");

    // Connect signals
    connect(m_startButton, &QPushButton::clicked,
            [this]() { emit clicked(m_tutorialId); });
}

void TutorialCard::setupLayout() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // Header with icon and title
    auto* headerLayout = new QHBoxLayout();
    headerLayout->addWidget(m_iconLabel);
    headerLayout->addWidget(m_titleLabel, 1);
    headerLayout->addWidget(m_completedLabel);

    // Metadata
    auto* metaLayout = new QHBoxLayout();
    metaLayout->addWidget(m_durationLabel);
    metaLayout->addStretch();
    metaLayout->addWidget(m_difficultyLabel);

    // Main layout
    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(m_descriptionLabel, 1);
    mainLayout->addLayout(metaLayout);
    mainLayout->addWidget(m_startButton);
}

void TutorialCard::setupAnimations() {
    m_hoverAnimation = new QPropertyAnimation(this, "hoverOpacity", this);
    m_hoverAnimation->setDuration(200);
    m_hoverAnimation->setStartValue(1.0);
    m_hoverAnimation->setEndValue(0.9);

    // Add drop shadow effect
    auto* shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(10);
    shadowEffect->setColor(QColor(0, 0, 0, 50));
    shadowEffect->setOffset(0, 2);
    setGraphicsEffect(shadowEffect);
}

void TutorialCard::updateCompletedState() {
    if (m_isCompleted) {
        m_completedLabel->setText("✓ Completed");
        m_completedLabel->show();
        m_startButton->setText("Review");
    } else {
        m_completedLabel->hide();
        m_startButton->setText("Start Tutorial");
    }
}

void TutorialCard::applyTheme() {
    // Apply theme-aware styling using current palette

    // For now, just update the card to use current palette
    QPalette palette = qApp->palette();

    // Update title label
    if (m_titleLabel) {
        QString titleStyle =
            QString("font-weight: bold; font-size: 14px; color: %1;")
                .arg(palette.color(QPalette::WindowText).name());
        m_titleLabel->setStyleSheet(titleStyle);
    }

    // Update description label
    if (m_descriptionLabel) {
        QString descStyle = QString("font-size: 12px; color: %1;")
                                .arg(palette.color(QPalette::Mid).name());
        m_descriptionLabel->setStyleSheet(descStyle);
    }

    // Update metadata labels
    if (m_durationLabel) {
        QString metaStyle = QString("font-size: 10px; color: %1;")
                                .arg(palette.color(QPalette::Dark).name());
        m_durationLabel->setStyleSheet(metaStyle);
    }

    if (m_difficultyLabel) {
        QString metaStyle = QString("font-size: 10px; color: %1;")
                                .arg(palette.color(QPalette::Dark).name());
        m_difficultyLabel->setStyleSheet(metaStyle);
    }

    // Completed label keeps its green color
    if (m_completedLabel) {
        m_completedLabel->setStyleSheet(
            "font-size: 10px; color: #4CAF50; font-weight: bold;");
    }

    // Update button with theme-aware colors
    if (m_startButton) {
        QColor accentColor = palette.color(QPalette::Highlight);
        QColor accentHover = accentColor.lighter(110);
        QColor accentPressed = accentColor.darker(110);

        QString buttonStyle = QString(
                                  "QPushButton {"
                                  "    background-color: %1;"
                                  "    color: white;"
                                  "    border: none;"
                                  "    border-radius: 4px;"
                                  "    padding: 6px 12px;"
                                  "    font-size: 12px;"
                                  "}"
                                  "QPushButton:hover {"
                                  "    background-color: %2;"
                                  "}"
                                  "QPushButton:pressed {"
                                  "    background-color: %3;"
                                  "}")
                                  .arg(accentColor.name(), accentHover.name(),
                                       accentPressed.name());

        m_startButton->setStyleSheet(buttonStyle);
    }

    // Trigger repaint
    update();
}
