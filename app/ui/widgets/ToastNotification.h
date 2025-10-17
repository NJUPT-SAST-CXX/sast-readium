#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <functional>

/**
 * @brief ToastNotification - Non-blocking notification widget
 * 
 * Provides a modern, non-intrusive notification system similar to Android's
 * Toast or Material Design's Snackbar. Notifications appear at the bottom
 * of the screen, auto-dismiss after a timeout, and can include actions.
 * 
 * Features:
 * - Multiple notification types (Info, Success, Warning, Error)
 * - Auto-dismiss with configurable timeout
 * - Optional action button
 * - Smooth fade-in/fade-out animations
 * - Queue management for multiple notifications
 * - Theme-aware styling
 * 
 * Usage:
 * @code
 * ToastNotification::show(parentWidget, "File saved successfully", 
 *                         ToastNotification::Type::Success);
 * 
 * // With action
 * ToastNotification::show(parentWidget, "File deleted", 
 *                         ToastNotification::Type::Info,
 *                         3000, "Undo", []() { 
 *                             // Undo action
 *                         });
 * @endcode
 */
class ToastNotification : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    enum class Type {
        Info,     // Blue - informational messages
        Success,  // Green - successful operations
        Warning,  // Orange - warnings
        Error     // Red - errors
    };

    enum class Position {
        BottomCenter,  // Default - bottom center of parent
        BottomLeft,    // Bottom left corner
        BottomRight,   // Bottom right corner
        TopCenter,     // Top center
        TopLeft,       // Top left corner
        TopRight       // Top right corner
    };

    explicit ToastNotification(QWidget* parent = nullptr);
    ~ToastNotification() override;

    // Static convenience methods
    static void show(QWidget* parent, const QString& message, 
                     Type type = Type::Info, int duration = 3000);
    
    static void show(QWidget* parent, const QString& message, 
                     Type type, int duration,
                     const QString& actionText, 
                     std::function<void()> actionCallback);

    // Configuration
    void setMessage(const QString& message);
    void setType(Type type);
    void setDuration(int ms);
    void setPosition(Position position);
    void setActionButton(const QString& text, std::function<void()> callback);

    // Display control
    void showNotification();
    void hideNotification();

    qreal opacity() const;
    void setOpacity(qreal opacity);

signals:
    void dismissed();
    void actionTriggered();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUI();
    void setupAnimations();
    void updatePosition();
    void updateStyle();
    QColor getBackgroundColor() const;
    QColor getTextColor() const;
    QString getIcon() const;

    // UI Components
    QLabel* m_iconLabel;
    QLabel* m_messageLabel;
    QPushButton* m_actionButton;
    QPushButton* m_closeButton;

    // Animation
    QPropertyAnimation* m_fadeInAnimation;
    QPropertyAnimation* m_fadeOutAnimation;
    QGraphicsOpacityEffect* m_opacityEffect;
    QTimer* m_dismissTimer;

    // Configuration
    Type m_type;
    Position m_position;
    int m_duration;
    std::function<void()> m_actionCallback;
    bool m_isShowing;
};

/**
 * @brief ToastManager - Manages toast notification queue
 * 
 * Singleton class that manages the display queue for toast notifications,
 * ensuring only one notification is shown at a time and queuing others.
 */
class ToastManager : public QObject {
    Q_OBJECT

public:
    static ToastManager& instance();

    void showToast(QWidget* parent, const QString& message, 
                   ToastNotification::Type type, int duration);
    
    void showToast(QWidget* parent, const QString& message, 
                   ToastNotification::Type type, int duration,
                   const QString& actionText, 
                   std::function<void()> actionCallback);

    void clearQueue();
    int queueSize() const;

private:
    ToastManager();
    ~ToastManager() override;
    ToastManager(const ToastManager&) = delete;
    ToastManager& operator=(const ToastManager&) = delete;

    void processQueue();
    void onToastDismissed();

    struct ToastRequest {
        QWidget* parent;
        QString message;
        ToastNotification::Type type;
        int duration;
        QString actionText;
        std::function<void()> actionCallback;
        bool hasAction;
    };

    QList<ToastRequest> m_queue;
    ToastNotification* m_currentToast;
    bool m_isProcessing;
};

// Convenience macros
#define TOAST_INFO(parent, message) \
    ToastNotification::show(parent, message, ToastNotification::Type::Info)

#define TOAST_SUCCESS(parent, message) \
    ToastNotification::show(parent, message, ToastNotification::Type::Success)

#define TOAST_WARNING(parent, message) \
    ToastNotification::show(parent, message, ToastNotification::Type::Warning)

#define TOAST_ERROR(parent, message) \
    ToastNotification::show(parent, message, ToastNotification::Type::Error)

