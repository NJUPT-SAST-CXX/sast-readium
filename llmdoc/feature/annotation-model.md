# Annotation Model

## 1. Purpose

The AnnotationModel manages PDF annotations within the application, providing a unified interface for creating, retrieving, modifying, and persisting annotations of various types (highlights, notes, shapes, drawings). It integrates with Qt's model/view architecture via QAbstractListModel and supports Poppler document annotation loading/saving.

## 2. How It Works

### Core Data Structure

PDFAnnotation struct represents a single annotation with:

- **Identity**: id (unique identifier), type (AnnotationType enum)
- **Location**: pageNumber, boundingRect (QRectF)
- **Content**: content (text), author, isVisible flag
- **Timestamps**: createdTime, modifiedTime (QDateTime objects)
- **Styling**: color (QColor), opacity (0.0-1.0), lineWidth
- **Type-Specific**: inkPath (for freehand drawing), startPoint/endPoint (for lines/arrows), fontFamily/fontSize (for text)

### Annotation Types Supported

- Highlight, Note, FreeText, Underline, StrikeOut, Squiggly (text markup)
- Rectangle, Circle, Line, Arrow (geometric shapes)
- Ink (freehand drawing)

### Model Operations

**CRUD Operations**:

- `addAnnotation()` - Add new annotation, returns bool success status
- `getAnnotation()` - Retrieve by ID
- `updateAnnotation()` - Modify existing annotation
- `removeAnnotation()` - Delete annotation by ID

**Page Operations**:

- `getAnnotationsForPage()` - List all annotations on specific page
- `removeAnnotationsForPage()` - Delete all annotations on page
- `getAnnotationCountForPage()` - Count annotations on page

**Serialization**:

- `toJson()` / `fromJson()` - Convert to/from QJsonObject for persistence
- `loadAnnotationsFromDocument()` - Load from Poppler document
- `saveAnnotationsToDocument()` - Persist to Poppler document

**Search & Filter**:

- `searchAnnotations()` - Search by content (case-insensitive)
- `getAnnotationsByType()` - Filter by annotation type
- `getAnnotationsByAuthor()` - Filter by author
- `getRecentAnnotations()` - Retrieve most recently modified

**Sticky Notes**:

- `addStickyNote()` - Helper for quick note creation
- `getStickyNotesForPage()` - Get notes for specific page

**Statistics**:

- `getTotalAnnotationCount()` - Total annotations in document
- `getAnnotationCountByType()` - Distribution by type
- `getAuthors()` - List unique annotation authors

### Critical Build Fixes Applied

**Field Name Consistency**: Fixed mismatches between struct definition and implementation:

- `boundary` → `boundingRect` (3 occurrences in serialization/deserialization)
- `creationDate` → `createdTime` (2 occurrences)
- `modificationDate` → `modifiedTime` (1 occurrence)

These fixes ensure field names in both `.cpp` implementation and `.h` struct definition match exactly, preventing runtime serialization errors.

### Qt Model Integration

Implements QAbstractListModel interface:

- `rowCount()` - Returns annotation count
- `data()` - Returns annotation data for display roles
- `setData()` - Updates annotation properties
- `roleNames()` - Maps role enums (IdRole, TypeRole, PageNumberRole, etc.)

Emits signals: `annotationAdded()`, `annotationRemoved()`, `annotationUpdated()`, `annotationsLoaded()`, `annotationsSaved()`, `annotationsCleared()`

### Integration with Application Layers

The AnnotationModel provides the data layer for the annotation system. Business logic and presentation are handled by complementary components:

- **[Annotation Controller](annotation-controller.md)** - Business logic, persistence, event integration, and command coordination
- **[Annotation Commands](annotation-commands.md)** - 13 command classes supporting full undo/redo for all modifications
- **[Annotation Rendering](annotation-rendering.md)** - Visual rendering of annotations on PDF pages

## 3. Relevant Code Modules

- `/app/model/AnnotationModel.h` - Header with PDFAnnotation struct and AnnotationModel class
- `/app/model/AnnotationModel.cpp` - Implementation with serialization and model logic
- `/app/controller/AnnotationController.h` - Business logic layer
- `/app/command/AnnotationCommands.h` - Command classes
- `/app/delegate/AnnotationRenderDelegate.h` - Rendering delegate
- Dependencies: Poppler-Qt6 (`poppler-qt6.h`, `poppler-annotation.h`), Qt Core (`QAbstractListModel`, `QJsonObject`, `QDateTime`)

## 4. Attention

- PDFAnnotation generates unique IDs automatically combining epoch timestamp and random number
- Serialization relies on correct field names (boundingRect, createdTime, modifiedTime) matching struct definition
- Poppler integration may require Poppler::Document pointer set via `setDocument()` before loading annotations
- All timestamps use QDateTime with millisecond precision for modification tracking
- AnnotationModel is used as dependency by AnnotationController and all command classes; do not instantiate directly
