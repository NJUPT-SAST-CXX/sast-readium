# Page Navigator Plugin

This plugin demonstrates advanced page navigation with history tracking.

## Features

- **Navigation History**: Back/forward navigation with history stack
- **Quick Jump**: Go to page by number or percentage
- **First/Last Page**: Quick navigation to document boundaries
- **Page Slider**: Visual page navigation in dock widget
- **Status Bar**: Current page and percentage display

## Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| Go Back | Alt+Left |
| Go Forward | Alt+Right |
| Go to Page | Ctrl+G |
| First Page | Home |
| Last Page | End |

## NavigationEntry Structure

```cpp
struct NavigationEntry {
    QString documentPath;
    int pageNumber;
    double scrollPosition;
    double zoomLevel;
    QDateTime timestamp;
};
```

## Inter-plugin Communication

### Go to Page

```cpp
QVariantMap msg;
msg["action"] = "go_to_page";
msg["pageNumber"] = 10;
pluginManager->sendMessage("Page Navigator", msg);
```

### Go to Percentage

```cpp
QVariantMap msg;
msg["action"] = "go_to_percentage";
msg["percentage"] = 50.0;  // Go to middle of document
pluginManager->sendMessage("Page Navigator", msg);
```

### Navigation History

```cpp
QVariantMap msg;
msg["action"] = "go_back";  // or "go_forward"
pluginManager->sendMessage("Page Navigator", msg);
```

### Get History Status

```cpp
QVariantMap msg;
msg["action"] = "get_history";
pluginManager->sendMessage("Page Navigator", msg);
// Response: { canGoBack, canGoForward, historySize, currentPage, totalPages }
```

## Events Published

### navigation.goToPage

```cpp
{
    "pageNumber": 10,
    "fromHistory": false  // true if from back/forward
}
```

## Configuration

```json
{
    "maxHistorySize": 50,
    "showMinimap": true,
    "showPageSlider": true
}
```

## Building

```bash
mkdir build && cd build
cmake .. && cmake --build .
```
