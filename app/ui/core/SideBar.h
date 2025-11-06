#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <QList>
#include <QString>
#include <memory>
#include "ElaDockWidget.h"
#include "model/ThumbnailModel.h"  // For complete type in getter

// Forward declarations
class ElaTabWidget;
class ThumbnailPanel;
class BookmarkPanel;
class OutlinePanel;
class ThumbnailModel;
class BookmarkModel;
class PDFOutlineModel;
class QPropertyAnimation;
class QSettings;
class PDFOutlineWidget;   // Compat: legacy outline widget type
class ThumbnailListView;  // Compat: legacy thumbnail view type

namespace Poppler {
class Document;
}

/**
 * @brief ElaSideBar - 左侧边栏组件
 *
 * 包含三个标签页：
 * 1. 缩略图 (Thumbnails) - 显示所有页面的缩略图
 * 2. 书签 (Bookmarks) - 显示和管理书签
 * 3. 大纲 (Outline) - 显示 PDF 文档大纲/目录
 *
 * 复用现有业务逻辑：
 * - ThumbnailModel - 缩略图生成和管理
 * - BookmarkModel - 书签管理
 * - PDFOutlineModel - PDF 大纲管理
 */
class SideBar : public ElaDockWidget {
    Q_OBJECT

public:
    /**
     * @brief 标签页索引
     */
    enum TabIndex { ThumbnailsTab = 0, BookmarksTab = 1, OutlineTab = 2 };
    Q_ENUM(TabIndex)

    explicit SideBar(QWidget* parent = nullptr);
    ~SideBar() override;

    // ========================================================================
    // 文档操作
    // ========================================================================

    /**
     * @brief 设置 PDF 文档
     */
    void setDocument(std::shared_ptr<Poppler::Document> document);

    /**
     * @brief 清除文档
     */
    void clearDocument();

    // ========================================================================
    // 标签页控制
    // ========================================================================

    /**
     * @brief 切换到指定标签页
     */
    void switchToTab(TabIndex index);

    /**
     * @brief 获取当前标签页
     */
    TabIndex currentTab() const;

    /**
     * @brief 访问内部的标签页控件
     */
    ElaTabWidget* tabWidget() const;

    // ========================================================================
    // 缩略图功能
    // ========================================================================

    /**
     * @brief 设置当前页面（高亮对应缩略图）
     */
    void setCurrentPage(int pageNumber);

    /**
     * @brief 刷新缩略图
     */
    void refreshThumbnails();

    /**
     * @brief 设置缩略图大小
     */
    void setThumbnailSize(int size);
    // Backward-compatibility: overload accepting QSize
    void setThumbnailSize(const QSize& size);

    // ========================================================================
    // 书签功能
    // ========================================================================

    /**
     * @brief 添加书签
     */
    void addBookmark(int pageNumber, const QString& title = QString());

    /**
     * @brief 删除书签
     */
    void removeBookmark(int pageNumber);

    /**
     * @brief 清除所有书签
     */
    void clearBookmarks();

    /**
     * @brief 导出书签
     */
    bool exportBookmarks(const QString& filePath);

    /**
     * @brief 导入书签
     */
    bool importBookmarks(const QString& filePath);

    // ========================================================================
    // 大纲功能
    // ========================================================================

    /**
     * @brief 刷新大纲
     */
    void refreshOutline();

    /**
     * @brief 展开所有大纲项
     */
    void expandAllOutline();

    /**
     * @brief 折叠所有大纲项
     */
    void collapseAllOutline();

    // ========================================================================
    // 业务逻辑集成
    // ========================================================================

    /**
     * @brief 设置缩略图模型
     */
    void setThumbnailModel(ThumbnailModel* model);

    /**
     * @brief 设置书签模型
     */
    void setBookmarkModel(BookmarkModel* model);

    /**
     * @brief 设置大纲模型
     */
    void setOutlineModel(PDFOutlineModel* model);

    // ========================================================================
    // 可见性和宽度管理
    // ========================================================================

    /**
     * @brief 显示侧边栏（带动画）
     * @param animated 是否使用动画
     */
    void show(bool animated = true);

    /**
     * @brief 隐藏侧边栏（带动画）
     * @param animated 是否使用动画
     */
    void hide(bool animated = true);

    /**
     * @brief 切换可见性
     * @param animated 是否使用动画
     */
    void toggleVisibility(bool animated = true);

    /**
     * @brief QWidget 兼容的可见性设置（带动画选项）
     */
    void setVisible(bool visible, bool animated);
    // QWidget-compatible override for callers using the standard signature
    void setVisible(bool visible) override;

    /**
     * @brief 设置首选宽度
     */
    void setPreferredWidth(int width);

    /**
     * @brief 获取首选宽度
     */
    int getPreferredWidth() const { return m_preferredWidth; }

    /**
     * @brief 最小/最大宽度（兼容旧接口）
     */
    int getMinimumWidth() const { return minimumWidth; }
    int getMaximumWidth() const { return maximumWidth; }

    /**
     * @brief 是否可见
     */
    bool isSideBarVisible() const { return m_isCurrentlyVisible; }

    // Backward-compatibility getters expected by legacy tests
    PDFOutlineWidget* getOutlineWidget() const { return m_compatOutlineWidget; }
    ThumbnailListView* getThumbnailView() const {
        return m_compatThumbnailView;
    }
    ThumbnailModel* getThumbnailModel() const { return m_thumbnailModel; }

    /**
     * @brief 访问内部 TabWidget（兼容旧接口名）
     */
    ElaTabWidget* getTabWidget() const { return m_tabWidget; }

    /**
     * @brief 保存状态到 QSettings
     */
    void saveState();

    /**
     * @brief 从 QSettings 恢复状态
     */
    void restoreState();

signals:
    /**
     * @brief 请求跳转到页面
     */
    void pageJumpRequested(int pageNumber);

    /**
     * @brief 书签添加
     */
    void bookmarkAdded(int pageNumber, const QString& title);

    /**
     * @brief 书签删除
     */
    void bookmarkRemoved(int pageNumber);

    /**
     * @brief 大纲项点击
     */
    void outlineItemClicked(int pageNumber);

    // Backward-compatibility signals expected by legacy tests
    void pageClicked(int pageNumber);
    void pageDoubleClicked(int pageNumber);

    /**
     * @brief 标签页切换
     */
    void tabChanged(TabIndex index);

    /**
     * @brief 可见性改变
     */
    void visibilityChanged(bool visible);

    /**
     * @brief 宽度改变
     */
    void widthChanged(int width);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onAnimationFinished();

private:
    // UI 组件
    ElaTabWidget* m_tabWidget;
    ThumbnailPanel* m_thumbnailPanel;
    BookmarkPanel* m_bookmarkPanel;
    OutlinePanel* m_outlinePanel;

    // 业务逻辑模型
    ThumbnailModel* m_thumbnailModel;
    BookmarkModel* m_bookmarkModel;
    PDFOutlineModel* m_outlineModel;

    // 文档
    std::shared_ptr<Poppler::Document> m_document;

    // 当前状态
    int m_currentPage;

    // 可见性和宽度管理
    QPropertyAnimation* m_animation;
    QSettings* m_settings;
    bool m_isCurrentlyVisible;
    int m_preferredWidth;
    int m_lastWidth;

    // 常量
    static const int minimumWidth = 200;
    // Backward-compatibility adapters (not shown in UI)
    PDFOutlineWidget* m_compatOutlineWidget{nullptr};
    ThumbnailListView* m_compatThumbnailView{nullptr};

    static const int maximumWidth = 400;
    static const int defaultWidth = 250;
    static const int animationDuration = 300;

    // 初始化方法
    void setupUi();
    void setupTabs();
    void connectSignals();
    void initAnimation();
    void initSettings();

    // 辅助方法
    void retranslateUi();
};

#endif  // SIDEBAR_H
