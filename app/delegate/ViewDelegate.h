#pragma once

#include <QObject>
#include <memory>

// Forward declarations
class QWidget;
class QMainWindow;
class QSplitter;
class ViewWidget;
class SideBar;
class RightSideBar;
class StatusBar;
class ToolBar;
class MenuBar;
class QVariant;

/**
 * @brief ViewDelegate - Manages view layout and presentation logic
 *
 * This delegate follows the Delegate pattern to handle view-specific
 * concerns such as layout management, visibility control, and UI state
 * management. It separates presentation logic from business logic.
 */
class ViewDelegate : public QObject {
    Q_OBJECT

public:
    explicit ViewDelegate(QMainWindow* mainWindow, QObject* parent = nullptr);
    ~ViewDelegate();

    // Layout management
    void setupMainLayout();
    void adjustSplitterSizes();
    void saveLayoutState();
    void restoreLayoutState();

    // Visibility control
    void showSideBar(bool show);
    void showRightSideBar(bool show);
    void toggleSideBar();
    void toggleRightSideBar();
    bool isSideBarVisible() const;
    bool isRightSideBarVisible() const;

    // View state management
    void setFullScreenMode(bool fullScreen);
    void setPresentationMode(bool presentation);
    void setFocusMode(bool focus);

    // Component access
    void setSideBar(SideBar* sideBar);
    void setRightSideBar(RightSideBar* rightSideBar);
    void setViewWidget(ViewWidget* viewWidget);
    void setStatusBar(StatusBar* statusBar);
    void setToolBar(ToolBar* toolBar);
    void setMenuBar(MenuBar* menuBar);
    void setSplitter(QSplitter* splitter);

    // Layout presets
    void applyDefaultLayout();
    void applyReadingLayout();
    void applyEditingLayout();
    void applyCompactLayout();

signals:
    void layoutChanged();
    void visibilityChanged(const QString& component, bool visible);
    void modeChanged(const QString& mode, bool active);

private slots:
    void onSplitterMoved(int pos, int index);
    void onComponentResized();

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};

/**
 * @brief MainViewDelegate - Specialized delegate for the main document view
 *
 * Handles specific rendering and interaction logic for the main PDF viewer.
 */
class MainViewDelegate : public QObject {
    Q_OBJECT

public:
    explicit MainViewDelegate(ViewWidget* viewWidget, QObject* parent = nullptr);
    ~MainViewDelegate();

    // Rendering control
    void setRenderQuality(int quality);
    void setAntiAliasing(bool enabled);
    void setSmoothPixmapTransform(bool enabled);

    // Zoom control
    void zoomIn();
    void zoomOut();
    void zoomToFit();
    void zoomToWidth();
    void setZoomLevel(double level);
    double zoomLevel() const;

    // View modes
    void setSinglePageMode();
    void setContinuousMode();
    void setFacingPagesMode();
    void setBookViewMode();

    // Scroll management
    void scrollToTop();
    void scrollToBottom();
    void scrollToPage(int page);
    void centerOnPage(int page);

    // Selection and interaction
    void enableTextSelection(bool enable);
    void enableAnnotations(bool enable);
    void setHighlightCurrentPage(bool highlight);

signals:
    void zoomChanged(double level);
    void viewModeChanged(const QString& mode);
    void pageChanged(int page);
    void renderQualityChanged(int quality);

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};

/**
 * @brief SideBarDelegate - Manages sidebar-specific UI logic
 */
class SideBarDelegate : public QObject {
    Q_OBJECT

public:
    explicit SideBarDelegate(SideBar* sideBar, QObject* parent = nullptr);
    ~SideBarDelegate();

    // Tab management
    void showTab(int index);
    void showTab(const QString& name);
    void enableTab(int index, bool enable);
    void setTabVisible(int index, bool visible);

    // Content management
    void updateOutline();
    void updateThumbnails();
    void updateBookmarks();
    void updateAnnotations();

    // Width control
    void setPreferredWidth(int width);
    int preferredWidth() const;
    void setMinimumWidth(int width);
    void setMaximumWidth(int width);

    // State
    void saveState();
    void restoreState();

signals:
    void tabChanged(int index);
    void widthChanged(int width);
    void contentUpdated(const QString& type);

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};
