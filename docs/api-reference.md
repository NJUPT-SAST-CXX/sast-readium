# SAST Readium API Reference

This document provides detailed API documentation for the core components of SAST Readium.

## Table of Contents

- [Command Pattern](#command-pattern)
- [Service Locator](#service-locator)
- [Event Bus](#event-bus)
- [Factory Pattern](#factory-pattern)
- [Models](#models)
- [Controllers](#controllers)

## Command Pattern

### CommandManager

Central command execution and history management.

**Header:** `app/command/CommandManager.h`

#### Key Methods

```cpp
class CommandManager : public QObject {
public:
    // Command execution
    bool executeCommand(const QString& commandId);
    bool executeCommand(QObject* command);
    
    // Command registration
    void registerCommand(const QString& id, CommandFactory factory);
    void registerShortcut(const QString& commandId, const QString& shortcut);
    
    // Undo/Redo
    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();
    void clearHistory();
    
    // History management
    void setHistorySize(int size);
    int historySize() const;
    QStringList commandHistory() const;
    
    // State management
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    // UI integration
    QAction* undoAction() const;
    QAction* redoAction() const;
    
signals:
    void commandExecuted(const QString& commandName, bool success);
    void historyChanged();
    void undoAvailable(bool available);
    void redoAvailable(bool available);
};
```

#### Usage Example

```cpp
// Register a command
GlobalCommandManager::registerCommand("open_document", 
    []() { return new OpenDocumentCommand(); }
);

// Register keyboard shortcut
GlobalCommandManager::registerShortcut("open_document", "Ctrl+O");

// Execute command
if (GlobalCommandManager::execute("open_document")) {
    qDebug() << "Command executed successfully";
}

// Undo/Redo
if (GlobalCommandManager::canUndo()) {
    GlobalCommandManager::undo();
}
```

### Command Base Classes

#### DocumentCommand

Base class for document-related commands.

```cpp
class DocumentCommand : public QObject {
public:
    virtual bool execute() = 0;
    virtual bool undo() = 0;
    virtual QString name() const = 0;
    virtual QString description() const;
    
signals:
    void executed(bool success);
    void undone(bool success);
};
```

#### NavigationCommand

Base class for navigation commands.

```cpp
class NavigationCommand : public QObject {
public:
    virtual bool execute() = 0;
    virtual QString name() const = 0;
    
signals:
    void executed(bool success);
};
```

### Built-in Commands

#### Document Commands

- **OpenDocumentCommand**: Open a PDF document
- **CloseDocumentCommand**: Close the current document
- **SaveDocumentCommand**: Save document changes

#### Navigation Commands

- **NextPageCommand**: Navigate to next page
- **PreviousPageCommand**: Navigate to previous page
- **GotoPageCommand**: Jump to specific page
- **FirstPageCommand**: Go to first page
- **LastPageCommand**: Go to last page

## Service Locator

### ServiceLocator

Dependency injection and service management.

**Header:** `app/controller/ServiceLocator.h`

#### Key Methods

```cpp
class ServiceLocator : public QObject {
public:
    static ServiceLocator& instance();
    
    // Service registration
    template<typename T>
    void registerService(std::function<T*()> factory);
    
    template<typename T>
    void registerServiceInstance(T* instance);
    
    // Service retrieval
    template<typename T>
    T* getService();
    
    template<typename T>
    std::shared_ptr<T> getSharedService();
    
    // Service management
    bool hasService(const QString& typeName) const;
    void unregisterService(const QString& typeName);
    void clearServices();
    
    // Configuration
    void setLazyLoading(bool enabled);
    bool isLazyLoading() const;
    
signals:
    void serviceRegistered(const QString& typeName);
    void serviceCreated(const QString& typeName);
    void serviceRequested(const QString& typeName);
};
```

#### Usage Example

```cpp
// Register a service with factory
ServiceLocator::instance().registerService<DocumentController>(
    []() { return new DocumentController(); }
);

// Register an existing instance
auto* controller = new PageController();
ServiceLocator::instance().registerServiceInstance(controller);

// Retrieve a service
auto* docController = ServiceLocator::instance().getService<DocumentController>();

// Check if service exists
if (ServiceLocator::instance().hasService("DocumentController")) {
    // Service is available
}
```

## Event Bus

### EventBus

Publish-subscribe event system for decoupled communication.

**Header:** `app/controller/EventBus.h`

#### Key Methods

```cpp
class EventBus : public QObject {
public:
    static EventBus& instance();
    
    // Subscription
    void subscribe(const QString& eventType, QObject* subscriber, EventHandler handler);
    void unsubscribe(const QString& eventType, QObject* subscriber);
    void unsubscribeAll(QObject* subscriber);
    
    // Publishing
    void publish(Event* event);
    void publish(const QString& eventType, const QVariant& data = QVariant());
    void publishAsync(Event* event, int delayMs = 0);
    void publishAsync(const QString& eventType, const QVariant& data, int delayMs = 0);
    
    // Filtering
    void addFilter(const QString& eventType, EventFilter filter);
    void removeFilter(const QString& eventType);
    
    // Configuration
    void setAsyncProcessingEnabled(bool enabled);
    bool isAsyncProcessingEnabled() const;
    void setMaxQueueSize(int size);
    
    // Statistics
    int totalEventsPublished() const;
    int queueSize() const;
    void clearStatistics();
    
signals:
    void eventPublished(const QString& eventType);
    void queueSizeChanged(int size);
};
```

#### Event Class

```cpp
class Event {
public:
    Event(const QString& type);
    
    QString type() const;
    QVariant data() const;
    void setData(const QVariant& data);
    
    QDateTime timestamp() const;
    
    void stopPropagation();
    bool isPropagationStopped() const;
};
```

#### Usage Example

```cpp
// Subscribe to events
EventBus::instance().subscribe("document_opened", this,
    [](Event* event) {
        QString filename = event->data().toString();
        qDebug() << "Document opened:" << filename;
    }
);

// Publish synchronous event
EventBus::instance().publish("document_opened", "/path/to/file.pdf");

// Publish asynchronous event with delay
EventBus::instance().publishAsync("auto_save", QVariant(), 5000); // 5 second delay

// Add event filter
EventBus::instance().addFilter("document_opened",
    [](Event* event) -> bool {
        // Only allow PDF files
        QString filename = event->data().toString();
        return filename.endsWith(".pdf");
    }
);

// Unsubscribe
EventBus::instance().unsubscribe("document_opened", this);
```

## Factory Pattern

### ModelFactory

Creates and configures model objects.

**Header:** `app/factory/ModelFactory.h`

#### Key Methods

```cpp
class ModelFactory : public QObject {
public:
    explicit ModelFactory(QObject* parent = nullptr);
    
    // Individual model creation
    RenderModel* createRenderModel(int dpiX, int dpiY);
    DocumentModel* createDocumentModel(RenderModel* renderModel);
    PageModel* createPageModel(RenderModel* renderModel);
    ThumbnailModel* createThumbnailModel(DocumentModel* documentModel);
    BookmarkModel* createBookmarkModel(DocumentModel* documentModel);
    AnnotationModel* createAnnotationModel(DocumentModel* documentModel);
    SearchModel* createSearchModel(DocumentModel* documentModel);
    PDFOutlineModel* createPDFOutlineModel(DocumentModel* documentModel);
    AsyncDocumentLoader* createAsyncDocumentLoader(DocumentModel* documentModel);
    
    // Model set creation
    struct ModelSet {
        RenderModel* renderModel = nullptr;
        DocumentModel* documentModel = nullptr;
        PageModel* pageModel = nullptr;
        ThumbnailModel* thumbnailModel = nullptr;
        SearchModel* searchModel = nullptr;
        PDFOutlineModel* outlineModel = nullptr;
    };
    
    ModelSet createViewerModelSet(int dpiX = 96, int dpiY = 96);
    
    // Custom model registration
    void registerCustomModel(const QString& typeName, ModelCreator creator);
    QObject* createCustomModel(const QString& typeName);
    
signals:
    void modelCreated(const QString& typeName, QObject* model);
    void modelSetCreated(const ModelSet& models);
    void creationError(const QString& typeName, const QString& error);
};
```

#### Usage Example

```cpp
// Create individual models
ModelFactory factory;
auto* renderModel = factory.createRenderModel(96, 96);
auto* documentModel = factory.createDocumentModel(renderModel);

// Create a complete model set
auto modelSet = factory.createViewerModelSet();
// Use modelSet.documentModel, modelSet.renderModel, etc.

// Register custom model
factory.registerCustomModel("MyCustomModel",
    [](QObject* parent) { return new MyCustomModel(parent); }
);

// Create custom model
auto* customModel = factory.createCustomModel("MyCustomModel");
```

### WidgetFactory

Creates UI widgets with proper connections.

**Header:** `app/factory/WidgetFactory.h`

#### Key Methods

```cpp
class WidgetFactory : public QObject {
public:
    explicit WidgetFactory(PageController* controller, QObject* parent = nullptr);
    
    // Button creation
    QPushButton* createButton(actionID id, const QString& text);
    
    // Action IDs
    enum class actionID {
        next,
        prev,
        first,
        last,
        goto_page
    };
};
```

#### Usage Example

```cpp
WidgetFactory factory(pageController);

// Create navigation buttons
auto* nextButton = factory.createButton(actionID::next, "Next");
auto* prevButton = factory.createButton(actionID::prev, "Previous");

// Buttons are automatically connected to appropriate commands
layout->addWidget(prevButton);
layout->addWidget(nextButton);
```

## Models

### DocumentModel

Manages PDF document state and operations.

**Header:** `app/model/DocumentModel.h`

#### Key Methods

```cpp
class DocumentModel : public QObject {
public:
    // Document management
    bool loadDocument(const QString& filePath);
    void closeDocument();
    bool isDocumentLoaded() const;
    
    // Document properties
    QString filePath() const;
    QString fileName() const;
    int pageCount() const;
    QSizeF pageSize(int pageIndex) const;
    
    // Document access
    std::shared_ptr<Poppler::Document> document() const;
    Poppler::Page* page(int index) const;
    
signals:
    void documentLoaded(const QString& filePath);
    void documentClosed();
    void documentError(const QString& error);
    void pageCountChanged(int count);
};
```

### RenderModel

Coordinates PDF page rendering.

**Header:** `app/model/RenderModel.h`

#### Key Methods

```cpp
class RenderModel : public QObject {
public:
    // Rendering
    QImage renderPage(int pageIndex, double scaleFactor = 1.0);
    void renderPageAsync(int pageIndex, double scaleFactor = 1.0);
    
    // Configuration
    void setDPI(int dpiX, int dpiY);
    void setRenderHints(Poppler::Page::RenderHints hints);
    
signals:
    void pageRendered(int pageIndex, const QImage& image);
    void renderError(int pageIndex, const QString& error);
};
```

### ThumbnailModel

Manages thumbnail generation and caching.

**Header:** `app/model/ThumbnailModel.h`

#### Key Methods

```cpp
class ThumbnailModel : public QAbstractListModel {
public:
    // Configuration
    void setDocument(std::shared_ptr<Poppler::Document> document);
    void setThumbnailSize(const QSize& size);
    void setCacheSize(int size);
    void setMemoryLimit(qint64 bytes);
    
    // Thumbnail access
    QImage thumbnail(int pageIndex) const;
    void requestThumbnail(int pageIndex);
    
    // Cache management
    void clearCache();
    int cacheHitCount() const;
    int cacheMissCount() const;
    qint64 currentMemoryUsage() const;
    
signals:
    void thumbnailReady(int pageIndex, const QImage& thumbnail);
    void cacheStatisticsChanged();
};
```

## Controllers

### ApplicationController

Application-level coordination and lifecycle management.

**Header:** `app/controller/ApplicationController.h`

#### Key Methods

```cpp
class ApplicationController : public QObject {
public:
    // Initialization
    bool initialize();
    void shutdown();
    
    // Application state
    bool isInitialized() const;
    
    // Component access
    DocumentController* documentController() const;
    PageController* pageController() const;
    ConfigurationManager* configurationManager() const;
    
signals:
    void initialized();
    void shutdownRequested();
    void initializationError(const QString& error);
};
```

### DocumentController

Document lifecycle and operations management.

**Header:** `app/controller/DocumentController.h`

#### Key Methods

```cpp
class DocumentController : public QObject {
public:
    // Document operations
    bool openDocument(const QString& filePath);
    void closeDocument();
    bool saveDocument();
    bool saveDocumentAs(const QString& filePath);
    
    // Document state
    bool hasDocument() const;
    QString currentFilePath() const;
    bool isModified() const;
    
    // Document access
    DocumentModel* documentModel() const;
    
signals:
    void documentOpened(const QString& filePath);
    void documentClosed();
    void documentSaved(const QString& filePath);
    void documentModified(bool modified);
    void operationError(const QString& error);
};
```

### PageController

Page navigation and state management.

**Header:** `app/controller/PageController.h`

#### Key Methods

```cpp
class PageController : public QObject {
public:
    // Navigation
    void nextPage();
    void previousPage();
    void gotoPage(int pageIndex);
    void firstPage();
    void lastPage();
    
    // Page state
    int currentPage() const;
    int pageCount() const;
    bool canGoNext() const;
    bool canGoPrevious() const;
    
signals:
    void currentPageChanged(int pageIndex);
    void pageCountChanged(int count);
};
```

## Related Documentation

- [Architecture Guide](architecture.md) - Comprehensive architecture overview
- [Logging System](logging-system.md) - Logging system documentation
- [Thread Safety Guidelines](thread-safety-guidelines.md) - Thread safety best practices
- [Testing Guide](../tests/README.md) - Testing infrastructure
