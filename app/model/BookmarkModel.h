#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QList>
#include <QDateTime>
#include <QSettings>
#include <QJsonObject>
#include <QJsonArray>
#include <QRectF>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDir>

/**
 * Represents a single bookmark entry
 */
struct Bookmark {
    QString id;              // Unique identifier
    QString title;           // User-defined title
    QString documentPath;    // Path to the PDF document
    int pageNumber;          // Page number (0-based)
    QDateTime createdTime;   // When bookmark was created
    QDateTime lastAccessed;  // When bookmark was last accessed
    QString notes;           // Optional user notes
    QRectF highlightRect;    // Optional highlight rectangle
    QString category;        // Optional category/folder
    
    Bookmark() : pageNumber(-1) {}
    
    Bookmark(const QString& docPath, int page, const QString& bookmarkTitle = QString())
        : title(bookmarkTitle), documentPath(docPath), pageNumber(page)
        , createdTime(QDateTime::currentDateTime())
        , lastAccessed(QDateTime::currentDateTime())
    {
        // Generate unique ID based on timestamp and content
        id = QString("%1_%2_%3").arg(QDateTime::currentMSecsSinceEpoch())
                                .arg(qHash(docPath))
                                .arg(page);
        
        // Auto-generate title if not provided
        if (title.isEmpty()) {
            QFileInfo fileInfo(docPath);
            title = QString("%1 - Page %2").arg(fileInfo.baseName()).arg(page + 1);
        }
    }
    
    // Serialization
    QJsonObject toJson() const;
    static Bookmark fromJson(const QJsonObject& json);
    
    // Comparison operators
    bool operator==(const Bookmark& other) const { return id == other.id; }
    bool operator!=(const Bookmark& other) const { return !(*this == other); }
};

/**
 * Model for managing bookmarks with persistent storage
 */
class BookmarkModel : public QAbstractItemModel {
    Q_OBJECT

public:
    enum BookmarkRole {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        DocumentPathRole,
        PageNumberRole,
        CreatedTimeRole,
        LastAccessedRole,
        NotesRole,
        HighlightRectRole,
        CategoryRole
    };

    explicit BookmarkModel(QObject* parent = nullptr);
    ~BookmarkModel() = default;

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Bookmark operations
    bool addBookmark(const Bookmark& bookmark);
    bool removeBookmark(const QString& bookmarkId);
    bool updateBookmark(const QString& bookmarkId, const Bookmark& updatedBookmark);
    Bookmark getBookmark(const QString& bookmarkId) const;
    QList<Bookmark> getAllBookmarks() const;
    
    // Document-specific operations
    QList<Bookmark> getBookmarksForDocument(const QString& documentPath) const;
    bool hasBookmarkForPage(const QString& documentPath, int pageNumber) const;
    Bookmark getBookmarkForPage(const QString& documentPath, int pageNumber) const;
    
    // Category operations
    QStringList getCategories() const;
    QList<Bookmark> getBookmarksInCategory(const QString& category) const;
    bool moveBookmarkToCategory(const QString& bookmarkId, const QString& category);
    
    // Search and filtering
    QList<Bookmark> searchBookmarks(const QString& query) const;
    QList<Bookmark> getRecentBookmarks(int count = 10) const;

    // Statistics and utility methods
    int getBookmarkCount() const;
    int getBookmarkCountForDocument(const QString& documentPath) const;
    QStringList getDocumentPaths() const;
    void clearAllBookmarks();
    bool exportBookmarks(const QString& filePath) const;
    bool importBookmarks(const QString& filePath);
    
    // Persistence
    bool saveToFile();
    bool loadFromFile();
    void setAutoSave(bool enabled) { m_autoSave = enabled; }
    bool isAutoSaveEnabled() const { return m_autoSave; }

signals:
    void bookmarkAdded(const Bookmark& bookmark);
    void bookmarkRemoved(const QString& bookmarkId);
    void bookmarkUpdated(const Bookmark& bookmark);
    void bookmarksLoaded(int count);
    void bookmarksSaved(int count);
    void bookmarksCleared();
    void bookmarksImported(int importedCount, int skippedCount);
    void bookmarksExported(int count, const QString& filePath);
    void errorOccurred(const QString& error);

private slots:
    void onDataChanged();

private:
    void initializeStorage();
    QString getStorageFilePath() const;
    int findBookmarkIndex(const QString& bookmarkId) const;
    void sortBookmarks();
    bool validateBookmark(const Bookmark& bookmark) const;
    
    QList<Bookmark> m_bookmarks;
    bool m_autoSave;
    QString m_storageFile;
};
