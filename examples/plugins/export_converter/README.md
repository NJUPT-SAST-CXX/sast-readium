# Export Converter Plugin

This plugin demonstrates multi-format document export with `IDocumentProcessorPlugin`.

## Features

- **Plain Text Export**: Extract text content to .txt files
- **HTML Export**: Formatted HTML with responsive styling
- **Markdown Export**: Markdown formatted output
- **Template Support**: Customizable HTML templates
- **Menu Integration**: Export options in File menu

## Supported Export Formats

| Format | Extension | Description |
|--------|-----------|-------------|
| Text | .txt | Plain text extraction |
| HTML | .html | Styled HTML document |
| Markdown | .md | Markdown formatted |

## Inter-plugin Communication

### Export Document

```cpp
QVariantMap msg;
msg["action"] = "export";
msg["sourcePath"] = "/path/to/doc.pdf";
msg["targetPath"] = "/path/to/output.html";
msg["format"] = "html";
msg["options"] = QVariantMap{{"title", "My Document"}};
pluginManager->sendMessage("Export Converter", msg);
```

### Get Supported Formats

```cpp
QVariantMap msg;
msg["action"] = "get_formats";
pluginManager->sendMessage("Export Converter", msg);
// Response: { "formats": ["txt", "html", "markdown"] }
```

## HTML Output Example

```html
<!DOCTYPE html>
<html>
<head>
    <title>Document Title</title>
    <style>/* Responsive styling */</style>
</head>
<body>
    <h1>Document Title</h1>
    <div class="content">...</div>
</body>
</html>
```

## Hook Registration

- `export.pre_execute`: Called before export, can validate format
- `export.post_execute`: Called after export completes

## Configuration

```json
{
    "defaultFormat": "txt",
    "includeMetadata": true,
    "htmlTemplate": "default"
}
```

## Building

```bash
mkdir build && cd build
cmake .. && cmake --build .
```
