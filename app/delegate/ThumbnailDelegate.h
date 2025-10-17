#pragma once

#include <QColor>
#include <QSize>
#include <QStyledItemDelegate>
#include <memory>

// 前向声明
class QObject;
class QPainter;
class QStyleOptionViewItem;
class QModelIndex;
class StyleManager;
enum class Theme : std::uint8_t;

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

    // Disable copy and move (Qt parent-child ownership handles lifetime)
    ThumbnailDelegate(const ThumbnailDelegate&) = delete;
    ThumbnailDelegate& operator=(const ThumbnailDelegate&) = delete;
    ThumbnailDelegate(ThumbnailDelegate&&) = delete;
    ThumbnailDelegate& operator=(ThumbnailDelegate&&) = delete;

    // QStyledItemDelegate接口
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const override;

    // 自定义设置
    void setThumbnailSize(const QSize& size);
    [[nodiscard]] QSize thumbnailSize() const;

    void setMargins(int margin);
    [[nodiscard]] int margins() const;

    void setBorderRadius(int radius);
    [[nodiscard]] int borderRadius() const;

    void setShadowEnabled(bool enabled);
    [[nodiscard]] bool shadowEnabled() const;

    void setAnimationEnabled(bool enabled);
    [[nodiscard]] bool animationEnabled() const;

    // 颜色主题
    void setLightTheme();
    void setDarkTheme();
    void setCustomColors(const QColor& background, const QColor& border,
                         const QColor& text, const QColor& accent);

    // 性能优化控制
    void setRenderCacheEnabled(bool enabled);
    [[nodiscard]] bool isRenderCacheEnabled() const;

    void setHighQualityRenderingEnabled(bool enabled);
    [[nodiscard]] bool isHighQualityRenderingEnabled() const;

    void setAntiAliasingEnabled(bool enabled);
    [[nodiscard]] bool isAntiAliasingEnabled() const;

    // 缓存管理
    void clearRenderCache();
    void setMaxCacheSize(int size);
    [[nodiscard]] int maxCacheSize() const;

    // 性能监控
    [[nodiscard]] double averagePaintTime() const;
    [[nodiscard]] double cacheHitRate() const;
    [[nodiscard]] int totalPaintCalls() const;
    void resetPerformanceStats();

protected:
    bool eventFilter(QObject* object, QEvent* event) override;

private slots:
    void onAnimationValueChanged();
    void onLoadingAnimationTimer();

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};
