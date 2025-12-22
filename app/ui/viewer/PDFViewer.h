#ifndef PDFVIEWER_H
#define PDFVIEWER_H

#include <QList>
#include <QObject>
#include <QRectF>
#include <QString>
#include <memory>
#include "ElaScrollArea.h"

// Forward declarations
namespace Poppler {
class Document;
class Page;
}  // namespace Poppler

class RenderModel;
class PageModel;
class PDFPrerenderer;
class AnnotationRenderDelegate;
class AnnotationSelectionManager;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;

/**
 * @brief ElaPDFViewer - PDF 查看器核心组件
 *
 * 这是 PDF 查看器的核心组件，负责：
 * - PDF 文档渲染和显示
 * - 页面导航
 * - 缩放控制
 * - 旋转控制
 * - 文本选择
 * - 搜索高亮
 * - 多种视图模式（单页、连续、双页、书籍）
 *
 * 复用现有的业务逻辑：
 * - RenderModel：页面渲染
 * - PageModel：页面状态管理
 */
class PDFViewer : public ElaScrollArea {
    Q_OBJECT

public:
    /**
     * @brief 视图模式枚举
     */
    enum ViewMode {
        SinglePage = 0,  // 单页模式
        Continuous = 1,  // 连续模式
        TwoPage = 2,     // 双页模式
        BookMode = 3     // 书籍模式（双页，第一页单独）
    };
    Q_ENUM(ViewMode)

    /**
     * @brief 工具模式枚举
     */
    enum ToolMode {
        Browse = 0,      // 浏览模式（默认）
        SelectText = 1,  // 文本选择模式
        Highlight = 2,   // 高亮标注模式
        Underline = 3,   // 下划线标注模式
        StrikeOut = 4,   // 删除线标注模式
        Note = 5,        // 笔记标注模式
        Hand = 6         // 手型工具（拖动）
    };
    Q_ENUM(ToolMode)

    explicit PDFViewer(QWidget* parent = nullptr);
    // Overload used in tests to disable styling overhead
    explicit PDFViewer(QWidget* parent, bool enableStyling);
    ~PDFViewer() override;

    // ========================================================================
    // 工具模式
    // ========================================================================

    /**
     * @brief 设置当前工具模式
     */
    void setToolMode(ToolMode mode);

    /**
     * @brief 获取当前工具模式
     */
    ToolMode toolMode() const;

    // ========================================================================
    // 外观设置
    // ========================================================================

    /**
     * @brief 开启/关闭夜间模式
     */
    void setNightMode(bool enabled);

    /**
     * @brief 是否处于夜间模式
     */
    bool isNightMode() const;

    // ========================================================================
    // 文档操作
    // ========================================================================

    /**
     * @brief 设置 PDF 文档
     * @param document Poppler 文档对象
     * @return 是否成功
     */
    bool setDocument(std::shared_ptr<Poppler::Document> document);

    /**
     * @brief 清除文档
     */
    void clearDocument();

    /**
     * @brief 获取当前文档
     */
    std::shared_ptr<Poppler::Document> document() const;

    /**
     * @brief 是否有文档
     */
    bool hasDocument() const;

    // ========================================================================
    // 页面导航
    // ========================================================================

    /**
     * @brief 跳转到指定页面
     * @param pageNumber 页码（从 1 开始）
     */
    void goToPage(int pageNumber);

    /**
     * @brief 下一页
     */
    void goToNextPage();

    /**
     * @brief 上一页
     */
    void goToPreviousPage();

    /**
     * @brief 首页
     */
    void goToFirstPage();

    /**
     * @brief 末页
     */
    void goToLastPage();

    /**
     * @brief 获取当前页码
     */
    int currentPage() const;

    /**
     * @brief 获取总页数
     */
    int pageCount() const;

    // ========================================================================
    // 缩放控制
    // ========================================================================

    /**
     * @brief 设置缩放级别
     * @param zoomFactor 缩放因子（1.0 = 100%）
     */
    void setZoom(double zoomFactor);

    /**
     * @brief 放大
     */
    void zoomIn();

    /**
     * @brief 缩小
     */
    void zoomOut();

    /**
     * @brief 适应宽度
     */
    void fitToWidth();

    /**
     * @brief 适应页面
     */
    void fitToPage();

    /**
     * @brief 适应高度
     */
    void fitToHeight();

    /**
     * @brief 获取当前缩放级别
     */
    double zoom() const;

    // ========================================================================
    // 滚动控制
    // ========================================================================

    /**
     * @brief 获取当前滚动位置（相对于内容）
     */
    QPoint scrollPosition() const;

    /**
     * @brief 设置滚动位置
     */
    void setScrollPosition(const QPoint& position);

    /**
     * @brief 滚动到顶部
     */
    void scrollToTop();

    /**
     * @brief 滚动到底部
     */
    void scrollToBottom();

    // ========================================================================
    // 旋转控制
    // ========================================================================

    /**
     * @brief 向左旋转（逆时针 90 度）
     */
    void rotateLeft();

    /**
     * @brief 向右旋转（顺时针 90 度）
     */
    void rotateRight();

    /**
     * @brief 重置旋转
     */
    void resetRotation();

    /**
     * @brief 获取当前旋转角度
     */
    int rotation() const;

    // ========================================================================
    // 视图模式
    // ========================================================================

    /**
     * @brief 设置视图模式
     */
    void setViewMode(ViewMode mode);

    /**
     * @brief 获取当前视图模式
     */
    ViewMode viewMode() const;

    // ========================================================================
    // 搜索高亮
    // ========================================================================

    /**
     * @brief 高亮搜索结果
     * @param pageNumber 页码
     * @param results 搜索结果矩形列表
     */
    void highlightSearchResults(int pageNumber, const QList<QRectF>& results);

    /**
     * @brief 清除搜索高亮
     */
    void clearSearchHighlights();

    // ========================================================================
    // 业务逻辑集成
    // ========================================================================

    /**
     * @brief 设置渲染模型
     */
    void setRenderModel(RenderModel* model);

    /**
     * @brief 设置页面模型
     */
    void setPageModel(PageModel* model);

    // QGraphics PDF rendering mode control (used by integration/performance
    // tests)
    void setQGraphicsRenderingEnabled(bool enabled);
    bool isQGraphicsRenderingEnabled() const;
    void setQGraphicsHighQualityRendering(bool enabled);

    // ========================================================================
    // Annotation System Integration
    // ========================================================================

    /**
     * @brief Set annotation render delegate for rendering annotations
     * @param delegate The annotation render delegate
     */
    void setAnnotationRenderDelegate(AnnotationRenderDelegate* delegate);

    /**
     * @brief Set annotation selection manager for interaction
     * @param manager The annotation selection manager
     */
    void setAnnotationSelectionManager(AnnotationSelectionManager* manager);

    // ------------------------------------------------------------------------
    // Backward-compatibility API for tests
    // ------------------------------------------------------------------------
    enum class PDFViewMode { SinglePage, ContinuousScroll };

    // Accept raw pointer for convenient test usage (no ownership taken)
    bool setDocument(Poppler::Document* document);

    // Legacy getters
    int getCurrentPage() const;  // 0-based for tests
    int getPageCount() const;
    double getCurrentZoom() const;

    // Legacy navigation and zoom helpers
    void nextPage();
    void previousPage();
    void zoomToWidth();
    void zoomToFit();

    // Legacy view mode wrappers
    void setViewMode(PDFViewMode mode);
    PDFViewMode getViewMode() const;

    // Legacy search UI stubs (no-op in current implementation)
    void showSearch();
    void hideSearch();
    void toggleSearch();
    void findNext();
    void findPrevious();
    void clearSearch();

signals:
    // 文档信号
    void documentLoaded(int pageCount);
    void documentClosed();

    // 页面信号
    void pageChanged(int currentPage, int totalPages);
    void pageRendered(int pageNumber);

    // 缩放信号
    void zoomChanged(double zoomFactor);

    // 旋转信号
    void rotationChanged(int rotation);

    // 视图模式信号
    void viewModeChanged(ViewMode mode);

    // 工具模式信号
    void toolModeChanged(ToolMode mode);

    // 链接点击信号
    void linkClicked(const QString& url);
    void linkDestination(int pageNumber, double x, double y);

    // 错误信号
    void renderError(const QString& error);

    // 新增信号
    void signal1();
    void signal2();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    // 内部实现类（Pimpl 模式）
    class Implementation;
    std::unique_ptr<Implementation> m_impl;

    // 渲染相关
    void renderCurrentPages();
    void renderPage(int pageNumber);
    void updateLayout();
    void clearLayout();

    // 空状态管理
    void createEmptyStateWidget();
    void showEmptyState();
    void hideEmptyState();

    // 缩放辅助
    double calculateFitWidthZoom();
    double calculateFitPageZoom();
    double calculateFitHeightZoom();

    // 视图模式辅助
    void applySinglePageMode();
    void applyContinuousMode();
    void applyTwoPageMode();
    void applyBookMode();

    // 虚拟滚动辅助
    void updateVisiblePages();
};

// Backward-compatibility alias for tests expecting a top-level PDFViewMode
using PDFViewMode = PDFViewer::PDFViewMode;

#endif  // PDFVIEWER_H
