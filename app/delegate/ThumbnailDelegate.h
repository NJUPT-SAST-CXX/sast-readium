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
    QSize thumbnailSize() const;

    void setMargins(int margin);
    int margins() const;

    void setBorderRadius(int radius);
    int borderRadius() const;

    void setShadowEnabled(bool enabled);
    bool shadowEnabled() const;

    void setAnimationEnabled(bool enabled);
    bool animationEnabled() const;

    // 颜色主题
    void setLightTheme();
    void setDarkTheme();
    void setCustomColors(const QColor& background, const QColor& border,
                         const QColor& text, const QColor& accent);

    // 性能优化控制
    void setRenderCacheEnabled(bool enabled);
    bool isRenderCacheEnabled() const;

    void setHighQualityRenderingEnabled(bool enabled);
    bool isHighQualityRenderingEnabled() const;

    void setAntiAliasingEnabled(bool enabled);
    bool isAntiAliasingEnabled() const;

    // 缓存管理
    void clearRenderCache();
    void setMaxCacheSize(int size);
    int maxCacheSize() const;

    // 性能监控
    double averagePaintTime() const;
    double cacheHitRate() const;
    int totalPaintCalls() const;
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
