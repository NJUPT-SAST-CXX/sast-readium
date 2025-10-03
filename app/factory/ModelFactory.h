#pragma once

#include <QHash>
#include <QObject>
#include <functional>
#include <memory>
#include "../logging/SimpleLogging.h"

// Forward declarations
class RenderModel;
class DocumentModel;
class PageModel;
class ThumbnailModel;
class BookmarkModel;
class AnnotationModel;
class SearchModel;
class PDFOutlineModel;
class AsyncDocumentLoader;

/**
 * @brief ModelFactory - Creates and configures model objects
 *
 * This factory follows the Factory Method and Abstract Factory patterns
 * to encapsulate model creation logic and provide proper dependency injection.
 * It ensures models are created with correct dependencies and configurations.
 */
class ModelFactory : public QObject {
    Q_OBJECT

public:
    explicit ModelFactory(QObject* parent = nullptr);
    ~ModelFactory();

    // Factory methods for creating models
    RenderModel* createRenderModel(int dpiX, int dpiY);
    DocumentModel* createDocumentModel(RenderModel* renderModel);
    PageModel* createPageModel(RenderModel* renderModel);
    ThumbnailModel* createThumbnailModel(DocumentModel* documentModel);
    BookmarkModel* createBookmarkModel(DocumentModel* documentModel);
    AnnotationModel* createAnnotationModel(DocumentModel* documentModel);
    SearchModel* createSearchModel(DocumentModel* documentModel);
    PDFOutlineModel* createPDFOutlineModel(DocumentModel* documentModel);
    AsyncDocumentLoader* createAsyncDocumentLoader(
        DocumentModel* documentModel);

    // Composite factory methods
    struct ModelSet {
        RenderModel* renderModel = nullptr;
        DocumentModel* documentModel = nullptr;
        PageModel* pageModel = nullptr;
        ThumbnailModel* thumbnailModel = nullptr;
        BookmarkModel* bookmarkModel = nullptr;
        AnnotationModel* annotationModel = nullptr;
        SearchModel* searchModel = nullptr;
        PDFOutlineModel* outlineModel = nullptr;
        AsyncDocumentLoader* documentLoader = nullptr;
    };

    ModelSet createCompleteModelSet(int dpiX, int dpiY);
    ModelSet createMinimalModelSet(int dpiX, int dpiY);
    ModelSet createViewerModelSet(int dpiX, int dpiY);

    // Configuration methods
    void setModelParent(QObject* parent) { m_modelParent = parent; }
    void setAutoDelete(bool autoDelete) { m_autoDelete = autoDelete; }

    // Registration for custom model types
    using ModelCreator = std::function<QObject*(QObject*)>;
    void registerModelType(const QString& typeName, ModelCreator creator);
    QObject* createCustomModel(const QString& typeName);

signals:
    void modelCreated(const QString& modelType, QObject* model);
    void modelSetCreated(const ModelSet& models);
    void creationError(const QString& modelType, const QString& error);

private:
    // Helper methods
    void connectModelSignals(QObject* model, const QString& modelType);
    void configureModel(QObject* model);
    bool validateDependencies(QObject* model);

    // Configuration
    QObject* m_modelParent = nullptr;
    bool m_autoDelete = false;

    // Custom model registry
    QHash<QString, ModelCreator> m_customCreators;

    // Logging
    SastLogging::CategoryLogger m_logger;
};

/**
 * @brief SingletonModelFactory - Singleton factory for global model access
 *
 * Provides a global access point for model creation while ensuring
 * single instances of critical models.
 */
class SingletonModelFactory {
public:
    static SingletonModelFactory& instance();

    // Get or create singleton models
    RenderModel* getRenderModel();
    DocumentModel* getDocumentModel();
    PageModel* getPageModel();

    // Reset singleton instances (for testing)
    void reset();

private:
    SingletonModelFactory();
    ~SingletonModelFactory();
    SingletonModelFactory(const SingletonModelFactory&) = delete;
    SingletonModelFactory& operator=(const SingletonModelFactory&) = delete;

    // Singleton instances
    std::unique_ptr<RenderModel> m_renderModel;
    std::unique_ptr<DocumentModel> m_documentModel;
    std::unique_ptr<PageModel> m_pageModel;
    std::unique_ptr<ModelFactory> m_factory;

    // Configuration
    int m_dpiX = 96;
    int m_dpiY = 96;
};

/**
 * @brief ModelBuilder - Builder pattern for complex model configuration
 *
 * Provides a fluent interface for building and configuring models
 * with complex initialization requirements.
 */
class ModelBuilder {
public:
    ModelBuilder();
    ~ModelBuilder();

    // Fluent interface for configuration
    ModelBuilder& withDpi(int dpiX, int dpiY);
    ModelBuilder& withParent(QObject* parent);
    ModelBuilder& withRenderModel(RenderModel* model);
    ModelBuilder& withDocumentModel(DocumentModel* model);
    ModelBuilder& withThumbnails(bool enable);
    ModelBuilder& withBookmarks(bool enable);
    ModelBuilder& withAnnotations(bool enable);
    ModelBuilder& withSearch(bool enable);
    ModelBuilder& withOutline(bool enable);
    ModelBuilder& withAsyncLoading(bool enable);

    // Build methods
    ModelFactory::ModelSet build();
    std::unique_ptr<ModelFactory::ModelSet> buildUnique();

private:
    struct BuilderData;
    std::unique_ptr<BuilderData> m_data;
};
