# UI Components File Organization and Relationships

## Overview

This document provides a complete file structure map of all UI components and their interdependencies in the SAST Readium project.

---

## Directory Structure

```
d:\Project\sast-readium\
├── app/
│   ├── MainWindow.h
│   ├── MainWindow.cpp
│   └── ui/
│       ├── core/
│       │   ├── MenuBar.h
│       │   ├── MenuBar.cpp
│       │   ├── ToolBar.h
│       │   ├── ToolBar.cpp
│       │   ├── StatusBar.h
│       │   ├── StatusBar.cpp
│       │   ├── SideBar.h
│       │   ├── SideBar.cpp
│       │   ├── RightSideBar.h
│       │   ├── RightSideBar.cpp
│       │   ├── ViewWidget.h
│       │   ├── ViewWidget.cpp
│       │   ├── ContextMenuManager.h
│       │   ├── ContextMenuManager.cpp
│       │   ├── UIStateManager.h
│       │   ├── UIStateManager.cpp
│       │   ├── UIErrorHandler.h
│       │   ├── UIErrorHandler.cpp
│       │   ├── UIResourceManager.h
│       │   ├── UIResourceManager.cpp
│       │   ├── UIRecoveryManager.h
│       │   ├── UIRecoveryManager.cpp
│       │   ├── UIConsistencyManager.h
│       │   └── UIConsistencyManager.cpp
│       ├── dialogs/
│       │   ├── DocumentMetadataDialog.h
│       │   ├── DocumentMetadataDialog.cpp
│       │   ├── DocumentComparison.h
│       │   ├── DocumentComparison.cpp
│       │   ├── SettingsDialog.h
│       │   └── SettingsDialog.cpp
│       ├── widgets/
│       │   ├── DocumentTabWidget.h
│       │   ├── DocumentTabWidget.cpp
│       │   ├── SearchWidget.h
│       │   ├── SearchWidget.cpp
│       │   ├── WelcomeWidget.h
│       │   ├── WelcomeWidget.cpp
│       │   ├── DocumentPropertiesPanel.h
│       │   ├── DocumentPropertiesPanel.cpp
│       │   ├── BookmarkWidget.h
│       │   ├── BookmarkWidget.cpp
│       │   ├── DebugLogPanel.h
│       │   ├── DebugLogPanel.cpp
│       │   ├── TutorialCard.h
│       │   ├── TutorialCard.cpp
│       │   ├── AnnotationToolbar.h
│       │   ├── AnnotationToolbar.cpp
│       │   ├── ToastNotification.h
│       │   ├── ToastNotification.cpp
│       │   ├── NotificationHelper.h
│       │   ├── NotificationHelper.cpp
│       │   ├── RecentFileListWidget.h
│       │   ├── RecentFileListWidget.cpp
│       │   ├── OnboardingWidget.h
│       │   ├── OnboardingWidget.cpp
│       │   ├── SkeletonWidget.h
│       │   ├── SkeletonWidget.cpp
│       │   └── EnhancedFocusIndicator.h
│       │   └── EnhancedFocusIndicator.cpp
│       ├── viewer/
│       │   ├── PDFViewer.h
│       │   ├── PDFViewer.cpp
│       │   ├── PDFPrerenderer.h
│       │   ├── PDFPrerenderer.cpp
│       │   ├── PDFAnimations.h
│       │   ├── PDFAnimations.cpp
│       │   ├── PDFOutlineWidget.h
│       │   ├── PDFOutlineWidget.cpp
│       │   ├── QGraphicsPDFViewer.h
│       │   ├── QGraphicsPDFViewer.cpp
│       │   ├── PDFViewerComponents.h
│       │   ├── PDFViewerComponents.cpp
│       │   ├── SplitViewManager.h
│       │   └── SplitViewManager.cpp
│       ├── thumbnail/
│       │   ├── ThumbnailListView.h
│       │   ├── ThumbnailListView.cpp
│       │   ├── ThumbnailGenerator.h
│       │   ├── ThumbnailGenerator.cpp
│       │   ├── ThumbnailWidget.h
│       │   ├── ThumbnailWidget.cpp
│       │   ├── ThumbnailContextMenu.h
│       │   └── ThumbnailContextMenu.cpp
│       ├── managers/
│       │   ├── WelcomeScreenManager.h
│       │   └── WelcomeScreenManager.cpp
│       ├── theme/
│       │   ├── ReadingModeManager.h
│       │   └── ReadingModeManager.cpp
│       └── utils/
│           ├── ValidationUtils.h
│           └── ValidationUtils.cpp
```

---

## Component Dependency Map

### Core Components Dependencies

```
MainWindow
├── ApplicationController
├── ViewDelegate
├── MenuBar
│   └── RecentFilesManager
├── ToolBar
│   └── ContextMenuManager
├── StatusBar
│   └── WidgetFactory
├── ViewWidget
│   ├── DocumentController
│   ├── DocumentModel
│   ├── PDFOutlineModel
│   ├── DocumentTabWidget
│   │   └── ContextMenuManager
│   └── PDFViewer
│       ├── DocumentModel
│       ├── SearchModel
│       ├── PDFAnimations
│       ├── PDFPrerenderer
│       └── SearchWidget
├── SideBar
│   ├── ThumbnailListView
│   │   ├── ThumbnailModel
│   │   └── ThumbnailDelegate
│   ├── BookmarkWidget
│   │   └── BookmarkModel
│   ├── ThumbnailDelegate
│   └── PDFOutlineWidget
│       └── PDFOutlineModel
├── RightSideBar
│   ├── DocumentPropertiesPanel
│   ├── AnnotationToolbar
│   ├── SearchWidget
│   └── DebugLogPanel
│       └── LoggingManager
├── ContextMenuManager
├── UIStateManager (Singleton)
├── UIErrorHandler (Singleton)
├── UIResourceManager (Singleton)
├── UIRecoveryManager
├── UIConsistencyManager
└── WelcomeScreenManager
    └── DocumentModel
```

### Dialog Components Dependencies

```
DocumentMetadataDialog
├── StyleManager
└── Poppler::Document

SettingsDialog (no external dependencies)

DocumentComparison
└── PDFViewer (multiple instances)
```

### Widget Components Dependencies

```
DocumentTabWidget
├── DocumentModel
└── ContextMenuManager

SearchWidget
├── SearchModel
├── Poppler::Document
└── ContextMenuManager

WelcomeWidget
├── RecentFilesManager
├── WelcomeScreenManager
├── OnboardingManager
├── CommandManager
└── RecentFileListWidget

DocumentPropertiesPanel
└── Poppler::Document

BookmarkWidget
└── BookmarkModel

DebugLogPanel
├── Logger
└── LoggingManager

TutorialCard (no external dependencies)

AnnotationToolbar
└── AnnotationModel

ToastNotification (no external dependencies)

NotificationHelper (no external dependencies)

RecentFileListWidget
└── RecentFilesManager

OnboardingWidget
└── CommandManager

SkeletonWidget (no external dependencies)

EnhancedFocusIndicator (no external dependencies)
```

### Viewer Components Dependencies

```
PDFViewer
├── DocumentModel
├── SearchModel
├── PDFAnimations
├── PDFPrerenderer
├── SearchWidget
└── Poppler::Document

PDFPageWidget
├── Poppler::Page
└── PDFPrerenderer

PDFPrerenderer
├── Poppler::Document
└── RenderModel

PDFAnimations (no external dependencies)

PDFOutlineWidget
└── PDFOutlineModel

QGraphicsPDFViewer [Optional - ENABLE_QGRAPHICS_PDF_SUPPORT]
└── Poppler::Document

PDFViewerComponents (utility components)

SplitViewManager
├── PDFViewer (multiple instances)
└── DocumentModel
```

### Thumbnail Components Dependencies

```
ThumbnailListView
├── ThumbnailModel
├── ThumbnailDelegate
└── ThumbnailContextMenu

ThumbnailGenerator
└── Poppler::Document

ThumbnailWidget
└── ThumbnailListView

ThumbnailContextMenu
└── ThumbnailModel
```

### Manager Components Dependencies

```
WelcomeScreenManager
├── MainWindow
├── WelcomeWidget
└── DocumentModel
```

---

## File Groups and Responsibilities

### Main Window Group (1 file pair)

- `MainWindow` - Application entry point and main container

### Core UI Components (12 file pairs)

- `MenuBar` - Menu management
- `ToolBar` - Toolbar management with file, navigation, zoom, view, tools operations
- `StatusBar` - Status display with expandable panels
- `SideBar` - Left sidebar with thumbnails and bookmarks
- `RightSideBar` - Right sidebar with properties, tools, search, debug
- `ViewWidget` - Document viewing area with multi-tab support
- `ContextMenuManager` - Context menu creation and management
- `UIStateManager` - UI state persistence and management (Singleton)
- `UIErrorHandler` - Error handling and user feedback (Singleton)
- `UIResourceManager` - Resource tracking and cleanup (Singleton)
- `UIRecoveryManager` - Error recovery strategies
- `UIConsistencyManager` - UI consistency enforcement

### Dialogs (3 file pairs)

- `DocumentMetadataDialog` - Metadata display dialog
- `DocumentComparison` - Document comparison dialog
- `SettingsDialog` - Settings dialog

### Widgets (14 file pairs)

- `DocumentTabWidget` - Multi-document tab management
- `SearchWidget` - Search interface
- `WelcomeWidget` - Welcome screen
- `DocumentPropertiesPanel` - Properties panel (sidebar)
- `BookmarkWidget` - Bookmark management
- `DebugLogPanel` - Debug logging display
- `TutorialCard` - Tutorial card widget
- `AnnotationToolbar` - Annotation tools toolbar
- `ToastNotification` - Toast notifications
- `NotificationHelper` - Notification management helper
- `RecentFileListWidget` - Recent files list
- `OnboardingWidget` - Onboarding interface
- `SkeletonWidget` - Loading skeleton
- `EnhancedFocusIndicator` - Focus indicator

### Viewers (6 file pairs)

- `PDFViewer` - Main PDF viewer
- `PDFPrerenderer` - Asynchronous page prerendering
- `PDFAnimations` - Animation support
- `PDFOutlineWidget` - Document outline tree
- `QGraphicsPDFViewer` - Graphics view-based viewer (optional)
- `SplitViewManager` - Split view management

### Thumbnails (4 file pairs)

- `ThumbnailListView` - Thumbnail list with virtual scrolling
- `ThumbnailGenerator` - Thumbnail generation
- `ThumbnailWidget` - Thumbnail container
- `ThumbnailContextMenu` - Thumbnail context menu

### Managers (1 file pair)

- `WelcomeScreenManager` - Welcome screen state management

### Theme/Utils (2 file pairs)

- `ReadingModeManager` - Reading mode and theme management
- `ValidationUtils` - UI input validation utilities

---

## Component Initialization Order

### Application Startup Sequence

1. **MainWindow Creation**
   - Creates ApplicationController and ViewDelegate

2. **UI Components Creation (in MainWindow or ApplicationController)**
   - MenuBar
   - ToolBar
   - StatusBar
   - SideBar
   - RightSideBar
   - ViewWidget

3. **Manager Initialization**
   - UIStateManager (Singleton - lazy init)
   - UIErrorHandler (Singleton - lazy init)
   - UIResourceManager (Singleton - lazy init)
   - WelcomeScreenManager
   - RecentFilesManager

4. **State Restoration**
   - UIStateManager::restoreAllStates()
   - WelcomeScreenManager::loadSettings()

5. **Welcome Screen Setup**
   - WelcomeWidget display (if enabled and no documents open)

---

## Inter-Component Communication Patterns

### Signal/Slot Connections

1. **Menu Actions → Document Operations**

   ```
   MenuBar::onExecuted() → ApplicationController::onActionRequested()
   ```

2. **Document Changes → UI Updates**

   ```
   DocumentModel::documentOpened() → ViewWidget::onDocumentOpened()
   DocumentModel::documentOpened() → SideBar::setDocument()
   DocumentModel::documentOpened() → RightSideBar::setDocument()
   ```

3. **Page Navigation → Viewer Updates**

   ```
   ToolBar::pageJumpRequested() → ViewWidget::goToPage()
   StatusBar::pageJumpRequested() → ViewWidget::goToPage()
   PDFOutlineWidget::pageNavigationRequested() → ViewWidget::goToPage()
   ThumbnailListView::pageClicked() → ViewWidget::goToPage()
   ```

4. **Search Operations → Highlighting**

   ```
   SearchWidget::searchRequested() → PDFViewer::performSearch()
   SearchWidget::navigateToResult() → PDFViewer::scrollToResult()
   ```

5. **State Changes → Persistence**

   ```
   ViewWidget::currentViewerPageChanged() → UIStateManager::setState()
   SideBar::widthChanged() → UIStateManager::setState()
   UIStateManager::stateChanged() → Persistence to disk
   ```

6. **Error Handling → User Feedback**

   ```
   DocumentController::error() → UIErrorHandler::handleSystemError()
   UIErrorHandler::showFeedback() → ToastNotification display
   ```

7. **Resource Management → Cleanup**

   ```
   UIResourceManager tracks all widgets
   UIResourceManager::cleanupTimer() → periodically clean expired resources
   ```

---

## Component Usage Examples

### Opening a Document

```
1. User selects "Open" from MenuBar
2. MenuBar emits onExecuted(ActionMap::Open)
3. ApplicationController opens file dialog
4. ApplicationController calls DocumentController::openDocument()
5. DocumentController::openDocument() creates DocumentModel
6. DocumentModel emits documentOpened() signal
7. ViewWidget::onDocumentOpened() creates PDFViewer
8. PDFViewer loads document and renders pages
9. SideBar::setDocument() sets up thumbnails
10. RightSideBar::setDocument() displays metadata
11. StatusBar updates with document info
12. UIStateManager saves document state
```

### Searching in Document

```
1. User enters search text in SearchWidget
2. SearchWidget::performSearch() creates search request
3. SearchWidget emits searchRequested() signal
4. PDFViewer performs search on document
5. PDFViewer emits search results
6. SearchWidget displays results in list
7. User clicks result in SearchWidget
8. SearchWidget emits navigateToResult()
9. PDFViewer scrolls to result location
10. PDFViewer highlights found text
11. StatusBar updates search info
```

### Navigating Pages

```
1. User interacts with page navigation (spinbox, slider, thumbnail, outline)
2. Component emits pageJumpRequested() or similar signal
3. ViewWidget::goToPage() is called
4. PDFViewer displays requested page
5. PDFViewer emits pageChanged() signal
6. StatusBar updates page info
7. ToolBar updates page spinbox
8. ThumbnailListView updates highlight
9. PDFOutlineWidget updates selection
10. UIStateManager saves new page number
```

### Theme Change

```
1. User changes theme in SettingsDialog
2. SettingsDialog emits themeChanged() signal
3. MenuBar::changeEvent() is called
4. MenuBar emits themeChanged() signal (propagates)
5. All UI components connected to themeChanged() update appearance
6. UIStateManager saves new theme preference
```

---

## Compilation Dependencies

### Include Hierarchy

```
MainWindow.h
├── ApplicationController.h
├── ViewDelegate.h
└── logging/SimpleLogging.h

MenuBar.h
├── QMenuBar
├── controller/tool.hpp (ActionMap enum)
└── managers/RecentFilesManager.h

ToolBar.h
├── QToolBar
├── controller/tool.hpp
└── ui/core/ContextMenuManager.h

StatusBar.h
├── QStatusBar
├── factory/WidgetFactory.h
└── (Poppler headers)

SideBar.h
├── QWidget
├── model/ThumbnailModel.h
├── model/PDFOutlineModel.h
├── delegate/ThumbnailDelegate.h
└── ui/thumbnail/ThumbnailListView.h

RightSideBar.h
├── QWidget
├── poppler/qt6/poppler-qt6.h
└── ui/widgets/* (child panels)

ViewWidget.h
├── QWidget
├── model/DocumentModel.h
├── model/PDFOutlineModel.h
├── ui/widgets/DocumentTabWidget.h
└── ui/viewer/PDFViewer.h

ContextMenuManager.h
├── QObject
└── controller/tool.hpp

UIStateManager.h
├── QObject
├── QSettings
├── QJsonDocument
└── logging/SimpleLogging.h

UIErrorHandler.h
├── QObject
├── utils/ErrorHandling.h
└── ui/widgets/ToastNotification.h

UIResourceManager.h
├── QObject
└── logging/SimpleLogging.h

DocumentMetadataDialog.h
├── QDialog
├── poppler/qt6/poppler-qt6.h
└── (Many Qt widget headers)

SettingsDialog.h
├── QDialog
└── (Many Qt widget headers)

SearchWidget.h
├── QWidget
├── model/SearchModel.h
└── ui/core/ContextMenuManager.h

WelcomeWidget.h
├── QWidget
├── managers/RecentFilesManager.h
├── ui/managers/WelcomeScreenManager.h
└── ui/widgets/RecentFileListWidget.h

PDFViewer.h
├── QWidget
├── poppler/qt6/poppler-qt6.h
├── model/DocumentModel.h
├── model/SearchModel.h
├── ui/viewer/PDFAnimations.h
└── ui/viewer/PDFPrerenderer.h

ThumbnailListView.h
├── QListView
├── QPropertyAnimation
└── memory (for std::unique_ptr)

WelcomeScreenManager.h
├── QObject
├── QSettings
└── (Forward declarations)
```

---

## Memory Management Strategy

### Qt Parent-Child Ownership

```
MainWindow (ownership root)
├── MenuBar (parent)
├── ToolBar (parent)
├── StatusBar (parent)
├── SideBar (parent)
│   ├── ThumbnailListView (parent)
│   │   └── ThumbnailDelegate (owned by model)
│   └── PDFOutlineWidget (parent)
├── RightSideBar (parent)
│   ├── DocumentPropertiesPanel (parent)
│   ├── DebugLogPanel (parent)
│   ├── AnnotationToolbar (parent)
│   └── SearchWidget (parent)
└── ViewWidget (parent)
    ├── DocumentTabWidget (parent)
    │   └── DocumentTabBar (parent)
    └── QStackedWidget (parent)
        ├── PDFViewer instances
        └── SkeletonWidget instances
```

### Explicit Smart Pointer Management

- `SideBar` uses `std::unique_ptr<ThumbnailModel>`
- `SideBar` uses `std::unique_ptr<ThumbnailDelegate>`
- `ViewWidget` maintains `QList<PDFViewer*>` (Qt ownership)
- `ViewWidget` maintains `QList<PDFOutlineModel*>` (external ownership)

### Singleton Management

- `UIStateManager` - Instance destroyed on application exit
- `UIErrorHandler` - Instance destroyed on application exit
- `UIResourceManager` - Instance destroyed on application exit

---

## Performance Considerations

### Virtual Scrolling

- `ThumbnailListView` uses virtual scrolling to handle large documents efficiently
- Lazy loading of thumbnails
- Preloading margin configurable (default: 3 pages)

### Asynchronous Rendering

- `PDFPrerenderer` renders pages in background
- `PDFPageWidget` tracks render state (NotRendered, Rendering, Rendered, RenderError)
- DPI optimization for different screen resolutions

### Caching

- Search results cached in `SearchResultCache`
- Page text cached in `PageTextCache`
- Thumbnail pixmaps cached in QPixmapCache

### State Optimization

- `UIStateManager` supports state compression (configurable)
- State serialization optimized for size
- Batch updates for multiple state changes

---

## Testing Considerations

### Minimal Mode Support

- `StatusBar` supports minimal mode for headless testing
- All widgets safely handle nullptr in minimal configurations
- Defensive null checks in update methods

### Mockable Components

- All managers use abstract interfaces or factory injection
- Components accept injected models for testing
- Signals can be monitored for verification

### Isolated Testing

- Each component can be instantiated and tested independently
- Singleton managers provide instance() method for testing override

---

## Summary

The SAST Readium UI component system consists of:

- **Total Components:** 43+
- **File Pairs:** 43+ (header + implementation)
- **Total Files:** 86+ UI files
- **Organization:** 9 logical groups
- **Inheritance Model:** Qt standard with QWidget, QDialog, QObject
- **Communication:** Signals/slots + direct method calls
- **Memory Management:** Qt parent-child + explicit smart pointers
- **Patterns:** Singleton, Factory, Observer, State, Strategy

All components follow Qt conventions and SOLID principles for maintainability and extensibility.
