# Specialized Plugins Development Guide

This guide covers the specialized plugin types in SAST Readium that provide deep integration with the document workflow.

## Table of Contents

1. [Overview](#overview)
2. [Plugin Hook System](#plugin-hook-system)
3. [Document Processing Plugins](#document-processing-plugins)
4. [Rendering Plugins](#rendering-plugins)
5. [Search Enhancement Plugins](#search-enhancement-plugins)
6. [Cache Strategy Plugins](#cache-strategy-plugins)
7. [Annotation Plugins](#annotation-plugins)
8. [Workflow Coverage](#workflow-coverage)
9. [Best Practices](#best-practices)
10. [Examples](#examples)

## Overview

SAST Readium provides specialized plugin interfaces that allow deep integration with the application's document processing workflow. These plugins can:

- **Transform documents** during loading and export
- **Customize rendering** with filters and overlays
- **Enhance search** with custom algorithms
- **Optimize caching** with custom strategies
- **Extend annotations** with new types and formats

### Specialized Plugin Types

| Plugin Type | Interface | Purpose |
|-------------|-----------|---------|
| Document Processor | `IDocumentProcessorPlugin` | Transform, analyze, and export documents |
| Rendering | `IRenderPlugin` | Apply filters and overlays to rendered pages |
| Search Enhancement | `ISearchPlugin` | Custom search algorithms and result processing |
| Cache Strategy | `ICacheStrategyPlugin` | Custom caching algorithms and storage |
| Annotation | `IAnnotationPlugin` | Custom annotation types and formats |

## Plugin Hook System

The hook system provides a standardized way for plugins to integrate with the application workflow.

### Architecture

```
Application → Hook Point → PluginHookRegistry → Plugin Callbacks
```

**Components**:

- **PluginHookRegistry** - Central registry for all hook points
- **PluginHookPoint** - Individual hook with registered callbacks
- **StandardHooks** - Predefined hook names for common workflows

### Using the Hook Registry

```cpp
#include "plugin/PluginHookRegistry.h"

class MyPlugin : public PluginBase {
protected:
    bool onInitialize() override {
        // Get the hook registry
        auto& hookRegistry = PluginHookRegistry::instance();

        // Register a callback for a hook
        hookRegistry.registerCallback(
            StandardHooks::DOCUMENT_POST_LOAD,
            name(),
            [this](const QVariantMap& context) {
                QString filePath = context["filePath"].toString();
                // Process document
                return QVariant();
            });

        return true;
    }

    void onShutdown() override {
        // Unregister all callbacks
        auto& hookRegistry = PluginHookRegistry::instance();
        hookRegistry.unregisterAllCallbacks(name());
    }
};
```

### Standard Hook Points

**Document Workflow**:

- `document.pre_load` - Before document is loaded
- `document.post_load` - After document is loaded
- `document.pre_close` - Before document is closed
- `document.post_close` - After document is closed
- `document.metadata_extracted` - When metadata is extracted

**Rendering Workflow**:

- `render.pre_page` - Before page is rendered
- `render.post_page` - After page is rendered
- `render.apply_filter` - Apply rendering filter
- `render.overlay` - Render overlay on page

**Search Workflow**:

- `search.pre_execute` - Before search is executed
- `search.post_execute` - After search completes
- `search.index_build` - When search index is built
- `search.results_rank` - Rank search results

**Cache Workflow**:

- `cache.pre_add` - Before item is added to cache
- `cache.post_add` - After item is cached
- `cache.pre_evict` - Before cache eviction
- `cache.post_evict` - After cache eviction
- `cache.optimize` - Cache optimization requested

**Annotation Workflow**:

- `annotation.created` - Annotation created
- `annotation.updated` - Annotation updated
- `annotation.deleted` - Annotation deleted
- `annotation.render` - Render annotation

**Export Workflow**:

- `export.pre_execute` - Before export
- `export.post_execute` - After export

## Document Processing Plugins

Document processors can transform, analyze, and export documents at various workflow stages.

### Interface: `IDocumentProcessorPlugin`

```cpp
class IDocumentProcessorPlugin {
public:
    // Get workflow stages this plugin handles
    virtual QList<PluginWorkflowStage> handledStages() const = 0;

    // Process document at a workflow stage
    virtual DocumentProcessingResult processDocument(
        PluginWorkflowStage stage,
        const QString& filePath,
        const QJsonObject& context) = 0;

    // Check if plugin can process file
    virtual bool canProcessFile(const QString& filePath) const = 0;

    // Get supported file extensions
    virtual QStringList supportedExtensions() const = 0;

    // Extract metadata
    virtual QJsonObject extractMetadata(const QString& filePath) = 0;

    // Export document
    virtual DocumentProcessingResult exportDocument(
        const QString& sourcePath,
        const QString& targetPath,
        const QString& format,
        const QJsonObject& options) = 0;
};
```

### Workflow Stages

```cpp
enum class PluginWorkflowStage {
    PreDocumentLoad,      // Before document loads
    PostDocumentLoad,     // After document loads
    PreDocumentClose,     // Before document closes
    PostDocumentClose,    // After document closes
    PrePageRender,        // Before page renders
    PostPageRender,       // After page renders
    PreSearch,            // Before search executes
    PostSearch,           // After search completes
    PreCache,             // Before caching
    PostCache,            // After caching
    PreExport,            // Before export
    PostExport            // After export
};
```

### Example: Metadata Extractor Plugin

```cpp
class MetadataExtractorPlugin : public PluginBase,
                                 public IDocumentProcessorPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0"
                      FILE "metadata_extractor.json")
    Q_INTERFACES(IPluginInterface IDocumentProcessorPlugin)

public:
    QList<PluginWorkflowStage> handledStages() const override {
        return QList<PluginWorkflowStage>()
            << PluginWorkflowStage::PostDocumentLoad
            << PluginWorkflowStage::PreExport;
    }

    DocumentProcessingResult processDocument(
        PluginWorkflowStage stage,
        const QString& filePath,
        const QJsonObject& context) override {

        if (stage == PluginWorkflowStage::PostDocumentLoad) {
            // Extract metadata after document loads
            QJsonObject metadata = extractMetadata(filePath);
            return DocumentProcessingResult::createSuccess(
                "Metadata extracted",
                QVariant::fromValue(metadata));
        }

        return DocumentProcessingResult::createSuccess();
    }

    QJsonObject extractMetadata(const QString& filePath) override {
        QJsonObject metadata;
        QFileInfo info(filePath);

        metadata["fileName"] = info.fileName();
        metadata["fileSize"] = info.size();
        metadata["modified"] = info.lastModified().toString();

        // TODO: Extract PDF-specific metadata

        return metadata;
    }

    DocumentProcessingResult exportDocument(
        const QString& sourcePath,
        const QString& targetPath,
        const QString& format,
        const QJsonObject& options) override {

        if (format == "json") {
            QJsonObject metadata = extractMetadata(sourcePath);
            QFile file(targetPath);
            if (file.open(QIODevice::WriteOnly)) {
                QJsonDocument doc(metadata);
                file.write(doc.toJson());
                file.close();
                return DocumentProcessingResult::createSuccess();
            }
        }

        return DocumentProcessingResult::createFailure("Export failed");
    }
};
```

### Use Cases

1. **OCR Processing** - Extract text from scanned PDFs
2. **Format Conversion** - Convert between document formats
3. **Metadata Analysis** - Extract and analyze document metadata
4. **Document Validation** - Validate document structure and integrity
5. **Content Extraction** - Extract images, tables, or specific content

## Rendering Plugins

Rendering plugins customize how pages are displayed by applying filters and overlays.

### Interface: `IRenderPlugin`

```cpp
class IRenderPlugin {
public:
    // Get filter type
    virtual RenderFilterType filterType() const = 0;

    // Check if should process this page
    virtual bool shouldProcessPage(const QString& documentPath,
                                    int pageNumber) const = 0;

    // Apply filter to image
    virtual bool applyFilter(QImage& image, int pageNumber,
                             const QJsonObject& options) = 0;

    // Render overlay
    virtual void renderOverlay(QPainter* painter, const QRect& rect,
                               int pageNumber,
                               const QJsonObject& options) = 0;

    // Get filter priority (0-100, higher = first)
    virtual int filterPriority() const { return 50; }

    // Thread safety
    virtual bool isThreadSafe() const { return false; }
};
```

### Filter Types

```cpp
enum class RenderFilterType {
    ColorAdjustment,      // Brightness, contrast, saturation
    ImageEnhancement,     // Sharpen, denoise
    Overlay,              // Watermarks, annotations
    Transform,            // Rotate, scale, crop
    Custom                // Custom filter
};
```

### Example: Watermark Plugin

```cpp
class WatermarkPlugin : public PluginBase, public IRenderPlugin {
    Q_OBJECT
    Q_INTERFACES(IPluginInterface IRenderPlugin)

public:
    RenderFilterType filterType() const override {
        return RenderFilterType::Overlay;
    }

    bool shouldProcessPage(const QString& documentPath,
                           int pageNumber) const override {
        // Apply watermark to all pages
        return true;
    }

    bool applyFilter(QImage& image, int pageNumber,
                     const QJsonObject& options) override {
        // Not used for overlay type
        return false;
    }

    void renderOverlay(QPainter* painter, const QRect& rect,
                       int pageNumber,
                       const QJsonObject& options) override {
        QString text = options.value("text", "CONFIDENTIAL").toString();
        int opacity = options.value("opacity", 30).toInt();

        // Save painter state
        painter->save();

        // Set watermark properties
        QFont font = painter->font();
        font.setPointSize(72);
        font.setBold(true);
        painter->setFont(font);

        painter->setPen(QColor(255, 0, 0, opacity));
        painter->translate(rect.center());
        painter->rotate(-45);

        // Draw watermark
        painter->drawText(-200, 0, text);

        // Restore painter state
        painter->restore();
    }

    int filterPriority() const override {
        return 90;  // High priority - applied near the end
    }
};
```

### Use Cases

1. **Color Adjustment** - Adjust brightness, contrast, or color temperature
2. **Image Enhancement** - Sharpen or denoise page images
3. **Watermarks** - Add watermarks or stamps
4. **Night Mode** - Invert colors for dark theme
5. **Accessibility** - High contrast or color blind modes

## Search Enhancement Plugins

Search plugins provide custom algorithms, result processing, and index optimization.

### Interface: `ISearchPlugin`

```cpp
class ISearchPlugin {
public:
    // Get algorithm name
    virtual QString algorithmName() const = 0;

    // Check if can handle query
    virtual bool canHandleQuery(const QString& query,
                                const QJsonObject& options) const = 0;

    // Execute search
    virtual QList<PluginSearchResult> executeSearch(
        const QString& query,
        const QString& documentPath,
        const QJsonObject& options) = 0;

    // Post-process results
    virtual QList<PluginSearchResult> postProcessResults(
        const QList<PluginSearchResult>& results,
        const QString& query,
        SearchRankingStrategy strategy) = 0;

    // Index management
    virtual bool buildSearchIndex(const QString& documentPath,
                                   const QJsonObject& options) = 0;
    virtual qint64 getIndexSize(const QString& documentPath) const = 0;
    virtual void clearIndex(const QString& documentPath) = 0;
};
```

### Example: Fuzzy Search Plugin

```cpp
class FuzzySearchPlugin : public PluginBase, public ISearchPlugin {
    Q_OBJECT
    Q_INTERFACES(IPluginInterface ISearchPlugin)

public:
    QString algorithmName() const override {
        return "Fuzzy Search (Levenshtein)";
    }

    bool canHandleQuery(const QString& query,
                        const QJsonObject& options) const override {
        // Handle queries with fuzzy option enabled
        return options.value("fuzzy", false).toBool();
    }

    QList<PluginSearchResult> executeSearch(
        const QString& query,
        const QString& documentPath,
        const QJsonObject& options) override {

        QList<PluginSearchResult> results;
        int maxDistance = options.value("maxDistance", 2).toInt();

        // TODO: Implement fuzzy search with Levenshtein distance
        // For each word in document:
        //   Calculate edit distance to query
        //   If distance <= maxDistance, add as result

        return results;
    }

    QList<PluginSearchResult> postProcessResults(
        const QList<PluginSearchResult>& results,
        const QString& query,
        SearchRankingStrategy strategy) override {

        QList<PluginSearchResult> processed = results;

        if (strategy == SearchRankingStrategy::Relevance) {
            // Sort by relevance score
            std::sort(processed.begin(), processed.end(),
                     [](const auto& a, const auto& b) {
                         return a.relevanceScore > b.relevanceScore;
                     });
        }

        return processed;
    }
};
```

### Use Cases

1. **Fuzzy Search** - Find approximate matches
2. **Regex Search** - Pattern-based search
3. **Semantic Search** - Meaning-based search
4. **Multi-language Search** - Language-aware search
5. **Custom Ranking** - Domain-specific relevance ranking

## Cache Strategy Plugins

Cache strategy plugins provide custom caching algorithms and storage backends.

### Interface: `ICacheStrategyPlugin`

```cpp
class ICacheStrategyPlugin {
public:
    // Get strategy name
    virtual QString strategyName() const = 0;

    // Get eviction strategy
    virtual CacheEvictionStrategy evictionStrategy() const = 0;

    // Should cache this item?
    virtual bool shouldCache(const QString& key, qint64 size,
                             const QVariantMap& metadata) const = 0;

    // Select eviction candidate
    virtual QString selectEvictionCandidate(
        const QList<CacheEntryMetadata>& entries,
        qint64 newEntrySize) const = 0;

    // Calculate priority
    virtual int calculatePriority(
        const CacheEntryMetadata& metadata) const = 0;

    // Optimize cache
    virtual int optimizeCache(qint64 currentSize,
                              qint64 maxSize) = 0;

    // Persistence
    virtual bool persistCache(const QString& cachePath,
                              const QList<CacheEntryMetadata>& entries) = 0;
    virtual QList<CacheEntryMetadata> loadCache(
        const QString& cachePath) = 0;
};
```

### Example: Adaptive Replacement Cache

```cpp
class ARCStrategyPlugin : public PluginBase,
                          public ICacheStrategyPlugin {
    Q_OBJECT
    Q_INTERFACES(IPluginInterface ICacheStrategyPlugin)

public:
    QString strategyName() const override {
        return "Adaptive Replacement Cache (ARC)";
    }

    CacheEvictionStrategy evictionStrategy() const override {
        return CacheEvictionStrategy::ARC;
    }

    bool shouldCache(const QString& key, qint64 size,
                     const QVariantMap& metadata) const override {
        // Don't cache items larger than 10MB
        if (size > 10 * 1024 * 1024) {
            return false;
        }

        // Always cache frequently accessed items
        int accessCount = metadata.value("accessCount", 0).toInt();
        if (accessCount > 5) {
            return true;
        }

        return true;
    }

    QString selectEvictionCandidate(
        const QList<CacheEntryMetadata>& entries,
        qint64 newEntrySize) const override {

        // ARC maintains two LRU lists: T1 (recently used once)
        // and T2 (recently used more than once)

        // Simplified: Evict from T1 first
        for (const auto& entry : entries) {
            if (entry.accessCount == 1) {
                return entry.key;
            }
        }

        // If no T1 entries, evict from T2
        if (!entries.isEmpty()) {
            return entries.first().key;
        }

        return QString();
    }

    int calculatePriority(const CacheEntryMetadata& metadata) const override {
        // Higher priority for frequently accessed items
        int priority = metadata.accessCount * 10;

        // Boost priority for recent accesses
        qint64 secsSinceAccess =
            metadata.lastAccessedAt.secsTo(QDateTime::currentDateTime());
        if (secsSinceAccess < 60) {
            priority += 50;
        }

        return priority;
    }
};
```

### Use Cases

1. **Custom Eviction** - Domain-specific eviction policies
2. **Predictive Caching** - Pre-cache based on usage patterns
3. **Cloud Storage** - Store cache in cloud storage
4. **Database Backend** - Use database for cache persistence
5. **Compression** - Compress cache entries

## Annotation Plugins

Annotation plugins provide custom annotation types and import/export formats.

### Interface: `IAnnotationPlugin`

```cpp
class IAnnotationPlugin {
public:
    // Get supported types
    virtual QList<AnnotationType> supportedTypes() const = 0;

    // CRUD operations
    virtual bool createAnnotation(const AnnotationData& data,
                                   const QString& documentPath) = 0;
    virtual bool updateAnnotation(const QString& annotationId,
                                   const AnnotationData& data,
                                   const QString& documentPath) = 0;
    virtual bool deleteAnnotation(const QString& annotationId,
                                   const QString& documentPath) = 0;

    // Query
    virtual QList<AnnotationData> getAnnotationsForPage(
        int pageNumber, const QString& documentPath) const = 0;

    // Import/Export
    virtual bool exportAnnotations(const QString& documentPath,
                                    const QString& outputPath,
                                    const QString& format) = 0;
    virtual int importAnnotations(const QString& inputPath,
                                   const QString& documentPath,
                                   const QString& format) = 0;

    // Rendering
    virtual void renderAnnotation(QPainter* painter,
                                   const AnnotationData& annotation,
                                   const QRect& pageRect,
                                   double zoom) = 0;
};
```

### Example: Voice Note Annotation Plugin

```cpp
class VoiceNotePlugin : public PluginBase, public IAnnotationPlugin {
    Q_OBJECT
    Q_INTERFACES(IPluginInterface IAnnotationPlugin)

public:
    QList<AnnotationType> supportedTypes() const override {
        return QList<AnnotationType>() << AnnotationType::Custom;
    }

    bool createAnnotation(const AnnotationData& data,
                          const QString& documentPath) override {
        // Store voice note data
        QString audioPath = data.customProperties.value("audioPath").toString();

        // TODO: Store annotation in database or file
        m_annotations[data.id] = data;

        return true;
    }

    void renderAnnotation(QPainter* painter,
                          const AnnotationData& annotation,
                          const QRect& pageRect,
                          double zoom) override {
        // Draw voice note icon
        painter->save();

        QRect rect = annotation.boundingRect;
        painter->setPen(Qt::blue);
        painter->setBrush(QColor(173, 216, 230, 100));
        painter->drawEllipse(rect);

        // Draw microphone icon
        painter->setPen(Qt::darkBlue);
        painter->drawText(rect, Qt::AlignCenter, "🎤");

        painter->restore();
    }

    bool exportAnnotations(const QString& documentPath,
                           const QString& outputPath,
                           const QString& format) override {
        // Export voice notes to audio files + metadata
        return true;
    }
};
```

### Use Cases

1. **Voice Notes** - Audio annotations
2. **Drawing Tools** - Free-form sketching
3. **Math Equations** - LaTeX equation annotations
4. **Code Snippets** - Syntax-highlighted code blocks
5. **Collaborative Annotations** - Multi-user annotations

## Workflow Coverage

### Document Loading Workflow

```
1. User opens document
2. Hook: document.pre_load
   - Document processors can validate/prepare file
3. Document loaded into memory
4. Hook: document.post_load
   - Document processors extract metadata
   - Search plugins build index
5. Document ready for viewing
```

### Rendering Workflow

```
1. Page needs rendering
2. Hook: render.pre_page
   - Rendering plugins prepare filters
3. Page rendered to QImage
4. Hook: render.apply_filter
   - Color adjustments, enhancements
5. Hook: render.overlay
   - Watermarks, annotations
6. Hook: render.post_page
   - Final post-processing
7. Page displayed
```

### Search Workflow

```
1. User enters search query
2. Hook: search.pre_execute
   - Search plugins validate/transform query
3. Search executed
4. Hook: search.post_execute
   - Search plugins rank results
5. Results displayed
```

### Cache Workflow

```
1. Item needs caching
2. Hook: cache.pre_add
   - Cache strategies decide if should cache
3. Check cache capacity
4. Hook: cache.pre_evict (if needed)
   - Cache strategies select eviction candidate
5. Item added to cache
6. Hook: cache.post_add
   - Update statistics
```

## Best Practices

### 1. Thread Safety

```cpp
class MyRenderPlugin : public IRenderPlugin {
public:
    bool isThreadSafe() const override {
        return true;  // Only if truly thread-safe!
    }

    bool applyFilter(QImage& image, int pageNumber,
                     const QJsonObject& options) override {
        // Use thread-local or lock-free data structures
        // No shared mutable state
        return true;
    }
};
```

### 2. Error Handling

```cpp
DocumentProcessingResult processDocument(...) override {
    try {
        // Processing logic
        return DocumentProcessingResult::createSuccess();
    } catch (const std::exception& e) {
        m_logger.error("Processing failed: {}", e.what());
        return DocumentProcessingResult::createFailure(
            "Processing error",
            QStringList() << e.what());
    }
}
```

### 3. Resource Management

```cpp
void onShutdown() override {
    // Cleanup hook callbacks
    auto& hookRegistry = PluginHookRegistry::instance();
    hookRegistry.unregisterAllCallbacks(name());

    // Cleanup events
    eventBus()->unsubscribeAll(this);

    // Clear caches
    m_cache.clear();

    // Close files/connections
    m_database.close();
}
```

### 4. Performance

```cpp
// Cache expensive operations
QJsonObject extractMetadata(const QString& filePath) override {
    if (m_metadataCache.contains(filePath)) {
        return m_metadataCache[filePath];
    }

    QJsonObject metadata = extractMetadataImpl(filePath);
    m_metadataCache[filePath] = metadata;
    return metadata;
}
```

### 5. Configuration

```cpp
bool onInitialize() override {
    // Load plugin configuration
    bool enabled = m_configuration.value("enabled", true).toBool();
    int threshold = m_configuration.value("threshold", 10).toInt();

    // Apply configuration
    setEnabled(enabled);
    setThreshold(threshold);

    return true;
}
```

## Examples

See the `examples/plugins/` directory for complete plugin implementations:

- **metadata_extractor** - Document processor plugin
- **watermark** - Rendering plugin (coming soon)
- **fuzzy_search** - Search enhancement plugin (coming soon)
- **arc_cache** - Cache strategy plugin (coming soon)
- **voice_notes** - Annotation plugin (coming soon)

Each example includes:

- Complete source code
- Plugin metadata JSON
- CMakeLists.txt for building
- README with usage instructions

## See Also

- [Plugin Development Guide](development-guide.md) - Basic plugin development
- [Plugin UI Integration](ui-integration.md) - UI extension plugins
- [Plugin System Architecture](../llmdoc/feature/plugin-system.md) - Technical details
- [API Reference](../docs/api-reference.md) - Complete API documentation
