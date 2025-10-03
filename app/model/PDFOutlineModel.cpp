#include "PDFOutlineModel.h"
#include <QDebug>
#include <QStringList>

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

    // 使用专门的方法解析目录项
    try {
        auto rootNode = parseOutlineItem(outline, 0);
        if (rootNode && !rootNode->children.isEmpty()) {
            rootNodes = rootNode->children;
        }

        totalItemCount = countNodes(rootNodes);

        qDebug() << "PDFOutlineModel: Parsed" << totalItemCount
                 << "outline items";
        emit outlineParsed();
        return true;
    } catch (const std::exception& e) {
        qWarning() << "PDFOutlineModel: Error parsing outline:" << e.what();
        clear();
        return false;
    }
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

std::shared_ptr<PDFOutlineNode> PDFOutlineModel::parseOutlineItem(
    const QList<Poppler::OutlineItem>& items, int level) {
    // 创建一个虚拟根节点来容纳所有顶级项
    auto rootNode = std::make_shared<PDFOutlineNode>("", -1, level - 1);

    // 防止递归深度过大
    const int MAX_DEPTH = 50;
    if (level > MAX_DEPTH) {
        qWarning() << "PDFOutlineModel: Maximum recursion depth reached";
        return rootNode;
    }

    // 解析每个目录项
    for (const auto& item : items) {
        auto node = std::make_shared<PDFOutlineNode>();
        parseOutlineItemRecursive(item, node, level);

        // 只添加有效的节点
        if (!node->title.isEmpty()) {
            rootNode->addChild(node);
        }
    }

    return rootNode;
}

void PDFOutlineModel::parseOutlineItemRecursive(
    const Poppler::OutlineItem& item, std::shared_ptr<PDFOutlineNode> node,
    int level) {
    if (!node) {
        qWarning() << "PDFOutlineModel: Node is null";
        return;
    }

    // 防止递归深度过大
    const int MAX_DEPTH = 50;
    if (level > MAX_DEPTH) {
        qWarning() << "PDFOutlineModel: Maximum recursion depth reached";
        return;
    }

    // 设置节点基本信息
    node->title = item.name().trimmed();  // 去除首尾空白
    node->level = level;
    node->pageNumber = -1;  // 默认无效页面

    // 获取目标页面
    try {
        if (item.destination()) {
            auto dest = item.destination();
            if (dest && dest->pageNumber() > 0) {
                node->pageNumber = dest->pageNumber() - 1;  // 转换为0-based
            }
        }
    } catch (const std::exception& e) {
        qWarning() << "PDFOutlineModel: Error getting destination for item"
                   << node->title << ":" << e.what();
    }

    // 递归处理子项
    if (item.hasChildren()) {
        try {
            QList<Poppler::OutlineItem> children = item.children();
            for (const auto& child : children) {
                auto childNode = std::make_shared<PDFOutlineNode>();
                parseOutlineItemRecursive(child, childNode, level + 1);

                // 只添加有效的子节点
                if (!childNode->title.isEmpty()) {
                    // 手动设置父子关系，因为addChild需要shared_from_this
                    childNode->parent = std::weak_ptr<PDFOutlineNode>(node);
                    node->children.append(childNode);
                    node->hasChildren = true;
                }
            }
        } catch (const std::exception& e) {
            qWarning() << "PDFOutlineModel: Error processing children for item"
                       << node->title << ":" << e.what();
        }
    }
}

int PDFOutlineModel::countNodes(
    const QList<std::shared_ptr<PDFOutlineNode>>& nodes) const {
    int count = 0;

    for (const auto& node : nodes) {
        if (node) {
            count++;
            count += countNodes(node->children);
        }
    }

    return count;
}

std::shared_ptr<PDFOutlineNode> PDFOutlineModel::findNodeByPageRecursive(
    const QList<std::shared_ptr<PDFOutlineNode>>& nodes, int pageNumber) const {
    // 检查页面号有效性
    if (pageNumber < 0) {
        return nullptr;
    }

    for (const auto& node : nodes) {
        if (!node) {
            continue;
        }

        // 检查当前节点
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

QList<std::shared_ptr<PDFOutlineNode>> PDFOutlineModel::searchByTitle(
    const QString& title, bool caseSensitive) const {
    QList<std::shared_ptr<PDFOutlineNode>> result;
    if (title.isEmpty()) {
        return result;
    }

    searchByTitleRecursive(rootNodes, title, caseSensitive, result);
    return result;
}

QList<std::shared_ptr<PDFOutlineNode>> PDFOutlineModel::getNodesByLevel(
    int level) const {
    QList<std::shared_ptr<PDFOutlineNode>> result;
    if (level < 0) {
        return result;
    }

    getNodesByLevelRecursive(rootNodes, level, result);
    return result;
}

int PDFOutlineModel::getMaxDepth() const {
    return getMaxDepthRecursive(rootNodes);
}

void PDFOutlineModel::flattenNodesRecursive(
    const QList<std::shared_ptr<PDFOutlineNode>>& nodes,
    QList<std::shared_ptr<PDFOutlineNode>>& result) const {
    for (const auto& node : nodes) {
        if (node) {
            result.append(node);
            flattenNodesRecursive(node->children, result);
        }
    }
}

void PDFOutlineModel::searchByTitleRecursive(
    const QList<std::shared_ptr<PDFOutlineNode>>& nodes, const QString& title,
    bool caseSensitive, QList<std::shared_ptr<PDFOutlineNode>>& result) const {
    for (const auto& node : nodes) {
        if (!node) {
            continue;
        }

        // 检查标题是否匹配
        bool matches = false;
        if (caseSensitive) {
            matches = node->title.contains(title);
        } else {
            matches = node->title.contains(title, Qt::CaseInsensitive);
        }

        if (matches) {
            result.append(node);
        }

        // 递归搜索子节点
        searchByTitleRecursive(node->children, title, caseSensitive, result);
    }
}

void PDFOutlineModel::getNodesByLevelRecursive(
    const QList<std::shared_ptr<PDFOutlineNode>>& nodes, int targetLevel,
    QList<std::shared_ptr<PDFOutlineNode>>& result) const {
    for (const auto& node : nodes) {
        if (!node) {
            continue;
        }

        if (node->level == targetLevel) {
            result.append(node);
        }

        // 继续搜索子节点
        getNodesByLevelRecursive(node->children, targetLevel, result);
    }
}

int PDFOutlineModel::getMaxDepthRecursive(
    const QList<std::shared_ptr<PDFOutlineNode>>& nodes) const {
    int maxDepth = -1;

    for (const auto& node : nodes) {
        if (!node) {
            continue;
        }

        maxDepth = qMax(maxDepth, node->level);

        // 递归检查子节点
        int childMaxDepth = getMaxDepthRecursive(node->children);
        maxDepth = qMax(maxDepth, childMaxDepth);
    }

    return maxDepth;
}

// PDFOutlineNode 方法实现
QString PDFOutlineNode::getFullPath(const QString& separator) const {
    QStringList pathParts;

    // 从当前节点向上遍历到根节点
    auto current = shared_from_this();
    while (current) {
        if (!current->title.isEmpty()) {
            pathParts.prepend(current->title);
        }

        auto parentPtr = current->parent.lock();
        if (!parentPtr || parentPtr->title.isEmpty()) {
            break;
        }
        current = parentPtr;
    }

    return pathParts.join(separator);
}

std::shared_ptr<PDFOutlineNode> PDFOutlineNode::findChildByTitle(
    const QString& title, bool caseSensitive) const {
    for (const auto& child : children) {
        if (!child) {
            continue;
        }

        bool matches = false;
        if (caseSensitive) {
            matches = (child->title == title);
        } else {
            matches = (child->title.compare(title, Qt::CaseInsensitive) == 0);
        }

        if (matches) {
            return child;
        }
    }

    return nullptr;
}

int PDFOutlineNode::getDescendantCount() const {
    int count = children.size();

    for (const auto& child : children) {
        if (child) {
            count += child->getDescendantCount();
        }
    }

    return count;
}

int PDFOutlineNode::getSiblingIndex() const {
    auto parentPtr = parent.lock();
    if (!parentPtr) {
        return -1;  // 根节点或孤立节点
    }

    for (int i = 0; i < parentPtr->children.size(); ++i) {
        if (parentPtr->children[i].get() == this) {
            return i;
        }
    }

    return -1;  // 未找到（不应该发生）
}
