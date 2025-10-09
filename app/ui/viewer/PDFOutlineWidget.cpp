#include "PDFOutlineWidget.h"
#include <QApplication>
#include <QBrush>
#include <QClipboard>
#include <QColor>
#include <QContextMenuEvent>
#include <QDebug>
#include <QFont>
#include <QKeyEvent>
#include <QTreeWidgetItemIterator>

// 自定义数据角色
enum { PageNumberRole = Qt::UserRole + 1, NodePtrRole = Qt::UserRole + 2 };

PDFOutlineWidget::PDFOutlineWidget(QWidget* parent)
    : QTreeWidget(parent),
      outlineModel(nullptr),
      currentHighlightedItem(nullptr) {
    setupUI();
    setupContextMenu();
    setupConnections();
}

void PDFOutlineWidget::setupUI() {
    // 设置基本属性
    setHeaderHidden(true);
    setRootIsDecorated(true);
    setAlternatingRowColors(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    // 设置样式
    setStyleSheet(
        "QTreeWidget {"
        "    border: none;"
        "    background-color: transparent;"
        "    outline: none;"
        "}"
        "QTreeWidget::item {"
        "    padding: 4px;"
        "    border: none;"
        "}"
        "QTreeWidget::item:selected {"
        "    background-color: #3daee9;"
        "    color: white;"
        "}"
        "QTreeWidget::item:hover {"
        "    background-color: #e3f2fd;"
        "}");

    // 设置空状态显示
    QTreeWidgetItem* emptyItem = new QTreeWidgetItem(this);
    emptyItem->setText(0, "无目录信息");
    emptyItem->setFlags(Qt::NoItemFlags);
    QFont font = emptyItem->font(0);
    font.setItalic(true);
    emptyItem->setFont(0, font);
    emptyItem->setForeground(0, QBrush(QColor(128, 128, 128)));
}

void PDFOutlineWidget::setupContextMenu() {
    contextMenu = new QMenu(this);

    expandAllAction = new QAction("展开全部", this);
    collapseAllAction = new QAction("折叠全部", this);
    copyTitleAction = new QAction("复制标题", this);

    contextMenu->addAction(expandAllAction);
    contextMenu->addAction(collapseAllAction);
    contextMenu->addSeparator();
    contextMenu->addAction(copyTitleAction);
}

void PDFOutlineWidget::setupConnections() {
    connect(this, &QTreeWidget::itemClicked, this,
            &PDFOutlineWidget::onItemClicked);
    connect(this, &QTreeWidget::itemDoubleClicked, this,
            &PDFOutlineWidget::onItemDoubleClicked);
    connect(this, &QTreeWidget::itemSelectionChanged, this,
            &PDFOutlineWidget::onItemSelectionChanged);

    connect(expandAllAction, &QAction::triggered, this,
            &PDFOutlineWidget::onExpandAllRequested);
    connect(collapseAllAction, &QAction::triggered, this,
            &PDFOutlineWidget::onCollapseAllRequested);
    connect(copyTitleAction, &QAction::triggered, this,
            &PDFOutlineWidget::onCopyTitleRequested);
}

void PDFOutlineWidget::setOutlineModel(PDFOutlineModel* model) {
    if (outlineModel) {
        disconnect(outlineModel, nullptr, this, nullptr);
    }

    outlineModel = model;

    if (outlineModel) {
        connect(outlineModel, &PDFOutlineModel::outlineParsed, this,
                &PDFOutlineWidget::onOutlineParsed);
        connect(outlineModel, &PDFOutlineModel::outlineCleared, this,
                &PDFOutlineWidget::onOutlineCleared);

        // 如果模型已有数据，立即刷新
        if (outlineModel->hasOutline()) {
            refreshOutline();
        }
    }
}

void PDFOutlineWidget::refreshOutline() {
    if (!outlineModel || !outlineModel->hasOutline()) {
        clearOutline();
        return;
    }

    buildOutlineTree();
}

void PDFOutlineWidget::clearOutline() {
    clear();
    currentHighlightedItem = nullptr;

    // 显示空状态
    QTreeWidgetItem* emptyItem = new QTreeWidgetItem(this);
    emptyItem->setText(0, "无目录信息");
    emptyItem->setFlags(Qt::NoItemFlags);
    QFont font = emptyItem->font(0);
    font.setItalic(true);
    emptyItem->setFont(0, font);
    emptyItem->setForeground(0, QBrush(QColor(128, 128, 128)));
}

void PDFOutlineWidget::buildOutlineTree() {
    clear();
    currentHighlightedItem = nullptr;

    if (!outlineModel || !outlineModel->hasOutline()) {
        return;
    }

    const auto& rootNodes = outlineModel->getRootNodes();
    addOutlineNodes(nullptr, rootNodes);

    // 默认展开第一层
    expandToLevel(0);
}

void PDFOutlineWidget::addOutlineNodes(
    QTreeWidgetItem* parentItem,
    const QList<std::shared_ptr<PDFOutlineNode>>& nodes) {
    for (const auto& node : nodes) {
        QTreeWidgetItem* item = createOutlineItem(node, parentItem);
        setItemStyle(item, node);

        // 递归添加子节点
        if (node->hasChildren) {
            addOutlineNodes(item, node->children);
        }
    }
}

QTreeWidgetItem* PDFOutlineWidget::createOutlineItem(
    const std::shared_ptr<PDFOutlineNode>& node, QTreeWidgetItem* parent) {
    QTreeWidgetItem* item;
    if (parent) {
        item = new QTreeWidgetItem(parent);
    } else {
        item = new QTreeWidgetItem(this);
    }

    item->setText(0, node->title);
    item->setData(0, PageNumberRole, node->pageNumber);

    // 存储节点指针（用于后续操作）
    item->setData(0, NodePtrRole, QVariant::fromValue(node.get()));

    return item;
}

void PDFOutlineWidget::setItemStyle(
    QTreeWidgetItem* item, const std::shared_ptr<PDFOutlineNode>& node) {
    QFont font = item->font(0);

    // 根据层级设置字体大小
    if (node->level == 0) {
        font.setBold(true);
        font.setPointSize(font.pointSize() + 1);
    } else if (node->level == 1) {
        font.setBold(false);
        font.setPointSize(font.pointSize());
    } else {
        font.setBold(false);
        font.setPointSize(font.pointSize() - 1);
    }

    item->setFont(0, font);

    // 设置工具提示
    QString tooltip = node->title;
    if (node->isValidPageReference()) {
        tooltip += QString(" (第 %1 页)").arg(node->pageNumber + 1);
    }
    item->setToolTip(0, tooltip);

    // 如果没有有效的页面引用，设置为灰色
    if (!node->isValidPageReference()) {
        item->setForeground(0, QBrush(QColor(128, 128, 128)));
    }
}

void PDFOutlineWidget::onOutlineParsed() { refreshOutline(); }

void PDFOutlineWidget::onOutlineCleared() { clearOutline(); }

void PDFOutlineWidget::onItemClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column)

    if (!item)
        return;

    int pageNumber = getItemPageNumber(item);
    if (pageNumber >= 0) {
        emit pageNavigationRequested(pageNumber);
        highlightItem(item);
    }
}

void PDFOutlineWidget::onItemDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column)

    if (!item)
        return;

    // 双击时展开/折叠节点
    if (item->childCount() > 0) {
        item->setExpanded(!item->isExpanded());
    }
}

void PDFOutlineWidget::onItemSelectionChanged() {
    QTreeWidgetItem* current = currentItem();
    if (current) {
        int pageNumber = getItemPageNumber(current);
        if (pageNumber >= 0) {
            emit itemSelectionChanged(pageNumber);
        }
    }
}

int PDFOutlineWidget::getItemPageNumber(QTreeWidgetItem* item) const {
    if (!item)
        return -1;

    QVariant data = item->data(0, PageNumberRole);
    return data.toInt();
}

int PDFOutlineWidget::getCurrentSelectedPage() const {
    QTreeWidgetItem* current = currentItem();
    return getItemPageNumber(current);
}

void PDFOutlineWidget::highlightPageItem(int pageNumber) {
    QTreeWidgetItem* item = findItemByPage(pageNumber);
    if (item) {
        highlightItem(item);
        scrollToItem(item);
    }
}

void PDFOutlineWidget::expandAll() { QTreeWidget::expandAll(); }

void PDFOutlineWidget::collapseAll() { QTreeWidget::collapseAll(); }

void PDFOutlineWidget::expandToLevel(int level) {
    collapseAll();

    QTreeWidgetItemIterator it(this);
    while (*it) {
        auto node = static_cast<PDFOutlineNode*>(
            (*it)->data(0, NodePtrRole).value<void*>());
        if (node && node->level <= level) {
            (*it)->setExpanded(true);
        }
        ++it;
    }
}

void PDFOutlineWidget::searchItems(const QString& searchText) {
    if (searchText.isEmpty()) {
        // 显示所有项目
        QTreeWidgetItemIterator it(this);
        while (*it) {
            (*it)->setHidden(false);
            ++it;
        }
        return;
    }

    // 隐藏所有项目，然后显示匹配的项目
    QTreeWidgetItemIterator it(this);
    while (*it) {
        (*it)->setHidden(true);
        ++it;
    }

    // 搜索并显示匹配的项目
    bool found = false;
    searchItemsRecursive(invisibleRootItem(), searchText, found);
}

QTreeWidgetItem* PDFOutlineWidget::findItemByPage(int pageNumber,
                                                  QTreeWidgetItem* parent) {
    if (!parent) {
        parent = invisibleRootItem();
    }

    for (int i = 0; i < parent->childCount(); ++i) {
        QTreeWidgetItem* child = parent->child(i);
        if (getItemPageNumber(child) == pageNumber) {
            return child;
        }

        QTreeWidgetItem* found = findItemByPage(pageNumber, child);
        if (found) {
            return found;
        }
    }

    return nullptr;
}

void PDFOutlineWidget::highlightItem(QTreeWidgetItem* item) {
    clearHighlight();

    if (item) {
        currentHighlightedItem = item;
        setCurrentItem(item);
        item->setSelected(true);
    }
}

void PDFOutlineWidget::clearHighlight() {
    if (currentHighlightedItem) {
        currentHighlightedItem->setSelected(false);
        currentHighlightedItem = nullptr;
    }
}

void PDFOutlineWidget::searchItemsRecursive(QTreeWidgetItem* item,
                                            const QString& searchText,
                                            bool& found) {
    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem* child = item->child(i);

        if (child->text(0).contains(searchText, Qt::CaseInsensitive)) {
            child->setHidden(false);
            found = true;

            // 显示所有父项
            QTreeWidgetItem* parent = child->parent();
            while (parent) {
                parent->setHidden(false);
                parent->setExpanded(true);
                parent = parent->parent();
            }
        }

        searchItemsRecursive(child, searchText, found);
    }
}

void PDFOutlineWidget::contextMenuEvent(QContextMenuEvent* event) {
    QTreeWidgetItem* item = itemAt(event->pos());

    // 根据是否有选中项目启用/禁用菜单项
    copyTitleAction->setEnabled(item != nullptr);

    contextMenu->exec(event->globalPos());
}

void PDFOutlineWidget::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (currentItem()) {
                onItemClicked(currentItem(), 0);
            }
            break;
        case Qt::Key_Space:
            if (currentItem() && currentItem()->childCount() > 0) {
                currentItem()->setExpanded(!currentItem()->isExpanded());
            }
            break;
        default:
            QTreeWidget::keyPressEvent(event);
            break;
    }
}

void PDFOutlineWidget::onExpandAllRequested() { expandAll(); }

void PDFOutlineWidget::onCollapseAllRequested() { collapseAll(); }

void PDFOutlineWidget::onCopyTitleRequested() {
    QTreeWidgetItem* item = currentItem();
    if (item) {
        QApplication::clipboard()->setText(item->text(0));
    }
}
