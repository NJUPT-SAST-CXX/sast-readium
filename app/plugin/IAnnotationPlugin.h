#pragma once

#include <QColor>
#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QPainter>
#include <QRect>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include "model/AnnotationModel.h"

/**
 * @brief Annotation Data
 *
 * Contains annotation properties and content.
 */
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

    AnnotationData()
        : type(AnnotationType::Highlight), pageNumber(-1), color(Qt::yellow) {}
};

/**
 * @brief IAnnotationPlugin - Interface for annotation plugins
 *
 * Plugins implementing this interface can provide custom annotation types,
 * import/export formats, and collaborative annotation features.
 */
class IAnnotationPlugin {
public:
    virtual ~IAnnotationPlugin() = default;

    /**
     * @brief Get supported annotation types
     */
    virtual QList<AnnotationType> supportedTypes() const = 0;

    /**
     * @brief Create annotation
     * @param data Annotation data
     * @param documentPath Document path
     * @return True if annotation was created successfully
     */
    virtual bool createAnnotation(const AnnotationData& data,
                                  const QString& documentPath) = 0;

    /**
     * @brief Update annotation
     * @param annotationId Annotation ID
     * @param data Updated annotation data
     * @param documentPath Document path
     * @return True if annotation was updated successfully
     */
    virtual bool updateAnnotation(const QString& annotationId,
                                  const AnnotationData& data,
                                  const QString& documentPath) = 0;

    /**
     * @brief Delete annotation
     * @param annotationId Annotation ID
     * @param documentPath Document path
     * @return True if annotation was deleted successfully
     */
    virtual bool deleteAnnotation(const QString& annotationId,
                                  const QString& documentPath) = 0;

    /**
     * @brief Get annotations for page
     * @param pageNumber Page number (0-based)
     * @param documentPath Document path
     * @return List of annotations on this page
     */
    virtual QList<AnnotationData> getAnnotationsForPage(
        int pageNumber, const QString& documentPath) const = 0;

    /**
     * @brief Export annotations to file
     * @param documentPath Document path
     * @param outputPath Output file path
     * @param format Export format (e.g., "json", "xfdf", "xml")
     * @return True if export succeeded
     */
    virtual bool exportAnnotations(const QString& documentPath,
                                   const QString& outputPath,
                                   const QString& format) = 0;

    /**
     * @brief Import annotations from file
     * @param inputPath Input file path
     * @param documentPath Target document path
     * @param format Import format
     * @return Number of annotations imported
     */
    virtual int importAnnotations(const QString& inputPath,
                                  const QString& documentPath,
                                  const QString& format) = 0;

    /**
     * @brief Render annotation on page
     * @param painter QPainter to draw with
     * @param annotation Annotation to render
     * @param pageRect Page rectangle
     * @param zoom Current zoom level
     */
    virtual void renderAnnotation(QPainter* painter,
                                  const AnnotationData& annotation,
                                  const QRect& pageRect, double zoom) = 0;
};

Q_DECLARE_INTERFACE(IAnnotationPlugin, "com.sast.readium.IAnnotationPlugin/1.0")
