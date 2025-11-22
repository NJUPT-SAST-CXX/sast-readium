# Annotation Integration Helper

## 1. Purpose

The AnnotationIntegrationHelper provides a centralized integration point that coordinates all annotation system components (controller, renderer, selection manager, UI components). Simplifies attachment to PDFViewer and RightSideBar, manages rendering pipeline integration, and routes mouse events through the selection system.

## 2. How It Works

### Component Coordination

**Architecture Layers**

```
UI Integration Layer (AnnotationIntegrationHelper)
├─ AnnotationController (business logic, persistence)
├─ AnnotationRenderDelegate (visual rendering)
├─ AnnotationSelectionManager (user interaction)
├─ AnnotationToolbar (UI controls)
├─ AnnotationsPanel (annotation list)
└─ PDFViewer (rendering surface)
```

Helper acts as facade pattern, hiding complexity of inter-component coordination.

**Component Creation**

```
initialize()
├─ Retrieve AnnotationController from ServiceLocator
├─ Create AnnotationRenderDelegate with controller
├─ Create AnnotationSelectionManager with controller + delegate
├─ Connect all internal signals
└─ Return success/failure
```

### Document Management

**Document Lifecycle**

```
setDocument(poppler::Document*, filePath)
├─ Pass to AnnotationController
├─ Controller loads annotations from cache/document
└─ Helper stores current file path

hasDocument() → bool
├─ Returns true if controller has active document
└─ False before document set or after clearDocument()

clearDocument()
├─ Inform AnnotationController to save annotations
├─ Reset internal state
└─ Emit documentCleared signal
```

### Rendering Integration

**Page Rendering**

```
renderAnnotations(QPainter*, pageNumber, pageRect, zoomFactor)
├─ Delegate to AnnotationRenderDelegate
├─ Render delegates to render all annotations on page
└─ Return to caller for post-processing
```

Called from PDFViewer's page rendering pipeline after PDF content drawn.

**Integration Point**

Rendering should be called from page rendering code like:

```cpp
// In PageWidget::paintEvent()
painter.drawPDF(...);  // Render PDF page
helper->renderAnnotations(&painter, pageNumber, pageRect, zoom);
```

### Mouse Event Handling

**Event Flow**

```
Mouse Press:
  handleMousePress(point, pageNumber, zoom)
  └─ SelectionManager.handleMousePress()
     └─ Selects annotation or detects resize handle

Mouse Move:
  handleMouseMove(point, zoom)
  └─ SelectionManager.handleMouseMove()
     └─ Tracks movement (no changes applied yet)

Mouse Release:
  handleMouseRelease(point, zoom)
  └─ SelectionManager.handleMouseRelease()
     └─ Commits changes via AnnotationController commands
```

All mouse coordinates should be in page-relative coordinates (0-based indices).

**Event Filter Integration**

Helper installs event filter on PDFViewer:

```cpp
attachToPDFViewer(PDFViewer* viewer)
└─ Calls viewer->installEventFilter(this)
   └─ Intercepts QMouseEvent before PDFViewer processes
```

Event filter overrides `eventFilter(QObject*, QEvent*)` to route mouse events.

### UI Component Integration

**PDFViewer Attachment**

```
attachToPDFViewer(PDFViewer* viewer)
├─ Store viewer reference
├─ Install event filter for mouse events
├─ Enable annotation rendering on page paint
└─ Return true on successful attachment
```

**RightSideBar Attachment**

```
attachToRightSideBar(RightSideBar* sidebar)
├─ Find or create AnnotationsPanel in sidebar
├─ Connect panel signals to controller
├─ Enable annotation list updates
└─ Return true on successful attachment
```

AnnotationsPanel displays list of all annotations with click to select.

**Toolbar Attachment**

```
attachToolbar(AnnotationToolbar* toolbar)
├─ Connect toolbar tool selection signals
├─ Connect color/style selection signals
└─ Allow toolbar to initiate new annotation creation
```

### Signal Connections

**Internal Signal System**

```
AnnotationController signals
├─ annotationAdded → onAnnotationAdded()
│  └─ Emit annotationsChanged() for repaints
├─ annotationRemoved → onAnnotationRemoved()
│  └─ Trigger rerender
├─ annotationUpdated → onAnnotationModified()
│  └─ Trigger rerender
└─ [other signals] → [handlers]

SelectionManager signals
├─ selectionChanged → onSelectionChanged()
│  └─ Emit annotationSelected() to UI components
└─ annotationMoved/Resized → trigger rerender
```

**External Signal Emission**

```
annotationSelected(QString id)
└─ Emitted when user selects annotation

selectionCleared()
└─ Emitted when selection cleared

annotationsChanged()
└─ Emitted when annotations modified (for UI rerender)
```

### Component Access

Read-only access to internal components:

```cpp
controller() → AnnotationController*
renderDelegate() → AnnotationRenderDelegate*
selectionManager() → AnnotationSelectionManager*
```

Allows calling component-specific APIs if needed beyond integration helper interface.

### Lifecycle Management

**Initialization Sequence**

1. Create AnnotationIntegrationHelper in PDFViewerPage constructor
2. Call initialize() to create all sub-components
3. When document loaded: setDocument(document, filePath)
4. Call attachToPDFViewer(viewer) and attachToRightSideBar(sidebar)
5. When document closed: clearDocument() then setDocument(nullptr, "")

**Cleanup**

Destructor automatically:

- Calls detachFromPDFViewer() to remove event filter
- Calls disconnectSignals() to clean up connections
- Destroys render delegate and selection manager

## 3. Relevant Code Modules

- `/app/ui/integration/AnnotationIntegrationHelper.h` - Header with integration interface
- `/app/ui/integration/AnnotationIntegrationHelper.cpp` - Implementation (302 lines)
- `/app/controller/AnnotationController.h` - Annotation business logic
- `/app/delegate/AnnotationRenderDelegate.h` - Visual rendering
- `/app/ui/managers/AnnotationSelectionManager.h` - User interaction
- `/app/ui/core/RightSideBar.h` - Right sidebar for panels
- `/app/ui/viewer/PDFViewer.h` - PDF rendering surface
- `/app/controller/ServiceLocator.h` - Component registration
- `/app/logging/SimpleLogging.h` - Logging integration

## 4. Attention

- Must call initialize() before using helper; returns false if AnnotationController not registered in ServiceLocator
- Document must be set before attaching to viewers; attachments assume document context exists
- All mouse coordinates must be page-relative (0-based indices), not widget-relative or document-relative
- Event filter registration with PDFViewer requires PDFViewer to support eventFilter(); verify compatibility
- Destruction order: detach from viewers before destroying helper to prevent dangling references
