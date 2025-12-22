#pragma once

#include <QColor>
#include <QPainter>
#include <QPropertyAnimation>
#include <QWidget>

/**
 * @brief EnhancedFocusIndicator - Animated focus indicator for accessibility
 *
 * Provides a highly visible, animated focus indicator that improves keyboard
 * navigation accessibility. The indicator features:
 * - High contrast colors for visibility
 * - Smooth animations when focus changes
 * - Customizable appearance
 * - Automatic positioning around focused widget
 *
 * This widget is designed to be used as an overlay that follows the focused
 * widget, providing clear visual feedback for keyboard navigation.
 *
 * Usage:
 * @code
 * EnhancedFocusIndicator* indicator = new EnhancedFocusIndicator(parentWidget);
 * indicator->setTargetWidget(focusedWidget);
 * indicator->show();
 * @endcode
 */
class EnhancedFocusIndicator : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal borderOpacity READ borderOpacity WRITE setBorderOpacity)
    Q_PROPERTY(int borderWidth READ borderWidth WRITE setBorderWidth)

public:
    static EnhancedFocusIndicator& instance() {
        static EnhancedFocusIndicator instance;
        return instance;
    }

    enum class Style {
        Solid,    // Solid border
        Dashed,   // Dashed border
        Glow,     // Glowing effect
        Animated  // Animated border (moving dashes)
    };

    explicit EnhancedFocusIndicator(QWidget* parent = nullptr);
    ~EnhancedFocusIndicator() override;

    // Target widget management
    void setTargetWidget(QWidget* widget);
    QWidget* targetWidget() const { return m_targetWidget; }

    // Appearance configuration
    void setStyle(Style style);
    Style style() const { return m_style; }

    void setFocusColor(const QColor& color);
    QColor focusColor() const { return m_focusColor; }

    void setBorderThickness(int thickness);
    int borderThickness() const { return m_borderThickness; }

    void setAnimationDuration(int ms);
    int animationDuration() const { return m_animationDuration; }

    // Animation control
    void showIndicator();
    void hideIndicator();
    void updatePosition();

    // Property accessors
    qreal borderOpacity() const { return m_borderOpacity; }
    void setBorderOpacity(qreal opacity);

    int borderWidth() const { return m_borderWidth; }
    void setBorderWidth(int width);

protected:
    void paintEvent(QPaintEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupAnimations();
    void animateShow();
    void animateHide();
    void drawSolidBorder(QPainter& painter);
    void drawDashedBorder(QPainter& painter);
    void drawGlowBorder(QPainter& painter);
    void drawAnimatedBorder(QPainter& painter);

    QWidget* m_targetWidget;
    Style m_style;
    QColor m_focusColor;
    int m_borderThickness;
    int m_animationDuration;

    // Animation properties
    QPropertyAnimation* m_showAnimation;
    QPropertyAnimation* m_hideAnimation;
    QPropertyAnimation* m_pulseAnimation;
    qreal m_borderOpacity;
    int m_borderWidth;
    qreal m_animationPhase;  // For animated border style

    bool m_isVisible;
};

/**
 * @brief FocusManager - Global focus indicator manager
 *
 * Singleton class that manages a global focus indicator, automatically
 * showing it when widgets receive keyboard focus and hiding it when
 * focus is lost.
 *
 * This provides a consistent focus indication across the entire application.
 */
class FocusManager : public QObject {
    Q_OBJECT

public:
    static FocusManager& instance();

    // Enable/disable global focus indicator
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    // Configure indicator appearance
    void setIndicatorStyle(EnhancedFocusIndicator::Style style);
    void setIndicatorColor(const QColor& color);
    void setIndicatorThickness(int thickness);

    // Install on application
    void installOnApplication();
    void uninstallFromApplication();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    FocusManager();
    ~FocusManager() override;
    FocusManager(const FocusManager&) = delete;
    FocusManager& operator=(const FocusManager&) = delete;

    void onFocusChanged(QWidget* old, QWidget* now);
    bool shouldShowIndicatorFor(QWidget* widget) const;

    EnhancedFocusIndicator* m_indicator;
    QWidget* m_currentFocusWidget;
    bool m_enabled;
    bool m_installed;

    // Configuration
    EnhancedFocusIndicator::Style m_style;
    QColor m_color;
    int m_thickness;
};
