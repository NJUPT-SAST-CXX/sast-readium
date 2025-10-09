#pragma once

#include <QColor>
#include <QContextMenuEvent>
#include <QEnterEvent>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QRect>
#include <QSize>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore>
#include <QtGui>
#include <QtWidgets>

/**
 * @brief Chrome风格的PDF页面缩略图组件
 *
 * 特性：
 * - Chrome浏览器风格的视觉设计
 * - 圆角边框和阴影效果
 * - 悬停和选中状态动画
 * - 页码显示
 * - 异步图片加载支持
 */
class ThumbnailWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal shadowOpacity READ shadowOpacity WRITE setShadowOpacity)
    Q_PROPERTY(qreal borderOpacity READ borderOpacity WRITE setBorderOpacity)

public:
    enum State { Normal, Hovered, Selected, Loading, Error };

    explicit ThumbnailWidget(int pageNumber = 0, QWidget* parent = nullptr);
    ~ThumbnailWidget() override;

    // 基础属性
    void setPageNumber(int pageNumber);
    int pageNumber() const { return m_pageNumber; }

    void setPixmap(const QPixmap& pixmap);
    QPixmap pixmap() const { return m_pixmap; }

    void setState(State state);
    State state() const { return m_state; }

    // 尺寸设置
    void setThumbnailSize(const QSize& size);
    QSize thumbnailSize() const { return m_thumbnailSize; }

    // 动画属性
    qreal shadowOpacity() const { return m_shadowOpacity; }
    void setShadowOpacity(qreal opacity);

    qreal borderOpacity() const { return m_borderOpacity; }
    void setBorderOpacity(qreal opacity);

    // 加载状态
    void setLoading(bool loading);
    bool isLoading() const { return m_state == Loading; }

    void setError(const QString& errorMessage = QString());
    bool hasError() const { return m_state == Error; }

signals:
    void clicked(int pageNumber);
    void doubleClicked(int pageNumber);
    void rightClicked(int pageNumber, const QPoint& globalPos);
    void hoverEntered(int pageNumber);
    void hoverLeft(int pageNumber);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private slots:
    void onHoverAnimationFinished();
    void onSelectionAnimationFinished();
    void updateLoadingAnimation();

private:
    void setupUI();
    void setupAnimations();
    void updateShadowEffect();
    void updateBorderEffect();
    void drawThumbnail(QPainter& painter, const QRect& rect);
    void drawPageNumber(QPainter& painter, const QRect& rect);
    void drawLoadingIndicator(QPainter& painter, const QRect& rect);
    void drawErrorIndicator(QPainter& painter, const QRect& rect);
    void drawBorder(QPainter& painter, const QRect& rect);
    void drawShadow(QPainter& painter, const QRect& rect);

    QRect getThumbnailRect() const;
    QRect getPageNumberRect() const;

private:
    // 基础数据
    int m_pageNumber;
    QPixmap m_pixmap;
    State m_state;
    QSize m_thumbnailSize;
    QString m_errorMessage;

    // 动画属性
    qreal m_shadowOpacity;
    qreal m_borderOpacity;
    int m_loadingAngle;

    // 动画对象
    QPropertyAnimation* m_hoverAnimation;
    QPropertyAnimation* m_selectionAnimation;
    QPropertyAnimation* m_shadowAnimation;
    QPropertyAnimation* m_borderAnimation;
    QTimer* m_loadingTimer;

    // 视觉效果
    QGraphicsDropShadowEffect* m_shadowEffect;

    // 样式常量
    static constexpr int DEFAULT_THUMBNAIL_WIDTH = 120;
    static constexpr int DEFAULT_THUMBNAIL_HEIGHT = 160;
    static constexpr int PAGE_NUMBER_HEIGHT = 24;
    static constexpr int BORDER_RADIUS = 8;
    static constexpr int SHADOW_BLUR_RADIUS = 12;
    static constexpr int SHADOW_OFFSET = 2;
    static constexpr int BORDER_WIDTH = 2;
    static constexpr int MARGIN = 8;
    static constexpr int LOADING_SPINNER_SIZE = 24;

    // 颜色常量
    static const QColor BORDER_COLOR_NORMAL;
    static const QColor BORDER_COLOR_HOVERED;
    static const QColor BORDER_COLOR_SELECTED;
    static const QColor SHADOW_COLOR;
    static const QColor PAGE_NUMBER_BG_COLOR;
    static const QColor PAGE_NUMBER_TEXT_COLOR;
    static const QColor LOADING_COLOR;
    static const QColor ERROR_COLOR;
};
