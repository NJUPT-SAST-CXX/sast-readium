#include "ModelFactory.h"
#include <QApplication>
#include <QScreen>
#include "../model/AnnotationModel.h"
#include "../model/AsyncDocumentLoader.h"
#include "../model/BookmarkModel.h"
#include "../model/DocumentModel.h"
#include "../model/PDFOutlineModel.h"
#include "../model/PageModel.h"
#include "../model/RenderModel.h"
#include "../model/SearchModel.h"
#include "../model/ThumbnailModel.h"
#include "../utils/ErrorHandling.h"

// ModelFactory implementation
ModelFactory::ModelFactory(QObject* parent)
    : QObject(parent), m_logger("ModelFactory") {
    m_logger.debug("ModelFactory created");
}

ModelFactory::~ModelFactory() {
    if (m_autoDelete) {
        // Models will be deleted automatically if they're children of this
        // factory
        m_logger.debug("ModelFactory destroyed with auto-delete");
    } else {
        m_logger.debug("ModelFactory destroyed");
    }
}

RenderModel* ModelFactory::createRenderModel(int dpiX, int dpiY) {
    try {
        m_logger.debug(QString("Creating RenderModel with DPI: %1x%2")
                           .arg(dpiX)
                           .arg(dpiY));

        auto* model = new RenderModel(dpiX, dpiY);
        if (m_modelParent) {
            model->setParent(m_modelParent);
        }

        configureModel(model);
        connectModelSignals(model, "RenderModel");

        emit modelCreated("RenderModel", model);
        return model;

    } catch (const std::exception& e) {
        QString error =
            QString("Failed to create RenderModel: %1").arg(e.what());
        m_logger.error(error);
        emit creationError("RenderModel", error);
        return nullptr;
    }
}

DocumentModel* ModelFactory::createDocumentModel(RenderModel* renderModel) {
    if (!renderModel) {
        QString error = "Cannot create DocumentModel without RenderModel";
        m_logger.error(error);
        emit creationError("DocumentModel", error);
        return nullptr;
    }

    try {
        m_logger.debug("Creating DocumentModel");

        auto* model = new DocumentModel(renderModel);
        if (m_modelParent) {
            model->setParent(m_modelParent);
        }

        configureModel(model);
        connectModelSignals(model, "DocumentModel");

        emit modelCreated("DocumentModel", model);
        return model;

    } catch (const std::exception& e) {
        QString error =
            QString("Failed to create DocumentModel: %1").arg(e.what());
        m_logger.error(error);
        emit creationError("DocumentModel", error);
        return nullptr;
    }
}

PageModel* ModelFactory::createPageModel(RenderModel* renderModel) {
    if (!renderModel) {
        QString error = "Cannot create PageModel without RenderModel";
        m_logger.error(error);
        emit creationError("PageModel", error);
        return nullptr;
    }

    try {
        m_logger.debug("Creating PageModel");

        auto* model = new PageModel(renderModel);
        if (m_modelParent) {
            model->setParent(m_modelParent);
        }

        configureModel(model);
        connectModelSignals(model, "PageModel");

        emit modelCreated("PageModel", model);
        return model;

    } catch (const std::exception& e) {
        QString error = QString("Failed to create PageModel: %1").arg(e.what());
        m_logger.error(error);
        emit creationError("PageModel", error);
        return nullptr;
    }
}

ThumbnailModel* ModelFactory::createThumbnailModel(
    DocumentModel* documentModel) {
    if (!documentModel) {
        QString error = "Cannot create ThumbnailModel without DocumentModel";
        m_logger.error(error);
        emit creationError("ThumbnailModel", error);
        return nullptr;
    }

    try {
        m_logger.debug("Creating ThumbnailModel");

        auto* model = new ThumbnailModel(documentModel);
        if (m_modelParent) {
            model->setParent(m_modelParent);
        }

        configureModel(model);
        connectModelSignals(model, "ThumbnailModel");

        emit modelCreated("ThumbnailModel", model);
        return model;

    } catch (const std::exception& e) {
        QString error =
            QString("Failed to create ThumbnailModel: %1").arg(e.what());
        m_logger.error(error);
        emit creationError("ThumbnailModel", error);
        return nullptr;
    }
}

BookmarkModel* ModelFactory::createBookmarkModel(DocumentModel* documentModel) {
    if (!documentModel) {
        QString error = "Cannot create BookmarkModel without DocumentModel";
        m_logger.error(error);
        emit creationError("BookmarkModel", error);
        return nullptr;
    }

    try {
        m_logger.debug("Creating BookmarkModel");

        auto* model = new BookmarkModel(documentModel);
        if (m_modelParent) {
            model->setParent(m_modelParent);
        }

        configureModel(model);
        connectModelSignals(model, "BookmarkModel");

        emit modelCreated("BookmarkModel", model);
        return model;

    } catch (const std::exception& e) {
        QString error =
            QString("Failed to create BookmarkModel: %1").arg(e.what());
        m_logger.error(error);
        emit creationError("BookmarkModel", error);
        return nullptr;
    }
}

AnnotationModel* ModelFactory::createAnnotationModel(
    DocumentModel* documentModel) {
    if (!documentModel) {
        QString error = "Cannot create AnnotationModel without DocumentModel";
        m_logger.error(error);
        emit creationError("AnnotationModel", error);
        return nullptr;
    }

    try {
        m_logger.debug("Creating AnnotationModel");

        auto* model = new AnnotationModel(documentModel);
        if (m_modelParent) {
            model->setParent(m_modelParent);
        }

        configureModel(model);
        connectModelSignals(model, "AnnotationModel");

        emit modelCreated("AnnotationModel", model);
        return model;

    } catch (const std::exception& e) {
        QString error =
            QString("Failed to create AnnotationModel: %1").arg(e.what());
        m_logger.error(error);
        emit creationError("AnnotationModel", error);
        return nullptr;
    }
}

SearchModel* ModelFactory::createSearchModel(DocumentModel* documentModel) {
    if (!documentModel) {
        QString error = "Cannot create SearchModel without DocumentModel";
        m_logger.error(error);
        emit creationError("SearchModel", error);
        return nullptr;
    }

    try {
        m_logger.debug("Creating SearchModel");

        auto* model = new SearchModel(documentModel);
        if (m_modelParent) {
            model->setParent(m_modelParent);
        }

        configureModel(model);
        connectModelSignals(model, "SearchModel");

        emit modelCreated("SearchModel", model);
        return model;

    } catch (const std::exception& e) {
        QString error =
            QString("Failed to create SearchModel: %1").arg(e.what());
        m_logger.error(error);
        emit creationError("SearchModel", error);
        return nullptr;
    }
}

PDFOutlineModel* ModelFactory::createPDFOutlineModel(
    DocumentModel* documentModel) {
    if (!documentModel) {
        QString error = "Cannot create PDFOutlineModel without DocumentModel";
        m_logger.error(error);
        emit creationError("PDFOutlineModel", error);
        return nullptr;
    }

    try {
        m_logger.debug("Creating PDFOutlineModel");

        auto* model = new PDFOutlineModel(documentModel);
        if (m_modelParent) {
            model->setParent(m_modelParent);
        }

        configureModel(model);
        connectModelSignals(model, "PDFOutlineModel");

        emit modelCreated("PDFOutlineModel", model);
        return model;

    } catch (const std::exception& e) {
        QString error =
            QString("Failed to create PDFOutlineModel: %1").arg(e.what());
        m_logger.error(error);
        emit creationError("PDFOutlineModel", error);
        return nullptr;
    }
}

AsyncDocumentLoader* ModelFactory::createAsyncDocumentLoader(
    DocumentModel* documentModel) {
    if (!documentModel) {
        QString error =
            "Cannot create AsyncDocumentLoader without DocumentModel";
        m_logger.error(error);
        emit creationError("AsyncDocumentLoader", error);
        return nullptr;
    }

    try {
        m_logger.debug("Creating AsyncDocumentLoader");

        auto* loader = new AsyncDocumentLoader(documentModel);
        if (m_modelParent) {
            loader->setParent(m_modelParent);
        }

        configureModel(loader);
        connectModelSignals(loader, "AsyncDocumentLoader");

        emit modelCreated("AsyncDocumentLoader", loader);
        return loader;

    } catch (const std::exception& e) {
        QString error =
            QString("Failed to create AsyncDocumentLoader: %1").arg(e.what());
        m_logger.error(error);
        emit creationError("AsyncDocumentLoader", error);
        return nullptr;
    }
}

ModelFactory::ModelSet ModelFactory::createCompleteModelSet(int dpiX,
                                                            int dpiY) {
    m_logger.debug("Creating complete model set");

    ModelSet models;

    // Create core models
    models.renderModel = createRenderModel(dpiX, dpiY);
    if (!models.renderModel) {
        m_logger.error("Failed to create RenderModel - aborting model set creation");
        emit creationError("ModelSet", "Failed to create RenderModel");
        return models;
    }

    models.documentModel = createDocumentModel(models.renderModel);
    if (!models.documentModel) {
        m_logger.error("Failed to create DocumentModel - cleaning up and aborting");
        emit creationError("ModelSet", "Failed to create DocumentModel");
        // Clean up already-created models if auto-delete is enabled
        if (m_autoDelete && models.renderModel) {
            delete models.renderModel;
            models.renderModel = nullptr;
        }
        return models;
    }

    models.pageModel = createPageModel(models.renderModel);
    if (!models.pageModel) {
        m_logger.warning("Failed to create PageModel - continuing with partial set");
    }

    // Create auxiliary models - failures are non-critical
    models.thumbnailModel = createThumbnailModel(models.documentModel);
    if (!models.thumbnailModel) {
        m_logger.warning("Failed to create ThumbnailModel - continuing");
    }

    models.bookmarkModel = createBookmarkModel(models.documentModel);
    if (!models.bookmarkModel) {
        m_logger.warning("Failed to create BookmarkModel - continuing");
    }

    models.annotationModel = createAnnotationModel(models.documentModel);
    if (!models.annotationModel) {
        m_logger.warning("Failed to create AnnotationModel - continuing");
    }

    models.searchModel = createSearchModel(models.documentModel);
    if (!models.searchModel) {
        m_logger.warning("Failed to create SearchModel - continuing");
    }

    models.outlineModel = createPDFOutlineModel(models.documentModel);
    if (!models.outlineModel) {
        m_logger.warning("Failed to create PDFOutlineModel - continuing");
    }

    models.documentLoader = createAsyncDocumentLoader(models.documentModel);
    if (!models.documentLoader) {
        m_logger.warning("Failed to create AsyncDocumentLoader - continuing");
    }

    emit modelSetCreated(models);
    m_logger.info("Complete model set created successfully");

    return models;
}

ModelFactory::ModelSet ModelFactory::createMinimalModelSet(int dpiX, int dpiY) {
    m_logger.debug("Creating minimal model set");

    ModelSet models;

    // Create only essential models
    models.renderModel = createRenderModel(dpiX, dpiY);
    if (!models.renderModel) {
        m_logger.error("Failed to create RenderModel - aborting minimal model set creation");
        emit creationError("ModelSet", "Failed to create RenderModel");
        return models;
    }

    models.documentModel = createDocumentModel(models.renderModel);
    if (!models.documentModel) {
        m_logger.error("Failed to create DocumentModel - cleaning up and aborting");
        emit creationError("ModelSet", "Failed to create DocumentModel");
        // Clean up already-created models if auto-delete is enabled
        if (m_autoDelete && models.renderModel) {
            delete models.renderModel;
            models.renderModel = nullptr;
        }
        return models;
    }

    models.pageModel = createPageModel(models.renderModel);
    if (!models.pageModel) {
        m_logger.warning("Failed to create PageModel - continuing with partial set");
    }

    emit modelSetCreated(models);
    m_logger.info("Minimal model set created successfully");

    return models;
}

ModelFactory::ModelSet ModelFactory::createViewerModelSet(int dpiX, int dpiY) {
    m_logger.debug("Creating viewer model set");

    ModelSet models;

    // Create models needed for viewing
    models.renderModel = createRenderModel(dpiX, dpiY);
    if (!models.renderModel) {
        m_logger.error("Failed to create RenderModel - aborting viewer model set creation");
        emit creationError("ModelSet", "Failed to create RenderModel");
        return models;
    }

    models.documentModel = createDocumentModel(models.renderModel);
    if (!models.documentModel) {
        m_logger.error("Failed to create DocumentModel - cleaning up and aborting");
        emit creationError("ModelSet", "Failed to create DocumentModel");
        // Clean up already-created models if auto-delete is enabled
        if (m_autoDelete && models.renderModel) {
            delete models.renderModel;
            models.renderModel = nullptr;
        }
        return models;
    }

    models.pageModel = createPageModel(models.renderModel);
    if (!models.pageModel) {
        m_logger.warning("Failed to create PageModel - continuing");
    }

    models.thumbnailModel = createThumbnailModel(models.documentModel);
    if (!models.thumbnailModel) {
        m_logger.warning("Failed to create ThumbnailModel - continuing");
    }

    models.outlineModel = createPDFOutlineModel(models.documentModel);
    if (!models.outlineModel) {
        m_logger.warning("Failed to create PDFOutlineModel - continuing");
    }

    models.searchModel = createSearchModel(models.documentModel);
    if (!models.searchModel) {
        m_logger.warning("Failed to create SearchModel - continuing");
    }

    emit modelSetCreated(models);
    m_logger.info("Viewer model set created successfully");

    return models;
}

void ModelFactory::registerModelType(const QString& typeName,
                                     ModelCreator creator) {
    m_customCreators[typeName] = creator;
    m_logger.debug(QString("Registered custom model type: %1").arg(typeName));
}

QObject* ModelFactory::createCustomModel(const QString& typeName) {
    if (!m_customCreators.contains(typeName)) {
        QString error = QString("Unknown model type: %1").arg(typeName);
        m_logger.error(error);
        emit creationError(typeName, error);
        return nullptr;
    }

    try {
        auto creator = m_customCreators[typeName];
        QObject* model = creator(m_modelParent);

        configureModel(model);
        connectModelSignals(model, typeName);

        emit modelCreated(typeName, model);
        return model;

    } catch (const std::exception& e) {
        QString error =
            QString("Failed to create %1: %2").arg(typeName).arg(e.what());
        m_logger.error(error);
        emit creationError(typeName, error);
        return nullptr;
    }
}

void ModelFactory::connectModelSignals(QObject* model,
                                       const QString& modelType) {
    // Connect common model signals for monitoring
    if (model) {
        connect(model, &QObject::destroyed, this, [this, modelType]() {
            m_logger.debug(QString("%1 destroyed").arg(modelType));
        });
    }
}

void ModelFactory::configureModel(QObject* model) {
    // Apply common configuration to all models
    if (model) {
        model->setObjectName(model->metaObject()->className());
    }
}

bool ModelFactory::validateDependencies(QObject* model) {
    // Validate that model pointer is valid
    // Note: Dependency validation (e.g., DocumentModel has RenderModel) is
    // performed during model creation in the individual factory methods.
    // This method serves as a final sanity check that the model was created
    // successfully.
    if (!model) {
        m_logger.warning("Model validation failed: null model pointer");
        return false;
    }

    // Verify the model has a valid object name (set by configureModel)
    if (model->objectName().isEmpty()) {
        m_logger.warning(
            QString("Model validation warning: %1 has no object name")
                .arg(model->metaObject()->className()));
        // This is a warning, not a failure - continue
    }

    return true;
}

// SingletonModelFactory implementation
SingletonModelFactory& SingletonModelFactory::instance() {
    static SingletonModelFactory instance;
    return instance;
}

SingletonModelFactory::SingletonModelFactory() {
    m_factory = std::make_unique<ModelFactory>();

    // Get DPI from primary screen
    if (QApplication::instance()) {
        QScreen* screen = QApplication::primaryScreen();
        if (screen) {
            m_dpiX = screen->logicalDotsPerInchX();
            m_dpiY = screen->logicalDotsPerInchY();
        }
    }
}

SingletonModelFactory::~SingletonModelFactory() = default;

RenderModel* SingletonModelFactory::getRenderModel() {
    if (!m_renderModel) {
        m_renderModel.reset(m_factory->createRenderModel(m_dpiX, m_dpiY));
    }
    return m_renderModel.get();
}

DocumentModel* SingletonModelFactory::getDocumentModel() {
    if (!m_documentModel) {
        RenderModel* renderModel = getRenderModel();
        if (renderModel) {
            m_documentModel.reset(m_factory->createDocumentModel(renderModel));
        }
    }
    return m_documentModel.get();
}

PageModel* SingletonModelFactory::getPageModel() {
    if (!m_pageModel) {
        RenderModel* renderModel = getRenderModel();
        if (renderModel) {
            m_pageModel.reset(m_factory->createPageModel(renderModel));
        }
    }
    return m_pageModel.get();
}

void SingletonModelFactory::reset() {
    m_pageModel.reset();
    m_documentModel.reset();
    m_renderModel.reset();
}

// ModelBuilder implementation
struct ModelBuilder::BuilderData {
    int dpiX = 96;
    int dpiY = 96;
    QObject* parent = nullptr;
    RenderModel* renderModel = nullptr;
    DocumentModel* documentModel = nullptr;
    bool enableThumbnails = true;
    bool enableBookmarks = true;
    bool enableAnnotations = true;
    bool enableSearch = true;
    bool enableOutline = true;
    bool enableAsyncLoading = true;
};

ModelBuilder::ModelBuilder() : m_data(std::make_unique<BuilderData>()) {}

ModelBuilder::~ModelBuilder() = default;

ModelBuilder& ModelBuilder::withDpi(int dpiX, int dpiY) {
    m_data->dpiX = dpiX;
    m_data->dpiY = dpiY;
    return *this;
}

ModelBuilder& ModelBuilder::withParent(QObject* parent) {
    m_data->parent = parent;
    return *this;
}

ModelBuilder& ModelBuilder::withRenderModel(RenderModel* model) {
    m_data->renderModel = model;
    return *this;
}

ModelBuilder& ModelBuilder::withDocumentModel(DocumentModel* model) {
    m_data->documentModel = model;
    return *this;
}

ModelBuilder& ModelBuilder::withThumbnails(bool enable) {
    m_data->enableThumbnails = enable;
    return *this;
}

ModelBuilder& ModelBuilder::withBookmarks(bool enable) {
    m_data->enableBookmarks = enable;
    return *this;
}

ModelBuilder& ModelBuilder::withAnnotations(bool enable) {
    m_data->enableAnnotations = enable;
    return *this;
}

ModelBuilder& ModelBuilder::withSearch(bool enable) {
    m_data->enableSearch = enable;
    return *this;
}

ModelBuilder& ModelBuilder::withOutline(bool enable) {
    m_data->enableOutline = enable;
    return *this;
}

ModelBuilder& ModelBuilder::withAsyncLoading(bool enable) {
    m_data->enableAsyncLoading = enable;
    return *this;
}

ModelFactory::ModelSet ModelBuilder::build() {
    ModelFactory factory(m_data->parent);
    factory.setModelParent(m_data->parent);

    ModelFactory::ModelSet models;

    // Use provided models or create new ones
    models.renderModel = m_data->renderModel ? m_data->renderModel
                                             : factory.createRenderModel(
                                                   m_data->dpiX, m_data->dpiY);

    models.documentModel =
        m_data->documentModel ? m_data->documentModel
                              : factory.createDocumentModel(models.renderModel);

    if (models.renderModel) {
        models.pageModel = factory.createPageModel(models.renderModel);
    }

    if (models.documentModel) {
        if (m_data->enableThumbnails) {
            models.thumbnailModel =
                factory.createThumbnailModel(models.documentModel);
        }
        if (m_data->enableBookmarks) {
            models.bookmarkModel =
                factory.createBookmarkModel(models.documentModel);
        }
        if (m_data->enableAnnotations) {
            models.annotationModel =
                factory.createAnnotationModel(models.documentModel);
        }
        if (m_data->enableSearch) {
            models.searchModel =
                factory.createSearchModel(models.documentModel);
        }
        if (m_data->enableOutline) {
            models.outlineModel =
                factory.createPDFOutlineModel(models.documentModel);
        }
        if (m_data->enableAsyncLoading) {
            models.documentLoader =
                factory.createAsyncDocumentLoader(models.documentModel);
        }
    }

    return models;
}

std::unique_ptr<ModelFactory::ModelSet> ModelBuilder::buildUnique() {
    return std::make_unique<ModelFactory::ModelSet>(build());
}
