# Complete UI Components Inventory - SAST Readium

## Overview

This document provides a comprehensive catalog of all UI components in the SAST Readium PDF reader application. The application uses Qt6 with a modular architecture separating UI components into logical groups by functionality and inheritance hierarchy.

---

## 1. Main Window Component

### MainWindow
- **Files:** `app/MainWindow.h`, `app/MainWindow.cpp`
- **Base Class:** QMainWindow
- **Purpose:** Application entry point and main window container
- **Architecture:** Acts as thin shell delegating functionality to specialized components via dependency injection
- **Key Components:** ApplicationController, ViewDelegate
- **Signals:**
  - `onInitializationCompleted()`
  - `onInitializationFailed(const QString& error)`
  - `onApplicationError(const QString& context, const QString& error)`
- **Slots:**
  - `onInitializationCompleted()`
  - `onInitializationFailed(const QString& error)`
  - `onApplicationError(const QString& context, const QString& error)`

---

## 2. Core UI Components (app/ui/core/)

### MenuBar
- **Files:** `app/ui/core/MenuBar.h`, `app/ui/core/MenuBar.cpp`
- **Base Class:** QMenuBar
- **Purpose:** Main menu bar with File, Tabs, View, Settings menus
- **Features:**
  - File operations (open, save, print, email)
  - Recent files management with intelligent path truncation
  - Tab management with keyboard shortcuts (Ctrl+Alt+1-9)
  - View controls (sidebar, fullscreen, theme, language)
  - Settings and help menus
- **Signals:**
  - `themeChanged(const QString& theme)`
  - `languageChanged(const QString& languageCode)`
  - `onExecuted(ActionMap id, QWidget* context)`
  - `openRecentFileRequested(const QString& filePath)`
  - `welcomeScreenToggleRequested()`
  - `debugPanelToggleRequested()`
  - `debugPanelClearRequested()`
  - `debugPanelExportRequested()`
- **Slots:**
  - `setRecentFilesManager(RecentFilesManager* manager)`
  - `setWelcomeScreenEnabled(bool enabled)`
  - `updateRecentFilesMenu()`
  - `onRecentFileTriggered()`
  - `onClearRecentFilesTriggered()`

### ToolBar
- **Files:** `app/ui/core/ToolBar.h`, `app/ui/core/ToolBar.cpp`
- **Base Class:** QToolBar
- **Purpose:** Enhanced toolbar with file operations, navigation, zoom, view, and tools
- **Features:**
  - Simplified mode with essential actions (CollapsibleSections disabled for performance)
  - File operations group with actions (open, save, print, email)
  - Navigation group with page spinbox, slider, and preview
  - Zoom group with slider, presets (fit width/page/height)
  - View group with sidebar toggle, fullscreen, view modes
  - Tools group (search, annotate, highlight, bookmark, snapshot, rotate)
  - Quick access bar (theme, settings, help)
  - Compact mode support
- **Signals:**
  - `actionTriggered(ActionMap action)`
  - `pageJumpRequested(int pageNumber)`
  - `zoomLevelChanged(int percentage)`
  - `viewModeChanged(const QString& modeName)`
  - `sectionExpandChanged(const QString& sectionName, bool expanded)`
- **Slots:**
  - `onPageSpinBoxChanged(int pageNumber)`
  - `onViewModeChanged()`
  - `onZoomSliderChanged(int value)`
  - `onSectionExpandChanged(bool expanded)`
- **Public Methods:**
  - `updatePageInfo(int currentPage, int totalPages)`
  - `updateZoomLevel(double zoomFactor)`
  - `updateDocumentInfo(const QString& fileName, qint64 fileSize, const QDateTime& lastModified)`
  - `setActionsEnabled(bool enabled)`
  - `setCompactMode(bool compact)`

### CollapsibleSection (ToolBar helper)
- **Base Class:** QWidget
- **Purpose:** Collapsible sections in toolbar (preserved for future use, not currently used)
- **Methods:**
  - `setContentWidget(QWidget* widget)`
  - `setExpanded(bool expanded)`
  - `isExpanded() const`
- **Signals:** `expandedChanged(bool expanded)`

### StatusBar
- **Files:** `app/ui/core/StatusBar.h`, `app/ui/core/StatusBar.cpp`
- **Base Class:** QStatusBar
- **Purpose:** Enhanced status bar with document info, progress tracking, expandable panels
- **Features:**
  - Main section with file name, page info, zoom level, clock
  - Document info panel (title, author, subject, keywords, dates, file size)
  - Statistics panel (word count, character count, page count, reading time)
  - Security panel (encryption, permissions)
  - Search results display with result counter
  - Loading progress tracking with message
  - Quick action buttons (bookmark, annotate, share, export)
  - Page input for direct navigation
  - Minimal mode for headless/testing environments
- **Signals:**
  - `pageJumpRequested(int pageNumber)`
  - `zoomLevelChangeRequested(double zoomLevel)`
  - `searchRequested(const QString& text)`
- **Slots:**
  - `onPageInputReturnPressed()`
  - `onPageInputEditingFinished()`
  - `onPageInputTextChanged(const QString& text)`
  - `onZoomInputReturnPressed()`
  - `onSearchInputReturnPressed()`
  - `updateClock()`
  - `onMessageTimerTimeout()`
- **Public Methods:**
  - `setDocumentInfo(const QString& fileName, int currentPage, int totalPages, double zoomLevel)`
  - `setDocumentMetadata(...)`
  - `setDocumentStatistics(...)`
  - `setDocumentSecurity(...)`
  - `setErrorMessage(const QString& message, int timeout)`
  - `setSuccessMessage(const QString& message, int timeout)`
  - `setWarningMessage(const QString& message, int timeout)`
  - `showMessage(const QString& message, MessagePriority priority, int timeout)`
  - `setSearchResults(int currentMatch, int totalMatches)`
  - `showLoadingProgress(const QString& message)`
  - `updateLoadingProgress(int progress)`
  - `setCompactMode(bool compact)`

### ExpandableInfoPanel (StatusBar helper)
- **Base Class:** QWidget
- **Purpose:** Expandable collapsible panels in status bar
- **Methods:**
  - `setContentWidget(QWidget* widget)`
  - `setExpanded(bool expanded, bool animated)`
  - `isExpanded() const`
- **Signals:** `expandedChanged(bool expanded)`

### SideBar
- **Files:** `app/ui/core/SideBar.h`, `app/ui/core/SideBar.cpp`
- **Base Class:** QWidget
- **Purpose:** Left sidebar with thumbnails and bookmarks tabs
- **Features:**
  - Thumbnail view with synchronized selection
  - Bookmark/outline tree view
  - Animated show/hide with state persistence
  - Configurable width (200-400px default 250px)
- **Signals:**
  - `visibilityChanged(bool visible)`
  - `widthChanged(int width)`
  - `pageClicked(int pageNumber)`
  - `pageDoubleClicked(int pageNumber)`
  - `thumbnailSizeChanged(const QSize& size)`
  - `bookmarkNavigationRequested(const QString& documentPath, int pageNumber)`
- **Slots:**
  - `show(bool animated)`
  - `hide(bool animated)`
  - `onAnimationFinished()`
- **Public Methods:**
  - `isVisible() const`
  - `setVisible(bool visible, bool animated)`
  - `toggleVisibility(bool animated)`
  - `getPreferredWidth() const`
  - `setPreferredWidth(int width)`
  - `saveState()`
  - `restoreState()`
  - `setDocument(std::shared_ptr<Poppler::Document> document)`
  - `setThumbnailSize(const QSize& size)`
  - `addBookmark(...)`
  - `removeBookmark(...)`

### RightSideBar
- **Files:** `app/ui/core/RightSideBar.h`, `app/ui/core/RightSideBar.cpp`
- **Base Class:** QWidget
- **Purpose:** Right sidebar with properties, tools, annotations, search, and debug panels
- **Features:**
  - Properties panel (document metadata)
  - Tools panel (annotations, highlights)
  - Annotation toolbar
  - Search widget
  - Debug log panel with filtering and export
  - Animated show/hide with state persistence
  - Theme-aware styling
- **Signals:**
  - `visibilityChanged(bool visible)`
  - `widthChanged(int width)`
  - `viewFullDetailsRequested(Poppler::Document* document, const QString& filePath)`
- **Slots:**
  - `show(bool animated)`
  - `hide(bool animated)`
  - `onAnimationFinished()`
  - `onViewFullDetailsRequested(...)`
- **Public Methods:**
  - `setDocument(Poppler::Document* document, const QString& filePath)`
  - `clearDocument()`
  - `getSearchWidget() const`
  - `isVisible() const`
  - `setVisible(bool visible, bool animated)`
  - `toggleVisibility(bool animated)`
  - `saveState()`
  - `restoreState()`

### ViewWidget
- **Files:** `app/ui/core/ViewWidget.h`, `app/ui/core/ViewWidget.cpp`
- **Base Class:** QWidget
- **Purpose:** Main document viewing widget with multi-tab PDF viewer support
- **Features:**
  - Multi-document tab management via DocumentTabWidget
  - PDF viewer instances for each open document
  - Document loading states with skeleton widgets and progress
  - Empty state display when no documents are open
  - Document lifecycle management (open, close, switch)
  - Page navigation and zoom controls
  - View mode management
  - Undo/redo support for zoom and scroll position
  - DocumentState preservation and restoration
- **Signals:**
  - `currentViewerPageChanged(int pageNumber, int totalPages)`
  - `currentViewerZoomChanged(double zoomFactor)`
  - `scaleChanged(double zoomFactor)`
- **Slots:**
  - `onDocumentOpened(int index, const QString& fileName)`
  - `onDocumentClosed(int index)`
  - `onCurrentDocumentChanged(int index)`
  - `onAllDocumentsClosed()`
  - `onDocumentLoadingStarted(const QString& filePath)`
  - `onDocumentLoadingProgress(int progress)`
  - `onDocumentLoadingFailed(const QString& error, const QString& filePath)`
  - `onTabCloseRequested(int index)`
  - `onTabSwitched(int index)`
  - `onTabMoved(int from, int to)`
  - `onPDFPageChanged(int pageNumber)`
  - `onPDFZoomChanged(double zoomFactor)`
  - `onRenderPageDone(const QImage& image)`
- **Public Methods:**
  - `setDocumentController(DocumentController* controller)`
  - `setDocumentModel(DocumentModel* model)`
  - `setOutlineModel(PDFOutlineModel* model)`
  - `openDocument(const QString& filePath)`
  - `closeDocument(int index)`
  - `switchToDocument(int index)`
  - `goToPage(int pageNumber)`
  - `setCurrentViewMode(int mode)`
  - `executePDFAction(ActionMap action)`
  - `hasDocuments() const`
  - `getCurrentDocumentIndex() const`
  - `getCurrentPage() const`
  - `getCurrentZoom() const`
  - `setZoom(double zoomFactor)`
  - `getScrollPosition() const`
  - `setScrollPosition(const QPoint& position)`
  - `getDocumentState(int index) const`
  - `setDocumentState(int index, const DocumentState& state)`
  - `validateDocumentIndex(int index, const QString& operation) const`
  - `hasUnsavedChanges(int index) const`

### ContextMenuManager
- **Files:** `app/ui/core/ContextMenuManager.h`, `app/ui/core/ContextMenuManager.cpp`
- **Base Class:** QObject
- **Purpose:** Centralized context menu management for all UI components
- **Features:**
  - Document content context menus with PDF-specific actions
  - UI element context menus (tabs, sidebars, toolbars)
  - Nested submenu support with proper navigation
  - Context-sensitive action enabling/disabling
  - Error handling for all context menu operations
  - On-demand menu creation with caching for performance
- **Menu Types:**
  - DocumentViewer
  - DocumentTab
  - SidebarThumbnail
  - SidebarBookmark
  - ToolbarArea
  - SearchWidget
  - StatusBar
  - RightSidebar
- **Signals:**
  - `actionTriggered(ActionMap action, const QVariantMap& context)`
  - `customActionTriggered(const QString& actionId, const QVariantMap& context)`
- **Slots:**
  - `onDocumentViewerAction()`
  - `onTabAction()`
  - `onSidebarAction()`
  - `onToolbarAction()`
  - `onSearchAction()`
- **Public Methods:**
  - `showDocumentViewerMenu(const QPoint& position, const DocumentContext& context, QWidget* parent)`
  - `showDocumentTabMenu(...)`
  - `showSidebarMenu(...)`
  - `showToolbarMenu(...)`
  - `showSearchMenu(...)`
  - `showStatusBarMenu(...)`
  - `showRightSidebarMenu(...)`
  - `updateMenuStates(const DocumentContext& documentContext)`
  - `clearMenuCache()`

### UIStateManager
- **Files:** `app/ui/core/UIStateManager.h`, `app/ui/core/UIStateManager.cpp`
- **Base Class:** QObject (Singleton)
- **Purpose:** Comprehensive UI state management system with automatic persistence
- **Features:**
  - Automatic state saving and restoration
  - Component state synchronization
  - Memory-efficient state storage
  - Error recovery for corrupted state
  - Thread-safe state operations
  - Multiple state scopes: Session, User, Global, Component
  - State priority levels: Low, Normal, High, Critical
  - Batch update operations
  - State compression and encryption (configurable)
  - State backup and restore capabilities
- **Signals:**
  - `stateChanged(const QString& key, const QVariant& value, StateScope scope)`
  - `componentStateChanged(const QString& componentId)`
  - `stateSaved(StateScope scope, int itemCount)`
  - `stateRestored(StateScope scope, int itemCount)`
  - `stateError(const QString& operation, const QString& error)`
- **Public Methods:**
  - `setState(const QString& key, const QVariant& value, StateScope scope, StatePriority priority, const QString& component)`
  - `getState(const QString& key, const QVariant& defaultValue, StateScope scope) const`
  - `registerComponent(QWidget* widget, const QString& componentId)`
  - `saveComponentState(QWidget* widget)`
  - `restoreComponentState(QWidget* widget)`
  - `saveWindowState(QMainWindow* window)`
  - `restoreWindowState(QMainWindow* window)`
  - `saveSplitterState(QSplitter* splitter, const QString& key)`
  - `restoreSplitterState(QSplitter* splitter, const QString& key)`
  - `enableAutosave(bool enabled, int intervalMs)`
  - `validateState(const QString& key, StateScope scope)`
  - `exportState(StateScope scope) const`
  - `importState(const QJsonObject& stateData, StateScope scope)`

### UIErrorHandler
- **Files:** `app/ui/core/UIErrorHandler.h`, `app/ui/core/UIErrorHandler.cpp`
- **Base Class:** QObject (Singleton)
- **Purpose:** Comprehensive UI error handling and user feedback manager
- **Features:**
  - User input validation with clear error messages
  - System error handling with appropriate user messages
  - File operation error handling
  - Unexpected error handling with logging
  - Visual feedback systems for all user interactions
  - Toast notifications for transient messages
  - Validation state visualization on widgets
  - Error recovery with recovery dialogs
  - Context-sensitive error messages
- **Feedback Types:** Success, Info, Warning, Error, Critical
- **Validation Results:** Valid, Warning, Invalid, Critical
- **Signals:**
  - `errorHandled(const QString& context, const QString& error)`
  - `validationFailed(QWidget* widget, const QString& field, const QString& error)`
  - `recoveryAttempted(const QString& component, bool success)`
  - `userFeedbackShown(QWidget* parent, const QString& message, FeedbackType type)`
- **Public Methods:**
  - `handleUserInputError(QWidget* parent, const QString& field, const QString& error, const QString& suggestion)`
  - `handleSystemError(QWidget* parent, const ErrorHandling::ErrorInfo& error)`
  - `handleFileOperationError(QWidget* parent, const QString& operation, const QString& filePath, const QString& error)`
  - `handleUnexpectedError(QWidget* parent, const QString& context, const std::exception& exception)`
  - `showFeedback(QWidget* parent, const QString& message, FeedbackType type, int duration)`
  - `validatePageNumber(int page, int totalPages)`
  - `validateZoomLevel(double zoom)`
  - `validateFilePath(const QString& path, bool mustExist)`
  - `validateSearchQuery(const QString& query)`
  - `setWidgetValidationState(QWidget* widget, ValidationResult result, const QString& tooltip)`
  - `attemptErrorRecovery(const ErrorHandling::ErrorInfo& error, const QString& component, QWidget* parent)`

### UIResourceManager
- **Files:** `app/ui/core/UIResourceManager.h`, `app/ui/core/UIResourceManager.cpp`
- **Base Class:** QObject (Singleton)
- **Purpose:** Comprehensive UI resource management and cleanup system
- **Features:**
  - Automatic resource tracking and cleanup
  - Widget lifecycle management
  - Timer management
  - Memory usage monitoring
  - Resource leak detection
  - Memory threshold enforcement
  - Pixmap and stylesheet cache management
- **Resource Types:** Widget, Timer, Animation, PixmapCache, StyleSheet, Connection, EventFilter, Other
- **Signals:**
  - `resourceRegistered(QObject* object, ResourceType type)`
  - `resourceUnregistered(QObject* object, ResourceType type)`
  - `memoryThresholdExceeded(qint64 currentUsage, qint64 threshold)`
  - `resourceLeakDetected(const QString& description)`
  - `cleanupCompleted(ResourceType type, int cleanedCount)`
- **Public Methods:**
  - `registerResource(QObject* object, ResourceType type, const QString& description, qint64 memoryUsage, bool autoCleanup)`
  - `registerWidget(QWidget* widget, const QString& description)`
  - `scheduleWidgetCleanup(QWidget* widget, int delayMs)`
  - `createManagedTimer(QObject* parent, const QString& description)`
  - `getTotalMemoryUsage() const`
  - `getResourceCount(ResourceType type) const`
  - `getResourceList(ResourceType type) const`
  - `cleanupAllResources()`
  - `optimizeMemoryUsage()`
  - `validateResources()`

### UIRecoveryManager
- **Files:** `app/ui/core/UIRecoveryManager.h`, `app/ui/core/UIRecoveryManager.cpp`
- **Base Class:** QObject
- **Purpose:** UI-specific error recovery and state restoration

### UIConsistencyManager
- **Files:** `app/ui/core/UIConsistencyManager.h`, `app/ui/core/UIConsistencyManager.cpp`
- **Base Class:** QObject
- **Purpose:** Ensures consistent UI behavior and styling across components

---

## 3. Dialog Components (app/ui/dialogs/)

### DocumentMetadataDialog
- **Files:** `app/ui/dialogs/DocumentMetadataDialog.h`, `app/ui/dialogs/DocumentMetadataDialog.cpp`
- **Base Class:** QDialog
- **Purpose:** Comprehensive PDF document metadata display dialog
- **Features:**
  - Tabbed interface with multiple metadata categories
  - Basic info tab (file name, path, size, page count, PDF version, dates)
  - Properties tab (title, author, subject, keywords, creator, producer, dates)
  - Security tab (encryption, permissions info)
  - Advanced tab with font information and PDF features
  - Copy to clipboard functionality
  - Export metadata capability
  - Dynamic theme adaptation
- **Tabs:**
  - Basic Info
  - Document Properties
  - Security Info
  - Advanced Info
- **Public Methods:**
  - `setDocument(Poppler::Document* document, const QString& filePath)`

### SettingsDialog
- **Files:** `app/ui/dialogs/SettingsDialog.h`, `app/ui/dialogs/SettingsDialog.cpp`
- **Base Class:** QDialog
- **Purpose:** Comprehensive application settings interface
- **Features:**
  - Appearance tab (theme, language selection with preview)
  - Performance tab (cache settings, rendering options, page preloading)
  - Behavior tab (default zoom, page mode, recent files, window state)
  - Advanced tab (logging, debug options, cache path)
- **Signals:**
  - `settingsApplied()`
  - `themeChanged(const QString& theme)`
  - `languageChanged(const QString& languageCode)`
- **Public Methods:**
  - `loadSettings()`
  - `saveSettings()`
  - `applySettings()`
  - `restoreDefaults()`

### DocumentComparison
- **Files:** `app/ui/dialogs/DocumentComparison.h`, `app/ui/dialogs/DocumentComparison.cpp`
- **Base Class:** QDialog
- **Purpose:** Document comparison dialog for analyzing multiple PDFs

---

## 4. Widget Components (app/ui/widgets/)

### DocumentTabWidget
- **Files:** `app/ui/widgets/DocumentTabWidget.h`, `app/ui/widgets/DocumentTabWidget.cpp`
- **Base Class:** QTabWidget
- **Purpose:** Multi-document tab management with drag-drop support
- **Features:**
  - Custom tab bar with drag-drop support
  - Tab loading state indicator
  - Context menus for tab operations
  - File path tracking per tab
- **Signals:**
  - `tabCloseRequested(int index)`
  - `tabSwitched(int index)`
  - `tabMoved(int from, int to)`
  - `allTabsClosed()`
- **Public Methods:**
  - `addDocumentTab(const QString& fileName, const QString& filePath)`
  - `removeDocumentTab(int index)`
  - `updateTabText(int index, const QString& fileName)`
  - `setCurrentTab(int index)`
  - `setTabLoadingState(int index, bool loading)`
  - `moveTab(int from, int to)`
  - `getTabFilePath(int index) const`
  - `getTabCount() const`

### DocumentTabBar (DocumentTabWidget helper)
- **Base Class:** QTabBar
- **Purpose:** Custom tab bar with drag-drop and move support

### SearchWidget
- **Files:** `app/ui/widgets/SearchWidget.h`, `app/ui/widgets/SearchWidget.cpp`
- **Base Class:** QWidget
- **Purpose:** Comprehensive search widget with options and results display
- **Features:**
  - Search input with real-time search capability
  - Navigation buttons (previous/next result)
  - Search options toggle (case-sensitive, whole words, regex, search backward)
  - Advanced options:
    - Fuzzy search with threshold control
    - Page range search
    - Search history with dropdown
  - Results display with progress bar
  - Highlight color customization
  - Search history persistence
  - Input validation and error handling
- **Signals:**
  - `searchRequested(const QString& query, const SearchOptions& options)`
  - `resultSelected(const SearchResult& result)`
  - `navigateToResult(int pageNumber, const QRectF& rect)`
  - `searchClosed()`
  - `searchCleared()`
  - `highlightColorsChanged(const QColor& normalColor, const QColor& currentColor)`
- **Public Slots:**
  - `performSearch()`
  - `performRealTimeSearch()`
  - `nextResult()`
  - `previousResult()`
  - `onResultClicked(const QModelIndex& index)`
- **Public Methods:**
  - `setDocument(Poppler::Document* document)`
  - `focusSearchInput()`
  - `clearSearch()`
  - `showSearchOptions(bool show)`
  - `getSearchModel() const`
  - `hasResults() const`
  - `getResultCount() const`
  - `setFuzzySearchEnabled(bool enabled)`
  - `setPageRangeEnabled(bool enabled)`
  - `setPageRange(int startPage, int endPage)`
  - `setHighlightColors(const QColor& normalColor, const QColor& currentColor)`
  - `validateSearchInput(const QString& query) const`

### WelcomeWidget
- **Files:** `app/ui/widgets/WelcomeWidget.h`, `app/ui/widgets/WelcomeWidget.cpp`
- **Base Class:** QWidget
- **Purpose:** Enhanced welcome screen with onboarding and productivity features
- **Features:**
  - Application branding with logo display
  - Quick action buttons (new file, open file, open folder)
  - Recent files section
  - Interactive tutorial cards
  - Daily tips and productivity suggestions
  - Keyboard shortcuts reference
  - First-time user onboarding integration
  - Fade-in animations
  - Responsive layout with scrollable content
- **Signals:**
  - `fileOpenRequested(const QString& filePath)`
  - `newFileRequested()`
  - `openFileRequested()`
  - `openFolderRequested()`
  - `tutorialRequested(const QString& tutorialId)`
  - `showSettingsRequested()`
  - `showDocumentationRequested()`
  - `startOnboardingRequested()`
- **Public Slots:**
  - `onRecentFilesChanged()`
  - `onThemeChanged()`
- **Public Methods:**
  - `setRecentFilesManager(RecentFilesManager* manager)`
  - `setWelcomeScreenManager(WelcomeScreenManager* manager)`
  - `setOnboardingManager(OnboardingManager* manager)`
  - `setCommandManager(CommandManager* manager)`
  - `applyTheme()`
  - `refreshContent()`
  - `refreshTips()`
  - `refreshShortcuts()`
  - `saveState()`
  - `loadState()`
  - `resetState()`

### DocumentPropertiesPanel
- **Files:** `app/ui/widgets/DocumentPropertiesPanel.h`, `app/ui/widgets/DocumentPropertiesPanel.cpp`
- **Base Class:** QWidget
- **Purpose:** Compact document properties panel for sidebar display
- **Features:**
  - File info section (file name, size, page count, PDF version)
  - Document info section (title, author, subject, creator)
  - Dates section (creation and modification dates)
  - "View Full Details" button to open DocumentMetadataDialog
- **Signals:**
  - `viewFullDetailsRequested(Poppler::Document* document, const QString& filePath)`
- **Public Methods:**
  - `setDocument(Poppler::Document* document, const QString& filePath)`
  - `clearProperties()`

### BookmarkWidget
- **Files:** `app/ui/widgets/BookmarkWidget.h`, `app/ui/widgets/BookmarkWidget.cpp`
- **Base Class:** QWidget
- **Purpose:** Comprehensive bookmark management widget
- **Features:**
  - Tree view for organized bookmark display
  - Search/filter capability
  - Context menu with bookmark operations
  - Bookmark list management
- **Signals:**
  - `bookmarkSelected(const Bookmark& bookmark)`
  - `navigateToBookmark(const QString& documentPath, int pageNumber)`
  - `bookmarkAdded(const Bookmark& bookmark)`
  - `bookmarkRemoved(const QString& bookmarkId)`
  - `bookmarkUpdated(const Bookmark& bookmark)`
- **Public Methods:**
  - `setCurrentDocument(const QString& documentPath)`
  - `addBookmark(const QString& documentPath, int pageNumber, const QString& title)`
  - `removeBookmark(const QString& bookmarkId)`
  - `hasBookmarkForPage(const QString& documentPath, int pageNumber) const`
  - `refreshView()`
  - `expandAll()`
  - `collapseAll()`
  - `getBookmarkModel() const`

### DebugLogPanel
- **Files:** `app/ui/widgets/DebugLogPanel.h`, `app/ui/widgets/DebugLogPanel.cpp`
- **Base Class:** QWidget
- **Purpose:** Comprehensive debug logging panel with filtering and export
- **Features:**
  - Real-time log message display in table widget
  - Filtering by log level (Debug, Info, Warning, Error)
  - Filtering by category
  - Search functionality with highlighting
  - Log statistics display
  - Export capabilities (CSV, JSON, TXT)
  - Clear log functionality
  - Configuration options
- **Public Methods:**
  - `addLogEntry(const LogEntry& entry)`
  - `clearLogs()`
  - `exportLogs(const QString& filePath)`
  - `setFilterLevel(Logger::LogLevel level)`
  - `setFilterCategory(const QString& category)`
  - `searchLogs(const QString& searchText)`

### TutorialCard
- **Files:** `app/ui/widgets/TutorialCard.h`, `app/ui/widgets/TutorialCard.cpp`
- **Base Class:** QWidget
- **Purpose:** Interactive tutorial card for onboarding

### AnnotationToolbar
- **Files:** `app/ui/widgets/AnnotationToolbar.h`, `app/ui/widgets/AnnotationToolbar.cpp`
- **Base Class:** QToolBar
- **Purpose:** Toolbar for annotation tools (highlight, underline, strikethrough, notes)

### ToastNotification
- **Files:** `app/ui/widgets/ToastNotification.h`, `app/ui/widgets/ToastNotification.cpp`
- **Base Class:** QWidget
- **Purpose:** Toast-style notification widget for transient messages

### NotificationHelper
- **Files:** `app/ui/widgets/NotificationHelper.h`, `app/ui/widgets/NotificationHelper.cpp`
- **Base Class:** QObject
- **Purpose:** Helper class for managing notification display

### RecentFileListWidget
- **Files:** `app/ui/widgets/RecentFileListWidget.h`, `app/ui/widgets/RecentFileListWidget.cpp`
- **Base Class:** QListWidget
- **Purpose:** List widget for displaying recent files with preview

### OnboardingWidget
- **Files:** `app/ui/widgets/OnboardingWidget.h`, `app/ui/widgets/OnboardingWidget.cpp`
- **Base Class:** QWidget
- **Purpose:** Onboarding tutorial sequence for first-time users

### SkeletonWidget
- **Files:** `app/ui/widgets/SkeletonWidget.h`, `app/ui/widgets/SkeletonWidget.cpp`
- **Base Class:** QWidget
- **Purpose:** Loading skeleton/placeholder widget while documents are loading

### EnhancedFocusIndicator
- **Files:** `app/ui/widgets/EnhancedFocusIndicator.h`, `app/ui/widgets/EnhancedFocusIndicator.cpp`
- **Base Class:** QWidget
- **Purpose:** Visual focus indicator for accessibility support

---

## 5. Viewer Components (app/ui/viewer/)

### PDFViewer
- **Files:** `app/ui/viewer/PDFViewer.h`, `app/ui/viewer/PDFViewer.cpp`
- **Base Class:** QWidget
- **Purpose:** Main PDF document viewer with comprehensive features
- **Features:**
  - Multi-page rendering with caching
  - Multiple view modes (single page, continuous scroll)
  - Zoom control (fixed values, fit width/height/page)
  - Page navigation with keyboard and mouse support
  - Document rotation support
  - Search integration with highlighting
  - Annotation support
  - Gesture support (pinch-zoom, pan on touch devices)
  - DPI optimization
  - Asynchronous rendering with prerenderer
  - Drag-and-drop support

### PDFPageWidget
- **Files:** `app/ui/viewer/PDFViewer.h`
- **Base Class:** QLabel
- **Purpose:** Individual PDF page rendering widget
- **Features:**
  - Asynchronous page rendering
  - Page caching with render states
  - Search result highlighting
  - DPI-aware rendering
- **States:** NotRendered, Rendering, Rendered, RenderError

### PDFPrerenderer
- **Files:** `app/ui/viewer/PDFPrerenderer.h`, `app/ui/viewer/PDFPrerenderer.cpp`
- **Base Class:** QObject
- **Purpose:** Asynchronous PDF page prerendering system

### PDFAnimations
- **Files:** `app/ui/viewer/PDFAnimations.h`, `app/ui/viewer/PDFAnimations.cpp`
- **Base Class:** QObject
- **Purpose:** Animation support for viewer transitions and effects

### PDFOutlineWidget
- **Files:** `app/ui/viewer/PDFOutlineWidget.h`, `app/ui/viewer/PDFOutlineWidget.cpp`
- **Base Class:** QTreeWidget
- **Purpose:** PDF document outline/bookmarks tree widget
- **Features:**
  - Hierarchical outline display
  - Click-to-navigate functionality
  - Search within outline
  - Expand/collapse controls
  - Current page highlighting
- **Signals:**
  - `pageNavigationRequested(int pageNumber)`
  - `itemSelectionChanged(int pageNumber)`
- **Public Methods:**
  - `setOutlineModel(PDFOutlineModel* model)`
  - `refreshOutline()`
  - `clearOutline()`
  - `highlightPageItem(int pageNumber)`
  - `expandAll()`
  - `collapseAll()`
  - `expandToLevel(int level)`
  - `searchItems(const QString& searchText)`
  - `getCurrentSelectedPage() const`

### QGraphicsPDFViewer
- **Files:** `app/ui/viewer/QGraphicsPDFViewer.h`, `app/ui/viewer/QGraphicsPDFViewer.cpp`
- **Base Class:** QGraphicsView
- **Purpose:** Graphics view-based PDF viewer (optional, with ENABLE_QGRAPHICS_PDF_SUPPORT)

### PDFViewerComponents
- **Files:** `app/ui/viewer/PDFViewerComponents.h`, `app/ui/viewer/PDFViewerComponents.cpp`
- **Base Class:** N/A (utility components)
- **Purpose:** Reusable components for PDF viewer functionality

### SplitViewManager
- **Files:** `app/ui/viewer/SplitViewManager.h`, `app/ui/viewer/SplitViewManager.cpp`
- **Base Class:** QWidget
- **Purpose:** Split view management for side-by-side document comparison

---

## 6. Thumbnail Components (app/ui/thumbnail/)

### ThumbnailListView
- **Files:** `app/ui/thumbnail/ThumbnailListView.h`, `app/ui/thumbnail/ThumbnailListView.cpp`
- **Base Class:** QListView
- **Purpose:** Chrome-style PDF thumbnail list view with virtual scrolling
- **Features:**
  - High-performance virtual scrolling
  - Intelligent lazy loading and preloading
  - Smooth scroll animations
  - Right-click context menu support
  - Keyboard navigation
  - Optional drag-drop support
  - Fade-in animations for loaded items
  - Visible range tracking for optimization
- **Signals:**
  - `pageClicked(int pageNumber)`
  - `pageDoubleClicked(int pageNumber)`
  - `pageRightClicked(int pageNumber, const QPoint& globalPos)`
  - `currentPageChanged(int pageNumber)`
  - `pageSelectionChanged(const QList<int>& selectedPages)`
  - `scrollPositionChanged(int position, int maximum)`
  - `visibleRangeChanged(int firstVisible, int lastVisible)`
- **Public Methods:**
  - `setThumbnailModel(ThumbnailModel* model)`
  - `setThumbnailDelegate(ThumbnailDelegate* delegate)`
  - `setThumbnailSize(const QSize& size)`
  - `setThumbnailSpacing(int spacing)`
  - `scrollToPage(int pageNumber, bool animated)`
  - `setCurrentPage(int pageNumber, bool animated)`
  - `selectPage(int pageNumber)`
  - `selectPages(const QList<int>& pageNumbers)`
  - `clearSelection()`
  - `setAnimationEnabled(bool enabled)`
  - `setSmoothScrolling(bool enabled)`
  - `setFadeInEnabled(bool enabled)`
  - `setPreloadMargin(int margin)`
  - `setAutoPreload(bool enabled)`
  - `setContextMenuEnabled(bool enabled)`

### ThumbnailGenerator
- **Files:** `app/ui/thumbnail/ThumbnailGenerator.h`, `app/ui/thumbnail/ThumbnailGenerator.cpp`
- **Base Class:** QObject
- **Purpose:** Asynchronous thumbnail generation for PDF pages

### ThumbnailWidget
- **Files:** `app/ui/thumbnail/ThumbnailWidget.h`, `app/ui/thumbnail/ThumbnailWidget.cpp`
- **Base Class:** QWidget
- **Purpose:** Container widget for thumbnail display

### ThumbnailContextMenu
- **Files:** `app/ui/thumbnail/ThumbnailContextMenu.h`, `app/ui/thumbnail/ThumbnailContextMenu.cpp`
- **Base Class:** QMenu
- **Purpose:** Context menu for thumbnail operations

---

## 7. Manager Components (app/ui/managers/)

### WelcomeScreenManager
- **Files:** `app/ui/managers/WelcomeScreenManager.h`, `app/ui/managers/WelcomeScreenManager.cpp`
- **Base Class:** QObject
- **Purpose:** Manages welcome screen display state and settings persistence
- **Features:**
  - Welcome screen visibility control
  - Settings persistence (enabled state, show on startup)
  - Application lifecycle integration (startup, shutdown, document changes)
  - Delayed visibility checking
- **Signals:**
  - `welcomeScreenVisibilityChanged(bool visible)`
  - `welcomeScreenEnabledChanged(bool enabled)`
  - `showWelcomeScreenRequested()`
  - `hideWelcomeScreenRequested()`
- **Public Methods:**
  - `setMainWindow(MainWindow* mainWindow)`
  - `setWelcomeWidget(WelcomeWidget* welcomeWidget)`
  - `setDocumentModel(DocumentModel* documentModel)`
  - `isWelcomeScreenEnabled() const`
  - `setWelcomeScreenEnabled(bool enabled)`
  - `shouldShowWelcomeScreen() const`
  - `showWelcomeScreen()`
  - `hideWelcomeScreen()`
  - `isWelcomeScreenVisible() const`
  - `hasOpenDocuments() const`
  - `loadSettings()`
  - `saveSettings()`
  - `resetToDefaults()`

---

## 8. Theme/Styling Components (app/ui/theme/)

### ReadingModeManager
- **Files:** `app/ui/theme/ReadingModeManager.h`, `app/ui/theme/ReadingModeManager.cpp`
- **Base Class:** QObject
- **Purpose:** Reading mode and theme management

---

## 9. Utility Components (app/ui/utils/)

### ValidationUtils
- **Files:** `app/ui/utils/ValidationUtils.h`, `app/ui/utils/ValidationUtils.cpp`
- **Base Class:** Utility class (static methods)
- **Purpose:** Common validation utilities for UI input

---

## 10. Helper Classes

### ComponentStateGuard
- **Location:** UIStateManager
- **Base Class:** Utility class (RAII)
- **Purpose:** RAII helper for automatic state saving/restoration

### StateBinding
- **Location:** UIStateManager
- **Base Class:** QObject
- **Purpose:** Automatic state synchronization between UI and state manager

### InputValidator
- **Location:** UIErrorHandler
- **Base Class:** Utility class
- **Purpose:** Input validation for common UI patterns

### ResourceGuard
- **Location:** UIResourceManager
- **Base Class:** Utility class (RAII)
- **Purpose:** Automatic resource cleanup on scope exit

### ManagedWidgetFactory
- **Location:** UIResourceManager
- **Base Class:** Factory (template class)
- **Purpose:** Memory-aware widget creation with automatic tracking

---

## Component Inheritance Hierarchy Summary

```
QObject
├── QWidget
│   ├── MainWindow (QMainWindow)
│   ├── MenuBar (QMenuBar)
│   ├── ToolBar (QToolBar)
│   │   └── CollapsibleSection
│   ├── StatusBar (QStatusBar)
│   │   └── ExpandableInfoPanel
│   ├── SideBar
│   ├── RightSideBar
│   ├── ViewWidget
│   ├── DocumentTabWidget (QTabWidget)
│   │   └── DocumentTabBar (QTabBar)
│   ├── SearchWidget
│   ├── WelcomeWidget
│   ├── DocumentPropertiesPanel
│   ├── BookmarkWidget
│   ├── DebugLogPanel
│   ├── TutorialCard
│   ├── AnnotationToolbar (QToolBar)
│   ├── ToastNotification
│   ├── NotificationHelper
│   ├── RecentFileListWidget (QListWidget)
│   ├── OnboardingWidget
│   ├── SkeletonWidget
│   ├── EnhancedFocusIndicator
│   ├── PDFViewer
│   │   └── PDFPageWidget (QLabel)
│   ├── PDFOutlineWidget (QTreeWidget)
│   ├── QGraphicsPDFViewer (QGraphicsView) [optional]
│   ├── ThumbnailListView (QListView)
│   ├── ThumbnailWidget
│   └── SplitViewManager
│
├── QDialog
│   ├── DocumentMetadataDialog
│   ├── SettingsDialog
│   └── DocumentComparison
│
└── QObject (non-widget)
    ├── ContextMenuManager
    ├── UIStateManager (Singleton)
    ├── UIErrorHandler (Singleton)
    ├── UIResourceManager (Singleton)
    ├── UIRecoveryManager
    ├── UIConsistencyManager
    ├── PDFPrerenderer
    ├── PDFAnimations
    ├── ThumbnailGenerator
    ├── ThumbnailContextMenu (QMenu)
    ├── WelcomeScreenManager
    ├── ReadingModeManager
    └── Helper classes (ComponentStateGuard, StateBinding, etc.)
```

---

## Summary Statistics

- **Total UI Components:** 43+
- **Main Window:** 1
- **Core Components:** 12
- **Dialogs:** 3
- **Widgets:** 14
- **Viewers:** 6
- **Thumbnails:** 4
- **Managers:** 1
- **Theme/Utility:** 2
- **Helper Classes:** 7+

---

## Key Design Patterns

1. **Singleton Pattern:** UIStateManager, UIErrorHandler, UIResourceManager
2. **RAII Pattern:** ComponentStateGuard, ResourceGuard
3. **Factory Pattern:** WidgetFactory, ManagedWidgetFactory
4. **Observer Pattern:** Signals/slots throughout
5. **State Pattern:** UIStateManager with multiple scopes and priorities
6. **Strategy Pattern:** Error handling with recovery strategies
