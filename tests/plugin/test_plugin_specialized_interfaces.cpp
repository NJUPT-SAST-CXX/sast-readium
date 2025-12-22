#include <QImage>
#include <QJsonObject>
#include <QPainter>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "../../app/plugin/IAnnotationPlugin.h"
#include "../../app/plugin/ICacheStrategyPlugin.h"
#include "../../app/plugin/IDocumentProcessorPlugin.h"
#include "../../app/plugin/IRenderPlugin.h"
#include "../../app/plugin/ISearchPlugin.h"
#include "../../app/plugin/PluginInterface.h"
#include "../../app/plugin/PluginManager.h"
#include "../TestUtilities.h"

// ============================================================================
// Mock Implementations
// ============================================================================

/**
 * @brief Mock implementation of IDocumentProcessorPlugin
 */
class MockDocumentProcessorPlugin : public PluginBase,
                                    public IDocumentProcessorPlugin {
    Q_OBJECT
    Q_INTERFACES(IPluginInterface)

public:
    explicit MockDocumentProcessorPlugin(QObject* parent = nullptr)
        : PluginBase(parent) {
        m_metadata.name = "MockDocumentProcessor";
        m_metadata.version = "1.0.0";
        m_metadata.author = "Test";
        m_metadata.description = "Mock document processor for tests";
        m_capabilities.provides = {"document.handler"};
    }

    // IDocumentProcessorPlugin implementation
    QList<PluginWorkflowStage> handledStages() const override {
        return m_handledStages;
    }

    DocumentProcessingResult processDocument(
        PluginWorkflowStage stage, const QString& filePath,
        const QJsonObject& context) override {
        m_lastProcessedStage = stage;
        m_lastProcessedPath = filePath;
        m_lastProcessedContext = context;
        m_processDocumentCalled = true;

        if (m_shouldFail) {
            return DocumentProcessingResult::createFailure("Mock failure");
        }
        return DocumentProcessingResult::createSuccess("Processed successfully",
                                                       m_resultData);
    }

    bool canProcessFile(const QString& filePath) const override {
        for (const QString& ext : m_supportedExtensions) {
            if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
                return true;
            }
        }
        return false;
    }

    QStringList supportedExtensions() const override {
        return m_supportedExtensions;
    }

    QJsonObject extractMetadata(const QString& filePath) override {
        Q_UNUSED(filePath);
        return m_mockMetadata;
    }

    DocumentProcessingResult exportDocument(
        const QString& sourcePath, const QString& targetPath,
        const QString& format, const QJsonObject& options) override {
        m_lastExportSource = sourcePath;
        m_lastExportTarget = targetPath;
        m_lastExportFormat = format;
        m_lastExportOptions = options;
        m_exportCalled = true;

        if (m_shouldFail) {
            return DocumentProcessingResult::createFailure("Export failed");
        }
        return DocumentProcessingResult::createSuccess("Exported successfully");
    }

    // Test configuration
    void setHandledStages(const QList<PluginWorkflowStage>& stages) {
        m_handledStages = stages;
    }
    void setSupportedExtensions(const QStringList& exts) {
        m_supportedExtensions = exts;
    }
    void setMockMetadata(const QJsonObject& metadata) {
        m_mockMetadata = metadata;
    }
    void setShouldFail(bool fail) { m_shouldFail = fail; }
    void setResultData(const QVariant& data) { m_resultData = data; }

    // Test verification
    bool wasProcessDocumentCalled() const { return m_processDocumentCalled; }
    bool wasExportCalled() const { return m_exportCalled; }
    PluginWorkflowStage lastProcessedStage() const {
        return m_lastProcessedStage;
    }
    QString lastProcessedPath() const { return m_lastProcessedPath; }

protected:
    bool onInitialize() override { return true; }
    void onShutdown() override {}

private:
    QList<PluginWorkflowStage> m_handledStages;
    QStringList m_supportedExtensions = {".pdf", ".epub"};
    QJsonObject m_mockMetadata;
    bool m_shouldFail = false;
    QVariant m_resultData;

    bool m_processDocumentCalled = false;
    bool m_exportCalled = false;
    PluginWorkflowStage m_lastProcessedStage =
        PluginWorkflowStage::PreDocumentLoad;
    QString m_lastProcessedPath;
    QJsonObject m_lastProcessedContext;
    QString m_lastExportSource;
    QString m_lastExportTarget;
    QString m_lastExportFormat;
    QJsonObject m_lastExportOptions;
};

/**
 * @brief Mock implementation of IRenderPlugin
 */
class MockRenderPlugin : public PluginBase, public IRenderPlugin {
    Q_OBJECT
    Q_INTERFACES(IPluginInterface)

public:
    explicit MockRenderPlugin(QObject* parent = nullptr) : PluginBase(parent) {
        m_metadata.name = "MockRenderPlugin";
        m_metadata.version = "1.0.0";
        m_metadata.author = "Test";
        m_metadata.description = "Mock render plugin for tests";
        m_capabilities.provides = {"render.filter"};
    }

    // IRenderPlugin implementation
    RenderFilterType filterType() const override { return m_filterType; }

    bool shouldProcessPage(const QString& documentPath,
                           int pageNumber) const override {
        Q_UNUSED(documentPath);
        return m_processAllPages || m_pagesToProcess.contains(pageNumber);
    }

    bool applyFilter(QImage& image, int pageNumber,
                     const QJsonObject& options) override {
        m_filterAppliedCount++;
        m_lastFilterPage = pageNumber;
        m_lastFilterOptions = options;

        if (m_shouldFail) {
            return false;
        }

        // Apply simple filter: invert colors
        if (m_invertColors && !image.isNull()) {
            image.invertPixels();
        }
        return true;
    }

    void renderOverlay(QPainter* painter, const QRect& rect, int pageNumber,
                       const QJsonObject& options) override {
        m_overlayRenderedCount++;
        m_lastOverlayPage = pageNumber;

        if (painter && m_drawOverlay) {
            painter->setPen(Qt::red);
            painter->drawRect(rect.adjusted(5, 5, -5, -5));
        }
        Q_UNUSED(options);
    }

    int filterPriority() const override { return m_priority; }
    bool isThreadSafe() const override { return m_threadSafe; }

    // Test configuration
    void setFilterType(RenderFilterType type) { m_filterType = type; }
    void setPagesToProcess(const QList<int>& pages) {
        m_pagesToProcess = pages;
    }
    void setProcessAllPages(bool all) { m_processAllPages = all; }
    void setShouldFail(bool fail) { m_shouldFail = fail; }
    void setInvertColors(bool invert) { m_invertColors = invert; }
    void setDrawOverlay(bool draw) { m_drawOverlay = draw; }
    void setPriority(int priority) { m_priority = priority; }
    void setThreadSafe(bool safe) { m_threadSafe = safe; }

    // Test verification
    int filterAppliedCount() const { return m_filterAppliedCount; }
    int overlayRenderedCount() const { return m_overlayRenderedCount; }
    int lastFilterPage() const { return m_lastFilterPage; }

protected:
    bool onInitialize() override { return true; }
    void onShutdown() override {}

private:
    RenderFilterType m_filterType = RenderFilterType::ColorAdjustment;
    QList<int> m_pagesToProcess;
    bool m_processAllPages = true;
    bool m_shouldFail = false;
    bool m_invertColors = false;
    bool m_drawOverlay = false;
    int m_priority = 50;
    bool m_threadSafe = false;

    int m_filterAppliedCount = 0;
    int m_overlayRenderedCount = 0;
    int m_lastFilterPage = -1;
    QJsonObject m_lastFilterOptions;
    int m_lastOverlayPage = -1;
};

/**
 * @brief Mock implementation of ISearchPlugin
 */
class MockSearchPlugin : public PluginBase, public ISearchPlugin {
    Q_OBJECT
    Q_INTERFACES(IPluginInterface)

public:
    explicit MockSearchPlugin(QObject* parent = nullptr) : PluginBase(parent) {
        m_metadata.name = "MockSearchPlugin";
        m_metadata.version = "1.0.0";
        m_metadata.author = "Test";
        m_metadata.description = "Mock search plugin for tests";
        m_capabilities.provides = {"search.algorithm"};
    }

    // ISearchPlugin implementation
    QString algorithmName() const override { return m_algorithmName; }

    bool canHandleQuery(const QString& query,
                        const QJsonObject& options) const override {
        Q_UNUSED(options);
        if (m_handleAllQueries) {
            return true;
        }
        return query.length() >= m_minQueryLength;
    }

    QList<PluginSearchResult> executeSearch(
        const QString& query, const QString& documentPath,
        const QJsonObject& options) override {
        m_lastQuery = query;
        m_lastDocumentPath = documentPath;
        m_lastSearchOptions = options;
        m_searchExecutedCount++;

        return m_mockResults;
    }

    QList<PluginSearchResult> postProcessResults(
        const QList<PluginSearchResult>& results, const QString& query,
        SearchRankingStrategy strategy) override {
        m_lastRankingStrategy = strategy;
        m_postProcessCalled = true;

        if (strategy == SearchRankingStrategy::Relevance) {
            // Sort by relevance score (descending)
            QList<PluginSearchResult> sorted = results;
            std::sort(
                sorted.begin(), sorted.end(),
                [](const PluginSearchResult& a, const PluginSearchResult& b) {
                    return a.relevanceScore > b.relevanceScore;
                });
            return sorted;
        }
        Q_UNUSED(query);
        return results;
    }

    bool buildSearchIndex(const QString& documentPath,
                          const QJsonObject& options) override {
        m_indexBuiltFor = documentPath;
        m_buildIndexCalled = true;
        Q_UNUSED(options);
        return !m_shouldFail;
    }

    qint64 getIndexSize(const QString& documentPath) const override {
        Q_UNUSED(documentPath);
        return m_mockIndexSize;
    }

    void clearIndex(const QString& documentPath) override {
        m_indexClearedFor = documentPath;
        m_clearIndexCalled = true;
    }

    // Test configuration
    void setAlgorithmName(const QString& name) { m_algorithmName = name; }
    void setHandleAllQueries(bool all) { m_handleAllQueries = all; }
    void setMinQueryLength(int len) { m_minQueryLength = len; }
    void setMockResults(const QList<PluginSearchResult>& results) {
        m_mockResults = results;
    }
    void setMockIndexSize(qint64 size) { m_mockIndexSize = size; }
    void setShouldFail(bool fail) { m_shouldFail = fail; }

    // Test verification
    int searchExecutedCount() const { return m_searchExecutedCount; }
    QString lastQuery() const { return m_lastQuery; }
    bool wasBuildIndexCalled() const { return m_buildIndexCalled; }
    bool wasClearIndexCalled() const { return m_clearIndexCalled; }
    bool wasPostProcessCalled() const { return m_postProcessCalled; }

protected:
    bool onInitialize() override { return true; }
    void onShutdown() override {}

private:
    QString m_algorithmName = "MockSearch";
    bool m_handleAllQueries = true;
    int m_minQueryLength = 3;
    QList<PluginSearchResult> m_mockResults;
    qint64 m_mockIndexSize = 1024;
    bool m_shouldFail = false;

    int m_searchExecutedCount = 0;
    QString m_lastQuery;
    QString m_lastDocumentPath;
    QJsonObject m_lastSearchOptions;
    SearchRankingStrategy m_lastRankingStrategy =
        SearchRankingStrategy::Relevance;
    bool m_buildIndexCalled = false;
    bool m_clearIndexCalled = false;
    bool m_postProcessCalled = false;
    QString m_indexBuiltFor;
    QString m_indexClearedFor;
};

/**
 * @brief Mock implementation of ICacheStrategyPlugin
 */
class MockCacheStrategyPlugin : public PluginBase, public ICacheStrategyPlugin {
    Q_OBJECT
    Q_INTERFACES(IPluginInterface)

public:
    explicit MockCacheStrategyPlugin(QObject* parent = nullptr)
        : PluginBase(parent) {
        m_metadata.name = "MockCacheStrategy";
        m_metadata.version = "1.0.0";
        m_metadata.author = "Test";
        m_metadata.description = "Mock cache strategy plugin for tests";
        m_capabilities.provides = {"cache.strategy"};
    }

    // ICacheStrategyPlugin implementation
    QString strategyName() const override { return m_strategyName; }

    CacheEvictionStrategy evictionStrategy() const override {
        return m_evictionStrategy;
    }

    bool shouldCache(const QString& key, qint64 size,
                     const QVariantMap& metadata) const override {
        Q_UNUSED(key);
        Q_UNUSED(metadata);
        return size <= m_maxCacheableSize;
    }

    QString selectEvictionCandidate(const QList<CacheEntryMetadata>& entries,
                                    qint64 newEntrySize) const override {
        Q_UNUSED(newEntrySize);
        if (entries.isEmpty()) {
            return QString();
        }

        // Simple LRU: return oldest accessed entry
        CacheEntryMetadata oldest = entries.first();
        for (const CacheEntryMetadata& entry : entries) {
            if (entry.lastAccessedAt < oldest.lastAccessedAt) {
                oldest = entry;
            }
        }
        return oldest.key;
    }

    int calculatePriority(const CacheEntryMetadata& metadata) const override {
        // Higher access count = higher priority
        return metadata.accessCount * 10 + metadata.priority;
    }

    int optimizeCache(qint64 currentSize, qint64 maxSize) override {
        m_optimizeCalled = true;
        m_lastCurrentSize = currentSize;
        m_lastMaxSize = maxSize;

        if (currentSize <= maxSize) {
            return 0;
        }
        return 1;  // Simulated optimization performed
    }

    bool persistCache(const QString& cachePath,
                      const QList<CacheEntryMetadata>& entries) override {
        m_persistCalled = true;
        m_lastPersistPath = cachePath;
        m_lastPersistEntryCount = entries.size();
        return !m_shouldFail;
    }

    QList<CacheEntryMetadata> loadCache(const QString& cachePath) override {
        m_loadCalled = true;
        m_lastLoadPath = cachePath;
        return m_mockCacheEntries;
    }

    // Test configuration
    void setStrategyName(const QString& name) { m_strategyName = name; }
    void setEvictionStrategy(CacheEvictionStrategy strategy) {
        m_evictionStrategy = strategy;
    }
    void setMaxCacheableSize(qint64 size) { m_maxCacheableSize = size; }
    void setMockCacheEntries(const QList<CacheEntryMetadata>& entries) {
        m_mockCacheEntries = entries;
    }
    void setShouldFail(bool fail) { m_shouldFail = fail; }

    // Test verification
    bool wasOptimizeCalled() const { return m_optimizeCalled; }
    bool wasPersistCalled() const { return m_persistCalled; }
    bool wasLoadCalled() const { return m_loadCalled; }

protected:
    bool onInitialize() override { return true; }
    void onShutdown() override {}

private:
    QString m_strategyName = "MockLRU";
    CacheEvictionStrategy m_evictionStrategy = CacheEvictionStrategy::LRU;
    qint64 m_maxCacheableSize = 10 * 1024 * 1024;  // 10MB
    QList<CacheEntryMetadata> m_mockCacheEntries;
    bool m_shouldFail = false;

    bool m_optimizeCalled = false;
    bool m_persistCalled = false;
    bool m_loadCalled = false;
    qint64 m_lastCurrentSize = 0;
    qint64 m_lastMaxSize = 0;
    QString m_lastPersistPath;
    QString m_lastLoadPath;
    int m_lastPersistEntryCount = 0;
};

/**
 * @brief Mock implementation of IAnnotationPlugin
 */
class MockAnnotationPlugin : public PluginBase, public IAnnotationPlugin {
    Q_OBJECT
    Q_INTERFACES(IPluginInterface)

public:
    explicit MockAnnotationPlugin(QObject* parent = nullptr)
        : PluginBase(parent) {
        m_metadata.name = "MockAnnotationPlugin";
        m_metadata.version = "1.0.0";
        m_metadata.author = "Test";
        m_metadata.description = "Mock annotation plugin for tests";
        m_capabilities.provides = {"annotation.handler"};
    }

    // IAnnotationPlugin implementation
    QList<AnnotationType> supportedTypes() const override {
        return m_supportedTypes;
    }

    bool createAnnotation(const AnnotationData& data,
                          const QString& documentPath) override {
        m_createCalled = true;
        m_lastCreatedAnnotation = data;
        m_lastDocumentPath = documentPath;

        if (!m_shouldFail) {
            m_annotations.append(data);
        }
        return !m_shouldFail;
    }

    bool updateAnnotation(const QString& annotationId,
                          const AnnotationData& data,
                          const QString& documentPath) override {
        m_updateCalled = true;
        Q_UNUSED(documentPath);

        for (int i = 0; i < m_annotations.size(); ++i) {
            if (m_annotations[i].id == annotationId) {
                m_annotations[i] = data;
                return true;
            }
        }
        return false;
    }

    bool deleteAnnotation(const QString& annotationId,
                          const QString& documentPath) override {
        m_deleteCalled = true;
        Q_UNUSED(documentPath);

        for (int i = 0; i < m_annotations.size(); ++i) {
            if (m_annotations[i].id == annotationId) {
                m_annotations.removeAt(i);
                return true;
            }
        }
        return false;
    }

    QList<AnnotationData> getAnnotationsForPage(
        int pageNumber, const QString& documentPath) const override {
        Q_UNUSED(documentPath);

        QList<AnnotationData> result;
        for (const AnnotationData& ann : m_annotations) {
            if (ann.pageNumber == pageNumber) {
                result.append(ann);
            }
        }
        return result;
    }

    bool exportAnnotations(const QString& documentPath,
                           const QString& outputPath,
                           const QString& format) override {
        m_exportCalled = true;
        m_lastExportFormat = format;
        Q_UNUSED(documentPath);
        Q_UNUSED(outputPath);
        return !m_shouldFail;
    }

    int importAnnotations(const QString& inputPath, const QString& documentPath,
                          const QString& format) override {
        m_importCalled = true;
        m_lastImportFormat = format;
        Q_UNUSED(inputPath);
        Q_UNUSED(documentPath);
        return m_mockImportCount;
    }

    void renderAnnotation(QPainter* painter, const AnnotationData& annotation,
                          const QRect& pageRect, double zoom) override {
        m_renderCalled = true;
        m_lastRenderedAnnotation = annotation;

        if (painter) {
            painter->setPen(annotation.color);
            QRect scaledRect = annotation.boundingRect;
            scaledRect.setWidth(static_cast<int>(scaledRect.width() * zoom));
            scaledRect.setHeight(static_cast<int>(scaledRect.height() * zoom));
            painter->drawRect(scaledRect);
        }
        Q_UNUSED(pageRect);
    }

    // Test configuration
    void setSupportedTypes(const QList<AnnotationType>& types) {
        m_supportedTypes = types;
    }
    void setShouldFail(bool fail) { m_shouldFail = fail; }
    void setMockImportCount(int count) { m_mockImportCount = count; }
    void clearAnnotations() { m_annotations.clear(); }

    // Test verification
    bool wasCreateCalled() const { return m_createCalled; }
    bool wasUpdateCalled() const { return m_updateCalled; }
    bool wasDeleteCalled() const { return m_deleteCalled; }
    bool wasExportCalled() const { return m_exportCalled; }
    bool wasImportCalled() const { return m_importCalled; }
    bool wasRenderCalled() const { return m_renderCalled; }
    int annotationCount() const { return m_annotations.size(); }

protected:
    bool onInitialize() override { return true; }
    void onShutdown() override {}

private:
    QList<AnnotationType> m_supportedTypes = {
        AnnotationType::Highlight, AnnotationType::Underline,
        AnnotationType::Strikethrough, AnnotationType::Note};
    bool m_shouldFail = false;
    int m_mockImportCount = 5;

    QList<AnnotationData> m_annotations;

    bool m_createCalled = false;
    bool m_updateCalled = false;
    bool m_deleteCalled = false;
    bool m_exportCalled = false;
    bool m_importCalled = false;
    bool m_renderCalled = false;
    AnnotationData m_lastCreatedAnnotation;
    AnnotationData m_lastRenderedAnnotation;
    QString m_lastDocumentPath;
    QString m_lastExportFormat;
    QString m_lastImportFormat;
};

// ============================================================================
// Test Fixture
// ============================================================================

class PluginSpecializedInterfacesTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cleanupTestCase() override;

    // DocumentProcessingResult tests
    void test_document_processing_result_default();
    void test_document_processing_result_success();
    void test_document_processing_result_failure();

    // IDocumentProcessorPlugin tests
    void test_document_processor_handled_stages();
    void test_document_processor_can_process_file();
    void test_document_processor_process_document();
    void test_document_processor_process_document_failure();
    void test_document_processor_extract_metadata();
    void test_document_processor_export_document();

    // IRenderPlugin tests
    void test_render_plugin_filter_type();
    void test_render_plugin_should_process_page();
    void test_render_plugin_apply_filter();
    void test_render_plugin_apply_filter_failure();
    void test_render_plugin_render_overlay();
    void test_render_plugin_priority_and_thread_safety();

    // ISearchPlugin tests
    void test_search_plugin_algorithm_name();
    void test_search_plugin_can_handle_query();
    void test_search_plugin_execute_search();
    void test_search_plugin_post_process_results();
    void test_search_plugin_build_index();
    void test_search_plugin_clear_index();
    void test_search_plugin_index_size();

    // ICacheStrategyPlugin tests
    void test_cache_strategy_name_and_eviction();
    void test_cache_strategy_should_cache();
    void test_cache_strategy_select_eviction_candidate();
    void test_cache_strategy_calculate_priority();
    void test_cache_strategy_optimize();
    void test_cache_strategy_persist_and_load();

    // IAnnotationPlugin tests
    void test_annotation_plugin_supported_types();
    void test_annotation_plugin_create_annotation();
    void test_annotation_plugin_update_annotation();
    void test_annotation_plugin_delete_annotation();
    void test_annotation_plugin_get_annotations_for_page();
    void test_annotation_plugin_export_import();
    void test_annotation_plugin_render();

    // PluginWorkflowStage enum tests
    void test_plugin_workflow_stage_values();

    // RenderFilterType enum tests
    void test_render_filter_type_values();

    // SearchRankingStrategy enum tests
    void test_search_ranking_strategy_values();

    // CacheEvictionStrategy enum tests
    void test_cache_eviction_strategy_values();
};

void PluginSpecializedInterfacesTest::initTestCase() {
    TestBase::initTestCase();
}

void PluginSpecializedInterfacesTest::cleanupTestCase() {
    TestBase::cleanupTestCase();
}

// ============================================================================
// DocumentProcessingResult Tests
// ============================================================================

void PluginSpecializedInterfacesTest::
    test_document_processing_result_default() {
    DocumentProcessingResult result;
    QVERIFY(!result.success);
    QVERIFY(result.message.isEmpty());
    QVERIFY(!result.data.isValid());
    QVERIFY(result.warnings.isEmpty());
    QVERIFY(result.errors.isEmpty());
}

void PluginSpecializedInterfacesTest::
    test_document_processing_result_success() {
    auto result =
        DocumentProcessingResult::createSuccess("Operation completed", 42);

    QVERIFY(result.success);
    QCOMPARE(result.message, QString("Operation completed"));
    QCOMPARE(result.data.toInt(), 42);
    QVERIFY(result.errors.isEmpty());
}

void PluginSpecializedInterfacesTest::
    test_document_processing_result_failure() {
    QStringList errors = {"Error 1", "Error 2"};
    auto result = DocumentProcessingResult::createFailure("Failed", errors);

    QVERIFY(!result.success);
    QCOMPARE(result.message, QString("Failed"));
    QCOMPARE(result.errors.size(), 2);
    QVERIFY(result.errors.contains("Error 1"));
}

// ============================================================================
// IDocumentProcessorPlugin Tests
// ============================================================================

void PluginSpecializedInterfacesTest::test_document_processor_handled_stages() {
    MockDocumentProcessorPlugin plugin;

    QList<PluginWorkflowStage> stages = {PluginWorkflowStage::PreDocumentLoad,
                                         PluginWorkflowStage::PostDocumentLoad};
    plugin.setHandledStages(stages);

    QCOMPARE(plugin.handledStages().size(), 2);
    QVERIFY(
        plugin.handledStages().contains(PluginWorkflowStage::PreDocumentLoad));
}

void PluginSpecializedInterfacesTest::
    test_document_processor_can_process_file() {
    MockDocumentProcessorPlugin plugin;
    plugin.setSupportedExtensions({".pdf", ".epub"});

    QVERIFY(plugin.canProcessFile("/path/to/document.pdf"));
    QVERIFY(plugin.canProcessFile("/path/to/book.EPUB"));
    QVERIFY(!plugin.canProcessFile("/path/to/file.txt"));
    QVERIFY(!plugin.canProcessFile("/path/to/file.docx"));
}

void PluginSpecializedInterfacesTest::
    test_document_processor_process_document() {
    MockDocumentProcessorPlugin plugin;
    plugin.setResultData(QVariant("processed_data"));

    QJsonObject context;
    context["option"] = "value";

    auto result = plugin.processDocument(PluginWorkflowStage::PostDocumentLoad,
                                         "/path/doc.pdf", context);

    QVERIFY(plugin.wasProcessDocumentCalled());
    QVERIFY(result.success);
    QCOMPARE(plugin.lastProcessedStage(),
             PluginWorkflowStage::PostDocumentLoad);
    QCOMPARE(plugin.lastProcessedPath(), QString("/path/doc.pdf"));
}

void PluginSpecializedInterfacesTest::
    test_document_processor_process_document_failure() {
    MockDocumentProcessorPlugin plugin;
    plugin.setShouldFail(true);

    auto result = plugin.processDocument(PluginWorkflowStage::PreDocumentLoad,
                                         "/path/doc.pdf", QJsonObject());

    QVERIFY(!result.success);
    QCOMPARE(result.message, QString("Mock failure"));
}

void PluginSpecializedInterfacesTest::
    test_document_processor_extract_metadata() {
    MockDocumentProcessorPlugin plugin;

    QJsonObject metadata;
    metadata["title"] = "Test Document";
    metadata["author"] = "Test Author";
    metadata["pages"] = 100;
    plugin.setMockMetadata(metadata);

    QJsonObject extracted = plugin.extractMetadata("/path/doc.pdf");
    QCOMPARE(extracted["title"].toString(), QString("Test Document"));
    QCOMPARE(extracted["author"].toString(), QString("Test Author"));
    QCOMPARE(extracted["pages"].toInt(), 100);
}

void PluginSpecializedInterfacesTest::
    test_document_processor_export_document() {
    MockDocumentProcessorPlugin plugin;

    QJsonObject options;
    options["quality"] = "high";

    auto result =
        plugin.exportDocument("/src/doc.pdf", "/dst/doc.html", "html", options);

    QVERIFY(plugin.wasExportCalled());
    QVERIFY(result.success);
}

// ============================================================================
// IRenderPlugin Tests
// ============================================================================

void PluginSpecializedInterfacesTest::test_render_plugin_filter_type() {
    MockRenderPlugin plugin;

    plugin.setFilterType(RenderFilterType::ColorAdjustment);
    QCOMPARE(plugin.filterType(), RenderFilterType::ColorAdjustment);

    plugin.setFilterType(RenderFilterType::ImageEnhancement);
    QCOMPARE(plugin.filterType(), RenderFilterType::ImageEnhancement);
}

void PluginSpecializedInterfacesTest::test_render_plugin_should_process_page() {
    MockRenderPlugin plugin;

    // Process all pages
    plugin.setProcessAllPages(true);
    QVERIFY(plugin.shouldProcessPage("/doc.pdf", 0));
    QVERIFY(plugin.shouldProcessPage("/doc.pdf", 100));

    // Process specific pages only
    plugin.setProcessAllPages(false);
    plugin.setPagesToProcess({0, 2, 4});
    QVERIFY(plugin.shouldProcessPage("/doc.pdf", 0));
    QVERIFY(!plugin.shouldProcessPage("/doc.pdf", 1));
    QVERIFY(plugin.shouldProcessPage("/doc.pdf", 2));
}

void PluginSpecializedInterfacesTest::test_render_plugin_apply_filter() {
    MockRenderPlugin plugin;
    plugin.setInvertColors(true);

    QImage image(100, 100, QImage::Format_RGB32);
    image.fill(Qt::white);

    QJsonObject options;
    options["brightness"] = 50;

    bool result = plugin.applyFilter(image, 0, options);
    QVERIFY(result);
    QCOMPARE(plugin.filterAppliedCount(), 1);
    QCOMPARE(plugin.lastFilterPage(), 0);

    // Image should be inverted (white -> black)
    QCOMPARE(image.pixelColor(50, 50), QColor(Qt::black));
}

void PluginSpecializedInterfacesTest::
    test_render_plugin_apply_filter_failure() {
    MockRenderPlugin plugin;
    plugin.setShouldFail(true);

    QImage image(100, 100, QImage::Format_RGB32);
    bool result = plugin.applyFilter(image, 0, QJsonObject());

    QVERIFY(!result);
}

void PluginSpecializedInterfacesTest::test_render_plugin_render_overlay() {
    MockRenderPlugin plugin;
    plugin.setDrawOverlay(true);

    QImage image(200, 200, QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter(&image);

    QRect rect(0, 0, 200, 200);
    plugin.renderOverlay(&painter, rect, 5, QJsonObject());

    QCOMPARE(plugin.overlayRenderedCount(), 1);
}

void PluginSpecializedInterfacesTest::
    test_render_plugin_priority_and_thread_safety() {
    MockRenderPlugin plugin;

    plugin.setPriority(75);
    QCOMPARE(plugin.filterPriority(), 75);

    QVERIFY(!plugin.isThreadSafe());  // Default
    plugin.setThreadSafe(true);
    QVERIFY(plugin.isThreadSafe());
}

// ============================================================================
// ISearchPlugin Tests
// ============================================================================

void PluginSpecializedInterfacesTest::test_search_plugin_algorithm_name() {
    MockSearchPlugin plugin;
    plugin.setAlgorithmName("FuzzySearch");
    QCOMPARE(plugin.algorithmName(), QString("FuzzySearch"));
}

void PluginSpecializedInterfacesTest::test_search_plugin_can_handle_query() {
    MockSearchPlugin plugin;
    plugin.setHandleAllQueries(false);
    plugin.setMinQueryLength(3);

    QVERIFY(!plugin.canHandleQuery("ab", QJsonObject()));  // Too short
    QVERIFY(plugin.canHandleQuery("abc", QJsonObject()));  // OK
    QVERIFY(plugin.canHandleQuery("longer query", QJsonObject()));
}

void PluginSpecializedInterfacesTest::test_search_plugin_execute_search() {
    MockSearchPlugin plugin;

    PluginSearchResult result1;
    result1.text = "Found text 1";
    result1.pageNumber = 0;
    result1.relevanceScore = 0.9;

    PluginSearchResult result2;
    result2.text = "Found text 2";
    result2.pageNumber = 2;
    result2.relevanceScore = 0.7;

    plugin.setMockResults({result1, result2});

    QJsonObject options;
    options["caseSensitive"] = false;

    auto results = plugin.executeSearch("test", "/doc.pdf", options);

    QCOMPARE(plugin.searchExecutedCount(), 1);
    QCOMPARE(plugin.lastQuery(), QString("test"));
    QCOMPARE(results.size(), 2);
    QCOMPARE(results[0].text, QString("Found text 1"));
}

void PluginSpecializedInterfacesTest::
    test_search_plugin_post_process_results() {
    MockSearchPlugin plugin;

    PluginSearchResult r1;
    r1.relevanceScore = 0.5;
    PluginSearchResult r2;
    r2.relevanceScore = 0.9;
    PluginSearchResult r3;
    r3.relevanceScore = 0.7;

    QList<PluginSearchResult> input = {r1, r2, r3};

    auto sorted = plugin.postProcessResults(input, "query",
                                            SearchRankingStrategy::Relevance);

    QVERIFY(plugin.wasPostProcessCalled());
    QCOMPARE(sorted.size(), 3);
    // Should be sorted by relevance descending
    QCOMPARE(sorted[0].relevanceScore, 0.9);
    QCOMPARE(sorted[1].relevanceScore, 0.7);
    QCOMPARE(sorted[2].relevanceScore, 0.5);
}

void PluginSpecializedInterfacesTest::test_search_plugin_build_index() {
    MockSearchPlugin plugin;

    QVERIFY(plugin.buildSearchIndex("/doc.pdf", QJsonObject()));
    QVERIFY(plugin.wasBuildIndexCalled());

    plugin.setShouldFail(true);
    QVERIFY(!plugin.buildSearchIndex("/doc2.pdf", QJsonObject()));
}

void PluginSpecializedInterfacesTest::test_search_plugin_clear_index() {
    MockSearchPlugin plugin;

    plugin.clearIndex("/doc.pdf");
    QVERIFY(plugin.wasClearIndexCalled());
}

void PluginSpecializedInterfacesTest::test_search_plugin_index_size() {
    MockSearchPlugin plugin;
    plugin.setMockIndexSize(2048);

    QCOMPARE(plugin.getIndexSize("/doc.pdf"), qint64(2048));
}

// ============================================================================
// ICacheStrategyPlugin Tests
// ============================================================================

void PluginSpecializedInterfacesTest::test_cache_strategy_name_and_eviction() {
    MockCacheStrategyPlugin plugin;

    plugin.setStrategyName("AdaptiveLRU");
    QCOMPARE(plugin.strategyName(), QString("AdaptiveLRU"));

    plugin.setEvictionStrategy(CacheEvictionStrategy::LFU);
    QCOMPARE(plugin.evictionStrategy(), CacheEvictionStrategy::LFU);
}

void PluginSpecializedInterfacesTest::test_cache_strategy_should_cache() {
    MockCacheStrategyPlugin plugin;
    plugin.setMaxCacheableSize(1024);  // 1KB

    QVERIFY(plugin.shouldCache("key1", 512, QVariantMap()));    // OK
    QVERIFY(plugin.shouldCache("key2", 1024, QVariantMap()));   // OK (equal)
    QVERIFY(!plugin.shouldCache("key3", 2048, QVariantMap()));  // Too large
}

void PluginSpecializedInterfacesTest::
    test_cache_strategy_select_eviction_candidate() {
    MockCacheStrategyPlugin plugin;

    CacheEntryMetadata entry1;
    entry1.key = "key1";
    entry1.lastAccessedAt = QDateTime::currentDateTime().addSecs(-100);

    CacheEntryMetadata entry2;
    entry2.key = "key2";
    entry2.lastAccessedAt = QDateTime::currentDateTime().addSecs(-50);

    CacheEntryMetadata entry3;
    entry3.key = "key3";
    entry3.lastAccessedAt = QDateTime::currentDateTime();

    QList<CacheEntryMetadata> entries = {entry1, entry2, entry3};

    QString candidate = plugin.selectEvictionCandidate(entries, 1024);
    QCOMPARE(candidate, QString("key1"));  // Oldest accessed
}

void PluginSpecializedInterfacesTest::test_cache_strategy_calculate_priority() {
    MockCacheStrategyPlugin plugin;

    CacheEntryMetadata entry;
    entry.accessCount = 5;
    entry.priority = 10;

    int priority = plugin.calculatePriority(entry);
    QCOMPARE(priority, 60);  // 5 * 10 + 10
}

void PluginSpecializedInterfacesTest::test_cache_strategy_optimize() {
    MockCacheStrategyPlugin plugin;

    int optimizations = plugin.optimizeCache(100, 200);
    QVERIFY(plugin.wasOptimizeCalled());
    QCOMPARE(optimizations, 0);  // Under max size

    optimizations = plugin.optimizeCache(300, 200);
    QCOMPARE(optimizations, 1);  // Over max size, optimization performed
}

void PluginSpecializedInterfacesTest::test_cache_strategy_persist_and_load() {
    MockCacheStrategyPlugin plugin;

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString cachePath = tempDir.filePath("cache.dat");

    CacheEntryMetadata entry1;
    entry1.key = "key1";
    entry1.size = 100;

    CacheEntryMetadata entry2;
    entry2.key = "key2";
    entry2.size = 200;

    plugin.setMockCacheEntries({entry1, entry2});

    // Persist
    QVERIFY(plugin.persistCache(cachePath, {entry1, entry2}));
    QVERIFY(plugin.wasPersistCalled());

    // Load
    auto loaded = plugin.loadCache(cachePath);
    QVERIFY(plugin.wasLoadCalled());
    QCOMPARE(loaded.size(), 2);
}

// ============================================================================
// IAnnotationPlugin Tests
// ============================================================================

void PluginSpecializedInterfacesTest::test_annotation_plugin_supported_types() {
    MockAnnotationPlugin plugin;

    auto types = plugin.supportedTypes();
    QVERIFY(types.contains(AnnotationType::Highlight));
    QVERIFY(types.contains(AnnotationType::Underline));
    QVERIFY(types.contains(AnnotationType::Note));
}

void PluginSpecializedInterfacesTest::
    test_annotation_plugin_create_annotation() {
    MockAnnotationPlugin plugin;

    AnnotationData data;
    data.id = "ann-1";
    data.type = AnnotationType::Highlight;
    data.pageNumber = 0;
    data.content = "Highlighted text";
    data.color = Qt::yellow;

    QVERIFY(plugin.createAnnotation(data, "/doc.pdf"));
    QVERIFY(plugin.wasCreateCalled());
    QCOMPARE(plugin.annotationCount(), 1);
}

void PluginSpecializedInterfacesTest::
    test_annotation_plugin_update_annotation() {
    MockAnnotationPlugin plugin;

    // Create first
    AnnotationData data;
    data.id = "ann-1";
    data.content = "Original";
    plugin.createAnnotation(data, "/doc.pdf");

    // Update
    data.content = "Updated";
    QVERIFY(plugin.updateAnnotation("ann-1", data, "/doc.pdf"));
    QVERIFY(plugin.wasUpdateCalled());
}

void PluginSpecializedInterfacesTest::
    test_annotation_plugin_delete_annotation() {
    MockAnnotationPlugin plugin;

    AnnotationData data;
    data.id = "ann-1";
    plugin.createAnnotation(data, "/doc.pdf");
    QCOMPARE(plugin.annotationCount(), 1);

    QVERIFY(plugin.deleteAnnotation("ann-1", "/doc.pdf"));
    QVERIFY(plugin.wasDeleteCalled());
    QCOMPARE(plugin.annotationCount(), 0);
}

void PluginSpecializedInterfacesTest::
    test_annotation_plugin_get_annotations_for_page() {
    MockAnnotationPlugin plugin;

    AnnotationData ann1;
    ann1.id = "ann-1";
    ann1.pageNumber = 0;
    plugin.createAnnotation(ann1, "/doc.pdf");

    AnnotationData ann2;
    ann2.id = "ann-2";
    ann2.pageNumber = 0;
    plugin.createAnnotation(ann2, "/doc.pdf");

    AnnotationData ann3;
    ann3.id = "ann-3";
    ann3.pageNumber = 1;
    plugin.createAnnotation(ann3, "/doc.pdf");

    auto page0Anns = plugin.getAnnotationsForPage(0, "/doc.pdf");
    QCOMPARE(page0Anns.size(), 2);

    auto page1Anns = plugin.getAnnotationsForPage(1, "/doc.pdf");
    QCOMPARE(page1Anns.size(), 1);

    auto page2Anns = plugin.getAnnotationsForPage(2, "/doc.pdf");
    QCOMPARE(page2Anns.size(), 0);
}

void PluginSpecializedInterfacesTest::test_annotation_plugin_export_import() {
    MockAnnotationPlugin plugin;
    plugin.setMockImportCount(10);

    QVERIFY(plugin.exportAnnotations("/doc.pdf", "/out.json", "json"));
    QVERIFY(plugin.wasExportCalled());

    int imported = plugin.importAnnotations("/in.json", "/doc.pdf", "json");
    QVERIFY(plugin.wasImportCalled());
    QCOMPARE(imported, 10);
}

void PluginSpecializedInterfacesTest::test_annotation_plugin_render() {
    MockAnnotationPlugin plugin;

    QImage image(200, 200, QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter(&image);

    AnnotationData ann;
    ann.type = AnnotationType::Highlight;
    ann.boundingRect = QRect(10, 10, 50, 20);
    ann.color = Qt::yellow;

    plugin.renderAnnotation(&painter, ann, QRect(0, 0, 200, 200), 1.0);
    QVERIFY(plugin.wasRenderCalled());
}

// ============================================================================
// Enum Tests
// ============================================================================

void PluginSpecializedInterfacesTest::test_plugin_workflow_stage_values() {
    // Verify enum values are distinct
    QVERIFY(PluginWorkflowStage::PreDocumentLoad !=
            PluginWorkflowStage::PostDocumentLoad);
    QVERIFY(PluginWorkflowStage::PrePageRender !=
            PluginWorkflowStage::PostPageRender);
    QVERIFY(PluginWorkflowStage::PreSearch != PluginWorkflowStage::PostSearch);
    QVERIFY(PluginWorkflowStage::PreCache != PluginWorkflowStage::PostCache);
    QVERIFY(PluginWorkflowStage::PreExport != PluginWorkflowStage::PostExport);
}

void PluginSpecializedInterfacesTest::test_render_filter_type_values() {
    QVERIFY(RenderFilterType::ColorAdjustment !=
            RenderFilterType::ImageEnhancement);
    QVERIFY(RenderFilterType::Overlay != RenderFilterType::Transform);
    QVERIFY(RenderFilterType::Custom != RenderFilterType::ColorAdjustment);
}

void PluginSpecializedInterfacesTest::test_search_ranking_strategy_values() {
    QVERIFY(SearchRankingStrategy::Frequency !=
            SearchRankingStrategy::Position);
    QVERIFY(SearchRankingStrategy::Relevance != SearchRankingStrategy::Custom);
}

void PluginSpecializedInterfacesTest::test_cache_eviction_strategy_values() {
    QVERIFY(CacheEvictionStrategy::LRU != CacheEvictionStrategy::LFU);
    QVERIFY(CacheEvictionStrategy::FIFO != CacheEvictionStrategy::ARC);
    QVERIFY(CacheEvictionStrategy::Custom != CacheEvictionStrategy::LRU);
}

QTEST_MAIN(PluginSpecializedInterfacesTest)
#include "test_plugin_specialized_interfaces.moc"
