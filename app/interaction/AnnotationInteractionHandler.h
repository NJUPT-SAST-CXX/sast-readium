#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QColor>
#include <QList>
#include <QObject>
#include <QPainterPath>
#include <QPointF>
#include <QRectF>
#include "../model/AnnotationModel.h"

/**
 * @brief Handles interactive annotation drawing operations
 *
 * Manages the creation and manipulation of annotations through
 * user interaction (mouse/touch events).
 */
class AnnotationInteractionHandler : public QObject {
    Q_OBJECT

public:
    enum DrawMode {
        None,
        Highlight,
        Underline,
        StrikeOut,
        Rectangle,
        Circle,
        FreehandDraw,
        Arrow,
        Line,
        Text
    };

    explicit AnnotationInteractionHandler(QObject* parent = nullptr);
    ~AnnotationInteractionHandler() = default;

    // Mode management
    void setDrawMode(DrawMode mode);
    DrawMode getDrawMode() const { return m_currentMode; }
    bool isDrawing() const { return m_isDrawing; }

    // Drawing state
    void startDrawing(const QPointF& point, int pageNumber);
    void continueDrawing(const QPointF& point);
    void finishDrawing(const QPointF& point);
    void cancelDrawing();

    // Properties
    void setColor(const QColor& color) { m_currentColor = color; }
    QColor getColor() const { return m_currentColor; }
    void setLineWidth(qreal width) { m_lineWidth = width; }
    qreal getLineWidth() const { return m_lineWidth; }
    void setOpacity(qreal opacity) { m_opacity = opacity; }
    qreal getOpacity() const { return m_opacity; }

    // Preview
    QPainterPath getPreviewPath() const { return m_previewPath; }
    QRectF getPreviewRect() const { return m_previewRect; }
    bool hasPreview() const { return m_isDrawing; }

signals:
    void annotationCreated(const PDFAnnotation& annotation);
    void previewUpdated();
    void drawingCancelled();
    void modeChanged(DrawMode mode);

private:
    PDFAnnotation createAnnotationFromDrawing();
    void updatePreview();
    QRectF calculateBoundingRect() const;

    DrawMode m_currentMode;
    bool m_isDrawing;
    int m_currentPage;
    QColor m_currentColor;
    qreal m_lineWidth;
    qreal m_opacity;

    // Drawing data
    QPointF m_startPoint;
    QPointF m_currentPoint;
    QList<QPointF> m_drawingPoints;
    QPainterPath m_previewPath;
    QRectF m_previewRect;
};
