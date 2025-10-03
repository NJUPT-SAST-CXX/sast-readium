#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QSlider>
#include <QPixmap>
#include <QTimer>
#include <QComboBox>
#include <QStackedWidget>
#include <QShortcut>
#include <QList>
#include <QColor>
#include <QPainter>
#include <QObject>
#include <QHash>
#include <QtGlobal>
#include "../../model/SearchModel.h"
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QGraphicsDropShadowEffect>
#include <QGestureEvent>
#include <QSwipeGesture>
#include <QPinchGesture>
#include <QPanGesture>
#include <QTouchEvent>
#include <QEasingCurve>
#include <QCache>
#include <QMutex>
#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QPoint>
#include <QObject>
#include <QHash>
#include <QtGlobal>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <poppler/qt6/poppler-qt6.h>
#include "../../model/DocumentModel.h"
#include "../../model/SearchModel.h"
#include "PDFAnimations.h"

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
#include "QGraphicsPDFViewer.h"
#endif
#include "PDFPrerenderer.h"
#include "../widgets/SearchWidget.h"

// 页面查看模式枚举
enum class PDFViewMode {
    SinglePage,      // 单页视图
    ContinuousScroll // 连续滚动视图
};

// 缩放类型枚举
enum class ZoomType {
    FixedValue,      // 固定缩放值
    FitWidth,        // 适应宽度
    FitHeight,       // 适应高度
    FitPage          // 适应整页
};

class PDFPageWidget : public QLabel {
    Q_OBJECT

public:
    enum RenderState {
        NotRendered,
        Rendering,
        Rendered,
        RenderError
    };

    PDFPageWidget(QWidget* parent = nullptr);
    void setPage(Poppler::Page* page, double scaleFactor = 1.0, int rotation = 0);
    void setScaleFactor(double factor);
    void setRotation(int degrees);
    double getScaleFactor() const { return currentScaleFactor; }
    int getRotation() const { return currentRotation; }
    void renderPage(); // Make public for refresh functionality

    // Asynchronous rendering support
    void setAsyncRenderingEnabled(bool enabled);
    void setPrerenderer(class PDFPrerenderer* prerenderer);
    void setPageNumber(int pageNumber);
    RenderState getRenderState() const { return renderState; }
    void cancelPendingRender();

    // DPI optimization support
    void setDPICalculator(class PDFViewer* viewer);

    // Search highlight management
    void setSearchResults(const QList<SearchResult>& results);
    void clearSearchHighlights();
    void setCurrentSearchResult(int index);
    void updateHighlightColors(const QColor& normalColor, const QColor& currentColor);
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

private slots:
    void onAsyncRenderCompleted(int pageNumber, double scaleFactor, int rotation);
    void onRenderDebounceTimeout();

private:

    Poppler::Page* currentPage;
    double currentScaleFactor;
    int currentRotation;
    QPixmap renderedPixmap;
    bool isDragging;
    QPoint lastPanPoint;

    // Asynchronous rendering members
    bool asyncRenderingEnabled;
    class PDFPrerenderer* prerenderer;
    int pageNumber;
    RenderState renderState;
    QTimer* renderDebounceTimer;
    bool hasPendingRender;

    // DPI optimization
    class PDFViewer* dpiCalculator;

    // Async rendering retry mechanism
    int asyncRenderRetryCount;
    static const int MAX_ASYNC_RETRY_COUNT = 3;

    // Search highlighting members
    QList<SearchResult> m_searchResults;
    int m_currentSearchResultIndex;
    QColor m_normalHighlightColor;
    QColor m_currentHighlightColor;

    // Optimized search highlighting
    QPixmap m_searchHighlightLayer; // Pre-rendered search highlights
    bool m_searchHighlightsDirty; // Flag to track if highlights need re-rendering
    bool m_searchHighlightsEnabled; // Flag to enable/disable highlighting optimization

    // Helper methods for highlighting
    void drawSearchHighlights(QPainter& painter);
    void updateSearchResultCoordinates();

    // Optimized search highlighting methods
    void renderSearchHighlightsToLayer();
    void invalidateSearchHighlights();
    void updateSearchHighlightLayer();

signals:
    void scaleChanged(double scale);
    void pageClicked(QPoint position);
};

class PDFViewer : public QWidget {
    Q_OBJECT

public:
    enum PageLoadState {
        NotLoaded,
        Loading,
        Loaded,
        LoadError
    };

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

    // 滚动操作 (for undo/redo support)
    QPoint getScrollPosition() const;
    void setScrollPosition(const QPoint& position);
    void scrollToTop();
    void scrollToBottom();

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

    // DPI optimization (public for PDFPageWidget access)
    double calculateOptimalDPI(double scaleFactor);

    // Prerenderer access (public for PDFPageWidget fallback)
    class PDFPrerenderer* getPrerenderer() const;

    // Scroll direction tracking for prerendering optimization
    void updateScrollDirection(int direction); // -1 = up, 0 = none, 1 = down

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    // QGraphics rendering mode
    void setQGraphicsRenderingEnabled(bool enabled);
    bool isQGraphicsRenderingEnabled() const;
    void setQGraphicsHighQualityRendering(bool enabled);
    void setQGraphicsViewMode(int mode); // 0=SinglePage, 1=ContinuousPage, etc.
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

    // True virtual scrolling methods
    void setupVirtualScrolling();
    void updateVirtualScrolling();
    void createPageWidget(int pageNumber);
    void destroyPageWidget(int pageNumber);
    QWidget* createPlaceholderWidget(int pageNumber);
    void updatePlaceholderSizes();
    int estimatePageHeight(int pageNumber);
    void calculateTotalDocumentHeight();

    // Virtual scrolling optimization methods
    void invalidatePagePositionsCache();
    void updatePagePositionsCache();
    int findPageAtPosition(int yPosition);
    QPair<int, int> calculateVisiblePageRange(int scrollValue, int viewportHeight);

    // Lazy loading methods
    void setupLazyLoading();
    void scheduleLazyLoad(int pageNumber);
    void processLazyLoads();
    void updatePageLoadState(int pageNumber, PageLoadState state);
    QWidget* createLoadingPlaceholder(int pageNumber);
    bool isPageInViewport(int pageNumber);
    void prioritizeVisiblePages();

    // 缓存管理方法
    QPixmap getCachedPage(int pageNumber, double zoomFactor, int rotation);
    void setCachedPage(int pageNumber, const QPixmap& pixmap, double zoomFactor, int rotation);
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
    void onHighlightColorsChanged(const QColor& normalColor, const QColor& currentColor);

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
    int currentRotation; // 当前旋转角度（0, 90, 180, 270）

    // 缩放控制
    QTimer* zoomTimer;
    double pendingZoomFactor;
    bool isZoomPending;

    // 测试支持
    bool m_enableStyling;

    // 虚拟化渲染
    int visiblePageStart;
    int visiblePageEnd;
    int renderBuffer; // 预渲染缓冲区大小
    QTimer* scrollTimer; // 滚动防抖定时器

    // True virtual scrolling support
    QHash<int, PDFPageWidget*> activePageWidgets; // Currently created page widgets
    QHash<int, QWidget*> placeholderWidgets; // Placeholder widgets for non-visible pages
    int totalDocumentHeight; // Estimated total document height
    QHash<int, int> pageHeights; // Cache of individual page heights
    bool isVirtualScrollingEnabled; // Flag to enable/disable virtual scrolling

    // Virtual scrolling optimization
    QVector<int> pagePositions; // Cached Y positions of each page
    bool pagePositionsCacheValid; // Flag to track if positions cache is valid
    int lastScrollValue; // Last scroll position to detect direction
    int scrollDirection; // -1 for up, 1 for down, 0 for no movement

    // Lazy loading support
    QHash<int, PageLoadState> pageLoadStates; // Track loading state of each page
    QTimer* lazyLoadTimer; // Timer for lazy loading
    QSet<int> pendingLoads; // Pages pending load
    int maxConcurrentLoads; // Maximum concurrent page loads

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

    // Enhanced page cache with optimized key generation
    struct PageCacheItem {
        QPixmap pixmap;
        double zoomFactor;
        int rotation;
        qint64 lastAccessed;
        qint64 memorySize;
        int accessCount;
        double importance; // Calculated importance score

        // LRU list pointers for O(1) operations
        PageCacheItem* prev;
        PageCacheItem* next;

        PageCacheItem() : prev(nullptr), next(nullptr) {}
    };

    // Optimized cache with integer keys instead of strings
    QHash<quint64, PageCacheItem*> pageCache; // Use 64-bit integer key
    int maxCacheSize;
    qint64 maxCacheMemory; // Maximum memory usage in bytes
    qint64 currentCacheMemory; // Current memory usage

    // LRU cache implementation
    PageCacheItem* cacheHead; // Most recently used
    PageCacheItem* cacheTail; // Least recently used

    // Cache key optimization
    QHash<double, quint32> zoomFactorToInt; // Cache zoom factor to integer mapping
    quint32 nextZoomFactorId;

    // Cache management methods
    quint64 getCacheKey(int pageNumber, double zoomFactor, int rotation);
    quint32 getZoomFactorId(double zoomFactor);
    void evictLeastImportantItems();
    qint64 calculatePixmapMemorySize(const QPixmap& pixmap);
    double calculateCacheItemImportance(const PageCacheItem& item, int currentPage);

    // LRU cache operations
    void moveToHead(PageCacheItem* item);
    void removeFromList(PageCacheItem* item);
    void addToHead(PageCacheItem* item);
    PageCacheItem* removeTail();

    // DPI optimization
    QHash<double, double> dpiCache; // Cache DPI calculations for scale factors
    void clearDPICache();

    // Render optimization
    QTimer* renderOptimizationTimer;
    bool isRenderOptimizationEnabled;
    void optimizeRenderingSettings();

    // 动画管理器
    PDFAnimationManager* animationManager;

    // 预渲染器
    PDFPrerenderer* prerenderer;

    // Scroll direction tracking for optimized prerendering
    int currentScrollDirection; // -1 = up, 0 = none, 1 = down
    QTime lastScrollTime;

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
