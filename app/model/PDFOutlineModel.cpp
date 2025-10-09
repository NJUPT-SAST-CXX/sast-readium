#include "PDFOutlineModel.h"
#include <QDebug>

PDFOutlineModel::PDFOutlineModel(QObject* parent)
    : QObject(parent), totalItemCount(0) {}

bool PDFOutlineModel::parseOutline(Poppler::Document* document) {
    clear();

    if (!document) {
        qWarning() << "PDFOutlineModel: Document is null";
        return false;
    }

    // 获取PDF文档的目录
    QList<Poppler::OutlineItem> outline = document->outline();
    if (outline.isEmpty()) {
        qDebug() << "PDFOutlineModel: Document has no outline";
        return false;
    }

    // 解析目录项
    for (const auto& item : outline) {
        auto node = std::make_shared<PDFOutlineNode>();
        parseOutlineItemRecursive(item, node, 0);
        if (!node->title.isEmpty()) {
            rootNodes.append(node);
        }
    }

    totalItemCount = countNodes(rootNodes);

    qDebug() << "PDFOutlineModel: Parsed" << totalItemCount << "outline items";
    emit outlineParsed();
    return true;
}

void PDFOutlineModel::clear() {
    rootNodes.clear();
    totalItemCount = 0;
    emit outlineCleared();
}

const QList<std::shared_ptr<PDFOutlineNode>>& PDFOutlineModel::getRootNodes()
    const {
    return rootNodes;
}

bool PDFOutlineModel::hasOutline() const { return !rootNodes.isEmpty(); }

int PDFOutlineModel::getTotalItemCount() const { return totalItemCount; }

std::shared_ptr<PDFOutlineNode> PDFOutlineModel::findNodeByPage(
    int pageNumber) const {
    return findNodeByPageRecursive(rootNodes, pageNumber);
}

QList<std::shared_ptr<PDFOutlineNode>> PDFOutlineModel::getFlattenedNodes()
    const {
    QList<std::shared_ptr<PDFOutlineNode>> result;
    flattenNodesRecursive(rootNodes, result);
    return result;
}

void PDFOutlineModel::parseOutlineItemRecursive(
    const Poppler::OutlineItem& item, std::shared_ptr<PDFOutlineNode> node,
    int level) {
    // 设置节点基本信息
    node->title = item.name();
    node->level = level;

    // 获取目标页面
    if (item.destination()) {
        auto dest = item.destination();
        if (dest->pageNumber() > 0) {
            node->pageNumber = dest->pageNumber() - 1;  // 转换为0-based
        }
    }

    // 递归处理子项
    if (item.hasChildren()) {
        QList<Poppler::OutlineItem> children = item.children();
        for (const auto& child : children) {
            auto childNode = std::make_shared<PDFOutlineNode>();
            parseOutlineItemRecursive(child, childNode, level + 1);
            if (!childNode->title.isEmpty()) {
                node->addChild(childNode);
            }
        }
    }
}

int PDFOutlineModel::countNodes(
    const QList<std::shared_ptr<PDFOutlineNode>>& nodes) const {
    int count = nodes.size();
    for (const auto& node : nodes) {
        count += countNodes(node->children);
    }
    return count;
}

std::shared_ptr<PDFOutlineNode> PDFOutlineModel::findNodeByPageRecursive(
    const QList<std::shared_ptr<PDFOutlineNode>>& nodes, int pageNumber) const {
    for (const auto& node : nodes) {
        if (node->pageNumber == pageNumber) {
            return node;
        }

        // 递归搜索子节点
        auto found = findNodeByPageRecursive(node->children, pageNumber);
        if (found) {
            return found;
        }
    }

    return nullptr;
}

void PDFOutlineModel::flattenNodesRecursive(
    const QList<std::shared_ptr<PDFOutlineNode>>& nodes,
    QList<std::shared_ptr<PDFOutlineNode>>& result) const {
    for (const auto& node : nodes) {
        result.append(node);
        flattenNodesRecursive(node->children, result);
    }
}
