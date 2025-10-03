#pragma once

#include <QAction>
#include <QGraphicsOpacityEffect>
#include <QListView>
#include <QMenu>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QTimer>
#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <memory>

class ThumbnailModel;
class ThumbnailDelegate;

/**
 * @brief Chrome风格的PDF缩略图列表视图
 *
 * 特性：
 * - 基于QListView的高性能虚拟滚动
 * - Chrome浏览器风格的视觉设计
 * - 智能懒加载和预加载
 * - 流畅的滚动动画
 * - 右键菜单支持
 * - 键盘导航
 * - 拖拽支持（可选）
 */
class ThumbnailListView : public QListView {
    Q_OBJECT

public:
    explicit ThumbnailListView(QWidget* parent = nullptr);
    ~ThumbnailListView() override;

    // 模型和委托
    void setThumbnailModel(ThumbnailModel* model);
    ThumbnailModel* thumbnailModel() const;

    void setThumbnailDelegate(ThumbnailDelegate* delegate);
    ThumbnailDelegate* thumbnailDelegate() const;

    // 缩略图设置
    void setThumbnailSize(const QSize& size);
    QSize thumbnailSize() const;

    void setThumbnailSpacing(int spacing);
    int thumbnailSpacing() const;

    // 滚动和导航
    void scrollToPage(int pageNumber, bool animated = true);
    void scrollToTop(bool animated = true);
    void scrollToBottom(bool animated = true);

    int currentPage() const;
    void setCurrentPage(int pageNumber, bool animated = true);

    // 选择管理
    void selectPage(int pageNumber);
    void selectPages(const QList<int>& pageNumbers);
    void clearSelection();
    QList<int> selectedPages() const;

    // 视觉效果
    void setAnimationEnabled(bool enabled);
    bool animationEnabled() const { return m_animationEnabled; }

    void setSmoothScrolling(bool enabled);
    bool smoothScrolling() const { return m_smoothScrolling; }

    void setFadeInEnabled(bool enabled);
    bool fadeInEnabled() const { return m_fadeInEnabled; }

    // 预加载控制
    void setPreloadMargin(int margin);
    int preloadMargin() const { return m_preloadMargin; }

    void setAutoPreload(bool enabled);
    bool autoPreload() const { return m_autoPreload; }

    // 右键菜单
    void setContextMenuEnabled(bool enabled);
    bool contextMenuEnabled() const { return m_contextMenuEnabled; }

    void addContextMenuAction(QAction* action);
    void removeContextMenuAction(QAction* action);
    void clearContextMenuActions();

signals:
    void pageClicked(int pageNumber);
    void pageDoubleClicked(int pageNumber);
    void pageRightClicked(int pageNumber, const QPoint& globalPos);
    void currentPageChanged(int pageNumber);
    void pageSelectionChanged(const QList<int>& selectedPages);
    void scrollPositionChanged(int position, int maximum);
    void visibleRangeChanged(int firstVisible, int lastVisible);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onScrollBarValueChanged(int value);
    void onScrollBarRangeChanged(int min, int max);
    void onModelDataChanged(const QModelIndex& topLeft,
                            const QModelIndex& bottomRight);
    void onModelRowsInserted(const QModelIndex& parent, int first, int last);
    void onModelRowsRemoved(const QModelIndex& parent, int first, int last);
    void onScrollAnimationFinished();
    void onPreloadTimer();
    void onFadeInTimer();
    void updateVisibleRange();
    void scheduleViewportUpdate();
    void optimizedUpdateVisibleRange();

private:
    void setupUI();
    void setupScrollBars();
    void setupAnimations();
    void setupContextMenu();
    void connectSignals();

    void updateItemSizes();
    void updatePreloadRange();
    void updateScrollBarStyle();

    void animateScrollTo(int position);
    void stopScrollAnimation();

    void fadeInVisibleItems();
    void updateFadeEffect();

    QModelIndex indexAtPage(int pageNumber) const;
    int pageAtIndex(const QModelIndex& index) const;
    QRect itemRect(int pageNumber) const;

    void handlePageClick(int pageNumber);
    void handlePageDoubleClick(int pageNumber);
    void handlePageRightClick(int pageNumber, const QPoint& globalPos);

    void showContextMenu(const QPoint& position);
    void updateContextMenuActions();

    // Context menu functionality
    void copyPageToClipboard(int pageNumber);
    void exportPageToFile(int pageNumber);

private:
    // 核心组件
    ThumbnailModel* m_thumbnailModel;
    ThumbnailDelegate* m_thumbnailDelegate;

    // 视觉设置
    QSize m_thumbnailSize;
    int m_thumbnailSpacing;
    bool m_animationEnabled;
    bool m_smoothScrolling;
    bool m_fadeInEnabled;

    // 滚动动画
    QPropertyAnimation* m_scrollAnimation;
    int m_targetScrollPosition;
    bool m_isScrollAnimating;

    // 预加载
    int m_preloadMargin;
    bool m_autoPreload;
    QTimer* m_preloadTimer;
    int m_lastFirstVisible;
    int m_lastLastVisible;

    // 可见范围跟踪 - 优化版本
    QPair<int, int> m_visibleRange;
    bool m_isScrolling;

    // 性能优化
    QTimer* m_viewportUpdateTimer;
    bool m_viewportUpdatePending;
    int m_lastVisibleStart;
    int m_lastVisibleEnd;

    // 淡入效果
    QTimer* m_fadeInTimer;
    QGraphicsOpacityEffect* m_opacityEffect;

    // 右键菜单
    bool m_contextMenuEnabled;
    QMenu* m_contextMenu;
    QList<QAction*> m_contextMenuActions;
    int m_contextMenuPage;

    // 状态跟踪
    int m_currentPage;
    QList<int> m_selectedPages;

    // 常量
    static constexpr int DEFAULT_THUMBNAIL_WIDTH = 120;
    static constexpr int DEFAULT_THUMBNAIL_HEIGHT = 160;
    static constexpr int DEFAULT_SPACING = 8;
    static constexpr int DEFAULT_PRELOAD_MARGIN = 3;
    static constexpr int SCROLL_ANIMATION_DURATION = 300;  // ms
    static constexpr int PRELOAD_TIMER_INTERVAL = 200;     // ms
    static constexpr int FADE_IN_DURATION = 150;           // ms
    static constexpr int FADE_IN_TIMER_INTERVAL = 50;      // ms
    static constexpr int SMOOTH_SCROLL_STEP = 120;  // pixels per wheel step
};
