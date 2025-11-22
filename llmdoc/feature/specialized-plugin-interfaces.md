# Specialized Plugin Interfaces

## 1. Purpose

The specialized plugin system provides five dedicated plugin interfaces that extend the core plugin architecture to handle specific workflows: document processing, rendering, search, caching, and annotations. Each interface targets a distinct domain with standardized result types, hook integration, and lifecycle management. This system enables plugins to hook into critical application stages and customize core subsystems.

## 2. Architecture

### Plugin Interfaces

```
PluginBase (Foundation)
├── IDocumentProcessorPlugin (Document processing pipeline)
├── IRenderPlugin (Rendering customization)
├── ISearchPlugin (Search algorithms and indexing)
├── ICacheStrategyPlugin (Cache eviction and storage)
└── IAnnotationPlugin (Annotation types and lifecycle)
```

### Workflow Stage Pattern

Plugins hook into document processing via `PluginWorkflowStage` enum (12 stages):

- **Document Workflow**: `PreDocumentLoad`, `PostDocumentLoad`, `PreDocumentClose`, `PostDocumentClose`
- **Rendering Workflow**: `PrePageRender`, `PostPageRender`
- **Search Workflow**: `PreSearch`, `PostSearch`
- **Cache Workflow**: `PreCache`, `PostCache`
- **Export Workflow**: `PreExport`, `PostExport`

### Result Standardization

All processing operations return `DocumentProcessingResult` with:

```cpp
struct DocumentProcessingResult {
    bool success;           // Operation success state
    QString message;        // Human-readable message
    QVariant data;          // Result payload
    QStringList warnings;   // Non-fatal issues
    QStringList errors;     // Error descriptions
};
```

## 3. Plugin Interfaces

### IDocumentProcessorPlugin

**Purpose**: Transform, analyze, and export documents at workflow stages.

**Core Methods**:

- `handledStages()` - List workflow stages this plugin handles
- `processDocument(stage, filePath, context)` - Execute processing logic
- `canProcessFile(filePath)` - Check file compatibility
- `supportedExtensions()` - Return handled file types (e.g., ".pdf", ".epub")
- `extractMetadata(filePath)` - Parse document metadata into JSON
- `exportDocument(sourcePath, targetPath, format, options)` - Convert to target format

**Code Location**: `app/plugin/SpecializedPlugins.h` (lines 86-138)

**Example Implementation**: `examples/plugins/metadata_extractor/MetadataExtractorPlugin.cpp`

### IRenderPlugin

**Purpose**: Apply filters, transformations, and overlays to rendered pages.

**Filter Types**:

- `ColorAdjustment` - Brightness, contrast, saturation
- `ImageEnhancement` - Sharpen, denoise
- `Overlay` - Watermarks, annotations
- `Transform` - Rotate, scale, crop
- `Custom` - Application-specific filters

**Core Methods**:

- `filterType()` - Return filter category
- `shouldProcessPage(documentPath, pageNumber)` - Selective page filtering
- `applyFilter(image, pageNumber, options)` - Modify page QImage in-place
- `renderOverlay(painter, rect, pageNumber, options)` - Draw on page via QPainter
- `filterPriority()` - Execution order (0-100, higher = first)
- `isThreadSafe()` - Declare if filter can run in parallel

**Code Location**: `app/plugin/SpecializedPlugins.h` (lines 160-208)

**Thread Safety**: Plugins declaring `isThreadSafe() = true` can be applied concurrently via thread pools. Default is `false` for safety.

### ISearchPlugin

**Purpose**: Provide alternative search algorithms and result ranking strategies.

**Ranking Strategies**:

- `Frequency` - Term occurrence count
- `Position` - Location in document
- `Relevance` - BM25 or TF-IDF score
- `Custom` - Plugin-defined algorithm

**Core Methods**:

- `algorithmName()` - Unique algorithm identifier
- `canHandleQuery(query, options)` - Query compatibility check
- `executeSearch(query, documentPath, options)` - Execute search algorithm
- `postProcessResults(results, query, strategy)` - Rank and filter results
- `buildSearchIndex(documentPath, options)` - Pre-compute search index
- `getIndexSize(documentPath)` - Return index memory footprint
- `clearIndex(documentPath)` - Release index resources

**Result Structure** (`PluginSearchResult`):

```cpp
struct PluginSearchResult {
    QString text;           // Matched text
    int pageNumber;         // 0-based page index
    QRect boundingRect;     // Position on page
    double relevanceScore;  // 0.0-1.0 ranking
    QVariantMap metadata;   // Custom result data
};
```

**Code Location**: `app/plugin/SpecializedPlugins.h` (lines 244-306)

### ICacheStrategyPlugin

**Purpose**: Implement custom cache eviction policies and storage backends.

**Eviction Strategies**:

- `LRU` - Least Recently Used (standard)
- `LFU` - Least Frequently Used (hot data)
- `FIFO` - First In First Out (FIFO order)
- `ARC` - Adaptive Replacement Cache (hybrid)
- `Custom` - Application-specific policy

**Core Methods**:

- `strategyName()` - Eviction algorithm name
- `evictionStrategy()` - Return strategy enum
- `shouldCache(key, size, metadata)` - Decide if item qualifies for caching
- `selectEvictionCandidate(entries, newEntrySize)` - Choose item to remove
- `calculatePriority(metadata)` - Compute cache priority (higher = keep)
- `optimizeCache(currentSize, maxSize)` - Defragment or rebalance cache
- `persistCache(cachePath, entries)` - Serialize cache to disk
- `loadCache(cachePath)` - Restore cache from disk

**Entry Metadata**:

```cpp
struct CacheEntryMetadata {
    QString key;
    qint64 size;
    QDateTime createdAt;
    QDateTime lastAccessedAt;
    int accessCount;
    int priority;
    QVariantMap customData;
};
```

**Code Location**: `app/plugin/SpecializedPlugins.h` (lines 343-407)

### IAnnotationPlugin

**Purpose**: Extend annotation types, support import/export formats, enable collaboration.

**Annotation Types**:

- `Highlight` - Text highlight
- `Note` - Sticky note
- `Underline` - Text underline
- `Strikeout` - Text strikeout
- `FreeText` - Free text annotation
- `Stamp` - Stamp annotation
- `Ink` - Drawing/freehand annotation
- `Link` - Hyperlink annotation
- `Custom` - Plugin-defined type

**Core Methods**:

- `supportedTypes()` - List handled annotation types
- `createAnnotation(data, documentPath)` - Add new annotation
- `updateAnnotation(id, data, documentPath)` - Modify existing annotation
- `deleteAnnotation(id, documentPath)` - Remove annotation
- `getAnnotationsForPage(pageNumber, documentPath)` - Query page annotations
- `exportAnnotations(documentPath, outputPath, format)` - Serialize annotations
- `importAnnotations(inputPath, documentPath, format)` - Deserialize annotations
- `renderAnnotation(painter, annotation, pageRect, zoom)` - Draw on page

**Annotation Data Structure**:

```cpp
struct AnnotationData {
    QString id;
    AnnotationType type;
    int pageNumber;
    QRect boundingRect;
    QString content;
    QColor color;
    QString author;
    QDateTime createdAt;
    QDateTime modifiedAt;
    QVariantMap customProperties;
};
```

**Code Location**: `app/plugin/SpecializedPlugins.h` (lines 456-536)

## 4. Hook Registry System

### Purpose

`PluginHookRegistry` provides centralized callback management at workflow stages. Plugins register callbacks at predefined hook points to intercept and modify behavior.

### Core API

```cpp
class PluginHookRegistry {
public:
    static PluginHookRegistry& instance();

    // Hook management
    bool registerHook(const QString& hookName, const QString& description);
    void unregisterHook(const QString& hookName);
    bool hasHook(const QString& hookName) const;

    // Callback management
    bool registerCallback(const QString& hookName,
                         const QString& pluginName,
                         PluginHookPoint::HookCallback callback);
    void unregisterCallback(const QString& hookName, const QString& pluginName);
    void unregisterAllCallbacks(const QString& pluginName);

    // Hook execution
    QVariant executeHook(const QString& hookName, const QVariantMap& context);

    // Hook state
    int getCallbackCount(const QString& hookName) const;
    void setHookEnabled(const QString& hookName, bool enabled);
    bool isHookEnabled(const QString& hookName) const;
};
```

### Standard Hooks (StandardHooks namespace)

**Document Hooks**:

- `document.pre_load` - Before document loads
- `document.post_load` - After document loads
- `document.pre_close` - Before document closes
- `document.post_close` - After document closes
- `document.metadata_extracted` - Metadata extraction complete

**Render Hooks**:

- `render.pre_page` - Before page renders
- `render.post_page` - After page renders
- `render.apply_filter` - Apply rendering filters
- `render.overlay` - Render overlays

**Search Hooks**:

- `search.pre_execute` - Before search starts
- `search.post_execute` - After search completes
- `search.index_build` - Index creation
- `search.results_rank` - Result ranking

**Cache Hooks**:

- `cache.pre_add` - Before cache insert
- `cache.post_add` - After cache insert
- `cache.pre_evict` - Before cache eviction
- `cache.post_evict` - After cache eviction
- `cache.optimize` - Cache optimization

**Annotation Hooks**:

- `annotation.created` - Annotation created
- `annotation.updated` - Annotation updated
- `annotation.deleted` - Annotation deleted
- `annotation.render` - Annotation rendering

**Export Hooks**:

- `export.pre_execute` - Before export starts
- `export.post_execute` - After export completes

### Hook Registration in PluginManager

`registerStandardHooks()` in `PluginManager` (called during plugin system initialization in `app/main.cpp` line 286) pre-registers all standard hooks:

```cpp
void PluginManager::registerStandardHooks() {
    auto& registry = PluginHookRegistry::instance();

    // Document hooks
    registry.registerHook(StandardHooks::DOCUMENT_PRE_LOAD, "Before document loads");
    registry.registerHook(StandardHooks::DOCUMENT_POST_LOAD, "After document loads");
    // ... register remaining hooks ...
}
```

### Hook Callback Signature

Callbacks are `std::function<QVariant(const QVariantMap&)>`:

```cpp
// Example: Plugin registers hook callback
auto callback = [this](const QVariantMap& context) -> QVariant {
    QString filePath = context["filePath"].toString();
    int pageNumber = context["pageNumber"].toInt();
    // Process and return result
    return QVariant::fromValue(processedData);
};

PluginHookRegistry::instance().registerCallback(
    StandardHooks::DOCUMENT_POST_LOAD,
    "MyPlugin",
    callback);
```

### Signals

Registry emits signals for lifecycle events:

- `hookRegistered(hookName)` - New hook registered
- `hookUnregistered(hookName)` - Hook unregistered
- `callbackRegistered(hookName, pluginName)` - Callback registered
- `callbackUnregistered(hookName, pluginName)` - Callback unregistered
- `hookExecuted(hookName, callbackCount)` - Hook executed

**Code Location**: `app/plugin/PluginHookRegistry.h` (lines 1-202), `app/plugin/PluginHookRegistry.cpp`

## 5. PluginManager Integration

### Specialized Plugin Retrieval

PluginManager provides type-safe accessor methods:

```cpp
QList<IDocumentProcessorPlugin*> getDocumentProcessorPlugins() const;
QList<IRenderPlugin*> getRenderPlugins() const;
QList<ISearchPlugin*> getSearchPlugins() const;
QList<ICacheStrategyPlugin*> getCacheStrategyPlugins() const;
QList<IAnnotationPlugin*> getAnnotationPlugins() const;
```

**Code Location**: `app/plugin/PluginManager.h` (lines 169-173)

### Hook Cleanup

`unregisterAllHooks(pluginName)` removes all callbacks registered by a plugin when it unloads:

```cpp
void PluginManager::unregisterAllHooks(const QString& pluginName) {
    PluginHookRegistry::instance().unregisterAllCallbacks(pluginName);
}
```

**Code Location**: `app/plugin/PluginManager.h` (line 236), `app/plugin/PluginManager.cpp`

## 6. Relevant Code Modules

- `app/plugin/SpecializedPlugins.h` - Interface definitions (86-538 lines)
- `app/plugin/PluginHookRegistry.h` - Hook registry system
- `app/plugin/PluginHookRegistry.cpp` - Hook registry implementation
- `app/plugin/PluginManager.h` - Specialized plugin retrieval (lines 169-173, 235-236)
- `app/plugin/PluginManager.cpp` - Hook registration and cleanup
- `examples/plugins/metadata_extractor/` - Example IDocumentProcessorPlugin
- `app/main.cpp` - Hook registration call (line 286)

## 7. Attention

- **Thread Safety**: IRenderPlugin methods may run in parallel; declare thread-safety via `isThreadSafe()` to enable concurrent execution
- **Hook Cleanup**: Plugins must unregister hooks in shutdown to prevent memory leaks and orphaned callbacks
- **Metadata Extraction**: IDocumentProcessorPlugin must validate file types via `canProcessFile()` before processing
- **Priority Ordering**: IRenderPlugin filters execute in descending priority order (100 before 50); set `filterPriority()` accordingly
- **Performance**: ICacheStrategyPlugin eviction decisions impact performance; optimize `selectEvictionCandidate()` for large caches
