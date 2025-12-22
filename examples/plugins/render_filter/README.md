# Render Filter Plugin

This plugin demonstrates the `IRenderPlugin` interface by providing page rendering filters and overlays.

## Features

- **Night Mode Filter**: Inverts colors and reduces blue light for comfortable dark reading
- **Sepia Filter**: Warm, vintage color adjustment for reduced eye strain
- **Grayscale Filter**: Convert pages to black and white
- **Brightness/Contrast**: Basic image adjustments
- **Watermark Overlay**: Customizable repeating text watermark
- **Thread-Safe**: Filters can be applied in parallel for performance
- **Hook Integration**: Hooks into render workflow for pre/post processing

## IRenderPlugin Interface

This plugin implements the `IRenderPlugin` interface:

```cpp
class IRenderPlugin {
    RenderFilterType filterType() const;
    bool shouldProcessPage(const QString& documentPath, int pageNumber) const;
    bool applyFilter(QImage& image, int pageNumber, const QJsonObject& options);
    void renderOverlay(QPainter* painter, const QRect& rect, int pageNumber,
                       const QJsonObject& options);
    int filterPriority() const;
    bool isThreadSafe() const;
};
```

## Filter Types

| Filter | Description | RenderFilterType |
|--------|-------------|------------------|
| Night Mode | Inverts colors, reduces blue | ColorAdjustment |
| Sepia | Warm vintage tones | ColorAdjustment |
| Grayscale | Black and white | ColorAdjustment |
| Watermark | Text overlay | Overlay |

## Configuration

```json
{
    "activeFilter": "none",
    "brightness": 0,
    "contrast": 0,
    "enableWatermark": false,
    "watermarkText": "SAMPLE",
    "watermarkColor": "#808080",
    "watermarkOpacity": 30,
    "watermarkSize": 48
}
```

### Configuration Options

- `activeFilter`: Active color filter ("none", "night", "sepia", "grayscale")
- `brightness`: Brightness adjustment (-100 to +100)
- `contrast`: Contrast adjustment (-100 to +100)
- `enableWatermark`: Enable watermark overlay
- `watermarkText`: Watermark text content
- `watermarkColor`: Watermark color (hex)
- `watermarkOpacity`: Watermark opacity (0-100)
- `watermarkSize`: Watermark font size in points

## Hook Registration

The plugin registers callbacks for:

- `render.pre_page`: Called before page rendering
- `render.post_page`: Called after page rendering
- `render.apply_filter`: Called when filter is applied

## Inter-plugin Communication

The plugin responds to messages:

- `set_filter`: Set active filter (`{ "action": "set_filter", "filter": "night" }`)
- `set_watermark`: Configure watermark
- `get_status`: Get current filter status

## Thread Safety

The filter implementations are stateless and marked as thread-safe (`isThreadSafe() = true`), allowing the rendering system to apply filters in parallel across multiple pages for improved performance.

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Usage Example

```cpp
// Get render plugins from PluginManager
auto renderPlugins = PluginManager::instance().getRenderPlugins();

for (auto* plugin : renderPlugins) {
    if (plugin->shouldProcessPage(docPath, pageNum)) {
        plugin->applyFilter(pageImage, pageNum, options);
        plugin->renderOverlay(&painter, pageRect, pageNum, options);
    }
}
```
