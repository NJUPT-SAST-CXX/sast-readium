#pragma once

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QString>

// Forward declarations
class AnnotationController;
class AnnotationRenderDelegate;
class PDFAnnotation;

/**
 * @brief Manages annotation selection and interactive editing
 *
 * Handles user interaction with annotations including selection,
 * moving, and resizing through mouse/touch events.
 */
class AnnotationSelectionManager : public QObject {
    Q_OBJECT

public:
    enum class HandlePosition {
        None,
        TopLeft,
        TopCenter,
        TopRight,
        CenterRight,
        BottomRight,
        BottomCenter,
        BottomLeft,
        CenterLeft,
        Inside  // Inside annotation for moving
    };

    explicit AnnotationSelectionManager(QObject* parent = nullptr);
    ~AnnotationSelectionManager() override = default;

    // Controller and delegate
    void setController(AnnotationController* controller);
    void setRenderDelegate(AnnotationRenderDelegate* delegate);

    AnnotationController* controller() const { return m_controller; }
    AnnotationRenderDelegate* renderDelegate() const {
        return m_renderDelegate;
    }

    // Selection management
    void selectAnnotation(const QString& annotationId);
    void clearSelection();
    bool hasSelection() const { return !m_selectedAnnotationId.isEmpty(); }
    QString selectedAnnotationId() const { return m_selectedAnnotationId; }

    // Hit testing
    QString findAnnotationAt(const QPointF& point, int pageNumber) const;
    HandlePosition findResizeHandle(const QPointF& point, double zoom) const;

    // Interaction state
    bool isInteracting() const { return m_isInteracting; }
    bool isMoving() const { return m_isMoving; }
    bool isResizing() const { return m_isResizing; }

    // Mouse/touch event handling
    bool handleMousePress(const QPointF& point, int pageNumber, double zoom);
    bool handleMouseMove(const QPointF& point, double zoom);
    bool handleMouseRelease(const QPointF& point, double zoom);

    // Settings
    void setHandleSize(double size) { m_handleSize = size; }
    double handleSize() const { return m_handleSize; }

    void setHitTolerance(double tolerance) { m_hitTolerance = tolerance; }
    double hitTolerance() const { return m_hitTolerance; }

signals:
    void selectionChanged(const QString& annotationId);
    void selectionCleared();
    void annotationMoved(const QString& annotationId,
                         const QPointF& newPosition);
    void annotationResized(const QString& annotationId,
                           const QRectF& newBoundary);
    void interactionStarted();
    void interactionEnded();

private:
    bool isPointInRect(const QPointF& point, const QRectF& rect,
                       double tolerance = 0.0) const;
    QRectF calculateNewBoundary(const QRectF& original, HandlePosition handle,
                                const QPointF& delta) const;
    QPointF constrainPoint(const QPointF& point) const;

    AnnotationController* m_controller;
    AnnotationRenderDelegate* m_renderDelegate;

    // Selection state
    QString m_selectedAnnotationId;
    int m_selectedPageNumber;

    // Interaction state
    bool m_isInteracting;
    bool m_isMoving;
    bool m_isResizing;
    HandlePosition m_currentHandle;

    // Interaction data
    QPointF m_interactionStartPoint;
    QRectF m_originalBoundary;
    QPointF m_originalPosition;

    // Settings
    double m_handleSize;    // Size of resize handles
    double m_hitTolerance;  // Tolerance for hit testing
};
