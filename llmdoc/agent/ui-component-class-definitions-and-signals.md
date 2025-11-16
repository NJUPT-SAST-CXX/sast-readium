# UI Component Class Definitions and Signal/Slot Details

## Overview

This document provides detailed class definitions, Qt inheritance information, and comprehensive signal/slot documentation for all SAST Readium UI components.

---

## Core UI Component Class Details

### 1. MenuBar

**File:** `d:\Project\sast-readium\app\ui\core\MenuBar.h`

**Class Definition:**

```cpp
class MenuBar : public QMenuBar {
    Q_OBJECT
public:
    MenuBar(QWidget* parent = nullptr);
    ~MenuBar();
};
```

**Qt Inheritance Chain:** QMenuBar -> QWidget -> QObject

**Signals:**

- `themeChanged(const QString& theme)` - Emitted when theme changes (light/dark)
- `languageChanged(const QString& languageCode)` - Emitted when language changes (en/zh)
- `onExecuted(ActionMap id, QWidget* context)` - Emitted when menu action is executed
- `openRecentFileRequested(const QString& filePath)` - Emitted when recent file selected
- `welcomeScreenToggleRequested()` - Emitted to toggle welcome screen
- `debugPanelToggleRequested()` - Emitted to toggle debug panel
- `debugPanelClearRequested()` - Emitted to clear debug logs
- `debugPanelExportRequested()` - Emitted to export debug logs

**Slots:**

- `setRecentFilesManager(RecentFilesManager* manager)` - Set manager for recent files
- `setWelcomeScreenEnabled(bool enabled)` - Enable/disable welcome screen in menu
- `updateRecentFilesMenu()` - Refresh recent files submenu
- `onRecentFileTriggered()` - Handle recent file selection
- `onClearRecentFilesTriggered()` - Handle clear recent files action

---

### 2. ToolBar

**File:** `d:\Project\sast-readium\app\ui\core\ToolBar.h`

**Class Definition:**

```cpp
class ToolBar : public QToolBar {
    Q_OBJECT
public:
    ToolBar(QWidget* parent = nullptr);
    ~ToolBar();
};

class CollapsibleSection : public QWidget {
    Q_OBJECT
public:
    CollapsibleSection(const QString& title, QWidget* parent = nullptr);
    void setContentWidget(QWidget* widget);
    void setExpanded(bool expanded);
    bool isExpanded() const { return m_expanded; }
signals:
    void expandedChanged(bool expanded);
};
```

**Qt Inheritance Chain:** QToolBar -> QWidget -> QObject; CollapsibleSection -> QWidget -> QObject

**Signals:**

- `actionTriggered(ActionMap action)` - Any toolbar action triggered
- `pageJumpRequested(int pageNumber)` - Jump to specific page
- `zoomLevelChanged(int percentage)` - Zoom level changed
- `viewModeChanged(const QString& modeName)` - View mode changed
- `sectionExpandChanged(const QString& sectionName, bool expanded)` - Collapsible section expanded/collapsed

**Slots:**

- `onPageSpinBoxChanged(int pageNumber)` - Page number spinbox changed
- `onViewModeChanged()` - View mode changed
- `onZoomSliderChanged(int value)` - Zoom slider changed
- `onSectionExpandChanged(bool expanded)` - Section expand state changed

**State Tracking Methods:**

- `void updatePageInfo(int currentPage, int totalPages)` - Update page display
- `void updateZoomLevel(double zoomFactor)` - Update zoom display
- `void updateDocumentInfo(const QString& fileName, qint64 fileSize, const QDateTime& lastModified)` - Update file info
- `void setActionsEnabled(bool enabled)` - Enable/disable all actions
- `void setCompactMode(bool compact)` - Switch to compact layout

---

### 3. StatusBar

**File:** `d:\Project\sast-readium\app\ui\core\StatusBar.h`

**Class Definition:**

```cpp
class StatusBar : public QStatusBar {
    Q_OBJECT
public:
    explicit StatusBar(QWidget* parent = nullptr, bool minimalMode = false);
    StatusBar(WidgetFactory* factory, QWidget* parent = nullptr);
    ~StatusBar();
};

class ExpandableInfoPanel : public QWidget {
    Q_OBJECT
public:
    ExpandableInfoPanel(const QString& title, QWidget* parent = nullptr);
    void setContentWidget(QWidget* widget);
    void setExpanded(bool expanded, bool animated = true);
    bool isExpanded() const { return m_expanded; }
signals:
    void expandedChanged(bool expanded);
};
```

**Qt Inheritance Chain:** QStatusBar -> QWidget -> QObject; ExpandableInfoPanel -> QWidget -> QObject

**Signals:**

- `pageJumpRequested(int pageNumber)` - Jump to page via status bar input
- `zoomLevelChangeRequested(double zoomLevel)` - Zoom change via status bar
- `searchRequested(const QString& text)` - Search via status bar

**Slots:**

- `onPageInputReturnPressed()` - Page input field enter key
- `onPageInputEditingFinished()` - Page input editing finished
- `onPageInputTextChanged(const QString& text)` - Page input text changed
- `onZoomInputReturnPressed()` - Zoom input enter key
- `onSearchInputReturnPressed()` - Search input enter key
- `updateClock()` - Update clock display
- `onMessageTimerTimeout()` - Message display timeout

**MessagePriority Enum:**

```cpp
enum class MessagePriority { Low = 0, Normal = 1, High = 2, Critical = 3 };
```

---

### 4. SideBar

**File:** `d:\Project\sast-readium\app\ui\core\SideBar.h`

**Class Definition:**

```cpp
class SideBar : public QWidget {
    Q_OBJECT
public:
    SideBar(QWidget* parent = nullptr);
    ~SideBar();
};
```

**Qt Inheritance Chain:** QWidget -> QObject

**Signals:**

- `visibilityChanged(bool visible)` - Visibility state changed
- `widthChanged(int width)` - Width changed
- `pageClicked(int pageNumber)` - Thumbnail clicked
- `pageDoubleClicked(int pageNumber)` - Thumbnail double-clicked
- `thumbnailSizeChanged(const QSize& size)` - Thumbnail size changed
- `bookmarkNavigationRequested(const QString& documentPath, int pageNumber)` - Navigate to bookmark

**Slots:**

- `show(bool animated = true)` - Show sidebar with optional animation
- `hide(bool animated = true)` - Hide sidebar with optional animation
- `onAnimationFinished()` - Animation completion handler

**State Management:**

- Min width: 200px, Max width: 400px, Default: 250px
- Animated show/hide with 300ms duration
- State persistence via QSettings

---

### 5. RightSideBar

**File:** `d:\Project\sast-readium\app\ui\core\RightSideBar.h`

**Class Definition:**

```cpp
class RightSideBar : public QWidget {
    Q_OBJECT
public:
    RightSideBar(QWidget* parent = nullptr);
    ~RightSideBar();
};
```

**Qt Inheritance Chain:** QWidget -> QObject

**Signals:**

- `visibilityChanged(bool visible)` - Visibility state changed
- `widthChanged(int width)` - Width changed
- `viewFullDetailsRequested(Poppler::Document* document, const QString& filePath)` - Open full metadata dialog

**Slots:**

- `show(bool animated = true)` - Show sidebar
- `hide(bool animated = true)` - Hide sidebar
- `onAnimationFinished()` - Animation completion
- `onViewFullDetailsRequested(Poppler::Document* document, const QString& filePath)` - Handle details request

**Tabs Contained:**

1. Properties tab (DocumentPropertiesPanel)
2. Tools tab (AnnotationToolbar)
3. Search tab (SearchWidget)
4. Debug tab (DebugLogPanel)

---

### 6. ViewWidget

**File:** `d:\Project\sast-readium\app\ui\core\ViewWidget.h`

**Class Definition:**

```cpp
class ViewWidget : public QWidget {
    Q_OBJECT
public:
    ViewWidget(QWidget* parent = nullptr);
    ~ViewWidget();

    struct DocumentState {
        int currentPage = 1;
        double zoomLevel = 1.0;
        int rotation = 0;
        QPoint scrollPosition = QPoint(0, 0);
        int viewMode = 0;
    };
};
```

**Qt Inheritance Chain:** QWidget -> QObject

**Signals:**

- `currentViewerPageChanged(int pageNumber, int totalPages)` - Current page changed
- `currentViewerZoomChanged(double zoomFactor)` - Zoom changed
- `scaleChanged(double zoomFactor)` - Scale changed (compatibility)

**Slots:**

- `onDocumentOpened(int index, const QString& fileName)` - Document opened
- `onDocumentClosed(int index)` - Document closed
- `onCurrentDocumentChanged(int index)` - Current document changed
- `onAllDocumentsClosed()` - All documents closed
- `onDocumentLoadingStarted(const QString& filePath)` - Loading started
- `onDocumentLoadingProgress(int progress)` - Loading progress (0-100)
- `onDocumentLoadingFailed(const QString& error, const QString& filePath)` - Loading error
- `onTabCloseRequested(int index)` - Tab close requested
- `onTabSwitched(int index)` - Tab switched
- `onTabMoved(int from, int to)` - Tab moved
- `onPDFPageChanged(int pageNumber)` - PDF viewer page changed
- `onPDFZoomChanged(double zoomFactor)` - PDF viewer zoom changed
- `onRenderPageDone(const QImage& image)` - Page render complete

**Key Features:**

- Manages list of PDFViewer instances (one per document)
- Manages DocumentState for each viewer
- Tracks document modifications
- Supports undo/redo for zoom and scroll position
- Loading widgets with progress bars
- Empty state display management

---

### 7. ContextMenuManager

**File:** `d:\Project\sast-readium\app\ui\core\ContextMenuManager.h`

**Class Definition:**

```cpp
class ContextMenuManager : public QObject {
    Q_OBJECT
public:
    enum class MenuType {
        DocumentViewer,
        DocumentTab,
        SidebarThumbnail,
        SidebarBookmark,
        ToolbarArea,
        SearchWidget,
        StatusBar,
        RightSidebar
    };

    struct DocumentContext {
        bool hasDocument = false;
        bool hasSelection = false;
        bool canCopy = false;
        bool canZoom = false;
        bool canRotate = false;
        int currentPage = 0;
        int totalPages = 0;
        double zoomLevel = 1.0;
        QString documentPath;
        QString selectedText;
    };

    struct UIElementContext {
        QWidget* targetWidget = nullptr;
        int elementIndex = -1;
        QString elementId;
        QVariantMap properties;
        bool isEnabled = true;
        bool isVisible = true;
    };

    explicit ContextMenuManager(QObject* parent = nullptr);
    ~ContextMenuManager();
};
```

**Qt Inheritance Chain:** QObject

**Signals:**

- `actionTriggered(ActionMap action, const QVariantMap& context)` - Standard action triggered
- `customActionTriggered(const QString& actionId, const QVariantMap& context)` - Custom action triggered

**Slots:**

- `onDocumentViewerAction()` - Document viewer menu action
- `onTabAction()` - Tab menu action
- `onSidebarAction()` - Sidebar menu action
- `onToolbarAction()` - Toolbar menu action
- `onSearchAction()` - Search menu action

**Public Show Methods:**

- `void showDocumentViewerMenu(const QPoint& position, const DocumentContext& context, QWidget* parent)`
- `void showDocumentTabMenu(const QPoint& position, int tabIndex, const UIElementContext& context, QWidget* parent)`
- `void showSidebarMenu(const QPoint& position, MenuType menuType, const UIElementContext& context, QWidget* parent)`
- `void showToolbarMenu(const QPoint& position, const UIElementContext& context, QWidget* parent)`
- `void showSearchMenu(const QPoint& position, const UIElementContext& context, QWidget* parent)`
- `void showStatusBarMenu(const QPoint& position, const UIElementContext& context, QWidget* parent)`
- `void showRightSidebarMenu(const QPoint& position, const UIElementContext& context, QWidget* parent)`

---

### 8. UIStateManager

**File:** `d:\Project\sast-readium\app\ui\core\UIStateManager.h`

**Class Definition:**

```cpp
class UIStateManager : public QObject {
    Q_OBJECT
    friend class ComponentStateGuard;
public:
    enum class StateScope {
        Session,
        User,
        Global,
        Component
    };

    enum class StatePriority {
        Low,
        Normal,
        High,
        Critical
    };

    struct StateInfo {
        QString key;
        QVariant value;
        StateScope scope;
        StatePriority priority;
        QDateTime lastModified;
        QString component;
    };

    static UIStateManager& instance();
};

class ComponentStateGuard {
public:
    explicit ComponentStateGuard(QWidget* widget, const QString& componentId = QString());
    ~ComponentStateGuard();
    void commit();
    void rollback();
};

class StateBinding : public QObject {
    Q_OBJECT
public:
    StateBinding(QWidget* widget, const QString& stateKey, const QString& property = QString(), QObject* parent = nullptr);
    void setTwoWay(bool enabled);
    void setTransform(std::function<QVariant(const QVariant&)> toWidget, std::function<QVariant(const QVariant&)> fromWidget);
};
```

**Qt Inheritance Chain:** QObject (Singleton)

**Signals:**

- `stateChanged(const QString& key, const QVariant& value, StateScope scope)` - State value changed
- `componentStateChanged(const QString& componentId)` - Component state changed
- `stateSaved(StateScope scope, int itemCount)` - State saved successfully
- `stateRestored(StateScope scope, int itemCount)` - State restored successfully
- `stateError(const QString& operation, const QString& error)` - State operation error

**Slots:**

- `onAutosaveTimer()` - Autosave timer triggered
- `onComponentDestroyed(QObject* object)` - Component destroyed

**Access Pattern:** `UIStateManager::instance()` or `UI_STATE_MANAGER` macro

---

### 9. UIErrorHandler

**File:** `d:\Project\sast-readium\app\ui\core\UIErrorHandler.h`

**Class Definition:**

```cpp
class UIErrorHandler : public QObject {
    Q_OBJECT
public:
    enum class FeedbackType {
        Success,
        Info,
        Warning,
        Error,
        Critical
    };

    enum class ValidationResult {
        Valid,
        Warning,
        Invalid,
        Critical
    };

    struct ValidationInfo {
        ValidationResult result;
        QString message;
        QString suggestion;
        bool canProceed;
    };

    static UIErrorHandler& instance();
};

class InputValidator {
public:
    static UIErrorHandler::ValidationInfo validateFilePath(const QString& path, bool mustExist = true, bool mustBeWritable = false);
    static UIErrorHandler::ValidationInfo validateRange(double value, double min, double max, const QString& fieldName);
    static UIErrorHandler::ValidationInfo validateTextInput(const QString& text, int minLength = 0, int maxLength = -1, const QString& pattern = QString());
    static UIErrorHandler::ValidationInfo validatePDFFile(const QString& filePath);
    static UIErrorHandler::ValidationInfo validatePageRange(int start, int end, int totalPages);
    static UIErrorHandler::ValidationInfo validateZoomRange(double zoom);
    static UIErrorHandler::ValidationInfo validateSearchQuery(const QString& query, bool allowEmpty = false, bool checkRegex = false);
};
```

**Qt Inheritance Chain:** QObject (Singleton)

**Signals:**

- `errorHandled(const QString& context, const QString& error)` - Error handled
- `validationFailed(QWidget* widget, const QString& field, const QString& error)` - Validation failed
- `recoveryAttempted(const QString& component, bool success)` - Recovery attempt completed
- `userFeedbackShown(QWidget* parent, const QString& message, FeedbackType type)` - Feedback shown

**Access Pattern:** `UIErrorHandler::instance()` or `UI_HANDLE_INPUT_ERROR()` macros

---

### 10. UIResourceManager

**File:** `d:\Project\sast-readium\app\ui\core\UIResourceManager.h`

**Class Definition:**

```cpp
class UIResourceManager : public QObject {
    Q_OBJECT
public:
    enum class ResourceType {
        Widget,
        Timer,
        Animation,
        PixmapCache,
        StyleSheet,
        Connection,
        EventFilter,
        Other
    };

    struct ResourceInfo {
        ResourceType type;
        QObject* object;
        QString description;
        QDateTime created;
        qint64 memoryUsage;
        bool autoCleanup;
    };

    static UIResourceManager& instance();
};

class ResourceGuard {
public:
    explicit ResourceGuard(QObject* resource, UIResourceManager::ResourceType type, const QString& description = QString());
    ~ResourceGuard();
    void release();
    QObject* get() const;
};

template <typename T, typename... Args>
class ManagedWidgetFactory {
public:
    static T* create(QWidget* parent, const QString& description, Args&&... args);
    template <typename T>
    static void scheduleDestroy(T* widget, int delayMs = 0);
};
```

**Qt Inheritance Chain:** QObject (Singleton)

**Signals:**

- `resourceRegistered(QObject* object, ResourceType type)` - Resource registered
- `resourceUnregistered(QObject* object, ResourceType type)` - Resource unregistered
- `memoryThresholdExceeded(qint64 currentUsage, qint64 threshold)` - Memory threshold exceeded
- `resourceLeakDetected(const QString& description)` - Resource leak detected
- `cleanupCompleted(ResourceType type, int cleanedCount)` - Cleanup completed

**Slots:**

- `onResourceDestroyed(QObject* object)` - Resource object destroyed
- `onCleanupTimer()` - Cleanup timer triggered
- `onMemoryPressure()` - Memory pressure detected

---

## Dialog Component Class Details

### 11. DocumentMetadataDialog

**File:** `d:\Project\sast-readium\app\ui\dialogs\DocumentMetadataDialog.h`

**Class Definition:**

```cpp
class DocumentMetadataDialog : public QDialog {
    Q_OBJECT
public:
    explicit DocumentMetadataDialog(QWidget* parent = nullptr);
    void setDocument(Poppler::Document* document, const QString& filePath);
};
```

**Qt Inheritance Chain:** QDialog -> QWidget -> QObject

**Tabs:**

1. **Basic Info Tab:** File name, path, size, page count, PDF version, creation/modification dates
2. **Document Properties Tab:** Title, author, subject, keywords, creator, producer, dates
3. **Security Tab:** Encryption status, permissions (copy, print, modify, annotate, fill forms, assemble)
4. **Advanced Tab:** Font information, linearization, forms, JavaScript, embedded files

**Slots:**

- `onThemeChanged()` - Theme changed, update appearance

**Operations:**

- Copy to clipboard for individual fields
- Export all metadata to file
- Multi-tab interface with scrollable content areas

---

### 12. SettingsDialog

**File:** `d:\Project\sast-readium\app\ui\dialogs\SettingsDialog.h`

**Class Definition:**

```cpp
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog() = default;
signals:
    void settingsApplied();
    void themeChanged(const QString& theme);
    void languageChanged(const QString& languageCode);
};
```

**Qt Inheritance Chain:** QDialog -> QWidget -> QObject

**Signals:**

- `settingsApplied()` - Settings applied
- `themeChanged(const QString& theme)` - Theme changed (light/dark)
- `languageChanged(const QString& languageCode)` - Language changed

**Tabs:**

1. **Appearance:** Theme selection, language selection with preview
2. **Performance:** Cache settings, page preloading, rendering quality
3. **Behavior:** Default zoom, default page mode, recent files count, window state, last file
4. **Advanced:** Logging level, debug panel, welcome screen, cache path, clear cache

**Validation:**

- Cache size validation
- Recent files count validation
- Cache path validation

---

## Widget Component Class Details

### 13. DocumentTabWidget

**File:** `d:\Project\sast-readium\app\ui\widgets\DocumentTabWidget.h`

**Class Definition:**

```cpp
class DocumentTabBar : public QTabBar {
    Q_OBJECT
public:
    DocumentTabBar(QWidget* parent = nullptr);
signals:
    void tabMoveRequested(int from, int to);
};

class DocumentTabWidget : public QTabWidget {
    Q_OBJECT
public:
    DocumentTabWidget(QWidget* parent = nullptr);
signals:
    void tabCloseRequested(int index);
    void tabSwitched(int index);
    void tabMoved(int from, int to);
    void allTabsClosed();
};
```

**Qt Inheritance Chain:** QTabWidget -> QWidget -> QObject; QTabBar -> QWidget -> QObject

**Signals:**

- `tabCloseRequested(int index)` - Close tab requested
- `tabSwitched(int index)` - Tab switched
- `tabMoved(int from, int to)` - Tab moved via drag
- `allTabsClosed()` - All tabs closed

**Slots:**

- `onTabCloseRequested(int index)` - Handle close request
- `onTabMoveRequested(int from, int to)` - Handle move request

**Drag-Drop Support:**

- Drag start position tracking
- Drag-in-progress state
- MIME data handling for file drops

---

### 14. SearchWidget

**File:** `d:\Project\sast-readium\app\ui\widgets\SearchWidget.h`

**Class Definition:**

```cpp
class SearchWidget : public QWidget {
    Q_OBJECT
public:
    explicit SearchWidget(QWidget* parent = nullptr);
    ~SearchWidget();
signals:
    void searchRequested(const QString& query, const SearchOptions& options);
    void resultSelected(const SearchResult& result);
    void navigateToResult(int pageNumber, const QRectF& rect);
    void searchClosed();
    void searchCleared();
    void highlightColorsChanged(const QColor& normalColor, const QColor& currentColor);
};
```

**Qt Inheritance Chain:** QWidget -> QObject

**Signals:**

- `searchRequested(const QString& query, const SearchOptions& options)` - Search initiated
- `resultSelected(const SearchResult& result)` - Result selected
- `navigateToResult(int pageNumber, const QRectF& rect)` - Navigate to result
- `searchClosed()` - Search closed
- `searchCleared()` - Search cleared
- `highlightColorsChanged(const QColor& normalColor, const QColor& currentColor)` - Colors changed

**Public Slots:**

- `performSearch()` - Perform search with current options
- `performRealTimeSearch()` - Perform real-time search as typing
- `nextResult()` - Navigate to next result
- `previousResult()` - Navigate to previous result
- `onResultClicked(const QModelIndex& index)` - Result selected

**Private Slots:**

- `onSearchTextChanged()` - Search input changed
- `onSearchStarted()` - Search started
- `onSearchFinished(int resultCount)` - Search completed
- `onSearchError(const QString& error)` - Search error
- `onCurrentResultChanged(int index)` - Current result changed
- `toggleSearchOptions()` - Toggle options panel
- `onRealTimeSearchStarted()` - Real-time search started
- `onRealTimeResultsUpdated(const QList<SearchResult>& results)` - Results updated
- `onRealTimeSearchProgress(int currentPage, int totalPages)` - Progress update
- `navigateToCurrentResult()` - Navigate to current result
- `onFuzzySearchToggled(bool enabled)` - Fuzzy search toggled
- `onPageRangeToggled(bool enabled)` - Page range toggled
- `onPageRangeChanged()` - Page range changed
- `onSearchHistorySelected(const QString& query)` - History item selected
- `onClearHistoryClicked()` - Clear history clicked
- `onHighlightColorClicked()` - Highlight color picker clicked
- `onCurrentHighlightColorClicked()` - Current result color picker clicked

---

### 15. WelcomeWidget

**File:** `d:\Project\sast-readium\app\ui\widgets\WelcomeWidget.h`

**Class Definition:**

```cpp
class WelcomeWidget : public QWidget {
    Q_OBJECT
public:
    explicit WelcomeWidget(QWidget* parent = nullptr);
    ~WelcomeWidget();
signals:
    void fileOpenRequested(const QString& filePath);
    void newFileRequested();
    void openFileRequested();
    void openFolderRequested();
    void tutorialRequested(const QString& tutorialId);
    void showSettingsRequested();
    void showDocumentationRequested();
    void startOnboardingRequested();
};
```

**Qt Inheritance Chain:** QWidget -> QObject

**Signals:**

- `fileOpenRequested(const QString& filePath)` - File open requested
- `newFileRequested()` - New file requested
- `openFileRequested()` - Open dialog requested
- `openFolderRequested()` - Open folder requested
- `tutorialRequested(const QString& tutorialId)` - Tutorial requested
- `showSettingsRequested()` - Settings dialog requested
- `showDocumentationRequested()` - Documentation requested
- `startOnboardingRequested()` - Onboarding sequence requested

**Public Slots:**

- `onRecentFilesChanged()` - Recent files updated
- `onThemeChanged()` - Theme changed

**Private Slots:**

- `onNewFileClicked()`
- `onOpenFileClicked()`
- `onOpenFolderClicked()`
- `onRecentFileClicked(const QString& filePath)`
- `onTutorialCardClicked(const QString& tutorialId)`
- `onQuickActionClicked()`
- `onShowMoreTipsClicked()`
- `onKeyboardShortcutClicked()`
- `onStartTourClicked()`
- `onFadeInFinished()`

---

### 16. DocumentPropertiesPanel

**File:** `d:\Project\sast-readium\app\ui\widgets\DocumentPropertiesPanel.h`

**Class Definition:**

```cpp
class DocumentPropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit DocumentPropertiesPanel(QWidget* parent = nullptr);
    ~DocumentPropertiesPanel() = default;
signals:
    void viewFullDetailsRequested(Poppler::Document* document, const QString& filePath);
};
```

**Qt Inheritance Chain:** QWidget -> QObject

**Signals:**

- `viewFullDetailsRequested(Poppler::Document* document, const QString& filePath)` - Open full metadata dialog

**Property Groups:**

1. **File Info:** File name, size, page count, PDF version
2. **Document Info:** Title, author, subject, creator
3. **Dates:** Creation and modification dates

---

### 17. BookmarkWidget

**File:** `d:\Project\sast-readium\app\ui\widgets\BookmarkWidget.h`

**Class Definition:**

```cpp
class BookmarkWidget : public QWidget {
    Q_OBJECT
public:
    explicit BookmarkWidget(QWidget* parent = nullptr);
    ~BookmarkWidget() = default;
signals:
    void bookmarkSelected(const Bookmark& bookmark);
    void navigateToBookmark(const QString& documentPath, int pageNumber);
    void bookmarkAdded(const Bookmark& bookmark);
    void bookmarkRemoved(const QString& bookmarkId);
    void bookmarkUpdated(const Bookmark& bookmark);
};
```

**Qt Inheritance Chain:** QWidget -> QObject

**Signals:**

- `bookmarkSelected(const Bookmark& bookmark)` - Bookmark selected
- `navigateToBookmark(const QString& documentPath, int pageNumber)` - Navigate to bookmark
- `bookmarkAdded(const Bookmark& bookmark)` - Bookmark added
- `bookmarkRemoved(const QString& bookmarkId)` - Bookmark removed
- `bookmarkUpdated(const Bookmark& bookmark)` - Bookmark updated

---

### 18. DebugLogPanel

**File:** `d:\Project\sast-readium\app\ui\widgets\DebugLogPanel.h`

**Class Definition:**

```cpp
class DebugLogPanel : public QWidget {
    Q_OBJECT
public:
    struct LogEntry {
        QDateTime timestamp;
        Logger::LogLevel level;
        QString category;
        QString message;
    };
};
```

**Qt Inheritance Chain:** QWidget -> QObject

**Features:**

- Real-time log display in table widget
- Log level filtering (Debug, Info, Warning, Error)
- Category filtering
- Search with highlighting
- Statistics display
- Export functionality (CSV, JSON, TXT)
- Clear logs capability

---

## Viewer Component Class Details

### 19. PDFViewer

**File:** `d:\Project\sast-readium\app\ui\viewer\PDFViewer.h`

**Class Definition:**

```cpp
class PDFPageWidget : public QLabel {
    Q_OBJECT
public:
    enum RenderState { NotRendered, Rendering, Rendered, RenderError };
    PDFPageWidget(QWidget* parent = nullptr);
    void setPage(Poppler::Page* page, double scaleFactor = 1.0, int rotation = 0);
    void setScaleFactor(double factor);
    void setRotation(int degrees);
    double getScaleFactor() const;
    int getRotation() const;
    void renderPage();
};

enum class PDFViewMode {
    SinglePage,
    ContinuousScroll
};

enum class ZoomType {
    FixedValue,
    FitWidth,
    FitHeight,
    FitPage
};
```

**Qt Inheritance Chain:** QWidget -> QObject; PDFPageWidget -> QLabel -> QWidget -> QObject

**Key Features:**

- Multiple view modes (single page, continuous scroll)
- Multiple zoom types (fixed, fit width/height/page)
- Page rendering with caching
- Asynchronous rendering support
- Search highlight integration
- Touch gesture support (pinch-zoom, pan, swipe)
- Keyboard and mouse navigation
- Rotation support

---

### 20. PDFOutlineWidget

**File:** `d:\Project\sast-readium\app\ui\viewer\PDFOutlineWidget.h`

**Class Definition:**

```cpp
class PDFOutlineWidget : public QTreeWidget {
    Q_OBJECT
public:
    explicit PDFOutlineWidget(QWidget* parent = nullptr);
    ~PDFOutlineWidget() = default;
signals:
    void pageNavigationRequested(int pageNumber);
    void itemSelectionChanged(int pageNumber);
};
```

**Qt Inheritance Chain:** QTreeWidget -> QWidget -> QObject

**Signals:**

- `pageNavigationRequested(int pageNumber)` - Navigate to page
- `itemSelectionChanged(int pageNumber)` - Selection changed

**Public Methods:**

- `setOutlineModel(PDFOutlineModel* model)`
- `refreshOutline()`
- `clearOutline()`
- `highlightPageItem(int pageNumber)`
- `expandAll()`
- `collapseAll()`
- `expandToLevel(int level)`
- `searchItems(const QString& searchText)`
- `getCurrentSelectedPage() const`

---

### 21. ThumbnailListView

**File:** `d:\Project\sast-readium\app\ui\thumbnail\ThumbnailListView.h`

**Class Definition:**

```cpp
class ThumbnailListView : public QListView {
    Q_OBJECT
public:
    explicit ThumbnailListView(QWidget* parent = nullptr);
    ~ThumbnailListView() override;
signals:
    void pageClicked(int pageNumber);
    void pageDoubleClicked(int pageNumber);
    void pageRightClicked(int pageNumber, const QPoint& globalPos);
    void currentPageChanged(int pageNumber);
    void pageSelectionChanged(const QList<int>& selectedPages);
    void scrollPositionChanged(int position, int maximum);
    void visibleRangeChanged(int firstVisible, int lastVisible);
};
```

**Qt Inheritance Chain:** QListView -> QWidget -> QObject

**Signals:**

- `pageClicked(int pageNumber)` - Single click
- `pageDoubleClicked(int pageNumber)` - Double click
- `pageRightClicked(int pageNumber, const QPoint& globalPos)` - Right click
- `currentPageChanged(int pageNumber)` - Current selection changed
- `pageSelectionChanged(const QList<int>& selectedPages)` - Multi-selection changed
- `scrollPositionChanged(int position, int maximum)` - Scroll position changed
- `visibleRangeChanged(int firstVisible, int lastVisible)` - Visible range changed

**Animation Settings (Constants):**

```cpp
static constexpr int DEFAULT_THUMBNAIL_WIDTH = 120;
static constexpr int DEFAULT_THUMBNAIL_HEIGHT = 160;
static constexpr int DEFAULT_SPACING = 8;
static constexpr int DEFAULT_PRELOAD_MARGIN = 3;
static constexpr int SCROLL_ANIMATION_DURATION = 300;  // ms
static constexpr int PRELOAD_TIMER_INTERVAL = 200;     // ms
static constexpr int FADE_IN_DURATION = 150;           // ms
static constexpr int FADE_IN_TIMER_INTERVAL = 50;      // ms
static constexpr int SMOOTH_SCROLL_STEP = 120;         // pixels per wheel
```

---

## Manager Component Class Details

### 22. WelcomeScreenManager

**File:** `d:\Project\sast-readium\app\ui\managers\WelcomeScreenManager.h`

**Class Definition:**

```cpp
class WelcomeScreenManager : public QObject {
    Q_OBJECT
public:
    explicit WelcomeScreenManager(QObject* parent = nullptr);
    ~WelcomeScreenManager();
signals:
    void welcomeScreenVisibilityChanged(bool visible);
    void welcomeScreenEnabledChanged(bool enabled);
    void showWelcomeScreenRequested();
    void hideWelcomeScreenRequested();
};
```

**Qt Inheritance Chain:** QObject

**Signals:**

- `welcomeScreenVisibilityChanged(bool visible)` - Visibility changed
- `welcomeScreenEnabledChanged(bool enabled)` - Enabled state changed
- `showWelcomeScreenRequested()` - Show requested
- `hideWelcomeScreenRequested()` - Hide requested

**Public Slots:**

- `onDocumentModelChanged()` - Document model changed
- `onWelcomeScreenToggleRequested()` - Toggle requested
- `checkWelcomeScreenVisibility()` - Check and update visibility

**Private Slots:**

- `onDelayedVisibilityCheck()` - Delayed visibility check (100ms)

**Settings Keys (Constants):**

```cpp
static const QString SETTINGS_GROUP;
static const QString SETTINGS_ENABLED_KEY;
static const QString SETTINGS_SHOW_ON_STARTUP_KEY;
static const bool DEFAULT_ENABLED = true;
static const bool DEFAULT_SHOW_ON_STARTUP = true;
static const int VISIBILITY_CHECK_DELAY = 100;  // ms
```

---

## Macro Convenience Functions

### UIStateManager Macros

```cpp
#define UI_STATE_MANAGER UIStateManager::instance()
#define SAVE_COMPONENT_STATE(widget) UI_STATE_MANAGER.saveComponentState(widget)
#define RESTORE_COMPONENT_STATE(widget) UI_STATE_MANAGER.restoreComponentState(widget)
#define SET_UI_STATE(key, value) UI_STATE_MANAGER.setState(key, value)
#define GET_UI_STATE(key, defaultValue) UI_STATE_MANAGER.getState(key, defaultValue)
#define REGISTER_UI_COMPONENT(widget, id) UI_STATE_MANAGER.registerComponent(widget, id)
#define STATE_GUARD(widget, id) ComponentStateGuard guard(widget, id)
```

### UIErrorHandler Macros

```cpp
#define UI_HANDLE_INPUT_ERROR(parent, field, error) \
    UIErrorHandler::instance().handleUserInputError(parent, field, error)
#define UI_HANDLE_SYSTEM_ERROR(parent, error) \
    UIErrorHandler::instance().handleSystemError(parent, error)
#define UI_HANDLE_FILE_ERROR(parent, operation, path, error) \
    UIErrorHandler::instance().handleFileOperationError(parent, operation, path, error)
#define UI_SHOW_FEEDBACK(parent, message, type) \
    UIErrorHandler::instance().showFeedback(parent, message, UIErrorHandler::FeedbackType::type)
#define UI_VALIDATE_AND_SHOW(widget, validation) /* Complex macro for validation */
```

### UIResourceManager Macros

```cpp
#define UI_RESOURCE_MANAGER UIResourceManager::instance()
#define REGISTER_UI_RESOURCE(object, type, description) \
    UI_RESOURCE_MANAGER.registerResource(object, UIResourceManager::ResourceType::type, description)
#define CREATE_MANAGED_WIDGET(Type, parent, description, ...) \
    ManagedWidgetFactory::create<Type>(parent, description, ##__VA_ARGS__)
#define RESOURCE_GUARD(object, type, description) \
    ResourceGuard guard(object, UIResourceManager::ResourceType::type, description)
#define CLEANUP_WIDGET(widget) UI_RESOURCE_MANAGER.cleanupWidget(widget)
```

---

## Summary

This document provides comprehensive class definitions and signal/slot details for the SAST Readium UI component system. All components follow Qt conventions and use:

- **Signals/Slots:** For inter-component communication
- **Inheritance:** From appropriate Qt base classes (QWidget, QDialog, QObject)
- **Singletons:** For managers (UIStateManager, UIErrorHandler, UIResourceManager)
- **RAII Patterns:** For automatic resource cleanup (ComponentStateGuard, ResourceGuard)
- **Enumerations:** For type-safe state representation

Each component is designed with separation of concerns and follows SOLID principles for maintainability and extensibility.
