#pragma once

#include <QWidget>
#include <QPropertyAnimation>
#include <QTimer>
#include <QPainter>
#include <QColor>

/**
 * @brief SkeletonWidget - A loading placeholder widget with shimmer animation
 * 
 * This widget provides a modern skeleton screen loading indicator that shows
 * the approximate layout of content while it's loading, improving perceived
 * performance compared to traditional spinners.
 * 
 * Features:
 * - Smooth shimmer animation
 * - Customizable shape (rectangle, circle, text line)
 * - Theme-aware colors
 * - Configurable animation speed
 * 
 * Usage:
 * @code
 * auto* skeleton = new SkeletonWidget(SkeletonWidget::Shape::Rectangle);
 * skeleton->setFixedSize(200, 100);
 * skeleton->startAnimation();
 * @endcode
 */
class SkeletonWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal shimmerPosition READ shimmerPosition WRITE setShimmerPosition)

public:
    enum class Shape {
        Rectangle,  // Standard rectangular skeleton
        Circle,     // Circular skeleton (for avatars, icons)
        TextLine,   // Text line skeleton (thin rectangle)
        Custom      // Custom shape (override paintEvent)
    };

    explicit SkeletonWidget(Shape shape = Shape::Rectangle, QWidget* parent = nullptr);
    ~SkeletonWidget() override;

    // Animation control
    void startAnimation();
    void stopAnimation();
    bool isAnimating() const { return m_isAnimating; }

    // Configuration
    void setShape(Shape shape);
    Shape shape() const { return m_shape; }

    void setAnimationDuration(int ms);
    int animationDuration() const { return m_animationDuration; }

    void setCornerRadius(int radius);
    int cornerRadius() const { return m_cornerRadius; }

    // Shimmer effect
    qreal shimmerPosition() const { return m_shimmerPosition; }
    void setShimmerPosition(qreal position);

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void setupAnimation();
    void drawRectangle(QPainter& painter);
    void drawCircle(QPainter& painter);
    void drawTextLine(QPainter& painter);
    void drawShimmer(QPainter& painter, const QRect& rect);

    QColor getBaseColor() const;
    QColor getShimmerColor() const;

    Shape m_shape;
    QPropertyAnimation* m_shimmerAnimation;
    qreal m_shimmerPosition;
    int m_animationDuration;
    int m_cornerRadius;
    bool m_isAnimating;
};

/**
 * @brief DocumentSkeletonWidget - Skeleton for document loading
 * 
 * Provides a skeleton screen specifically designed for document loading,
 * showing a placeholder that resembles a document page.
 */
class DocumentSkeletonWidget : public QWidget {
    Q_OBJECT

public:
    explicit DocumentSkeletonWidget(QWidget* parent = nullptr);
    ~DocumentSkeletonWidget() override;

    void startAnimation();
    void stopAnimation();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void setupLayout();

    SkeletonWidget* m_headerSkeleton;
    SkeletonWidget* m_contentSkeleton1;
    SkeletonWidget* m_contentSkeleton2;
    SkeletonWidget* m_contentSkeleton3;
};

/**
 * @brief ThumbnailSkeletonWidget - Skeleton for thumbnail loading
 * 
 * Provides a skeleton screen specifically designed for thumbnail loading,
 * showing a placeholder that resembles a thumbnail with page number.
 */
class ThumbnailSkeletonWidget : public QWidget {
    Q_OBJECT

public:
    explicit ThumbnailSkeletonWidget(QWidget* parent = nullptr);
    ~ThumbnailSkeletonWidget() override;

    void startAnimation();
    void stopAnimation();

    void setThumbnailSize(const QSize& size);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void setupLayout();

    SkeletonWidget* m_thumbnailSkeleton;
    SkeletonWidget* m_pageNumberSkeleton;
    QSize m_thumbnailSize;
};

