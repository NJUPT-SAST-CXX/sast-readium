#include "OutlinePanel.h"

// ElaWidgetTools
#include "ElaIcon.h"
#include "ElaPushButton.h"
#include "ElaTreeView.h"

// Qt
#include <QEvent>
#include <QHBoxLayout>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QVBoxLayout>

// Logging
#include "logging/SimpleLogging.h"

// Business logic
#include "model/PDFOutlineModel.h"

OutlinePanel::OutlinePanel(QWidget* parent)
    : QWidget(parent),
      m_treeView(nullptr),
      m_expandAllBtn(nullptr),
      m_collapseAllBtn(nullptr),
      m_outlineModel(nullptr),
      m_treeModel(nullptr),
      m_document(nullptr) {
    SLOG_INFO("OutlinePanel: Constructor started");

    setupUi();
    connectSignals();

    SLOG_INFO("OutlinePanel: Constructor completed");
}

OutlinePanel::~OutlinePanel() { SLOG_INFO("OutlinePanel: Destructor called"); }

void OutlinePanel::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);

    // 工具栏
    auto* toolbarLayout = new QHBoxLayout();

    m_expandAllBtn = new ElaPushButton(tr("Expand All"), this);
    m_expandAllBtn->setIcon(
        ElaIcon::getInstance()->getElaIcon(ElaIconType::Plus));
    toolbarLayout->addWidget(m_expandAllBtn);

    m_collapseAllBtn = new ElaPushButton(tr("Collapse All"), this);
    m_collapseAllBtn->setIcon(
        ElaIcon::getInstance()->getElaIcon(ElaIconType::Minus));
    toolbarLayout->addWidget(m_collapseAllBtn);

    toolbarLayout->addStretch();

    layout->addLayout(toolbarLayout);

    // 大纲树视图
    m_treeView = new ElaTreeView(this);
    m_treeView->setHeaderHidden(true);
    m_treeView->setAnimated(true);

    // 创建树模型
    m_treeModel = new QStandardItemModel(this);
    m_treeView->setModel(m_treeModel);

    layout->addWidget(m_treeView);
}

void OutlinePanel::connectSignals() {
    connect(m_expandAllBtn, &ElaPushButton::clicked, this,
            &OutlinePanel::expandAll);
    connect(m_collapseAllBtn, &ElaPushButton::clicked, this,
            &OutlinePanel::collapseAll);

    connect(m_treeView, &ElaTreeView::clicked, this,
            [this](const QModelIndex& index) {
                if (index.isValid()) {
                    int pageNumber = index.data(Qt::UserRole).toInt();
                    if (pageNumber > 0) {
                        emit outlineItemClicked(pageNumber);
                    }
                }
            });
}

void OutlinePanel::setDocument(std::shared_ptr<Poppler::Document> document) {
    SLOG_INFO("OutlinePanel: Setting document");

    m_document = document;

    if (!m_outlineModel) {
        m_outlineModel = new PDFOutlineModel(this);
    }

    // 解析文档大纲
    if (document) {
        m_outlineModel->parseOutline(document.get());
        buildTreeFromOutline();
    }
}

void OutlinePanel::clearDocument() {
    SLOG_INFO("OutlinePanel: Clearing document");

    m_document.reset();

    if (m_outlineModel) {
        m_outlineModel->clear();
    }

    if (m_treeModel) {
        m_treeModel->clear();
    }
}

void OutlinePanel::refresh() {
    if (m_document && m_outlineModel) {
        m_outlineModel->parseOutline(m_document.get());
        buildTreeFromOutline();
    }
}

void OutlinePanel::expandAll() { m_treeView->expandAll(); }

void OutlinePanel::collapseAll() { m_treeView->collapseAll(); }

void OutlinePanel::setOutlineModel(PDFOutlineModel* model) {
    m_outlineModel = model;
    buildTreeFromOutline();
}

void OutlinePanel::buildTreeFromOutline() {
    if (!m_outlineModel || !m_treeModel) {
        return;
    }

    m_treeModel->clear();

    if (!m_outlineModel->hasOutline()) {
        return;
    }

    // 递归构建树
    auto buildNode = [](QStandardItem* parentItem,
                        const std::shared_ptr<PDFOutlineNode>& node,
                        auto& buildNodeRef) -> void {
        if (!node) {
            return;
        }

        auto* item = new QStandardItem(node->title);
        item->setData(node->pageNumber + 1, Qt::UserRole);  // 转换为 1-based
        item->setEditable(false);

        if (parentItem) {
            parentItem->appendRow(item);
        }

        // 递归添加子节点
        for (const auto& child : node->children) {
            buildNodeRef(item, child, buildNodeRef);
        }
    };

    // 添加所有根节点
    for (const auto& rootNode : m_outlineModel->getRootNodes()) {
        buildNode(m_treeModel->invisibleRootItem(), rootNode, buildNode);
    }
}

void OutlinePanel::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void OutlinePanel::retranslateUi() {
    SLOG_INFO("OutlinePanel: Retranslating UI");

    m_expandAllBtn->setText(tr("Expand All"));
    m_collapseAllBtn->setText(tr("Collapse All"));
}
