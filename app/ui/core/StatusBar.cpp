#include "StatusBar.h"

// ElaWidgetTools
#include "ElaDef.h"
#include "ElaIcon.h"
#include "ElaLineEdit.h"
#include "ElaProgressBar.h"
#include "ElaText.h"
#include "ElaToolButton.h"

// Qt

#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QVBoxLayout>

#include <QPropertyAnimation>
#include <QTimer>

// Logging
#include "logging/SimpleLogging.h"

// Icon helper macro
#define ELA_ICON(iconName) \
    ElaIcon::getInstance()->getElaIcon(ElaIconType::iconName)

// ============================================================================
// 构造和析构
// ============================================================================

StatusBar::StatusBar(QWidget* parent)
    : QStatusBar(parent),
      m_currentPage(0),
      m_totalPages(0),
      m_zoomFactor(1.0),
      m_currentPanel(nullptr),
      m_pageInputEdit(nullptr),
      m_zoomInputEdit(nullptr),
      m_messageTimer(nullptr),
      m_messageAnimation(nullptr),
      m_currentMessagePriority(MessagePriority::Low),
      m_messagePriorityTimer(nullptr),
      m_loadingProgressBar(nullptr),
      m_loadingMessageLabel(nullptr),
      m_progressAnimation(nullptr),
      m_progressVisible(false),
      m_currentProgressPriority(0) {
    SLOG_INFO("StatusBar: Constructor started");

    setupUi();
    connectSignals();

    SLOG_INFO("StatusBar: Constructor completed");
}

StatusBar::StatusBar(QWidget* parent, bool minimalMode) : StatusBar(parent) {
    m_minimalMode = minimalMode;
    if (m_minimalMode) {
        // Reduce visual complexity in minimal/offscreen mode
        if (m_docInfoBtn)
            m_docInfoBtn->hide();
        if (m_statisticsBtn)
            m_statisticsBtn->hide();
        if (m_securityBtn)
            m_securityBtn->hide();
    }
}

StatusBar::~StatusBar() { SLOG_INFO("StatusBar: Destructor called"); }

// ============================================================================
// UI 初始化
// ============================================================================

void StatusBar::setupUi() {
    setFixedHeight(30);
    setSizeGripEnabled(false);

    setupMainInfo();
    setupPanelButtons();
    setupPanels();
}

void StatusBar::setupMainInfo() {
    // 文件名标签
    m_fileNameLabel = new ElaText(this);
    m_fileNameLabel->setMinimumWidth(200);
    addWidget(m_fileNameLabel);

    // 分隔符
    QFrame* separator1 = new QFrame(this);
    separator1->setFrameShape(QFrame::VLine);
    separator1->setFrameShadow(QFrame::Sunken);
    addWidget(separator1);

    // 页面输入框（交互式）
    m_pageInputEdit = new ElaLineEdit(this);
    m_pageInputEdit->setPlaceholderText("0/0");
    m_pageInputEdit->setMaximumWidth(80);
    m_pageInputEdit->setAlignment(Qt::AlignCenter);
    m_pageInputEdit->setEnabled(false);
    addWidget(m_pageInputEdit);

    // 页面信息标签（显示总页数）
    m_pageInfoLabel = new ElaText(this);
    m_pageInfoLabel->setMinimumWidth(50);
    addWidget(m_pageInfoLabel);

    // 分隔符
    QFrame* separator2 = new QFrame(this);
    separator2->setFrameShape(QFrame::VLine);
    separator2->setFrameShadow(QFrame::Sunken);
    addWidget(separator2);

    // 缩放输入框（交互式）
    m_zoomInputEdit = new ElaLineEdit(this);
    m_zoomInputEdit->setText("100%");
    m_zoomInputEdit->setMaximumWidth(60);
    m_zoomInputEdit->setAlignment(Qt::AlignCenter);
    addWidget(m_zoomInputEdit);

    // 分隔符
    QFrame* separator3 = new QFrame(this);
    separator3->setFrameShape(QFrame::VLine);
    separator3->setFrameShadow(QFrame::Sunken);
    addWidget(separator3);

    // 视图模式标签
    m_viewModeLabel = new ElaText(this);
    m_viewModeLabel->setMinimumWidth(100);
    addWidget(m_viewModeLabel);

    // 弹性空间
    addPermanentWidget(new QWidget(this), 1);

    // 消息标签（覆盖层，初始隐藏）
    m_messageLabel = new ElaText(this);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setMinimumWidth(280);
    m_messageLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_messageLabel->hide();

    // 消息定时器
    m_messageTimer = new QTimer(this);
    m_messageTimer->setSingleShot(true);

    // 消息优先级定时器
    m_messagePriorityTimer = new QTimer(this);
    m_messagePriorityTimer->setSingleShot(true);

    // 进度消息标签
    m_loadingMessageLabel = new ElaText(this);
    m_loadingMessageLabel->setMinimumWidth(100);
    m_loadingMessageLabel->setVisible(false);
    addPermanentWidget(m_loadingMessageLabel);

    // 进度条
    m_loadingProgressBar = new ElaProgressBar(this);
    m_loadingProgressBar->setMinimumWidth(150);
    m_loadingProgressBar->setMaximumHeight(15);
    m_loadingProgressBar->setVisible(false);
    addPermanentWidget(m_loadingProgressBar);

    // 进度动画
    m_progressAnimation =
        new QPropertyAnimation(m_loadingProgressBar, "value", this);
    m_progressAnimation->setDuration(200);

    // 初始化文本
    updateLabels();
}

void StatusBar::setupPanelButtons() {
    // 文档信息按钮
    m_docInfoBtn = new ElaToolButton(this);
    m_docInfoBtn->setIcon(ELA_ICON(FileLines));
    m_docInfoBtn->setToolTip(tr("Document Information"));
    m_docInfoBtn->setCheckable(true);
    m_docInfoBtn->setFixedSize(24, 24);
    addPermanentWidget(m_docInfoBtn);

    // 统计信息按钮
    m_statisticsBtn = new ElaToolButton(this);
    m_statisticsBtn->setIcon(ELA_ICON(ChartBar));
    m_statisticsBtn->setToolTip(tr("Statistics"));
    m_statisticsBtn->setCheckable(true);
    m_statisticsBtn->setFixedSize(24, 24);
    addPermanentWidget(m_statisticsBtn);

    // 安全信息按钮
    m_securityBtn = new ElaToolButton(this);
    m_securityBtn->setIcon(ELA_ICON(Lock));
    m_securityBtn->setToolTip(tr("Security"));
    m_securityBtn->setCheckable(true);
    m_securityBtn->setFixedSize(24, 24);
    addPermanentWidget(m_securityBtn);
}

void StatusBar::setupPanels() {
    // 创建面板（初始隐藏）
    m_docInfoPanel = createDocumentInfoPanel();
    m_statisticsPanel = createStatisticsPanel();
    m_securityPanel = createSecurityPanel();
}

void StatusBar::connectSignals() {
    // 面板按钮信号
    connect(m_docInfoBtn, &ElaToolButton::toggled, this, [this](bool checked) {
        if (checked) {
            showDocumentInfoPanel();
        } else {
            hidePanel(m_docInfoPanel);
        }
    });

    connect(m_statisticsBtn, &ElaToolButton::toggled, this,
            [this](bool checked) {
                if (checked) {
                    showStatisticsPanel();
                } else {
                    hidePanel(m_statisticsPanel);
                }
            });

    connect(m_securityBtn, &ElaToolButton::toggled, this, [this](bool checked) {
        if (checked) {
            showSecurityPanel();
        } else {
            hidePanel(m_securityPanel);
        }
    });

    // 连接输入框信号
    connect(m_pageInputEdit, &ElaLineEdit::returnPressed, this,
            &StatusBar::onPageInputReturnPressed);
    connect(m_zoomInputEdit, &ElaLineEdit::returnPressed, this,
            &StatusBar::onZoomInputReturnPressed);

    // 连接消息定时器
    connect(m_messageTimer, &QTimer::timeout, this,
            &StatusBar::onMessageTimerTimeout);
    connect(m_messagePriorityTimer, &QTimer::timeout, this,
            [this]() { m_currentMessagePriority = MessagePriority::Low; });
}

// ============================================================================
// 基本信息显示
// ============================================================================

void StatusBar::setFileName(const QString& fileName) {
    m_fileName = fileName;
    updateLabels();
}

void StatusBar::setPageInfo(int currentPage, int totalPages) {
    m_currentPage = currentPage;
    m_totalPages = totalPages;
    updateLabels();
}

void StatusBar::setZoomLevel(double zoomFactor) {
    m_zoomFactor = zoomFactor;
    updateLabels();
}

void StatusBar::setViewMode(const QString& mode) {
    m_viewMode = mode;
    updateLabels();
}

// ============================================================================
// 消息和进度
// ============================================================================

void StatusBar::showMessage(const QString& message, MessagePriority priority,
                            int timeout) {
    // Only show if priority is higher or equal to current
    if (priority < m_currentMessagePriority) {
        return;
    }

    m_currentMessagePriority = priority;

    // Set priority timeout
    if (m_messagePriorityTimer) {
        m_messagePriorityTimer->stop();
        int priorityTimeout = timeout + (static_cast<int>(priority) * 1000);
        m_messagePriorityTimer->start(priorityTimeout);
    }

    // Determine colors based on priority
    QColor backgroundColor, textColor;
    switch (priority) {
        case MessagePriority::Critical:
            backgroundColor = QColor("#dc3545");  // Red
            textColor = QColor("#ffffff");
            break;
        case MessagePriority::High:
            backgroundColor = QColor("#ffc107");  // Amber
            textColor = QColor("#000000");
            break;
        case MessagePriority::Normal:
            backgroundColor = QColor("#17a2b8");  // Info blue
            textColor = QColor("#ffffff");
            break;
        case MessagePriority::Low:
        default:
            backgroundColor = QColor("#6c757d");  // Gray
            textColor = QColor("#ffffff");
            break;
    }

    displayTransientMessage(message, timeout, backgroundColor, textColor);
    QStatusBar::showMessage(message, timeout);
}

void StatusBar::setErrorMessage(const QString& message, int timeout) {
    showMessage(message, MessagePriority::Critical, timeout);
}

void StatusBar::setSuccessMessage(const QString& message, int timeout) {
    QColor backgroundColor("#28a745");  // Green
    QColor textColor("#ffffff");
    displayTransientMessage(message, timeout, backgroundColor, textColor);
    QStatusBar::showMessage(message, timeout);
}

void StatusBar::setWarningMessage(const QString& message, int timeout) {
    showMessage(message, MessagePriority::High, timeout);
}

// --------------------------------------------------------------------------
// Backward-compatibility helpers expected by some tests
// --------------------------------------------------------------------------
void StatusBar::setMessage(const QString& message) {
    showMessage(message, MessagePriority::Normal, 3000);
}

void StatusBar::showLoadingProgress(const QString& message) {
    showProgress(message);
}

void StatusBar::updateLoadingProgress(int progress) {
    updateProgress(progress);
}

void StatusBar::hideLoadingProgress() { hideProgress(); }

void StatusBar::clearMessages(MessagePriority maxPriority) {
    if (m_currentMessagePriority <= maxPriority) {
        if (m_messageTimer) {
            m_messageTimer->stop();
        }
        if (m_messageLabel) {
            m_messageLabel->hide();
            m_messageLabel->clear();
        }
        QStatusBar::clearMessage();
        m_currentMessagePriority = MessagePriority::Low;
    }
}

void StatusBar::showProgress(const QString& message, int priority) {
    if (!m_loadingProgressBar || !m_loadingMessageLabel) {
        return;
    }

    // Only show if priority is higher or equal to current
    if (m_progressVisible && priority < m_currentProgressPriority) {
        return;
    }

    m_currentProgressPriority = priority;
    m_progressVisible = true;

    QString displayMessage = message.isEmpty() ? tr("Processing...") : message;
    m_loadingMessageLabel->setText(displayMessage);
    m_loadingMessageLabel->setVisible(true);
    m_loadingProgressBar->setValue(0);
    m_loadingProgressBar->setVisible(true);

    // Hide other controls for high priority operations
    if (priority > 5) {
        m_fileNameLabel->setVisible(false);
    }
}

void StatusBar::updateProgress(int progress, const QString& message) {
    if (!m_loadingProgressBar || !m_progressVisible) {
        return;
    }

    progress = qBound(0, progress, 100);

    // Ensure immediate value update for testing/headless environments
    m_loadingProgressBar->setValue(progress);

    // Optionally animate for UI smoothness (no-op in offscreen tests)
    if (m_progressAnimation) {
        m_progressAnimation->stop();
        m_progressAnimation->setStartValue(progress);
        m_progressAnimation->setEndValue(progress);
        m_progressAnimation->start();
    }

    if (!message.isEmpty() && m_loadingMessageLabel) {
        m_loadingMessageLabel->setText(message);
    }
}

void StatusBar::hideProgress() {
    if (m_loadingProgressBar) {
        m_loadingProgressBar->setVisible(false);
    }
    if (m_loadingMessageLabel) {
        // -----------------------------------------------------------------------------
        m_loadingMessageLabel->setVisible(false);
    }
    if (m_fileNameLabel) {
        m_fileNameLabel->setVisible(true);
    }
    m_progressVisible = false;
    m_currentProgressPriority = 0;
}

// Backward-compatibility API implementations
// -----------------------------------------------------------------------------
void StatusBar::setDocumentInfo(const QString& fileName, int currentPage,
                                int totalPages, double zoomLevel) {
    setFileName(fileName);
    setPageInfo(currentPage, totalPages);
    setZoomLevel(zoomLevel);
}

void StatusBar::setDocumentMetadata(const QString& title, const QString& author,
                                    const QString& subject,
                                    const QString& keywords,
                                    const QDateTime& created,
                                    const QDateTime& modified) {
    QMap<QString, QString> metadata;
    metadata["Title"] = title;
    metadata["Author"] = author;
    metadata["Subject"] = subject;
    metadata["Keywords"] = keywords;
    metadata["CreationDate"] = created.toString(Qt::ISODate);
    metadata["ModDate"] = modified.toString(Qt::ISODate);
    setDocumentMetadata(metadata);
}

void StatusBar::setDocumentStatistics(int wordCount, int charCount,
                                      int pageCount) {
    Q_UNUSED(wordCount);
    Q_UNUSED(charCount);
    QMap<QString, QString> statistics;
    statistics["Pages"] = QString::number(pageCount);
    setDocumentStatistics(statistics);
}

void StatusBar::setDocumentSecurity(bool encrypted, bool copyAllowed,
                                    bool printAllowed) {
    QMap<QString, QString> security;
    security["Encrypted"] = encrypted ? tr("Yes") : tr("No");
    security["CopyAllowed"] = copyAllowed ? tr("Yes") : tr("No");
    security["PrintAllowed"] = printAllowed ? tr("Yes") : tr("No");
    setDocumentSecurity(security);
}

void StatusBar::setSearchResults(int currentMatch, int totalMatches) {
    if (!m_searchResultsLabel) {
        m_searchResultsLabel = new ElaText(this);
        m_searchResultsLabel->setMinimumWidth(100);
        addPermanentWidget(m_searchResultsLabel);
    }
    m_searchResultsLabel->setText(
        tr("Search: %1 / %2").arg(currentMatch).arg(totalMatches));
    m_searchResultsLabel->setVisible(true);
}

void StatusBar::clearSearchResults() {
    if (m_searchResultsLabel) {
        m_searchResultsLabel->clear();
        m_searchResultsLabel->setVisible(false);
    }
}

void StatusBar::enablePageInput(bool enabled) {
    if (m_pageInputEdit) {
        m_pageInputEdit->setEnabled(enabled);
    }
}

void StatusBar::setPageInputRange(int minPage, int maxPage) {
    m_pageMinRange = qMax(1, minPage);
    m_pageMaxRange = qMax(0, maxPage);
    // Also sync with labels and validation by updating totals
    if (m_pageMaxRange > 0) {
        setPageInfo(m_currentPage, m_pageMaxRange);
    }
}

void StatusBar::setCompactMode(bool compact) {
    m_compactMode = compact;
    setFixedHeight(compact ? 24 : 30);
    if (m_fileNameLabel)
        m_fileNameLabel->setVisible(!compact);
    if (m_viewModeLabel)
        m_viewModeLabel->setVisible(!compact);
}

void StatusBar::expandAllPanels() {
    showDocumentInfoPanel();
    showStatisticsPanel();
    showSecurityPanel();
}

void StatusBar::collapseAllPanels() { hideAllPanels(); }

void StatusBar::setLoadingMessage(const QString& message) {
    if (m_loadingMessageLabel) {
        m_loadingMessageLabel->setText(message);
        m_loadingMessageLabel->setVisible(true);
    }
}

void StatusBar::clearDocumentInfo() {
    m_fileName.clear();
    m_currentPage = 0;
    m_totalPages = 0;
    m_zoomFactor = 1.0;
    updateLabels();
}

// ============================================================================
// 文档信息
// ============================================================================

void StatusBar::setDocumentMetadata(const QMap<QString, QString>& metadata) {
    m_metadata = metadata;
    updateDocumentInfoPanel();
}

void StatusBar::setDocumentStatistics(
    const QMap<QString, QString>& statistics) {
    m_statistics = statistics;
    updateStatisticsPanel();
}

void StatusBar::setDocumentSecurity(const QMap<QString, QString>& security) {
    m_security = security;
    updateSecurityPanel();
}

// ============================================================================
// 面板控制
// ============================================================================

void StatusBar::showDocumentInfoPanel() {
    SLOG_INFO("ElaStatusBar: Showing document info panel");

    // 取消其他按钮
    m_statisticsBtn->setChecked(false);
    m_securityBtn->setChecked(false);

    showPanel(m_docInfoPanel);
    emit panelVisibilityChanged("DocumentInfo", true);
}

void StatusBar::showStatisticsPanel() {
    SLOG_INFO("ElaStatusBar: Showing statistics panel");

    // 取消其他按钮
    m_docInfoBtn->setChecked(false);
    m_securityBtn->setChecked(false);

    showPanel(m_statisticsPanel);
    emit panelVisibilityChanged("Statistics", true);
}

void StatusBar::showSecurityPanel() {
    SLOG_INFO("ElaStatusBar: Showing security panel");

    // 取消其他按钮
    m_docInfoBtn->setChecked(false);
    m_statisticsBtn->setChecked(false);

    showPanel(m_securityPanel);
    emit panelVisibilityChanged("Security", true);
}

void StatusBar::hideAllPanels() {
    SLOG_INFO("ElaStatusBar: Hiding all panels");

    m_docInfoBtn->setChecked(false);
    m_statisticsBtn->setChecked(false);
    m_securityBtn->setChecked(false);

    hidePanel(m_docInfoPanel);
    hidePanel(m_statisticsPanel);
    hidePanel(m_securityPanel);
}

// ============================================================================
// 状态管理
// ============================================================================

void StatusBar::clearAll() {
    SLOG_INFO("ElaStatusBar: Clearing all information");

    m_fileName.clear();
    m_currentPage = 0;
    m_totalPages = 0;
    m_zoomFactor = 1.0;
    m_viewMode.clear();
    m_metadata.clear();
    m_statistics.clear();
    m_security.clear();

    updateLabels();
    hideAllPanels();
    hideProgress();
    m_messageLabel->clear();
}

void StatusBar::setEnabled(bool enabled) {
    QStatusBar::setEnabled(enabled);

    m_docInfoBtn->setEnabled(enabled);
    m_statisticsBtn->setEnabled(enabled);
    m_securityBtn->setEnabled(enabled);
}

// ============================================================================
// 事件处理
// ============================================================================

void StatusBar::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QStatusBar::changeEvent(event);
}

// ============================================================================
// 面板创建
// ============================================================================

QWidget* StatusBar::createDocumentInfoPanel() {
    QWidget* panel = new QWidget(this);
    panel->setObjectName("docInfoPanel");
    panel->setFixedHeight(200);
    panel->setVisible(false);

    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 10, 10, 10);

    ElaText* titleLabel = new ElaText(tr("Document Information"), panel);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);

    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setSpacing(5);
    layout->addLayout(gridLayout);

    // 添加信息行
    int row = 0;
    QStringList keys = {"Title",   "Author",   "Subject",      "Keywords",
                        "Creator", "Producer", "CreationDate", "ModDate"};
    for (const QString& key : keys) {
        ElaText* keyLabel =
            new ElaText(tr(key.toUtf8().constData()) + ":", panel);
        ElaText* valueLabel = new ElaText(panel);
        valueLabel->setObjectName("value_" + key);
        valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        gridLayout->addWidget(keyLabel, row, 0);
        gridLayout->addWidget(valueLabel, row, 1);
        row++;
    }

    layout->addStretch();

    return panel;
}

QWidget* StatusBar::createStatisticsPanel() {
    QWidget* panel = new QWidget(this);
    panel->setObjectName("statisticsPanel");
    panel->setFixedHeight(200);
    panel->setVisible(false);

    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 10, 10, 10);

    ElaText* titleLabel = new ElaText(tr("Statistics"), panel);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);

    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setSpacing(5);
    layout->addLayout(gridLayout);

    // 添加统计信息行
    int row = 0;
    QStringList keys = {"Pages", "FileSize", "PDFVersion", "PageSize",
                        "Orientation"};
    for (const QString& key : keys) {
        ElaText* keyLabel =
            new ElaText(tr(key.toUtf8().constData()) + ":", panel);
        ElaText* valueLabel = new ElaText(panel);
        valueLabel->setObjectName("stat_" + key);
        valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        gridLayout->addWidget(keyLabel, row, 0);
        gridLayout->addWidget(valueLabel, row, 1);
        row++;
    }

    layout->addStretch();

    return panel;
}

QWidget* StatusBar::createSecurityPanel() {
    QWidget* panel = new QWidget(this);
    panel->setObjectName("securityPanel");
    panel->setFixedHeight(200);
    panel->setVisible(false);

    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 10, 10, 10);

    ElaText* titleLabel = new ElaText(tr("Security"), panel);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);

    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setSpacing(5);
    layout->addLayout(gridLayout);

    // 添加安全信息行
    int row = 0;
    QStringList keys = {"Encrypted", "PrintAllowed", "CopyAllowed",
                        "ModifyAllowed", "AnnotateAllowed"};
    for (const QString& key : keys) {
        ElaText* keyLabel =
            new ElaText(tr(key.toUtf8().constData()) + ":", panel);
        ElaText* valueLabel = new ElaText(panel);
        valueLabel->setObjectName("sec_" + key);
        valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        gridLayout->addWidget(keyLabel, row, 0);
        gridLayout->addWidget(valueLabel, row, 1);
        row++;
    }

    layout->addStretch();

    return panel;
}

// ============================================================================
// 面板更新
// ============================================================================

void StatusBar::updateDocumentInfoPanel() {
    if (!m_docInfoPanel)
        return;

    QStringList keys = {"Title",   "Author",   "Subject",      "Keywords",
                        "Creator", "Producer", "CreationDate", "ModDate"};
    for (const QString& key : keys) {
        ElaText* valueLabel =
            m_docInfoPanel->findChild<ElaText*>("value_" + key);
        if (valueLabel) {
            QString value = m_metadata.value(key, tr("N/A"));
            valueLabel->setText(value);
        }
    }
}

void StatusBar::updateStatisticsPanel() {
    if (!m_statisticsPanel)
        return;

    QStringList keys = {"Pages", "FileSize", "PDFVersion", "PageSize",
                        "Orientation"};
    for (const QString& key : keys) {
        ElaText* valueLabel =
            m_statisticsPanel->findChild<ElaText*>("stat_" + key);
        if (valueLabel) {
            QString value = m_statistics.value(key, tr("N/A"));
            valueLabel->setText(value);
        }
    }
}

void StatusBar::updateSecurityPanel() {
    if (!m_securityPanel)
        return;

    QStringList keys = {"Encrypted", "PrintAllowed", "CopyAllowed",
                        "ModifyAllowed", "AnnotateAllowed"};
    for (const QString& key : keys) {
        ElaText* valueLabel =
            m_securityPanel->findChild<ElaText*>("sec_" + key);
        if (valueLabel) {
            QString value = m_security.value(key, tr("N/A"));
            valueLabel->setText(value);
        }
    }
}

// ============================================================================
// 辅助方法
// ============================================================================

void StatusBar::showPanel(QWidget* panel) {
    if (!panel)
        return;

    // 隐藏当前面板
    if (m_currentPanel && m_currentPanel != panel) {
        hidePanel(m_currentPanel);
    }

    // 显示新面板
    panel->setVisible(true);
    m_currentPanel = panel;

    // 将面板添加到主窗口（需要父窗口支持）
    // 这里简化处理，实际应该通过信号通知主窗口
}

void StatusBar::hidePanel(QWidget* panel) {
    if (!panel)
        return;

    panel->setVisible(false);

    if (m_currentPanel == panel) {
        m_currentPanel = nullptr;
    }
}

void StatusBar::retranslateUi() {
    SLOG_INFO("ElaStatusBar: Retranslating UI");

    // 更新按钮提示
    m_docInfoBtn->setToolTip(tr("Document Information"));
    m_statisticsBtn->setToolTip(tr("Statistics"));
    m_securityBtn->setToolTip(tr("Security"));

    // 更新标签
    updateLabels();

    // 更新面板
    updateDocumentInfoPanel();
    updateStatisticsPanel();
    updateSecurityPanel();
}

void StatusBar::updateLabels() {
    // 文件名
    if (m_fileName.isEmpty()) {
        m_fileNameLabel->setText(tr("No document"));
    } else {
        m_fileNameLabel->setText(m_fileName);
    }

    // 页面信息
    if (m_totalPages > 0) {
        m_pageInputEdit->setPlaceholderText(
            QString("%1/%2").arg(m_currentPage).arg(m_totalPages));
        m_pageInputEdit->setEnabled(true);
        m_pageInputEdit->setToolTip(
            tr("Enter page number (1-%1) and press Enter to jump")
                .arg(m_totalPages));
        m_pageInfoLabel->setText(tr("/ %1").arg(m_totalPages));
    } else {
        m_pageInputEdit->setPlaceholderText("0/0");
        m_pageInputEdit->setEnabled(false);
        m_pageInfoLabel->setText("");
    }

    // 缩放级别
    m_zoomInputEdit->setText(
        QString("%1%").arg(static_cast<int>(m_zoomFactor * 100)));

    // 视图模式
    if (m_viewMode.isEmpty()) {
        m_viewModeLabel->setText(tr("No mode"));
    } else {
        m_viewModeLabel->setText(tr("Mode: %1").arg(m_viewMode));
    }
}

// ============================================================================
// 私有槽函数
// ============================================================================

void StatusBar::onPageInputReturnPressed() {
    QString input = m_pageInputEdit->text().trimmed();
    if (validateAndJumpToPage(input)) {
        m_pageInputEdit->clear();
    }
}

void StatusBar::onZoomInputReturnPressed() {
    QString input = m_zoomInputEdit->text().trimmed();

    // Remove % sign if present
    input.remove('%');

    bool ok = false;
    double zoomLevel = input.toDouble(&ok);

    if (ok && zoomLevel >= 10.0 && zoomLevel <= 500.0) {
        emit zoomLevelChangeRequested(zoomLevel / 100.0);
        setLineEditInvalid(m_zoomInputEdit, false);
    } else {
        setLineEditInvalid(m_zoomInputEdit, true);
    }
}

void StatusBar::onMessageTimerTimeout() {
    if (m_messageLabel) {
        m_messageLabel->hide();
        m_messageLabel->clear();
    }
}

// ============================================================================
// 辅助方法
// ============================================================================

void StatusBar::displayTransientMessage(const QString& text, int timeout,
                                        const QColor& background,
                                        const QColor& foreground) {
    if (!m_messageLabel) {
        return;
    }

    m_messageTimer->stop();

    updateMessageAppearance(background, foreground);
    m_messageLabel->setText(text);
    m_messageLabel->adjustSize();

    const int x = (width() - m_messageLabel->width()) / 2;
    const int y = height() - m_messageLabel->height() - 10;
    m_messageLabel->move(std::max(0, x), std::max(0, y));
    m_messageLabel->raise();
    m_messageLabel->setWindowOpacity(1.0);
    m_messageLabel->show();

    if (timeout > 0) {
        m_messageTimer->start(timeout);
    }
}

void StatusBar::updateMessageAppearance(const QColor& background,
                                        const QColor& text) {
    if (!m_messageLabel) {
        return;
    }

    QString styleSheet = QString(
                             "QLabel {"
                             "  background-color: %1;"
                             "  color: %2;"
                             "  border-radius: 4px;"
                             "  padding: 8px 16px;"
                             "  font-weight: 500;"
                             "}")
                             .arg(background.name())
                             .arg(text.name());

    m_messageLabel->setStyleSheet(styleSheet);
}

bool StatusBar::validateAndJumpToPage(const QString& input) {
    if (input.isEmpty() || (m_totalPages == 0 && m_pageMaxRange == 0)) {
        return false;
    }

    bool ok = false;
    int pageNumber = input.toInt(&ok);

    int minAllowed = qMax(1, m_pageMinRange);
    int maxAllowed = (m_pageMaxRange > 0) ? m_pageMaxRange : m_totalPages;

    if (ok && pageNumber >= minAllowed && pageNumber <= maxAllowed) {
        emit pageJumpRequested(pageNumber);
        setLineEditInvalid(m_pageInputEdit, false);
        return true;
    } else {
        setLineEditInvalid(m_pageInputEdit, true);
        return false;
    }
}

void StatusBar::setLineEditInvalid(ElaLineEdit* edit, bool invalid) {
    if (!edit) {
        return;
    }

    if (invalid) {
        edit->setStyleSheet(
            "QLineEdit { border: 1px solid #dc3545; background-color: #fff5f5; "
            "}");
    } else {
        edit->setStyleSheet("");
    }
}
