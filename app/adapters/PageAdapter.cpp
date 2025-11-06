#include "PageAdapter.h"

// Controllers
#include "controller/PageController.h"

// Pages
#include "ui/pages/PDFViewerPage.h"

// Logging
#include "logging/SimpleLogging.h"

PageAdapter::PageAdapter(QObject* parent)
    : QObject(parent), m_pageController(nullptr), m_pdfViewerPage(nullptr) {
    SLOG_INFO("PageAdapter: Constructor");
}

PageAdapter::~PageAdapter() { SLOG_INFO("PageAdapter: Destructor"); }

void PageAdapter::setPageController(PageController* controller) {
    if (m_pageController) {
        disconnect(m_pageController, nullptr, this, nullptr);
    }

    m_pageController = controller;

    if (m_pageController) {
        connectControllerSignals();
    }
}

void PageAdapter::setPDFViewerPage(PDFViewerPage* page) {
    m_pdfViewerPage = page;
}

void PageAdapter::connectControllerSignals() {
    if (!m_pageController) {
        return;
    }

    // 连接页面控制器的信号
    connect(m_pageController, &PageController::pageChanged, this,
            [this](int currentPage, int totalPages) {
                SLOG_INFO_F("PageAdapter: Page changed: {}/{}", currentPage,
                            totalPages);
                emit pageChanged(currentPage, totalPages);
            });

    connect(m_pageController, &PageController::zoomChanged, this,
            [this](double zoomFactor) {
                SLOG_INFO_F("PageAdapter: Zoom changed: {}", zoomFactor);
                emit zoomChanged(zoomFactor);
            });

    connect(m_pageController, &PageController::rotationChanged, this,
            [this](int rotation) {
                SLOG_INFO_F("PageAdapter: Rotation changed: {}", rotation);
                emit rotationChanged(rotation);
            });
}

// 页面导航
void PageAdapter::goToPage(int pageNumber) {
    SLOG_INFO_F("PageAdapter: Going to page: {}", pageNumber);

    if (!m_pageController) {
        SLOG_ERROR("PageAdapter: PageController is null");
        return;
    }

    m_pageController->goToPage(pageNumber);
}

void PageAdapter::goToNextPage() {
    SLOG_INFO("PageAdapter: Going to next page");

    if (!m_pageController) {
        SLOG_ERROR("PageAdapter: PageController is null");
        return;
    }

    m_pageController->goToNextPage();
}

void PageAdapter::goToPreviousPage() {
    SLOG_INFO("PageAdapter: Going to previous page");

    if (!m_pageController) {
        SLOG_ERROR("PageAdapter: PageController is null");
        return;
    }

    m_pageController->goToPrevPage();
}

void PageAdapter::goToFirstPage() {
    SLOG_INFO("PageAdapter: Going to first page");

    if (!m_pageController) {
        SLOG_ERROR("PageAdapter: PageController is null");
        return;
    }

    m_pageController->goToFirstPage();
}

void PageAdapter::goToLastPage() {
    SLOG_INFO("PageAdapter: Going to last page");

    if (!m_pageController) {
        SLOG_ERROR("PageAdapter: PageController is null");
        return;
    }

    m_pageController->goToLastPage();
}

void PageAdapter::goBack() {
    SLOG_INFO("PageAdapter: Going back");

    if (!m_pageController) {
        SLOG_ERROR("PageAdapter: PageController is null");
        return;
    }

    m_pageController->goBack();
}

void PageAdapter::goForward() {
    SLOG_INFO("PageAdapter: Going forward");

    if (!m_pageController) {
        SLOG_ERROR("PageAdapter: PageController is null");
        return;
    }

    m_pageController->goForward();
}

// 缩放控制
void PageAdapter::setZoom(double zoomFactor) {
    SLOG_INFO_F("PageAdapter: Setting zoom: {}", zoomFactor);

    if (!m_pageController) {
        SLOG_ERROR("PageAdapter: PageController is null");
        return;
    }

    m_pageController->setZoomLevel(zoomFactor);
}

void PageAdapter::zoomIn() {
    SLOG_INFO("PageAdapter: Zooming in");

    if (!m_pageController) {
        SLOG_ERROR("PageAdapter: PageController is null");
        return;
    }

    double currentZoom = m_pageController->getCurrentZoomLevel();
    m_pageController->setZoomLevel(currentZoom * 1.2);
}

void PageAdapter::zoomOut() {
    SLOG_INFO("PageAdapter: Zooming out");

    if (!m_pageController) {
        SLOG_ERROR("PageAdapter: PageController is null");
        return;
    }

    double currentZoom = m_pageController->getCurrentZoomLevel();
    m_pageController->setZoomLevel(currentZoom / 1.2);
}

void PageAdapter::fitToWidth() {
    SLOG_INFO("PageAdapter: Fitting to width");

    if (!m_pageController) {
        SLOG_ERROR("PageAdapter: PageController is null");
        return;
    }

    if (!m_pdfViewerPage) {
        SLOG_ERROR("PageAdapter: PDFViewerPage is null");
        return;
    }

    // Delegate to PDFViewerPage which has access to the viewer widget
    m_pdfViewerPage->fitToWidth();
}

void PageAdapter::fitToPage() {
    SLOG_INFO("PageAdapter: Fitting to page");

    if (!m_pageController) {
        SLOG_ERROR("PageAdapter: PageController is null");
        return;
    }

    if (!m_pdfViewerPage) {
        SLOG_ERROR("PageAdapter: PDFViewerPage is null");
        return;
    }

    // Delegate to PDFViewerPage which has access to the viewer widget
    m_pdfViewerPage->fitToPage();
}

void PageAdapter::fitToHeight() {
    SLOG_INFO("PageAdapter: Fitting to height");

    if (!m_pageController) {
        SLOG_ERROR("PageAdapter: PageController is null");
        return;
    }

    if (!m_pdfViewerPage) {
        SLOG_ERROR("PageAdapter: PDFViewerPage is null");
        return;
    }

    // Delegate to PDFViewerPage which has access to the viewer widget
    m_pdfViewerPage->fitToHeight();
}

// 旋转控制
void PageAdapter::rotateLeft() {
    SLOG_INFO("PageAdapter: Rotating left");

    if (!m_pageController) {
        SLOG_ERROR("PageAdapter: PageController is null");
        return;
    }

    int currentRotation = m_pageController->getCurrentRotation();
    m_pageController->setRotation(currentRotation - 90);
}

void PageAdapter::rotateRight() {
    SLOG_INFO("PageAdapter: Rotating right");

    if (!m_pageController) {
        SLOG_ERROR("PageAdapter: PageController is null");
        return;
    }

    int currentRotation = m_pageController->getCurrentRotation();
    m_pageController->setRotation(currentRotation + 90);
}

void PageAdapter::resetRotation() {
    SLOG_INFO("PageAdapter: Resetting rotation");

    if (!m_pageController) {
        SLOG_ERROR("PageAdapter: PageController is null");
        return;
    }

    m_pageController->setRotation(0);
}

// 书签管理
void PageAdapter::addBookmark() {
    SLOG_INFO("PageAdapter: Adding bookmark");

    if (!m_pageController) {
        SLOG_ERROR("PageAdapter: PageController is null");
        return;
    }

    int currentPage = m_pageController->getCurrentPage();
    m_pageController->addBookmark();  // 使用无参数版本，会自动使用当前页
    emit bookmarkAdded(currentPage);
}

void PageAdapter::removeBookmark() {
    SLOG_INFO("PageAdapter: Removing bookmark");

    if (!m_pageController) {
        SLOG_ERROR("PageAdapter: PageController is null");
        return;
    }

    int currentPage = m_pageController->getCurrentPage();
    m_pageController->removeBookmarkAtPage(currentPage);
    emit bookmarkRemoved(currentPage);
}

void PageAdapter::toggleBookmark() {
    SLOG_INFO("PageAdapter: Toggling bookmark");

    if (!m_pageController) {
        SLOG_ERROR("PageAdapter: PageController is null");
        return;
    }

    int currentPage = m_pageController->getCurrentPage();
    if (m_pageController->hasBookmarkAtPage(currentPage)) {
        m_pageController->removeBookmarkAtPage(currentPage);
        emit bookmarkRemoved(currentPage);
    } else {
        m_pageController->addBookmark();
        emit bookmarkAdded(currentPage);
    }
}
