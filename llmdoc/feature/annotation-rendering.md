# Annotation Rendering

## 1. Purpose

AnnotationRenderDelegate handles visual rendering of PDF annotations on pages. Supports all 11 annotation types (highlights, notes, text, shapes, freehand) with type-specific rendering logic, opacity control, selection highlighting, and resize handle display. Integrates with AnnotationController to fetch annotation data and respects zoom factors for accurate positioning.

## 2. How It Works

### Rendering Pipeline

**Primary Rendering Flow**

```
renderAnnotations(QPainter, pageNumber, pageRect, zoomFactor)
├─ Query controller for all annotations on page
├─ For each visible annotation:
│  └─ renderAnnotation() with type-specific handler
└─ Emit renderingCompleted(pageNumber, count)
```

**Rendering Process**

1. Save painter state (QPainter::save())
2. Set painter opacity from annotation
3. Dispatch to type-specific renderer
4. Apply selection highlighting if selected
5. Restore painter state (QPainter::restore())

### Type-Specific Renderers

Each annotation type has dedicated rendering method:

**Text Markup Annotations**

- `renderHighlight()` - Semi-transparent colored rectangle
- `renderNote()` - Icon with colored background, text in tooltip
- `renderFreeText()` - Rendered text with border and background
- `renderUnderline()` - Line under text with offset
- `renderStrikeOut()` - Line through text center
- `renderSquiggly()` - Wavy line under text

Text markup annotations use stored bounding rectangles and color/opacity from annotation data.

**Geometric Shapes**

- `renderRectangle()` - Outlined/filled rectangle
- `renderCircle()` - Circle/ellipse shape
- `renderLine()` - Line segment between startPoint and endPoint
- `renderArrow()` - Line with arrowhead at endpoint

Shapes use annotation boundingRect and custom startPoint/endPoint for lines/arrows.

**Freehand Drawing**

- `renderInk()` - Polyline drawn from inkPath QList<QPointF>

Ink annotations replay stored path with line width and color.

### Coordinate System Management

**Zoom Factor Handling**

Helper methods apply zoom transformation:

- `scaleRect(rect, zoom)` - Applies zoom to bounding rectangle
- `scalePoint(point, zoom)` - Applies zoom to single point

Ensures annotations scale correctly when viewer zooms in/out.

**Page Coordinate System**

Annotation coordinates stored in page-relative coordinates (0.0-1.0 range or document coordinates). Rendering applies pageRect bounds for accurate viewport positioning.

### Styling System

**Pen and Brush Creation**

- `createPen()- Creates QPen from annotation color and lineWidth
- `createBrush()` - Creates QBrush from annotation color

Pen includes:

- Color from annotation.color
- Width: `annotation.lineWidth * zoom`
- Cap style: FlatCap for clean edges
- Join style: BevelJoin for shapes

**Opacity Handling**

- Annotation opacity applied directly to QPainter (0.0-1.0)
- Color opacity adjusted via `adjustColorOpacity()`
- Method converts QColor to ARGB with opacity channel

### Selection Rendering

**Selection Management**

- `setSelectedAnnotationId(QString)` - Mark annotation as selected
- `clearSelection()` - Deselect current annotation
- `m_selectedAnnotationId` - Stores currently selected annotation

**Selection Visual Feedback**

When annotation selected:

1. `renderSelectionBorder()` - Draws border around annotation bounds
   - Color: typically bright (e.g., blue)
   - Width: `2.0 * zoom` pixels
   - Dashed line pattern for visibility

2. `renderResizeHandles()` - Draws corner and edge handles if enabled
   - 8 handles: 4 corners + 4 edge midpoints
   - Square size: `8.0 x 8.0` pixels (adjusted for zoom)
   - Filled with selection color

**Selection Options**

- `setShowSelectionHandles(bool)` - Toggle resize handle visibility
- `setHighlightSelected(bool)` - Toggle selection border highlight

### Signal Emission

**Rendering Completion Signals**

- `renderingCompleted(int pageNumber, int annotationCount)` - Emitted after page rendering completes
- `annotationHovered(QString annotationId)` - Emitted when hover detection determines annotation under cursor

Signals enable UI updates, status bar information, and hover effects.

### Integration Points

**AnnotationController Integration**

Delegate maintains weak reference to controller:

```cpp
AnnotationController* m_controller;  // Set via setController()
```

Uses controller to fetch annotation data:

```cpp
auto annotations = m_controller->getAnnotationsForPage(pageNumber);
```

**Rendering Context**

Called from PDF page rendering pipeline with parameters:

- `QPainter* painter` - Active graphics context
- `int pageNumber` - Page being rendered (0-based)
- `QRectF pageRect` - Page bounds in viewport coordinates
- `double zoomFactor` - Current zoom level (1.0 = 100%)

### Error Handling

Graceful handling of rendering failures:

- Null painter check: silently skips rendering
- Null controller check: renders no annotations
- Unknown annotation type: logs warning, skips annotation
- Invalid coordinates: clamped to page bounds

Invalid annotation handling via try-catch in tight rendering loops prevents UI freeze.

## 3. Relevant Code Modules

- `/app/delegate/AnnotationRenderDelegate.h` - Header with renderer interface
- `/app/delegate/AnnotationRenderDelegate.cpp` - Implementation (400+ lines)
- `/app/controller/AnnotationController.h` - Annotation data provider
- `/app/model/AnnotationModel.h` - PDFAnnotation data structure
- `/app/logging/SimpleLogging.h` - Logging for warnings and errors
- Qt Core: `QPainter`, `QPen`, `QBrush`, `QFont`
- Qt GUI: `QPainterPath` for arrow and wavy line rendering

## 4. Attention

- Null pointer checks on painter and controller are critical; missing checks cause crashes
- Zoom factor must be > 0.0; values <= 0 cause visual corruption
- Selection border rendering should use different color than annotation fill for visibility
- Render performance scales linearly with annotation count per page; consider batching for 100+ annotations
- Opacity blending assumes 32-bit ARGB color format; verify on platform-specific renderers
