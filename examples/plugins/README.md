# SAST Readium Plugin Examples

This directory contains comprehensive example plugins demonstrating all plugin interfaces and features of the SAST Readium plugin system.

## Plugin Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                      PluginManager                               │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────────────┐ │
│  │   Discovery  │ │  Lifecycle   │ │   Dependency Resolution  │ │
│  └──────────────┘ └──────────────┘ └──────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                              │
         ┌────────────────────┼────────────────────┐
         ▼                    ▼                    ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│  PluginBase     │ │PluginHookRegistry│ │  EventBus       │
│  (Interface)    │ │ (20+ Hooks)     │ │ (Pub/Sub)       │
└─────────────────┘ └─────────────────┘ └─────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────────┐
│               Specialized Plugin Interfaces                      │
├─────────────┬─────────────┬─────────────┬─────────────┬─────────┤
│IDocProcessor│ IRenderPlugin│ISearchPlugin│ICacheStrategy│IAnnotation│
└─────────────┴─────────────┴─────────────┴─────────────┴─────────┘
```

## Example Plugins

### Core Interface Examples

| Plugin | Interface | Key Features |
|--------|-----------|--------------|
| [hello_plugin](hello_plugin/) | `PluginBase` + `IUIExtension` | UI extensions, hooks, inter-plugin communication |
| [metadata_extractor](metadata_extractor/) | `IDocumentProcessorPlugin` | Document processing, metadata export, configuration |
| [render_filter](render_filter/) | `IRenderPlugin` | Night mode, sepia, watermark overlays |
| [smart_search](smart_search/) | `ISearchPlugin` | Fuzzy search, relevance ranking, indexing |
| [cache_optimizer](cache_optimizer/) | `ICacheStrategyPlugin` | LFU eviction, persistence, optimization |
| [annotation_sync](annotation_sync/) | `IAnnotationPlugin` | Annotation import/export, cloud sync |

### Feature Examples

| Plugin | Interface | Key Features |
|--------|-----------|--------------|
| [theme_provider](theme_provider/) | `IUIExtension` | 6 built-in themes, custom themes, dynamic switching |
| [keyboard_shortcuts](keyboard_shortcuts/) | `IUIExtension` | Command registration, shortcut customization, command palette |
| [document_statistics](document_statistics/) | `IDocumentProcessorPlugin` + `IUIExtension` | Word count, reading time, statistics export |
| [export_converter](export_converter/) | `IDocumentProcessorPlugin` + `IUIExtension` | Export to TXT, HTML, Markdown |
| [bookmark_manager](bookmark_manager/) | `IUIExtension` | Bookmark CRUD, categories, persistence, sync |
| [page_navigator](page_navigator/) | `IUIExtension` | Navigation history, back/forward, quick jump |
| [tts_enhancer](tts_enhancer/) | `IUIExtension` | Text-to-Speech, voice selection, speed control |
| [reading_progress](reading_progress/) | `IUIExtension` | Progress tracking, sessions, reading statistics |

## Interface Coverage

### PluginBase (All Plugins)

- Plugin lifecycle (`onInitialize()`, `onShutdown()`)
- Metadata declaration (name, version, author, dependencies)
- Configuration management (JSON-based)
- Logging (`m_logger`)
- Service access (`eventBus()`, `serviceLocator()`)

### IUIExtension (hello_plugin)

```cpp
QList<QAction*> menuActions() const;        // Menu items
QList<QAction*> toolbarActions() const;     // Toolbar buttons
QList<QAction*> contextMenuActions() const; // Context menu
QString statusBarMessage() const;           // Status bar
QWidget* createDockWidget();                // Dock widgets
```

### IDocumentProcessorPlugin (metadata_extractor)

```cpp
QList<PluginWorkflowStage> handledStages() const;
DocumentProcessingResult processDocument(...);
QJsonObject extractMetadata(const QString& filePath);
DocumentProcessingResult exportDocument(...);
```

### IRenderPlugin (render_filter)

```cpp
RenderFilterType filterType() const;
bool shouldProcessPage(...) const;
bool applyFilter(QImage& image, ...);
void renderOverlay(QPainter* painter, ...);
bool isThreadSafe() const;
```

### ISearchPlugin (smart_search)

```cpp
QString algorithmName() const;
QList<PluginSearchResult> executeSearch(...);
QList<PluginSearchResult> postProcessResults(...);
bool buildSearchIndex(...);
void clearIndex(...);
```

### ICacheStrategyPlugin (cache_optimizer)

```cpp
CacheEvictionStrategy evictionStrategy() const;
bool shouldCache(...) const;
QString selectEvictionCandidate(...) const;
int calculatePriority(...) const;
bool persistCache(...);
QList<CacheEntryMetadata> loadCache(...);
```

### IAnnotationPlugin (annotation_sync)

```cpp
QList<AnnotationType> supportedTypes() const;
bool createAnnotation(...);
bool exportAnnotations(...);
int importAnnotations(...);
void renderAnnotation(QPainter* painter, ...);
```

## Hook Points

All plugins can register callbacks for these workflow hooks:

### Document Workflow

- `document.pre_load` - Before document loading
- `document.post_load` - After document loaded
- `document.pre_close` - Before document closes
- `document.post_close` - After document closed
- `document.metadata_extracted` - After metadata extraction

### Render Workflow

- `render.pre_page` - Before page rendering
- `render.post_page` - After page rendered
- `render.apply_filter` - When filter applied
- `render.overlay` - When overlay rendered

### Search Workflow

- `search.pre_execute` - Before search
- `search.post_execute` - After search
- `search.index_build` - During index building
- `search.results_rank` - When ranking results

### Cache Workflow

- `cache.pre_add` - Before adding to cache
- `cache.post_add` - After added to cache
- `cache.pre_evict` - Before eviction
- `cache.post_evict` - After eviction
- `cache.optimize` - During optimization

### Annotation Workflow

- `annotation.created` - After annotation created
- `annotation.updated` - After annotation updated
- `annotation.deleted` - After annotation deleted
- `annotation.render` - During annotation rendering

### Export Workflow

- `export.pre_execute` - Before export
- `export.post_execute` - After export

## Building All Examples

```bash
# From project root
cmake --preset=Release-Windows  # or Release-Unix
cmake --build --preset=Release-Windows

# Or build individual plugin
cd examples/plugins/hello_plugin
mkdir build && cd build
cmake ..
cmake --build .
```

## Installing Plugins

Copy compiled plugins to the plugins directory:

- **Windows**: `%APPDATA%/SAST/Readium/plugins/`
- **Linux**: `~/.local/share/SAST/Readium/plugins/`
- **macOS**: `~/Library/Application Support/SAST/Readium/plugins/`

## Creating Your Own Plugin

1. **Choose a template**: Start with the plugin closest to your needs
2. **Copy and rename**: Duplicate the plugin directory
3. **Update metadata**: Edit the JSON file
4. **Implement interface**: Override required methods
5. **Register hooks**: Use `PluginHookRegistry` for workflow integration
6. **Test**: Build and install to test

### Minimal Plugin Structure

```
my_plugin/
├── MyPlugin.h              # Plugin header
├── MyPlugin.cpp            # Implementation
├── my_plugin.json          # Metadata
├── CMakeLists.txt          # Build config
└── README.md               # Documentation
```

### Plugin Metadata JSON

```json
{
    "name": "My Plugin",
    "version": "1.0.0",
    "description": "Plugin description",
    "author": "Your Name",
    "dependencies": [],
    "features": ["feature1", "feature2"],
    "configuration": {
        "option1": "default_value"
    }
}
```

## Best Practices

1. **Use hooks over events** when intercepting workflow stages
2. **Declare capabilities** in metadata for extension point matching
3. **Handle configuration changes** dynamically when possible
4. **Clean up resources** in `onShutdown()`
5. **Use logging** via `m_logger` for debugging
6. **Document your plugin** with a comprehensive README

## Related Documentation

- [Plugin System Architecture](../../llmdoc/feature/plugin-system.md)
- [Plugin Development Guide](../../docs/plugin-development.md)
- [Plugin Manager Page](../../app/ui/pages/PluginManagerPage.h)
