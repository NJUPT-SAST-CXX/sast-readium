#include "StatusBar.h"
#include <QEasingCurve>
#include <QEvent>
#include <QFileInfo>
#include <QFontMetrics>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QStyle>
#include <algorithm>
#include "../../logging/LoggingMacros.h"
#include "../../managers/StyleManager.h"
#include "UIErrorHandler.h"

// ExpandableInfoPanel Implementation
ExpandableInfoPanel::ExpandableInfoPanel(const QString& title, QWidget* parent)
    : QWidget(parent), m_expanded(false) {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Create toggle button
    m_toggleButton = new QPushButton(title, this);
    m_toggleButton->setCheckable(true);
    m_toggleButton->setCursor(Qt::PointingHandCursor);

    // Create content frame
    m_contentFrame = new QFrame(this);
    m_contentFrame->setFrameStyle(QFrame::NoFrame);
    m_contentFrame->setMaximumHeight(0);

    // Setup animation with StyleManager duration
    // Don't create animation in offscreen mode to avoid crashes
    if (QGuiApplication::platformName() == "offscreen") {
        m_animation = nullptr;
    } else {
        m_animation =
            new QPropertyAnimation(m_contentFrame, "maximumHeight", this);
        m_animation->setDuration(STYLE.animationNormal());
        m_animation->setEasingCurve(QEasingCurve::InOutQuad);
    }

    mainLayout->addWidget(m_toggleButton);
    mainLayout->addWidget(m_contentFrame);

    connect(m_toggleButton, &QPushButton::toggled, this,
            [this](bool checked) { setExpanded(checked); });

    connect(&STYLE, &StyleManager::themeChanged, this,
            [this](Theme) { applyTheme(); });

    applyTheme();
    updateToggleButton();
}

ExpandableInfoPanel::~ExpandableInfoPanel() {
    // Stop animation before destruction to avoid crashes in offscreen mode
    if (m_animation) {
        m_animation->stop();
        delete m_animation;
        m_animation = nullptr;
    }
}

void ExpandableInfoPanel::setContentWidget(QWidget* widget) {
    if (m_contentWidget) {
        delete m_contentWidget;
    }

    m_contentWidget = widget;
    QVBoxLayout* contentLayout = new QVBoxLayout(m_contentFrame);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->addWidget(widget);
}

void ExpandableInfoPanel::setExpanded(bool expanded, bool animated) {
    if (m_expanded == expanded) {
        return;
    }

    m_expanded = expanded;
    m_toggleButton->setChecked(expanded);
    updateToggleButton();

    // If animation is disabled (nullptr in offscreen mode), always use instant
    // transition
    if (animated && m_animation) {
        m_animation->stop();
        m_animation->setStartValue(m_contentFrame->height());
        m_animation->setEndValue(expanded ? m_contentFrame->sizeHint().height()
                                          : 0);
        m_animation->start();
    } else {
        m_contentFrame->setMaximumHeight(
            expanded ? m_contentFrame->sizeHint().height() : 0);
    }

    emit expandedChanged(expanded);
}

void ExpandableInfoPanel::updateToggleButton() {
    // Use ASCII-safe arrows to avoid encoding issues on some platforms
    const QString arrow = m_expanded ? QString(">") : QString("v");

    // Preserve the original title if already prefixed with an arrow
    QString current = m_toggleButton->text();
    if (current.startsWith("v ") || current.startsWith("> ")) {
        current = current.mid(2);
    }
    m_toggleButton->setText(arrow + " " + current);
}

void ExpandableInfoPanel::applyTheme() {
    // Skip stylesheet application in offscreen mode to avoid crashes
    if (QGuiApplication::platformName() == "offscreen") {
        return;
    }

    const QString buttonStyle = STYLE.createToggleButtonStyle();
    m_toggleButton->setStyleSheet(buttonStyle);

    const QString frameStyle =
        QStringLiteral("QFrame { %1 }").arg(STYLE.createCardStyle());
    m_contentFrame->setStyleSheet(frameStyle);

    if (auto* layout = m_contentFrame->layout()) {
        layout->setContentsMargins(STYLE.spacingSM(), STYLE.spacingSM(),
                                   STYLE.spacingSM(), STYLE.spacingSM());
        layout->setSpacing(STYLE.spacingSM());
    }
}

// StatusBar Implementation
StatusBar::StatusBar(QWidget* parent, bool minimalMode)
    : QStatusBar(parent),
      m_currentTotalPages(0),
      m_currentPageNumber(0),
      m_compactMode(false) {
    // Minimal mode: Skip all UI creation to avoid Qt platform issues in tests
    if (minimalMode) {
        // Initialize all pointers to nullptr
        m_mainSection = nullptr;
        m_fileNameLabel = nullptr;
        m_pageLabel = nullptr;
        m_pageInputEdit = nullptr;
        m_zoomLabel = nullptr;
        m_zoomInputEdit = nullptr;
        m_clockLabel = nullptr;
        m_clockTimer = nullptr;
        m_messageLabel = nullptr;
        m_messageTimer = nullptr;
        m_messageAnimation = nullptr;
        m_loadingProgressBar = nullptr;
        m_loadingMessageLabel = nullptr;
        m_progressAnimation = nullptr;
        m_searchFrame = nullptr;
        m_searchInput = nullptr;
        m_searchResultsLabel = nullptr;
        m_documentInfoPanel = nullptr;
        m_statisticsPanel = nullptr;
        m_securityPanel = nullptr;
        m_quickActionsPanel = nullptr;
        separatorLabel1 = nullptr;
        separatorLabel2 = nullptr;
        separatorLabel3 = nullptr;
        m_titleLabel = nullptr;
        m_authorLabel = nullptr;
        m_subjectLabel = nullptr;
        m_keywordsLabel = nullptr;
        m_createdLabel = nullptr;
        m_modifiedLabel = nullptr;
        m_fileSizeLabel = nullptr;
        m_wordCountLabel = nullptr;
        m_charCountLabel = nullptr;
        m_pageCountLabel = nullptr;
        m_avgWordsPerPageLabel = nullptr;
        m_readingTimeLabel = nullptr;
        m_encryptionLabel = nullptr;
        m_copyPermissionLabel = nullptr;
        m_printPermissionLabel = nullptr;
        m_modifyPermissionLabel = nullptr;
        m_bookmarkBtn = nullptr;
        m_annotateBtn = nullptr;
        m_shareBtn = nullptr;
        m_exportBtn = nullptr;

        // Set a simple message using QStatusBar's built-in functionality
        QStatusBar::showMessage("StatusBar (Minimal Mode)");
        return;
    }

    // Normal mode: Full UI creation
    // Setup main horizontal layout with optimized spacing
    QWidget* containerWidget = new QWidget(this);
    containerWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QHBoxLayout* mainLayout = new QHBoxLayout(containerWidget);
    mainLayout->setContentsMargins(STYLE.spacingXS(), STYLE.spacingXS(),
                                   STYLE.spacingXS(), STYLE.spacingXS());
    mainLayout->setSpacing(STYLE.spacingSM());

    // Setup main section (always visible)
    setupMainSection();
    mainLayout->addWidget(m_mainSection);

    // Setup expandable panels (emoji characters replaced with ASCII for Windows
    // compatibility)
    setupDocumentInfoPanel();
    setupStatisticsPanel();
    setupSecurityPanel();
    setupQuickActionsPanel();

    mainLayout->addWidget(m_documentInfoPanel);
    mainLayout->addWidget(m_statisticsPanel);
    mainLayout->addWidget(m_securityPanel);
    mainLayout->addWidget(m_quickActionsPanel);
    mainLayout->addStretch();

    addWidget(containerWidget);

    // Setup message label (overlay)
    m_messageLabel = new QLabel(this);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setMinimumWidth(280);
    m_messageLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_messageLabel->hide();

    // Setup message timer
    m_messageTimer = new QTimer(this);
    m_messageTimer->setSingleShot(true);
    connect(m_messageTimer, &QTimer::timeout, this,
            &StatusBar::onMessageTimerTimeout);

    // Setup message priority timer
    m_messagePriorityTimer = new QTimer(this);
    m_messagePriorityTimer->setSingleShot(true);
    connect(m_messagePriorityTimer, &QTimer::timeout, this, [this]() {
        m_currentMessagePriority = MessagePriority::Low;
        LOG_DEBUG("StatusBar: Message priority reset to Low");
    });

    // Setup message animation
    // Don't create animation in offscreen mode to avoid crashes
    if (QGuiApplication::platformName() == "offscreen") {
        m_messageAnimation = nullptr;
    } else {
        m_messageAnimation =
            new QPropertyAnimation(m_messageLabel, "windowOpacity", this);
        m_messageAnimation->setDuration(300);
        connect(m_messageAnimation, &QPropertyAnimation::finished, this,
                [this]() { m_messageLabel->hide(); });
    }

    // Setup clock timer
    m_clockTimer = new QTimer(this);
    connect(m_clockTimer, &QTimer::timeout, this, &StatusBar::updateClock);
    m_clockTimer->start(1000);

    // Apply enhanced styling
    applyEnhancedStyle();
    connect(&STYLE, &StyleManager::themeChanged, this,
            [this](Theme) { applyEnhancedStyle(); });

    connect(this, &QStatusBar::messageChanged, this,
            [this](const QString& text) {
                if (!m_messageLabel) {
                    return;
                }
                if (text.isEmpty()) {
                    if (m_messageTimer) {
                        m_messageTimer->stop();
                    }
                    if (m_messageAnimation) {
                        m_messageAnimation->stop();
                    }
                    m_messageLabel->hide();
                    m_messageLabel->clear();
                }
            });

    // Set initial state
    clearDocumentInfo();
}

StatusBar::~StatusBar() {
    // Stop and clean up timers
    if (m_clockTimer) {
        m_clockTimer->stop();
        // Timer will be deleted by Qt parent-child ownership
    }

    if (m_messageTimer) {
        m_messageTimer->stop();
        // Timer will be deleted by Qt parent-child ownership
    }

    // Stop animations
    if (m_messageAnimation) {
        m_messageAnimation->stop();
        // Animation will be deleted by Qt parent-child ownership
    }

    if (m_progressAnimation) {
        m_progressAnimation->stop();
        // Animation will be deleted by Qt parent-child ownership
    }

    // All widgets are deleted automatically by Qt parent-child ownership
    // No manual deletion needed for widgets created with 'this' as parent

    LOG_DEBUG("StatusBar destroyed successfully");
}

void StatusBar::setupMainSection() {
    m_mainSection = new QFrame(this);
    m_mainSection->setFrameStyle(QFrame::NoFrame);

    QHBoxLayout* layout = new QHBoxLayout(m_mainSection);
    layout->setContentsMargins(STYLE.spacingSM(), STYLE.spacingXS(),
                               STYLE.spacingSM(), STYLE.spacingXS());
    layout->setSpacing(STYLE.spacingMD());

    // File name
    m_fileNameLabel = new QLabel(tr("No Document"), this);
    m_fileNameLabel->setMinimumWidth(150);
    m_fileNameLabel->setMaximumWidth(300);
    layout->addWidget(m_fileNameLabel);

    // Separator
    separatorLabel1 = new QLabel("|", this);
    separatorLabel1->setAlignment(Qt::AlignCenter);
    layout->addWidget(separatorLabel1);

    // Page navigation
    m_pageLabel = new QLabel(tr("Page:"), this);
    layout->addWidget(m_pageLabel);

    m_pageInputEdit = new QLineEdit(this);
    m_pageInputEdit->setMaximumWidth(60);
    m_pageInputEdit->setAlignment(Qt::AlignCenter);
    m_pageInputEdit->setPlaceholderText("0/0");
    m_pageInputEdit->setAccessibleName(tr("Page number input"));
    layout->addWidget(m_pageInputEdit);

    // Separator
    separatorLabel2 = new QLabel("|", this);
    separatorLabel2->setAlignment(Qt::AlignCenter);
    layout->addWidget(separatorLabel2);

    // Zoom
    m_zoomLabel = new QLabel(tr("Zoom:"), this);
    layout->addWidget(m_zoomLabel);

    m_zoomInputEdit = new QLineEdit(this);
    m_zoomInputEdit->setMaximumWidth(60);
    m_zoomInputEdit->setAlignment(Qt::AlignCenter);
    m_zoomInputEdit->setText("100%");
    m_zoomInputEdit->setAccessibleName(tr("Zoom percentage input"));
    layout->addWidget(m_zoomInputEdit);

    // Separator
    separatorLabel3 = new QLabel("|", this);
    separatorLabel3->setAlignment(Qt::AlignCenter);
    layout->addWidget(separatorLabel3);

    // Search
    m_searchFrame = new QFrame(this);
    QHBoxLayout* searchLayout = new QHBoxLayout(m_searchFrame);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(STYLE.spacingXS());

    QLabel* searchIcon = new QLabel(tr("Search:"), this);
    searchLayout->addWidget(searchIcon);

    m_searchInput = new QLineEdit(this);
    m_searchInput->setPlaceholderText(tr("Search..."));
    m_searchInput->setMinimumWidth(150);
    m_searchInput->setAccessibleName(tr("Document search input"));
    searchLayout->addWidget(m_searchInput);

    m_searchResultsLabel = new QLabel("", this);
    searchLayout->addWidget(m_searchResultsLabel);

    layout->addWidget(m_searchFrame);

    // Loading progress
    m_loadingProgressBar = new QProgressBar(this);
    m_loadingProgressBar->setMinimumWidth(150);
    m_loadingProgressBar->setMaximumWidth(200);
    m_loadingProgressBar->setMaximumHeight(16);
    m_loadingProgressBar->setTextVisible(true);
    m_loadingProgressBar->hide();
    m_loadingProgressBar->setAccessibleName(tr("Document loading progress"));

    m_loadingMessageLabel = new QLabel("", this);
    m_loadingMessageLabel->hide();

    layout->addWidget(m_loadingMessageLabel);
    layout->addWidget(m_loadingProgressBar);

    layout->addStretch();

    // Clock
    m_clockLabel = new QLabel(this);
    layout->addWidget(m_clockLabel);
    updateClock();

    // Connect signals
    connect(m_pageInputEdit, &QLineEdit::returnPressed, this,
            &StatusBar::onPageInputReturnPressed);
    connect(m_pageInputEdit, &QLineEdit::editingFinished, this,
            &StatusBar::onPageInputEditingFinished);
    connect(m_pageInputEdit, &QLineEdit::textChanged, this,
            &StatusBar::onPageInputTextChanged);
    connect(m_zoomInputEdit, &QLineEdit::returnPressed, this,
            &StatusBar::onZoomInputReturnPressed);
    connect(m_searchInput, &QLineEdit::returnPressed, this,
            &StatusBar::onSearchInputReturnPressed);

    // Enhanced keyboard navigation support
    m_pageInputEdit->installEventFilter(this);
    m_zoomInputEdit->installEventFilter(this);
    m_searchInput->installEventFilter(this);

    // Setup input validator
    QIntValidator* validator = new QIntValidator(1, 9999, this);
    m_pageInputEdit->setValidator(validator);
    m_pageInputEdit->setEnabled(false);
    m_pageInputEdit->setToolTip("");
}

void StatusBar::setupDocumentInfoPanel() {
    // Use ASCII character instead of emoji for Windows compatibility
    m_documentInfoPanel = new ExpandableInfoPanel(tr("Document Info"), this);

    QWidget* content = new QWidget();
    QGridLayout* layout = new QGridLayout(content);
    layout->setSpacing(STYLE.spacingXS());
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setColumnStretch(1, 1);

    // Create labels
    m_titleLabel = new QLabel(tr("Title: -"), this);
    m_authorLabel = new QLabel(tr("Author: -"), this);
    m_subjectLabel = new QLabel(tr("Subject: -"), this);
    m_keywordsLabel = new QLabel(tr("Keywords: -"), this);
    m_createdLabel = new QLabel(tr("Created: -"), this);
    m_modifiedLabel = new QLabel(tr("Modified: -"), this);
    m_fileSizeLabel = new QLabel(tr("Size: -"), this);

    // Style labels
    // Add to layout
    layout->addWidget(m_titleLabel, 0, 0, 1, 2);
    layout->addWidget(m_authorLabel, 1, 0, 1, 2);
    layout->addWidget(m_subjectLabel, 2, 0, 1, 2);
    layout->addWidget(m_keywordsLabel, 3, 0, 1, 2);
    layout->addWidget(m_createdLabel, 4, 0);
    layout->addWidget(m_modifiedLabel, 4, 1);
    layout->addWidget(m_fileSizeLabel, 5, 0);

    m_documentInfoPanel->setContentWidget(content);
}

void StatusBar::setupStatisticsPanel() {
    // Use ASCII character instead of emoji for Windows compatibility
    m_statisticsPanel = new ExpandableInfoPanel(tr("Statistics"), this);

    QWidget* content = new QWidget();
    QGridLayout* layout = new QGridLayout(content);
    layout->setSpacing(STYLE.spacingXS());
    layout->setContentsMargins(0, 0, 0, 0);

    // Create labels
    m_wordCountLabel = new QLabel(tr("Words: -"), this);
    m_charCountLabel = new QLabel(tr("Characters: -"), this);
    m_pageCountLabel = new QLabel(tr("Pages: -"), this);
    m_avgWordsPerPageLabel = new QLabel(tr("Avg Words/Page: -"), this);
    m_readingTimeLabel = new QLabel(tr("Est. Reading Time: -"), this);

    // Style labels
    // Add to layout
    layout->addWidget(m_pageCountLabel, 0, 0);
    layout->addWidget(m_wordCountLabel, 0, 1);
    layout->addWidget(m_charCountLabel, 1, 0);
    layout->addWidget(m_avgWordsPerPageLabel, 1, 1);
    layout->addWidget(m_readingTimeLabel, 2, 0, 1, 2);

    // Add a mini chart or graph placeholder
    QLabel* chartPlaceholder = new QLabel(this);
    chartPlaceholder->setMinimumHeight(60);
    chartPlaceholder->setFrameStyle(QFrame::Box);
    chartPlaceholder->setAlignment(Qt::AlignCenter);
    chartPlaceholder->setText(tr("Page Distribution"));
    chartPlaceholder->setObjectName("statisticsChartPlaceholder");
    layout->addWidget(chartPlaceholder, 3, 0, 1, 2);

    m_statisticsPanel->setContentWidget(content);
}

void StatusBar::setupSecurityPanel() {
    // Use ASCII character instead of emoji for Windows compatibility
    m_securityPanel = new ExpandableInfoPanel(tr("Security"), this);

    QWidget* content = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setSpacing(STYLE.spacingXS());
    layout->setContentsMargins(0, 0, 0, 0);

    // Create labels (using ASCII instead of emojis for Windows compatibility)
    m_encryptionLabel = new QLabel(tr("Encryption: No"), this);
    m_copyPermissionLabel = new QLabel(tr("Copy: Allowed"), this);
    m_printPermissionLabel = new QLabel(tr("Print: Allowed"), this);
    m_modifyPermissionLabel = new QLabel(tr("Modify: Not Allowed"), this);

    // Style labels
    // Add to layout
    layout->addWidget(m_encryptionLabel);
    layout->addWidget(m_copyPermissionLabel);
    layout->addWidget(m_printPermissionLabel);
    layout->addWidget(m_modifyPermissionLabel);

    m_securityPanel->setContentWidget(content);
}

void StatusBar::setupQuickActionsPanel() {
    // Use ASCII character instead of emoji for Windows compatibility
    m_quickActionsPanel = new ExpandableInfoPanel(tr("Quick Actions"), this);

    QWidget* content = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(content);
    layout->setSpacing(STYLE.spacingSM());
    layout->setContentsMargins(0, 0, 0, 0);

    // Create action buttons (using ASCII instead of emojis for Windows
    // compatibility)
    m_bookmarkBtn = new QPushButton(tr("Bookmark"), this);
    m_annotateBtn = new QPushButton(tr("Annotate"), this);
    m_shareBtn = new QPushButton(tr("Share"), this);
    m_exportBtn = new QPushButton(tr("Export"), this);

    // Style buttons
    // Add to layout
    layout->addWidget(m_bookmarkBtn);
    layout->addWidget(m_annotateBtn);
    layout->addWidget(m_shareBtn);
    layout->addWidget(m_exportBtn);
    layout->addStretch();

    m_quickActionsPanel->setContentWidget(content);
}

void StatusBar::applyEnhancedStyle() {
    // Skip stylesheet application in offscreen mode to avoid crashes
    if (QGuiApplication::platformName() == "offscreen") {
        return;
    }

    setStyleSheet(STYLE.getStatusBarStyleSheet());
    setFont(STYLE.defaultFont());

    const QString accent = STYLE.accentColor().name();
    const QString secondary = STYLE.textSecondaryColor().name();
    const QString mutedBorder = STYLE.mutedBorderColor().name();

    if (m_mainSection) {
        const QString frameStyle =
            QStringLiteral(
                "QFrame { background-color: %1; border: 1px solid %2; "
                "border-radius: %3px; }")
                .arg(STYLE.surfaceColor().name())
                .arg(mutedBorder)
                .arg(STYLE.radiusLG());
        m_mainSection->setStyleSheet(frameStyle);
        if (auto* layout =
                qobject_cast<QHBoxLayout*>(m_mainSection->layout())) {
            layout->setContentsMargins(STYLE.spacingSM(), STYLE.spacingXS(),
                                       STYLE.spacingSM(), STYLE.spacingXS());
            layout->setSpacing(STYLE.spacingMD());
        }
    }

    if (m_fileNameLabel) {
        m_fileNameLabel->setStyleSheet(
            QStringLiteral("QLabel { font-weight: 600; color: %1; }")
                .arg(accent));
    }

    const QString separatorStyle =
        QStringLiteral("QLabel { color: %1; padding: 0 %2px; }")
            .arg(secondary)
            .arg(STYLE.spacingXS());
    if (separatorLabel1) {
        separatorLabel1->setStyleSheet(separatorStyle);
    }
    if (separatorLabel2) {
        separatorLabel2->setStyleSheet(separatorStyle);
    }
    if (separatorLabel3) {
        separatorLabel3->setStyleSheet(separatorStyle);
    }

    if (m_clockLabel) {
        m_clockLabel->setStyleSheet(
            QStringLiteral(
                "QLabel { color: %1; font-family: 'JetBrains Mono',"
                " 'Consolas', 'Monaco', monospace; font-size: 12px; }")
                .arg(secondary));
    }

    applyFieldStyles();
    applyPanelTypography();
    applyQuickActionStyles();

    if (m_loadingMessageLabel) {
        m_loadingMessageLabel->setStyleSheet(
            QStringLiteral("QLabel { color: %1; }").arg(secondary));
    }

    if (m_loadingProgressBar) {
        const QString progressStyle =
            QStringLiteral(
                "QProgressBar { background-color: %1; border: 1px solid %2;"
                " border-radius: %3px; text-align: center; font-size: 11px; }"
                " QProgressBar::chunk { background-color: %4; border-radius: "
                "%3px;"
                " }")
                .arg(STYLE.surfaceAltColor().name())
                .arg(mutedBorder)
                .arg(STYLE.radiusLG())
                .arg(STYLE.successColor().name());
        m_loadingProgressBar->setStyleSheet(progressStyle);
    }

    if (m_searchResultsLabel) {
        m_searchResultsLabel->setStyleSheet(
            QStringLiteral("QLabel { color: %1; font-size: 11px; }")
                .arg(secondary));
    }

    if (m_encryptionLabel) {
        setDocumentSecurity(m_lastEncrypted, m_lastCopyAllowed,
                            m_lastPrintAllowed);
    }

    if (m_messageLabel) {
        m_messageLabel->setAttribute(Qt::WA_StyledBackground, true);
        m_messageLabel->setAlignment(Qt::AlignCenter);
        m_messageLabel->setFont(STYLE.headingFont());
        updateMessageAppearance(STYLE.overlayColor(), STYLE.textColor());
    }
}

void StatusBar::applyFieldStyles() {
    const QString inputStyle = STYLE.createInputStyle();

    auto styleLineEdit = [&](QLineEdit* edit) {
        if (!edit) {
            return;
        }
        edit->setStyleSheet(inputStyle);
        edit->setFont(STYLE.defaultFont());
        edit->setClearButtonEnabled(true);
    };

    styleLineEdit(m_pageInputEdit);
    styleLineEdit(m_zoomInputEdit);
    styleLineEdit(m_searchInput);

    if (m_searchFrame) {
        if (auto* layout =
                qobject_cast<QHBoxLayout*>(m_searchFrame->layout())) {
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(STYLE.spacingXS());
        }
    }

    if (m_pageInputEdit) {
        setLineEditInvalid(m_pageInputEdit,
                           m_pageInputEdit->property("invalid").toBool());
    }
}

void StatusBar::applyPanelTypography() {
    const QString labelStyle =
        QStringLiteral("QLabel { color: %1; font-size: 11px; }")
            .arg(STYLE.textSecondaryColor().name());

    auto applyLabelStyle = [&](const QList<QLabel*>& labels) {
        for (QLabel* label : labels) {
            if (!label) {
                continue;
            }
            label->setStyleSheet(labelStyle);
            label->setFont(STYLE.captionFont());
        }
    };

    applyLabelStyle({m_titleLabel, m_authorLabel, m_subjectLabel,
                     m_keywordsLabel, m_createdLabel, m_modifiedLabel,
                     m_fileSizeLabel});

    applyLabelStyle({m_wordCountLabel, m_charCountLabel, m_pageCountLabel,
                     m_avgWordsPerPageLabel, m_readingTimeLabel});

    applyLabelStyle({m_pageLabel, m_zoomLabel});

    if (QWidget* chart = m_statisticsPanel
                             ? m_statisticsPanel->findChild<QWidget*>(
                                   "statisticsChartPlaceholder")
                             : nullptr) {
        const QString chartStyle =
            QStringLiteral(
                "QLabel#statisticsChartPlaceholder { background-color: %1;"
                " border: 1px dashed %2; border-radius: %3px; color: %4; }")
                .arg(STYLE.surfaceAltColor().name())
                .arg(STYLE.mutedBorderColor().name())
                .arg(STYLE.radiusMD())
                .arg(STYLE.textSecondaryColor().name());
        chart->setStyleSheet(chartStyle);
        chart->setFont(STYLE.captionFont());
    }
}

void StatusBar::applyQuickActionStyles() {
    const QString buttonStyle = STYLE.createButtonStyle();
    auto applyButton = [&](QPushButton* button) {
        if (!button) {
            return;
        }
        button->setStyleSheet(buttonStyle);
        button->setCursor(Qt::PointingHandCursor);
        button->setMinimumHeight(STYLE.buttonHeight());
        button->setMinimumWidth(STYLE.buttonMinWidth());
        button->setFont(STYLE.buttonFont());
    };

    applyButton(m_bookmarkBtn);
    applyButton(m_annotateBtn);
    applyButton(m_shareBtn);
    applyButton(m_exportBtn);
}

void StatusBar::updateMessageAppearance(const QColor& background,
                                        const QColor& text) {
    if (!m_messageLabel) {
        return;
    }
    m_messageLabel->setStyleSheet(
        STYLE.createMessageLabelStyle(background, text));
}

void StatusBar::setLineEditInvalid(QLineEdit* edit, bool invalid) {
    if (!edit) {
        return;
    }
    edit->setProperty("invalid", invalid);
    edit->style()->unpolish(edit);
    edit->style()->polish(edit);
    edit->update();
}

void StatusBar::displayTransientMessage(const QString& text, int timeout,
                                        const QColor& background,
                                        const QColor& foreground) {
    if (!m_messageLabel) {
        return;
    }

    m_messageTimer->stop();
    if (m_messageAnimation) {
        m_messageAnimation->stop();
    }

    updateMessageAppearance(background, foreground);
    m_messageLabel->setText(text);
    m_messageLabel->adjustSize();

    const int x = (width() - m_messageLabel->width()) / 2;
    const int y = height() - m_messageLabel->height() - STYLE.spacingMD();
    m_messageLabel->move(std::max(0, x), std::max(0, y));
    m_messageLabel->raise();
    m_messageLabel->setWindowOpacity(1.0);
    m_messageLabel->show();

    if (timeout > 0) {
        m_messageTimer->start(timeout);
    }
}

void StatusBar::setDocumentInfo(const QString& fileName, int currentPage,
                                int totalPages, double zoomLevel) {
    if (!m_fileNameLabel) {
        return;  // Skip if in minimal mode
    }
    setFileName(fileName);
    setPageInfo(currentPage, totalPages);
    setZoomLevel(zoomLevel);
}

void StatusBar::setDocumentInfo(const QString& fileName, int currentPage,
                                int totalPages, double zoomLevel,
                                qint64 fileSize) {
    if (!m_fileNameLabel) {
        return;  // Skip if in minimal mode
    }
    setFileName(fileName, fileSize);
    setPageInfo(currentPage, totalPages);
    setZoomLevel(zoomLevel);
}

void StatusBar::setDocumentMetadata(const QString& title, const QString& author,
                                    const QString& subject,
                                    const QString& keywords,
                                    const QDateTime& created,
                                    const QDateTime& modified) {
    if (!m_titleLabel) {
        return;  // Skip if in minimal mode
    }

    // Enhanced metadata display with proper validation and formatting
    QString displayTitle = title.trimmed();
    if (displayTitle.length() > 50) {
        displayTitle = displayTitle.left(47) + "...";
    }
    m_titleLabel->setText(
        tr("Title: %1").arg(displayTitle.isEmpty() ? "-" : displayTitle));
    m_titleLabel->setToolTip(title.isEmpty() ? tr("No title available")
                                             : title);

    QString displayAuthor = author.trimmed();
    if (displayAuthor.length() > 30) {
        displayAuthor = displayAuthor.left(27) + "...";
    }
    m_authorLabel->setText(
        tr("Author: %1").arg(displayAuthor.isEmpty() ? "-" : displayAuthor));
    m_authorLabel->setToolTip(author.isEmpty() ? tr("No author information")
                                               : author);

    QString displaySubject = subject.trimmed();
    if (displaySubject.length() > 40) {
        displaySubject = displaySubject.left(37) + "...";
    }
    m_subjectLabel->setText(
        tr("Subject: %1").arg(displaySubject.isEmpty() ? "-" : displaySubject));
    m_subjectLabel->setToolTip(subject.isEmpty() ? tr("No subject information")
                                                 : subject);

    QString displayKeywords = keywords.trimmed();
    if (displayKeywords.length() > 40) {
        displayKeywords = displayKeywords.left(37) + "...";
    }
    m_keywordsLabel->setText(
        tr("Keywords: %1")
            .arg(displayKeywords.isEmpty() ? "-" : displayKeywords));
    m_keywordsLabel->setToolTip(keywords.isEmpty() ? tr("No keywords available")
                                                   : keywords);

    QString createdText = formatDateTime(created);
    m_createdLabel->setText(tr("Created: %1").arg(createdText));
    m_createdLabel->setToolTip(created.isValid() ? created.toString()
                                                 : tr("Creation date unknown"));

    QString modifiedText = formatDateTime(modified);
    m_modifiedLabel->setText(tr("Modified: %1").arg(modifiedText));
    m_modifiedLabel->setToolTip(modified.isValid()
                                    ? modified.toString()
                                    : tr("Modification date unknown"));

    LOG_DEBUG(
        "StatusBar::setDocumentMetadata() - Metadata updated: title='{}', "
        "author='{}'",
        title.toStdString(), author.toStdString());
}

void StatusBar::setDocumentStatistics(int wordCount, int charCount,
                                      int pageCount) {
    if (!m_wordCountLabel) {
        return;  // Skip if in minimal mode
    }

    // Enhanced statistics display with proper validation and formatting
    if (wordCount < 0 || charCount < 0 || pageCount < 0) {
        LOG_WARNING(
            "StatusBar::setDocumentStatistics() - Invalid statistics: "
            "words={}, chars={}, pages={}",
            wordCount, charCount, pageCount);
        // Set to safe defaults
        wordCount = std::max(0, wordCount);
        charCount = std::max(0, charCount);
        pageCount = std::max(0, pageCount);
    }

    // Format numbers with thousands separators for better readability
    QLocale locale;
    m_wordCountLabel->setText(tr("Words: %1").arg(locale.toString(wordCount)));
    m_wordCountLabel->setToolTip(tr("Total word count in document"));

    m_charCountLabel->setText(
        tr("Characters: %1").arg(locale.toString(charCount)));
    m_charCountLabel->setToolTip(tr("Total character count including spaces"));

    m_pageCountLabel->setText(tr("Pages: %1").arg(pageCount));
    m_pageCountLabel->setToolTip(tr("Total number of pages in document"));

    if (pageCount > 0 && wordCount > 0) {
        int avgWords = wordCount / pageCount;
        m_avgWordsPerPageLabel->setText(tr("Avg Words/Page: %1").arg(avgWords));
        m_avgWordsPerPageLabel->setToolTip(tr("Average words per page"));

        // Estimate reading time (assuming 200 words per minute for average
        // reader)
        int readingMinutes = wordCount / 200;
        if (readingMinutes < 1) {
            m_readingTimeLabel->setText(tr("Est. Reading Time: < 1 min"));
            m_readingTimeLabel->setToolTip(
                tr("Estimated reading time for average reader (200 wpm)"));
        } else if (readingMinutes < 60) {
            m_readingTimeLabel->setText(
                tr("Est. Reading Time: %1 min").arg(readingMinutes));
            m_readingTimeLabel->setToolTip(
                tr("Estimated reading time: %1 minutes").arg(readingMinutes));
        } else {
            int hours = readingMinutes / 60;
            int minutes = readingMinutes % 60;
            if (minutes > 0) {
                m_readingTimeLabel->setText(
                    tr("Est. Reading Time: %1h %2min").arg(hours).arg(minutes));
                m_readingTimeLabel->setToolTip(
                    tr("Estimated reading time: %1 hours %2 minutes")
                        .arg(hours)
                        .arg(minutes));
            } else {
                m_readingTimeLabel->setText(
                    tr("Est. Reading Time: %1h").arg(hours));
                m_readingTimeLabel->setToolTip(
                    tr("Estimated reading time: %1 hours").arg(hours));
            }
        }
    } else {
        m_avgWordsPerPageLabel->setText(tr("Avg Words/Page: -"));
        m_avgWordsPerPageLabel->setToolTip(
            tr("Cannot calculate average - insufficient data"));
        m_readingTimeLabel->setText(tr("Est. Reading Time: -"));
        m_readingTimeLabel->setToolTip(
            tr("Cannot estimate reading time - insufficient data"));
    }

    LOG_DEBUG(
        "StatusBar::setDocumentStatistics() - Statistics updated: {} words, {} "
        "chars, {} pages",
        wordCount, charCount, pageCount);
}

void StatusBar::setDocumentSecurity(bool encrypted, bool copyAllowed,
                                    bool printAllowed) {
    if (!m_encryptionLabel) {
        return;  // Skip if in minimal mode
    }

    m_lastEncrypted = encrypted;
    m_lastCopyAllowed = copyAllowed;
    m_lastPrintAllowed = printAllowed;

    m_encryptionLabel->setText(
        tr("Encryption: %1").arg(encrypted ? tr("Enabled") : tr("Disabled")));
    m_copyPermissionLabel->setText(
        tr("Copy: %1").arg(copyAllowed ? tr("Allowed") : tr("Not Allowed")));
    m_printPermissionLabel->setText(
        tr("Print: %1").arg(printAllowed ? tr("Allowed") : tr("Not Allowed")));
    m_modifyPermissionLabel->setText(tr("Modify: %1").arg(tr("Not Allowed")));

    const QString strongStyle = QStringLiteral(
        "QLabel { color: %1; font-weight: 600; font-size: 11px; }");

    if (m_encryptionLabel) {
        const QColor color =
            encrypted ? STYLE.warningColor() : STYLE.successColor();
        m_encryptionLabel->setStyleSheet(strongStyle.arg(color.name()));
    }
    if (m_copyPermissionLabel) {
        const QColor color =
            copyAllowed ? STYLE.successColor() : STYLE.errorColor();
        m_copyPermissionLabel->setStyleSheet(strongStyle.arg(color.name()));
    }
    if (m_printPermissionLabel) {
        const QColor color =
            printAllowed ? STYLE.successColor() : STYLE.errorColor();
        m_printPermissionLabel->setStyleSheet(strongStyle.arg(color.name()));
    }
    if (m_modifyPermissionLabel) {
        m_modifyPermissionLabel->setStyleSheet(
            QStringLiteral("QLabel { color: %1; font-size: 11px; }")
                .arg(STYLE.textSecondaryColor().name()));
    }
}

void StatusBar::setPageInfo(int current, int total) {
    if (!m_pageLabel) {
        return;  // Skip if in minimal mode
    }

    // Enhanced input validation using UIErrorHandler
    auto validation = UIErrorHandler::instance().validatePageNumber(
        current + 1, total);  // Convert to 1-based
    if (validation.result != UIErrorHandler::ValidationResult::Valid) {
        UIErrorHandler::instance().showValidationFeedback(this, validation);

        // Correct invalid values
        if (total < 0)
            total = 0;
        if (current < 0)
            current = 0;
        if (current >= total && total > 0)
            current = total - 1;
    }

    m_currentTotalPages = total;
    m_currentPageNumber = current;

    if (total > 0) {
        // 更新页码输入框的占位符文�?
        m_pageInputEdit->setPlaceholderText(
            QString("%1/%2").arg(current + 1).arg(total));
        m_pageInputEdit->setEnabled(true);
        m_pageInputEdit->setToolTip(
            tr("Enter page number (1-%1) and press Enter to jump").arg(total));
        setLineEditInvalid(m_pageInputEdit, false);

        // Update validator range
        if (auto* validator = qobject_cast<const QIntValidator*>(
                m_pageInputEdit->validator())) {
            const_cast<QIntValidator*>(validator)->setRange(1, total);
        }
    } else {
        m_pageInputEdit->setPlaceholderText("0/0");
        m_pageInputEdit->setEnabled(false);
        m_pageInputEdit->setToolTip(tr("No document loaded"));
        setLineEditInvalid(m_pageInputEdit, false);
    }
}

void StatusBar::setZoomLevel(int percent) {
    if (!m_zoomInputEdit) {
        return;  // Skip if in minimal mode
    }

    // Enhanced input validation with user feedback
    bool hasValidationErrors = false;
    QString validationMessage;

    if (percent < 10 || percent > 500) {
        LOG_WARNING(
            "StatusBar::setZoomLevel() - Zoom level {} out of range [10, 500]",
            percent);
        validationMessage = tr("Zoom level adjusted to valid range (10%-500%)");
        hasValidationErrors = true;
        percent = std::clamp(percent, 10, 500);
    }

    // Show validation warning if zoom was clamped
    if (hasValidationErrors) {
        setWarningMessage(validationMessage, 2000);
    }

    m_zoomInputEdit->setText(QString("%1%").arg(percent));

    // Update tooltip with current zoom info
    m_zoomInputEdit->setToolTip(
        tr("Current zoom: %1% (Range: 10%-500%)").arg(percent));
}

void StatusBar::setZoomLevel(double percent) {
    // Enhanced input validation with user feedback
    bool hasValidationErrors = false;
    QString validationMessage;

    if (percent < 0.1 || percent > 5.0) {
        LOG_WARNING(
            "StatusBar::setZoomLevel() - Zoom factor {} out of range [0.1, "
            "5.0]",
            percent);
        validationMessage =
            tr("Zoom factor adjusted to valid range (10%-500%)");
        hasValidationErrors = true;
        percent = std::clamp(percent, 0.1, 5.0);
    }

    // Show validation warning if zoom was clamped
    if (hasValidationErrors) {
        setWarningMessage(validationMessage, 2000);
    }

    int roundedPercent = static_cast<int>((percent * 100) + 0.5);  // 四舍五入
    setZoomLevel(roundedPercent);
}

void StatusBar::setFileName(const QString& fileName) {
    if (!m_fileNameLabel) {
        return;  // Skip if in minimal mode
    }

    m_currentFileName = fileName;
    if (fileName.isEmpty()) {
        m_fileNameLabel->setText(tr("No Document"));
        m_fileNameLabel->setToolTip("");
        if (m_fileSizeLabel) {
            m_fileSizeLabel->setText(tr("Size: -"));
        }
    } else {
        QString displayName = formatFileName(fileName);
        m_fileNameLabel->setText(displayName);
        m_fileNameLabel->setToolTip(fileName);  // Full path as tooltip

        // Update file size if we have the file path
        if (m_fileSizeLabel) {
            QFileInfo fileInfo(fileName);
            if (fileInfo.exists()) {
                qint64 size = fileInfo.size();
                m_fileSizeLabel->setText(
                    tr("Size: %1").arg(formatFileSize(size)));
            } else {
                m_fileSizeLabel->setText(tr("Size: -"));
            }
        }
    }
}

void StatusBar::setFileName(const QString& fileName, qint64 fileSize) {
    if (!m_fileNameLabel) {
        return;  // Skip if in minimal mode
    }

    m_currentFileName = fileName;
    if (fileName.isEmpty()) {
        m_fileNameLabel->setText(tr("No Document"));
        m_fileNameLabel->setToolTip("");
        if (m_fileSizeLabel) {
            m_fileSizeLabel->setText(tr("Size: -"));
        }
    } else {
        QString displayName = formatFileName(fileName);
        m_fileNameLabel->setText(displayName);
        m_fileNameLabel->setToolTip(fileName);  // Full path as tooltip

        // Update file size with provided size
        if (m_fileSizeLabel) {
            if (fileSize > 0) {
                m_fileSizeLabel->setText(
                    tr("Size: %1").arg(formatFileSize(fileSize)));
            } else {
                // Fallback to file system check
                QFileInfo fileInfo(fileName);
                if (fileInfo.exists()) {
                    qint64 size = fileInfo.size();
                    m_fileSizeLabel->setText(
                        tr("Size: %1").arg(formatFileSize(size)));
                } else {
                    m_fileSizeLabel->setText(tr("Size: -"));
                }
            }
        }
    }
}

void StatusBar::setMessage(const QString& message) {
    displayTransientMessage(message, 3000, STYLE.overlayColor(),
                            STYLE.textColor());
    QStatusBar::showMessage(message, 3000);
}

void StatusBar::setErrorMessage(const QString& message, int timeout) {
    showMessage(message, MessagePriority::High, timeout);
    displayTransientMessage(message, timeout, STYLE.errorColor(), Qt::white);
    QStatusBar::showMessage(message, timeout);
}

void StatusBar::setSuccessMessage(const QString& message, int timeout) {
    showMessage(message, MessagePriority::Normal, timeout);
    displayTransientMessage(message, timeout, STYLE.successColor(), Qt::white);
    QStatusBar::showMessage(message, timeout);
}

void StatusBar::setWarningMessage(const QString& message, int timeout) {
    showMessage(message, MessagePriority::High, timeout);
    const QColor warningText = (STYLE.currentTheme() == Theme::Dark)
                                   ? STYLE.backgroundColor()
                                   : QColor(QStringLiteral("#1f2933"));
    displayTransientMessage(message, timeout, STYLE.warningColor(),
                            warningText);
    QStatusBar::showMessage(message, timeout);
}

void StatusBar::showMessage(const QString& message, MessagePriority priority,
                            int timeout) {
    // Only show if priority is higher or equal to current
    if (priority < m_currentMessagePriority) {
        LOG_DEBUG(
            "StatusBar::showMessage() - Ignoring lower priority message: {} < "
            "{}",
            static_cast<int>(priority),
            static_cast<int>(m_currentMessagePriority));
        return;
    }

    m_currentMessagePriority = priority;

    // Set priority timeout - higher priority messages reset the timer
    if (m_messagePriorityTimer) {
        m_messagePriorityTimer->stop();
        int priorityTimeout = timeout + (static_cast<int>(priority) * 1000);
        m_messagePriorityTimer->start(priorityTimeout);
    }

    // Display the message
    QColor backgroundColor, textColor;
    switch (priority) {
        case MessagePriority::Critical:
            backgroundColor = STYLE.errorColor();
            textColor = Qt::white;
            break;
        case MessagePriority::High:
            backgroundColor = STYLE.warningColor();
            textColor = (STYLE.currentTheme() == Theme::Dark)
                            ? STYLE.backgroundColor()
                            : QColor(QStringLiteral("#1f2933"));
            break;
        case MessagePriority::Normal:
            backgroundColor = STYLE.successColor();
            textColor = Qt::white;
            break;
        case MessagePriority::Low:
        default:
            backgroundColor = STYLE.overlayColor();
            textColor = STYLE.textColor();
            break;
    }

    displayTransientMessage(message, timeout, backgroundColor, textColor);
    QStatusBar::showMessage(message, timeout);

    LOG_DEBUG("StatusBar::showMessage() - Showing message with priority {}: {}",
              static_cast<int>(priority), message.toStdString());
}

void StatusBar::clearMessages(MessagePriority maxPriority) {
    if (m_currentMessagePriority <= maxPriority) {
        if (m_messageTimer) {
            m_messageTimer->stop();
        }
        if (m_messageAnimation) {
            m_messageAnimation->stop();
        }
        if (m_messageLabel) {
            m_messageLabel->hide();
            m_messageLabel->clear();
        }
        QStatusBar::clearMessage();
        m_currentMessagePriority = MessagePriority::Low;

        LOG_DEBUG(
            "StatusBar::clearMessages() - Messages cleared up to priority {}",
            static_cast<int>(maxPriority));
    }
}

void StatusBar::setSearchResults(int currentMatch, int totalMatches) {
    if (totalMatches > 0) {
        m_searchResultsLabel->setText(
            tr("%1 of %2").arg(currentMatch).arg(totalMatches));
        m_searchResultsLabel->show();
    } else {
        m_searchResultsLabel->setText(tr("No matches"));
        m_searchResultsLabel->show();
    }
}

void StatusBar::clearSearchResults() {
    m_searchResultsLabel->clear();
    m_searchResultsLabel->hide();
    m_searchInput->clear();
}

void StatusBar::clearDocumentInfo() {
    // Skip if in minimal mode
    if (!m_fileNameLabel) {
        LOG_DEBUG("StatusBar::clearDocumentInfo() - Skipping in minimal mode");
        return;
    }

    m_fileNameLabel->setText(tr("No Document"));
    m_fileNameLabel->setToolTip("");
    m_pageInputEdit->setPlaceholderText("0/0");
    m_pageInputEdit->setEnabled(false);
    m_pageInputEdit->setToolTip("");
    m_pageInputEdit->clear();
    setLineEditInvalid(m_pageInputEdit, false);
    m_zoomInputEdit->setText("100%");
    clearSearchResults();

    // Clear metadata (only if panels were created)
    if (m_titleLabel) {
        m_titleLabel->setText(tr("Title: -"));
    }
    if (m_authorLabel) {
        m_authorLabel->setText(tr("Author: -"));
    }
    if (m_subjectLabel) {
        m_subjectLabel->setText(tr("Subject: -"));
    }
    if (m_keywordsLabel) {
        m_keywordsLabel->setText(tr("Keywords: -"));
    }
    if (m_createdLabel) {
        m_createdLabel->setText(tr("Created: -"));
    }
    if (m_modifiedLabel) {
        m_modifiedLabel->setText(tr("Modified: -"));
    }
    if (m_fileSizeLabel) {
        m_fileSizeLabel->setText(tr("Size: -"));
    }

    // Clear statistics (only if panels were created)
    if (m_wordCountLabel) {
        m_wordCountLabel->setText(tr("Words: -"));
    }
    if (m_charCountLabel) {
        m_charCountLabel->setText(tr("Characters: -"));
    }
    if (m_pageCountLabel) {
        m_pageCountLabel->setText(tr("Pages: -"));
    }
    if (m_avgWordsPerPageLabel) {
        m_avgWordsPerPageLabel->setText(tr("Avg Words/Page: -"));
    }
    if (m_readingTimeLabel) {
        m_readingTimeLabel->setText(tr("Est. Reading Time: -"));
    }

    m_currentTotalPages = 0;
    m_currentPageNumber = 0;

    if (m_encryptionLabel) {
        setDocumentSecurity(false, true, true);
    }
}

void StatusBar::setCompactMode(bool compact) {
    m_compactMode = compact;

    if (compact) {
        collapseAllPanels();
        m_searchFrame->hide();
    } else {
        m_searchFrame->show();
    }
}

void StatusBar::expandAllPanels() {
    m_documentInfoPanel->setExpanded(true);
    m_statisticsPanel->setExpanded(true);
    m_securityPanel->setExpanded(true);
    m_quickActionsPanel->setExpanded(true);
}

void StatusBar::collapseAllPanels() {
    m_documentInfoPanel->setExpanded(false);
    m_statisticsPanel->setExpanded(false);
    m_securityPanel->setExpanded(false);
    m_quickActionsPanel->setExpanded(false);
}

QString StatusBar::formatFileName(const QString& fullPath) const {
    if (fullPath.isEmpty()) {
        return tr("No Document");
    }

    QFileInfo fileInfo(fullPath);
    QString baseName = fileInfo.fileName();

    // 如果文件名太长，进行截断
    QFontMetrics metrics(m_fileNameLabel->font());
    int maxWidth = m_fileNameLabel->maximumWidth() - 16;  // 留出padding空间

    if (metrics.horizontalAdvance(baseName) > maxWidth) {
        baseName = metrics.elidedText(baseName, Qt::ElideMiddle, maxWidth);
    }

    return baseName;
}

QString StatusBar::formatFileSize(qint64 size) const {
    if (size < 1024) {
        return QString("%1 B").arg(size);
    }
    if (size < 1024 * 1024) {
        return QString("%1 KB").arg(size / 1024.0, 0, 'f', 2);
    }
    if (size < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 2);
    } else {
        return QString("%1 GB").arg(size / (1024.0 * 1024.0 * 1024.0), 0, 'f',
                                    2);
    }
}

QString StatusBar::formatDateTime(const QDateTime& dateTime) const {
    if (!dateTime.isValid()) {
        return "-";
    }
    return dateTime.toString("yyyy-MM-dd hh:mm");
}

void StatusBar::onPageInputReturnPressed() {
    QString input = m_pageInputEdit->text().trimmed();
    if (validateAndJumpToPage(input)) {
        m_pageInputEdit->clear();
        m_pageInputEdit->clearFocus();
    }
}

void StatusBar::onPageInputEditingFinished() {
    if (m_pageInputEdit) {
        m_pageInputEdit->clear();
        setLineEditInvalid(m_pageInputEdit, false);
    }
}

void StatusBar::onPageInputTextChanged(const QString& text) {
    bool invalid = false;
    if (!text.isEmpty() && m_currentTotalPages > 0) {
        bool ok = false;
        const int pageNumber = text.toInt(&ok);
        invalid = !ok || pageNumber < 1 || pageNumber > m_currentTotalPages;
    }
    setLineEditInvalid(m_pageInputEdit, invalid);
}

void StatusBar::onZoomInputReturnPressed() {
    QString input = m_zoomInputEdit->text().trimmed();
    input.remove('%');

    bool ok;
    double zoomLevel = input.toDouble(&ok);

    if (ok && zoomLevel >= 25 && zoomLevel <= 400) {
        emit zoomLevelChangeRequested(zoomLevel / 100.0);
        m_zoomInputEdit->clearFocus();
        setSuccessMessage(
            tr("Zoom set to %1%").arg(static_cast<int>(zoomLevel)), 2000);
    } else {
        setErrorMessage(tr("Invalid zoom level (25-400%)"), 2000);
    }
}

void StatusBar::onSearchInputReturnPressed() {
    if (!m_searchInput) {
        return;  // Skip if in minimal mode
    }

    QString searchText = m_searchInput->text().trimmed();
    if (searchText.isEmpty()) {
        setWarningMessage(tr("Please enter search text"), 2000);
        return;
    }

    if (searchText.length() < 2) {
        setWarningMessage(tr("Search text must be at least 2 characters"),
                          2000);
        return;
    }

    // Show search progress
    showProgress(tr("Searching for: %1").arg(searchText), 5);

    // Clear previous search results
    clearSearchResults();

    emit searchRequested(searchText);

    LOG_DEBUG("StatusBar::onSearchInputReturnPressed() - Search requested: {}",
              searchText.toStdString());
}

void StatusBar::updateClock() {
    QDateTime current = QDateTime::currentDateTime();
    m_clockLabel->setText(current.toString("hh:mm:ss"));
}

void StatusBar::onMessageTimerTimeout() {
    // Animate fade out (if animation is available)
    if (m_messageAnimation) {
        m_messageAnimation->setStartValue(1.0);
        m_messageAnimation->setEndValue(0.0);
        m_messageAnimation->start();
    } else {
        // In offscreen mode, just hide immediately
        m_messageLabel->hide();
    }
}

bool StatusBar::validateAndJumpToPage(const QString& input) {
    if (input.isEmpty() || m_currentTotalPages <= 0) {
        return false;
    }

    bool ok;
    int pageNumber = input.toInt(&ok);

    if (!ok || pageNumber < 1 || pageNumber > m_currentTotalPages) {
        setErrorMessage(
            tr("Invalid page number (1-%1)").arg(m_currentTotalPages), 2000);
        setLineEditInvalid(m_pageInputEdit, true);
        return false;
    }

    // 发出页码跳转信号
    emit pageJumpRequested(pageNumber - 1);  // 转换�?-based
    setSuccessMessage(tr("Jumped to page %1").arg(pageNumber), 2000);
    setLineEditInvalid(m_pageInputEdit, false);
    return true;
}

void StatusBar::enablePageInput(bool enabled) {
    pageInputEdit->setEnabled(enabled);
    if (!enabled) {
        setLineEditInvalid(pageInputEdit, false);
    }
}

void StatusBar::setPageInputRange(int min, int max) {
    currentTotalPages = max;
    if (max > 0) {
        pageInputEdit->setToolTip(
            tr("Enter page number (%1-%2) and press Enter to jump")
                .arg(min)
                .arg(max));
        setLineEditInvalid(pageInputEdit, false);
    }
}

StatusBar::StatusBar(WidgetFactory* factory, QWidget* parent)
    : StatusBar(parent) {
    QPushButton* prevButton = factory->createButton(actionID::prev, "Prev");
    QPushButton* nextButton = factory->createButton(actionID::next, "Next");

    addWidget(prevButton);
    addWidget(nextButton);
}

void StatusBar::showLoadingProgress(const QString& message) {
    QString displayMessage = message.isEmpty() ? tr("Loading...") : message;
    loadingMessageLabel->setText(displayMessage);
    loadingMessageLabel->setVisible(true);
    loadingProgressBar->setValue(0);
    loadingProgressBar->setVisible(true);

    // 隐藏其他控件以节省空�?
    fileNameLabel->setVisible(false);
    separatorLabel1->setVisible(false);
}

void StatusBar::updateLoadingProgress(int progress) {
    progress = qBound(0, progress, 100);

    // 使用动画更新进度
    progressAnimation->stop();
    progressAnimation->setStartValue(loadingProgressBar->value());
    progressAnimation->setEndValue(progress);
    progressAnimation->start();
}

void StatusBar::setLoadingMessage(const QString& message) {
    if (loadingMessageLabel->isVisible()) {
        loadingMessageLabel->setText(message);
    }
}

void StatusBar::hideLoadingProgress() {
    if (!loadingProgressBar || !loadingMessageLabel) {
        return;  // Skip if in minimal mode
    }

    loadingProgressBar->setVisible(false);
    loadingMessageLabel->setVisible(false);

    // 恢复其他控件显示
    fileNameLabel->setVisible(true);
    separatorLabel1->setVisible(true);

    m_progressVisible = false;
    m_currentProgressPriority = 0;
}

void StatusBar::showProgress(const QString& message, int priority) {
    if (!m_loadingProgressBar || !m_loadingMessageLabel) {
        return;  // Skip if in minimal mode
    }

    // Only show if priority is higher or equal to current
    if (m_progressVisible && priority < m_currentProgressPriority) {
        LOG_DEBUG(
            "StatusBar::showProgress() - Ignoring lower priority progress: {} "
            "< {}",
            priority, m_currentProgressPriority);
        return;
    }

    m_currentProgressPriority = priority;
    m_progressVisible = true;

    QString displayMessage = message.isEmpty() ? tr("Processing...") : message;
    m_loadingMessageLabel->setText(displayMessage);
    m_loadingMessageLabel->setVisible(true);
    m_loadingProgressBar->setValue(0);
    m_loadingProgressBar->setVisible(true);

    // Hide other controls to save space for high priority operations
    if (priority > 5) {
        m_fileNameLabel->setVisible(false);
        separatorLabel1->setVisible(false);
    }

    LOG_DEBUG(
        "StatusBar::showProgress() - Showing progress with priority {}: {}",
        priority, displayMessage.toStdString());
}

void StatusBar::updateProgress(int progress, const QString& message) {
    if (!m_loadingProgressBar || !m_progressVisible) {
        return;  // Skip if in minimal mode or no progress shown
    }

    progress = qBound(0, progress, 100);

    // Use animation for smooth progress updates
    if (m_progressAnimation) {
        m_progressAnimation->stop();
        m_progressAnimation->setStartValue(m_loadingProgressBar->value());
        m_progressAnimation->setEndValue(progress);
        m_progressAnimation->start();
    } else {
        // Fallback for minimal mode
        m_loadingProgressBar->setValue(progress);
    }

    if (!message.isEmpty() && m_loadingMessageLabel) {
        m_loadingMessageLabel->setText(message);
    }

    LOG_DEBUG("StatusBar::updateProgress() - Progress updated to {}%",
              progress);
}

void StatusBar::hideProgress() {
    if (!m_loadingProgressBar || !m_loadingMessageLabel) {
        return;  // Skip if in minimal mode
    }

    m_loadingProgressBar->setVisible(false);
    m_loadingMessageLabel->setVisible(false);

    // Restore other controls
    if (m_fileNameLabel) {
        m_fileNameLabel->setVisible(true);
    }
    if (separatorLabel1) {
        separatorLabel1->setVisible(true);
    }

    m_progressVisible = false;
    m_currentProgressPriority = 0;

    LOG_DEBUG("StatusBar::hideProgress() - Progress hidden");
}

void StatusBar::setProgressPriority(int priority) {
    m_currentProgressPriority = priority;
}

void StatusBar::retranslateUi() {
    // Update labels with current translations
    pageLabel->setText(tr("Page:"));
    pageInputEdit->setPlaceholderText(tr("Page"));

    // Re-apply current state with new translations
    if (currentFileName.isEmpty()) {
        fileNameLabel->setText(tr("No Document"));
    } else {
        setFileName(currentFileName);
    }

    // Update page info tooltips
    if (currentTotalPages > 0) {
        pageInputEdit->setToolTip(
            tr("Enter page number (1-%1) and press Enter to jump")
                .arg(currentTotalPages));
    }

    // Update zoom label (will be set with correct value by setZoomLevel)
    zoomLabel->setText(tr("Zoom: %1%").arg(100));
}

void StatusBar::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QStatusBar::changeEvent(event);
}

void StatusBar::resizeEvent(QResizeEvent* event) {
    QStatusBar::resizeEvent(event);

    // Adjust compact mode based on width
    if (event->size().width() < 800 && !m_compactMode) {
        setCompactMode(true);
    } else if (event->size().width() >= 800 && m_compactMode) {
        setCompactMode(false);
    }

    if (m_messageLabel && m_messageLabel->isVisible()) {
        const int x = (width() - m_messageLabel->width()) / 2;
        const int y = height() - m_messageLabel->height() - STYLE.spacingMD();
        m_messageLabel->move(std::max(0, x), std::max(0, y));
    }
}

bool StatusBar::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        // Enhanced keyboard navigation for input fields
        if (watched == m_pageInputEdit) {
            if (keyEvent->key() == Qt::Key_Up && m_currentTotalPages > 0) {
                // Navigate to next page
                int currentPage = m_currentPageNumber + 1;
                if (currentPage < m_currentTotalPages) {
                    emit pageJumpRequested(currentPage);
                    return true;
                }
            } else if (keyEvent->key() == Qt::Key_Down &&
                       m_currentTotalPages > 0) {
                // Navigate to previous page
                int currentPage = m_currentPageNumber - 1;
                if (currentPage >= 0) {
                    emit pageJumpRequested(currentPage);
                    return true;
                }
            }
        } else if (watched == m_zoomInputEdit) {
            if (keyEvent->key() == Qt::Key_Up) {
                // Increase zoom by 10%
                QString currentText = m_zoomInputEdit->text();
                currentText.remove('%');
                bool ok;
                double currentZoom = currentText.toDouble(&ok);
                if (ok && currentZoom < 400) {
                    double newZoom = std::min(400.0, currentZoom + 10.0);
                    emit zoomLevelChangeRequested(newZoom / 100.0);
                    return true;
                }
            } else if (keyEvent->key() == Qt::Key_Down) {
                // Decrease zoom by 10%
                QString currentText = m_zoomInputEdit->text();
                currentText.remove('%');
                bool ok;
                double currentZoom = currentText.toDouble(&ok);
                if (ok && currentZoom > 25) {
                    double newZoom = std::max(25.0, currentZoom - 10.0);
                    emit zoomLevelChangeRequested(newZoom / 100.0);
                    return true;
                }
            }
        } else if (watched == m_searchInput) {
            if (keyEvent->key() == Qt::Key_Escape) {
                // Clear search and hide results
                clearSearchResults();
                m_searchInput->clear();
                return true;
            }
        }

        // Tab navigation between input fields
        if (keyEvent->key() == Qt::Key_Tab) {
            if (watched == m_pageInputEdit) {
                m_zoomInputEdit->setFocus();
                return true;
            } else if (watched == m_zoomInputEdit) {
                m_searchInput->setFocus();
                return true;
            } else if (watched == m_searchInput) {
                m_pageInputEdit->setFocus();
                return true;
            }
        }
    }

    return QStatusBar::eventFilter(watched, event);
}
