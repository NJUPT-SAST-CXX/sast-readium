# Plugin System Integration Guide

This document describes the comprehensive integration of the SAST Readium plugin system into the application's core functionality.

## Overview

The plugin system has been fully integrated into the following application components:

1. **DocumentController** - Document lifecycle hooks
2. **SearchEngine** - Search workflow hooks
3. **CacheManager** - Cache operation hooks
4. **AnnotationController** - Annotation event hooks
5. **PDFViewer** - Render pipeline hooks

## Hook Integration Points

### Document Workflow Hooks

Located in `app/controller/DocumentController.cpp`:

| Hook Name | Trigger Point | Context Data |
|-----------|---------------|--------------|
| `DOCUMENT_PRE_LOAD` | Before `documentModel->openFromFile()` | `filePath`, `fileName` |
| `DOCUMENT_POST_LOAD` | After successful document load | `filePath`, `fileName`, `pageCount`, `success` |
| `DOCUMENT_PRE_CLOSE` | Before `documentModel->closeDocument()` | `filePath`, `index` |
| `DOCUMENT_POST_CLOSE` | After document close | `filePath`, `index`, `success` |

**Example plugin callback:**

```cpp
PluginHookRegistry::instance().registerCallback(
    StandardHooks::DOCUMENT_POST_LOAD,
    "MyPlugin",
    [](const QVariantMap& context) -> QVariant {
        QString filePath = context.value("filePath").toString();
        int pageCount = context.value("pageCount").toInt();
        // Process loaded document
        return QVariant();
    });
```

### Search Workflow Hooks

Located in `app/search/SearchEngine.cpp`:

| Hook Name | Trigger Point | Context Data |
|-----------|---------------|--------------|
| `SEARCH_PRE_EXECUTE` | Before search starts | `query`, `caseSensitive`, `wholeWords`, `useRegex` |
| `SEARCH_POST_EXECUTE` | After search completes | `query`, `resultCount`, `cacheHit`, `incremental`, `duration` |

### Cache Operation Hooks

Located in `app/cache/CacheManager.cpp`:

| Hook Name | Trigger Point | Context Data |
|-----------|---------------|--------------|
| `CACHE_PRE_EVICT` | Before cache eviction | `cacheType`, `bytesToEvict` |
| `CACHE_POST_EVICT` | After cache eviction | `cacheType`, `bytesEvicted` |

### Annotation Event Hooks

Located in `app/controller/AnnotationController.cpp`:

| Hook Name | Trigger Point | Context Data |
|-----------|---------------|--------------|
| `ANNOTATION_CREATED` | After annotation added | `annotationId`, `type`, `pageNumber`, `author` |
| `ANNOTATION_UPDATED` | After annotation updated | `annotationId`, `type`, `pageNumber` |
| `ANNOTATION_DELETED` | After annotation removed | `annotationId` |

### Render Pipeline Hooks

Located in `app/ui/viewer/PDFViewer.cpp`:

| Hook Name | Trigger Point | Context Data |
|-----------|---------------|--------------|
| `RENDER_POST_PAGE` | After page rendered | `pageNumber`, `zoomFactor`, `rotation` |

## Specialized Plugin Interfaces

The following specialized plugin interfaces are now integrated:

### IDocumentProcessorPlugin

**Integration:** `DocumentController::openDocument()`

Plugins implementing `IDocumentProcessorPlugin` are automatically called after document loading:

```cpp
QList<IDocumentProcessorPlugin*> docProcessors =
    PluginManager::instance().getDocumentProcessorPlugins();
for (IDocumentProcessorPlugin* processor : docProcessors) {
    if (processor->canProcessFile(filePath)) {
        processor->processDocument(
            PluginWorkflowStage::PostDocumentLoad, filePath, context);
    }
}
```

### ISearchPlugin

**Integration:** `SearchEngine::executeFullSearch()`

Plugins can post-process search results:

```cpp
QList<ISearchPlugin*> searchPlugins =
    PluginManager::instance().getSearchPlugins();
for (ISearchPlugin* plugin : searchPlugins) {
    if (plugin->canHandleQuery(query, options)) {
        pluginResults = plugin->postProcessResults(
            pluginResults, query, SearchRankingStrategy::Relevance);
    }
}
```

### ICacheStrategyPlugin

**Integration:** `CacheManager::performMemoryPressureEviction()`

Plugins can suggest eviction candidates:

```cpp
QList<ICacheStrategyPlugin*> cachePlugins =
    PluginManager::instance().getCacheStrategyPlugins();
for (ICacheStrategyPlugin* plugin : cachePlugins) {
    QString evictKey = plugin->selectEvictionCandidate(entries, bytesToFree);
    // Use plugin suggestion for eviction
}
```

### IRenderPlugin

**Integration:** `PDFViewer::renderPage()`

Plugins can apply filters to rendered pages:

```cpp
QList<IRenderPlugin*> renderPlugins =
    PluginManager::instance().getRenderPlugins();
for (IRenderPlugin* plugin : renderPlugins) {
    if (plugin->shouldProcessPage(pageNumber, context)) {
        plugin->applyFilter(image, pageNumber, context);
    }
}
```

### IAnnotationPlugin

**Integration:** `AnnotationController::addAnnotation()`

Plugins are notified when annotations are created:

```cpp
QList<IAnnotationPlugin*> annotationPlugins =
    PluginManager::instance().getAnnotationPlugins();
for (IAnnotationPlugin* plugin : annotationPlugins) {
    if (plugin->supportedTypes().contains(ann.type)) {
        plugin->createAnnotation(data, m_currentFilePath);
    }
}
```

## EventBus Integration

Core application events are published to the EventBus for plugin subscription:

| Event Name | Trigger | Data |
|------------|---------|------|
| `document.opened` | After document load | `filePath` |
| `document.closed` | After document close | `filePath` |

**Subscribing to events:**

```cpp
EventBus::instance().subscribe("document.opened", this,
    [](Event* event) {
        QString filePath = event->data().toString();
        // Handle document opened
    });
```

## UI Extension Points

The following extension points are registered in `MainWindow::initPluginUIExtensions()`:

| Extension Point | Description | Plugin Interface |
|-----------------|-------------|------------------|
| `MenuExtensionPoint` | Add menu items | `IUIExtension::menuActions()` |
| `ToolbarExtensionPoint` | Add toolbar buttons | `IUIExtension::toolbarActions()` |
| `ContextMenuExtensionPoint` | Add context menu items | `IUIExtension::contextMenuActions()` |
| `StatusBarExtensionPoint` | Add status bar widgets | `IUIExtension::statusBarMessage()` |

**Note:** DockWidget extension is deferred due to ElaWindow limitations.

## Testing

Integration tests are available in `tests/plugin/test_plugin_hooks.cpp`:

```bash
# Run plugin hook tests
ctest --test-dir build -R test_plugin_hooks
```

## Best Practices

1. **Hook callbacks should be fast** - Avoid blocking operations in hook callbacks
2. **Use hooks for observation** - Prefer hooks for read-only operations
3. **Use specialized interfaces for modification** - Implement specialized interfaces when you need to modify data
4. **Clean up resources** - Always unregister callbacks in plugin shutdown
5. **Handle errors gracefully** - Wrap plugin operations in try-catch

## Files Modified

| File | Changes |
|------|---------|
| `app/controller/DocumentController.cpp` | Added document hooks + IDocumentProcessorPlugin |
| `app/search/SearchEngine.cpp` | Added search hooks + ISearchPlugin |
| `app/cache/CacheManager.cpp` | Added cache hooks + ICacheStrategyPlugin |
| `app/controller/AnnotationController.cpp` | Added annotation hooks + IAnnotationPlugin |
| `app/ui/viewer/PDFViewer.cpp` | Added render hooks + IRenderPlugin |
