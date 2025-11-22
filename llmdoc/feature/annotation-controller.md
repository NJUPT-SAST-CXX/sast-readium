# Annotation Controller

## 1. Purpose

The AnnotationController provides the business logic layer for managing PDF annotations. It integrates annotation operations with the application's core systems: CommandManager for undo/redo, EventBus for event-driven communication, CacheManager for persistence, and ServiceLocator for dependency injection. Acts as the primary interface between the UI layer and the AnnotationModel.

## 2. How It Works

### Architecture Overview

AnnotationController follows the MVC pattern as the controller component:

- **Model**: AnnotationModel (data management, Qt model/view integration)
- **Controller**: AnnotationController (business logic, persistence, event coordination)
- **View**: AnnotationRenderDelegate and UI components (presentation)

### Key Components

**Singleton Pattern**

- Registered with ServiceLocator during application initialization
- Accessed via `AnnotationController::instance()`
- Manages a single AnnotationModel instance for the application lifecycle

**Document Management**

```
setDocument(Poppler::Document*, filePath)
├─ Saves current annotations (if auto-save enabled)
├─ Sets document reference in AnnotationModel
├─ Loads annotations from cache or document
└─ Publishes "annotation.document_changed" event
```

**Command Integration**

All modification operations execute through CommandManager for undo/redo support:

- `addAnnotation()` → AddAnnotationCommand
- `removeAnnotation()` → RemoveAnnotationCommand
- `updateAnnotation()` → UpdateAnnotationCommand
- `moveAnnotation()` → MoveAnnotationCommand
- `resizeAnnotation()` → ResizeAnnotationCommand
- `changeAnnotationColor()` → ChangeAnnotationColorCommand
- `changeAnnotationOpacity()` → ChangeAnnotationOpacityCommand
- `toggleAnnotationVisibility()` → ToggleAnnotationVisibilityCommand
- `editAnnotationContent()` → UpdateAnnotationContentCommand (with mergeWith support for rapid edits)

**Batch Operations**

Efficient handling of multiple annotations:

- `batchAddAnnotations()` → BatchAddAnnotationsCommand
- `removeAnnotationsForPage()` → RemovePageAnnotationsCommand
- `clearAllAnnotations()` → ClearAllAnnotationsCommand

**Quick Annotation Creation Helpers**

Convenience methods pre-populate annotation properties:

- `addHighlight()` - Creates text highlight with optional color
- `addNote()` - Creates sticky note at position
- `addShape()` - Creates geometric shape (rectangle, circle, line, arrow)

### Persistence System

**Auto-Save Mechanism**

- Enabled by default via `setAutoSaveEnabled()`
- Triggered when document changes or on clearDocument()
- Saves to both Poppler document and cache

**Cache Integration**

- `loadAnnotationsFromCache()` - Loads from CacheManager if available
- `saveAnnotationsToCache()` - Caches for fast access
- `getCacheKey()` - Generates cache key from document path

Cache key format: `annotation_<md5_hash_of_file_path>`

**Serialization**

- `saveAnnotations()` - Exports to Poppler document and optionally saves to file
- `loadAnnotations()` - Restores from Poppler document
- `exportAnnotations()` - Exports to external file (JSON format)
- `importAnnotations()` - Imports from external file

### Event System

AnnotationController publishes EventBus events for system-wide coordination:

- `annotation.document_changed` - When document is set
- `annotation.document_cleared` - When document is cleared
- `annotation.added` - When annotation is added
- `annotation.removed` - When annotation is removed
- `annotation.updated` - When annotation is modified

Events forwarded from AnnotationModel signals for loose coupling between UI components.

### Signal Forwarding

Controller forwards all model signals plus additional status signals:

**Model Signals:**

- `annotationAdded(PDFAnnotation)`
- `annotationRemoved(QString)`
- `annotationUpdated(PDFAnnotation)`
- `annotationsLoaded(int count)`
- `annotationsSaved(int count)`
- `annotationsCleared()`

**Controller Signals:**

- `documentChanged()`
- `documentCleared()`
- `operationCompleted(bool success, QString message)`
- `error(QString errorMessage)`

### Query Operations

Read-only access to annotation data:

- `getAnnotationsForPage(int pageNumber)` - All annotations on page
- `getAnnotation(QString id)` - Single annotation by ID
- `searchAnnotations(QString query)` - Content search (case-insensitive)
- `getAnnotationsByType(AnnotationType)` - Filter by type
- `getTotalAnnotationCount()` - Total count
- `getAnnotationCountForPage(int)` - Count on page

### Settings Management

- `setDefaultAuthor(QString)` - Author name for new annotations
- `setAutoSaveEnabled(bool)` - Enable/disable auto-save on document change
- `isAutoSaveEnabled()` - Check auto-save status

## 3. Relevant Code Modules

- `/app/controller/AnnotationController.h` - Header with controller interface
- `/app/controller/AnnotationController.cpp` - Implementation (577 lines)
- `/app/model/AnnotationModel.h` - Data model (required dependency)
- `/app/command/AnnotationCommands.h` - Command classes for undo/redo
- `/app/controller/ServiceLocator.h` - Dependency injection mechanism
- `/app/controller/EventBus.h` - Event-driven communication
- `/app/cache/CacheManager.h` - Persistence caching
- `/app/command/CommandManager.h` - Undo/redo stack management
- `/app/logging/SimpleLogging.h` - Logging integration

## 4. Attention

- AnnotationController::instance() will create a new instance if none exists; typically initialized in main.cpp
- Auto-save behavior saves annotations before switching documents; disable for performance-critical scenarios
- PDFAnnotation validation occurs during add/update operations; invalid annotations are rejected
- Cache keys are deterministic based on file path to ensure consistent cache access
- All modification operations require CommandManager registration; direct model access bypasses undo/redo
