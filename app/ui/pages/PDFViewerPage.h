#ifndef PDFVIEWERPAGE_H
#define PDFVIEWERPAGE_H

#include <QString>
#include <QWidget>
#include <memory>
#include "controller/tool.hpp"  // For ActionMap enum

// Forward declarations
class MenuBar;
class ToolBar;
class StatusBar;
class SideBar;
class RightSideBar;
class PDFViewer;
class SearchPanel;
class DocumentTabWidget;
class QStackedWidget;
class ElaProgressRing;
class ElaBreadcrumbBar;

// Dialogs and Panels
class DocumentComparison;
class DocumentMetadataDialog;
class AnnotationToolbar;
class DocumentSkeletonWidget;

// Business logic
class DocumentController;
class PageController;
class ApplicationController;
class ViewDelegate;
class SearchEngine;
class DocumentModel;
class PDFOutlineModel;

// Adapters
class SearchAdapter;

namespace Poppler {
class Document;
}

/**
 * @brief PDFViewerPage - PDF 查看器主页面
 *
 * 这是应用的核心页面，集成所有 UI 组件和业务逻辑：
 * - 菜单栏 (ElaMenuBar)
 * - 工具栏 (ToolBar)
 * - 左侧边栏 (SideBar) - 缩略图、书签、大纲
 * - 右侧边栏 (RightSideBar) - 属性、注释
 * - PDF 查看器 (PDFViewer)
 * - 搜索面板 (SearchPanel)
 * - 状态栏 (ElaStatusBar)
 *
 * 复用现有业务逻辑：
 * - DocumentController - 文档操作控制器
 * - PageController - 页面控制器
 * - ApplicationController - 应用控制器
 * - ViewDelegate - 视图代理
 */
class PDFViewerPage : public QWidget {
    Q_OBJECT

public:
    explicit PDFViewerPage(QWidget* parent = nullptr);
    ~PDFViewerPage() override;

    // ========================================================================
    // 文档操作 - Multi-document support
    // ========================================================================

    /**
     * @brief 打开 PDF 文件（在新标签页中）
     */
    bool openFile(const QString& filePath);

    /**
     * @brief 打开多个文档
     */
    void openDocuments(const QStringList& filePaths);

    /**
     * @brief 关闭指定索引的文档
     */
    void closeDocument(int index = -1);  // -1 means current document

    /**
     * @brief 关闭当前文档
     */
    void closeCurrentDocument();

    /**
     * @brief 关闭所有文档
     */
    void closeAllDocuments();

    /**
     * @brief 切换到指定文档
     */
    void switchToDocument(int index);

    /**
     * @brief 保存文档副本
     */
    bool saveDocumentCopy(const QString& filePath);

    /**
     * @brief 打印文档
     */
    void printDocument();

    /**
     * @brief 导出文档
     */
    bool exportDocument(const QString& filePath, const QString& format);

    /**
     * @brief 显示文档元数据对话框
     */
    void showDocumentMetadata();

    /**
     * @brief 显示文档比较对话框
     */
    void showDocumentComparison();

    /**
     * @brief 切换注释工具栏
     */
    void toggleAnnotationToolbar();
    void showAnnotationToolbar();
    void hideAnnotationToolbar();

    // ========================================================================
    // 页面导航
    // ========================================================================

    void goToPage(int pageNumber);
    void goToNextPage();
    void goToPreviousPage();
    void goToFirstPage();
    void goToLastPage();
    void goBack();
    void goForward();

    // ========================================================================
    // 缩放控制
    // ========================================================================

    void setZoom(double zoomFactor);
    void zoomIn();
    void zoomOut();
    void fitToWidth();
    void fitToPage();
    void fitToHeight();

    // ========================================================================
    // 旋转控制
    // ========================================================================

    void rotateLeft();
    void rotateRight();
    void resetRotation();

    // ========================================================================
    // 视图模式
    // ========================================================================

    void setViewMode(int mode);
    void setSinglePageMode();
    void setContinuousMode();
    void setTwoPageMode();
    void setBookMode();

    // ========================================================================
    // 搜索功能
    // ========================================================================

    void showSearchPanel();
    void hideSearchPanel();
    void toggleSearchPanel();
    void search(const QString& query);
    void findNext();
    void findPrevious();

    // ========================================================================
    // 侧边栏控制
    // ========================================================================

    void showLeftSideBar();
    void hideLeftSideBar();
    void toggleLeftSideBar();

    void showRightSideBar();
    void hideRightSideBar();
    void toggleRightSideBar();

    // ========================================================================
    // 工具栏和状态栏控制
    // ========================================================================

    void showToolBar();
    void hideToolBar();
    void toggleToolBar();

    void showStatusBar();
    void hideStatusBar();
    void toggleStatusBar();

    // ========================================================================
    // 书签功能
    // ========================================================================

    void addBookmark();
    void removeBookmark();
    void showBookmarks();

    // ========================================================================
    // 全屏和演示
    // ========================================================================

    void enterFullScreen();
    void exitFullScreen();
    void toggleFullScreen();
    void startPresentation();
    void stopPresentation();
    void togglePresentation();

    // ========================================================================
    // 状态查询 - Multi-document support
    // ========================================================================

    bool hasDocument() const;
    bool hasDocuments() const;  // Check if any documents are open
    int getCurrentDocumentIndex() const;
    int getDocumentCount() const;
    QString currentFilePath() const;
    QString getDocumentFilePath(int index) const;
    int currentPage() const;
    int pageCount() const;
    double zoomLevel() const;
    bool isFullScreen() const;
    bool isPresentation() const;

    // ========================================================================
    // 业务逻辑集成
    // ========================================================================

    void setDocumentController(DocumentController* controller);
    void setDocumentModel(DocumentModel* model);
    void setPageController(PageController* controller);
    void setApplicationController(ApplicationController* controller);
    void setViewDelegate(ViewDelegate* delegate);

signals:
    // 文档信号
    void documentOpened(const QString& filePath);
    void documentClosed();
    void documentModified();

    // 页面信号
    void pageChanged(int currentPage, int totalPages);

    // 缩放信号
    void zoomChanged(double zoomFactor);

    // 视图信号
    void viewModeChanged(int mode);
    void fullScreenChanged(bool fullScreen);

    // 错误信号
    void errorOccurred(const QString& error);

protected:
    void changeEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    // UI 组件
    MenuBar* m_menuBar;
    ToolBar* m_toolBar;
    StatusBar* m_statusBar;
    SideBar* m_leftSideBar;
    RightSideBar* m_rightSideBar;
    SearchPanel* m_searchPanel;

    // Multi-document support
    DocumentTabWidget* m_tabWidget;
    QStackedWidget* m_viewerStack;
    QWidget* m_emptyWidget;
    QList<PDFViewer*> m_pdfViewers;           // One viewer per document
    QList<PDFOutlineModel*> m_outlineModels;  // One outline model per document

    // 业务逻辑控制器
    DocumentController* m_documentController;
    DocumentModel* m_documentModel;
    PageController* m_pageController;
    ApplicationController* m_applicationController;
    ViewDelegate* m_viewDelegate;
    SearchEngine* m_searchEngine;

    // 适配器
    SearchAdapter* m_searchAdapter;

    // Enhanced components
    DocumentComparison* m_documentComparison;
    DocumentMetadataDialog* m_metadataDialog;
    AnnotationToolbar* m_annotationToolbar;
    DocumentSkeletonWidget* m_loadingSkeleton;
    ElaProgressRing* m_loadingRing;
    ElaBreadcrumbBar* m_breadcrumbBar;

    // 当前状态
    bool m_isFullScreen;
    bool m_isPresentation;
    int m_lastActiveIndex;  // Track last active document index

    // Document state preservation
    struct DocumentState {
        int currentPage = 1;
        double zoomLevel = 1.0;
        int rotation = 0;
        QPoint scrollPosition = QPoint(0, 0);
        int viewMode = 0;
    };
    QList<DocumentState> m_documentStates;

    // 初始化方法
    void setupUi();
    void setupLayout();
    void setupControllers();
    void connectSignals();
    void connectMenuBarSignals();
    void connectToolBarSignals();
    void connectStatusBarSignals();
    void connectPDFViewerSignals();
    void connectSideBarSignals();
    void connectSearchPanelSignals();

    // 辅助方法
    void updateWindowTitle();
    void updateMenuStates();
    void updateToolBarStates();
    void updateStatusBar();
    void retranslateUi();
    void handleAction(ActionMap action);  // 处理动作的共享方法

    // 文档加载 - Multi-document support
    bool loadDocument(const QString& filePath);
    void unloadDocument(int index = -1);  // -1 means current document

    // Multi-document management
    PDFViewer* createPDFViewer();
    PDFViewer* getCurrentViewer() const;
    void updateCurrentViewer();
    void showEmptyState();
    void hideEmptyState();

    // Document state management
    DocumentState getDocumentState(int index) const;
    void setDocumentState(int index, const DocumentState& state);
    void preserveCurrentDocumentState();
    void restoreDocumentState(int index);

    // Validation helpers
    bool validateDocumentIndex(int index, const QString& operation) const;

private slots:
    // Tab management slots
    void onTabCloseRequested(int index);
    void onTabSwitched(int index);
    void onAllTabsClosed();
};

#endif  // PDFVIEWERPAGE_H
