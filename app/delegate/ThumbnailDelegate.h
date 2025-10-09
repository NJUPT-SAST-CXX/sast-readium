#pragma once

#include <QColor>
#include <QEasingCurve>
#include <QFont>
#include <QHash>
#include <QModelIndex>
#include <QMutex>
#include <QObject>
#include <QPainter>
#include <QPersistentModelIndex>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QSize>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// 前向声明
class StyleManager;
enum class Theme;

/**
 * @brief Chrome风格的缩略图渲染委托
 *
 * 特性：
 * - Chrome浏览器风格的视觉设计
 * - 圆角边框和阴影效果
 * - 悬停和选中状态动画
 * - 加载指示器和错误状态显示
 * - 页码标签渲染
 * - 高DPI支持
 */
class ThumbnailDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit ThumbnailDelegate(QObject* parent = nullptr);
    ~ThumbnailDelegate() override;

    // QStyledItemDelegate接口
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

    // 自定义设置
    void setThumbnailSize(const QSize& size);
    QSize thumbnailSize() const { return m_thumbnailSize; }

    void setMargins(int margin);
    int margins() const { return m_margin; }

    void setBorderRadius(int radius);
    int borderRadius() const { return m_borderRadius; }

    void setShadowEnabled(bool enabled);
    bool shadowEnabled() const { return m_shadowEnabled; }

    void setAnimationEnabled(bool enabled);
    bool animationEnabled() const { return m_animationEnabled; }

    // 颜色主题
    void setLightTheme();
    void setDarkTheme();
    void setCustomColors(const QColor& background, const QColor& border,
                         const QColor& text, const QColor& accent);

protected:
    bool eventFilter(QObject* object, QEvent* event) override;

private slots:
    void onAnimationValueChanged();
    void onLoadingAnimationTimer();

private:
    struct AnimationState {
        qreal hoverOpacity = 0.0;
        qreal selectionOpacity = 0.0;
        int loadingAngle = 0;
        QPropertyAnimation* hoverAnimation = nullptr;
        QPropertyAnimation* selectionAnimation = nullptr;

        AnimationState() = default;
        ~AnimationState() {
            delete hoverAnimation;
            delete selectionAnimation;
        }
    };

    void paintThumbnail(QPainter* painter, const QRect& rect,
                        const QPixmap& pixmap,
                        const QStyleOptionViewItem& option) const;
    void paintBackground(QPainter* painter, const QRect& rect,
                         const QStyleOptionViewItem& option) const;
    void paintBorder(QPainter* painter, const QRect& rect,
                     const QStyleOptionViewItem& option) const;
    void paintShadow(QPainter* painter, const QRect& rect,
                     const QStyleOptionViewItem& option) const;
    void paintPageNumber(QPainter* painter, const QRect& rect, int pageNumber,
                         const QStyleOptionViewItem& option) const;
    void paintLoadingIndicator(QPainter* painter, const QRect& rect,
                               const QStyleOptionViewItem& option) const;
    void paintErrorIndicator(QPainter* painter, const QRect& rect,
                             const QString& errorMessage,
                             const QStyleOptionViewItem& option) const;

    QRect getThumbnailRect(const QRect& itemRect) const;
    QRect getPageNumberRect(const QRect& thumbnailRect) const;

    AnimationState* getAnimationState(const QModelIndex& index) const;
    void updateHoverState(const QModelIndex& index, bool hovered);
    void updateSelectionState(const QModelIndex& index, bool selected);

    void setupAnimations(AnimationState* state, const QModelIndex& index) const;
    void cleanupAnimations();

    // 性能优化方法
    Qt::TransformationMode getOptimalTransformationMode(
        const QSize& sourceSize, const QSize& targetSize) const;

private:
    // 尺寸设置
    QSize m_thumbnailSize;
    int m_margin;
    int m_borderRadius;
    int m_pageNumberHeight;

    // 视觉效果设置
    bool m_shadowEnabled;
    bool m_animationEnabled;
    int m_shadowBlurRadius;
    int m_shadowOffset;
    int m_borderWidth;

    // 颜色主题
    QColor m_backgroundColor;
    QColor m_borderColorNormal;
    QColor m_borderColorHovered;
    QColor m_borderColorSelected;
    QColor m_shadowColor;
    QColor m_pageNumberBgColor;
    QColor m_pageNumberTextColor;
    QColor m_loadingColor;
    QColor m_errorColor;
    QColor m_placeholderColor;

    // 动画管理
    mutable QHash<QPersistentModelIndex, AnimationState*> m_animationStates;
    QTimer* m_loadingTimer;

    // 字体
    QFont m_pageNumberFont;
    QFont m_errorFont;

    // 常量
    static constexpr int DEFAULT_THUMBNAIL_WIDTH = 120;
    static constexpr int DEFAULT_THUMBNAIL_HEIGHT = 160;
    static constexpr int DEFAULT_MARGIN = 8;
    static constexpr int DEFAULT_BORDER_RADIUS = 8;
    static constexpr int DEFAULT_PAGE_NUMBER_HEIGHT = 24;
    static constexpr int DEFAULT_SHADOW_BLUR_RADIUS = 12;
    static constexpr int DEFAULT_SHADOW_OFFSET = 2;
    static constexpr int DEFAULT_BORDER_WIDTH = 2;
    static constexpr int LOADING_SPINNER_SIZE = 24;
    static constexpr int LOADING_ANIMATION_INTERVAL = 50;     // ms
    static constexpr int HOVER_ANIMATION_DURATION = 200;      // ms
    static constexpr int SELECTION_ANIMATION_DURATION = 300;  // ms

    // Chrome风格颜色常量
    static const QColor GOOGLE_BLUE;
    static const QColor GOOGLE_BLUE_DARK;
    static const QColor GOOGLE_RED;
    static const QColor LIGHT_BACKGROUND;
    static const QColor LIGHT_BORDER;
    static const QColor LIGHT_TEXT;
    static const QColor DARK_BACKGROUND;
    static const QColor DARK_BORDER;
    static const QColor DARK_TEXT;
};
