#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QCache>
#include <QColor>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEasingCurve>
#include <QEvent>
#include <QGestureEvent>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QList>
#include <QMimeData>
#include <QMouseEvent>
#include <QMutex>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPanGesture>
#include <QPinchGesture>
#include <QPixmap>
#include <QPoint>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QSlider>
#include <QSpinBox>
#include <QStackedWidget>
#include <QSwipeGesture>
#include <QTimer>
#include <QTouchEvent>
#include <QUrl>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWidget>
#include <QtGlobal>
#include "../../model/DocumentModel.h"
#include "../../model/SearchModel.h"
#include "PDFAnimations.h"

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
#include "QGraphicsPDFViewer.h"
#endif
#include "../widgets/SearchWidget.h"
#include "PDFPrerenderer.h"

// 页面查看模式枚举
enum class PDFViewMode {
    SinglePage,       // 单页视图
    ContinuousScroll  // 连续滚动视图
};

// 缩放类型枚举
enum class ZoomType {
    FixedValue,  // 固定缩放值
    FitWidth,    // 适应宽度
    FitHeight,   // 适应高度
    FitPage      // 适应整页
};

class PDFPageWidget : public QLabel {
    Q_OBJECT

public:
    PDFPageWidget(QWidget* parent = nullptr);
    void setPage(Poppler::Page* page, double scaleFactor = 1.0,
                 int rotation = 0);
    void setScaleFactor(double factor);
    void setRotation(int degrees);
    double getScaleFactor() const { return currentScaleFactor; }
    int getRotation() const { return currentRotation; }
    void renderPage();  // Make public for refresh functionality

    // Search highlight management
    void setSearchResults(const QList<SearchResult>& results);
    void clearSearchHighlights();
    void setCurrentSearchResult(int index);
    void updateHighlightColors(const QColor& normalColor,
                               const QColor& currentColor);
    bool hasSearchResults() const { return !m_searchResults.isEmpty(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool event(QEvent* event) override;
    bool gestureEvent(QGestureEvent* event);
    void pinchTriggered(QPinchGesture* gesture);
    void swipeTriggered(QSwipeGesture* gesture);
    void panTriggered(QPanGesture* gesture);
    void touchEvent(QTouchEvent* event);

    // Drag and drop support
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    Poppler::Page* currentPage;
    double currentScaleFactor;
    int currentRotation;
    QPixmap renderedPixmap;
    bool isDragging;
    QPoint lastPanPoint;

    // Search highlighting members
    QList<SearchResult> m_searchResults;
    int m_currentSearchResultIndex;
    QColor m_normalHighlightColor;
    QColor m_currentHighlightColor;

    // Helper methods for highlighting
    void drawSearchHighlights(QPainter& painter);
    void updateSearchResultCoordinates();

signals:
    void scaleChanged(double scale);
    void pageClicked(QPoint position);
};

class PDFViewer : public QWidget {
    Q_OBJECT

public:
    PDFViewer(QWidget* parent = nullptr, bool enableStyling = true);
    ~PDFViewer() = default;

    // 文档操作
    void setDocument(Poppler::Document* document);
    void clearDocument();

    // 页面导航
    void goToPage(int pageNumber);
    void nextPage();
    void previousPage();
    void firstPage();
    void lastPage();
    bool goToPageWithValidation(int pageNumber, bool showMessage = true);

    // 缩放操作
    void zoomIn();
    void zoomOut();
    void zoomToFit();
    void zoomToWidth();
    void zoomToHeight();
    void setZoom(double factor);
    void setZoomWithType(double factor, ZoomType type);
    void setZoomFromPercentage(int percentage);

    // 旋转操作
    void rotateLeft();
    void rotateRight();
    void resetRotation();
    void setRotation(int degrees);

    // 主题切换
    void toggleTheme();

    // 搜索功能
    void showSearch();
    void hideSearch();
    void toggleSearch();
    void findNext();
    void findPrevious();
    void clearSearch();

    // Search highlighting functionality
    void setSearchResults(const QList<SearchResult>& results);
    void clearSearchHighlights();
    void highlightCurrentSearchResult(const SearchResult& result);

    // 书签功能
    void addBookmark();
    void addBookmarkForPage(int pageNumber);
    void removeBookmark();
    void toggleBookmark();
    bool hasBookmarkForCurrentPage() const;

    // 查看模式操作
    void setViewMode(PDFViewMode mode);
    PDFViewMode getViewMode() const { return currentViewMode; }

    // 获取状态
    int getCurrentPage() const { return currentPageNumber; }
    int getPageCount() const;
    double getCurrentZoom() const;
    bool hasDocument() const { return document != nullptr; }

    // 消息显示
    void setMessage(const QString& message);

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    // QGraphics rendering mode
    void setQGraphicsRenderingEnabled(bool enabled);
    bool isQGraphicsRenderingEnabled() const;
    void setQGraphicsHighQualityRendering(bool enabled);
    void setQGraphicsViewMode(
        int mode);  // 0=SinglePage, 1=ContinuousPage, etc.
#endif

protected:
    void setupUI();
    void setupConnections();
    void setupShortcuts();
    void updatePageDisplay();
    void updateNavigationButtons();
    void updateZoomControls();
    bool eventFilter(QObject* object, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    // 查看模式相关方法
    void setupViewModes();
    void switchToSinglePageMode();
    void switchToContinuousMode();
    void updateContinuousView();
    void updateContinuousViewRotation();
    void createContinuousPages();

    // 虚拟化渲染方法
    void updateVisiblePages();
    void renderVisiblePages();
    void onScrollChanged();

    // 缓存管理方法
    QPixmap getCachedPage(int pageNumber, double zoomFactor, int rotation);
    void setCachedPage(int pageNumber, const QPixmap& pixmap, double zoomFactor,
                       int rotation);
    void clearPageCache();
    void cleanupCache();

    // 缩放相关方法
    void applyZoom(double factor);
    void saveZoomSettings();
    void loadZoomSettings();

    // Search highlighting helper methods
    void updateSearchHighlightsForCurrentPage();
    int findSearchResultIndex(const SearchResult& target);
    void updateAllPagesSearchHighlights();

private slots:
    void onPageNumberChanged(int pageNumber);
    void onZoomSliderChanged(int value);
    void onScaleChanged(double scale);
    void onViewModeChanged(int index);
    void onZoomPercentageChanged();
    void onZoomTimerTimeout();

    // 搜索相关槽函数
    void onSearchRequested(const QString& query, const SearchOptions& options);
    void onSearchResultSelected(const SearchResult& result);
    void onNavigateToSearchResult(int pageNumber, const QRectF& rect);

private:
    // UI组件
    QVBoxLayout* mainLayout;
    QHBoxLayout* toolbarLayout;
    QStackedWidget* viewStack;

    // 单页视图组件
    QScrollArea* singlePageScrollArea;
    PDFPageWidget* singlePageWidget;

    // 连续滚动视图组件
    QScrollArea* continuousScrollArea;
    QWidget* continuousWidget;
    QVBoxLayout* continuousLayout;

    // 工具栏控件
    QPushButton* firstPageBtn;
    QPushButton* prevPageBtn;
    QSpinBox* pageNumberSpinBox;
    QLabel* pageCountLabel;
    QPushButton* nextPageBtn;
    QPushButton* lastPageBtn;

    QPushButton* zoomInBtn;
    QPushButton* zoomOutBtn;
    QSlider* zoomSlider;
    QSpinBox* zoomPercentageSpinBox;
    QPushButton* fitWidthBtn;
    QPushButton* fitHeightBtn;
    QPushButton* fitPageBtn;

    // 旋转控件
    QPushButton* rotateLeftBtn;
    QPushButton* rotateRightBtn;

    // 主题切换控件
    QPushButton* themeToggleBtn;

    // 查看模式控件
    QComboBox* viewModeComboBox;

    // 搜索控件
    SearchWidget* searchWidget;

    // 文档数据
    Poppler::Document* document;
    int currentPageNumber;
    double currentZoomFactor;
    PDFViewMode currentViewMode;
    ZoomType currentZoomType;
    int currentRotation;  // 当前旋转角度（0, 90, 180, 270）

    // 缩放控制
    QTimer* zoomTimer;
    double pendingZoomFactor;
    bool isZoomPending;

    // 测试支持
    bool m_enableStyling;

    // 虚拟化渲染
    int visiblePageStart;
    int visiblePageEnd;
    int renderBuffer;     // 预渲染缓冲区大小
    QTimer* scrollTimer;  // 滚动防抖定时器

    // 动画效果
    QPropertyAnimation* fadeAnimation;
    QGraphicsOpacityEffect* opacityEffect;

    // 键盘快捷键
    QShortcut* zoomInShortcut;
    QShortcut* zoomOutShortcut;
    QShortcut* fitPageShortcut;
    QShortcut* fitWidthShortcut;
    QShortcut* fitHeightShortcut;
    QShortcut* rotateLeftShortcut;
    QShortcut* rotateRightShortcut;
    QShortcut* firstPageShortcut;
    QShortcut* lastPageShortcut;
    QShortcut* nextPageShortcut;
    QShortcut* prevPageShortcut;

    // 页面缓存
    struct PageCacheItem {
        QPixmap pixmap;
        double zoomFactor;
        int rotation;
        qint64 lastAccessed;
    };
    QHash<int, PageCacheItem> pageCache;
    int maxCacheSize;

    // 动画管理器
    PDFAnimationManager* animationManager;

    // 预渲染器
    PDFPrerenderer* prerenderer;

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    // QGraphics-based PDF viewer (when enabled)
    QGraphicsPDFViewer* qgraphicsViewer;
    bool useQGraphicsViewer;
#endif

    // Search highlighting members
    QList<SearchResult> m_allSearchResults;
    int m_currentSearchResultIndex;

    // 常量
    static constexpr double MIN_ZOOM = 0.1;
    static constexpr double MAX_ZOOM = 5.0;
    static constexpr double DEFAULT_ZOOM = 1.0;
    static constexpr double ZOOM_STEP = 0.1;

signals:
    void pageChanged(int pageNumber);
    void zoomChanged(double factor);
    void documentChanged(bool hasDocument);
    void viewModeChanged(PDFViewMode mode);
    void rotationChanged(int degrees);
    void sidebarToggleRequested();
    void searchRequested(const QString& text);
    void bookmarkRequested(int pageNumber);
    void fullscreenToggled(bool fullscreen);
    void fileDropped(const QString& filePath);
};
