#include "SearchConfiguration.h"
#include <QTransform>
#include <QtMath>

void SearchResult::transformToWidgetCoordinates(double scaleFactor, int rotation,
                                              const QSizeF& pageSize, const QSize& widgetSize) {
    if (boundingRect.isEmpty()) {
        widgetRect = QRectF();
        return;
    }

    // Start with PDF coordinates (in points, origin at bottom-left)
    QRectF pdfRect = boundingRect;

    // Convert from PDF coordinate system (bottom-left origin) to Qt coordinate system (top-left origin)
    QRectF qtRect;
    qtRect.setLeft(pdfRect.left());
    qtRect.setTop(pageSize.height() - pdfRect.bottom());
    qtRect.setWidth(pdfRect.width());
    qtRect.setHeight(pdfRect.height());

    // Apply rotation transformation
    QTransform transform;
    QPointF center(pageSize.width() / 2.0, pageSize.height() / 2.0);

    switch (rotation) {
        case 90:
            transform.translate(center.x(), center.y());
            transform.rotate(90);
            transform.translate(-center.y(), -center.x());
            break;
        case 180:
            transform.translate(center.x(), center.y());
            transform.rotate(180);
            transform.translate(-center.x(), -center.y());
            break;
        case 270:
            transform.translate(center.x(), center.y());
            transform.rotate(270);
            transform.translate(-center.y(), -center.x());
            break;
        default:
            // No rotation (0 degrees)
            break;
    }

    // Apply transformation if rotation is needed
    if (rotation != 0) {
        qtRect = transform.mapRect(qtRect);
    }

    // Scale to widget coordinates
    double scaleX = static_cast<double>(widgetSize.width()) / pageSize.width();
    double scaleY = static_cast<double>(widgetSize.height()) / pageSize.height();

    // Apply uniform scaling (maintain aspect ratio)
    double uniformScale = qMin(scaleX, scaleY) * scaleFactor;

    widgetRect.setLeft(qtRect.left() * uniformScale);
    widgetRect.setTop(qtRect.top() * uniformScale);
    widgetRect.setWidth(qtRect.width() * uniformScale);
    widgetRect.setHeight(qtRect.height() * uniformScale);

    // Center the result if aspect ratios don't match
    if (scaleX != scaleY) {
        double offsetX = (widgetSize.width() - pageSize.width() * uniformScale) / 2.0;
        double offsetY = (widgetSize.height() - pageSize.height() * uniformScale) / 2.0;
        widgetRect.translate(offsetX, offsetY);
    }
}
