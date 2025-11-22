#ifndef ANNOTATIONINTEGRATIONHELPER_H
#define ANNOTATIONINTEGRATIONHELPER_H

#include <QObject>
#include <QString>
#include <memory>

// Forward declarations
class AnnotationController;
class AnnotationRenderDelegate;
class AnnotationSelectionManager;
class AnnotationToolbar;
class AnnotationsPanel;
class PDFViewer;
class PDFViewerPage;
class RightSideBar;
class QUndoStack;
class QPainter;

namespace Poppler {
class Document;
}

/**
 * @brief AnnotationIntegrationHelper - Annotation system integration helper
 *
 * This class provides a centralized integration point for the annotation
 * system, connecting:
 * - AnnotationController (business logic)
 * - AnnotationRenderDelegate (rendering)
 * - AnnotationSelectionManager (selection/interaction)
 * - AnnotationToolbar (UI controls)
 * - AnnotationsPanel (annotation list)
 * - PDFViewer (rendering integration)
 *
 * Usage:
 * @code
 * // In PDFViewerPage constructor:
 * m_annotationHelper = new AnnotationIntegrationHelper(this);
 * m_annotationHelper->initialize();
 *
 * // When document is loaded:
 * m_annotationHelper->setDocument(document.get(), filePath);
 * m_annotationHelper->attachToPDFViewer(m_pdfViewer);
 * m_annotationHelper->attachToRightSideBar(m_rightSideBar);
 *
 * // When document is closed:
 * m_annotationHelper->detachFromPDFViewer();
 * m_annotationHelper->clearDocument();
 * @endcode
 *
 * Architecture integration:
 * - Uses ServiceLocator to get AnnotationController
 * - Follows MVC pattern (Controller, View, Delegate)
 * - Integrates with EventBus for decoupled communication
 * - Uses Command Pattern for undo/redo (via AnnotationController)
 */
class AnnotationIntegrationHelper : public QObject {
    Q_OBJECT

public:
    explicit AnnotationIntegrationHelper(QObject* parent = nullptr);
    ~AnnotationIntegrationHelper() override;

    // ========================================================================
    // Initialization
    // ========================================================================

    /**
     * @brief Initialize the annotation system components
     *
     * This should be called once during application startup.
     * It retrieves the AnnotationController from ServiceLocator and
     * initializes all annotation components.
     */
    bool initialize();

    // ========================================================================
    // Document Management
    // ========================================================================

    /**
     * @brief Set the current document
     * @param document Poppler document
     * @param filePath Path to the PDF file
     */
    void setDocument(Poppler::Document* document, const QString& filePath);

    /**
     * @brief Clear the current document
     */
    void clearDocument();

    /**
     * @brief Check if a document is loaded
     */
    bool hasDocument() const;

    // ========================================================================
    // Component Integration
    // ========================================================================

    /**
     * @brief Attach to PDFViewer for rendering and mouse event handling
     * @param viewer The PDFViewer instance
     *
     * This installs an event filter on the viewer to intercept mouse events
     * for annotation interaction.
     */
    void attachToPDFViewer(PDFViewer* viewer);

    /**
     * @brief Detach from PDFViewer
     */
    void detachFromPDFViewer();

    /**
     * @brief Attach to RightSideBar to connect AnnotationsPanel
     * @param sidebar The RightSideBar instance
     */
    void attachToRightSideBar(RightSideBar* sidebar);

    /**
     * @brief Attach toolbar for annotation tool selection
     * @param toolbar The AnnotationToolbar instance
     */
    void attachToolbar(AnnotationToolbar* toolbar);

    // ========================================================================
    // Rendering Integration
    // ========================================================================

    /**
     * @brief Render annotations for a specific page
     *
     * This should be called from PageWidget::paintEvent() after drawing the PDF
     * page.
     *
     * @param painter QPainter for drawing
     * @param pageNumber Page number (0-based)
     * @param pageRect Page rectangle in widget coordinates
     * @param zoomFactor Current zoom factor
     */
    void renderAnnotations(QPainter* painter, int pageNumber,
                           const QRectF& pageRect, double zoomFactor);

    // ========================================================================
    // Mouse Event Handling
    // ========================================================================

    /**
     * @brief Handle mouse press event
     * @param point Point in page coordinates
     * @param pageNumber Page number (0-based)
     * @param zoomFactor Current zoom factor
     * @return true if event was handled
     */
    bool handleMousePress(const QPointF& point, int pageNumber,
                          double zoomFactor);

    /**
     * @brief Handle mouse move event
     * @param point Point in page coordinates
     * @param zoomFactor Current zoom factor
     * @return true if event was handled
     */
    bool handleMouseMove(const QPointF& point, double zoomFactor);

    /**
     * @brief Handle mouse release event
     * @param point Point in page coordinates
     * @param zoomFactor Current zoom factor
     * @return true if event was handled
     */
    bool handleMouseRelease(const QPointF& point, double zoomFactor);

    // ========================================================================
    // Component Access
    // ========================================================================

    /**
     * @brief Get the annotation controller
     */
    AnnotationController* controller() const { return m_controller; }

    /**
     * @brief Get the render delegate
     */
    AnnotationRenderDelegate* renderDelegate() const {
        return m_renderDelegate.get();
    }

    /**
     * @brief Get the selection manager
     */
    AnnotationSelectionManager* selectionManager() const {
        return m_selectionManager.get();
    }

signals:
    /**
     * @brief Emitted when an annotation is selected
     */
    void annotationSelected(const QString& annotationId);

    /**
     * @brief Emitted when selection is cleared
     */
    void selectionCleared();

    /**
     * @brief Emitted when annotations change (for triggering repaints)
     */
    void annotationsChanged();

private slots:
    // Internal signal handlers
    void onAnnotationAdded(const QString& annotationId);
    void onAnnotationRemoved(const QString& annotationId);
    void onAnnotationModified(const QString& annotationId);
    void onSelectionChanged(const QString& annotationId);

private:
    // Core annotation components
    AnnotationController* m_controller;
    std::unique_ptr<AnnotationRenderDelegate> m_renderDelegate;
    std::unique_ptr<AnnotationSelectionManager> m_selectionManager;

    // Attached UI components
    PDFViewer* m_pdfViewer;
    RightSideBar* m_rightSideBar;
    AnnotationToolbar* m_annotationToolbar;
    AnnotationsPanel* m_annotationsPanel;

    // State
    bool m_initialized;
    QString m_currentFilePath;

    // Helper methods
    void connectSignals();
    void disconnectSignals();
    bool eventFilter(QObject* obj, QEvent* event) override;
};

#endif  // ANNOTATIONINTEGRATIONHELPER_H
