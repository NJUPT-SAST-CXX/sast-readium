#ifndef VIEWADAPTER_H
#define VIEWADAPTER_H

#include <QObject>

// Forward declarations
class ViewDelegate;
class PDFViewerPage;

/**
 * @brief ViewAdapter - 视图代理适配器
 *
 * 桥接 ElaWidgetTools UI 和现有的 ViewDelegate
 * 负责：
 * - 视图模式切换
 * - 全屏模式
 * - 演示模式
 * - 侧边栏显示/隐藏
 */
class ViewAdapter : public QObject {
    Q_OBJECT

public:
    explicit ViewAdapter(QObject* parent = nullptr);
    ~ViewAdapter() override;

    void setViewDelegate(ViewDelegate* delegate);
    void setPDFViewerPage(PDFViewerPage* page);

public slots:
    void setViewMode(int mode);
    void toggleFullScreen();
    void togglePresentation();
    void toggleLeftSideBar();
    void toggleRightSideBar();
    void toggleToolBar();
    void toggleStatusBar();

signals:
    void viewModeChanged(int mode);
    void fullScreenChanged(bool fullScreen);
    void presentationChanged(bool presentation);

private:
    ViewDelegate* m_viewDelegate;
    PDFViewerPage* m_pdfViewerPage;

    void connectDelegateSignals();
};

#endif  // VIEWADAPTER_H
