#include "AnnotationInteractionHandler.h"
#include <QDateTime>
#include <QPainter>
#include <QtMath>

AnnotationInteractionHandler::AnnotationInteractionHandler(QObject* parent)
    : QObject(parent),
      m_currentMode(None),
      m_isDrawing(false),
      m_currentPage(-1),
      m_currentColor(Qt::yellow),
      m_lineWidth(2.0),
      m_opacity(0.5) {}

void AnnotationInteractionHandler::setDrawMode(DrawMode mode) {
    if (m_currentMode != mode) {
        if (m_isDrawing) {
            cancelDrawing();
        }
        m_currentMode = mode;
        emit modeChanged(mode);
    }
}

void AnnotationInteractionHandler::startDrawing(const QPointF& point,
                                                int pageNumber) {
    if (m_currentMode == None || m_isDrawing) {
        return;
    }

    m_isDrawing = true;
    m_currentPage = pageNumber;
    m_startPoint = point;
    m_currentPoint = point;
    m_drawingPoints.clear();
    m_drawingPoints.append(point);

    updatePreview();
    emit previewUpdated();
}

void AnnotationInteractionHandler::continueDrawing(const QPointF& point) {
    if (!m_isDrawing) {
        return;
    }

    m_currentPoint = point;

    // For freehand drawing, add all points
    if (m_currentMode == FreehandDraw) {
        m_drawingPoints.append(point);
    }

    updatePreview();
    emit previewUpdated();
}

void AnnotationInteractionHandler::finishDrawing(const QPointF& point) {
    if (!m_isDrawing) {
        return;
    }

    m_currentPoint = point;

    // Create the annotation
    PDFAnnotation annotation = createAnnotationFromDrawing();

    // Reset state
    m_isDrawing = false;
    m_drawingPoints.clear();
    m_previewPath = QPainterPath();
    m_previewRect = QRectF();

    emit annotationCreated(annotation);
    emit previewUpdated();
}

void AnnotationInteractionHandler::cancelDrawing() {
    if (!m_isDrawing) {
        return;
    }

    m_isDrawing = false;
    m_drawingPoints.clear();
    m_previewPath = QPainterPath();
    m_previewRect = QRectF();

    emit drawingCancelled();
    emit previewUpdated();
}

void AnnotationInteractionHandler::updatePreview() {
    m_previewPath = QPainterPath();
    m_previewRect = QRectF();

    if (!m_isDrawing) {
        return;
    }

    switch (m_currentMode) {
        case Highlight:
        case Rectangle: {
            // Draw rectangle from start to current point
            m_previewRect = QRectF(m_startPoint, m_currentPoint).normalized();
            m_previewPath.addRect(m_previewRect);
            break;
        }

        case Circle: {
            // Draw ellipse from start to current point
            m_previewRect = QRectF(m_startPoint, m_currentPoint).normalized();
            m_previewPath.addEllipse(m_previewRect);
            break;
        }

        case Line:
        case Underline:
        case StrikeOut: {
            // Draw line from start to current point
            m_previewPath.moveTo(m_startPoint);
            m_previewPath.lineTo(m_currentPoint);
            m_previewRect = QRectF(m_startPoint, m_currentPoint).normalized();
            break;
        }

        case Arrow: {
            // Draw arrow from start to current point
            m_previewPath.moveTo(m_startPoint);
            m_previewPath.lineTo(m_currentPoint);

            // Calculate arrowhead
            QLineF line(m_startPoint, m_currentPoint);
            qreal angle = line.angle();
            qreal arrowSize = 10.0;

            QPointF arrowP1 =
                m_currentPoint +
                QPointF(qCos(qDegreesToRadians(angle + 150)) * arrowSize,
                        -qSin(qDegreesToRadians(angle + 150)) * arrowSize);
            QPointF arrowP2 =
                m_currentPoint +
                QPointF(qCos(qDegreesToRadians(angle - 150)) * arrowSize,
                        -qSin(qDegreesToRadians(angle - 150)) * arrowSize);

            m_previewPath.moveTo(m_currentPoint);
            m_previewPath.lineTo(arrowP1);
            m_previewPath.moveTo(m_currentPoint);
            m_previewPath.lineTo(arrowP2);

            m_previewRect = m_previewPath.boundingRect();
            break;
        }

        case FreehandDraw: {
            // Draw smooth curve through points
            if (m_drawingPoints.size() > 1) {
                m_previewPath.moveTo(m_drawingPoints.first());
                for (int i = 1; i < m_drawingPoints.size(); ++i) {
                    m_previewPath.lineTo(m_drawingPoints[i]);
                }
            }
            m_previewRect = m_previewPath.boundingRect();
            break;
        }

        case Text: {
            // Text annotation area
            m_previewRect = QRectF(m_startPoint, QSizeF(200, 100));
            m_previewPath.addRect(m_previewRect);
            break;
        }

        default:
            break;
    }
}

PDFAnnotation AnnotationInteractionHandler::createAnnotationFromDrawing() {
    PDFAnnotation annotation;
    annotation.pageNumber = m_currentPage;
    annotation.color = m_currentColor;
    annotation.opacity = m_opacity;
    annotation.creationDate = QDateTime::currentDateTime();
    annotation.modificationDate = annotation.creationDate;
    annotation.author = "User";  // Should get from settings

    QRectF bounds = calculateBoundingRect();
    annotation.boundary = bounds;

    switch (m_currentMode) {
        case Highlight:
            annotation.type = PDFAnnotation::Highlight;
            annotation.content = "Highlight";
            break;

        case Underline:
            annotation.type = PDFAnnotation::Underline;
            annotation.content = "Underline";
            break;

        case StrikeOut:
            annotation.type = PDFAnnotation::StrikeOut;
            annotation.content = "Strike Out";
            break;

        case Rectangle:
            annotation.type = PDFAnnotation::Square;
            annotation.lineWidth = m_lineWidth;
            annotation.content = "Rectangle";
            break;

        case Circle:
            annotation.type = PDFAnnotation::Circle;
            annotation.lineWidth = m_lineWidth;
            annotation.content = "Circle";
            break;

        case Line:
            annotation.type = PDFAnnotation::Line;
            annotation.lineWidth = m_lineWidth;
            annotation.startPoint = m_startPoint;
            annotation.endPoint = m_currentPoint;
            annotation.content = "Line";
            break;

        case Arrow:
            annotation.type = PDFAnnotation::Line;
            annotation.lineWidth = m_lineWidth;
            annotation.startPoint = m_startPoint;
            annotation.endPoint = m_currentPoint;
            annotation.hasArrow = true;
            annotation.content = "Arrow";
            break;

        case FreehandDraw:
            annotation.type = PDFAnnotation::Ink;
            annotation.lineWidth = m_lineWidth;
            annotation.inkPaths.append(m_drawingPoints);
            annotation.content = "Freehand Drawing";
            break;

        case Text:
            annotation.type = PDFAnnotation::FreeText;
            annotation.content = "";  // Will be filled by text dialog
            annotation.boundary = QRectF(m_startPoint, QSizeF(200, 100));
            break;

        default:
            annotation.type = PDFAnnotation::Note;
            annotation.content = "Note";
            break;
    }

    return annotation;
}

QRectF AnnotationInteractionHandler::calculateBoundingRect() const {
    if (m_drawingPoints.isEmpty()) {
        return QRectF(m_startPoint, m_currentPoint).normalized();
    }

    QRectF rect;
    for (const QPointF& point : m_drawingPoints) {
        if (rect.isNull()) {
            rect = QRectF(point, point);
        } else {
            rect = rect.united(QRectF(point, point));
        }
    }

    // Add padding for line width
    qreal padding = m_lineWidth / 2.0;
    rect.adjust(-padding, -padding, padding, padding);

    return rect.normalized();
}
