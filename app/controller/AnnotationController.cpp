#include "AnnotationController.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "../cache/CacheManager.h"
#include "../command/CommandManager.h"
#include "../controller/DocumentController.h"
#include "../controller/EventBus.h"
#include "../controller/ServiceLocator.h"
#include "../logging/SimpleLogging.h"

AnnotationController* AnnotationController::s_instance = nullptr;

AnnotationController::AnnotationController(QObject* parent)
    : QObject(parent),
      m_model(new AnnotationModel(this)),
      m_document(nullptr),
      m_defaultAuthor("User"),
      m_autoSaveEnabled(true) {
    setupConnections();

    SLOG_INFO("AnnotationController initialized");
}

AnnotationController* AnnotationController::instance() {
    static AnnotationController instanceObj;
    s_instance = &instanceObj;
    return s_instance;
}

void AnnotationController::setupConnections() {
    // Forward model signals
    connect(m_model, &AnnotationModel::annotationAdded, this,
            &AnnotationController::onAnnotationAdded);
    connect(m_model, &AnnotationModel::annotationRemoved, this,
            &AnnotationController::onAnnotationRemoved);
    connect(m_model, &AnnotationModel::annotationUpdated, this,
            &AnnotationController::onAnnotationUpdated);
    connect(m_model, &AnnotationModel::annotationsLoaded, this,
            &AnnotationController::annotationsLoaded);
    connect(m_model, &AnnotationModel::annotationsSaved, this,
            &AnnotationController::annotationsSaved);
    connect(m_model, &AnnotationModel::annotationsCleared, this,
            &AnnotationController::annotationsCleared);
}

void AnnotationController::setDocument(Poppler::Document* document,
                                       const QString& filePath) {
    if (m_document == document && m_currentFilePath == filePath) {
        return;
    }

    // Save current annotations if auto-save is enabled
    if (m_autoSaveEnabled && !m_currentFilePath.isEmpty()) {
        if (!saveAnnotations()) {
            emit error(
                "Failed to auto-save annotations before changing document");
        }
    }

    m_document = document;
    m_currentFilePath = filePath;
    m_model->setDocument(document);

    // Load annotations from cache or document
    if (!filePath.isEmpty()) {
        if (!loadAnnotationsFromCache()) {
            loadAnnotations();
        }
    }

    emit documentChanged();
    publishEvent("annotation.document_changed", filePath);

    SLOG_INFO_F("Document set for annotations: {}", filePath);
}

void AnnotationController::clearDocument() {
    // Save before clearing if auto-save is enabled
    if (m_autoSaveEnabled && !m_currentFilePath.isEmpty()) {
        if (!saveAnnotations()) {
            emit error(
                "Failed to auto-save annotations before clearing document");
        }
    }

    m_document = nullptr;
    m_currentFilePath.clear();
    m_model->clearAnnotations();

    emit documentCleared();
    publishEvent("annotation.document_cleared", QVariant());

    SLOG_INFO("Annotation document cleared");
}

bool AnnotationController::hasDocument() const {
    return !m_document.isNull() && !m_currentFilePath.isEmpty();
}

bool AnnotationController::addAnnotation(const PDFAnnotation& annotation) {
    if (!validateAnnotation(annotation)) {
        SLOG_WARNING("Invalid annotation, cannot add");
        emit error("Invalid annotation");
        return false;
    }

    // Set author if not set
    PDFAnnotation ann = annotation;
    if (ann.author.isEmpty()) {
        ann.author = m_defaultAuthor;
    }

    bool success = m_model->addAnnotation(ann);
    if (success) {
        // Auto-save if enabled
        if (m_autoSaveEnabled) {
            saveAnnotationsToCache();
        }
        emit operationCompleted(true, "Annotation added successfully");
    } else {
        emit error("Failed to add annotation");
    }

    return success;
}

bool AnnotationController::removeAnnotation(const QString& annotationId) {
    if (annotationId.isEmpty()) {
        SLOG_WARNING("Cannot remove annotation with empty ID");
        return false;
    }

    bool success = m_model->removeAnnotation(annotationId);
    if (success) {
        // Auto-save if enabled
        if (m_autoSaveEnabled) {
            saveAnnotationsToCache();
        }
        emit operationCompleted(true, "Annotation removed successfully");
    } else {
        emit error("Failed to remove annotation");
    }

    return success;
}

bool AnnotationController::updateAnnotation(
    const QString& annotationId, const PDFAnnotation& updatedAnnotation) {
    if (!validateAnnotation(updatedAnnotation)) {
        SLOG_WARNING("Invalid annotation, cannot update");
        emit error("Invalid annotation");
        return false;
    }

    bool success = m_model->updateAnnotation(annotationId, updatedAnnotation);
    if (success) {
        // Auto-save if enabled
        if (m_autoSaveEnabled) {
            saveAnnotationsToCache();
        }
        emit operationCompleted(true, "Annotation updated successfully");
    } else {
        emit error("Failed to update annotation");
    }

    return success;
}

bool AnnotationController::moveAnnotation(const QString& annotationId,
                                          const QPointF& newPosition) {
    bool success = m_model->moveAnnotation(annotationId, newPosition);
    if (success && m_autoSaveEnabled) {
        saveAnnotationsToCache();
    }
    return success;
}

bool AnnotationController::resizeAnnotation(const QString& annotationId,
                                            const QRectF& newBoundary) {
    bool success = m_model->resizeAnnotation(annotationId, newBoundary);
    if (success && m_autoSaveEnabled) {
        saveAnnotationsToCache();
    }
    return success;
}

bool AnnotationController::changeAnnotationColor(const QString& annotationId,
                                                 const QColor& newColor) {
    bool success = m_model->changeAnnotationColor(annotationId, newColor);
    if (success && m_autoSaveEnabled) {
        saveAnnotationsToCache();
    }
    return success;
}

bool AnnotationController::changeAnnotationOpacity(const QString& annotationId,
                                                   double opacity) {
    bool success = m_model->changeAnnotationOpacity(annotationId, opacity);
    if (success && m_autoSaveEnabled) {
        saveAnnotationsToCache();
    }
    return success;
}

bool AnnotationController::editAnnotationContent(const QString& annotationId,
                                                 const QString& newContent) {
    bool success = m_model->editAnnotationContent(annotationId, newContent);
    if (success && m_autoSaveEnabled) {
        saveAnnotationsToCache();
    }
    return success;
}

bool AnnotationController::toggleAnnotationVisibility(
    const QString& annotationId) {
    PDFAnnotation annotation = m_model->getAnnotation(annotationId);
    if (annotation.id.isEmpty()) {
        return false;
    }

    annotation.isVisible = !annotation.isVisible;
    return updateAnnotation(annotationId, annotation);
}

bool AnnotationController::removeAnnotationsForPage(int pageNumber) {
    bool success = m_model->removeAnnotationsForPage(pageNumber);
    if (success && m_autoSaveEnabled) {
        saveAnnotationsToCache();
    }
    return success;
}

bool AnnotationController::clearAllAnnotations() {
    m_model->clearAnnotations();
    if (m_autoSaveEnabled) {
        saveAnnotationsToCache();
    }
    emit operationCompleted(true, "All annotations cleared");
    return true;
}

bool AnnotationController::batchAddAnnotations(
    const QList<PDFAnnotation>& annotations) {
    int successCount = 0;
    for (const PDFAnnotation& annotation : annotations) {
        if (m_model->addAnnotation(annotation)) {
            successCount++;
        }
    }

    if (successCount > 0) {
        if (m_autoSaveEnabled) {
            saveAnnotationsToCache();
        }
        emit operationCompleted(
            true, QString("Added %1 annotations").arg(successCount));
        return true;
    }

    emit error("Failed to add annotations");
    return false;
}

bool AnnotationController::addHighlight(int pageNumber,
                                        const QRectF& boundingRect,
                                        const QString& text,
                                        const QColor& color) {
    PDFAnnotation annotation;
    annotation.type = AnnotationType::Highlight;
    annotation.pageNumber = pageNumber;
    annotation.boundingRect = boundingRect;
    annotation.content = text;
    annotation.color = color;
    annotation.opacity = 0.4;
    annotation.author = m_defaultAuthor;
    annotation.createdTime = QDateTime::currentDateTime();
    annotation.modifiedTime = annotation.createdTime;

    return addAnnotation(annotation);
}

bool AnnotationController::addNote(int pageNumber, const QPointF& position,
                                   const QString& content,
                                   const QColor& color) {
    bool success = m_model->addStickyNote(pageNumber, position, content, color);

    if (success) {
        if (m_autoSaveEnabled) {
            saveAnnotationsToCache();
        }
        emit operationCompleted(true, "Note annotation added successfully");
    } else {
        emit error("Failed to add note annotation");
    }

    return success;
}

bool AnnotationController::addShape(AnnotationType shapeType, int pageNumber,
                                    const QRectF& boundingRect,
                                    const QColor& color) {
    PDFAnnotation annotation;
    annotation.type = shapeType;
    annotation.pageNumber = pageNumber;
    annotation.boundingRect = boundingRect;
    annotation.color = color;
    annotation.opacity = 1.0;
    annotation.lineWidth = 2.0;
    annotation.author = m_defaultAuthor;
    annotation.createdTime = QDateTime::currentDateTime();
    annotation.modifiedTime = annotation.createdTime;

    return addAnnotation(annotation);
}

bool AnnotationController::saveAnnotations() {
    if (!hasDocument()) {
        SLOG_WARNING("No document loaded, cannot save annotations");
        return false;
    }

    bool success = m_model->saveAnnotationsToDocument();
    if (success) {
        saveAnnotationsToCache();
        SLOG_INFO_F("Saved annotations to document: {}", m_currentFilePath);
    } else {
        SLOG_ERROR_F("Failed to save annotations to document: {}",
                     m_currentFilePath);
    }

    return success;
}

bool AnnotationController::loadAnnotations() {
    if (!hasDocument()) {
        SLOG_WARNING("No document loaded, cannot load annotations");
        return false;
    }

    bool success = m_model->loadAnnotationsFromDocument();
    if (success) {
        SLOG_INFO_F("Loaded annotations from document: {}", m_currentFilePath);
    } else {
        SLOG_WARNING_F("Failed to load annotations from document: {}",
                       m_currentFilePath);
    }

    return success;
}

bool AnnotationController::exportAnnotations(const QString& filePath,
                                             const QString& format) {
    if (format.toLower() != "json") {
        SLOG_WARNING_F("Unsupported export format: {}", format);
        emit error(QString("Unsupported export format: %1").arg(format));
        return false;
    }

    QJsonArray annotationsArray;
    for (const PDFAnnotation& annotation : m_model->getAllAnnotations()) {
        annotationsArray.append(annotation.toJson());
    }

    QJsonObject root;
    root["version"] = "1.0";
    root["document"] = m_currentFilePath;
    root["exportDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["annotationCount"] = annotationsArray.size();
    root["annotations"] = annotationsArray;

    QJsonDocument doc(root);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        SLOG_ERROR_F("Failed to open file for export: {}", filePath);
        emit error(QString("Failed to open file: %1").arg(filePath));
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    SLOG_INFO_F("Exported {} annotations to {}", annotationsArray.size(),
                filePath);
    emit operationCompleted(
        true, QString("Exported %1 annotations").arg(annotationsArray.size()));
    return true;
}

bool AnnotationController::importAnnotations(const QString& filePath,
                                             const QString& format) {
    if (format.toLower() != "json") {
        SLOG_WARNING_F("Unsupported import format: {}", format);
        emit error(QString("Unsupported import format: %1").arg(format));
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        SLOG_ERROR_F("Failed to open file for import: {}", filePath);
        emit error(QString("Failed to open file: %1").arg(filePath));
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        SLOG_ERROR("Invalid JSON format");
        emit error("Invalid JSON format");
        return false;
    }

    QJsonObject root = doc.object();
    QJsonArray annotationsArray = root["annotations"].toArray();

    QList<PDFAnnotation> annotations;
    for (const QJsonValue& value : annotationsArray) {
        PDFAnnotation annotation = PDFAnnotation::fromJson(value.toObject());
        annotations.append(annotation);
    }

    bool success = batchAddAnnotations(annotations);
    if (success) {
        SLOG_INFO_F("Imported {} annotations from {}", annotations.size(),
                    filePath);
    }

    return success;
}

bool AnnotationController::saveAnnotationsToCache() {
    if (!hasDocument()) {
        return false;
    }

    // Use sidecar file for annotation persistence
    QString annotationFile = m_currentFilePath + ".annotations.json";

    QJsonArray annotationsArray;
    for (const PDFAnnotation& annotation : m_model->getAllAnnotations()) {
        annotationsArray.append(annotation.toJson());
    }

    QJsonObject root;
    root["version"] = "1.0";
    root["document"] = m_currentFilePath;
    root["annotationCount"] = annotationsArray.size();
    root["annotations"] = annotationsArray;

    QJsonDocument doc(root);
    QFile file(annotationFile);
    if (!file.open(QIODevice::WriteOnly)) {
        SLOG_WARNING_F("Failed to save annotations to cache: {}",
                       annotationFile);
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Compact));
    file.close();

    SLOG_DEBUG_F("Saved {} annotations to cache file: {}",
                 annotationsArray.size(), annotationFile);
    return true;
}

bool AnnotationController::loadAnnotationsFromCache() {
    if (!hasDocument()) {
        return false;
    }

    // Load from sidecar file
    QString annotationFile = m_currentFilePath + ".annotations.json";
    QFile file(annotationFile);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return false;  // No cached annotations found
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        SLOG_WARNING("Invalid cached annotation data format");
        return false;
    }

    QJsonObject root = doc.object();
    QJsonArray annotationsArray = root["annotations"].toArray();

    QList<PDFAnnotation> annotations;
    for (const QJsonValueRef& value : annotationsArray) {
        PDFAnnotation annotation = PDFAnnotation::fromJson(value.toObject());
        annotations.append(annotation);
    }

    m_model->clearAnnotations();
    bool success = batchAddAnnotations(annotations);
    if (success) {
        SLOG_INFO_F("Loaded {} annotations from cache file: {}",
                    annotations.size(), annotationFile);
    }

    return success;
}

void AnnotationController::clearAnnotationsCache() {
    if (!hasDocument()) {
        return;
    }

    // Remove sidecar file
    QString annotationFile = m_currentFilePath + ".annotations.json";
    QFile file(annotationFile);
    if (file.exists() && file.remove()) {
        SLOG_DEBUG_F("Cleared annotations cache file: {}", annotationFile);
    }
}

QList<PDFAnnotation> AnnotationController::getAnnotationsForPage(
    int pageNumber) const {
    return m_model->getAnnotationsForPage(pageNumber);
}

PDFAnnotation AnnotationController::getAnnotation(
    const QString& annotationId) const {
    return m_model->getAnnotation(annotationId);
}

QList<PDFAnnotation> AnnotationController::searchAnnotations(
    const QString& query) const {
    return m_model->searchAnnotations(query);
}

QList<PDFAnnotation> AnnotationController::getAnnotationsByType(
    AnnotationType type) const {
    return m_model->getAnnotationsByType(type);
}

int AnnotationController::getTotalAnnotationCount() const {
    return m_model->getTotalAnnotationCount();
}

int AnnotationController::getAnnotationCountForPage(int pageNumber) const {
    return m_model->getAnnotationCountForPage(pageNumber);
}

void AnnotationController::onAnnotationAdded(const PDFAnnotation& annotation) {
    emit annotationAdded(annotation);
    publishEvent("annotation.added", QVariant::fromValue(annotation.id));
}

void AnnotationController::onAnnotationRemoved(const QString& annotationId) {
    emit annotationRemoved(annotationId);
    publishEvent("annotation.removed", annotationId);
}

void AnnotationController::onAnnotationUpdated(
    const PDFAnnotation& annotation) {
    emit annotationUpdated(annotation);
    publishEvent("annotation.updated", QVariant::fromValue(annotation.id));
}

void AnnotationController::publishEvent(const QString& eventName,
                                        const QVariant& data) {
    EventBus::instance().publish(eventName, data);
}

QString AnnotationController::getCacheKey() const {
    return QString("annotations_%1")
        .arg(QString(QCryptographicHash::hash(m_currentFilePath.toUtf8(),
                                              QCryptographicHash::Md5)
                         .toHex()));
}

bool AnnotationController::validateAnnotation(
    const PDFAnnotation& annotation) const {
    if (annotation.id.isEmpty()) {
        SLOG_WARNING("Annotation ID is empty");
        return false;
    }

    if (annotation.pageNumber < 0) {
        SLOG_WARNING_F("Invalid page number: {}", annotation.pageNumber);
        return false;
    }

    if (!annotation.boundingRect.isValid()) {
        SLOG_WARNING("Invalid bounding rectangle");
        return false;
    }

    return true;
}
