#include "NavigationCommands.h"
#include "../controller/PageController.h"
#include "../ui/core/ViewWidget.h"
#include "../model/DocumentModel.h"
#include <QWidget>
#include <QApplication>

// NavigationCommand base class implementation
NavigationCommand::NavigationCommand(const QString& name, QObject* parent)
    : QObject(parent), m_name(name), m_logger("NavigationCommand") {
    m_logger.debug(QString("Created navigation command: %1").arg(name));
}

// NextPageCommand implementation
NextPageCommand::NextPageCommand(PageController* controller, QObject* parent)
    : NavigationCommand("Next Page", parent), m_controller(controller) {
    setDescription("Navigate to the next page");
    setShortcut("Right");
}

bool NextPageCommand::execute() {
    if (!m_controller) {
        m_logger.error("PageController is null");
        emit executed(false);
        return false;
    }

    m_previousPage = m_controller->getCurrentPage();
    
    try {
        m_controller->goToNextPage();
        int newPage = m_controller->getCurrentPage();
        
        if (newPage != m_previousPage) {
            emit navigationChanged(newPage);
            emit executed(true);
            m_logger.debug(QString("Navigated to next page: %1").arg(newPage));
            return true;
        } else {
            m_logger.warning("Already at last page");
            emit executed(false);
            return false;
        }
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to navigate to next page: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool NextPageCommand::canExecute() const {
    if (!m_controller) return false;
    
    int currentPage = m_controller->getCurrentPage();
    int totalPages = m_controller->getTotalPages();
    return currentPage < totalPages;
}

bool NextPageCommand::undo() {
    if (!m_controller || m_previousPage == -1) {
        return false;
    }
    
    try {
        m_controller->goToPage(m_previousPage);
        emit navigationChanged(m_previousPage);
        m_logger.debug(QString("Undid next page navigation, returned to page: %1").arg(m_previousPage));
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to undo next page navigation: %1").arg(e.what()));
        return false;
    }
}

// PreviousPageCommand implementation
PreviousPageCommand::PreviousPageCommand(PageController* controller, QObject* parent)
    : NavigationCommand("Previous Page", parent), m_controller(controller) {
    setDescription("Navigate to the previous page");
    setShortcut("Left");
}

bool PreviousPageCommand::execute() {
    if (!m_controller) {
        m_logger.error("PageController is null");
        emit executed(false);
        return false;
    }

    m_previousPage = m_controller->getCurrentPage();
    
    try {
        m_controller->goToPrevPage();
        int newPage = m_controller->getCurrentPage();
        
        if (newPage != m_previousPage) {
            emit navigationChanged(newPage);
            emit executed(true);
            m_logger.debug(QString("Navigated to previous page: %1").arg(newPage));
            return true;
        } else {
            m_logger.warning("Already at first page");
            emit executed(false);
            return false;
        }
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to navigate to previous page: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool PreviousPageCommand::canExecute() const {
    if (!m_controller) return false;
    
    int currentPage = m_controller->getCurrentPage();
    return currentPage > 1;
}

bool PreviousPageCommand::undo() {
    if (!m_controller || m_previousPage == -1) {
        return false;
    }
    
    try {
        m_controller->goToPage(m_previousPage);
        emit navigationChanged(m_previousPage);
        m_logger.debug(QString("Undid previous page navigation, returned to page: %1").arg(m_previousPage));
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to undo previous page navigation: %1").arg(e.what()));
        return false;
    }
}

// GoToPageCommand implementation
GoToPageCommand::GoToPageCommand(PageController* controller, int targetPage, QObject* parent)
    : NavigationCommand("Go To Page", parent), m_controller(controller), m_targetPage(targetPage) {
    setDescription(QString("Navigate to page %1").arg(targetPage));
    setShortcut("Ctrl+G");
}

bool GoToPageCommand::execute() {
    if (!m_controller) {
        m_logger.error("PageController is null");
        emit executed(false);
        return false;
    }

    if (!m_controller->isValidPage(m_targetPage)) {
        m_logger.error(QString("Invalid target page: %1").arg(m_targetPage));
        emit executed(false);
        return false;
    }

    m_previousPage = m_controller->getCurrentPage();
    
    try {
        m_controller->goToPage(m_targetPage);
        emit navigationChanged(m_targetPage);
        emit executed(true);
        m_logger.debug(QString("Navigated to page: %1").arg(m_targetPage));
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to navigate to page %1: %2").arg(m_targetPage).arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool GoToPageCommand::canExecute() const {
    if (!m_controller) return false;
    return m_controller->isValidPage(m_targetPage);
}

bool GoToPageCommand::undo() {
    if (!m_controller || m_previousPage == -1) {
        return false;
    }
    
    try {
        m_controller->goToPage(m_previousPage);
        emit navigationChanged(m_previousPage);
        m_logger.debug(QString("Undid go to page navigation, returned to page: %1").arg(m_previousPage));
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to undo go to page navigation: %1").arg(e.what()));
        return false;
    }
}

// FirstPageCommand implementation
FirstPageCommand::FirstPageCommand(PageController* controller, QObject* parent)
    : NavigationCommand("First Page", parent), m_controller(controller) {
    setDescription("Navigate to the first page");
    setShortcut("Home");
}

bool FirstPageCommand::execute() {
    if (!m_controller) {
        m_logger.error("PageController is null");
        emit executed(false);
        return false;
    }

    m_previousPage = m_controller->getCurrentPage();
    
    try {
        m_controller->goToFirstPage();
        emit navigationChanged(1);
        emit executed(true);
        m_logger.debug("Navigated to first page");
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to navigate to first page: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool FirstPageCommand::canExecute() const {
    if (!m_controller) return false;
    return m_controller->getCurrentPage() > 1;
}

bool FirstPageCommand::undo() {
    if (!m_controller || m_previousPage == -1) {
        return false;
    }
    
    try {
        m_controller->goToPage(m_previousPage);
        emit navigationChanged(m_previousPage);
        m_logger.debug(QString("Undid first page navigation, returned to page: %1").arg(m_previousPage));
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to undo first page navigation: %1").arg(e.what()));
        return false;
    }
}

// LastPageCommand implementation
LastPageCommand::LastPageCommand(PageController* controller, QObject* parent)
    : NavigationCommand("Last Page", parent), m_controller(controller) {
    setDescription("Navigate to the last page");
    setShortcut("End");
}

bool LastPageCommand::execute() {
    if (!m_controller) {
        m_logger.error("PageController is null");
        emit executed(false);
        return false;
    }

    m_previousPage = m_controller->getCurrentPage();
    
    try {
        m_controller->goToLastPage();
        int totalPages = m_controller->getTotalPages();
        emit navigationChanged(totalPages);
        emit executed(true);
        m_logger.debug(QString("Navigated to last page: %1").arg(totalPages));
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to navigate to last page: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool LastPageCommand::canExecute() const {
    if (!m_controller) return false;
    
    int currentPage = m_controller->getCurrentPage();
    int totalPages = m_controller->getTotalPages();
    return currentPage < totalPages;
}

bool LastPageCommand::undo() {
    if (!m_controller || m_previousPage == -1) {
        return false;
    }
    
    try {
        m_controller->goToPage(m_previousPage);
        emit navigationChanged(m_previousPage);
        m_logger.debug(QString("Undid last page navigation, returned to page: %1").arg(m_previousPage));
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to undo last page navigation: %1").arg(e.what()));
        return false;
    }
}

// ZoomInCommand implementation
ZoomInCommand::ZoomInCommand(ViewWidget* viewWidget, double factor, QObject* parent)
    : NavigationCommand("Zoom In", parent), m_viewWidget(viewWidget), m_factor(factor) {
    setDescription("Zoom in to increase magnification");
    setShortcut("Ctrl++");
}

bool ZoomInCommand::execute() {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget is null");
        emit executed(false);
        return false;
    }

    m_previousZoom = m_viewWidget->getCurrentZoom();

    try {
        m_viewWidget->executePDFAction(ActionMap::zoomIn);
        double newZoom = m_viewWidget->getCurrentZoom();

        if (newZoom != m_previousZoom) {
            emit zoomChanged(newZoom);
            emit executed(true);
            m_logger.debug(QString("Zoomed in to: %1").arg(newZoom));
            return true;
        } else {
            m_logger.warning("Already at maximum zoom");
            emit executed(false);
            return false;
        }
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to zoom in: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool ZoomInCommand::canExecute() const {
    if (!m_viewWidget) return false;

    double currentZoom = m_viewWidget->getCurrentZoom();
    return currentZoom < 10.0; // Maximum zoom level
}

bool ZoomInCommand::undo() {
    if (!m_viewWidget || m_previousZoom <= 0) {
        return false;
    }

    try {
        // Note: ViewWidget doesn't have setZoom method, so we can't directly undo zoom
        // This would require extending ViewWidget API or using a different approach
        m_logger.warning("Zoom undo not implemented - ViewWidget API limitation");
        return false;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to undo zoom in: %1").arg(e.what()));
        return false;
    }
}

// ZoomOutCommand implementation
ZoomOutCommand::ZoomOutCommand(ViewWidget* viewWidget, double factor, QObject* parent)
    : NavigationCommand("Zoom Out", parent), m_viewWidget(viewWidget), m_factor(factor) {
    setDescription("Zoom out to decrease magnification");
    setShortcut("Ctrl+-");
}

bool ZoomOutCommand::execute() {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget is null");
        emit executed(false);
        return false;
    }

    m_previousZoom = m_viewWidget->getCurrentZoom();

    try {
        m_viewWidget->executePDFAction(ActionMap::zoomOut);
        double newZoom = m_viewWidget->getCurrentZoom();

        if (newZoom != m_previousZoom) {
            emit zoomChanged(newZoom);
            emit executed(true);
            m_logger.debug(QString("Zoomed out to: %1").arg(newZoom));
            return true;
        } else {
            m_logger.warning("Already at minimum zoom");
            emit executed(false);
            return false;
        }
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to zoom out: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool ZoomOutCommand::canExecute() const {
    if (!m_viewWidget) return false;

    double currentZoom = m_viewWidget->getCurrentZoom();
    return currentZoom > 0.1; // Minimum zoom level
}

bool ZoomOutCommand::undo() {
    if (!m_viewWidget || m_previousZoom <= 0) {
        return false;
    }

    try {
        // Note: ViewWidget doesn't have setZoom method, so we can't directly undo zoom
        // This would require extending ViewWidget API or using a different approach
        m_logger.warning("Zoom undo not implemented - ViewWidget API limitation");
        return false;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to undo zoom out: %1").arg(e.what()));
        return false;
    }
}

// FitWidthCommand implementation
FitWidthCommand::FitWidthCommand(ViewWidget* viewWidget, QObject* parent)
    : NavigationCommand("Fit Width", parent), m_viewWidget(viewWidget) {
    setDescription("Fit page width to window");
    setShortcut("Ctrl+2");
}

bool FitWidthCommand::execute() {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget is null");
        emit executed(false);
        return false;
    }

    m_previousZoom = m_viewWidget->getCurrentZoom();

    try {
        m_viewWidget->executePDFAction(ActionMap::fitToWidth);
        double newZoom = m_viewWidget->getCurrentZoom();
        emit zoomChanged(newZoom);
        emit executed(true);
        m_logger.debug(QString("Fit to width, new zoom: %1").arg(newZoom));
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to fit to width: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool FitWidthCommand::canExecute() const {
    return m_viewWidget != nullptr && m_viewWidget->hasDocuments();
}

bool FitWidthCommand::undo() {
    if (!m_viewWidget || m_previousZoom <= 0) {
        return false;
    }

    try {
        // Note: ViewWidget doesn't have setZoom method, so we can't directly undo zoom
        // This would require extending ViewWidget API or using a different approach
        m_logger.warning("Zoom undo not implemented - ViewWidget API limitation");
        return false;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to undo fit to width: %1").arg(e.what()));
        return false;
    }
}

// FitPageCommand implementation
FitPageCommand::FitPageCommand(ViewWidget* viewWidget, QObject* parent)
    : NavigationCommand("Fit Page", parent), m_viewWidget(viewWidget) {
    setDescription("Fit entire page to window");
    setShortcut("Ctrl+1");
}

bool FitPageCommand::execute() {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget is null");
        emit executed(false);
        return false;
    }

    m_previousZoom = m_viewWidget->getCurrentZoom();

    try {
        m_viewWidget->executePDFAction(ActionMap::fitToPage);
        double newZoom = m_viewWidget->getCurrentZoom();
        emit zoomChanged(newZoom);
        emit executed(true);
        m_logger.debug(QString("Fit to page, new zoom: %1").arg(newZoom));
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to fit to page: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool FitPageCommand::canExecute() const {
    return m_viewWidget != nullptr && m_viewWidget->hasDocuments();
}

bool FitPageCommand::undo() {
    if (!m_viewWidget || m_previousZoom <= 0) {
        return false;
    }

    try {
        // Restore previous zoom level
        m_viewWidget->setZoom(m_previousZoom);
        emit zoomChanged(m_previousZoom);
        m_logger.debug(QString("Undid fit to page, restored zoom: %1").arg(m_previousZoom));
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to undo fit to page: %1").arg(e.what()));
        return false;
    }
}

// SetZoomCommand implementation
SetZoomCommand::SetZoomCommand(ViewWidget* viewWidget, double zoomLevel, QObject* parent)
    : NavigationCommand("Set Zoom", parent), m_viewWidget(viewWidget), m_zoomLevel(zoomLevel) {
    setDescription(QString("Set zoom level to %1%").arg(zoomLevel * 100));
    setShortcut("Ctrl+0");
}

bool SetZoomCommand::execute() {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget is null");
        emit executed(false);
        return false;
    }

    m_previousZoom = m_viewWidget->getCurrentZoom();

    try {
        // Set the zoom level
        m_viewWidget->setZoom(m_zoomLevel);
        emit zoomChanged(m_zoomLevel);
        emit executed(true);
        m_logger.debug(QString("Set zoom to: %1").arg(m_zoomLevel));
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to set zoom: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool SetZoomCommand::canExecute() const {
    return m_viewWidget != nullptr && m_viewWidget->hasDocuments() && m_zoomLevel > 0;
}

bool SetZoomCommand::undo() {
    if (!m_viewWidget || m_previousZoom <= 0) {
        return false;
    }

    try {
        // Restore previous zoom level
        m_viewWidget->setZoom(m_previousZoom);
        emit zoomChanged(m_previousZoom);
        m_logger.debug(QString("Undid set zoom, restored zoom: %1").arg(m_previousZoom));
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to undo set zoom: %1").arg(e.what()));
        return false;
    }
}

// RotateViewCommand implementation
RotateViewCommand::RotateViewCommand(ViewWidget* viewWidget, RotationDirection direction, int degrees, QObject* parent)
    : NavigationCommand("Rotate View", parent), m_viewWidget(viewWidget), m_direction(direction), m_degrees(degrees) {
    QString directionStr = (direction == Clockwise) ? "clockwise" : "counter-clockwise";
    setDescription(QString("Rotate view %1 by %2 degrees").arg(directionStr).arg(degrees));
    setShortcut((direction == Clockwise) ? "Ctrl+R" : "Ctrl+Shift+R");
}

bool RotateViewCommand::execute() {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget is null");
        emit executed(false);
        return false;
    }

    if (!m_viewWidget->hasDocuments()) {
        m_logger.warning("No documents open");
        emit executed(false);
        return false;
    }

    // Store previous rotation for undo functionality
    m_previousRotation = 0;

    try {
        if (m_direction == Clockwise) {
            m_viewWidget->executePDFAction(ActionMap::rotateRight);
        } else {
            m_viewWidget->executePDFAction(ActionMap::rotateLeft);
        }

        emit executed(true);
        m_logger.debug(QString("Rotated view %1 by %2 degrees")
                      .arg(m_direction == Clockwise ? "clockwise" : "counter-clockwise")
                      .arg(m_degrees));
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to rotate view: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool RotateViewCommand::canExecute() const {
    return m_viewWidget != nullptr && m_viewWidget->hasDocuments();
}

bool RotateViewCommand::undo() {
    if (!m_viewWidget) {
        return false;
    }

    try {
        // Rotate in opposite direction to undo
        if (m_direction == Clockwise) {
            m_viewWidget->executePDFAction(ActionMap::rotateLeft);
        } else {
            m_viewWidget->executePDFAction(ActionMap::rotateRight);
        }

        m_logger.debug("Undid view rotation");
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to undo view rotation: %1").arg(e.what()));
        return false;
    }
}

// ToggleFullscreenCommand implementation
ToggleFullscreenCommand::ToggleFullscreenCommand(QWidget* mainWindow, QObject* parent)
    : NavigationCommand("Toggle Fullscreen", parent), m_mainWindow(mainWindow) {
    setDescription("Toggle fullscreen mode");
    setShortcut("F11");
}

bool ToggleFullscreenCommand::execute() {
    if (!m_mainWindow) {
        m_logger.error("Main window is null");
        emit executed(false);
        return false;
    }

    try {
        m_wasFullscreen = m_mainWindow->isFullScreen();

        if (m_wasFullscreen) {
            m_mainWindow->showNormal();
            m_logger.debug("Exited fullscreen mode");
        } else {
            m_mainWindow->showFullScreen();
            m_logger.debug("Entered fullscreen mode");
        }

        emit executed(true);
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to toggle fullscreen: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool ToggleFullscreenCommand::canExecute() const {
    return m_mainWindow != nullptr;
}

// ChangeViewModeCommand implementation
ChangeViewModeCommand::ChangeViewModeCommand(ViewWidget* viewWidget, ViewMode mode, QObject* parent)
    : NavigationCommand("Change View Mode", parent), m_viewWidget(viewWidget), m_mode(mode) {
    QString modeStr;
    switch (mode) {
        case SinglePage: modeStr = "Single Page"; break;
        case Continuous: modeStr = "Continuous"; break;
        case FacingPages: modeStr = "Facing Pages"; break;
        case BookView: modeStr = "Book View"; break;
    }
    setDescription(QString("Change view mode to %1").arg(modeStr));
    setShortcut("Ctrl+M");
}

bool ChangeViewModeCommand::execute() {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget is null");
        emit executed(false);
        return false;
    }

    if (!m_viewWidget->hasDocuments()) {
        m_logger.warning("No documents open");
        emit executed(false);
        return false;
    }

    // Store previous mode for undo functionality
    m_previousMode = SinglePage;

    try {
        // Map our ViewMode enum to ViewWidget's mode system
        int viewModeInt = static_cast<int>(m_mode);
        m_viewWidget->setCurrentViewMode(viewModeInt);

        QString modeStr;
        switch (m_mode) {
            case SinglePage: modeStr = "Single Page"; break;
            case Continuous: modeStr = "Continuous"; break;
            case FacingPages: modeStr = "Facing Pages"; break;
            case BookView: modeStr = "Book View"; break;
        }

        emit viewModeChanged(modeStr);
        emit executed(true);
        m_logger.debug(QString("Changed view mode to: %1").arg(modeStr));
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to change view mode: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool ChangeViewModeCommand::canExecute() const {
    return m_viewWidget != nullptr && m_viewWidget->hasDocuments();
}

bool ChangeViewModeCommand::undo() {
    if (!m_viewWidget) {
        return false;
    }

    try {
        int previousModeInt = static_cast<int>(m_previousMode);
        m_viewWidget->setCurrentViewMode(previousModeInt);

        QString modeStr;
        switch (m_previousMode) {
            case SinglePage: modeStr = "Single Page"; break;
            case Continuous: modeStr = "Continuous"; break;
            case FacingPages: modeStr = "Facing Pages"; break;
            case BookView: modeStr = "Book View"; break;
        }

        emit viewModeChanged(modeStr);
        m_logger.debug(QString("Undid view mode change, restored to: %1").arg(modeStr));
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to undo view mode change: %1").arg(e.what()));
        return false;
    }
}

// ScrollToPositionCommand implementation
ScrollToPositionCommand::ScrollToPositionCommand(ViewWidget* viewWidget, ScrollDirection direction, QObject* parent)
    : NavigationCommand("Scroll To Position", parent), m_viewWidget(viewWidget), m_direction(direction) {
    QString directionStr;
    switch (direction) {
        case Top: directionStr = "Top"; break;
        case Bottom: directionStr = "Bottom"; break;
        case Left: directionStr = "Left"; break;
        case Right: directionStr = "Right"; break;
    }
    setDescription(QString("Scroll to %1").arg(directionStr));

    // Set shortcuts based on direction
    switch (direction) {
        case Top: setShortcut("Ctrl+Home"); break;
        case Bottom: setShortcut("Ctrl+End"); break;
        case Left: setShortcut("Ctrl+Left"); break;
        case Right: setShortcut("Ctrl+Right"); break;
    }
}

bool ScrollToPositionCommand::execute() {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget is null");
        emit executed(false);
        return false;
    }

    if (!m_viewWidget->hasDocuments()) {
        m_logger.warning("No documents open");
        emit executed(false);
        return false;
    }

    try {
        // Save current scroll position for undo
        m_previousPosition = m_viewWidget->getScrollPosition();

        // Execute scroll based on direction
        QString directionStr;
        switch (m_direction) {
            case Top:
                directionStr = "top";
                m_viewWidget->scrollToTop();
                break;
            case Bottom:
                directionStr = "bottom";
                m_viewWidget->scrollToBottom();
                break;
            case Left:
                directionStr = "left";
                // For left/right, use custom position if set
                if (m_x >= 0) {
                    QPoint pos = m_viewWidget->getScrollPosition();
                    m_viewWidget->setScrollPosition(QPoint(m_x, pos.y()));
                }
                break;
            case Right:
                directionStr = "right";
                // For left/right, use custom position if set
                if (m_x >= 0) {
                    QPoint pos = m_viewWidget->getScrollPosition();
                    m_viewWidget->setScrollPosition(QPoint(m_x, pos.y()));
                }
                break;
        }

        emit executed(true);
        m_logger.debug(QString("Scrolled to %1").arg(directionStr));
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to scroll: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool ScrollToPositionCommand::canExecute() const {
    return m_viewWidget != nullptr && m_viewWidget->hasDocuments();
}

bool ScrollToPositionCommand::undo() {
    if (!m_viewWidget) {
        return false;
    }

    try {
        // Restore previous scroll position
        m_viewWidget->setScrollPosition(m_previousPosition);
        m_logger.debug(QString("Undid scroll, restored position: (%1, %2)")
                      .arg(m_previousPosition.x()).arg(m_previousPosition.y()));
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to undo scroll: %1").arg(e.what()));
        return false;
    }
}

// NavigationCommandFactory implementation
std::unique_ptr<NavigationCommand> NavigationCommandFactory::createPageNavigationCommand(
    const QString& type, PageController* controller) {

    if (type == "next") {
        return std::make_unique<NextPageCommand>(controller);
    } else if (type == "previous") {
        return std::make_unique<PreviousPageCommand>(controller);
    } else if (type == "first") {
        return std::make_unique<FirstPageCommand>(controller);
    } else if (type == "last") {
        return std::make_unique<LastPageCommand>(controller);
    } else if (type.startsWith("goto:")) {
        QString pageStr = type.mid(5); // Remove "goto:" prefix
        bool ok;
        int pageNumber = pageStr.toInt(&ok);
        if (ok && pageNumber > 0) {
            return std::make_unique<GoToPageCommand>(controller, pageNumber);
        }
    }

    return nullptr;
}

std::unique_ptr<NavigationCommand> NavigationCommandFactory::createZoomCommand(
    const QString& type, ViewWidget* viewWidget) {

    if (type == "in") {
        return std::make_unique<ZoomInCommand>(viewWidget);
    } else if (type == "out") {
        return std::make_unique<ZoomOutCommand>(viewWidget);
    } else if (type == "fit-width") {
        return std::make_unique<FitWidthCommand>(viewWidget);
    } else if (type == "fit-page") {
        return std::make_unique<FitPageCommand>(viewWidget);
    } else if (type.startsWith("set:")) {
        QString zoomStr = type.mid(4); // Remove "set:" prefix
        bool ok;
        double zoomLevel = zoomStr.toDouble(&ok);
        if (ok && zoomLevel > 0) {
            return std::make_unique<SetZoomCommand>(viewWidget, zoomLevel);
        }
    }

    return nullptr;
}

std::unique_ptr<NavigationCommand> NavigationCommandFactory::createViewCommand(
    const QString& type, ViewWidget* viewWidget) {

    if (type == "rotate-clockwise") {
        return std::make_unique<RotateViewCommand>(viewWidget, RotateViewCommand::Clockwise);
    } else if (type == "rotate-counter-clockwise") {
        return std::make_unique<RotateViewCommand>(viewWidget, RotateViewCommand::CounterClockwise);
    } else if (type == "single-page") {
        return std::make_unique<ChangeViewModeCommand>(viewWidget, ChangeViewModeCommand::SinglePage);
    } else if (type == "continuous") {
        return std::make_unique<ChangeViewModeCommand>(viewWidget, ChangeViewModeCommand::Continuous);
    } else if (type == "facing-pages") {
        return std::make_unique<ChangeViewModeCommand>(viewWidget, ChangeViewModeCommand::FacingPages);
    } else if (type == "book-view") {
        return std::make_unique<ChangeViewModeCommand>(viewWidget, ChangeViewModeCommand::BookView);
    } else if (type == "scroll-top") {
        return std::make_unique<ScrollToPositionCommand>(viewWidget, ScrollToPositionCommand::Top);
    } else if (type == "scroll-bottom") {
        return std::make_unique<ScrollToPositionCommand>(viewWidget, ScrollToPositionCommand::Bottom);
    } else if (type == "scroll-left") {
        return std::make_unique<ScrollToPositionCommand>(viewWidget, ScrollToPositionCommand::Left);
    } else if (type == "scroll-right") {
        return std::make_unique<ScrollToPositionCommand>(viewWidget, ScrollToPositionCommand::Right);
    }

    return nullptr;
}

void NavigationCommandFactory::registerShortcuts(QWidget* widget) {
    // Register keyboard shortcuts with the widget
    // Note: Shortcut registration is handled by the application's shortcut system
    // This method is reserved for future per-widget shortcut customization
    if (widget) {
        // Future: Set up QShortcut objects or integrate with custom shortcut system
    }
}
