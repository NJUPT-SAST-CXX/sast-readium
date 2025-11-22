#pragma once

#include <QObject>
#include <QPainter>
#include <QRectF>
#include "../model/AnnotationModel.h"

// Forward declarations
class AnnotationController;

/**
 * @brief Delegate for rendering annotations on PDF pages
 *
 * Handles the visual rendering of annotations overlaid on PDF content.
 * Supports all annotation types with proper styling and transparency.
 */
class AnnotationRenderDelegate : public QObject {
    Q_OBJECT

public:
    explicit AnnotationRenderDelegate(QObject* parent = nullptr);
    ~AnnotationRenderDelegate() override = default;

    // Controller management
    void setController(AnnotationController* controller);
    AnnotationController* controller() const { return m_controller; }

    // Rendering
    void renderAnnotations(QPainter* painter, int pageNumber,
                           const QRectF& pageRect, double zoomFactor);
    void renderAnnotation(QPainter* painter, const PDFAnnotation& annotation,
                          const QRectF& pageRect, double zoomFactor);

    // Selection rendering
    void setSelectedAnnotationId(const QString& annotationId);
    QString selectedAnnotationId() const { return m_selectedAnnotationId; }
    void clearSelection();

    // Rendering options
    void setShowSelectionHandles(bool show) { m_showSelectionHandles = show; }
    bool showSelectionHandles() const { return m_showSelectionHandles; }

    void setHighlightSelected(bool highlight) {
        m_highlightSelected = highlight;
    }
    bool highlightSelected() const { return m_highlightSelected; }

signals:
    void renderingCompleted(int pageNumber, int annotationCount);
    void annotationHovered(const QString& annotationId);

private:
    // Type-specific rendering
    void renderHighlight(QPainter* painter, const PDFAnnotation& annotation,
                         double zoom);
    void renderNote(QPainter* painter, const PDFAnnotation& annotation,
                    double zoom);
    void renderFreeText(QPainter* painter, const PDFAnnotation& annotation,
                        double zoom);
    void renderUnderline(QPainter* painter, const PDFAnnotation& annotation,
                         double zoom);
    void renderStrikeOut(QPainter* painter, const PDFAnnotation& annotation,
                         double zoom);
    void renderSquiggly(QPainter* painter, const PDFAnnotation& annotation,
                        double zoom);
    void renderRectangle(QPainter* painter, const PDFAnnotation& annotation,
                         double zoom);
    void renderCircle(QPainter* painter, const PDFAnnotation& annotation,
                      double zoom);
    void renderLine(QPainter* painter, const PDFAnnotation& annotation,
                    double zoom);
    void renderArrow(QPainter* painter, const PDFAnnotation& annotation,
                     double zoom);
    void renderInk(QPainter* painter, const PDFAnnotation& annotation,
                   double zoom);

    // Selection rendering
    void renderSelectionBorder(QPainter* painter, const QRectF& rect,
                               double zoom);
    void renderResizeHandles(QPainter* painter, const QRectF& rect,
                             double zoom);

    // Helpers
    QRectF scaleRect(const QRectF& rect, double zoom) const;
    QPointF scalePoint(const QPointF& point, double zoom) const;
    QPen createPen(const PDFAnnotation& annotation, double zoom) const;
    QBrush createBrush(const PDFAnnotation& annotation) const;
    QColor adjustColorOpacity(const QColor& color, double opacity) const;

    AnnotationController* m_controller;
    QString m_selectedAnnotationId;
    bool m_showSelectionHandles;
    bool m_highlightSelected;
};
