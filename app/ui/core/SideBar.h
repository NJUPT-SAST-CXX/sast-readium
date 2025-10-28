#pragma once

#include <QPropertyAnimation>
#include <QSettings>
#include <QTabWidget>
#include <QWidget>
#include <memory>
#include "../../delegate/ThumbnailDelegate.h"
#include "../../model/PDFOutlineModel.h"
#include "../../model/ThumbnailModel.h"
#include "../thumbnail/ThumbnailListView.h"
#include "../viewer/PDFOutlineWidget.h"

// 前向声明
namespace Poppler {
class Document;
}
class BookmarkWidget;

/**
 * @brief Left sidebar with thumbnails and bookmarks tabs
 *
 * @details Provides document navigation via:
 * - Thumbnail view with synchronized selection
 * - Bookmark/outline tree view
 * - Animated show/hide with state persistence
 * - Configurable width with min/max constraints
 *
 * @note State (visibility, width, active tab) is persisted via QSettings
 */
class SideBar : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief Construct a new Side Bar object
     * @param parent Parent widget (optional)
     */
    SideBar(QWidget* parent = nullptr);

    /**
     * @brief Destroy the Side Bar object and clean up resources
     */
    ~SideBar();

    // 显示/隐藏控制
    bool isVisible() const;
    using QWidget::setVisible;  // Bring base class method into scope
    void setVisible(bool visible, bool animated = true);
    void toggleVisibility(bool animated = true);

    // 宽度管理
    int getPreferredWidth() const;
    void setPreferredWidth(int width);
    int getMinimumWidth() const { return minimumWidth; }
    int getMaximumWidth() const { return maximumWidth; }

    // 状态持久化
    void saveState();
    void restoreState();

    // PDF目录相关
    void setOutlineModel(PDFOutlineModel* model);
    PDFOutlineWidget* getOutlineWidget() const { return outlineWidget; }

    // 缩略图相关
    void setDocument(std::shared_ptr<Poppler::Document> document);
    void setThumbnailSize(const QSize& size);
    void refreshThumbnails();
    ThumbnailListView* getThumbnailView() const { return thumbnailView; }
    ThumbnailModel* getThumbnailModel() const { return thumbnailModel.get(); }

    // Tab widget access for delegate
    QTabWidget* getTabWidget() const { return tabWidget; }

    // 书签管理
    class BookmarkWidget* getBookmarkWidget() const { return bookmarkWidget; }
    void setCurrentDocumentPath(const QString& documentPath);
    bool addBookmark(const QString& documentPath, int pageNumber,
                     const QString& title = QString());
    bool removeBookmark(const QString& bookmarkId);
    bool hasBookmarkForPage(const QString& documentPath, int pageNumber) const;

public slots:
    void show(bool animated = true);
    void hide(bool animated = true);

signals:
    void visibilityChanged(bool visible);
    void widthChanged(int width);

private slots:
    void onAnimationFinished();

signals:
    void pageClicked(int pageNumber);
    void pageDoubleClicked(int pageNumber);
    void thumbnailSizeChanged(const QSize& size);
    void bookmarkNavigationRequested(const QString& documentPath,
                                     int pageNumber);

private:
    QTabWidget* tabWidget;
    QPropertyAnimation* animation;
    QSettings* settings;
    PDFOutlineWidget* outlineWidget;

    // 缩略图组件
    ThumbnailListView* thumbnailView;
    std::unique_ptr<ThumbnailModel> thumbnailModel;
    std::unique_ptr<ThumbnailDelegate> thumbnailDelegate;

    // 书签组件
    class BookmarkWidget* bookmarkWidget;

    bool isCurrentlyVisible;
    int preferredWidth;
    int lastWidth;

    static const int minimumWidth = 200;
    static const int maximumWidth = 400;
    static const int defaultWidth = 250;
    static const int animationDuration = 300;

    void initWindow();
    void initContent();
    void initAnimation();
    void initSettings();

    QWidget* createThumbnailsTab();
    QWidget* createBookmarksTab();
};
