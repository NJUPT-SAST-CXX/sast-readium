#pragma once

#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>
#include "../../controller/DocumentController.h"
#include "../../model/DocumentModel.h"
#include "../../model/PDFOutlineModel.h"
#include "../viewer/PDFViewer.h"
#include "../widgets/DocumentTabWidget.h"
#include "../widgets/SkeletonWidget.h"

/**
 * @brief Main document viewing widget with multi-tab PDF viewer support
 *
 * @details This widget provides the main document viewing area with:
 * - Multi-document tab management via DocumentTabWidget
 * - PDF viewer instances for each open document
 * - Document loading states with skeleton widgets and progress tracking
 * - Empty state display when no documents are open
 * - Document lifecycle management (open, close, switch)
 * - Page navigation and zoom controls
 * - View mode management (single page, continuous, etc.)
 * - Undo/redo support for zoom and scroll position
 *
 * **Document Lifecycle:**
 * 1. openDocument() - Creates PDFViewer, loads document asynchronously
 * 2. Document loading shows skeleton widget with progress bar
 * 3. On load complete, switches to PDFViewer
 * 4. closeDocument() - Removes viewer and cleans up resources
 *
 * **Resource Management:**
 * - All PDFViewer instances are managed via Qt parent-child ownership
 * - Document models are managed externally by DocumentController
 * - Proper cleanup in destructor ensures no memory leaks
 *
 * @note All document operations are bounds-checked and log errors for invalid
 * indices
 */
class ViewWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Construct a new View Widget object
     * @param parent Parent widget (optional)
     */
    ViewWidget(QWidget* parent = nullptr);

    /**
     * @brief Destroy the View Widget object and clean up resources
     */
    ~ViewWidget();

    // 设置控制器和模型
    void setDocumentController(DocumentController* controller);
    void setDocumentModel(DocumentModel* model);
    void setOutlineModel(PDFOutlineModel* model);

    // 文档操作
    void openDocument(const QString& filePath);
    void closeDocument(int index);
    void switchToDocument(int index);

    // 页面导航
    void goToPage(int pageNumber);

    // 查看模式控制
    void setCurrentViewMode(int mode);
    int getCurrentViewMode() const;

    // PDF操作控制
    void executePDFAction(ActionMap action);

    // 获取当前状态
    bool hasDocuments() const;
    int getCurrentDocumentIndex() const;
    PDFOutlineModel* getCurrentOutlineModel() const;

    // 获取当前PDF查看器状态
    int getCurrentPage() const;
    int getCurrentPageCount() const;
    double getCurrentZoom() const;
    int getCurrentRotation() const;

    // Zoom control for undo/redo support
    void setZoom(double zoomFactor);

    // Scroll position control for undo/redo support
    QPoint getScrollPosition() const;
    void setScrollPosition(const QPoint& position);
    void scrollToTop();
    void scrollToBottom();

protected:
    void setupUI();
    void setupConnections();
    void updateCurrentViewer();
    QWidget* createLoadingWidget(const QString& fileName);

private slots:
    // 文档模型信号处理
    void onDocumentOpened(int index, const QString& fileName);
    void onDocumentClosed(int index);
    void onCurrentDocumentChanged(int index);
    void onAllDocumentsClosed();
    void onDocumentLoadingStarted(const QString& filePath);
    void onDocumentLoadingProgress(int progress);
    void onDocumentLoadingFailed(const QString& error, const QString& filePath);

    // 标签页信号处理
    void onTabCloseRequested(int index);
    void onTabSwitched(int index);
    void onTabMoved(int from, int to);

    // PDF查看器信号处理
    void onPDFPageChanged(int pageNumber);
    void onPDFZoomChanged(double zoomFactor);

public slots:
    // RenderModel信号处理
    void onRenderPageDone(const QImage& image);

signals:
    void currentViewerPageChanged(int pageNumber, int totalPages);
    void currentViewerZoomChanged(double zoomFactor);
    void scaleChanged(double zoomFactor);

private:
    // UI组件
    QVBoxLayout* mainLayout;
    DocumentTabWidget* tabWidget;
    QStackedWidget* viewerStack;
    QWidget* emptyWidget;

    // 数据和控制
    DocumentController* documentController;
    DocumentModel* documentModel;
    PDFOutlineModel* outlineModel;
    QList<PDFViewer*> pdfViewers;           // 每个文档对应一个PDFViewer
    QList<PDFOutlineModel*> outlineModels;  // 每个文档对应一个目录模型

    // Loading state tracking
    QMap<QString, QWidget*> loadingWidgets;     // filePath -> loading widget
    QMap<QString, QProgressBar*> progressBars;  // filePath -> progress bar

    // 辅助方法
    PDFViewer* createPDFViewer();
    void removePDFViewer(int index);
    void showEmptyState();
    void hideEmptyState();
};
