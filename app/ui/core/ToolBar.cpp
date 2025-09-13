#include "ToolBar.h"
#include "../../managers/StyleManager.h"
#include <QAction>
#include <QHBoxLayout>
#include <QWidget>
#include <QEvent>

ToolBar::ToolBar(QWidget* parent) : QToolBar(parent) {
    setMovable(true);
    setObjectName("MainToolBar");
    setToolButtonStyle(Qt::ToolButtonIconOnly);

    // Initialize all controls
    setupFileActions();
    createSeparator();
    setupNavigationActions();
    createSeparator();
    setupZoomActions();
    createSeparator();
    setupViewActions();
    createSeparator();
    setupRotationActions();
    createSeparator();
    setupThemeActions();

    // Apply style
    applyToolBarStyle();

    // Initial state: disable all actions (when no document)
    setActionsEnabled(false);
}

void ToolBar::setupFileActions() {
    // Open file
    openAction = new QAction("📁", this);
    openAction->setToolTip(tr("Open PDF File (Ctrl+O)"));
    openAction->setShortcut(QKeySequence("Ctrl+O"));
    addAction(openAction);

    // Open folder
    openFolderAction = new QAction("📂", this);
    openFolderAction->setToolTip(tr("Open Folder (Ctrl+Shift+O)"));
    openFolderAction->setShortcut(QKeySequence("Ctrl+Shift+O"));
    addAction(openFolderAction);

    // Save file
    saveAction = new QAction("💾", this);
    saveAction->setToolTip(tr("Save File (Ctrl+S)"));
    saveAction->setShortcut(QKeySequence("Ctrl+S"));
    addAction(saveAction);

    // Connect signals
    connect(openAction, &QAction::triggered, this, [this]() {
        emit actionTriggered(ActionMap::openFile);
    });
    connect(openFolderAction, &QAction::triggered, this, [this]() {
        emit actionTriggered(ActionMap::openFolder);
    });
    connect(saveAction, &QAction::triggered, this, [this]() {
        emit actionTriggered(ActionMap::save);
    });
}

void ToolBar::setupNavigationActions() {
    // First page
    firstPageAction = new QAction("⏮", this);
    firstPageAction->setToolTip(tr("First Page (Ctrl+Home)"));
    addAction(firstPageAction);

    // Previous page
    prevPageAction = new QAction("◀", this);
    prevPageAction->setToolTip(tr("Previous Page (Page Up)"));
    addAction(prevPageAction);

    // Page number input
    QWidget* pageWidget = new QWidget(this);
    QHBoxLayout* pageLayout = new QHBoxLayout(pageWidget);
    pageLayout->setContentsMargins(4, 0, 4, 0);
    pageLayout->setSpacing(2);

    pageSpinBox = new QSpinBox(pageWidget);
    pageSpinBox->setMinimum(1);
    pageSpinBox->setMaximum(1);
    pageSpinBox->setValue(1);
    pageSpinBox->setFixedWidth(60);
    pageSpinBox->setToolTip(tr("Current Page"));

    pageCountLabel = new QLabel("/ 1", pageWidget);
    pageCountLabel->setMinimumWidth(30);

    pageLayout->addWidget(pageSpinBox);
    pageLayout->addWidget(pageCountLabel);
    addWidget(pageWidget);

    // Next page
    nextPageAction = new QAction("▶", this);
    nextPageAction->setToolTip(tr("Next Page (Page Down)"));
    addAction(nextPageAction);

    // Last page
    lastPageAction = new QAction("⏭", this);
    lastPageAction->setToolTip(tr("Last Page (Ctrl+End)"));
    addAction(lastPageAction);

    // Connect signals
    connect(firstPageAction, &QAction::triggered, this, [this]() {
        emit actionTriggered(ActionMap::firstPage);
    });
    connect(prevPageAction, &QAction::triggered, this, [this]() {
        emit actionTriggered(ActionMap::previousPage);
    });
    connect(nextPageAction, &QAction::triggered, this, [this]() {
        emit actionTriggered(ActionMap::nextPage);
    });
    connect(lastPageAction, &QAction::triggered, this, [this]() {
        emit actionTriggered(ActionMap::lastPage);
    });
    connect(pageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ToolBar::onPageSpinBoxChanged);
}

void ToolBar::setupZoomActions() {
    // Zoom out
    zoomOutAction = new QAction("🔍-", this);
    zoomOutAction->setToolTip(tr("Zoom Out (Ctrl+-)"));
    addAction(zoomOutAction);

    // Zoom in
    zoomInAction = new QAction("🔍+", this);
    zoomInAction->setToolTip(tr("Zoom In (Ctrl++)"));
    addAction(zoomInAction);

    // Fit to width
    fitWidthAction = new QAction("📏", this);
    fitWidthAction->setToolTip(tr("Fit to Width (Ctrl+1)"));
    addAction(fitWidthAction);

    // Fit to page
    fitPageAction = new QAction("🗎", this);
    fitPageAction->setToolTip(tr("Fit to Page (Ctrl+0)"));
    addAction(fitPageAction);

    // Fit to height
    fitHeightAction = new QAction("📐", this);
    fitHeightAction->setToolTip(tr("Fit to Height (Ctrl+2)"));
    addAction(fitHeightAction);

    // Connect signals
    connect(zoomOutAction, &QAction::triggered, this, [this]() {
        emit actionTriggered(ActionMap::zoomOut);
    });
    connect(zoomInAction, &QAction::triggered, this, [this]() {
        emit actionTriggered(ActionMap::zoomIn);
    });
    connect(fitWidthAction, &QAction::triggered, this, [this]() {
        emit actionTriggered(ActionMap::fitToWidth);
    });
    connect(fitPageAction, &QAction::triggered, this, [this]() {
        emit actionTriggered(ActionMap::fitToPage);
    });
    connect(fitHeightAction, &QAction::triggered, this, [this]() {
        emit actionTriggered(ActionMap::fitToHeight);
    });
}

void ToolBar::setupViewActions() {
    // Toggle sidebar
    toggleSidebarAction = new QAction("📋", this);
    toggleSidebarAction->setToolTip(tr("Toggle Sidebar (F9)"));
    toggleSidebarAction->setCheckable(true);
    toggleSidebarAction->setChecked(true);
    addAction(toggleSidebarAction);

    // View mode selection
    QWidget* viewWidget = new QWidget(this);
    QHBoxLayout* viewLayout = new QHBoxLayout(viewWidget);
    viewLayout->setContentsMargins(4, 0, 4, 0);

    viewModeCombo = new QComboBox(viewWidget);
    viewModeCombo->addItem(tr("Single Page View"));
    viewModeCombo->addItem(tr("Continuous Scroll"));
    viewModeCombo->setCurrentIndex(0);
    viewModeCombo->setToolTip(tr("Select View Mode"));
    viewModeCombo->setFixedWidth(100);

    viewLayout->addWidget(viewModeCombo);
    addWidget(viewWidget);

    // Connect signals
    connect(toggleSidebarAction, &QAction::triggered, this, [this]() {
        emit actionTriggered(ActionMap::toggleSideBar);
    });
    connect(viewModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolBar::onViewModeChanged);
}

void ToolBar::setupRotationActions() {
    // Rotate left
    rotateLeftAction = new QAction("↺", this);
    rotateLeftAction->setToolTip(tr("Rotate Left 90° (Ctrl+L)"));
    addAction(rotateLeftAction);

    // Rotate right
    rotateRightAction = new QAction("↻", this);
    rotateRightAction->setToolTip(tr("Rotate Right 90° (Ctrl+R)"));
    addAction(rotateRightAction);

    // Connect signals
    connect(rotateLeftAction, &QAction::triggered, this, [this]() {
        emit actionTriggered(ActionMap::rotateLeft);
    });
    connect(rotateRightAction, &QAction::triggered, this, [this]() {
        emit actionTriggered(ActionMap::rotateRight);
    });
}

void ToolBar::setupThemeActions() {
    // Theme toggle
    themeToggleAction = new QAction("🌙", this);
    themeToggleAction->setToolTip(tr("Toggle Theme (Ctrl+T)"));
    addAction(themeToggleAction);

    // Connect signals
    connect(themeToggleAction, &QAction::triggered, this, [this]() {
        emit actionTriggered(ActionMap::toggleTheme);
    });
}

void ToolBar::createSeparator() {
    addSeparator();
}

void ToolBar::applyToolBarStyle() {
    // Apply toolbar style
    setStyleSheet(STYLE.getToolbarStyleSheet());

    // Set tool button style
    QList<QAction*> actions = this->actions();
    for (QAction* action : actions) {
        if (!action->isSeparator()) {
            QWidget* widget = widgetForAction(action);
            if (widget) {
                widget->setStyleSheet(STYLE.getButtonStyleSheet());
            }
        }
    }
}

void ToolBar::updatePageInfo(int currentPage, int totalPages) {
    if (pageSpinBox && pageCountLabel) {
        pageSpinBox->blockSignals(true);
        pageSpinBox->setMaximum(totalPages);
        pageSpinBox->setValue(currentPage + 1); // Convert from 0-based to 1-based
        pageSpinBox->blockSignals(false);

        pageCountLabel->setText(QString("/ %1").arg(totalPages));

        // Update navigation button states
        firstPageAction->setEnabled(currentPage > 0);
        prevPageAction->setEnabled(currentPage > 0);
        nextPageAction->setEnabled(currentPage < totalPages - 1);
        lastPageAction->setEnabled(currentPage < totalPages - 1);
    }
}

void ToolBar::updateZoomLevel(double zoomFactor) {
    // Can update zoom-related UI state here
    // For example, disable/enable zoom buttons based on current zoom level
    Q_UNUSED(zoomFactor)
}

void ToolBar::setActionsEnabled(bool enabled) {
    // File operations are always available
    openAction->setEnabled(true);
    openFolderAction->setEnabled(true);
    saveAction->setEnabled(enabled);

    // Document-related operations are only available when there is a document
    firstPageAction->setEnabled(enabled);
    prevPageAction->setEnabled(enabled);
    nextPageAction->setEnabled(enabled);
    lastPageAction->setEnabled(enabled);
    pageSpinBox->setEnabled(enabled);

    zoomInAction->setEnabled(enabled);
    zoomOutAction->setEnabled(enabled);
    fitWidthAction->setEnabled(enabled);
    fitPageAction->setEnabled(enabled);
    fitHeightAction->setEnabled(enabled);

    viewModeCombo->setEnabled(enabled);

    rotateLeftAction->setEnabled(enabled);
    rotateRightAction->setEnabled(enabled);

    // Sidebar and theme toggle are always available
    toggleSidebarAction->setEnabled(true);
    themeToggleAction->setEnabled(true);
}

void ToolBar::onPageSpinBoxChanged(int pageNumber) {
    // Emit page jump request (convert to 0-based)
    emit pageJumpRequested(pageNumber - 1);
}

void ToolBar::onViewModeChanged() {
    int mode = viewModeCombo->currentIndex();
    if (mode == 0) {
        emit actionTriggered(ActionMap::setSinglePageMode);
    } else if (mode == 1) {
        emit actionTriggered(ActionMap::setContinuousScrollMode);
    }
}

void ToolBar::retranslateUi() {
    // Update all tooltips and text with new translations
    openAction->setToolTip(tr("Open PDF File (Ctrl+O)"));
    openFolderAction->setToolTip(tr("Open Folder (Ctrl+Shift+O)"));
    saveAction->setToolTip(tr("Save File (Ctrl+S)"));
    
    firstPageAction->setToolTip(tr("First Page (Ctrl+Home)"));
    prevPageAction->setToolTip(tr("Previous Page (Page Up)"));
    nextPageAction->setToolTip(tr("Next Page (Page Down)"));
    lastPageAction->setToolTip(tr("Last Page (Ctrl+End)"));
    pageSpinBox->setToolTip(tr("Current Page"));
    
    zoomOutAction->setToolTip(tr("Zoom Out (Ctrl+-)"));
    zoomInAction->setToolTip(tr("Zoom In (Ctrl++)"));
    fitWidthAction->setToolTip(tr("Fit to Width (Ctrl+1)"));
    fitPageAction->setToolTip(tr("Fit to Page (Ctrl+0)"));
    fitHeightAction->setToolTip(tr("Fit to Height (Ctrl+2)"));
    
    toggleSidebarAction->setToolTip(tr("Toggle Sidebar (F9)"));
    
    // Update combo box items
    int currentViewMode = viewModeCombo->currentIndex();
    viewModeCombo->blockSignals(true);
    viewModeCombo->clear();
    viewModeCombo->addItem(tr("Single Page View"));
    viewModeCombo->addItem(tr("Continuous Scroll"));
    viewModeCombo->setCurrentIndex(currentViewMode);
    viewModeCombo->setToolTip(tr("Select View Mode"));
    viewModeCombo->blockSignals(false);
    
    rotateLeftAction->setToolTip(tr("Rotate Left 90° (Ctrl+L)"));
    rotateRightAction->setToolTip(tr("Rotate Right 90° (Ctrl+R)"));
    
    themeToggleAction->setToolTip(tr("Toggle Theme (Ctrl+T)"));
}

void ToolBar::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QToolBar::changeEvent(event);
}
