#include "AnnotationIntegrationHelper.h"

// Annotation system
#include "controller/AnnotationController.h"
#include "controller/ServiceLocator.h"
#include "delegate/AnnotationRenderDelegate.h"
#include "ui/managers/AnnotationSelectionManager.h"
#include "ui/widgets/AnnotationToolbar.h"
#include "ui/widgets/AnnotationsPanel.h"

// PDF viewer components
#include "ui/core/RightSideBar.h"
#include "ui/viewer/PDFViewer.h"

// Logging
#include "logging/SimpleLogging.h"

// Qt
#include <poppler/qt6/poppler-qt6.h>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>

// ============================================================================
// Constructor and Destructor
// ============================================================================

AnnotationIntegrationHelper::AnnotationIntegrationHelper(QObject* parent)
    : QObject(parent),
      m_controller(nullptr),
      m_pdfViewer(nullptr),
      m_rightSideBar(nullptr),
      m_annotationToolbar(nullptr),
      m_annotationsPanel(nullptr),
      m_initialized(false) {
    SLOG_INFO("AnnotationIntegrationHelper: Constructor");
}

AnnotationIntegrationHelper::~AnnotationIntegrationHelper() {
    SLOG_INFO("AnnotationIntegrationHelper: Destructor");
    detachFromPDFViewer();
    disconnectSignals();
}

// ============================================================================
// Initialization
// ============================================================================

bool AnnotationIntegrationHelper::initialize() {
    if (m_initialized) {
        SLOG_WARNING("AnnotationIntegrationHelper: Already initialized");
        return true;
    }

    SLOG_INFO("AnnotationIntegrationHelper: Initializing");

    // Get AnnotationController from ServiceLocator
    auto& serviceLocator = ServiceLocator::instance();
    m_controller = serviceLocator.getService<AnnotationController>();

    if (!m_controller) {
        SLOG_ERROR(
            "AnnotationIntegrationHelper: AnnotationController not found in "
            "ServiceLocator");
        SLOG_ERROR(
            "AnnotationIntegrationHelper: Make sure "
            "initializeAnnotationSystem() "
            "was called in main.cpp");
        return false;
    }

    // Create render delegate
    m_renderDelegate = std::make_unique<AnnotationRenderDelegate>(this);
    m_renderDelegate->setAnnotationController(m_controller);

    // Create selection manager
    m_selectionManager = std::make_unique<AnnotationSelectionManager>(this);
    m_selectionManager->setController(m_controller);
    m_selectionManager->setRenderDelegate(m_renderDelegate.get());

    // Connect signals
    connectSignals();

    m_initialized = true;
    SLOG_INFO("AnnotationIntegrationHelper: Initialization successful");
    return true;
}

// ========================================================================
// Document Management
// ========================================================================

void AnnotationIntegrationHelper::setDocument(Poppler::Document* document,
                                              const QString& filePath) {
    if (!m_initialized) {
        SLOG_ERROR(
            "AnnotationIntegrationHelper: Not initialized. Call initialize() "
            "first.");
        return;
    }

    if (!document || filePath.isEmpty()) {
        SLOG_WARNING(
            "AnnotationIntegrationHelper: Invalid document or file path");
        clearDocument();
        return;
    }

    SLOG_INFO_F("AnnotationIntegrationHelper: Setting document: {}",
                filePath.toStdString());

    m_currentFilePath = filePath;

    // Set document in controller
    m_controller->setDocument(document, filePath);

    // Load annotations from sidecar file
    m_controller->loadAnnotationsFromCache();

    SLOG_INFO_F("AnnotationIntegrationHelper: Loaded {} annotations",
                m_controller->getTotalAnnotationCount());

    emit annotationsChanged();
}

void AnnotationIntegrationHelper::clearDocument() {
    SLOG_INFO("AnnotationIntegrationHelper: Clearing document");

    m_currentFilePath.clear();

    if (m_controller) {
        m_controller->clearDocument();
    }

    if (m_selectionManager) {
        m_selectionManager->clearSelection();
    }

    emit annotationsChanged();
}

bool AnnotationIntegrationHelper::hasDocument() const {
    return !m_currentFilePath.isEmpty();
}

// ========================================================================
// Component Integration
// ========================================================================

void AnnotationIntegrationHelper::attachToPDFViewer(PDFViewer* viewer) {
    if (m_pdfViewer == viewer) {
        return;
    }

    if (m_pdfViewer) {
        detachFromPDFViewer();
    }

    m_pdfViewer = viewer;

    if (m_pdfViewer) {
        // Install event filter to intercept mouse events
        m_pdfViewer->installEventFilter(this);
        SLOG_INFO("AnnotationIntegrationHelper: Attached to PDFViewer");
    }
}

void AnnotationIntegrationHelper::detachFromPDFViewer() {
    if (m_pdfViewer) {
        m_pdfViewer->removeEventFilter(this);
        m_pdfViewer = nullptr;
        SLOG_INFO("AnnotationIntegrationHelper: Detached from PDFViewer");
    }
}

void AnnotationIntegrationHelper::attachToRightSideBar(RightSideBar* sidebar) {
    m_rightSideBar = sidebar;

    if (m_rightSideBar) {
        // Get AnnotationsPanel from RightSideBar
        m_annotationsPanel = m_rightSideBar->annotationsPanel();

        if (m_annotationsPanel) {
            // Connect panel signals
            connect(m_annotationsPanel, &AnnotationsPanel::annotationSelected,
                    this, [this](const QString& annotationId) {
                        // Select annotation in selection manager
                        if (m_selectionManager) {
                            m_selectionManager->selectAnnotation(annotationId);
                        }
                        emit annotationSelected(annotationId);
                    });

            SLOG_INFO(
                "AnnotationIntegrationHelper: Attached to RightSideBar and "
                "AnnotationsPanel");
        }
    }
}

void AnnotationIntegrationHelper::attachToolbar(AnnotationToolbar* toolbar) {
    m_annotationToolbar = toolbar;

    if (m_annotationToolbar) {
        // TODO: Connect toolbar signals for tool selection
        // connect(m_annotationToolbar, &AnnotationToolbar::toolSelected, ...);
        SLOG_INFO("AnnotationIntegrationHelper: Attached to AnnotationToolbar");
    }
}

// ========================================================================
// Rendering Integration
// ========================================================================

void AnnotationIntegrationHelper::renderAnnotations(QPainter* painter,
                                                    int pageNumber,
                                                    const QRectF& pageRect,
                                                    double zoomFactor) {
    if (!m_initialized || !m_renderDelegate || !m_controller) {
        return;
    }

    if (!hasDocument()) {
        return;
    }

    // Render annotations for this page
    m_renderDelegate->renderAnnotations(painter, pageNumber, pageRect,
                                        zoomFactor);
}

// ========================================================================
// Mouse Event Handling
// ========================================================================

bool AnnotationIntegrationHelper::handleMousePress(const QPointF& point,
                                                   int pageNumber,
                                                   double zoomFactor) {
    if (!m_initialized || !m_selectionManager) {
        return false;
    }

    if (!hasDocument()) {
        return false;
    }

    return m_selectionManager->handleMousePress(point, pageNumber, zoomFactor);
}

bool AnnotationIntegrationHelper::handleMouseMove(const QPointF& point,
                                                  double zoomFactor) {
    if (!m_initialized || !m_selectionManager) {
        return false;
    }

    if (!hasDocument()) {
        return false;
    }

    return m_selectionManager->handleMouseMove(point, zoomFactor);
}

bool AnnotationIntegrationHelper::handleMouseRelease(const QPointF& point,
                                                     double zoomFactor) {
    if (!m_initialized || !m_selectionManager) {
        return false;
    }

    if (!hasDocument()) {
        return false;
    }

    return m_selectionManager->handleMouseRelease(point, zoomFactor);
}

// ========================================================================
// Event Filter
// ========================================================================

bool AnnotationIntegrationHelper::eventFilter(QObject* obj, QEvent* event) {
    // Intercept mouse events on PDFViewer for annotation interaction
    if (obj == m_pdfViewer && hasDocument()) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            // TODO: Convert widget coordinates to page coordinates
            // This requires knowledge of the current page and zoom level
            // For now, this is a placeholder
            // return handleMousePress(mouseEvent->pos(), currentPage, zoom);
        } else if (event->type() == QEvent::MouseMove) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            // TODO: Convert widget coordinates to page coordinates
            // return handleMouseMove(mouseEvent->pos(), zoom);
        } else if (event->type() == QEvent::MouseButtonRelease) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            // TODO: Convert widget coordinates to page coordinates
            // return handleMouseRelease(mouseEvent->pos(), zoom);
        }
    }

    return QObject::eventFilter(obj, event);
}

// ========================================================================
// Signal Handling
// ========================================================================

void AnnotationIntegrationHelper::connectSignals() {
    if (!m_controller) {
        return;
    }

    // Connect controller signals
    connect(m_controller, &AnnotationController::annotationAdded, this,
            &AnnotationIntegrationHelper::onAnnotationAdded);
    connect(m_controller, &AnnotationController::annotationRemoved, this,
            &AnnotationIntegrationHelper::onAnnotationRemoved);
    connect(m_controller, &AnnotationController::annotationModified, this,
            &AnnotationIntegrationHelper::onAnnotationModified);

    // Connect selection manager signals
    if (m_selectionManager) {
        connect(m_selectionManager,
                &AnnotationSelectionManager::selectionChanged, this,
                &AnnotationIntegrationHelper::onSelectionChanged);
        connect(m_selectionManager,
                &AnnotationSelectionManager::selectionCleared, this,
                &AnnotationIntegrationHelper::selectionCleared);
        connect(m_selectionManager,
                &AnnotationSelectionManager::annotationMoved, this,
                [this]() { emit annotationsChanged(); });
        connect(m_selectionManager,
                &AnnotationSelectionManager::annotationResized, this,
                [this]() { emit annotationsChanged(); });
    }

    SLOG_DEBUG("AnnotationIntegrationHelper: Signals connected");
}

void AnnotationIntegrationHelper::disconnectSignals() {
    // Disconnect all signals
    if (m_controller) {
        disconnect(m_controller, nullptr, this, nullptr);
    }

    if (m_selectionManager) {
        disconnect(m_selectionManager.get(), nullptr, this, nullptr);
    }

    SLOG_DEBUG("AnnotationIntegrationHelper: Signals disconnected");
}

void AnnotationIntegrationHelper::onAnnotationAdded(
    const QString& annotationId) {
    SLOG_DEBUG_F("AnnotationIntegrationHelper: Annotation added: {}",
                 annotationId);
    emit annotationsChanged();
}

void AnnotationIntegrationHelper::onAnnotationRemoved(
    const QString& annotationId) {
    SLOG_DEBUG_F("AnnotationIntegrationHelper: Annotation removed: {}",
                 annotationId);
    emit annotationsChanged();
}

void AnnotationIntegrationHelper::onAnnotationModified(
    const QString& annotationId) {
    SLOG_DEBUG_F("AnnotationIntegrationHelper: Annotation modified: {}",
                 annotationId);
    emit annotationsChanged();
}

void AnnotationIntegrationHelper::onSelectionChanged(
    const QString& annotationId) {
    SLOG_DEBUG_F("AnnotationIntegrationHelper: Selection changed: {}",
                 annotationId);

    // Update render delegate's selected annotation
    if (m_renderDelegate) {
        m_renderDelegate->setSelectedAnnotationId(annotationId);
    }

    emit annotationSelected(annotationId);
    emit annotationsChanged();  // Trigger repaint to show selection
}
