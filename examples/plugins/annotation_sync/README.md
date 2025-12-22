# Annotation Sync Plugin

This plugin demonstrates the `IAnnotationPlugin` interface with import/export and cloud sync.

## Features

- **Multi-format Export**: JSON and XFDF annotation export
- **Import Support**: Load annotations from JSON files
- **Cloud Sync Simulation**: Mock synchronization to cloud endpoint
- **Custom Rendering**: Highlight, underline, strikethrough, and note rendering

## IAnnotationPlugin Interface

```cpp
class IAnnotationPlugin {
    QList<AnnotationType> supportedTypes() const;
    bool createAnnotation(const AnnotationData& data, const QString& documentPath);
    bool updateAnnotation(const QString& annotationId, const AnnotationData& data,
                          const QString& documentPath);
    bool deleteAnnotation(const QString& annotationId, const QString& documentPath);
    QList<AnnotationData> getAnnotationsForPage(int pageNumber,
                                                const QString& documentPath) const;
    bool exportAnnotations(const QString& documentPath, const QString& outputPath,
                           const QString& format);
    int importAnnotations(const QString& inputPath, const QString& documentPath,
                          const QString& format);
    void renderAnnotation(QPainter* painter, const AnnotationData& annotation,
                          const QRect& pageRect, double zoom);
};
```

## Supported Annotation Types

| Type | Rendering |
|------|-----------|
| Highlight | Translucent color fill |
| Underline | Line at bottom edge |
| Strikethrough | Line through center |
| Note | Filled rect with border |
| FreeText | Text overlay |

## Export Formats

### JSON

```json
{
    "version": "1.0",
    "exportedBy": "Annotation Sync",
    "annotations": [
        {
            "id": "uuid",
            "type": 0,
            "pageNumber": 1,
            "content": "Important text",
            "color": "#ffff00",
            "boundingRect": {"x": 100, "y": 200, "width": 300, "height": 50}
        }
    ]
}
```

### XFDF

Adobe-compatible XFDF format for interoperability with PDF readers.

## Configuration

```json
{
    "autoSync": false,
    "cloudEndpoint": "http://localhost:8080",
    "exportFormats": ["json", "xfdf"],
    "importFormats": ["json"]
}
```

## Hook Registration

- `annotation.created`: Called when annotation is created
- `annotation.updated`: Called when annotation is updated
- `annotation.render`: Called during annotation rendering

## Building

```bash
mkdir build && cd build
cmake .. && cmake --build .
```
