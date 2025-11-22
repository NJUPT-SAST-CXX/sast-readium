# Annotation Selection Manager

## 1. Purpose

The AnnotationSelectionManager handles user interaction with annotations including selection, moving, and resizing through mouse/touch events. Provides hit testing, resize handle detection, and state tracking for interactive annotation editing on the PDF viewer.

## 2. How It Works

### Selection System

**Annotation Selection**

```
selectAnnotation(annotationId)
├─ Updates m_selectedAnnotationId
├─ Notifies AnnotationRenderDelegate of selection
├─ Caches page number for quick access
└─ Emits selectionChanged(annotationId) signal
```

**Deselection**

```
clearSelection()
├─ Clears m_selectedAnnotationId
├─ Resets page number to -1
├─ Notifies AnnotationRenderDelegate to clear highlights
└─ Emits selectionCleared() signal
```

### Hit Testing

**Annotation Detection**

```
findAnnotationAt(point, pageNumber)
├─ Queries AnnotationController for page annotations
├─ Iterates in reverse (top-most annotation first)
├─ Applies hit tolerance (default 5.0 pixels)
└─ Returns annotation ID or empty string if no match
```

Hit tolerance allows clicking slightly outside annotation bounds for better UX.

**Resize Handle Detection**

```
findResizeHandle(point, zoom)
├─ Returns HandlePosition enum for 8 resize handles:
│  ├─ TopLeft, TopCenter, TopRight
│  ├─ CenterLeft, CenterRight
│  ├─ BottomLeft, BottomCenter, BottomRight
│  ├─ Inside (for moving)
│  └─ None (no handle hit)
├─ Scales handle size by zoom for accurate hit testing
└─ Uses hit tolerance for forgiving click targets
```

Handle positions correspond to standard resize directions (corners and edge midpoints).

### Mouse Event Handling

**Mouse Press (handleMousePress)**

```
handleMousePress(point, pageNumber, zoom)
├─ If selected annotation hit:
│  ├─ Check for resize handle hit
│  ├─ If Inside: enter move mode (m_isMoving = true)
│  ├─ If handle: enter resize mode (m_isResizing = true, store handle)
│  └─ Store original position/boundary for delta calculation
├─ Else if point hits annotation:
│  └─ Select that annotation
└─ Else: clear selection
```

**Mouse Move (handleMouseMove)**

```
handleMouseMove(point, zoom)
├─ If in move mode:
│  └─ Calculate delta from interaction start point
│     (doesn't apply changes yet - waits for release)
├─ If in resize mode:
│  ├─ Calculate new boundary using calculateNewBoundary()
│  └─ Enforce minimum size (10x10 pixels)
└─ Return true if event handled
```

Move and resize operations track changes without applying them until mouse release (deferred commit pattern).

**Mouse Release (handleMouseRelease)**

```
handleMouseRelease(point, zoom)
├─ If moved with meaningful delta:
│  ├─ Call controller.moveAnnotation()
│  └─ Emit annotationMoved(id, newPosition)
├─ If resized with valid boundary:
│  ├─ Call controller.resizeAnnotation()
│  └─ Emit annotationResized(id, newBoundary)
└─ Reset interaction state (m_isInteracting, etc.)
```

AnnotationController automatically executes commands for undo/redo support.

### State Management

**Interaction States**

- `m_isInteracting` - Currently in mouse interaction
- `m_isMoving` - Moving selected annotation
- `m_isResizing` - Resizing selected annotation
- `m_currentHandle` - Active resize handle position

**Data Tracking**

- `m_selectedAnnotationId` - ID of selected annotation
- `m_selectedPageNumber` - Page number of selection
- `m_interactionStartPoint` - Initial click point
- `m_originalPosition` - Annotation position before move
- `m_originalBoundary` - Annotation boundary before resize

### Configuration

**Resize Handle Size**

```cpp
setHandleSize(double size);  // Default: 8.0 pixels
```

Controls visual size of resize handle squares.

**Hit Tolerance**

```cpp
setHitTolerance(double tolerance);  // Default: 5.0 pixels
```

Expands clickable area around annotations and handles for better UX.

### Signal System

**Selection Signals**

- `selectionChanged(annotationId)` - New annotation selected
- `selectionCleared()` - Selection cleared
- `interactionStarted()` - Mouse interaction began
- `interactionEnded()` - Mouse interaction completed

**Modification Signals**

- `annotationMoved(annotationId, newPosition)` - After successful move
- `annotationResized(annotationId, newBoundary)` - After successful resize

### Helper Methods

**Point-in-Rectangle Testing**

```cpp
isPointInRect(point, rect, tolerance)
```

Expands rect by tolerance and checks containment. Used for hit testing annotations and handles.

**Boundary Calculation**

```cpp
calculateNewBoundary(original, handle, delta)
```

Applies delta movement to appropriate edge(s) based on handle position. Normalizes resulting rectangle (positive width/height).

## 3. Relevant Code Modules

- `/app/ui/managers/AnnotationSelectionManager.h` - Header with selection interface
- `/app/ui/managers/AnnotationSelectionManager.cpp` - Implementation (302 lines)
- `/app/controller/AnnotationController.h` - Annotation data and command execution
- `/app/delegate/AnnotationRenderDelegate.h` - Selection visual feedback
- `/app/logging/SimpleLogging.h` - Logging integration
- Qt Core: `QObject`, `QPointF`, `QRectF`, `QString`

## 4. Attention

- Handle positions use 8-point system (corners + edge midpoints); coordinates must be page-relative
- Hit tolerance should balance responsiveness (too small) vs false positives (too large); 5 pixels is recommended
- Deferred commit pattern (no changes applied during move) prevents intermediate visual glitches
- Zoom factor must be > 0.0; scale adjustments fail with invalid zoom
- Minimum size enforcement (10x10 pixels) prevents creating degenerate annotations during resize
