#include "AnnotationRenderDelegate.h"
#include <QBrush>
#include <QFont>
#include <QPainterPath>
#include <QPen>
#include "../controller/AnnotationController.h"
#include "../logging/SimpleLogging.h"

AnnotationRenderDelegate::AnnotationRenderDelegate(QObject* parent)
    : QObject(parent),
      m_controller(nullptr),
      m_showSelectionHandles(true),
      m_highlightSelected(true) {}

void AnnotationRenderDelegate::setController(AnnotationController* controller) {
    m_controller = controller;
}

void AnnotationRenderDelegate::renderAnnotations(QPainter* painter,
                                                 int pageNumber,
                                                 const QRectF& pageRect,
                                                 double zoomFactor) {
    if (!m_controller || !painter) {
        return;
    }

    painter->save();

    QList<PDFAnnotation> annotations =
        m_controller->getAnnotationsForPage(pageNumber);

    int renderedCount = 0;
    for (const PDFAnnotation& annotation : annotations) {
        if (!annotation.isVisible) {
            continue;
        }

        renderAnnotation(painter, annotation, pageRect, zoomFactor);
        renderedCount++;
    }

    painter->restore();

    emit renderingCompleted(pageNumber, renderedCount);
}

void AnnotationRenderDelegate::renderAnnotation(QPainter* painter,
                                                const PDFAnnotation& annotation,
                                                const QRectF& pageRect,
                                                double zoomFactor) {
    if (!painter) {
        return;
    }

    painter->save();

    // Apply opacity
    painter->setOpacity(annotation.opacity);

    // Render based on type
    switch (annotation.type) {
        case AnnotationType::Highlight:
            renderHighlight(painter, annotation, zoomFactor);
            break;
        case AnnotationType::Note:
            renderNote(painter, annotation, zoomFactor);
            break;
        case AnnotationType::FreeText:
            renderFreeText(painter, annotation, zoomFactor);
            break;
        case AnnotationType::Underline:
            renderUnderline(painter, annotation, zoomFactor);
            break;
        case AnnotationType::StrikeOut:
            renderStrikeOut(painter, annotation, zoomFactor);
            break;
        case AnnotationType::Squiggly:
            renderSquiggly(painter, annotation, zoomFactor);
            break;
        case AnnotationType::Rectangle:
            renderRectangle(painter, annotation, zoomFactor);
            break;
        case AnnotationType::Circle:
            renderCircle(painter, annotation, zoomFactor);
            break;
        case AnnotationType::Line:
            renderLine(painter, annotation, zoomFactor);
            break;
        case AnnotationType::Arrow:
            renderArrow(painter, annotation, zoomFactor);
            break;
        case AnnotationType::Ink:
            renderInk(painter, annotation, zoomFactor);
            break;
        default:
            SLOG_WARNING_F("Unknown annotation type: {}",
                           static_cast<int>(annotation.type));
            break;
    }

    // Render selection if this annotation is selected
    if (m_highlightSelected && annotation.id == m_selectedAnnotationId) {
        QRectF scaledRect = scaleRect(annotation.boundingRect, zoomFactor);
        renderSelectionBorder(painter, scaledRect, zoomFactor);
        if (m_showSelectionHandles) {
            renderResizeHandles(painter, scaledRect, zoomFactor);
        }
    }

    painter->restore();
}

void AnnotationRenderDelegate::renderHighlight(QPainter* painter,
                                               const PDFAnnotation& annotation,
                                               double zoom) {
    QRectF rect = scaleRect(annotation.boundingRect, zoom);
    QBrush brush = createBrush(annotation);
    painter->fillRect(rect, brush);
}

void AnnotationRenderDelegate::renderNote(QPainter* painter,
                                          const PDFAnnotation& annotation,
                                          double zoom) {
    QRectF rect = scaleRect(annotation.boundingRect, zoom);

    // Draw note icon (sticky note)
    QPen pen(annotation.color, 1.5 * zoom);
    painter->setPen(pen);
    QBrush brush(annotation.color);
    painter->setBrush(brush);

    // Draw folded corner effect
    QPainterPath path;
    path.moveTo(rect.topLeft());
    path.lineTo(rect.right() - 5 * zoom, rect.top());
    path.lineTo(rect.right(), rect.top() + 5 * zoom);
    path.lineTo(rect.right(), rect.bottom());
    path.lineTo(rect.left(), rect.bottom());
    path.closeSubpath();

    painter->drawPath(path);

    // Draw fold triangle
    QPainterPath fold;
    fold.moveTo(rect.right() - 5 * zoom, rect.top());
    fold.lineTo(rect.right() - 5 * zoom, rect.top() + 5 * zoom);
    fold.lineTo(rect.right(), rect.top() + 5 * zoom);
    fold.closeSubpath();
    painter->fillPath(fold, annotation.color.darker(120));
}

void AnnotationRenderDelegate::renderFreeText(QPainter* painter,
                                              const PDFAnnotation& annotation,
                                              double zoom) {
    QRectF rect = scaleRect(annotation.boundingRect, zoom);

    // Draw background
    QBrush brush(QColor(255, 255, 200, 200));  // Light yellow background
    painter->fillRect(rect, brush);

    // Draw border
    QPen pen(annotation.color, 1.0 * zoom);
    painter->setPen(pen);
    painter->drawRect(rect);

    // Draw text
    QFont font(annotation.fontFamily, annotation.fontSize * zoom);
    painter->setFont(font);
    painter->setPen(Qt::black);
    painter->drawText(rect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                      annotation.content);
}

void AnnotationRenderDelegate::renderUnderline(QPainter* painter,
                                               const PDFAnnotation& annotation,
                                               double zoom) {
    QRectF rect = scaleRect(annotation.boundingRect, zoom);
    QPen pen = createPen(annotation, zoom);
    painter->setPen(pen);

    // Draw line at bottom of rect
    painter->drawLine(rect.bottomLeft(), rect.bottomRight());
}

void AnnotationRenderDelegate::renderStrikeOut(QPainter* painter,
                                               const PDFAnnotation& annotation,
                                               double zoom) {
    QRectF rect = scaleRect(annotation.boundingRect, zoom);
    QPen pen = createPen(annotation, zoom);
    painter->setPen(pen);

    // Draw line through middle
    QPointF left(rect.left(), rect.center().y());
    QPointF right(rect.right(), rect.center().y());
    painter->drawLine(left, right);
}

void AnnotationRenderDelegate::renderSquiggly(QPainter* painter,
                                              const PDFAnnotation& annotation,
                                              double zoom) {
    QRectF rect = scaleRect(annotation.boundingRect, zoom);
    QPen pen = createPen(annotation, zoom);
    painter->setPen(pen);

    // Draw squiggly line at bottom
    QPainterPath path;
    double x = rect.left();
    double y = rect.bottom();
    double amplitude = 2 * zoom;
    double wavelength = 4 * zoom;

    path.moveTo(x, y);
    while (x < rect.right()) {
        x += wavelength / 2;
        y = static_cast<int>(x / wavelength) % 2 == 0
                ? rect.bottom() - amplitude
                : rect.bottom() + amplitude;
        path.lineTo(x, y);
    }

    painter->drawPath(path);
}

void AnnotationRenderDelegate::renderRectangle(QPainter* painter,
                                               const PDFAnnotation& annotation,
                                               double zoom) {
    QRectF rect = scaleRect(annotation.boundingRect, zoom);
    QPen pen = createPen(annotation, zoom);
    painter->setPen(pen);
    QBrush brush = createBrush(annotation);
    brush.setStyle(Qt::NoBrush);  // No fill for shapes by default
    painter->setBrush(brush);

    painter->drawRect(rect);
}

void AnnotationRenderDelegate::renderCircle(QPainter* painter,
                                            const PDFAnnotation& annotation,
                                            double zoom) {
    QRectF rect = scaleRect(annotation.boundingRect, zoom);
    QPen pen = createPen(annotation, zoom);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    painter->drawEllipse(rect);
}

void AnnotationRenderDelegate::renderLine(QPainter* painter,
                                          const PDFAnnotation& annotation,
                                          double zoom) {
    QPointF start = scalePoint(annotation.startPoint, zoom);
    QPointF end = scalePoint(annotation.endPoint, zoom);
    QPen pen = createPen(annotation, zoom);
    painter->setPen(pen);

    painter->drawLine(start, end);
}

void AnnotationRenderDelegate::renderArrow(QPainter* painter,
                                           const PDFAnnotation& annotation,
                                           double zoom) {
    QPointF start = scalePoint(annotation.startPoint, zoom);
    QPointF end = scalePoint(annotation.endPoint, zoom);
    QPen pen = createPen(annotation, zoom);
    painter->setPen(pen);
    painter->setBrush(QBrush(annotation.color));

    // Draw line
    painter->drawLine(start, end);

    // Draw arrowhead
    double arrowSize = 10 * zoom;
    double angle = std::atan2(end.y() - start.y(), end.x() - start.x());

    QPointF arrowP1 = end - QPointF(std::cos(angle + M_PI / 6) * arrowSize,
                                    std::sin(angle + M_PI / 6) * arrowSize);
    QPointF arrowP2 = end - QPointF(std::cos(angle - M_PI / 6) * arrowSize,
                                    std::sin(angle - M_PI / 6) * arrowSize);

    QPainterPath arrowHead;
    arrowHead.moveTo(end);
    arrowHead.lineTo(arrowP1);
    arrowHead.lineTo(arrowP2);
    arrowHead.closeSubpath();

    painter->drawPath(arrowHead);
    painter->fillPath(arrowHead, QBrush(annotation.color));
}

void AnnotationRenderDelegate::renderInk(QPainter* painter,
                                         const PDFAnnotation& annotation,
                                         double zoom) {
    if (annotation.inkPath.isEmpty()) {
        return;
    }

    QPen pen = createPen(annotation, zoom);
    painter->setPen(pen);

    QPainterPath path;
    path.moveTo(scalePoint(annotation.inkPath.first(), zoom));
    for (int i = 1; i < annotation.inkPath.size(); ++i) {
        path.lineTo(scalePoint(annotation.inkPath.at(i), zoom));
    }

    painter->drawPath(path);
}

void AnnotationRenderDelegate::renderSelectionBorder(QPainter* painter,
                                                     const QRectF& rect,
                                                     double zoom) {
    QPen pen(QColor(0, 120, 215), 2 * zoom);  // Blue selection border
    pen.setStyle(Qt::DashLine);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(rect);
}

void AnnotationRenderDelegate::renderResizeHandles(QPainter* painter,
                                                   const QRectF& rect,
                                                   double zoom) {
    double handleSize = 8 * zoom;
    QPen pen(QColor(0, 120, 215), 1 * zoom);
    painter->setPen(pen);
    painter->setBrush(QBrush(Qt::white));

    // Draw 8 resize handles (corners and midpoints)
    QList<QPointF> handlePositions = {
        rect.topLeft(),     QPointF(rect.center().x(), rect.top()),
        rect.topRight(),    QPointF(rect.right(), rect.center().y()),
        rect.bottomRight(), QPointF(rect.center().x(), rect.bottom()),
        rect.bottomLeft(),  QPointF(rect.left(), rect.center().y())};

    for (const QPointF& pos : handlePositions) {
        QRectF handleRect(pos.x() - handleSize / 2, pos.y() - handleSize / 2,
                          handleSize, handleSize);
        painter->drawRect(handleRect);
    }
}

void AnnotationRenderDelegate::setSelectedAnnotationId(
    const QString& annotationId) {
    m_selectedAnnotationId = annotationId;
}

void AnnotationRenderDelegate::clearSelection() {
    m_selectedAnnotationId.clear();
}

QRectF AnnotationRenderDelegate::scaleRect(const QRectF& rect,
                                           double zoom) const {
    return QRectF(rect.x() * zoom, rect.y() * zoom, rect.width() * zoom,
                  rect.height() * zoom);
}

QPointF AnnotationRenderDelegate::scalePoint(const QPointF& point,
                                             double zoom) const {
    return QPointF(point.x() * zoom, point.y() * zoom);
}

QPen AnnotationRenderDelegate::createPen(const PDFAnnotation& annotation,
                                         double zoom) const {
    QPen pen(annotation.color);
    pen.setWidthF(annotation.lineWidth * zoom);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    return pen;
}

QBrush AnnotationRenderDelegate::createBrush(
    const PDFAnnotation& annotation) const {
    QColor color = adjustColorOpacity(annotation.color, annotation.opacity);
    return QBrush(color);
}

QColor AnnotationRenderDelegate::adjustColorOpacity(const QColor& color,
                                                    double opacity) const {
    QColor adjusted = color;
    adjusted.setAlphaF(opacity);
    return adjusted;
}
