#pragma once

#include <QAction>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include "../../model/BookmarkModel.h"

class PDFBookmarkWidget : public QWidget {
    Q_OBJECT

public:
    explicit PDFBookmarkWidget(QWidget* parent = nullptr);
    ~PDFBookmarkWidget() = default;

    // 设置书签模型
    void setBookmarkModel(BookmarkModel* model);

    // 刷新书签显示
    void refreshBookmarks();

    // 清空书签
    void clearBookmarks();

    // 添加书签
    void addBookmark(const Bookmark& bookmark);

    // 删除书签
    void removeBookmark(const QString& bookmarkId);

    // 搜索书签
    void searchBookmarks(const QString& searchText);

    // 获取当前选中的书签
    Bookmark getCurrentSelectedBookmark() const;

public slots:
    void onBookmarkAdded(const Bookmark& bookmark);
    void onBookmarkRemoved(const QString& bookmarkId);
    void onBookmarkUpdated(const Bookmark& bookmark);

signals:
    void pageNavigationRequested(int pageNumber);
    void bookmarkSelectionChanged(const Bookmark& bookmark);
    void bookmarkEditRequested(const Bookmark& bookmark);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onItemClicked(QListWidgetItem* item);
    void onItemDoubleClicked(QListWidgetItem* item);
    void onItemSelectionChanged();
    void onAddBookmarkRequested();
    void onEditBookmarkRequested();
    void onDeleteBookmarkRequested();
    void onCopyTitleRequested();

private:
    QPointer<BookmarkModel> bookmarkModel;
    QListWidget* bookmarkList;
    QListWidgetItem* currentHighlightedItem;
    QMenu* contextMenu;
    QAction* addBookmarkAction;
    QAction* editBookmarkAction;
    QAction* deleteBookmarkAction;
    QAction* copyTitleAction;

    // 初始化UI
    void setupUI();
    void setupContextMenu();
    void setupConnections();

    // 构建书签列表
    void buildBookmarkList();

    // 创建书签项
    QListWidgetItem* createBookmarkItem(const Bookmark& bookmark);

    // 设置项目样式
    void setItemStyle(QListWidgetItem* item, const Bookmark& bookmark);

    // 查找包含指定书签的项目
    QListWidgetItem* findItemByBookmarkId(const QString& bookmarkId);

    // 高亮项目
    void highlightItem(QListWidgetItem* item);
    void clearHighlight();

    // 获取项目对应的书签
    Bookmark getItemBookmark(QListWidgetItem* item) const;

    // 递归搜索项目
    void searchItemsRecursive(const QString& searchText);

    // 显示空状态
    void showEmptyState();

    // TODO: 实现书签管理功能
    // TODO: 实现书签分类功能
    // TODO: 实现书签导入导出功能
    // TODO: 实现书签同步功能
};