#include "ViewDelegate.h"
#include <QApplication>
#include <QMainWindow>
#include <QPointer>
#include <QSettings>
#include <QSplitter>
#include <QVariant>
#include "../logging/SimpleLogging.h"
#include "../ui/core/MenuBar.h"
#include "../ui/core/RightSideBar.h"
#include "../ui/core/SideBar.h"
#include "../ui/core/StatusBar.h"
#include "../ui/core/ToolBar.h"
#include "../ui/core/ViewWidget.h"

#include "ElaTabWidget.h"

// ViewDelegate Implementation class
class ViewDelegate::Implementation {
public:
    explicit Implementation(ViewDelegate* q, QMainWindow* mainWindow)
        : q_ptr(q), mainWindow(mainWindow), logger("ViewDelegate") {
        logger.debug("ViewDelegate created");
    }

    ~Implementation() { logger.debug("ViewDelegate destroyed"); }

    // Helper methods
    void connectSignals();
    void saveState(const QString& key, const QVariant& value);
    QVariant loadState(const QString& key,
                       const QVariant& defaultValue = QVariant());

    ViewDelegate* q_ptr;

    // Main window reference
    QPointer<QMainWindow> mainWindow;

    // View components (not owned)
    QPointer<SideBar> sideBar;
    QPointer<RightSideBar> rightSideBar;
    QPointer<ViewWidget> viewWidget;
    QPointer<StatusBar> statusBar;
    QPointer<ToolBar> toolBar;
    QPointer<MenuBar> menuBar;
    QPointer<QSplitter> splitter;

    // State
    bool isFullScreen = false;
    bool isPresentationMode = false;
    bool isFocusMode = false;

    // Layout state
    QList<int> savedSplitterSizes;
    bool sideBarWasVisible = true;
    bool rightSideBarWasVisible = true;

    // Logging
    SastLogging::CategoryLogger logger;
};

// MainViewDelegate Implementation class
class MainViewDelegate::Implementation {
public:
    explicit Implementation(MainViewDelegate* q, ViewWidget* viewWidget)
        : q_ptr(q), viewWidget(viewWidget), logger("MainViewDelegate") {
        logger.debug("MainViewDelegate created");
    }

    ~Implementation() { logger.debug("MainViewDelegate destroyed"); }

    // Helper methods
    void updateRenderSettings();
    void applyViewMode();

    MainViewDelegate* q_ptr;

    // View widget reference (not owned)
    QPointer<ViewWidget> viewWidget;

    // Settings
    int renderQuality = 100;
    bool antiAliasing = true;
    bool smoothTransform = true;
    double zoomLevel = 1.0;
    QString currentViewMode = "single";
    bool textSelectionEnabled = true;
    bool annotationsEnabled = true;
    bool highlightCurrentPage = true;

    // Logging
    SastLogging::CategoryLogger logger;
};

// SideBarDelegate Implementation class
class SideBarDelegate::Implementation {
public:
    explicit Implementation(SideBarDelegate* q, SideBar* sideBar)
        : q_ptr(q), sideBar(sideBar), logger("SideBarDelegate") {
        logger.debug("SideBarDelegate created");
    }

    ~Implementation() { logger.debug("SideBarDelegate destroyed"); }

    SideBarDelegate* q_ptr;
    QPointer<SideBar> sideBar;
    int preferredWidth = 250;
    int currentTab = 0;
    SastLogging::CategoryLogger logger;
};

// ViewDelegate implementation
ViewDelegate::ViewDelegate(QMainWindow* mainWindow, QObject* parent)
    : QObject(parent), d(std::make_unique<Implementation>(this, mainWindow)) {}

ViewDelegate::~ViewDelegate() { saveLayoutState(); }

void ViewDelegate::setupMainLayout() {
    if (!d->mainWindow) {
        d->logger.error("MainWindow is null");
        return;
    }

    d->logger.debug("Setting up main layout with responsive design");

    // Configure main window with responsive constraints
    d->mainWindow->resize(1280, 800);
    d->mainWindow->setWindowTitle("SAST Readium");

    // Ensure central widget has proper size policy
    if (d->mainWindow->centralWidget()) {
        d->mainWindow->centralWidget()->setSizePolicy(QSizePolicy::Expanding,
                                                      QSizePolicy::Expanding);
    }

    // Apply default layout
    applyDefaultLayout();

    // Connect signals after setup
    d->connectSignals();

    d->logger.debug("Main layout setup complete with responsive behavior");
}

// Component setter methods
void ViewDelegate::setSideBar(SideBar* sideBar) { d->sideBar = sideBar; }

void ViewDelegate::setRightSideBar(RightSideBar* rightSideBar) {
    d->rightSideBar = rightSideBar;
}

void ViewDelegate::setViewWidget(ViewWidget* viewWidget) {
    d->viewWidget = viewWidget;
}

void ViewDelegate::setStatusBar(StatusBar* statusBar) {
    d->statusBar = statusBar;
}

void ViewDelegate::setToolBar(ToolBar* toolBar) { d->toolBar = toolBar; }

void ViewDelegate::setMenuBar(MenuBar* menuBar) { d->menuBar = menuBar; }

void ViewDelegate::setSplitter(QSplitter* splitter) { d->splitter = splitter; }

// Implementation class method definitions
void ViewDelegate::Implementation::connectSignals() {
    if (splitter) {
        QObject::connect(splitter, &QSplitter::splitterMoved, q_ptr,
                         &ViewDelegate::onSplitterMoved);
    }
}

void ViewDelegate::Implementation::saveState(const QString& key,
                                             const QVariant& value) {
    QSettings settings;
    settings.setValue(QString("ViewDelegate/%1").arg(key), value);
}

QVariant ViewDelegate::Implementation::loadState(const QString& key,
                                                 const QVariant& defaultValue) {
    QSettings settings;
    return settings.value(QString("ViewDelegate/%1").arg(key), defaultValue);
}

void MainViewDelegate::Implementation::updateRenderSettings() {
    if (!viewWidget) {
        return;
    }

    // Note: ViewWidget doesn't expose direct render quality settings
    // These settings are stored for potential future use or for
    // propagation to PDFViewer through ViewWidget's executePDFAction
    logger.debug(QString("Render settings updated: quality=%1, "
                         "antiAliasing=%2, smoothTransform=%3")
                     .arg(renderQuality)
                     .arg(antiAliasing)
                     .arg(smoothTransform));
}

void MainViewDelegate::Implementation::applyViewMode() {
    if (!viewWidget) {
        return;
    }

    // Map string view mode to integer mode for ViewWidget
    int mode = 0;  // Default to SinglePage
    if (currentViewMode == "single") {
        mode = 0;  // PDFViewMode::SinglePage
    } else if (currentViewMode == "continuous") {
        mode = 1;  // PDFViewMode::ContinuousScroll
    } else if (currentViewMode == "facing") {
        mode = 2;  // Future: FacingPages mode
    } else if (currentViewMode == "book") {
        mode = 3;  // Future: BookView mode
    }

    viewWidget->setCurrentViewMode(mode);
    logger.debug(QString("Applied view mode: %1 (mode=%2)")
                     .arg(currentViewMode)
                     .arg(mode));
}

void ViewDelegate::adjustSplitterSizes() {
    if (!d->splitter) {
        return;
    }

    int leftWidth = d->sideBar && d->sideBar->isVisible()
                        ? d->sideBar->getPreferredWidth()
                        : 0;
    int rightWidth = d->rightSideBar && d->rightSideBar->isVisible()
                         ? d->rightSideBar->getPreferredWidth()
                         : 0;

    // Calculate center width based on current splitter size
    int totalWidth = d->splitter->width();
    int centerWidth = totalWidth - leftWidth - rightWidth;
    // Ensure center has minimum reasonable width
    centerWidth = qMax(centerWidth, 400);

    d->splitter->setSizes({leftWidth, centerWidth, rightWidth});

    d->logger.debug(QString("Adjusted splitter sizes: %1, %2, %3")
                        .arg(leftWidth)
                        .arg(centerWidth)
                        .arg(rightWidth));
}

// ViewDelegate method implementations
void ViewDelegate::saveLayoutState() {
    QSettings settings;
    settings.beginGroup("ViewLayout");

    if (d->splitter) {
        settings.setValue("splitterSizes",
                          QVariant::fromValue(d->splitter->sizes()));
    }

    settings.setValue("sideBarVisible", isSideBarVisible());
    settings.setValue("rightSideBarVisible", isRightSideBarVisible());
    settings.setValue("fullScreen", d->isFullScreen);
    settings.setValue("presentationMode", d->isPresentationMode);
    settings.setValue("focusMode", d->isFocusMode);

    settings.endGroup();
    d->logger.debug("Layout state saved");
}

void ViewDelegate::restoreLayoutState() {
    QSettings settings;
    settings.beginGroup("ViewLayout");

    // Restore splitter sizes
    if (d->splitter) {
        QVariant splitterSizesVar = settings.value("splitterSizes");
        if (splitterSizesVar.isValid()) {
            QList<int> sizes = splitterSizesVar.value<QList<int>>();
            if (!sizes.isEmpty()) {
                d->splitter->setSizes(sizes);
                d->savedSplitterSizes = sizes;
            }
        }
    }

    // Restore sidebar visibility
    bool sideBarVisible = settings.value("sideBarVisible", true).toBool();
    showSideBar(sideBarVisible);

    // Restore right sidebar visibility
    bool rightSideBarVisible =
        settings.value("rightSideBarVisible", false).toBool();
    showRightSideBar(rightSideBarVisible);

    // Restore view modes
    d->isFullScreen = settings.value("fullScreen", false).toBool();
    d->isPresentationMode = settings.value("presentationMode", false).toBool();
    d->isFocusMode = settings.value("focusMode", false).toBool();

    settings.endGroup();
    d->logger.debug("Layout state restored successfully");
}

bool ViewDelegate::isSideBarVisible() const {
    return d->sideBar && d->sideBar->isVisible();
}

bool ViewDelegate::isRightSideBarVisible() const {
    return d->rightSideBar && d->rightSideBar->isVisible();
}

void ViewDelegate::showSideBar(bool show) {
    if (d->sideBar) {
        QWidget* widget = d->sideBar;
        widget->setVisible(show);
        adjustSplitterSizes();
        emit visibilityChanged("sideBar", show);
        d->logger.debug(QString("SideBar visibility: %1").arg(show));
    }
}

void ViewDelegate::showRightSideBar(bool show) {
    if (d->rightSideBar) {
        QWidget* widget = d->rightSideBar;
        widget->setVisible(show);
        adjustSplitterSizes();
        emit visibilityChanged("rightSideBar", show);
        d->logger.debug(QString("RightSideBar visibility: %1").arg(show));
    }
}

void ViewDelegate::toggleSideBar() { showSideBar(!isSideBarVisible()); }

void ViewDelegate::toggleRightSideBar() {
    showRightSideBar(!isRightSideBarVisible());
}

void ViewDelegate::setFullScreenMode(bool fullScreen) {
    d->isFullScreen = fullScreen;
    emit modeChanged("fullScreen", fullScreen);
    d->logger.debug(QString("Full screen mode: %1").arg(fullScreen));
}

void ViewDelegate::setPresentationMode(bool presentation) {
    d->isPresentationMode = presentation;
    emit modeChanged("presentation", presentation);
    d->logger.debug(QString("Presentation mode: %1").arg(presentation));
}

void ViewDelegate::setFocusMode(bool focus) {
    d->isFocusMode = focus;
    emit modeChanged("focus", focus);
    d->logger.debug(QString("Focus mode: %1").arg(focus));
}

void ViewDelegate::applyDefaultLayout() {
    showSideBar(true);
    showRightSideBar(false);
    adjustSplitterSizes();
    emit layoutChanged();
    d->logger.debug("Applied default layout");
}

void ViewDelegate::applyReadingLayout() {
    showSideBar(false);
    showRightSideBar(false);
    setFocusMode(true);
    emit layoutChanged();
    d->logger.debug("Applied reading layout");
}

void ViewDelegate::applyEditingLayout() {
    showSideBar(true);
    showRightSideBar(true);
    setFocusMode(false);
    adjustSplitterSizes();
    emit layoutChanged();
    d->logger.debug("Applied editing layout");
}

void ViewDelegate::applyCompactLayout() {
    showSideBar(false);
    showRightSideBar(false);
    if (d->toolBar) {
        d->toolBar->hide();
    }
    emit layoutChanged();
    d->logger.debug("Applied compact layout");
}

void ViewDelegate::onSplitterMoved(int pos, int index) {
    Q_UNUSED(pos)
    Q_UNUSED(index)
    emit layoutChanged();
}

void ViewDelegate::onComponentResized() { adjustSplitterSizes(); }

// MainViewDelegate implementation
MainViewDelegate::MainViewDelegate(ViewWidget* viewWidget, QObject* parent)
    : QObject(parent), d(std::make_unique<Implementation>(this, viewWidget)) {}

MainViewDelegate::~MainViewDelegate() = default;

double MainViewDelegate::zoomLevel() const { return d->zoomLevel; }

void MainViewDelegate::setZoomLevel(double level) {
    d->zoomLevel = qBound(0.1, level, 10.0);
    emit zoomChanged(d->zoomLevel);
    d->logger.debug(QString("Zoom level: %1").arg(d->zoomLevel));
}

void MainViewDelegate::zoomIn() { setZoomLevel(d->zoomLevel * 1.25); }

void MainViewDelegate::zoomOut() { setZoomLevel(d->zoomLevel * 0.8); }

// MainViewDelegate method implementations
void MainViewDelegate::setRenderQuality(int quality) {
    d->renderQuality = qBound(1, quality, 100);
}
void MainViewDelegate::setAntiAliasing(bool enabled) {
    d->antiAliasing = enabled;
}
void MainViewDelegate::setSmoothPixmapTransform(bool enabled) {
    d->smoothTransform = enabled;
}
void MainViewDelegate::zoomToFit() {
    d->logger.debug("Executing zoom to fit");
    if (d->viewWidget && d->viewWidget->hasDocuments()) {
        d->viewWidget->executePDFAction(ActionMap::fitToPage);
        // Update our internal zoom level to match
        d->zoomLevel = d->viewWidget->getCurrentZoom();
        emit zoomChanged(d->zoomLevel);
        d->logger.debug(QString("Zoom to fit complete: %1").arg(d->zoomLevel));
    } else {
        d->logger.warning("Cannot zoom to fit: no documents open");
    }
}

void MainViewDelegate::zoomToWidth() {
    d->logger.debug("Executing zoom to width");
    if (d->viewWidget && d->viewWidget->hasDocuments()) {
        d->viewWidget->executePDFAction(ActionMap::fitToWidth);
        // Update our internal zoom level to match
        d->zoomLevel = d->viewWidget->getCurrentZoom();
        emit zoomChanged(d->zoomLevel);
        d->logger.debug(
            QString("Zoom to width complete: %1").arg(d->zoomLevel));
    } else {
        d->logger.warning("Cannot zoom to width: no documents open");
    }
}
void MainViewDelegate::setSinglePageMode() {
    d->currentViewMode = "single";
    d->applyViewMode();
    emit viewModeChanged("single");
    d->logger.debug("Set single page mode");
}

void MainViewDelegate::setContinuousMode() {
    d->currentViewMode = "continuous";
    d->applyViewMode();
    emit viewModeChanged("continuous");
    d->logger.debug("Set continuous mode");
}

void MainViewDelegate::setFacingPagesMode() {
    d->currentViewMode = "facing";
    d->applyViewMode();
    emit viewModeChanged("facing");
    d->logger.debug("Set facing pages mode");
}

void MainViewDelegate::setBookViewMode() {
    d->currentViewMode = "book";
    d->applyViewMode();
    emit viewModeChanged("book");
    d->logger.debug("Set book view mode");
}

void MainViewDelegate::scrollToTop() {
    d->logger.debug("Scroll to top");
    if (d->viewWidget) {
        d->viewWidget->scrollToTop();
    }
}

void MainViewDelegate::scrollToBottom() {
    d->logger.debug("Scroll to bottom");
    if (d->viewWidget) {
        d->viewWidget->scrollToBottom();
    }
}

void MainViewDelegate::scrollToPage(int page) {
    d->logger.debug(QString("Scroll to page: %1").arg(page));
    if (d->viewWidget) {
        d->viewWidget->goToPage(page);
    }
    emit pageChanged(page);
}

void MainViewDelegate::centerOnPage(int page) {
    d->logger.debug(QString("Center on page: %1").arg(page));
    if (d->viewWidget) {
        d->viewWidget->goToPage(page);
    }
    emit pageChanged(page);
}
void MainViewDelegate::enableTextSelection(bool enable) {
    d->textSelectionEnabled = enable;
}
void MainViewDelegate::enableAnnotations(bool enable) {
    d->annotationsEnabled = enable;
}
void MainViewDelegate::setHighlightCurrentPage(bool highlight) {
    d->highlightCurrentPage = highlight;
}

// SideBarDelegate implementation
SideBarDelegate::SideBarDelegate(SideBar* sideBar, QObject* parent)
    : QObject(parent), d(std::make_unique<Implementation>(this, sideBar)) {}

SideBarDelegate::~SideBarDelegate() = default;

int SideBarDelegate::preferredWidth() const { return d->preferredWidth; }

void SideBarDelegate::setPreferredWidth(int width) {
    d->preferredWidth = qBound(150, width, 500);
    if (d->sideBar) {
        d->sideBar->setPreferredWidth(d->preferredWidth);
    }
    emit widthChanged(d->preferredWidth);
    d->logger.debug(QString("Preferred width: %1").arg(d->preferredWidth));
}

// SideBarDelegate method implementations
void SideBarDelegate::showTab(int index) {
    d->currentTab = index;
    emit tabChanged(index);
}
void SideBarDelegate::showTab(const QString& name) {
    if (!d->sideBar) {
        d->logger.warning("Cannot show tab: SideBar is null");
        return;
    }

    auto* tabWidget = d->sideBar->tabWidget();
    if (!tabWidget) {
        d->logger.warning("Cannot show tab: TabWidget is null");
        return;
    }

    // Map tab name to index
    int targetIndex = -1;
    QString lowerName = name.toLower();

    for (int i = 0; i < tabWidget->count(); ++i) {
        QString tabText = tabWidget->tabText(i).toLower();
        if (tabText.contains(lowerName) || lowerName.contains(tabText)) {
            targetIndex = i;
            break;
        }
    }

    if (targetIndex >= 0) {
        d->currentTab = targetIndex;
        tabWidget->setCurrentIndex(targetIndex);
        emit tabChanged(targetIndex);
        d->logger.debug(
            QString("Showing tab: %1 (index=%2)").arg(name).arg(targetIndex));
    } else {
        d->logger.warning(QString("Tab not found: %1").arg(name));
    }
}

void SideBarDelegate::enableTab(int index, bool enable) {
    if (!d->sideBar) {
        d->logger.warning("Cannot enable/disable tab: SideBar is null");
        return;
    }

    auto* tabWidget = d->sideBar->tabWidget();
    if (!tabWidget) {
        d->logger.warning("Cannot enable/disable tab: TabWidget is null");
        return;
    }

    if (index >= 0 && index < tabWidget->count()) {
        tabWidget->setTabEnabled(index, enable);
        d->logger.debug(QString("Tab %1 %2")
                            .arg(index)
                            .arg(enable ? "enabled" : "disabled"));
    } else {
        d->logger.warning(QString("Invalid tab index: %1").arg(index));
    }
}

void SideBarDelegate::setTabVisible(int index, bool visible) {
    if (!d->sideBar) {
        d->logger.warning("Cannot set tab visibility: SideBar is null");
        return;
    }

    auto* tabWidget = d->sideBar->tabWidget();
    if (!tabWidget) {
        d->logger.warning("Cannot set tab visibility: TabWidget is null");
        return;
    }

    if (index >= 0 && index < tabWidget->count()) {
        tabWidget->setTabVisible(index, visible);
        d->logger.debug(
            QString("Tab %1 visibility set to %2").arg(index).arg(visible));
    } else {
        d->logger.warning(QString("Invalid tab index: %1").arg(index));
    }
}
void SideBarDelegate::updateOutline() { emit contentUpdated("outline"); }
void SideBarDelegate::updateThumbnails() { emit contentUpdated("thumbnails"); }
void SideBarDelegate::updateBookmarks() { emit contentUpdated("bookmarks"); }
void SideBarDelegate::updateAnnotations() {
    emit contentUpdated("annotations");
}
void SideBarDelegate::setMinimumWidth(int width) {
    if (d->sideBar) {
        d->sideBar->setMinimumWidth(width);
    }
}
void SideBarDelegate::setMaximumWidth(int width) {
    if (d->sideBar) {
        d->sideBar->setMaximumWidth(width);
    }
}
void SideBarDelegate::saveState() {
    QSettings settings;
    settings.beginGroup("SideBarDelegate");

    settings.setValue("currentTab", d->currentTab);
    settings.setValue("preferredWidth", d->preferredWidth);

    settings.endGroup();
    d->logger.debug(QString("State saved: tab=%1, width=%2")
                        .arg(d->currentTab)
                        .arg(d->preferredWidth));
}

void SideBarDelegate::restoreState() {
    QSettings settings;
    settings.beginGroup("SideBarDelegate");

    d->currentTab = settings.value("currentTab", 0).toInt();
    d->preferredWidth = settings.value("preferredWidth", 250).toInt();

    // Apply restored tab if sidebar has a tab widget
    if (d->sideBar) {
        auto* tabWidget = d->sideBar->tabWidget();
        if (tabWidget && d->currentTab >= 0 &&
            d->currentTab < tabWidget->count()) {
            tabWidget->setCurrentIndex(d->currentTab);
        }

        // Apply restored width
        d->sideBar->setPreferredWidth(d->preferredWidth);
    }

    settings.endGroup();
    d->logger.debug(QString("State restored: tab=%1, width=%2")
                        .arg(d->currentTab)
                        .arg(d->preferredWidth));
}
