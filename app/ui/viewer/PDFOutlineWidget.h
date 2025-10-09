#pragma once

#include <QAction>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <memory>
#include "../../model/PDFOutlineModel.h"

class PDFOutlineWidget : public QTreeWidget {
    Q_OBJECT

public:
    explicit PDFOutlineWidget(QWidget* parent = nullptr);
    ~PDFOutlineWidget() = default;

    // 设置目录模型
    void setOutlineModel(PDFOutlineModel* model);

    // 刷新目录显示
    void refreshOutline();

    // 清空目录
    void clearOutline();

    // 高亮指定页面对应的目录项
    void highlightPageItem(int pageNumber);

    // 展开所有节点
    void expandAll();

    // 折叠所有节点
    void collapseAll();

    // 展开到指定层级
    void expandToLevel(int level);

    // 搜索目录项
    void searchItems(const QString& searchText);

    // 获取当前选中的页面号
    int getCurrentSelectedPage() const;

public slots:
    void onOutlineParsed();
    void onOutlineCleared();

signals:
    void pageNavigationRequested(int pageNumber);
    void itemSelectionChanged(int pageNumber);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onItemSelectionChanged();
    void onExpandAllRequested();
    void onCollapseAllRequested();
    void onCopyTitleRequested();

private:
    PDFOutlineModel* outlineModel;
    QTreeWidgetItem* currentHighlightedItem;
    QMenu* contextMenu;
    QAction* expandAllAction;
    QAction* collapseAllAction;
    QAction* copyTitleAction;

    // 初始化UI
    void setupUI();
    void setupContextMenu();
    void setupConnections();

    // 构建目录树
    void buildOutlineTree();
    void addOutlineNodes(QTreeWidgetItem* parentItem,
                         const QList<std::shared_ptr<PDFOutlineNode>>& nodes);

    // 创建目录项
    QTreeWidgetItem* createOutlineItem(
        const std::shared_ptr<PDFOutlineNode>& node,
        QTreeWidgetItem* parent = nullptr);

    // 设置项目样式
    void setItemStyle(QTreeWidgetItem* item,
                      const std::shared_ptr<PDFOutlineNode>& node);

    // 查找包含指定页面的项目
    QTreeWidgetItem* findItemByPage(int pageNumber,
                                    QTreeWidgetItem* parent = nullptr);

    // 高亮项目
    void highlightItem(QTreeWidgetItem* item);
    void clearHighlight();

    // 获取项目对应的页面号
    int getItemPageNumber(QTreeWidgetItem* item) const;

    // 递归搜索项目
    void searchItemsRecursive(QTreeWidgetItem* item, const QString& searchText,
                              bool& found);

    // 保存和恢复展开状态
    void saveExpandedState();
    void restoreExpandedState();
    QStringList getExpandedItems(QTreeWidgetItem* parent = nullptr) const;
    void setExpandedItems(const QStringList& expandedPaths,
                          QTreeWidgetItem* parent = nullptr);
    QString getItemPath(QTreeWidgetItem* item) const;
};
