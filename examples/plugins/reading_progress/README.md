# Reading Progress Plugin

This plugin demonstrates reading progress tracking with session management.

## Features

- **Progress Tracking**: Track pages read and completion percentage
- **Reading Sessions**: Record reading time per session
- **Statistics**: Reading speed, total time, estimated completion
- **History**: Recently read documents with progress
- **Persistence**: Save/load progress data to JSON

## Data Structures

### DocumentProgress

```cpp
struct DocumentProgress {
    QString documentPath;
    QString documentTitle;
    int totalPages;
    int lastReadPage;
    double percentComplete;
    qint64 totalReadingTime;
    QDateTime lastAccessed;
    QDateTime firstAccessed;
    QList<ReadingSession> sessions;
};
```

### ReadingSession

```cpp
struct ReadingSession {
    QString documentPath;
    QDateTime startTime;
    QDateTime endTime;
    int startPage;
    int endPage;
    int pagesRead;
    qint64 durationSeconds;
};
```

## Inter-plugin Communication

### Get Progress

```cpp
QVariantMap msg;
msg["action"] = "get_progress";
msg["documentPath"] = "/path/to/doc.pdf";
pluginManager->sendMessage("Reading Progress", msg);
// Response: { totalPages, lastReadPage, percentComplete, totalReadingTime }
```

### Get Recent Documents

```cpp
QVariantMap msg;
msg["action"] = "get_recent";
msg["limit"] = 10;
pluginManager->sendMessage("Reading Progress", msg);
// Response: { documents: [...] }
```

### Get Statistics

```cpp
QVariantMap msg;
msg["action"] = "get_statistics";
pluginManager->sendMessage("Reading Progress", msg);
// Response: { readingSpeed, totalReadingTime, totalPagesRead, documentsTracked }
```

### Reset Progress

```cpp
QVariantMap msg;
msg["action"] = "reset_progress";
msg["documentPath"] = "/path/to/doc.pdf";
pluginManager->sendMessage("Reading Progress", msg);
```

## Events Published

### progress.updated

```cpp
{
    "documentPath": "/path/to/doc.pdf",
    "pageNumber": 42,
    "percentComplete": 35.5
}
```

## Statistics

| Metric | Description |
|--------|-------------|
| Reading Speed | Pages per minute average |
| Total Reading Time | Cumulative reading time |
| Pages Read | Total pages viewed |
| Estimated Completion | Minutes to finish current document |

## Configuration

```json
{
    "sessionTimeoutMinutes": 30,
    "trackReadingSpeed": true,
    "maxHistoryItems": 100
}
```

## Storage Format

```json
{
    "documents": [
        {
            "documentPath": "/path/to/doc.pdf",
            "totalPages": 100,
            "lastReadPage": 42,
            "percentComplete": 42.0,
            "totalReadingTime": 3600,
            "lastAccessed": "2024-01-01T12:00:00"
        }
    ],
    "totalReadingTime": 36000,
    "totalPagesRead": 500,
    "averageReadingSpeed": 0.5
}
```

## Building

```bash
mkdir build && cd build
cmake .. && cmake --build .
```
