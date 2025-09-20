#include "StatusBar.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QFileInfo>
#include <QFontMetrics>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QResizeEvent>
#include <QGridLayout>

// ExpandableInfoPanel Implementation
ExpandableInfoPanel::ExpandableInfoPanel(const QString& title, QWidget* parent)
    : QWidget(parent), m_expanded(false) {
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Create toggle button
    m_toggleButton = new QPushButton(title, this);
    m_toggleButton->setCheckable(true);
    m_toggleButton->setStyleSheet(
        "QPushButton {"
        "   text-align: left;"
        "   padding: 6px 12px;"
        "   border: none;"
        "   background-color: #f8f9fa;"
        "   border-bottom: 1px solid #dee2e6;"
        "   font-weight: bold;"
        "   color: #495057;"
        "}"
        "QPushButton:hover {"
        "   background-color: #e9ecef;"
        "}"
        "QPushButton:checked {"
        "   background-color: #007bff;"
        "   color: white;"
        "}"
    );
    
    // Create content frame
    m_contentFrame = new QFrame(this);
    m_contentFrame->setFrameStyle(QFrame::NoFrame);
    m_contentFrame->setMaximumHeight(0);
    m_contentFrame->setStyleSheet(
        "QFrame {"
        "   background-color: #ffffff;"
        "   border: 1px solid #dee2e6;"
        "   border-top: none;"
        "   padding: 8px;"
        "}"
    );
    
    // Setup animation
    m_animation = new QPropertyAnimation(m_contentFrame, "maximumHeight", this);
    m_animation->setDuration(200);
    m_animation->setEasingCurve(QEasingCurve::InOutQuad);
    
    mainLayout->addWidget(m_toggleButton);
    mainLayout->addWidget(m_contentFrame);
    
    connect(m_toggleButton, &QPushButton::toggled, this, [this](bool checked) {
        setExpanded(checked);
    });
    
    updateToggleButton();
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
    if (m_expanded == expanded) return;
    
    m_expanded = expanded;
    m_toggleButton->setChecked(expanded);
    updateToggleButton();
    
    if (animated) {
        m_animation->stop();
        m_animation->setStartValue(m_contentFrame->height());
        m_animation->setEndValue(expanded ? m_contentFrame->sizeHint().height() : 0);
        m_animation->start();
    } else {
        m_contentFrame->setMaximumHeight(expanded ? m_contentFrame->sizeHint().height() : 0);
    }
    
    emit expandedChanged(expanded);
}

void ExpandableInfoPanel::updateToggleButton() {
    QString arrow = m_expanded ? "▼" : "▶";
    m_toggleButton->setText(arrow + " " + m_toggleButton->text().mid(2));
}

// StatusBar Implementation
StatusBar::StatusBar(QWidget* parent)
    : QStatusBar(parent), m_currentTotalPages(0), m_currentPageNumber(0), m_compactMode(false) {
    
    // Setup main horizontal layout
    QWidget* containerWidget = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(containerWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Setup main section (always visible)
    setupMainSection();
    mainLayout->addWidget(m_mainSection);
    
    // Setup expandable panels
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
    m_messageLabel->setStyleSheet(
        "QLabel {"
        "   background-color: rgba(0, 0, 0, 0.8);"
        "   color: white;"
        "   padding: 8px 16px;"
        "   border-radius: 4px;"
        "   font-size: 14px;"
        "}"
    );
    m_messageLabel->hide();
    
    // Setup message timer
    m_messageTimer = new QTimer(this);
    m_messageTimer->setSingleShot(true);
    connect(m_messageTimer, &QTimer::timeout, this, &StatusBar::onMessageTimerTimeout);
    
    // Setup message animation
    m_messageAnimation = new QPropertyAnimation(m_messageLabel, "windowOpacity", this);
    m_messageAnimation->setDuration(300);
    
    // Setup clock timer
    m_clockTimer = new QTimer(this);
    connect(m_clockTimer, &QTimer::timeout, this, &StatusBar::updateClock);
    m_clockTimer->start(1000);
    
    // Apply enhanced styling
    applyEnhancedStyle();
    
    // Set initial state
    clearDocumentInfo();
}

void StatusBar::setupMainSection() {
    m_mainSection = new QFrame(this);
    m_mainSection->setFrameStyle(QFrame::NoFrame);
    
    QHBoxLayout* layout = new QHBoxLayout(m_mainSection);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(12);
    
    // File name
    m_fileNameLabel = new QLabel(tr("No Document"), this);
    m_fileNameLabel->setMinimumWidth(150);
    m_fileNameLabel->setMaximumWidth(300);
    m_fileNameLabel->setStyleSheet(
        "QLabel {"
        "   font-weight: bold;"
        "   color: #007bff;"
        "}"
    );
    layout->addWidget(m_fileNameLabel);
    
    // Separator
    separatorLabel1 = new QLabel("|", this);
    separatorLabel1->setAlignment(Qt::AlignCenter);
    separatorLabel1->setStyleSheet("QLabel { color: gray; padding: 2px 4px; }");
    layout->addWidget(separatorLabel1);
    
    // Page navigation
    m_pageLabel = new QLabel(tr("Page:"), this);
    layout->addWidget(m_pageLabel);
    
    m_pageInputEdit = new QLineEdit(this);
    m_pageInputEdit->setMaximumWidth(60);
    m_pageInputEdit->setAlignment(Qt::AlignCenter);
    m_pageInputEdit->setPlaceholderText("0/0");
    m_pageInputEdit->setStyleSheet(
        "QLineEdit {"
        "   border: 1px solid #ced4da;"
        "   border-radius: 4px;"
        "   padding: 2px 4px;"
        "   background-color: white;"
        "}"
        "QLineEdit:hover {"
        "   border-color: #80bdff;"
        "}"
        "QLineEdit:focus {"
        "   border-color: #007bff;"
        "   background-color: #f0f8ff;"
        "}"
    );
    layout->addWidget(m_pageInputEdit);
    
    // Separator
    separatorLabel2 = new QLabel("|", this);
    separatorLabel2->setAlignment(Qt::AlignCenter);
    separatorLabel2->setStyleSheet("QLabel { color: gray; padding: 2px 4px; }");
    layout->addWidget(separatorLabel2);
    
    // Zoom
    m_zoomLabel = new QLabel(tr("Zoom:"), this);
    layout->addWidget(m_zoomLabel);
    
    m_zoomInputEdit = new QLineEdit(this);
    m_zoomInputEdit->setMaximumWidth(60);
    m_zoomInputEdit->setAlignment(Qt::AlignCenter);
    m_zoomInputEdit->setText("100%");
    m_zoomInputEdit->setStyleSheet(m_pageInputEdit->styleSheet());
    layout->addWidget(m_zoomInputEdit);
    
    // Search
    m_searchFrame = new QFrame(this);
    QHBoxLayout* searchLayout = new QHBoxLayout(m_searchFrame);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(4);
    
    QLabel* searchIcon = new QLabel("🔍", this);
    searchLayout->addWidget(searchIcon);
    
    m_searchInput = new QLineEdit(this);
    m_searchInput->setPlaceholderText(tr("Search..."));
    m_searchInput->setMinimumWidth(150);
    m_searchInput->setStyleSheet(m_pageInputEdit->styleSheet());
    searchLayout->addWidget(m_searchInput);
    
    m_searchResultsLabel = new QLabel("", this);
    m_searchResultsLabel->setStyleSheet("QLabel { color: #6c757d; font-size: 11px; }");
    searchLayout->addWidget(m_searchResultsLabel);
    
    layout->addWidget(m_searchFrame);
    
    // Loading progress
    m_loadingProgressBar = new QProgressBar(this);
    m_loadingProgressBar->setMinimumWidth(150);
    m_loadingProgressBar->setMaximumWidth(200);
    m_loadingProgressBar->setMaximumHeight(16);
    m_loadingProgressBar->setTextVisible(true);
    m_loadingProgressBar->hide();
    m_loadingProgressBar->setStyleSheet(
        "QProgressBar {"
        "   border: 1px solid #ced4da;"
        "   border-radius: 8px;"
        "   text-align: center;"
        "   font-size: 10px;"
        "}"
        "QProgressBar::chunk {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "       stop:0 #28a745, stop:1 #20c997);"
        "   border-radius: 7px;"
        "}"
    );
    
    m_loadingMessageLabel = new QLabel("", this);
    m_loadingMessageLabel->hide();
    m_loadingMessageLabel->setStyleSheet("QLabel { color: #6c757d; }");
    
    layout->addWidget(m_loadingMessageLabel);
    layout->addWidget(m_loadingProgressBar);
    
    layout->addStretch();
    
    // Clock
    m_clockLabel = new QLabel(this);
    m_clockLabel->setStyleSheet(
        "QLabel {"
        "   color: #6c757d;"
        "   font-family: 'Consolas', 'Monaco', monospace;"
        "   font-size: 12px;"
        "}"
    );
    layout->addWidget(m_clockLabel);
    updateClock();
    
    // Connect signals
    connect(m_pageInputEdit, &QLineEdit::returnPressed, 
            this, &StatusBar::onPageInputReturnPressed);
    connect(m_pageInputEdit, &QLineEdit::editingFinished, 
            this, &StatusBar::onPageInputEditingFinished);
    connect(m_pageInputEdit, &QLineEdit::textChanged,
            this, &StatusBar::onPageInputTextChanged);
    connect(m_zoomInputEdit, &QLineEdit::returnPressed, 
            this, &StatusBar::onZoomInputReturnPressed);
    connect(m_searchInput, &QLineEdit::returnPressed, 
            this, &StatusBar::onSearchInputReturnPressed);

    // Setup input validator
    QIntValidator* validator = new QIntValidator(1, 9999, this);
    m_pageInputEdit->setValidator(validator);
    m_pageInputEdit->setEnabled(false);
    m_pageInputEdit->setToolTip("");
}

void StatusBar::setupDocumentInfoPanel() {
    m_documentInfoPanel = new ExpandableInfoPanel(tr("▶ Document Info"), this);
    
    QWidget* content = new QWidget();
    QGridLayout* layout = new QGridLayout(content);
    layout->setSpacing(4);
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
    QString labelStyle = "QLabel { color: #495057; font-size: 11px; padding: 2px; }";
    m_titleLabel->setStyleSheet(labelStyle);
    m_authorLabel->setStyleSheet(labelStyle);
    m_subjectLabel->setStyleSheet(labelStyle);
    m_keywordsLabel->setStyleSheet(labelStyle);
    m_createdLabel->setStyleSheet(labelStyle);
    m_modifiedLabel->setStyleSheet(labelStyle);
    m_fileSizeLabel->setStyleSheet(labelStyle);
    
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
    m_statisticsPanel = new ExpandableInfoPanel(tr("▶ Statistics"), this);
    
    QWidget* content = new QWidget();
    QGridLayout* layout = new QGridLayout(content);
    layout->setSpacing(4);
    
    // Create labels
    m_wordCountLabel = new QLabel(tr("Words: -"), this);
    m_charCountLabel = new QLabel(tr("Characters: -"), this);
    m_pageCountLabel = new QLabel(tr("Pages: -"), this);
    m_avgWordsPerPageLabel = new QLabel(tr("Avg Words/Page: -"), this);
    m_readingTimeLabel = new QLabel(tr("Est. Reading Time: -"), this);
    
    // Style labels
    QString labelStyle = "QLabel { color: #495057; font-size: 11px; padding: 2px; }";
    m_wordCountLabel->setStyleSheet(labelStyle);
    m_charCountLabel->setStyleSheet(labelStyle);
    m_pageCountLabel->setStyleSheet(labelStyle);
    m_avgWordsPerPageLabel->setStyleSheet(labelStyle);
    m_readingTimeLabel->setStyleSheet(labelStyle);
    
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
    chartPlaceholder->setText(tr("📊 Page Distribution"));
    chartPlaceholder->setStyleSheet(
        "QLabel {"
        "   background-color: #f8f9fa;"
        "   border: 1px solid #dee2e6;"
        "   border-radius: 4px;"
        "   color: #6c757d;"
        "}"
    );
    layout->addWidget(chartPlaceholder, 3, 0, 1, 2);
    
    m_statisticsPanel->setContentWidget(content);
}

void StatusBar::setupSecurityPanel() {
    m_securityPanel = new ExpandableInfoPanel(tr("▶ Security"), this);
    
    QWidget* content = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setSpacing(4);
    
    // Create labels
    m_encryptionLabel = new QLabel(tr("🔒 Encryption: No"), this);
    m_copyPermissionLabel = new QLabel(tr("📋 Copy: Allowed"), this);
    m_printPermissionLabel = new QLabel(tr("🖨️ Print: Allowed"), this);
    m_modifyPermissionLabel = new QLabel(tr("✏️ Modify: Not Allowed"), this);
    
    // Style labels
    QString labelStyle = "QLabel { color: #495057; font-size: 11px; padding: 4px; }";
    m_encryptionLabel->setStyleSheet(labelStyle);
    m_copyPermissionLabel->setStyleSheet(labelStyle);
    m_printPermissionLabel->setStyleSheet(labelStyle);
    m_modifyPermissionLabel->setStyleSheet(labelStyle);
    
    // Add to layout
    layout->addWidget(m_encryptionLabel);
    layout->addWidget(m_copyPermissionLabel);
    layout->addWidget(m_printPermissionLabel);
    layout->addWidget(m_modifyPermissionLabel);
    
    m_securityPanel->setContentWidget(content);
}

void StatusBar::setupQuickActionsPanel() {
    m_quickActionsPanel = new ExpandableInfoPanel(tr("▶ Quick Actions"), this);
    
    QWidget* content = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(content);
    layout->setSpacing(8);
    
    // Create action buttons
    m_bookmarkBtn = new QPushButton(tr("🔖 Bookmark"), this);
    m_annotateBtn = new QPushButton(tr("✏️ Annotate"), this);
    m_shareBtn = new QPushButton(tr("📤 Share"), this);
    m_exportBtn = new QPushButton(tr("💾 Export"), this);
    
    // Style buttons
    QString buttonStyle = 
        "QPushButton {"
        "   background-color: #007bff;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 4px;"
        "   padding: 6px 12px;"
        "   font-size: 12px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #0056b3;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #004085;"
        "}";
    
    m_bookmarkBtn->setStyleSheet(buttonStyle);
    m_annotateBtn->setStyleSheet(buttonStyle);
    m_shareBtn->setStyleSheet(buttonStyle);
    m_exportBtn->setStyleSheet(buttonStyle);
    
    // Add to layout
    layout->addWidget(m_bookmarkBtn);
    layout->addWidget(m_annotateBtn);
    layout->addWidget(m_shareBtn);
    layout->addWidget(m_exportBtn);
    layout->addStretch();
    
    m_quickActionsPanel->setContentWidget(content);
}

void StatusBar::applyEnhancedStyle() {
    setStyleSheet(
        "QStatusBar {"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "       stop:0 #ffffff, stop:1 #f8f9fa);"
        "   border-top: 2px solid #dee2e6;"
        "   min-height: 32px;"
        "}"
    );
    
    // Add drop shadow effect
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(8);
    shadow->setColor(QColor(0, 0, 0, 20));
    shadow->setOffset(0, -2);
    setGraphicsEffect(shadow);
}

void StatusBar::setDocumentInfo(const QString& fileName, int currentPage, int totalPages, double zoomLevel) {
    setFileName(fileName);
    setPageInfo(currentPage, totalPages);
    setZoomLevel(zoomLevel);
}

void StatusBar::setDocumentMetadata(const QString& title, const QString& author,
                                   const QString& subject, const QString& keywords,
                                   const QDateTime& created, const QDateTime& modified) {
    m_titleLabel->setText(tr("Title: %1").arg(title.isEmpty() ? "-" : title));
    m_authorLabel->setText(tr("Author: %1").arg(author.isEmpty() ? "-" : author));
    m_subjectLabel->setText(tr("Subject: %1").arg(subject.isEmpty() ? "-" : subject));
    m_keywordsLabel->setText(tr("Keywords: %1").arg(keywords.isEmpty() ? "-" : keywords));
    m_createdLabel->setText(tr("Created: %1").arg(formatDateTime(created)));
    m_modifiedLabel->setText(tr("Modified: %1").arg(formatDateTime(modified)));
}

void StatusBar::setDocumentStatistics(int wordCount, int charCount, int pageCount) {
    m_wordCountLabel->setText(tr("Words: %1").arg(wordCount));
    m_charCountLabel->setText(tr("Characters: %1").arg(charCount));
    m_pageCountLabel->setText(tr("Pages: %1").arg(pageCount));
    
    if (pageCount > 0) {
        int avgWords = wordCount / pageCount;
        m_avgWordsPerPageLabel->setText(tr("Avg Words/Page: %1").arg(avgWords));
        
        // Estimate reading time (assuming 200 words per minute)
        int readingMinutes = wordCount / 200;
        if (readingMinutes < 1) {
            m_readingTimeLabel->setText(tr("Est. Reading Time: < 1 min"));
        } else if (readingMinutes < 60) {
            m_readingTimeLabel->setText(tr("Est. Reading Time: %1 min").arg(readingMinutes));
        } else {
            int hours = readingMinutes / 60;
            int minutes = readingMinutes % 60;
            m_readingTimeLabel->setText(tr("Est. Reading Time: %1h %2min").arg(hours).arg(minutes));
        }
    }
}

void StatusBar::setDocumentSecurity(bool encrypted, bool copyAllowed, bool printAllowed) {
    m_encryptionLabel->setText(tr("🔒 Encryption: %1").arg(encrypted ? tr("Yes") : tr("No")));
    m_copyPermissionLabel->setText(tr("📋 Copy: %1").arg(copyAllowed ? tr("Allowed") : tr("Not Allowed")));
    m_printPermissionLabel->setText(tr("🖨️ Print: %1").arg(printAllowed ? tr("Allowed") : tr("Not Allowed")));
    
    // Update colors based on permissions
    if (!copyAllowed || !printAllowed) {
        m_securityPanel->setStyleSheet("QLabel { color: #dc3545; }");
    } else {
        m_securityPanel->setStyleSheet("QLabel { color: #28a745; }");
    }
}

void StatusBar::setPageInfo(int current, int total) {
    m_currentTotalPages = total;
    m_currentPageNumber = current;

    if (total > 0) {
        // 更新页码输入框的占位符文本
        m_pageInputEdit->setPlaceholderText(QString("%1/%2").arg(current + 1).arg(total));
        m_pageInputEdit->setEnabled(true);
        m_pageInputEdit->setToolTip(tr("Enter page number (1-%1) and press Enter to jump").arg(total));
    } else {
        m_pageInputEdit->setPlaceholderText("0/0");
        m_pageInputEdit->setEnabled(false);
        m_pageInputEdit->setToolTip("");
    }
}

void StatusBar::setZoomLevel(int percent) {
    m_zoomInputEdit->setText(QString("%1%").arg(percent));
}

void StatusBar::setZoomLevel(double percent) {
    int roundedPercent = static_cast<int>(percent * 100 + 0.5); // 四舍五入
    setZoomLevel(roundedPercent);
}

void StatusBar::setFileName(const QString& fileName) {
    m_currentFileName = fileName;
    if (fileName.isEmpty()) {
        m_fileNameLabel->setText(tr("No Document"));
        m_fileNameLabel->setToolTip("");
    } else {
        QString displayName = formatFileName(fileName);
        m_fileNameLabel->setText(displayName);
        m_fileNameLabel->setToolTip(fileName); // Full path as tooltip
    }
}

void StatusBar::setMessage(const QString& message) {
    showMessage(message, 3000);
}

void StatusBar::setErrorMessage(const QString& message, int timeout) {
    m_messageLabel->setStyleSheet(
        "QLabel {"
        "   background-color: rgba(220, 53, 69, 0.9);"
        "   color: white;"
        "   padding: 8px 16px;"
        "   border-radius: 4px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "}"
    );
    setMessage("❌ " + message);
    m_messageTimer->start(timeout);
}

void StatusBar::setSuccessMessage(const QString& message, int timeout) {
    m_messageLabel->setStyleSheet(
        "QLabel {"
        "   background-color: rgba(40, 167, 69, 0.9);"
        "   color: white;"
        "   padding: 8px 16px;"
        "   border-radius: 4px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "}"
    );
    setMessage("✅ " + message);
    m_messageTimer->start(timeout);
}

void StatusBar::setWarningMessage(const QString& message, int timeout) {
    m_messageLabel->setStyleSheet(
        "QLabel {"
        "   background-color: rgba(255, 193, 7, 0.9);"
        "   color: #212529;"
        "   padding: 8px 16px;"
        "   border-radius: 4px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "}"
    );
    setMessage("⚠️ " + message);
    m_messageTimer->start(timeout);
}

void StatusBar::setSearchResults(int currentMatch, int totalMatches) {
    if (totalMatches > 0) {
        m_searchResultsLabel->setText(tr("%1 of %2").arg(currentMatch).arg(totalMatches));
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
    m_fileNameLabel->setText(tr("No Document"));
    m_fileNameLabel->setToolTip("");
    m_pageInputEdit->setPlaceholderText("0/0");
    m_pageInputEdit->setEnabled(false);
    m_pageInputEdit->setToolTip("");
    m_pageInputEdit->clear();
    m_zoomInputEdit->setText("100%");
    clearSearchResults();
    
    // Clear metadata
    m_titleLabel->setText(tr("Title: -"));
    m_authorLabel->setText(tr("Author: -"));
    m_subjectLabel->setText(tr("Subject: -"));
    m_keywordsLabel->setText(tr("Keywords: -"));
    m_createdLabel->setText(tr("Created: -"));
    m_modifiedLabel->setText(tr("Modified: -"));
    m_fileSizeLabel->setText(tr("Size: -"));
    
    // Clear statistics
    m_wordCountLabel->setText(tr("Words: -"));
    m_charCountLabel->setText(tr("Characters: -"));
    m_pageCountLabel->setText(tr("Pages: -"));
    m_avgWordsPerPageLabel->setText(tr("Avg Words/Page: -"));
    m_readingTimeLabel->setText(tr("Est. Reading Time: -"));
    
    m_currentTotalPages = 0;
    m_currentPageNumber = 0;
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
    int maxWidth = m_fileNameLabel->maximumWidth() - 16; // 留出padding空间

    if (metrics.horizontalAdvance(baseName) > maxWidth) {
        baseName = metrics.elidedText(baseName, Qt::ElideMiddle, maxWidth);
    }

    return baseName;
}

QString StatusBar::formatFileSize(qint64 size) const {
    if (size < 1024) {
        return QString("%1 B").arg(size);
    } else if (size < 1024 * 1024) {
        return QString("%1 KB").arg(size / 1024.0, 0, 'f', 2);
    } else if (size < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 2);
    } else {
        return QString("%1 GB").arg(size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
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
    m_pageInputEdit->clear();
}

void StatusBar::onPageInputTextChanged(const QString& text) {
    // 实时验证输入
    if (text.isEmpty()) {
        return;
    }

    bool ok;
    int pageNumber = text.toInt(&ok);

    if (!ok || pageNumber < 1 || pageNumber > m_currentTotalPages) {
        // 显示错误样式
        m_pageInputEdit->setStyleSheet(
            m_pageInputEdit->styleSheet() + 
            " QLineEdit:focus { border: 2px solid #dc3545; }"
        );
    } else {
        // 恢复正常样式
        m_pageInputEdit->setStyleSheet(
            m_pageInputEdit->styleSheet().replace(
                "border: 2px solid #dc3545;", "border: 1px solid #ced4da;"
            )
        );
    }
}

void StatusBar::onZoomInputReturnPressed() {
    QString input = m_zoomInputEdit->text().trimmed();
    input.remove('%');
    
    bool ok;
    double zoomLevel = input.toDouble(&ok);
    
    if (ok && zoomLevel >= 25 && zoomLevel <= 400) {
        emit zoomLevelChangeRequested(zoomLevel / 100.0);
        m_zoomInputEdit->clearFocus();
        setSuccessMessage(tr("Zoom set to %1%").arg(static_cast<int>(zoomLevel)), 2000);
    } else {
        setErrorMessage(tr("Invalid zoom level (25-400%)"), 2000);
    }
}

void StatusBar::onSearchInputReturnPressed() {
    QString searchText = m_searchInput->text().trimmed();
    if (!searchText.isEmpty()) {
        emit searchRequested(searchText);
    }
}

void StatusBar::updateClock() {
    QDateTime current = QDateTime::currentDateTime();
    m_clockLabel->setText(current.toString("hh:mm:ss"));
}

void StatusBar::onMessageTimerTimeout() {
    // Animate fade out
    m_messageAnimation->setStartValue(1.0);
    m_messageAnimation->setEndValue(0.0);
    m_messageAnimation->start();
    
    connect(m_messageAnimation, &QPropertyAnimation::finished, this, [this]() {
        m_messageLabel->hide();
    });
}

bool StatusBar::validateAndJumpToPage(const QString& input) {
    if (input.isEmpty() || m_currentTotalPages <= 0) {
        return false;
    }

    bool ok;
    int pageNumber = input.toInt(&ok);

    if (!ok || pageNumber < 1 || pageNumber > m_currentTotalPages) {
        setErrorMessage(tr("Invalid page number (1-%1)").arg(m_currentTotalPages), 2000);
        return false;
    }

    // 发出页码跳转信号
    emit pageJumpRequested(pageNumber - 1); // 转换为0-based
    setSuccessMessage(tr("Jumped to page %1").arg(pageNumber), 2000);
    return true;
}

void StatusBar::enablePageInput(bool enabled) {
    pageInputEdit->setEnabled(enabled);
}

void StatusBar::setPageInputRange(int min, int max) {
    currentTotalPages = max;
    if (max > 0) {
        pageInputEdit->setToolTip(tr("Enter page number (%1-%2) and press Enter to jump").arg(min).arg(max));
    }
}

StatusBar::StatusBar(WidgetFactory* factory, QWidget* parent)
    : StatusBar(parent)
{
    QPushButton* prevButton = factory->createButton(actionID::prev, "Prev");
    QPushButton* nextButton = factory->createButton(actionID::next, "Next");

    addWidget(prevButton);
    addWidget(nextButton);
}

void StatusBar::showLoadingProgress(const QString& message)
{
    QString displayMessage = message.isEmpty() ? tr("Loading...") : message;
    loadingMessageLabel->setText(displayMessage);
    loadingMessageLabel->setVisible(true);
    loadingProgressBar->setValue(0);
    loadingProgressBar->setVisible(true);

    // 隐藏其他控件以节省空间
    fileNameLabel->setVisible(false);
    separatorLabel1->setVisible(false);
}

void StatusBar::updateLoadingProgress(int progress)
{
    progress = qBound(0, progress, 100);

    // 使用动画更新进度
    progressAnimation->stop();
    progressAnimation->setStartValue(loadingProgressBar->value());
    progressAnimation->setEndValue(progress);
    progressAnimation->start();
}

void StatusBar::setLoadingMessage(const QString& message)
{
    if (loadingMessageLabel->isVisible()) {
        loadingMessageLabel->setText(message);
    }
}

void StatusBar::hideLoadingProgress()
{
    loadingProgressBar->setVisible(false);
    loadingMessageLabel->setVisible(false);

    // 恢复其他控件显示
    fileNameLabel->setVisible(true);
    separatorLabel1->setVisible(true);
}

void StatusBar::retranslateUi()
{
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
        pageInputEdit->setToolTip(tr("Enter page number (1-%1) and press Enter to jump").arg(currentTotalPages));
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
}
