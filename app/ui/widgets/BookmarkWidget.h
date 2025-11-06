#pragma once

#include <QAction>

#include <QContextMenuEvent>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>

#include <QListView>

#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include "../../model/BookmarkModel.h"
#include "ElaTreeView.h"

class QSortFilterProxyModel;
class ElaPushButton;
class ElaLineEdit;
class ElaComboBox;
class ElaText;
class ElaMenu;

/**
 * Comprehensive bookmark management widget
 */
class BookmarkWidget : public QWidget {
    Q_OBJECT

public:
    explicit BookmarkWidget(QWidget* parent = nullptr);
    ~BookmarkWidget() = default;

    // Model access
    BookmarkModel* getBookmarkModel() const { return m_bookmarkModel; }
    void setCurrentDocument(const QString& documentPath);

    // Bookmark operations
    bool addBookmark(const QString& documentPath, int pageNumber,
                     const QString& title = QString());
    bool removeBookmark(const QString& bookmarkId);
    bool hasBookmarkForPage(const QString& documentPath, int pageNumber) const;

    // UI state
    void refreshView();
    void expandAll();
    void collapseAll();

signals:
    void bookmarkSelected(const Bookmark& bookmark);
    void navigateToBookmark(const QString& documentPath, int pageNumber);
    void bookmarkAdded(const Bookmark& bookmark);
    void bookmarkRemoved(const QString& bookmarkId);
    void bookmarkUpdated(const Bookmark& bookmark);

protected:
    void changeEvent(QEvent* event) override;

public slots:
    void onBookmarkDoubleClicked(const QModelIndex& index);
    void onBookmarkSelectionChanged();
    void onAddBookmarkRequested();
    void onRemoveBookmarkRequested();
    void onEditBookmarkRequested();
    void showContextMenu(const QPoint& position);

private slots:
    void onSearchTextChanged();
    void onCategoryFilterChanged();
    void onSortOrderChanged();

private:
    void setupUI();
    void setupConnections();
    void setupContextMenu();
    void retranslateUi();
    void updateBookmarkActions();
    void updateCategoryFilter();
    void filterBookmarks();
    Bookmark getSelectedBookmark() const;
    QModelIndex getSelectedIndex() const;

    // UI Components
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_toolbarLayout;
    QHBoxLayout* m_filterLayout;

    // Toolbar
    ElaPushButton* m_addButton;
    ElaPushButton* m_removeButton;
    ElaPushButton* m_editButton;
    ElaPushButton* m_refreshButton;

    // Filter controls
    ElaLineEdit* m_searchEdit;
    ElaComboBox* m_categoryFilter;
    ElaComboBox* m_sortOrder;
    ElaText* m_countLabel;

    // Main view
    ElaTreeView* m_bookmarkView;
    QSortFilterProxyModel* m_proxyModel;

    // Context menu
    ElaMenu* m_contextMenu;
    QAction* m_navigateAction;
    QAction* m_editAction;
    QAction* m_deleteAction;
    QAction* m_addCategoryAction;
    QAction* m_removeCategoryAction;

    // Data
    BookmarkModel* m_bookmarkModel;
    QString m_currentDocument;
};
