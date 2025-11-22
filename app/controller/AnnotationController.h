#pragma once

#include <poppler-qt6.h>
#include <QColor>
#include <QObject>
#include <QPointer>
#include <QRectF>
#include <QString>
#include "../model/AnnotationModel.h"

// Forward declarations
class CacheManager;
class DocumentController;
class CommandManager;

/**
 * @brief Controller for managing PDF annotations
 *
 * Provides business logic layer for annotation operations, integrating with:
 * - AnnotationModel for data management
 * - CommandManager for undo/redo support
 * - EventBus for event-driven communication
 * - CacheManager for persistence
 * - ServiceLocator for dependency injection
 *
 * Follows the project's MVC architecture and command pattern.
 */
class AnnotationController : public QObject {
    Q_OBJECT

public:
    explicit AnnotationController(QObject* parent = nullptr);
    ~AnnotationController() override = default;

    // Singleton access (registered with ServiceLocator)
    static AnnotationController* instance();

    // Model access
    AnnotationModel* model() const { return m_model; }

    // Document management
    void setDocument(Poppler::Document* document, const QString& filePath);
    void clearDocument();
    bool hasDocument() const;
    QString currentFilePath() const { return m_currentFilePath; }

    // Annotation operations (execute via CommandManager)
    bool addAnnotation(const PDFAnnotation& annotation);
    bool removeAnnotation(const QString& annotationId);
    bool updateAnnotation(const QString& annotationId,
                          const PDFAnnotation& updatedAnnotation);
    bool moveAnnotation(const QString& annotationId,
                        const QPointF& newPosition);
    bool resizeAnnotation(const QString& annotationId,
                          const QRectF& newBoundary);

    // Property modifications
    bool changeAnnotationColor(const QString& annotationId,
                               const QColor& newColor);
    bool changeAnnotationOpacity(const QString& annotationId, double opacity);
    bool editAnnotationContent(const QString& annotationId,
                               const QString& newContent);
    bool toggleAnnotationVisibility(const QString& annotationId);

    // Batch operations
    bool removeAnnotationsForPage(int pageNumber);
    bool clearAllAnnotations();
    bool batchAddAnnotations(const QList<PDFAnnotation>& annotations);

    // Quick annotation creation
    bool addHighlight(int pageNumber, const QRectF& boundingRect,
                      const QString& text, const QColor& color = Qt::yellow);
    bool addNote(int pageNumber, const QPointF& position,
                 const QString& content, const QColor& color = Qt::yellow);
    bool addShape(AnnotationType shapeType, int pageNumber,
                  const QRectF& boundingRect, const QColor& color = Qt::blue);

    // Persistence
    bool saveAnnotations();
    bool loadAnnotations();
    bool exportAnnotations(const QString& filePath,
                           const QString& format = "json");
    bool importAnnotations(const QString& filePath,
                           const QString& format = "json");

    // Cache management
    bool saveAnnotationsToCache();
    bool loadAnnotationsFromCache();
    void clearAnnotationsCache();

    // Query operations
    QList<PDFAnnotation> getAnnotationsForPage(int pageNumber) const;
    PDFAnnotation getAnnotation(const QString& annotationId) const;
    QList<PDFAnnotation> searchAnnotations(const QString& query) const;
    QList<PDFAnnotation> getAnnotationsByType(AnnotationType type) const;
    int getTotalAnnotationCount() const;
    int getAnnotationCountForPage(int pageNumber) const;

    // Settings
    void setDefaultAuthor(const QString& author) { m_defaultAuthor = author; }
    QString defaultAuthor() const { return m_defaultAuthor; }
    void setAutoSave(bool enabled) { m_autoSaveEnabled = enabled; }
    bool isAutoSaveEnabled() const { return m_autoSaveEnabled; }

signals:
    // Document signals
    void documentChanged();
    void documentCleared();

    // Annotation signals (forwarded from model + EventBus integration)
    void annotationAdded(const PDFAnnotation& annotation);
    void annotationRemoved(const QString& annotationId);
    void annotationUpdated(const PDFAnnotation& annotation);
    void annotationsLoaded(int count);
    void annotationsSaved(int count);
    void annotationsCleared();

    // Operation status
    void operationCompleted(bool success, const QString& message);
    void error(const QString& errorMessage);

private slots:
    void onAnnotationAdded(const PDFAnnotation& annotation);
    void onAnnotationRemoved(const QString& annotationId);
    void onAnnotationUpdated(const PDFAnnotation& annotation);

private:
    void setupConnections();
    void publishEvent(const QString& eventName, const QVariant& data);
    QString getCacheKey() const;
    bool validateAnnotation(const PDFAnnotation& annotation) const;

    // Dependencies
    AnnotationModel* m_model;
    QPointer<Poppler::Document> m_document;
    QString m_currentFilePath;

    // Settings
    QString m_defaultAuthor;
    bool m_autoSaveEnabled;

    // Singleton instance
    static AnnotationController* s_instance;
};
