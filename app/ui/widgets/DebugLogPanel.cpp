#include "DebugLogPanel.h"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QHideEvent>
#include <QIODevice>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMutexLocker>
#include <QOverload>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QShowEvent>
#include <QSizePolicy>
#include <QSplitter>
#include <QStandardPaths>
#include <QStringConverter>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/Qt>
#include "../../managers/StyleManager.h"

const QString DebugLogPanel::SETTINGS_GROUP = "DebugLogPanel";
const int DebugLogPanel::DEFAULT_MAX_ENTRIES = 10000;
const int DebugLogPanel::UPDATE_INTERVAL_MS = 100;
const int DebugLogPanel::STATISTICS_UPDATE_INTERVAL_MS = 1000;

DebugLogPanel::DebugLogPanel(QWidget* parent)
    : QWidget(parent),
      m_mainLayout(nullptr),
      m_mainSplitter(nullptr),
      m_logDisplay(nullptr),
      m_scrollBar(nullptr),
      m_filterGroup(nullptr),
      m_logLevelFilter(nullptr),
      m_categoryFilter(nullptr),
      m_searchEdit(nullptr),
      m_searchNextBtn(nullptr),
      m_searchPrevBtn(nullptr),
      m_caseSensitiveCheck(nullptr),
      m_regexCheck(nullptr),
      m_actionLayout(nullptr),
      m_clearBtn(nullptr),
      m_exportBtn(nullptr),
      m_copyBtn(nullptr),
      m_pauseBtn(nullptr),
      m_settingsBtn(nullptr),
      m_autoScrollCheck(nullptr),
      m_statsGroup(nullptr),
      m_statsTable(nullptr),
      m_messagesPerSecLabel(nullptr),
      m_memoryUsageBar(nullptr),
      m_contextMenu(nullptr),
      m_copyAction(nullptr),
      m_copyAllAction(nullptr),
      m_clearAction(nullptr),
      m_exportAction(nullptr),
      m_pauseAction(nullptr),
      m_updateTimer(nullptr),
      m_statisticsTimer(nullptr),
      m_paused(false),
      m_autoScroll(true),
      m_currentSearchIndex(-1),
      m_lastUpdateTime(QDateTime::currentDateTime()),
      m_pendingMessages(0),
      m_settings(nullptr) {
    setupUI();
    connectSignals();
    loadConfiguration();
    applyConfiguration();

    // Initialize timers
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(UPDATE_INTERVAL_MS);
    connect(m_updateTimer, &QTimer::timeout, this,
            &DebugLogPanel::onUpdateTimer);
    m_updateTimer->start();

    m_statisticsTimer = new QTimer(this);
    m_statisticsTimer->setInterval(STATISTICS_UPDATE_INTERVAL_MS);
    connect(m_statisticsTimer, &QTimer::timeout, this,
            &DebugLogPanel::updateStatistics);
    m_statisticsTimer->start();

    // Initialize settings
    m_settings = new QSettings(this);

    // Connect to logging manager
    connect(&LoggingManager::instance(), &LoggingManager::statisticsUpdated,
            this, &DebugLogPanel::updateStatisticsDisplay);

    // Connect to real-time log message streaming (using Qt::QueuedConnection
    // for thread safety)
    connect(&LoggingManager::instance(), &LoggingManager::logMessageReceived,
            this, &DebugLogPanel::onLogMessageDetailed, Qt::QueuedConnection);

    // Connect to theme changes
    connect(&STYLE, &StyleManager::themeChanged, this, [this](Theme theme) {
        Q_UNUSED(theme)
        applyTheme();
    });

    // Apply initial theme
    applyTheme();
}

DebugLogPanel::~DebugLogPanel() { saveConfiguration(); }

void DebugLogPanel::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSpacing(5);

    // Create main splitter for resizable sections
    m_mainSplitter = new QSplitter(Qt::Vertical, this);
    m_mainLayout->addWidget(m_mainSplitter);

    setupLogDisplay();
    setupFilterControls();
    setupActionButtons();
    setupStatisticsDisplay();
    setupContextMenu();
}

void DebugLogPanel::setupLogDisplay() {
    // Create log display widget
    QWidget* logWidget = new QWidget();
    QVBoxLayout* logLayout = new QVBoxLayout(logWidget);
    logLayout->setContentsMargins(0, 0, 0, 0);

    // Log display text edit
    m_logDisplay = new QTextEdit();
    m_logDisplay->setReadOnly(true);
    m_logDisplay->setFont(QFont("Consolas", 9));
    m_logDisplay->setLineWrapMode(QTextEdit::WidgetWidth);
    m_logDisplay->setContextMenuPolicy(Qt::CustomContextMenu);
    m_logDisplay->document()->setMaximumBlockCount(DEFAULT_MAX_ENTRIES);

    // Set minimum size
    m_logDisplay->setMinimumHeight(200);
    m_logDisplay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    logLayout->addWidget(m_logDisplay);
    m_mainSplitter->addWidget(logWidget);

    // Set initial splitter sizes
    m_mainSplitter->setSizes({400, 150});
}

void DebugLogPanel::setupFilterControls() {
    m_filterGroup = new QGroupBox("Filters", this);
    QGridLayout* filterLayout = new QGridLayout(m_filterGroup);

    // Log level filter
    filterLayout->addWidget(new QLabel("Level:"), 0, 0);
    m_logLevelFilter = new QComboBox();
    m_logLevelFilter->addItem("All", static_cast<int>(Logger::LogLevel::Trace));
    m_logLevelFilter->addItem("Debug+",
                              static_cast<int>(Logger::LogLevel::Debug));
    m_logLevelFilter->addItem("Info+",
                              static_cast<int>(Logger::LogLevel::Info));
    m_logLevelFilter->addItem("Warning+",
                              static_cast<int>(Logger::LogLevel::Warning));
    m_logLevelFilter->addItem("Error+",
                              static_cast<int>(Logger::LogLevel::Error));
    m_logLevelFilter->addItem("Critical",
                              static_cast<int>(Logger::LogLevel::Critical));
    m_logLevelFilter->setCurrentIndex(1);  // Default to Debug+
    filterLayout->addWidget(m_logLevelFilter, 0, 1);

    // Category filter
    filterLayout->addWidget(new QLabel("Category:"), 0, 2);
    m_categoryFilter = new QComboBox();
    m_categoryFilter->addItem("All Categories");
    m_categoryFilter->setEditable(true);
    filterLayout->addWidget(m_categoryFilter, 0, 3);

    // Search controls
    filterLayout->addWidget(new QLabel("Search:"), 1, 0);
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Search log messages...");
    filterLayout->addWidget(m_searchEdit, 1, 1, 1, 2);

    // Search buttons
    QHBoxLayout* searchBtnLayout = new QHBoxLayout();
    m_searchNextBtn = new QPushButton("Next");
    m_searchNextBtn->setMaximumWidth(60);
    m_searchPrevBtn = new QPushButton("Prev");
    m_searchPrevBtn->setMaximumWidth(60);
    searchBtnLayout->addWidget(m_searchPrevBtn);
    searchBtnLayout->addWidget(m_searchNextBtn);
    searchBtnLayout->addStretch();
    filterLayout->addLayout(searchBtnLayout, 1, 3);

    // Search options
    QHBoxLayout* searchOptionsLayout = new QHBoxLayout();
    m_caseSensitiveCheck = new QCheckBox("Case sensitive");
    m_regexCheck = new QCheckBox("Regex");
    searchOptionsLayout->addWidget(m_caseSensitiveCheck);
    searchOptionsLayout->addWidget(m_regexCheck);
    searchOptionsLayout->addStretch();
    filterLayout->addLayout(searchOptionsLayout, 2, 0, 1, 4);

    m_mainSplitter->addWidget(m_filterGroup);
}

void DebugLogPanel::setupActionButtons() {
    QWidget* actionWidget = new QWidget();
    m_actionLayout = new QHBoxLayout(actionWidget);
    m_actionLayout->setContentsMargins(0, 5, 0, 5);

    // Control buttons
    m_pauseBtn = new QPushButton("Pause");
    m_pauseBtn->setCheckable(true);
    m_pauseBtn->setMaximumWidth(80);

    m_clearBtn = new QPushButton("Clear");
    m_clearBtn->setMaximumWidth(80);

    m_exportBtn = new QPushButton("Export");
    m_exportBtn->setMaximumWidth(80);

    m_copyBtn = new QPushButton("Copy");
    m_copyBtn->setMaximumWidth(80);

    m_settingsBtn = new QPushButton("Settings");
    m_settingsBtn->setMaximumWidth(80);

    // Auto-scroll checkbox
    m_autoScrollCheck = new QCheckBox("Auto-scroll");
    m_autoScrollCheck->setChecked(true);

    // Add to layout
    m_actionLayout->addWidget(m_pauseBtn);
    m_actionLayout->addWidget(m_clearBtn);
    m_actionLayout->addWidget(m_exportBtn);
    m_actionLayout->addWidget(m_copyBtn);
    m_actionLayout->addWidget(m_settingsBtn);
    m_actionLayout->addStretch();
    m_actionLayout->addWidget(m_autoScrollCheck);

    m_mainLayout->addWidget(actionWidget);
}

void DebugLogPanel::setupStatisticsDisplay() {
    m_statsGroup = new QGroupBox("Statistics", this);
    QVBoxLayout* statsLayout = new QVBoxLayout(m_statsGroup);

    // Statistics table
    m_statsTable = new QTableWidget(6, 2);
    m_statsTable->setHorizontalHeaderLabels({"Metric", "Value"});
    m_statsTable->setVerticalHeaderLabels(
        {"Total", "Debug", "Info", "Warning", "Error", "Critical"});
    m_statsTable->horizontalHeader()->setStretchLastSection(true);
    m_statsTable->setMaximumHeight(150);
    m_statsTable->setAlternatingRowColors(true);

    // Initialize statistics display
    for (int i = 0; i < 6; ++i) {
        m_statsTable->setItem(i, 0, new QTableWidgetItem("0"));
        m_statsTable->setItem(i, 1, new QTableWidgetItem("0%"));
    }

    statsLayout->addWidget(m_statsTable);

    // Messages per second label
    m_messagesPerSecLabel = new QLabel("Messages/sec: 0.0");
    statsLayout->addWidget(m_messagesPerSecLabel);

    // Memory usage bar
    QHBoxLayout* memoryLayout = new QHBoxLayout();
    memoryLayout->addWidget(new QLabel("Memory:"));
    m_memoryUsageBar = new QProgressBar();
    m_memoryUsageBar->setMaximum(100);
    m_memoryUsageBar->setValue(0);
    memoryLayout->addWidget(m_memoryUsageBar);
    statsLayout->addLayout(memoryLayout);

    m_mainSplitter->addWidget(m_statsGroup);
}

void DebugLogPanel::setupContextMenu() {
    m_contextMenu = new QMenu(this);

    m_copyAction = m_contextMenu->addAction("Copy Selected");
    m_copyAllAction = m_contextMenu->addAction("Copy All");
    m_contextMenu->addSeparator();
    m_clearAction = m_contextMenu->addAction("Clear Logs");
    m_exportAction = m_contextMenu->addAction("Export Logs...");
    m_contextMenu->addSeparator();
    m_pauseAction = m_contextMenu->addAction("Pause Logging");
    m_pauseAction->setCheckable(true);
}

void DebugLogPanel::connectSignals() {
    // Filter controls
    connect(m_logLevelFilter,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DebugLogPanel::onLogLevelFilterChanged);
    connect(m_categoryFilter,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DebugLogPanel::onCategoryFilterChanged);
    connect(m_searchEdit, &QLineEdit::textChanged, this,
            &DebugLogPanel::onSearchTextChanged);
    connect(m_searchNextBtn, &QPushButton::clicked, this,
            &DebugLogPanel::onSearchNext);
    connect(m_searchPrevBtn, &QPushButton::clicked, this,
            &DebugLogPanel::onSearchPrevious);
    connect(m_caseSensitiveCheck, &QCheckBox::toggled, this,
            &DebugLogPanel::onFilterChanged);
    connect(m_regexCheck, &QCheckBox::toggled, this,
            &DebugLogPanel::onFilterChanged);

    // Action buttons
    connect(m_pauseBtn, &QPushButton::toggled, this,
            &DebugLogPanel::onPauseToggled);
    connect(m_clearBtn, &QPushButton::clicked, this,
            &DebugLogPanel::onClearLogs);
    connect(m_exportBtn, &QPushButton::clicked, this,
            &DebugLogPanel::onExportLogs);
    connect(m_copyBtn, &QPushButton::clicked, this,
            &DebugLogPanel::onCopySelected);
    connect(m_settingsBtn, &QPushButton::clicked, this,
            &DebugLogPanel::onShowSettingsDialog);
    connect(m_autoScrollCheck, &QCheckBox::toggled, this,
            &DebugLogPanel::onAutoScrollToggled);

    // Context menu
    connect(m_logDisplay, &QTextEdit::customContextMenuRequested, this,
            &DebugLogPanel::onContextMenuRequested);
    connect(m_copyAction, &QAction::triggered, this,
            &DebugLogPanel::onCopySelected);
    connect(m_copyAllAction, &QAction::triggered, this,
            &DebugLogPanel::onCopyAll);
    connect(m_clearAction, &QAction::triggered, this,
            &DebugLogPanel::onClearLogs);
    connect(m_exportAction, &QAction::triggered, this,
            &DebugLogPanel::onExportLogs);
    connect(m_pauseAction, &QAction::toggled, this,
            &DebugLogPanel::onPauseToggled);
}

void DebugLogPanel::setConfiguration(const PanelConfiguration& config) {
    QMutexLocker locker(&m_logMutex);
    m_config = config;
    applyConfiguration();
    emit configurationChanged();
}

void DebugLogPanel::saveConfiguration() {
    if (!m_settings)
        return;

    m_settings->beginGroup(SETTINGS_GROUP);
    m_settings->setValue("maxLogEntries", m_config.maxLogEntries);
    m_settings->setValue("autoScroll", m_config.autoScroll);
    m_settings->setValue("showTimestamp", m_config.showTimestamp);
    m_settings->setValue("showLevel", m_config.showLevel);
    m_settings->setValue("showCategory", m_config.showCategory);
    m_settings->setValue("showThreadId", m_config.showThreadId);
    m_settings->setValue("showSourceLocation", m_config.showSourceLocation);
    m_settings->setValue("wordWrap", m_config.wordWrap);
    m_settings->setValue("colorizeOutput", m_config.colorizeOutput);
    m_settings->setValue("timestampFormat", m_config.timestampFormat);
    m_settings->setValue("logFont", m_config.logFont.toString());
    m_settings->setValue("minLogLevel", static_cast<int>(m_config.minLogLevel));
    m_settings->setValue("enabledCategories", m_config.enabledCategories);
    m_settings->setValue("searchFilter", m_config.searchFilter);
    m_settings->setValue("caseSensitiveSearch", m_config.caseSensitiveSearch);
    m_settings->setValue("regexSearch", m_config.regexSearch);
    m_settings->setValue("updateIntervalMs", m_config.updateIntervalMs);
    m_settings->setValue("batchSize", m_config.batchSize);
    m_settings->setValue("pauseOnHighFrequency", m_config.pauseOnHighFrequency);
    m_settings->setValue("highFrequencyThreshold",
                         m_config.highFrequencyThreshold);
    m_settings->endGroup();
}

void DebugLogPanel::loadConfiguration() {
    if (!m_settings)
        return;

    m_settings->beginGroup(SETTINGS_GROUP);
    m_config.maxLogEntries =
        m_settings->value("maxLogEntries", DEFAULT_MAX_ENTRIES).toInt();
    m_config.autoScroll = m_settings->value("autoScroll", true).toBool();
    m_config.showTimestamp = m_settings->value("showTimestamp", true).toBool();
    m_config.showLevel = m_settings->value("showLevel", true).toBool();
    m_config.showCategory = m_settings->value("showCategory", true).toBool();
    m_config.showThreadId = m_settings->value("showThreadId", false).toBool();
    m_config.showSourceLocation =
        m_settings->value("showSourceLocation", false).toBool();
    m_config.wordWrap = m_settings->value("wordWrap", true).toBool();
    m_config.colorizeOutput =
        m_settings->value("colorizeOutput", true).toBool();
    m_config.timestampFormat =
        m_settings->value("timestampFormat", "hh:mm:ss.zzz").toString();

    QString fontString =
        m_settings->value("logFont", QFont("Consolas", 9).toString())
            .toString();
    m_config.logFont.fromString(fontString);

    m_config.minLogLevel = static_cast<Logger::LogLevel>(
        m_settings
            ->value("minLogLevel", static_cast<int>(Logger::LogLevel::Debug))
            .toInt());
    m_config.enabledCategories =
        m_settings->value("enabledCategories", QStringList()).toStringList();
    m_config.searchFilter = m_settings->value("searchFilter", "").toString();
    m_config.caseSensitiveSearch =
        m_settings->value("caseSensitiveSearch", false).toBool();
    m_config.regexSearch = m_settings->value("regexSearch", false).toBool();
    m_config.updateIntervalMs =
        m_settings->value("updateIntervalMs", UPDATE_INTERVAL_MS).toInt();
    m_config.batchSize = m_settings->value("batchSize", 50).toInt();
    m_config.pauseOnHighFrequency =
        m_settings->value("pauseOnHighFrequency", true).toBool();
    m_config.highFrequencyThreshold =
        m_settings->value("highFrequencyThreshold", 1000).toInt();
    m_settings->endGroup();
}

void DebugLogPanel::resetToDefaults() {
    m_config = PanelConfiguration();
    applyConfiguration();
    emit configurationChanged();
}

void DebugLogPanel::applyConfiguration() {
    if (!m_logDisplay)
        return;

    // Apply font and display settings
    m_logDisplay->setFont(m_config.logFont);
    m_logDisplay->setLineWrapMode(m_config.wordWrap ? QTextEdit::WidgetWidth
                                                    : QTextEdit::NoWrap);
    m_logDisplay->document()->setMaximumBlockCount(m_config.maxLogEntries);

    // Update UI controls
    if (m_autoScrollCheck) {
        m_autoScrollCheck->setChecked(m_config.autoScroll);
        m_autoScroll = m_config.autoScroll;
    }

    if (m_searchEdit) {
        m_searchEdit->setText(m_config.searchFilter);
    }

    if (m_caseSensitiveCheck) {
        m_caseSensitiveCheck->setChecked(m_config.caseSensitiveSearch);
    }

    if (m_regexCheck) {
        m_regexCheck->setChecked(m_config.regexSearch);
    }

    if (m_logLevelFilter) {
        int levelIndex = static_cast<int>(m_config.minLogLevel);
        if (levelIndex >= 0 && levelIndex < m_logLevelFilter->count()) {
            m_logLevelFilter->setCurrentIndex(levelIndex);
        }
    }

    // Update timers
    if (m_updateTimer) {
        m_updateTimer->setInterval(m_config.updateIntervalMs);
    }
}

void DebugLogPanel::clearLogs() {
    QMutexLocker locker(&m_logMutex);
    m_logEntries.clear();
    m_filteredEntries.clear();
    m_pendingEntries.clear();
    m_pendingMessages = 0;

    if (m_logDisplay) {
        m_logDisplay->clear();
    }

    resetStatistics();
    updateStatisticsDisplay();
}

void DebugLogPanel::pauseLogging(bool pause) {
    m_paused = pause;
    if (m_pauseBtn) {
        m_pauseBtn->setChecked(pause);
        m_pauseBtn->setText(pause ? "Resume" : "Pause");
    }
    if (m_pauseAction) {
        m_pauseAction->setChecked(pause);
        m_pauseAction->setText(pause ? "Resume Logging" : "Pause Logging");
    }
}

DebugLogPanel::LogStatistics DebugLogPanel::getStatistics() const {
    QMutexLocker locker(&m_logMutex);
    return m_statistics;
}

void DebugLogPanel::resetStatistics() {
    QMutexLocker locker(&m_logMutex);
    m_statistics = LogStatistics();
    m_statistics.firstLogTime = QDateTime::currentDateTime();
    m_statistics.lastLogTime = QDateTime::currentDateTime();
}

// Slot implementations
void DebugLogPanel::onLogMessage(const QString& message, int level) {
    QDateTime timestamp = QDateTime::currentDateTime();
    QString category = "general";
    onLogMessageDetailed(timestamp, level, category, message);
}

void DebugLogPanel::onLogMessageDetailed(const QDateTime& timestamp, int level,
                                         const QString& category,
                                         const QString& message,
                                         const QString& threadId,
                                         const QString& sourceLocation) {
    if (m_paused)
        return;

    LogEntry entry(timestamp, static_cast<Logger::LogLevel>(level), category,
                   message, threadId, sourceLocation);

    QMutexLocker locker(&m_logMutex);
    m_pendingEntries.push_back(entry);
    m_pendingMessages++;

    // Update category filter if new category is encountered
    if (!category.isEmpty() && m_categoryFilter) {
        bool categoryExists = false;
        for (int i = 0; i < m_categoryFilter->count(); ++i) {
            if (m_categoryFilter->itemText(i) == category) {
                categoryExists = true;
                break;
            }
        }
        if (!categoryExists) {
            m_categoryFilter->addItem(category);
        }
    }

    // Update statistics
    m_statistics.totalMessages++;
    m_statistics.lastLogTime = timestamp;
    if (m_statistics.firstLogTime.isNull()) {
        m_statistics.firstLogTime = timestamp;
    }

    switch (static_cast<Logger::LogLevel>(level)) {
        case Logger::LogLevel::Debug:
            m_statistics.debugMessages++;
            break;
        case Logger::LogLevel::Info:
            m_statistics.infoMessages++;
            break;
        case Logger::LogLevel::Warning:
            m_statistics.warningMessages++;
            break;
        case Logger::LogLevel::Error:
            m_statistics.errorMessages++;
            break;
        case Logger::LogLevel::Critical:
            m_statistics.criticalMessages++;
            break;
        default:
            break;
    }
}

void DebugLogPanel::showPanel() {
    setVisible(true);
    emit panelVisibilityChanged(true);
}

void DebugLogPanel::hidePanel() {
    setVisible(false);
    emit panelVisibilityChanged(false);
}

void DebugLogPanel::togglePanel() {
    setVisible(!isVisible());
    emit panelVisibilityChanged(isVisible());
}

void DebugLogPanel::onFilterChanged() {
    filterLogEntries();
    highlightSearchResults();
}

void DebugLogPanel::onSearchTextChanged() {
    m_config.searchFilter = m_searchEdit->text();
    m_currentSearchIndex = -1;
    highlightSearchResults();
}

void DebugLogPanel::onSearchNext() {
    if (m_searchResults.isEmpty())
        return;

    m_currentSearchIndex = (m_currentSearchIndex + 1) % m_searchResults.size();
    // Implementation for jumping to next search result would go here
}

void DebugLogPanel::onSearchPrevious() {
    if (m_searchResults.isEmpty())
        return;

    m_currentSearchIndex = (m_currentSearchIndex - 1 + m_searchResults.size()) %
                           m_searchResults.size();
    // Implementation for jumping to previous search result would go here
}

void DebugLogPanel::onClearLogs() { clearLogs(); }

void DebugLogPanel::onExportLogs() {
    QString fileName = QFileDialog::getSaveFileName(
        this, "Export Logs",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) +
            "/debug_logs.txt",
        "Text Files (*.txt);;All Files (*)");

    if (!fileName.isEmpty()) {
        exportToFile(fileName);
    }
}

void DebugLogPanel::onCopySelected() {
    if (m_logDisplay && m_logDisplay->textCursor().hasSelection()) {
        QApplication::clipboard()->setText(
            m_logDisplay->textCursor().selectedText());
    }
}

void DebugLogPanel::onCopyAll() {
    if (m_logDisplay) {
        QApplication::clipboard()->setText(m_logDisplay->toPlainText());
    }
}

void DebugLogPanel::onPauseToggled(bool paused) { pauseLogging(paused); }

void DebugLogPanel::onAutoScrollToggled(bool enabled) {
    m_config.autoScroll = enabled;
    m_autoScroll = enabled;
}

void DebugLogPanel::onConfigurationChanged() { applyConfiguration(); }

void DebugLogPanel::onUpdateTimer() { updateLogDisplay(); }

void DebugLogPanel::onLogLevelFilterChanged() {
    if (m_logLevelFilter) {
        int levelIndex = m_logLevelFilter->currentIndex();
        m_config.minLogLevel = static_cast<Logger::LogLevel>(levelIndex);
        onFilterChanged();
    }
}

void DebugLogPanel::onCategoryFilterChanged() {
    // Update enabled categories based on current selection
    QString selectedCategory = m_categoryFilter->currentText();
    if (selectedCategory == "All Categories") {
        m_config.enabledCategories
            .clear();  // Empty list means all categories enabled
    } else {
        m_config.enabledCategories.clear();
        m_config.enabledCategories.append(selectedCategory);
    }

    onFilterChanged();
}

void DebugLogPanel::onShowSettingsDialog() { showSettingsDialog(); }

void DebugLogPanel::onContextMenuRequested(const QPoint& pos) {
    if (m_contextMenu && m_logDisplay) {
        m_contextMenu->exec(m_logDisplay->mapToGlobal(pos));
    }
}

// Event handlers
void DebugLogPanel::contextMenuEvent(QContextMenuEvent* event) {
    if (m_contextMenu) {
        m_contextMenu->exec(event->globalPos());
    }
}

void DebugLogPanel::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    emit panelVisibilityChanged(true);
}

void DebugLogPanel::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    emit panelVisibilityChanged(false);
}

// Core functionality methods
void DebugLogPanel::updateLogDisplay() {
    if (!m_logDisplay || m_pendingEntries.empty())
        return;

    QMutexLocker locker(&m_logMutex);

    // Process pending entries in batches
    int processed = 0;
    while (!m_pendingEntries.empty() && processed < m_config.batchSize) {
        LogEntry entry = m_pendingEntries.front();
        m_pendingEntries.pop_front();

        // Add to main log entries
        m_logEntries.push_back(entry);

        // Maintain maximum entries limit
        if (m_logEntries.size() > static_cast<size_t>(m_config.maxLogEntries)) {
            m_logEntries.pop_front();
        }

        // Add to display if it passes filters
        if (passesFilter(entry)) {
            addLogEntryToDisplay(entry);
        }

        processed++;
        m_pendingMessages--;
    }

    // Auto-scroll if enabled
    if (m_autoScroll && processed > 0) {
        scrollToBottom();
    }
}

void DebugLogPanel::addLogEntryToDisplay(const LogEntry& entry) {
    if (!m_logDisplay)
        return;

    QString formattedEntry = formatLogEntry(entry);

    // Apply color formatting if enabled
    if (m_config.colorizeOutput) {
        QTextCursor cursor = m_logDisplay->textCursor();
        cursor.movePosition(QTextCursor::End);

        QTextCharFormat format;
        format.setForeground(getLogLevelColor(entry.level));

        cursor.insertText(formattedEntry + "\n", format);
    } else {
        m_logDisplay->append(formattedEntry);
    }
}

void DebugLogPanel::filterLogEntries() {
    if (!m_logDisplay)
        return;

    QMutexLocker locker(&m_logMutex);

    m_filteredEntries.clear();
    m_logDisplay->clear();

    for (const auto& entry : m_logEntries) {
        if (passesFilter(entry)) {
            m_filteredEntries.push_back(entry);
            addLogEntryToDisplay(entry);
        }
    }

    m_statistics.filteredMessages = static_cast<int>(m_filteredEntries.size());
}

void DebugLogPanel::highlightSearchResults() {
    if (!m_logDisplay || m_config.searchFilter.isEmpty())
        return;

    // Clear previous highlights
    QTextCursor cursor = m_logDisplay->textCursor();
    cursor.select(QTextCursor::Document);
    QTextCharFormat clearFormat;
    cursor.setCharFormat(clearFormat);

    // Apply new highlights
    QTextDocument* document = m_logDisplay->document();
    QTextCursor searchCursor(document);

    QTextCharFormat highlightFormat;
    highlightFormat.setBackground(
        QColor(255, 255, 0, 100));  // Yellow highlight

    Qt::CaseSensitivity caseSensitivity =
        m_config.caseSensitiveSearch ? Qt::CaseSensitive : Qt::CaseInsensitive;

    m_searchResults.clear();

    if (m_config.regexSearch) {
        QRegularExpression regex(m_config.searchFilter);
        if (!m_config.caseSensitiveSearch) {
            regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        }

        QRegularExpressionMatchIterator it =
            regex.globalMatch(document->toPlainText());
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            searchCursor.setPosition(match.capturedStart());
            searchCursor.setPosition(match.capturedEnd(),
                                     QTextCursor::KeepAnchor);
            searchCursor.setCharFormat(highlightFormat);
            m_searchResults.append(QString::number(match.capturedStart()));
        }
    } else {
        while (!searchCursor.isNull() && !searchCursor.atEnd()) {
            searchCursor =
                document->find(m_config.searchFilter, searchCursor,
                               QTextDocument::FindFlag(caseSensitivity));
            if (!searchCursor.isNull()) {
                searchCursor.setCharFormat(highlightFormat);
                m_searchResults.append(
                    QString::number(searchCursor.position()));
            }
        }
    }
}

void DebugLogPanel::updateStatistics() {
    QMutexLocker locker(&m_logMutex);

    // Calculate messages per second
    if (!m_statistics.firstLogTime.isNull() &&
        !m_statistics.lastLogTime.isNull()) {
        qint64 timeSpanMs =
            m_statistics.firstLogTime.msecsTo(m_statistics.lastLogTime);
        if (timeSpanMs > 0) {
            m_statistics.messagesPerSecond =
                (m_statistics.totalMessages * 1000.0) / timeSpanMs;
        }
    }

    emit logStatisticsUpdated(m_statistics);
}

void DebugLogPanel::updateStatisticsDisplay() {
    if (!m_statsTable || !m_messagesPerSecLabel)
        return;

    LogStatistics stats = getStatistics();

    // Update statistics table
    m_statsTable->item(0, 0)->setText(QString::number(stats.totalMessages));
    m_statsTable->item(1, 0)->setText(QString::number(stats.debugMessages));
    m_statsTable->item(2, 0)->setText(QString::number(stats.infoMessages));
    m_statsTable->item(3, 0)->setText(QString::number(stats.warningMessages));
    m_statsTable->item(4, 0)->setText(QString::number(stats.errorMessages));
    m_statsTable->item(5, 0)->setText(QString::number(stats.criticalMessages));

    // Update percentages
    if (stats.totalMessages > 0) {
        m_statsTable->item(1, 1)->setText(
            QString::number((stats.debugMessages * 100.0) / stats.totalMessages,
                            'f', 1) +
            "%");
        m_statsTable->item(2, 1)->setText(
            QString::number((stats.infoMessages * 100.0) / stats.totalMessages,
                            'f', 1) +
            "%");
        m_statsTable->item(3, 1)->setText(
            QString::number(
                (stats.warningMessages * 100.0) / stats.totalMessages, 'f', 1) +
            "%");
        m_statsTable->item(4, 1)->setText(
            QString::number((stats.errorMessages * 100.0) / stats.totalMessages,
                            'f', 1) +
            "%");
        m_statsTable->item(5, 1)->setText(
            QString::number(
                (stats.criticalMessages * 100.0) / stats.totalMessages, 'f',
                1) +
            "%");
    }

    // Update messages per second
    m_messagesPerSecLabel->setText(
        QString("Messages/sec: %1").arg(stats.messagesPerSecond, 0, 'f', 2));

    // Update memory usage (simplified calculation)
    int memoryUsagePercent = qMin(
        100,
        static_cast<int>((m_logEntries.size() * 100) / m_config.maxLogEntries));
    if (m_memoryUsageBar) {
        m_memoryUsageBar->setValue(memoryUsagePercent);
    }
}

// Utility methods
QString DebugLogPanel::formatLogEntry(const LogEntry& entry) const {
    QStringList parts;

    if (m_config.showTimestamp) {
        parts << entry.timestamp.toString(m_config.timestampFormat);
    }

    if (m_config.showLevel) {
        parts << QString("[%1]").arg(getLogLevelText(entry.level));
    }

    if (m_config.showCategory && !entry.category.isEmpty()) {
        parts << QString("[%1]").arg(entry.category);
    }

    if (m_config.showThreadId && !entry.threadId.isEmpty()) {
        parts << QString("[Thread:%1]").arg(entry.threadId);
    }

    if (m_config.showSourceLocation && !entry.sourceLocation.isEmpty()) {
        parts << QString("[%1]").arg(entry.sourceLocation);
    }

    parts << entry.message;

    return parts.join(" ");
}

QColor DebugLogPanel::getLogLevelColor(Logger::LogLevel level) const {
    switch (level) {
        case Logger::LogLevel::Trace:
            return QColor(128, 128, 128);  // Gray
        case Logger::LogLevel::Debug:
            return QColor(0, 128, 255);  // Blue
        case Logger::LogLevel::Info:
            return QColor(0, 0, 0);  // Black
        case Logger::LogLevel::Warning:
            return QColor(255, 165, 0);  // Orange
        case Logger::LogLevel::Error:
            return QColor(255, 0, 0);  // Red
        case Logger::LogLevel::Critical:
            return QColor(128, 0, 128);  // Purple
        default:
            return QColor(0, 0, 0);  // Black
    }
}

QString DebugLogPanel::getLogLevelText(Logger::LogLevel level) const {
    switch (level) {
        case Logger::LogLevel::Trace:
            return "TRACE";
        case Logger::LogLevel::Debug:
            return "DEBUG";
        case Logger::LogLevel::Info:
            return "INFO";
        case Logger::LogLevel::Warning:
            return "WARN";
        case Logger::LogLevel::Error:
            return "ERROR";
        case Logger::LogLevel::Critical:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

bool DebugLogPanel::passesFilter(const LogEntry& entry) const {
    // Check log level filter
    if (entry.level < m_config.minLogLevel) {
        return false;
    }

    // Check category filter
    if (!m_config.enabledCategories.isEmpty() &&
        !m_config.enabledCategories.contains(entry.category)) {
        return false;
    }

    // Check search filter
    if (!m_config.searchFilter.isEmpty()) {
        Qt::CaseSensitivity caseSensitivity = m_config.caseSensitiveSearch
                                                  ? Qt::CaseSensitive
                                                  : Qt::CaseInsensitive;

        if (m_config.regexSearch) {
            QRegularExpression regex(m_config.searchFilter);
            if (!m_config.caseSensitiveSearch) {
                regex.setPatternOptions(
                    QRegularExpression::CaseInsensitiveOption);
            }

            QString fullText = formatLogEntry(entry);
            if (!regex.match(fullText).hasMatch()) {
                return false;
            }
        } else {
            QString fullText = formatLogEntry(entry);
            if (!fullText.contains(m_config.searchFilter, caseSensitivity)) {
                return false;
            }
        }
    }

    return true;
}

void DebugLogPanel::scrollToBottom() {
    if (m_logDisplay) {
        QScrollBar* scrollBar = m_logDisplay->verticalScrollBar();
        if (scrollBar) {
            scrollBar->setValue(scrollBar->maximum());
        }
    }
}

void DebugLogPanel::exportToFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(
            this, "Export Error",
            QString("Could not open file for writing: %1").arg(filePath));
        return;
    }

    QTextStream out(&file);
    // Note: setEncoding is Qt6 specific, for Qt5 compatibility we'll skip
    // encoding setting

    // Write header
    out << "Debug Log Export\n";
    out << "Generated: " << QDateTime::currentDateTime().toString() << "\n";
    out << "Total Entries: " << m_filteredEntries.size() << "\n";
    out << QString(80, '=') << "\n\n";

    // Write log entries
    QMutexLocker locker(&m_logMutex);
    for (const auto& entry : m_filteredEntries) {
        out << formatLogEntry(entry) << "\n";
    }

    file.close();

    QMessageBox::information(
        this, "Export Complete",
        QString("Log exported successfully to: %1").arg(filePath));
}

void DebugLogPanel::showSettingsDialog() {
    // This would open a settings dialog for configuring the debug panel
    // For now, just show a placeholder message
    QMessageBox::information(
        this, "Settings",
        "Debug panel settings dialog would be implemented here.");
}

void DebugLogPanel::applyTheme() {
    // Apply theme-based styling to the debug panel
    Theme currentTheme = STYLE.currentTheme();

    QString backgroundColor, textColor, borderColor, buttonColor,
        highlightColor;

    if (currentTheme == Theme::Dark) {
        backgroundColor = "#2b2b2b";
        textColor = "#ffffff";
        borderColor = "#555555";
        buttonColor = "#404040";
        highlightColor = "#0078d4";
    } else {
        backgroundColor = "#ffffff";
        textColor = "#000000";
        borderColor = "#cccccc";
        buttonColor = "#f0f0f0";
        highlightColor = "#0078d4";
    }

    // Apply styles to main components
    if (m_logDisplay) {
        m_logDisplay->setStyleSheet(
            QString("QTextEdit {"
                    "    background-color: %1;"
                    "    color: %2;"
                    "    border: 1px solid %3;"
                    "    font-family: 'Consolas', 'Monaco', monospace;"
                    "    selection-background-color: %4;"
                    "}")
                .arg(backgroundColor, textColor, borderColor, highlightColor));
    }

    // Apply styles to filter group
    if (m_filterGroup) {
        m_filterGroup->setStyleSheet(
            QString("QGroupBox {"
                    "    font-weight: bold;"
                    "    border: 1px solid %1;"
                    "    border-radius: 3px;"
                    "    margin-top: 5px;"
                    "    background-color: %2;"
                    "    color: %3;"
                    "}"
                    "QGroupBox::title {"
                    "    subcontrol-origin: margin;"
                    "    left: 10px;"
                    "    padding: 0 5px 0 5px;"
                    "}")
                .arg(borderColor, backgroundColor, textColor));
    }

    // Apply styles to statistics group
    if (m_statsGroup) {
        m_statsGroup->setStyleSheet(
            QString("QGroupBox {"
                    "    font-weight: bold;"
                    "    border: 1px solid %1;"
                    "    border-radius: 3px;"
                    "    margin-top: 5px;"
                    "    background-color: %2;"
                    "    color: %3;"
                    "}"
                    "QGroupBox::title {"
                    "    subcontrol-origin: margin;"
                    "    left: 10px;"
                    "    padding: 0 5px 0 5px;"
                    "}")
                .arg(borderColor, backgroundColor, textColor));
    }

    // Apply styles to buttons
    QString buttonStyle =
        QString(
            "QPushButton {"
            "    background-color: %1;"
            "    color: %2;"
            "    border: 1px solid %3;"
            "    border-radius: 3px;"
            "    padding: 5px 10px;"
            "    min-width: 60px;"
            "}"
            "QPushButton:hover {"
            "    background-color: %4;"
            "}"
            "QPushButton:pressed {"
            "    background-color: %3;"
            "}"
            "QPushButton:checked {"
            "    background-color: %4;"
            "    font-weight: bold;"
            "}")
            .arg(buttonColor, textColor, borderColor, highlightColor);

    if (m_clearBtn)
        m_clearBtn->setStyleSheet(buttonStyle);
    if (m_exportBtn)
        m_exportBtn->setStyleSheet(buttonStyle);
    if (m_copyBtn)
        m_copyBtn->setStyleSheet(buttonStyle);
    if (m_pauseBtn)
        m_pauseBtn->setStyleSheet(buttonStyle);
    if (m_settingsBtn)
        m_settingsBtn->setStyleSheet(buttonStyle);
    if (m_searchNextBtn)
        m_searchNextBtn->setStyleSheet(buttonStyle);
    if (m_searchPrevBtn)
        m_searchPrevBtn->setStyleSheet(buttonStyle);

    // Apply styles to combo boxes and line edits
    QString inputStyle =
        QString(
            "QComboBox, QLineEdit {"
            "    background-color: %1;"
            "    color: %2;"
            "    border: 1px solid %3;"
            "    border-radius: 3px;"
            "    padding: 3px;"
            "}"
            "QComboBox:hover, QLineEdit:hover {"
            "    border-color: %4;"
            "}"
            "QComboBox:focus, QLineEdit:focus {"
            "    border-color: %4;"
            "    outline: none;"
            "}")
            .arg(backgroundColor, textColor, borderColor, highlightColor);

    if (m_logLevelFilter)
        m_logLevelFilter->setStyleSheet(inputStyle);
    if (m_categoryFilter)
        m_categoryFilter->setStyleSheet(inputStyle);
    if (m_searchEdit)
        m_searchEdit->setStyleSheet(inputStyle);

    // Apply styles to checkboxes
    QString checkboxStyle =
        QString(
            "QCheckBox {"
            "    color: %1;"
            "}"
            "QCheckBox::indicator {"
            "    width: 16px;"
            "    height: 16px;"
            "}"
            "QCheckBox::indicator:unchecked {"
            "    border: 1px solid %2;"
            "    background-color: %3;"
            "}"
            "QCheckBox::indicator:checked {"
            "    border: 1px solid %4;"
            "    background-color: %4;"
            "}")
            .arg(textColor, borderColor, backgroundColor, highlightColor);

    if (m_caseSensitiveCheck)
        m_caseSensitiveCheck->setStyleSheet(checkboxStyle);
    if (m_regexCheck)
        m_regexCheck->setStyleSheet(checkboxStyle);
    if (m_autoScrollCheck)
        m_autoScrollCheck->setStyleSheet(checkboxStyle);

    // Apply styles to statistics table
    if (m_statsTable) {
        m_statsTable->setStyleSheet(
            QString("QTableWidget {"
                    "    background-color: %1;"
                    "    color: %2;"
                    "    border: 1px solid %3;"
                    "    gridline-color: %3;"
                    "    selection-background-color: %4;"
                    "}"
                    "QTableWidget::item {"
                    "    padding: 3px;"
                    "}"
                    "QHeaderView::section {"
                    "    background-color: %5;"
                    "    color: %2;"
                    "    border: 1px solid %3;"
                    "    padding: 3px;"
                    "}")
                .arg(backgroundColor, textColor, borderColor, highlightColor,
                     buttonColor));
    }

    // Apply styles to progress bar
    if (m_memoryUsageBar) {
        m_memoryUsageBar->setStyleSheet(
            QString("QProgressBar {"
                    "    border: 1px solid %1;"
                    "    border-radius: 3px;"
                    "    background-color: %2;"
                    "    color: %3;"
                    "    text-align: center;"
                    "}"
                    "QProgressBar::chunk {"
                    "    background-color: %4;"
                    "    border-radius: 2px;"
                    "}")
                .arg(borderColor, backgroundColor, textColor, highlightColor));
    }
}
