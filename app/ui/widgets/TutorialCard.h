#pragma once

#include <QIcon>
#include <QWidget>

class QLabel;
class QPushButton;
class QPropertyAnimation;

/**
 * TutorialCard
 *
 * An interactive card widget that displays tutorial information
 * and allows users to start guided tours of specific features.
 */
class TutorialCard : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal hoverOpacity READ hoverOpacity WRITE setHoverOpacity)

public:
    explicit TutorialCard(const QString& id, const QString& title,
                          const QString& description,
                          const QIcon& icon = QIcon(),
                          QWidget* parent = nullptr);
    ~TutorialCard();

    // Properties
    QString tutorialId() const { return m_tutorialId; }
    QString title() const { return m_title; }
    QString description() const { return m_description; }

    void setTitle(const QString& title);
    void setDescription(const QString& description);
    void setIcon(const QIcon& icon);
    void setDuration(const QString& duration);
    void setDifficulty(const QString& difficulty);
    void setCompleted(bool completed);

    bool isCompleted() const { return m_isCompleted; }

    // Theme
    void applyTheme();

    // Animation
    qreal hoverOpacity() const { return m_hoverOpacity; }
    void setHoverOpacity(qreal opacity);

signals:
    void clicked(const QString& tutorialId);
    void startRequested(const QString& tutorialId);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void initializeUI();
    void setupLayout();
    void setupAnimations();
    void updateCompletedState();

    // Data
    QString m_tutorialId;
    QString m_title;
    QString m_description;
    QString m_duration;
    QString m_difficulty;
    QIcon m_icon;
    bool m_isCompleted;

    // UI Components
    QLabel* m_iconLabel;
    QLabel* m_titleLabel;
    QLabel* m_descriptionLabel;
    QLabel* m_durationLabel;
    QLabel* m_difficultyLabel;
    QLabel* m_completedLabel;
    QPushButton* m_startButton;

    // Animation
    QPropertyAnimation* m_hoverAnimation;
    qreal m_hoverOpacity;
    bool m_isHovered;
    bool m_isPressed;

    // Style constants
    static const int CARD_WIDTH = 280;
    static const int CARD_HEIGHT = 180;
    static const int ICON_SIZE = 48;
    static const int BORDER_RADIUS = 8;
};
