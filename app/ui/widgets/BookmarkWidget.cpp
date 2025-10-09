#include "BookmarkWidget.h"
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QFileInfo>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QSortFilterProxyModel>

BookmarkWidget::BookmarkWidget(QWidget* parent)
    : QWidget(parent),
      m_bookmarkModel(new BookmarkModel(this)),
      m_proxyModel(new QSortFilterProxyModel(this)) {
    setupUI();
    setupConnections();
    setupContextMenu();

    // Configure proxy model
    m_proxyModel->setSourceModel(m_bookmarkModel);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(-1);  // Filter all columns
    m_bookmarkView->setModel(m_proxyModel);

    refreshView();
}

void BookmarkWidget::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(6, 6, 6, 6);
    m_mainLayout->setSpacing(4);

    // Toolbar
    m_toolbarLayout = new QHBoxLayout();

    m_addButton = new QPushButton("添加书签");
    m_addButton->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    m_addButton->setToolTip("为当前页面添加书签");

    m_removeButton = new QPushButton("删除");
    m_removeButton->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_TrashIcon));
    m_removeButton->setToolTip("删除选中的书签");
    m_removeButton->setEnabled(false);

    m_editButton = new QPushButton("编辑");
    m_editButton->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    m_editButton->setToolTip("编辑选中的书签");
    m_editButton->setEnabled(false);

    m_refreshButton = new QPushButton("刷新");
    m_refreshButton->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_BrowserReload));
    m_refreshButton->setToolTip("刷新书签列表");

    m_toolbarLayout->addWidget(m_addButton);
    m_toolbarLayout->addWidget(m_removeButton);
    m_toolbarLayout->addWidget(m_editButton);
    m_toolbarLayout->addStretch();
    m_toolbarLayout->addWidget(m_refreshButton);

    // Filter controls
    m_filterLayout = new QHBoxLayout();

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("搜索书签...");
    m_searchEdit->setClearButtonEnabled(true);

    m_categoryFilter = new QComboBox();
    m_categoryFilter->addItem("所有分类", "");
    m_categoryFilter->setMinimumWidth(120);

    m_sortOrder = new QComboBox();
    m_sortOrder->addItem("最近访问", "recent");
    m_sortOrder->addItem("创建时间", "created");
    m_sortOrder->addItem("标题", "title");
    m_sortOrder->addItem("页码", "page");
    m_sortOrder->setMinimumWidth(100);

    m_countLabel = new QLabel("0 个书签");

    m_filterLayout->addWidget(new QLabel("搜索:"));
    m_filterLayout->addWidget(m_searchEdit);
    m_filterLayout->addWidget(new QLabel("分类:"));
    m_filterLayout->addWidget(m_categoryFilter);
    m_filterLayout->addWidget(new QLabel("排序:"));
    m_filterLayout->addWidget(m_sortOrder);
    m_filterLayout->addStretch();
    m_filterLayout->addWidget(m_countLabel);

    // Main bookmark view
    m_bookmarkView = new QTreeView();
    m_bookmarkView->setAlternatingRowColors(true);
    m_bookmarkView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_bookmarkView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_bookmarkView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_bookmarkView->setSortingEnabled(true);
    m_bookmarkView->setRootIsDecorated(false);

    // Configure header
    QHeaderView* header = m_bookmarkView->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Stretch);  // Title column
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);  // Document
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);  // Page
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);  // Created

    // Add to layout
    m_mainLayout->addLayout(m_toolbarLayout);
    m_mainLayout->addLayout(m_filterLayout);
    m_mainLayout->addWidget(m_bookmarkView, 1);
}

void BookmarkWidget::setupConnections() {
    // Toolbar buttons
    connect(m_addButton, &QPushButton::clicked, this,
            &BookmarkWidget::onAddBookmarkRequested);
    connect(m_removeButton, &QPushButton::clicked, this,
            &BookmarkWidget::onRemoveBookmarkRequested);
    connect(m_editButton, &QPushButton::clicked, this,
            &BookmarkWidget::onEditBookmarkRequested);
    connect(m_refreshButton, &QPushButton::clicked, this,
            &BookmarkWidget::refreshView);

    // Filter controls
    connect(m_searchEdit, &QLineEdit::textChanged, this,
            &BookmarkWidget::onSearchTextChanged);
    connect(m_categoryFilter,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &BookmarkWidget::onCategoryFilterChanged);
    connect(m_sortOrder, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BookmarkWidget::onSortOrderChanged);

    // View interactions
    connect(m_bookmarkView, &QTreeView::doubleClicked, this,
            &BookmarkWidget::onBookmarkDoubleClicked);
    connect(m_bookmarkView, &QTreeView::customContextMenuRequested, this,
            &BookmarkWidget::showContextMenu);
    connect(m_bookmarkView->selectionModel(),
            &QItemSelectionModel::selectionChanged, this,
            &BookmarkWidget::onBookmarkSelectionChanged);

    // Model signals
    connect(m_bookmarkModel, &BookmarkModel::bookmarkAdded, this,
            &BookmarkWidget::bookmarkAdded);
    connect(m_bookmarkModel, &BookmarkModel::bookmarkRemoved, this,
            &BookmarkWidget::bookmarkRemoved);
    connect(m_bookmarkModel, &BookmarkModel::bookmarkUpdated, this,
            &BookmarkWidget::bookmarkUpdated);
    connect(m_bookmarkModel, &BookmarkModel::bookmarksLoaded, this,
            [this](int count) {
                m_countLabel->setText(QString("%1 个书签").arg(count));
                updateCategoryFilter();
            });
}

void BookmarkWidget::setupContextMenu() {
    m_contextMenu = new QMenu(this);

    m_navigateAction = m_contextMenu->addAction("导航到此页", [this]() {
        Bookmark bookmark = getSelectedBookmark();
        if (!bookmark.id.isEmpty()) {
            emit navigateToBookmark(bookmark.documentPath, bookmark.pageNumber);
        }
    });

    m_contextMenu->addSeparator();

    m_editAction = m_contextMenu->addAction(
        "编辑书签", this, &BookmarkWidget::onEditBookmarkRequested);
    m_deleteAction = m_contextMenu->addAction(
        "删除书签", this, &BookmarkWidget::onRemoveBookmarkRequested);

    m_contextMenu->addSeparator();

    m_addCategoryAction = m_contextMenu->addAction("添加到分类", [this]() {
        Bookmark bookmark = getSelectedBookmark();
        if (!bookmark.id.isEmpty()) {
            bool ok;
            QString category = QInputDialog::getText(
                this, "添加到分类", "分类名称:", QLineEdit::Normal,
                bookmark.category, &ok);
            if (ok) {
                m_bookmarkModel->moveBookmarkToCategory(bookmark.id, category);
                updateCategoryFilter();
            }
        }
    });

    m_removeCategoryAction = m_contextMenu->addAction("移除分类", [this]() {
        Bookmark bookmark = getSelectedBookmark();
        if (!bookmark.id.isEmpty()) {
            m_bookmarkModel->moveBookmarkToCategory(bookmark.id, "");
            updateCategoryFilter();
        }
    });
}

void BookmarkWidget::setCurrentDocument(const QString& documentPath) {
    m_currentDocument = documentPath;
    m_addButton->setEnabled(!documentPath.isEmpty());
}

bool BookmarkWidget::addBookmark(const QString& documentPath, int pageNumber,
                                 const QString& title) {
    if (documentPath.isEmpty() || pageNumber < 0) {
        return false;
    }

    // Check if bookmark already exists
    if (m_bookmarkModel->hasBookmarkForPage(documentPath, pageNumber)) {
        QMessageBox::information(
            this, "书签已存在",
            QString("第 %1 页已经有书签了").arg(pageNumber + 1));
        return false;
    }

    // Create new bookmark
    Bookmark bookmark(documentPath, pageNumber, title);

    // Allow user to customize title
    if (title.isEmpty()) {
        bool ok;
        QString customTitle = QInputDialog::getText(
            this, "添加书签", "书签标题:", QLineEdit::Normal, bookmark.title,
            &ok);
        if (!ok) {
            return false;  // User cancelled
        }
        bookmark.title = customTitle;
    }

    return m_bookmarkModel->addBookmark(bookmark);
}

bool BookmarkWidget::removeBookmark(const QString& bookmarkId) {
    return m_bookmarkModel->removeBookmark(bookmarkId);
}

bool BookmarkWidget::hasBookmarkForPage(const QString& documentPath,
                                        int pageNumber) const {
    return m_bookmarkModel->hasBookmarkForPage(documentPath, pageNumber);
}

void BookmarkWidget::refreshView() {
    updateCategoryFilter();
    filterBookmarks();
    m_countLabel->setText(QString("%1 个书签").arg(m_proxyModel->rowCount()));
}

void BookmarkWidget::expandAll() { m_bookmarkView->expandAll(); }

void BookmarkWidget::collapseAll() { m_bookmarkView->collapseAll(); }

void BookmarkWidget::onBookmarkDoubleClicked(const QModelIndex& index) {
    if (!index.isValid()) {
        return;
    }

    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    Bookmark bookmark = m_bookmarkModel->getBookmark(
        m_bookmarkModel->data(sourceIndex, BookmarkModel::IdRole).toString());

    if (!bookmark.id.isEmpty()) {
        // Update last accessed time
        bookmark.lastAccessed = QDateTime::currentDateTime();
        m_bookmarkModel->updateBookmark(bookmark.id, bookmark);

        emit bookmarkSelected(bookmark);
        emit navigateToBookmark(bookmark.documentPath, bookmark.pageNumber);
    }
}

void BookmarkWidget::onBookmarkSelectionChanged() { updateBookmarkActions(); }

void BookmarkWidget::onAddBookmarkRequested() {
    if (m_currentDocument.isEmpty()) {
        QMessageBox::warning(this, "无法添加书签", "请先打开一个PDF文档");
        return;
    }

    // This would typically get the current page from the parent viewer
    // For now, we'll ask the user
    bool ok;
    int pageNumber =
        QInputDialog::getInt(this, "添加书签", "页码:", 1, 1, 9999, 1, &ok) - 1;
    if (ok) {
        addBookmark(m_currentDocument, pageNumber);
    }
}

void BookmarkWidget::onRemoveBookmarkRequested() {
    Bookmark bookmark = getSelectedBookmark();
    if (bookmark.id.isEmpty()) {
        return;
    }

    int ret = QMessageBox::question(
        this, "删除书签",
        QString("确定要删除书签 \"%1\" 吗?").arg(bookmark.title),
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        removeBookmark(bookmark.id);
    }
}

void BookmarkWidget::onEditBookmarkRequested() {
    Bookmark bookmark = getSelectedBookmark();
    if (bookmark.id.isEmpty()) {
        return;
    }

    bool ok;
    QString newTitle = QInputDialog::getText(
        this, "编辑书签", "书签标题:", QLineEdit::Normal, bookmark.title, &ok);
    if (ok && newTitle != bookmark.title) {
        bookmark.title = newTitle;
        m_bookmarkModel->updateBookmark(bookmark.id, bookmark);
    }
}

void BookmarkWidget::showContextMenu(const QPoint& position) {
    QModelIndex index = m_bookmarkView->indexAt(position);
    bool hasSelection = index.isValid();

    m_navigateAction->setEnabled(hasSelection);
    m_editAction->setEnabled(hasSelection);
    m_deleteAction->setEnabled(hasSelection);
    m_addCategoryAction->setEnabled(hasSelection);
    m_removeCategoryAction->setEnabled(hasSelection);

    if (hasSelection) {
        m_contextMenu->exec(m_bookmarkView->mapToGlobal(position));
    }
}

void BookmarkWidget::onSearchTextChanged() { filterBookmarks(); }

void BookmarkWidget::onCategoryFilterChanged() { filterBookmarks(); }

void BookmarkWidget::onSortOrderChanged() {
    QString sortType = m_sortOrder->currentData().toString();

    if (sortType == "recent") {
        m_proxyModel->sort(3, Qt::DescendingOrder);  // Last accessed
    } else if (sortType == "created") {
        m_proxyModel->sort(3, Qt::DescendingOrder);  // Created time
    } else if (sortType == "title") {
        m_proxyModel->sort(0, Qt::AscendingOrder);  // Title
    } else if (sortType == "page") {
        m_proxyModel->sort(2, Qt::AscendingOrder);  // Page number
    }
}

void BookmarkWidget::updateBookmarkActions() {
    bool hasSelection = !getSelectedBookmark().id.isEmpty();
    m_removeButton->setEnabled(hasSelection);
    m_editButton->setEnabled(hasSelection);
}

void BookmarkWidget::filterBookmarks() {
    QString searchText = m_searchEdit->text();
    QString categoryFilter = m_categoryFilter->currentData().toString();

    // Combine search and category filters
    QString filterPattern = searchText;

    m_proxyModel->setFilterRegularExpression(QRegularExpression(
        filterPattern, QRegularExpression::CaseInsensitiveOption));

    // Update count
    m_countLabel->setText(QString("%1 个书签").arg(m_proxyModel->rowCount()));
}

Bookmark BookmarkWidget::getSelectedBookmark() const {
    QModelIndex index = getSelectedIndex();
    if (!index.isValid()) {
        return Bookmark();
    }

    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    QString bookmarkId =
        m_bookmarkModel->data(sourceIndex, BookmarkModel::IdRole).toString();
    return m_bookmarkModel->getBookmark(bookmarkId);
}

QModelIndex BookmarkWidget::getSelectedIndex() const {
    QItemSelectionModel* selection = m_bookmarkView->selectionModel();
    if (!selection || !selection->hasSelection()) {
        return QModelIndex();
    }

    return selection->currentIndex();
}

void BookmarkWidget::updateCategoryFilter() {
    QString currentCategory = m_categoryFilter->currentData().toString();

    m_categoryFilter->clear();
    m_categoryFilter->addItem("所有分类", "");

    QStringList categories = m_bookmarkModel->getCategories();
    for (const QString& category : categories) {
        m_categoryFilter->addItem(category, category);
    }

    // Restore selection if possible
    int index = m_categoryFilter->findData(currentCategory);
    if (index >= 0) {
        m_categoryFilter->setCurrentIndex(index);
    }
}
