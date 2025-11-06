#ifndef BOOKMARKPANEL_H
#define BOOKMARKPANEL_H

#include <QString>
#include <QWidget>
#include <memory>

// Forward declarations
class QListView;
class ElaToolButton;
class BookmarkModel;

namespace Poppler {
class Document;
}

/**
 * @brief ElaBookmarkPanel - 书签面板
 *
 * 显示和管理书签
 *
 * 复用现有业务逻辑：
 * - BookmarkModel - 书签数据模型
 */
class BookmarkPanel : public QWidget {
    Q_OBJECT

public:
    explicit BookmarkPanel(QWidget* parent = nullptr);
    ~BookmarkPanel() override;

    // 文档管理
    void setDocument(std::shared_ptr<Poppler::Document> document);
    void clearDocument();

    // 书签管理
    void addBookmark(int pageNumber, const QString& title = QString());
    void removeBookmark(int pageNumber);
    void clearBookmarks();
    bool exportBookmarks(const QString& filePath);
    bool importBookmarks(const QString& filePath);

    void setBookmarkModel(BookmarkModel* model);

signals:
    void bookmarkSelected(int pageNumber);
    void bookmarkAdded(int pageNumber, const QString& title);
    void bookmarkRemoved(int pageNumber);

protected:
    void changeEvent(QEvent* event) override;

private:
    QListView* m_listView;
    ElaToolButton* m_addBtn;
    ElaToolButton* m_removeBtn;
    ElaToolButton* m_clearBtn;
    ElaToolButton* m_exportBtn;
    ElaToolButton* m_importBtn;
    BookmarkModel* m_model;
    std::shared_ptr<Poppler::Document> m_document;

    void setupUi();
    void connectSignals();
    void retranslateUi();
};

#endif  // BOOKMARKPANEL_H
