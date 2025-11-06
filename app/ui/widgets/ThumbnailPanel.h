#ifndef THUMBNAILPANEL_H
#define THUMBNAILPANEL_H

#include <QWidget>
#include <memory>

// Forward declarations
class QListView;
class ThumbnailModel;
class ThumbnailDelegate;

namespace Poppler {
class Document;
}

/**
 * @brief ElaThumbnailPanel - 缩略图面板
 *
 * 显示 PDF 文档所有页面的缩略图
 *
 * 复用现有业务逻辑：
 * - ThumbnailModel - 缩略图数据模型
 * - ThumbnailDelegate - 缩略图渲染代理
 */
class ThumbnailPanel : public QWidget {
    Q_OBJECT

public:
    explicit ThumbnailPanel(QWidget* parent = nullptr);
    ~ThumbnailPanel() override;

    void setDocument(std::shared_ptr<Poppler::Document> document);
    void clearDocument();
    void setCurrentPage(int pageNumber);
    void setThumbnailSize(int size);
    void refresh();

    void setThumbnailModel(ThumbnailModel* model);
    void setThumbnailDelegate(ThumbnailDelegate* delegate);

signals:
    void pageSelected(int pageNumber);

private:
    QListView* m_listView;
    ThumbnailModel* m_model;
    ThumbnailDelegate* m_delegate;
    std::shared_ptr<Poppler::Document> m_document;
    int m_currentPage;

    void setupUi();
    void connectSignals();
};

#endif  // THUMBNAILPANEL_H
