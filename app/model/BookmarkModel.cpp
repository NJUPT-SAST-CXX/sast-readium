#include "BookmarkModel.h"
#include <QDebug>
#include <QFileInfo>
#include <QJsonParseError>
#include <algorithm>

// Bookmark serialization implementation
QJsonObject Bookmark::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["title"] = title;
    obj["documentPath"] = documentPath;
    obj["pageNumber"] = pageNumber;
    obj["createdTime"] = createdTime.toString(Qt::ISODate);
    obj["lastAccessed"] = lastAccessed.toString(Qt::ISODate);
    obj["notes"] = notes;
    obj["category"] = category;

    if (!highlightRect.isNull()) {
        QJsonObject rectObj;
        rectObj["x"] = highlightRect.x();
        rectObj["y"] = highlightRect.y();
        rectObj["width"] = highlightRect.width();
        rectObj["height"] = highlightRect.height();
        obj["highlightRect"] = rectObj;
    }

    return obj;
}

Bookmark Bookmark::fromJson(const QJsonObject& json) {
    Bookmark bookmark;
    bookmark.id = json["id"].toString();
    bookmark.title = json["title"].toString();
    bookmark.documentPath = json["documentPath"].toString();
    bookmark.pageNumber = json["pageNumber"].toInt();
    bookmark.createdTime =
        QDateTime::fromString(json["createdTime"].toString(), Qt::ISODate);
    bookmark.lastAccessed =
        QDateTime::fromString(json["lastAccessed"].toString(), Qt::ISODate);
    bookmark.notes = json["notes"].toString();
    bookmark.category = json["category"].toString();

    if (json.contains("highlightRect")) {
        QJsonObject rectObj = json["highlightRect"].toObject();
        bookmark.highlightRect =
            QRectF(rectObj["x"].toDouble(), rectObj["y"].toDouble(),
                   rectObj["width"].toDouble(), rectObj["height"].toDouble());
    }

    return bookmark;
}

BookmarkModel::BookmarkModel(QObject* parent)
    : QAbstractItemModel(parent), m_autoSave(true) {
    initializeStorage();
    loadFromFile();

    // Connect to auto-save on data changes
    connect(this, &BookmarkModel::dataChanged, this,
            &BookmarkModel::onDataChanged);
    connect(this, &BookmarkModel::rowsInserted, this,
            &BookmarkModel::onDataChanged);
    connect(this, &BookmarkModel::rowsRemoved, this,
            &BookmarkModel::onDataChanged);
}

QModelIndex BookmarkModel::index(int row, int column,
                                 const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent) || parent.isValid()) {
        return QModelIndex();
    }

    return createIndex(row, column, nullptr);
}

QModelIndex BookmarkModel::parent(const QModelIndex& child) const {
    Q_UNUSED(child)
    return QModelIndex();  // Flat list model
}

int BookmarkModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return m_bookmarks.size();
}

int BookmarkModel::columnCount(const QModelIndex& parent) const {
    Q_UNUSED(parent)
    return 4;  // Title, Document, Page, Created
}

QVariant BookmarkModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_bookmarks.size()) {
        return QVariant();
    }

    const Bookmark& bookmark = m_bookmarks.at(index.row());

    switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
                case 0:
                    return bookmark.title;
                case 1:
                    return QFileInfo(bookmark.documentPath).baseName();
                case 2:
                    return bookmark.pageNumber +
                           1;  // Display 1-based page numbers
                case 3:
                    return bookmark.createdTime.toString("yyyy-MM-dd hh:mm");
                default:
                    return QVariant();
            }
        case Qt::ToolTipRole:
            return QString("Document: %1\nPage: %2\nCreated: %3\nNotes: %4")
                .arg(bookmark.documentPath)
                .arg(bookmark.pageNumber + 1)
                .arg(bookmark.createdTime.toString())
                .arg(bookmark.notes.isEmpty() ? "None" : bookmark.notes);
        case IdRole:
            return bookmark.id;
        case TitleRole:
            return bookmark.title;
        case DocumentPathRole:
            return bookmark.documentPath;
        case PageNumberRole:
            return bookmark.pageNumber;
        case CreatedTimeRole:
            return bookmark.createdTime;
        case LastAccessedRole:
            return bookmark.lastAccessed;
        case NotesRole:
            return bookmark.notes;
        case HighlightRectRole:
            return bookmark.highlightRect;
        case CategoryRole:
            return bookmark.category;
        default:
            return QVariant();
    }
}

QVariant BookmarkModel::headerData(int section, Qt::Orientation orientation,
                                   int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (section) {
        case 0:
            return "Title";
        case 1:
            return "Document";
        case 2:
            return "Page";
        case 3:
            return "Created";
        default:
            return QVariant();
    }
}

bool BookmarkModel::setData(const QModelIndex& index, const QVariant& value,
                            int role) {
    if (!index.isValid() || index.row() >= m_bookmarks.size()) {
        return false;
    }

    Bookmark& bookmark = m_bookmarks[index.row()];
    bool changed = false;

    switch (role) {
        case TitleRole:
            if (bookmark.title != value.toString()) {
                bookmark.title = value.toString();
                changed = true;
            }
            break;
        case NotesRole:
            if (bookmark.notes != value.toString()) {
                bookmark.notes = value.toString();
                changed = true;
            }
            break;
        case CategoryRole:
            if (bookmark.category != value.toString()) {
                bookmark.category = value.toString();
                changed = true;
            }
            break;
        default:
            return false;
    }

    if (changed) {
        emit dataChanged(index, index, {role});
        emit bookmarkUpdated(bookmark);
        return true;
    }

    return false;
}

Qt::ItemFlags BookmarkModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QHash<int, QByteArray> BookmarkModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "display";
    roles[IdRole] = "id";
    roles[TitleRole] = "title";
    roles[DocumentPathRole] = "documentPath";
    roles[PageNumberRole] = "pageNumber";
    roles[CreatedTimeRole] = "createdTime";
    roles[LastAccessedRole] = "lastAccessed";
    roles[NotesRole] = "notes";
    roles[HighlightRectRole] = "highlightRect";
    roles[CategoryRole] = "category";
    return roles;
}

bool BookmarkModel::addBookmark(const Bookmark& bookmark) {
    // Check for duplicates
    if (hasBookmarkForPage(bookmark.documentPath, bookmark.pageNumber)) {
        qDebug() << "Bookmark already exists for this page";
        return false;
    }

    beginInsertRows(QModelIndex(), m_bookmarks.size(), m_bookmarks.size());
    m_bookmarks.append(bookmark);
    endInsertRows();

    sortBookmarks();
    emit bookmarkAdded(bookmark);

    return true;
}

bool BookmarkModel::removeBookmark(const QString& bookmarkId) {
    int index = findBookmarkIndex(bookmarkId);
    if (index < 0) {
        return false;
    }

    beginRemoveRows(QModelIndex(), index, index);
    m_bookmarks.removeAt(index);
    endRemoveRows();

    emit bookmarkRemoved(bookmarkId);
    return true;
}

bool BookmarkModel::updateBookmark(const QString& bookmarkId,
                                   const Bookmark& updatedBookmark) {
    int index = findBookmarkIndex(bookmarkId);
    if (index < 0) {
        return false;
    }

    m_bookmarks[index] = updatedBookmark;
    QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, this->index(index, columnCount() - 1));
    emit bookmarkUpdated(updatedBookmark);

    return true;
}

Bookmark BookmarkModel::getBookmark(const QString& bookmarkId) const {
    int index = findBookmarkIndex(bookmarkId);
    if (index >= 0) {
        return m_bookmarks.at(index);
    }
    return Bookmark();
}

QList<Bookmark> BookmarkModel::getAllBookmarks() const { return m_bookmarks; }

QList<Bookmark> BookmarkModel::getBookmarksForDocument(
    const QString& documentPath) const {
    QList<Bookmark> result;
    for (const Bookmark& bookmark : m_bookmarks) {
        if (bookmark.documentPath == documentPath) {
            result.append(bookmark);
        }
    }
    return result;
}

bool BookmarkModel::hasBookmarkForPage(const QString& documentPath,
                                       int pageNumber) const {
    for (const Bookmark& bookmark : m_bookmarks) {
        if (bookmark.documentPath == documentPath &&
            bookmark.pageNumber == pageNumber) {
            return true;
        }
    }
    return false;
}

Bookmark BookmarkModel::getBookmarkForPage(const QString& documentPath,
                                           int pageNumber) const {
    for (const Bookmark& bookmark : m_bookmarks) {
        if (bookmark.documentPath == documentPath &&
            bookmark.pageNumber == pageNumber) {
            return bookmark;
        }
    }
    return Bookmark();
}

void BookmarkModel::initializeStorage() {
    m_storageFile = getStorageFilePath();

    // Ensure directory exists
    QFileInfo fileInfo(m_storageFile);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

QString BookmarkModel::getStorageFilePath() const {
    QString dataPath =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(dataPath).filePath("bookmarks.json");
}

int BookmarkModel::findBookmarkIndex(const QString& bookmarkId) const {
    for (int i = 0; i < m_bookmarks.size(); ++i) {
        if (m_bookmarks.at(i).id == bookmarkId) {
            return i;
        }
    }
    return -1;
}

void BookmarkModel::sortBookmarks() {
    std::sort(m_bookmarks.begin(), m_bookmarks.end(),
              [](const Bookmark& a, const Bookmark& b) {
                  return a.lastAccessed > b.lastAccessed;  // Most recent first
              });
}

void BookmarkModel::onDataChanged() {
    if (m_autoSave) {
        saveToFile();
    }
}

QStringList BookmarkModel::getCategories() const {
    QStringList categories;
    for (const Bookmark& bookmark : m_bookmarks) {
        if (!bookmark.category.isEmpty() &&
            !categories.contains(bookmark.category)) {
            categories.append(bookmark.category);
        }
    }
    categories.sort();
    return categories;
}

QList<Bookmark> BookmarkModel::getBookmarksInCategory(
    const QString& category) const {
    QList<Bookmark> result;
    for (const Bookmark& bookmark : m_bookmarks) {
        if (bookmark.category == category) {
            result.append(bookmark);
        }
    }
    return result;
}

bool BookmarkModel::moveBookmarkToCategory(const QString& bookmarkId,
                                           const QString& category) {
    int index = findBookmarkIndex(bookmarkId);
    if (index < 0) {
        return false;
    }

    m_bookmarks[index].category = category;
    QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, {CategoryRole});
    emit bookmarkUpdated(m_bookmarks[index]);

    return true;
}

QList<Bookmark> BookmarkModel::searchBookmarks(const QString& query) const {
    QList<Bookmark> result;
    QString lowerQuery = query.toLower();

    for (const Bookmark& bookmark : m_bookmarks) {
        if (bookmark.title.toLower().contains(lowerQuery) ||
            bookmark.notes.toLower().contains(lowerQuery) ||
            bookmark.documentPath.toLower().contains(lowerQuery) ||
            bookmark.category.toLower().contains(lowerQuery)) {
            result.append(bookmark);
        }
    }

    return result;
}

QList<Bookmark> BookmarkModel::getRecentBookmarks(int count) const {
    QList<Bookmark> sorted = m_bookmarks;
    std::sort(sorted.begin(), sorted.end(),
              [](const Bookmark& a, const Bookmark& b) {
                  return a.lastAccessed > b.lastAccessed;
              });

    if (count > 0 && sorted.size() > count) {
        return sorted.mid(0, count);
    }

    return sorted;
}

bool BookmarkModel::saveToFile() {
    QJsonArray bookmarksArray;
    for (const Bookmark& bookmark : m_bookmarks) {
        bookmarksArray.append(bookmark.toJson());
    }

    QJsonObject rootObject;
    rootObject["version"] = "1.0";
    rootObject["bookmarks"] = bookmarksArray;
    rootObject["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonDocument doc(rootObject);

    QFile file(m_storageFile);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open bookmarks file for writing:"
                   << m_storageFile;
        return false;
    }

    qint64 bytesWritten = file.write(doc.toJson());
    file.close();

    if (bytesWritten > 0) {
        emit bookmarksSaved(m_bookmarks.size());
        qDebug() << "Saved" << m_bookmarks.size() << "bookmarks to"
                 << m_storageFile;
        return true;
    }

    return false;
}

bool BookmarkModel::loadFromFile() {
    QFile file(m_storageFile);
    if (!file.exists()) {
        qDebug() << "Bookmarks file does not exist, starting with empty list";
        return true;  // Not an error for first run
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open bookmarks file for reading:"
                   << m_storageFile;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse bookmarks JSON:"
                   << parseError.errorString();
        return false;
    }

    QJsonObject rootObject = doc.object();
    QJsonArray bookmarksArray = rootObject["bookmarks"].toArray();

    beginResetModel();
    m_bookmarks.clear();

    for (const QJsonValue& value : bookmarksArray) {
        if (value.isObject()) {
            Bookmark bookmark = Bookmark::fromJson(value.toObject());
            if (!bookmark.id.isEmpty()) {
                m_bookmarks.append(bookmark);
            }
        }
    }

    sortBookmarks();
    endResetModel();

    emit bookmarksLoaded(m_bookmarks.size());
    qDebug() << "Loaded" << m_bookmarks.size() << "bookmarks from"
             << m_storageFile;

    return true;
}
