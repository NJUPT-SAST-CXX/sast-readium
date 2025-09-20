#include "ToolBar.h"
#include "../../managers/StyleManager.h"
#include <QAction>
#include <QHBoxLayout>
#include <QWidget>
#include <QEvent>
#include <QDateTime>
#include <QEasingCurve>
#include <QGraphicsDropShadowEffect>
#include <QPushButton>
#include <QStyle>
#include <QGridLayout>

// CollapsibleSection Implementation
CollapsibleSection::CollapsibleSection(const QString& title, QWidget* parent)
    : QWidget(parent), m_expanded(true) {
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Create header with toggle button
    QFrame* headerFrame = new QFrame(this);
    headerFrame->setFrameStyle(QFrame::StyledPanel);
    headerFrame->setStyleSheet(
        "QFrame {"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "   stop:0 #f8f9fa, stop:1 #e9ecef);"
        "   border: 1px solid #dee2e6;"
        "   border-radius: 4px;"
        "   padding: 2px;"
        "}"
    );
    
    QHBoxLayout* headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(4, 2, 4, 2);
    
    m_toggleButton = new QToolButton(this);
    m_toggleButton->setText(title);
    m_toggleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toggleButton->setArrowType(Qt::DownArrow);
    m_toggleButton->setCheckable(true);
    m_toggleButton->setChecked(true);
    m_toggleButton->setStyleSheet(
        "QToolButton {"
        "   border: none;"
        "   padding: 4px;"
        "   font-weight: bold;"
        "   color: #495057;"
        "}"
        "QToolButton:hover {"
        "   background-color: rgba(0, 123, 255, 0.1);"
        "   border-radius: 4px;"
        "}"
    );
    
    headerLayout->addWidget(m_toggleButton);
    headerLayout->addStretch();
    
    // Create content frame
    m_contentFrame = new QFrame(this);
    m_contentFrame->setFrameStyle(QFrame::NoFrame);
    m_contentFrame->setStyleSheet(
        "QFrame {"
        "   background-color: #ffffff;"
        "   border: 1px solid #dee2e6;"
        "   border-top: none;"
        "   border-radius: 0 0 4px 4px;"
        "   padding: 8px;"
        "}"
    );
    
    // Setup animation
    m_animation = new QPropertyAnimation(m_contentFrame, "maximumHeight", this);
    m_animation->setDuration(200);
    m_animation->setEasingCurve(QEasingCurve::InOutQuad);
    
    mainLayout->addWidget(headerFrame);
    mainLayout->addWidget(m_contentFrame);
    
    connect(m_toggleButton, &QToolButton::toggled, this, [this](bool checked) {
        setExpanded(checked);
    });
}

void CollapsibleSection::setContentWidget(QWidget* widget) {
    if (m_contentWidget) {
        delete m_contentWidget;
    }
    
    m_contentWidget = widget;
    QVBoxLayout* contentLayout = new QVBoxLayout(m_contentFrame);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->addWidget(widget);
}

void CollapsibleSection::setExpanded(bool expanded) {
    if (m_expanded == expanded) return;
    
    m_expanded = expanded;
    m_toggleButton->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
    
    m_animation->stop();
    m_animation->setStartValue(m_contentFrame->height());
    m_animation->setEndValue(expanded ? m_contentFrame->sizeHint().height() : 0);
    m_animation->start();
    
    emit expandedChanged(expanded);
}

void CollapsibleSection::toggleExpanded() {
    setExpanded(!m_expanded);
}

// ToolBar Implementation
ToolBar::ToolBar(QWidget* parent) 
    : QToolBar(parent), m_compactMode(false), m_isHovered(false) {
    
    setObjectName("MainToolBar");
    setMovable(true);
    setIconSize(QSize(24, 24));
    
    // Setup main container widget
    QWidget* containerWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(containerWidget);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    
    // Setup quick access bar at top
    setupQuickAccessBar();
    mainLayout->addWidget(m_quickAccessBar);
    
    // Create horizontal layout for sections
    QHBoxLayout* sectionsLayout = new QHBoxLayout();
    sectionsLayout->setSpacing(12);
    
    // Setup all sections
    setupFileSection();
    setupNavigationSection();
    setupZoomSection();
    setupViewSection();
    setupToolsSection();
    
    sectionsLayout->addWidget(m_fileSection);
    sectionsLayout->addWidget(m_navigationSection);
    sectionsLayout->addWidget(m_zoomSection);
    sectionsLayout->addWidget(m_viewSection);
    sectionsLayout->addWidget(m_toolsSection);
    sectionsLayout->addStretch();
    
    mainLayout->addLayout(sectionsLayout);
    
    // Add document info at bottom
    QHBoxLayout* infoLayout = new QHBoxLayout();
    m_documentInfoLabel = new QLabel(tr("No Document"), this);
    m_fileSizeLabel = new QLabel("", this);
    m_lastModifiedLabel = new QLabel("", this);
    
    infoLayout->addWidget(m_documentInfoLabel);
    infoLayout->addWidget(m_fileSizeLabel);
    infoLayout->addWidget(m_lastModifiedLabel);
    infoLayout->addStretch();
    
    mainLayout->addLayout(infoLayout);
    
    addWidget(containerWidget);
    
    // Apply enhanced styling
    applyEnhancedStyle();
    
    // Initialize animations
    m_hoverAnimation = new QPropertyAnimation(this, "minimumHeight", this);
    m_hoverAnimation->setDuration(150);
    m_hoverAnimation->setEasingCurve(QEasingCurve::InOutQuad);

    // Connect section expand/collapse signals
    connect(m_fileSection, &CollapsibleSection::expandedChanged, this, &ToolBar::onSectionExpandChanged);
    connect(m_navigationSection, &CollapsibleSection::expandedChanged, this, &ToolBar::onSectionExpandChanged);
    connect(m_zoomSection, &CollapsibleSection::expandedChanged, this, &ToolBar::onSectionExpandChanged);
    connect(m_viewSection, &CollapsibleSection::expandedChanged, this, &ToolBar::onSectionExpandChanged);
    connect(m_toolsSection, &CollapsibleSection::expandedChanged, this, &ToolBar::onSectionExpandChanged);

    // Set initial state
    setActionsEnabled(false);
}

void ToolBar::setupFileSection() {
    m_fileSection = new CollapsibleSection(tr("File"), this);
    
    QWidget* content = new QWidget();
    QGridLayout* layout = new QGridLayout(content);
    layout->setSpacing(4);
    
    // Create file actions with icons
    m_openAction = new QAction("📁", this);
    m_openAction->setToolTip(tr("Open PDF File (Ctrl+O)"));
    m_openAction->setShortcut(QKeySequence::Open);
    
    m_openFolderAction = new QAction("📂", this);
    m_openFolderAction->setToolTip(tr("Open Folder (Ctrl+Shift+O)"));
    m_openFolderAction->setShortcut(QKeySequence("Ctrl+Shift+O"));
    
    m_saveAction = new QAction("💾", this);
    m_saveAction->setToolTip(tr("Save File (Ctrl+S)"));
    m_saveAction->setShortcut(QKeySequence::Save);
    
    m_saveAsAction = new QAction("💾", this);
    m_saveAsAction->setToolTip(tr("Save As (Ctrl+Shift+S)"));
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    
    m_printAction = new QAction("🖨️", this);
    m_printAction->setToolTip(tr("Print (Ctrl+P)"));
    m_printAction->setShortcut(QKeySequence::Print);
    
    m_emailAction = new QAction("📧", this);
    m_emailAction->setToolTip(tr("Email Document"));
    
    // Create tool buttons
    auto createButton = [](QAction* action) {
        QToolButton* btn = new QToolButton();
        btn->setDefaultAction(action);
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setMinimumSize(60, 60);
        btn->setStyleSheet(
            "QToolButton {"
            "   border: 1px solid transparent;"
            "   border-radius: 4px;"
            "   padding: 4px;"
            "}"
            "QToolButton:hover {"
            "   background-color: rgba(0, 123, 255, 0.1);"
            "   border: 1px solid #007bff;"
            "}"
            "QToolButton:pressed {"
            "   background-color: rgba(0, 123, 255, 0.2);"
            "}"
        );
        return btn;
    };
    
    layout->addWidget(createButton(m_openAction), 0, 0);
    layout->addWidget(createButton(m_openFolderAction), 0, 1);
    layout->addWidget(createButton(m_saveAction), 1, 0);
    layout->addWidget(createButton(m_saveAsAction), 1, 1);
    layout->addWidget(createButton(m_printAction), 2, 0);
    layout->addWidget(createButton(m_emailAction), 2, 1);
    
    m_fileSection->setContentWidget(content);
    
    // Connect signals
    connect(m_openAction, &QAction::triggered, [this]() {
        emit actionTriggered(ActionMap::openFile);
    });
    connect(m_openFolderAction, &QAction::triggered, [this]() {
        emit actionTriggered(ActionMap::openFolder);
    });
    connect(m_saveAction, &QAction::triggered, [this]() {
        emit actionTriggered(ActionMap::save);
    });
}

void ToolBar::setupNavigationSection() {
    m_navigationSection = new CollapsibleSection(tr("Navigation"), this);
    
    QWidget* content = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(content);
    
    // Navigation controls
    QHBoxLayout* navLayout = new QHBoxLayout();
    
    m_firstPageAction = new QAction("⏮", this);
    m_firstPageAction->setToolTip(tr("First Page (Ctrl+Home)"));
    m_prevPageAction = new QAction("◀", this);
    m_prevPageAction->setToolTip(tr("Previous Page (Page Up)"));
    m_nextPageAction = new QAction("▶", this);
    m_nextPageAction->setToolTip(tr("Next Page (Page Down)"));
    m_lastPageAction = new QAction("⏭", this);
    m_lastPageAction->setToolTip(tr("Last Page (Ctrl+End)"));
    
    auto createNavButton = [](QAction* action) {
        QToolButton* btn = new QToolButton();
        btn->setDefaultAction(action);
        btn->setMinimumSize(32, 32);
        return btn;
    };
    
    navLayout->addWidget(createNavButton(m_firstPageAction));
    navLayout->addWidget(createNavButton(m_prevPageAction));
    
    // Page input
    m_pageSpinBox = new QSpinBox();
    m_pageSpinBox->setMinimum(1);
    m_pageSpinBox->setMaximum(1);
    m_pageSpinBox->setMinimumWidth(60);
    m_pageSpinBox->setToolTip(tr("Current Page"));
    
    m_pageCountLabel = new QLabel("/ 1");
    
    navLayout->addWidget(m_pageSpinBox);
    navLayout->addWidget(m_pageCountLabel);
    navLayout->addWidget(createNavButton(m_nextPageAction));
    navLayout->addWidget(createNavButton(m_lastPageAction));
    
    layout->addLayout(navLayout);
    
    // Page slider
    m_pageSlider = new QSlider(Qt::Horizontal);
    m_pageSlider->setMinimum(1);
    m_pageSlider->setMaximum(1);
    m_pageSlider->setTickPosition(QSlider::TicksBelow);
    m_pageSlider->setStyleSheet(
        "QSlider::groove:horizontal {"
        "   border: 1px solid #999999;"
        "   height: 8px;"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "       stop:0 #B1B1B1, stop:1 #c4c4c4);"
        "   margin: 2px 0;"
        "   border-radius: 4px;"
        "}"
        "QSlider::handle:horizontal {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "       stop:0 #007bff, stop:1 #0056b3);"
        "   border: 1px solid #5c5c5c;"
        "   width: 18px;"
        "   margin: -2px 0;"
        "   border-radius: 9px;"
        "}"
    );
    
    layout->addWidget(m_pageSlider);
    
    // Thumbnail preview
    m_thumbnailPreview = new QLabel();
    m_thumbnailPreview->setMinimumSize(120, 160);
    m_thumbnailPreview->setMaximumSize(120, 160);
    m_thumbnailPreview->setFrameStyle(QFrame::Box);
    m_thumbnailPreview->setAlignment(Qt::AlignCenter);
    m_thumbnailPreview->setText(tr("Page Preview"));
    m_thumbnailPreview->setStyleSheet(
        "QLabel {"
        "   background-color: #f8f9fa;"
        "   border: 2px solid #dee2e6;"
        "   border-radius: 4px;"
        "}"
    );
    
    layout->addWidget(m_thumbnailPreview, 0, Qt::AlignCenter);
    
    m_navigationSection->setContentWidget(content);
    
    // Connect signals
    connect(m_pageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ToolBar::onPageSpinBoxChanged);
    connect(m_pageSlider, &QSlider::valueChanged, [this](int value) {
        m_pageSpinBox->setValue(value);
    });
    connect(m_firstPageAction, &QAction::triggered, [this]() {
        emit actionTriggered(ActionMap::firstPage);
    });
    connect(m_prevPageAction, &QAction::triggered, [this]() {
        emit actionTriggered(ActionMap::previousPage);
    });
    connect(m_nextPageAction, &QAction::triggered, [this]() {
        emit actionTriggered(ActionMap::nextPage);
    });
    connect(m_lastPageAction, &QAction::triggered, [this]() {
        emit actionTriggered(ActionMap::lastPage);
    });
}

void ToolBar::setupZoomSection() {
    m_zoomSection = new CollapsibleSection(tr("Zoom"), this);
    
    QWidget* content = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(content);
    
    // Zoom controls
    QHBoxLayout* zoomButtonsLayout = new QHBoxLayout();
    
    m_zoomOutAction = new QAction("🔍-", this);
    m_zoomOutAction->setToolTip(tr("Zoom Out (Ctrl+-)"));
    m_zoomInAction = new QAction("🔍+", this);
    m_zoomInAction->setToolTip(tr("Zoom In (Ctrl++)"));
    
    auto createZoomButton = [](QAction* action) {
        QToolButton* btn = new QToolButton();
        btn->setDefaultAction(action);
        btn->setMinimumSize(32, 32);
        return btn;
    };
    
    zoomButtonsLayout->addWidget(createZoomButton(m_zoomOutAction));
    
    // Zoom slider
    m_zoomSlider = new QSlider(Qt::Horizontal);
    m_zoomSlider->setMinimum(25);
    m_zoomSlider->setMaximum(400);
    m_zoomSlider->setValue(100);
    m_zoomSlider->setTickInterval(25);
    m_zoomSlider->setTickPosition(QSlider::TicksBelow);
    
    m_zoomValueLabel = new QLabel("100%");
    m_zoomValueLabel->setMinimumWidth(50);
    m_zoomValueLabel->setAlignment(Qt::AlignCenter);
    
    zoomButtonsLayout->addWidget(m_zoomSlider);
    zoomButtonsLayout->addWidget(m_zoomValueLabel);
    zoomButtonsLayout->addWidget(createZoomButton(m_zoomInAction));
    
    layout->addLayout(zoomButtonsLayout);
    
    // Zoom presets
    m_zoomPresets = new QComboBox();
    m_zoomPresets->addItems({"50%", "75%", "100%", "125%", "150%", "200%", "300%", "400%"});
    m_zoomPresets->setCurrentText("100%");
    m_zoomPresets->setEditable(true);
    
    layout->addWidget(m_zoomPresets);
    
    // Fit options
    QHBoxLayout* fitLayout = new QHBoxLayout();
    
    m_fitWidthAction = new QAction("📏", this);
    m_fitWidthAction->setToolTip(tr("Fit to Width (Ctrl+1)"));
    m_fitPageAction = new QAction("🗎", this);
    m_fitPageAction->setToolTip(tr("Fit to Page (Ctrl+0)"));
    m_fitHeightAction = new QAction("📐", this);
    m_fitHeightAction->setToolTip(tr("Fit to Height (Ctrl+2)"));
    
    fitLayout->addWidget(createZoomButton(m_fitWidthAction));
    fitLayout->addWidget(createZoomButton(m_fitPageAction));
    fitLayout->addWidget(createZoomButton(m_fitHeightAction));
    
    layout->addLayout(fitLayout);
    
    m_zoomSection->setContentWidget(content);
    
    // Connect signals
    connect(m_zoomSlider, &QSlider::valueChanged, this, &ToolBar::onZoomSliderChanged);
    connect(m_zoomPresets, &QComboBox::currentTextChanged, [this](const QString& text) {
        QString percentText = text;
        percentText.remove('%');
        bool ok;
        int value = percentText.toInt(&ok);
        if (ok) {
            m_zoomSlider->setValue(value);
        }
    });
    
    connect(m_zoomInAction, &QAction::triggered, [this]() {
        emit actionTriggered(ActionMap::zoomIn);
    });
    connect(m_zoomOutAction, &QAction::triggered, [this]() {
        emit actionTriggered(ActionMap::zoomOut);
    });
    connect(m_fitWidthAction, &QAction::triggered, [this]() {
        emit actionTriggered(ActionMap::fitToWidth);
    });
    connect(m_fitPageAction, &QAction::triggered, [this]() {
        emit actionTriggered(ActionMap::fitToPage);
    });
    connect(m_fitHeightAction, &QAction::triggered, [this]() {
        emit actionTriggered(ActionMap::fitToHeight);
    });
}

void ToolBar::setupViewSection() {
    m_viewSection = new CollapsibleSection(tr("View"), this);
    
    QWidget* content = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(content);
    
    // View mode combo
    m_viewModeCombo = new QComboBox();
    m_viewModeCombo->addItems({tr("Single Page"), tr("Continuous"), tr("Two Pages"), tr("Book View")});
    m_viewModeCombo->setToolTip(tr("Select View Mode"));
    layout->addWidget(m_viewModeCombo);
    
    // Layout combo
    m_layoutCombo = new QComboBox();
    m_layoutCombo->addItems({tr("Vertical"), tr("Horizontal")});
    layout->addWidget(m_layoutCombo);
    
    // View actions
    QGridLayout* viewActionsLayout = new QGridLayout();
    
    m_toggleSidebarAction = new QAction("📋", this);
    m_toggleSidebarAction->setToolTip(tr("Toggle Sidebar (F9)"));
    m_toggleSidebarAction->setCheckable(true);
    m_toggleSidebarAction->setChecked(true);
    
    m_toggleFullscreenAction = new QAction("🖥️", this);
    m_toggleFullscreenAction->setToolTip(tr("Fullscreen"));
    m_toggleFullscreenAction->setCheckable(true);
    
    m_nightModeAction = new QAction("🌙", this);
    m_nightModeAction->setToolTip(tr("Night Mode"));
    m_nightModeAction->setCheckable(true);
    
    m_readingModeAction = new QAction("📖", this);
    m_readingModeAction->setToolTip(tr("Reading Mode"));
    m_readingModeAction->setCheckable(true);
    
    auto createViewButton = [](QAction* action) {
        QToolButton* btn = new QToolButton();
        btn->setDefaultAction(action);
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setMinimumSize(50, 50);
        return btn;
    };
    
    viewActionsLayout->addWidget(createViewButton(m_toggleSidebarAction), 0, 0);
    viewActionsLayout->addWidget(createViewButton(m_toggleFullscreenAction), 0, 1);
    viewActionsLayout->addWidget(createViewButton(m_nightModeAction), 1, 0);
    viewActionsLayout->addWidget(createViewButton(m_readingModeAction), 1, 1);
    
    layout->addLayout(viewActionsLayout);
    
    m_viewSection->setContentWidget(content);
    
    // Connect signals
    connect(m_viewModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolBar::onViewModeChanged);
    connect(m_toggleSidebarAction, &QAction::triggered, [this]() {
        emit actionTriggered(ActionMap::toggleSideBar);
    });
}

void ToolBar::setupToolsSection() {
    m_toolsSection = new CollapsibleSection(tr("Tools"), this);
    
    QWidget* content = new QWidget();
    QGridLayout* layout = new QGridLayout(content);
    layout->setSpacing(4);
    
    // Create tool actions
    m_searchAction = new QAction("🔍", this);
    m_searchAction->setToolTip(tr("Search (Ctrl+F)"));
    m_searchAction->setShortcut(QKeySequence::Find);
    
    m_annotateAction = new QAction("✏️", this);
    m_annotateAction->setToolTip(tr("Annotate"));
    m_highlightAction = new QAction("🖆", this);
    m_highlightAction->setToolTip(tr("Highlight"));
    m_bookmarkAction = new QAction("🔖", this);
    m_bookmarkAction->setToolTip(tr("Bookmark"));
    m_snapshotAction = new QAction("📷", this);
    m_snapshotAction->setToolTip(tr("Snapshot"));
    
    m_rotateLeftAction = new QAction("↺", this);
    m_rotateLeftAction->setToolTip(tr("Rotate Left 90° (Ctrl+L)"));
    m_rotateRightAction = new QAction("↻", this);
    m_rotateRightAction->setToolTip(tr("Rotate Right 90° (Ctrl+R)"));
    
    auto createToolButton = [](QAction* action) {
        QToolButton* btn = new QToolButton();
        btn->setDefaultAction(action);
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setMinimumSize(60, 60);
        return btn;
    };
    
    layout->addWidget(createToolButton(m_searchAction), 0, 0);
    layout->addWidget(createToolButton(m_annotateAction), 0, 1);
    layout->addWidget(createToolButton(m_highlightAction), 0, 2);
    layout->addWidget(createToolButton(m_bookmarkAction), 1, 0);
    layout->addWidget(createToolButton(m_snapshotAction), 1, 1);
    layout->addWidget(createToolButton(m_rotateLeftAction), 2, 0);
    layout->addWidget(createToolButton(m_rotateRightAction), 2, 1);
    
    m_toolsSection->setContentWidget(content);
    
    // Connect signals
    connect(m_rotateLeftAction, &QAction::triggered, [this]() {
        emit actionTriggered(ActionMap::rotateLeft);
    });
    connect(m_rotateRightAction, &QAction::triggered, [this]() {
        emit actionTriggered(ActionMap::rotateRight);
    });
}

void ToolBar::setupQuickAccessBar() {
    m_quickAccessBar = new QFrame(this);
    m_quickAccessBar->setFrameStyle(QFrame::NoFrame);
    m_quickAccessBar->setStyleSheet(
        "QFrame {"
        "   background-color: #f8f9fa;"
        "   border-bottom: 1px solid #dee2e6;"
        "   padding: 4px;"
        "}"
    );
    
    QHBoxLayout* layout = new QHBoxLayout(m_quickAccessBar);
    layout->setContentsMargins(8, 4, 8, 4);
    
    // Logo or app name
    QLabel* appLabel = new QLabel("📖 SAST Readium");
    appLabel->setStyleSheet(
        "QLabel {"
        "   font-size: 16px;"
        "   font-weight: bold;"
        "   color: #007bff;"
        "}"
    );
    layout->addWidget(appLabel);
    
    layout->addStretch();
    
    // Quick access buttons
    m_themeToggleAction = new QAction("🌙", this);
    m_themeToggleAction->setToolTip(tr("Toggle Theme (Ctrl+T)"));
    m_settingsAction = new QAction("⚙️", this);
    m_settingsAction->setToolTip(tr("Settings"));
    m_helpAction = new QAction("❓", this);
    m_helpAction->setToolTip(tr("Help"));
    
    auto createQuickButton = [](QAction* action) {
        QToolButton* btn = new QToolButton();
        btn->setDefaultAction(action);
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        btn->setMinimumSize(32, 32);
        btn->setStyleSheet(
            "QToolButton {"
            "   border: none;"
            "   border-radius: 16px;"
            "   padding: 4px;"
            "}"
            "QToolButton:hover {"
            "   background-color: rgba(0, 0, 0, 0.05);"
            "}"
        );
        return btn;
    };
    
    layout->addWidget(createQuickButton(m_themeToggleAction));
    layout->addWidget(createQuickButton(m_settingsAction));
    layout->addWidget(createQuickButton(m_helpAction));
    
    connect(m_themeToggleAction, &QAction::triggered, [this]() {
        emit actionTriggered(ActionMap::toggleTheme);
    });
}

void ToolBar::applyEnhancedStyle() {
    setStyleSheet(
        "QToolBar {"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "       stop:0 #ffffff, stop:1 #f8f9fa);"
        "   border: none;"
        "   border-bottom: 2px solid #dee2e6;"
        "   padding: 8px;"
        "}"
        "QLabel {"
        "   color: #495057;"
        "   font-size: 12px;"
        "}"
        "QComboBox {"
        "   border: 1px solid #ced4da;"
        "   border-radius: 4px;"
        "   padding: 4px;"
        "   background-color: white;"
        "}"
        "QComboBox:hover {"
        "   border-color: #80bdff;"
        "}"
        "QComboBox:focus {"
        "   border-color: #007bff;"
        "   outline: none;"
        "}"
        "QSpinBox {"
        "   border: 1px solid #ced4da;"
        "   border-radius: 4px;"
        "   padding: 4px;"
        "   background-color: white;"
        "}"
        "QSpinBox:hover {"
        "   border-color: #80bdff;"
        "}"
        "QSpinBox:focus {"
        "   border-color: #007bff;"
        "}"
    );
    
    // Add drop shadow effect
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(10);
    shadow->setColor(QColor(0, 0, 0, 30));
    shadow->setOffset(0, 2);
    setGraphicsEffect(shadow);
}

void ToolBar::updatePageInfo(int currentPage, int totalPages) {
    m_pageSpinBox->blockSignals(true);
    m_pageSlider->blockSignals(true);
    
    m_pageSpinBox->setMaximum(totalPages);
    m_pageSpinBox->setValue(currentPage + 1);
    m_pageCountLabel->setText(QString("/ %1").arg(totalPages));
    
    m_pageSlider->setMaximum(totalPages);
    m_pageSlider->setValue(currentPage + 1);
    
    m_pageSpinBox->blockSignals(false);
    m_pageSlider->blockSignals(false);
    
    // Update navigation button states
    m_firstPageAction->setEnabled(currentPage > 0);
    m_prevPageAction->setEnabled(currentPage > 0);
    m_nextPageAction->setEnabled(currentPage < totalPages - 1);
    m_lastPageAction->setEnabled(currentPage < totalPages - 1);
}

void ToolBar::updateZoomLevel(double zoomFactor) {
    int percentage = static_cast<int>(zoomFactor * 100);
    
    m_zoomSlider->blockSignals(true);
    m_zoomSlider->setValue(percentage);
    m_zoomSlider->blockSignals(false);
    
    m_zoomValueLabel->setText(QString("%1%").arg(percentage));
    m_zoomPresets->setCurrentText(QString("%1%").arg(percentage));
}

void ToolBar::updateDocumentInfo(const QString& fileName, qint64 fileSize, 
                                const QDateTime& lastModified) {
    m_documentInfoLabel->setText(fileName);
    
    // Format file size
    QString sizeText;
    if (fileSize < 1024) {
        sizeText = QString("%1 B").arg(fileSize);
    } else if (fileSize < 1024 * 1024) {
        sizeText = QString("%1 KB").arg(fileSize / 1024.0, 0, 'f', 2);
    } else {
        sizeText = QString("%1 MB").arg(fileSize / (1024.0 * 1024.0), 0, 'f', 2);
    }
    m_fileSizeLabel->setText(tr("Size: %1").arg(sizeText));
    
    // Format last modified date
    m_lastModifiedLabel->setText(tr("Modified: %1").arg(lastModified.toString("yyyy-MM-dd hh:mm")));
}

void ToolBar::setActionsEnabled(bool enabled) {
    // File actions always enabled except save
    m_openAction->setEnabled(true);
    m_openFolderAction->setEnabled(true);
    m_saveAction->setEnabled(enabled);
    m_saveAsAction->setEnabled(enabled);
    m_printAction->setEnabled(enabled);
    m_emailAction->setEnabled(enabled);
    
    // Navigation actions
    m_navigationSection->setEnabled(enabled);
    
    // Zoom actions
    m_zoomSection->setEnabled(enabled);
    
    // View actions
    m_viewSection->setEnabled(enabled);
    
    // Tool actions
    m_toolsSection->setEnabled(enabled);
}

void ToolBar::setCompactMode(bool compact) {
    m_compactMode = compact;
    
    if (compact) {
        // Collapse all sections in compact mode
        m_fileSection->setExpanded(false);
        m_navigationSection->setExpanded(false);
        m_zoomSection->setExpanded(false);
        m_viewSection->setExpanded(false);
        m_toolsSection->setExpanded(false);
    } else {
        // Expand important sections
        m_navigationSection->setExpanded(true);
        m_zoomSection->setExpanded(true);
    }
}

void ToolBar::onPageSpinBoxChanged(int pageNumber) {
    emit pageJumpRequested(pageNumber - 1);
    m_pageSlider->setValue(pageNumber);
}

void ToolBar::onViewModeChanged() {
    int mode = m_viewModeCombo->currentIndex();
    switch (mode) {
        case 0:
            emit actionTriggered(ActionMap::setSinglePageMode);
            break;
        case 1:
            emit actionTriggered(ActionMap::setContinuousScrollMode);
            break;
        // Add more cases for other view modes
    }
}

void ToolBar::onZoomSliderChanged(int value) {
    m_zoomValueLabel->setText(QString("%1%").arg(value));
    m_zoomPresets->setCurrentText(QString("%1%").arg(value));
    emit zoomLevelChanged(value);
}

void ToolBar::onSectionExpandChanged(bool expanded) {
    // Get the sender section to identify which section changed
    CollapsibleSection* section = qobject_cast<CollapsibleSection*>(sender());
    if (!section) {
        return;
    }

    // Update layout and animations based on section state
    if (expanded) {
        // Section was expanded - ensure proper spacing and visibility
        section->adjustSize();

        // If in compact mode, temporarily expand the toolbar
        if (m_compactMode && m_hoverAnimation) {
            m_hoverAnimation->setStartValue(height());
            m_hoverAnimation->setEndValue(sizeHint().height());
            m_hoverAnimation->start();
        }
    } else {
        // Section was collapsed - optimize space usage
        section->adjustSize();

        // In compact mode, check if we can shrink the toolbar
        if (m_compactMode) {
            bool anyExpanded = m_fileSection->isExpanded() ||
                              m_navigationSection->isExpanded() ||
                              m_zoomSection->isExpanded() ||
                              m_viewSection->isExpanded() ||
                              m_toolsSection->isExpanded();

            if (!anyExpanded && m_hoverAnimation) {
                // All sections collapsed, shrink toolbar
                m_hoverAnimation->setStartValue(height());
                m_hoverAnimation->setEndValue(minimumSizeHint().height());
                m_hoverAnimation->start();
            }
        }
    }

    // Emit signal for external components that might need to respond
    emit sectionExpandChanged(section->windowTitle(), expanded);
}

void ToolBar::enterEvent(QEnterEvent* event) {
    Q_UNUSED(event);
    m_isHovered = true;
    
    if (m_compactMode) {
        // Expand on hover in compact mode
        m_hoverAnimation->setStartValue(height());
        m_hoverAnimation->setEndValue(sizeHint().height());
        m_hoverAnimation->start();
    }
}

void ToolBar::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    m_isHovered = false;
    
    if (m_compactMode) {
        // Collapse on leave in compact mode
        m_hoverAnimation->setStartValue(height());
        m_hoverAnimation->setEndValue(60); // Minimum height in compact mode
        m_hoverAnimation->start();
    }
}

void ToolBar::retranslateUi() {
    // Update section titles
    m_fileSection->setWindowTitle(tr("File"));
    m_navigationSection->setWindowTitle(tr("Navigation"));
    m_zoomSection->setWindowTitle(tr("Zoom"));
    m_viewSection->setWindowTitle(tr("View"));
    m_toolsSection->setWindowTitle(tr("Tools"));
    
    // Update all tooltips and text with new translations
    m_openAction->setToolTip(tr("Open PDF File (Ctrl+O)"));
    m_openFolderAction->setToolTip(tr("Open Folder (Ctrl+Shift+O)"));
    m_saveAction->setToolTip(tr("Save File (Ctrl+S)"));
    
    m_firstPageAction->setToolTip(tr("First Page (Ctrl+Home)"));
    m_prevPageAction->setToolTip(tr("Previous Page (Page Up)"));
    m_nextPageAction->setToolTip(tr("Next Page (Page Down)"));
    m_lastPageAction->setToolTip(tr("Last Page (Ctrl+End)"));
    m_pageSpinBox->setToolTip(tr("Current Page"));
    
    m_zoomOutAction->setToolTip(tr("Zoom Out (Ctrl+-)"));
    m_zoomInAction->setToolTip(tr("Zoom In (Ctrl++)"));
    m_fitWidthAction->setToolTip(tr("Fit to Width (Ctrl+1)"));
    m_fitPageAction->setToolTip(tr("Fit to Page (Ctrl+0)"));
    m_fitHeightAction->setToolTip(tr("Fit to Height (Ctrl+2)"));
    
    m_toggleSidebarAction->setToolTip(tr("Toggle Sidebar (F9)"));
    
    // Update combo box items
    int currentViewMode = m_viewModeCombo->currentIndex();
    m_viewModeCombo->blockSignals(true);
    m_viewModeCombo->clear();
    m_viewModeCombo->addItems({tr("Single Page"), tr("Continuous"), tr("Two Pages"), tr("Book View")});
    m_viewModeCombo->setCurrentIndex(currentViewMode);
    m_viewModeCombo->setToolTip(tr("Select View Mode"));
    m_viewModeCombo->blockSignals(false);
    
    m_rotateLeftAction->setToolTip(tr("Rotate Left 90° (Ctrl+L)"));
    m_rotateRightAction->setToolTip(tr("Rotate Right 90° (Ctrl+R)"));
    
    m_themeToggleAction->setToolTip(tr("Toggle Theme (Ctrl+T)"));
}

void ToolBar::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QToolBar::changeEvent(event);
}
