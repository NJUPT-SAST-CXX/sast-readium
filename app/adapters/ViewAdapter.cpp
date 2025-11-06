#include "ViewAdapter.h"

// Delegates
#include "delegate/ViewDelegate.h"

// Pages
#include "ui/pages/PDFViewerPage.h"

// Logging
#include "logging/SimpleLogging.h"

ViewAdapter::ViewAdapter(QObject* parent)
    : QObject(parent), m_viewDelegate(nullptr), m_pdfViewerPage(nullptr) {
    SLOG_INFO("ViewAdapter: Constructor");
}

ViewAdapter::~ViewAdapter() { SLOG_INFO("ViewAdapter: Destructor"); }

void ViewAdapter::setViewDelegate(ViewDelegate* delegate) {
    if (m_viewDelegate) {
        disconnect(m_viewDelegate, nullptr, this, nullptr);
    }

    m_viewDelegate = delegate;

    if (m_viewDelegate) {
        connectDelegateSignals();
    }
}

void ViewAdapter::setPDFViewerPage(PDFViewerPage* page) {
    m_pdfViewerPage = page;
}

void ViewAdapter::connectDelegateSignals() {
    if (!m_viewDelegate) {
        return;
    }

    // Connect ViewDelegate signals to adapter signals
    // ViewDelegate manages view modes and display settings
    connect(m_viewDelegate, &ViewDelegate::layoutChanged, this, [this]() {
        SLOG_INFO("ViewAdapter: Layout changed signal received");
    });

    connect(m_viewDelegate, &ViewDelegate::visibilityChanged, this,
            [](const QString& component, bool visible) {
                SLOG_INFO("ViewAdapter: Visibility changed: " + component +
                          " = " + (visible ? "true" : "false"));
            });

    connect(m_viewDelegate, &ViewDelegate::modeChanged, this,
            [this](const QString& mode, bool active) {
                SLOG_INFO("ViewAdapter: Mode changed: " + mode + " = " +
                          (active ? "true" : "false"));

                // Map mode strings to view mode integers
                if (mode == "fullscreen") {
                    emit fullScreenChanged(active);
                } else if (mode == "presentation") {
                    emit presentationChanged(active);
                }
            });

    SLOG_INFO("ViewAdapter: Delegate signals connected");
}

void ViewAdapter::setViewMode(int mode) {
    SLOG_INFO_F("ViewAdapter: Setting view mode: {}", mode);

    if (!m_pdfViewerPage) {
        SLOG_ERROR("ViewAdapter: PDFViewerPage is null");
        return;
    }

    // Delegate to PDFViewerPage which has access to the viewer widget
    m_pdfViewerPage->setViewMode(mode);

    // Emit signal to notify UI components
    emit viewModeChanged(mode);
}

void ViewAdapter::toggleFullScreen() {
    SLOG_INFO("ViewAdapter: Toggling full screen");

    if (!m_pdfViewerPage) {
        SLOG_ERROR("ViewAdapter: PDFViewerPage is null");
        return;
    }

    // Delegate to PDFViewerPage which has access to full screen functionality
    m_pdfViewerPage->toggleFullScreen();
}

void ViewAdapter::togglePresentation() {
    SLOG_INFO("ViewAdapter: Toggling presentation mode");

    if (!m_pdfViewerPage) {
        SLOG_ERROR("ViewAdapter: PDFViewerPage is null");
        return;
    }

    // Delegate to PDFViewerPage which has access to presentation functionality
    m_pdfViewerPage->togglePresentation();
}

void ViewAdapter::toggleLeftSideBar() {
    SLOG_INFO("ViewAdapter: Toggling left sidebar");

    if (!m_pdfViewerPage) {
        SLOG_ERROR("ViewAdapter: PDFViewerPage is null");
        return;
    }

    m_pdfViewerPage->toggleLeftSideBar();
}

void ViewAdapter::toggleRightSideBar() {
    SLOG_INFO("ViewAdapter: Toggling right sidebar");

    if (!m_pdfViewerPage) {
        SLOG_ERROR("ViewAdapter: PDFViewerPage is null");
        return;
    }

    m_pdfViewerPage->toggleRightSideBar();
}

void ViewAdapter::toggleToolBar() {
    SLOG_INFO("ViewAdapter: Toggling toolbar");

    if (!m_pdfViewerPage) {
        SLOG_ERROR("ViewAdapter: PDFViewerPage is null");
        return;
    }

    // Delegate to PDFViewerPage which has access to the toolbar widget
    m_pdfViewerPage->toggleToolBar();
}

void ViewAdapter::toggleStatusBar() {
    SLOG_INFO("ViewAdapter: Toggling status bar");

    if (!m_pdfViewerPage) {
        SLOG_ERROR("ViewAdapter: PDFViewerPage is null");
        return;
    }

    // Delegate to PDFViewerPage which has access to the status bar widget
    m_pdfViewerPage->toggleStatusBar();
}
