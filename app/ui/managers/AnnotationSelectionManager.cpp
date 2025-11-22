#include "AnnotationSelectionManager.h"
#include "../../controller/AnnotationController.h"
#include "../../delegate/AnnotationRenderDelegate.h"
#include "../../logging/SimpleLogging.h"

AnnotationSelectionManager::AnnotationSelectionManager(QObject* parent)
    : QObject(parent),
      m_controller(nullptr),
      m_renderDelegate(nullptr),
      m_selectedPageNumber(-1),
      m_isInteracting(false),
      m_isMoving(false),
      m_isResizing(false),
      m_currentHandle(HandlePosition::None),
      m_handleSize(8.0),
      m_hitTolerance(5.0) {}

void AnnotationSelectionManager::setController(
    AnnotationController* controller) {
    m_controller = controller;
}

void AnnotationSelectionManager::setRenderDelegate(
    AnnotationRenderDelegate* delegate) {
    m_renderDelegate = delegate;
}

void AnnotationSelectionManager::selectAnnotation(const QString& annotationId) {
    if (m_selectedAnnotationId == annotationId) {
        return;
    }

    m_selectedAnnotationId = annotationId;

    if (m_renderDelegate) {
        m_renderDelegate->setSelectedAnnotationId(annotationId);
    }

    if (!annotationId.isEmpty() && m_controller) {
        PDFAnnotation annotation = m_controller->getAnnotation(annotationId);
        m_selectedPageNumber = annotation.pageNumber;
        SLOG_DEBUG_F("Selected annotation: {}", annotationId);
    }

    emit selectionChanged(annotationId);
}

void AnnotationSelectionManager::clearSelection() {
    if (m_selectedAnnotationId.isEmpty()) {
        return;
    }

    m_selectedAnnotationId.clear();
    m_selectedPageNumber = -1;

    if (m_renderDelegate) {
        m_renderDelegate->clearSelection();
    }

    SLOG_DEBUG("Cleared annotation selection");
    emit selectionCleared();
}

QString AnnotationSelectionManager::findAnnotationAt(const QPointF& point,
                                                     int pageNumber) const {
    if (!m_controller) {
        return QString();
    }

    // Get annotations for the page
    QList<PDFAnnotation> annotations =
        m_controller->getAnnotationsForPage(pageNumber);

    // Search in reverse order (top-most annotation first)
    for (int i = annotations.size() - 1; i >= 0; --i) {
        const PDFAnnotation& annotation = annotations.at(i);
        if (!annotation.isVisible) {
            continue;
        }

        // Check if point is inside annotation's bounding rect with tolerance
        if (isPointInRect(point, annotation.boundingRect, m_hitTolerance)) {
            return annotation.id;
        }
    }

    return QString();
}

AnnotationSelectionManager::HandlePosition
AnnotationSelectionManager::findResizeHandle(const QPointF& point,
                                             double zoom) const {
    if (m_selectedAnnotationId.isEmpty() || !m_controller) {
        return HandlePosition::None;
    }

    PDFAnnotation annotation =
        m_controller->getAnnotation(m_selectedAnnotationId);
    if (annotation.id.isEmpty()) {
        return HandlePosition::None;
    }

    QRectF rect = annotation.boundingRect;
    double handleSize = m_handleSize / zoom;  // Scale handle size with zoom
    double tolerance = m_hitTolerance / zoom;

    // Check each handle position
    QList<QPair<HandlePosition, QPointF>> handles = {
        {HandlePosition::TopLeft, rect.topLeft()},
        {HandlePosition::TopCenter, QPointF(rect.center().x(), rect.top())},
        {HandlePosition::TopRight, rect.topRight()},
        {HandlePosition::CenterRight, QPointF(rect.right(), rect.center().y())},
        {HandlePosition::BottomRight, rect.bottomRight()},
        {HandlePosition::BottomCenter,
         QPointF(rect.center().x(), rect.bottom())},
        {HandlePosition::BottomLeft, rect.bottomLeft()},
        {HandlePosition::CenterLeft, QPointF(rect.left(), rect.center().y())}};

    for (const auto& handle : handles) {
        QRectF handleRect(handle.second.x() - handleSize / 2,
                          handle.second.y() - handleSize / 2, handleSize,
                          handleSize);
        if (isPointInRect(point, handleRect, tolerance)) {
            return handle.first;
        }
    }

    // Check if inside annotation (for moving)
    if (isPointInRect(point, rect, 0.0)) {
        return HandlePosition::Inside;
    }

    return HandlePosition::None;
}

bool AnnotationSelectionManager::handleMousePress(const QPointF& point,
                                                  int pageNumber, double zoom) {
    if (!m_controller) {
        return false;
    }

    // First, check if clicking on a resize handle of selected annotation
    if (!m_selectedAnnotationId.isEmpty()) {
        HandlePosition handle = findResizeHandle(point, zoom);
        if (handle != HandlePosition::None) {
            m_isInteracting = true;
            m_interactionStartPoint = point;

            if (handle == HandlePosition::Inside) {
                // Start moving
                m_isMoving = true;
                m_isResizing = false;
                PDFAnnotation annotation =
                    m_controller->getAnnotation(m_selectedAnnotationId);
                m_originalPosition = annotation.boundingRect.topLeft();
                SLOG_DEBUG_F("Started moving annotation: {}",
                             m_selectedAnnotationId);
            } else {
                // Start resizing
                m_isResizing = true;
                m_isMoving = false;
                m_currentHandle = handle;
                PDFAnnotation annotation =
                    m_controller->getAnnotation(m_selectedAnnotationId);
                m_originalBoundary = annotation.boundingRect;
                SLOG_DEBUG_F("Started resizing annotation: {}",
                             m_selectedAnnotationId);
            }

            emit interactionStarted();
            return true;
        }
    }

    // Try to select annotation at click point
    QString annotationId = findAnnotationAt(point, pageNumber);
    if (!annotationId.isEmpty()) {
        selectAnnotation(annotationId);
        return true;
    }

    // Click on empty space - clear selection
    clearSelection();
    return false;
}

bool AnnotationSelectionManager::handleMouseMove(const QPointF& point,
                                                 double zoom) {
    if (!m_isInteracting || m_selectedAnnotationId.isEmpty() || !m_controller) {
        return false;
    }

    QPointF delta = point - m_interactionStartPoint;

    if (m_isMoving) {
        // Update annotation position
        QPointF newPosition = m_originalPosition + delta;
        PDFAnnotation annotation =
            m_controller->getAnnotation(m_selectedAnnotationId);
        QSizeF size = annotation.boundingRect.size();
        annotation.boundingRect = QRectF(newPosition, size);

        // Note: Don't update model yet, just track the change
        // Actual update happens on mouse release via command
        return true;

    } else if (m_isResizing) {
        // Calculate new boundary based on handle and delta
        QRectF newBoundary =
            calculateNewBoundary(m_originalBoundary, m_currentHandle, delta);

        // Ensure minimum size
        if (newBoundary.width() < 10.0 / zoom ||
            newBoundary.height() < 10.0 / zoom) {
            return true;  // Reject too small
        }

        // Note: Don't update model yet, just track the change
        // Actual update happens on mouse release via command
        return true;
    }

    return false;
}

bool AnnotationSelectionManager::handleMouseRelease(const QPointF& point,
                                                    double zoom) {
    if (!m_isInteracting || m_selectedAnnotationId.isEmpty() || !m_controller) {
        return false;
    }

    QPointF delta = point - m_interactionStartPoint;
    bool changed = false;

    if (m_isMoving && (qAbs(delta.x()) > 1.0 || qAbs(delta.y()) > 1.0)) {
        // Apply move via controller (which will use command for undo/redo)
        QPointF newPosition = m_originalPosition + delta;
        m_controller->moveAnnotation(m_selectedAnnotationId, newPosition);
        emit annotationMoved(m_selectedAnnotationId, newPosition);
        SLOG_DEBUG_F("Moved annotation: {}", m_selectedAnnotationId);
        changed = true;

    } else if (m_isResizing) {
        // Apply resize via controller (which will use command for undo/redo)
        QRectF newBoundary =
            calculateNewBoundary(m_originalBoundary, m_currentHandle, delta);

        // Ensure minimum size
        if (newBoundary.width() >= 10.0 / zoom &&
            newBoundary.height() >= 10.0 / zoom) {
            m_controller->resizeAnnotation(m_selectedAnnotationId, newBoundary);
            emit annotationResized(m_selectedAnnotationId, newBoundary);
            SLOG_DEBUG_F("Resized annotation: {}", m_selectedAnnotationId);
            changed = true;
        }
    }

    // Reset interaction state
    m_isInteracting = false;
    m_isMoving = false;
    m_isResizing = false;
    m_currentHandle = HandlePosition::None;

    emit interactionEnded();
    return changed;
}

bool AnnotationSelectionManager::isPointInRect(const QPointF& point,
                                               const QRectF& rect,
                                               double tolerance) const {
    QRectF expandedRect =
        rect.adjusted(-tolerance, -tolerance, tolerance, tolerance);
    return expandedRect.contains(point);
}

QRectF AnnotationSelectionManager::calculateNewBoundary(
    const QRectF& original, HandlePosition handle, const QPointF& delta) const {
    QRectF result = original;

    switch (handle) {
        case HandlePosition::TopLeft:
            result.setTopLeft(original.topLeft() + delta);
            break;
        case HandlePosition::TopCenter:
            result.setTop(original.top() + delta.y());
            break;
        case HandlePosition::TopRight:
            result.setTopRight(original.topRight() + delta);
            break;
        case HandlePosition::CenterRight:
            result.setRight(original.right() + delta.x());
            break;
        case HandlePosition::BottomRight:
            result.setBottomRight(original.bottomRight() + delta);
            break;
        case HandlePosition::BottomCenter:
            result.setBottom(original.bottom() + delta.y());
            break;
        case HandlePosition::BottomLeft:
            result.setBottomLeft(original.bottomLeft() + delta);
            break;
        case HandlePosition::CenterLeft:
            result.setLeft(original.left() + delta.x());
            break;
        default:
            break;
    }

    // Ensure rect is normalized (positive width/height)
    return result.normalized();
}

QPointF AnnotationSelectionManager::constrainPoint(const QPointF& point) const {
    // Optional: Add page boundary constraints here
    return point;
}
