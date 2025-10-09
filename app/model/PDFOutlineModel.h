#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QList>
#include <QObject>
#include <QString>
#include <memory>

// PDF目录节点数据结构
struct PDFOutlineNode {
    QString title;     // 目录项标题
    int pageNumber;    // 目标页面号（0-based）
    int level;         // 层级深度（0为根级）
    bool hasChildren;  // 是否有子节点
    QList<std::shared_ptr<PDFOutlineNode>> children;  // 子节点列表

    PDFOutlineNode(const QString& title = "", int page = -1, int level = 0)
        : title(title), pageNumber(page), level(level), hasChildren(false) {}

    // 添加子节点
    void addChild(std::shared_ptr<PDFOutlineNode> child) {
        children.append(child);
        hasChildren = true;
    }

    // 获取子节点数量
    int childCount() const { return children.size(); }

    // 检查是否为有效的页面引用
    bool isValidPageReference() const { return pageNumber >= 0; }
};

// PDF目录模型类
class PDFOutlineModel : public QObject {
    Q_OBJECT

public:
    explicit PDFOutlineModel(QObject* parent = nullptr);
    ~PDFOutlineModel() = default;

    // 解析PDF文档的目录
    bool parseOutline(Poppler::Document* document);

    // 清空目录
    void clear();

    // 获取根节点列表
    const QList<std::shared_ptr<PDFOutlineNode>>& getRootNodes() const;

    // 检查是否有目录
    bool hasOutline() const;

    // 获取目录项总数
    int getTotalItemCount() const;

    // 根据页面号查找对应的目录节点
    std::shared_ptr<PDFOutlineNode> findNodeByPage(int pageNumber) const;

    // 获取扁平化的目录列表（用于搜索等功能）
    QList<std::shared_ptr<PDFOutlineNode>> getFlattenedNodes() const;

signals:
    void outlineParsed();
    void outlineCleared();

private:
    QList<std::shared_ptr<PDFOutlineNode>> rootNodes;
    int totalItemCount;

    // 递归解析目录项
    std::shared_ptr<PDFOutlineNode> parseOutlineItem(
        const QList<Poppler::OutlineItem>& items, int level = 0);

    // 递归解析单个目录项
    void parseOutlineItemRecursive(const Poppler::OutlineItem& item,
                                   std::shared_ptr<PDFOutlineNode> parentNode,
                                   int level);

    // 计算目录项总数
    int countNodes(const QList<std::shared_ptr<PDFOutlineNode>>& nodes) const;

    // 递归查找节点
    std::shared_ptr<PDFOutlineNode> findNodeByPageRecursive(
        const QList<std::shared_ptr<PDFOutlineNode>>& nodes,
        int pageNumber) const;

    // 递归获取扁平化列表
    void flattenNodesRecursive(
        const QList<std::shared_ptr<PDFOutlineNode>>& nodes,
        QList<std::shared_ptr<PDFOutlineNode>>& result) const;
};
