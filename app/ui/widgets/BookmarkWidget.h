#pragma once

#include <QAction>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QPushButton>
#include <QSplitter>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>
#include "../../model/BookmarkModel.h"

class QSortFilterProxyModel;

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
    QPushButton* m_addButton;
    QPushButton* m_removeButton;
    QPushButton* m_editButton;
    QPushButton* m_refreshButton;

    // Filter controls
    QLineEdit* m_searchEdit;
    QComboBox* m_categoryFilter;
    QComboBox* m_sortOrder;
    QLabel* m_countLabel;

    // Main view
    QTreeView* m_bookmarkView;
    QSortFilterProxyModel* m_proxyModel;

    // Context menu
    QMenu* m_contextMenu;
    QAction* m_navigateAction;
    QAction* m_editAction;
    QAction* m_deleteAction;
    QAction* m_addCategoryAction;
    QAction* m_removeCategoryAction;

    // Data
    BookmarkModel* m_bookmarkModel;
    QString m_currentDocument;
};
