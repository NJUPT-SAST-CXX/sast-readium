# Bookmark Manager Plugin

This plugin demonstrates bookmark management with `IUIExtension` and persistence.

## Features

- **Bookmark CRUD**: Create, read, update, delete bookmarks
- **Categories**: Organize bookmarks by category
- **Navigation**: Quick jump to bookmarked pages
- **Persistence**: Save/load bookmarks to JSON
- **Sync Simulation**: Mock cloud synchronization
- **Dock Widget**: Bookmark panel UI
- **Context Menu**: Right-click to bookmark page

## Bookmark Structure

```cpp
struct Bookmark {
    QString id;
    QString documentPath;
    int pageNumber;
    QString title;
    QString description;
    QString category;
    QColor color;
    QDateTime createdAt;
    QDateTime modifiedAt;
};
```

## Inter-plugin Communication

### Add Bookmark

```cpp
QVariantMap msg;
msg["action"] = "add";
msg["documentPath"] = "/path/to/doc.pdf";
msg["pageNumber"] = 5;
msg["title"] = "Important Section";
msg["category"] = "Research";
pluginManager->sendMessage("Bookmark Manager", msg);
// Response: { "success": true, "bookmarkId": "uuid" }
```

### Delete Bookmark

```cpp
QVariantMap msg;
msg["action"] = "delete";
msg["bookmarkId"] = "uuid";
pluginManager->sendMessage("Bookmark Manager", msg);
```

### Get Bookmarks

```cpp
QVariantMap msg;
msg["action"] = "get_bookmarks";
msg["documentPath"] = "/path/to/doc.pdf";  // Optional
pluginManager->sendMessage("Bookmark Manager", msg);
// Response: { "bookmarks": [...] }
```

### Navigate to Bookmark

```cpp
QVariantMap msg;
msg["action"] = "navigate";
msg["bookmarkId"] = "uuid";
pluginManager->sendMessage("Bookmark Manager", msg);
```

## Events Published

### bookmark.created

```cpp
{
    "bookmarkId": "uuid",
    "pageNumber": 5,
    "title": "Important Section"
}
```

### bookmark.deleted

```cpp
{
    "bookmarkId": "uuid"
}
```

## Storage Format

```json
{
    "bookmarks": [
        {
            "id": "uuid",
            "documentPath": "/path/to/doc.pdf",
            "pageNumber": 5,
            "title": "Important Section",
            "category": "Research",
            "color": "#ffff00",
            "createdAt": "2024-01-01T00:00:00",
            "modifiedAt": "2024-01-01T00:00:00"
        }
    ],
    "savedAt": "2024-01-01T23:59:59"
}
```

## Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| Add Bookmark | Ctrl+D |
| Show Bookmarks | Ctrl+Shift+B |

## Configuration

```json
{
    "autoSync": false,
    "defaultCategory": "Default",
    "maxBookmarksPerDocument": 100
}
```

## Building

```bash
mkdir build && cd build
cmake .. && cmake --build .
```
