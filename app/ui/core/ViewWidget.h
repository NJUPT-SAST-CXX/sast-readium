#pragma once

#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>
#include "../../controller/DocumentController.h"
#include "../../model/DocumentModel.h"
#include "../../model/PDFOutlineModel.h"
#include "../viewer/PDFViewer.h"
#include "../widgets/DocumentTabWidget.h"

class ViewWidget : public QWidget {
    Q_OBJECT

public:
    ViewWidget(QWidget* parent = nullptr);

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

signals:
    void currentViewerPageChanged(int pageNumber, int totalPages);
    void currentViewerZoomChanged(double zoomFactor);

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

    // 辅助方法
    PDFViewer* createPDFViewer();
    void removePDFViewer(int index);
    void showEmptyState();
    void hideEmptyState();
};
