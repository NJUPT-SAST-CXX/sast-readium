#ifndef PAGEADAPTER_H
#define PAGEADAPTER_H

#include <QObject>

// Forward declarations
class PageController;
class PDFViewerPage;

/**
 * @brief PageAdapter - 页面控制器适配器
 *
 * 桥接 ElaWidgetTools UI 和现有的 PageController
 * 负责：
 * - 页面导航
 * - 缩放控制
 * - 旋转控制
 * - 书签管理
 */
class PageAdapter : public QObject {
    Q_OBJECT

public:
    explicit PageAdapter(QObject* parent = nullptr);
    ~PageAdapter() override;

    void setPageController(PageController* controller);
    void setPDFViewerPage(PDFViewerPage* page);

public slots:
    // 页面导航
    void goToPage(int pageNumber);
    void goToNextPage();
    void goToPreviousPage();
    void goToFirstPage();
    void goToLastPage();
    void goBack();
    void goForward();

    // 缩放控制
    void setZoom(double zoomFactor);
    void zoomIn();
    void zoomOut();
    void fitToWidth();
    void fitToPage();
    void fitToHeight();

    // 旋转控制
    void rotateLeft();
    void rotateRight();
    void resetRotation();

    // 书签管理
    void addBookmark();
    void removeBookmark();
    void toggleBookmark();

signals:
    void pageChanged(int currentPage, int totalPages);
    void zoomChanged(double zoomFactor);
    void rotationChanged(int rotation);
    void bookmarkAdded(int pageNumber);
    void bookmarkRemoved(int pageNumber);

private:
    PageController* m_pageController;
    PDFViewerPage* m_pdfViewerPage;

    void connectControllerSignals();
};

#endif  // PAGEADAPTER_H
