#ifndef OUTLINEPANEL_H
#define OUTLINEPANEL_H

#include <QWidget>
#include <memory>

// Forward declarations
class QTreeView;
class ElaPushButton;
class PDFOutlineModel;
class QStandardItemModel;

namespace Poppler {
class Document;
}

/**
 * @brief ElaOutlinePanel - 大纲/目录面板
 *
 * 显示 PDF 文档的大纲/目录结构
 *
 * 复用现有业务逻辑：
 * - PDFOutlineModel - PDF 大纲数据模型
 */
class OutlinePanel : public QWidget {
    Q_OBJECT

public:
    explicit OutlinePanel(QWidget* parent = nullptr);
    ~OutlinePanel() override;

    void setDocument(std::shared_ptr<Poppler::Document> document);
    void clearDocument();
    void refresh();
    void expandAll();
    void collapseAll();

    void setOutlineModel(PDFOutlineModel* model);

signals:
    void outlineItemClicked(int pageNumber);

protected:
    void changeEvent(QEvent* event) override;

private:
    QTreeView* m_treeView;
    ElaPushButton* m_expandAllBtn;
    ElaPushButton* m_collapseAllBtn;
    PDFOutlineModel* m_outlineModel;
    QStandardItemModel* m_treeModel;
    std::shared_ptr<Poppler::Document> m_document;

    void setupUi();
    void connectSignals();
    void retranslateUi();
    void buildTreeFromOutline();
};

#endif  // OUTLINEPANEL_H
