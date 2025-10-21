#include "ToolBar.h"
#include <QAction>
#include <QDateTime>
#include <QEasingCurve>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStyle>
#include <QWidget>
#include "../../logging/LoggingMacros.h"
#include "../../managers/StyleManager.h"

// CollapsibleSection Implementation
CollapsibleSection::CollapsibleSection(const QString& title, QWidget* parent)
    : QWidget(parent), m_expanded(true) {
    StyleManager* styleManager = &StyleManager::instance();

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
        "}");

    QHBoxLayout* headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(
        styleManager->spacingXS(), styleManager->spacingXS() / 2,
        styleManager->spacingXS(), styleManager->spacingXS() / 2);

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
        "}");

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
        "}");

    // Setup animation
    m_animation = new QPropertyAnimation(m_contentFrame, "maximumHeight", this);
    m_animation->setDuration(styleManager->animationNormal());
    m_animation->setEasingCurve(QEasingCurve::InOutQuad);

    mainLayout->addWidget(headerFrame);
    mainLayout->addWidget(m_contentFrame);

    connect(m_toggleButton, &QToolButton::toggled, this,
            [this](bool checked) { setExpanded(checked); });
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
    if (m_expanded == expanded) {
        return;
    }

    m_expanded = expanded;
    m_toggleButton->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);

    m_animation->stop();
    m_animation->setStartValue(m_contentFrame->height());
    m_animation->setEndValue(expanded ? m_contentFrame->sizeHint().height()
                                      : 0);
    m_animation->start();

    emit expandedChanged(expanded);
}

void CollapsibleSection::toggleExpanded() { setExpanded(!m_expanded); }

// ToolBar Implementation
ToolBar::ToolBar(QWidget* parent)
    : QToolBar(parent), m_compactMode(false), m_isHovered(false) {
    StyleManager* styleManager = &StyleManager::instance();

    setObjectName("MainToolBar");
    setMovable(true);
    setIconSize(QSize(24, 24));

    // Set size policy for toolbar
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Set consistent spacing using StyleManager
    layout()->setSpacing(styleManager->spacingXS());

    // Create simple toolbar actions without CollapsibleSection to avoid hang
    // File actions
    m_openAction = new QAction(tr("Open"), this);
    m_openAction->setToolTip(tr("Open File (Ctrl+O)"));
    m_openAction->setShortcut(QKeySequence::Open);
    addAction(m_openAction);
    connect(m_openAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::openFile); });

    m_saveAction = new QAction(tr("Save"), this);
    m_saveAction->setToolTip(tr("Save File (Ctrl+S)"));
    m_saveAction->setShortcut(QKeySequence::Save);
    addAction(m_saveAction);
    connect(m_saveAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::save); });

    addSeparator();

    // Navigation actions
    m_firstPageAction = new QAction(tr("First"), this);
    m_firstPageAction->setToolTip(tr("First Page"));
    addAction(m_firstPageAction);
    connect(m_firstPageAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::firstPage); });

    m_prevPageAction = new QAction(tr("Prev"), this);
    m_prevPageAction->setToolTip(tr("Previous Page"));
    addAction(m_prevPageAction);
    connect(m_prevPageAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::previousPage); });

    m_nextPageAction = new QAction(tr("Next"), this);
    m_nextPageAction->setToolTip(tr("Next Page"));
    addAction(m_nextPageAction);
    connect(m_nextPageAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::nextPage); });

    m_lastPageAction = new QAction(tr("Last"), this);
    m_lastPageAction->setToolTip(tr("Last Page"));
    addAction(m_lastPageAction);
    connect(m_lastPageAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::lastPage); });

    addSeparator();

    // Zoom actions
    m_zoomOutAction = new QAction(tr("Zoom Out"), this);
    m_zoomOutAction->setToolTip(tr("Zoom Out"));
    addAction(m_zoomOutAction);
    connect(m_zoomOutAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::zoomOut); });

    m_zoomInAction = new QAction(tr("Zoom In"), this);
    m_zoomInAction->setToolTip(tr("Zoom In"));
    addAction(m_zoomInAction);
    connect(m_zoomInAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::zoomIn); });

    addSeparator();

    // Theme toggle
    m_themeToggleAction = new QAction(tr("Theme"), this);
    m_themeToggleAction->setToolTip(tr("Toggle Theme"));
    addAction(m_themeToggleAction);
    connect(m_themeToggleAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::toggleTheme); });

    // Initialize ALL action pointers to nullptr (not used in simplified
    // version)
    m_openFolderAction = nullptr;
    m_saveAsAction = nullptr;
    m_printAction = nullptr;
    m_emailAction = nullptr;
    m_fitWidthAction = nullptr;
    m_fitPageAction = nullptr;
    m_fitHeightAction = nullptr;
    m_rotateLeftAction = nullptr;
    m_rotateRightAction = nullptr;
    m_searchAction = nullptr;
    m_bookmarkAction = nullptr;
    m_annotateAction = nullptr;
    m_highlightAction = nullptr;
    m_snapshotAction = nullptr;
    m_toggleSidebarAction = nullptr;
    m_toggleFullscreenAction = nullptr;
    m_nightModeAction = nullptr;
    m_readingModeAction = nullptr;
    m_settingsAction = nullptr;
    m_helpAction = nullptr;

    // Initialize section pointers to nullptr (not used in simplified version)
    m_fileSection = nullptr;
    m_navigationSection = nullptr;
    m_zoomSection = nullptr;
    m_viewSection = nullptr;
    m_toolsSection = nullptr;
    m_quickAccessBar = nullptr;
    m_hoverAnimation = nullptr;
    m_expandAnimation = nullptr;

    // Initialize widget pointers to nullptr
    m_pageSpinBox = nullptr;
    m_pageCountLabel = nullptr;
    m_pageSlider = nullptr;
    m_thumbnailPreview = nullptr;
    m_zoomSlider = nullptr;
    m_zoomValueLabel = nullptr;
    m_zoomPresets = nullptr;
    m_viewModeCombo = nullptr;
    m_layoutCombo = nullptr;
    m_documentInfoLabel = nullptr;
    m_fileSizeLabel = nullptr;
    m_lastModifiedLabel = nullptr;
}

ToolBar::~ToolBar() {
    // Stop animations if they exist
    if (m_hoverAnimation) {
        m_hoverAnimation->stop();
        // Animation will be deleted by Qt parent-child ownership
    }

    if (m_expandAnimation) {
        m_expandAnimation->stop();
        // Animation will be deleted by Qt parent-child ownership
    }

    // All widgets and actions are deleted automatically by Qt parent-child
    // ownership No manual deletion needed for widgets/actions created with
    // 'this' as parent

    LOG_DEBUG("ToolBar destroyed successfully");
}

void ToolBar::setupFileSection() {
    LOG_DEBUG("ToolBar::setupFileSection() - Creating file section");

    m_fileSection = new CollapsibleSection(tr("File"), this);

    QWidget* content = new QWidget();
    QGridLayout* layout = new QGridLayout(content);
    layout->setSpacing(4);

    // Create file actions with icons
    m_openAction = new QAction(tr("Open"), this);
    m_openAction->setToolTip(tr("Open PDF File (Ctrl+O)"));
    m_openAction->setShortcut(QKeySequence::Open);

    m_openFolderAction = new QAction(tr("Open Folder"), this);
    m_openFolderAction->setToolTip(tr("Open Folder (Ctrl+Shift+O)"));
    m_openFolderAction->setShortcut(QKeySequence("Ctrl+Shift+O"));

    m_saveAction = new QAction(tr("Save"), this);
    m_saveAction->setToolTip(tr("Save File (Ctrl+S)"));
    m_saveAction->setShortcut(QKeySequence::Save);

    m_saveAsAction = new QAction(tr("Save As"), this);
    m_saveAsAction->setToolTip(tr("Save As (Ctrl+Shift+S)"));
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);

    m_printAction = new QAction(tr("Print"), this);
    m_printAction->setToolTip(tr("Print (Ctrl+P)"));
    m_printAction->setShortcut(QKeySequence::Print);

    m_emailAction = new QAction(tr("Email"), this);
    m_emailAction->setToolTip(tr("Email Document"));

    // Create tool buttons
    auto createButton = [](QAction* action) {
        QToolButton* btn = new QToolButton();
        btn->setAccessibleName("Toolbar Button");
        btn->setToolTip(btn->text().isEmpty() ? QString("Toolbar Button")
                                              : btn->text());
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
            "}");
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
    connect(m_openAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::openFile); });
    connect(m_openFolderAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::openFolder); });
    connect(m_saveAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::save); });

    LOG_DEBUG(
        "ToolBar::setupFileSection() - File section created successfully");
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
        btn->setAccessibleName("Toolbar Button");
        btn->setToolTip(btn->text().isEmpty() ? QString("Toolbar Button")
                                              : btn->text());
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
        "}");

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
        "}");

    layout->addWidget(m_thumbnailPreview, 0, Qt::AlignCenter);

    m_navigationSection->setContentWidget(content);

    // Connect signals
    connect(m_pageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &ToolBar::onPageSpinBoxChanged);
    connect(m_pageSlider, &QSlider::valueChanged,
            [this](int value) { m_pageSpinBox->setValue(value); });
    connect(m_firstPageAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::firstPage); });
    connect(m_prevPageAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::previousPage); });
    connect(m_nextPageAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::nextPage); });
    connect(m_lastPageAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::lastPage); });
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
        btn->setAccessibleName("Toolbar Button");
        btn->setToolTip(btn->text().isEmpty() ? QString("Toolbar Button")
                                              : btn->text());
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
    m_zoomPresets->addItems(
        {"50%", "75%", "100%", "125%", "150%", "200%", "300%", "400%"});
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
    connect(m_zoomSlider, &QSlider::valueChanged, this,
            &ToolBar::onZoomSliderChanged);
    connect(m_zoomPresets, &QComboBox::currentTextChanged,
            [this](const QString& text) {
                QString percentText = text;
                percentText.remove('%');
                bool ok;
                int value = percentText.toInt(&ok);
                if (ok) {
                    m_zoomSlider->setValue(value);
                }
            });

    connect(m_zoomInAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::zoomIn); });
    connect(m_zoomOutAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::zoomOut); });
    connect(m_fitWidthAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::fitToWidth); });
    connect(m_fitPageAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::fitToPage); });
    connect(m_fitHeightAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::fitToHeight); });
}

void ToolBar::setupViewSection() {
    m_viewSection = new CollapsibleSection(tr("View"), this);

    QWidget* content = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(content);

    // View mode combo
    m_viewModeCombo = new QComboBox();
    m_viewModeCombo->addItems({tr("Single Page"), tr("Continuous"),
                               tr("Two Pages"), tr("Book View")});
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

    m_toggleFullscreenAction = new QAction("🖥", this);
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
        btn->setAccessibleName("Toolbar Button");
        btn->setToolTip(btn->text().isEmpty() ? QString("Toolbar Button")
                                              : btn->text());
        btn->setDefaultAction(action);
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setMinimumSize(50, 50);
        return btn;
    };

    viewActionsLayout->addWidget(createViewButton(m_toggleSidebarAction), 0, 0);
    viewActionsLayout->addWidget(createViewButton(m_toggleFullscreenAction), 0,
                                 1);
    viewActionsLayout->addWidget(createViewButton(m_nightModeAction), 1, 0);
    viewActionsLayout->addWidget(createViewButton(m_readingModeAction), 1, 1);

    layout->addLayout(viewActionsLayout);

    m_viewSection->setContentWidget(content);

    // Connect signals
    connect(m_viewModeCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ToolBar::onViewModeChanged);
    connect(m_toggleSidebarAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::toggleSideBar); });
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
        btn->setAccessibleName("Toolbar Button");
        btn->setToolTip(btn->text().isEmpty() ? QString("Toolbar Button")
                                              : btn->text());
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
    connect(m_rotateLeftAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::rotateLeft); });
    connect(m_rotateRightAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::rotateRight); });
}

void ToolBar::setupQuickAccessBar() {
    m_quickAccessBar = new QFrame(this);
    m_quickAccessBar->setFrameStyle(QFrame::NoFrame);
    m_quickAccessBar->setStyleSheet(
        "QFrame {"
        "   background-color: #f8f9fa;"
        "   border-bottom: 1px solid #dee2e6;"
        "   padding: 4px;"
        "}");

    QHBoxLayout* layout = new QHBoxLayout(m_quickAccessBar);
    layout->setContentsMargins(8, 4, 8, 4);

    // Logo or app name
    QLabel* appLabel = new QLabel("📖 SAST Readium");
    appLabel->setStyleSheet(
        "QLabel {"
        "   font-size: 16px;"
        "   font-weight: bold;"
        "   color: #007bff;"
        "}");
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
        btn->setAccessibleName("Toolbar Button");
        btn->setToolTip(btn->text().isEmpty() ? QString("Toolbar Button")
                                              : btn->text());
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
            "}");
        return btn;
    };

    layout->addWidget(createQuickButton(m_themeToggleAction));
    layout->addWidget(createQuickButton(m_settingsAction));
    layout->addWidget(createQuickButton(m_helpAction));

    connect(m_themeToggleAction, &QAction::triggered,
            [this]() { emit actionTriggered(ActionMap::toggleTheme); });
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
        "}");

    // Add drop shadow effect
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(10);
    shadow->setColor(QColor(0, 0, 0, 30));
    shadow->setOffset(0, 2);
    setGraphicsEffect(shadow);
}

void ToolBar::updatePageInfo(int currentPage, int totalPages) {
    // Defensive null pointer checks
    if (!m_pageSpinBox || !m_pageSlider || !m_pageCountLabel) {
        LOG_WARNING("ToolBar::updatePageInfo() - Page widgets not initialized");
        return;
    }

    if (!m_firstPageAction || !m_prevPageAction || !m_nextPageAction ||
        !m_lastPageAction) {
        LOG_WARNING(
            "ToolBar::updatePageInfo() - Navigation actions not initialized");
        return;
    }

    // Input validation
    if (totalPages < 0) {
        LOG_WARNING("ToolBar::updatePageInfo() - Invalid total pages: {}",
                    totalPages);
        totalPages = 0;
    }

    if (currentPage < 0) {
        LOG_WARNING("ToolBar::updatePageInfo() - Invalid current page: {}",
                    currentPage);
        currentPage = 0;
    }

    if (currentPage >= totalPages && totalPages > 0) {
        LOG_WARNING(
            "ToolBar::updatePageInfo() - Current page {} exceeds total pages "
            "{}",
            currentPage, totalPages);
        currentPage = totalPages - 1;
    }

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
    // Defensive null pointer checks
    if (!m_zoomSlider || !m_zoomValueLabel || !m_zoomPresets) {
        LOG_WARNING(
            "ToolBar::updateZoomLevel() - Zoom widgets not initialized");
        return;
    }

    // Input validation - reasonable zoom range is 0.1 to 5.0
    if (zoomFactor < 0.1 || zoomFactor > 5.0) {
        LOG_WARNING(
            "ToolBar::updateZoomLevel() - Zoom factor {} out of range [0.1, "
            "5.0]",
            zoomFactor);
        zoomFactor = std::clamp(zoomFactor, 0.1, 5.0);
    }

    int percentage = static_cast<int>(zoomFactor * 100);

    m_zoomSlider->blockSignals(true);
    m_zoomSlider->setValue(percentage);
    m_zoomSlider->blockSignals(false);

    m_zoomValueLabel->setText(QString("%1%").arg(percentage));
    m_zoomPresets->setCurrentText(QString("%1%").arg(percentage));
}

void ToolBar::updateDocumentInfo(const QString& fileName, qint64 fileSize,
                                 const QDateTime& lastModified) {
    // Defensive null pointer checks
    if (!m_documentInfoLabel || !m_fileSizeLabel || !m_lastModifiedLabel) {
        LOG_WARNING(
            "ToolBar::updateDocumentInfo() - Document info labels not "
            "initialized");
        return;
    }

    m_documentInfoLabel->setText(fileName);

    // Format file size
    QString sizeText;
    if (fileSize < 1024) {
        sizeText = QString("%1 B").arg(fileSize);
    } else if (fileSize < 1024 * 1024) {
        sizeText = QString("%1 KB").arg(fileSize / 1024.0, 0, 'f', 2);
    } else {
        sizeText =
            QString("%1 MB").arg(fileSize / (1024.0 * 1024.0), 0, 'f', 2);
    }
    m_fileSizeLabel->setText(tr("Size: %1").arg(sizeText));

    // Format last modified date
    m_lastModifiedLabel->setText(
        tr("Modified: %1").arg(lastModified.toString("yyyy-MM-dd hh:mm")));
}

void ToolBar::setActionsEnabled(bool enabled) {
    // File actions always enabled except save
    if (m_openAction) {
        m_openAction->setEnabled(true);
    }
    if (m_openFolderAction) {
        m_openFolderAction->setEnabled(true);
    }
    if (m_saveAction) {
        m_saveAction->setEnabled(enabled);
    }
    if (m_saveAsAction) {
        m_saveAsAction->setEnabled(enabled);
    }
    if (m_printAction) {
        m_printAction->setEnabled(enabled);
    }
    if (m_emailAction) {
        m_emailAction->setEnabled(enabled);
    }

    // Navigation actions
    if (m_navigationSection) {
        m_navigationSection->setEnabled(enabled);
    }

    // Zoom actions
    if (m_zoomSection) {
        m_zoomSection->setEnabled(enabled);
    }

    // View actions
    if (m_viewSection) {
        m_viewSection->setEnabled(enabled);
    }

    // Tool actions
    if (m_toolsSection) {
        m_toolsSection->setEnabled(enabled);
    }
}

void ToolBar::setCompactMode(bool compact) {
    m_compactMode = compact;

    if (compact) {
        // Collapse all sections in compact mode
        if (m_fileSection) {
            m_fileSection->setExpanded(false);
        }
        if (m_navigationSection) {
            m_navigationSection->setExpanded(false);
        }
        if (m_zoomSection) {
            m_zoomSection->setExpanded(false);
        }
        if (m_viewSection) {
            m_viewSection->setExpanded(false);
        }
        if (m_toolsSection) {
            m_toolsSection->setExpanded(false);
        }
    } else {
        // Expand important sections
        if (m_navigationSection) {
            m_navigationSection->setExpanded(true);
        }
        if (m_zoomSection) {
            m_zoomSection->setExpanded(true);
        }
    }
}

void ToolBar::onPageSpinBoxChanged(int pageNumber) {
    emit pageJumpRequested(pageNumber - 1);
    if (m_pageSlider) {
        m_pageSlider->setValue(pageNumber);
    }
}

void ToolBar::onViewModeChanged() {
    if (!m_viewModeCombo) {
        LOG_WARNING(
            "ToolBar::onViewModeChanged() - View mode combo not initialized");
        return;
    }

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
    if (m_zoomValueLabel) {
        m_zoomValueLabel->setText(QString("%1%").arg(value));
    }
    if (m_zoomPresets) {
        m_zoomPresets->setCurrentText(QString("%1%").arg(value));
    }
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
            bool anyExpanded =
                (m_fileSection && m_fileSection->isExpanded()) ||
                (m_navigationSection && m_navigationSection->isExpanded()) ||
                (m_zoomSection && m_zoomSection->isExpanded()) ||
                (m_viewSection && m_viewSection->isExpanded()) ||
                (m_toolsSection && m_toolsSection->isExpanded());

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

    if (m_compactMode && m_hoverAnimation) {
        // Expand on hover in compact mode
        m_hoverAnimation->setStartValue(height());
        m_hoverAnimation->setEndValue(sizeHint().height());
        m_hoverAnimation->start();
    }
}

void ToolBar::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    m_isHovered = false;

    if (m_compactMode && m_hoverAnimation) {
        // Collapse on leave in compact mode
        m_hoverAnimation->setStartValue(height());
        m_hoverAnimation->setEndValue(60);  // Minimum height in compact mode
        m_hoverAnimation->start();
    }
}

void ToolBar::retranslateUi() {
    // Update section titles
    if (m_fileSection) {
        m_fileSection->setWindowTitle(tr("File"));
    }
    if (m_navigationSection) {
        m_navigationSection->setWindowTitle(tr("Navigation"));
    }
    if (m_zoomSection) {
        m_zoomSection->setWindowTitle(tr("Zoom"));
    }
    if (m_viewSection) {
        m_viewSection->setWindowTitle(tr("View"));
    }
    if (m_toolsSection) {
        m_toolsSection->setWindowTitle(tr("Tools"));
    }

    // Update all tooltips and text with new translations
    if (m_openAction) {
        m_openAction->setToolTip(tr("Open PDF File (Ctrl+O)"));
    }
    if (m_openFolderAction) {
        m_openFolderAction->setToolTip(tr("Open Folder (Ctrl+Shift+O)"));
    }
    if (m_saveAction) {
        m_saveAction->setToolTip(tr("Save File (Ctrl+S)"));
    }

    if (m_firstPageAction) {
        m_firstPageAction->setToolTip(tr("First Page (Ctrl+Home)"));
    }
    if (m_prevPageAction) {
        m_prevPageAction->setToolTip(tr("Previous Page (Page Up)"));
    }
    if (m_nextPageAction) {
        m_nextPageAction->setToolTip(tr("Next Page (Page Down)"));
    }
    if (m_lastPageAction) {
        m_lastPageAction->setToolTip(tr("Last Page (Ctrl+End)"));
    }
    if (m_pageSpinBox) {
        m_pageSpinBox->setToolTip(tr("Current Page"));
    }

    if (m_zoomOutAction) {
        m_zoomOutAction->setToolTip(tr("Zoom Out (Ctrl+-)"));
    }
    if (m_zoomInAction) {
        m_zoomInAction->setToolTip(tr("Zoom In (Ctrl++)"));
    }
    if (m_fitWidthAction) {
        m_fitWidthAction->setToolTip(tr("Fit to Width (Ctrl+1)"));
    }
    if (m_fitPageAction) {
        m_fitPageAction->setToolTip(tr("Fit to Page (Ctrl+0)"));
    }
    if (m_fitHeightAction) {
        m_fitHeightAction->setToolTip(tr("Fit to Height (Ctrl+2)"));
    }

    if (m_toggleSidebarAction) {
        m_toggleSidebarAction->setToolTip(tr("Toggle Sidebar (F9)"));
    }

    // Update combo box items
    if (m_viewModeCombo) {
        int currentViewMode = m_viewModeCombo->currentIndex();
        m_viewModeCombo->blockSignals(true);
        m_viewModeCombo->clear();
        m_viewModeCombo->addItems({tr("Single Page"), tr("Continuous"),
                                   tr("Two Pages"), tr("Book View")});
        m_viewModeCombo->setCurrentIndex(currentViewMode);
        m_viewModeCombo->setToolTip(tr("Select View Mode"));
        m_viewModeCombo->blockSignals(false);
    }

    if (m_rotateLeftAction) {
        m_rotateLeftAction->setToolTip(tr("Rotate Left 90° (Ctrl+L)"));
    }
    if (m_rotateRightAction) {
        m_rotateRightAction->setToolTip(tr("Rotate Right 90° (Ctrl+R)"));
    }

    if (m_themeToggleAction) {
        m_themeToggleAction->setToolTip(tr("Toggle Theme (Ctrl+T)"));
    }
}

void ToolBar::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QToolBar::changeEvent(event);
}
