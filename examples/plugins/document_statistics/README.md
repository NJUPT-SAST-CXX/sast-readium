# Document Statistics Plugin

This plugin demonstrates document analysis with `IDocumentProcessorPlugin` and `IUIExtension`.

## Features

- **Text Analysis**: Word count, character count
- **Reading Time**: Estimated reading time based on configurable WPM
- **Structure Analysis**: Page count, images, links, annotations
- **Per-page Statistics**: Word distribution across pages
- **Dock Widget**: Statistics panel
- **Export**: Statistics to JSON or CSV

## Statistics Collected

| Metric | Description |
|--------|-------------|
| Page Count | Total pages in document |
| Word Count | Total words extracted |
| Character Count | Total characters |
| Image Count | Embedded images |
| Link Count | Hyperlinks |
| Annotation Count | PDF annotations |
| Reading Time | Estimated minutes to read |
| Words Per Page | Distribution by page |

## Inter-plugin Communication

### Get Statistics

```cpp
QVariantMap msg;
msg["action"] = "get_statistics";
msg["documentPath"] = "/path/to/doc.pdf";  // Optional, uses current if empty
pluginManager->sendMessage("Document Statistics", msg);
// Response: { pageCount, wordCount, characterCount, readingTime, ... }
```

### Trigger Analysis

```cpp
QVariantMap msg;
msg["action"] = "analyze";
msg["documentPath"] = "/path/to/doc.pdf";
pluginManager->sendMessage("Document Statistics", msg);
```

## Export Formats

### JSON

```json
{
    "pageCount": 10,
    "wordCount": 5000,
    "characterCount": 25000,
    "imageCount": 3,
    "linkCount": 15,
    "readingTimeMinutes": 25,
    "wordsPerPage": {
        "1": 520,
        "2": 480,
        ...
    }
}
```

### CSV

```csv
Metric,Value
Pages,10
Words,5000
Characters,25000
Images,3
Reading Time (min),25
```

## Status Bar Integration

When enabled, shows: `Words: 5000 | Pages: 10 | ~25 min read`

## Configuration

```json
{
    "wordsPerMinute": 200,
    "autoAnalyze": true,
    "showInStatusBar": true
}
```

## Building

```bash
mkdir build && cd build
cmake .. && cmake --build .
```
