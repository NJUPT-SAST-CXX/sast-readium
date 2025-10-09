# QGraphics PDF Support

This document describes the conditional QGraphics PDF rendering support in SAST Readium.

## Overview

SAST Readium now supports enhanced PDF rendering through Qt's QGraphics framework as an optional feature. This provides improved performance, smoother interactions, and advanced viewing capabilities while maintaining backward compatibility with the traditional QLabel-based rendering.

## Build Configuration

### Enabling QGraphics Support

#### Using xmake
```bash
xmake config --enable_qgraphics_pdf=true
xmake build
```

#### Using CMake
```bash
cmake -DENABLE_QGRAPHICS_PDF_SUPPORT=ON ..
make
```

### Disabling QGraphics Support (Default)

#### Using xmake
```bash
xmake config --enable_qgraphics_pdf=false
xmake build
```

#### Using CMake
```bash
cmake -DENABLE_QGRAPHICS_PDF_SUPPORT=OFF ..
make
```

## Features

### Traditional Rendering (Always Available)
- QLabel-based PDF page display
- Basic zoom, pan, and rotation
- Search highlighting
- Page caching
- Continuous and single page modes

### QGraphics Enhanced Rendering (Optional)
- QGraphicsView-based PDF display
- Smooth zooming and panning
- Hardware-accelerated rendering
- Advanced view modes:
  - Single Page
  - Continuous Page
  - Facing Pages
  - Continuous Facing
- Enhanced interaction:
  - Rubber band selection for zoom
  - Middle-click panning
  - Smooth scrolling
- Better performance for large documents
- Configurable page spacing and margins

## Usage

### Basic Usage

```cpp
#include "ui/viewer/PDFViewer.h"

PDFViewer* viewer = new PDFViewer(parent);

// Load a document
Poppler::Document* document = Poppler::Document::load("document.pdf");
viewer->setDocument(document);

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
// Enable QGraphics rendering (if compiled with support)
viewer->setQGraphicsRenderingEnabled(true);

// Configure QGraphics-specific features
viewer->setQGraphicsHighQualityRendering(true);
viewer->setQGraphicsViewMode(0); // Single page mode
#endif
```

### Enhanced Usage with Demo Controls

```cpp
#include "ui/viewer/PDFRenderingDemo.h"

EnhancedPDFViewer* enhancedViewer = new EnhancedPDFViewer(parent);

// Load document
enhancedViewer->setDocument(document);

// Enable QGraphics rendering
enhancedViewer->setQGraphicsRenderingEnabled(true);

// The demo controls provide a UI for switching between modes
```

### Runtime Mode Switching

```cpp
// Check if QGraphics support is available
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
bool qgraphicsAvailable = true;
#else
bool qgraphicsAvailable = false;
#endif

if (qgraphicsAvailable) {
    // Switch to QGraphics mode
    viewer->setQGraphicsRenderingEnabled(true);

    // Configure enhanced features
    viewer->setQGraphicsHighQualityRendering(true);
    viewer->setQGraphicsViewMode(1); // Continuous mode
} else {
    // Use traditional rendering
    viewer->setQGraphicsRenderingEnabled(false);
}
```

## API Reference

### PDFViewer Class Extensions

When `ENABLE_QGRAPHICS_PDF_SUPPORT` is defined:

```cpp
// Enable/disable QGraphics rendering
void setQGraphicsRenderingEnabled(bool enabled);
bool isQGraphicsRenderingEnabled() const;

// Configure QGraphics-specific features
void setQGraphicsHighQualityRendering(bool enabled);
void setQGraphicsViewMode(int mode);
```

### QGraphicsPDFViewer Class

Direct QGraphics PDF viewer (available when macro is enabled):

```cpp
// Document management
void setDocument(Poppler::Document* document);
void clearDocument();

// Navigation
void goToPage(int pageNumber);
void nextPage();
void previousPage();

// Zoom operations
void zoomIn();
void zoomOut();
void zoomToFit();
void setZoom(double factor);

// View modes
enum ViewMode {
    SinglePage,
    ContinuousPage,
    FacingPages,
    ContinuousFacing
};
void setViewMode(ViewMode mode);

// Enhanced features
void setHighQualityRendering(bool enabled);
void setPageSpacing(int spacing);
void setPageMargin(int margin);
```

## Performance Considerations

### When to Use QGraphics Rendering
- Large PDF documents (>50 pages)
- Documents requiring frequent zoom/pan operations
- When smooth animations are important
- On systems with good graphics hardware

### When to Use Traditional Rendering
- Small PDF documents (<10 pages)
- Memory-constrained environments
- Older hardware without graphics acceleration
- When minimal dependencies are preferred

## Troubleshooting

### Compilation Issues

1. **QGraphics headers not found**: Ensure Qt6 is properly installed with all components
2. **Macro not defined**: Check that the build option is correctly set
3. **Linking errors**: Verify that Qt6::Widgets is linked

### Runtime Issues

1. **QGraphics mode not working**: Check if support was compiled in using `isQGraphicsRenderingEnabled()`
2. **Performance issues**: Try disabling high-quality rendering or reducing page spacing
3. **Memory usage**: Monitor memory consumption and adjust cache settings

## Examples

See the `PDFRenderingDemo` class for a complete example of how to integrate and control both rendering modes.

## Future Enhancements

Planned improvements for QGraphics PDF support:
- Annotation rendering in QGraphics mode
- Advanced selection tools
- Multi-document tabs with QGraphics
- Custom page layouts
- Performance profiling tools
