#include "PDFBookmarkWidget.h"
#include <QApplication>
#include <QBrush>
#include <QClipboard>
#include <QColor>
#include <QContextMenuEvent>
#include <QDebug>
#include <QFont>
#include <QKeyEvent>
#include <QLabel>
#include <QVBoxLayout>

// 自定义数据角色
enum { BookmarkIdRole = Qt::UserRole + 1, BookmarkDataRole = Qt::UserRole + 2 };

PDFBookmarkWidget::PDFBookmarkWidget(QWidget* parent)
    : QWidget(parent),
      bookmarkModel(nullptr),
      bookmarkList(nullptr),
      currentHighlightedItem(nullptr) {
    setupUI();
    setupContextMenu();
    setupConnections();
}

void PDFBookmarkWidget::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 创建书签列表
    bookmarkList = new QListWidget(this);
    bookmarkList->setAlternatingRowColors(true);
    bookmarkList->setSelectionMode(QAbstractItemView::SingleSelection);
    bookmarkList->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 设置样式
    bookmarkList->setStyleSheet(
        "QListWidget {"
        "    border: none;"
        "    background-color: transparent;"
        "    outline: none;"
        "}"
        "QListWidget::item {"
        "    padding: 8px;"
        "    border: none;"
        "    border-bottom: 1px solid #e0e0e0;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #3daee9;"
        "    color: white;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: #e3f2fd;"
        "}");

    layout->addWidget(bookmarkList);

    // 设置空状态显示
    showEmptyState();
}

void PDFBookmarkWidget::setupContextMenu() {
    contextMenu = new QMenu(this);

    addBookmarkAction = new QAction("添加书签", this);
    editBookmarkAction = new QAction("编辑书签", this);
    deleteBookmarkAction = new QAction("删除书签", this);
    copyTitleAction = new QAction("复制标题", this);

    contextMenu->addAction(addBookmarkAction);
    contextMenu->addSeparator();
    contextMenu->addAction(editBookmarkAction);
    contextMenu->addAction(deleteBookmarkAction);
    contextMenu->addSeparator();
    contextMenu->addAction(copyTitleAction);
}

void PDFBookmarkWidget::setupConnections() {
    connect(bookmarkList, &QListWidget::itemClicked, this,
            &PDFBookmarkWidget::onItemClicked);
    connect(bookmarkList, &QListWidget::itemDoubleClicked, this,
            &PDFBookmarkWidget::onItemDoubleClicked);
    connect(bookmarkList, &QListWidget::itemSelectionChanged, this,
            &PDFBookmarkWidget::onItemSelectionChanged);

    connect(addBookmarkAction, &QAction::triggered, this,
            &PDFBookmarkWidget::onAddBookmarkRequested);
    connect(editBookmarkAction, &QAction::triggered, this,
            &PDFBookmarkWidget::onEditBookmarkRequested);
    connect(deleteBookmarkAction, &QAction::triggered, this,
            &PDFBookmarkWidget::onDeleteBookmarkRequested);
    connect(copyTitleAction, &QAction::triggered, this,
            &PDFBookmarkWidget::onCopyTitleRequested);
}

void PDFBookmarkWidget::setBookmarkModel(BookmarkModel* model) {
    if (bookmarkModel) {
        disconnect(bookmarkModel, nullptr, this, nullptr);
    }

    bookmarkModel = model;

    if (bookmarkModel) {
        connect(bookmarkModel, &BookmarkModel::bookmarkAdded, this,
                &PDFBookmarkWidget::onBookmarkAdded);
        connect(bookmarkModel, &BookmarkModel::bookmarkRemoved, this,
                &PDFBookmarkWidget::onBookmarkRemoved);
        connect(bookmarkModel, &BookmarkModel::bookmarkUpdated, this,
                &PDFBookmarkWidget::onBookmarkUpdated);

        // 刷新显示
        refreshBookmarks();
    }
}

void PDFBookmarkWidget::refreshBookmarks() {
    // TODO: 实现书签刷新逻辑
    
    if (!bookmarkModel) {
        showEmptyState();
        return;
    }
    
    buildBookmarkList();
}

void PDFBookmarkWidget::clearBookmarks() {
    // TODO: 实现清空书签逻辑
    
    bookmarkList->clear();
    currentHighlightedItem = nullptr;
    showEmptyState();
}

void PDFBookmarkWidget::addBookmark(const Bookmark& bookmark) {
    // TODO: 实现添加书签逻辑
    Q_UNUSED(bookmark)
}

void PDFBookmarkWidget::removeBookmark(const QString& bookmarkId) {
    // TODO: 实现删除书签逻辑
    Q_UNUSED(bookmarkId)
}

void PDFBookmarkWidget::searchBookmarks(const QString& searchText) {
    // TODO: 实现书签搜索逻辑
    Q_UNUSED(searchText)
}

Bookmark PDFBookmarkWidget::getCurrentSelectedBookmark() const {
    // TODO: 实现获取当前选中书签逻辑
    return Bookmark();
}

void PDFBookmarkWidget::onBookmarkAdded(const Bookmark& bookmark) {
    // TODO: 处理书签添加事件
    Q_UNUSED(bookmark)
}

void PDFBookmarkWidget::onBookmarkRemoved(const QString& bookmarkId) {
    // TODO: 处理书签删除事件
    Q_UNUSED(bookmarkId)
}

void PDFBookmarkWidget::onBookmarkUpdated(const Bookmark& bookmark) {
    // TODO: 处理书签更新事件
    Q_UNUSED(bookmark)
}

void PDFBookmarkWidget::contextMenuEvent(QContextMenuEvent* event) {
    QListWidgetItem* item = bookmarkList->itemAt(
        bookmarkList->mapFromParent(event->pos()));

    // 根据是否有选中项目启用/禁用菜单项
    editBookmarkAction->setEnabled(item != nullptr);
    deleteBookmarkAction->setEnabled(item != nullptr);
    copyTitleAction->setEnabled(item != nullptr);

    contextMenu->exec(event->globalPos());
}

void PDFBookmarkWidget::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (bookmarkList->currentItem()) {
                onItemClicked(bookmarkList->currentItem());
            }
            break;
        case Qt::Key_Delete:
            if (bookmarkList->currentItem()) {
                onDeleteBookmarkRequested();
            }
            break;
        default:
            QWidget::keyPressEvent(event);
            break;
    }
}

void PDFBookmarkWidget::onItemClicked(QListWidgetItem* item) {
    // TODO: 实现项目点击逻辑
    Q_UNUSED(item)
}

void PDFBookmarkWidget::onItemDoubleClicked(QListWidgetItem* item) {
    // TODO: 实现项目双击逻辑
    Q_UNUSED(item)
}

void PDFBookmarkWidget::onItemSelectionChanged() {
    // TODO: 实现选择变化逻辑
}

void PDFBookmarkWidget::onAddBookmarkRequested() {
    // TODO: 实现添加书签请求逻辑
}

void PDFBookmarkWidget::onEditBookmarkRequested() {
    // TODO: 实现编辑书签请求逻辑
}

void PDFBookmarkWidget::onDeleteBookmarkRequested() {
    // TODO: 实现删除书签请求逻辑
}

void PDFBookmarkWidget::onCopyTitleRequested() {
    // TODO: 实现复制标题逻辑
}

void PDFBookmarkWidget::buildBookmarkList() {
    // TODO: 实现构建书签列表逻辑
}

QListWidgetItem* PDFBookmarkWidget::createBookmarkItem(const Bookmark& bookmark) {
    // TODO: 实现创建书签项逻辑
    Q_UNUSED(bookmark)
    return nullptr;
}

void PDFBookmarkWidget::setItemStyle(QListWidgetItem* item, const Bookmark& bookmark) {
    // TODO: 实现设置项目样式逻辑
    Q_UNUSED(item)
    Q_UNUSED(bookmark)
}

QListWidgetItem* PDFBookmarkWidget::findItemByBookmarkId(const QString& bookmarkId) {
    // TODO: 实现查找书签项逻辑
    Q_UNUSED(bookmarkId)
    return nullptr;
}

void PDFBookmarkWidget::highlightItem(QListWidgetItem* item) {
    // TODO: 实现高亮项目逻辑
    Q_UNUSED(item)
}

void PDFBookmarkWidget::clearHighlight() {
    // TODO: 实现清除高亮逻辑
}

Bookmark PDFBookmarkWidget::getItemBookmark(QListWidgetItem* item) const {
    // TODO: 实现获取项目书签逻辑
    Q_UNUSED(item)
    return Bookmark();
}

void PDFBookmarkWidget::searchItemsRecursive(const QString& searchText) {
    // TODO: 实现递归搜索逻辑
    Q_UNUSED(searchText)
}

void PDFBookmarkWidget::showEmptyState() {
    bookmarkList->clear();
    
    QListWidgetItem* emptyItem = new QListWidgetItem(bookmarkList);
    emptyItem->setText("暂无书签");
    emptyItem->setFlags(Qt::NoItemFlags);
    QFont font = emptyItem->font();
    font.setItalic(true);
    emptyItem->setFont(font);
    emptyItem->setForeground(QBrush(QColor(128, 128, 128)));
}