# Annotation Commands

## 1. Purpose

AnnotationCommands provides 13 command classes implementing the Command pattern for all annotation modification operations. Each command supports full undo/redo functionality through QUndoStack integration. Enables repeatable annotation workflows with complete state restoration and merge optimization for rapid successive edits.

## 2. How It Works

### Command Pattern Architecture

All annotation commands inherit from `AnnotationCommand` base class:

```
AnnotationCommand (base)
├─ AddAnnotationCommand
├─ RemoveAnnotationCommand
├─ UpdateAnnotationCommand
├─ UpdateAnnotationContentCommand
├─ MoveAnnotationCommand
├─ ResizeAnnotationCommand
├─ ChangeAnnotationColorCommand
├─ ChangeAnnotationOpacityCommand
├─ ToggleAnnotationVisibilityCommand
├─ ClearAllAnnotationsCommand
├─ RemovePageAnnotationsCommand
├─ BatchAddAnnotationsCommand
└─ BatchRemoveAnnotationsCommand
```

Base class (`AnnotationCommand`) stores reference to AnnotationModel and provides common error handling.

### State Management Pattern

Each command maintains internal state to enable undo/redo:

**State Preservation Pattern**

```
Command Creation:
1. Store ORIGINAL state (from model)
2. Execute operation (first redo)
3. On undo(): restore original state
4. On redo(): re-apply operation
```

**First-Time Execution Optimization**

- `m_firstTime` flag prevents duplicate execution
- First redo() skips model operation (already executed during creation)
- Subsequent redo() calls re-apply the operation

### Individual Command Specifications

**AddAnnotationCommand (ID: 2001)**

Stores annotation copy; undo removes, redo adds:

```
Creation: Copy PDFAnnotation provided
Undo:    Model::removeAnnotation(id)
Redo:    Model::addAnnotation(stored_annotation)
```

**RemoveAnnotationCommand (ID: 2002)**

Captures annotation before deletion for restoration:

```
Creation: Fetch existing annotation from model, store it
Undo:    Model::addAnnotation(removed_annotation)
Redo:    Model::removeAnnotation(id)
```

**UpdateAnnotationContentCommand (ID: 2003)**

Supports `mergeWith()` for consecutive edits (e.g., typing in note):

```
Creation: Store annotation ID, old content, new content
Undo:    Model::updateAnnotation() with old content
Redo:    Model::updateAnnotation() with new content
Merge:   If consecutive edits, combine into single command
```

Merge algorithm: If new command modifies same annotation ID and is created within merge time window, combines commands and updates final content.

**MoveAnnotationCommand (ID: 2004)**

Stores bounding rectangle top-left position:

```
Creation: Capture old position (boundingRect.topLeft())
Undo:    Model::updateAnnotation() with old position
Redo:    Model::updateAnnotation() with new position
```

**ResizeAnnotationCommand (ID: 2005)**

Stores complete bounding rectangles:

```
Creation: Capture old boundingRect
Undo:    Model::updateAnnotation() with old rect
Redo:    Model::updateAnnotation() with new rect
```

**ChangeAnnotationColorCommand (ID: 2006)**

Stores QColor values:

```
Creation: Capture old color
Undo:    Model::updateAnnotation() with old color
Redo:    Model::updateAnnotation() with new color
```

**ChangeAnnotationOpacityCommand (ID: 2007)**

Stores double opacity values (0.0-1.0):

```
Creation: Capture old opacity
Undo:    Model::updateAnnotation() with old opacity
Redo:    Model::updateAnnotation() with new opacity
```

**ToggleAnnotationVisibilityCommand (ID: 2008)**

Stores boolean visibility state:

```
Creation: Capture current visibility state
Undo:    Model::updateAnnotation() with original visibility
Redo:    Model::updateAnnotation() with toggled visibility
```

**UpdateAnnotationCommand (ID: 2009)**

Full annotation replacement:

```
Creation: Store entire old annotation
Undo:    Model::updateAnnotation() with old annotation
Redo:    Model::updateAnnotation() with new annotation
```

**ClearAllAnnotationsCommand (ID: 2010)**

Batch operation on all annotations:

```
Creation: Fetch all annotations from model
Undo:    Model::batchAddAnnotations() with all stored annotations
Redo:    Model::clearAnnotations()
```

**RemovePageAnnotationsCommand (ID: 2011)**

Batch operation on single page:

```
Creation: Fetch all annotations for page number
Undo:    Model::batchAddAnnotations() with stored page annotations
Redo:    Model::removeAnnotationsForPage(pageNumber)
```

**BatchAddAnnotationsCommand (ID: 2012)**

Multiple simultaneous additions:

```
Creation: Store annotation list
Undo:    Remove each annotation individually
Redo:    Add all annotations (skips first redo)
```

**BatchRemoveAnnotationsCommand (ID: 2013)**

Multiple simultaneous removals:

```
Creation: Fetch all annotations matching IDs
Undo:    Add all stored annotations
Redo:    Remove matching ID annotations
```

### AnnotationCommandFactory

Centralized factory for command creation. Provides static factory methods:

- `createAddCommand()` → AddAnnotationCommand*
- `createRemoveCommand()` → RemoveAnnotationCommand*
- `createUpdateContentCommand()` → UpdateAnnotationContentCommand*
- `createMoveCommand()` → MoveAnnotationCommand*
- `createResizeCommand()` → ResizeAnnotationCommand*
- `createChangeColorCommand()` → ChangeAnnotationColorCommand*
- `createChangeOpacityCommand()` → ChangeAnnotationOpacityCommand*
- `createToggleVisibilityCommand()` → ToggleAnnotationVisibilityCommand*
- `createUpdateCommand()` → UpdateAnnotationCommand*
- `createClearAllCommand()` → ClearAllAnnotationsCommand*
- `createRemovePageCommand()` → RemovePageAnnotationsCommand*
- `createBatchAddCommand()` → BatchAddAnnotationsCommand*
- `createBatchRemoveCommand()` → BatchRemoveAnnotationsCommand*

Factory enables consistent error handling and potential logging injection.

### Integration with QUndoStack

AnnotationController methods execute commands via CommandManager:

```cpp
bool AnnotationController::addAnnotation(const PDFAnnotation& annotation) {
    auto* cmd = new AddAnnotationCommand(m_model, annotation);
    CommandManager::instance().executeCommand(cmd);
    return true;
}
```

CommandManager manages QUndoStack lifecycle and provides undo/redo history.

### Logging Integration

All commands use SimpleLogging for debug output:

- SLOG_DEBUG_F() on undo/redo operations
- SLOG_ERROR() on null model access
- Command ID and annotation ID included for traceability

## 3. Relevant Code Modules

- `/app/command/AnnotationCommands.h` - Header with all 13 command classes and factory
- `/app/command/AnnotationCommands.cpp` - Implementation (520 lines)
- `/app/model/AnnotationModel.h` - PDFAnnotation and AnnotationModel
- `/app/controller/AnnotationController.h` - Command execution through controller
- `/app/command/CommandManager.h` - Undo/redo stack management
- `/app/logging/SimpleLogging.h` - Logging macros for debug output
- `/tests/test_annotation_commands.cpp` - Unit tests covering all commands

## 4. Attention

- First-time execution uses m_firstTime flag to prevent duplicate model operations; redo() must check this flag before calling model methods
- Merge support only implemented in UpdateAnnotationContentCommand; other commands use default mergeWith() returning false
- Batch commands store complete annotation objects for undo restoration; memory overhead for large batch operations
- Command IDs (2001-2013) are unique across all command types to prevent unintended merge operations
- All commands must be allocated on heap; CommandManager takes ownership and handles deletion
